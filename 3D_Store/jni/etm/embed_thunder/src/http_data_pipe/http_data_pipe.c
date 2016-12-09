#if !defined(__HTTP_DATA_PIPE_C_20080614)
#define __HTTP_DATA_PIPE_C_20080614

#include "platform/sd_socket.h"
#include "utility/settings.h"
#include "utility/base64.h"
#include "p2sp_download/p2sp_task.h"
#include "utility/sd_iconv.h"
#include "platform/sd_network.h"
#include "utility/logid.h"
#define LOGID LOGID_HTTP_PIPE
#include "utility/logger.h"

#include "http_data_pipe.h"
#include "http_data_pipe_interface.h"
#include "download_task/download_task.h"
#include "http_mini.h"

#include "socket_proxy_interface.h"

static _u32 g_403_count = 0;
#ifdef _ENABLE_SSL
SSL_CTX*	gp_ssl_ctx;
#endif
/*--------------------------------------------------------------------------*/
/*                Call back functions                                       */
/*--------------------------------------------------------------------------*/

static _int32 http_relation_pipe_add_range(HTTP_DATA_PIPE * p_http_pipe)
{
	LOG_DEBUG("p_http_pipe=0x%X p_http_pipe->_p_http_server_resource->_block_info_num=%u",
		p_http_pipe, p_http_pipe->_p_http_server_resource->_block_info_num);

	RANGE last_range;

	last_range._index = -1;
	last_range._num = 0;

	_u32 block_idx = 0;
	for (block_idx = 0; block_idx < p_http_pipe->_p_http_server_resource->_block_info_num; block_idx++)
	{
		RELATION_BLOCK_INFO* p_block_info = &p_http_pipe->_p_http_server_resource->_p_block_info_arr[block_idx];
		RANGE r;
		r._index = (p_block_info->_origion_start_pos + get_data_unit_size() - 1) / get_data_unit_size();
		r._num = (p_block_info->_origion_start_pos + p_block_info->_length) / get_data_unit_size() - r._index;

		LOG_DEBUG("p_http_pipe=0x%X last_range(%u,%u), cur_range(%u,%u)",
			p_http_pipe, last_range._index, last_range._num, r._index, r._num);

		if (last_range._index != -1 && last_range._num != 0)
		{
			// 关联资源可加速的范围全部为非相邻，相邻的需要去掉一个range
			// 否则调度时会导致错误.. 两个不同的range会连成一个大的range
			if (last_range._index + last_range._num == r._index)
			{
				r._index = r._index + 1;
				r._num = (p_block_info->_origion_start_pos + p_block_info->_length) / get_data_unit_size() - r._index;
				if (r._num == 0)
				{
					LOG_DEBUG("error because range is zero range ...p_http_pipe=0x%X is relation resource pipe,"
						"block(%llu, %llu, %llu) add range(%u, %u)",
						p_http_pipe,
						p_block_info->_origion_start_pos, p_block_info->_relation_start_pos, p_block_info->_length,
						r._index, r._num);
					continue;
				}
			}

			if (last_range._index + last_range._num > r._index)
			{
				sd_assert(0); // 理论上不可能走到这个逻辑，增加保护机制...
				//日志版本让其直接崩溃，非日志版本则走保护逻辑

				http_pipe_failure_exit(p_http_pipe, HTTP_PIPE_STATE_UNKNOW);
				CHECK_VALUE(HTTP_PIPE_STATE_UNKNOW);
			}
		}

		last_range._index = r._index;
		last_range._num = r._num;

		LOG_DEBUG("p_http_pipe=0x%X is relation resource pipe, block(%llu, %llu, %llu) add range(%u, %u)",
			p_http_pipe, p_block_info->_origion_start_pos, p_block_info->_relation_start_pos, p_block_info->_length, r._index, r._num);

		sd_assert(r._num > 0);  // 由于设置的最小 length为32K,所以肯定有一个完整的range
		dp_add_can_download_ranges(&(p_http_pipe->_data_pipe), &r);
	}


	return SUCCESS;
}


/*****************************************************************************
* Interface name : http_pipe_handle_connect
*----------------------------------------------------------------------------*
* Input parameters  :
* -------------------
*
*   HTTP_DATA_PIPE * _p_http_pipe
*
*    _int32 _errcode : Error code
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
* Connecting socket call back function
*----------------------------------------------------------------------------*
******************************************************************************/
_int32 http_pipe_handle_connect(_int32 errcode, _u32 notice_count_left, void *user_data)
{
	HTTP_DATA_PIPE * p_http_pipe = (HTTP_DATA_PIPE *)user_data;
	RANGE all_range;
	_int32 ret_val = SUCCESS;

	LOG_DEBUG(" enter http_pipe_handle_connect([0x%X] %d, scheme_type=%d)...",
		p_http_pipe, errcode, p_http_pipe->_p_http_server_resource->_url._schema_type);


#ifdef _DEBUG
	sd_assert(p_http_pipe != NULL);
#endif

	if (!p_http_pipe) return HTTP_ERR_INVALID_PIPE;

	if ((HTTP_GET_STATE != HTTP_PIPE_STATE_CONNECTING) && (HTTP_GET_STATE != HTTP_PIPE_STATE_CLOSING))
		CHECK_VALUE(HTTP_ERR_WRONG_STATE);

	if (errcode == SUCCESS)
	{
		/* Connecting success */
		HTTP_SET_PIPE_STATE(PIPE_DOWNLOADING);
		p_http_pipe->_is_connected = TRUE;
		p_http_pipe->_data_pipe._dispatch_info._time_stamp = 0;   ///????

		LOG_DEBUG("p_http_pipe=0x%X Connecting success! ", p_http_pipe);
#ifdef _DEBUG
		if (HTTP_RESOURCE_IS_ORIGIN(p_http_pipe->_p_http_server_resource) == TRUE)
		{
			LOG_DEBUG("+++ The origin resource:%s is connected!!", p_http_pipe->_p_http_server_resource->_o_url);
		}
		else
		{
			LOG_DEBUG("+++ The inquire resource:%s is connected is_relation:%d!!",
				p_http_pipe->_p_http_server_resource->_o_url, p_http_pipe->_p_http_server_resource->_relation_res);
		}
#endif
		/* reset the speed */
		//uninit_speed_calculator(&(_p_http_pipe->_speed_calculator));
		//init_speed_calculator(&(_p_http_pipe->_speed_calculator), 10, 1000);
		//calculate_speed(&(_p_http_pipe->_speed_calculator), &(_p_http_pipe->_data_pipe._dispatch_info._speed));

		ret_val = sd_time_ms(&(p_http_pipe->_data_pipe._dispatch_info._time_stamp));
		if (ret_val == SUCCESS)
		{
			HTTP_SET_STATE(HTTP_PIPE_STATE_CONNECTED)
				//  _p_http_pipe->_data_pipe._dispatch_info._is_support_long_connect = TRUE;
			if (p_http_pipe->_b_redirect == TRUE)
				HTTP_RESOURCE_PLUS_REDIRECT_TIMES(p_http_pipe->_p_http_server_resource);


			if (dp_get_can_download_ranges_list_size(&(p_http_pipe->_data_pipe)) == 0)
			{

				LOG_DEBUG("p_http_pipe=0x%X dp_get_can_download_ranges_list_size( &(p_http_pipe->_data_pipe) ) ==0 ", p_http_pipe);

				if (p_http_pipe->_p_http_server_resource->_relation_res)
				{
					http_relation_pipe_add_range(p_http_pipe);
				}
				else
				{

					_u64 file_size = http_pipe_get_file_size(p_http_pipe);
					if (file_size == 0)
					{
						all_range._index = 0;
						all_range._num = MAX_U32;
					}
					else
					{
						all_range = pos_length_to_range(0, file_size, file_size);
					}

					LOG_DEBUG("p_http_pipe=0x%X add all range, index: %u, num:%u", p_http_pipe, all_range._index, all_range._num);

					dp_add_can_download_ranges(&(p_http_pipe->_data_pipe), &all_range);
				}
			}
			else
			{
				LOG_DEBUG("p_http_pipe=0x%X dp_get_can_download_ranges_list_size not zero, no need add.", p_http_pipe);
			}


			LOG_DEBUG("p_http_pipe=0x%X Connected!_connected_time=%u ",
				p_http_pipe, p_http_pipe->_data_pipe._dispatch_info._time_stamp);
			if (dp_get_uncomplete_ranges_list_size(&(p_http_pipe->_data_pipe)) != 0)
			{
				if (p_http_pipe->_b_ranges_changed != TRUE)
					p_http_pipe->_b_ranges_changed = TRUE;

				ret_val = http_pipe_send_request(p_http_pipe);

			}
			else
			{
				if (http_pipe_get_file_size(p_http_pipe) == 0)
				{
					RANGE alloc_range;
					alloc_range._index = 0;
					alloc_range._num = MAX_U32;
					http_pipe_change_ranges((DATA_PIPE *)p_http_pipe, &alloc_range);
				}
				else
				{
					LOG_DEBUG("_p_http_pipe=0x%X:dm_notify_dispatch_data_finish ", p_http_pipe);
					pi_notify_dispatch_data_finish((DATA_PIPE *)p_http_pipe);
				}
			}
		}

		if (ret_val != SUCCESS)
			http_pipe_failure_exit(p_http_pipe, ret_val);
	}
	else if (errcode == MSG_CANCELLED)
	{
		if (HTTP_GET_STATE == HTTP_PIPE_STATE_CLOSING)
		{
			LOG_DEBUG("p_http_pipe=0x%X MSG_CANCELLED,HTTP_PIPE_STATE_CLOSING", p_http_pipe);
			http_pipe_close_connection(p_http_pipe);
		}
	}
	else
	{
		LOG_DEBUG("p_http_pipe=0x%X  call SOCKET_PROXY_CLOSE(fd=%u)", p_http_pipe, p_http_pipe->_socket_fd);
		ret_val = socket_proxy_close(p_http_pipe->_socket_fd);
		p_http_pipe->_socket_fd = 0;
		if (ret_val == SUCCESS)
		{
			if (p_http_pipe->_already_try_count >= p_http_pipe->_max_try_count)
			{
				/* Failed */
				ret_val = errcode;

			}
			else
			{
				/* Try again later... */
				HTTP_SET_STATE(HTTP_PIPE_STATE_RETRY_WAITING);
				/* Start timer */
				LOG_DEBUG("p_http_pipe=0x%X  call retry_connect START_TIMER", p_http_pipe);
				ret_val = start_timer(http_pipe_handle_retry_connect_timeout, NOTICE_ONCE,
					RETRY_CONNECT_WAIT_M_SEC, 0, p_http_pipe, &(p_http_pipe->_retry_connect_timer_id));
			}

		}

		if (ret_val != SUCCESS)
			http_pipe_failure_exit(p_http_pipe, ret_val);

	}

	return SUCCESS;

} /* End of http_pipe_handle_connect */




/*****************************************************************************
* Interface name : http_pipe_handle_send
*----------------------------------------------------------------------------*
* Input parameters  :
* -------------------
*
*   HTTP_DATA_PIPE * _p_http_pipe
*
*    _int32 _errcode : Error code
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
* Writing socket call back function
*----------------------------------------------------------------------------*
******************************************************************************/
_int32 http_pipe_handle_send(_int32 errcode, _u32 notice_count_left, const char* buffer, _u32 had_send, void *user_data)
{
	_u64 conten_len = 0;
	HTTP_DATA_PIPE * p_http_pipe = (HTTP_DATA_PIPE *)user_data;
	_int32 ret_val = SUCCESS;
	BOOL send_end = TRUE;

	LOG_DEBUG(" enter[0x%X] http_pipe_handle_send(_errcode=%d,had_send=%u)...",
		p_http_pipe, errcode, had_send);

#ifdef _DEBUG
	sd_assert(p_http_pipe != NULL);
#endif

	if (!p_http_pipe) return HTTP_ERR_INVALID_PIPE;

	if ((HTTP_GET_STATE != HTTP_PIPE_STATE_REQUESTING) && (HTTP_GET_STATE != HTTP_PIPE_STATE_CLOSING))
		CHECK_VALUE(HTTP_ERR_WRONG_STATE);

	if (errcode == SUCCESS)
	{
		LOG_DEBUG("http_pipe[0x%X]Sending success! ", p_http_pipe);

		if (p_http_pipe->_is_post)
		{
			/* 发送完毕，通知界面 */
			ret_val = mini_http_notify_sent_data((DATA_PIPE*)p_http_pipe, had_send, &send_end);
			if (ret_val != SUCCESS)
			{
				http_pipe_failure_exit(p_http_pipe, ret_val);
				return SUCCESS;
			}

			if (!send_end)
			{
				_u8 * data = NULL;
				_u32 data_len = 0;
				/* 继续发送下一块数据 */
				ret_val = mini_http_get_post_data((DATA_PIPE*)p_http_pipe, &data, &data_len);
				if (ret_val != SUCCESS)
				{
					http_pipe_failure_exit(p_http_pipe, ret_val);
					return SUCCESS;
				}
				HTTP_SET_STATE(HTTP_PIPE_STATE_REQUESTING);
				ret_val = socket_proxy_send_browser(p_http_pipe->_socket_fd, (char*)data, data_len,
					http_pipe_handle_send, (void*)p_http_pipe);
				if (ret_val != SUCCESS)
				{
					http_pipe_failure_exit(p_http_pipe, ret_val);
				}
				return SUCCESS;
			}
		}
		conten_len = p_http_pipe->_data._content_length;
		/* Requesting success */
		HTTP_SET_STATE(HTTP_PIPE_STATE_READING);

		http_reset_respn_header(&(p_http_pipe->_header));
		http_pipe_reset_respn_data(p_http_pipe, &(p_http_pipe->_data));


		p_http_pipe->_data._content_length = conten_len;

		p_http_pipe->_b_header_received = FALSE;

		p_http_pipe->_header._header_buffer_length = HTTP_HEADER_DEFAULT_LEN;

		if (p_http_pipe->_header._header_buffer == NULL)
		{
			ret_val = http_get_1024_buffer((void**)&(p_http_pipe->_header._header_buffer));

		}

		if (ret_val == SUCCESS)
		{
			sd_memset(p_http_pipe->_header._header_buffer, 0, p_http_pipe->_header._header_buffer_length);
			LOG_DEBUG("http_pipe[0x%X] call SOCKET_PROXY_UNCOMPLETE_RECV(fd=%u,buffer_length=%u) ",
				p_http_pipe, p_http_pipe->_socket_fd, p_http_pipe->_header._header_buffer_length);
#ifdef _ENABLE_SSL
			if (p_http_pipe->_is_https)
			{
				ret_val = socket_proxy_recv_ssl(p_http_pipe->_socket_fd, p_http_pipe->_pbio,
					p_http_pipe->_header._header_buffer,
					p_http_pipe->_header._header_buffer_length - 1,
					http_pipe_handle_uncomplete_recv, (void*)p_http_pipe);
			}
			else
#endif
			if (p_http_pipe->_data_pipe._id != 0)  // Browser request
			{
				ret_val = socket_proxy_uncomplete_recv_browser(p_http_pipe->_socket_fd, p_http_pipe->_header._header_buffer,
					p_http_pipe->_header._header_buffer_length - 1, http_pipe_handle_uncomplete_recv, (void*)p_http_pipe);
			}
			else
			{
				ret_val = socket_proxy_uncomplete_recv(p_http_pipe->_socket_fd, p_http_pipe->_header._header_buffer,
					p_http_pipe->_header._header_buffer_length - 1, http_pipe_handle_uncomplete_recv, (void*)p_http_pipe);
			}

		}

#ifdef _DEBUG
		sd_assert(ret_val == SUCCESS);
#endif

		if (ret_val != SUCCESS)
			http_pipe_failure_exit(p_http_pipe, ret_val);
	}
	else if ((errcode == MSG_CANCELLED) && (HTTP_GET_STATE == HTTP_PIPE_STATE_CLOSING))
	{
		LOG_DEBUG("http_pipe[0x%X] Sending canceled! ", p_http_pipe);
		if (p_http_pipe->_is_post)
		{
			/* 发送完毕，通知界面 */
			mini_http_notify_sent_data((DATA_PIPE*)p_http_pipe, had_send, &send_end);
		}
		http_pipe_close_connection(p_http_pipe);

	}
	else
	{
		/* Failed */
		LOG_DEBUG("http_pipe[0x%X] Sending failed! ", p_http_pipe);
		if (p_http_pipe->_is_post)
		{
			/* 发送完毕，通知界面 */
			mini_http_notify_sent_data((DATA_PIPE*)p_http_pipe, had_send, &send_end);
		}
		http_pipe_failure_exit(p_http_pipe, errcode);
	}

	return SUCCESS;

}/* End of http_pipe_handle_send */


