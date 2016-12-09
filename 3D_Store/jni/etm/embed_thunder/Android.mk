LOCAL_PATH := $(call my-dir)

LOCAL_PATH := $(call my-dir)
$(warning $(LOCAL_PATH))
MY_C_INCLUDES := \
            $(LOCAL_PATH)/src/ \
            $(LOCAL_PATH)/../xl_common/include/ \
            $(LOCAL_PATH)/include/ 

MY_SRC_FILES := \
            $(wildcard $(LOCAL_PATH)/src/*.c) \
            $(wildcard $(LOCAL_PATH)/src/autonomous_lan/*.c) \
            $(wildcard $(LOCAL_PATH)/src/bt_download/bt_data_manager/*.c) \
            $(wildcard $(LOCAL_PATH)/src/bt_download/bt_data_pipe/*.c) \
            $(wildcard $(LOCAL_PATH)/src/bt_download/bt_magnet/*.c) \
            $(wildcard $(LOCAL_PATH)/src/bt_download/bt_task/*.c) \
            $(wildcard $(LOCAL_PATH)/src/bt_download/bt_utility/*.c) \
            $(wildcard $(LOCAL_PATH)/src/bt_download/torrent_parser/*.c) \
            $(wildcard $(LOCAL_PATH)/src/connect_manager/*.c) \
            $(wildcard $(LOCAL_PATH)/src/data_manager/*.c) \
            $(wildcard $(LOCAL_PATH)/src/dispatcher/*.c) \
            $(wildcard $(LOCAL_PATH)/src/download_task/*.c) \
            $(wildcard $(LOCAL_PATH)/src/drm/*.c) \
            $(wildcard $(LOCAL_PATH)/src/emule/*.c) \
            $(wildcard $(LOCAL_PATH)/src/emule/emule_data_manager/*.c) \
            $(wildcard $(LOCAL_PATH)/src/emule/emule_pipe/*.c) \
            $(wildcard $(LOCAL_PATH)/src/emule/emule_query/*.c) \
            $(wildcard $(LOCAL_PATH)/src/emule/emule_server/*.c) \
            $(wildcard $(LOCAL_PATH)/src/emule/emule_transfer_layer/*.c) \
            $(wildcard $(LOCAL_PATH)/src/emule/emule_utility/*.c) \
            $(wildcard $(LOCAL_PATH)/src/ftp_data_pipe/*.c) \
            $(wildcard $(LOCAL_PATH)/src/high_speed_channel/*.c) \
            $(wildcard $(LOCAL_PATH)/src/http_data_pipe/*.c) \
            $(wildcard $(LOCAL_PATH)/src/http_server/*.c) \
            $(wildcard $(LOCAL_PATH)/src/index_parser/*.c) \
            $(wildcard $(LOCAL_PATH)/src/p2p_data_pipe/*.c) \
            $(wildcard $(LOCAL_PATH)/src/p2p_transfer_layer/*.c) \
            $(wildcard $(LOCAL_PATH)/src/p2p_transfer_layer/tcp/*.c) \
            $(wildcard $(LOCAL_PATH)/src/p2p_transfer_layer/udt/*.c) \
            $(wildcard $(LOCAL_PATH)/src/p2sp_download/*.c) \
            $(wildcard $(LOCAL_PATH)/src/res_query/*.c) \
            $(wildcard $(LOCAL_PATH)/src/res_query/dk_res_query/*.c) \
            $(wildcard $(LOCAL_PATH)/src/rsa/*.c) \
            $(wildcard $(LOCAL_PATH)/src/cmd_proxy/*.c) \
            $(wildcard $(LOCAL_PATH)/src/task_manager/*.c) \
	     $(wildcard $(LOCAL_PATH)/src/socket_proxy/*.c)  \
	     $(wildcard $(LOCAL_PATH)/src/vod_data_manager/*.c) \
	     #$(wildcard $(LOCAL_PATH)/src/upload_manager/*.c) \
	     #$(wildcard $(LOCAL_PATH)/src/reporter/*.c) \

MY_SRC_FILES := $(subst jni/etm/embed_thunder/, ./, $(MY_SRC_FILES))

MY_CFLAGS := -MD -DLINUX -D_ANDROID_LINUX -DMOBILE_PHONE -DCLOUD_PLAY_PROJ 

#release 
#MY_EXTRA_CFLAGS := -O2 -mlong-calls
#MY_LDLIBS += -lz

#debug
MY_EXTRA_CFLAGS := -D_LOGGER -D_LOGGER_ANDROID -g -D_DEBUG
MY_LDLIBS += -lz -llog

include $(CLEAR_VARS)
LOCAL_MODULE := embed_download
LOCAL_C_INCLUDES := $(MY_C_INCLUDES) 
LOCAL_SRC_FILES := $(MY_SRC_FILES)
#LOCAL_CFLAGS += $(MY_CFLAGS) $(MY_EXTRA_CFLAGS)
LOCAL_CFLAGS += $(MY_CFLAGS)
LOCAL_LDLIBS += $(MY_LDLIBS)
LOCAL_SHARED_LIBRARIES := superd_common
#LOCAL_STATIC_LIBRARIES := crypto ssl
include $(BUILD_SHARED_LIBRARY)
