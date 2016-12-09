#include "utility/define.h"
#include "utility/errcode.h"
#define LOGID LOGID_ASYN_FRAME_TEST
#include "utility/logger.h"


static _int32 g_critical_error = SUCCESS;

#ifdef _DEBUG
_int32 sd_check_value(_int32 errcode,const char * func, const char * file , int line)
{
	if (SUCCESS == errcode)
	{
	    return SUCCESS;
	}
	
	LOG_URGENT("Assert Fatal Error:%s in %s:%d:CHECK_VALUE(%d)", func, file, line, errcode);
	logger_printf("Assert Fatal Error:%s in %s:%d:CHECK_VALUE(%d)", func, file, line, errcode);
#ifdef MACOS
	printf( "\n Assert Fatal Error:%s in %s:%d:CHECK_VALUE(%d)\n", func,file,line,errcode);
#endif

#if (defined(LINUX) && (!defined(_ANDROID_LINUX)))
	print_strace();
#else
    sleep(20);
#endif
    
	assert(FALSE);
#ifdef WINCE
	exit(errcode);
#endif
	return errcode;
}
#endif /* _DEBUG  */

_int32 set_critical_error(_int32 errcode)
{
    g_critical_error = errcode;
    if (SUCCESS != errcode)
    {
        LOG_URGENT( "set_critical_error:%d",errcode);
#ifndef ENABLE_ETM_MEDIA_CENTER
        print_strace_to_file();
#endif
	}
	return SUCCESS;
}

_int32 get_critical_error(void)
{
    return g_critical_error;
}

