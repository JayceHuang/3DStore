#include "utility/string.h"
#include "utility/utility.h"

#include "utility/logid.h"
#define	 LOGID	LOGID_SOCKET_PROXY
#include "utility/logger.h"

#include "socket_proxy_encap_http_client.h"
#include "socket_proxy_encap.h"

static _int32 socket_encap_http_client_recv_impl(SOCKET sock, char* buffer, _u32 len,
	socket_proxy_recv_handler callback_handler, void* user_data, BOOL is_one_shot)
{
	_int32 ret = SUCCESS;
	BOOL one_shot = is_one_shot;
	_u32 bufsize = len;
	_u32 sock_num = sock;
	BOOL recv_impl_is_one_shot = is_one_shot;
	SOCKET_ENCAP_PROP_S *p_encap = NULL;
	SOCKET_ENCAP_HTTP_CLIENT_S *p_encap_http = NULL;
	ret = get_socket_encap_prop_by_sock(sock_num, &p_encap);
	CHECK_VALUE(ret);
	p_encap_http = &(p_encap->_encap_state._http_client);

	LOG_DEBUG("ENCAP HTTP DO RECV, sock=%d, size=%d, one_shot=%d", sock_num, bufsize, one_shot);

	p_encap_http->_p_recv_body_data = buffer;
	p_encap_http->_recv_body_data_size = bufsize;
	p_encap_http->_recv_one_shot = one_shot;
	p_encap_http->_recv_body_len = 0;
	p_encap_http->_callback_handler_recv = callback_handler;
	p_encap_http->_callback_recv_data._sock = sock;
	p_encap_http->_callback_recv_data._user_data = user_data;

	// 如果header buf 中还有 body ，从header buf 中读取。
	if (p_encap_http->_recv_body_remain_in_buf > 0)
	{
		// 当recv_len=0时, socket_reactor 也会回调并返回成功。
		ret = socket_proxy_recv_function(sock, buffer, 0, 
			socket_encap_http_client_recv_handler, &p_encap_http->_callback_recv_data, one_shot);

		return SUCCESS;
	}

	if (! p_encap_http->_recv_enable)
	{
		// 不能RECV （之前没有SEND操作），发一个HTTP GET 先。
		LOG_DEBUG("ENCAP HTTP DO RECV (Send GET First), sock=%d, size=%d, one_shot=%d", sock_num, bufsize, one_shot);

		/*
		// 暂存recv操作。（不需要）
		p_encap_http->_http_protocal_need_recv = TRUE;
		p_encap_http->_bak_op_recv._buffer = buffer;
		p_encap_http->_bak_op_recv._size = bufsize;
		p_encap_http->_bak_op_recv._callback_handler = callback_handler;
		p_encap_http->_bak_op_recv._callback_data._sock = sock;
		p_encap_http->_bak_op_recv._callback_data._user_data = user_data;
		p_encap_http->_bak_op_recv._one_shot = one_shot;
		ret = socket_encap_http_client_send_http_get(sock, NULL, 0, NULL, user_data);
		return ret;
		*/
		
		ret = socket_encap_http_client_send_http_get(sock, NULL, 0, NULL, user_data);
	}

	// 开始RECV

	if (SOCKET_ENCAP_STATE_HTTP_CLIENT_HEADER == p_encap_http->_recv_state)
	{
		// 用 header buf 接收 http header.
		p_encap_http->_p_recv_data = p_encap_http->_recv_header_buf + p_encap_http->_recv_keyword_remain_in_buf;
		p_encap_http->_recv_data_size = SOCKET_ENCAP_HTTP_CLIENT_HEADER_BUF_SIZE - p_encap_http->_recv_keyword_remain_in_buf;
	}
	else if (SOCKET_ENCAP_STATE_HTTP_CLIENT_BODY == p_encap_http->_recv_state)
	{
		// 直接用上层传入的buf 接收数据。
		p_encap_http->_p_recv_data = buffer;
		p_encap_http->_recv_data_size = MIN(bufsize, p_encap_http->_recv_http_body_remain);
	}

	recv_impl_is_one_shot = (p_encap_http->_recv_state == SOCKET_ENCAP_STATE_HTTP_CLIENT_HEADER)? TRUE: one_shot;

	LOG_DEBUG("ENCAP HTTP DO RECV_IMPL, sock=%d, need_len=%d, recv_impl_is_one_shot=%d", 
		sock_num, p_encap_http->_recv_data_size, recv_impl_is_one_shot);

	ret = socket_proxy_recv_function(sock_num,
		p_encap_http->_p_recv_data,
		p_encap_http->_recv_data_size,
		socket_encap_http_client_recv_handler,
		(void *)(&p_encap_http->_callback_recv_data),
		recv_impl_is_one_shot);
	return ret;
}