/*****************************************************************************
* Interface name : http_pipe_handle_uncomplete_recv
*----------------------------------------------------------------------------*
* Input parameters  :
* -------------------
*
*   HTTP_DATA_PIPE * _p_http_pipe
*
*    _int32 _errcode : Error code
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
* Uncomplete reading socket call back function
*----------------------------------------------------------------------------*
******************************************************************************/
_int32 http_pipe_handle_uncomplete_recv(_int32 errcode, _u32 notice_count_left, char* buffer, _u32 had_recv, void *user_data)
{
	//_u32 _cur_time =0;
	HTTP_DATA_PIPE * p_http_pipe = (HTTP_DATA_PIPE *)user_data;
	_int32 ret_val = SUCCESS;

	LOG_DEBUG(" enter[0x%X] http_pipe_handle_uncomplete_recv(_errcode=%d,had_recv=%u)...", p_http_pipe, errcode, had_recv);

#ifdef _DEBUG
	sd_assert(p_http_pipe != NULL);
#endif

	if (!p_http_pipe) return HTTP_ERR_INVALID_PIPE;

	if ((HTTP_GET_STATE != HTTP_PIPE_STATE_READING) && (HTTP_GET_STATE != HTTP_PIPE_STATE_CLOSING))
		return HTTP_ERR_WRONG_STATE;


	if (errcode == SUCCESS)
	{
		sd_time_ms(&(p_http_pipe->_data_pipe._dispatch_info._last_recv_data_time));
		if (had_recv != 0)
		{
			LOG_DEBUG("  http_pipe[0x%X] uncomplete Receiving success! ", p_http_pipe);
			if (p_http_pipe->_b_header_received == FALSE)
			{
				/* Reading success,start parse the http header  */
				ret_val = http_pipe_handle_recv_header(p_http_pipe, had_recv);
			}
			else if (p_http_pipe->_header._is_chunked == TRUE)
			{
				ret_val = http_pipe_handle_recv_chunked(p_http_pipe, had_recv);
			}
			else if (p_http_pipe->_header._has_content_length == FALSE)
			{
				LOG_DEBUG("  http_pipe[0x%X]_b_header_received==TRUE && _p_http_pipe->_header._has_content_length==FALSE", p_http_pipe);

				/* Reading success,start parse the http header and data */
				ret_val = http_pipe_handle_recv_body(p_http_pipe, had_recv);

			}
			else
			{
				LOG_DEBUG("  http_pipe[0x%X]handle_uncomplete_recv HTTP_ERR_UNKNOWN", p_http_pipe);
#ifdef _DEBUG
				CHECK_VALUE(1);
#endif
				ret_val = HTTP_ERR_UNKNOWN;

			}

		}
		else
		{
			ret_val = http_pipe_handle_recv_0_byte(p_http_pipe);
		}
	}




	else if (errcode == SOCKET_CLOSED)
	{
		if ((p_http_pipe->_rev_wap_page == TRUE) && (had_recv == 0) && (p_http_pipe->_b_header_received == FALSE))
		{
			LOG_DEBUG("_p_http_pipe=0x%X Disconnect by remote in CMWAP when request in second time,need reconnect! ", p_http_pipe);
			HTTP_SET_STATE(HTTP_PIPE_STATE_RANGE_CHANGED_NEED_RECONNECT);
			ret_val = SUCCESS;
		}
		else
		{
			ret_val = http_pipe_handle_recv_2249(p_http_pipe, had_recv);
			if (ret_val == SUCCESS)
			{
				ret_val = errcode;
			}
		}
	}
	else if ((errcode == MSG_CANCELLED) && (p_http_pipe->_http_state == HTTP_PIPE_STATE_CLOSING))
	{
		LOG_DEBUG("  http_pipe[0x%X]Receiving canceled! ", p_http_pipe);
		http_pipe_close_connection(p_http_pipe);
		return SUCCESS;
	}
	else
	{
		/* Failed */
		LOG_DEBUG("  http_pipe[0x%X]Receiving failed! ", p_http_pipe);
		ret_val = errcode;
	}

	LOG_DEBUG("errcode = %d, ret_val = %d, http_state = %d", errcode, ret_val, p_http_pipe->_http_state);

	if (p_http_pipe->_rev_wap_page == TRUE)
	{
		p_http_pipe->_rev_wap_page = FALSE;
	}


	if ((ret_val == SUCCESS) || (ret_val == HTTP_ERR_HEADER_NOT_FOUND))
	{
		http_pipe_do_next(p_http_pipe);
	}
	else
	{
		http_pipe_failure_exit(p_http_pipe, ret_val);
	}
	return SUCCESS;

}/* End of http_pipe_handle_uncomplete_recv */


/*****************************************************************************
* Interface name : http_pipe_handle_recv
*----------------------------------------------------------------------------*
* Input parameters  :
* -------------------
*
*   HTTP_DATA_PIPE * _p_http_pipe
*
*    _int32 _errcode : Error code
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
* Reading socket call back function
*----------------------------------------------------------------------------*
******************************************************************************/

_int32  http_pipe_handle_recv(_int32 errcode, _u32 notice_count_left, char* buffer, _u32 had_recv, void *user_data)
{
	HTTP_DATA_PIPE * p_http_pipe = (HTTP_DATA_PIPE *)user_data;
	_int32 ret_val = SUCCESS;
	LOG_DEBUG(" enter[0x%X] http_pipe_handle_recv(_errcode=%d,had_recv=%u)...", p_http_pipe, errcode, had_recv);

#ifdef _DEBUG
	sd_assert(p_http_pipe != NULL);
#endif

	if (!p_http_pipe) return HTTP_ERR_INVALID_PIPE;

	if ((HTTP_GET_STATE != HTTP_PIPE_STATE_READING) && (HTTP_GET_STATE != HTTP_PIPE_STATE_CLOSING))
		CHECK_VALUE(HTTP_ERR_WRONG_STATE);


	if (errcode == SUCCESS)
	{
		sd_time_ms(&(p_http_pipe->_data_pipe._dispatch_info._last_recv_data_time));
		if (had_recv != 0)
		{
			LOG_DEBUG("  http_pipe[0x%X]Receiving success! ", p_http_pipe);
			if (p_http_pipe->_header._is_chunked == TRUE)
			{
				ret_val = http_pipe_handle_recv_chunked(p_http_pipe, had_recv);
			}
			else
			{

				ret_val = http_pipe_handle_recv_body(p_http_pipe, had_recv);
			}
		}
		else
		{
			/* Error */
			ret_val = http_pipe_handle_recv_0_byte(p_http_pipe);
		}

	}
	else if (errcode == SOCKET_CLOSED)
	{
		ret_val = http_pipe_handle_recv_2249(p_http_pipe, had_recv);
		if (ret_val == SUCCESS)
		{
			ret_val = errcode;
		}
	}
	else if ((errcode == MSG_CANCELLED) && (HTTP_GET_STATE == HTTP_PIPE_STATE_CLOSING))
	{
		LOG_DEBUG("  http_pipe[0x%X] Receiving canceled! ", p_http_pipe);
		http_pipe_close_connection(p_http_pipe);
		return SUCCESS;

	}
	else
	{
		/* Failed */
		LOG_DEBUG("  http_pipe[0x%X]Receiving failed! ", p_http_pipe);
		ret_val = errcode;
	}

	if (ret_val == SUCCESS)
	{
		http_pipe_do_next(p_http_pipe);
	}
	else
	{
		http_pipe_failure_exit(p_http_pipe, ret_val);
	}
	return SUCCESS;

} /* End of http_pipe_handle_recv */





/*****************************************************************************
* Interface name : http_pipe_handle_timeout
*----------------------------------------------------------------------------*
* Input parameters  :
* -------------------
*
*   HTTP_DATA_PIPE * _p_http_pipe
*
*    _int32 _errcode : Error code
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
* Timer timeout call back function
*----------------------------------------------------------------------------*
******************************************************************************/
_int32 http_pipe_handle_retry_connect_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid)
{
	HTTP_DATA_PIPE * p_http_pipe = (HTTP_DATA_PIPE *)(msg_info->_user_data);
	_int32 ret_val = SUCCESS;

	LOG_DEBUG(" enter[0x%X] http_pipe_handle_retry_connect_timeout(errcode=%d,left=%u,expired=%u,msgid=%u)...",
		p_http_pipe, errcode, notice_count_left, expired, msgid);

#ifdef _DEBUG
	sd_assert(p_http_pipe != NULL);
#endif

	if (!p_http_pipe) return HTTP_ERR_INVALID_PIPE;

	LOG_DEBUG(" http_pipe[0x%X] _http_state=%d,_retry_connect_timer_id=%u,_b_data_full=%d,"
		"_retry_get_buffer_timer_id=%u,_error_code=%d,"
		"_b_retry_set_file_size=%d,_retry_set_file_size_timer_id=%d)...",
		p_http_pipe, HTTP_GET_STATE, p_http_pipe->_retry_connect_timer_id,
		p_http_pipe->_b_data_full, p_http_pipe->_retry_get_buffer_timer_id, p_http_pipe->_error_code,
		p_http_pipe->_b_retry_set_file_size, p_http_pipe->_retry_set_file_size_timer_id);

	if (errcode == MSG_TIMEOUT)
	{
		if ((HTTP_GET_STATE == HTTP_PIPE_STATE_RETRY_WAITING) && (msgid == p_http_pipe->_retry_connect_timer_id))
		{
			/* retry connect time out */
			p_http_pipe->_already_try_count++;
			LOG_DEBUG(" _retry_connect_timer_id =%u,_already_try_connect_count=%u",
				p_http_pipe->_retry_connect_timer_id, p_http_pipe->_already_try_count);
			p_http_pipe->_retry_connect_timer_id = 0;
			/* connect again */
			ret_val = http_pipe_do_connect(p_http_pipe);
			if (ret_val != SUCCESS) http_pipe_failure_exit(p_http_pipe, ret_val);
		}
		else
		{
			LOG_DEBUG(" p_http_pipe=0x%X HTTP_ERR_WRONG_TIME_OUT", p_http_pipe);
#ifdef _DEBUG
			CHECK_VALUE(1);
#endif
		}

	}
	else if (errcode == MSG_CANCELLED)
	{
		LOG_DEBUG(" MSG_CANCELLED:_retry_connect_timer_id=%u,_retry_get_buffer_timer_id=%u,"
			"_retry_set_file_size_timer_id=%u,_wait_delete=%d",
			p_http_pipe->_retry_connect_timer_id, p_http_pipe->_retry_get_buffer_timer_id,
			p_http_pipe->_retry_set_file_size_timer_id, p_http_pipe->_wait_delete);
		if (p_http_pipe->_retry_connect_timer_id == msgid)
			p_http_pipe->_retry_connect_timer_id = 0;

		if (p_http_pipe->_wait_delete == TRUE)
		{
			http_pipe_close((DATA_PIPE *)p_http_pipe);
			return SUCCESS;
		}
	}
	else
	{
		LOG_DEBUG(" p_http_pipe=0x%X HTTP_ERR_WRONG_TIME_OUT,errcode=%d", p_http_pipe, errcode);
#ifdef _DEBUG
		CHECK_VALUE(1);
#endif
	}

	return SUCCESS;
} /* End of http_pipe_handle_timeout */
_u32 http_pipe_get_buffer_wait_timeout(HTTP_DATA_PIPE * p_http_pipe)
{
	_u32 time_out = 1000;
	/*
	if(HTTP_SPEED>100000)
	{
	time_out= 50;
	}
	else
	if(HTTP_SPEED>50000)
	{
	time_out= 100;
	}
	else
	if(HTTP_SPEED>20000)
	{
	time_out= 200;
	}
	else
	{
	time_out= 300;
	}
	*/
	LOG_DEBUG(" enter[0x%X] http_pipe_get_buffer_wait_timeout(HTTP_SPEED=%u,time_out=%u)...", p_http_pipe, HTTP_SPEED, time_out);
	return time_out;
}


