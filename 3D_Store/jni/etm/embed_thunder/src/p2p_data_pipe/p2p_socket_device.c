#include "p2p_socket_device.h"
#include "p2p_pipe_impl.h"
#include "p2p_pipe_interface.h"
#include "p2p_memory_slab.h"
#include "p2p_data_pipe.h"
#include "p2p_cmd_define.h"
#include "p2p_cmd_builder.h"
#include "p2p_cmd_handler.h"
#include "p2p_send_cmd.h"
#include "p2p_transfer_layer/ptl_interface.h"
#include "p2p_transfer_layer/udt/udt_interface.h"
#include "p2p_transfer_layer/udt/udt_utility.h"
#include "p2p_transfer_layer/udt/udt_device_manager.h"
#include "p2p_transfer_layer/tcp/tcp_interface.h"
#include "p2p_transfer_layer/mhxy/encryption_algorithm.h"
#include "connect_manager/pipe_interface.h"
#include "utility/bytebuffer.h"
#include "utility/peer_capability.h"
#include "utility/string.h"
#include "utility/sd_assert.h"
#include "utility/time.h"
#include "utility/local_ip.h"
#include "utility/peerid.h"
#include "utility/utility.h"
#include "utility/logid.h"
#include "platform/sd_network.h"

#define	 LOGID	LOGID_P2P_PIPE
#include "utility/logger.h"

#define GET_DATA_TIMEOUT			1000

#ifdef VVVV_VOD_DEBUG
     extern _u64 gloabal_discard_data = 0;
#endif

#ifdef _SUPPORT_MHXY
#define MHXY_FIRST_HEAD_LEN 12
#define CDN_P2P_CMD_HEADER_LENGTH (sizeof(int)*5+sizeof(_u8))
#endif

_int32 p2p_create_socket_device(P2P_DATA_PIPE* p2p_pipe)
{
	_int32 ret = p2p_malloc_socket_device(&p2p_pipe->_socket_device);
	CHECK_VALUE(ret);
	sd_memset(p2p_pipe->_socket_device, 0, sizeof(SOCKET_DEVICE));
	p2p_pipe->_socket_device->_keepalive_timeout_id = INVALID_MSG_ID;
	p2p_pipe->_socket_device->_get_data_buffer_timeout_id = INVALID_MSG_ID;
	p2p_pipe->_socket_device->_is_valid_data = TRUE;		//这里一定要初始化为TRUE，因为54以前的p2p版本没有字段来判断是否是有效数据，只能假定都是有效的
	init_speed_calculator(&p2p_pipe->_socket_device->_speed_calculator, 20, 500);
	return SUCCESS;
}

_int32 p2p_close_socket_device(P2P_DATA_PIPE* p2p_pipe)
{
	_int32 ret = SUCCESS;
	if(p2p_pipe->_socket_device->_keepalive_timeout_id != INVALID_MSG_ID)
	{
		cancel_timer(p2p_pipe->_socket_device->_keepalive_timeout_id);
		p2p_pipe->_socket_device->_keepalive_timeout_id = INVALID_MSG_ID;
	}
	if(p2p_pipe->_socket_device->_get_data_buffer_timeout_id != INVALID_MSG_ID)
	{
		cancel_timer(p2p_pipe->_socket_device->_get_data_buffer_timeout_id);
		p2p_pipe->_socket_device->_get_data_buffer_timeout_id = INVALID_MSG_ID;
	}
	ret = ptl_close_device(p2p_pipe->_device);
	if(ret == SUCCESS)
		p2p_destroy_socket_device(p2p_pipe);		//设备关闭成功了，直接删socket_device结构
	return ret;
}

_int32 p2p_destroy_socket_device(P2P_DATA_PIPE* p2p_pipe)
{
	_int32 ret = SUCCESS;
	if(p2p_pipe->_socket_device->_cmd_buffer != NULL)
	{
		if(p2p_pipe->_socket_device->_cmd_buffer_len == RECV_CMD_BUFFER_LEN)
			p2p_free_recv_cmd_buffer(p2p_pipe->_socket_device->_cmd_buffer);
		else
		{
			sd_free(p2p_pipe->_socket_device->_cmd_buffer);
			p2p_pipe->_socket_device->_cmd_buffer = NULL;
		}
	}
	p2p_pipe->_socket_device->_cmd_buffer = NULL;
	p2p_socket_device_free_data_buffer(p2p_pipe);
	if(p2p_pipe->_socket_device->_p2p_sending_cmd != NULL)
	{
		sd_assert(p2p_pipe->_socket_device->_p2p_sending_queue != NULL);
		//		ptl_free_cmd_buffer(device->_p2p_sending_cmd->buffer);		这里不能释放，因为已经被底层ptl接管了，由ptl释放
		p2p_free_sending_cmd(p2p_pipe->_socket_device->_p2p_sending_cmd);
		p2p_pipe->_socket_device->_p2p_sending_cmd = NULL;
	}
	if(p2p_pipe->_socket_device->_p2p_sending_queue != NULL)
	{
		ret = close_p2p_sending_queue(p2p_pipe->_socket_device->_p2p_sending_queue);
		CHECK_VALUE(ret);
		p2p_pipe->_socket_device->_p2p_sending_queue = NULL;
	}
	uninit_speed_calculator(&p2p_pipe->_socket_device->_speed_calculator);
	return p2p_free_socket_device(p2p_pipe->_socket_device);
}

_int32 p2p_socket_device_connect(P2P_DATA_PIPE* p2p_pipe, const P2P_RESOURCE* res)
{
	return ptl_connect(p2p_pipe->_device, res->_peer_capability, res->_gcid, res->_file_size, res->_peer_id, res->_ip, res->_tcp_port, res->_udp_port);
}

//cdn的头转换成正常协议包头
_int32 p2p_cmd_head_cdn_to_common(char* cmd_buffer, _u32 len)
{
    char *temp_buf = cmd_buffer;
    _int32 temp_len = len;
    _u32 rand_value = 0;
    sd_get_int32_from_lt( &temp_buf, &temp_len, &rand_value);
    
    _u8 cmd_type = 0;
    sd_get_int8( &temp_buf, &temp_len, &cmd_type);

    sd_get_int32_from_lt( &temp_buf, &temp_len, &rand_value);
    
    _u32 version = 0;
    sd_get_int32_from_lt( &temp_buf, &temp_len, &version);

    sd_get_int32_from_lt( &temp_buf, &temp_len, &rand_value);
    
    _u32 body_len = 0;
    sd_get_int32_from_lt( &temp_buf, &temp_len, &body_len);

    _int32 i=0;
    for (i=9; i<len-12; i++)
    {
        cmd_buffer[i] = cmd_buffer[i+12];
    }
    temp_buf = cmd_buffer;
    temp_len = len-12;
    
    // protocol version
    sd_set_int32_to_lt( &temp_buf, &temp_len, version );
    // body_length
    _u32 body_len_new = body_len+1;
    sd_set_int32_to_lt( &temp_buf, &temp_len, body_len_new );//因为模糊协议时，包头包含command_type,而非模糊协议则不包括
    // cmd_type_id
    sd_set_int8( &temp_buf, &temp_len, cmd_type );
    
    return SUCCESS;
}

