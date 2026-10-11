#
# Copyright (C) 2015 The Android Open Source Project
# Copyright (C) 2026 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

DEVICE_PATH := device/huawei/angler

# Architecture
TARGET_ARCH := arm64
TARGET_ARCH_VARIANT := armv8-a
TARGET_CPU_ABI := arm64-v8a
TARGET_CPU_VARIANT := cortex-a53

TARGET_2ND_ARCH := arm
TARGET_2ND_ARCH_VARIANT := armv8-a
TARGET_2ND_CPU_ABI := armeabi-v7a
TARGET_2ND_CPU_ABI2 := armeabi
TARGET_2ND_CPU_VARIANT := cortex-a53.a57

# Bootloader
TARGET_BOOTLOADER_BOARD_NAME := angler
TARGET_NO_BOOTLOADER := true
TARGET_NO_RADIOIMAGE := true
TARGET_BOARD_INFO_FILE := $(DEVICE_PATH)/board-info.txt

# Platform
TARGET_BOARD_PLATFORM := msm8994
TARGET_SCREEN_DENSITY := 560
TARGET_USES_AOSP := true

# Match the newest proprietary vendor image published for Angler. Reporting
# the platform patch level here would overstate the age of the closed blobs.
VENDOR_SECURITY_PATCH := 2018-12-05

# The stock Angler camera stack was built for jemalloc and corrupts Scudo's
# chunk header while tearing down ISP statistics modules. Match the working
# msm8994 OnePlus 2 configuration by keeping jemalloc for 32-bit processes.
MALLOC_SVELTE_FOR_LIBC32 := true
TARGET_PROCESS_SDK_VERSION_OVERRIDE := \
    /vendor/bin/mm-qcamera-daemon=22

# BCM4358 Wi-Fi stack
BOARD_WLAN_DEVICE := bcmdhd
BOARD_HOSTAPD_DRIVER := NL80211
BOARD_HOSTAPD_PRIVATE_LIB := lib_driver_cmd_bcmdhd
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_bcmdhd
WIFI_AVOID_IFACE_RESET_MAC_CHANGE := true
WIFI_DRIVER_FW_PATH_AP := "/vendor/firmware/fw_bcmdhd_apsta.bin"
WIFI_DRIVER_FW_PATH_PARAM := "/sys/module/bcmdhd/parameters/firmware_path"
WIFI_DRIVER_FW_PATH_STA := "/vendor/firmware/fw_bcmdhd.bin"
WIFI_HIDL_UNIFIED_SUPPLICANT_SERVICE_RC_ENTRY := true
WPA_SUPPLICANT_VERSION := VER_0_8_X

# BCM4358 Bluetooth controller used by stock Angler and LineageOS 17.1.
BOARD_HAVE_BLUETOOTH := true
BOARD_HAVE_BLUETOOTH_BCM := true

# Legacy msm8994 display stack, exposed through the HIDL allocator/composer
# adapters used by current SurfaceFlinger.
TARGET_ADDITIONAL_GRALLOC_10_USAGE_BITS := 0x2000U | 0x02000000U
TARGET_USES_ION := true
TARGET_USES_C2D_COMPOSITION := true
TARGET_USES_GRALLOC1_ADAPTER := true
TARGET_FORCE_HWC_FOR_VIRTUAL_DISPLAYS := true
MAX_VIRTUAL_DISPLAY_DIMENSION := 2048
TARGET_USES_HWC2 := true
TARGET_DISABLE_POSTRENDER_CLEANUP := true
NUM_FRAMEBUFFER_SURFACE_BUFFERS := 3

# Qualcomm source modules restored from the pinned LineageOS-UL msm8994 tree.
BOARD_VENDOR_QCOM_GPS_LOC_API_HARDWARE := $(TARGET_BOARD_PLATFORM)
BOARD_VENDOR_QCOM_LOC_PDK_FEATURE_SET := true
$(call soong_config_set,qtiaudio,feature_dsm_feedback,true)
$(call soong_config_set,rmnetctl,old_rmnet_data,true)