char *socket_encap_http_client_build_http_send_header(char *ip, _u16 port, _u32 body_size,
	char *header_buf, _u32 header_buf_size, _u32 *p_header_len)
{
	char * http_send_template = NULL;
	if (body_size == 0)
	{
		http_send_template = "GET /p2p/index.html HTTP/1.1\r\n\
Host: %s:%u\r\n\
Content-Type: application/octet-stream\r\n\
Connection: Close\r\n\
\r\n";

		sd_snprintf(header_buf, header_buf_size, http_send_template, ip, port);
	}
	else
	{
		http_send_template = "POST /p2p/index.html HTTP/1.1\r\n\
Host: %s:%u\r\n\
Content-Length: %u\r\n\
Content-Type: application/octet-stream\r\n\
Connection: Close\r\n\
\r\n";

		sd_snprintf(header_buf, header_buf_size, http_send_template, ip, port, body_size);
	}
	*p_header_len = sd_strlen(header_buf);
	return header_buf;
}

// 从header buf 中获取header。
// 返回>0  成功，返回值为 header 的大小。
// 返回0  http header不完整。
// 返回<0 不是http header。
_int32 socket_encap_http_client_parse_header(char *buf, _u32 buf_size, _u32 *p_http_body_total)
{
	char * str_content_length = "Content-Length:";
	char * p_content_length = NULL;
	char * p_end = NULL;
	char * p_line_end = NULL;
	char tmp[8] = {0};

	sd_assert(buf_size <= SOCKET_ENCAP_HTTP_CLIENT_HEADER_BUF_SIZE);

	*p_http_body_total = 0;

	buf[buf_size] = '\0';	// 用于sd_strstr() 查找。
	p_content_length = sd_strstr(buf, str_content_length, 0);
	p_end = sd_strstr(buf, "\r\n\r\n", 0);

	if (p_end == NULL)
	{
		// 没收完。
		if (buf_size == SOCKET_ENCAP_HTTP_CLIENT_HEADER_BUF_SIZE)
		{
			// 全部 header buf 收完都没发现 "\r\n\r\n"。
			return -1;
		}
		else
		{
			return 0;
		}
	}

	if (p_content_length == NULL || p_content_length > p_end)
	{
		// "Content-Length:"没有找到或者错位。
		return -2;
	}

	// 解析content_length
	p_line_end = sd_strstr(p_content_length, "\r\n", 0);
	p_content_length += sd_strlen(str_content_length);
	while (*p_content_length == ' ' || *p_content_length == '\t')
	{
		p_content_length ++;
	}
	sd_strncpy(tmp, p_content_length, p_line_end - p_content_length);
	tmp[p_line_end - p_content_length] = 0;
	*p_http_body_total = sd_atoi(tmp);
	return p_end - buf + 4;
}

