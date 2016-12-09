#include <jni.h>
#include "com_runmit_sweedee_downloadinterface_DownloadEngine.h"
#include "et_task_manager.h"
#include <android/log.h>
#include <signal.h>
#include <sys/types.h>
#include "platform/android_interface/sd_android_log.h"
#include "utility/define.h"
#include "utility/sd_assert.h"
#include "utility/errcode.h"
#include "platform/android_interface/sd_android_utility.h"
#include "platform/sd_fs.h"

typedef _int32 (*SWITCH_TO_UI_THREAD_HANDLER)(void * user_data);
static void post_event(SWITCH_TO_UI_THREAD_HANDLER callback, void* user_data);

#define MAX_URL_LENTH 1024
#define LOG_TAG "HTC_SHARE"

jobject gEngine;

typedef struct create_task_userdata {
	char cookie[MAX_URL_LENTH];
	char url[MAX_URL_LENTH];
	char poster[MAX_URL_LENTH]; //图片地址
	char subtitleJson[MAX_URL_LENTH];//
	_int32 albumId;
	_int32 videoId;
	_int32 mode;
} Create_Task_Userdata;

typedef struct tagTask_State_change
{
	u32 _task_id;
	ETM_TASK_INFO _task_info;
}Task_State_change;

JNIEXPORT jint JNICALL Java_com_runmit_sweedee_downloadinterface_DownloadEngine_setDownloadLimitSpeed(
		JNIEnv *env, jobject obj, jint speed) {
	ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "function %s begin", __FUNCTION__);
	int ret = SUCCESS;
	ret = etm_set_download_limit_speed(speed);
	ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "function %s end ret = %d", __FUNCTION__, ret);
	return ret;
}

JNIEXPORT jint JNICALL Java_com_runmit_sweedee_downloadinterface_DownloadEngine_getDownloadLimitSpeed(
		JNIEnv *env, jobject obj) {
	int limit_speed = etm_get_download_limit_speed();
	return limit_speed;
}

JNIEXPORT jstring JNICALL Java_com_runmit_sweedee_downloadinterface_DownloadEngine_getDownloadPath(
		JNIEnv *env, jobject obj) {
	const char *path = NULL;
	ANDROID_LOG(ANDROID_LOG_DEBUG,LOG_TAG,"function %s begin",__FUNCTION__);
	path = etm_get_download_path();
	if (NULL == path) {
		return NULL;
	} ANDROID_LOG(ANDROID_LOG_DEBUG,LOG_TAG,"function %s end",__FUNCTION__);
	return (*env)->NewStringUTF(env, path);
}

JNIEXPORT jint JNICALL Java_com_runmit_sweedee_downloadinterface_DownloadEngine_setNetType
  (JNIEnv *env, jobject obj, jint nettype)
{
	jint ret = 0;
	ret = etm_set_net_type(nettype);
	return ret;
}

JNIEXPORT jint JNICALL Java_com_runmit_sweedee_downloadinterface_DownloadEngine_setDownloadPath(
		JNIEnv *env, jobject obj, jstring path_str) {
	const char *path = NULL;
	jint ret = 0;
	ANDROID_LOG(ANDROID_LOG_DEBUG,LOG_TAG,"function %s begin",__FUNCTION__);
	path = (*env)->GetStringUTFChars(env, path_str, NULL);
	if (NULL == path) {
		return -1;
	}
	ret = sd_ensure_path_exist(path);
	if (0 != ret) {

		return ret;
	}
	ret = etm_set_download_path(path, sd_strlen(path));
	if (0 != ret) {
		return ret;
	}
	(*env)->ReleaseStringUTFChars(env, path_str, path);
	ANDROID_LOG(ANDROID_LOG_DEBUG,LOG_TAG,"function %s end",__FUNCTION__);
	return 0;
}

