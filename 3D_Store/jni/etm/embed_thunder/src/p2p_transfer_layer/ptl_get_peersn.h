/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/10/10
-----------------------------------------------------------------------------------------------------------*/
#ifndef _PTL_GET_PEERSN_H_
#define _PTL_GET_PEERSN_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "asyn_frame/msg.h"
#include "p2p_transfer_layer/ptl_cmd_define.h"

typedef _int32 (*ptl_get_peersn_callback)(_int32 errcode, _u32 sn_ip, _u16 sn_port, void* user_data);

typedef enum tagPTL_GET_PEERSN_STATE
{
	PEERSN_INIT_STATE = 0, PEERSN_GETTING_STATE = 0, PEERSN_IN_CACHE_STATE, PEERSN_CANCEL_STATE
}PTL_GET_PEERSN_STATE;

typedef struct tagGET_PEERSN_DATA
{
	char						_peerid[PEER_ID_SIZE + 1];
	ptl_get_peersn_callback		_callback;
	void*					_user_data;
	PTL_GET_PEERSN_STATE	_state;
	_u32					_timestamp;
	_u32					_try_times;
	_u32					_msg_id;
}GET_PEERSN_DATA;

typedef struct tagPEERSN_CACHE_DATA
{
	char		_peerid[PEER_ID_SIZE + 1];
	_u32	_sn_ip;
	_u16	_sn_port;
	_u32	_add_time;						/*millisecond*/			
}PEERSN_CACHE_DATA;

_int32 ptl_get_peersn_data_comparator(void *E1, void *E2);

_int32 ptl_peersn_cache_data_comparator(void *E1, void *E2);

_int32 ptl_init_get_peersn(void);

_int32 ptl_uninit_get_peersn(void);

_int32 ptl_get_peersn(const char* peerid, ptl_get_peersn_callback callback, void* user_data);

//因为peerid无法唯一标识一个get_peersn请求，所以必须加上user_data才能唯一标识
_int32 ptl_cancel_get_peersn(const char* peerid, void* user_data);

_int32 ptl_handle_get_peersn_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid);

_int32 ptl_recv_get_peersn_resp_cmd(GET_PEERSN_RESP_CMD* cmd);

BOOL ptl_is_peersn_in_cache(const char* peerid);

_int32 ptl_peersn_in_cache_callback(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid);

_int32 ptl_handle_peersn_cache_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid);

_int32 ptl_cache_peersn(const char* peerid, _u32 sn_ip, _u16 sn_port);

_int32 ptl_erase_get_peersn_data(GET_PEERSN_DATA* data);
#ifdef __cplusplus
}
#endif
#endif

