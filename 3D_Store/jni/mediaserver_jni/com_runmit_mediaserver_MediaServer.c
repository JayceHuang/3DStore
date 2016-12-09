#include "com_runmit_mediaserver_MediaServer.h"
#include <stddef.h>
#include "media_server.h"
#include <malloc.h>
//#include "logger.h"



enum VODTASKINFO_FIELD_ID_TYPE
{
	mPeerInfo,
	mServerInfo,
	mCdnInfo,
	mTotalRecvSize,
	mTotalDwonloadSize,
	mDownloadTime,
	mTaskStatus,
	VODTASKINFO_FIELD_ID_SIZE
};


enum RESOURCEINFO_FIELD_ID_TYPE
{
	mTotalNum,
	mValidNum,
	mConnectedPipeNum,
	mDownloadSpeed,
	mDownloadSize,
	mRecvSize,
	RESOURCEINFO_FIELD_ID_SIZE
};

static jfieldID g_vod_task_info_field_id;
static jfieldID g_vodtaskinfo_field_id_array[VODTASKINFO_FIELD_ID_SIZE];
static jfieldID g_resourceinfo_field_id_array[RESOURCEINFO_FIELD_ID_SIZE];




/*
 * Class:     com_xunlei_mediaserver_MediaServer
 * Method:    init
 * Signature: (ILjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_runmit_mediaserver_MediaServer_init
  (JNIEnv *env, jclass jClass, jint appid, jint port, jstring jRootPath)//, jint screenw, jint sectionType, jstring jConfigPath, jstring jCachePath)
{
  int result = -1;
  jclass class = NULL;
  g_vod_task_info_field_id = NULL;
  memset(g_vodtaskinfo_field_id_array, 0, sizeof(g_vodtaskinfo_field_id_array));
  memset(g_resourceinfo_field_id_array, 0, sizeof(g_resourceinfo_field_id_array));

  const char *root_path = (*env)->GetStringUTFChars(env, jRootPath, NULL);
  result = media_server_init(appid, port, root_path);//, screenw, sectionType);
  (*env)->ReleaseStringUTFChars(env, jRootPath, root_path);
  return result;
}

JNIEXPORT jint JNICALL Java_com_runmit_mediaserver_MediaServer_setLocalInfo
  (JNIEnv *env, jclass jClass, jstring jImei, jstring jDeviceType, jstring JOsVersion, jstring jNetType)
 {
 	 const char * imei = (*env)->GetStringUTFChars(env, jImei, NULL);
 	 const char * device_type = (*env)->GetStringUTFChars(env, jDeviceType, NULL);
 	 const char * os_version = (*env)->GetStringUTFChars(env, JOsVersion, NULL);
 	 const char * net_type = (*env)->GetStringUTFChars(env, jNetType, NULL);

 	 int ret = media_server_set_local_info(imei, imei, device_type, os_version, net_type);

 	 (*env)->ReleaseStringUTFChars(env, jImei, imei);
 	 (*env)->ReleaseStringUTFChars(env, jDeviceType, device_type);
 	 (*env)->ReleaseStringUTFChars(env, JOsVersion, os_version);
 	 (*env)->ReleaseStringUTFChars(env, jNetType, net_type);

 	 return ret;
 }



/*
 * Class:     com_xunlei_mediaserver_MediaServer
 * Method:    fini
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_runmit_mediaserver_MediaServer_uninit
  (JNIEnv *env, jclass jClass)
{
  media_server_uninit();
}

/*
 * Class:     com_xunlei_mediaserver_MediaServer
 * Method:    run
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_runmit_mediaserver_MediaServer_run
  (JNIEnv *env, jclass jClass)
{
  media_server_run();
}

/*
 * Class:     com_xunlei_mediaserver_MediaServer
 * Method:    getHttpListenPort
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_runmit_mediaserver_MediaServer_getHttpListenPort
  (JNIEnv *env, jclass jClass)
{
  return media_server_get_http_listen_port();
}