_int32 http_pipe_handle_retry_get_buffer_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid)
{
	HTTP_DATA_PIPE * p_http_pipe = (HTTP_DATA_PIPE *)(msg_info->_user_data);
	_int32 ret_val = SUCCESS;

	LOG_DEBUG(" enter[0x%X] http_pipe_handle_retry_get_buffer_timeout(errcode=%d,left=%u,expired=%u,msgid=%u)...",
		p_http_pipe, errcode, notice_count_left, expired, msgid);

#ifdef _DEBUG
	sd_assert(p_http_pipe != NULL);
#endif

	if (!p_http_pipe) return HTTP_ERR_INVALID_PIPE;

	LOG_DEBUG(" http_pipe[0x%X] _http_state=%d,_retry_connect_timer_id=%u,_b_data_full=%d,"
		"_retry_get_buffer_timer_id=%u,_error_code=%d,_b_retry_set_file_size=%d,"
		"_retry_set_file_size_timer_id=%d)...",
		p_http_pipe, HTTP_GET_STATE, p_http_pipe->_retry_connect_timer_id, p_http_pipe->_b_data_full,
		p_http_pipe->_retry_get_buffer_timer_id, p_http_pipe->_error_code, p_http_pipe->_b_retry_set_file_size,
		p_http_pipe->_retry_set_file_size_timer_id);

	if (errcode == MSG_TIMEOUT)
	{
		if ((HTTP_GET_STATE == HTTP_PIPE_STATE_READING)
			&& (p_http_pipe->_b_data_full == TRUE)
			&& (msgid == p_http_pipe->_retry_get_buffer_timer_id))
		{
			/* get buffer time out */
			LOG_DEBUG("_http_state=%d, _retry_get_buffer_timer_id =%u,_already_try_get_data_count=%u",
				p_http_pipe, p_http_pipe->_retry_get_buffer_timer_id, p_http_pipe->_get_buffer_try_count + 1);
			p_http_pipe->_retry_get_buffer_timer_id = 0;
			p_http_pipe->_b_data_full = FALSE;
			pipe_set_err_get_buffer(&(p_http_pipe->_data_pipe), FALSE);

			/* Get buffer from data manager,maybe the _data_buffer_length will be longer than _data_length */
			LOG_DEBUG("  http_pipe[0x%X] DM_GET_DATA_BUFFER(_p_data_manager=0x%X,_buffer=0x%X,"
				"&(_buffer)=0x%X,_buffer_length=%u)...",
				p_http_pipe, p_http_pipe->_data_pipe._p_data_manager, p_http_pipe->_data._data_buffer,
				&(p_http_pipe->_data._data_buffer), p_http_pipe->_data._data_buffer_length);

			sd_assert(p_http_pipe->_data._data_buffer == NULL);

			ret_val = pi_get_data_buffer((DATA_PIPE *)p_http_pipe, &(p_http_pipe->_data._data_buffer),
				&(p_http_pipe->_data._data_buffer_length));
			if (ret_val != SUCCESS)
			{
				sd_assert(p_http_pipe->_b_data_full != TRUE);

				p_http_pipe->_b_data_full = TRUE;
				pipe_set_err_get_buffer(&(p_http_pipe->_data_pipe), TRUE);
				p_http_pipe->_get_buffer_try_count = 0;
				/* Start timer */
				LOG_DEBUG("  http_pipe[0x%X] call START_retry_get_buffer_timer_TIMER", p_http_pipe);
				ret_val = start_timer(http_pipe_handle_retry_get_buffer_timeout, NOTICE_ONCE,
					http_pipe_get_buffer_wait_timeout(p_http_pipe), 0, p_http_pipe,
					&(p_http_pipe->_retry_get_buffer_timer_id));
				if (ret_val != SUCCESS)  http_pipe_failure_exit(p_http_pipe, ret_val);
#ifdef _DEBUG
				CHECK_VALUE(ret_val);
#endif
				return SUCCESS;
			}

			/* Get buffer OK !*/

			if (p_http_pipe->_data._temp_data_buffer_length != 0)
			{
				ret_val = http_pipe_store_temp_data_buffer(p_http_pipe);
				if (ret_val == SUCCESS)
				{
					ret_val = http_pipe_parse_file_content(p_http_pipe);
				}

				if (ret_val != SUCCESS)
				{
					http_pipe_failure_exit(p_http_pipe, ret_val);
					return SUCCESS;
				}

			}

			/* Do the next */

			LOG_DEBUG("  http_pipe[0x%X] _b_data_full=%d,_data_length=%u,_data_buffer_end_pos=%u",
				p_http_pipe, p_http_pipe->_b_data_full, p_http_pipe->_data._data_length, p_http_pipe->_data._data_buffer_end_pos);
			http_pipe_do_next(p_http_pipe);

		}
		else
		{
			LOG_DEBUG(" p_http_pipe=0x%X HTTP_ERR_WRONG_TIME_OUT", p_http_pipe);
#ifdef _DEBUG
			CHECK_VALUE(1);
#endif
		}

	}
	else if (errcode == MSG_CANCELLED)
	{
		LOG_DEBUG(" MSG_CANCELLED:_retry_connect_timer_id=%u,_retry_get_buffer_timer_id=%u,"
			"_retry_set_file_size_timer_id=%u,_wait_delete=%d",
			p_http_pipe->_retry_connect_timer_id, p_http_pipe->_retry_get_buffer_timer_id,
			p_http_pipe->_retry_set_file_size_timer_id, p_http_pipe->_wait_delete);

		if (p_http_pipe->_retry_get_buffer_timer_id == msgid)
			p_http_pipe->_retry_get_buffer_timer_id = 0;

		if (p_http_pipe->_wait_delete == TRUE)
		{
			http_pipe_close((DATA_PIPE *)p_http_pipe);
			return SUCCESS;
		}
	}
	else
	{
		LOG_DEBUG(" p_http_pipe=0x%X HTTP_ERR_WRONG_TIME_OUT,errcode=%d", p_http_pipe, errcode);
#ifdef _DEBUG
		CHECK_VALUE(1);
#endif
	}

	return SUCCESS;
} /* End of http_pipe_handle_timeout */

_int32 http_pipe_handle_retry_set_file_size_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid)
{
	HTTP_DATA_PIPE * p_http_pipe = (HTTP_DATA_PIPE *)(msg_info->_user_data);
	_int32 ret_val = SUCCESS;
	_u64 file_size_from_data_manager = 0;

	LOG_DEBUG(" enter[0x%X] http_pipe_handle_retry_set_file_size_timeout(errcode=%d,left=%u,expired=%u,msgid=%u)...",
		p_http_pipe, errcode, notice_count_left, expired, msgid);

#ifdef _DEBUG
	sd_assert(p_http_pipe != NULL);
#endif

	if (!p_http_pipe) return HTTP_ERR_INVALID_PIPE;

	LOG_DEBUG(" http_pipe[0x%X] _http_state=%d,_retry_connect_timer_id=%u,_b_data_full=%d,"
		"_retry_get_buffer_timer_id=%u,_error_code=%d,"
		"_b_retry_set_file_size=%d,_retry_set_file_size_timer_id=%d)...",
		p_http_pipe, HTTP_GET_STATE, p_http_pipe->_retry_connect_timer_id, p_http_pipe->_b_data_full,
		p_http_pipe->_retry_get_buffer_timer_id, p_http_pipe->_error_code,
		p_http_pipe->_b_retry_set_file_size, p_http_pipe->_retry_set_file_size_timer_id);

	if (errcode == MSG_TIMEOUT)
	{
		if ((p_http_pipe->_b_retry_set_file_size == TRUE) && (msgid == p_http_pipe->_retry_set_file_size_timer_id))
		{

			LOG_DEBUG(" http_pipe[0x%X] _retry_set_file_size_timer_id =%u",
				p_http_pipe, p_http_pipe->_retry_set_file_size_timer_id);
			/* retry set file time out */
			p_http_pipe->_retry_set_file_size_timer_id = 0;
			p_http_pipe->_b_retry_set_file_size = FALSE;

			if ((p_http_pipe->_wait_delete == TRUE) ||
				((p_http_pipe->_http_state != HTTP_PIPE_STATE_READING) &&
				(p_http_pipe->_http_state != HTTP_PIPE_STATE_DOWNLOAD_COMPLETED)))
			{
				ret_val = HTTP_ERR_WRONG_TIME_OUT;
#ifdef _DEBUG
				CHECK_VALUE(ret_val);
#endif
				http_pipe_failure_exit(p_http_pipe, ret_val);
				return SUCCESS;
			}

			file_size_from_data_manager = pi_get_file_size((DATA_PIPE *)p_http_pipe);
			LOG_DEBUG(" http_pipe[0x%X] _retry_set_file_size_timer_id =%u,"
				"_file_size_from_data_manager=%llu,server_resource->_file_size=%llu",
				p_http_pipe, p_http_pipe->_retry_set_file_size_timer_id,
				file_size_from_data_manager, HTTP_RESOURCE_GET_FILE_SIZE(p_http_pipe->_p_http_server_resource));
			/* Get and set the file size */
			if (file_size_from_data_manager != HTTP_RESOURCE_GET_FILE_SIZE(p_http_pipe->_p_http_server_resource))
			{
				/* Check if the file size is ok */
				if ((file_size_from_data_manager > 10 * 1024) && (HTTP_RESOURCE_GET_FILE_SIZE(p_http_pipe->_p_http_server_resource) < 1024))
				{
					LOG_DEBUG(" http_pipe[0x%X] the file size=%llu from origin resource is too small !"
						"file_size_from_data_manager=%llu,UNACCEPTABLE !!\n",
						p_http_pipe, HTTP_RESOURCE_GET_FILE_SIZE(p_http_pipe->_p_http_server_resource), file_size_from_data_manager);
					ret_val = HTTP_ERR_INVALID_SERVER_FILE_SIZE;
				}
				else
				{
					LOG_DEBUG(" http_pipe[0x%X] call dm_set_file_size", p_http_pipe);
					ret_val = pi_set_file_size((DATA_PIPE *)p_http_pipe, HTTP_RESOURCE_GET_FILE_SIZE(p_http_pipe->_p_http_server_resource));
					if (ret_val != SUCCESS)
					{
						if (ret_val == FILE_SIZE_NOT_BELIEVE)
						{
							/* Start timer to wait dm_set_file_size */
							p_http_pipe->_b_retry_set_file_size = TRUE;
							/* Start timer */
							LOG_DEBUG(" http_pipe[0x%X] call START_TIMER to wait dm_set_file_size", p_http_pipe);
							ret_val = start_timer(http_pipe_handle_retry_set_file_size_timeout, NOTICE_ONCE, SET_FILE_SIZE_WAIT_M_SEC, 0,
								p_http_pipe, &(p_http_pipe->_retry_set_file_size_timer_id));
							if (ret_val == SUCCESS) return ret_val;
						}
						else
						{
							LOG_DEBUG(" http_pipe[0x%X] HTTP_ERR_INVALID_SERVER_FILE_SIZE", p_http_pipe);
							ret_val = HTTP_ERR_INVALID_SERVER_FILE_SIZE;
						}

					}
					else
					{
						if ((p_http_pipe->_b_get_file_size_after_data == TRUE) && (p_http_pipe->_b_set_request == FALSE))
						{
							pt_get_file_size_after_data((TASK *)dp_get_task_ptr((DATA_PIPE *)p_http_pipe));
						}
					}
				}

				if (ret_val != SUCCESS)
				{
					http_pipe_failure_exit(p_http_pipe, ret_val);
					return SUCCESS;
				}

			}

			/* Set the _data_pipe._dispatch_info._can_download_ranges ! */
			ret_val = http_pipe_set_can_download_ranges(p_http_pipe, HTTP_RESOURCE_GET_FILE_SIZE(p_http_pipe->_p_http_server_resource));

			if ((ret_val == SUCCESS) && (p_http_pipe->_http_state != HTTP_PIPE_STATE_DOWNLOAD_COMPLETED))
			{
				/* Parse body... */
				if (p_http_pipe->_b_header_received == TRUE)
				{
					ret_val = http_pipe_parse_body(p_http_pipe);
				}
			}

			if (ret_val == SUCCESS)
			{
				http_pipe_do_next(p_http_pipe);
			}

			//#ifdef _DEBUG
			//          CHECK_VALUE(ret_val);
			//#endif
			if (ret_val != SUCCESS) http_pipe_failure_exit(p_http_pipe, ret_val);

			return SUCCESS;

		}
		else
		{
			LOG_DEBUG(" p_http_pipe=0x%X HTTP_ERR_WRONG_TIME_OUT", p_http_pipe);
#ifdef _DEBUG
			CHECK_VALUE(1);
#endif
		}

	}
	else if (errcode == MSG_CANCELLED)
	{
		LOG_DEBUG(" MSG_CANCELLED:_retry_connect_timer_id=%u,_retry_get_buffer_timer_id=%u,"
			"_retry_set_file_size_timer_id=%u,_wait_delete=%d",
			p_http_pipe->_retry_connect_timer_id, p_http_pipe->_retry_get_buffer_timer_id,
			p_http_pipe->_retry_set_file_size_timer_id, p_http_pipe->_wait_delete);
		if (p_http_pipe->_retry_set_file_size_timer_id == msgid)
			p_http_pipe->_retry_set_file_size_timer_id = 0;

		if (p_http_pipe->_wait_delete == TRUE)
		{
			http_pipe_close((DATA_PIPE *)p_http_pipe);
			return SUCCESS;
		}
	}
	else
	{
		LOG_DEBUG(" p_http_pipe=0x%X HTTP_ERR_WRONG_TIME_OUT,errcode=%d", p_http_pipe, errcode);
#ifdef _DEBUG
		CHECK_VALUE(1);
#endif
	}

	return SUCCESS;
} /* End of http_pipe_handle_timeout */



/*--------------------------------------------------------------------------*/
/*                Internal functions                                       */
/*--------------------------------------------------------------------------*/



/*****************************************************************************
* Interface name : http_pipe_send_request
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
_int32 http_pipe_force_put_data(HTTP_DATA_PIPE * p_http_pipe)
{
	_int32 ret_val = SUCCESS;
	if (p_http_pipe->_data_pipe._id != 0)
	{
		RANGE download_range;
		ret_val = dp_get_download_range(&(p_http_pipe->_data_pipe), &download_range);
		CHECK_VALUE(ret_val);
		download_range._num = 1;
		if ((p_http_pipe->_data._data_buffer != NULL) && (p_http_pipe->_data._data_buffer_end_pos != 0))
		{
			LOG_ERROR(" enter[0x%X] http_pipe_force_put_data:%u", p_http_pipe, p_http_pipe->_data._data_buffer_end_pos);
			pi_put_data((DATA_PIPE *)p_http_pipe, &download_range,
				&(p_http_pipe->_data._data_buffer), p_http_pipe->_data._data_buffer_end_pos,
				p_http_pipe->_data._data_buffer_length, p_http_pipe->_data_pipe._p_resource);

		}
		else if ((p_http_pipe->_data._temp_data_buffer != NULL) && (p_http_pipe->_data._temp_data_buffer_length != 0))
		{
			CHECK_VALUE(HTTP_ERR_BAD_TEMP_BUFFER);

		}
	}
	return SUCCESS;
}



_int32 http_pipe_failure_exit(HTTP_DATA_PIPE * p_http_pipe, _int32 err_code)
{
	LOG_ERROR(" enter[0x%X] http_pipe_failure_exit(pipe state = %d,_http_state = %d,error code = %d )...",
		p_http_pipe, HTTP_GET_PIPE_STATE, HTTP_GET_STATE, err_code);

	/* halt the program for debug */
	//CHECK_VALUE(_p_http_pipe->_error_code);
#ifdef _DEBUG
	if (HTTP_RESOURCE_IS_ORIGIN(p_http_pipe->_p_http_server_resource) == TRUE)
	{
		LOG_ERROR("+++ [_p_http_pipe=0x%X]The origin resource:%s, ERROR=%d !! ",
			p_http_pipe, p_http_pipe->_p_http_server_resource->_o_url, err_code);
	}
	else
	{
		LOG_ERROR("+++ [_p_http_pipe=0x%X]The inquire resource:%s, ERROR=%d !!",
			p_http_pipe, p_http_pipe->_p_http_server_resource->_o_url, err_code);
	}
