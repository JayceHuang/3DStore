#include "asyn_frame/device_handler.h"
#include "utility/define.h"
#include "utility/errcode.h"
#include "utility/string.h"
#include "platform/sd_socket.h"
#include "platform/sd_fs.h"
#include "utility/sha1.h"

#ifdef _ENABLE_SSL
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

#define LOGID LOGID_DEVICE_HANDLER
#include "utility/logger.h"

#if defined(WINCE)
#define SEND_PIECE_BYTES        (1024)
#endif

/* socket recved bytes */
static _u32 g_socket_tcp_device_recved_bytes = 0;
static _u32 g_socket_udp_device_recved_bytes = 0;

static _u32 g_socket_tcp_device_send_bytes = 0;
static _u32 g_socket_udp_device_send_bytes = 0;

static _u64 upper_limit = 4294967296ull;

typedef _int32(*op_handler)(MSG_INFO *info, BOOL *completed);

_int32 op_null_handler(MSG_INFO *info, BOOL *completed);

_int32 op_accept_handler(MSG_INFO *info, BOOL *completed);

_int32 op_recv_handler(MSG_INFO *info, BOOL *completed);
_int32 op_send_handler(MSG_INFO *info, BOOL *completed);

_int32 op_recvfrom_handler(MSG_INFO *info, BOOL *completed);
_int32 op_sendto_handler(MSG_INFO *info, BOOL *completed);

_int32 op_conn_handler(MSG_INFO *info, BOOL *completed);
_int32 op_proxy_conn_handler(MSG_INFO *info, BOOL *completed);

_int32 op_open_handler(MSG_INFO *info, BOOL *completed);
_int32 op_read_handler(MSG_INFO *info, BOOL *completed);
_int32 op_write_handler(MSG_INFO *info, BOOL *completed);

_int32 op_close_handler(MSG_INFO *info, BOOL *completed);

_int32 op_calc_hash_handler(MSG_INFO* info, BOOL* completed);

#if defined(LINUX)
#if defined(_ANDROID_LINUX) || defined( MACOS)
static void* g_handler_table[] =
{
	(void*)op_proxy_conn_handler,
	(void*)op_accept_handler,
	(void*)op_recv_handler,
	(void*)op_send_handler,
	(void*)op_recvfrom_handler,
	(void*)op_sendto_handler,
	(void*)op_conn_handler,
	(void*)op_open_handler,
	(void*)op_read_handler,
	(void*)op_write_handler,
	(void*)op_close_handler,
    (void*)op_calc_hash_handler
};
#else
static void* g_handler_table[] =
{
	op_null_handler,
	op_accept_handler,
	op_recv_handler,
	op_send_handler,
	op_recvfrom_handler,
	op_sendto_handler,
	op_conn_handler,
	op_open_handler,
	op_read_handler,
	op_write_handler,
	op_close_handler,
    op_calc_hash_handler
};
#endif
#elif defined(WINCE)
static void* g_handler_table[] =
{
	(void*)op_proxy_conn_handler,
	(void*)op_accept_handler,
	(void*)op_recv_handler,
	(void*)op_send_handler,
	(void*)op_recvfrom_handler,
	(void*)op_sendto_handler,
	(void*)op_conn_handler,
	(void*)op_open_handler,
	(void*)op_read_handler,
	(void*)op_write_handler,
	(void*)op_close_handler,
    (void*)op_calc_hash_handler
};
#else
static void* g_handler_table[] =
{
	op_null_handler,
	op_accept_handler,
	op_recv_handler,
	op_send_handler,
	op_recvfrom_handler,
	op_sendto_handler,
	op_conn_handler,
	op_open_handler,
	op_read_handler,
	op_write_handler,
	op_close_handler,
    op_calc_hash_handler
};
#endif

_int32 op_calc_hash_handler(MSG_INFO *info, BOOL *completed)
{
    _int32 ret_val = SUCCESS;
    OP_PARA_CACL_HASH* para = (OP_PARA_CACL_HASH*)info->_operation_parameter;
    ctx_sha1 ctx;
    sha1_initialize(&ctx);
    sha1_update(&ctx, para->_data_buffer, para->_data_len);
    sha1_finish(&ctx, para->_hash_result);
    *completed = TRUE;
    return ret_val;
}

_int32 op_null_handler(MSG_INFO *info, BOOL *completed)
{
	*completed = TRUE;
	return SUCCESS;
}

