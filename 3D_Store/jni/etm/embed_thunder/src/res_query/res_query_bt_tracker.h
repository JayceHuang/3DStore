#ifndef _RES_QUERY_BT_TRACKER_H_
#define	_RES_QUERY_BT_TRACKER_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#ifdef ENABLE_BT

#ifdef ENABLE_BT_PROTOCOL

#include "asyn_frame/msg.h"
#include "bt_download/bt_utility/bt_define.h"
#include "data_manager/file_manager_interface.h"
#include "platform/sd_socket.h"

struct tagDATA_PIPE;
struct _tag_range;
struct tagRESOURCE;
struct _tag_range_data_buffer;
struct notify_read_result;

typedef _int32 (*res_query_bt_tracker_handler)(void* user_data,_int32 errcode);
typedef _int32 (*res_query_add_bt_res_handler)(void* user_data, _u32 host_ip, _u16 port);

typedef enum tagBT_TRACKER_STATE{BT_TRACKER_RUNNING, BT_TRACKER_SUCCESS, BT_TRACKER_FAILED} BT_TRACKER_STATE;

typedef struct tagBT_TRACKER_CMD
{
	void*			_callback;
	void*			_user_data;
	char			_tracker_url[MAX_URL_LEN];
	_u8				_info_hash[BT_INFO_HASH_LEN];
	BT_TRACKER_STATE _state;
}BT_TRACKER_CMD;

_int32 init_bt_tracker(void);

_int32 uninit_bt_tracker(void);
 
BOOL   res_query_bt_tracker_exist(void* user_data);

_int32 res_query_register_add_bt_res_handler_impl(res_query_add_bt_res_handler bt_peer_handler);

_int32 res_query_bt_tracker_impl(void* user_data, res_query_bt_tracker_handler callback, const char *url, _u8* info_hash);

_int32 bt_tracker_cancel_query(void* user_data);

_int32 bt_tracker_query(void);

_int32 bt_tracker_build_query_cmd(char** buffer, _u32* len, BT_TRACKER_CMD* cmd);

_int32 bt_tracker_handle_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid);

_int32 bt_tracker_handle_result(_int32 errcode);

//»Øµ÷º¯Êý
_int32 bt_tracker_change_range(struct tagDATA_PIPE *p_data_pipe, const struct _tag_range *range, BOOL cancel_flag);

_int32 bt_tracker_get_dispatcher_range(struct tagDATA_PIPE *p_data_pipe, const struct _tag_range *p_dispatcher_range, void *p_pipe_range);

_int32 bt_tracker_set_dispatcher_range(struct tagDATA_PIPE *p_data_pipe, const void *p_pipe_range, struct _tag_range *p_dispatcher_range);

_u64 bt_tracker_get_file_size( struct tagDATA_PIPE *p_data_pipe);

_int32 bt_tracker_set_file_size( struct tagDATA_PIPE *p_data_pipe, _u64 filesize);

_int32 bt_tracker_get_data_buffer( struct tagDATA_PIPE *p_data_pipe, char **pp_data_buffer, _u32 *p_data_len);

_int32 bt_tracker_free_data_buffer( struct tagDATA_PIPE *p_data_pipe, char **pp_data_buffer, _u32 data_len);

_int32 bt_tracker_put_data_buffer( struct tagDATA_PIPE *p_data_pipe, const struct _tag_range *p_range, char **pp_data_buffer, _u32 data_len,_u32 buffer_len, struct tagRESOURCE *p_resource );

_int32 bt_tracker_read_data( struct tagDATA_PIPE *p_data_pipe, void *p_user, struct _tag_range_data_buffer* p_data_buffer, notify_read_result p_call_back);

_int32 bt_tracker_notify_dispatch_data_finish( struct tagDATA_PIPE *p_data_pipe);

_int32 bt_tracker_get_file_type( struct tagDATA_PIPE *p_data_pipe);
#endif
#endif

#ifdef __cplusplus
}
#endif
#endif

