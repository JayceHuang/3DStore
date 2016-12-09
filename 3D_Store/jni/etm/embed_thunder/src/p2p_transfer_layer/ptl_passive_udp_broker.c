#include "ptl_passive_udp_broker.h"
#include "ptl_cmd_define.h"
#include "ptl_passive_connect.h"
#include "udt/udt_utility.h"
#include "utility/map.h"
#include "utility/mempool.h"
#include "utility/string.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_P2P_TRANSFER_LAYER
#include "utility/logger.h"
#include "utility/time.h"

#define BORKER_DATA_MAX_EXIST_TIME 600

static	SET		g_passive_udp_broker_data_set;

_int32 ptl_passive_udp_broker_data_comparator(void *E1, void *E2)
{
	PASSIVE_UDP_BROKER_DATA* left = (PASSIVE_UDP_BROKER_DATA*)E1;
	PASSIVE_UDP_BROKER_DATA* right = (PASSIVE_UDP_BROKER_DATA*)E2;
	if(left->_seq != right->_seq)
		return (_int32)(left->_seq - right->_seq);
	return sd_strcmp(left->_remote_peerid, right->_remote_peerid);
}

_int32 ptl_init_passive_udp_broker(void)
{
	set_init(&g_passive_udp_broker_data_set, ptl_passive_udp_broker_data_comparator);
	return SUCCESS;
}

_int32 ptl_uninit_passive_udp_broker(void)
{
	PASSIVE_UDP_BROKER_DATA* data = NULL;
	SET_ITERATOR cur_iter, next_iter; 
	LOG_DEBUG("ptl_uninit_passive_udp_broker.");
	cur_iter = SET_BEGIN(g_passive_udp_broker_data_set);
	while(cur_iter != SET_END(g_passive_udp_broker_data_set))
	{
		next_iter = SET_NEXT(g_passive_udp_broker_data_set, cur_iter);
		data = (PASSIVE_UDP_BROKER_DATA*)SET_DATA(cur_iter);
		ptl_erase_passive_udp_broker_data(data);
		cur_iter = next_iter;
		data = NULL;
	}
	return SUCCESS;
}

_int32 ptl_passive_udp_broker(struct tagUDP_BROKER_CMD* cmd)
{
	_int32 ret = SUCCESS;
	PASSIVE_UDP_BROKER_DATA* data = NULL, *result = NULL;
	LOG_DEBUG("ptl_passive_udp_broker, seq = %u, remote_peerid_hashcode = %u.", cmd->_seq_num, udt_hash_peerid(cmd->_peerid));
	ret = sd_malloc(sizeof(PASSIVE_UDP_BROKER_DATA), (void**)&data);
	if(ret != SUCCESS)
		return ret;
	data->_seq = cmd->_seq_num;
	data->_ip = cmd->_ip;
	data->_udp_port = cmd->_udp_port;	
	sd_time( &data->_born_time );
	sd_memcpy(data->_remote_peerid, cmd->_peerid, PEER_ID_SIZE + 1);
	set_find_node(&g_passive_udp_broker_data_set, data, (void**)&result);
	if(result != SUCCESS)
	{
		LOG_DEBUG("ptl_passive_udp_broker, but request is exist.");
		return sd_free(data);
	}
	ret = ptl_accept_passive_udp_broker_connect(data);
	if(ret != SUCCESS)
	{
		return sd_free(data);
	}
	ret = set_insert_node(&g_passive_udp_broker_data_set, data);
	CHECK_VALUE(ret);
	return ret;
}

/*
PASSIVE_UDP_BROKER_DATA* ptl_find_passive_udp_broker_data(_u32 peerid_hashcode, _u32 broker_seq)
{
	SET_ITERATOR iter; 
	PASSIVE_UDP_BROKER_DATA* data = NULL;
	LOG_DEBUG("ptl_find_passive_udp_broker_data, peerid_hashcode = %u, broker_seq = %u.", peerid_hashcode, broker_seq);
	for(iter = SET_BEGIN(g_passive_udp_broker_data_set); iter != SET_END(g_passive_udp_broker_data_set); iter = SET_NEXT(g_passive_udp_broker_data_set, iter))
	{
		data = (PASSIVE_UDP_BROKER_DATA*)SET_DATA(iter);
		if(data->_seq == broker_seq && udt_hash_peerid(data->_remote_peerid) == peerid_hashcode)
			return data;
	}
	return NULL;
}
*/
void ptl_delete_passive_udp_broker_data(_u32 peerid_hashcode, _u32 broker_seq)
{
	SET_ITERATOR iter, next_iter; 
	PASSIVE_UDP_BROKER_DATA* data = NULL;
	_u32 cur_time = 0;
	LOG_DEBUG("ptl_delete_passive_udp_broker_data, peerid_hashcode = %u, broker_seq = %u.", peerid_hashcode, broker_seq);

	sd_time(&cur_time);
	iter = SET_BEGIN(g_passive_udp_broker_data_set);
	while(iter != SET_END(g_passive_udp_broker_data_set))
	{
		data = (PASSIVE_UDP_BROKER_DATA*)SET_DATA(iter);
		if( (data->_seq == broker_seq && udt_hash_peerid(data->_remote_peerid) == peerid_hashcode)
		     || data->_born_time + BORKER_DATA_MAX_EXIST_TIME < cur_time )
		{
			next_iter = SET_NEXT(g_passive_udp_broker_data_set, iter);
			set_erase_iterator( &g_passive_udp_broker_data_set, iter );
			sd_free(data);
			data = NULL;
			iter = next_iter;
			continue;
		}
		iter = SET_NEXT(g_passive_udp_broker_data_set, iter);
	}
}

_int32 ptl_erase_passive_udp_broker_data(PASSIVE_UDP_BROKER_DATA* data)
{
	_int32 ret = SUCCESS;
	LOG_DEBUG("ptl_erase_passive_udp_broker_data, seq = %u, remote_peerid = %s.", data->_seq, data->_remote_peerid);
	ret = set_erase_node(&g_passive_udp_broker_data_set, data);
	CHECK_VALUE(ret);
	return sd_free(data);
}


