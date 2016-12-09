/*--------------------------------------------------------------------------*/
/*                               IDENTIFICATION                             */
/*--------------------------------------------------------------------------*/
/*     Filename  : http_data_process.h                                         */
/*     Author     : ZengYuqing                                              */
/*     Project    : EmbedThunder                                         */
/*     Module     : HTTP													*/
/*     Version    : 1.3  													*/
/*--------------------------------------------------------------------------*/
/*                  Shenzhen XunLei Networks			                    */
/*--------------------------------------------------------------------------*/
/*                  - C (copyright) - www.xunlei.com -		     		    */
/*                                                                          */
/*      This document is the proprietary of XunLei                          */
/*                                                                          */
/*      All rights reserved. Integral or partial reproduction               */
/*      prohibited without a written authorization from the                 */
/*      permanent of the author rights.                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
/*                         FUNCTIONAL DESCRIPTION                           */
/*--------------------------------------------------------------------------*/
/* This file contains the http data process defined types and platforms       */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*                              HISTORY                                     */
/*--------------------------------------------------------------------------*/
/*   Date     |    Author   | Modification                                  */
/*--------------------------------------------------------------------------*/
/* 2009.03.05 | ZengYuqing  | Creation                                      */
/*--------------------------------------------------------------------------*/

#if !defined(__HTTP_DATA_PROCESS_H_20090305)
#define __HTTP_DATA_PROCESS_H_20090305

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

#include "http_data_pipe/http_data_pipe.h"


//#define HTTP_DEBUG  

/*--------------------------------------------------------------------------*/
/*                Structures definition                                     */
/*--------------------------------------------------------------------------*/


/*  states for chunked      */
enum HTTP_PIPE_CHUNKED_STATE
{
/*
	HTTP_PIPE_CHUNKED_STATE_NONE = 0,
	HTTP_PIPE_CHUNKED_STATE_BREAK,
	HTTP_PIPE_CHUNKED_STATE_BREAK_II,
	HTTP_PIPE_CHUNKED_STATE_HEAD,
	HTTP_PIPE_CHUNKED_STATE_RANGE_BREAK,
	HTTP_PIPE_CHUNKED_STATE_RANGE_BREAK_NEXT,
	HTTP_PIPE_CHUNKED_STATE_END
*/
	//HTTP_CHUNKED_STATE_NONE = 0,
	HTTP_CHUNKED_STATE_RECEIVE_HEAD,
	HTTP_CHUNKED_STATE_RECEIVE_DATA,
	HTTP_CHUNKED_STATE_END
};

#define CHUNKED_HEADER_BUFFER_LEN 12
#define CHUNKED_HEADER_DEFAULT_LEN 3
#define CHUNKED_TEMP_BUFFER_LEN 8

/* Structure for http response chunked data       */
typedef struct
{       
	enum HTTP_PIPE_CHUNKED_STATE _state;
	_u32 _valid_data_len;
	_u32 _last_pack_not_rec_len;
	_u32 _last_pack_not_rec_next_range_len;
	
	_u32 _pack_len;
	_u32 _pack_received_len;
	
	char 		_head_buffer[CHUNKED_HEADER_BUFFER_LEN]; /*  buffer for reveiving every header of a chunked package */
	char 		_temp_buffer[CHUNKED_TEMP_BUFFER_LEN]; /*  temp buffer for chunked */

	_u32		_head_buffer_end_pos;  /* the length of used buffer in _head_buffer */
	_u32 		_temp_buffer_end_pos; /*  the length of used buffer in  temp buffer */

}HTTP_RESPN_DATA_CHUNKED;

/* Structure for http response data       */
typedef struct
{       
	HTTP_RESPN_DATA_CHUNKED * p_chunked;
	RANGE		_request_range; /* a copy of  uncomplete_head_range, 以防正在下载的时候被change_range 而导致本次请求的数据的初始地址和长度计算有误 */
	_u64		_content_length; /* _end_pos - _start_pos  = content lenght,  本次向服务器申请传收的数据字节数*/
	_u64		_recv_end_pos; /* the length of data already received in _content_length */

	char* 		_data_buffer; /*  _data_buffer get from data manager*/
	char* 		_temp_data_buffer; /*  temp buffer when data is full */

	_u32 		_data_buffer_length; /* the length of _data_buffer get from data manager  */
	_u32		_data_buffer_end_pos;  /* the length of used buffer in _data_buffer */
	_u32 		_data_length; /* the length of data, 本次向socket接受的数据字节数= _p_http_pipe->_data_pipe._dispatch_info._down_range_num*HTTP_DEFAULT_RANGE_LEN*/
	_u32 		_temp_data_buffer_length; /* the length of temp buffer */
	BOOL 					_b_received_0_byte;
	BOOL		_parse_file_content;
}HTTP_RESPN_DATA;


struct tag_HTTP_DATA_PIPE;



/*--------------------------------------------------------------------------*/
/*                Call back functions                                       */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*                Internal functions							            */
/*--------------------------------------------------------------------------*/



/*--------------------------------------------------------------------------*/
/*                Interface provided by other modules							            */
/*--------------------------------------------------------------------------*/
_int32 http_pipe_get_request_range( struct tag_HTTP_DATA_PIPE * p_http_pipe ,_u64 * p_start_pos,_u64 * p_end_pos);
_int32 http_pipe_get_down_range( struct tag_HTTP_DATA_PIPE * p_http_pipe );
_int32 http_pipe_parse_file_content( struct tag_HTTP_DATA_PIPE * p_http_pipe );
_int32 http_pipe_parse_body( struct tag_HTTP_DATA_PIPE * p_http_pipe );
_int32 http_pipe_set_file_size( struct tag_HTTP_DATA_PIPE * p_http_pipe ,_u64 result );
_u64 http_pipe_get_file_size( struct tag_HTTP_DATA_PIPE * p_http_pipe );
_int32 http_pipe_get_buffer( struct tag_HTTP_DATA_PIPE * p_http_pipe );
_int32 http_pipe_store_temp_data_buffer( struct tag_HTTP_DATA_PIPE * p_http_pipe );
_int32 http_pipe_down_range_success( struct tag_HTTP_DATA_PIPE * p_http_pipe);
_int32 http_pipe_update_down_range( struct tag_HTTP_DATA_PIPE * p_http_pipe,_u64 received_pos);
_int32 http_pipe_reset_respn_data(struct tag_HTTP_DATA_PIPE * p_http_pipe, HTTP_RESPN_DATA * p_respn_data );
_int32 http_pipe_create_chunked(HTTP_RESPN_DATA * p_respn_data);
_int32 http_pipe_delete_chunked(HTTP_RESPN_DATA * p_respn_data);
_int32 http_pipe_parse_chunked_header( struct tag_HTTP_DATA_PIPE * p_http_pipe ,char* header,_u32 header_len);
_int32 http_pipe_parse_chunked_data( struct tag_HTTP_DATA_PIPE * p_http_pipe );
//_int32 http_pipe_parse_chunked_data( struct tag_HTTP_DATA_PIPE * p_http_pipe );
 _int32 http_pipe_store_chunked_temp_data_buffer( struct tag_HTTP_DATA_PIPE * p_http_pipe );
_u32 http_pipe_get_receive_len(struct tag_HTTP_DATA_PIPE * p_http_pipe);
_u32 http_pipe_get_download_range_index(struct tag_HTTP_DATA_PIPE * p_http_pipe);
_u32 http_pipe_get_download_range_num(struct tag_HTTP_DATA_PIPE * p_http_pipe);







#ifdef __cplusplus
}
#endif

#endif /* __HTTP_DATA_PROCESS_H_20090305 */