//将正常协议包头转换成cdn协议的头
_int32 p2p_cmd_head_common_to_cdn(char* cmd_buffer, _u32 len)
{
    char *temp_buf = cmd_buffer;
    _int32 temp_len = len;
    _u32 version = 0;
    sd_get_int32_from_lt( &temp_buf, &temp_len, &version);
    _u32 body_len = 0;
    sd_get_int32_from_lt( &temp_buf, &temp_len, &body_len);
    _u8 cmd_type = 0;
    sd_get_int8( &temp_buf, &temp_len, &cmd_type);

    _int32 i=0;
    for (i=len-1; i>=9; i--)
    {
        cmd_buffer[i+12] = cmd_buffer[i];
    }

    temp_buf = cmd_buffer;
    temp_len = len+12; //因为需要的缓冲区长度已经大了12字节

    _int32 rand_val = rand() + 257;
    sd_set_int32_to_lt( &temp_buf, &temp_len, rand_val);
    // cmd_type_id
    sd_set_int8( &temp_buf, &temp_len, cmd_type );
    //random num 2
    rand_val = rand();
    sd_set_int32_to_lt( &temp_buf, &temp_len, rand_val );
    // protocol version
    sd_set_int32_to_lt( &temp_buf, &temp_len, version );
    //random num 3
    rand_val = rand();
    sd_set_int32_to_lt( &temp_buf, &temp_len, rand_val );
    // body_length
    _u32 body_len_new = body_len-1;
    sd_set_int32_to_lt( &temp_buf, &temp_len, body_len_new ); //因为模糊协议时，包头包含command_type,而非模糊协议则不包括

   //LOG_DEBUG("cmd type:%d, version:%u, body_len:%d, new body_len%u", cmd_type, version, body_len, body_len_new);
    return SUCCESS;
}

_int32 p2p_socket_device_send(P2P_DATA_PIPE* p2p_pipe, _u8 command_type, char* cmd_buffer, _u32 len)
{
#ifdef _SUPPORT_MHXY
	BOOL is_cdn = p2p_is_cdn_resource(p2p_pipe->_data_pipe._p_resource);
	BOOL support_tcp_mhxy =  p2p_is_support_mhxy((DATA_PIPE *)p2p_pipe) && TCP_TYPE==p2p_pipe->_device->_type;
	if ( is_cdn && support_tcp_mhxy )
	{
            //char cmd_hex_str[513];
            //sd_memset(cmd_hex_str, 0, sizeof(cmd_hex_str));
            //str2hex( (char*)cmd_buffer, len, cmd_hex_str, sizeof(cmd_hex_str)-1);
            //LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x] cmd:%s", p2p_pipe, p2p_pipe->_device, cmd_hex_str);
	
	    //发送时模糊协议
	    TCP_DEVICE *p_tcp_device = (TCP_DEVICE *)p2p_pipe->_device->_device;
	    _u32 encrypt_buf_len= len+12;
	    _u8 *new_cmd_buffer = NULL;
	    _u32 new_buffer_len = len + 12*2+1;
	    _u32 new_offset = 0;
	    ptl_malloc_cmd_buffer((char **)&new_cmd_buffer, &new_buffer_len, &new_offset, p2p_pipe->_device);
	    sd_memset(new_cmd_buffer, 0, new_buffer_len);
	    sd_memcpy(new_cmd_buffer, cmd_buffer, len);
	    p2p_cmd_head_common_to_cdn(new_cmd_buffer, len);
	    ptl_free_cmd_buffer(cmd_buffer);
	    cmd_buffer = new_cmd_buffer;

            //sd_memset(cmd_hex_str, 0, sizeof(cmd_hex_str));
            //str2hex( (char*)cmd_buffer, len+12, cmd_hex_str, sizeof(cmd_hex_str)-1);
            //LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x] cmd mhxy:%s", p2p_pipe, p2p_pipe->_device, cmd_hex_str);

            BOOL is_first_encrypt = p_tcp_device->_mhxy->_is_first_decrypt;
            _u32 key_pos = 0;
            if (p_tcp_device->_mhxy->_ea_encrypt) key_pos = p_tcp_device->_mhxy->_ea_encrypt->_key_pos;
                
	    mhxy_tcp_encrypt2( p_tcp_device->_mhxy, cmd_buffer, &encrypt_buf_len);
	    len = encrypt_buf_len;
        /*
            char file_name[256];
            sprintf(file_name, "/tmp/send_cmd_%x.dat",  p2p_pipe->_device);
            FILE *pFile = fopen(file_name, "ab+");
            if (pFile)
            {
                fwrite(cmd_buffer, len, 1, pFile);
                fclose(pFile);
            }
        */
	    //LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x] mhxy tcp encrypt len:%d, is first encrypt:%d, key_pos:%u.", p2p_pipe, p2p_pipe->_device, len, is_first_encrypt, key_pos);
	}
#endif

	P2P_SENDING_CMD* sending_cmd;
	_int32 ret = p2p_alloc_sending_cmd(&sending_cmd, cmd_buffer, len);
	if(ret != SUCCESS)
	{
		ptl_free_cmd_buffer(cmd_buffer);
		return ret;
	}
	if(p2p_pipe->_socket_device->_p2p_sending_cmd == NULL)
	{	/*no command is sending, don't push queue, send immediately*/
#ifdef _LOGGER
		char cmd_str[17][16] = {"HANDSHAKE", "HANDSHAKE_RESP", "INTERESTED", "INTERESTED_RESP", "NOT_INTERESTED", "KEEPALIVE", "REQUEST", "REQUEST_RESP", "CANCEL",
				"CANCEL_RESP", "UNKNOWN", "UNKNOWN", "UNKNOWN_COMMAND", "CHOKE", "UNCHOKE", "FIN", "FIN_RESP"};
		LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p send %s command, cmd_len = %u.", p2p_pipe, p2p_pipe->_device, cmd_str[command_type - 100], len);
#endif
        if(command_type == REQUEST_RESP)
        {
            ret = ptl_send_in_speed_limit(p2p_pipe->_device, cmd_buffer, len);
        }
        else
        {
		    ret = ptl_send(p2p_pipe->_device, cmd_buffer, len);
        }
		if(ret == SUCCESS)
		{
			p2p_pipe->_socket_device->_p2p_sending_cmd = sending_cmd;
			/* 记录发送的指令为HANDSHAKE，用于检测迅雷p2p协议是否被屏蔽 */
			if(HANDSHAKE==command_type)
			{
				p2p_pipe->_last_cmd_is_handshake = TRUE;
			}
			else
			{
				p2p_pipe->_last_cmd_is_handshake = FALSE;
			}
		} 
	}
	else
	{	/*last command is sending, push sending_cmd in p2p sending queue*/
		sending_cmd->_cmd_type = command_type;
		ret = p2p_push_sending_cmd(p2p_pipe->_socket_device->_p2p_sending_queue, sending_cmd, command_type);
	}
	if(ret != SUCCESS)
	{
		LOG_ERROR("[p2p_pipe = 0x%x, device = 0x%x]ptl_send failed.", p2p_pipe, p2p_pipe->_device);
		ptl_free_cmd_buffer(cmd_buffer);
		p2p_free_sending_cmd(sending_cmd);
	}
	return ret;
}

