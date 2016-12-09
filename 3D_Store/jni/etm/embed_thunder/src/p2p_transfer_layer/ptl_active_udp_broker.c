#include "ptl_active_udp_broker.h"
#include "ptl_interface.h"
#include "ptl_cmd_define.h"
#include "ptl_get_peersn.h"
#include "ptl_callback_func.h"
#include "ptl_cmd_sender.h"
#include "utility/time.h"
#include "utility/string.h"
#include "utility/utility.h"
#include "utility/map.h"
#include "utility/mempool.h"
#include "utility/errcode.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_P2P_TRANSFER_LAYER
#include "utility/logger.h"

#define	PTL_ACTIVE_UDP_BROKER_TIMEOUT		(6000)
#define	PTL_ACTIVE_UDP_BROKER_MAX_RETRY	(3)

static	_u32	g_seq_num = 0;
static	SET		g_active_udp_broker_data_set;

_int32 ptl_active_udp_broker_data_comparator(void *E1, void *E2)
{
	return (_int32)((_int32)E1 - (_int32)E2);
}

_int32 ptl_init_active_udp_broker()
{
	_u32 timer = 0;
	LOG_DEBUG("ptl_init_active_udp_broker.");
	sd_time(&timer);
	sd_srand(timer);
	//注意这里一定要转换为_u16，在外面的迅雷5版本有bug,如果g_seq_num
	//超过_u16的表示范围，那么active_broker将不能连接成功,原因是迅雷5的
	//实现中把32位数赋给16位数导致该bug.
	g_seq_num = (_u16)sd_rand();		
	set_init(&g_active_udp_broker_data_set, ptl_active_udp_broker_data_comparator);
	return SUCCESS;
}

_int32 ptl_uninit_active_udp_broker()
{
	ACTIVE_UDP_BROKER_DATA* data = NULL;
	SET_ITERATOR cur_iter, next_iter; 
	LOG_DEBUG("ptl_uninit_active_udp_broker.");
	cur_iter = SET_BEGIN(g_active_udp_broker_data_set);
	while (cur_iter != SET_END(g_active_udp_broker_data_set))
	{
		next_iter = SET_NEXT(g_active_udp_broker_data_set, cur_iter);
		data = (ACTIVE_UDP_BROKER_DATA*)SET_DATA(cur_iter);
		ptl_erase_active_udp_broker_data(data);
		cur_iter = next_iter;
		data = NULL;
	}
	return SUCCESS;
}

_int32 ptl_active_udp_broker(PTL_DEVICE* device, const char* remote_peerid)
{
	_int32 ret = SUCCESS;
	ACTIVE_UDP_BROKER_DATA* data = NULL;
	ret = sd_malloc(sizeof(ACTIVE_UDP_BROKER_DATA), (void**)&data);
	CHECK_VALUE(ret);
	sd_memset(data, 0, sizeof(ACTIVE_UDP_BROKER_DATA));
	data->_device = device;
	data->_seq_num = g_seq_num++;
	data->_timeout_id = INVALID_MSG_ID;
	data->_retry_time = 0;
	sd_memcpy(data->_remote_peerid, remote_peerid, PEER_ID_SIZE);
	ret = set_insert_node(&g_active_udp_broker_data_set, data);
	CHECK_VALUE(ret);
	ret = ptl_get_peersn(remote_peerid, ptl_active_udp_broker_get_peersn_callback, data);
	CHECK_VALUE(ret);
	return ret;
}

