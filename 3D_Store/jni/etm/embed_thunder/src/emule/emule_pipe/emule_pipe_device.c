
#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
#include "emule_pipe_device.h"
#include "emule_pipe_wait_accept.h"
#include "../emule_server/emule_server_interface.h"
#include "../emule_server/emule_server_impl.h"
#include "../emule_interface.h"
#include "../emule_impl.h"
#include "emule_pipe_impl.h"
#include "../emule_data_manager/emule_data_manager.h"
#include "../emule_utility/emule_opcodes.h"
#include "../emule_utility/emule_define.h"
#include "../emule_utility/emule_utility.h"
#include "socket_proxy_interface.h"
#include "utility/mempool.h"
#include "utility/utility.h"
#include "utility/bytebuffer.h"
#include "utility/string.h"
//#include "utility/zlib.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"

#include "utility/list.h"

#include "emule_pipe_upload.h"

#define LAGE_DATA_BUFFER_SIZE (1*1024)
#define LAGE_DATA_BUFFER_MAX_NUM 3
#define LAGE_BUFFER_NUM_RESET_TICKS 100


static _int32 g_large_data_buffer_num = 0;
static  LIST g_pipe_device_user;

static _int32 g_sending_ticks = 0;


void emule_pipe_device_module_init()
{
    g_large_data_buffer_num = 0;
    g_sending_ticks = 0;
    list_init(&g_pipe_device_user);
}

void emule_pipe_device_module_uninit()
{
    g_large_data_buffer_num = 0;
    g_sending_ticks = 0;
    list_clear(&g_pipe_device_user);
}

static	void*	g_emule_pipe_device_callback_table[6] = 
{
	emule_pipe_device_connect_callback, 
	emule_pipe_device_send_callback,
	emule_pipe_device_recv_cmd_callback,
	emule_pipe_device_recv_data_callback,
	emule_pipe_device_close_callback,
	emule_pipe_device_notify_error
};

void** emule_pipe_device_get_callback_table(void)
{
	return g_emule_pipe_device_callback_table;
}

_int32 emule_pipe_device_create(EMULE_PIPE_DEVICE** pipe_device)
{
	_int32 ret = SUCCESS;
	ret = sd_malloc(sizeof(EMULE_PIPE_DEVICE), (void**)pipe_device);
	if(ret != SUCCESS)
		return ret;
	sd_memset(*pipe_device, 0, sizeof(EMULE_PIPE_DEVICE));
	(*pipe_device)->_pipe = NULL;
	(*pipe_device)->_get_buffer_timeout_id = INVALID_MSG_ID;
	(*pipe_device)->_valid_data = FALSE;
	(*pipe_device)->_passive = FALSE;
	init_speed_calculator(&(*pipe_device)->_download_speed, 0, 0);
	return ret;
}

_int32 emule_pipe_device_close(EMULE_PIPE_DEVICE* pipe_device)
{
	LOG_DEBUG("[emule_pipe_device = 0x%x]emule_pipe_device close.", pipe_device);
	if(pipe_device->_get_buffer_timeout_id != INVALID_MSG_ID)
	{
		cancel_timer(pipe_device->_get_buffer_timeout_id);
		pipe_device->_get_buffer_timeout_id = INVALID_MSG_ID;
	}

	if(pipe_device->_socket_device != NULL)
		return emule_socket_device_close(pipe_device->_socket_device);
	else
		return emule_pipe_device_destroy(pipe_device);
}

_int32 emule_pipe_device_close_callback(void* user_data)
{
	EMULE_PIPE_DEVICE* pipe_device = (EMULE_PIPE_DEVICE*)user_data;
	return emule_pipe_device_destroy(pipe_device);
}