_int32 p2p_socket_device_recv_cmd(P2P_DATA_PIPE* p2p_pipe, _u32 cmd_len)
{
#ifdef _SUPPORT_MHXY
	BOOL is_cdn = p2p_is_cdn_resource(p2p_pipe->_data_pipe._p_resource);
	BOOL support_tcp_mhxy =  p2p_is_support_mhxy((DATA_PIPE *)p2p_pipe) && TCP_TYPE==p2p_pipe->_device->_type;
	if ((is_cdn && support_tcp_mhxy) && 0==p2p_pipe->_socket_device->_cmd_buffer_offset)
	{
	    //模糊协议先取头部
	    cmd_len = MHXY_FIRST_HEAD_LEN;
	}
#endif    
	if(p2p_pipe->_socket_device->_cmd_buffer_len < cmd_len + p2p_pipe->_socket_device->_cmd_buffer_offset)
	{	/*extend recv buffer*/
		_int32 ret = SUCCESS;
		char*  buffer = NULL;
		LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p_tcp_device_recv_cmd, but recv cmd len is %u, greater than RECV_CMD_BUFFER_LEN, extend recv buffer.",p2p_pipe, p2p_pipe->_device, cmd_len + p2p_pipe->_socket_device->_cmd_buffer_offset);
		ret = sd_malloc(cmd_len + p2p_pipe->_socket_device->_cmd_buffer_offset, (void**)&buffer);
		CHECK_VALUE(ret);
		sd_memcpy(buffer, p2p_pipe->_socket_device->_cmd_buffer, p2p_pipe->_socket_device->_cmd_buffer_offset);
		if(p2p_pipe->_socket_device->_cmd_buffer_len == RECV_CMD_BUFFER_LEN)
			p2p_free_recv_cmd_buffer(p2p_pipe->_socket_device->_cmd_buffer);
		else
		{
			sd_free(p2p_pipe->_socket_device->_cmd_buffer);
			p2p_pipe->_socket_device->_cmd_buffer = NULL;
		}
		p2p_pipe->_socket_device->_cmd_buffer = buffer;
		p2p_pipe->_socket_device->_cmd_buffer_len = cmd_len + p2p_pipe->_socket_device->_cmd_buffer_offset;
	}
	return ptl_recv_cmd(p2p_pipe->_device, p2p_pipe->_socket_device->_cmd_buffer + p2p_pipe->_socket_device->_cmd_buffer_offset, cmd_len, p2p_pipe->_socket_device->_cmd_buffer_len - p2p_pipe->_socket_device->_cmd_buffer_offset);
}

_int32 p2p_socket_device_recv_data(P2P_DATA_PIPE* p2p_pipe, _u32 data_len)
{
	RANGE down_range;
	P2P_RESOURCE* peer_resource = (P2P_RESOURCE*)p2p_pipe->_data_pipe._p_resource;
	if(p2p_pipe->_socket_device->_data_buffer == NULL)
	{	/*allocate a new buffer from data manager*/
		_int32 ret;
		dp_get_download_range(&p2p_pipe->_data_pipe, &down_range);
		//如果此时处于cancel状态，那么down_range为[0,0],这样就是malloc一块内存用来存放收到的无效数据，在收到cancel_resp时才把这块内存释放掉
		down_range._num = 1;
		p2p_pipe->_socket_device->_request_data_len = range_to_length(&down_range, peer_resource->_file_size);
		p2p_pipe->_socket_device->_data_buffer_len = p2p_pipe->_socket_device->_request_data_len;			//最后一块大小可能是不规则的
		ret = pi_get_data_buffer(&p2p_pipe->_data_pipe,  &p2p_pipe->_socket_device->_data_buffer, &p2p_pipe->_socket_device->_data_buffer_len);
		if(ret != SUCCESS)
		{
			if(ret == DATA_BUFFER_IS_FULL || ret == OUT_OF_MEMORY)
			{
				LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]dm_get_data_buffer failed, start timer to get data buffer later.", p2p_pipe, p2p_pipe->_device);
				return start_timer(get_data_buffer_timeout_handler, NOTICE_ONCE, GET_DATA_TIMEOUT, data_len, p2p_pipe, &p2p_pipe->_socket_device->_get_data_buffer_timeout_id);
			}
			else
			{
				LOG_ERROR("[p2p_pipe = 0x%x, device = 0x%x]dm_get_data_buffer failed, pipe set to FAILURE.", p2p_pipe, p2p_pipe->_device);
				return ret;
			}
		}
		p2p_pipe->_socket_device->_data_buffer_offset = 0;
	}
	if(p2p_pipe->_socket_device->_is_valid_data == TRUE && data_len > p2p_pipe->_socket_device->_request_data_len - p2p_pipe->_socket_device->_data_buffer_offset)		//只有是有效数据时才有意义，否则收回来后就丢弃了
	{	
		LOG_ERROR("[p2p_pipe = 0x%x, device = 0x%x]receive data length is greater than request data length, must be logical error. data_len = %u,device->_request_data_len = %u, device->_data_buffer_offset = %u.", p2p_pipe, p2p_pipe->_device,data_len, p2p_pipe->_socket_device->_request_data_len, p2p_pipe->_socket_device->_data_buffer_offset);
		sd_assert(FALSE);
		return -1;
	}
	/*
	如果A发送1，2，3，4，5块请求给B,此时B处理了第一块后发送choke给A,又马上发送unchoke给A,
	这就会导致A收到错误的pos，len。原因是B发送choke命令时可能还没发送出去，而是放在发送队列里面，
	并把未处理的请求2，3，4全清空;紧接着又发送unckoke命令，此时会把队列中的choke命令清除掉，导致
	A没有收到B的choke命令而是直接收到unchoke命令，没有把之前的请求2，3，4清掉。这样后面收到的数据
	都是第5块后面的数据，导致pos,len对不上号，目前的处理方法是直接丢弃该连接。
	*/
	//sd_assert(data_len <= p2p_pipe->_socket_device->_data_buffer_len - p2p_pipe->_socket_device->_data_buffer_offset);
	if(data_len > p2p_pipe->_socket_device->_data_buffer_len - p2p_pipe->_socket_device->_data_buffer_offset)
		return -1;			//这个用来防止越界写数据
	return ptl_recv_data(p2p_pipe->_device, p2p_pipe->_socket_device->_data_buffer + p2p_pipe->_socket_device->_data_buffer_offset, data_len, p2p_pipe->_socket_device->_data_buffer_len - p2p_pipe->_socket_device->_data_buffer_offset);
}