JNIEXPORT jobject JNICALL Java_com_runmit_sweedee_downloadinterface_DownloadEngine_getTaskInfoByTaskId
(JNIEnv *env, jobject obj, jint task_id)
{
	jint ret = 0;
	jfieldID field_id = 0;
	jclass  cls = NULL;
	jobject object = NULL;
	jmethodID method_id = 0;
	ETM_RUNNING_STATUS task_info;
	sd_memset(&task_info, 0, sizeof(ETM_RUNNING_STATUS));

	ANDROID_LOG(ANDROID_LOG_DEBUG,LOG_TAG,"function %s begin",__FUNCTION__);
	ANDROID_LOG(ANDROID_LOG_ERROR,LOG_TAG,"In function %s, task_id = %llu",__FUNCTION__,task_id);
	ret = etm_get_task_running_status(task_id, &task_info);
	if(0 != ret)
	{
		ANDROID_LOG(ANDROID_LOG_ERROR,LOG_TAG,"function etm_get_task_running_status, ret = %d",ret);
		return NULL;
	}
	cls = (*env)->FindClass(env,"com/runmit/sweedee/model/TaskInfo");
	method_id = (*env)->GetMethodID(env,cls, "<init>", "()V");
	object = (*env)->NewObject(env,cls, method_id);

	field_id = (*env)->GetFieldID(env, cls,"mTaskId","I");
	if(NULL == field_id)
	{
		ANDROID_LOG(ANDROID_LOG_ERROR,LOG_TAG,"mTaskId field_id == null");
		return NULL;
	}
	 (*env)->SetIntField(env, object,field_id,task_id);

	field_id = (*env)->GetFieldID(env, cls,"mTaskState","I");
	if(NULL == field_id)
	{
		ANDROID_LOG(ANDROID_LOG_ERROR,LOG_TAG,"mTaskState field_id == null");
		return NULL;
	}
	(*env)->SetIntField(env, object,field_id,task_info._state);
	ANDROID_LOG(ANDROID_LOG_ERROR,LOG_TAG,"In function %s, task_info._state = %d",__FUNCTION__,task_info._state);

	field_id = (*env)->GetFieldID(env, cls,"downloadedSize","J");
	if(NULL == field_id)
	{
		ANDROID_LOG(ANDROID_LOG_ERROR,LOG_TAG,"downloadedSize field_id == null");
		return NULL;
	}
	(*env)->SetLongField(env, object,field_id,task_info._downloaded_data_size);
	ANDROID_LOG(ANDROID_LOG_ERROR,LOG_TAG,"In function %s, task_info._downloaded_data_size = %llu",__FUNCTION__,task_info._downloaded_data_size);
	field_id = (*env)->GetFieldID(env, cls,"fileSize","J");
	if(NULL == field_id)
	{
		ANDROID_LOG(ANDROID_LOG_ERROR,LOG_TAG,"fileSize field_id == null");
		return NULL;
	}
	(*env)->SetLongField(env, object,field_id,task_info._file_size);
	ANDROID_LOG(ANDROID_LOG_ERROR,LOG_TAG,"In function %s, task_info._dl_speed = %u",__FUNCTION__,task_info._dl_speed);
	field_id = (*env)->GetFieldID(env, cls,"mDownloadSpeed","I");
	if(NULL == field_id)
	{
		ANDROID_LOG(ANDROID_LOG_ERROR,LOG_TAG,"mDownloadSpeed field_id == null");
		return NULL;
	}
	(*env)->SetIntField(env, object,field_id,task_info._dl_speed);
	ANDROID_LOG(ANDROID_LOG_ERROR,LOG_TAG,"In function %s, task_info._file_size = %llu",__FUNCTION__,task_info._file_size);
	ANDROID_LOG(ANDROID_LOG_DEBUG,LOG_TAG,"function %s end",__FUNCTION__);
	(*env)->DeleteLocalRef(env, cls);

	return object;
}

