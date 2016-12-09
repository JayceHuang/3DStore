#include "ptl_get_peersn.h"
#include "ptl_udp_device.h"
#include "ptl_cmd_sender.h"
#include "ptl_memory_slab.h"
#include "ptl_cmd_define.h"
#include "ptl_cmd_extractor.h"
#include "asyn_frame/msg.h"
#include "asyn_frame/device.h"
#include "utility/settings.h"
#include "utility/map.h"
#include "utility/string.h"
#include "utility/errcode.h"
#include "utility/mempool.h"
#include "utility/time.h"
#include "utility/logid.h"
#define  LOGID	LOGID_P2P_TRANSFER_LAYER
#include "utility/logger.h"

#define	MAX_CACHE_PEERSN_SIZE	(200)

static SET	g_get_peersn_data_set;				/*node type is GET_PEERSN_DATA*/
static _u32	g_get_peersn_timeout = 10 * 1000;
static _u32	g_get_peersn_timeout_id = INVALID_MSG_ID;
static SET	g_peersn_cache_data_set;			/*the super node cache, node type is PEERSN_CACHE_DATA*/
static _u32	g_peersn_cache_timeout = 120 * 1000;
static _u32	g_peersn_cache_timeout_id = INVALID_MSG_ID;

_int32 ptl_get_peersn_data_comparator(void *E1, void *E2)
{
	_int32 result = 0;
	GET_PEERSN_DATA* left = (GET_PEERSN_DATA*)E1;
	GET_PEERSN_DATA* right = (GET_PEERSN_DATA*)E2;
	result = sd_strcmp(left->_peerid, right->_peerid);
	if(result != 0)
		return result;
	else
		return (_int32)left->_user_data - (_int32)right->_user_data;
}	

_int32 ptl_peersn_cache_data_comparator(void *E1, void *E2)
{
	return sd_strcmp(((PEERSN_CACHE_DATA*)E1)->_peerid, ((PEERSN_CACHE_DATA*)E2)->_peerid);
}

_int32 ptl_init_get_peersn(void)
{
	LOG_DEBUG("ptl_init_get_peersn.");
	settings_get_int_item("p2p_setting.peersn_req_timeout", (_int32*)&g_get_peersn_timeout);
	settings_get_int_item("p2p_setting.cache_peersn_timeout", (_int32*)&g_peersn_cache_timeout);
	start_timer(ptl_handle_get_peersn_timeout, NOTICE_INFINITE, g_get_peersn_timeout, 0, NULL, &g_get_peersn_timeout_id);
	start_timer(ptl_handle_peersn_cache_timeout, NOTICE_INFINITE, g_peersn_cache_timeout, 0, NULL, &g_peersn_cache_timeout_id);
	set_init(&g_get_peersn_data_set, ptl_get_peersn_data_comparator);
	set_init(&g_peersn_cache_data_set, ptl_peersn_cache_data_comparator);
	return SUCCESS;
}