#endif
	p_http_pipe->_error_code = err_code;

	/* Set resource error code */
	set_resource_err_code((RESOURCE*)(p_http_pipe->_p_http_server_resource), (_u32)err_code);

	if ((err_code == HTTP_ERR_INVALID_SERVER_FILE_SIZE)
		|| (err_code == HTTP_ERR_REDIRECT_TO_FTP)
		|| (err_code == HTTP_ERR_RELATION_RES_NOT_SUPPORT_RANGE))
	{
#ifdef _DEBUG
		if (HTTP_RESOURCE_IS_ORIGIN(p_http_pipe->_p_http_server_resource) == TRUE)
		{
			LOG_DEBUG("+++ [_p_http_pipe=0x%X]The origin resource:%s is abandoned !! ",
				p_http_pipe, p_http_pipe->_p_http_server_resource->_o_url);
		}
		else
		{
			LOG_DEBUG("+++ [_p_http_pipe=0x%X]The inquire resource:%s is abandoned !!",
				p_http_pipe, p_http_pipe->_p_http_server_resource->_o_url);
		}
#endif
		set_resource_state(p_http_pipe->_data_pipe._p_resource, ABANDON);
	}

	if (HTTP_RESOURCE_IS_ORIGIN(p_http_pipe->_p_http_server_resource) == TRUE)
	{
		if (p_http_pipe->_data_pipe._p_resource->_retry_times + 1 >= p_http_pipe->_data_pipe._p_resource->_max_retry_time)
		{
			//LOG_ERROR( "oooo [_p_http_pipe=0x%X]The origin resource:%s, ERROR=%d ,retry_times=%u,max_retry_time=%u!! ",
			//	p_http_pipe,p_http_pipe->_p_http_server_resource->_o_url,err_code,p_http_pipe->_data_pipe._p_resource->_retry_times, 
			//	p_http_pipe->_data_pipe._p_resource->_max_retry_time);
			//pt_set_origin_mode((TASK *) dp_get_task_ptr( (DATA_PIPE *)p_http_pipe ),FALSE);
		}
	}

	http_pipe_force_put_data(p_http_pipe);

	HTTP_SET_PIPE_STATE(PIPE_FAILURE);
	HTTP_SET_STATE(HTTP_PIPE_STATE_UNKNOW);

	return SUCCESS;

} /* End of http_pipe_failure_exit */

_int32 http_pipe_handle_recv_header(HTTP_DATA_PIPE * p_http_pipe, _u32 had_recv)
{
	p_http_pipe->_header._header_buffer_end_pos += had_recv;
	p_http_pipe->_sum_recv_bytes += had_recv;

	/* Reading success,start parse the http header and data */
	return http_pipe_parse_response(p_http_pipe);
}

_int32 http_pipe_handle_recv_chunked(HTTP_DATA_PIPE * p_http_pipe, _u32 had_recv)
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG("  http_pipe[0x%X]http_pipe_handle_recv_chunked(_state=%d)! ", p_http_pipe, p_http_pipe->_data.p_chunked->_state);
	p_http_pipe->_sum_recv_bytes += had_recv;

	if (p_http_pipe->_data.p_chunked->_state == HTTP_CHUNKED_STATE_RECEIVE_HEAD)
	{
		if (had_recv == 0)
			CHECK_VALUE(HTTP_ERR_CHUNKED_HEAD_NO_BUFFER);

		p_http_pipe->_data.p_chunked->_head_buffer_end_pos += had_recv;
		ret_val = http_pipe_parse_chunked_header(p_http_pipe, p_http_pipe->_data.p_chunked->_head_buffer,
			p_http_pipe->_data.p_chunked->_head_buffer_end_pos);
		if ((ret_val == SUCCESS) && ((p_http_pipe->_data.p_chunked->_state == HTTP_CHUNKED_STATE_END) ||
			((p_http_pipe->_data._data_buffer_end_pos != 0) && (p_http_pipe->_data._data_buffer_end_pos == p_http_pipe->_data._data_length))))
		{
			ret_val = http_pipe_parse_chunked_data(p_http_pipe);
		}
	}
	else if (p_http_pipe->_data.p_chunked->_state == HTTP_CHUNKED_STATE_RECEIVE_DATA)
	{
		if (had_recv != 0)
		{
			p_http_pipe->_data._data_buffer_end_pos += had_recv;
			p_http_pipe->_data._recv_end_pos += had_recv;
			p_http_pipe->_data.p_chunked->_pack_received_len += had_recv;

			add_speed_record(&(p_http_pipe->_speed_calculator), had_recv);
		}

		LOG_DEBUG("  http_pipe[0x%X]_connected_time=%u,_speed=%u", p_http_pipe, HTTP_TIME_STAMP, HTTP_SPEED);
		/* Reading success,start parse the http header and data */
		ret_val = http_pipe_parse_response(p_http_pipe);
	}
	else
	{
		CHECK_VALUE(HTTP_ERR_UNKNOWN);
	}

	return ret_val;
}
_int32 http_pipe_handle_recv_body(HTTP_DATA_PIPE * p_http_pipe, _u32 had_recv)
{
	p_http_pipe->_data._data_buffer_end_pos += had_recv;
	p_http_pipe->_data._recv_end_pos += had_recv;
	p_http_pipe->_sum_recv_bytes += had_recv;

	add_speed_record(&(p_http_pipe->_speed_calculator), had_recv);

	LOG_DEBUG("  http_pipe[0x%X]_connected_time=%u,_speed=%u", p_http_pipe, HTTP_TIME_STAMP, HTTP_SPEED);

	/* Reading success,start parse the http header and data */
	return http_pipe_parse_response(p_http_pipe);
}
_int32 http_pipe_handle_recv_0_byte(HTTP_DATA_PIPE * p_http_pipe)
{
	LOG_DEBUG("  http_pipe[0x%X] received 0 byte meaning remote server closed the conection!", p_http_pipe);
	//p_http_pipe->_is_connected = FALSE;
	//HTTP_TIME_STAMP = 0;   ///????

	LOG_DEBUG("  http_pipe[0x%X] _b_header_received=%d, _is_chunked=%d,_has_content_length=%d",
		p_http_pipe, p_http_pipe->_b_header_received, p_http_pipe->_header._is_chunked, p_http_pipe->_header._has_content_length);

	if (p_http_pipe->_header._is_chunked == TRUE)
	{
		p_http_pipe->_data._b_received_0_byte = TRUE;
		return http_pipe_handle_recv_chunked(p_http_pipe, 0);
	}
	else if ((p_http_pipe->_b_header_received == TRUE)
		&& (p_http_pipe->_header._is_chunked == FALSE)
		&& (p_http_pipe->_header._has_content_length == FALSE))
	{
		p_http_pipe->_data._b_received_0_byte = TRUE;
		/* Reading success,start parse the http header and data */
		return http_pipe_parse_response(p_http_pipe);
	}
	else
	{
		/* Error */
		if (dp_get_uncomplete_ranges_list_size(&(p_http_pipe->_data_pipe)) != 0)
		{
			LOG_DEBUG("  http_pipe[0x%X] There are still some ranges have not been downloaded yet,need connect again!", p_http_pipe);
			if (p_http_pipe->_b_ranges_changed != TRUE)
				p_http_pipe->_b_ranges_changed = TRUE;

			if (p_http_pipe->_is_connected)
			{
				socket_proxy_close(p_http_pipe->_socket_fd);
				p_http_pipe->_is_connected = FALSE;
				p_http_pipe->_data_pipe._dispatch_info._time_stamp = 0;   ///????
				//_p_http_pipe->_sum_recv_bytes = 0;
				p_http_pipe->_socket_fd = 0;
			}

			HTTP_SET_PIPE_STATE(PIPE_DOWNLOADING);
			HTTP_SET_STATE(HTTP_PIPE_STATE_RANGE_CHANGED);
			return http_pipe_open((DATA_PIPE *)p_http_pipe);

		}
		else
		{
			return HTTP_ERR_SERVER_DISCONNECTED;
		}
	}

}
_int32 http_pipe_handle_recv_2249(HTTP_DATA_PIPE * p_http_pipe, _u32 had_recv)
{
	if (p_http_pipe->_header._is_chunked == TRUE)
	{
		p_http_pipe->_data._b_received_0_byte = TRUE;
		return http_pipe_handle_recv_chunked(p_http_pipe, had_recv);
	}
	else if ((p_http_pipe->_b_header_received == TRUE) && (p_http_pipe->_header._is_chunked == FALSE))
	{
		LOG_DEBUG("  http_pipe[0x%X](_errcode == 2249)&&(_p_http_pipe->_b_header_received!=FALSE)&&(_p_http_pipe->_header._is_chunked !=TRUE)",
			p_http_pipe);
		if (had_recv != 0)
		{
			p_http_pipe->_data._data_buffer_end_pos += had_recv;
			p_http_pipe->_data._recv_end_pos += had_recv;
			p_http_pipe->_sum_recv_bytes += had_recv;
			//p_http_pipe->_data_pipe._p_resource->_sum_recv_bytes+=had_recv;
			add_speed_record(&(p_http_pipe->_speed_calculator), had_recv);

			LOG_DEBUG("  http_pipe[0x%X]_connected_time=%u,_speed=%u", p_http_pipe, HTTP_TIME_STAMP, HTTP_SPEED);

		}

		p_http_pipe->_data._b_received_0_byte = TRUE;
		/* Reading success,start parse the http header and data */
		return http_pipe_parse_response(p_http_pipe);

	}
	return 2249;
}




_int32 http_pipe_do_next(HTTP_DATA_PIPE * p_http_pipe)
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG(" enter [0x%X] http_pipe_do_next(http_state=%d)...", p_http_pipe, HTTP_GET_STATE);

	if (p_http_pipe->_b_retry_request_not_discnt == TRUE)
	{
		LOG_DEBUG("  http_pipe[0x%X]_b_retry_request... :_is_connected=%d,_b_retry_request=%d,_pipe_state=%d",
			p_http_pipe, p_http_pipe->_is_connected, p_http_pipe->_b_retry_request, HTTP_GET_PIPE_STATE);

		ret_val = http_pipe_send_request(p_http_pipe);

		if (ret_val != SUCCESS)
			http_pipe_failure_exit(p_http_pipe, ret_val);

		p_http_pipe->_b_retry_request_not_discnt = FALSE;
		p_http_pipe->_rev_wap_page = TRUE;
	}
	else if ((HTTP_GET_STATE == HTTP_PIPE_STATE_READING) && (p_http_pipe->_b_retry_set_file_size != TRUE))
	{
		ret_val = http_pipe_continue_reading(p_http_pipe);

		if (ret_val != SUCCESS)
			http_pipe_failure_exit(p_http_pipe, ret_val);
	}
	else if (HTTP_GET_STATE == HTTP_PIPE_STATE_RANGE_CHANGED)
	{
		LOG_DEBUG("  http_pipe[0x%X]HTTP_PIPE_STATE_RANGE_CHANGED:_is_support_long_connect=%d,_is_connected=%d,"
			"_b_redirect=%d,_pipe_state=%d",
			p_http_pipe, HTTP_RESOURCE_IS_SUPPORT_LONG_CONNECT(p_http_pipe->_p_http_server_resource), p_http_pipe->_is_connected,
			p_http_pipe->_b_redirect, HTTP_GET_PIPE_STATE);
		/* download the next range */
		if (HTTP_RESOURCE_IS_SUPPORT_LONG_CONNECT(p_http_pipe->_p_http_server_resource) == TRUE)
		{
			ret_val = http_pipe_send_request(p_http_pipe);

			if (ret_val != SUCCESS)
				http_pipe_failure_exit(p_http_pipe, ret_val);
		}
		else
		{
			/* Connect again */
			http_pipe_reconnect(p_http_pipe);
		}

	}
	else if (HTTP_GET_STATE == HTTP_PIPE_STATE_RANGE_CHANGED_NEED_RECONNECT)
	{
		LOG_DEBUG("  http_pipe[0x%X] HTTP_PIPE_STATE_RANGE_CHANGED_NEED_RECONNECT:_is_connected=%d,_b_redirect=%d,_pipe_state=%d",
			p_http_pipe, p_http_pipe->_is_connected, p_http_pipe->_b_redirect, HTTP_GET_PIPE_STATE);
		/* Connect again */
		http_pipe_reconnect(p_http_pipe);
	}
	else if ((HTTP_GET_STATE == HTTP_PIPE_STATE_IDLE) && (p_http_pipe->_b_retry_request == TRUE))
	{
		LOG_DEBUG("  http_pipe[0x%X]_b_retry_request... :_is_connected=%d,_b_retry_request=%d,_pipe_state=%d",
			p_http_pipe, p_http_pipe->_is_connected, p_http_pipe->_b_retry_request, HTTP_GET_PIPE_STATE);

		if (p_http_pipe->_request_step == UCS_END)
		{
			LOG_DEBUG("  http_pipe[0x%X] Already retry all the case ,and failed!", p_http_pipe);
			ret_val = HTTP_ERR_FILE_NOT_FOUND;
			http_pipe_failure_exit(p_http_pipe, ret_val);
		}

		/* Connect again */
		http_pipe_reconnect(p_http_pipe);
	}
	else if ((HTTP_GET_STATE == HTTP_PIPE_STATE_IDLE) && (p_http_pipe->_b_redirect == TRUE))
	{
		LOG_DEBUG("  http_pipe[0x%X] Redirecting... :_is_connected=%d,_b_redirect=%d,_pipe_state=%d",
			p_http_pipe, p_http_pipe->_is_connected, p_http_pipe->_b_redirect, HTTP_GET_PIPE_STATE);
		/* redirecting... */
		/* Connect again */
		http_pipe_reconnect(p_http_pipe);
	}
	else if ((HTTP_GET_STATE == HTTP_PIPE_STATE_DOWNLOAD_COMPLETED)
		&& (p_http_pipe->_b_data_full != TRUE)
		&& (p_http_pipe->_b_retry_set_file_size != TRUE))
	{
		LOG_DEBUG("  http_pipe[0x%X] HTTP_PIPE_STATE_DOWNLOAD_COMPLETED:dm_notify_dispatch_data_finish;_is_connected=%d,"
			"_b_redirect=%d,_pipe_state=%d",
			p_http_pipe, p_http_pipe->_is_connected,
			p_http_pipe->_b_redirect, HTTP_GET_PIPE_STATE);
		/* notify the data manager */
		pi_notify_dispatch_data_finish((DATA_PIPE *)p_http_pipe);
	}

	return SUCCESS;

} /* End of http_pipe_failure_exit */

_int32 http_pipe_reconnect(HTTP_DATA_PIPE * p_http_pipe)
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG(" enter[0x%X] http_pipe_reconnect()...", p_http_pipe);

	if (p_http_pipe->_is_connected == TRUE)
	{
		http_pipe_close_connection(p_http_pipe);
	}
	else if (((dp_get_uncomplete_ranges_list_size(&(p_http_pipe->_data_pipe)) != 0) || (p_http_pipe->_b_redirect == TRUE))
		&& (HTTP_GET_PIPE_STATE != PIPE_FAILURE))
	{
		ret_val = http_pipe_do_connect(p_http_pipe);

		if (ret_val != SUCCESS)
			http_pipe_failure_exit(p_http_pipe, ret_val);

	}
	else
	{
		LOG_DEBUG("  http_pipe[0x%X]HTTP:need reconnect the scoket...HTTP_ERR_UNKNOWN", p_http_pipe);
#ifdef _DEBUG
		CHECK_VALUE(1);
#endif
		ret_val = HTTP_ERR_UNKNOWN;
		http_pipe_failure_exit(p_http_pipe, ret_val);
	}
	return SUCCESS;

} /* End of http_pipe_failure_exit */