JNIEXPORT jobjectArray JNICALL Java_com_runmit_sweedee_downloadinterface_DownloadEngine_getAllLocalTasks(
		JNIEnv *env, jobject obj) {
	jint ret = 0;
	_u32 albumId;
	_u32 videoId;
	_u32 mode;
	_u32 task_id_array[512] = { 0 };
	_u32 id_array_size = 512;
	_u32 index = 0;
	_u32 task_id = 0;
	_u32 task_state = 0;
	_u64 downloaded_size = 0;
	_u64 file_size = 0;
	_u32 start_time = 0;
	_u32 finished_time = 0;
	_u32 task_fail_code = 0;

	jstring file_name_str = NULL;
	jstring file_path_str = NULL;
	jstring file_url_str = NULL;
	jstring file_poster = NULL;
	jstring subtitlejsonsting = NULL ;
	jclass cls = NULL;

	jobjectArray  objectArray = NULL;
	jobject mJavaTaskInfo = NULL;
	jclass javaClass = NULL;
	jmethodID methodId = NULL;

	javaClass = (*env)->FindClass(env,"com/runmit/sweedee/model/TaskInfo");
	if (javaClass == NULL) {
		ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG,"In function %s, and not find class TaskInfo",__FUNCTION__);
		return NULL;
	}
	methodId = (*env)->GetMethodID(env, javaClass, "<init>","(Ljava/lang/String;IILjava/lang/String;Ljava/lang/String;Ljava/lang/String;JJLjava/lang/String;IIIIII)V");
	if (methodId == NULL) {
		ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG,"In function %s, is not init method", __FUNCTION__);
		return NULL;
	}

	ETM_TASK_INFO task_info;
	sd_memset(&task_info, 0, sizeof(ETM_TASK_INFO));

	ret = etm_get_all_task_ids(task_id_array, &id_array_size);
	ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "In function %s, etm_get_all_task_ids  ret = %d id_array_size= %d", __FUNCTION__, ret,id_array_size);
	if (ret == 0) {
		if (id_array_size == 0) {
			ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "In function %s, etm_get_all_task_ids: no task ", __FUNCTION__);
		} else {
			ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "In function %s, etm_get_all_task_ids:%u tasks:\n", __FUNCTION__, id_array_size);
			objectArray = (*env)->NewObjectArray(env, id_array_size, javaClass, NULL);
			if (objectArray == NULL) {
				ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG,"In function %s, new Objectarray is failed", __FUNCTION__);
				return NULL;
			}
			_u32 len = 0;
			for (index = 0; index < id_array_size; index++) {
				task_id = task_id_array[index];
				len = 0;
				ret = etm_get_task_user_data(task_id, NULL, &len);

				if (len != 0) {
					Create_Task_Userdata* pUserData;
					ret = sd_malloc(len, (void**) &pUserData);
					if (ret != 0) {
						return -1;
					}
					ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "In function %s, before etm_get_task_user_data len is %d \n", __FUNCTION__, len);
					ret = etm_get_task_user_data(task_id, (void*) pUserData,&len);

					ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "In function %s, etm_get_task_user_data failed! ret is %d\n", __FUNCTION__, ret);
					if (0 != ret) {
						ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "In function %s, etm_get_task_user_data failed! ret is %d\n",__FUNCTION__, ret);
					} else {
						ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "In function %s, taskid is %u\n",__FUNCTION__, task_id); ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "In function %s, cookie is %s\n",__FUNCTION__, pUserData->cookie); ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "In function %s, url is %s\n",__FUNCTION__, pUserData->url);

						const char *pUrl = NULL;
						pUrl = etm_get_task_url(task_id);
						if (NULL != pUrl) {
							file_url_str = (*env)->NewStringUTF(env, pUrl);
						}
						ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "In function %s, url is %s\n", __FUNCTION__, pUrl);

						const char* pPoster = pUserData->poster;
						if (NULL != pPoster) {
							file_poster = (*env)->NewStringUTF(env, pPoster);
						}

						const char* pSubtitleJson = pUserData->subtitleJson;
						if (NULL != pSubtitleJson) {
							subtitlejsonsting = (*env)->NewStringUTF(env, pSubtitleJson);
						}
						ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "In function %s, pSubtitleJson is %s \n", __FUNCTION__, pSubtitleJson);

						albumId = pUserData->albumId;
						mode = pUserData->mode;
						videoId = pUserData->videoId;
					}
					sd_free(pUserData);
				}

				ret = etm_get_task_download_info(task_id, &task_info);
				ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "In function %s, task_id_array[%u] = %u, task_info._state = %d\n", __FUNCTION__, index, task_id, task_info._state);

				task_state = task_info._state;
				file_name_str = (*env)->NewStringUTF(env, task_info._file_name);
				file_path_str = (*env)->NewStringUTF(env, task_info._file_path);

				downloaded_size = task_info._downloaded_data_size;
				file_size = task_info._file_size;
				task_fail_code = task_info._failed_code;
				start_time = task_info._start_time;
				finished_time = task_info._finished_time;

				ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "In function %s, task_fail_code = %u, task_state = %d\n", __FUNCTION__, task_fail_code, task_state);
				mJavaTaskInfo = (*env)->NewObject(env, javaClass, methodId, subtitlejsonsting,task_id, task_state,
						file_name_str, file_path_str, file_url_str,
						downloaded_size, file_size,
						file_poster, task_fail_code, start_time, finished_time,albumId,mode,videoId);

				(*env)->SetObjectArrayElement(env, objectArray, index, mJavaTaskInfo);

				(*env)->DeleteLocalRef(env, file_name_str);
				(*env)->DeleteLocalRef(env, file_path_str);
				(*env)->DeleteLocalRef(env, file_url_str);
				(*env)->DeleteLocalRef(env, file_poster);
				(*env)->DeleteLocalRef(env, subtitlejsonsting);
				(*env)->DeleteLocalRef(env, mJavaTaskInfo);
			}
		}

	}
	(*env)->DeleteLocalRef(env, javaClass);

	ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "In function %s, etm_get_all_task_ids:%u tasks:\n", __FUNCTION__, id_array_size);
	return objectArray;
}

JNIEXPORT jint JNICALL Java_com_runmit_sweedee_downloadinterface_DownloadEngine_setMaxDownloadTasks(
		JNIEnv *env, jobject obj, jint num) {
	jint ret = 0;
	ret = etm_set_max_tasks(num);
	return ret;
}