_int32 op_accept_handler(MSG_INFO *info, BOOL *completed)
{
	_int32 ret_val = SUCCESS;
	OP_PARA_ACCEPT *para = (OP_PARA_ACCEPT*)info->_operation_parameter;

	ret_val = sd_accept(info->_device_id, (_u32*)&para->_socket, &para->_addr);
	if (ret_val == WOULDBLOCK)
	{
		*completed = FALSE;
		return ret_val;
	}

	*completed = TRUE;
	return ret_val;
}

_int32 op_recv_handler(MSG_INFO *info, BOOL *completed)
{
	_int32 ret_val = SUCCESS, op_size = 0;

	OP_PARA_CONN_SOCKET_RW *para = (OP_PARA_CONN_SOCKET_RW*)info->_operation_parameter;

	if (para->_operated_size >= para->_expect_size || (para->_oneshot && para->_operated_size > 0))
	{
		*completed = TRUE;
		return ret_val;
	}
#ifdef _ENABLE_SSL
	if (SSL_MAGICNUM == info->_ssl_magicnum)
	{
		BIO* pbio = info->_pbio;
		ret_val = BIO_read(pbio, para->_buffer + para->_operated_size, para->_expect_size - para->_operated_size);
		if (ret_val > 0)
		{
			op_size = ret_val;
			para->_operated_size += op_size;
			g_socket_tcp_device_recved_bytes += op_size;
            LOG_DEBUG("op_recv_handler, op_size = %d", op_size);
			if (para->_oneshot || para->_operated_size >= para->_expect_size)
			{
				*completed = TRUE;
			}
			else
			{
				*completed = FALSE;
			}
			return SUCCESS;
		}
		else if (ret_val == 0)
		{
			*completed = TRUE;
			return SOCKET_CLOSED;
		}
		else
		{
			if (BIO_should_retry(pbio))
			{
				*completed = FALSE;
				return SUCCESS;
			}
			*completed = TRUE;
			ret_val = (_int32)ERR_peek_last_error();
			LOG_DEBUG("ssl recv error: %d", ret_val);
			return ret_val;
		}
	}
	else
#endif
	{
		do
		{
			ret_val = sd_recv(info->_device_id, para->_buffer + para->_operated_size, 
				para->_expect_size - para->_operated_size, &op_size);
			para->_operated_size += op_size;

			/* statics info */
			g_socket_tcp_device_recved_bytes += op_size;
            LOG_DEBUG("op_recv_handler, op_size = %d", op_size);


			if (para->_oneshot && op_size > 0)
			{
				*completed = TRUE;
				return ret_val;
			}

			if (ret_val == SUCCESS && op_size == 0)
			{
				/* closed graceful */
				*completed = TRUE;
				return SOCKET_CLOSED;
			}

			if (ret_val == WOULDBLOCK)
			{
				*completed = FALSE;
				return ret_val;
			}

		} while ((ret_val == SUCCESS && para->_operated_size < para->_expect_size));

		*completed = TRUE;
	}

	return ret_val;
}

