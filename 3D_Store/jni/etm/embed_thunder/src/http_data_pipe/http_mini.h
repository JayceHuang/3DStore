/*
 * Copyright ï¿?008 Nokia Corporation.
 */

#ifndef __HTTP_MINI_H_20100701__
#define __HTTP_MINI_H_20100701__

#include "http_data_pipe/http_mini_interface.h"
#include "http_data_pipe/http_data_pipe_interface.h"
#include "http_data_pipe/http_data_pipe.h"
#include "data_manager/file_manager_interface.h"
#include "platform/sd_socket.h"
#include "asyn_frame/msg.h"
#include "utility/define.h"

#define HTTP_CANCEL	10

struct tagDATA_PIPE;
struct _tag_range;
struct tagRESOURCE;
struct _tag_range_data_buffer;
struct notify_read_result;

typedef enum tagMINI_HTTP_STATE{MINI_HTTP_RUNNING, MINI_HTTP_SUCCESS, MINI_HTTP_FAILED} MINI_HTTP_STATE;
typedef struct t_http_mini_header
{
	_u32 	_status_code;
	char 	_last_modified[64];  
	char 	_cookie[1024]; 
	char 	_content_encoding[32];	
	_u64 	_content_length;
} HTTP_MINI_HEADER;

typedef struct tagMINI_HTTP
{
	_u32 _id;
	MINI_HTTP_STATE _state;
	HTTP_PARAM _http_param;
	HTTP_DATA_PIPE* _http_pipe;
	RESOURCE* _res;
	_u64 _dledsize;
	_u64 _filesize;
	_u64 _sentsize;
	BOOL _notified;
	_u8 * _current_send_data;
	_u8 * _current_recv_data;
	_u32 _start_time;
	BOOL _wait_delete;
	BOOL _send_data_need_notified;
	BOOL _recv_data_need_notified;
}MINI_HTTP;

_int32 mini_http_id_comp(void *E1, void *E2);

//_int32 mini_http_build_get_request(char** buffer, _u32* len, MINI_HTTP * p_mini_http);
//_int32 mini_http_build_post_request(char** buffer, _u32* len, MINI_HTTP * p_mini_http);

_int32 mini_http_handle_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid);


//»Øµ÷º¯Êý
_int32 mini_http_change_range(struct tagDATA_PIPE *p_data_pipe, const struct _tag_range *range, BOOL cancel_flag);

_int32 mini_http_get_dispatcher_range(struct tagDATA_PIPE *p_data_pipe, const struct _tag_range *p_dispatcher_range, void *p_pipe_range);

_int32 mini_http_set_dispatcher_range(struct tagDATA_PIPE *p_data_pipe, const void *p_pipe_range, struct _tag_range *p_dispatcher_range);

_u64 mini_http_get_file_size( struct tagDATA_PIPE *p_data_pipe);

_int32 mini_http_set_file_size( struct tagDATA_PIPE *p_data_pipe, _u64 filesize);

_int32 mini_http_get_data_buffer( struct tagDATA_PIPE *p_data_pipe, char **pp_data_buffer, _u32 *p_data_len);

_int32 mini_http_free_data_buffer( struct tagDATA_PIPE *p_data_pipe, char **pp_data_buffer, _u32 data_len);

_int32 mini_http_put_data_buffer( struct tagDATA_PIPE *p_data_pipe, const struct _tag_range *p_range, char **pp_data_buffer, _u32 data_len,_u32 buffer_len, struct tagRESOURCE *p_resource );

_int32 mini_http_read_data( struct tagDATA_PIPE *p_data_pipe, void *p_user, struct _tag_range_data_buffer* p_data_buffer, notify_read_result p_call_back);

_int32 mini_http_notify_dispatch_data_finish( struct tagDATA_PIPE *p_data_pipe);

_int32 mini_http_get_file_type( struct tagDATA_PIPE *p_data_pipe);

_int32 mini_http_get_post_data(struct tagDATA_PIPE* p_data_pipe,_u8 ** data,_u32 *data_len);
_int32 mini_http_notify_sent_data(struct tagDATA_PIPE* p_data_pipe,_u32 sent_data_len,BOOL * send_end);


_int32 mini_http_set_header( struct tagDATA_PIPE *p_data_pipe, HTTP_RESPN_HEADER * p_header,_int32 status_code);

_int32 mini_http_malloc(MINI_HTTP **pp_slip);
_int32 mini_http_free(MINI_HTTP * p_slip);
_int32 mini_http_reset_id(void);
_u32 mini_http_create_id(void);
_int32 mini_http_decrease_id(void);
MINI_HTTP * mini_http_get_from_map(_u32 id);
_int32 mini_http_add_to_map(MINI_HTTP * p_mini_http);
_int32 mini_http_remove_from_map(MINI_HTTP * p_mini_http);
_int32 mini_http_clear_map(void);
_int32 mini_http_close(MINI_HTTP * p_mini_http,_u32 close_code);
_int32 mini_http_cancel(MINI_HTTP * p_mini_http);
_int32 mini_http_scheduler(void);


#endif // __HTTP_MINI_H_20100701__