JNIEXPORT jint JNICALL Java_com_runmit_sweedee_downloadinterface_DownloadEngine_createTaskByUrl(JNIEnv *env, jobject obj, jint videoId, jint mode,jint albumId,
jstring url, jstring filePath,jstring file_name, jstring cookie, jlong filesize, jstring poster,jstring subtitleJson) {
	ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "function %s begin", __FUNCTION__);
	_int32 ret = SUCCESS;
	_u32 task_id = 0;
	const char* pUrl = NULL;
	const char* pFilePath = NULL;
	const char* pFileName = NULL;
	const char* pCookie = NULL;
	const char* pPoster = NULL;
	const char* pSubtitleJson = NULL;

	jfieldID field_id = 0;
	jfieldID fileNameField_id = NULL;
	jclass cls = NULL;

	ETM_CREATE_TASK task_info;
	sd_memset(&task_info, 0, sizeof(task_info));

	pUrl = (char*) (*env)->GetStringUTFChars(env, url, NULL);
	ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "In function %s, pUrl = %s", __FUNCTION__, pUrl);
	if (NULL == pUrl) {
		return -1;
	}

	pFilePath = (char*) (*env)->GetStringUTFChars(env, filePath, NULL);
	ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "In function %s, pFilePath = %s", __FUNCTION__, pFilePath);
	if (NULL != pFilePath) {
		ret = sd_ensure_path_exist(pFilePath);
		ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "In function %s , sd_ensure_path_exist ret = %d", __FUNCTION__, ret);
		if (0 != ret) {
			(*env)->ReleaseStringUTFChars(env, url, pUrl);
			return ret;
		}
		task_info._file_path = pFilePath;
		task_info._file_path_len = sd_strlen(pFilePath);
	} else {
		(*env)->ReleaseStringUTFChars(env, url, pUrl);
		return -1;
	}

	Create_Task_Userdata userdata;
	sd_memset(&userdata, 0, sizeof(userdata));

	char buffer[512] = { 0 };
	u32 len = 512;
	pFileName = (char*) (*env)->GetStringUTFChars(env, file_name, NULL);
	ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "In function %s, pFileName = %s", __FUNCTION__, pFileName);
	if (pFileName == NULL) {
		(*env)->ReleaseStringUTFChars(env, url, pUrl);
		(*env)->ReleaseStringUTFChars(env, filePath, pFilePath);
		return -1;
	}
	task_info._file_name = pFileName;
	task_info._file_name_len = sd_strlen(pFileName);

	pCookie = (char*) (*env)->GetStringUTFChars(env, cookie, NULL);
	pPoster = (char*) (*env)->GetStringUTFChars(env, poster, NULL);
	pSubtitleJson= (char*) (*env)->GetStringUTFChars(env, subtitleJson, NULL);

	ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "In function %s , pSubtitleJson = %s and albumId= %d", __FUNCTION__, pSubtitleJson,albumId);

	userdata.albumId = albumId;
	userdata.mode = mode;
	userdata.videoId = videoId;
	if(NULL != pPoster) {
		sd_strncpy(userdata.poster, pPoster, sd_strlen(pPoster) + 1);
	}
	if(NULL != pSubtitleJson){
		sd_strncpy(userdata.subtitleJson, pSubtitleJson, sd_strlen(pSubtitleJson) + 1);
	}
	if (NULL != pCookie) {
		sd_strncpy(userdata.cookie, pCookie, sd_strlen(pCookie) + 1);
	}
	if (NULL != pUrl) {
		sd_strncpy(userdata.url, pUrl, sd_strlen(pUrl) + 1);
	}

	task_info._file_size = filesize;
	task_info._user_data = &userdata;
	task_info._user_data_len = sizeof(userdata);
	ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "In function %s, size of userdata is   %d", __FUNCTION__, sizeof(userdata));

	ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "function %s ,pFileName = %s", __FUNCTION__, pFileName);
	if (sd_strncmp(pUrl, "ed2k://", strlen("ed2k://")) == 0) {
		task_info._type = ETT_EMULE;
	} else {
		task_info._type = ETT_URL;
	}
	task_info._url = pUrl;
	task_info._url_len = sd_strlen(pUrl);

	ret = etm_create_task(&task_info, &task_id);

	cls = (*env)->GetObjectClass(env, obj);
	jstring nameForId = NULL;
	nameForId = (*env)->NewStringUTF(env, task_info._file_name);
	fileNameField_id = (*env)->GetFieldID(env, cls, "fileName","Ljava/lang/String;");
	if (NULL == nameForId || fileNameField_id == NULL) {
		return -1;
	}
	(*env)->SetObjectField(env, obj, fileNameField_id, nameForId);
	(*env)->DeleteLocalRef(env, nameForId);

	ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "function %s ,etm_create_task,ret = %d", __FUNCTION__, ret);
	if (0 != ret) {
		(*env)->ReleaseStringUTFChars(env, url, pUrl);
		(*env)->ReleaseStringUTFChars(env, filePath, pFilePath);
		(*env)->ReleaseStringUTFChars(env, file_name, pFileName);
		(*env)->ReleaseStringUTFChars(env, cookie, pCookie);
		(*env)->ReleaseStringUTFChars(env, poster, pPoster);
		(*env)->ReleaseStringUTFChars(env, subtitleJson, pSubtitleJson);
		return ret;
	}

	ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "function %s ,task_id = %u", __FUNCTION__, task_id);
	field_id = (*env)->GetFieldID(env, cls, "mTaskId", "J");
	if (NULL == field_id) {
		(*env)->ReleaseStringUTFChars(env, url, pUrl);
		(*env)->ReleaseStringUTFChars(env, filePath, pFilePath);
		(*env)->ReleaseStringUTFChars(env, file_name, pFileName);
		(*env)->ReleaseStringUTFChars(env, cookie, pCookie);
		(*env)->ReleaseStringUTFChars(env, poster, pPoster);
		(*env)->ReleaseStringUTFChars(env, subtitleJson, pSubtitleJson);
		return -1;
	}
	(*env)->SetLongField(env, obj, field_id, task_id);
	(*env)->ReleaseStringUTFChars(env, url, pUrl);
	(*env)->ReleaseStringUTFChars(env, filePath, pFilePath);
	(*env)->ReleaseStringUTFChars(env, file_name, pFileName);
	(*env)->ReleaseStringUTFChars(env, cookie, pCookie);
	(*env)->ReleaseStringUTFChars(env, poster, pPoster);
	(*env)->ReleaseStringUTFChars(env, subtitleJson, pSubtitleJson);
	ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "function %s end", __FUNCTION__);
	return ret;
}

