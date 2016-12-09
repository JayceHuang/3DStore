#include "utility/sd_assert.h"
#include "utility/string.h"

#define LOGID LOGID_ASYN_FRAME_TEST

#include "utility/logger.h"
#if defined(_PC_LINUX)
#include <execinfo.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#ifdef _DEBUG

void print_strace(void)
{
#if defined(_PC_LINUX)
        void *array[10] = {0};
        size_t size;
        char **strings;
        size_t i;
        int nCount = 0;

        size =backtrace(array,10);
        strings = backtrace_symbols(array,size);

        for(i = 0;i<size;i++)
        {
            LOG_DEBUG("%s\n",strings[i]);
            printf("%s\n",strings[i]);
            if(sd_strstr(strings[i],"ILibWebServer_Release",0)!=NULL)
            {
                    nCount++;
            }
        }
        sleep(10);
        free(strings);
#endif
		return;
}


void log_assert(const char * func, const char * file , int line, const char * ex)
{
	LOG_URGENT( "Assert Fatal Error:%s in %s:%d:%s", func,file,line,ex);
	fprintf(stderr, "Assert Fatal Error:%s in %s:%d:%s", func,file,line,ex);
#if defined(LINUX) && (!defined(_ANDROID_LINUX))
	print_strace();
#else
    sd_sleep(10000);
#endif
    assert(FALSE);
}

#endif

