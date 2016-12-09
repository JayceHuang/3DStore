/*--------------------------------------------------------------------------*/
/*                               IDENTIFICATION                             */
/*--------------------------------------------------------------------------*/
/*     Filename   : ftp_data_pipe.h                               */
/*     Author     : ZengYuqing                                              */
/*     Project    : EmbedThunder                                         */
/*     Module     : FTP  													*/
/*     Version    : 1.0  													*/
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
/* This file contains the data pipe defined types and platforms           */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*                              HISTORY                                     */
/*--------------------------------------------------------------------------*/
/*   Date     |    Author   | Modification                                  */
/*--------------------------------------------------------------------------*/
/* 2008.06.14 | ZengYuqing  | Creation                                      */
/*--------------------------------------------------------------------------*/

#if !defined(__FTP_DATA_PIPE_H_20080614)
#define __FTP_DATA_PIPE_H_20080614

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



//#define FTP_DEBUG  

/*--------------------------------------------------------------------------*/
/*                Structures definition                                     */
/*--------------------------------------------------------------------------*/

/*  states of ftp data pipe      */
enum FTP_PIPE_STATE
{
	FTP_PIPE_STATE_UNKNOW = -1,
	FTP_PIPE_STATE_IDLE = 0,
	FTP_PIPE_STATE_RETRY_WAITING,
	FTP_PIPE_STATE_CONNECTING,
	FTP_PIPE_STATE_CONNECTED,
	FTP_PIPE_STATE_REQUESTING,
	FTP_PIPE_STATE_READING,
	FTP_PIPE_STATE_RANGE_CHANGED,
	FTP_PIPE_STATE_DOWNLOADING,
	FTP_PIPE_STATE_DOWNLOAD_COMPLETED,
	FTP_PIPE_STATE_CLOSING
//	FTP_PIPE_STATE_SET_FILE_WAITING
};

/* FTP commands from pipe to server      */
enum FTP_COMMAND
{
     FTP_COMMAND_USER =  1,
     FTP_COMMAND_PASWD,
     FTP_COMMAND_FEAT ,
     FTP_COMMAND_CWD ,
     FTP_COMMAND_TYPE_I ,
     FTP_COMMAND_MDTM ,
     FTP_COMMAND_SIZE,
     FTP_COMMAND_PASV  ,
     FTP_COMMAND_REST ,
     FTP_COMMAND_RETR  ,
     FTP_COMMAND_ABOR ,
     FTP_COMMAND_QUIT  
};

 /* Structure for ftp data  receiving     */
typedef struct
{       
	char* 					_buffer; /*  _data_buffer get from data manager*/
	_u32 					_buffer_length; /* the length of _data_buffer get from data manager = _current_recving_range._num*FTP_DEFAULT_RANGE_LEN */
	_u32					_buffer_end_pos;  /* the length of used buffer in _data_buffer */
	_u32 					_data_length; /* the length of _data */

	RANGE					_current_recving_range; /* FTP_DEFAULT_RANGE_NUM of _current_range (a piece of _current_range) */
	_u64					_content_length; /* = content lenght  */
	_u64					_recv_end_pos; /* the length of data already received in _content_length */

}FTP_DATA;


/* Structure for ftp response        */
typedef struct
{       
	char* 					_buffer; /*  respns-buffer for reading */
	_u32 					_buffer_length; /* the length of respns-buffer  = FTP_RESPN_DEFAULT_LEN */
	_u32					_buffer_end_pos;  /* the length of used buffer in respns-buffer */


	_u32  _code;
	char * _description; 

}FTP_RESPONSE;

/* Struct for passive mode */
typedef struct 
{
	_u32 					_socket_fd;  /* The socket id for data transfer from socket module */
	enum FTP_PIPE_STATE 					_state;
	BOOL    					_is_connected; 
	_u32  					_try_count;
	_u32 					_retry_timer_id;

	char 				_PASV_ip[MAX_HOST_NAME_LEN];
	_u32 				_PASV_port;
	
}FTP_PASSIVE;

typedef struct tag_FTP_SERVER_RESOURCE
{
       RESOURCE			_resource;
   	URL_OBJECT			_url;
#ifdef _DEBUG
	char 				_o_url[MAX_URL_LEN];
#endif
	//char 				_lastmodified_from_server[MAX_LAST_MODIFIED_LEN];
	_u64 				_file_size;	
  	BOOL 				_b_is_origin;
	BOOL 				_b_file_exist;
  	BOOL 				_b_path_changed;
	BOOL 				_is_support_range;

//	char 				_PASV_ip[MAX_HOST_NAME_LEN];   // Move to FTP_DATA_PIPE
//	_u32 				_PASV_port;					    // Move to FTP_DATA_PIPE
	
}FTP_SERVER_RESOURCE ;


/* Structure for ftp data pipe       */
typedef struct 
{   
	DATA_PIPE				_data_pipe;  /* Base data pipe */
	FTP_SERVER_RESOURCE *	_p_ftp_server_resource;
	FTP_RESPONSE			_response;
	FTP_DATA				_data;
	FTP_PASSIVE				_pasv;
	enum	FTP_PIPE_STATE 			_ftp_state;
//	RANGE					_current_range;  /* replaced by  _data_pipe._dispatch_info._down_range*/
//	char* 					_o_url;
	_u64 					_sum_recv_bytes;
	_u32 					_socket_fd;  /* The socket id from socket module */
	_u32  					_already_try_count;
	_u32  					_get_buffer_try_count;
	_int32     					_error_code;
	_u32 					_retry_connect_timer_id;
	_u32 					_retry_get_buffer_timer_id;
	_u32 					_retry_set_file_size_timer_id;
	BOOL     					_b_ranges_changed;
	BOOL 					_b_data_full;
	BOOL    					_wait_delete;
	BOOL 					_is_connected;
	BOOL 					_b_retry_set_file_size;
	
	_int32						_cur_command_id;

	char * 					_ftp_request;
	_u32 					_ftp_request_buffer_len;

	SPEED_CALCULATOR		_speed_calculator;

	_u32 					_reveive_range_num;
	_u32 					_reveive_block_len;
	enum URL_CONVERT_STEP _request_step;
	BOOL 					_b_retry_request;
}FTP_DATA_PIPE;

