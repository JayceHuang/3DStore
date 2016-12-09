#include "p2p_send_cmd.h"
#include "p2p_cmd_define.h"
#include "p2p_cmd_builder.h"
#include "p2p_socket_device.h"
#include "p2p_utility.h"
#include "p2p_pipe_interface.h"
#ifdef UPLOAD_ENABLE
#include "upload_manager/upload_manager.h"
#endif
#include "p2p_transfer_layer/ptl_interface.h"
#include "p2p_transfer_layer/ptl_utility.h"
#include "data_manager/data_manager_interface.h"
#include "utility/version.h"
#include "utility/bytebuffer.h"
#include "utility/peerid.h"
#include "utility/local_ip.h"
#include "utility/time.h"
#include "utility/mempool.h"
#include "utility/utility.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_P2P_PIPE
#include "utility/logger.h"

#define INTERESTED_RESP_BUFFER_LEN		1024

_int32 p2p_send_hanshake_cmd(P2P_DATA_PIPE* p2p_pipe)
{
	_int32 ret;
	_u32  len = 0;
	char* cmd_buffer = NULL;
	HANDSHAKE_CMD	handshake_cmd;
	P2P_RESOURCE*	peer_resource = (P2P_RESOURCE*)p2p_pipe->_data_pipe._p_resource;
	sd_assert(peer_resource != NULL && p2p_pipe->_data_pipe._p_resource->_resource_type == PEER_RESOURCE);
	sd_memset(&handshake_cmd, 0, sizeof(HANDSHAKE_CMD));
	handshake_cmd._handshake_connect_id = p2p_pipe->_handshake_connect_id;
	handshake_cmd._by_what = 1;		/*by gcid filesize*/
	handshake_cmd._gcid_len = CID_SIZE;
	sd_memcpy(handshake_cmd._gcid, peer_resource->_gcid, CID_SIZE);
	handshake_cmd._file_size = peer_resource->_file_size;
	handshake_cmd._file_status = P2P_FILE_STATUS_DOWNLOADING;
	handshake_cmd._peerid_len = PEER_ID_SIZE;
	get_peerid(handshake_cmd._peerid, PEER_ID_SIZE + 1);
	if(p2p_get_from(p2p_pipe->_data_pipe._p_resource) == P2P_FROM_VIP_HUB)
	{
		LOG_DEBUG("p2p_send_hanshake_cmd datepipe 0x%x is vip cdn .. dont set real internal ip", p2p_pipe); 
		sd_strncpy(handshake_cmd._internal_addr, "0.0.0.0", 8);
		handshake_cmd._internal_addr_len = 7;
	}
	else
	{
		LOG_DEBUG("p2p_send_hanshake_cmd datepipe 0x%x is normal peer", p2p_pipe); 
	ret = sd_inet_ntoa(sd_get_local_ip(), handshake_cmd._internal_addr, 24);
	handshake_cmd._internal_addr_len = sd_strlen(handshake_cmd._internal_addr);
	}
	sd_assert(handshake_cmd._internal_addr_len < 24);
	handshake_cmd._tcp_listen_port = ptl_get_local_tcp_port();
	handshake_cmd._product_ver = get_product_id();
	handshake_cmd._downbytes_in_history = 0;
	handshake_cmd._upbytes_in_history = 0;
	if(sd_is_in_nat(sd_get_local_ip()) == TRUE)
		handshake_cmd._not_in_nat = 0;
	else
		handshake_cmd._not_in_nat = 1;
	handshake_cmd._upload_speed_limit = 0;
	handshake_cmd._same_nat_tcp_speed_max = 0;
	handshake_cmd._diff_nat_tcp_speed_max = 0;
	handshake_cmd._udp_speed_max = 0;
	handshake_cmd._p2p_capability = get_p2p_capability();
	handshake_cmd._phub_res_count = 4;
	handshake_cmd._load_level = 0;
	handshake_cmd._home_location_len = 0;
	/*build package*/
	ret = build_handshake_cmd(&cmd_buffer, &len, p2p_pipe->_device, &handshake_cmd);
	CHECK_VALUE(ret);
	return p2p_socket_device_send(p2p_pipe,handshake_cmd._header._command_type, cmd_buffer, len);
}

_int32 p2p_send_interested_cmd(P2P_DATA_PIPE* p2p_pipe)
{
	_int32 ret;
	char* cmd_buffer = NULL;
	_u32 len = 0;
	INTERESTED_CMD interested;
	interested._by_what = 1;
	interested._min_block_size = 0;
	interested._max_block_count = MAX_U32;
	ret = build_interested_cmd(&cmd_buffer, &len, p2p_pipe->_device, &interested);
	CHECK_VALUE(ret);
	return p2p_socket_device_send(p2p_pipe, interested._header._command_type, cmd_buffer, len);
}

