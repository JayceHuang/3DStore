#if !defined(__HTTP_DATA_PIPE_INTERFACE_C_20090305)
#define __HTTP_DATA_PIPE_INTERFACE_C_20090305


#include "platform/sd_socket.h"
#include "utility/settings.h"
#include "utility/base64.h"
#include "utility/sd_iconv.h"
#include "utility/logid.h"
#define LOGID LOGID_HTTP_PIPE
#include "utility/logger.h"

#include "http_data_pipe.h"
#include "download_task/download_task.h"
#include "socket_proxy_interface.h"

/*--------------------------------------------------------------------------*/
/*                Global declarations				        */
/*--------------------------------------------------------------------------*/
/* Pointer of the http_resource and http_pipe slab */
static SLAB *		gp_http_pipe_slab = NULL;
static SLAB *		gp_http_1024_slab = NULL;
static _u32			g_http_every_time_reveive_range_num = 0;
#ifdef _ENABLE_SSL
extern SSL_CTX*		gp_ssl_ctx;
#endif

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for other modules				        */
/*--------------------------------------------------------------------------*/
_int32 init_http_pipe_module(void)
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG("init_http_pipe_module");

	ret_val = init_http_resource_module();
	CHECK_VALUE(ret_val);

	if (gp_http_pipe_slab == NULL)
	{
		ret_val = mpool_create_slab(sizeof(HTTP_DATA_PIPE), MIN_HTTP_PIPE_MEMORY, 0, &gp_http_pipe_slab);
		if (ret_val != SUCCESS)
		{
			uninit_http_resource_module();
		}
	}

	if (gp_http_1024_slab == NULL)
	{
		ret_val = mpool_create_slab(HTTP_1024_LEN, MIN_HTTP_1024_MEMORY, 0, &gp_http_1024_slab);
		if (ret_val != SUCCESS)
		{
			uninit_http_resource_module();
			mpool_destory_slab(gp_http_pipe_slab);
			gp_http_pipe_slab = NULL;
		}
	}

	g_http_every_time_reveive_range_num = HTTP_DEFAULT_RANGE_NUM;
	settings_get_int_item("http_data_pipe.receive_ranges_number", (_int32*)&(g_http_every_time_reveive_range_num));
#ifdef _ENABLE_SSL
	SSL_library_init();
	SSL_load_error_strings();
	SSLeay_add_all_algorithms();
	SSLeay_add_ssl_algorithms();

	gp_ssl_ctx = SSL_CTX_new(SSLv23_client_method());
	///TODO: check error
	SSL_CTX_set_default_verify_paths(gp_ssl_ctx);
	SSL_CTX_load_verify_locations(gp_ssl_ctx, NULL, NULL);
	SSL_CTX_set_verify(gp_ssl_ctx, SSL_VERIFY_NONE, NULL);
#endif
	return ret_val;

}


_int32 uninit_http_pipe_module(void)
{
	LOG_DEBUG("uninit_http_pipe_module");

	uninit_http_resource_module();

	if (gp_http_pipe_slab != NULL)
	{
		mpool_destory_slab(gp_http_pipe_slab);
		gp_http_pipe_slab = NULL;
	}

	if (gp_http_1024_slab != NULL)
	{
		mpool_destory_slab(gp_http_1024_slab);
		gp_http_1024_slab = NULL;
	}
#ifdef _ENABLE_SSL
	if (gp_ssl_ctx)
	{
		SSL_CTX_free(gp_ssl_ctx);
	}
#endif
	return SUCCESS;

}


/*****************************************************************************
 * Interface name : http_pipe_create
 *----------------------------------------------------------------------------*
 * Input parameters  :
 * -------------------
 *    DATA_MANAGER*  _p_data_manager :
 * pointer to DATA_MANAGER which stores the received data
 *
 * Output parameters :
 * -------------------
 *   HTTP_DATA_PIPE ** _p_http_pipe :
 *  pointer to new HTTP_DATA_PIPE,release this memory by http_pipe_destroy(...)
 *
 * Return values :
 * -------------------
 *   0 - success;other values - error
 *
 *----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*
 *                                    DESCRIPTION
 *
 *   Create a new http data pipe and initiate the HTTP_DATA_PIPE structure
 *----------------------------------------------------------------------------*
 ******************************************************************************/
