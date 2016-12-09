#include "platform/sd_socket.h"
#include "platform/sd_fs.h"
#include "utility/settings.h"
#include "utility/base64.h"
#include "utility/sd_iconv.h"
#include "utility/logid.h"
#define LOGID LOGID_HTTP_PIPE
#include "utility/logger.h"

#include "http_resource.h"
#include "download_task/download_task.h"
#include "socket_proxy_interface.h"
/*--------------------------------------------------------------------------*/
/*                Global declarations				        */
/*--------------------------------------------------------------------------*/
/* Pointer of the http_resource and http_pipe slab */
static SLAB *gp_http_res_slab = NULL;

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for other modules				        */
/*--------------------------------------------------------------------------*/
_int32 init_http_resource_module(void)
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG("init_http_resource_module");

	if (gp_http_res_slab == NULL)
	{
		ret_val = mpool_create_slab(sizeof(HTTP_SERVER_RESOURCE), MIN_HTTP_RES_MEMORY, 0, &gp_http_res_slab);
		CHECK_VALUE(ret_val);
	}


	return ret_val;

}


_int32 uninit_http_resource_module(void)
{
	LOG_DEBUG("uninit_http_resource_module");

	if (gp_http_res_slab != NULL)
	{
		mpool_destory_slab(gp_http_res_slab);
		gp_http_res_slab = NULL;
	}

	return SUCCESS;

}


_int32 http_resource_create(
	char *					url,
	_u32					url_size,
	char *					refer_url,
	_u32					refer_url_size,
	BOOL					is_origin,
	RESOURCE **			pp_http_resouce)
{
	return http_resource_create_ex(url, url_size, refer_url, refer_url_size, is_origin, FALSE, 0, NULL, 0, pp_http_resouce);
}

