#include "em_errcode.h"
#include "em_logid.h"

#define EM_LOGID LOGID_ASYN_FRAME_TEST

#include "em_logger.h"

static _int32 g_em_critical_error = SUCCESS;
extern _int32 print_strace_to_file(void);
_int32 em_set_critical_error(_int32 errcode)
{
	g_em_critical_error = errcode;
	if(errcode!=SUCCESS)
	{
		_int32 *p = NULL;
		EM_LOG_URGENT( "em_set_critical_error:%d",errcode);
		print_strace_to_file();
		/* 当下载库遇到无法修复的错误时，让程序直接崩溃退出算了!  -- by zyq @ 20120425 */
		*p = errcode;
	}
	return SUCCESS;
}

_int32 em_get_critical_error(void)
{
	return g_em_critical_error;
}


