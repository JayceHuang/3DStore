#ifndef _UPLOAD_CONTROLLER_H_20130115_
#define _UPLOAD_CONTROLLER_H_20130115_


#ifdef __cplusplus
extern "C"
{   
#endif

#include "utility/define.h"

#ifdef UPLOAD_ENABLE

struct tag_ULM_CONTROLLER
{
    _int32 _enable_upload_from_cfg;
    _int32 _enable_upload_when_download_from_cfg;
    _int32 _enable_upload;

	_int32 _policy;
	
    _u32 _speed_limit_in_charge;
    _u32 _speed_limit_in_lock;
    _u32 _speed_limit_in_download;
    _int32 _lower_limit_charge_from_cfg;

    _int32 _is_in_charge;
    _int32 _is_in_lock;
    _int32 _now_charge;
};

typedef struct tag_ULM_CONTROLLER  ULM_CONTROLLER;

typedef enum STRATEGY
{
    UPLOAD_CONTROL_STRATEGY_INIT,
    NEED_START,
    NEED_STOP,
}STRATEGY;

_int32 init_ulc();

_int32 uninit_ulc();

_int32 ulc_enable_upload();

#endif //UPLOAD_ENABLE

#ifdef __cplusplus
}
#endif

#endif  // _UPLOAD_CONTROLLER_H_20130115_

