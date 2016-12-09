#ifndef SD_ASSERT_H_00138F8F2E70_200806182035
#define SD_ASSERT_H_00138F8F2E70_200806182035
#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"

#include "assert.h"

#ifdef WINCE
#define __func__  NULL
#endif
#ifdef _DEBUG

#if defined(LINUX) && (!defined(_ANDROID_LINUX)) 
void print_strace(void);
#endif

void log_assert(const char * func, const char * file , int line, const char * ex);

#ifdef _LOGGER
#define	sd_assert(expr)	((expr) ? (void)0 : log_assert(__func__, __FILE__, \
			    __LINE__, #expr))
#else 
#define	sd_assert(expr) assert(expr);
#endif

//#define sd_assert(expr)	assert(expr)

#else

#define sd_assert(expr)	

#endif
#ifdef __cplusplus
}
#endif
#endif
