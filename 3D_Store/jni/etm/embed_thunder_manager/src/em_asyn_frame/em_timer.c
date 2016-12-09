#include "em_asyn_frame/em_timer.h"
#include "em_common/em_define.h"
#include "em_common/em_time.h"
#include "em_common/em_mempool.h"
#include "em_common/em_string.h"
#include "em_common/em_list.h"

#define NODE_ADD(node1, node2)	(((node1) + (node2)) % CYCLE_NODE)
#define NODE_SUB(node1, node2)	(((node1) + CYCLE_NODE - (node2)) % CYCLE_NODE)

#ifdef _DEBUG
#define CHECK_LOOP(node) {if(node){TIME_NODE *tmp = node->_next_node; for(;tmp;tmp = tmp->_next_node) sd_assert(tmp != node);} }
#else
#define CHECK_LOOP(node)
#endif

#if defined(AMLOS)
#include <sys/types.h>
#include <os/OS_API.h>
#endif
#define EM_LOGID EM_LOGID_TIMER
#include "em_common/em_logger.h"


static TIME_NODE *g_em_timer[CYCLE_NODE];
static LIST g_em_infinite_list;

static _int32 g_em_last_node = 0;
/* distance from last_node to current */
static _u32 g_em_distance = 0; 

static _u32 g_em_cur_timespot = 0;

static SLAB *gp_em_timer_node_slab = NULL;

_int32 em_init_timer(void)
{
	_int32 ret_val = SUCCESS;

	ret_val = em_mpool_create_slab(sizeof(TIME_NODE), EM_MIN_TIMER_COUNT, 0, &gp_em_timer_node_slab);
	CHECK_VALUE(ret_val);

	/* init timer */
	em_memset(g_em_timer, 0, sizeof(TIME_NODE*) * CYCLE_NODE);

	g_em_last_node = 0; g_em_distance = 0;

	ret_val = em_time_ms(&g_em_cur_timespot);
	CHECK_VALUE(ret_val);

	em_list_init(&g_em_infinite_list);

	return ret_val;
}

_int32 em_uninit_timer(void)
{
	_int32 ret_val = SUCCESS;

	ret_val = em_mpool_destory_slab(gp_em_timer_node_slab);
	CHECK_VALUE(ret_val);
	gp_em_timer_node_slab = NULL;

	return ret_val;
}

_int32 em_refresh_timer(void)
{
	_int32 ret_val = SUCCESS;
	_u32 last_spot = g_em_cur_timespot, node_inv = 0;

	ret_val = em_time_ms(&g_em_cur_timespot);
	CHECK_VALUE(ret_val);

	if(g_em_cur_timespot < last_spot)
		last_spot = g_em_cur_timespot ;

	node_inv = (g_em_cur_timespot - last_spot + last_spot % GRANULARITY) / GRANULARITY;

	g_em_distance += node_inv;

	EM_LOG_DEBUG("refresh timer:  last_spot: %u, cur_spot: %u, node_inv: %u, distance: %u", 
		last_spot, g_em_cur_timespot, node_inv, g_em_distance);

	return ret_val;
}