_int32 p2p_socket_device_recv_discard_data(P2P_DATA_PIPE* p2p_pipe, _u32 discard_len)
{
//	LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p_tcp_device_recv_discard_data.", p2p_pipe, p2p_pipe->_device);
	return ptl_recv_discard_data(p2p_pipe->_device, p2p_pipe->_socket_device->_cmd_buffer+p2p_pipe->_socket_device->_cmd_buffer_offset, discard_len, p2p_pipe->_socket_device->_cmd_buffer_len-p2p_pipe->_socket_device->_cmd_buffer_offset);
}

_int32 p2p_socket_device_free_data_buffer(P2P_DATA_PIPE* p2p_pipe)
{
	_int32 ret = SUCCESS;
	if(p2p_pipe->_socket_device->_data_buffer == NULL)
		return SUCCESS;
	ret = pi_free_data_buffer(&p2p_pipe->_data_pipe, &p2p_pipe->_socket_device->_data_buffer, p2p_pipe->_socket_device->_data_buffer_len);
	sd_assert(p2p_pipe->_socket_device->_data_buffer == NULL);
	CHECK_VALUE(ret);
	p2p_pipe->_socket_device->_data_buffer = NULL;
	p2p_pipe->_socket_device->_data_buffer_len = 0;
	p2p_pipe->_socket_device->_data_buffer_offset = 0;
	p2p_pipe->_socket_device->_request_data_len = 0;
	return SUCCESS;
}

//以下部分是回调函数，由ptl层回调回来
_int32 p2p_socket_device_connect_callback(_int32 errcode, PTL_DEVICE* device)
{
	_int32 ret = SUCCESS;
	P2P_DATA_PIPE* p2p_pipe =device->_user_data;
	P2P_RESOURCE* res = (P2P_RESOURCE*)p2p_pipe->_data_pipe._p_resource;
	LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p_socket_device_connect_callback, errcode = %d, PTL_DEVICE=0x%X,type=%d,device=0x%X,cnt_type=%d.", 
		p2p_pipe, p2p_pipe->_device, errcode,device,device->_type,device->_device,device->_connect_type);
	if(errcode == ERR_PTL_TRY_UDT_CONNECT)
	{
#if defined(MOBILE_PHONE)
		if(!(NT_GPRS_WAP & sd_get_net_type()))
		{
#endif  /* MOBILE_PHONE */

			UDT_DEVICE* udt = NULL;
			LOG_DEBUG("[p2p_pipe = 0x%x]p2p connect by tcp failed, try to connect by udt.", p2p_pipe);
			sd_assert(device->_type == TCP_TYPE);
			ret = tcp_device_close((TCP_DEVICE*)device->_device);
			//sd_assert(ret == SUCCESS);
		
			ret = udt_device_create(&udt, udt_generate_source_port(), 0, udt_hash_peerid(res->_peer_id), device);
			CHECK_VALUE(ret);
			udt_add_device(udt);
			device->_type = UDT_TYPE;
			device->_device= udt;
			device->_connect_type = ACTIVE_UDT_CONN;
			return udt_device_connect(udt, res->_ip, res->_udp_port);
			
#if defined(MOBILE_PHONE)
		}
#endif /* MOBILE_PHONE */
	}
	if(p2p_pipe->_data_pipe._dispatch_info._pipe_state == PIPE_FAILURE)
	{
		LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p_socket_device_connect_callback, errcode = %d, but pipe state is FAILURE, not process, just return.", p2p_pipe, p2p_pipe->_device, errcode);
		return SUCCESS;
	}
	if(errcode != SUCCESS)
	{
		LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p_socket_device_connect failed, errcode = %d", p2p_pipe, p2p_pipe->_device, errcode);
		return p2p_pipe_handle_error(p2p_pipe, errcode);
	}
	/*connect success*/
	LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p_socket_device connect success.", p2p_pipe, p2p_pipe->_device);
	p2p_pipe_change_state(p2p_pipe, PIPE_CHOKED);
	sd_time_ms(&p2p_pipe->_data_pipe._dispatch_info._time_stamp);	/*set dispatch connect time*/
	/*create sending queue*/
	if(create_p2p_sending_queue(&p2p_pipe->_socket_device->_p2p_sending_queue) != SUCCESS)
	{	/*create sending queue failed, change status to notify close this pipe*/
		return p2p_pipe_handle_error(p2p_pipe, -1);
	}
	/*create a recv buffer*/
	ret = p2p_malloc_recv_cmd_buffer(&p2p_pipe->_socket_device->_cmd_buffer);
	if(ret != SUCCESS)
	{
		LOG_ERROR("device->_cmd_buffer malloc failed.");
		return p2p_pipe_handle_error(p2p_pipe, ret);
	}
	p2p_pipe->_socket_device->_cmd_buffer_len = RECV_CMD_BUFFER_LEN;
	sd_assert(p2p_pipe->_socket_device->_cmd_buffer_offset == 0);
	/*try to receive protocol header*/
	ret = p2p_socket_device_recv_cmd(p2p_pipe, P2P_CMD_HEADER_LENGTH);
	if(ret != SUCCESS)
	{
		return p2p_pipe_handle_error(p2p_pipe, ret);
	}
	//send handshake command	
	ret = p2p_send_hanshake_cmd(p2p_pipe);
	if(ret != SUCCESS)
		return p2p_pipe_handle_error(p2p_pipe, ret);
	return ret;
}