_int32 http_resource_create_ex(
	char *					url,
	_u32					url_size,
	char *					refer_url,
	_u32					refer_url_size,
	BOOL					is_origin,
	BOOL					is_relation_res,
	_u32					block_num,
	RELATION_BLOCK_INFO*	p_block_info_arr,
	_u64					res_file_size,
	RESOURCE **			pp_http_resouce)
{
	HTTP_SERVER_RESOURCE * p_http_resource = NULL;
	URL_OBJECT url_o;
	_int32 ret_val = SUCCESS;
	//BOOL ref_url_invalid = TRUE;
	LOG_DEBUG("http_resource_create_ex(is_origin=%d,url[%d]=%s, is_relation=%d, filesize:%llu)",
		is_origin, url_size, url, is_relation_res, res_file_size);

	if ((url == NULL) || (url_size == 0) || (url_size > MAX_URL_LEN - 1))
		return HTTP_ERR_INVALID_URL;

	ret_val = mpool_get_slip(gp_http_res_slab, (void**)&p_http_resource);
	CHECK_VALUE(ret_val);

	sd_memset(p_http_resource, 0, sizeof(HTTP_SERVER_RESOURCE));
#ifdef _DEBUG
	sd_memcpy(p_http_resource->_o_url, url, url_size);
#endif

	if (sd_url_to_object(url, url_size, &(p_http_resource->_url)) != SUCCESS)
	{
		mpool_free_slip(gp_http_res_slab, (void*)p_http_resource);
		return HTTP_ERR_INVALID_URL;
	}

	if (((p_http_resource->_url._schema_type != HTTP) && (p_http_resource->_url._schema_type != HTTPS)))
	{
		mpool_free_slip(gp_http_res_slab, (void*)p_http_resource);
		return HTTP_ERR_INVALID_URL;
	}

	if (sd_url_to_object(refer_url, refer_url_size, &url_o) == SUCCESS)
	{
		//if((sd_strstr(url_o._host, ".xunlei.com", 0)==NULL)&&(sd_strstr(url_o._host, ".sandai.net", 0)==NULL))
		{
			ret_val = sd_malloc(refer_url_size + 1, (void**)&(p_http_resource->_ref_url));
			if (ret_val == SUCCESS)
			{
				sd_memcpy(p_http_resource->_ref_url, refer_url, refer_url_size);
				p_http_resource->_ref_url[refer_url_size] = '\0';
				//ref_url_invalid = FALSE;
			}
		}
	}

	//	if(ref_url_invalid==TRUE)
	//	{
	/* No ref_url,use url as ref_url */
	//		sd_url_object_to_ref_url(&(p_http_resource->_url),p_http_resource->_ref_url);
	//	}	

	init_resource_info(&(p_http_resource->_resource), HTTP_RESOURCE);

#if 0 // def ENABLE_COOKIES
	list_init(&(p_http_resource->_cookies_list));
#endif
	p_http_resource->_b_is_origin = is_origin;

	p_http_resource->_is_support_range = TRUE;
	p_http_resource->_is_support_long_connect = TRUE;
	p_http_resource->_relation_res = is_relation_res;
	p_http_resource->_block_info_num = block_num;
	p_http_resource->_p_block_info_arr = p_block_info_arr;
	p_http_resource->_file_size = res_file_size;
	if (is_origin)
	{
		set_resource_level((RESOURCE*)p_http_resource, MAX_RES_LEVEL);
	}
	*pp_http_resouce = (RESOURCE*)(p_http_resource);

	LOG_DEBUG("++++http_resource_create successful!(p_http_resource=0x%X,is_origin=%d,url[%d]=%s)", p_http_resource, is_origin, url_size, url);

	return SUCCESS;

}
_int32 http_resource_destroy(RESOURCE ** pp_http_resouce)
{
#if 0 //def ENABLE_COOKIES
	LIST_ITERATOR p_list_node=NULL ;
	char * cookie = NULL;  
#endif
	HTTP_SERVER_RESOURCE * p_http_resource = (HTTP_SERVER_RESOURCE *)(*pp_http_resouce);
	_int32 ret_val = SUCCESS;

	LOG_DEBUG("http_resource_destroy:p_http_resource=0x%X", p_http_resource);

#ifdef ENABLE_COOKIES
	SAFE_DELETE(p_http_resource->_cookie);
	/*
	SAFE_DELETE(p_http_resource->_private_cookie);
	SAFE_DELETE(p_http_resource->_system_cookie);

	for(p_list_node = LIST_BEGIN(p_http_resource->_cookies_list); p_list_node != LIST_END(p_http_resource->_cookies_list); p_list_node = LIST_NEXT(p_list_node))
	{
	cookie = (char * )LIST_VALUE(p_list_node);
	SAFE_DELETE(cookie);
	}
	list_clear(&p_http_resource->_cookies_list);
	*/
#endif	
	SAFE_DELETE(p_http_resource->_extra_header);
	SAFE_DELETE(p_http_resource->_post_data);
	uninit_resource_info(&(p_http_resource->_resource));

	SAFE_DELETE(p_http_resource->_redirect_url);
	SAFE_DELETE(p_http_resource->_ref_url);
	SAFE_DELETE(p_http_resource->_server_return_name);

	ret_val = mpool_free_slip(gp_http_res_slab, (void*)p_http_resource);
	CHECK_VALUE(ret_val);
	*pp_http_resouce = NULL;

	LOG_DEBUG("++++http_resource_destroy SUCCESS!");

	return SUCCESS;
}

URL_OBJECT* http_resource_get_url_object(HTTP_SERVER_RESOURCE * p_http_resource)
{
	LOG_DEBUG("http_resource_get_url_object[0x%X]", p_http_resource);

	if (p_http_resource->_url._host[0] != '\0')
	{
		return &(p_http_resource->_url);
	}

#ifdef _DEBUG	
	sd_assert(FALSE);
#endif

	return NULL;
}
char * http_resource_get_dynamic_path(HTTP_SERVER_RESOURCE * p_http_resource)
{
	char *pos1 = NULL, *pos2 = NULL;
	static char u_buffer[MAX_FILE_PATH_LEN];

	if (p_http_resource->_url._is_dynamic_url == FALSE || NULL == p_http_resource->_url._path) return NULL;

	sd_memset(u_buffer, 0x00, MAX_FILE_PATH_LEN);
	// http://www.appoole.com/download/down.php?id=276
	pos1 = sd_strchr(p_http_resource->_url._path, '?', 0);
	sd_strncpy(u_buffer, p_http_resource->_url._path, pos1 - p_http_resource->_url._path);
	pos2 = sd_strrchr(u_buffer, '/');
	pos2[1] = '\0';
	return u_buffer;
}

