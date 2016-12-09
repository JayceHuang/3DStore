#if defined(_ANDROID_LINUX)

#include "platform/android_interface/sd_android_device_info.h"
#include "platform/android_interface/sd_android_utility.h"
#include "utility/logid.h"
#ifdef LOGID
#undef LOGID
#endif

#define LOGID LOGID_COMMON

#include "utility/logger.h"

static _int32 screen_x = 0;
static _int32 screen_y = 0;
static char system_info[MAX_OS_LEN]={0};
static char system_imei[IMEI_LEN]={0};

/*
const char * get_android_system_info(void)
{
	if(sd_strlen(system_info)==0)
	{
		JavaVM *vm = NULL;
		JNIEnv *env = NULL;
		_int32 status,len=0;

		jclass Activity_class;
		jobject thiz;
		jmethodID methodId;
		jobject jinfo;

		char s_info[128];
		const char *p_info = NULL;
		int arr = 0;

		if (!sd_android_utility_is_init())
		{
			return system_info;
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
			return system_info;
		}
			arr = 1;
		}

		Activity_class 	= (*env)->GetObjectClass(env, thiz);
		methodId = (*env)->GetMethodID(env, Activity_class, "getSystemInfo", "()Ljava/lang/String;");
		jinfo = (*env)->CallObjectMethod(env, thiz, methodId);
		if ((*env)->ExceptionCheck(env))
		{
			if(1 == arr)
			{
				(*vm)->DetachCurrentThread(vm);
				arr = 0;
			}
			return system_info;
		}

		sd_memset(s_info, 0, 128);
		p_info = (*env)->GetStringUTFChars(env, (jstring)jinfo, 0);
		sd_memcpy(s_info, p_info, 127);
		
		(*env)->DeleteLocalRef(env, jinfo);
		(*env)->DeleteLocalRef(env, Activity_class);
		//8_2.2,2.15.1010.2 CL203557 release-eys,HTC,Nexus One
		LOG_ERROR( "get_android_system_info :%s",s_info);
		len = sd_strlen(s_info);
		if(len>MAX_OS_LEN-sizeof("android_")-1)
		{
			s_info[MAX_OS_LEN-sizeof("android_")-1] = '\0';
		}
		len = 0;
		while(s_info[len]!='\0')
		{
			if(s_info[len]=='-') s_info[len]='_';
			len++;
		}
		sd_strncpy(system_info, "android_",sizeof("android_"));
		sd_strcat(system_info, s_info, sd_strlen(s_info));

		if(1 == arr)
		{
			(*vm)->DetachCurrentThread(vm);
			arr = 0;
		}
	}

	return system_info;
}

_int32 get_android_screen_size(_int32 *x,_int32 *y)
{
	if(screen_x==0)
	{
		JavaVM *vm = NULL;
		JNIEnv *env = NULL;
		_int32 status;

		jclass Activity_class;
		jobject thiz;
		jmethodID methodId;
		int arr = 0;

		if (!sd_android_utility_is_init())
		{
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
			return 1;
		}
			arr = 1;
		}

		Activity_class 	= (*env)->GetObjectClass(env, thiz);
		/////////////////////
		methodId = (*env)->GetMethodID(env, Activity_class, "getScreenWidth", "()I");
		screen_x = (*env)->CallIntMethod(env, thiz, methodId);
		if ((*env)->ExceptionCheck(env))
		{
			if(1 == arr)
			{
				(*vm)->DetachCurrentThread(vm);
				arr = 0;
			}
			return -1;
		}
		/////////////////////
		methodId = (*env)->GetMethodID(env, Activity_class, "getScreenHeight", "()I");
		screen_y = (*env)->CallIntMethod(env, thiz, methodId);
		if ((*env)->ExceptionCheck(env))
		{
			if(1 == arr)
			{
				(*vm)->DetachCurrentThread(vm);
				arr = 0;
			}

			return -1;
		}

		(*env)->DeleteLocalRef(env, Activity_class);

		if(1 == arr)
		{
			(*vm)->DetachCurrentThread(vm);
			arr = 0;
		}
	}

	LOG_ERROR( "get_android_screen_size :%d,%d",screen_x,screen_y);
	if(x)  *x = screen_x;
	if(y)  *y = screen_y;
	
	return SUCCESS;
}

// 获取设备IMEI号
const char * get_android_imei(void)
{
	JavaVM *vm = NULL;
	JNIEnv *env = NULL;
	_int32 status;

	jclass Activity_class;
	jobject thiz;
	jmethodID methodId;
	jobject jimei;
	int arr = 0;

	static const char s_imei[IMEI_LEN + 1];
	const char *p_imei = NULL;

	if (!sd_android_utility_is_init())
	{
		return NULL;
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

	sd_memset(s_imei, 0, IMEI_LEN + 1);
	p_imei = (*env)->GetStringUTFChars(env, (jstring)jimei, 0);
	if(p_imei!= NULL)
	{
	sd_memcpy(s_imei, p_imei, IMEI_LEN);
	}
	else
	{
		(*env)->DeleteLocalRef(env, jimei);
		(*env)->DeleteLocalRef(env, Activity_class);
		if(1 == arr)
		{
			(*vm)->DetachCurrentThread(vm);
			arr = 0;
		}
		return NULL;
	
	}
	(*env)->DeleteLocalRef(env, jimei);
	(*env)->DeleteLocalRef(env, Activity_class);
	(*vm)->DetachCurrentThread(vm);
	if(1 == arr)
	{
		(*vm)->DetachCurrentThread(vm);
		arr = 0;
	}

	return s_imei;
}
*/