JNIEXPORT jint JNICALL Java_com_runmit_sweedee_downloadinterface_DownloadEngine_addTaskUrl(JNIEnv *env, jobject obj, jint task_id, jstring url, jstring cookie, jstring ref_url) {
  	jint ret = 0;
  	char *pUrl = NULL;
  	char *pRefUrl = NULL;
  	char *pCookie = NULL;

  	pUrl = (char*) (*env)->GetStringUTFChars(env, url, NULL);
	
	if (NULL == pUrl) {
		ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "In function %s, url null.", __FUNCTION__);
		return -1;
	}

	pRefUrl = (char*) (*env)->GetStringUTFChars(env, ref_url, NULL);
	if (NULL == pUrl) {
		(*env)->ReleaseStringUTFChars(env, url, pUrl);
		ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "In function %s, ref url null.", __FUNCTION__);
		return -1;
	}

	pCookie = (char*) (*env)->GetStringUTFChars(env, cookie, NULL);
	if (NULL == pUrl) {
		(*env)->ReleaseStringUTFChars(env, url, pUrl);
		(*env)->ReleaseStringUTFChars(env, ref_url, pRefUrl);
		ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "In function %s, pUrl = %s", __FUNCTION__, pUrl);
		return -1;
	}
	ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "In function %s, pUrl = %s, pRefUrl = %s, pCookie = %s", __FUNCTION__, pUrl, pRefUrl, pCookie);

	ETM_SERVER_RES etm_res;
	sd_memset(&etm_res, 0, sizeof(etm_res));

	etm_res._resource_priority = 1;
	etm_res._url = pUrl;
	etm_res._url_len = sd_strlen(pUrl);

	etm_res._ref_url = pRefUrl;
	etm_res._ref_url_len = sd_strlen(pRefUrl);

	etm_res._cookie = pCookie;
	etm_res._cookie_len = sd_strlen(pCookie);

	ret = etm_add_server_resource(task_id, &etm_res);

	if(ret != 0){
		(*env)->ReleaseStringUTFChars(env, url, pUrl);
		(*env)->ReleaseStringUTFChars(env, ref_url, pRefUrl);
		(*env)->ReleaseStringUTFChars(env, cookie, pCookie);
		ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "function %s, ret = %d", __FUNCTION__, ret);
		return ret;
	}

	ret = etm_resume_task(task_id);

	(*env)->ReleaseStringUTFChars(env, url, pUrl);
	(*env)->ReleaseStringUTFChars(env, ref_url, pRefUrl);
	(*env)->ReleaseStringUTFChars(env, cookie, pCookie);

  	ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "function %s, ret = %d", __FUNCTION__, ret);
  	return ret;
}

JNIEXPORT jint JNICALL Java_com_runmit_sweedee_downloadinterface_DownloadEngine_pauseTask(
		JNIEnv *env, jobject obj, jint task_id) {
	jint ret = 0;
	ANDROID_LOG(ANDROID_LOG_ERROR,LOG_TAG,"function %s begin",__FUNCTION__);
	ret = etm_pause_task(task_id);
	if (0 != ret) {
		ANDROID_LOG(ANDROID_LOG_ERROR,LOG_TAG,"function etm_pause_task, ret = %d",ret);
		return ret;
	} ANDROID_LOG(ANDROID_LOG_ERROR,LOG_TAG,"function %s end",__FUNCTION__);
	return 0;
}

