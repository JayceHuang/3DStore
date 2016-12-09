#if !defined(__HTTP_DATA_PIPE_H_20080614)
#define __HTTP_DATA_PIPE_H_20080614

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#include "utility/errcode.h"
#include "utility/mempool.h"
#include "utility/test.h"
#include "utility/time.h"
#include "utility/utility.h"
#include "utility/string.h"
#include "utility/url.h"
#include "utility/range.h"
#include "asyn_frame/msg.h"
#include "utility/speed_calculator.h"

#include "dispatcher/dispatcher_interface.h"
#include "connect_manager/data_pipe.h"

#include "socket_proxy_interface.h"

#include "http_data_pipe/http_data_pipe_interface.h"
#include "http_data_pipe/http_data_process.h"
#include "http_data_pipe/http_protocol.h"
#include "http_data_pipe/http_resource.h"

#ifdef _ENABLE_SSL
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define BIO_FREE_ALL(pbio) do { BIO_free_all(pbio); pbio = NULL;} while(0)
#endif

extern _int32 pt_origion_pos_to_relation_pos(RELATION_BLOCK_INFO* p_block_info_arr, _u32 block_num, _u64 origion_pos, _u64* relation_pos);

//#define HTTP_DEBUG  

/*--------------------------------------------------------------------------*/
/*                Structures definition                                     */
/*--------------------------------------------------------------------------*/

/*  states of http data pipe      */
enum HTTP_PIPE_STATE
{
    HTTP_PIPE_STATE_UNKNOW = -1,
    HTTP_PIPE_STATE_IDLE = 0,
    HTTP_PIPE_STATE_RETRY_WAITING,
    HTTP_PIPE_STATE_CONNECTING,
    HTTP_PIPE_STATE_CONNECTED,
    HTTP_PIPE_STATE_REQUESTING,
    HTTP_PIPE_STATE_READING,
    HTTP_PIPE_STATE_RANGE_CHANGED,
    HTTP_PIPE_STATE_DOWNLOAD_COMPLETED,
    HTTP_PIPE_STATE_CLOSING,
    HTTP_PIPE_STATE_RANGE_CHANGED_NEED_RECONNECT
};

/* Structure for http data pipe       */
typedef struct tag_HTTP_DATA_PIPE
{
    DATA_PIPE _data_pipe;  /* Base data pipe */
    HTTP_RESPN_HEADER _header;
    HTTP_RESPN_DATA _data;
    SPEED_CALCULATOR _speed_calculator;
    HTTP_SERVER_RESOURCE* _p_http_server_resource;
    _u64 _sum_recv_bytes;
    _u64 _req_end_pos;
    enum HTTP_PIPE_STATE _http_state;
    enum URL_CONVERT_STEP _request_step;
    char* _http_request;
    _u32 _socket_fd;  /* The socket id from socket module */
    _u32 _already_try_count;
    _u32 _max_try_count;
    _u32 _get_buffer_try_count;
    _u32 _retry_connect_timer_id;
    _u32 _retry_get_buffer_timer_id;
    _u32 _http_request_buffer_len;
    _u32 _http_request_len;
    _u32 _retry_set_file_size_timer_id;
    _int32 _error_code;
    BOOL _b_ranges_changed;
    BOOL _b_data_full;
    BOOL _wait_delete;
    BOOL _b_header_received;
    BOOL _b_redirect;
    BOOL _is_connected;
    BOOL _b_retry_set_file_size;
    BOOL _b_retry_request;
    BOOL _b_set_request;
    BOOL _b_get_file_size_after_data;
    BOOL _b_retry_request_not_discnt;
    BOOL _rev_wap_page;
    BOOL _is_post;
    BOOL _super_pipe;
#ifdef _ENABLE_SSL
	BOOL _is_https;
	BIO* _pbio;
#endif
}HTTP_DATA_PIPE;


#define HTTP_SET_PIPE_STATE(state) dp_set_state(&p_http_pipe->_data_pipe, state)
#define HTTP_GET_PIPE_STATE  (p_http_pipe->_data_pipe._dispatch_info._pipe_state)
#define HTTP_SET_STATE(state) {p_http_pipe->_http_state=state;}
#define HTTP_GET_STATE  (p_http_pipe->_http_state)
#define HTTP_ERR_CODE p_http_pipe->_error_code

