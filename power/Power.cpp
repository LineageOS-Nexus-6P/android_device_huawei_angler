// SPDX-License-Identifier: Apache-2.0

#include "Power.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <dlfcn.h>

#include <algorithm>
#include <cmath>

namespace aidl::android::hardware::power::impl::angler {

using ndk::ScopedAStatus;

namespace {

constexpr char kPerfClientLibrary[] = "libqti-perfd-client.so";
constexpr int kDefaultInteractionDurationMs = 1500;
constexpr int kMaximumBoostDurationMs = 5000;
constexpr int kLaunchDurationMs = 4250;

constexpr int kLittleMinFrequency1200MHz = 0x020C;
constexpr int kBigMinFrequency1200MHz = 0x1F0C;
constexpr int kBigMaxFrequency1800MHz = 0x2312;
constexpr int kSchedUpmigrateBase = 0x4E00;
constexpr int kSchedDownmigrateBase = 0x4F00;

constexpr char kGpuMinFrequencyPath[] =
        "/sys/class/kgsl/kgsl-3d0/devfreq/min_freq";
constexpr char kGpuIdleFrequencyHz[] = "180000000";
constexpr char kGpuBoostFrequencyHz[] = "450000000";
constexpr int kDefaultGpuBoostDurationMs = 1000;
constexpr int kMinimumGpuBoostDurationMs = 750;
constexpr int kMaximumGpuBoostDurationMs = 1500;

}  // namespace

Power::Power() {
    gpuBoostThread_ = std::thread(&Power::gpuBoostLoop, this);

    perfClient_ = dlopen(kPerfClientLibrary, RTLD_NOW | RTLD_LOCAL);
    if (perfClient_ == nullptr) {
        LOG(ERROR) << "Unable to load " << kPerfClientLibrary << ": " << dlerror();
        return;
    }

    perfLockAcquire_ = reinterpret_cast<PerfLockAcquire>(
            dlsym(perfClient_, "perf_lock_acq"));
    perfLockRelease_ = reinterpret_cast<PerfLockRelease>(
            dlsym(perfClient_, "perf_lock_rel"));
    if (perfLockAcquire_ == nullptr || perfLockRelease_ == nullptr) {
        LOG(ERROR) << "Angler perflock entry points are unavailable";
        dlclose(perfClient_);
        perfClient_ = nullptr;
        perfLockAcquire_ = nullptr;
        perfLockRelease_ = nullptr;
    }
}

Power::~Power() {
    {
        std::lock_guard lock(perfLockMutex_);
        releaseBoostLocked(interactionHandles_);
        releaseBoostLocked(audioHandles_);
        releaseBoostLocked(launchHandles_);
        releaseGpuBoostLocked();
        stopGpuBoostThread_ = true;
    }
    gpuBoostCv_.notify_all();
    if (gpuBoostThread_.joinable()) {
        gpuBoostThread_.join();
    }
    if (perfClient_ != nullptr) {
        dlclose(perfClient_);
    }
}

void Power::scheduleGpuBoostLocked(int durationMs) {
    const int requestedDuration = durationMs > 0
            ? durationMs
            : kDefaultGpuBoostDurationMs;
    const int boundedDuration = std::clamp(requestedDuration,
                                           kMinimumGpuBoostDurationMs,
                                           kMaximumGpuBoostDurationMs);

    if (!gpuBoostActive_) {
        if (!::android::base::WriteStringToFile(kGpuBoostFrequencyHz,
                                                 kGpuMinFrequencyPath)) {
            PLOG(ERROR) << "Unable to raise Angler GPU minimum frequency";
            return;
        }
        gpuBoostActive_ = true;
    }

    gpuBoostDeadline_ = std::chrono::steady_clock::now() +
            std::chrono::milliseconds(boundedDuration);
    gpuBoostCv_.notify_all();
}

void Power::releaseGpuBoostLocked() {
    if (!gpuBoostActive_) {
        return;
    }

    if (!::android::base::WriteStringToFile(kGpuIdleFrequencyHz,
                                             kGpuMinFrequencyPath)) {
        PLOG(ERROR) << "Unable to restore Angler GPU minimum frequency";
    }
    gpuBoostActive_ = false;
    gpuBoostDeadline_ = {};
    gpuBoostCv_.notify_all();
}

void Power::gpuBoostLoop() {
    std::unique_lock lock(perfLockMutex_);
    while (!stopGpuBoostThread_) {
        gpuBoostCv_.wait(lock, [this] {
            return stopGpuBoostThread_ || gpuBoostActive_;
        });
        if (stopGpuBoostThread_) {
            break;
        }

        const auto deadline = gpuBoostDeadline_;
        const bool changed = gpuBoostCv_.wait_until(lock, deadline, [this, deadline] {
            return stopGpuBoostThread_ || !gpuBoostActive_ ||
                    gpuBoostDeadline_ != deadline;
        });
        if (!changed && gpuBoostActive_) {
            releaseGpuBoostLocked();
        }
    }
}

int Power::acquireLocked(int handle, int durationMs, int* resources,
                         size_t resourceCount) {
    if (perfLockAcquire_ == nullptr) {
        return handle;
    }

    const int acquired = perfLockAcquire_(handle, durationMs, resources,
                                          static_cast<int>(resourceCount));
    if (acquired < 0) {
        LOG(WARNING) << "Failed to acquire Angler perflock";
        return handle;
    }
    return acquired;
}

void Power::releaseBoostLocked(std::array<int, 4>& handles) {
    if (perfLockRelease_ == nullptr) {
        handles.fill(0);
        return;
    }

    for (int& handle : handles) {
        if (handle > 0) {
            perfLockRelease_(handle);
            handle = 0;
        }
    }
}

void Power::applyTimedBoostLocked(std::array<int, 4>& handles, int durationMs,
                                  bool throttleInteractions) {
    if (perfLockAcquire_ == nullptr) {
        return;
    }

    const int requestedDuration = std::max(durationMs, 0);
    int littleDuration = kDefaultInteractionDurationMs;
    int downmigrateDuration = 1000;
    int upmigrate = 20;

    if (throttleInteractions) {
        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                now - previousInteraction_).count();
        const int64_t boundedElapsed = std::min<int64_t>(elapsed, 750000);

        if (previousInteraction_.time_since_epoch().count() != 0 &&
            boundedElapsed < 250000 && requestedDuration <= 750) {
            return;
        }

        const double ratio = static_cast<double>(boundedElapsed) / 750000.0;
        upmigrate = 95 - static_cast<int>(75.0 * ratio * ratio);
        if (requestedDuration >= 750) {
            upmigrate = 20;
        }
        previousInteraction_ = now;
    }