JNIEXPORT jint JNICALL Java_com_runmit_sweedee_downloadinterface_DownloadEngine_resumeTask(
		JNIEnv *env, jobject obj, jint task_id) {
	jint ret = 0;
	ANDROID_LOG(ANDROID_LOG_ERROR,LOG_TAG,"function %s begin",__FUNCTION__);
	ret = etm_resume_task(task_id);
	if (0 != ret) {
		ANDROID_LOG(ANDROID_LOG_ERROR,LOG_TAG,"function etm_resume_task, ret = %d",ret);
		return ret;
	} ANDROID_LOG(ANDROID_LOG_ERROR,LOG_TAG,"function %s end",__FUNCTION__);
	return 0;
}

JNIEXPORT jint JNICALL Java_com_runmit_sweedee_downloadinterface_DownloadEngine_deleteTask(
		JNIEnv *env, jobject obj, jint task_id, jboolean is_delete_file) {
	jint ret = 0;
	ANDROID_LOG(ANDROID_LOG_DEBUG,LOG_TAG,"function %s begin",__FUNCTION__);
	ret = etm_destroy_task(task_id, is_delete_file);
	ANDROID_LOG(ANDROID_LOG_DEBUG,LOG_TAG,"function %s end ret=",__FUNCTION__, ret);
	return ret;
}

//通知上层任务状态改变的回调
_int32 ui_on_task_state_changed_notify(void* user_data)
{
	u32 task_id = 0;
	ETM_TASK_INFO * p_task_info = NULL;
	Task_State_change* data = NULL;
	JNIEnv *env = NULL;
	JavaVM * vm = NULL;
	_int32 status = 0;
	JavaParm * java_param = NULL;
	jobject obj = NULL;
	jclass Activity_class = NULL;
	jmethodID method_id = 0;
	int arr = 0;

	ANDROID_LOG(ANDROID_LOG_DEBUG,LOG_TAG,"function %s begin",__FUNCTION__);
	data = (Task_State_change*)user_data;
	task_id = data->_task_id;
	p_task_info = &data->_task_info;

	jstring file_name_str = NULL;
	jstring file_path_str = NULL;

	java_param = sd_android_utility_get_java();
	vm = java_param->_vm;
	obj = java_param->_downloadengine;
	status = (*vm)->GetEnv(vm,&env,JNI_VERSION_1_4);
	if (status != JNI_OK)
	{
		ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "%s  GetEnv failed.", __FUNCTION__);
		status = (*vm)->AttachCurrentThread(vm, &env, NULL);
		if(JNI_OK != status)
		{
			sd_free(data);
			ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "%s  AttachCurrentThread failed.", __FUNCTION__);
			return -1;
		}
		arr = 1;
	}
	Activity_class = (*env)->GetObjectClass(env,obj);
	method_id = (*env)->GetMethodID(env, Activity_class, "jniCall_taskStateChanged", "(IIILjava/lang/String;Ljava/lang/String;)I");
	if(NULL == method_id)
	{
		sd_free(data);
		ANDROID_LOG(ANDROID_LOG_DEBUG,LOG_TAG,"In function %s ,GetMethodID failed",__FUNCTION__);
		if(1 == arr)
		{
			(*vm)->DetachCurrentThread(vm);
			arr = 0;
		}
		return -1;
	}

	file_name_str = (*env)->NewStringUTF(env, p_task_info->_file_name);
	file_path_str = (*env)->NewStringUTF(env, p_task_info->_file_path);

	(*env)->CallIntMethod(env, obj, method_id, task_id, p_task_info->_state,p_task_info->_failed_code,file_name_str,file_path_str);
	ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "function %s end",__FUNCTION__);
	(*env)->DeleteLocalRef(env,file_name_str);
	(*env)->DeleteLocalRef(env,file_path_str);
	sd_free(data);
	if(1 == arr)
	{
		(*vm)->DetachCurrentThread(vm);
		arr = 0;
	}

	return 0;
}