// 从header buf 中获取最近一个http包中的body。
_int32 socket_encap_http_client_get_body_once_from_header_buf(
	char * buf,
	_u32 buf_size, 
	char *body_buf,
	_u32 body_buf_size,

	_u32 *p_body_remain_in_buf,
	_u32 *p_http_body_total,
	_u32 *p_http_body_remain,

	_u32 *p_header_len, 
	_u32 *p_body_len,
	_u32 *p_total_remain_in_buf)
{
	_int32 header_len = 0;
	sd_assert(
		(*p_body_remain_in_buf <= *p_http_body_remain)
		&& (buf_size >= *p_body_remain_in_buf)
		);

	if (*p_body_remain_in_buf > 0)
	{
		// 还有数据。
		*p_body_len = MIN(*p_body_remain_in_buf, body_buf_size);
		*p_body_remain_in_buf -= *p_body_len;
		*p_http_body_remain -= *p_body_len;
		*p_header_len = 0;
		*p_total_remain_in_buf -= *p_body_len;

		sd_memcpy(body_buf, buf, *p_body_len);
		sd_memmove(buf, buf + *p_body_len, *p_total_remain_in_buf);
		return SUCCESS;
	}

	header_len = socket_encap_http_client_parse_header(buf, buf_size, p_http_body_total);
	if (header_len > 0)
	{
		// 收到完整的http头。
		*p_header_len = header_len;
		*p_body_remain_in_buf = MIN(buf_size - *p_header_len, *p_http_body_total);
		*p_http_body_remain = *p_http_body_total;
		*p_body_len = 0;
		*p_total_remain_in_buf = buf_size - *p_header_len;
		sd_memmove(buf, buf + *p_header_len, *p_total_remain_in_buf);

		if (*p_http_body_total > 0)
		{
			*p_body_len = MIN(*p_body_remain_in_buf, body_buf_size);
			*p_body_remain_in_buf -= *p_body_len;
			*p_http_body_remain -= *p_body_len;
			*p_total_remain_in_buf -= *p_body_len;

			sd_memcpy(body_buf, buf, *p_body_len);
			sd_memmove(buf, buf + *p_body_len, *p_total_remain_in_buf);
		}
		return SUCCESS;
	}
	else if (header_len == 0)
	{
		// http 头不完整。
		*p_header_len = header_len;
		*p_body_remain_in_buf = 0;
		*p_http_body_total = 0;
		*p_http_body_remain = 0;
		*p_body_len = 0;
		*p_total_remain_in_buf = buf_size;
		return SUCCESS;
	}
	else
	{
		// 不是http 头。
		return -1;
	}

	return SUCCESS;
}

// 从header buf 中获取全部body。
_int32 socket_encap_http_client_get_body_from_header_buf(
	char * buf,
	_u32 buf_size, 
	char *body_buf,
	_u32 body_buf_size,

	_u32 *p_body_remain_in_buf,
	_u32 *p_http_body_total,
	_u32 *p_http_body_remain,

	_u32 *p_header_len, 
	_u32 *p_body_len,
	_u32 *p_total_remain_in_buf)
{
	_int32 ret = SUCCESS;
	_u32 body_len =0, body_len_once_get = 0;
	*p_total_remain_in_buf = buf_size;
	do
	{
		ret = socket_encap_http_client_get_body_once_from_header_buf(
			buf,
			*p_total_remain_in_buf, 
			body_buf + body_len,
			body_buf_size - body_len,

			p_body_remain_in_buf,
			p_http_body_total,
			p_http_body_remain,

			p_header_len, 
			&body_len_once_get,
			p_total_remain_in_buf);

		if (ret != SUCCESS)
		{
			return ret;
		}
		body_len += body_len_once_get;
	}
	while (body_len < body_buf_size && *p_total_remain_in_buf > 0 && body_len_once_get > 0);
	*p_body_len = body_len;
	return SUCCESS;
}