_int32 http_resource_set_redirect_url(HTTP_SERVER_RESOURCE * p_http_resource, char*url, _u32 url_size)
{
	char * full_path = NULL, *pos1 = NULL, *pos2 = NULL, *dynamic_path = NULL;
	char u_buffer[MAX_URL_LEN] = { 0 };
	_u32 path_len = 0;
	_int32 ret_val = SUCCESS;
	URL_OBJECT url_o;
	LOG_DEBUG("http_resource_set_redirect_url[0x%X]", p_http_resource);

	if (sd_url_to_object(url, url_size, &url_o) != SUCCESS)
	{
		if (sd_strstr(url, "://", 0) == NULL)
		{
			//Location: torrents/2009/09/16/1253101347.torrent
			full_path = sd_strchr(url, '/', 0);
			if ((full_path != NULL) && (full_path - url < url_size))
			{
				path_len = url_size - (full_path - url);
				if (full_path != url)
				{
					sd_memcpy(u_buffer, url, full_path - url);
					pos1 = sd_strchr(u_buffer, '=', 0);
					pos1 = sd_strchr(u_buffer, ':', 0);
					if ((pos1 == NULL) && (pos2 == NULL))
					{
						sd_memset(u_buffer, 0, MAX_URL_LEN);
						dynamic_path = http_resource_get_dynamic_path(p_http_resource);
						if (dynamic_path != NULL)
						{
							sd_strncpy(u_buffer, dynamic_path, sd_strlen(dynamic_path));
						}
						else
						{
							sd_strncpy(u_buffer, "/", 1);
						}
						sd_strcat(u_buffer, url, url_size);
						full_path = u_buffer;
						path_len = sd_strlen(u_buffer);
					}
				}

				sd_memcpy(&url_o, &(p_http_resource->_url), sizeof(URL_OBJECT));
				ret_val = sd_url_object_set_path(&url_o, full_path, path_len);
				CHECK_VALUE(ret_val);
				//if(p_http_resource->_url._is_binary_file==TRUE)
				//{
				//	if((p_http_resource->_redirect_url._is_binary_file==FALSE)&&(p_http_resource->_redirect_url._is_dynamic_url==FALSE))
				//		CHECK_VALUE( HTTP_ERR_INVALID_REDIRECT_URL);
				//}
			}
			else
				return HTTP_ERR_INVALID_REDIRECT_URL;
		}
		else
			return HTTP_ERR_INVALID_REDIRECT_URL;
	}

	if (p_http_resource->_redirect_url == NULL)
	{
		ret_val = sd_malloc(sizeof(URL_OBJECT), (void**)&(p_http_resource->_redirect_url));
		CHECK_VALUE(ret_val);
	}

	sd_memcpy(p_http_resource->_redirect_url, &url_o, sizeof(URL_OBJECT));
	p_http_resource->_redirect_url->_path = p_http_resource->_redirect_url->_full_path;
	p_http_resource->_redirect_url->_file_name = p_http_resource->_redirect_url->_full_path + p_http_resource->_redirect_url->_path_len;

	return SUCCESS;
}
_int32 http_resource_get_redirect_url(RESOURCE * p_resouce, char* redirect_url_buffer, _u32 buffer_size)
{
	HTTP_SERVER_RESOURCE * p_http_resource = (HTTP_SERVER_RESOURCE *)p_resouce;
	_int32 ret_val = SUCCESS;

	LOG_DEBUG("http_resource_get_redirect_url[0x%X]", p_resouce);

	if ((!p_resouce) || (p_resouce->_resource_type != HTTP_RESOURCE)) return HTTP_ERR_RESOURCE_NOT_HTTP;
	if ((!redirect_url_buffer) || (buffer_size < MAX_URL_LEN)) return HTTP_ERR_BUFFER_NOT_ENOUGH;

	if (p_http_resource->_redirect_url != NULL)
	{
		sd_memset(redirect_url_buffer, 0, MAX_URL_LEN);
		ret_val = sd_url_object_to_string(p_http_resource->_redirect_url, redirect_url_buffer);
	}
	else
	{
		ret_val = HTTP_ERR_INVALID_REDIRECT_URL;
	}

	return ret_val;
}
URL_OBJECT* http_resource_get_redirect_url_object(HTTP_SERVER_RESOURCE * p_http_resource)
{

	LOG_DEBUG("http_resource_get_redirect_url_object[0x%X]", p_http_resource);

	//if(p_http_resource->_redirect_url!=NULL)
	//{
	return (p_http_resource->_redirect_url);
	//}

	//return NULL;
}
char * http_resource_get_ref_url(HTTP_SERVER_RESOURCE * p_http_resource)
{
	LOG_DEBUG("http_resource_get_ref_url[0x%X]:%s", p_http_resource, p_http_resource->_ref_url);
	return p_http_resource->_ref_url;
}