_int32 http_pipe_create(void*  p_data_manager, RESOURCE*  p_resource, DATA_PIPE ** pp_pipe)
{
	HTTP_DATA_PIPE * p_http_pipe = NULL;
	HTTP_SERVER_RESOURCE * p_http_resource = NULL;
	_int32 ret_val = SUCCESS;

	LOG_DEBUG(" enter http_pipe_create(p_data_manager=0x%X,p_resource=0x%X)...", p_data_manager, p_resource);

#ifdef _DEBUG	
	sd_assert((*pp_pipe) == NULL);
	sd_assert(p_resource != NULL);
	//sd_assert(p_data_manager!=NULL);
	sd_assert(p_resource->_resource_type == HTTP_RESOURCE);
#endif

	if (*pp_pipe)	return HTTP_ERR_PIPE_NOT_EMPTY;
	if (p_resource == NULL)	return HTTP_ERR_RESOURCE_EMPTY;
	//if(p_data_manager == NULL)	return HTTP_ERR_DATA_MANAGER_EMPTY;
	if (p_resource->_resource_type != HTTP_RESOURCE)	return HTTP_ERR_RESOURCE_NOT_HTTP;

	p_http_resource = (HTTP_SERVER_RESOURCE*)p_resource;

	if (p_http_resource->_url._host[0] == '\0')
		CHECK_VALUE(HTTP_ERR_INVALID_URL);
	if ((p_http_resource->_url._schema_type != HTTP) && (p_http_resource->_url._schema_type != HTTPS))
		CHECK_VALUE(HTTP_ERR_INVALID_URL);

	ret_val = mpool_get_slip(gp_http_pipe_slab, (void**)&p_http_pipe);
	CHECK_VALUE(ret_val);

	sd_memset(p_http_pipe, 0, sizeof(HTTP_DATA_PIPE));

	init_pipe_info(&(p_http_pipe->_data_pipe), HTTP_PIPE, p_resource, p_data_manager);

	init_speed_calculator(&(p_http_pipe->_speed_calculator), 20, 500);

	p_http_pipe->_max_try_count = MAX_RETRY_CONNECT_TIMES;

	p_http_pipe->_p_http_server_resource = (HTTP_SERVER_RESOURCE*)p_resource;
	HTTP_SET_STATE(HTTP_PIPE_STATE_IDLE);
	p_http_pipe->_is_post = p_http_pipe->_p_http_server_resource->_is_post;
#ifdef LITTLE_FILE_TEST
	LOG_DEBUG( "http pipe[0x%X]b_is_origin=%d,err_code=%d,max_speed=%u" ,
		p_http_pipe,p_http_pipe->_p_http_server_resource->_b_is_origin,p_resource->_err_code,p_resource->_max_speed);

	if(p_http_pipe->_p_http_server_resource->_b_is_origin && p_resource->_err_code == SUCCESS && p_resource->_max_speed ==0)
	{
		LOG_DEBUG( "++++http pipe[0x%X]this is super pipe!++++" ,p_http_pipe);
		p_http_pipe->_super_pipe = TRUE;
	}
#endif
	*pp_pipe = (DATA_PIPE *)(p_http_pipe);

	LOG_DEBUG("++++http pipe[0x%X](p_data_manager=0x%X,p_resource=0x%X) is created success!++++",
		p_http_pipe, p_data_manager, p_resource);
	/* OK */
	return SUCCESS;


} /* End of http_pipe_create */
/*****************************************************************************
 * Interface name : http_pipe_set_request
 *----------------------------------------------------------------------------*
 * Input parameters  :
 * -------------------
 *   HTTP_DATA_PIPE * _p_http_pipe
 *
 * Output parameters :
 * -------------------
 *   NONE
 *
 * Return values :
 * -------------------
 *   0 - success;other values - error
 *
 *----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*
 *                                    DESCRIPTION
 *
 * Open the http data pipe
 *----------------------------------------------------------------------------*
 ******************************************************************************/