_int32 op_send_handler(MSG_INFO *info, BOOL *completed)
{
#if defined(LINUX)
	_int32 ret_val = SUCCESS, op_size = 0;

	OP_PARA_CONN_SOCKET_RW *para = (OP_PARA_CONN_SOCKET_RW*)info->_operation_parameter;

	if (para->_operated_size >= para->_expect_size || (para->_oneshot && para->_operated_size > 0))
	{
		*completed = TRUE;
		return ret_val;
	}
#ifdef _ENABLE_SSL
	if (SSL_MAGICNUM == info->_ssl_magicnum)
	{
		BIO* pbio = info->_pbio;
		ret_val = BIO_write(pbio, para->_buffer + para->_operated_size, para->_expect_size - para->_operated_size);
		if (ret_val > 0)
		{
			op_size = ret_val;
			para->_operated_size += op_size;
			g_socket_tcp_device_send_bytes += op_size;
            LOG_DEBUG("op_send_handler, op_size = %d", op_size);
			if (para->_oneshot || para->_operated_size >= para->_expect_size)
			{
				*completed = TRUE;
			}
			else
			{
				*completed = FALSE;
			}
			return SUCCESS;
		}
		else if (ret_val == 0)
		{
			*completed = TRUE;
			return SOCKET_CLOSED;
		}
		else
		{
			if (BIO_should_retry(pbio))
			{
				*completed = FALSE;
				return SUCCESS;
			}
			*completed = TRUE;
			ret_val = (_int32)ERR_peek_last_error();
			LOG_DEBUG("ssl send error: %d", ret_val);
			return ret_val;
		}
	}
	else
#endif
	{
		do
		{
			ret_val = sd_send(info->_device_id, para->_buffer + para->_operated_size,
				para->_expect_size - para->_operated_size, &op_size);
			para->_operated_size += op_size;


			/* statics */
			g_socket_tcp_device_send_bytes += op_size;
            LOG_DEBUG("op_send_handler, op_size = %d", op_size);

			if (para->_oneshot && op_size > 0)
			{
				*completed = TRUE;
				return ret_val;
			}

			if (ret_val == WOULDBLOCK)
			{
				*completed = FALSE;
				return ret_val;
			}

		} while ((ret_val == SUCCESS && para->_operated_size < para->_expect_size));

		*completed = TRUE;
	}
#elif defined(WINCE)
	_int32 ret_val = SUCCESS, op_size = 0;

	OP_PARA_CONN_SOCKET_RW *para = (OP_PARA_CONN_SOCKET_RW*)info->_operation_parameter;

	if(para->_operated_size >= para->_expect_size || (para->_oneshot && para->_operated_size > 0))
	{
		*completed = TRUE;
		return ret_val;
	}

	do
	{
		ret_val = sd_send(info->_device_id, para->_buffer + para->_operated_size,
			para->_expect_size - para->_operated_size, &op_size);
		para->_operated_size += op_size;


		/* statics */
		g_socket_tcp_device_send_bytes += op_size;
        LOG_DEBUG("op_send_handler, op_size = %d", op_size);

		if(para->_oneshot && op_size > 0)
		{
			*completed = TRUE;
			return ret_val;
		}

		if(ret_val == WOULDBLOCK)
		{
			*completed = FALSE;
			return ret_val;
		}

	}
	while((ret_val == SUCCESS && para->_operated_size < para->_expect_size));

	*completed = TRUE;
#else
	_int32 ret_val = SUCCESS, op_size = 0;

	OP_PARA_CONN_SOCKET_RW *para = (OP_PARA_CONN_SOCKET_RW*)info->_operation_parameter;

	if(para->_operated_size >= para->_expect_size || (para->_oneshot && para->_operated_size > 0))
	{
		*completed = TRUE;
		return ret_val;
	}

	do
	{
		ret_val = sd_send(info->_device_id, para->_buffer + para->_operated_size,
			para->_expect_size - para->_operated_size, &op_size);
		para->_operated_size += op_size;


		/* statics */
		g_socket_tcp_device_send_bytes += op_size;
        LOG_DEBUG("op_send_handler, op_size = %d", op_size);

		if(para->_oneshot && op_size > 0)
		{
			*completed = TRUE;
			return ret_val;
		}

		if(ret_val == WOULDBLOCK)
		{
			*completed = FALSE;
			return ret_val;
		}

	}
	while((ret_val == SUCCESS && para->_operated_size < para->_expect_size));

	*completed = TRUE;
#endif

	return ret_val;
}

_int32 op_recvfrom_handler(MSG_INFO *info, BOOL *completed)
{
	/* one-shot */
	_int32 ret_val = SUCCESS, op_size = 0;

	OP_PARA_NOCONN_SOCKET_RW *para = (OP_PARA_NOCONN_SOCKET_RW*)info->_operation_parameter;

	if (para->_operated_size > 0)
	{
		*completed = TRUE;
		return ret_val;
	}

	ret_val = sd_recvfrom(info->_device_id, para->_buffer, para->_expect_size, &para->_sockaddr, &op_size);
	para->_operated_size = op_size;

	/* statics */
	g_socket_udp_device_recved_bytes += op_size;
    LOG_DEBUG("op_recvfrom_handler, op_size = %d", op_size);

	if (op_size == 0 && (ret_val == WOULDBLOCK))
	{
		*completed = FALSE;
		return ret_val;
	}

	*completed = TRUE;
	return ret_val;
}

_int32 op_sendto_handler(MSG_INFO *info, BOOL *completed)
{
	/* one-shot */
	_int32 ret_val = SUCCESS, op_size = 0;

	OP_PARA_NOCONN_SOCKET_RW *para = (OP_PARA_NOCONN_SOCKET_RW*)info->_operation_parameter;

	if (para->_operated_size > 0)
	{
		*completed = TRUE;
		return ret_val;
	}

	ret_val = sd_sendto(info->_device_id, para->_buffer, para->_expect_size, &para->_sockaddr, &op_size);
	para->_operated_size = op_size;

	/* statics */
	g_socket_udp_device_send_bytes += op_size;
    LOG_DEBUG("op_sendto_handler, op_size = %d", op_size);

	if (op_size == 0 && (ret_val == WOULDBLOCK))
	{
		*completed = FALSE;
		return ret_val;
	}

	*completed = TRUE;
	return ret_val;
}

