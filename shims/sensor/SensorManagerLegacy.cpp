// SPDX-FileCopyrightText: 2026 The LineageOS Project
// SPDX-License-Identifier: Apache-2.0

#include <binder/ProcessState.h>
#include <dlfcn.h>
#include <log/log.h>
#include <sensor/SensorManager.h>
#include <utils/String16.h>

namespace {

constexpr char kGetInstanceForPackageSymbol[] =
        "_ZN7android13SensorManager21getInstanceForPackageERKNS_8String16E";

using GetInstanceForPackage =
        android::SensorManager& (*)(const android::String16&);

__attribute__((constructor)) void startBinderThreadPool() {
    android::ProcessState::self()->setThreadPoolMaxThreadCount(1);
    android::ProcessState::self()->startThreadPool();
}

}  // namespace

android::SensorManager& legacyGetInstanceForPackage(
        const android::String16& packageName)
        __asm__("_ZN7android13SensorManager21getInstanceForPackageERKNS_8String16E");

android::SensorManager& legacyGetInstanceForPackage(
        const android::String16& /* packageName */) {
    static auto getInstanceForPackage = reinterpret_cast<GetInstanceForPackage>(
            dlsym(RTLD_NEXT, kGetInstanceForPackageSymbol));
    LOG_ALWAYS_FATAL_IF(getInstanceForPackage == nullptr,
                        "Unable to resolve SensorManager::getInstanceForPackage: %s",
                        dlerror());

    // Android 15 asks PackageManager for a target SDK when a native sensor
    // client supplies an ordinary package name. The stock camera blobs use
    // "camera" and "GoogSensorWrapper", neither of which is an installed
    // package. A native sensor-service descriptor is the supported identity
    // for VNDK clients and deliberately bypasses that package lookup.
    static const android::String16 nativeSensorClient(
            "android.frameworks.sensorservice@1.0");
    return getInstanceForPackage(nativeSensorClient);
}

#if defined(__arm__)

// String8/String16 and sp<T> are non-trivial types.  The 32-bit ARM EABI
// passes their storage through hidden pointers.  Forward that storage
// directly so the legacy caller and current libsensor share one strong
// reference; constructing a typed wrapper here can briefly drop the last
// reference and make SensorEventQueue::onFirstRef() request its BitTube twice.
extern "C" void currentCreateEventQueue(
        void** result,
        void* manager,
        void** packageName,
        int mode,
        void** attributionTag)
        __asm__("_ZN7android13SensorManager16createEventQueueENS_7String8EiNS_8String16E");

extern "C" void string16Constructor(void** string, const char* value)
        __asm__("_ZN7android8String16C1EPKc");

extern "C" void string16Destructor(void** string)
        __asm__("_ZN7android8String16D1Ev");

extern "C" void legacyCreateEventQueue(
        void** result,
        void* manager,
        void** packageName,
        int mode)
        __asm__("_ZN7android13SensorManager16createEventQueueENS_7String8Ei");

extern "C" void legacyCreateEventQueue(
        void** result,
        void* manager,
        void** packageName,
        int mode) {
    void* attributionTag;

    string16Constructor(&attributionTag, "");
    currentCreateEventQueue(
            result, manager, packageName, mode, &attributionTag);
    string16Destructor(&attributionTag);
}

#else

android::sp<android::SensorEventQueue> legacyCreateEventQueue(
        android::SensorManager* manager,
        android::String8 packageName,
        int mode)
        __asm__("_ZN7android13SensorManager16createEventQueueENS_7String8Ei");

android::sp<android::SensorEventQueue> legacyCreateEventQueue(
        android::SensorManager* manager,
        android::String8 packageName,
        int mode) {
    return manager->createEventQueue(packageName, mode, android::String16(""));
}

#endif