_int32 http_pipe_set_request(DATA_PIPE * p_pipe, char* request, _u32 request_len)
{
	HTTP_DATA_PIPE * p_http_pipe = (HTTP_DATA_PIPE*)p_pipe;
	_int32 ret_val = SUCCESS;
	//LOG_DEBUG( "HTTP:enter http_pipe_set_request(p_pipe=0x%X,request=%s)...",p_pipe ,request);
	// "GET /announce?info_hash=%s&peer_id=%s&ip=%s&port=%u&uploaded=0&downloaded=0&left=0&numwant=200&key=%d&compact=1&event=%s HTTP/1.0\r\nUser-Agent: Bittorrent\r\nAccept: */*\r\nAccept-Encoding: gzip\r\nConnection: closed\r\n\r\n" 
#ifdef _DEBUG	
	sd_assert(p_pipe != NULL);
	sd_assert(p_pipe->_data_pipe_type == HTTP_PIPE);
	sd_assert(p_http_pipe->_wait_delete != TRUE);
	sd_assert(p_http_pipe->_is_connected != TRUE);
	sd_assert(request != NULL);
	sd_assert(request_len != 0);
#endif

	if ((!p_http_pipe) || (p_pipe->_data_pipe_type != HTTP_PIPE)) return HTTP_ERR_INVALID_PIPE;

	if (p_http_pipe->_wait_delete) return HTTP_ERR_WAITING_DELETE;

	LOG_DEBUG("  _pipe_state=%d,_http_state=%d ", HTTP_GET_PIPE_STATE, HTTP_GET_STATE);

	//	if((HTTP_GET_PIPE_STATE != PIPE_IDLE)&&(HTTP_GET_PIPE_STATE != PIPE_DOWNLOADING))
	//		CHECK_VALUE( HTTP_ERR_OPEN_WRONG_STATE);

	//	if((HTTP_GET_STATE != HTTP_PIPE_STATE_IDLE)&&(HTTP_GET_STATE != HTTP_PIPE_STATE_RANGE_CHANGED)
	//	  &&(HTTP_GET_STATE != HTTP_PIPE_STATE_DOWNLOAD_COMPLETED))
	//		CHECK_VALUE( HTTP_ERR_OPEN_WRONG_STATE);

	//	if(p_http_pipe->_is_connected == TRUE)
	//		CHECK_VALUE( HTTP_ERR_OPEN_WRONG_STATE);

	if (request_len + 1 > p_http_pipe->_http_request_buffer_len)
	{
		if (p_http_pipe->_http_request_buffer_len == 0)
		{
			if (request_len + 1 > MAX_HTTP_REQ_HEADER_LEN)
				p_http_pipe->_http_request_buffer_len = request_len + 1;
			else
				p_http_pipe->_http_request_buffer_len = MAX_HTTP_REQ_HEADER_LEN;
		}
		else
		{
			SAFE_DELETE(p_http_pipe->_http_request);
			p_http_pipe->_http_request_buffer_len = request_len + 1;
		}

		ret_val = sd_malloc(p_http_pipe->_http_request_buffer_len, (void**)&(p_http_pipe->_http_request));
		if (ret_val != SUCCESS) return ret_val;

	}
	sd_memset(p_http_pipe->_http_request, 0, p_http_pipe->_http_request_buffer_len);
	sd_memcpy(p_http_pipe->_http_request, request, request_len);
	p_http_pipe->_http_request_len = request_len;
	p_http_pipe->_b_set_request = TRUE;

	return SUCCESS;

} /* End of http_pipe_open */

/*****************************************************************************
 * Interface name : http_pipe_open
 *----------------------------------------------------------------------------*
 * Input parameters  :
 * -------------------
 *   HTTP_DATA_PIPE * _p_http_pipe
 *
 * Output parameters :
 * -------------------
 *   NONE
 *
 * Return values :
 * -------------------
 *   0 - success;other values - error
 *
 *----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*
 *                                    DESCRIPTION
 *
 * Open the http data pipe
 *----------------------------------------------------------------------------*
 ******************************************************************************/