_int32 p2p_socket_device_send_callback(_int32 errcode, PTL_DEVICE* device, _u32 had_send)
{
	_int32 ret = SUCCESS;
	P2P_DATA_PIPE* p2p_pipe = (P2P_DATA_PIPE*)device->_user_data;
	if(errcode != SUCCESS)
		return p2p_pipe_handle_error(p2p_pipe, errcode);
	if(p2p_pipe->_data_pipe._dispatch_info._pipe_state == PIPE_FAILURE)
		return SUCCESS;
	LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p_socket_device_send_callback, had_send = %u", p2p_pipe, p2p_pipe->_device, had_send);
#ifdef UPLOAD_ENABLE
	/*send success*/
	add_speed_record(&p2p_pipe->_upload_speed, had_send);
#endif
	/*send success*/
//	ptl_free_cmd_buffer(device->_p2p_sending_cmd->buffer);	//这里发送成功了不能删除buffer，因为已经被底层udt接管或者被tcp设备回收了
	p2p_free_sending_cmd(p2p_pipe->_socket_device->_p2p_sending_cmd);
	p2p_pipe->_socket_device->_p2p_sending_cmd = NULL;
	/*p2p command had send success,send next command if exist*/
	p2p_pop_sending_cmd(p2p_pipe->_socket_device->_p2p_sending_queue
	                    , &p2p_pipe->_socket_device->_p2p_sending_cmd);  // 
	if(p2p_pipe->_socket_device->_p2p_sending_cmd == NULL)
	{	/*no command waiting to send*/
		return SUCCESS;	
	}

    if(p2p_pipe->_socket_device->_p2p_sending_cmd->_cmd_type == REQUEST_RESP)
    {
        ret = ptl_send_in_speed_limit(p2p_pipe->_device
	    , p2p_pipe->_socket_device->_p2p_sending_cmd->buffer
	    , p2p_pipe->_socket_device->_p2p_sending_cmd->len );
    }
    else
    {
	    ret = ptl_send(p2p_pipe->_device
	    , p2p_pipe->_socket_device->_p2p_sending_cmd->buffer
	    , p2p_pipe->_socket_device->_p2p_sending_cmd->len );
    }
	if(ret != SUCCESS)
	{
		LOG_ERROR("[p2p_pipe = 0x%x, device = 0x%x]ptl_send failed.", p2p_pipe, p2p_pipe->_device);
		ptl_free_cmd_buffer(p2p_pipe->_socket_device->_p2p_sending_cmd->buffer);
		p2p_pipe_handle_error(p2p_pipe, ret);
	}
	return SUCCESS;
}

