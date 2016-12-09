#include "ptl_passive_punch_hole.h"
#include "ptl_cmd_sender.h"
#include "ptl_cmd_define.h"
#include "udt/udt_utility.h"
#include "udt/udt_device.h"
#include "udt/udt_device_manager.h"
#include "utility/mempool.h"
#include "utility/map.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_P2P_TRANSFER_LAYER
#include "utility/logger.h"

typedef struct tagPASSIVE_PUNCH_HOLE_DATA
{
	CONN_ID		_id;
	_u32		_remote_ip;
	_u16		_remote_port;
	_u16		_latest_remote_port;
	_u16		_guess_remote_port;
	_u32		_retry_times;
	_u32		_timeout_id;
}PASSIVE_PUNCH_HOLE_DATA;

static	SET		g_passive_punch_hole_data_set;			//node is PASSIVE_PUNCH_HOLE_DATA

#define	PTL_PUNCH_HOLE_TIMEOUT		(5000)
#define	PTL_PUNCH_HOLE_MAX_RETRY	(10)


_int32 ptl_init_passive_punch_hole()
{
	LOG_DEBUG("ptl_init_passvie_punch_hole.");
	set_init(&g_passive_punch_hole_data_set, conn_id_comparator);
	return SUCCESS;
}

_int32 ptl_uninit_passive_punch_hole()
{
	PASSIVE_PUNCH_HOLE_DATA* data = NULL;
	SET_ITERATOR cur_iter, next_iter; 
	LOG_DEBUG("ptl_uninit_passive_punch_hole.");
	cur_iter = SET_BEGIN(g_passive_punch_hole_data_set);
	while(cur_iter != SET_END(g_passive_punch_hole_data_set))
	{
		next_iter = SET_NEXT(g_passive_punch_hole_data_set, cur_iter);
		data = (PASSIVE_PUNCH_HOLE_DATA*)SET_DATA(cur_iter);
		ptl_erase_passive_punch_hole_data(data);
		cur_iter = next_iter;
		data = NULL;
	}
	return SUCCESS;
}

_int32 ptl_recv_someonecallyou_cmd(struct tagSOMEONECALLYOU_CMD* cmd)
{
	_int32 ret = SUCCESS;
	PASSIVE_PUNCH_HOLE_DATA* data = NULL;
	CONN_ID id;
	id._virtual_source_port = 0;
	id._virtual_target_port = cmd->_virtual_port;
	id._peerid_hashcode = udt_hash_peerid(cmd->_source_peerid);
	set_find_node(&g_passive_punch_hole_data_set, (void*)&id, (void**)&data);
	if(data != NULL)
	{
		LOG_DEBUG("ptl_recv_someonecallyou_cmd, peerid = %s, conn_id[%u, %u, %u].", (_u32)cmd->_source_peerid, (_u32)id._virtual_source_port,
					id._virtual_target_port, id._peerid_hashcode);
		return SUCCESS;
	}
	ret = sd_malloc(sizeof(PASSIVE_PUNCH_HOLE_DATA), (void**)&data);
	CHECK_VALUE(ret);
	data->_id = id;
	data->_remote_ip = cmd->_source_ip;
	data->_remote_port = cmd->_source_port;
	data->_latest_remote_port = cmd->_latest_remote_port;
	data->_guess_remote_port = cmd->_guess_remote_port;
	data->_retry_times = 0;
	start_timer(ptl_handle_punch_hole_timeout, NOTICE_INFINITE, PTL_PUNCH_HOLE_TIMEOUT, 0, data, &data->_timeout_id);
	ret = set_insert_node(&g_passive_punch_hole_data_set, data);
	CHECK_VALUE(ret);
	LOG_DEBUG("ptl_recv_someonecallyou_cmd, peerid = %s, start punch_hole, conn_id[%u, %u, %u].", cmd->_source_peerid, (_u32)data->_id._virtual_source_port, (_u32)data->_id._virtual_target_port, data->_id._peerid_hashcode);
	ret = ptl_send_punch_hole_cmd(id._virtual_source_port, id._virtual_target_port, cmd->_source_ip,
							   cmd->_source_port, cmd->_latest_remote_port, cmd->_guess_remote_port);
	return ret;
}

_int32 ptl_handle_punch_hole_timeout(const MSG_INFO * msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid)
{
	PASSIVE_PUNCH_HOLE_DATA* data = (PASSIVE_PUNCH_HOLE_DATA*)msg_info->_user_data;
	if(errcode == MSG_CANCELLED)
		return SUCCESS;
	++data->_retry_times;
	if(data->_retry_times >= PTL_PUNCH_HOLE_MAX_RETRY)
	{
		LOG_DEBUG("ptl_punch_hole_timeout, set_erase_node, conn_id[%u, %u, %u].", (_u32)data->_id._virtual_source_port, (_u32)data->_id._virtual_target_port, data->_id._peerid_hashcode);
		return ptl_erase_passive_punch_hole_data(data);
	}
	return ptl_send_punch_hole_cmd(data->_id._virtual_source_port, data->_id._virtual_target_port, data->_remote_ip,
									data->_remote_port, data->_latest_remote_port, data->_guess_remote_port);
}

_int32 ptl_remove_passive_punch_hole_data(struct tagSYN_CMD* cmd)
{
	PASSIVE_PUNCH_HOLE_DATA* data = NULL;
	CONN_ID id;
	sd_assert(cmd->_flags == 0);
	id._virtual_source_port = cmd->_target_port;
	id._virtual_target_port = cmd->_source_port;
	id._peerid_hashcode = cmd->_peerid_hashcode;
	set_find_node(&g_passive_punch_hole_data_set, &id, (void**)&data);
	if(data == NULL)
	{
		LOG_DEBUG("ptl_remove_passive_punch_hole_data, but data can't find, conn_id[%u, %u, %u].",
					(_u32)id._virtual_source_port, (_u32)id._virtual_target_port, id._peerid_hashcode);
		return SUCCESS;			//已经不存在了
	}
	//收到syn就说明打通了
	return ptl_erase_passive_punch_hole_data(data);
}

_int32 ptl_erase_passive_punch_hole_data(PASSIVE_PUNCH_HOLE_DATA* data)
{
	_int32 ret = SUCCESS;
	sd_assert(data->_timeout_id != INVALID_MSG_ID);
	LOG_DEBUG("ptl_erase_passive_punch_hole_data, conn_id[%u, %u, %u].", (_u32)data->_id._virtual_source_port, (_u32)data->_id._virtual_target_port, data->_id._peerid_hashcode);
	ret = set_erase_node(&g_passive_punch_hole_data_set, data);
	CHECK_VALUE(ret);
	ret = cancel_timer(data->_timeout_id);
	CHECK_VALUE(ret);
	data->_timeout_id = INVALID_MSG_ID;
	sd_free(data);
	data = NULL;
	return ret;
}