_int32 ptl_uninit_get_peersn(void)
{
	GET_PEERSN_DATA* peersn_data = NULL;
	PEERSN_CACHE_DATA*  cache_data = NULL;
	SET_ITERATOR cur_iter = NULL, next_iter = NULL;
	LOG_DEBUG("ptl_uninit_get_peersn.");
	if(g_get_peersn_data_set._comp == NULL)
		return SUCCESS;			/*init_peersn_requestor not call before*/
	//清空所有的ger_peersn请求
	cur_iter = SET_BEGIN(g_get_peersn_data_set);	
	while(cur_iter != SET_END(g_get_peersn_data_set))
	{
		next_iter = SET_NEXT(g_get_peersn_data_set, cur_iter);
		peersn_data = (GET_PEERSN_DATA*)(cur_iter->_data);
		peersn_data->_callback(MSG_CANCELLED, 0, 0, peersn_data->_user_data);
		ptl_erase_get_peersn_data(peersn_data);
		cur_iter = next_iter;
	}
	//清空所有的peersn_cache
	if(g_peersn_cache_data_set._comp == NULL)
		return SUCCESS;			/*init_peersn_cache not call before*/
	cur_iter = SET_BEGIN(g_peersn_cache_data_set);
	while(cur_iter != SET_END(g_peersn_cache_data_set))
	{
		next_iter = SET_NEXT(g_peersn_cache_data_set, cur_iter);
		cache_data = (PEERSN_CACHE_DATA*)(cur_iter->_data);
		ptl_free_peersn_cache_data(cache_data);	
		set_erase_iterator(&g_peersn_cache_data_set, cur_iter);	
		cur_iter = next_iter;
	}
	if(g_get_peersn_timeout_id != INVALID_MSG_ID)
		cancel_timer(g_get_peersn_timeout_id);
	if(g_peersn_cache_timeout_id != INVALID_MSG_ID)
		cancel_timer(g_peersn_cache_timeout_id);
	return SUCCESS;
}

_int32 ptl_get_peersn(const char* peerid, ptl_get_peersn_callback callback, void* user_data)
{
	MSG_INFO msg = {};
	GET_PEERSN_DATA* data = NULL;
	_int32 ret = SUCCESS;
	LOG_DEBUG("ptl_get_peersn, peerid = %s, callback = 0x%x, user_data = 0x%x", peerid, callback, user_data);
	ret = ptl_malloc_get_peersn_data(&data);
	CHECK_VALUE(ret);
	sd_memcpy(data->_peerid, peerid, PEER_ID_SIZE + 1);
	data->_callback = callback;
	data->_user_data = user_data;
	data->_state = PEERSN_INIT_STATE;
	sd_time_ms(&data->_timestamp);
	data->_try_times = 1;
	data->_msg_id = INVALID_MSG_ID;
	ret = set_insert_node(&g_get_peersn_data_set, data);
	CHECK_VALUE(ret);
	//这个请求的peerid对应的sn 在cache里面
	if(ptl_is_peersn_in_cache(peerid) == TRUE)
	{
		data->_state = PEERSN_IN_CACHE_STATE;
		msg._device_id = 0;
		msg._user_data = data;
		msg._device_type = DEVICE_NONE;
		msg._operation_type = OP_NONE;
		ret = post_message(&msg, ptl_peersn_in_cache_callback, NOTICE_ONCE, 0, &data->_msg_id);
		CHECK_VALUE(ret);
		return ret;
	}
	//不在cache里面，去服务器查询
	data->_state = PEERSN_GETTING_STATE;
	ret = ptl_send_get_peersn_cmd(peerid);
	CHECK_VALUE(ret);
	return ret;
}

_int32 ptl_cancel_get_peersn(const char* peerid, void* user_data)
{
	GET_PEERSN_DATA data;
	GET_PEERSN_DATA* result = NULL;
	sd_memcpy(data._peerid, peerid, PEER_ID_SIZE + 1);
	data._user_data = user_data;
	set_find_node(&g_get_peersn_data_set, &data, (void**)&result);
	sd_assert(result != NULL);
	result->_state = PEERSN_CANCEL_STATE;
	return SUCCESS;
}