_int32 em_put_into_timer(_u32 timeout, void *data, _int32 *time_index)
{
	_int32 ret_val = SUCCESS;
	_u32 layer = 0, node_inv = 0, node_index;
	TIME_NODE *pnode = NULL, *plastnode = NULL, *pnewnode = NULL;

#ifdef _DEBUG
	_u32 dbg_timestamp = 0;
	em_time_ms(&dbg_timestamp);
	EM_LOG_DEBUG("cur_timespot(%u) while now(%u) when put a timer-item", g_em_cur_timespot, dbg_timestamp);
#endif

	if(timeout == WAIT_INFINITE)
	{
		ret_val = em_list_push(&g_em_infinite_list, data);
		CHECK_VALUE(ret_val);

		*time_index = TIMER_INDEX_INFINITE;

		EM_LOG_DEBUG("put a infinite node into timer");
		return ret_val;
	}

	node_inv = (timeout + g_em_cur_timespot % GRANULARITY) / GRANULARITY;

	/* calculate from last_node */
	layer = (g_em_distance + node_inv) / CYCLE_NODE;
	node_index = (g_em_last_node + g_em_distance + node_inv) % CYCLE_NODE;

	/* find its node */
	plastnode = pnode = g_em_timer[node_index];

	CHECK_LOOP(g_em_timer[node_index]);
	EM_LOG_DEBUG("timer-item(timeout: %u, node_inv: %u), ready to find a fit position(last_node: %u, distance: %u, layer: %u, node_index: %u",
		timeout, node_inv, g_em_last_node, g_em_distance, layer, node_index);

	while(pnode && layer > pnode->_interval)
	{
		plastnode = pnode;
		layer -= pnode->_interval;
		pnode = pnode->_next_node;
	}

	if(pnode && layer == pnode->_interval)
	{
		pnewnode = pnode;
#ifdef _DEBUG
		EM_LOG_DEBUG("join a timer-node(timestamp: %u) while (now: %u, timeout: %u)",
			pnewnode->_data_timestamp, dbg_timestamp, timeout);
#endif
	}
	else
	{/* !pnode || layer < pnode->_interval */
		ret_val = em_mpool_get_slip(gp_em_timer_node_slab, (void **)&pnewnode);
		CHECK_VALUE(ret_val);

#ifdef _DEBUG
		pnewnode->_data_timestamp = g_em_cur_timespot + timeout;
		em_time_ms(&dbg_timestamp);

		EM_LOG_DEBUG("put a new timer-node(timestamp: %u) into timer while now stamp(%u)", g_em_cur_timespot, dbg_timestamp);
#endif

		em_list_init(&pnewnode->_data);
		pnewnode->_interval = layer;
		if(pnode)
			pnode->_interval -= layer;

		if(plastnode != pnode)
			plastnode->_next_node = pnewnode;
		else /* parent of new-node is g_em_timer[node_index] */
			g_em_timer[node_index] = pnewnode;

		pnewnode->_next_node = pnode;
	}

#ifdef _DEBUG
	pnode = g_em_timer[node_index];
	while(pnode != pnewnode)
	{
		EM_LOG_DEBUG("interval: %u", pnode->_interval);
		pnode = pnode->_next_node;
	}
	EM_LOG_DEBUG("------end of the put------");
#endif

	CHECK_LOOP(g_em_timer[node_index]);


	ret_val = em_list_push(&pnewnode->_data, data);
	CHECK_VALUE(ret_val);

	*time_index = node_index;

	return ret_val;
}

static _int32 em_pop_a_expire_timer(_int32 index, _int32 layer, LIST *data)
{
	_int32 ret_val = SUCCESS;
	TIME_NODE *pnode = g_em_timer[index], *last_node = NULL;

#ifdef _DEBUG
	_u32 dbg_timestamp = 0;
#endif

	CHECK_LOOP(g_em_timer[index]);
	while(pnode)
	{
		if(pnode->_interval > (_u32)layer || (layer > 0 && pnode->_interval == (_u32)layer))
		{
			pnode->_interval -= layer;
			break;
		}

		layer -= pnode->_interval;

		/* timeout */
		em_list_cat_and_clear(data, &pnode->_data);

#ifdef _DEBUG
		em_time_ms(&dbg_timestamp);
		EM_LOG_DEBUG("pop a expire timer-node(timestamp: %u, interval: %u, index: %u) and (now: %u, layer: %u)", 
			pnode->_data_timestamp, pnode->_interval, index, dbg_timestamp, layer);
#endif

		last_node = pnode;
		pnode = pnode->_next_node;

		ret_val = em_mpool_free_slip(gp_em_timer_node_slab, last_node);
		CHECK_VALUE(ret_val);
	}

	g_em_timer[index] = pnode;
	CHECK_LOOP(g_em_timer[index]);

	return ret_val;
}