_int32 socket_encap_http_client_send_handler_impl(_int32 errcode, _u32 pending_op_count,
	const char* buffer, _u32 had_send, void* user_data, _u32 *p_body_data_len, BOOL *p_need_callback)
{
	_int32 ret = SUCCESS;
	BOOL need_send_one_more = FALSE;
	char *next_send_buffer = NULL;
	_u32 next_send_buffer_size = 0;
	_u32 send_len = had_send;

	SOCKET_ENCAP_PROP_S *p_encap = NULL;
	SOCKET_ENCAP_HTTP_CLIENT_S *p_encap_http = NULL;
	SOCKET_ENCAP_HTTP_CLIENT_CALLBACK_DATA_S *p_callback_data
		= (SOCKET_ENCAP_HTTP_CLIENT_CALLBACK_DATA_S *)user_data;
	_u32 sock_num = p_callback_data->_sock;
	
	ret = get_socket_encap_prop_by_sock(sock_num, &p_encap);
	CHECK_VALUE(ret);
	p_encap_http = &(p_encap->_encap_state._http_client);
		
	*p_body_data_len = 0;
	if (errcode == SUCCESS)
	{
		if (p_encap_http->_send_state == SOCKET_ENCAP_STATE_HTTP_CLIENT_HEADER)
		{
			p_encap_http->_sent_header_len += send_len;
			if (p_encap_http->_sent_header_len >= p_encap_http->_send_header_total_len)
			{
				// 全部 header 发完。
				if (p_encap_http->_send_body_data_size == 0)
				{
					// 没有 body。
				}
				else
				{
					// 准备发 body 。
					p_encap_http->_send_state = SOCKET_ENCAP_STATE_HTTP_CLIENT_BODY;
					p_encap_http->_sent_body_len = 0;

					next_send_buffer = p_encap_http->_p_send_body_data;
					next_send_buffer_size = p_encap_http->_send_body_data_size;
					need_send_one_more = TRUE;
				}
			}
			else
			{
				// 没有发完
				next_send_buffer = p_encap_http->_send_header_buf + p_encap_http->_sent_header_len;
				next_send_buffer_size = p_encap_http->_send_header_total_len - p_encap_http->_sent_header_len;
				need_send_one_more = TRUE;
			}
		}
		else if (p_encap_http->_send_state == SOCKET_ENCAP_STATE_HTTP_CLIENT_BODY)
		{
			p_encap_http->_sent_body_len += send_len;
			if (p_encap_http->_sent_body_len >= p_encap_http->_send_body_data_size)
			{
				// 全部 body 发完。
				p_encap_http->_send_state = SOCKET_ENCAP_STATE_HTTP_CLIENT_HEADER;
				*p_body_data_len = p_encap_http->_sent_body_len;
			}
			else
			{
				// 没有发完
				next_send_buffer = p_encap_http->_p_send_body_data + p_encap_http->_sent_body_len;
				next_send_buffer_size = p_encap_http->_send_body_data_size - p_encap_http->_sent_body_len;
				need_send_one_more = TRUE;
			}
		}
		else
		{
			sd_assert(0);
		}

		if (need_send_one_more)
		{
			// 没有发完，继续发送。
			LOG_DEBUG("ENCAP HTTP DO SEND_IMPL ONCE MORE, sock=%d, done_len=%d, need_len=%d", 
				sock_num, *p_body_data_len, next_send_buffer_size);

			ret = socket_proxy_send_function(sock_num, next_send_buffer, next_send_buffer_size, 
				socket_encap_http_client_send_handler, (void *)(&p_encap_http->_callback_send_data));

			if(ret!=SUCCESS)
			{
				*p_need_callback = TRUE;
				return ret;
			}

			*p_need_callback = FALSE;
			return SUCCESS;
		}

		// 全部发完。

		if (p_encap_http->_send_type == SOCKET_ENCAP_HTTP_PROTOCAL_SEND_GET)
		{
			// 这是HTTP GET 产生的回调。
			p_encap_http->_send_type = SOCKET_ENCAP_HTTP_PROTOCAL_SEND_NONE;

			// 发送暂存的send
			if (p_encap_http->_http_protocal_need_send)
			{
				p_encap_http->_http_protocal_need_send = FALSE;

				ret = socket_encap_http_client_send(
					p_callback_data->_sock,
					p_encap_http->_bak_op_send._buffer,
					p_encap_http->_bak_op_send._size,
					p_encap_http->_bak_op_send._callback_handler,
					p_encap_http->_bak_op_send._callback_data._user_data);
				if(ret!=SUCCESS)
				{
					*p_need_callback = TRUE;
					return ret;
				}
			}

			// HTTP GET 不需要返回给上层。
			*p_need_callback = FALSE;
		}
		else
		{
			// 这是上层调用 send 产生的回调。
			p_encap_http->_send_type = SOCKET_ENCAP_HTTP_PROTOCAL_SEND_NONE;
			*p_need_callback = TRUE;
		}

		p_encap_http->_recv_enable = TRUE;

		// 发送暂存的recv
		if (p_encap_http->_http_protocal_need_recv)
		{
			p_encap_http->_http_protocal_need_recv = FALSE;
			
			ret = socket_encap_http_client_recv_impl(
					p_callback_data->_sock,
					p_encap_http->_bak_op_recv._buffer,
					p_encap_http->_bak_op_recv._size,
					p_encap_http->_bak_op_recv._callback_handler,
					p_encap_http->_bak_op_recv._callback_data._user_data,
					p_encap_http->_bak_op_recv._one_shot);			
			
			if(ret!=SUCCESS)
			{
				*p_need_callback = TRUE;
				return ret;
			}
		}

	}
	else
	{
		// 底层scoket 错误。
		*p_need_callback = TRUE;
	}

	return SUCCESS;
}