_int32 emule_pipe_device_destroy(EMULE_PIPE_DEVICE* pipe_device)
{
	if(pipe_device->_socket_device != NULL)
		emule_socket_device_destroy(&pipe_device->_socket_device);

	if(pipe_device->_recv_buffer != NULL)
	{
		SAFE_DELETE(pipe_device->_recv_buffer);
		pipe_device->_recv_buffer_len = 0;
		pipe_device->_recv_buffer_offset = 0;
	}

	if(pipe_device->_data_buffer != NULL)
	{
		dm_free_buffer_to_data_buffer(pipe_device->_data_buffer_len, &pipe_device->_data_buffer);
	}

	LOG_DEBUG("[emule_pipe_device = 0x%x]emule_pipe_device destroy.", pipe_device);
	return sd_free(pipe_device);
}

_int32 emule_pipe_device_connect(EMULE_PIPE_DEVICE* pipe_device, EMULE_RESOURCE* res)
{
	_int32 ret = SUCCESS;
	EMULE_PEER* local_peer = emule_get_local_peer();

	if(IS_HIGHID(res->_client_id))
	{
        LOG_DEBUG("remote is high id, so connect remote directly, remote client_id = %u.", res->_client_id);
		
        ret = emule_socket_device_create(&pipe_device->_socket_device, EMULE_TCP_TYPE, NULL, 
            g_emule_pipe_device_callback_table, pipe_device);
		CHECK_VALUE(ret);

		return emule_socket_device_connect(pipe_device->_socket_device, res->_client_id, res->_client_port, NULL);
	}

	if(IS_HIGHID(local_peer->_client_id))
	{	
        LOG_DEBUG("local is high id, local client_id = %u.", local_peer->_client_id);
        // 反连：本地是高id，远端是低id
		if(emule_is_local_server(res->_server_ip, res->_server_port))
		{
            LOG_DEBUG("we stay in the same emule server, server_ip = %u, server_port = %hu.", res->_server_ip,
                res->_server_port);

			ret = emule_server_request_callback(res->_client_id);
			CHECK_VALUE(ret);

			return emule_add_wait_accept_pipe(pipe_device->_pipe);
		}
	}

	//low to low,  如果支持verycd版穿透，则尝试打洞
	if(emule_get_user_id_type(res->_user_id) == SO_EMULE)
	{
		ret = emule_socket_device_create(&pipe_device->_socket_device, EMULE_UDT_TYPE, NULL, 
            g_emule_pipe_device_callback_table, pipe_device);
		CHECK_VALUE(ret);

		return emule_socket_device_connect(pipe_device->_socket_device, res->_client_id, 
            res->_client_port, res->_user_id);
	}

	return -1;
}

_int32 emule_pipe_device_connect_callback(void* user_data, _int32 errcode)
{
	EMULE_PIPE_DEVICE* pipe_device = (EMULE_PIPE_DEVICE*)user_data;
    LOG_DEBUG("emule_pipe_device_connect_callback, errcode = %d, user_data = 0x%x.", errcode, user_data);

	if(errcode != SUCCESS)
	{
		if(pipe_device->_passive == TRUE)		//这是个被动连接
		{
			sd_assert(FALSE);
			return emule_pipe_device_close(pipe_device);
		}
		return SUCCESS;
	}

	return emule_pipe_notify_connnect(pipe_device);
}


_int32 emule_pipe_device_send(EMULE_PIPE_DEVICE* pipe_device, char* buffer, _int32 len)
{
    if(len>LAGE_DATA_BUFFER_SIZE )
    {
        if(g_large_data_buffer_num >= LAGE_DATA_BUFFER_MAX_NUM)
        {
            return EMULE_SEND_DATA_WAIT;
        }
        g_large_data_buffer_num++;
    }
	return emule_socket_device_send(pipe_device->_socket_device, buffer, len);
}