_int32 http_pipe_open(DATA_PIPE * p_pipe)
{
	HTTP_DATA_PIPE * p_http_pipe = (HTTP_DATA_PIPE*)p_pipe;
	URL_OBJECT	*p_url_o = NULL;
	_int32 ret_val = SUCCESS;
	LOG_DEBUG("HTTP:enter http_pipe_open(p_pipe=0x%X)...", p_pipe);

#ifdef _DEBUG	
	sd_assert(p_pipe != NULL);
	sd_assert(p_pipe->_data_pipe_type == HTTP_PIPE);
	sd_assert(p_http_pipe->_wait_delete != TRUE);
	sd_assert(p_http_pipe->_is_connected != TRUE);
#endif

	if ((!p_http_pipe) || (p_pipe->_data_pipe_type != HTTP_PIPE)) return HTTP_ERR_INVALID_PIPE;

	if (p_http_pipe->_wait_delete) return HTTP_ERR_WAITING_DELETE;

	LOG_DEBUG("  _pipe_state=%d,_http_state=%d ", HTTP_GET_PIPE_STATE, HTTP_GET_STATE);

	if ((HTTP_GET_PIPE_STATE != PIPE_IDLE) && (HTTP_GET_PIPE_STATE != PIPE_DOWNLOADING))
		CHECK_VALUE(HTTP_ERR_OPEN_WRONG_STATE);

	if ((HTTP_GET_STATE != HTTP_PIPE_STATE_IDLE) && (HTTP_GET_STATE != HTTP_PIPE_STATE_RANGE_CHANGED)
		&& (HTTP_GET_STATE != HTTP_PIPE_STATE_DOWNLOAD_COMPLETED))
		CHECK_VALUE(HTTP_ERR_OPEN_WRONG_STATE);

	if (p_http_pipe->_is_connected == TRUE)
		CHECK_VALUE(HTTP_ERR_OPEN_WRONG_STATE);

	p_url_o = http_pipe_get_url_object(p_http_pipe);
	if (p_url_o == NULL)
	{
		ret_val = HTTP_ERR_INVALID_URL;
		goto ErrorHanle;
	}
#ifdef _ENABLE_SSL
	p_http_pipe->_is_https = FALSE;
	if (p_url_o->_schema_type == HTTPS)
	{
		char server[1024];
		SSL* ssl_conn;

		p_http_pipe->_is_https = TRUE;
		p_http_pipe->_pbio = BIO_new_ssl_connect(gp_ssl_ctx);
		if (p_http_pipe->_pbio == NULL)
		{
			ret_val = HTTP_ERR_SSL_CREATE;
			goto ErrorHanle;
		}

		BIO_set_nbio(p_http_pipe->_pbio, 1); // set non-blocking bio, always return 1

		BIO_get_ssl(p_http_pipe->_pbio, &ssl_conn);
		SSL_set_mode(ssl_conn, SSL_MODE_AUTO_RETRY);

		sd_snprintf(server, sizeof(server), "%s:%d", p_url_o->_host, p_url_o->_port);
		BIO_set_conn_hostname(p_http_pipe->_pbio, server); // always return 1
		LOG_DEBUG("pipe(0x%X): set ssl dest host(%s)", p_http_pipe, server);

		HTTP_SET_PIPE_STATE(PIPE_CONNECTING);
		HTTP_SET_STATE(HTTP_PIPE_STATE_CONNECTING);
		ret_val = BIO_do_connect(p_http_pipe->_pbio);
		if (ret_val <= 0)
		{
			if (!BIO_should_retry(p_http_pipe->_pbio))
			{
				LOG_ERROR("pipe(0x%X): bio do connect(%s) 1 failed", p_http_pipe, server);
				ret_val = HTTP_ERR_SSL_CONNECT;
				BIO_FREE_ALL(p_http_pipe->_pbio);
				goto ErrorHanle;
			}
			LOG_DEBUG("ssl handshake with %s will retry", server);
		}
		else
		{ // > 0 success
			LOG_DEBUG("ssl handshake with %s success", server);
			ret_val = BIO_get_fd(p_http_pipe->_pbio, &p_http_pipe->_socket_fd);
			if (ret_val <= 0)
			{
				LOG_ERROR("pipe(0x%X): 1 bio get fd filed", p_http_pipe);
				ret_val = HTTP_ERR_SSL_CONNECT;
				BIO_FREE_ALL(p_http_pipe->_pbio);
				goto ErrorHanle;
			}
			return http_pipe_handle_connect(0, 0, p_http_pipe);
		}

		ret_val = BIO_get_fd(p_http_pipe->_pbio, &p_http_pipe->_socket_fd);
		if (ret_val <= 0)
		{
			LOG_ERROR("pipe(0x%X): 2 bio get fd filed", p_http_pipe);
			ret_val = HTTP_ERR_SSL_CONNECT;
			BIO_FREE_ALL(p_http_pipe->_pbio);
			goto ErrorHanle;
		}
		ret_val = SUCCESS;
		LOG_DEBUG("pipe(0x%X): create ssl connect with fd:%d", p_http_pipe, p_http_pipe->_socket_fd);
	}
	else
#endif
	if (p_pipe->_id != 0)
	{
		ret_val = socket_proxy_create_browser(&(p_http_pipe->_socket_fd), SD_SOCK_STREAM);
	}
	else
	{
		ret_val = socket_proxy_create(&(p_http_pipe->_socket_fd), SD_SOCK_STREAM);
	}

	if ((ret_val != SUCCESS) || (p_http_pipe->_socket_fd == 0))
	{
		/* Create socket error ! */
		goto ErrorHanle;
	}

	LOG_DEBUG("http_pipe_open p_http_pipe->_socket_fd = %u", p_http_pipe->_socket_fd);
	p_http_pipe->_data_pipe._dispatch_info._last_recv_data_time = MAX_U32;

	HTTP_SET_PIPE_STATE(PIPE_CONNECTING);
	HTTP_SET_STATE(HTTP_PIPE_STATE_CONNECTING);
#ifdef _ENABLE_SSL
	if (p_http_pipe->_is_https)
	{
		ret_val = socket_proxy_connect_ssl(p_http_pipe->_socket_fd, p_http_pipe->_pbio,
			http_pipe_handle_connect, p_http_pipe);
	}
	else
#endif
	{
		LOG_DEBUG("call SOCKET_PROXY_CONNECT(fd=%u,_host=%s,port=%u)",
			p_http_pipe->_socket_fd, p_url_o->_host, p_url_o->_port);
		ret_val = socket_proxy_connect_by_domain(p_http_pipe->_socket_fd, p_url_o->_host, p_url_o->_port,
			http_pipe_handle_connect, (void*)p_http_pipe);
	}

	if (ret_val != SUCCESS)
	{
		/* Open socket error ! */
#ifdef _DEBUG	
		sd_assert(FALSE);
#endif
		goto ErrorHanle;
	}


	return SUCCESS;

ErrorHanle:
	http_pipe_failure_exit(p_http_pipe, ret_val);
	return ret_val;


} /* End of http_pipe_open */