_int32 p2p_socket_device_recv_cmd_callback(_int32 errcode, PTL_DEVICE* device, _u32 had_recv)
{

	_int32 ret = SUCCESS;
	char* buffer;
	_int32 buffer_len;
	_u32 total_len, command_len;
	_u8	 command_type;
	P2P_DATA_PIPE* p2p_pipe = (P2P_DATA_PIPE*)device->_user_data;
	if(p2p_pipe->_data_pipe._dispatch_info._pipe_state == PIPE_FAILURE)
		return SUCCESS;
	if(errcode != SUCCESS)
		return p2p_pipe_handle_error(p2p_pipe, errcode);
	/*recv success*/
//	LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p_tcp_device_recv_cmd_callback, had_recv = %u.", p2p_pipe, p2p_pipe->_device, had_recv);
	sd_assert(had_recv != 0);
       if (0 == p2p_pipe->_socket_device->_cmd_buffer_offset) p2p_pipe->_socket_device->_mhxy_read_head = 1;
    //char file_name[256];
    //sprintf(file_name, "/tmp/handshake%x.dat",  p2p_pipe->_device);
    //FILE *pFile = fopen(file_name, "ab+");
    //if (pFile)
    //{
    //    fwrite(p2p_pipe->_socket_device->_cmd_buffer+p2p_pipe->_socket_device->_cmd_buffer_offset, had_recv, 1, pFile);
   //     fclose(pFile);
    //}
	p2p_pipe->_socket_device->_cmd_buffer_offset += had_recv;
    
#ifdef _SUPPORT_MHXY
       BOOL is_cdn = p2p_is_cdn_resource(p2p_pipe->_data_pipe._p_resource);
       BOOL support_tcp_mhxy = p2p_is_support_mhxy((DATA_PIPE *)p2p_pipe) && TCP_TYPE==p2p_pipe->_device->_type;
	if ( is_cdn && support_tcp_mhxy && p2p_pipe->_socket_device->_mhxy_read_head)
	{
	    //模糊协议先处理头部

        LOG_DEBUG("mhxy head: recv len:%d pipe:0x%x", had_recv, p2p_pipe);

	    TCP_DEVICE *p_tcp_device = (TCP_DEVICE *)p2p_pipe->_device->_device;
            //todo: 如果一次收不到12字节，会有问题
	    if (had_recv==p2p_pipe->_socket_device->_cmd_buffer_offset)
	    {
        	    _u8 first_decrpted_head_buf[128]={0};
        	    _u32 first_decrpted_head_len = 0;
        	    ret = mhxy_tcp_decrypt1(p_tcp_device->_mhxy, (unsigned char *)p2p_pipe->_socket_device->_cmd_buffer, p2p_pipe->_socket_device->_cmd_buffer_offset, first_decrpted_head_buf, &first_decrpted_head_len);
        	    sd_assert(0!=ret);
        	    if(first_decrpted_head_len > MHXY_FIRST_HEAD_LEN )
        	    {
        	        LOG_ERROR("mhxy head: decrpt return false");
        	        return -1;
        	    }
        	    sd_memcpy(p2p_pipe->_socket_device->_cmd_buffer, first_decrpted_head_buf, first_decrpted_head_len);
        	    p2p_pipe->_socket_device->_cmd_buffer_offset = first_decrpted_head_len;
                  if( first_decrpted_head_len < CDN_P2P_CMD_HEADER_LENGTH ) // Get remained head if necessary
                  {
        	        return p2p_socket_device_recv_cmd(p2p_pipe, CDN_P2P_CMD_HEADER_LENGTH - first_decrpted_head_len);
                  }
	    }
	
            if( p2p_pipe->_socket_device->_cmd_buffer_offset-had_recv< CDN_P2P_CMD_HEADER_LENGTH )
            {
                _u32 first_decrpted_head_len = p2p_pipe->_socket_device->_cmd_buffer_offset - had_recv;
                if ( p2p_pipe->_socket_device->_cmd_buffer_offset< CDN_P2P_CMD_HEADER_LENGTH )
                {
                    LOG_ERROR( "mhxy get remain head : Get %u bytes, but expect %u bytes.", had_recv, (CDN_P2P_CMD_HEADER_LENGTH-first_decrpted_head_len) );
                    return -1;
                }	

                _u32 decrptLen=CDN_P2P_CMD_HEADER_LENGTH-first_decrpted_head_len;
                ret = mhxy_tcp_decrypt2( p_tcp_device->_mhxy, p2p_pipe->_socket_device->_cmd_buffer+first_decrpted_head_len, &decrptLen);
                sd_assert(0!=ret);
                if( decrptLen != (CDN_P2P_CMD_HEADER_LENGTH-first_decrpted_head_len) )
                {
                    LOG_ERROR("mhxy head: decrpt return error");
                    return -1;
                }
                p2p_cmd_head_cdn_to_common( p2p_pipe->_socket_device->_cmd_buffer, CDN_P2P_CMD_HEADER_LENGTH);
                p2p_pipe->_socket_device->_cmd_buffer_offset = P2P_CMD_HEADER_LENGTH;
                p2p_pipe->_socket_device->_mhxy_read_head = 0;
            }
    
	}
#endif

	if(p2p_pipe->_socket_device->_cmd_buffer_offset < P2P_CMD_HEADER_LENGTH)
	{
		sd_assert(FALSE);
		return p2p_pipe_handle_error(p2p_pipe, -1);
	}
	/*have recv protocol header*/
	buffer = p2p_pipe->_socket_device->_cmd_buffer;
	buffer_len = p2p_pipe->_socket_device->_cmd_buffer_offset;
	sd_get_int32_from_lt(&buffer, &buffer_len, (_int32*)&p2p_pipe->_remote_protocol_ver);
	if(p2p_pipe->_remote_protocol_ver < P2P_PROTOCOL_VERSION_51)
	{	/*old p2p protocol, not support*/
		LOG_ERROR("[p2p_pipe = 0x%x]recv a p2p command, but this version is not support, version = %d", p2p_pipe, p2p_pipe->_remote_protocol_ver);
		return p2p_pipe_handle_error(p2p_pipe, -1);
	}
	sd_get_int32_from_lt(&buffer, &buffer_len, (_int32*)&command_len);
	sd_get_int8(&buffer, &buffer_len, (_int8*)&command_type);
	total_len = command_len + P2P_CMD_HEADER_LENGTH - 1;//因为P2P_CMD_HEADER_LENGTH计算包括command_type,而command_len也把command_type算在包体里，所以导致重复计算了一次
	if(total_len > 64 * 1024)
	{	/*不可能存在长度大于64K的数据*/
		LOG_ERROR("[p2p_pipe = 0x%x, device = 0x%x]recv a invalid p2p command , command_len bigger than 64K, command_len = %u", p2p_pipe, p2p_pipe->_device, total_len);
		return p2p_pipe_handle_error(p2p_pipe, -1);
	}
	if(total_len == p2p_pipe->_socket_device->_cmd_buffer_offset)
	{	/*have recv complete package*/
#ifdef _SUPPORT_MHXY
        	if (is_cdn && support_tcp_mhxy )
        	{
        	    TCP_DEVICE *p_tcp_device = (TCP_DEVICE *)p2p_pipe->_device->_device;
        	    _u32 decrypt_body_len = total_len - P2P_CMD_HEADER_LENGTH;
        	    ret = mhxy_tcp_decrypt2( p_tcp_device->_mhxy, p2p_pipe->_socket_device->_cmd_buffer+P2P_CMD_HEADER_LENGTH, &decrypt_body_len);
        	    sd_assert(0!=ret);                
        	}
#endif
		ret = handle_recv_cmd(p2p_pipe, command_type, p2p_pipe->_socket_device->_cmd_buffer, p2p_pipe->_socket_device->_cmd_buffer_offset);
		if(ret != SUCCESS)
		{
			return p2p_pipe_handle_error(p2p_pipe, ret);
		}
		/*recv next command*/
		p2p_pipe->_socket_device->_cmd_buffer_offset = 0;
		ret = p2p_socket_device_recv_cmd(p2p_pipe, P2P_CMD_HEADER_LENGTH);
		if(ret != SUCCESS)
		{
			return p2p_pipe_handle_error(p2p_pipe, ret);
		}
		return SUCCESS;
	}
	/*uncomplete package*/
	if(command_type != REQUEST_RESP)
	{
		if(total_len < p2p_pipe->_socket_device->_cmd_buffer_offset)
		{
			LOG_ERROR("[p2p_pipe = 0x%x]p2p_tcp_device_recv_cmd failed, total_len = %u, cmd_buffer_offset = %u.", p2p_pipe, total_len, p2p_pipe->_socket_device->_cmd_buffer_offset);
			return p2p_pipe_handle_error(p2p_pipe, -1);
		}	
		ret = p2p_socket_device_recv_cmd(p2p_pipe, total_len - p2p_pipe->_socket_device->_cmd_buffer_offset);
		if(ret != SUCCESS)
		{
			LOG_ERROR("[p2p_pipe = 0x%x]p2p_tcp_device_recv_cmd failed, errcode = %d.", p2p_pipe, ret);
			return p2p_pipe_handle_error(p2p_pipe, ret);
		}
		return SUCCESS;
	}
	/*it is REQUEST_RESPONSE_CMD, this command must be processed specially*/
	if(p2p_pipe->_socket_device->_cmd_buffer_offset == P2P_CMD_HEADER_LENGTH)
	{	/*recv result section*/
		return p2p_socket_device_recv_cmd(p2p_pipe, 1);
	}

#ifdef _SUPPORT_MHXY
    	if (is_cdn && support_tcp_mhxy )
    	{
    	    TCP_DEVICE *p_tcp_device = (TCP_DEVICE *)p2p_pipe->_device->_device;
    	    _u32 decrypt_body_len = had_recv;
    	    ret = mhxy_tcp_decrypt2( p_tcp_device->_mhxy, p2p_pipe->_socket_device->_cmd_buffer+p2p_pipe->_socket_device->_cmd_buffer_offset-had_recv, &decrypt_body_len);
    	    sd_assert(0!=ret);                
    	}
#endif    
	if(*buffer != P2P_RESULT_SUCC)
	{	/*request response result not SUCCESS*/
		LOG_ERROR("[p2p_pipe = 0x%x]recv REQUEST RESPONSE COMMAND, but result = %d, not success.", p2p_pipe, *buffer);
		return p2p_pipe_handle_error(p2p_pipe, ERR_P2P_REQUEST_RESP_FAIL);
	}
	/*result session must be 0, else it means no data response which processed by handle_recv_cmd*/
	if(p2p_pipe->_remote_protocol_ver >= P2P_PROTOCOL_VERSION_54 && p2p_pipe->_socket_device->_cmd_buffer_offset == P2P_CMD_HEADER_LENGTH + 1)
	{	/*recv data_pos section, data_len section*/
		return p2p_socket_device_recv_cmd(p2p_pipe, 8 + 4);	
	}
	ret = handle_request_resp_cmd(p2p_pipe, p2p_pipe->_socket_device->_cmd_buffer, p2p_pipe->_socket_device->_cmd_buffer_offset);
	if(ret != SUCCESS)
	{
		return p2p_pipe_handle_error(p2p_pipe, ret);
	}
	return SUCCESS;
}