_int32 ptl_handle_get_peersn_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid)
{
	/*remove or retry get_peersn which is timeout*/
	_u32 timestamp;
	GET_PEERSN_DATA* data = NULL;
	SET_ITERATOR cur_iter = NULL, next_iter = NULL;
	if(errcode == MSG_CANCELLED)
		return SUCCESS;
	sd_time_ms(&timestamp);
	cur_iter = SET_BEGIN(g_get_peersn_data_set);
	while(cur_iter != SET_END(g_get_peersn_data_set))
	{
		next_iter = SET_NEXT(g_get_peersn_data_set, cur_iter);
		data = (GET_PEERSN_DATA*)cur_iter->_data;
		if(data->_state == PEERSN_CANCEL_STATE)
		{	//被上层取消了
			data->_callback(MSG_CANCELLED, 0, 0, data->_user_data);
			ptl_erase_get_peersn_data(data);
		}
		else if(TIME_GE(timestamp, data->_timestamp + g_get_peersn_timeout))
		{
			if(data->_try_times < 5)
			{	/*retry*/
				++data->_try_times;
				data->_timestamp = timestamp;
				LOG_DEBUG("ptl_handle_get_peersn_timeout, ptl_get_peersn, target_peerid = %s, retry_times = %u.", data->_peerid, data->_try_times);
				ptl_send_get_peersn_cmd(data->_peerid);
			}
			else
			{	/*remove*/
				LOG_DEBUG("ptl_get_peersn timeout, peerid = %s.", data->_peerid);
				data->_callback(MSG_TIMEOUT, 0, 0, data->_user_data);
				ptl_erase_get_peersn_data(data);
			}
		}
		cur_iter = next_iter;
	};
	return SUCCESS;
}

_int32 ptl_recv_get_peersn_resp_cmd(GET_PEERSN_RESP_CMD* cmd)
{
	_int32 ret = SUCCESS;
	_int32 errcode = SUCCESS;
	SET_ITERATOR iter;
	GET_PEERSN_DATA* data = NULL;
	LOG_DEBUG("ptl_recv_get_peersn_resp_cmd, peerid = %s", cmd->_peerid);
	for(iter = SET_BEGIN(g_get_peersn_data_set); iter != SET_END(g_get_peersn_data_set); iter = SET_NEXT(g_get_peersn_data_set, iter))
	{
		data = (GET_PEERSN_DATA*)iter->_data;
		if(sd_strcmp(data->_peerid, cmd->_peerid) == 0)
			break;
	}
	if(data == NULL)
	{
		LOG_DEBUG("recv peersn response, but this peersn I have processed, peerid = %s", cmd->_peerid);
		return SUCCESS;
	}
	if(cmd->_result == 1)
		errcode = SUCCESS;
	else
		errcode = ERR_PTL_GET_PEERSN_FAILED;
	if(data->_state == PEERSN_CANCEL_STATE)
		errcode = MSG_CANCELLED;
	data->_callback(errcode, cmd->_sn_ip, cmd->_sn_port, data->_user_data);
	/*remove peersn request*/
	ptl_erase_get_peersn_data(data);
	/*cache peersn*/
	ret = ptl_cache_peersn(cmd->_peerid, cmd->_sn_ip, cmd->_sn_port);
	CHECK_VALUE(ret);
	return SUCCESS;
}

BOOL ptl_is_peersn_in_cache(const char* peerid)
{
	_u32 now;
	PEERSN_CACHE_DATA* data = NULL;
	set_find_node(&g_peersn_cache_data_set, (void*)peerid, (void**)&data);	
	if(data == NULL)
		return FALSE;			/*target peer's super node not exist*/
	sd_time_ms(&now);
	if(TIME_GE(now, data->_add_time + g_peersn_cache_timeout))
		return FALSE;			/*target peer's super node is timeout, invalid*/
	else
		return TRUE;
}