_int32 op_conn_handler(MSG_INFO *info, BOOL *completed)
{
	_int32 ret_val = SUCCESS;
	SD_SOCKADDR *paddr = (SD_SOCKADDR*)info->_operation_parameter;

#ifdef _ENABLE_SSL
	if (info->_ssl_magicnum == SSL_MAGICNUM)
	{
		BIO* pbio = (BIO*)info->_pbio;
		ret_val = BIO_do_connect(pbio);
		if (ret_val <= 0)
		{
			*completed = FALSE;
			if (BIO_should_retry(pbio))
			{
				return SUCCESS;
			}
			*completed = TRUE;
			ret_val = (_int32)ERR_peek_last_error();
			LOG_DEBUG("ssl handshake error: %d", ret_val);
			return ret_val;
		}
		else
		{
			*completed = TRUE;
			return SUCCESS;
		}
	}
#endif
	if (*completed)
	{
		/* first connect */
		ret_val = sd_connect(info->_device_id, paddr);
		if (ret_val == WOULDBLOCK)
		{
			*completed = FALSE;
			return ret_val;
		}

		*completed = TRUE;
	}
	else
	{
		/* check connected? can not be EINPROGRESS because epoll returned? */
		ret_val = get_socket_error(info->_device_id);

		/*
				if(ret_val == EINPROGRESS)
				{
				*completed = FALSE;
				return WOULDBLOCK;
				}
				*/

		*completed = TRUE;
	}

	return ret_val;
}
_int32 op_proxy_conn_handler(MSG_INFO *info, BOOL *completed)
{
	_int32 ret_val = SUCCESS;
	PROXY_CONNECT_DNS *pdata = (PROXY_CONNECT_DNS*)info->_user_data;
	OP_PARA_DNS *paddr = (OP_PARA_DNS*)info->_operation_parameter;

	if (*completed)
	{
		/* first connect */
		ret_val = sd_asyn_proxy_connect(info->_device_id, paddr->_host_name, pdata->_port, NULL, NULL);
		if (ret_val == WOULDBLOCK)
		{
			*completed = FALSE;
			return ret_val;
		}

		*completed = TRUE;
	}
	else
	{
		/* check connected? can not be EINPROGRESS because epoll returned? */
		ret_val = get_socket_error(info->_device_id);

		/*
				if(ret_val == EINPROGRESS)
				{
				*completed = FALSE;
				return WOULDBLOCK;
				}
				*/

		*completed = TRUE;
	}

	return ret_val;
}

#ifndef ENABLE_ETM_MEDIA_CENTER
_int32 op_open_handler(MSG_INFO * info, BOOL * completed)
{
	_int32 ret_val = SUCCESS;
	_u64 disk_space = 0;
	_int32 flag = 0;
	BOOL is_support_big_file = TRUE;
	OP_PARA_FS_OPEN *para = (OP_PARA_FS_OPEN*)info->_operation_parameter;

	LOG_DEBUG("op_open_handler, filesize = %lld", para->_file_size);

	if(para->_open_option & O_DEVICE_FS_CREATE)
		flag |= O_FS_CREATE;

	info->_device_id = INVALID_FILE_ID;
	ret_val = sd_open_ex(para->_filepath, flag, &info->_device_id);
	if(ret_val == SUCCESS && para->_file_size > 0)
	{
		LOG_DEBUG("sd_open_ex success 111");

		ret_val = sd_filesize(info->_device_id, &para->_cur_file_size);
		if(ret_val == SUCCESS )
		{
			LOG_DEBUG("sd_filesize success 111");

			if(para->_file_size > upper_limit)
			{
				LOG_ERROR("para->_file_size = %lld, > %lld", para->_file_size, upper_limit);
				sd_is_support_create_big_file(para->_filepath, &is_support_big_file);
				if(!is_support_big_file)
				{
					LOG_DEBUG("this disk format do not support create big file");
					sd_close_ex(info->_device_id);
					info->_device_id = INVALID_FILE_ID;
					ret_val = FILE_TOO_BIG;
					sd_delete_file(para->_filepath);
					return ret_val;
				}
			}    

			if(sd_get_free_disk(para->_filepath, &disk_space) == SUCCESS &&
				(para->_file_size > para->_cur_file_size) &&
				(disk_space < ((para->_file_size - para->_cur_file_size) / 1024)+MIN_FREE_DISK_SIZE))
			{
				LOG_ERROR("insufficient disk space(free: %llu KB, while need %u KB)",
					disk_space, para->_file_size / 1024);
				sd_close_ex(info->_device_id);
				info->_device_id = INVALID_FILE_ID;
				ret_val = INSUFFICIENT_DISK_SPACE;
				return ret_val;
			}

			sd_close_ex(info->_device_id);
			info->_device_id = INVALID_FILE_ID;
			ret_val = sd_open_ex(para->_filepath, flag, &info->_device_id );
			if(ret_val == SUCCESS)
			{
				LOG_DEBUG("sd_open_ex success 222");
				para->_cur_file_size = para->_file_size;
			}
			else
			{
				LOG_DEBUG("sd_open_ex fail 222");
			}

		}
		else
		{
			sd_close_ex(info->_device_id);
			info->_device_id = INVALID_FILE_ID;
			LOG_DEBUG("sd_filesize fail 111");
		}
	}
	else
	{
		LOG_DEBUG("sd_open_ex fail or file_size == 0");
	}
	return ret_val;
}
#else