_int32 p2p_socket_device_recv_data_callback(_int32 errcode, PTL_DEVICE* device, _u32 had_recv)
{
	_int32 ret = SUCCESS;
	RANGE rng, down_range;
	RANGE* req_range = NULL;
	P2P_DATA_PIPE* p2p_pipe = (P2P_DATA_PIPE*)device->_user_data;
	if(errcode != SUCCESS)
		return p2p_pipe_handle_error(p2p_pipe, errcode);
	if(p2p_pipe->_data_pipe._dispatch_info._pipe_state == PIPE_FAILURE)
		return SUCCESS;
	/*recv data success*/
	add_speed_record(&p2p_pipe->_socket_device->_speed_calculator, had_recv);
	LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p pipe had recv data, len = %u, speed = %ub/s", p2p_pipe, p2p_pipe->_device, had_recv, p2p_pipe->_data_pipe._dispatch_info._speed);
	if(p2p_pipe->_socket_device->_is_valid_data == TRUE)
	{
		sd_time_ms(&p2p_pipe->_data_pipe._dispatch_info._last_recv_data_time);
	      p2p_pipe->_socket_device->_data_buffer_offset += had_recv;		//只有当数据有效时才加进来
#ifdef _SUPPORT_MHXY	      
	      BOOL is_cdn = p2p_is_cdn_resource(p2p_pipe->_data_pipe._p_resource);
             BOOL support_tcp_mhxy = p2p_is_cdn_resource(p2p_pipe->_data_pipe._p_resource) && p2p_is_support_mhxy((DATA_PIPE *)p2p_pipe) && TCP_TYPE==p2p_pipe->_device->_type;
	      if ( is_cdn && support_tcp_mhxy )
	      {
	          //模糊协议解码收到的request data
        	    TCP_DEVICE *p_tcp_device = (TCP_DEVICE *)p2p_pipe->_device->_device;
        	    _u32 decrypt_body_len = had_recv;
                  
        	    ret = mhxy_tcp_decrypt2( p_tcp_device->_mhxy, p2p_pipe->_socket_device->_data_buffer+p2p_pipe->_socket_device->_data_buffer_offset - had_recv, &decrypt_body_len);
                   LOG_DEBUG("mhxy request data: pipe:0x%x recv len:%d decrypt_len:%u", p2p_pipe, had_recv, decrypt_body_len);
        	    sd_assert(0!=ret);
                  p2p_pipe->_socket_device->_data_buffer_offset += decrypt_body_len-had_recv;
              
	      }
#endif	      
	}
	else
	{
#ifdef VVVV_VOD_DEBUG
	      gloabal_discard_data += had_recv;
#endif
	}
	
	if(p2p_pipe->_socket_device->_request_data_len == p2p_pipe->_socket_device->_data_buffer_offset)
	{	/*all data had receive, commit to process*/
		if(p2p_pipe->_cancel_flag == TRUE)
		{	//有可能为TRUE,当在接收最后一块数据时上层发送了cancel命令，此时为TRUE,这块数据不用提交
			LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p_socket_device_recv_data_callback, ready to commit data, but this data is cancel.", p2p_pipe, p2p_pipe->_device);
			sd_assert(p2p_pipe->_data_pipe._dispatch_info._down_range._num == 0);
			p2p_socket_device_free_data_buffer(p2p_pipe);
		}
		else
		{	/*cancel flag is false, commit data to data manager*/
			sd_assert(p2p_pipe->_data_pipe._dispatch_info._down_range._num != 0);		/*if range is NULL means logical errors*/
			LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p_tcp_device_recv_data_callback, p2p recv data and commit to datamanager, range = [%u, %u]", p2p_pipe, p2p_pipe->_device,
				p2p_pipe->_data_pipe._dispatch_info._down_range._index, 1);
			dp_get_download_range(&p2p_pipe->_data_pipe, &down_range);
			rng._index = down_range._index;
			rng._num = 1;
			pi_put_data(&p2p_pipe->_data_pipe, &rng, &p2p_pipe->_socket_device->_data_buffer, p2p_pipe->_socket_device->_request_data_len, p2p_pipe->_socket_device->_data_buffer_len, p2p_pipe->_data_pipe._p_resource);
			sd_assert(p2p_pipe->_socket_device->_data_buffer == NULL);
			p2p_pipe->_socket_device->_data_buffer = NULL;
			p2p_pipe->_socket_device->_data_buffer_len = 0;
			p2p_pipe->_socket_device->_request_data_len = 0;
			p2p_pipe->_socket_device->_data_buffer_offset = 0;
			list_pop(&p2p_pipe->_request_data_queue, (void**)&req_range);
			p2p_free_range(req_range);
			++down_range._index;
			--down_range._num;
			dp_set_download_range(&p2p_pipe->_data_pipe, &down_range);
			ret = p2p_pipe_request_data(p2p_pipe);		/*request next data*/
			if(ret != SUCCESS)
			{
				LOG_DEBUG("p2p_pipe = 0x%x, device = 0x%x]p2p_pipe_request_data failed, errcode = %d", p2p_pipe, p2p_pipe->_device, ret);
				return p2p_pipe_handle_error(p2p_pipe, ret);
			}
		}
	}
	p2p_pipe->_socket_device->_cmd_buffer_offset = 0;
	if(p2p_pipe->_remote_protocol_ver > P2P_PROTOCOL_VERSION_57)	
	{
		return p2p_socket_device_recv_discard_data(p2p_pipe, 25);	/*丢弃request_response命令最后的25个字节的p2p命令数据，这些数据目前不需要用到*/
	}
	else
	{
		return p2p_socket_device_recv_cmd(p2p_pipe, P2P_CMD_HEADER_LENGTH);		/*receive next p2p command*/
	}
}

