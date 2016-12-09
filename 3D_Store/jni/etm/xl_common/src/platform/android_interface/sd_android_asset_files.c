#if defined(_ANDROID_LINUX)

#ifdef LOGID
#undef LOGID
#endif
#define LOGID LOGID_INTERFACE
#include "utility/logger.h"

#include "platform/android_interface/sd_android_asset_files.h"
#include "platform/android_interface/sd_android_utility.h"
#include "utility/errcode.h"

_int32 sd_asset_open_ex(char *filepath, _u32 *file_id)
{
	JavaVM *vm = NULL;
	JNIEnv *env = NULL;
	_int32 status;

	jclass Activity_class;
	jobject thiz;
	jmethodID openAssetFile_id;
	jstring jstr;
	_int32 arr = 0;

	LOG_DEBUG("sd_asset_open_ex: filepath=%s", filepath);
	if (!sd_android_utility_is_init())
	{
		LOG_ERROR("sd_asset_open_ex, java not init");
		return -1;
	}
	
	vm = sd_android_utility_get_java()->_vm;
	thiz= sd_android_utility_get_java()->_thiz;

	//获得环境
	status = (*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4);
	if (status != JNI_OK)
	{
		//多线程支持
		status = (*vm)->AttachCurrentThread(vm, &env, NULL);
		if (status != JNI_OK)
		{
			return NULL;
		}
		arr = 1;
	}

	Activity_class 	= (*env)->GetObjectClass(env, thiz);
	openAssetFile_id = (*env)->GetMethodID(env, Activity_class, "openAssetFile", "(Ljava/lang/String;)I");
	jstr = (*env)->NewStringUTF(env, filepath);
	*file_id = (*env)->CallIntMethod(env, thiz, openAssetFile_id, jstr);
	if ((*env)->ExceptionCheck(env))
	{
		LOG_ERROR("sd_asset_open_ex, ExceptionCheck");
		if(1 == arr)
		{
			(*vm)->DetachCurrentThread(vm);
			arr = 0;
		}
		return -1;
	}
	
	LOG_DEBUG("sd_asset_open_ex, call java ok. (id:0x%x), filepath=%s", *file_id, filepath);
	
	(*env)->DeleteLocalRef(env, jstr);
	(*env)->DeleteLocalRef(env, Activity_class);
	if(1 == arr)
	{
		(*vm)->DetachCurrentThread(vm);
		arr = 0;
	}
	return SUCCESS;
}

_int32 sd_asset_read(_u32 file_id, char *buffer, _int32 size, _u32 *readsize)
{
	JavaVM *vm = NULL;
	JNIEnv *env = NULL;
	_int32 status;

	jclass Activity_class;
	jobject thiz;
	jmethodID readAssetFile_id;
	
	jbyteArray byteArray;
	jbyte * p_bytes = NULL;

	LOG_DEBUG("sd_asset_read, begin. file_id=0x%x, size=%d", file_id, size);

	if (!sd_android_utility_is_init())
	{
		LOG_ERROR("sd_asset_read, java not init");
		return -1;
	}
	
	vm = sd_android_utility_get_java()->_vm;
	thiz= sd_android_utility_get_java()->_thiz;

	//获得环境
	status = (*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4);
	if (status != JNI_OK)
	{
		LOG_ERROR("sd_asset_read, GetEnv error: vm=0x%x, thiz=0x%x, status=%d", vm, thiz, status);
		return -1;
	}

	Activity_class 	= (*env)->GetObjectClass(env, thiz);
	readAssetFile_id = (*env)->GetMethodID(env, Activity_class, "readAssetFile", "(II)[B");

	byteArray = (*env)->CallObjectMethod(env, thiz, readAssetFile_id, file_id, size);
	if ((*env)->ExceptionCheck(env))
	{
		LOG_ERROR("sd_asset_read, ExceptionCheck");
		return -1;
	}

	if (byteArray == NULL)
	{
		LOG_DEBUG("sd_asset_read, call java error. (id:0x%x) size=%d", file_id, size);
		return -1;
	}
	
	*readsize = (*env)->GetArrayLength(env, byteArray);
	(*env)->GetByteArrayRegion(env, byteArray, 0, *readsize, (jbyte *)buffer);
	
	LOG_DEBUG("sd_asset_read, call java ok. (id:0x%x) size=%d, readsize=%d", file_id, size, *readsize);

	(*env)->DeleteLocalRef(env, byteArray);
	(*env)->DeleteLocalRef(env, Activity_class);
	return SUCCESS;
}

_int32 sd_asset_setfilepos(_u32 file_id, _u64 filepos)
{
	_int32 ret = 0;
	JavaVM *vm = NULL;
	JNIEnv *env = NULL;
	_int32 status;

	jclass Activity_class;
	jobject thiz;
	jmethodID method_id;
	_int32 arr = 0;

	LOG_DEBUG("sd_asset_setfilepos, begin. (id:0x%x), filepos=%llu", file_id, filepos);
	if (!sd_android_utility_is_init())
	{
		LOG_ERROR("sd_asset_setfilepos, java not init. (id:0x%x), filepos=%llu", file_id, filepos);
		return -1;
	}
	
	vm = sd_android_utility_get_java()->_vm;
	thiz= sd_android_utility_get_java()->_thiz;

	//获得环境
	status = (*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4);
	if (status != JNI_OK)
	{
		//多线程支持
		status = (*vm)->AttachCurrentThread(vm, &env, NULL);
		if (status != JNI_OK)
		{
			return NULL;
		}
		arr = 1;
	}

	Activity_class 	= (*env)->GetObjectClass(env, thiz);
	method_id = (*env)->GetMethodID(env, Activity_class, "setposAssetFile", "(IJ)I");

	ret = (*env)->CallIntMethod(env, thiz, method_id, file_id, filepos);
	if ((*env)->ExceptionCheck(env))
	{
		LOG_ERROR("sd_asset_setfilepos, ExceptionCheck");
		if(1 == arr)
		{
			(*vm)->DetachCurrentThread(vm);
			arr = 0;
		}
		return -1;
	}

	LOG_DEBUG("sd_asset_setfilepos, call java ok. (id:0x%x) ret=%d, filepos=%llu", file_id, ret, filepos);

	(*env)->DeleteLocalRef(env, Activity_class);
	if(1 == arr)
	{
		(*vm)->DetachCurrentThread(vm);
		arr = 0;
	}
	return ret;
}