/*****************************************************************************
* Interface name : http_pipe_close
*----------------------------------------------------------------------------*
* Input parameters  :
* -------------------
*   HTTP_DATA_PIPE * _p_http_pipe
*
* Output parameters :
* -------------------
*   NONE
*
* Return values :
* -------------------
*   0 - success;other values - error
*
*----------------------------------------------------------------------------*
*----------------------------------------------------------------------------*
*                                    DESCRIPTION
*
* Close the http data pipe
*----------------------------------------------------------------------------*
******************************************************************************/
_int32 http_pipe_close(DATA_PIPE * p_pipe)
{
	HTTP_DATA_PIPE * p_http_pipe = (HTTP_DATA_PIPE*)p_pipe;

	LOG_DEBUG(" enter http_pipe[0x%X]_close()...", p_pipe);
#ifdef LITTLE_FILE_TEST
	LOG_DEBUG("http_pipe_close[0x%X], _retry_connect_timer_id=%u,_retry_get_buffer_timer_id=%u,_is_connected=%d,_http_state=%d,_b_ranges_changed=%d,_b_redirect=%d",
		p_pipe,p_http_pipe->_retry_connect_timer_id,p_http_pipe->_retry_get_buffer_timer_id,p_http_pipe->_is_connected,HTTP_GET_STATE,p_http_pipe->_b_ranges_changed,p_http_pipe->_b_redirect);
	LOG_DEBUG( " ++++ _p_http_pipe[0x%X]->_sum_recv_bytes=%llu,LargerThanRange=%d,_http_state=%d,_pipe_state=%d,ErrCode=%d",
		p_http_pipe,p_http_pipe->_sum_recv_bytes,((p_http_pipe->_sum_recv_bytes>HTTP_DEFAULT_RANGE_LEN)?1:0),HTTP_GET_STATE,HTTP_GET_PIPE_STATE,p_http_pipe->_error_code);

	//PRINT_STRACE_TO_FILE;
#endif	

#ifdef _DEBUG	
	sd_assert(p_pipe != NULL);
	sd_assert(p_pipe->_data_pipe_type == HTTP_PIPE);
#endif

	if ((!p_pipe) || (p_pipe->_data_pipe_type != HTTP_PIPE)) return HTTP_ERR_INVALID_PIPE;


	p_http_pipe->_wait_delete = TRUE;

	LOG_DEBUG(" _retry_connect_timer_id=%u,_retry_get_buffer_timer_id=%u,_is_connected=%d,_http_state=%d,_b_ranges_changed=%d,_b_redirect=%d",
		p_http_pipe->_retry_connect_timer_id, p_http_pipe->_retry_get_buffer_timer_id, p_http_pipe->_is_connected, HTTP_GET_STATE, p_http_pipe->_b_ranges_changed, p_http_pipe->_b_redirect);

	p_http_pipe->_b_ranges_changed = FALSE;
	p_http_pipe->_b_redirect = FALSE;
	if (p_http_pipe->_data_pipe._id == 0 && !p_http_pipe->_data_pipe._is_user_free)
	{
		dp_set_pipe_interface(p_pipe, NULL);
	}
#ifdef _DEBUG	
	LOG_DEBUG(" ++++ _p_http_pipe[0x%X,url=%s]->_sum_recv_bytes=%llu,LargerThanRange=%d,_http_state=%d,_pipe_state=%d,ErrCode=%d",
		p_http_pipe, p_http_pipe->_p_http_server_resource->_o_url, p_http_pipe->_sum_recv_bytes, ((p_http_pipe->_sum_recv_bytes > HTTP_DEFAULT_RANGE_LEN) ? 1 : 0), HTTP_GET_STATE, HTTP_GET_PIPE_STATE, p_http_pipe->_error_code);
#endif
	/* Cancel the timer */
	if (p_http_pipe->_retry_connect_timer_id != 0)
	{
		cancel_timer(p_http_pipe->_retry_connect_timer_id);
		return SUCCESS;
	}
	else
	if (p_http_pipe->_retry_get_buffer_timer_id != 0)
	{
		cancel_timer(p_http_pipe->_retry_get_buffer_timer_id);
		return SUCCESS;
	}
	else
	if (p_http_pipe->_retry_set_file_size_timer_id != 0)
	{
		cancel_timer(p_http_pipe->_retry_set_file_size_timer_id);
		return SUCCESS;
	}

	uninit_speed_calculator(&(p_http_pipe->_speed_calculator));


	if (((p_http_pipe->_is_connected == TRUE) || (HTTP_GET_STATE == HTTP_PIPE_STATE_CONNECTING))
		&& (HTTP_GET_STATE != HTTP_PIPE_STATE_CLOSING))
	{
		http_pipe_close_connection(p_http_pipe);
	}
	else
	if (HTTP_GET_STATE == HTTP_PIPE_STATE_CLOSING)
	{
		CHECK_VALUE(HTTP_ERR_OPEN_WRONG_STATE);
	}
	else
	{
		HTTP_SET_PIPE_STATE(PIPE_IDLE);
		HTTP_SET_STATE(HTTP_PIPE_STATE_IDLE);
		http_pipe_destroy(p_http_pipe);
	}


	return SUCCESS;

} /* End of http_pipe_close */



