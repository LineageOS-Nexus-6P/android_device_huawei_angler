// SPDX-License-Identifier: Apache-2.0

#include "Power.h"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using aidl::android::hardware::power::impl::angler::Power;

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    std::shared_ptr<Power> service = ndk::SharedRefBase::make<Power>();
    const std::string instance = std::string(Power::descriptor) + "/default";
    CHECK_EQ(AServiceManager_addService(service->asBinder().get(), instance.c_str()),
             STATUS_OK);
    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;
}
