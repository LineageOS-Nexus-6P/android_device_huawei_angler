# Copyright (C) 2015 The Android Open Source Project
# Copyright (C) 2026 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := fstab.angler
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES := etc/fstab.angler
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_ETC)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := init.angler.diag.rc
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES := etc/init.angler.diag.rc
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_ETC)/init/hw
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := init.angler.power.sh
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_SRC_FILES := bin/init.angler.power.sh
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_EXECUTABLES)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := init.angler.rc
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES := etc/init.angler.rc
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_ETC)/init/hw
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := init.angler.root.rc
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES := init.angler.rc
LOCAL_MODULE_STEM := init.angler.rc
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := init.angler.sensorhub.rc
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES := etc/init.angler.sensorhub.rc
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_ETC)/init/hw
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := init.angler.usb.rc
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES := etc/init.angler.usb.rc
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_ETC)/init/hw
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := init.recovery.angler.rc
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES := etc/init.recovery.angler.rc
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := ueventd.angler.rc
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES := etc/ueventd.angler.rc
LOCAL_MODULE_STEM := ueventd.rc
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_ETC)
include $(BUILD_PREBUILT)
