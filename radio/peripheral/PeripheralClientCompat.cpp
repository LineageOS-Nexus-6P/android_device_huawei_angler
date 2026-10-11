/*
 * Compatibility implementation of Angler's Qualcomm peripheral-manager
 * client ABI.  The Binder protocol and exported C entry points match the
 * stock OPM7 library, but all Parcel objects use the current platform ABI.
 */

#define LOG_TAG "AnglerPeripheralClient"

#include <binder/IInterface.h>
#include <binder/IServiceManager.h>
#include <binder/Parcel.h>
#include <binder/ProcessState.h>
#include <log/log.h>
#include <utils/String8.h>

#include <algorithm>
#include <cstdint>
#include <list>
#include <mutex>
#include <new>

namespace android {

enum PmEvent : int32_t {
    kGoingOffline = 0,
    kIsOffline = 1,
    kGoingOnline = 2,
    kIsOnline = 3,
};

using PmClientNotifier = void (*)(void*, PmEvent);

class IPeriperalManagerCb : public IInterface {
  public:
    DECLARE_META_INTERFACE(PeriperalManagerCb);
    virtual void notifyCallback(int32_t event) = 0;
};

class BnPeriperalManagerCb : public BnInterface<IPeriperalManagerCb> {
  public:
    status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply,
                        uint32_t flags = 0) override;
};

class IPeripheralManager : public IInterface {
  public:
    DECLARE_META_INTERFACE(PeripheralManager);

    virtual status_t registar(const String8& devName, const String8& clientName,
                              const sp<IPeriperalManagerCb>& notifier,
                              int64_t* clientId, int64_t* state) = 0;
    virtual status_t unregister(int64_t clientId) = 0;
    virtual status_t connect(int64_t clientId) = 0;
    virtual status_t disconnect(int64_t clientId) = 0;
    virtual status_t acknowledge(int64_t clientId, int32_t event) = 0;
    virtual status_t forceShutdown(int64_t clientId) = 0;
};

class BnPeripheralManager : public BnInterface<IPeripheralManager> {
  public:
    status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply,
                        uint32_t flags = 0) override;
};

enum : uint32_t {
    kNotifyCallback = IBinder::FIRST_CALL_TRANSACTION,
    kRegister = IBinder::FIRST_CALL_TRANSACTION,
    kUnregister,
    kConnect,
    kDisconnect,
    kAcknowledge,
    kForceShutdown,
};

class BpPeriperalManagerCb : public BpInterface<IPeriperalManagerCb> {
  public:
    explicit BpPeriperalManagerCb(const sp<IBinder>& impl)
        : BpInterface<IPeriperalManagerCb>(impl) {}

    void notifyCallback(int32_t event) override {
        Parcel data, reply;
        data.writeInterfaceToken(IPeriperalManagerCb::getInterfaceDescriptor());
        data.writeInt32(event);
        remote()->transact(kNotifyCallback, data, &reply, IBinder::FLAG_ONEWAY);
    }
};

IMPLEMENT_META_INTERFACE(PeriperalManagerCb,
                         "com.qualcomm.IPeriperalManagerCb");

status_t BnPeriperalManagerCb::onTransact(uint32_t code, const Parcel& data,
                                          Parcel* reply, uint32_t flags) {
    if (code == kNotifyCallback) {
        CHECK_INTERFACE(IPeriperalManagerCb, data, reply);
        notifyCallback(data.readInt32());
        return NO_ERROR;
    }
    return BBinder::onTransact(code, data, reply, flags);
}

class BpPeripheralManager : public BpInterface<IPeripheralManager> {
  public:
    explicit BpPeripheralManager(const sp<IBinder>& impl)
        : BpInterface<IPeripheralManager>(impl) {}

    status_t registar(const String8& devName, const String8& clientName,
                      const sp<IPeriperalManagerCb>& notifier,
                      int64_t* clientId, int64_t* state) override {
        Parcel data, reply;
        data.writeInterfaceToken(IPeripheralManager::getInterfaceDescriptor());
        data.writeString8(devName);
        data.writeString8(clientName);
        data.writeStrongBinder(IInterface::asBinder(notifier));
        status_t status = remote()->transact(kRegister, data, &reply);
        if (status != OK) {
            return status;
        }
        *clientId = reply.readInt64();
        *state = reply.readInt64();
        return reply.readInt32();
    }