_int32 p2p_send_handshake_resp_cmd(P2P_DATA_PIPE* p2p_pipe, _u8 result)
{
	_int32 ret;
	char* cmd_buffer = NULL;
	_u32  len = 0;
	HANDSHAKE_RESP_CMD handshake_resp;
	sd_memset(&handshake_resp, 0, sizeof(HANDSHAKE_RESP_CMD));
	handshake_resp._result = result;
	handshake_resp._peerid_len = PEER_ID_SIZE;
	get_peerid(handshake_resp._peerid, PEER_ID_SIZE + 1);
	handshake_resp._product_ver = get_product_id();
	if(sd_is_in_nat(sd_get_local_ip()) == TRUE)
		handshake_resp._not_in_nat = 0;
	else
		handshake_resp._not_in_nat = 1;
	handshake_resp._p2p_capablility = get_p2p_capability();
	handshake_resp._phub_res_count = 4;
	ret = build_handshake_resp_cmd(&cmd_buffer, &len, p2p_pipe->_device, &handshake_resp);
	CHECK_VALUE(ret);
	return p2p_socket_device_send(p2p_pipe, handshake_resp._header._command_type, cmd_buffer, len);
}

_int32 p2p_send_request_cmd(P2P_DATA_PIPE* p2p_pipe, _u64 file_pos, _u64 file_len)
{
	_int32 ret = SUCCESS;
	char* cmd_buffer = NULL;
	_u32 len = 0;
	REQUEST_CMD	cmd;
	sd_memset(&cmd, 0, sizeof(REQUEST_CMD));
	cmd._by_what = 1;
	cmd._file_pos = file_pos;
	cmd._file_len = file_len;
	sd_assert(cmd._file_len > 0);
	cmd._max_package_size =  8192;
	cmd._priority = 5;	/*这个参数很关键，设置为最大表示请求优先级越高，这样对方的上传管理器会优先传数据给我，且不容易被choke*/
	ret = build_request_cmd(&cmd_buffer, &len, p2p_pipe->_device, &cmd);
	if(ret != SUCCESS)
	{
		LOG_ERROR("[p2p_pipe = 0x%x]build_request_cmd failed, errcode = %d.", p2p_pipe, ret);
		return ret;
	}
	sd_time_ms(&p2p_pipe->_data_pipe._dispatch_info._last_recv_data_time);
	return p2p_socket_device_send(p2p_pipe, REQUEST, cmd_buffer, len);
}

_int32 p2p_send_keepalive_cmd(P2P_DATA_PIPE* p2p_pipe)
{
	KEEPALIVE_CMD keepalive_cmd;
	char* buffer = NULL; _u32 len = 0;
	_int32 ret;
	ret = build_keepalive_cmd(&buffer, &len, p2p_pipe->_device, &keepalive_cmd);
	CHECK_VALUE(ret);
	return p2p_socket_device_send(p2p_pipe, keepalive_cmd._header._command_type, buffer, len);
}

_int32 p2p_send_cancel_cmd(P2P_DATA_PIPE* p2p_pipe)
{
	CANCEL_CMD cancel_cmd;
	char* buffer = NULL; _u32 len = 0;
	_int32 ret;
	ret = build_cancel_cmd(&buffer, &len, p2p_pipe->_device, &cancel_cmd);
	CHECK_VALUE(ret);
	return p2p_socket_device_send(p2p_pipe, cancel_cmd._header._command_type, buffer, len);
}

_int32 p2p_send_cancel_resp_cmd(P2P_DATA_PIPE* p2p_pipe)
{
	CANCEL_RESP_CMD cancel_resp_cmd;
	char* buffer = NULL;
	_u32 len = 0;
	_int32 ret = SUCCESS;
	ret = build_cancel_resp_cmd(&buffer, &len, p2p_pipe->_device, &cancel_resp_cmd);
	CHECK_VALUE(ret);
	return p2p_socket_device_send(p2p_pipe, cancel_resp_cmd._header._command_type, buffer, len);
}

_int32 p2p_send_fin_resp_cmd(P2P_DATA_PIPE* p2p_pipe)
{
	FIN_RESP_CMD cmd;
	char* buffer = NULL;
	_u32 len = 0;
	_int32 ret = SUCCESS;
	ret = build_fin_resp_cmd(&buffer, &len, p2p_pipe->_device, &cmd);
	CHECK_VALUE(ret);
	return p2p_socket_device_send(p2p_pipe, cmd._cmd_type, buffer, len);
}

#ifdef UPLOAD_ENABLE
_int32 p2p_send_choke_cmd(P2P_DATA_PIPE* p2p_pipe, BOOL choke)
{
	_int32 ret = SUCCESS;
	char* cmd_buffer = NULL;
	_u32 len = 0;
	ret = build_choke_cmd(&cmd_buffer, &len, p2p_pipe->_device, choke);
	CHECK_VALUE(ret);
	if(choke == FALSE)
		ret = p2p_socket_device_send(p2p_pipe, UNCHOKE, cmd_buffer, len);
	else
		ret = p2p_socket_device_send(p2p_pipe, CHOKE, cmd_buffer, len);
	return ret;
}