_int32 op_open_handler(MSG_INFO *info, BOOL *completed)
{
	_int32 ret_val = SUCCESS, flag = 0;
	_u64 disk_space = 0;
	OP_PARA_FS_OPEN *para = (OP_PARA_FS_OPEN*)info->_operation_parameter;

	LOG_DEBUG("op_open_handler");

	if (*completed)
	{
		if (para->_open_option & O_DEVICE_FS_CREATE)
			flag |= O_FS_CREATE;

		info->_device_id = INVALID_FILE_ID;
		ret_val = sd_open_ex(para->_filepath, flag, &info->_device_id);

		if (ret_val == SUCCESS && para->_file_size > 0)
		{
			ret_val = sd_filesize(info->_device_id, &para->_cur_file_size);
			if (ret_val != SUCCESS)
			{
				LOG_ERROR("get filesize of %s(fileid:%d) failed", para->_filepath, info->_device_id);
				sd_close_ex(info->_device_id);
				info->_device_id = INVALID_FILE_ID;
			}
			else if (sd_get_free_disk(para->_filepath, &disk_space) == SUCCESS &&
				(para->_file_size > para->_cur_file_size) &&
				(disk_space - 500) < (para->_file_size - para->_cur_file_size) / 1024)
			{
				LOG_ERROR("insufficient disk space(free: %u KB, while need %u KB)",
					disk_space, para->_file_size / 1024);
				sd_close_ex(info->_device_id);
				info->_device_id = INVALID_FILE_ID;

				ret_val = INSUFFICIENT_DISK_SPACE;
			}
		}
	}

	LOG_DEBUG("op_open_handler: open ret_val=%d", ret_val);

	*completed = TRUE;
	if (ret_val == SUCCESS && para->_file_size > 0)
	{
		if (para->_cur_file_size > para->_file_size)
		{
			LOG_ERROR("the current filesize(%llu) is bigger than requested filesize(%llu), failed",
				para->_cur_file_size, para->_file_size);

			sd_close_ex(info->_device_id);
			info->_device_id = INVALID_FILE_ID;
			ret_val = FILE_CANNOT_TRUNCATE;
		}
		else if (para->_cur_file_size < para->_file_size)
		{
			ret_val = sd_enlarge_file(info->_device_id, para->_file_size, &para->_cur_file_size);
			if (ret_val == SUCCESS)
			{
				if (para->_cur_file_size < para->_file_size)
				{
					*completed = FALSE;
				}
			}
		}
	}

	LOG_DEBUG("op_open_handler: ret_val=%d", ret_val);

	return ret_val;
}
#endif
_int32 op_read_handler(MSG_INFO *info, BOOL *completed)
{
	_int32 ret_val = SUCCESS;
	_u32 op_size = 0;
	OP_PARA_FS_RW *para = (OP_PARA_FS_RW*)info->_operation_parameter;

	if (para->_operated_size >= para->_expect_size)
	{
		*completed = TRUE;
		return ret_val;
	}

	do
	{
		ret_val = sd_pread(info->_device_id, para->_buffer + para->_operated_size,
			para->_expect_size - para->_operated_size, para->_operating_offset + para->_operated_size, &op_size);
		para->_operated_size += op_size;

		if (ret_val == SUCCESS && op_size == 0)
		{
			/* closed graceful */
			*completed = TRUE;
			return END_OF_FILE;
		}

	} while ((ret_val == SUCCESS && para->_operated_size < para->_expect_size));

	LOG_DEBUG("succeed in reading file(id:%d), and get %u bytes from offset %llu while expect %u bytes",
		info->_device_id, para->_operated_size, para->_operating_offset, para->_expect_size);

	*completed = TRUE;
	return ret_val;
}