    status_t unregister(int64_t clientId) override {
        return transactClient(kUnregister, clientId);
    }

    status_t connect(int64_t clientId) override {
        return transactClient(kConnect, clientId);
    }

    status_t disconnect(int64_t clientId) override {
        return transactClient(kDisconnect, clientId);
    }

    status_t acknowledge(int64_t clientId, int32_t event) override {
        Parcel data, reply;
        data.writeInterfaceToken(IPeripheralManager::getInterfaceDescriptor());
        data.writeInt64(clientId);
        data.writeInt32(event);
        status_t status = remote()->transact(kAcknowledge, data, &reply);
        return status == OK ? reply.readInt32() : status;
    }

    status_t forceShutdown(int64_t clientId) override {
        return transactClient(kForceShutdown, clientId);
    }

  private:
    status_t transactClient(uint32_t transaction, int64_t clientId) {
        Parcel data, reply;
        data.writeInterfaceToken(IPeripheralManager::getInterfaceDescriptor());
        data.writeInt64(clientId);
        status_t status = remote()->transact(transaction, data, &reply);
        return status == OK ? reply.readInt32() : status;
    }
};

IMPLEMENT_META_INTERFACE(PeripheralManager, "vendor.qcom.IPeripheralManager");

status_t BnPeripheralManager::onTransact(uint32_t code, const Parcel& data,
                                         Parcel* reply, uint32_t flags) {
    switch (code) {
        case kRegister: {
            CHECK_INTERFACE(IPeripheralManager, data, reply);
            const String8 devName = data.readString8();
            const String8 clientName = data.readString8();
            const sp<IPeriperalManagerCb> notifier =
                    interface_cast<IPeriperalManagerCb>(data.readStrongBinder());
            int64_t clientId = 0;
            int64_t state = kIsOffline;
            const status_t result =
                    registar(devName, clientName, notifier, &clientId, &state);
            reply->writeInt64(clientId);
            reply->writeInt64(state);
            reply->writeInt32(result);
            return NO_ERROR;
        }
        case kUnregister:
        case kConnect:
        case kDisconnect:
        case kForceShutdown: {
            CHECK_INTERFACE(IPeripheralManager, data, reply);
            const int64_t clientId = data.readInt64();
            status_t result;
            if (code == kUnregister) {
                result = unregister(clientId);
            } else if (code == kConnect) {
                result = connect(clientId);
            } else if (code == kDisconnect) {
                result = disconnect(clientId);
            } else {
                result = forceShutdown(clientId);
            }
            reply->writeInt32(result);
            return NO_ERROR;
        }
        case kAcknowledge: {
            CHECK_INTERFACE(IPeripheralManager, data, reply);
            const status_t result = acknowledge(data.readInt64(), data.readInt32());
            reply->writeInt32(result);
            return NO_ERROR;
        }
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

class EventNotifier : public BnPeriperalManagerCb {
  public:
    EventNotifier(PmClientNotifier callback, void* clientData)
        : callback_(callback), clientData_(clientData) {}

    void notifyCallback(int32_t event) override {
        if (callback_ != nullptr) {
            callback_(clientData_, static_cast<PmEvent>(event));
        }
    }

  private:
    PmClientNotifier callback_;
    void* clientData_;
};

struct PeripheralManagerClient {
    int64_t id = 0;
    String8 devName;
    String8 clientName;
    bool connected = false;
    sp<IPeriperalManagerCb> notifier;
    sp<IPeripheralManager> operations;
    std::mutex lock;
};

std::list<PeripheralManagerClient*> gClients;
std::mutex gClientsLock;

static PeripheralManagerClient* findAndLock(void* handle, bool remove) {
    std::lock_guard<std::mutex> clientsLock(gClientsLock);
    auto it = std::find(gClients.begin(), gClients.end(), handle);
    if (it == gClients.end()) {
        return nullptr;
    }
    PeripheralManagerClient* client = *it;
    client->lock.lock();
    if (remove) {
        gClients.erase(it);
    }
    return client;
}

static status_t registerClient(PeripheralManagerClient* client, PmEvent* state) {
    const sp<IServiceManager> serviceManager = defaultServiceManager();
    const sp<IBinder> service =
            serviceManager->getService(String16("vendor.qcom.PeripheralManager"));
    if (service == nullptr) {
        ALOGE("PeripheralManager service is unavailable");
        return NAME_NOT_FOUND;
    }

    client->operations = IPeripheralManager::asInterface(service);
    if (client->operations == nullptr) {
        return BAD_TYPE;
    }

    int64_t serverState = kIsOffline;
    status_t status = client->operations->registar(
            client->devName, client->clientName, client->notifier,
            &client->id, &serverState);
    if (status == OK && state != nullptr) {
        *state = static_cast<PmEvent>(serverState);
    }
    return status;
}

}  // namespace android

using android::EventNotifier;
using android::IPeripheralManager;
using android::PeripheralManagerClient;
using android::PmClientNotifier;
using android::PmEvent;
using android::findAndLock;
using android::gClients;
using android::gClientsLock;
using android::registerClient;

extern "C" {

int pm_show_peripherals(const char* names[8]) {
    if (names == nullptr) {
        return -1;
    }
    // Angler has one integrated modem. Current known users obtain the name
    // from libmdmdetect themselves; retain the stock helper ABI as well.
    names[0] = "modem";
    return 1;
}

int pm_client_register(PmClientNotifier callback, void* clientData,
                       const char* devName, const char* clientName,
                       PmEvent* state, void** handle) {
    if (devName == nullptr || clientName == nullptr || handle == nullptr) {
        return -1;
    }

    auto* client = new (std::nothrow) PeripheralManagerClient;
    if (client == nullptr) {
        return -1;
    }
    client->devName.setTo(devName);
    client->clientName.setTo(clientName);
    client->notifier = android::sp<EventNotifier>::make(callback, clientData);

    if (registerClient(client, state) != android::OK) {
        delete client;
        return -1;
    }

    {
        std::lock_guard<std::mutex> lock(gClientsLock);
        gClients.push_back(client);
    }
    android::ProcessState::self()->startThreadPool();
    *handle = client;
    ALOGI("%s registered for %s", clientName, devName);
    return 0;
}

int pm_client_unregister(void* handle) {
    PeripheralManagerClient* client = findAndLock(handle, true);
    if (client == nullptr) {
        return -1;
    }
    const android::status_t status = client->operations->unregister(client->id);
    client->operations.clear();
    client->notifier.clear();
    client->lock.unlock();
    delete client;
    return status == android::OK ? 0 : -1;
}

int pm_client_connect(void* handle) {
    PeripheralManagerClient* client = findAndLock(handle, false);
    if (client == nullptr) {
        return -1;
    }
    const android::status_t status = client->operations->connect(client->id);
    if (status == android::OK) {
        client->connected = true;
    }
    client->lock.unlock();
    return status == android::OK ? 0 : -1;
}

int pm_client_disconnect(void* handle) {
    PeripheralManagerClient* client = findAndLock(handle, false);
    if (client == nullptr) {
        return -1;
    }
    const android::status_t status = client->operations->disconnect(client->id);
    if (status == android::OK) {
        client->connected = false;
    }
    client->lock.unlock();
    return status == android::OK ? 0 : -1;
}

int pm_client_event_acknowledge(void* handle, PmEvent event) {
    PeripheralManagerClient* client = findAndLock(handle, false);
    if (client == nullptr) {
        return -1;
    }
    const android::status_t status =
            client->operations->acknowledge(client->id, static_cast<int32_t>(event));
    client->lock.unlock();
    return status == android::OK ? 0 : -1;
}

int pm_peripheral_force_shutdown(void* handle) {
    PeripheralManagerClient* client = findAndLock(handle, false);
    if (client == nullptr) {
        return -1;
    }
    const android::status_t status = client->operations->forceShutdown(client->id);
    client->lock.unlock();
    return status == android::OK ? 0 : -1;
}

}  // extern "C"