_int32 http_pipe_do_connect(HTTP_DATA_PIPE * p_http_pipe)
{
	_int32 ret_val = SUCCESS;
	URL_OBJECT  *p_url_o = NULL;
	LOG_DEBUG(" enter[0x%X] http_pipe_do_connect()...", p_http_pipe);
	p_url_o = http_pipe_get_url_object(p_http_pipe);
	if (p_url_o == NULL)
	{
		ret_val = HTTP_ERR_INVALID_URL;
		return ret_val;
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
			return HTTP_ERR_SSL_CREATE;
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
				LOG_ERROR("pipe(0x%X): bio do connect(%s) 1 failed",
					p_http_pipe, server);
				ret_val = HTTP_ERR_SSL_CONNECT;
				BIO_FREE_ALL(p_http_pipe->_pbio);
				return ret_val;
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
				return ret_val;
			}
			return http_pipe_handle_connect(0, 0, p_http_pipe);
		}

		ret_val = BIO_get_fd(p_http_pipe->_pbio, &p_http_pipe->_socket_fd);
		if (ret_val <= 0)
		{
			LOG_ERROR("pipe(0x%X): 2 bio get fd filed", p_http_pipe);
			ret_val = HTTP_ERR_SSL_CONNECT;
			BIO_FREE_ALL(p_http_pipe->_pbio);
			return ret_val;
		}
		ret_val = SUCCESS;
		LOG_DEBUG("pipe(0x%X): create ssl connection with fd:%d",
			p_http_pipe, p_http_pipe->_socket_fd);
	}
	else
#endif
	if (p_http_pipe->_data_pipe._id != 0)
	{
		ret_val = socket_proxy_create_browser(&(p_http_pipe->_socket_fd), SD_SOCK_STREAM);
	}
	else
		ret_val = socket_proxy_create(&(p_http_pipe->_socket_fd), SD_SOCK_STREAM);
	if ((ret_val != SUCCESS) || (p_http_pipe->_socket_fd == 0))
	{
		/* Create socket error ! */
		return ret_val;
	}

	LOG_DEBUG("http_pipe_do_connect p_http_pipe->_socket_fd = %u",
		p_http_pipe->_socket_fd);
	HTTP_SET_PIPE_STATE(PIPE_CONNECTING);
	HTTP_SET_STATE(HTTP_PIPE_STATE_CONNECTING);


	LOG_DEBUG("call SOCKET_PROXY_CONNECT(fd=%u,_host=%s,port=%u)",
		p_http_pipe->_socket_fd, p_url_o->_host, p_url_o->_port);
#ifdef _ENABLE_SSL
	if (p_http_pipe->_is_https)
	{
		return socket_proxy_connect_ssl(p_http_pipe->_socket_fd, p_http_pipe->_pbio,
			http_pipe_handle_connect, p_http_pipe);
	}
	else
#endif
	{
		return socket_proxy_connect_by_domain(p_http_pipe->_socket_fd, p_url_o->_host, p_url_o->_port,
			http_pipe_handle_connect, (void*)p_http_pipe);
	}

} /* End of http_pipe_failure_exit */


_int32 http_pipe_close_connection(HTTP_DATA_PIPE * p_http_pipe)
{
	_u32 _op_count = 0;
	_int32 ret_val = SUCCESS;
	LOG_DEBUG(" enter[0x%X] http_pipe_close_connection(_is_connected=%d,_http_state=%d)...",
		p_http_pipe, p_http_pipe->_is_connected, HTTP_GET_STATE);

	if ((p_http_pipe->_is_connected == TRUE)
		|| (HTTP_GET_STATE == HTTP_PIPE_STATE_CONNECTING)
		|| (HTTP_GET_STATE == HTTP_PIPE_STATE_CLOSING))
	{

		ret_val = socket_proxy_peek_op_count(p_http_pipe->_socket_fd, DEVICE_SOCKET_TCP, &_op_count);
		if (ret_val != SUCCESS)  goto ErrorHanle;

		/* Close the connection */
		if (_op_count != 0)
		{
			HTTP_SET_STATE(HTTP_PIPE_STATE_CLOSING);
			LOG_DEBUG("p_http_pipe=0x%X call SOCKET_PROXY_CANCEL(fd=%u)",
				p_http_pipe, p_http_pipe->_socket_fd);
			ret_val = socket_proxy_cancel(p_http_pipe->_socket_fd, DEVICE_SOCKET_TCP);
			if (ret_val != SUCCESS)    goto ErrorHanle;

			return SUCCESS;

		}
		else
		{

			HTTP_SET_STATE(HTTP_PIPE_STATE_CLOSING);
			LOG_DEBUG("p_http_pipe=0x%X call SOCKET_PROXY_CLOSE(fd=%u)",
				p_http_pipe, p_http_pipe->_socket_fd);
#ifdef _ENABLE_SSL
			if (p_http_pipe->_is_https)
			{
				BIO_FREE_ALL(p_http_pipe->_pbio);
			}
			else
#endif
			{
				ret_val = socket_proxy_close(p_http_pipe->_socket_fd);
				if (ret_val != SUCCESS)   goto ErrorHanle;
			}

		}
	}

	LOG_DEBUG("ok:_b_redirect=%d,_pipe_state=%d,_wait_delete=%d",
		p_http_pipe->_b_redirect, p_http_pipe->_data_pipe._dispatch_info._pipe_state, p_http_pipe->_wait_delete);
	p_http_pipe->_is_connected = FALSE;
	p_http_pipe->_data_pipe._dispatch_info._time_stamp = 0;   ///????
	//_p_http_pipe->_sum_recv_bytes = 0;
	p_http_pipe->_socket_fd = 0;
	HTTP_SET_STATE(HTTP_PIPE_STATE_IDLE);

	if (((dp_get_uncomplete_ranges_list_size(&(p_http_pipe->_data_pipe)) != 0)
		|| (p_http_pipe->_b_redirect == TRUE)
		|| (p_http_pipe->_b_retry_request == TRUE))
		&& (HTTP_GET_PIPE_STATE != PIPE_FAILURE) && (p_http_pipe->_wait_delete != TRUE))
	{
		return http_pipe_do_connect(p_http_pipe);
	}
	else if (p_http_pipe->_wait_delete == TRUE)
	{
		http_pipe_destroy(p_http_pipe);
	}



	return SUCCESS;

ErrorHanle:
	LOG_DEBUG("ErrorHanle:_wait_delete=%d,_error_code=%d",
		p_http_pipe->_wait_delete, p_http_pipe->_error_code);
	if (p_http_pipe->_wait_delete == TRUE)
	{
		http_pipe_destroy(p_http_pipe);  ////??????
		return SUCCESS;
	}
	else
	{
		http_pipe_failure_exit(p_http_pipe, ret_val);
		return ret_val;
	}


} /* End of http_pipe_close */



_int32 http_pipe_send_request(HTTP_DATA_PIPE * p_http_pipe)
{

	char full_path_buffer[MAX_URL_LEN], *full_path = NULL;
	URL_OBJECT  *p_url_o = NULL;
	_u64 _start_pos, _end_pos = 0;
	BOOL _need_send_0_range = FALSE, is_mini_http = FALSE;
	_int32 _full_path_len = 0, length = 0, ret_val = SUCCESS;
	char * cookies = NULL;
	_u32 post_data_len = 0;
	_u8 * post_data = NULL;


	LOG_DEBUG(" enter[0x%X] http_pipe_send_request(%d)...", p_http_pipe, p_http_pipe->_b_redirect);

	if (p_http_pipe->_b_set_request == FALSE)
	{
		is_mini_http = (p_http_pipe->_data_pipe._id != 0) ? TRUE : FALSE;
		/*--------------- Get ----------------------*/
		p_url_o = http_pipe_get_url_object(p_http_pipe);
        LOG_DEBUG("enter[0x%X] p_url_o->_path_valid_step = %d"
            , p_http_pipe,  p_url_o->_path_valid_step );
		if (p_http_pipe->_b_retry_request == TRUE)
		{
			///
			p_http_pipe->_b_retry_request = FALSE;
			ret_val = url_convert_format(p_url_o->_full_path, sd_strlen(p_url_o->_full_path), full_path_buffer,
				MAX_URL_LEN, &p_http_pipe->_request_step);
			//ret_val = http_pipe_convert_full_paths( p_http_pipe,full_path_buffer ,MAX_URL_LEN );
			if (ret_val != SUCCESS)
			{
				if (0 == http_pipe_get_download_range_index(p_http_pipe))
				{
					// send the request with "Range: bytes=0-"
					_need_send_0_range = TRUE;
					p_http_pipe->_b_retry_request = FALSE;
					p_http_pipe->_request_step = UCS_END;
					LOG_DEBUG("retry request step=UCS_END");
					_full_path_len = sd_strlen(p_url_o->_full_path);
					if (_full_path_len >= MAX_FULL_PATH_BUFFER_LEN)
						CHECK_VALUE(HTTP_ERR_FULL_PATH_TOO_LONG);

					full_path = p_url_o->_full_path;
					ret_val = SUCCESS;
				}
				else
					return  ret_val;
			}
			else
			{
				_full_path_len = sd_strlen(full_path_buffer);
				if (_full_path_len >= MAX_FULL_PATH_BUFFER_LEN)
					return  HTTP_ERR_FULL_PATH_TOO_LONG;

				full_path = full_path_buffer;
			}

		}
		else if (p_http_pipe->_b_redirect == TRUE)
		{
			_full_path_len = sd_strlen(p_url_o->_full_path);
			if (_full_path_len >= MAX_FULL_PATH_BUFFER_LEN)
				CHECK_VALUE(HTTP_ERR_FULL_PATH_TOO_LONG);

			full_path = p_url_o->_full_path;
		}
		else
		{
			if ((p_url_o->_path_valid_step != UCS_ORIGIN) && (p_url_o->_path_valid_step != UCS_END))
			{
				/* Convert the path format */
				ret_val = url_convert_format_to_step(p_url_o->_full_path, sd_strlen(p_url_o->_full_path),
					full_path_buffer, MAX_URL_LEN, p_url_o->_path_valid_step);
				CHECK_VALUE(ret_val);
				_full_path_len = sd_strlen(full_path_buffer);
				if (_full_path_len >= MAX_FULL_PATH_BUFFER_LEN)
					return HTTP_ERR_FULL_PATH_TOO_LONG;

				full_path = full_path_buffer;
			}
			else
			{
				if (p_http_pipe->_request_step == UCS_END)
					_need_send_0_range = TRUE;

				_full_path_len = sd_strlen(p_url_o->_full_path);
				if (_full_path_len >= MAX_FULL_PATH_BUFFER_LEN)
					CHECK_VALUE(HTTP_ERR_FULL_PATH_TOO_LONG);

				full_path = p_url_o->_full_path;
			}
		}
		//HTTP_TRACE( " full_path=%s" ,full_path);


		/*--------------- Range ----------------------*/
		/* Get the task of current request */
		ret_val = http_pipe_get_request_range(p_http_pipe, &_start_pos, &_end_pos);
		if (ret_val != SUCCESS) return ret_val;
		/* Check if need the send the "Range: bytes=" of this request */
		if ((_start_pos == 0) && (_end_pos == -1) && (_need_send_0_range == FALSE))
		{
			LOG_DEBUG(" _p_http_pipe=0x%X: Do not need to send the :[ Range: bytes=]", p_http_pipe);
			_start_pos = -1;
		}


#ifdef ENABLE_COOKIES
		cookies = http_resource_get_cookies(p_http_pipe->_p_http_server_resource);
#endif

#if 0 //def ENABLE_COOKIES
		/*--------------- Cookies ----------------------*/
		std::vector<std::string>::iterator it = _cookies.begin();
		while( it != _cookies.end() )
		{
			std::string cookie = "Cookie: ";
			cookie += *it;
			cookie += "\r\n";
			request_header += cookie;
			it++;
		}
		if (_server_resource_ptr != NULL)
		{
			std::string private_cookie = _server_resource_ptr->get_private_cookie();
			if (private_cookie != "")
			{
				std::string cookies = "Cookie: ";
				cookies += private_cookie;
				cookies += "\r\n";
				request_header += cookies;
			}
			else
			{
				std::string system_cookie;
				if (_server_resource_ptr->is_set_system_cookie())
					system_cookie = _server_resource_ptr->get_system_cookie();
				else
				{
					system_cookie = cookie_from_system(_url.to_string());
					_server_resource_ptr->set_system_cookie(system_cookie);
				}
				if( system_cookie != "" )
				{
					std::string cookie = "Cookie: ";
					cookie += system_cookie;
					cookie += "\r\n";
					request_header += cookie;
				}
			}
			std::string last_modified = _server_resource_ptr->get_lastmodified_to_server();
			if (last_modified != "")
			{
				request_header += "If-Modified-Since: ";
				request_header += last_modified;
				request_header += "\r\n";
			}
		}
#endif

		if (is_mini_http)
		{
			ret_val = mini_http_get_post_data((DATA_PIPE*)p_http_pipe, &post_data, &post_data_len);
			CHECK_VALUE(ret_val);
		}

		ret_val = http_create_request(&(p_http_pipe->_http_request), &(p_http_pipe->_http_request_buffer_len),
			full_path, http_resource_get_ref_url(p_http_pipe->_p_http_server_resource), p_url_o->_host, p_url_o->_port,
			p_url_o->_user, p_url_o->_password, cookies, _start_pos, _end_pos, is_mini_http,
			p_http_pipe->_p_http_server_resource->_is_post, post_data, post_data_len,
			&p_http_pipe->_http_request_len, http_resource_get_gzip(p_http_pipe->_p_http_server_resource),
			http_resource_get_send_gzip(p_http_pipe->_p_http_server_resource),
			http_resource_get_post_content_len(p_http_pipe->_p_http_server_resource));
		CHECK_VALUE(ret_val);
	}
	else
	{
		if (p_http_pipe->_b_retry_request == TRUE)
			return 400;
		p_http_pipe->_request_step = UCS_END;
		/* Get the task of current request */
		ret_val = http_pipe_get_request_range(p_http_pipe, &_start_pos, &_end_pos);
		if (ret_val != SUCCESS) return ret_val;
	}
	sd_assert(p_http_pipe->_http_request != NULL);
	sd_assert(p_http_pipe->_http_request_buffer_len != 0);

	if (p_http_pipe->_http_request_len == 0)
	{
		p_http_pipe->_http_request_len = sd_strlen(p_http_pipe->_http_request);
	}
	length = p_http_pipe->_http_request_len;
	LOG_DEBUG("+++ _p_http_pipe=0x%X:Sending[%d]:%s", p_http_pipe, length, p_http_pipe->_http_request);
	if (length > 400)
	{
		LOG_DEBUG("+++[%d]:%s", length - 400, p_http_pipe->_http_request + 400);
	}

	HTTP_SET_STATE(HTTP_PIPE_STATE_REQUESTING);
#ifdef _ENABLE_SSL
	if (p_http_pipe->_is_https)
	{
		ret_val = socket_proxy_send_ssl(p_http_pipe->_socket_fd, p_http_pipe->_pbio,
			p_http_pipe->_http_request, length, http_pipe_handle_send, (void*)p_http_pipe);
	}
	else
#endif
	if (p_http_pipe->_data_pipe._id != 0)  // Browser request
	{
		ret_val = socket_proxy_send_browser(p_http_pipe->_socket_fd, p_http_pipe->_http_request,
			length, http_pipe_handle_send, (void*)p_http_pipe);
	}
	else
	{
		ret_val = socket_proxy_send(p_http_pipe->_socket_fd, p_http_pipe->_http_request, length,
			http_pipe_handle_send, (void*)p_http_pipe);
	}
	CHECK_VALUE(ret_val);

	sd_time_ms(&(p_http_pipe->_data_pipe._dispatch_info._last_recv_data_time));

	LOG_DEBUG("_p_http_pipe=0x%X:call SOCKET_PROXY_SEND, ret_val=%d", p_http_pipe, ret_val);

	return SUCCESS;


} /* End of http_pipe_send_request */


