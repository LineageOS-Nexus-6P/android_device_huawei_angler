#define LOG_TAG "AnglerDsiCompat"

#include "DsiEventPolicy.h"

#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>

#include <cstdlib>

#include <log/log.h>

namespace {

using DsiHandle = void*;
using DsiEventPayload = void;
using DsiEventCallback = void (*)(DsiHandle, void*, int, DsiEventPayload*);
using RealGetDataServiceHandle = DsiHandle (*)(DsiEventCallback, void*);
using RealReleaseDataServiceHandle = void (*)(DsiHandle);
using RealStartDataCall = int (*)(DsiHandle);
using RealGetIpAddressCount = int (*)(DsiHandle);

constexpr useconds_t kAddressPollIntervalUs = 100000;
constexpr unsigned int kAddressPollAttempts = 300;

struct CallbackContext {
    DsiEventCallback callback;
    void* userData;
    pthread_mutex_t mutex;
    DsiHandle handle;
    bool connectedDelivered;
    bool completionScheduled;
    bool released;
    unsigned int refCount;
    int lastAddressCount;
};

struct HandleEntry {
    DsiHandle handle;
    CallbackContext* context;
    HandleEntry* next;
};

struct RealApi {
    void* library;
    RealGetDataServiceHandle getDataServiceHandle;
    RealReleaseDataServiceHandle releaseDataServiceHandle;
    RealStartDataCall startDataCall;
    RealGetIpAddressCount getIpAddressCount;
};

struct ConnectedPeerSnapshot {
    CallbackContext* context;
    int baselineAddressCount;
};

RealApi gRealApi{};
pthread_once_t gRealApiOnce = PTHREAD_ONCE_INIT;
pthread_mutex_t gHandleListMutex = PTHREAD_MUTEX_INITIALIZER;
HandleEntry* gHandleList = nullptr;

void loadRealApi() {
    gRealApi.library = dlopen("libdsi_netctrl.so", RTLD_NOW | RTLD_LOCAL);
    if (gRealApi.library == nullptr) {
        ALOGE("Unable to open stock libdsi_netctrl: %s", dlerror());
        return;
    }

    gRealApi.getDataServiceHandle = reinterpret_cast<RealGetDataServiceHandle>(
            dlsym(gRealApi.library, "dsi_get_data_srvc_hndl"));
    gRealApi.releaseDataServiceHandle = reinterpret_cast<RealReleaseDataServiceHandle>(
            dlsym(gRealApi.library, "dsi_rel_data_srvc_hndl"));
    gRealApi.startDataCall = reinterpret_cast<RealStartDataCall>(
            dlsym(gRealApi.library, "dsi_start_data_call"));
    gRealApi.getIpAddressCount = reinterpret_cast<RealGetIpAddressCount>(
            dlsym(gRealApi.library, "dsi_get_ip_addr_count"));

    if (gRealApi.getDataServiceHandle == nullptr
            || gRealApi.releaseDataServiceHandle == nullptr
            || gRealApi.startDataCall == nullptr
            || gRealApi.getIpAddressCount == nullptr) {
        ALOGE("Stock DSI entry points are incomplete: %s", dlerror());
    }
}

bool realApiReady() {
    pthread_once(&gRealApiOnce, loadRealApi);
    return gRealApi.getDataServiceHandle != nullptr
            && gRealApi.releaseDataServiceHandle != nullptr
            && gRealApi.startDataCall != nullptr
            && gRealApi.getIpAddressCount != nullptr;
}

void destroyReleasedContext(CallbackContext* context) {
    gRealApi.releaseDataServiceHandle(context->handle);
    pthread_mutex_destroy(&context->mutex);
    std::free(context);
}

void dropContextRef(CallbackContext* context) {
    bool destroy = false;
    pthread_mutex_lock(&context->mutex);
    if (context->refCount > 0) {
        --context->refCount;
    }
    destroy = context->released && context->refCount == 0;
    pthread_mutex_unlock(&context->mutex);

    if (destroy) {
        destroyReleasedContext(context);
    }
}

ConnectedPeerSnapshot acquireConnectedPeer(CallbackContext* excluded) {
    ConnectedPeerSnapshot snapshot{nullptr, -1};

    pthread_mutex_lock(&gHandleListMutex);
    for (HandleEntry* entry = gHandleList; entry != nullptr; entry = entry->next) {
        CallbackContext* candidate = entry->context;
        if (candidate == excluded) {
            continue;
        }

        pthread_mutex_lock(&candidate->mutex);
        if (!candidate->released && candidate->connectedDelivered
                && candidate->lastAddressCount > 0) {
            ++candidate->refCount;
            snapshot.context = candidate;
            snapshot.baselineAddressCount = candidate->lastAddressCount;
            pthread_mutex_unlock(&candidate->mutex);
            break;
        }
        pthread_mutex_unlock(&candidate->mutex);
    }
    pthread_mutex_unlock(&gHandleListMutex);

    return snapshot;
}

void rememberAddressCount(CallbackContext* context, int addressCount) {
    if (addressCount <= 0) {
        return;
    }

    pthread_mutex_lock(&context->mutex);
    if (!context->released && addressCount > context->lastAddressCount) {
        context->lastAddressCount = addressCount;
    }
    pthread_mutex_unlock(&context->mutex);
}

void* deferredConnectionWorker(void* cookie) {
    auto* context = static_cast<CallbackContext*>(cookie);

    for (unsigned int attempt = 0; attempt < kAddressPollAttempts; ++attempt) {
        pthread_mutex_lock(&context->mutex);
        const bool stop = context->released || context->connectedDelivered;
        pthread_mutex_unlock(&context->mutex);
        if (stop) {
            break;
        }

        const int addressCount = gRealApi.getIpAddressCount(context->handle);
        ConnectedPeerSnapshot peer = acquireConnectedPeer(context);
        int sharedAddressCount = -1;
        if (peer.context != nullptr) {
            sharedAddressCount = gRealApi.getIpAddressCount(peer.context->handle);
        }

        const bool hasEvidence = angler::dsi::deferredConnectionHasEvidence(
                addressCount, peer.baselineAddressCount, sharedAddressCount);
        if (hasEvidence) {
            DsiEventCallback callback = nullptr;
            void* userData = nullptr;

            rememberAddressCount(context, addressCount);
            if (peer.context != nullptr) {
                rememberAddressCount(peer.context, sharedAddressCount);
            }

            pthread_mutex_lock(&context->mutex);
            if (!context->released && !context->connectedDelivered) {
                context->connectedDelivered = true;
                callback = context->callback;
                userData = context->userData;
            }
            context->completionScheduled = false;
            pthread_mutex_unlock(&context->mutex);

            if (peer.context != nullptr) {
                dropContextRef(peer.context);
            }

            if (callback != nullptr) {
                ALOGI("Completing data setup after deferred address check: "
                      "addresses=%d shared=%d baseline=%d",
                      addressCount, sharedAddressCount, peer.baselineAddressCount);
                callback(context->handle, userData,
                         angler::dsi::kDsiEventNetIsConnected, nullptr);
            }
            dropContextRef(context);
            return nullptr;
        }

        if (peer.context != nullptr) {
            dropContextRef(peer.context);
        }

        usleep(kAddressPollIntervalUs);
    }

    pthread_mutex_lock(&context->mutex);
    const bool timedOut = !context->released && !context->connectedDelivered;
    context->completionScheduled = false;
    pthread_mutex_unlock(&context->mutex);

    if (timedOut) {
        ALOGE("Timed out waiting for an address after WDS connected event");
    }
    dropContextRef(context);
    return nullptr;
}

void scheduleDeferredConnection(CallbackContext* context) {
    pthread_mutex_lock(&context->mutex);
    if (context->released || context->connectedDelivered || context->completionScheduled) {
        pthread_mutex_unlock(&context->mutex);
        return;
    }
    context->completionScheduled = true;
    ++context->refCount;
    pthread_mutex_unlock(&context->mutex);

    pthread_t worker;
    const int createResult = pthread_create(&worker, nullptr, deferredConnectionWorker, context);
    if (createResult != 0) {
        pthread_mutex_lock(&context->mutex);
        context->completionScheduled = false;
        pthread_mutex_unlock(&context->mutex);
        ALOGE("Unable to start deferred DSI completion worker: %d", createResult);
        dropContextRef(context);
        return;
    }
    pthread_detach(worker);
}

void compatEventCallback(DsiHandle handle, void* cookie, int event,
                         DsiEventPayload* payload) {
    auto* context = static_cast<CallbackContext*>(cookie);

    pthread_mutex_lock(&context->mutex);
    const bool wasConnected = context->connectedDelivered;
    const angler::dsi::EventAction action = angler::dsi::decideEventAction(
            wasConnected,
            0,
            event);

    if (action == angler::dsi::EventAction::kForward
            && event == angler::dsi::kDsiEventNetIsConnected) {
        context->connectedDelivered = true;
    } else if (action == angler::dsi::EventAction::kForward
            && event == angler::dsi::kDsiEventNetNoNet) {
        context->connectedDelivered = false;
        context->lastAddressCount = 0;
    }
    pthread_mutex_unlock(&context->mutex);

    if (action == angler::dsi::EventAction::kDrop) {
        return;
    }

    if (action == angler::dsi::EventAction::kTranslateToNetIsConnected) {
        ALOGI("Deferring WDS connected event until stock DSI releases its callback lock");
        scheduleDeferredConnection(context);
        return;
    }
    context->callback(handle, context->userData, event, payload);
}

}  // namespace

