// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aidl/android/hardware/power/BnPower.h>
#include <aidl/android/hardware/power/SessionTag.h>

#include <array>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace aidl::android::hardware::power::impl::angler {

class Power : public BnPower {
  public:
    Power();
    ~Power() override;

    ndk::ScopedAStatus setMode(Mode type, bool enabled) override;
    ndk::ScopedAStatus isModeSupported(Mode type, bool* supported) override;
    ndk::ScopedAStatus setBoost(Boost type, int32_t durationMs) override;
    ndk::ScopedAStatus isBoostSupported(Boost type, bool* supported) override;
    ndk::ScopedAStatus createHintSession(
            int32_t tgid, int32_t uid, const std::vector<int32_t>& threadIds,
            int64_t durationNanos,
            std::shared_ptr<IPowerHintSession>* session) override;
    ndk::ScopedAStatus createHintSessionWithConfig(
            int32_t tgid, int32_t uid, const std::vector<int32_t>& threadIds,
            int64_t durationNanos, SessionTag tag, SessionConfig* config,
            std::shared_ptr<IPowerHintSession>* session) override;
    ndk::ScopedAStatus getHintSessionPreferredRate(int64_t* rateNanos) override;
    ndk::ScopedAStatus getSessionChannel(int32_t tgid, int32_t uid,
                                         ChannelConfig* config) override;
    ndk::ScopedAStatus closeSessionChannel(int32_t tgid, int32_t uid) override;
    ndk::ScopedAStatus getSupportInfo(SupportInfo* info) override;
    ndk::ScopedAStatus getCpuHeadroom(const CpuHeadroomParams& params,
                                      CpuHeadroomResult* result) override;
    ndk::ScopedAStatus getGpuHeadroom(const GpuHeadroomParams& params,
                                      GpuHeadroomResult* result) override;
    ndk::ScopedAStatus sendCompositionData(
            const std::vector<CompositionData>& data) override;
    ndk::ScopedAStatus sendCompositionUpdate(
            const CompositionUpdate& update) override;

  private:
    using PerfLockAcquire = int (*)(int, int, int[], int);
    using PerfLockRelease = int (*)(int);

    void applyTimedBoostLocked(std::array<int, 4>& handles, int durationMs,
                               bool throttleInteractions);
    void releaseBoostLocked(std::array<int, 4>& handles);
    int acquireLocked(int handle, int durationMs, int* resources,
                      size_t resourceCount);
    void scheduleGpuBoostLocked(int durationMs);
    void releaseGpuBoostLocked();
    void gpuBoostLoop();

    void* perfClient_ = nullptr;
    PerfLockAcquire perfLockAcquire_ = nullptr;
    PerfLockRelease perfLockRelease_ = nullptr;
    std::array<int, 4> interactionHandles_{};
    std::array<int, 4> audioHandles_{};
    std::array<int, 4> launchHandles_{};
    std::chrono::steady_clock::time_point previousInteraction_{};
    std::chrono::steady_clock::time_point gpuBoostDeadline_{};
    std::condition_variable gpuBoostCv_;
    std::thread gpuBoostThread_;
    bool gpuBoostActive_ = false;
    bool stopGpuBoostThread_ = false;
    std::mutex perfLockMutex_;
};

}  // namespace aidl::android::hardware::power::impl::angler