_int32 http_resource_set_server_return_file_name(HTTP_SERVER_RESOURCE * p_http_resource, char* file_name, _u32 file_name_size)
{
	_int32 ret_val = SUCCESS;
	_u32 len = MAX_FILE_NAME_LEN - 1;
	LOG_DEBUG("http_resource_set_server_return_file_name[0x%X]", p_http_resource);

	if (file_name_size < len)
		len = file_name_size;

	SAFE_DELETE(p_http_resource->_server_return_name);

	ret_val = sd_malloc(len + 1, (void**)&(p_http_resource->_server_return_name));
	CHECK_VALUE(ret_val);

	sd_memcpy(p_http_resource->_server_return_name, file_name, len);
	p_http_resource->_server_return_name[len] = '\0';

	return SUCCESS;
}

/* Interface for connect_manager getting file name from resource */
BOOL http_resource_get_file_name(RESOURCE * p_resource, char * file_name, _u32 buffer_len, BOOL compellent)
{
	char * suffix_pos = NULL, *_redirect_url_file_name = NULL;
	HTTP_SERVER_RESOURCE * p_http_resource = (HTTP_SERVER_RESOURCE *)p_resource;
	BOOL _redirect_url_is_dynamic_url = TRUE, _redirect_url_is_binary_file = FALSE;

#ifdef _DEBUG
	LOG_DEBUG(" enter[0x%X] http_resource_get_file_name(is_origin=%d,_compellent=%d,url=%s)...", p_resource, p_http_resource->_b_is_origin, compellent, p_http_resource->_o_url);
#else
	LOG_DEBUG( " enter[0x%X] http_resource_get_file_name(is_origin=%d,_compellent=%d)...",p_resource,p_http_resource->_b_is_origin,compellent );
#endif

	sd_memset(file_name, 0, buffer_len);

	if (p_http_resource->_redirect_url != NULL)
	{
		_redirect_url_file_name = p_http_resource->_redirect_url->_file_name;
		_redirect_url_is_dynamic_url = p_http_resource->_redirect_url->_is_dynamic_url;
		_redirect_url_is_binary_file = p_http_resource->_redirect_url->_is_binary_file;
	}

	if (p_http_resource->_server_return_name != NULL)
	{
		LOG_DEBUG("server_return_name : %s", p_http_resource->_server_return_name);
		/* Get file name from _server_return_name */
		suffix_pos = sd_strrchr(p_http_resource->_server_return_name, '.');
		if ((suffix_pos != NULL) && (sd_is_binary_file(suffix_pos, NULL) == TRUE))
		{
			sd_strncpy(file_name, p_http_resource->_server_return_name, buffer_len);
			/* remove all the '%' */
			if (sd_decode_file_name(file_name, suffix_pos, buffer_len) == TRUE)
			{
				if (sd_is_file_name_valid(file_name) == TRUE)
				{
					LOG_DEBUG("Get file name from _server_return_name=%s", file_name);
					return TRUE;
				}
			}
		}
	}
	else if ((p_http_resource->_url._is_dynamic_url == TRUE)
		&& (_redirect_url_file_name != NULL)
		/*&&(_redirect_url_is_dynamic_url!=TRUE)*/
		&& (_redirect_url_is_binary_file == TRUE))
	{
		if (p_http_resource->_redirect_url->_file_name_len >= buffer_len) return FALSE;
		/* Get file name from redirect url */
		sd_memcpy(file_name, p_http_resource->_redirect_url->_file_name, p_http_resource->_redirect_url->_file_name_len);
		/* remove all the '%' */
		if (sd_decode_file_name(file_name, p_http_resource->_redirect_url->_file_suffix, buffer_len) == TRUE)
		{
			if (sd_is_file_name_valid(file_name) == TRUE)
			{
				LOG_DEBUG("Get file name from redirect url=%s", file_name);
				return TRUE;
			}
		}
	}
	else if ((_redirect_url_file_name != NULL)
		&& (_redirect_url_is_dynamic_url != TRUE)
		&& (_redirect_url_is_binary_file == TRUE))
	{
		if (p_http_resource->_redirect_url->_file_name_len >= buffer_len) return FALSE;
		/* Get file name from redirect url */
		sd_memcpy(file_name, p_http_resource->_redirect_url->_file_name, p_http_resource->_redirect_url->_file_name_len);
		/* remove all the '%' */
		if (sd_decode_file_name(file_name, p_http_resource->_redirect_url->_file_suffix, buffer_len) == TRUE)
		{
			if (sd_is_file_name_valid(file_name) == TRUE)
			{
				LOG_DEBUG("Get file name from redirect url=%s", file_name);
				return TRUE;
			}
		}
	}
	else if ((p_http_resource->_url._file_name != NULL)
		&& (p_http_resource->_url._is_binary_file == TRUE))
	{
		if (p_http_resource->_url._file_name_len >= buffer_len) return FALSE;
		sd_memcpy(file_name, p_http_resource->_url._file_name, p_http_resource->_url._file_name_len);
		/* remove all the '%' */
		if (sd_decode_file_name(file_name, p_http_resource->_url._file_suffix, buffer_len) == TRUE)
		{
			if (sd_is_file_name_valid(file_name) == TRUE)
			{
				LOG_DEBUG("Get file name from url=%s", file_name);
				return TRUE;
			}
		}
	}

	if (compellent == TRUE)
	{
		if (p_http_resource->_server_return_name != NULL)
		{
			sd_strncpy(file_name, p_http_resource->_server_return_name, buffer_len);
		}
		else if (p_http_resource->_url._file_name != NULL)
		{
			if (p_http_resource->_url._file_name_len >= buffer_len) return FALSE;
			sd_memcpy(file_name, p_http_resource->_url._file_name, p_http_resource->_url._file_name_len);
		}
		else
		{
			sd_memcpy(file_name, p_http_resource->_url._host, sd_strlen(p_http_resource->_url._host));
			sd_strcat(file_name, ".html", sd_strlen(".html"));
		}

		/* remove all the '%' */
		if (sd_decode_file_name(file_name, NULL, buffer_len) == TRUE)
		{
			if (sd_is_file_name_valid(file_name) == TRUE)
			{
				LOG_DEBUG("compellent Get file name from url=%s", file_name);
				return TRUE;
			}
		}
	}

	LOG_DEBUG("Can not get file name!");
	return FALSE;

}