_int32 op_write_handler(MSG_INFO *info, BOOL *completed)
{
	_int32 ret_val = SUCCESS;
	_u32 op_size = 0;
	OP_PARA_FS_RW *para = (OP_PARA_FS_RW*)info->_operation_parameter;

	if (para->_operated_size >= para->_expect_size)
	{
		*completed = TRUE;
		return ret_val;
	}

	do
	{
		ret_val = sd_pwrite(info->_device_id, para->_buffer + para->_operated_size,
			para->_expect_size - para->_operated_size, para->_operating_offset + para->_operated_size, &op_size);
		para->_operated_size += op_size;
	} while ((ret_val == SUCCESS && para->_operated_size < para->_expect_size));

	LOG_DEBUG("succeed in writing file(id:%d), and get %u bytes from offset %llu while expect %u bytes, ret_val=%d",
		info->_device_id, para->_operated_size, para->_operating_offset, para->_expect_size, ret_val);


	*completed = TRUE;
	return ret_val;

}

void op_asyn_close_file_handler(void *arglist)
{
	_int32 ret_val = SUCCESS;
	_u32 file_id = (_u32)arglist;
#if 0 //defined(_DEBUG)
	_u32 old_time= 0,new_time = 0;
	sd_time_ms(&old_time);
	printf("\n **************************\n op_asyn_close_file_handler start:%u,task_id =%u\n",old_time,file_id);
#endif

	sd_pthread_detach();
	sd_ignore_signal();
	ret_val = sd_close_ex(file_id);

#if 0 //defined(_DEBUG)
	sd_time_ms(&new_time);
	printf("\n op_asyn_close_file_handler end:%u,use time =%u,ret_val=%d \n**************************\n",
		new_time,new_time-old_time,ret_val);
#endif
	return;
}
_int32 op_asyn_close_file(_u32 file_id)
{
	_int32 thread_id = 0, ret_val = SUCCESS;

	ret_val = sd_create_task(op_asyn_close_file_handler, 1024, (void*)file_id, &thread_id);
	CHECK_VALUE(ret_val);

	return SUCCESS;
}

_int32 op_close_handler(MSG_INFO *info, BOOL *completed)
{
	_int32 ret_val = SUCCESS;

#if defined(MACOS) ||defined(_ANDROID_LINUX)
	/* IPAD MACOS  中另起线程关闭任务文件*/
	ret_val = op_asyn_close_file(info->_device_id);
#else
	ret_val = sd_close_ex(info->_device_id);
#endif
	*completed = TRUE;
	return ret_val;
}


_int32 handle_msg(MSG *msg, BOOL *completed)
{
	MSG_INFO *pmsg_info = &msg->_msg_info;
	if (pmsg_info->_operation_type == OP_COMMON)
	{
		return SUCCESS;
	}

	_u32 fun_index = (DEVICE_MASK & (pmsg_info->_operation_type));
	if (fun_index > OP_MAX)
	{
		return INVALID_OPERATION_TYPE;
	}

	return ((op_handler)g_handler_table[fun_index - 1])(pmsg_info, completed);
}

_u32 socket_tcp_device_recved_bytes(void)
{
	return g_socket_tcp_device_recved_bytes;
}

_u32 socket_udp_device_recved_bytes(void)
{
	return g_socket_udp_device_recved_bytes;
}

_u32 socket_tcp_device_send_bytes(void)
{
	return g_socket_tcp_device_send_bytes;
}

_u32 socket_udp_device_send_bytes(void)
{
	return g_socket_udp_device_send_bytes;
}

_int32 get_network_flow(_u32 * download, _u32 * upload)
{
	if (download != NULL)
	{
		*download = socket_tcp_device_recved_bytes() + socket_udp_device_recved_bytes();
	}

	if (upload != NULL)
	{
		*upload = socket_tcp_device_send_bytes() + socket_udp_device_send_bytes();
	}
	return SUCCESS;
}