_int32 http_pipe_parse_response(HTTP_DATA_PIPE * p_http_pipe)
{
	_int32 ret_val = SUCCESS;

	LOG_DEBUG(" enter[0x%X] http_pipe_parse_response()...", p_http_pipe);

	/* Parse header... */
	if (p_http_pipe->_b_header_received == FALSE)
	{
		/* Parse response header */
		ret_val = http_parse_header(&(p_http_pipe->_header));
		if (ret_val != SUCCESS) return ret_val;

		p_http_pipe->_b_header_received = TRUE;
		//开始判断返回信息
		ret_val = http_pipe_parse_status_code(p_http_pipe);
		if (ret_val != SUCCESS)
		{
			/* Just for mini http */
			if (p_http_pipe->_data_pipe._id != 0)
			{
				mini_http_set_header((DATA_PIPE *)p_http_pipe, &(p_http_pipe->_header), ret_val);
			}
			return ret_val;
		}
	}

	/* Parse body... */
	if ((p_http_pipe->_b_retry_set_file_size != TRUE) && (p_http_pipe->_b_header_received))
	{
		return http_pipe_parse_body(p_http_pipe);
	}

	return SUCCESS;

} /* End of http_pipe_parse_response */


_int32 http_pipe_parse_status_code(HTTP_DATA_PIPE * p_http_pipe)
{
	HTTP_RESPN_HEADER * p_header = NULL;
	_u32 code = 0, range_index = 0;
	_int32 ret_val = SUCCESS;
	_u64 file_size = 0;
	URL_OBJECT* p_url_o = NULL;
	LOG_DEBUG("enter[0x%X] http_pipe_parse_status_code:code=%u", 
		p_http_pipe, p_http_pipe->_header._status_code);

	code = p_http_pipe->_header._status_code;
	p_header = &(p_http_pipe->_header);

	if (code != 403 && g_403_count != 0) g_403_count = 0;

	switch (code)
	{
	case 200:
	case 206:
#if defined(MOBILE_PHONE)
		if(NT_GPRS_WAP & sd_get_net_type())
		{
			if(p_header->_is_vnd_wap == TRUE)
			{
				LOG_DEBUG( "1111 http_pipe[0x%X] get code:%u in CMWAP,need request again!",p_http_pipe,code);
				p_http_pipe->_b_header_received = FALSE;
				p_http_pipe->_b_retry_request_not_discnt = TRUE;
				return SUCCESS;
			}
		}
#endif /* MOBILE_PHONE */
		if (p_header->_is_binary_file == FALSE)
		{
			LOG_DEBUG(" http_pipe[0x%X] is a html or txt file !", p_http_pipe);
			if (http_pipe_is_binary_file(p_http_pipe) == TRUE)
			{
				LOG_DEBUG(" http_pipe[0x%X] but it should be a binary file ! !", p_http_pipe);
				/* Bad response */
				if (p_http_pipe->_b_redirect == TRUE)
				{
					/* Need retry sending resuest to the origin url by RRS_DECODE */
					LOG_DEBUG(" http_pipe[0x%X] Need retry sending resuest to the origin url by RRS_DECODE", p_http_pipe);
					//return HTTP_ERR_NEED_RETRY_REQUEST;
					http_reset_respn_header(&(p_http_pipe->_header));
					http_pipe_reset_respn_data(p_http_pipe, &(p_http_pipe->_data));
					p_http_pipe->_b_header_received = FALSE;
					p_http_pipe->_b_redirect = FALSE;
					/* Maybe the path is in wrong format */
					p_url_o = http_pipe_get_url_object(p_http_pipe);
					if ((p_url_o->_path_encode_mode != UEM_ASCII) ||
						(p_url_o->_filename_encode_mode != UEM_ASCII))
					{
						if (p_url_o->_path_valid_step == UCS_ORIGIN)
						{
							p_http_pipe->_b_retry_request = TRUE;
							p_http_pipe->_http_state = HTTP_PIPE_STATE_IDLE;
							LOG_DEBUG("http_pipe[0x%X] ready to retry request..._request_step=%d",
								p_http_pipe, p_http_pipe->_request_step);
							return SUCCESS;
						}
					}
					return HTTP_ERR_FILE_NOT_FOUND;
				}
				else if (p_header->_location_len != 0)
				{
					goto HandleLocation;
				}
			}
		}

		p_url_o = http_pipe_get_url_object(p_http_pipe);
		if ((p_url_o->_path_valid_step == UCS_ORIGIN) &&
			(p_http_pipe->_request_step != UCS_ORIGIN) && (p_http_pipe->_request_step != UCS_END))
		{
			LOG_DEBUG("Get p_http_pipe[0x%X]->_p_http_server_resource->_url._filename_valid_step=%d!",
				p_http_pipe, p_http_pipe->_request_step);
			p_url_o->_path_valid_step = p_http_pipe->_request_step;
			p_url_o->_filename_valid_step = p_http_pipe->_request_step;
		}

		if (p_header->_server_return_name_len != 0 && !p_http_pipe->_p_http_server_resource->_relation_res)
		{
			http_resource_set_server_return_file_name(p_http_pipe->_p_http_server_resource,
				p_header->_server_return_name, p_header->_server_return_name_len);
		}

		if (p_header->_last_modified_len != 0)
		{
			http_resource_set_last_modified(p_http_pipe->_p_http_server_resource,
				p_header->_last_modified, p_header->_last_modified_len);
		}

		if (p_header->_is_chunked == TRUE)
		{
			LOG_DEBUG(" http_pipe[0x%X] is chunked !", p_http_pipe);
			HTTP_RESOURCE_SET_CHUNKED(p_http_pipe->_p_http_server_resource);
			ret_val = http_pipe_create_chunked(&(p_http_pipe->_data));
			CHECK_VALUE(ret_val);
		}

		if (p_header->_is_support_range == FALSE)
		{
			LOG_DEBUG(" http_pipe[0x%X] is not support ranges!", p_http_pipe);
			HTTP_RESOURCE_SET_NOT_SUPPORT_RANGE(p_http_pipe->_p_http_server_resource);
			p_http_pipe->_data_pipe._dispatch_info._is_support_range = FALSE;
			if (p_http_pipe->_p_http_server_resource->_relation_res)
			{
				LOG_DEBUG(" http_pipe[0x%X] is not support ranges and is a relation resource , need abandon!",
					p_http_pipe);
				return 	HTTP_ERR_RELATION_RES_NOT_SUPPORT_RANGE;
			}
		}

        // 对那些不规范的服务器，content_range 与 content_length明显出错的情况进行处理
        if(p_header->_has_content_length==TRUE && p_header->_is_content_range_length_valid)
        {
            if(p_header->_content_range_length < p_header->_content_length
                && p_header->_has_entity_length)
            {
                LOG_DEBUG("content_range_length = %llu, content_length = %llu"
                    , p_header->_content_range_length, p_header->_content_length);
                p_header->_content_length = p_header->_content_range_length;
            }
        }

		if (p_header->_is_support_long_connect == FALSE)
		{
			LOG_DEBUG(" http_pipe[0x%X] is not support long connection !", p_http_pipe);
			HTTP_RESOURCE_SET_NOT_SUPPORT_LONG_CONNECT(p_http_pipe->_p_http_server_resource);
		}

		if (p_header->_has_content_length && p_header->_body_len > p_header->_content_length)
		{
			p_header->_body_len = p_header->_content_length;
		}

		if (p_header->_has_entity_length && p_header->_body_len > p_header->_entity_length)
		{
			p_header->_body_len = p_header->_entity_length;
		}

		if (p_header->_body_len != 0)
		{
			LOG_DEBUG(" http_pipe[0x%X] while recv http response header, recv addional data, length: %u",
				p_http_pipe, p_header->_body_len);
			if (p_http_pipe->_data._temp_data_buffer == NULL)
			{
				ret_val = http_get_1024_buffer((void**)&(p_http_pipe->_data._temp_data_buffer));
				CHECK_VALUE(ret_val);
			}
			sd_memset(p_http_pipe->_data._temp_data_buffer, 0, HTTP_1024_LEN);
			sd_memcpy(p_http_pipe->_data._temp_data_buffer, p_http_pipe->_header._body_start_pos, p_http_pipe->_header._body_len);

			p_http_pipe->_data._temp_data_buffer_length = p_http_pipe->_header._body_len;

			add_speed_record(&(p_http_pipe->_speed_calculator), p_http_pipe->_header._body_len);
		}

		if (p_header->_has_content_length == TRUE)
		{
			LOG_DEBUG(" http_pipe[0x%X] _has_content_length:p_header->_content_length=%llu,_data._content_length=%llu",
				p_http_pipe, p_header->_content_length, p_http_pipe->_data._content_length);

			if (p_header->_content_length == 0 && !HTTP_RESOURCE_IS_ORIGIN(p_http_pipe->_p_http_server_resource))
				return HTTP_ERR_INVALID_SERVER_FILE_SIZE;

			range_index = http_pipe_get_download_range_index(p_http_pipe);

			LOG_DEBUG(" http_pipe[0x%X] _has_entity_length%d,range_index=%u,_req_end_pos=%llu",
				p_http_pipe, p_header->_has_entity_length, range_index, p_http_pipe->_req_end_pos);
			if ((p_header->_has_entity_length == FALSE)
				&& (range_index == 0)
				&& (p_http_pipe->_req_end_pos == -1 || 0 == pi_get_file_size((DATA_PIPE *)p_http_pipe)))
			{
				ret_val = http_pipe_set_file_size(p_http_pipe, p_header->_content_length);
				if (ret_val != SUCCESS) return ret_val;
			}
			else if (range_index != 0 && !p_http_pipe->_p_http_server_resource->_relation_res)
			{
				if (p_header->_has_entity_length == TRUE)
					file_size = p_header->_entity_length;
				else
					file_size = pi_get_file_size((DATA_PIPE *)p_http_pipe);

				if (file_size == 0)
				{
					file_size = HTTP_RESOURCE_GET_FILE_SIZE(p_http_pipe->_p_http_server_resource);
				}

				if ((file_size != 0) && (file_size > get_data_pos_from_index(range_index)))
				{
					if (file_size - get_data_pos_from_index(range_index) <p_header->_content_length)
					{
						p_header->_is_support_range = FALSE;
						HTTP_RESOURCE_SET_NOT_SUPPORT_RANGE(p_http_pipe->_p_http_server_resource);
						p_http_pipe->_data_pipe._dispatch_info._is_support_range = FALSE;
						LOG_DEBUG(" http_pipe[0x%X] is not support ranges!!", p_http_pipe);
						return HTTP_ERR_RANGE_NOT_SUPPORT;
					}
					else if ((file_size - get_data_pos_from_index(range_index) >= p_http_pipe->_data._content_length) &&
						(p_http_pipe->_data._content_length>p_header->_content_length))
					{
						LOG_DEBUG(" http_pipe[0x%X] HTTP_ERR_INVALID_SERVER_FILE_SIZE!!", p_http_pipe);
						return HTTP_ERR_INVALID_SERVER_FILE_SIZE;
					}

				}

			}
			else if (range_index != 0 && p_http_pipe->_p_http_server_resource->_relation_res)
			{
				RANGE tmp_r;
				tmp_r._index = range_index;
				tmp_r._num = 1;

				if (p_header->_has_entity_length == TRUE)
					file_size = p_header->_entity_length;


				if (file_size == 0)
				{
					file_size = HTTP_RESOURCE_GET_FILE_SIZE(p_http_pipe->_p_http_server_resource);
				}

				LOG_DEBUG("relation http_pipe[0x%X] file_size:%llu,p_header->_has_entity_length:%d, "
					"p_header->_entity_length:%llu, p_http_pipe->_data._content_lengt=%llu, p_header->_content_length=%llu",
					p_http_pipe, file_size, p_header->_has_entity_length,
					p_header->_entity_length, p_http_pipe->_data._content_length, p_header->_content_length);

				if (file_size != 0)
				{
					_u64 relation_pos = 0;
					_u64 origion_pos = range_index;
					origion_pos *= get_data_unit_size();
					_int32 ret = pt_origion_pos_to_relation_pos(p_http_pipe->_p_http_server_resource->_p_block_info_arr,
						p_http_pipe->_p_http_server_resource->_block_info_num, origion_pos, &relation_pos);

					if (ret != SUCCESS)
					{
						p_header->_is_support_range = FALSE;
						HTTP_RESOURCE_SET_NOT_SUPPORT_RANGE(p_http_pipe->_p_http_server_resource);
						p_http_pipe->_data_pipe._dispatch_info._is_support_range = FALSE;
						LOG_DEBUG("relation http_pipe[0x%X] is not support ranges!!", p_http_pipe);
						return HTTP_ERR_RELATION_RES_NOT_SUPPORT_RANGE;
					}

					if ((file_size - relation_pos) < p_header->_content_length)
					{
						p_header->_is_support_range = FALSE;
						HTTP_RESOURCE_SET_NOT_SUPPORT_RANGE(p_http_pipe->_p_http_server_resource);
						p_http_pipe->_data_pipe._dispatch_info._is_support_range = FALSE;
						LOG_DEBUG("relation http_pipe[0x%X] is not support ranges!!", p_http_pipe);
						return HTTP_ERR_RELATION_RES_NOT_SUPPORT_RANGE;
					}
					else if ((file_size - relation_pos) >= p_http_pipe->_data._content_length &&
						(p_http_pipe->_data._content_length>p_header->_content_length))
					{
						LOG_DEBUG("relation http_pipe[0x%X] HTTP_ERR_INVALID_SERVER_FILE_SIZE!!", p_http_pipe);
						return HTTP_ERR_INVALID_SERVER_FILE_SIZE;
					}
				}

			}

			if (p_http_pipe->_data._content_length > p_header->_content_length)
				p_http_pipe->_data._content_length = p_header->_content_length;


		}

		if (p_header->_has_entity_length)
		{
			LOG_DEBUG(" http_pipe[0x%X] _has_entity_length: %llu", p_http_pipe, p_header->_entity_length);

			if (p_header->_entity_length == 0)
				return HTTP_ERR_INVALID_SERVER_FILE_SIZE;

			file_size = pi_get_file_size((DATA_PIPE *)p_http_pipe);

			if (file_size != p_header->_entity_length)
			{
				ret_val = http_pipe_set_file_size(p_http_pipe, p_header->_entity_length);
				if (ret_val != SUCCESS) return ret_val;
			}

		}

		/* Just for mini http */
		if (p_http_pipe->_data_pipe._id != 0)
		{
			mini_http_set_header((DATA_PIPE *)p_http_pipe, p_header, code);
		}
		break;
	case 202: //Accepted
		LOG_DEBUG("http_pipe[0x%X] code == 202 :Accepted.but I don't know how to deal with this case Orz...", p_http_pipe);
		http_reset_respn_header(p_header);
		http_pipe_reset_respn_data(p_http_pipe, &(p_http_pipe->_data));
		p_http_pipe->_b_header_received = FALSE;
		return code;
		break;

	case 301:
	case 302:
	case 303:
	case 307:
	HandleLocation :
		LOG_DEBUG("http_pipe[0x%X] Redirection occured with code : http return code:%u ", p_http_pipe, code);
				   if (p_header->_location_len != 0)
				   {
					   ret_val = http_resource_set_redirect_url(p_http_pipe->_p_http_server_resource, 
						   p_header->_location, p_header->_location_len);
					   if (ret_val != SUCCESS) return ret_val;

					   if (p_http_pipe->_b_set_request == TRUE)
					   {
						   p_url_o = http_resource_get_redirect_url_object(p_http_pipe->_p_http_server_resource);
						   if ((p_url_o->_is_binary_file == FALSE) && (p_url_o->_is_dynamic_url == FALSE))
							   return HTTP_ERR_INVALID_REDIRECT_URL;
					   }

					   if (HTTP != sd_get_url_type(p_header->_location, p_header->_location_len))
					   {
						   if (p_http_pipe->_p_http_server_resource->_relation_res)
						   {
							   LOG_DEBUG("http_pipe[0x%X] HTTP_ERR_REDIRECT_TO_FTP , but is a relation resource , not support...",
								   p_http_pipe);
							   return HTTP_ERR_INVALID_REDIRECT_URL;
						   }
					   }

					   if (FTP == sd_get_url_type(p_header->_location, p_header->_location_len))
					   {
						   RES_QUERY_PARA res_query_pata;
						   res_query_pata._task_ptr = (TASK *)dp_get_task_ptr((DATA_PIPE *)p_http_pipe);
						   if (res_query_pata._task_ptr == NULL)
						   {
							   LOG_DEBUG("http_pipe[0x%X] HTTP_ERR_REDIRECT_TO_FTP", p_http_pipe);
							   return HTTP_ERR_REDIRECT_TO_FTP;
						   }
						   p_header->_location[p_header->_location_len] = '\0';
						   res_query_pata._file_index = dp_get_pipe_file_index((DATA_PIPE *)p_http_pipe);
						   ret_val = dt_add_server_resource((void*)&res_query_pata, p_header->_location, p_header->_location_len,
							   p_http_pipe->_p_http_server_resource->_ref_url, sd_strlen(p_http_pipe->_p_http_server_resource->_ref_url),
							   HTTP_RESOURCE_IS_ORIGIN(p_http_pipe->_p_http_server_resource), 0);

						   if (ret_val != SUCCESS)
						   {
							   LOG_DEBUG("http_pipe[0x%X] cm_add_server_resource ERROR!ret_val=%d", p_http_pipe, ret_val);
							   return ret_val;
						   }
						   else
						   {
							   LOG_DEBUG("http_pipe[0x%X] HTTP_ERR_REDIRECT_TO_FTP", p_http_pipe);
							   return HTTP_ERR_REDIRECT_TO_FTP;
						   }
					   }

					   if (p_header->_cookie_len != 0)
					   {
						   http_resource_set_cookies(p_http_pipe->_p_http_server_resource, p_header->_cookie, p_header->_cookie_len);
					   }

				   }
				   else
				   {
					   LOG_DEBUG("http_pipe[0x%X] HTTP_ERR_INVALID_REDIRECT_URL", p_http_pipe);
					   return HTTP_ERR_INVALID_REDIRECT_URL;
				   }

				   http_reset_respn_header(p_header);
				   http_pipe_reset_respn_data(p_http_pipe, &(p_http_pipe->_data));
				   p_http_pipe->_b_header_received = FALSE;
				   p_http_pipe->_b_redirect = TRUE;
				   p_http_pipe->_http_state = HTTP_PIPE_STATE_IDLE;
				   LOG_DEBUG("http_pipe[0x%X] ready to redirect...", p_http_pipe);
				   break;
	case 304: //NotModified
		LOG_DEBUG("http_pipe[0x%X] code == 304 //NotModified", p_http_pipe);
		//http_reset_respn_header( p_header);
		http_pipe_reset_respn_data(p_http_pipe, &(p_http_pipe->_data));
		p_http_pipe->_b_header_received = FALSE;
		return 304;
		break;
	case 400: //NotFound
	case 404:
		LOG_DEBUG("http_pipe[0x%X] code == 404 //NotFound", p_http_pipe);
		http_reset_respn_header(p_header);
		http_pipe_reset_respn_data(p_http_pipe, &(p_http_pipe->_data));
		p_http_pipe->_b_header_received = FALSE;


		//p_http_pipe->_b_redirect = FALSE;

		/* Maybe the path is in wrong format */
		p_url_o = http_pipe_get_url_object(p_http_pipe);
		//if((p_url_o->_path_encode_mode!=UEM_ASCII)||
		//   (p_url_o->_filename_encode_mode!=UEM_ASCII))
		{
			if (p_url_o->_path_valid_step == UCS_ORIGIN)
			{
				p_http_pipe->_b_retry_request = TRUE;
				p_http_pipe->_http_state = HTTP_PIPE_STATE_IDLE;
				LOG_DEBUG("http_pipe[0x%X] ready to retry request..._request_step=%d",
					p_http_pipe, p_http_pipe->_request_step);
				return SUCCESS;
			}
		}
		return code;
		break;
	case 407:
		LOG_DEBUG("http_pipe[0x%X] code == 407 //ProxyAuthenticationRequired", p_http_pipe);
		http_reset_respn_header(p_header);
		http_pipe_reset_respn_data(p_http_pipe, &(p_http_pipe->_data));
		p_http_pipe->_b_header_received = FALSE;
		return HTTP_ERR_PROXY_AUTH_REQED;
		break;
	case 416:
		LOG_DEBUG("http_pipe[0x%X] code == 416 //Requestedrangenotsatisfiable", p_http_pipe);
		http_reset_respn_header(p_header);
		http_pipe_reset_respn_data(p_http_pipe, &(p_http_pipe->_data));
		p_http_pipe->_b_header_received = FALSE;
		return HTTP_ERR_RANGE_OUT_OF_SIZE;
		break;
	case 502:
	case 503:
		LOG_DEBUG("http_pipe[0x%X] code == 503 //Limit Exceeded", p_http_pipe);
		http_reset_respn_header(p_header);
		http_pipe_reset_respn_data(p_http_pipe, &(p_http_pipe->_data));
		p_http_pipe->_b_header_received = FALSE;
		return HTTP_ERR_503_LIMIT_EXCEEDED;
		break;
	default:
		LOG_DEBUG("http_pipe[0x%X] code == %u //Unknown!", p_http_pipe, code);
#if defined(MOBILE_PHONE)
		if(code==403 && (NT_GPRS_WAP & sd_get_net_type()))
		{
			LOG_DEBUG( "3333 http_pipe[0x%X] get code:%u in CMWAP(%X),need reconnect network[%u]!",
				p_http_pipe,code,sd_get_net_type(),p_http_pipe->_socket_fd);
			LOG_DEBUG( "%s",p_http_pipe->_http_request);

			if(p_http_pipe->_header._header_buffer_end_pos!=0)
			{
				_u32 len =p_http_pipe->_header._header_buffer_end_pos;
				if(len>200)
				{
					char tmp_buffer[255];
					_u32 len_tmp = 0;
					sd_memset(tmp_buffer,0x00,255);
					sd_memcpy(tmp_buffer, p_http_pipe->_header._header_buffer, 200);
					len_tmp = 200;
					LOG_DEBUG( "[%d]%s",p_http_pipe->_header._header_buffer_end_pos,tmp_buffer);
					len=p_http_pipe->_header._header_buffer_end_pos-len_tmp;
					while(len>0)
					{
						if(len>200) len=200;

						sd_memcpy(tmp_buffer, p_http_pipe->_header._header_buffer+len_tmp, len);
						tmp_buffer[len]='\0';
						LOG_DEBUG( "%s",tmp_buffer);
						len_tmp+=len;
						len=p_http_pipe->_header._header_buffer_end_pos-len_tmp;
					}
				}
				else
				{
					LOG_DEBUG( "[%d]%s",p_http_pipe->_header._header_buffer_end_pos,p_http_pipe->_header._header_buffer);
				}
			}
			if(p_header->_is_vnd_wap )
			{
				LOG_URGENT( "It is a vnd.wap file:g_403_count=%d",g_403_count);
				if(g_403_count++ > MAX_WAP_403_COUNT)
				{
					g_403_count = 0;
					set_critical_error(NEED_RECONNECT_NETWORK);
					sd_assert(FALSE);
				}
			}
		}
#endif /* MOBILE_PHONE */
		http_reset_respn_header(p_header);
		http_pipe_reset_respn_data(p_http_pipe, &(p_http_pipe->_data));
		p_http_pipe->_b_header_received = FALSE;

        /* Maybe the path is in wrong format */
        p_url_o = http_pipe_get_url_object(p_http_pipe);
        //if((p_url_o->_path_encode_mode!=UEM_ASCII)||
        //   (p_url_o->_filename_encode_mode!=UEM_ASCII))
        {
            if(p_url_o->_path_valid_step==UCS_ORIGIN)
            {
                p_http_pipe->_b_retry_request = TRUE;
                p_http_pipe->_http_state = HTTP_PIPE_STATE_IDLE;
                LOG_DEBUG( "http_pipe[0x%X] ready to retry request..._request_step=%d" ,
					p_http_pipe,p_http_pipe->_request_step);
                return SUCCESS;
            }
        }
		return code;
	}

	return SUCCESS;

}

