#ifndef HSC_INTERFACE_20110416
#define HSC_INTERFACE_20110416

#include "utility/define.h"

#if defined(ENABLE_HSC) && !defined(ENABLE_ETM_MEDIA_CENTER)

#include "asyn_frame/notice.h"
#include "high_speed_channel/hsc_info.h"
#include "download_task/download_task.h"

typedef struct
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_u32 _task_id;
    _u32    _hsc_has_pay;
    char*   _hsc_task_name;
    _u32    _hsc_task_name_length;
}HSC_SWITCH;

typedef struct
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_u32 _task_id;
	_u64 _product_id;
	_u64 _sub_id;
}HSC_PRODUCT_SUB_ID_INFO;

typedef struct
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	BOOL _enable;
}HSC_AUTO_SWITCH;

typedef struct
{
    SEVENT_HANDLE   _handle;
    _int32          _result;
    _u32            _flag;
    _u32            _client_ver;
}HSC_BUSINESS_FLAG;

typedef struct
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_u32 _task_id;
	BOOL _used_before;
}HSC_USED_BEFORE_FLAG;

typedef struct
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	BOOL *_p_is_auto;
}HSC_AUTO_INFO;

typedef struct
{
    SEVENT_HANDLE   _handle;
    _int32          _result;
    void*           _p_callback;
}HSC_QUERY_FLUX_INFO;

_int32 hsc_enable(void *_param);
_int32 hsc_disable(void *_param);
//_int32 hsc_get_info(void *_param);
_int32 hsc_set_product_sub_id(void *_param);
_int32 hsc_auto_sw(void *_param);
_int32 hsc_set_used_hsc_before(void *_param);
_int32 hsc_is_auto(void *_param);
_int32 hsc_handle_auto_enable(TASK * p_task, _u32 file_index, _u8 * gcid, _u64 file_size );

_int32 hsc_set_business_flag(void* param);
_int32 hsc_get_business_flag();

_int32 hsc_query_flux_info(void* _param);


#endif /* ENABLE_HSC */

#endif /* HSC_INTERFACE_20110416 */