static void post_event(SWITCH_TO_UI_THREAD_HANDLER callback, void* user_data)
{
	JNIEnv *env = NULL;
	JavaVM * vm = NULL;
	_int32 status = 0;
	JavaParm * java_param = NULL;
	jclass Activity_class = NULL;
	jmethodID method_id = 0;
	int arr = 0;

	ANDROID_LOG(ANDROID_LOG_DEBUG,LOG_TAG,"function %s begin",__FUNCTION__);
	java_param = sd_android_utility_get_java();
	vm = java_param->_vm;
	status = (*vm)->GetEnv(vm,&env,JNI_VERSION_1_4);
	if(status != JNI_OK)
	{
		ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "%s  GetEnv failed.", __FUNCTION__);
		status = (*vm)->AttachCurrentThread(vm, &env, NULL);
		ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "%s  GetEnv failed..", __FUNCTION__);
		if (status != JNI_OK)
		{
			ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "%s  AttachCurrentThread failed.", __FUNCTION__);
			return;
		}
		ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "%s  GetEnv failed...", __FUNCTION__);
		arr = 1;
	}

	Activity_class = (*env)->GetObjectClass(env,gEngine);
	method_id = (*env)->GetMethodID(env, Activity_class, "jniCall_switchThread", "(II)V");
	if(NULL == method_id)
	{
		ANDROID_LOG(ANDROID_LOG_DEBUG,LOG_TAG,"In function %s ,GetMethodID failed",__FUNCTION__);
		if(1 == arr)
		{
			(*vm)->DetachCurrentThread(vm);
			arr = 0;
		}
		return;
	}
	(*env)->CallVoidMethod(env, gEngine, method_id, (_int32)callback, (_int32)user_data);
	ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "function %s end",__FUNCTION__);
	ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "post_event user_data %d end",user_data);
	ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "post_event callback %d end",callback);
	if(1 == arr)
	{
		(*vm)->DetachCurrentThread(vm);
		arr = 0;
	}
}

int32 on_task_state_changed_notify(u32 task_id, ETM_TASK_INFO * p_task_info){

	Task_State_change* data = NULL;
	_int32 ret = 0;
	ANDROID_LOG(ANDROID_LOG_ERROR,LOG_TAG,"In function %s , file_path is %s", __FUNCTION__, p_task_info->_file_path);
	ret = sd_malloc(sizeof(Task_State_change), (void**)&data);
	sd_memset(data,0,sizeof(Task_State_change));
	data->_task_id = task_id;
	sd_memcpy(&data->_task_info,p_task_info,sizeof(ETM_TASK_INFO));
	//切换线程
	post_event(ui_on_task_state_changed_notify,data);

	return 0;
}

JNIEXPORT jint JNICALL Java_com_runmit_sweedee_downloadinterface_DownloadEngine_initEtm
  (JNIEnv *env, jobject obj,jobject engine,
		  jstring sysPath, jstring uiVersion , jint channel,jint product_id, jstring systemInfo,jstring imei, jint width, jint height)
{
	JavaVM *vm = NULL;
	_int32 ret = 0;

   	ANDROID_LOG(ANDROID_LOG_DEBUG,LOG_TAG,"function %s begin",__FUNCTION__);

	ret = (*env)->GetJavaVM(env, (JavaVM **) &vm);
	if(0 != ret)
	{
		return ret;
	}
	ANDROID_LOG(ANDROID_LOG_DEBUG,LOG_TAG,"In function %s , ret = %d",__FUNCTION__,ret);
	gEngine = (*env)->NewGlobalRef(env, engine);

	char *pSystemInfo = (*env)->GetStringUTFChars(env,  systemInfo, NULL);
	char *pImei = (*env)->GetStringUTFChars(env,  imei, NULL);

	AndroidDeviceInfo deviceInfo;
	sd_strncpy(deviceInfo._system_info, pSystemInfo, sd_strlen(pSystemInfo)+1);
	sd_strncpy(deviceInfo._system_imei, pImei, sd_strlen(pImei)+1);

	deviceInfo._screenwidth = width;
	deviceInfo._screenheight = height;

	//初始化Common库的VM
	sd_android_utility_init(vm,gEngine,&deviceInfo);


	//初始化download engine
	char *path = (*env)->GetStringUTFChars(env,  sysPath, NULL);
	char *ui_version = (*env)->GetStringUTFChars(env,  uiVersion, NULL);
	ANDROID_LOG(ANDROID_LOG_DEBUG,LOG_TAG,"In function %s , path = %s , ui_version = %s",__FUNCTION__,path,ui_version);
	if(NULL != path)
	{
 		ret = sd_ensure_path_exist(path);
		ANDROID_LOG(ANDROID_LOG_DEBUG,LOG_TAG,"In function %s , sd_ensure_path_exist path = %s ret = %d",__FUNCTION__,path, ret);
		if(0 != ret)
		{
			return ret;
		}
	}
	else
	{
		return -1;
	}
	ANDROID_LOG(ANDROID_LOG_DEBUG,LOG_TAG,"In function %s ,start etm_init  ret = %d",__FUNCTION__, ret);
	ret = etm_init(path, sd_strlen(path));
	ANDROID_LOG(ANDROID_LOG_DEBUG,LOG_TAG,"In function %s ,end etm_init  ret = %d",__FUNCTION__, ret);

	//任务状态改变的回调通知
	etm_set_task_state_changed_callback(on_task_state_changed_notify);
	ANDROID_LOG(ANDROID_LOG_DEBUG,LOG_TAG,"In function %s ,etm_set_task_state_changed_callback  ret = %d",__FUNCTION__, ret);

	etm_init_network(-1);
	ANDROID_LOG(ANDROID_LOG_DEBUG,LOG_TAG,"In function %s ,etm_init_network  ui_version = %s  channel= %d",__FUNCTION__, ui_version,channel);

	//proudect id
	etm_set_ui_version((const char*)ui_version, product_id, channel);
	ANDROID_LOG(ANDROID_LOG_DEBUG,LOG_TAG,"In function %s ,etm_set_ui_version  ret = %d",__FUNCTION__, ret);
	(*env)->ReleaseStringUTFChars(env,sysPath,path);
	(*env)->ReleaseStringUTFChars(env,uiVersion,ui_version);
	(*env)->ReleaseStringUTFChars(env,systemInfo,pSystemInfo);
	(*env)->ReleaseStringUTFChars(env,imei,pImei);
	ANDROID_LOG(ANDROID_LOG_DEBUG,LOG_TAG,"function %s end , channel = %d  ",__FUNCTION__ , channel);
   	 return 0;
}