_int32 socket_encap_http_client_send_handler(_int32 errcode, _u32 pending_op_count,
	const char* buffer, _u32 had_send, void* user_data)
{
	_int32 ret = SUCCESS;
	_u32 body_data_len = 0;	// 传给上层的字节数。
	BOOL need_callback = FALSE;
	SOCKET_ENCAP_PROP_S *p_encap = NULL;
	SOCKET_ENCAP_HTTP_CLIENT_S *p_encap_http = NULL;
	SOCKET_ENCAP_HTTP_CLIENT_CALLBACK_DATA_S *p_callback_data
		= (SOCKET_ENCAP_HTTP_CLIENT_CALLBACK_DATA_S *)user_data;
	_u32 sock_num = p_callback_data->_sock;

	ret = get_socket_encap_prop_by_sock(sock_num, &p_encap);
	CHECK_VALUE(ret);
	p_encap_http = &(p_encap->_encap_state._http_client);

	ret = socket_encap_http_client_send_handler_impl(errcode, pending_op_count,
		buffer, had_send, user_data, &body_data_len, &need_callback);
	if(ret!=SUCCESS)
	{
		errcode = ret;
	}

	LOG_DEBUG("ENCAP HTTP HANDLE SEND, sock=%d, had_send=%d, body_data_len=%d, need_callback=%d",
		sock_num, had_send, body_data_len, need_callback);

	if (need_callback && p_encap_http->_callback_handler_send)
	{
		p_encap_http->_callback_handler_send(errcode, pending_op_count,
			p_encap_http->_p_send_body_data, body_data_len, p_callback_data->_user_data);
	}
	return ret;
}

_int32 socket_encap_http_client_send_http_get(SOCKET sock, char* buffer, _u32 len,
	socket_proxy_send_handler callback_handler, void* user_data)
{
	_int32 ret = 0;
	_u32 sock_num = sock;
	_u32 sendsize = len;
	SD_SOCKADDR addr={0};
	char tmpIP[16]={0};

	SOCKET_ENCAP_PROP_S *p_encap = NULL;
	SOCKET_ENCAP_HTTP_CLIENT_S *p_encap_http = NULL;

	ret = get_socket_encap_prop_by_sock(sock_num, &p_encap);
	CHECK_VALUE(ret);

	p_encap_http = &(p_encap->_encap_state._http_client);
	if (p_encap_http->_send_type != SOCKET_ENCAP_HTTP_PROTOCAL_SEND_NONE)
	{
		// 当前正在发送一个send，取消本次操作。
		LOG_DEBUG("ENCAP HTTP DO SEND GET(Cancel), sock=%d, size=%d", sock_num, sendsize);
		return SUCCESS;
	}

	LOG_DEBUG("ENCAP HTTP DO SEND GET, sock=%d, size=%d", sock_num, sendsize);

	// 状态初始化
	p_encap_http->_send_type = SOCKET_ENCAP_HTTP_PROTOCAL_SEND_GET;
	p_encap_http->_send_state = SOCKET_ENCAP_STATE_HTTP_CLIENT_HEADER;

	// build send header buf。
	ret = sd_getpeername(sock_num, &addr);
	if(ret!=SUCCESS) return ret;
	sd_assert(addr._sin_addr!=0);
	sd_inet_ntoa(addr._sin_addr, tmpIP, sizeof(tmpIP));
	sd_assert(sd_strlen(tmpIP)!=0);
	socket_encap_http_client_build_http_send_header(tmpIP, addr._sin_port, sendsize, p_encap_http->_send_header_buf, 
		SOCKET_ENCAP_HTTP_CLIENT_SEND_HEADER_BUF_SIZE, &p_encap_http->_send_header_total_len);

	p_encap_http->_sent_header_len = 0;
	p_encap_http->_sent_body_len = 0;
	p_encap_http->_p_send_body_data = NULL;
	p_encap_http->_send_body_data_size = 0;
	p_encap_http->_callback_handler_send = callback_handler;
	p_encap_http->_callback_send_data._sock = sock;
	p_encap_http->_callback_send_data._user_data = user_data;

	LOG_DEBUG("ENCAP HTTP DO SEND_IMPL, sock=%d, need_len=%d", 
		sock_num, p_encap_http->_send_header_total_len);

	return socket_proxy_send_function(sock_num, p_encap_http->_send_header_buf, p_encap_http->_send_header_total_len, 
		socket_encap_http_client_send_handler, (void *)(&p_encap_http->_callback_send_data));

}

