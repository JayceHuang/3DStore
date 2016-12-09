LOCAL_PATH 		:= $(call my-dir)

#mediaplayer
include $(CLEAR_VARS)
LOCAL_MODULE 			:= mediaserver
LOCAL_SRC_FILES := jni_lib/libmediaserver.so
include $(PREBUILT_SHARED_LIBRARY)

#liblocal
include $(CLEAR_VARS)
LOCAL_MODULE 			:= liblocal
LOCAL_SRC_FILES := jni_lib/liblocal.so
include $(PREBUILT_SHARED_LIBRARY)

#libmylib
include $(CLEAR_VARS)
LOCAL_MODULE 			:= libmylib
LOCAL_SRC_FILES := jni_lib/libmylib.so
include $(PREBUILT_SHARED_LIBRARY)