_int32 ptl_active_udp_broker_get_peersn_callback(_int32 errcode, _u32 sn_ip, _u16 sn_port, void* user_data)
{
	_int32 ret = SUCCESS;
	ACTIVE_UDP_BROKER_DATA* data = (ACTIVE_UDP_BROKER_DATA*)user_data;
	if(errcode == MSG_CANCELLED)
		return SUCCESS;
	if(data->_device == NULL)
	{	//上层p2p_pipe已经关闭
		return ptl_erase_active_udp_broker_data(data);
	}
	if(errcode != SUCCESS)
	{
		//这里回调的时候会不会p2p_pipe已经失效了呢，有空处理下
		ptl_connect_callback(errcode, data->_device);	
		return ptl_erase_active_udp_broker_data(data);
	}
	data->_sn_ip = sn_ip;
	data->_sn_port = sn_port;
	ret = ptl_send_udp_broker_req_cmd(data->_seq_num, data->_remote_peerid, sn_ip, sn_port);
	CHECK_VALUE(ret);
    if(data->_timeout_id != INVALID_MSG_ID) return SUCCESS;
	return start_timer(ptl_handle_active_udp_broker_timeout, NOTICE_INFINITE, PTL_ACTIVE_UDP_BROKER_TIMEOUT, 0, data, &data->_timeout_id);	
}

_int32 ptl_handle_active_udp_broker_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid)
{
	ACTIVE_UDP_BROKER_DATA* data = NULL;
	data = (ACTIVE_UDP_BROKER_DATA*)msg_info->_user_data;
	if(errcode == MSG_CANCELLED)
		return SUCCESS;
	if(data->_device == NULL)
	{	//上层p2p_pipe已经关闭
		return ptl_erase_active_udp_broker_data(data);
	}
	++data->_retry_time;
	if(data->_retry_time < PTL_ACTIVE_UDP_BROKER_MAX_RETRY)
	{
		return ptl_send_udp_broker_req_cmd(data->_seq_num, data->_remote_peerid, data->_sn_ip, data->_sn_port);
	}
	else
	{
		ptl_connect_callback(-1, data->_device);
		return ptl_erase_active_udp_broker_data(data);
	}
}

ACTIVE_UDP_BROKER_DATA* ptl_find_active_udp_broker_data(_u32 seq_num)
{
	SET_ITERATOR iter;
	ACTIVE_UDP_BROKER_DATA* data = NULL;
	LOG_DEBUG("ptl_find_active_udp_broker_data, seq_num = %u.", seq_num);
	for(iter = SET_BEGIN(g_active_udp_broker_data_set); iter != SET_END(g_active_udp_broker_data_set); iter = SET_NEXT(g_active_udp_broker_data_set, iter))
	{
		data = (ACTIVE_UDP_BROKER_DATA*)iter->_data;
		if(data->_seq_num == seq_num)
		{
			return data;
		}
	}
	return NULL;
}

_int32 ptl_erase_active_udp_broker_data(ACTIVE_UDP_BROKER_DATA* data)
{
	_int32 ret = SUCCESS;
	LOG_DEBUG("[device = 0x%x]ptl_erase_active_udp_broker_data, seq_num = %u.", data->_device, data->_seq_num);
	ret = set_erase_node(&g_active_udp_broker_data_set, data);
	CHECK_VALUE(ret);
	if(data->_timeout_id != INVALID_MSG_ID)
	{
		cancel_timer(data->_timeout_id);
		data->_timeout_id = INVALID_MSG_ID;
	}
	return sd_free(data);
}

_int32 ptl_cancel_active_udp_broker_req(struct tagPTL_DEVICE* device)
{
	SET_ITERATOR iter;
	ACTIVE_UDP_BROKER_DATA* data = NULL;
	LOG_DEBUG("[device = 0x%x]ptl_cancel_active_udp_broker_req.", device);
	for(iter = SET_BEGIN(g_active_udp_broker_data_set); iter != SET_END(g_active_udp_broker_data_set); iter = SET_NEXT(g_active_udp_broker_data_set, iter))
	{
		data = (ACTIVE_UDP_BROKER_DATA*)iter->_data;
		if(data->_device == device)
		{
			LOG_DEBUG("[device = 0x%x]ptl_cancel_active_udp_broker_req success.", device);
			data->_device = NULL;
			return SUCCESS;
		}
	}
	return SUCCESS;
}

	






