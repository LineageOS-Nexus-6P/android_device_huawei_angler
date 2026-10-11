/*
 * Angler's Oreo pm-service creates its Binder service object on main()'s
 * stack. Android 15's RefBase rejects the first strong reference to that
 * object. Limit the legacy promotion behavior to pm-service by linking this
 * library only into that vendor binary; no platform libutils behavior is
 * changed.
 */

#define LOG_TAG "AnglerPeripheralCompat"

#include <dlfcn.h>
#include <log/log.h>
#include <pthread.h>
#include <stdint.h>
#include <utils/RefBase.h>

namespace android {

using IncStrongFn = void (*)(const RefBase*, const void*);

static IncStrongFn getRealIncStrong() {
    static IncStrongFn realIncStrong = []() {
        auto fn = reinterpret_cast<IncStrongFn>(
                dlsym(RTLD_NEXT, "_ZNK7android7RefBase9incStrongEPKv"));
        LOG_ALWAYS_FATAL_IF(fn == nullptr, "cannot resolve Android 15 RefBase::incStrong");
        return fn;
    }();
    return realIncStrong;
}

static bool isOnCurrentThreadStack(const RefBase* object) {
    pthread_attr_t attributes;
    if (pthread_getattr_np(pthread_self(), &attributes) != 0) {
        return false;
    }

    void* stackBase = nullptr;
    size_t stackSize = 0;
    const int result = pthread_attr_getstack(&attributes, &stackBase, &stackSize);
    pthread_attr_destroy(&attributes);
    if (result != 0) {
        return false;
    }

    const auto address = reinterpret_cast<uintptr_t>(object);
    const auto begin = reinterpret_cast<uintptr_t>(stackBase);
    return address >= begin && address < begin + stackSize;
}

void RefBase::incStrong(const void* id) const {
    if (isOnCurrentThreadStack(this)) {
        LOG_ALWAYS_FATAL_IF(!getWeakRefs()->attemptIncStrong(id),
                            "failed to acquire legacy peripheral-manager reference");
        return;
    }

    const IncStrongFn realIncStrong = getRealIncStrong();
    realIncStrong(this, id);
}

}  // namespace android