_int32 http_resource_set_last_modified(HTTP_SERVER_RESOURCE * p_http_resource, char* last_modified, _u32 last_modified_size)
{
	LOG_DEBUG("[0x%X]http_resource_set_last_modified", p_http_resource);
	return SUCCESS;
}

BOOL http_resource_is_support_range(RESOURCE * p_resource)
{
	HTTP_SERVER_RESOURCE * p_http_resource = (HTTP_SERVER_RESOURCE *)p_resource;

	LOG_DEBUG("[0x%X]http_resource_is_support_range:%d", p_http_resource, p_http_resource->_is_support_range);

	return p_http_resource->_is_support_range;
}

#ifdef ENABLE_COOKIES
_int32 http_resource_set_cookies(HTTP_SERVER_RESOURCE * p_http_resource, const char* cookies, _u32 cookies_len)
{
	_int32 ret_val = SUCCESS;
	_u32 cookie_str_len = 0;
	LOG_DEBUG("[0x%X]http_resource_set_cookies:[%u]%s", p_http_resource, cookies_len, cookies);
	if (p_http_resource->_cookie != NULL)
	{
		SAFE_DELETE(p_http_resource->_cookie);
	}

	cookie_str_len = sd_strlen(cookies);
	if (cookie_str_len < cookies_len)
	{
		cookies_len = cookie_str_len;
	}

	if ((cookies == NULL) || (cookies_len == 0))
	{
		return SUCCESS;
	}

	//if(sd_strncmp(cookies,"Cookie:", 7)!=0)
	//	return SUCCESS;

	ret_val = sd_malloc(cookies_len + 3, (void **)&p_http_resource->_cookie);
	CHECK_VALUE(ret_val);
	sd_memset(p_http_resource->_cookie, 0x00, cookies_len + 3);
	sd_strncpy(p_http_resource->_cookie, cookies, cookies_len);
	if (p_http_resource->_cookie[cookies_len - 1] != '\n')
	{
		p_http_resource->_cookie[cookies_len] = '\r';
		p_http_resource->_cookie[cookies_len + 1] = '\n';
	}
	return SUCCESS;
}
char * http_resource_get_cookies(HTTP_SERVER_RESOURCE * p_http_resource)
{

	LOG_DEBUG("[0x%X]http_resource_get_cookies:%s", p_http_resource, p_http_resource->_cookie);
	return p_http_resource->_cookie;
}
#endif /* ENABLE_COOKIES  */
/*******************************************************************/
_int32 http_resource_set_extra_header(HTTP_SERVER_RESOURCE * p_http_resource, const char* header, _u32 header_len)
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG("[0x%X]http_resource_set_extra_header:[%u]%s", p_http_resource, header_len, header);
	if (p_http_resource->_extra_header != NULL)
	{
		SAFE_DELETE(p_http_resource->_extra_header);
	}
	if ((header == NULL) || (header_len == 0))
	{
		return SUCCESS;
	}

	//	if(sd_strncmp(header+header_len-2,"\r\n", 2)!=0)
	//	{
	//		CHECK_VALUE(INVALID_ARGUMENT);
	//	}

	ret_val = sd_malloc(header_len + 3, (void **)&p_http_resource->_extra_header);
	CHECK_VALUE(ret_val);
	sd_memset(p_http_resource->_extra_header, 0x00, header_len + 3);
	sd_strncpy(p_http_resource->_extra_header, header, header_len);
	if (p_http_resource->_extra_header[header_len - 1] != '\n')
	{
		p_http_resource->_extra_header[header_len] = '\r';
		p_http_resource->_extra_header[header_len + 1] = '\n';
	}
	return SUCCESS;
}
char * http_resource_get_extra_header(HTTP_SERVER_RESOURCE * p_http_resource)
{

	LOG_DEBUG("[0x%X]http_resource_get_extra_header:%s", p_http_resource, p_http_resource->_extra_header);
	return p_http_resource->_extra_header;
}
_int32 http_resource_set_post_content_len(HTTP_SERVER_RESOURCE * p_http_resource, _u64 content_len)
{
	//_int32 ret_val = SUCCESS;
	LOG_DEBUG("[0x%X]http_resource_set_post_content_len:%llu", p_http_resource, content_len);
	p_http_resource->_post_content_len = content_len;
	//if(content_len!=0)
	p_http_resource->_is_post = TRUE;
	return SUCCESS;
}
_u64 http_resource_get_post_content_len(HTTP_SERVER_RESOURCE * p_http_resource)
{
	return p_http_resource->_post_content_len;
}