_int32 http_pipe_continue_reading(HTTP_DATA_PIPE * p_http_pipe)
{
	_int32 ret_val = SUCCESS;
	_u32 receive_len = 0;
	LOG_DEBUG("enter[0x%X] http_pipe_continue_reading:_b_header_received=%d,_b_data_full=%d,"
		"_data_length=%u,_data_buffer_end_pos=%u",
		p_http_pipe, p_http_pipe->_b_header_received, p_http_pipe->_b_data_full,
		p_http_pipe->_data._data_length, p_http_pipe->_data._data_buffer_end_pos);
	/* continue */
	if (p_http_pipe->_b_header_received == TRUE)
	{
		if (p_http_pipe->_b_data_full != TRUE)
		{
			LOG_DEBUG(" http_pipe[0x%X]_b_header_received=%d, _is_chunked=%d,_has_content_length=%d",
				p_http_pipe, p_http_pipe->_b_header_received, p_http_pipe->_header._is_chunked,
				p_http_pipe->_header._has_content_length);

			if (p_http_pipe->_data._data_buffer == NULL)
			{
				if (p_http_pipe->_header._is_chunked == TRUE)
				{
					if ((p_http_pipe->_data.p_chunked->_state == HTTP_CHUNKED_STATE_RECEIVE_DATA)
						|| ((p_http_pipe->_data.p_chunked->_state == HTTP_CHUNKED_STATE_RECEIVE_HEAD)
						&& (p_http_pipe->_data.p_chunked->_temp_buffer_end_pos != 0)))
					{
						ret_val = http_pipe_get_buffer(p_http_pipe);
						CHECK_VALUE(ret_val);
					}
				}
				else
				{
					ret_val = http_pipe_get_buffer(p_http_pipe);
					CHECK_VALUE(ret_val);
				}

				if (p_http_pipe->_b_data_full == TRUE)
				{
					return SUCCESS;
				}
			}




			/* Chunked or no content_length */
			if (p_http_pipe->_header._is_chunked == TRUE)
			{
				if (p_http_pipe->_data.p_chunked->_state == HTTP_CHUNKED_STATE_RECEIVE_HEAD)
				{
					LOG_DEBUG(" http_pipe[0x%X]HTTP_CHUNKED_STATE_RECEIVE_HEAD!,_head_buffer_end_pos=%u",
						p_http_pipe, p_http_pipe->_data.p_chunked->_head_buffer_end_pos);
					if (p_http_pipe->_data.p_chunked->_temp_buffer_end_pos != 0)
					{
						ret_val = http_pipe_store_chunked_temp_data_buffer(p_http_pipe);
					}
					//sd_assert(p_http_pipe->_data.p_chunked->_head_buffer_end_pos==0);
					if (ret_val == SUCCESS)
					{
						if (p_http_pipe->_data.p_chunked->_head_buffer_end_pos == 0)
						{
							receive_len = 8;
							sd_memset(p_http_pipe->_data.p_chunked->_head_buffer, 0, CHUNKED_HEADER_BUFFER_LEN);
						}
						else
						{
							receive_len = CHUNKED_HEADER_BUFFER_LEN - p_http_pipe->_data.p_chunked->_head_buffer_end_pos - 1;
							sd_memset(p_http_pipe->_data.p_chunked->_head_buffer + p_http_pipe->_data.p_chunked->_head_buffer_end_pos, 
								0, receive_len + 1);
						}
						LOG_DEBUG(" http_pipe[0x%X]HTTP_CHUNKED_STATE_RECEIVE_HEAD "
							"call SOCKET_PROXY_UNCOMPLETE_RECV(_socket_fd=%u,CHUNKED_HEADER_BUFFER_LEN=%u,_head_buffer_end_pos=%u,receive_len=%u)",
							p_http_pipe,
							p_http_pipe->_socket_fd, CHUNKED_HEADER_BUFFER_LEN, 
							p_http_pipe->_data.p_chunked->_head_buffer_end_pos, receive_len);
#ifdef _ENABLE_SSL
						if (p_http_pipe->_is_https)
						{
							ret_val = socket_proxy_recv_ssl(p_http_pipe->_socket_fd, p_http_pipe->_pbio,
								p_http_pipe->_data.p_chunked->_head_buffer + p_http_pipe->_data.p_chunked->_head_buffer_end_pos,
								receive_len, http_pipe_handle_uncomplete_recv, (void*)p_http_pipe);
						}
						else
#endif
						if (p_http_pipe->_data_pipe._id != 0)  // Browser request
						{
							ret_val = socket_proxy_uncomplete_recv_browser(p_http_pipe->_socket_fd,
								p_http_pipe->_data.p_chunked->_head_buffer + p_http_pipe->_data.p_chunked->_head_buffer_end_pos,
								receive_len, http_pipe_handle_uncomplete_recv, (void*)p_http_pipe);
						}
						else
						{
							ret_val = socket_proxy_uncomplete_recv(p_http_pipe->_socket_fd,
								p_http_pipe->_data.p_chunked->_head_buffer + p_http_pipe->_data.p_chunked->_head_buffer_end_pos,
								receive_len, http_pipe_handle_uncomplete_recv, (void*)p_http_pipe);
						}
					}
				}
				else if (p_http_pipe->_data.p_chunked->_state == HTTP_CHUNKED_STATE_RECEIVE_DATA)
				{
					if (p_http_pipe->_data.p_chunked->_temp_buffer_end_pos != 0)
					{
						ret_val = http_pipe_store_chunked_temp_data_buffer(p_http_pipe);
					}

					if (ret_val == SUCCESS)
					{
						receive_len = http_pipe_get_receive_len(p_http_pipe);
						sd_assert(p_http_pipe->_data._data_length > p_http_pipe->_data._data_buffer_end_pos);
						sd_assert(receive_len != 0);
						LOG_DEBUG(" http_pipe[0x%X]HTTP_CHUNKED_STATE_RECEIVE_DATA "
							"call SOCKET_PROXY_RECV(_socket_fd=%u,_data_length=%u,_data_buffer_end_pos=%u,receive_len=%u)",
							p_http_pipe,
							p_http_pipe->_socket_fd, p_http_pipe->_data._data_length, 
							p_http_pipe->_data._data_buffer_end_pos, receive_len);
#ifdef _ENABLE_SSL
						if (p_http_pipe->_is_https)
						{
							ret_val = socket_proxy_recv_ssl(p_http_pipe->_socket_fd, p_http_pipe->_pbio,
								p_http_pipe->_data._data_buffer + p_http_pipe->_data._data_buffer_end_pos,
								receive_len, http_pipe_handle_recv, (void*)p_http_pipe);
						}
						else
#endif
						if (p_http_pipe->_data_pipe._id != 0)  // Browser request
						{
							ret_val = socket_proxy_recv_browser(p_http_pipe->_socket_fd,
								p_http_pipe->_data._data_buffer + p_http_pipe->_data._data_buffer_end_pos,
								receive_len, http_pipe_handle_recv, (void*)p_http_pipe);
						}
						else
						{
							ret_val = socket_proxy_recv(p_http_pipe->_socket_fd,
								p_http_pipe->_data._data_buffer + p_http_pipe->_data._data_buffer_end_pos,
								receive_len, http_pipe_handle_recv, (void*)p_http_pipe);
						}
					}
				}
				else
				{
					LOG_DEBUG(" http_pipe[0x%X]HTTP_CHUNKED_STATE_END!", p_http_pipe);
					sd_assert(FALSE);
				}
			}
			else if (p_http_pipe->_header._has_content_length == FALSE)
			{
				receive_len = http_pipe_get_receive_len(p_http_pipe);
				sd_assert(p_http_pipe->_data._data_length > p_http_pipe->_data._data_buffer_end_pos);
				sd_assert(receive_len != 0);
				LOG_DEBUG(" http_pipe[0x%X] "
					"call SOCKET_PROXY_UNCOMPLETE_RECV(_socket_fd=%u,_data_length=%u,_data_buffer_end_pos=%u,receive_len=%u)",
					p_http_pipe, p_http_pipe->_socket_fd, p_http_pipe->_data._data_length,
					p_http_pipe->_data._data_buffer_end_pos, receive_len);
#ifdef _ENABLE_SSL
				if (p_http_pipe->_is_https)
				{
					ret_val = socket_proxy_recv_ssl(p_http_pipe->_socket_fd, p_http_pipe->_pbio,
						p_http_pipe->_data._data_buffer + p_http_pipe->_data._data_buffer_end_pos,
						receive_len, http_pipe_handle_uncomplete_recv, (void*)p_http_pipe);
				}
				else
#endif
				if (p_http_pipe->_data_pipe._id != 0)  // Browser request
				{
					ret_val = socket_proxy_uncomplete_recv_browser(p_http_pipe->_socket_fd,
						p_http_pipe->_data._data_buffer + p_http_pipe->_data._data_buffer_end_pos,
						receive_len, http_pipe_handle_uncomplete_recv, (void*)p_http_pipe);
				}
				else
				{
					ret_val = socket_proxy_uncomplete_recv(p_http_pipe->_socket_fd,
						p_http_pipe->_data._data_buffer + p_http_pipe->_data._data_buffer_end_pos,
						receive_len, http_pipe_handle_uncomplete_recv, (void*)p_http_pipe);
				}
			}
			else
			{
				receive_len = http_pipe_get_receive_len(p_http_pipe);
				sd_assert(p_http_pipe->_data._data_length > p_http_pipe->_data._data_buffer_end_pos);
				sd_assert(receive_len != 0);
				LOG_DEBUG(" http_pipe[0x%X]"
					"call SOCKET_PROXY_RECV(_socket_fd=%u,_data_length=%u,_data_buffer_end_pos=%u,receive_len=%u)",
					p_http_pipe, p_http_pipe->_socket_fd, p_http_pipe->_data._data_length, 
					p_http_pipe->_data._data_buffer_end_pos, receive_len);
#ifdef _ENABLE_SSL
				if (p_http_pipe->_is_https)
				{
					ret_val = socket_proxy_recv_ssl(p_http_pipe->_socket_fd, p_http_pipe->_pbio,
						p_http_pipe->_data._data_buffer + p_http_pipe->_data._data_buffer_end_pos,
						receive_len, http_pipe_handle_recv, (void*)p_http_pipe);
				}
				else
#endif	
				if (p_http_pipe->_data_pipe._id != 0)  // Browser request
				{
					ret_val = socket_proxy_recv_browser(p_http_pipe->_socket_fd,
						p_http_pipe->_data._data_buffer + p_http_pipe->_data._data_buffer_end_pos,
						receive_len, http_pipe_handle_recv, (void*)p_http_pipe);
				}
				else
				{
					ret_val = socket_proxy_recv(p_http_pipe->_socket_fd,
						p_http_pipe->_data._data_buffer + p_http_pipe->_data._data_buffer_end_pos,
						receive_len, http_pipe_handle_recv, (void*)p_http_pipe);
				}
			}
		}
	}
	else
	{
		LOG_DEBUG(" http_pipe[0x%X]  HTTP_PIPE_STATE_READING:_header_buffer_length=%u,_header_buffer_end_pos=%u",
			p_http_pipe, p_http_pipe->_header._header_buffer_length, p_http_pipe->_header._header_buffer_end_pos);

		http_header_discard_excrescent(&(p_http_pipe->_header));
		if (p_http_pipe->_header._header_buffer_length - 1 > p_http_pipe->_header._header_buffer_end_pos)
		{
			LOG_DEBUG(" http_pipe[0x%X] call SOCKET_PROXY_UNCOMPLETE_RECV(_socket_fd=%u,"
				"_header_buffer_end_pos=%u,_header_buffer_length=%u)",
				p_http_pipe, p_http_pipe->_socket_fd,
				p_http_pipe->_header._header_buffer_end_pos, p_http_pipe->_header._header_buffer_length);
#ifdef _ENABLE_SSL
			if (p_http_pipe->_is_https)
			{
				ret_val = socket_proxy_recv_ssl(p_http_pipe->_socket_fd, p_http_pipe->_pbio,
					p_http_pipe->_header._header_buffer + p_http_pipe->_header._header_buffer_end_pos,
					p_http_pipe->_header._header_buffer_length - 1 - p_http_pipe->_header._header_buffer_end_pos,
					http_pipe_handle_uncomplete_recv, (void*)p_http_pipe);
			}
			else
#endif	
			if (p_http_pipe->_data_pipe._id != 0)  // Browser request
			{
				ret_val = socket_proxy_uncomplete_recv_browser(p_http_pipe->_socket_fd,
					p_http_pipe->_header._header_buffer + p_http_pipe->_header._header_buffer_end_pos,
					p_http_pipe->_header._header_buffer_length - 1 - p_http_pipe->_header._header_buffer_end_pos,
					http_pipe_handle_uncomplete_recv, (void*)p_http_pipe);
			}
			else
			{
				ret_val = socket_proxy_uncomplete_recv(p_http_pipe->_socket_fd,
					p_http_pipe->_header._header_buffer + p_http_pipe->_header._header_buffer_end_pos,
					p_http_pipe->_header._header_buffer_length - 1 - p_http_pipe->_header._header_buffer_end_pos,
					http_pipe_handle_uncomplete_recv, (void*)p_http_pipe);
			}
		}
		else
		{
			LOG_DEBUG(" http_pipe[0x%X] HTTP_ERR_UNKNOWN! ", p_http_pipe);
			return HTTP_ERR_UNKNOWN;
		}

	}
	return ret_val;
}