_int32 em_pop_all_expire_timer(LIST *data)
{
	_int32 ret_val = SUCCESS;
	_int32 layer = 0, cur_idx = 0, count = 0, i = 0, index = 0;

	/* caculate layer */
	layer = g_em_distance / CYCLE_NODE;
	cur_idx = (g_em_last_node + g_em_distance) % CYCLE_NODE;
	count = NODE_SUB(cur_idx, g_em_last_node);

	EM_LOG_DEBUG("try to pop expire timer:  last_node: %u, distance: %u, layer: %u", g_em_last_node, g_em_distance, layer);

	for(i = 0; i < count; i++)
	{
		index = (g_em_last_node + i) % CYCLE_NODE;
		ret_val = em_pop_a_expire_timer(index, layer + 1, data);
		CHECK_VALUE(ret_val);
	}

	count = CYCLE_NODE - count;
	if(layer > 0)
	{
		for(i = 0; i < count; i++)
		{
			index = (cur_idx + i) % CYCLE_NODE;
			ret_val = em_pop_a_expire_timer(index, layer, data);
			CHECK_VALUE(ret_val);
		}
	}
	else
	{/* layer == 0, only pop current node, need not to travel all left node */
		ret_val = em_pop_a_expire_timer(cur_idx, 0, data);
		CHECK_VALUE(ret_val);
	}

	/* skip g_cur_node becase it has been popped */
	g_em_last_node = cur_idx;
	g_em_distance = 0;

	return ret_val;
}

_int32 em_erase_from_timer(void *comparator_data, data_comparator comp_fun, _int32 timer_index, void **data)
{
	_int32 ret_val = SUCCESS;
	_int32 idx = 0;
	TIME_NODE *pnode = NULL, *plastnode = NULL;
	LIST_ITERATOR list_it;

	if(data)
		*data = NULL;

	if(timer_index >= 0 && timer_index < CYCLE_NODE)
	{
		idx = timer_index;

		CHECK_LOOP(g_em_timer[idx]);
		for(plastnode = pnode = g_em_timer[idx]; pnode; pnode = pnode->_next_node)
		{
			for(list_it = LIST_BEGIN(pnode->_data); list_it != LIST_END(pnode->_data); list_it = LIST_NEXT(list_it))
			{
				if(comp_fun(comparator_data, LIST_VALUE(list_it)) == 0)
				{ /* find it */

					if(data)
						*data = LIST_VALUE(list_it);

					em_list_erase(&pnode->_data, list_it);

					CHECK_LOOP(g_em_timer[idx]);

					if(em_list_size(&pnode->_data) == 0)
					{ /* erase this time_node */
						if(pnode == g_em_timer[idx])
						{
							g_em_timer[idx] = pnode->_next_node;
							if(g_em_timer[idx])
							{
								g_em_timer[idx]->_interval += pnode->_interval;
							}
						}
						else
						{
							plastnode->_next_node = pnode->_next_node;
							if(plastnode->_next_node)
							{
								plastnode->_next_node->_interval += pnode->_interval;
							}
						}

						ret_val = em_mpool_free_slip(gp_em_timer_node_slab, pnode);
						CHECK_VALUE(ret_val);
					}

					CHECK_LOOP(g_em_timer[idx]);
					return ret_val;
				}
			}

			plastnode = pnode;
		}
	}
	else if(timer_index == TIMER_INDEX_NONE)
	{
		for(idx = 0; idx < CYCLE_NODE; idx++)
		{
			ret_val = em_erase_from_timer(comparator_data, comp_fun, idx, data);
			CHECK_VALUE(ret_val);

			if(data && *data)
			{/* erase successly */
				break;
			}
		}

		if(idx == CYCLE_NODE)
		{/* find in infinite list */
			ret_val = em_erase_from_timer(comparator_data, comp_fun, TIMER_INDEX_INFINITE, data);
			CHECK_VALUE(ret_val);
		}
	}
	else if(timer_index == TIMER_INDEX_INFINITE)
	{
		/* find in infinite-list */
		for(list_it = LIST_BEGIN(g_em_infinite_list); list_it != LIST_END(g_em_infinite_list); list_it = LIST_NEXT(list_it))
		{
			if(comp_fun(comparator_data, LIST_VALUE(list_it)) == 0)
			{ /* find it */
				if(data)
					*data = LIST_VALUE(list_it);

				em_list_erase(&g_em_infinite_list, list_it);

				return ret_val;
			}
		}
	}
	else
	{
		/* error time_index */
		ret_val = INVALID_TIMER_INDEX;
	}

	return ret_val;
}

_u32 em_get_current_timestamp(void)
{
	return g_em_cur_timespot;
}