/*****************************************************************************
* Interface name : http_pipe_change_ranges
*----------------------------------------------------------------------------*
* Input parameters  :
* -------------------
*   const RANGE_LIST _range_list : New ranges assigned to the pipe
*
*   HTTP_DATA_PIPE * _p_http_pipe
*
* Output parameters :
* -------------------
*   NONE
*
* Return values :
* -------------------
*   0 - success;other values - error
*
*----------------------------------------------------------------------------*
*----------------------------------------------------------------------------*
*                                    DESCRIPTION
*
* Change the range http data pipe
*----------------------------------------------------------------------------*
******************************************************************************/
_int32 http_pipe_change_ranges(DATA_PIPE * p_pipe, const RANGE* alloc_range)
{
	HTTP_DATA_PIPE * p_http_pipe = (HTTP_DATA_PIPE*)p_pipe;
	RANGE range;
	_int32 ret_val = SUCCESS;

	LOG_DEBUG(" enter[0x%X] http_pipe_change_ranges(_index=%u,_num=%u).. _pipe_state=%d,_http_state=%d .", p_pipe, alloc_range->_index, alloc_range->_num, HTTP_GET_PIPE_STATE, HTTP_GET_STATE);


#ifdef _DEBUG	
	sd_assert(p_pipe != NULL);
	sd_assert(p_pipe->_data_pipe_type == HTTP_PIPE);
	sd_assert(p_http_pipe->_wait_delete != TRUE);
	sd_assert(alloc_range->_num != 0);
	sd_assert(HTTP_GET_PIPE_STATE != PIPE_FAILURE);
#endif

	if ((!p_pipe) || (p_pipe->_data_pipe_type != HTTP_PIPE)) return HTTP_ERR_INVALID_PIPE;

	if (p_http_pipe->_wait_delete) return HTTP_ERR_WAITING_DELETE;

	if (alloc_range->_num == 0) return HTTP_ERR_INVALID_RANGES;

	if (HTTP_GET_PIPE_STATE == PIPE_FAILURE)
		CHECK_VALUE(HTTP_ERR_WRONG_STATE);

	if (HTTP_RESOURCE_IS_SUPPORT_RANGE(p_http_pipe->_p_http_server_resource) == FALSE)
	{
		LOG_DEBUG(" [0x%X] http_pipe_change_ranges: the pipe is not support range,_req_end_pos=%llu,new_index_pos=%llu,down_range.index=%u,down_range.unm=%u", p_pipe, p_http_pipe->_req_end_pos, get_data_pos_from_index(alloc_range->_index), http_pipe_get_download_range_index(p_http_pipe), http_pipe_get_download_range_num(p_http_pipe));
		if ((alloc_range->_index >= http_pipe_get_download_range_index(p_http_pipe)) &&
			(p_http_pipe->_req_end_pos > get_data_pos_from_index(alloc_range->_index)))
		{
			range._index = http_pipe_get_download_range_index(p_http_pipe);
			range._num = (alloc_range->_index - http_pipe_get_download_range_index(p_http_pipe)) + alloc_range->_num;
		}
		else
		{
			range._index = 0;
			range._num = alloc_range->_index + alloc_range->_num;
		}
	}
	else
	{
		range._index = alloc_range->_index;
		range._num = alloc_range->_num;
	}

	if (0 == dp_get_uncomplete_ranges_list_size(&(p_http_pipe->_data_pipe)))
	{

		ret_val = dp_add_uncomplete_ranges(&(p_http_pipe->_data_pipe), &range);
		if (ret_val != SUCCESS)  		goto ErrorHanle;
	}
	else
	{

		ret_val = dp_clear_uncomplete_ranges_list(&(p_http_pipe->_data_pipe));
		if (ret_val != SUCCESS)  		goto ErrorHanle;
		ret_val = dp_add_uncomplete_ranges(&(p_http_pipe->_data_pipe), &range);
		if (ret_val != SUCCESS)  		goto ErrorHanle;
	}

	LOG_DEBUG(" [0x%X] http_pipe_change_ranges success!the new ranges is (_index=%u,_num=%u),down_range.index=%u,down_range.unm=%u", p_pipe, range._index, range._num, http_pipe_get_download_range_index(p_http_pipe), http_pipe_get_download_range_num(p_http_pipe));

	if (!p_http_pipe->_b_ranges_changed)
		p_http_pipe->_b_ranges_changed = TRUE;

	if ((p_http_pipe->_b_retry_set_file_size != TRUE) && (p_http_pipe->_b_data_full != TRUE))
	{
		if ((HTTP_GET_PIPE_STATE == PIPE_DOWNLOADING) &&
			((HTTP_GET_STATE == HTTP_PIPE_STATE_DOWNLOAD_COMPLETED) || (HTTP_GET_STATE == HTTP_PIPE_STATE_CONNECTED)))
		{
			ret_val = http_pipe_send_request(p_http_pipe);

			if (ret_val != SUCCESS)  		goto ErrorHanle;

		}
		else
		if (((HTTP_GET_PIPE_STATE == PIPE_IDLE) || (HTTP_GET_PIPE_STATE == PIPE_DOWNLOADING))
			&& ((HTTP_GET_STATE == HTTP_PIPE_STATE_IDLE)))
		{
			return http_pipe_open((DATA_PIPE *)p_http_pipe);
		}
	}

	return SUCCESS;

ErrorHanle:
	http_pipe_failure_exit(p_http_pipe, ret_val);
	return SUCCESS;

} /* End of http_pipe_change_ranges */