_int32 emule_pipe_device_send_data(EMULE_PIPE_DEVICE* pipe_device, char* buffer, _int32 len, void *p_user )
{
    _int32 ret = SUCCESS;
    if(len>LAGE_DATA_BUFFER_SIZE )
    {
        if(g_large_data_buffer_num >= LAGE_DATA_BUFFER_MAX_NUM)
        {
            list_push(&g_pipe_device_user, p_user);
            return EMULE_SEND_DATA_WAIT;
        }
        g_large_data_buffer_num++;
    }
    //ret = emule_pipe_device_send(pipe_device, buffer, len);
    ret = emule_socket_device_send_in_speed_limit(pipe_device->_socket_device, buffer, len);
	LOG_DEBUG("[emule_pipe_device = 0x%x, emule_pipe = 0x%x]emule_pipe_device_send_data, g_large_data_buffer_num:%d, len:%d, ret:%d, buffer:0x%x", 
      pipe_device, pipe_device->_pipe, g_large_data_buffer_num, len, ret, buffer);

    //emule协议由于申请数据一次可能有3个180k,当文件io速度大于网络发包速度时,会堆积大量buffer到sock层,导致内存不足,所以必须控制网络发包,
    //这里只能根据大小来判断是否能发下一个包

    return ret;
}

_int32 emule_pipe_device_send_callback(void* user_data, char* buffer, _int32 len, _int32 errcode)
{
    LIST_ITERATOR cur_node = NULL;
    LIST_ITERATOR next_node = NULL;
    void *p_uesr = NULL;
    void *p_cancel_user = NULL;
    EMULE_PIPE_DEVICE *pipe_device = NULL;
    //有可能出现这种情况:发送一个数据包,长时间发送不完,上层关闭emule_pipe,pipe发cancel到底层,此时底层只发送了部分数据,导致
    //此时g_large_data_buffer_num没有及时减1,所以导致emule_pipe申请的buffer迟迟发不出去

    LOG_DEBUG("emule_pipe_device_send_callback, user_data = 0x%x, len = %d, errcode = %d.", user_data, len, 
        errcode);

    if (errcode != SUCCESS)
    {
        // 取消对应device上的发送请求
        pipe_device = (EMULE_PIPE_DEVICE *)user_data;
        p_cancel_user = (void *)(pipe_device->_pipe->_upload_device);
        LOG_DEBUG("emule_pipe_device_send_callback, there is an error, pipe_device = 0x%x, p_cancel_user = 0x%x.", 
            pipe_device, p_cancel_user);
    }

    if(len>LAGE_DATA_BUFFER_SIZE )
    {
        if(g_large_data_buffer_num>0)
        {
            g_large_data_buffer_num--;
        }
        /*
        else
        {
            sd_assert(FALSE);
        }
        */
    }
   
    
    if(g_large_data_buffer_num == LAGE_DATA_BUFFER_MAX_NUM)
    {
        g_sending_ticks++;
    }
    else
    {
        g_sending_ticks = 0;
    }

    //此处发现g_large_data_buffer_num该减的没减,不去通知上层继续投递了,等上层关闭后重发数据包.
    if(g_sending_ticks > LAGE_BUFFER_NUM_RESET_TICKS)
    {
        //sd_assert(FALSE);
        g_large_data_buffer_num =0 ;
        g_sending_ticks = 0;
        list_clear(&g_pipe_device_user);
    }

	sd_free((char*)buffer);
	buffer = NULL;
	LOG_DEBUG("emule_pipe_device_send_callback, g_large_data_buffer_num:%d, len:%d, buffer:0x%x, g_sending_ticks:%d", 
     g_large_data_buffer_num, len, buffer, g_sending_ticks);

    cur_node = LIST_BEGIN(g_pipe_device_user);
    while( cur_node != LIST_END(g_pipe_device_user)
        && g_large_data_buffer_num<LAGE_DATA_BUFFER_MAX_NUM )
    {
        p_uesr = LIST_VALUE(cur_node);
        next_node = LIST_NEXT( cur_node );
        if (p_uesr != p_cancel_user)
        {
            emule_upload_process_call_back(p_uesr);
        }
        else
        {
            LOG_DEBUG("emule_pipe_device_send_callback, the upload device is canceled so don't process next.");
        }
        list_erase(&g_pipe_device_user, cur_node);
        cur_node = next_node;
    }
	return SUCCESS;
}

