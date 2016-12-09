LOCAL_PATH:= $(call my-dir)

MY_C_INCLUDES := $(LOCAL_PATH)/include
MY_SRC_FILES := $(wildcard $(LOCAL_PATH)/src/asyn_frame/*.c) \
                $(wildcard $(LOCAL_PATH)/src/utility/*.c) \
                $(wildcard $(LOCAL_PATH)/src/platform/*.c) \
                $(wildcard $(LOCAL_PATH)/src/platform/android_interface/*.c) \
                $(wildcard $(LOCAL_PATH)/src/torrent_parser/*.c) \
				$(wildcard $(LOCAL_PATH)/src/rsa/*.c)

MY_SRC_FILES := $(subst jni/etm/xl_common/, ./, $(MY_SRC_FILES))

FILTER_OUT_FIELS := %sd_android_asset_files.c

MY_SRC_FILES := $(filter-out $(FILTER_OUT_FIELS), $(MY_SRC_FILES))

MY_CFLAGS := -MD -DLINUX -D_ANDROID_LINUX -DMOBILE_PHONE -DCLOUD_PLAY_PROJ

#release
#MY_EXTRA_CFLAGS := -O2 -mlong-calls
#MY_LDLIBS += -lz

#debug
MY_EXTRA_CFLAGS := -D_LOGGER -D_LOGGER_ANDROID -g -D_DEBUG 
MY_LDLIBS += -lz -llog

include $(CLEAR_VARS)
LOCAL_MODULE := superd_common
LOCAL_SRC_FILES := $(MY_SRC_FILES)
#LOCAL_CFLAGS += $(MY_CFLAGS) $(MY_EXTRA_CFLAGS)
LOCAL_CFLAGS += $(MY_CFLAGS)
LOCAL_C_INCLUDES += $(MY_C_INCLUDES)
LOCAL_LDLIBS += $(MY_LDLIBS)
#LOCAL_STATIC_LIBRARIES := crypto
include $(BUILD_SHARED_LIBRARY)
