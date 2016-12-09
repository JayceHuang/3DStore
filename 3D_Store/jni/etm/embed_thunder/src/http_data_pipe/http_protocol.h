/*--------------------------------------------------------------------------*/
/*                               IDENTIFICATION                             */
/*--------------------------------------------------------------------------*/
/*     Filename  : http_protocol.h                                         */
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
/* This file contains the http protocol defined types and platforms       */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*                              HISTORY                                     */
/*--------------------------------------------------------------------------*/
/*   Date     |    Author   | Modification                                  */
/*--------------------------------------------------------------------------*/
/* 2009.03.05 | ZengYuqing  | Creation                                      */
/*--------------------------------------------------------------------------*/

#if !defined(__HTTP_PROTOCOL_H_20090305)
#define __HTTP_PROTOCOL_H_20090305

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



//#define HTTP_DEBUG  

/*--------------------------------------------------------------------------*/
/*                Structures definition                                     */
/*--------------------------------------------------------------------------*/


#define MAX_NUM_OF_FIELD_NAME 12
/*  Name of http header fields       */
enum HTTP_HEAD_FIELD_NAME
{ 
	HHFT_CONTENT_TYPE=0,  		/* "Content-Type" */ 
	HHFT_TRANSFER_ENCODING,  		/* "Transfer-Encoding" */ 
	HHFT_LOCATION,    				/* "Location" */ 
	HHFT_LOCATION2,    				/* "location" */ 
	HHFT_CONTENT_RANGE,  			/* "Content-Range" */ 
	HHFT_CONTENT_LENGTH,    		/* "Content-Length"  */ 
	HHFT_ACCEPT_RANGES,  			/* "Accept-Ranges" */ 
	HHFT_CONTENT_DISPOSITION,     	/* "Content-Disposition" */ 
	HHFT_CONNECTION ,   			/* "Connection" */    		
	HHFT_LAST_MODIFIED,   			/* "Last-Modified"  */ 
	HHFT_SET_COOKIE, 	   			/* "Set-Cookie"  */ 
	HHFT_CONTENT_ENCODING 	   		/* "Content-Encoding"  */ 
};

#ifdef __HTTP_PROTOCOL_C_20090305
const static char *g_http_head_field_name[] = {
	"Content-Type",
	"Transfer-Encoding",
	"Location",
	"location",
	"Content-Range",
	"Content-Length",
	"Accept-Ranges",
	"Content-Disposition" ,
	"Connection" ,
	"Last-Modified" ,
	"Set-Cookie",
	"Content-Encoding"
};
#endif

/* Structure for http response header       */
typedef struct 
{       
	char* 	_header_buffer; 			  	/*  指向从socket中接受到的响应包的起始地址 */
	_u32 	_header_buffer_length; 	  	/* 接受响应的内存长度，注意不是响应包的字节数 */
	_u32	_header_buffer_end_pos;  	/* 接受到的响应包的字节数，应<= _header_buffer_length*/
	
	_u32  _status_code;
	BOOL 	_is_binary_file;				/* 如果"Content-Type"的值为"text/html"则此值为true,否则为false */ 
	BOOL _is_chunked;/* "Transfer-Encoding" */ 
	char*	_location;  					/* "Location"，指向重定向url在_header_buffer中的起始地址，它的长度为_location_len */
	_u64 	_entity_length;				/* "Content-Range" ,整个文件的大小，若果为0且_has_entity_length为false，则服务器没有发回这一字段*/ 
	_u64 	_content_length;				/* "Content-Length" ,整个响应包中数据部分的大小,若果为0且_has_content_length为false，则服务器没有发回这一字段 */ 
	BOOL _is_support_range;   /* "Content-Range" */
	char* 	_server_return_name;		/* "Content-Disposition" ，指向文件名在_header_buffer中的起始地址，它的长度为_server_return_name_len */
	BOOL _is_support_long_connect; /*"Connection" */
	char* 	_last_modified;  				/* 指向last_modified的值在_header_buffer中的起始地址，它的长度为_last_modified_len */
	char* 	_cookie;   					/*  指向cookie的值在_header_buffer中的起始地址，它的长度为_cookie_len */
	char* 	_content_encoding;			/*  指向Content-Encoding的值在_header_buffer中的起始地址，它的长度为_content_encoding_len */

	char* 	_body_start_pos; 			/*  响应包中也有可能接受到小块数据，这个指针指向数据块位于_header_buffer中的起始地址，它的长度为_body_len。注意：这是一块_u8类型的数据，不一定是字符串，拷贝时不能用strcpy函数 ，要用memcpy*/
	_u32 	_body_len; 					/*   响应包中附带接受到的数据的长度 */
	
	BOOL 	_has_entity_length;			/* 如果为false，则服务器没有发回这一字段 */
	BOOL 	_has_content_length; 		/* 如果为false，则服务器没有发回这一字段 */
	_u32 	_location_len; /* the length of _location */
	_u32 	_server_return_name_len; /* the length of _server_return_name */
	_u32 	_last_modified_len; /* the length of _last_modified */
	_u32 	_cookie_len; /* the length of _cookie */
	_u32 	_content_encoding_len; /* the length of _content_encoding */

	BOOL 	_is_vnd_wap;  /*  just for cmwap */

    /*_content_range_length在 has_entity_length为真的时候这个值才起作用，
    range请求下载的时候实际的返回的range长度。因为即使用range请求下载，返回206，后面可能还是会
    带content-length，有些服务器这个content-length和请求的range长度不一致。导致后面逻辑出错。
    Content-Range:bytes 0-499/1234 */
    
    _u64    _content_range_length; 
    BOOL    _is_content_range_length_valid;
}HTTP_RESPN_HEADER;

/*--------------------------------------------------------------------------*/
/*                Call back functions                                       */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/*                Internal functions							            */
/*--------------------------------------------------------------------------*/
_int32 http_create_request(char** request_buffer,_u32* request_buffer_len,char* full_path,char * ref_url,char * host,_u32 port,char* user_name,char* pass_words,char * cookies,_u64 rang_from,_u64 rang_to,BOOL is_browser,
	BOOL is_post,_u8 * post_data,_u32 post_data_len,_u32 * total_send_bytes,BOOL gzip,BOOL post_gzip,_u64 post_content_len);

 _int32 http_parse_header( HTTP_RESPN_HEADER * p_http_respn );
_int32 http_parse_header_search_line_end(  char* _line,_int32 * _pos );

 _int32 http_parse_header_get_status_code( char* status_line,_u32 * status_code );

_int32 http_parse_header_one_line( HTTP_RESPN_HEADER * p_http_respn,  char* line );


_int32 http_parse_header_field_value( HTTP_RESPN_HEADER * p_http_respn, enum HTTP_HEAD_FIELD_NAME field_name, char* field_value );

_int32 http_reset_respn_header( HTTP_RESPN_HEADER * p_respn_header );

void http_header_discard_excrescent( HTTP_RESPN_HEADER * p_respn_header );


#ifdef __cplusplus
}
#endif

#endif /* __HTTP_PROTOCOL_H_20090305 */
