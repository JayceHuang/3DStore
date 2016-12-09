/*--------------------------------------------------------------------------*/
/*                               IDENTIFICATION                             */
/*--------------------------------------------------------------------------*/
/*     Filename  : http_resource.h                                         */
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
/* This file contains the http resource defined types and platforms       */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*                              HISTORY                                     */
/*--------------------------------------------------------------------------*/
/*   Date     |    Author   | Modification                                  */
/*--------------------------------------------------------------------------*/
/* 2009.03.5 | ZengYuqing  | Creation                                      */
/*--------------------------------------------------------------------------*/

#if !defined(__HTTP_RESOURCE_H_20090305)
#define __HTTP_RESOURCE_H_20090305

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





/*--------------------------------------------------------------------------*/
/*                Structures definition                                     */
/*--------------------------------------------------------------------------*/



/* Structure for  server resource       */
typedef struct tag_HTTP_SERVER_RESOURCE
{
	RESOURCE				_resource;
	URL_OBJECT				_url;
	URL_OBJECT * 			_redirect_url;
#ifdef ENABLE_COOKIES
	//LIST  				_cookies_list;
	//char* 				_private_cookie;
	//char* 				_system_cookie;
	char* 					_cookie;
#endif
#ifdef _DEBUG
	char 					_o_url[MAX_URL_LEN];
#endif
	char* 					_ref_url;
	char* 					_server_return_name;
	char* 					_extra_header;
	_u8* 					_post_data;
	_u32 					_post_data_len;
	BOOL 					_is_post;
	//char 					_lastmodified_from_server[MAX_LAST_MODIFIED_LEN];
	_u64 					_file_size;
	_u32 					_redirect_times;
	BOOL 					_b_is_origin;
	//BOOL 					_b_file_exist;
	//BOOL 					_b_set_file_info;
	//BOOL	 				_b_set_system_cookie;

	BOOL					_is_chunked;/* "Transfer-Encoding" */
	BOOL					_is_support_range;   /* "Content-Range" */
	BOOL					_is_support_long_connect; /*"Connection" */
	BOOL 					_gzip;
	BOOL 					_send_gzip;
	BOOL 					_lixian;  /* 时否为离线资源 */
	_u64					_post_content_len;
	BOOL 					_relation_res;
	_u32 					_block_info_num;
	RELATION_BLOCK_INFO*	_p_block_info_arr;

}HTTP_SERVER_RESOURCE;



/*--------------------------------------------------------------------------*/
/*                Call back functions                                       */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*                Internal functions							            */
/*--------------------------------------------------------------------------*/



/*--------------------------------------------------------------------------*/
/*                Interface provided by other modules							            */
/*--------------------------------------------------------------------------*/
#define HTTP_RESOURCE_SET_CHUNKED(http_resource) {http_resource->_is_chunked=TRUE;}
#define HTTP_RESOURCE_IS_CHUNKED(http_resource) (http_resource->_is_chunked)

#define HTTP_RESOURCE_SET_NOT_SUPPORT_RANGE(http_resource) {http_resource->_is_support_range=FALSE;}
#define HTTP_RESOURCE_IS_SUPPORT_RANGE(http_resource) (http_resource->_is_support_range)

#define HTTP_RESOURCE_SET_NOT_SUPPORT_LONG_CONNECT(http_resource) {http_resource->_is_support_long_connect=FALSE;}
#define HTTP_RESOURCE_IS_SUPPORT_LONG_CONNECT(http_resource) (http_resource->_is_support_long_connect)

#define HTTP_RESOURCE_SET_FILE_SIZE(http_resource,file_size) {http_resource->_file_size=file_size;}
#define HTTP_RESOURCE_GET_FILE_SIZE(http_resource) (http_resource->_file_size)

#define HTTP_RESOURCE_IS_ORIGIN(http_resource) (http_resource->_b_is_origin)

#define HTTP_RESOURCE_PLUS_REDIRECT_TIMES(http_resource) {http_resource->_redirect_times++;}
#define HTTP_RESOURCE_GET_REDIRECT_TIMES(http_resource) (http_resource->_redirect_times)