extern "C" DsiHandle dsi_get_data_srvc_hndl(DsiEventCallback callback, void* userData) {
    if (callback == nullptr || !realApiReady()) {
        return nullptr;
    }

    auto* context = static_cast<CallbackContext*>(std::calloc(1, sizeof(CallbackContext)));
    auto* entry = static_cast<HandleEntry*>(std::calloc(1, sizeof(HandleEntry)));
    if (context == nullptr || entry == nullptr) {
        std::free(context);
        std::free(entry);
        return nullptr;
    }

    context->callback = callback;
    context->userData = userData;
    context->refCount = 1;
    pthread_mutex_init(&context->mutex, nullptr);

    DsiHandle handle = gRealApi.getDataServiceHandle(compatEventCallback, context);
    if (handle == nullptr) {
        pthread_mutex_destroy(&context->mutex);
        std::free(context);
        std::free(entry);
        return nullptr;
    }
    context->handle = handle;

    entry->handle = handle;
    entry->context = context;
    pthread_mutex_lock(&gHandleListMutex);
    entry->next = gHandleList;
    gHandleList = entry;
    pthread_mutex_unlock(&gHandleListMutex);
    return handle;
}

extern "C" int dsi_start_data_call(DsiHandle handle) {
    if (!realApiReady()) {
        return -1;
    }
    return gRealApi.startDataCall(handle);
}

extern "C" void dsi_rel_data_srvc_hndl(DsiHandle handle) {
    if (!realApiReady()) {
        return;
    }

    pthread_mutex_lock(&gHandleListMutex);
    HandleEntry** link = &gHandleList;
    HandleEntry* entry = nullptr;
    while (*link != nullptr) {
        if ((*link)->handle == handle) {
            entry = *link;
            *link = entry->next;
            break;
        }
        link = &(*link)->next;
    }
    pthread_mutex_unlock(&gHandleListMutex);

    if (entry == nullptr) {
        gRealApi.releaseDataServiceHandle(handle);
        return;
    }

    CallbackContext* context = entry->context;
    std::free(entry);

    pthread_mutex_lock(&context->mutex);
    context->released = true;
    pthread_mutex_unlock(&context->mutex);
    dropContextRef(context);
}