JNIEXPORT void JNICALL Java_com_runmit_sweedee_downloadinterface_DownloadEngine_sendMessageHandler
  (JNIEnv *env, jclass cls, jint call_back, jint user_data)
{
	ANDROID_LOG(ANDROID_LOG_DEBUG,LOG_TAG,"function %s begin",__FUNCTION__);
	((SWITCH_TO_UI_THREAD_HANDLER)(call_back))((void*)user_data);
	ANDROID_LOG(ANDROID_LOG_DEBUG,LOG_TAG,"function %s end",__FUNCTION__);
	ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "sendMessageHandler user_data %d end",user_data);
	ANDROID_LOG(ANDROID_LOG_DEBUG, LOG_TAG, "sendMessageHandler callback %d end",call_back);
}

JNIEXPORT jint JNICALL Java_com_runmit_sweedee_downloadinterface_DownloadEngine_createTaskByCfg(JNIEnv *env, jobject obj, jstring taskpath){
  	char* ptaskpath = NULL;
  	jclass cls = (*env)->GetObjectClass(env, obj);
  	jfieldID field_id = (*env)->GetFieldID(env, cls, "mTaskId", "J");
	
	ptaskpath =(char*) (*env)->GetStringUTFChars(env,taskpath,NULL);
	if(NULL == ptaskpath)
	{
		return -2;
	}
	_u32 p_task_id = 0;

	int ret = etm_create_task_by_cfg_file(ptaskpath,&p_task_id);
	(*env)->ReleaseStringUTFChars(env , taskpath , ptaskpath);
	if (NULL != field_id) {
		(*env)->SetLongField(env, obj, field_id, p_task_id);
	}
	
  	return ret;
  }

JNIEXPORT jint JNICALL Java_com_runmit_sweedee_downloadinterface_DownloadEngine_createTaskByFile(JNIEnv *env, jobject obj, jstring taskpath,jstring taskname,jlong filesize){
  	char* ptaskpath = NULL;
  	jclass cls = (*env)->GetObjectClass(env, obj);
  	jfieldID field_id = (*env)->GetFieldID(env, cls, "mTaskId", "J");
  	
	ptaskpath =(char*) (*env)->GetStringUTFChars(env,taskpath,NULL);
	if(NULL == ptaskpath)
	{
		return -2;
	}
	char* ptaskname = NULL;
	ptaskname =(char*) (*env)->GetStringUTFChars(env,taskname,NULL);
	if(NULL == ptaskname)
	{
		(*env)->ReleaseStringUTFChars(env , taskname , ptaskpath);
		return -3;
	}

	_u32 p_task_id = 0;

	ETM_CREATE_TASK task_info;
	sd_memset(&task_info, 0, sizeof(task_info));

	task_info._type = ETT_FILE;
	task_info._file_size =filesize;

	task_info._file_path = ptaskpath;
	task_info._file_path_len = sd_strlen(ptaskpath);

	task_info._file_name = ptaskname;
	task_info._file_name_len = sd_strlen(ptaskname);

	int ret = etm_create_task(&task_info, &p_task_id);

	if (NULL != field_id) {
		(*env)->SetLongField(env, obj, field_id, p_task_id);
	}
	
	(*env)->ReleaseStringUTFChars(env , taskpath , ptaskpath);
	(*env)->ReleaseStringUTFChars(env , taskname , ptaskname);
	return ret;
}