const char * get_android_system_info(void)
{
	if(sd_strlen(system_info)==0)
	{
		AndroidDeviceInfo *device_info = NULL;
		char s_info[MAX_OS_LEN] = {0};
		_int32 len=0;
		if (!sd_android_utility_is_init())
		{
			return system_info;
		}
		device_info = sd_android_utility_get_device_info();
		sd_strncpy(s_info, device_info->_system_info, MAX_OS_LEN -1);
		
		//8_2.2,2.15.1010.2 CL203557 release-eys,HTC,Nexus One
		LOG_ERROR( "get_android_system_info :%s",s_info);
		len = sd_strlen(s_info);
		if(len>MAX_OS_LEN-sizeof("android_")-1)
		{
			s_info[MAX_OS_LEN-sizeof("android_")-1] = '\0';
		}
		len = 0;
		while(s_info[len]!='\0')
		{
			if(s_info[len]=='-') s_info[len]='_';
			len++;
		}
		sd_strncpy(system_info, "android_",sizeof("android_"));
		sd_strcat(system_info, s_info, sd_strlen(s_info));

	}

	return system_info;


}
_int32 get_android_screen_size(_int32 *x,_int32 *y)
{
	if(screen_x==0 || screen_y==0)
	{
		AndroidDeviceInfo *device_info = NULL;
		if (!sd_android_utility_is_init())
		{
			return -1;
		}
		device_info = sd_android_utility_get_device_info();
		screen_x = device_info->_screenwidth;
		screen_y = device_info->_screenheight;
	}

	if(x != NULL)  
		*x = screen_x;
	if(y != NULL)  
		*y = screen_y;
	
	return SUCCESS;
}

// 获取设备IMEI号
const char * get_android_imei(void)
{
	if(sd_strlen(system_imei)==0)
	{
		AndroidDeviceInfo *device_info = NULL;
		char s_imei[IMEI_LEN + 1] = {0};

		if (!sd_android_utility_is_init())
		{
			return system_imei;
		}
		device_info = sd_android_utility_get_device_info();
		sd_strncpy(s_imei, device_info->_system_imei, IMEI_LEN);
		
		//
		LOG_ERROR( "get_android_system_info :%s",s_imei);

		sd_strncpy(system_imei, s_imei, IMEI_LEN +1 );
	}

	return system_imei;
}


_u32 get_android_available_space(_u32 *size)
{
	JavaVM *vm = NULL;
	JNIEnv *env = NULL;
	_int32 status;

	jclass Activity_class;
	jobject thiz;
	jobject obj;
	jmethodID methodId;
	int arr = 0;
	int available_space = 0;

	if (!sd_android_utility_is_init())
	{
		return NULL;
	}
			
	vm = sd_android_utility_get_java()->_vm;
	obj= sd_android_utility_get_java()->_downloadengine;

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

	Activity_class 	= (*env)->GetObjectClass(env, obj);
	methodId = (*env)->GetMethodID(env, Activity_class, "jniCall_getSdcardSapce", "()I");
	available_space = (*env)->CallIntMethod(env, obj, methodId);
	LOG_DEBUG("jniCall_getSdcardSapce :available_space = %d", available_space);
	if ((*env)->ExceptionCheck(env))
	{
		if(1 == arr)
		{
			(*vm)->DetachCurrentThread(vm);
			arr = 0;
		}
		return 0;
	}
	(*env)->DeleteLocalRef(env, Activity_class);
	if(1 == arr)
	{
		(*vm)->DetachCurrentThread(vm);
		arr = 0;
	}
	
	//*size = (_u32)(space_size/(1<<10));
	*size = (_u32)(available_space * 1024);
	LOG_DEBUG( "get_android_available_space : free_size = %u", *size);
	return 0;
}

#endif
