#
# Copyright (C) 2015 The Android Open Source Project
# Copyright (C) 2026 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

ifneq ($(CALLED_FROM_SETUP),true)
ifneq ($(filter aconfig_value_set-lineage-bp1a,$(RELEASE_ACONFIG_VALUE_SETS)),aconfig_value_set-lineage-bp1a)
$(error Angler requires lineage_angler-bp1a-userdebug)
endif
endif

# Nexus 6P launched on Android 6.0 (API 23).
$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/product_launched_with_m.mk)

$(call inherit-product, device/huawei/angler/device.mk)

# Keep UI sound effects on AudioFlinger's native 48 kHz mixer rate. This must precede the generic
# Lineage audio package, whose first duplicate destination otherwise selects the 44.1 kHz tick.
PRODUCT_COPY_FILES += \
    frameworks/base/data/sounds/effects/ogg/Effect_Tick_48k.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/ui/Effect_Tick.ogg

$(call inherit-product, vendor/lineage/config/common_full_phone.mk)

# Angler's 3.10 kernel has neither userfaultfd nor MREMAP_DONTUNMAP.  Leaving
# this at the modern platform default makes odrefresh reject the prebuilt ART
# artifacts and compile both boot images on-device after every OTA.
PRODUCT_ENABLE_UFFD_GC := false

# Angler's BCM4358 is exposed to the current Bluetooth stack through the
# legacy HIDL controller service and the source-built Broadcom vendor library.
# Keep this outside the protected RIL product definition.
PRODUCT_SOONG_NAMESPACES += \
    hardware/broadcom/libbt

$(call soong_config_set,brcm_libbt,custom_bt_config,//device/huawei/angler:vnd_angler.txt)

PRODUCT_PACKAGES += \
    android.hardware.bluetooth.audio@2.1-impl \
    android.hardware.bluetooth@1.0-impl \
    android.hardware.bluetooth@1.0-service \
    audio.bluetooth.default \
    libbt-vendor

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.bluetooth.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.bluetooth.xml \
    frameworks/native/data/etc/android.hardware.bluetooth_le.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.bluetooth_le.xml

# Enable the classic phone profiles explicitly. Android 15 defaults all
# property-controlled profiles to disabled; GATT and ASHA are the only
# exceptions. This matches the msm8994 OnePlus 2 and Kitakami phone products.
PRODUCT_SYSTEM_PROPERTIES += \
    bluetooth.device.class_of_device=90,2,12 \
    bluetooth.le.disable_apcf_extended_features=1 \
    bluetooth.profile.a2dp.source.enabled=true \
    bluetooth.profile.asha.central.enabled=true \
    bluetooth.profile.avrcp.target.enabled=true \
    bluetooth.profile.bas.client.enabled=true \
    bluetooth.profile.gatt.enabled=true \
    bluetooth.profile.hfp.ag.enabled=true \
    bluetooth.profile.hid.device.enabled=true \
    bluetooth.profile.hid.host.enabled=true \
    bluetooth.profile.map.server.enabled=true \
    bluetooth.profile.opp.enabled=true \
    bluetooth.profile.pan.nap.enabled=true \
    bluetooth.profile.pan.panu.enabled=true \
    bluetooth.profile.pbap.server.enabled=true \
    bluetooth.profile.sap.server.enabled=true

# Restore Angler's factory Fluence profile without changing the protected RIL
# device configuration. The Qualcomm audio HAL uses these properties to keep
# the lower microphone primary while enabling the calibrated auxiliary inputs
# for handset, VoIP, and speakerphone noise cancellation.
PRODUCT_SYSTEM_PROPERTIES += \
    persist.audio.fluence.speaker=true \
    persist.audio.fluence.voicecall=true \
    persist.audio.fluence.voicecomm=true \
    ro.qc.sdk.audio.fluencetype=fluence

# Angler's legacy Adreno userspace can clobber the AArch64 x18 register while
# Zygote preloads GL. Bluetooth's SCS-enabled JNI then inherits the invalid
# shadow-stack pointer and crashes before reaching the HAL.
PRODUCT_PROPERTY_OVERRIDES += \
    ro.zygote.disable_gl_preload=1

PRODUCT_NAME := lineage_angler
PRODUCT_DEVICE := angler
PRODUCT_BRAND := google
PRODUCT_MODEL := Nexus 6P
PRODUCT_MANUFACTURER := Huawei

# Preserve the identity of Google's final factory image for Angler.
PRODUCT_BUILD_PROP_OVERRIDES += \
    BuildDesc="angler-user 8.1.0 OPM7.181205.001 5080180 release-keys" \
    BuildFingerprint=google/angler/angler:8.1.0/OPM7.181205.001/5080180:user/release-keys

TARGET_SCREEN_HEIGHT := 2560
TARGET_SCREEN_WIDTH := 1440
TARGET_VENDOR := huawei