BOOL emule_pipe_device_can_send()
{
    if(g_large_data_buffer_num >= LAGE_DATA_BUFFER_MAX_NUM)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

void emule_pipe_remove_user( void *p_target )
{
    LIST_ITERATOR cur_node = NULL;
    LIST_ITERATOR next_node = NULL;
    void *p_user = NULL;
    cur_node = LIST_BEGIN(g_pipe_device_user);
    
    while( cur_node != LIST_END(g_pipe_device_user) )
    {
        p_user = LIST_VALUE(cur_node);
        next_node = LIST_NEXT( cur_node );
        if( p_user == p_target )
        {
            LOG_DEBUG("emule_pipe_remove_user, p_target:0x%x", p_target);
            list_erase(&g_pipe_device_user, cur_node);
        }
        cur_node = next_node;
    }
}

_int32 emule_pipe_device_recv_cmd(EMULE_PIPE_DEVICE* pipe_device, _u32 len)
{
	_int32 ret = SUCCESS;

    sd_assert(pipe_device != NULL);
	if(pipe_device->_recv_buffer_offset + len > pipe_device->_recv_buffer_len)
	{
		ret = emule_pipe_device_extend_recv_buffer(pipe_device, MAX(pipe_device->_recv_buffer_offset + len, 1024));
		if(ret != SUCCESS)
			return emule_pipe_device_notify_error(pipe_device, ret);
	}

	LOG_DEBUG("[emule_pipe_device = 0x%x]emule_pipe_device_recv_cmd, socket_device = 0x%x, len = %u.", 
        pipe_device, pipe_device->_socket_device, len);

	return emule_socket_device_recv(pipe_device->_socket_device, 
        pipe_device->_recv_buffer + pipe_device->_recv_buffer_offset, len, FALSE);
}

_int32 emule_pipe_device_recv_cmd_callback(void* user_data, char* buffer, _int32 len, _int32 errcode)
{
	EMULE_PIPE_DEVICE* pipe_device = (EMULE_PIPE_DEVICE*)user_data;
	_int32 ret = SUCCESS;
	char* tmp_buf = NULL;
	_int32 tmp_len = 0;
	_u8 protocol = 0, op_type = 0;
	_u32 cmd_len = 0, total_len = 0;
	if(errcode != SUCCESS)
		return SUCCESS;
	//pipe已经出错了，这时可能还没被上层关闭，所以此后再收到什么数据包都丢弃不处理
    if ((pipe_device->_pipe != NULL) 
        && (pipe_device->_pipe->_data_pipe._dispatch_info._pipe_state == PIPE_FAILURE))
	    return SUCCESS;

	LOG_DEBUG("[emule_pipe_device = 0x%x pipe_device->_pipe = 0x%x]emule_pipe_device_recv_cmd_callback, len = %d.", 
        pipe_device, pipe_device->_pipe, len);

	pipe_device->_recv_buffer_offset += len;
	tmp_buf = pipe_device->_recv_buffer;
	tmp_len = (_int32)pipe_device->_recv_buffer_offset;
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&protocol);
	if(protocol != OP_EDONKEYHEADER && protocol != OP_EMULEPROT)
	{	
		LOG_ERROR("[emule_pipe_device = 0x%x pipe_device->_pipe = 0x%x]recv a unknowned cmd, protocol = %u.", 
            pipe_device, pipe_device->_pipe, protocol);
		return emule_pipe_device_notify_error(pipe_device, EMULE_PIPE_RECV_UNKNOWN_CMD);
	}
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd_len);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&op_type);
	total_len = cmd_len + EMULE_HEADER_SIZE;
    if(total_len > EMULE_MAX_CMD_LEN)//发现有的pipe传过来错误的数据包,导致去申请过大的buffer.
    {
        LOG_ERROR("[emule_pipe_device = 0x%x pipe_device->_pipe = 0x%x]recv a illegal cmd, total_len = %u.", 
            pipe_device, pipe_device->_pipe, total_len);
        return emule_pipe_device_notify_error(pipe_device, EMULE_PIPE_RECV_ILLEGAL_CMD);
    }
    
	if((total_len == pipe_device->_recv_buffer_offset) 
       || (op_type == OP_SENDINGPART && pipe_device->_recv_buffer_offset == 30)
	   || (op_type == OP_SENDINGPART_I64 && pipe_device->_recv_buffer_offset == 38))
	{	/*have recv complete package*/
		if(protocol == OP_EDONKEYHEADER)
			ret = emule_handle_recv_edonkey_cmd(pipe_device, pipe_device->_recv_buffer + EMULE_HEADER_SIZE, 
                    pipe_device->_recv_buffer_offset - EMULE_HEADER_SIZE);
		else
			ret = emule_handle_recv_emule_cmd(pipe_device->_pipe, pipe_device->_recv_buffer + EMULE_HEADER_SIZE, 
                    pipe_device->_recv_buffer_offset - EMULE_HEADER_SIZE);
		
        if(ret != SUCCESS)
			return emule_pipe_device_notify_error(pipe_device, ret);

		/*recv next command*/
		if(op_type != OP_SENDINGPART && op_type != OP_SENDINGPART_I64)
		{
			pipe_device->_recv_buffer_offset = 0;
			ret = emule_pipe_device_recv_cmd(pipe_device, EMULE_HEADER_SIZE + 1);
		}

		return ret;
	}

	//如果是数据包,那么进行特殊处理
	if(op_type == OP_SENDINGPART)
	{
		sd_assert(pipe_device->_recv_buffer_offset == EMULE_HEADER_SIZE + 1);
		return emule_pipe_device_recv_cmd(pipe_device, FILE_ID_SIZE + 8);
	}

	if(op_type == OP_SENDINGPART_I64)
	{
		sd_assert(pipe_device->_recv_buffer_offset == EMULE_HEADER_SIZE + 1);
		return emule_pipe_device_recv_cmd(pipe_device, FILE_ID_SIZE + 16);
	}

	/*uncomplete package*/
	sd_assert(total_len > pipe_device->_recv_buffer_offset);
	return emule_pipe_device_recv_cmd(pipe_device, total_len - pipe_device->_recv_buffer_offset);
}

