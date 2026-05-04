LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := mtk_spoof
LOCAL_SRC_FILES := main.cpp vulkan_hook.cpp dobby/dobby.cpp
LOCAL_C_INCLUDES := $(LOCAL_PATH)/dobby $(LOCAL_PATH)
LOCAL_LDLIBS := -llog -ldl
LOCAL_CPPFLAGS := -std=c++17 -Wall -Wextra
include $(BUILD_SHARED_LIBRARY)