/*--------------------------------------------------------------------------*/
/*                Call back functions                                       */
/*--------------------------------------------------------------------------*/

 _int32 ftp_pipe_handle_connect( _int32 _errcode, _u32 notice_count_left,void *user_data);
 _int32 ftp_pipe_handle_pasv_connect( _int32 _errcode, _u32 notice_count_left,void *user_data);
 _int32 ftp_pipe_handle_send( _int32 _errcode, _u32 notice_count_left,const char* buffer, _u32 had_send,void *user_data);
 _int32 ftp_pipe_handle_recv( _int32 _errcode, _u32 notice_count_left,char* buffer, _u32 had_recv,void *user_data);
 _int32 ftp_pipe_handle_uncomplete_recv( _int32 _errcode, _u32 notice_count_left,char* buffer, _u32 had_recv,void *user_data);

 _int32 ftp_pipe_handle_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid);

/*--------------------------------------------------------------------------*/
/*                Internal functions							            */
/*--------------------------------------------------------------------------*/

	_int32 ftp_pipe_failure_exit(FTP_DATA_PIPE * _p_ftp_pipe);
	_int32 ftp_pipe_send_command(FTP_DATA_PIPE * _p_ftp_pipe,_int32 _command_id);
	_int32 ftp_pipe_close_connection(FTP_DATA_PIPE * _p_ftp_pipe);
	_int32 ftp_pipe_open_pasv(FTP_DATA_PIPE * _p_ftp_pipe);
	_int32 ftp_pipe_close_pasv(FTP_DATA_PIPE * _p_ftp_pipe);
	_int32 ftp_pipe_get_line( const char* ptr, _u32 str_len, _u32 * line_len );
	_int32 ftp_pipe_parse_line( const char* line_ptr, _u32 line_len, _int32 * code, char * text_line );
 	_int32 ftp_pipe_parse_response(FTP_DATA_PIPE * _p_ftp_pipe);

	_int32 ftp_pipe_get_buffer( FTP_DATA_PIPE * _p_ftp_pipe );
	_int32 ftp_pipe_recving_range_success( FTP_DATA_PIPE * _p_ftp_pipe);
	_int32 ftp_pipe_range_success( FTP_DATA_PIPE * _p_ftp_pipe);

	_int32 ftp_pipe_reset_data(FTP_DATA_PIPE * _p_ftp_pipe,FTP_DATA * _p_data );
	_int32 ftp_pipe_reset_response( FTP_RESPONSE * _p_respn );
	_int32 ftp_pipe_reset_server_resource( FTP_SERVER_RESOURCE * _p_resource );
	_int32 ftp_pipe_reset_pipe( FTP_DATA_PIPE * _p_ftp_pipe );
	 _int32 ftp_pipe_destroy(FTP_DATA_PIPE * _p_ftp_pipe);



/*--------------------------------------------------------------------------*/
/*                Interface provided by other modules							            */
/*--------------------------------------------------------------------------*/
#ifdef FTP_DEBUG
#include "../test/ftp_data_pipe/test_ftp_data_pipe.h"
/* From Socket Proxy */
#define SOCKET_PROXY_CREATE ftp_socket_create
#define SOCKET_PROXY_CONNECT ftp_socket_open
#define SOCKET_PROXY_SEND ftp_socket_write
#define SOCKET_PROXY_RECV ftp_socket_read
#define SOCKET_PROXY_UNCOMPLETE_RECV ftp_socket_uncom_read
#define SOCKET_PROXY_CANCEL 
#define SOCKET_PROXY_CLOSE ftp_socket_close
/* From Data Manager */
//#define DM_PUT_DATA ftp_dm_put_data
//#define DM_GET_DATA_BUFFER ftp_dm_get_data_buffer
//#define DM_FREE_DATA_BUFFER ftp_dm_free_data_buffer
/* From Timer*/
#define START_TIMER ftp_start_timer
#define CANCEL_TIMER ftp_cancel_timer
#else
#ifdef __FTP_DATA_PIPE_C_20080614
/* From Socket Proxy */
#define SOCKET_PROXY_CREATE socket_proxy_create
#define SOCKET_PROXY_CONNECT socket_proxy_connect_by_domain
#define SOCKET_PROXY_SEND socket_proxy_send
#define SOCKET_PROXY_RECV socket_proxy_recv
#define SOCKET_PROXY_UNCOMPLETE_RECV socket_proxy_uncomplete_recv
#define SOCKET_PROXY_CANCEL socket_proxy_cancel
#define SOCKET_PROXY_CLOSE socket_proxy_close
#define SOCKET_PROXY_GET_OP_COUNT socket_proxy_peek_op_count
/* From Data Manager */
//#define DM_PUT_DATA dm_put_data
//#define DM_GET_DATA_BUFFER dm_get_data_buffer
//#define DM_FREE_DATA_BUFFER dm_free_data_buffer
/* From Timer*/
#define START_TIMER start_timer
#define CANCEL_TIMER cancel_timer
#endif /* __FTP_DATA_PIPE_C_20080614 */
#endif


#ifdef __cplusplus
}
#endif

#endif /* __FTP_DATA_PIPE_H_20080614 */
