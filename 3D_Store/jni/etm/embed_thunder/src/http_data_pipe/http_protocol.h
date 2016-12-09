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
	char* 	_header_buffer; 			  	/*  ָ���socket�н��ܵ�����Ӧ������ʼ��ַ */
	_u32 	_header_buffer_length; 	  	/* ������Ӧ���ڴ泤�ȣ�ע�ⲻ����Ӧ�����ֽ��� */
	_u32	_header_buffer_end_pos;  	/* ���ܵ�����Ӧ�����ֽ�����Ӧ<= _header_buffer_length*/
	
	_u32  _status_code;
	BOOL 	_is_binary_file;				/* ���"Content-Type"��ֵΪ"text/html"���ֵΪtrue,����Ϊfalse */ 
	BOOL _is_chunked;/* "Transfer-Encoding" */ 
	char*	_location;  					/* "Location"��ָ���ض���url��_header_buffer�е���ʼ��ַ�����ĳ���Ϊ_location_len */
	_u64 	_entity_length;				/* "Content-Range" ,�����ļ��Ĵ�С������Ϊ0��_has_entity_lengthΪfalse���������û�з�����һ�ֶ�*/ 
	_u64 	_content_length;				/* "Content-Length" ,������Ӧ�������ݲ��ֵĴ�С,����Ϊ0��_has_content_lengthΪfalse���������û�з�����һ�ֶ� */ 
	BOOL _is_support_range;   /* "Content-Range" */
	char* 	_server_return_name;		/* "Content-Disposition" ��ָ���ļ�����_header_buffer�е���ʼ��ַ�����ĳ���Ϊ_server_return_name_len */
	BOOL _is_support_long_connect; /*"Connection" */
	char* 	_last_modified;  				/* ָ��last_modified��ֵ��_header_buffer�е���ʼ��ַ�����ĳ���Ϊ_last_modified_len */
	char* 	_cookie;   					/*  ָ��cookie��ֵ��_header_buffer�е���ʼ��ַ�����ĳ���Ϊ_cookie_len */
	char* 	_content_encoding;			/*  ָ��Content-Encoding��ֵ��_header_buffer�е���ʼ��ַ�����ĳ���Ϊ_content_encoding_len */

	char* 	_body_start_pos; 			/*  ��Ӧ����Ҳ�п��ܽ��ܵ�С�����ݣ����ָ��ָ�����ݿ�λ��_header_buffer�е���ʼ��ַ�����ĳ���Ϊ_body_len��ע�⣺����һ��_u8���͵����ݣ���һ�����ַ���������ʱ������strcpy���� ��Ҫ��memcpy*/
	_u32 	_body_len; 					/*   ��Ӧ���и������ܵ������ݵĳ��� */
	
	BOOL 	_has_entity_length;			/* ���Ϊfalse���������û�з�����һ�ֶ� */
	BOOL 	_has_content_length; 		/* ���Ϊfalse���������û�з�����һ�ֶ� */
	_u32 	_location_len; /* the length of _location */
	_u32 	_server_return_name_len; /* the length of _server_return_name */
	_u32 	_last_modified_len; /* the length of _last_modified */
	_u32 	_cookie_len; /* the length of _cookie */
	_u32 	_content_encoding_len; /* the length of _content_encoding */

	BOOL 	_is_vnd_wap;  /*  just for cmwap */

    /*_content_range_length�� has_entity_lengthΪ���ʱ�����ֵ�������ã�
    range�������ص�ʱ��ʵ�ʵķ��ص�range���ȡ���Ϊ��ʹ��range�������أ�����206��������ܻ��ǻ�
    ��content-length����Щ���������content-length�������range���Ȳ�һ�¡����º����߼�����
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