/*************************/
/* structrue reset      */
/*************************/



_int32 http_pipe_reset_pipe(HTTP_DATA_PIPE * p_http_pipe)
{

	LOG_DEBUG(" enter http_pipe_reset_pipe(0x%X,%u)...", p_http_pipe, p_http_pipe->_data_pipe._id);

	uninit_pipe_info(&(p_http_pipe->_data_pipe));

	http_pipe_reset_respn_data(p_http_pipe, &(p_http_pipe->_data));

	http_reset_respn_header(&(p_http_pipe->_header));

	SAFE_DELETE(p_http_pipe->_http_request);

	sd_memset(p_http_pipe, 0, sizeof(HTTP_DATA_PIPE));

	return SUCCESS;

}
/*******************************************************************/

BOOL http_pipe_is_binary_file(HTTP_DATA_PIPE * p_http_pipe)
{
	PIPE_FILE_TYPE file_type = pi_get_file_type((DATA_PIPE *)p_http_pipe);
	BOOL redirect_url_is_binary_file = FALSE;

	if (p_http_pipe->_p_http_server_resource->_redirect_url != NULL)
	{
		redirect_url_is_binary_file = p_http_pipe->_p_http_server_resource->_redirect_url->_is_binary_file;
	}
	LOG_DEBUG("enter[0x%X] http_pipe_is_binary_file..."
		"header._is_binary_file=%d,_url._is_binary_file=%d,redirect_url._is_binary_file=%d,file_type=%d",
		p_http_pipe,
		p_http_pipe->_header._is_binary_file, p_http_pipe->_p_http_server_resource->_url._is_binary_file,
		redirect_url_is_binary_file, file_type);

	if (file_type == PIPE_FILE_BINARY_TYPE)
	{
		return TRUE;
	}
	else if (file_type == PIPE_FILE_HTML_TYPE)
	{
		return FALSE;
	}
	else  /* PIPE_FILE_UNKNOWN_TYPE  */
	if ((p_http_pipe->_header._is_binary_file == TRUE)
		|| (p_http_pipe->_p_http_server_resource->_url._is_binary_file == TRUE)
		|| (redirect_url_is_binary_file == TRUE))
	{
		LOG_DEBUG(" http_pipe[0x%X]_is_binary_file!", p_http_pipe);
		return TRUE;
	}
	LOG_DEBUG(" http_pipe[0x%X]_is not binary_file!", p_http_pipe);
	return FALSE;
}

_int32 http_pipe_set_can_download_ranges(HTTP_DATA_PIPE * p_http_pipe, _u64 file_size)
{
	RANGE all_range;
	_int32 ret_val = SUCCESS;
	LOG_DEBUG(" enter[0x%X] http_pipe_set_can_download_ranges:file_size=%llu,"
		"_b_retry_set_file_size=%d,_content_length=%llu",
		p_http_pipe, file_size, p_http_pipe->_b_retry_set_file_size,
		p_http_pipe->_data._content_length);
	/* Set the _data_pipe._dispatch_info._can_download_ranges ! */
	if ((p_http_pipe->_b_retry_set_file_size != TRUE)/*&&(dp_get_can_download_ranges_list_size( &(p_http_pipe->_data_pipe) ) ==0)*/)
	{

		if (dp_get_can_download_ranges_list_size(&(p_http_pipe->_data_pipe)) != 0)
		{
			ret_val = dp_clear_can_download_ranges_list(&(p_http_pipe->_data_pipe));
			CHECK_VALUE(ret_val);
		}

		if (p_http_pipe->_p_http_server_resource->_relation_res)
		{
			LOG_DEBUG("http_pipe_set_can_download_ranges p_http_pipe=0x%X p_http_pipe->_p_http_server_resource->_block_info_num=%u",
				p_http_pipe, p_http_pipe->_p_http_server_resource->_block_info_num);

			http_relation_pipe_add_range(p_http_pipe);
		}
		else
		{
			all_range = pos_length_to_range(0, file_size, file_size);
			//ret_val =pi_pos_len_to_valid_range((DATA_PIPE *) p_http_pipe, 0, file_size, &all_range );
			//CHECK_VALUE(ret_val);


			ret_val = dp_add_can_download_ranges(&(p_http_pipe->_data_pipe), &all_range);
			CHECK_VALUE(ret_val);
			LOG_DEBUG("set can_download_range,all_range._index=%u,all_range._num=%u", all_range._index, all_range._num);
		}

	}

	if ((p_http_pipe->_b_retry_set_file_size != TRUE)
		&& (p_http_pipe->_data._content_length > file_size)
		&& !p_http_pipe->_p_http_server_resource->_relation_res)
	{
		LOG_DEBUG("WARNING:_data._content_length =%llu>result=%llu", p_http_pipe->_data._content_length, file_size);
		p_http_pipe->_data._content_length = file_size;
	}
	return SUCCESS;

}
URL_OBJECT* http_pipe_get_url_object(HTTP_DATA_PIPE * p_http_pipe)
{
	URL_OBJECT* p_url_o = NULL;
	if (p_http_pipe->_b_redirect == FALSE)
		p_url_o = http_resource_get_url_object(p_http_pipe->_p_http_server_resource);
	else
		p_url_o = http_resource_get_redirect_url_object(p_http_pipe->_p_http_server_resource);

#ifdef _DEBUG
	sd_assert(p_url_o != NULL);
#endif
	return p_url_o;
}

_int32  http_pipe_correct_uncomplete_head_range_for_not_support_range(HTTP_DATA_PIPE * p_http_pipe)
{
	RANGE   uncom_head_range, new_uncom_head_range;
	_int32 ret_val = SUCCESS;

	LOG_DEBUG("_p_http_pipe=0x%X:http_pipe_correct_uncomplete_head_range_for_not_support_range", p_http_pipe);

	new_uncom_head_range._index = 0;
	new_uncom_head_range._num = 0;

	if (HTTP_RESOURCE_IS_SUPPORT_RANGE(p_http_pipe->_p_http_server_resource) == FALSE)
	{
		ret_val = dp_get_uncomplete_ranges_head_range(&(p_http_pipe->_data_pipe), &uncom_head_range);
		CHECK_VALUE(ret_val);

		if (0 != uncom_head_range._num)
		{
			if (uncom_head_range._index != 0)
			{
				//#ifdef _DEBUG
				//              LOG_DEBUG( "_p_http_pipe=0x%X:I don't think this is good!",p_http_pipe);
				//              sd_assert(FALSE);
				//#endif
				new_uncom_head_range._num += uncom_head_range._num + uncom_head_range._index;
				new_uncom_head_range._index = 0;
				ret_val = dp_clear_uncomplete_ranges_list(&(p_http_pipe->_data_pipe));
				CHECK_VALUE(ret_val);
				ret_val = dp_add_uncomplete_ranges(&(p_http_pipe->_data_pipe), &new_uncom_head_range);
				CHECK_VALUE(ret_val);
			}
		}
	}

	return SUCCESS;
}

#endif /* __HTTP_DATA_PIPE_C_20080614 */
