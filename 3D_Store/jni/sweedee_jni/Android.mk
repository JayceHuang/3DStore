LOCAL_PATH 		:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE			:= superd_download_android
LOCAL_C_INCLUDES	:=  $(LOCAL_PATH)/../etm/xl_common/include \
						$(LOCAL_PATH)/../etm/embed_thunder_manager/include 
	
LOCAL_SRC_FILES += com_runmit_sweedee_downloadinterface_DownloadEngine.c

LOCAL_SHARED_LIBRARIES	:= superd_common embed_download embed_download_manager
LOCAL_LDLIBS += -llog

MY_CFLAGS := -MD -DLINUX -D_ANDROID_LINUX -DMOBILE_PHONE -DCLOUD_PLAY_PROJ

MY_EXTRA_CFLAGS := -D_LOGGER -D_LOGGER_ANDROID -DFor_Android -D_DEBUG -g 

#LOCAL_CFLAGS += $(MY_CFLAGS) $(MY_EXTRA_CFLAGS)
LOCAL_CFLAGS += $(MY_CFLAGS)

include $(BUILD_SHARED_LIBRARY)