/*****************************************************************************
* Interface name : http_pipe_get_speed
*----------------------------------------------------------------------------*
* Input parameters  :
* -------------------
*
*   DATA_PIPE * p_pipe
*
* Output parameters :
* -------------------
*   NONE
*
* Return values :
* -------------------
*   0 - success;other values - error
*
*----------------------------------------------------------------------------*
*----------------------------------------------------------------------------*
*                                    DESCRIPTION
*
* Interface for connect_manager getting the pipe speed
*----------------------------------------------------------------------------*
******************************************************************************/

_int32 http_pipe_get_speed(DATA_PIPE * p_pipe)
{
	_int32 ret_val = SUCCESS;
	HTTP_DATA_PIPE * p_http_pipe = (HTTP_DATA_PIPE *)p_pipe;

#ifdef _DEBUG	
	sd_assert(p_pipe != NULL);
	sd_assert(p_pipe->_data_pipe_type == HTTP_PIPE);
#endif


	if ((!p_pipe) || (p_pipe->_data_pipe_type != HTTP_PIPE)) return HTTP_ERR_INVALID_PIPE;

	//if(p_http_pipe->_b_data_full != TRUE)
	{
		ret_val = calculate_speed(&(p_http_pipe->_speed_calculator), &(HTTP_SPEED));
		CHECK_VALUE(ret_val);

	}

	LOG_DEBUG(" enter[0x%X] http_pipe_get_speed=%u", p_http_pipe, HTTP_SPEED);

	return SUCCESS;
}