_int32 p2p_send_interested_resp_cmd(P2P_DATA_PIPE* p2p_pipe, _u32 min_block_size, _u32 max_block_num)
{
	_int32 ret = SUCCESS;	
	char* cmd_buffer = NULL;
	_u32 cmd_buffer_len = INTERESTED_RESP_BUFFER_LEN, offset = 0;
	_u32 *command_len_ptr = NULL, *block_num_ptr = NULL;
	char* tmp_buf;
	_int32  tmp_len;
	_u64 pos, length;	
	RANGE_LIST_ITEROATOR iter;
	RANGE_LIST* check_ranges = NULL;
	INTERESTED_RESP_CMD interested_resp_cmd;
	ret = ptl_malloc_cmd_buffer(&cmd_buffer, &cmd_buffer_len, &offset, p2p_pipe->_device);		//由于长度不固定，所以预先分配一个足够大的缓冲区
	if(ret != SUCCESS)
		return ret;
	interested_resp_cmd._header._version = P2P_PROTOCOL_VERSION;
	interested_resp_cmd._header._command_len = 0;	
	interested_resp_cmd._header._command_type = INTERESTED_RESP;
	interested_resp_cmd._download_ratio = 0;			//这个设为0，还不知道会不会有问题
	interested_resp_cmd._block_num = 0;
	tmp_buf = cmd_buffer + offset;
	tmp_len = cmd_buffer_len - offset;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)interested_resp_cmd._header._version);
	command_len_ptr = (_u32*)tmp_buf;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)interested_resp_cmd._header._command_len);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)interested_resp_cmd._header._command_type);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)interested_resp_cmd._download_ratio);
	block_num_ptr = (_u32*)tmp_buf;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)interested_resp_cmd._block_num);
	//接着开始填充range
	if(p2p_pipe->_data_pipe._p_data_manager == NULL)
	{	//纯上传的p2p_pipe
		pos = 0;
		ulm_get_file_size(&p2p_pipe->_data_pipe, &length);
		p2p_fill_interested_resp_block(&tmp_buf, &tmp_len, pos, length);
		interested_resp_cmd._block_num = 1;
	}
	else		
	{	
		check_ranges = pi_get_checked_range_list(&p2p_pipe->_data_pipe);
		if(check_ranges != NULL)
		{
			range_list_get_head_node(check_ranges, &iter);
			//一个block最大17个字节，要保证有足够空间填充一个block
			while(iter != NULL && interested_resp_cmd._block_num < max_block_num && tmp_len >= 17) 
			{
				if(get_data_unit_size() * iter->_range._num >= min_block_size)
				{	//这个是有效的
					pos = get_data_unit_size() * iter->_range._index;
					length = get_data_unit_size() * iter->_range._num;
					if(pos + length > pi_get_file_size(&p2p_pipe->_data_pipe))
						length = pi_get_file_size(&p2p_pipe->_data_pipe) - pos;		//如果是最后一个range，那么应该进行规整
					//添加了一个block
					LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p_fill_interested_resp_block. pos:%llu, len:%u", p2p_pipe, p2p_pipe->_device,  pos, length);		
					p2p_fill_interested_resp_block(&tmp_buf, &tmp_len, pos, length);
					++interested_resp_cmd._block_num;
				}
				range_list_get_next_node(check_ranges, iter, &iter);
			}
		}
		else
		{
			LOG_ERROR("[p2p_pipe = 0x%x, device = 0x%x]p2p_send_interested_resp_cmd, but check_ranges is NULL.", p2p_pipe, p2p_pipe->_device);
		}
	}
	//重新修正block数和命令长度
	*block_num_ptr = interested_resp_cmd._block_num;
	*command_len_ptr = INTERESTED_RESP_BUFFER_LEN - tmp_len - 8;  //减去4字节的version和4字节的command_len
	return p2p_socket_device_send(p2p_pipe, interested_resp_cmd._header._command_type, cmd_buffer, cmd_buffer_len - tmp_len);
}

_int32 p2p_fill_interested_resp_block(char** tmp_buf, _int32* tmp_len, _u64 pos, _u64 length)
{
	_u8	len = 0, len_hi = 0, len_low = 0, byte_value = 0;
	_u8* len_ptr = (_u8*)*tmp_buf;

	sd_set_int8(tmp_buf, tmp_len, len);			//这里先填充为0，之后再修改
	while(pos > 0 || len_low == 0)					//len_low == 0 说明是第一个字节
	{
		byte_value = (_u8)(pos & 0xff);
		sd_set_int8(tmp_buf, tmp_len, (_int8)byte_value);
		len_low++;
		pos >>= 8;
	}
	while(length > 0 || len_hi == 0)
	{
		byte_value = (_u8)(length & 0xff);
		sd_set_int8(tmp_buf, tmp_len, (_int8)byte_value);
		len_hi++;
		length >>= 8;
	}
	*len_ptr = (len_hi << 4) + len_low;
	return SUCCESS;
}
#endif