    if (requestedDuration > 1000) {
        downmigrateDuration = std::min(requestedDuration, kMaximumBoostDurationMs);
        littleDuration = std::min(downmigrateDuration + 750, 5750);
    }

    int littleResources[] = {kLittleMinFrequency1200MHz};
    int bigResources[] = {kBigMaxFrequency1800MHz, kBigMinFrequency1200MHz};
    int downmigrateResources[] = {kSchedDownmigrateBase | (upmigrate / 2)};
    int upmigrateResources[] = {kSchedUpmigrateBase | upmigrate};

    handles[0] = acquireLocked(handles[0], littleDuration, littleResources,
                               std::size(littleResources));
    handles[1] = acquireLocked(handles[1], 500,
                               bigResources, std::size(bigResources));
    handles[2] = acquireLocked(handles[2], downmigrateDuration,
                               downmigrateResources,
                               std::size(downmigrateResources));
    handles[3] = acquireLocked(handles[3], 500, upmigrateResources,
                               std::size(upmigrateResources));
}

ScopedAStatus Power::setMode(Mode type, bool enabled) {
    if (type == Mode::LAUNCH) {
        std::lock_guard lock(perfLockMutex_);
        if (enabled) {
            applyTimedBoostLocked(launchHandles_, kLaunchDurationMs, false);
        } else {
            releaseBoostLocked(launchHandles_);
        }
    }
    return ScopedAStatus::ok();
}

ScopedAStatus Power::isModeSupported(Mode type, bool* supported) {
    *supported = type == Mode::LAUNCH;
    return ScopedAStatus::ok();
}

ScopedAStatus Power::setBoost(Boost type, int32_t durationMs) {
    std::lock_guard lock(perfLockMutex_);
    switch (type) {
        case Boost::INTERACTION:
            if (durationMs < 0) {
                releaseBoostLocked(interactionHandles_);
                releaseGpuBoostLocked();
            } else {
                applyTimedBoostLocked(interactionHandles_, durationMs, true);
                scheduleGpuBoostLocked(durationMs);
            }
            break;
        case Boost::AUDIO_LAUNCH:
            if (durationMs < 0) {
                releaseBoostLocked(audioHandles_);
            } else {
                applyTimedBoostLocked(audioHandles_, durationMs, false);
            }
            break;
        default:
            break;
    }
    return ScopedAStatus::ok();
}

ScopedAStatus Power::isBoostSupported(Boost type, bool* supported) {
    *supported = type == Boost::INTERACTION || type == Boost::AUDIO_LAUNCH;
    return ScopedAStatus::ok();
}

ScopedAStatus Power::createHintSession(
        int32_t, int32_t, const std::vector<int32_t>&, int64_t,
        std::shared_ptr<IPowerHintSession>* session) {
    *session = nullptr;
    return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ScopedAStatus Power::createHintSessionWithConfig(
        int32_t, int32_t, const std::vector<int32_t>&, int64_t, SessionTag,
        SessionConfig* config, std::shared_ptr<IPowerHintSession>* session) {
    *config = {};
    *session = nullptr;
    return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ScopedAStatus Power::getHintSessionPreferredRate(int64_t* rateNanos) {
    *rateNanos = 0;
    return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ScopedAStatus Power::getSessionChannel(int32_t, int32_t, ChannelConfig* config) {
    *config = {};
    return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ScopedAStatus Power::closeSessionChannel(int32_t, int32_t) {
    return ScopedAStatus::ok();
}

ScopedAStatus Power::getSupportInfo(SupportInfo* info) {
    *info = {};
    return ScopedAStatus::ok();
}

ScopedAStatus Power::getCpuHeadroom(const CpuHeadroomParams&, CpuHeadroomResult*) {
    return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ScopedAStatus Power::getGpuHeadroom(const GpuHeadroomParams&, GpuHeadroomResult*) {
    return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ScopedAStatus Power::sendCompositionData(const std::vector<CompositionData>&) {
    return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ScopedAStatus Power::sendCompositionUpdate(const CompositionUpdate&) {
    return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

}  // namespace aidl::android::hardware::power::impl::angler
