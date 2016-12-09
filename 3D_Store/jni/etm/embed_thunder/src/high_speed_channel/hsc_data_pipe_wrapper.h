#ifndef __hsc_data_pipe_wrapper__
#define __hsc_data_pipe_wrapper__

#include "utility/define.h"
#ifdef ENABLE_HSC
#include "utility/list.h"
#include "utility/define_const_num.h"
#include "asyn_frame/notice.h"
#include "asyn_frame/msg.h"

typedef struct _tagHSC_DATA_PIPE_MANAGER
{
    LIST _lst_pipes;
}HSC_DATA_PIPE_MANAGER;

typedef enum _tagHSC_DATA_PIPE_TYPE
{
    HDPT_HTTP
}HSC_DATA_PIPE_TYPE;

typedef enum _tagHSC_DATA_PIPE_STATUS
{
    HDPS_READY = 0,
    HDPS_RESVING,
    HDPS_OK,
    HDPS_TIMEOUT,
    HDPS_WAITTING_FOR_RETRY,
    HDPS_FAIL,
    HDPS_UPBOUND,
}HSC_DATA_PIPE_STATUS;

typedef _int32 (*HSC_DATA_PIPE_INTERFACE_CALLBACK)(void*);
typedef struct _tagHSC_DATA_PIPE_INTERFACE
{
    HSC_DATA_PIPE_TYPE _type;
    HSC_DATA_PIPE_STATUS _status;
    char* _p_host;
    _u32 _host_len;
    _u32 _port;
    void* _p_pipe;
    void* _p_send_func;
    void* _p_cancel_send_func;
    void* _p_send_data;
    _u32 _send_data_len;
    void* _p_recv_data;
    _u32 _recv_data_len;
    _u32 _retry_count;
    _u32 _retry_interval;
    _u32 _time_out;
    _u32 _timer_id;
    HSC_DATA_PIPE_INTERFACE_CALLBACK* _p_callbacks;
    void* _p_user_data;
}HSC_DATA_PIPE_INTERFACE;

typedef HSC_DATA_PIPE_INTERFACE_CALLBACK    HSC_DATA_PIPE_INTERFACE_SEND_FUNC;


_int32  hsc_init_hdpi_modle();

_int32  hsc_init_hdpi(HSC_DATA_PIPE_INTERFACE* p_HDPI, const char* host, const _u32 host_len, const _u32 port);

_int32  hsc_uninit_hdpi(HSC_DATA_PIPE_INTERFACE* p_HDPI);

_int32  hsc_send_data(HSC_DATA_PIPE_INTERFACE* p_HDPI, const void* p_data, const _int32 data_len);

_int32  hsc_cancel_sending(HSC_DATA_PIPE_INTERFACE* p_HDPI);

_int32  hsc_start_pipe_timeout_timer(HSC_DATA_PIPE_INTERFACE* p_HDPI);

_int32  hsc_start_retry_interval_timer(HSC_DATA_PIPE_INTERFACE* p_HDPI);

_int32  hsc_cancel_pipe_timer(HSC_DATA_PIPE_INTERFACE* p_HDPI);

_int32  hsc_handle_hsc_data_pipe_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid);

_int32  hsc_handle_hsc_data_pipe_retry_interval(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid);

_int32  hsc_add_pipe_to_manager(HSC_DATA_PIPE_INTERFACE* p_HDPI);

_int32  hsc_remove_pipe_from_manager(HSC_DATA_PIPE_INTERFACE* p_HDPI);

_int32  hsc_remove_pipe_from_manager_by_it(LIST_ITERATOR it);

LIST_ITERATOR hsc_find_pipe_from_manager(HSC_DATA_PIPE_INTERFACE* p_HDPI);

LIST    hsc_get_hsc_pipe_manager();

_int32  hsc_fire_http_data_pipe_event(HSC_DATA_PIPE_INTERFACE* p_HDPI);

_int32  hsc_do_send_data(HSC_DATA_PIPE_INTERFACE* p_HDPI);

_int32  hsc_set_speed_limit_ex(HSC_DATA_PIPE_INTERFACE* p_HDPI);
#endif  //ENABLE_HSC

#endif  //__hsc_data_pipe_wrapper__