_int32 socket_encap_http_client_send(SOCKET sock, const char* buffer, _u32 len,
	socket_proxy_send_handler callback_handler, void* user_data)
{
	_int32 ret = 0;
	_u32 sock_num = sock;
	_u32 sendsize = len;

	SD_SOCKADDR addr={0};
	char tmpIP[16]={0};

	SOCKET_ENCAP_PROP_S *p_encap = NULL;
	SOCKET_ENCAP_HTTP_CLIENT_S *p_encap_http = NULL;

	ret = get_socket_encap_prop_by_sock(sock_num, &p_encap);
	CHECK_VALUE(ret);

	p_encap_http = &(p_encap->_encap_state._http_client);
	
	if (p_encap_http->_send_type == SOCKET_ENCAP_HTTP_PROTOCAL_SEND_GET)
	{
		// 当前正在发出一个send GET，暂存本次操作。
		LOG_DEBUG("ENCAP HTTP DO SEND(Sending GET), sock=%d, size=%d", sock_num, sendsize);

		p_encap_http->_http_protocal_need_send = TRUE;
		p_encap_http->_bak_op_send._buffer = (char *)buffer;
		p_encap_http->_bak_op_send._size = sendsize;
		p_encap_http->_bak_op_send._callback_handler = callback_handler;
		p_encap_http->_bak_op_send._callback_data._sock = sock;
		p_encap_http->_bak_op_send._callback_data._user_data = user_data;
		return SUCCESS;
	}
	else if (p_encap_http->_send_type == SOCKET_ENCAP_HTTP_PROTOCAL_SEND_CALL)
	{
		// 上层不能连发两个SEND请求。
		sd_assert(0);
	}

	LOG_DEBUG("ENCAP HTTP DO SEND, sock=%d, size=%d", sock_num, sendsize);

	// 状态初始化
	p_encap_http->_send_type = SOCKET_ENCAP_HTTP_PROTOCAL_SEND_CALL;
	p_encap_http->_send_state = SOCKET_ENCAP_STATE_HTTP_CLIENT_HEADER;

	// build send header buf。
	ret = sd_getpeername(sock_num, &addr);
	if(ret!=SUCCESS) return ret;
	sd_assert(addr._sin_addr!=0);
	sd_inet_ntoa(addr._sin_addr, tmpIP, sizeof(tmpIP));
	sd_assert(sd_strlen(tmpIP)!=0);
	socket_encap_http_client_build_http_send_header(tmpIP, addr._sin_port, sendsize, p_encap_http->_send_header_buf, 
		SOCKET_ENCAP_HTTP_CLIENT_SEND_HEADER_BUF_SIZE, &p_encap_http->_send_header_total_len);

	p_encap_http->_sent_header_len = 0;
	p_encap_http->_sent_body_len = 0;
	p_encap_http->_p_send_body_data = (char *)buffer;
	p_encap_http->_send_body_data_size = sendsize;
	p_encap_http->_callback_handler_send = callback_handler;
	p_encap_http->_callback_send_data._sock = sock;
	p_encap_http->_callback_send_data._user_data = user_data;

	LOG_DEBUG("ENCAP HTTP DO SEND_IMPL, sock=%d, need_len=%d", 
		sock_num, p_encap_http->_send_header_total_len);

	return socket_proxy_send_function(sock_num, p_encap_http->_send_header_buf, p_encap_http->_send_header_total_len, 
		socket_encap_http_client_send_handler, (void *)(&p_encap_http->_callback_send_data));
}

