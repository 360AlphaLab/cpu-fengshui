LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := fengshui
LOCAL_SRC_FILES := src/fengshui.c
LOCAL_LDLIBS += -llog

include $(BUILD_EXECUTABLE)