_int32 ptl_peersn_in_cache_callback(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid)
{
	_u32 now = 0;
	PEERSN_CACHE_DATA* cache_data = NULL;
	GET_PEERSN_DATA* peersn_data = (GET_PEERSN_DATA*)msg_info->_user_data;
	LOG_DEBUG("ptl_peersn_in_cache_callback, errcode = %d, elapsed = %u, msgid = %u.", errcode, elapsed, msgid);
	if(errcode == MSG_CANCELLED)
		return SUCCESS;				//被取消掉了
	LOG_DEBUG("ptl_peersn_in_cache_callback,  peerid = %s",  peersn_data->_peerid);
	sd_assert(peersn_data->_msg_id == msgid);
	peersn_data->_msg_id = INVALID_MSG_ID;
	if(peersn_data->_state == PEERSN_CANCEL_STATE)
	{	//该请求被上层cancel掉了
		peersn_data->_callback(MSG_CANCELLED, 0, 0, peersn_data->_user_data);
		return ptl_erase_get_peersn_data(peersn_data);
	}
	set_find_node(&g_peersn_cache_data_set, peersn_data->_peerid, (void**)&cache_data);
	if(cache_data == NULL)
	{	//该cache数据已经不存在
		peersn_data->_state = PEERSN_GETTING_STATE;
		return ptl_send_get_peersn_cmd(peersn_data->_peerid);
	}
	sd_time_ms(&now);
	if(TIME_GE(now, cache_data->_add_time + g_peersn_cache_timeout))
	{	//该cache数据已经超时了
		peersn_data->_state = PEERSN_GETTING_STATE;
		return ptl_send_get_peersn_cmd(peersn_data->_peerid);
	}
	//在cache中找到了有效数据
	peersn_data->_callback(SUCCESS, cache_data->_sn_ip, cache_data->_sn_port, peersn_data->_user_data);
	return ptl_erase_get_peersn_data(peersn_data);
}

_int32 ptl_handle_peersn_cache_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid)
{
	_u32 now;
	PEERSN_CACHE_DATA*	data = NULL;
	SET_ITERATOR cur_iter, next_iter;
	LOG_DEBUG("ptl_handle_peersn_cache_timeout...");
	if(set_size(&g_peersn_cache_data_set) < MAX_CACHE_PEERSN_SIZE)
		return SUCCESS;
	sd_time_ms(&now);
	cur_iter = SET_BEGIN(g_peersn_cache_data_set);
	next_iter = NULL;
	while(cur_iter != SET_END(g_peersn_cache_data_set))
	{
		next_iter = SET_NEXT(g_peersn_cache_data_set, cur_iter);
		data = (PEERSN_CACHE_DATA*)(cur_iter->_data);
		if(TIME_GE(now, data->_add_time + g_peersn_cache_timeout))
		{
			set_erase_iterator(&g_peersn_cache_data_set, cur_iter);
			ptl_free_peersn_cache_data(data);
		}
		cur_iter = next_iter;
	}
	return SUCCESS;
}

_int32 ptl_cache_peersn(const char* peerid, _u32 sn_ip, _u16 sn_port)
{
	_int32 ret = SUCCESS;
	PEERSN_CACHE_DATA*	data = NULL;
	set_find_node(&g_peersn_cache_data_set, (void*)peerid, (void**)&data);
	if(data != NULL)
	{
		data->_sn_ip = sn_ip;
		data->_sn_port = sn_port;
		sd_time_ms(&data->_add_time);
		return SUCCESS;
	}
	else
	{	/*insert new cache*/
		ret = ptl_malloc_peersn_cache_data(&data);
		CHECK_VALUE(ret);
		sd_memcpy(data->_peerid, peerid, PEER_ID_SIZE + 1);
		data->_sn_ip = sn_ip;
		data->_sn_port = sn_port;
		sd_time_ms(&data->_add_time);
		return set_insert_node(&g_peersn_cache_data_set, data);
	}
}

_int32 ptl_erase_get_peersn_data(GET_PEERSN_DATA* data)
{
	_int32 ret = SUCCESS;
	LOG_DEBUG("ptl_erase_get_peersn_data, peerid = %s, callback = 0x%x, user_data = 0x%x.", data->_peerid, data->_callback, data->_user_data);
	if(data->_msg_id != INVALID_MSG_ID)
	{
		cancel_message_by_msgid(data->_msg_id);
		data->_msg_id = INVALID_MSG_ID;
	}
	ret = set_erase_node(&g_get_peersn_data_set, data);
	CHECK_VALUE(ret);
	return ptl_free_get_peersn_data(data);
}