_int32 socket_encap_http_client_recv_handler_impl(_int32 errcode, _u32 pending_op_count,
	char* buffer, _u32 had_recv, void* user_data, _u32 *p_body_data_len, BOOL *p_need_callback)
{
	_int32 ret = SUCCESS;
	_u32 recv_len = had_recv;
	BOOL need_recv_once_more = FALSE;
	_u32 sock_num = 0;
	SOCKET_ENCAP_HTTP_CLIENT_CALLBACK_DATA_S *p_callback_data = NULL;
	SOCKET_ENCAP_PROP_S *p_encap = NULL;
	SOCKET_ENCAP_HTTP_CLIENT_S *p_encap_http = NULL;

	p_callback_data = (SOCKET_ENCAP_HTTP_CLIENT_CALLBACK_DATA_S *)user_data;
	sock_num = p_callback_data->_sock;
	ret = get_socket_encap_prop_by_sock(sock_num, &p_encap);
	CHECK_VALUE(ret);
	p_encap_http = &(p_encap->_encap_state._http_client);

	*p_body_data_len = 0;
	if (errcode != SUCCESS)
	{
		*p_need_callback = TRUE;
		return SUCCESS;
	}

	if (p_encap_http->_recv_state == SOCKET_ENCAP_STATE_HTTP_CLIENT_HEADER)
	{
		_u32 header_len = 0;
		_u32 body_len = 0;
		p_encap_http->_recv_keyword_remain_in_buf += recv_len;

		ret = socket_encap_http_client_get_body_from_header_buf(
			p_encap_http->_recv_header_buf,
			p_encap_http->_recv_keyword_remain_in_buf, 
			p_encap_http->_p_recv_body_data + p_encap_http->_recv_body_len, 
			p_encap_http->_recv_body_data_size - p_encap_http->_recv_body_len,

			&p_encap_http->_recv_body_remain_in_buf,
			&p_encap_http->_recv_http_body_total,
			&p_encap_http->_recv_http_body_remain,

			&header_len,
			&body_len,
			&p_encap_http->_recv_keyword_remain_in_buf);

		if (ret != SUCCESS)
		{
			return ret;
		}

		p_encap_http->_recv_body_len += body_len;
		if (p_encap_http->_recv_body_len == p_encap_http->_recv_body_data_size)
		{
			// 上层的请求body已全部收到。
			if (p_encap_http->_recv_keyword_remain_in_buf == 0
				&& p_encap_http->_recv_http_body_remain == 0)
			{
				// buf 中没有下一个header了，即所有数据都已收完。
				p_encap_http->_recv_enable = FALSE;
			}
		}
		else
		{
			// 上层的请求body还没有全部收到。此时 header buf 中没有 body 了。
			if (p_encap_http->_recv_keyword_remain_in_buf == 0
				&& p_encap_http->_recv_http_body_remain > 0)
			{
				p_encap_http->_recv_state = SOCKET_ENCAP_STATE_HTTP_CLIENT_BODY;
				p_encap_http->_p_recv_data = p_encap_http->_p_recv_body_data + p_encap_http->_recv_body_len;
				p_encap_http->_recv_data_size = MIN(
					p_encap_http->_recv_http_body_remain,
					p_encap_http->_recv_body_data_size - p_encap_http->_recv_body_len);

				need_recv_once_more = TRUE;
			}
			else
			{
				p_encap_http->_p_recv_data = p_encap_http->_recv_header_buf + p_encap_http->_recv_keyword_remain_in_buf;
				p_encap_http->_recv_data_size = SOCKET_ENCAP_HTTP_CLIENT_HEADER_BUF_SIZE 
					- p_encap_http->_recv_keyword_remain_in_buf;
				need_recv_once_more = TRUE;
			}
		}

	}
	else if (p_encap_http->_recv_state == SOCKET_ENCAP_STATE_HTTP_CLIENT_BODY)
	{
		sd_assert(recv_len > 0 && recv_len <= p_encap_http->_recv_http_body_remain);
		p_encap_http->_recv_body_len += recv_len;
		p_encap_http->_recv_http_body_remain -= recv_len;

		// 判断上层的请求body已全部收到?
		if (p_encap_http->_recv_body_len == p_encap_http->_recv_body_data_size)
		{
			// http包中所有body是否都已收完?
			if (p_encap_http->_recv_http_body_remain == 0)
			{
				p_encap_http->_recv_state = SOCKET_ENCAP_STATE_HTTP_CLIENT_HEADER;
				p_encap_http->_recv_enable = FALSE;
			}
		}
		else
		{
			// 上层的请求body还没有全部收到。
			if (p_encap_http->_recv_http_body_remain == 0)
			{
				// http body 已全部收完，上层还需要的数据，须再解析header后获取.
				sd_assert(p_encap_http->_recv_keyword_remain_in_buf == 0);

				p_encap_http->_recv_enable = FALSE; // http server可能不会再发包了。
				p_encap_http->_recv_state = SOCKET_ENCAP_STATE_HTTP_CLIENT_HEADER;
				p_encap_http->_p_recv_data = p_encap_http->_recv_header_buf
					+ p_encap_http->_recv_keyword_remain_in_buf;
				p_encap_http->_recv_data_size = SOCKET_ENCAP_HTTP_CLIENT_HEADER_BUF_SIZE 
					- p_encap_http->_recv_keyword_remain_in_buf;
			}
			else
			{
				p_encap_http->_p_recv_data = p_encap_http->_p_recv_body_data + p_encap_http->_recv_body_len;
				p_encap_http->_recv_data_size = MIN(
					p_encap_http->_recv_http_body_remain,
					p_encap_http->_recv_body_data_size - p_encap_http->_recv_body_len);
			}
			need_recv_once_more = TRUE;
		}
	}
	else
	{
		sd_assert(0);
	}

	*p_body_data_len = p_encap_http->_recv_body_len;
	sd_assert(p_encap_http->_recv_body_len <= p_encap_http->_recv_body_data_size);

	if (p_encap_http->_recv_one_shot && p_encap_http->_recv_body_len > 0)
	{
		// one shot
		need_recv_once_more = FALSE;
	}

	if (need_recv_once_more)
	{
		// 没有收完，继续接收。
		BOOL recv_impl_is_one_shot = (p_encap_http->_recv_state == SOCKET_ENCAP_STATE_HTTP_CLIENT_HEADER)?TRUE: p_encap_http->_recv_one_shot;

		LOG_DEBUG("ENCAP HTTP DO RECV_IMPL ONCE MORE, sock=%d, done_len=%d, need_len=%d, recv_impl_is_one_shot=%d", 
			sock_num, *p_body_data_len, p_encap_http->_recv_data_size, recv_impl_is_one_shot);

		if (! p_encap_http->_recv_enable)
		{
			// 不能RECV （之前没有SEND操作），发一个HTTP GET 先。
			LOG_DEBUG("ENCAP HTTP RECV_IMPL ONCE MORE (Send GET First)");

			// 不暂存recv操作。
			ret = socket_encap_http_client_send_http_get(p_callback_data->_sock, NULL, 0, 
				NULL, p_callback_data->_user_data);
		}

		ret = socket_proxy_recv_function(sock_num,
			p_encap_http->_p_recv_data,
			p_encap_http->_recv_data_size,
			socket_encap_http_client_recv_handler,
			(void *)(&p_encap_http->_callback_recv_data),
			recv_impl_is_one_shot);
		
		CHECK_VALUE(ret);

		*p_need_callback = FALSE;
		return SUCCESS;
	}

	*p_need_callback = TRUE;
	return SUCCESS;
}