#define HTTP_RESOURCE_IS_HTTPS(http_resource) ( ( (http_resource)->url._schema_type == HTTPS))

 _int32 init_http_resource_module(  void);

 _int32 uninit_http_resource_module(  void);


 _int32 http_resource_create(  char *url,_u32 url_size,char *	refer_url,_u32	refer_url_size,BOOL is_origin,RESOURCE ** pp_http_resouce);
 _int32 http_resource_create_ex(  char *url,_u32 url_size,char *	refer_url,_u32	refer_url_size,BOOL is_origin, BOOL is_relation_res, _u32 block_num, RELATION_BLOCK_INFO* p_block_info_arr, _u64 res_file_size ,RESOURCE ** pp_http_resouce);
 _int32 http_resource_destroy(RESOURCE ** pp_http_resouce);

 URL_OBJECT* http_resource_get_url_object(HTTP_SERVER_RESOURCE * p_http_resource);

 _int32 http_resource_set_redirect_url(HTTP_SERVER_RESOURCE * p_http_resource,char*url,_u32 url_size);
_int32 http_resource_get_redirect_url(RESOURCE * p_resouce,char* redirect_url_buffer,_u32 buffer_size);
URL_OBJECT* http_resource_get_redirect_url_object(HTTP_SERVER_RESOURCE * p_http_resource);
char * http_resource_get_ref_url(HTTP_SERVER_RESOURCE * p_http_resource);

 _int32 http_resource_set_server_return_file_name(HTTP_SERVER_RESOURCE * p_http_resource,char* file_name,_u32 file_name_size);
 
 
 /* Interface for connect_manager getting file suffix */
 //char* http_resource_get_file_suffix(RESOURCE * p_resource,BOOL *is_binary_file);

 /* Interface for connect_manager getting file name from resource */
 BOOL http_resource_get_file_name(RESOURCE * p_resource,char * file_name,_u32 buffer_len,BOOL compellent);
 _int32 http_resource_set_last_modified(HTTP_SERVER_RESOURCE * p_http_resource,char* last_modified,_u32 last_modified_size);

 BOOL http_resource_is_support_range(RESOURCE * p_resource);
 #ifdef ENABLE_COOKIES
 _int32 http_resource_set_cookies(HTTP_SERVER_RESOURCE * p_http_resource,const char* cookies,_u32 cookies_len);
 char * http_resource_get_cookies(HTTP_SERVER_RESOURCE * p_http_resource);
#endif /* ENABLE_COOKIES  */
 _int32 http_resource_set_extra_header(HTTP_SERVER_RESOURCE * p_http_resource,const char* header,_u32 header_len);
 char * http_resource_get_extra_header(HTTP_SERVER_RESOURCE * p_http_resource);
  _int32 http_resource_set_post_content_len(HTTP_SERVER_RESOURCE * p_http_resource,_u64 content_len);
 _u64 http_resource_get_post_content_len(HTTP_SERVER_RESOURCE * p_http_resource);

 _int32 http_resource_set_post_data(HTTP_SERVER_RESOURCE * p_http_resource,const _u8* data,_u32 data_len);
 _u8 * http_resource_get_post_data(HTTP_SERVER_RESOURCE * p_http_resource);
 _int32 http_resource_set_gzip(HTTP_SERVER_RESOURCE * p_http_resource,BOOL gzip);
BOOL http_resource_get_gzip(HTTP_SERVER_RESOURCE * p_http_resource);
 _int32 http_resource_set_send_gzip(HTTP_SERVER_RESOURCE * p_http_resource,BOOL gzip);
BOOL http_resource_get_send_gzip(HTTP_SERVER_RESOURCE * p_http_resource);
 _int32 http_resource_set_lixian(HTTP_SERVER_RESOURCE * p_http_resource,BOOL lixian);
BOOL http_resource_is_lixian(HTTP_SERVER_RESOURCE * p_http_resource);
BOOL http_resource_is_origin(HTTP_SERVER_RESOURCE * p_http_resource);

#ifdef __cplusplus
}
#endif

#endif /* __HTTP_RESOURCE_H_20090305 */