_int32 http_resource_set_post_data(HTTP_SERVER_RESOURCE * p_http_resource, const _u8* data, _u32 data_len)
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG("[0x%X]http_resource_set_post_data:%u", p_http_resource, data_len);
	if (p_http_resource->_post_data != NULL)
	{
		SAFE_DELETE(p_http_resource->_post_data);
	}
	if ((data == NULL) || (data_len == 0))
	{
		CHECK_VALUE(INVALID_ARGUMENT);
	}

	ret_val = sd_malloc(data_len, (void **)&p_http_resource->_post_data);
	CHECK_VALUE(ret_val);
	sd_memcpy(p_http_resource->_post_data, data, data_len);
	p_http_resource->_post_data_len = data_len;
	p_http_resource->_is_post = TRUE;
	return SUCCESS;
}
_u8 * http_resource_get_post_data(HTTP_SERVER_RESOURCE * p_http_resource)
{
	return p_http_resource->_post_data;
}

_int32 http_resource_set_gzip(HTTP_SERVER_RESOURCE * p_http_resource, BOOL gzip)
{
	//_int32 ret_val = SUCCESS;
	LOG_DEBUG("[0x%X]http_resource_set_gzip:%d", p_http_resource, gzip);
	p_http_resource->_gzip = gzip;
	return SUCCESS;
}
BOOL http_resource_get_gzip(HTTP_SERVER_RESOURCE * p_http_resource)
{
	return p_http_resource->_gzip;
}
_int32 http_resource_set_send_gzip(HTTP_SERVER_RESOURCE * p_http_resource, BOOL gzip)
{
	//_int32 ret_val = SUCCESS;
	LOG_DEBUG("[0x%X]http_resource_set_send_gzip:%d", p_http_resource, gzip);
	p_http_resource->_send_gzip = gzip;
	return SUCCESS;
}
BOOL http_resource_get_send_gzip(HTTP_SERVER_RESOURCE * p_http_resource)
{
	return p_http_resource->_send_gzip;
}

_int32 http_resource_set_lixian(HTTP_SERVER_RESOURCE * p_http_resource, BOOL lixian)
{
	//_int32 ret_val = SUCCESS;
	LOG_DEBUG("[0x%X]http_resource_set_lixian:%d", p_http_resource, lixian);
	p_http_resource->_lixian = lixian;
	return SUCCESS;
}
BOOL http_resource_is_lixian(HTTP_SERVER_RESOURCE * p_http_resource)
{
	return p_http_resource->_lixian;
}

BOOL http_resource_is_origin(HTTP_SERVER_RESOURCE * p_http_resource)
{
	return p_http_resource->_b_is_origin;
}
