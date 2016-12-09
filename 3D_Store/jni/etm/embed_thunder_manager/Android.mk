LOCAL_PATH := $(call my-dir)

MY_C_INCLUDES := \
            $(LOCAL_PATH)/src/ \
            $(LOCAL_PATH)/../ooc/include \
            $(LOCAL_PATH)/../xl_common/include \
            $(LOCAL_PATH)/../embed_thunder/include \
			$(LOCAL_PATH)/../xml/include/ \
            $(LOCAL_PATH)/include \

MY_SRC_FILES :=\
            $(wildcard $(LOCAL_PATH)/src/*.c) \
            $(wildcard $(LOCAL_PATH)/src/download_manager/*.c) \
            $(wildcard $(LOCAL_PATH)/src/em_asyn_frame/*.c) \
            $(wildcard $(LOCAL_PATH)/src/em_common/*.c) \
	     $(wildcard $(LOCAL_PATH)/src/em_interface/*.c) \
            $(wildcard $(LOCAL_PATH)/src/et_interface/*.c) \
            $(wildcard $(LOCAL_PATH)/src/lixian/*.c) \
            $(wildcard $(LOCAL_PATH)/src/member/*.c) \
            $(wildcard $(LOCAL_PATH)/src/scheduler/*.c) \
            $(wildcard $(LOCAL_PATH)/src/settings/*.c) \
            $(wildcard $(LOCAL_PATH)/src/tree_manager/*.c) \
            $(wildcard $(LOCAL_PATH)/src/vod_task/*.c) 
			
MY_SRC_FILES := $(subst jni/etm/embed_thunder_manager/, ./, $(MY_SRC_FILES))

MY_CFLAGS := -MD -DLINUX -D_ANDROID_LINUX -DMOBILE_PHONE -DCLOUD_PLAY_PROJ -D_SUPPORT_TASK_INFO_EX

#release
#MY_EXTRA_CFLAGS := -O2
#MY_LDLIBS += -lz

#debug
MY_EXTRA_CFLAGS := -D_LOGGER -D_LOGGER_ANDROID -DFor_Android -D_DEBUG -g 
MY_LDLIBS += -lz -llog 

include $(CLEAR_VARS)
LOCAL_MODULE := embed_download_manager
LOCAL_C_INCLUDES := $(MY_C_INCLUDES)
LOCAL_SRC_FILES := $(MY_SRC_FILES)
#LOCAL_CFLAGS += $(MY_CFLAGS) $(MY_EXTRA_CFLAGS)
LOCAL_CFLAGS += $(MY_CFLAGS)
LOCAL_LDLIBS += $(MY_LDLIBS)
LOCAL_STATIC_LIBRARIES := ooc xml
LOCAL_SHARED_LIBRARIES := superd_common embed_download
include $(BUILD_SHARED_LIBRARY)