_int32 socket_encap_http_client_recv_handler(_int32 errcode, _u32 pending_op_count,
	char* buffer, _u32 had_recv, void* user_data)
{
	_int32 ret = SUCCESS;
	_u32 body_data_len = 0;	// 传给上层的字节数。
	BOOL need_callback = FALSE;
	SOCKET_ENCAP_PROP_S *p_encap = NULL;
	SOCKET_ENCAP_HTTP_CLIENT_S *p_encap_http = NULL;
	SOCKET_ENCAP_HTTP_CLIENT_CALLBACK_DATA_S *p_callback_data
		= (SOCKET_ENCAP_HTTP_CLIENT_CALLBACK_DATA_S *)user_data;
	_u32 sock_num = p_callback_data->_sock;

	ret = get_socket_encap_prop_by_sock(sock_num, &p_encap);
	CHECK_VALUE(ret);
	p_encap_http = &(p_encap->_encap_state._http_client);

	ret = socket_encap_http_client_recv_handler_impl(errcode, pending_op_count,
		buffer, had_recv, user_data, &body_data_len, &need_callback);

	LOG_DEBUG("ENCAP HTTP HANDLE RECV, sock=%d, had_recv=%d, body_data_len=%d, need_callback=%d",
		sock_num, had_recv, body_data_len, need_callback);

	if (need_callback)
	{
		p_encap_http->_callback_handler_recv(errcode, pending_op_count,
			p_encap_http->_p_recv_body_data, body_data_len, p_callback_data->_user_data);
	}
	return ret;
}



_int32 socket_encap_http_client_recv(SOCKET sock, char* buffer, _u32 len,
	socket_proxy_recv_handler callback_handler, void* user_data)
{
	return socket_encap_http_client_recv_impl(sock, buffer, len,
		callback_handler, user_data, FALSE);
}

_int32 socket_encap_http_client_uncomplete_recv(SOCKET sock, char* buffer, _u32 len,
	socket_proxy_recv_handler callback_handler, void* user_data)
{
	return socket_encap_http_client_recv_impl(sock, buffer, len,
		callback_handler, user_data, TRUE);
}