_int32 p2p_socket_device_recv_diacard_data_callback(_int32 errcode, PTL_DEVICE* device, _u32 had_recv)
{
	P2P_DATA_PIPE* p2p_pipe = (P2P_DATA_PIPE*)device->_user_data;
	if(errcode != SUCCESS)
		return p2p_pipe_handle_error(p2p_pipe, errcode);
	if(p2p_pipe->_data_pipe._dispatch_info._pipe_state == PIPE_FAILURE)
		return SUCCESS;
    //以后版本升级，加了字段，会有问题
	sd_assert(had_recv == 25);

#ifdef _SUPPORT_MHXY
      BOOL is_cdn = p2p_is_cdn_resource(p2p_pipe->_data_pipe._p_resource);
      BOOL support_tcp_mhxy = p2p_is_cdn_resource(p2p_pipe->_data_pipe._p_resource) && p2p_is_support_mhxy((DATA_PIPE *)p2p_pipe) && TCP_TYPE==p2p_pipe->_device->_type;
      if ( is_cdn && support_tcp_mhxy )
      {
          //模糊协议解密,丢弃的包数据也要解密，要不然会影响后面数据的解密
    	    TCP_DEVICE *p_tcp_device = (TCP_DEVICE *)p2p_pipe->_device->_device;
    	    _u32 decrypt_body_len = had_recv;
    	    _int32 ret = mhxy_tcp_decrypt2( p_tcp_device->_mhxy, p2p_pipe->_socket_device->_cmd_buffer+p2p_pipe->_socket_device->_cmd_buffer_offset, &decrypt_body_len);
           LOG_DEBUG("mhxy diacard request data: pipe:0x%x recv len:%d decrypt_len:%u", p2p_pipe, had_recv, decrypt_body_len);
    	    sd_assert(0!=ret);          
      }
#endif    
//	LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p_tcp_device_recv_discard_data_callback.", p2p_pipe, p2p_pipe->_device);
	/*discard this data, and recv next p2p command*/
	p2p_pipe->_socket_device->_cmd_buffer_offset = 0;
	return p2p_socket_device_recv_cmd(p2p_pipe, P2P_CMD_HEADER_LENGTH);
}

_int32 p2p_notify_close_socket_device(PTL_DEVICE* device)
{
	P2P_DATA_PIPE* p2p_pipe = (P2P_DATA_PIPE*)device->_user_data;
	p2p_destroy_socket_device(p2p_pipe);
	return p2p_pipe_notify_close(p2p_pipe);
}

_int32 get_data_buffer_timeout_handler(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid)
{
	_int32 ret = SUCCESS;
	_u32 data_len;
	P2P_DATA_PIPE*  p2p_pipe = (P2P_DATA_PIPE*)msg_info->_user_data;
	sd_assert(notice_count_left == 0);
	if(errcode == MSG_CANCELLED)
	{	
		LOG_DEBUG("get_data_buffer_timeout_handler error, since timeout message is cancel.");
		return SUCCESS;
	}
	p2p_pipe->_socket_device->_get_data_buffer_timeout_id = INVALID_MSG_ID;
	data_len = msg_info->_device_id;
	if(errcode != MSG_TIMEOUT)
	{
		LOG_ERROR("[p2p_pipe = 0x%x, device = 0x%x]get_data_buffer_timeout_handler failed, error code is %d.", p2p_pipe, p2p_pipe->_device, errcode);
		return p2p_pipe_handle_error(p2p_pipe, errcode);
	}
	if(p2p_pipe->_data_pipe._dispatch_info._pipe_state == PIPE_FAILURE)
	{
		LOG_ERROR("[p2p_pipe = 0x%x, device = 0x%x]get_data_buffer_timeout_handler, but data pipe state is PIPE_FAILURE, just return.", p2p_pipe, p2p_pipe->_device);
		return SUCCESS;
	}
	if(p2p_pipe->_data_pipe._p_data_manager == NULL)
		return SUCCESS;			/*p2p_pipe have been close and _p_data_manager is set to NULL*/
	ret = pi_get_data_buffer(&p2p_pipe->_data_pipe, &p2p_pipe->_socket_device->_data_buffer, &p2p_pipe->_socket_device->_data_buffer_len);
	if(ret != SUCCESS)
	{
		if(ret == DATA_BUFFER_IS_FULL || ret == OUT_OF_MEMORY)
		{
			LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]dm_get_data_buffer failed, start timer to retry.", p2p_pipe, p2p_pipe->_device);
			pipe_set_err_get_buffer(&p2p_pipe->_data_pipe, TRUE);
			return start_timer(get_data_buffer_timeout_handler, NOTICE_ONCE, GET_DATA_TIMEOUT, data_len, p2p_pipe, &p2p_pipe->_socket_device->_get_data_buffer_timeout_id);
		}
		else
		{
			LOG_ERROR("[p2p_pipe = 0x%x, device = 0x%x]dm_get_data_buffer failed, pipe set to FAILURE.", p2p_pipe, p2p_pipe->_device);
			return p2p_pipe_handle_error(p2p_pipe, ret);
		}
	}
	pipe_set_err_get_buffer(&p2p_pipe->_data_pipe, FALSE);
	p2p_pipe->_socket_device->_get_data_buffer_timeout_id = INVALID_MSG_ID;		/*remember to set timeout id invalid*/
	p2p_pipe->_socket_device->_data_buffer_offset = 0;
	if(data_len > p2p_pipe->_socket_device->_request_data_len - p2p_pipe->_socket_device->_data_buffer_offset)
	{	/*receive data length is greater than request data length, must be logical error*/
		sd_assert(FALSE);
		return p2p_pipe_handle_error(p2p_pipe, errcode);
	}
	LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]get_data_buffer_timeout_handler success, try to recv data.", p2p_pipe, p2p_pipe->_device);
	ret = ptl_recv_data(p2p_pipe->_device, p2p_pipe->_socket_device->_data_buffer + p2p_pipe->_socket_device->_data_buffer_offset, data_len, p2p_pipe->_socket_device->_data_buffer_len - p2p_pipe->_socket_device->_data_buffer_offset);
	sd_assert(ret == SUCCESS);
	return ret;
}

