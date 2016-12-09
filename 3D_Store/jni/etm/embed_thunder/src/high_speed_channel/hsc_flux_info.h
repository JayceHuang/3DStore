#ifndef __HSC_FLUX_INFO__
#define __HSC_FLUX_INFO__

#include "utility/define.h"
#ifdef  ENABLE_HSC

typedef enum tagHSC_FLUX_INFO_RESULT
{
    HFIR_IDLE = 0,
    HFIR_QUERYING,
    HFIR_OK,
    HFIR_TIMEOUT,
    HFIR_FAILED
}HSC_FLUX_INFO_RESULT;

typedef struct tagHSC_FLUX_INFO
{
    HSC_FLUX_INFO_RESULT    _result;
    _u64                    _userid;
    _u64                    _capacity;
    _u64                    _remain;
}HSC_FLUX_INFO;

typedef     _int32 (*HSC_QUERY_HSC_FLUX_INFO_CALLBACK)(HSC_FLUX_INFO*);

#endif  //ENABLE_HSC
#endif  //__HSC_FLUX_INFO__