#define HTTP_SPEED (p_http_pipe->_data_pipe._dispatch_info._speed)
#define HTTP_TIME_STAMP (p_http_pipe->_data_pipe._dispatch_info._time_stamp)

/*--------------------------------------------------------------------------*/
/*                Call back functions                                       */
/*--------------------------------------------------------------------------*/
_int32 http_pipe_handle_connect( _int32 errcode, _u32 notice_count_left,void *user_data);
_int32 http_pipe_handle_send( _int32 errcode, _u32 notice_count_left,const char* buffer, _u32 had_send,void *user_data);
_int32 http_pipe_handle_uncomplete_recv( _int32 errcode, _u32 notice_count_left,char* buffer, _u32 had_recv,void *user_data);
_int32  http_pipe_handle_recv( _int32 errcode, _u32 notice_count_left,char* buffer, _u32 had_recv,void *user_data);

_int32 http_pipe_handle_retry_connect_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid);
_int32 http_pipe_handle_retry_get_buffer_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid);
_int32 http_pipe_handle_retry_set_file_size_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid);



/*--------------------------------------------------------------------------*/
/*                Internal functions							            */
/*--------------------------------------------------------------------------*/

_int32 http_pipe_failure_exit(HTTP_DATA_PIPE * p_http_pipe,_int32 err_code);
_int32 http_pipe_handle_recv_header(HTTP_DATA_PIPE * p_http_pipe, _u32 had_recv);
_int32 http_pipe_handle_recv_chunked(HTTP_DATA_PIPE * p_http_pipe, _u32 had_recv);
_int32 http_pipe_handle_recv_body(HTTP_DATA_PIPE * p_http_pipe,_u32 had_recv);
_int32 http_pipe_handle_recv_0_byte(HTTP_DATA_PIPE * p_http_pipe);
_int32 http_pipe_handle_recv_2249(HTTP_DATA_PIPE * p_http_pipe, _u32 had_recv);
_int32 http_pipe_do_next(HTTP_DATA_PIPE * p_http_pipe);
_int32 http_pipe_reconnect(HTTP_DATA_PIPE * p_http_pipe);
_int32 http_pipe_do_connect(HTTP_DATA_PIPE * p_http_pipe);
_int32 http_pipe_close_connection(HTTP_DATA_PIPE * p_http_pipe);
_int32 http_pipe_send_request(HTTP_DATA_PIPE * p_http_pipe);
_int32 http_pipe_parse_response(HTTP_DATA_PIPE * p_http_pipe);
_int32 http_pipe_parse_status_code( HTTP_DATA_PIPE * p_http_pipe );
_int32 http_pipe_continue_reading(HTTP_DATA_PIPE * p_http_pipe);
//_int32 http_pipe_convert_full_paths( HTTP_DATA_PIPE * p_http_pipe,char* str_buffer ,_int32 buffer_len );
//_int32 http_pipe_convert_full_paths_to_step( HTTP_DATA_PIPE * p_http_pipe,char* str_buffer ,_int32 buffer_len );
_int32 http_pipe_reset_pipe( HTTP_DATA_PIPE * p_http_pipe );
BOOL http_pipe_is_binary_file( HTTP_DATA_PIPE * p_http_pipe );
_int32 http_pipe_set_can_download_ranges( HTTP_DATA_PIPE * p_http_pipe,_u64 file_size );
URL_OBJECT* http_pipe_get_url_object(HTTP_DATA_PIPE * p_http_pipe);
_int32  http_pipe_correct_uncomplete_head_range_for_not_support_range(HTTP_DATA_PIPE * p_http_pipe);
 _u32 http_pipe_get_buffer_wait_timeout( HTTP_DATA_PIPE * p_http_pipe );


/*--------------------------------------------------------------------------*/
/*                Interface provided by other modules							            */
/*--------------------------------------------------------------------------*/


#ifdef __cplusplus
}
#endif

#endif /* __HTTP_DATA_PIPE_H_20080614 */