_int32 sd_asset_close_ex(_u32 file_id)
{
	_int32 ret = 0;
	JavaVM *vm = NULL;
	JNIEnv *env = NULL;
	_int32 status;

	jclass Activity_class;
	jobject thiz;
	jmethodID closeAssetFile_id;
	_int32 arr = 0;

	LOG_DEBUG("sd_asset_close_ex, begin. (id:0x%x)", file_id);
	if (!sd_android_utility_is_init())
	{
		LOG_ERROR("sd_asset_close_ex, java not init. (id:0x%x)", file_id);
		return -1;
	}
	
	vm = sd_android_utility_get_java()->_vm;
	thiz= sd_android_utility_get_java()->_thiz;

	//获得环境
	status = (*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4);
	if (status != JNI_OK)
	{
		//多线程支持
		status = (*vm)->AttachCurrentThread(vm, &env, NULL);
		if (status != JNI_OK)
		{
			return NULL;
		}
		arr = 1;
	}

	Activity_class 	= (*env)->GetObjectClass(env, thiz);
	closeAssetFile_id = (*env)->GetMethodID(env, Activity_class, "closeAssetFile", "(I)I");

	ret = (*env)->CallIntMethod(env, thiz, closeAssetFile_id, file_id);
	if ((*env)->ExceptionCheck(env))
	{
		LOG_ERROR("sd_asset_close_ex, ExceptionCheck");
		if(1 == arr)
		{
			(*vm)->DetachCurrentThread(vm);
			arr = 0;
		}
		return -1;
	}

	LOG_DEBUG("sd_asset_close_ex, call java ok. (id:0x%x) ret=%d", file_id, ret);

	(*env)->DeleteLocalRef(env, Activity_class);
	if(1 == arr)
	{
		(*vm)->DetachCurrentThread(vm);
		arr = 0;
	}
	return ret;
}

BOOL sd_asset_file_exist(char *filepath)
{
	JavaVM *vm = NULL;
	JNIEnv *env = NULL;
	_int32 status;
	jboolean is_exist = FALSE;

	jclass Activity_class;
	jobject thiz;
	jmethodID existAssetFile_id;
	jstring jstr;
	_int32 arr = 0;

	if (!sd_android_utility_is_init())
	{
		LOG_ERROR("sd_asset_file_exist, java not init");
		return -1;
	}
	
	vm = sd_android_utility_get_java()->_vm;
	thiz= sd_android_utility_get_java()->_thiz;

	LOG_DEBUG("sd_asset_file_exist: vm=0x%x, thiz=0x%x, filepath=%s", vm, thiz, filepath);

	//获得环境
	status = (*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4);
	if (status != JNI_OK)
	{
		//多线程支持
		status = (*vm)->AttachCurrentThread(vm, &env, NULL);
		if (status != JNI_OK)
		{
			return NULL;
		}
		arr = 1;
	}

	Activity_class 	= (*env)->GetObjectClass(env, thiz);
	methodId = (*env)->GetMethodID(env, Activity_class, "getIMEI", "()Ljava/lang/String;");
	jimei = (*env)->CallObjectMethod(env, thiz, methodId);
	if ((*env)->ExceptionCheck(env))
	{
		if(1 == arr)
		{
			(*vm)->DetachCurrentThread(vm);
			arr = 0;
		}
		return NULL;
	}

	Activity_class 	= (*env)->GetObjectClass(env, thiz);
	existAssetFile_id = (*env)->GetMethodID(env, Activity_class, "existAssetFile", "(Ljava/lang/String;)Z");
	jstr = (*env)->NewStringUTF(env, filepath);
	is_exist = (*env)->CallBooleanMethod(env, thiz, existAssetFile_id, jstr);
	if ((*env)->ExceptionCheck(env))
	{
		LOG_ERROR("sd_asset_file_exist, ExceptionCheck");
		if(1 == arr)
		{
			(*vm)->DetachCurrentThread(vm);
			arr = 0;
		}
		return -1;
	}

	LOG_DEBUG("sd_asset_file_exist, call java ok. (%d) filepath=%s", is_exist, filepath);

	(*env)->DeleteLocalRef(env, jstr);
	(*env)->DeleteLocalRef(env, Activity_class);
	if(1 == arr)
	{
		(*vm)->DetachCurrentThread(vm);
		arr = 0;
	}
	return is_exist;
}

BOOL sd_asset_is_asset_fd(_u32 file_id)
{
	if (file_id < ASSET_FD_BASE || file_id >= (ASSET_FD_BASE + ASSET_FD_SIZE))
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

#endif

