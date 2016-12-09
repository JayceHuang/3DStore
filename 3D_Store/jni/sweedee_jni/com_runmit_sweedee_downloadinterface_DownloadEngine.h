/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_runmit_tdstore_downloadinterface_DownloadEngine */

#ifndef _Included_com_runmit_tdstore_downloadinterface_DownloadEngine
#define _Included_com_runmit_tdstore_downloadinterface_DownloadEngine
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     com_runmit_tdstore_downloadinterface_DownloadEngine
 * Method:    setDownloadLimitSpeed
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_runmit_sweedee_downloadinterface_DownloadEngine_setDownloadLimitSpeed
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_runmit_tdstore_downloadinterface_DownloadEngine
 * Method:    getDownloadLimitSpeed
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_runmit_sweedee_downloadinterface_DownloadEngine_getDownloadLimitSpeed
  (JNIEnv *, jobject);

/*
 * Class:     com_runmit_tdstore_downloadinterface_DownloadEngine
 * Method:    getDownloadPath
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_runmit_sweedee_downloadinterface_DownloadEngine_getDownloadPath
  (JNIEnv *, jobject);

/*
 * Class:     com_runmit_tdstore_downloadinterface_DownloadEngine
 * Method:    setDownloadPath
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_runmit_sweedee_downloadinterface_DownloadEngine_setDownloadPath
  (JNIEnv *, jobject, jstring);

/*
 * Class:     com_runmit_tdstore_downloadinterface_DownloadEngine
 * Method:    getTaskInfoByTaskId
 * Signature: (J)Lcom/runmit/tdstore/model/TaskInfo;
 */
JNIEXPORT jobject JNICALL Java_com_runmit_sweedee_downloadinterface_DownloadEngine_getTaskInfoByTaskId
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_runmit_tdstore_downloadinterface_DownloadEngine
 * Method:    getAllLocalTasks
 * Signature: ()I
 */
JNIEXPORT jobjectArray JNICALL Java_com_runmit_sweedee_downloadinterface_DownloadEngine_getAllLocalTasks
  (JNIEnv *, jobject);

/*
 * Class:     com_runmit_tdstore_downloadinterface_DownloadEngine
 * Method:    setMaxDownloadTasks
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_runmit_sweedee_downloadinterface_DownloadEngine_setMaxDownloadTasks
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_runmit_tdstore_downloadinterface_DownloadEngine
 * Method:    createTaskByUrl
 * Signature: (IIILjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_runmit_sweedee_downloadinterface_DownloadEngine_createTaskByUrl
  (JNIEnv *, jobject, jint, jint, jint, jstring, jstring, jstring, jstring, jlong, jstring ,jstring );

/*
 * Class:     com_runmit_sweedee_downloadinterface_DownloadEngine
 * Method:    addTaskUrl
 * Signature: (ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_runmit_sweedee_downloadinterface_DownloadEngine_addTaskUrl
  (JNIEnv *, jobject, jint, jstring, jstring, jstring);

/*
 * Class:     com_runmit_tdstore_downloadinterface_DownloadEngine
 * Method:    pauseTask
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_runmit_sweedee_downloadinterface_DownloadEngine_pauseTask
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_runmit_tdstore_downloadinterface_DownloadEngine
 * Method:    resumeTask
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_runmit_sweedee_downloadinterface_DownloadEngine_resumeTask
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_runmit_tdstore_downloadinterface_DownloadEngine
 * Method:    deleteTask
 * Signature: (IZ)I
 */
JNIEXPORT jint JNICALL Java_com_runmit_sweedee_downloadinterface_DownloadEngine_deleteTask
  (JNIEnv *, jobject, jint, jboolean);

/*
 * Class:     Java_com_runmit_sweedee_downloadinterface_DownloadEngine
 * Method:    initEtm
 * Signature: (Lcom/xunlei/cloud/service/DownloadEngine;Ljava/lang/String;Ljava/lang/String;IILjava/lang/String;Ljava/lang/String;II)I
 */
JNIEXPORT jint JNICALL Java_com_runmit_sweedee_downloadinterface_DownloadEngine_initEtm
  (JNIEnv *, jobject, jobject, jstring, jstring, jint, jint, jstring, jstring, jint, jint);

/*
 * Class:     com_xunlei_cloud_service_DownloadEngine
 * Method:    setNetType
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_runmit_sweedee_downloadinterface_DownloadEngine_setNetType
  (JNIEnv *, jobject, jint);

JNIEXPORT void JNICALL Java_com_runmit_sweedee_downloadinterface_DownloadEngine_sendMessageHandler
	(JNIEnv *, jobject, jint, jint);

JNIEXPORT jint JNICALL Java_com_runmit_sweedee_downloadinterface_DownloadEngine_createTaskByFile
	(JNIEnv *, jobject, jstring,jstring,jlong);

JNIEXPORT jint JNICALL Java_com_runmit_sweedee_downloadinterface_DownloadEngine_createTaskByCfg
	(JNIEnv *, jobject, jstring);

#ifdef __cplusplus
}
#endif
#endif
