/*
 * Copyright (C) 2026 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <memory>
#include <string_view>

#include <health/utils.h>
#include <health2impl/Health.h>

using ::android::hardware::health::InitHealthdConfig;
using ::android::hardware::health::V2_1::IHealth;
using ::android::hardware::health::V2_1::implementation::Health;

using namespace std::literals;

extern "C" IHealth* HIDL_FETCH_IHealth(const char* instance) {
    if (instance != "default"sv) {
        return nullptr;
    }

    auto config = std::make_unique<healthd_config>();
    InitHealthdConfig(config.get());

    // Angler exposes full-charge data through its BMS power supply. Since the
    // BMS is not reported as type Battery, healthd cannot discover these paths.
    config->batteryFullChargePath = "/sys/class/power_supply/bms/charge_full";
    config->batteryFullChargeDesignCapacityUahPath =
            "/sys/class/power_supply/bms/charge_full_design";

    return new Health(std::move(config));
}
