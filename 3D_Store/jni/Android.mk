LOCAL_PATH := $(call my-dir)
MY_LOCAL_PATH := $(LOCAL_PATH)

include $(MY_LOCAL_PATH)/etm/Android.mk
include $(MY_LOCAL_PATH)/sweedee_jni/Android.mk
include $(MY_LOCAL_PATH)/mediaserver_jni/Android.mk
include $(MY_LOCAL_PATH)/Android_prebuilt.mk