# Angler uses Google's nanohub sensor stack. Keep the device-specific sensor
# catalogue in this tree while reusing the current LineageOS contexthub HAL.
$(call soong_config_set,nanohub_sensorhal,sensorlist_library,libsensorlist_angler)
$(call soong_config_set,nanohub_sensorhal,lid_state_enabled,true)
$(call soong_config_set,nanohub_sensorhal,usb_mag_bias_enabled,true)
$(call soong_config_set,nanohub_sensorhal,direct_report_enabled,true)

# VINTF. Stage 8 adds one proven service declaration at a time.
DEVICE_MANIFEST_FILE := $(DEVICE_PATH)/manifest.xml
DEVICE_MATRIX_FILE := $(DEVICE_PATH)/compatibility_matrix.xml
DEVICE_FRAMEWORK_COMPATIBILITY_MATRIX_FILE := $(DEVICE_PATH)/framework_compatibility_matrix.xml

# SELinux. Stage 11 grows this enforcing vendor policy from exact AVCs.
BOARD_VENDOR_SEPOLICY_DIRS += $(DEVICE_PATH)/sepolicy/vendor

# Kernel
BOARD_KERNEL_IMAGE_NAME := Image.gz-dtb
TARGET_KERNEL_ARCH := arm64
TARGET_KERNEL_HEADER_ARCH := arm64
TARGET_KERNEL_SOURCE := kernel/huawei/angler
TARGET_KERNEL_CONFIG := angler_defconfig
# Android 15 starts Bluetooth's main loop as SCHED_FIFO. Legacy RT cgroup
# bandwidth leaves the foreground cpuctl group with zero RT runtime, causing
# the Bluetooth stack to abort before it can reach the vendor HAL.
KERNEL_CONFIG_OVERRIDE := CONFIG_RT_GROUP_SCHED=n
TARGET_KERNEL_CLANG_COMPILE := false
TARGET_KERNEL_LLVM_BINUTILS := false

BOARD_KERNEL_BASE := 0x00000000
BOARD_KERNEL_PAGESIZE := 4096
BOARD_KERNEL_TAGS_OFFSET := 0x01E00000
BOARD_RAMDISK_OFFSET := 0x02000000
BOARD_KERNEL_CMDLINE := androidboot.hardware=angler androidboot.console=ttyHSL0 msm_rtb.filter=0x37 ehci-hcd.park=3 lpm_levels.sleep_disabled=1 boot_cpus=0-3 no_console_suspend loop.max_part=7
BOARD_MKBOOTIMG_ARGS := --ramdisk_offset $(BOARD_RAMDISK_OFFSET) --tags_offset $(BOARD_KERNEL_TAGS_OFFSET)

# Partitions: stock angler is non-A/B and has a separate recovery partition.
AB_OTA_UPDATER := false
TARGET_USERIMAGES_USE_EXT4 := true
BOARD_BOOTIMAGE_PARTITION_SIZE := 33554432
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 33554432
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 3221225472
BOARD_SYSTEMIMAGE_JOURNAL_SIZE := 0
BOARD_USERDATAIMAGE_PARTITION_SIZE := 26503790080
BOARD_CACHEIMAGE_PARTITION_SIZE := 104857600
BOARD_CACHEIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_VENDORIMAGE_PARTITION_SIZE := 209715200
BOARD_VENDORIMAGE_FILE_SYSTEM_TYPE := squashfs
BOARD_VENDORIMAGE_JOURNAL_SIZE := 0
BOARD_VENDORIMAGE_SQUASHFS_COMPRESSOR := lz4
BOARD_FLASH_BLOCK_SIZE := 131072
TARGET_COPY_OUT_VENDOR := vendor

# Recovery
TARGET_RECOVERY_FSTAB := device/huawei/angler/rootdir/etc/fstab.angler
TARGET_RELEASETOOLS_EXTENSIONS := device/huawei/angler
TARGET_RECOVERY_UI_LIB := librecovery_ui_nanohub
# Angler's bootloader relocates the recovery ramdisk to 0x02700000. Keep the
# compressed ramdisk below the 0x03400000 display reservation boundary.
TARGET_RECOVERY_DENSITY := xhdpi
BOARD_ROOT_EXTRA_FOLDERS := firmware persist

-include vendor/huawei/angler/BoardConfigVendor.mk