_int32 emule_pipe_device_recv_data(EMULE_PIPE_DEVICE* pipe_device, _u32 len)
{
	_int32 ret = SUCCESS;
	if(pipe_device->_data_buffer == NULL)
	{
		pipe_device->_data_buffer_len = get_data_unit_size();
		pipe_device->_data_buffer_offset = 0;
		ret = pi_get_data_buffer(&pipe_device->_pipe->_data_pipe, &pipe_device->_data_buffer, &pipe_device->_data_buffer_len);
		if(ret != SUCCESS)
		{
			if(ret == DATA_BUFFER_IS_FULL || ret == OUT_OF_MEMORY)
			{
				LOG_DEBUG("[emule_pipe = 0x%x]pi_get_data_buffer failed, start timer to get data buffer later.", pipe_device->_pipe);
				return start_timer(emule_pipe_device_get_buffer_timeout, NOTICE_ONCE, 1000, len, pipe_device, &pipe_device->_get_buffer_timeout_id);
			}
			else
			{
				LOG_ERROR("[emule_pipe = 0x%x]pi_get_data_buffer failed, pipe set to FAILURE.", pipe_device->_pipe);
				return ret;
			}
		}
	}
	pipe_device->_data_len = len;
	len = MIN(len, pipe_device->_data_buffer_len - pipe_device->_data_buffer_offset);
	LOG_DEBUG("[emule_pipe_device = 0x%x]emule_pipe_device_recv_data, len = %u.", pipe_device, len);
	return emule_socket_device_recv(pipe_device->_socket_device, pipe_device->_data_buffer + pipe_device->_data_buffer_offset, len, TRUE);
}