/*****************************************************************************
* Interface name : http_pipe_destroy
*----------------------------------------------------------------------------*
* Input parameters  :
* -------------------
*
*   HTTP_DATA_PIPE * _p_http_pipe
*
* Output parameters :
* -------------------
*   NONE
*
* Return values :
* -------------------
*   0 - success;other values - error
*
*----------------------------------------------------------------------------*
*----------------------------------------------------------------------------*
*                                    DESCRIPTION
*
* Destroy the http data pipe and  release memory
*----------------------------------------------------------------------------*
******************************************************************************/
_int32 http_pipe_destroy(struct tag_HTTP_DATA_PIPE * p_http_pipe)
{

	LOG_DEBUG(" enter http_pipe_destroy(0x%X)...", p_http_pipe);

	if (p_http_pipe != NULL)
	{

		http_pipe_reset_pipe(p_http_pipe);
		mpool_free_slip(gp_http_pipe_slab, (void*)p_http_pipe);

		LOG_DEBUG("http_pipe_destroy SUCCESS!");

		return SUCCESS;
	}

	LOG_DEBUG("http_pipe_destroy HTTP_ERR_INVALID_PIPE!");

	return HTTP_ERR_INVALID_PIPE;

} /* End of http_pipe_destroy */

_int32 http_get_1024_buffer(void** pp_buffer)
{
	return mpool_get_slip(gp_http_1024_slab, pp_buffer);
}
_int32 http_release_1024_buffer(void* p_buffer)
{
	return mpool_free_slip(gp_http_1024_slab, p_buffer);
}

_u32 http_get_every_time_reveive_range_num(void)
{
	return g_http_every_time_reveive_range_num;
}

#endif /* __HTTP_DATA_PIPE_INTERFACE_C_20090305 */