_int32 emule_pipe_device_recv_data_callback(void* user_data, char* buffer, _int32 len, _int32 errcode)
{
	_int32 ret = SUCCESS;
	RANGE down_range;
	EMULE_PIPE_DEVICE* pipe_device = (EMULE_PIPE_DEVICE*)user_data;
	if(errcode != SUCCESS)
		return SUCCESS;
	LOG_DEBUG("[emule_pipe_device = 0x%x]emule_pipe_device_recv_data_callback, len = %d.", pipe_device, len);
	sd_assert(len <= pipe_device->_data_len);
	pipe_device->_data_len -= len;
	add_speed_record(&pipe_device->_download_speed, len);
	if(pipe_device->_valid_data == TRUE)
		pipe_device->_data_buffer_offset += len;
	dp_get_download_range(&pipe_device->_pipe->_data_pipe, &down_range);
	down_range._num = 1;
	if(pipe_device->_data_buffer_offset == emule_get_range_size(pipe_device->_pipe->_data_pipe._p_data_manager, &down_range))
	{
		sd_assert(pipe_device->_data_pos == (_u64)get_data_unit_size() * down_range._index);
		emule_pipe_notify_recv_part_data(pipe_device->_pipe, &down_range);
		pipe_device->_data_buffer_offset = 0;		//上面已经提交了数据，要把offset置为0
	}
	if(pipe_device->_data_len > 0)
	{
		ret = emule_pipe_device_recv_data(pipe_device, pipe_device->_data_len);
	}
	else
	{	//接收下一个命令包
		pipe_device->_recv_buffer_offset = 0;
		ret = emule_pipe_device_recv_cmd(pipe_device, EMULE_HEADER_SIZE + 1);
	}
	return ret;
}

_int32 emule_pipe_device_notify_error(void* user_data, _int32 errcode)
{
	EMULE_PIPE_DEVICE* pipe_device = (EMULE_PIPE_DEVICE*)user_data;
	LOG_DEBUG("[emule_pipe_device = 0x%x]emule_device handle error, errcode = %d.", pipe_device, errcode);
	
    if(pipe_device->_pipe != NULL)
    {
        emule_pipe_handle_error(pipe_device->_pipe, errcode);
    }
    else
    {
        // 需要处理被动连接的emule设备出现错误
        emule_pipe_device_close(pipe_device);
    }

	return SUCCESS;
}

_int32 emule_pipe_device_extend_recv_buffer(EMULE_PIPE_DEVICE* pipe_device, _u32 len)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL;
	sd_assert(len > pipe_device->_recv_buffer_len && pipe_device->_recv_buffer_len >= pipe_device->_recv_buffer_offset);
	ret = sd_malloc(len, (void**)&buffer);
	if(ret != SUCCESS)
		return ret;
	if(pipe_device->_recv_buffer_offset > 0)
		sd_memcpy(buffer, pipe_device->_recv_buffer, pipe_device->_recv_buffer_offset);
	pipe_device->_recv_buffer_len = len;
	if(pipe_device->_recv_buffer != NULL)
		sd_free(pipe_device->_recv_buffer);
	pipe_device->_recv_buffer = buffer;
	LOG_DEBUG("[emule_pipe = 0x%x]emule_pipe_device_extend_recv_buffer, len = %u, _recv_buffer_offset:%u", 
        pipe_device->_pipe, len, pipe_device->_recv_buffer_offset);
	return ret;
}

_int32 emule_pipe_device_get_buffer_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid)
{
	EMULE_PIPE_DEVICE* pipe_device = (EMULE_PIPE_DEVICE*)msg_info->_user_data;
	_u32 len = msg_info->_device_id;
	if(errcode == MSG_CANCELLED)
		return SUCCESS;
	LOG_DEBUG("[emule_pipe = 0x%x]emule_device_get_buffer_timeout.", pipe_device->_pipe);
    
	pipe_device->_get_buffer_timeout_id = INVALID_MSG_ID;
	return emule_pipe_device_recv_data(pipe_device, len);
}

#endif /* ENABLE_EMULE */
#endif /* ENABLE_EMULE */

