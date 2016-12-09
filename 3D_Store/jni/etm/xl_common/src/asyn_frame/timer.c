#include "asyn_frame/timer.h"
#include "utility/define.h"
#include "utility/time.h"
#include "utility/mempool.h"
#include "utility/string.h"
#define LOGID LOGID_TIMER
#include "utility/logger.h"

#define NODE_ADD(node1, node2)	(((node1) + (node2)) % CYCLE_NODE)
#define NODE_SUB(node1, node2)	(((node1) + CYCLE_NODE - (node2)) % CYCLE_NODE)

#ifdef _DEBUG
#define CHECK_LOOP(node) {if(node){TIME_NODE *tmp = node->_next_node; for(;tmp;tmp = tmp->_next_node) sd_assert(tmp != node);} }
#else
#define CHECK_LOOP(node)
#endif

static TIME_NODE *g_timer[CYCLE_NODE];
static LIST g_infinite_list;

static _int32 g_last_node = 0;
/* distance from last_node to current */
static _u32 g_distance = 0; 

static _u32 g_cur_timespot = 0;

static SLAB *gp_timer_node_slab = NULL;

_int32 init_timer(void)
{
	_int32 ret_val = mpool_create_slab(sizeof(TIME_NODE), MIN_TIMER_COUNT, 0, &gp_timer_node_slab);
	CHECK_VALUE(ret_val);

	sd_memset(g_timer, 0, sizeof(TIME_NODE*) * CYCLE_NODE);

	g_last_node = 0;
	g_distance = 0;

	ret_val = sd_time_ms(&g_cur_timespot);
	CHECK_VALUE(ret_val);

	list_init(&g_infinite_list);

	return ret_val;
}

_int32 uninit_timer(void)
{
	_int32 ret_val = SUCCESS;

	ret_val = mpool_destory_slab(gp_timer_node_slab);
	CHECK_VALUE(ret_val);
	gp_timer_node_slab = NULL;

	return ret_val;
}

_int32 refresh_timer(void)
{
	_u32 last_spot = g_cur_timespot;

	_int32 ret_val = sd_time_ms(&g_cur_timespot);
	CHECK_VALUE(ret_val);

	if (g_cur_timespot < last_spot)
    {		
        LOG_ERROR("g_cur_timespot = %d, last_spot = %d, timer overflow", g_cur_timespot, last_spot);
        return ret_val;
    }
	_u32 node_inv = (g_cur_timespot - last_spot + last_spot % GRANULARITY) / GRANULARITY;
	g_distance += node_inv;

	LOG_DEBUG("refresh timer:  last_spot: %u, cur_spot: %u, node_inv: %u, distance: %u", 
		last_spot, g_cur_timespot, node_inv, g_distance);

	return ret_val;
}

_int32 put_into_timer(_u32 timeout, void *data, _int32 *time_index)
{
	_int32 ret_val = SUCCESS;
	if (timeout == WAIT_INFINITE)
	{
		ret_val = list_push(&g_infinite_list, data);
		CHECK_VALUE(ret_val);

		*time_index = TIMER_INDEX_INFINITE;
		LOG_DEBUG("put a infinite node into timer");
		return ret_val;
	}

	_u32 node_inv = (timeout + g_cur_timespot % GRANULARITY) / GRANULARITY;

	/* calculate from last_node */
	_u32 layer = (g_distance + node_inv) / CYCLE_NODE;
	_u32 node_index = (g_last_node + g_distance + node_inv) % CYCLE_NODE;

	/* find its node */
	TIME_NODE *pnode = g_timer[node_index];
	TIME_NODE *plastnode = g_timer[node_index];

	CHECK_LOOP(g_timer[node_index]);
	LOG_DEBUG("timer-item(timeout: %u, node_inv: %u), ready to find a fit position(last_node: %u, distance: %u, layer: %u, node_index: %u",
		timeout, node_inv, g_last_node, g_distance, layer, node_index);

	while (pnode && (layer > pnode->_interval))
	{
		plastnode = pnode;
		layer -= pnode->_interval;
		pnode = pnode->_next_node;
	}

    TIME_NODE *pnewnode = NULL;
	if (pnode && (layer == pnode->_interval))
	{
		pnewnode = pnode;
	}
	else/* !pnode || layer < pnode->_interval */
	{
		ret_val = mpool_get_slip(gp_timer_node_slab, (void **)&pnewnode);
		CHECK_VALUE(ret_val);

		list_init(&pnewnode->_data);
		pnewnode->_interval = layer;
		if (pnode)
		{
		    pnode->_interval -= layer;
		}

		if (plastnode != pnode)
		{
		    plastnode->_next_node = pnewnode;
		}
		else /* parent of new-node is g_timer[node_index] */
		{
		    g_timer[node_index] = pnewnode;
		}

		pnewnode->_next_node = pnode;
	}

	CHECK_LOOP(g_timer[node_index]);

	ret_val = list_push(&pnewnode->_data, data);
	CHECK_VALUE(ret_val);

	*time_index = node_index;

	return ret_val;
}

static _int32 pop_a_expire_timer(_int32 index, _int32 layer, LIST *data)
{
    _int32 ret_val = SUCCESS;
    TIME_NODE *pnode = g_timer[index];
    TIME_NODE *last_node = NULL;

	CHECK_LOOP(g_timer[index]);
	while (pnode)
	{
		if ((pnode->_interval > (_u32)layer) 
		    || ((layer > 0) && (pnode->_interval == (_u32)layer)))
		{
			pnode->_interval -= layer;
			break;
		}

		layer -= pnode->_interval;

		/* timeout */
		list_cat_and_clear(data, &pnode->_data);

		last_node = pnode;
		pnode = pnode->_next_node;

		ret_val = mpool_free_slip(gp_timer_node_slab, last_node);
		CHECK_VALUE(ret_val);
	}

	g_timer[index] = pnode;
	CHECK_LOOP(g_timer[index]);

	return ret_val;
}

_int32 pop_all_expire_timer(LIST *data)
{
	/* caculate layer */
	_int32 layer = g_distance / CYCLE_NODE;
	_int32 cur_idx = (g_last_node + g_distance) % CYCLE_NODE;
	_int32 count = NODE_SUB(cur_idx, g_last_node);

	LOG_DEBUG("try to pop expire timer:  last_node: %u, distance: %u, layer: %u", g_last_node, g_distance, layer);

	_int32 ret_val = SUCCESS;
    _int32 i = 0;
	_int32 index = 0;
	for (i = 0; i < count; i++)
	{
		index = (g_last_node + i) % CYCLE_NODE;
		ret_val = pop_a_expire_timer(index, layer + 1, data);
		CHECK_VALUE(ret_val);
	}

	count = CYCLE_NODE - count;
	if (layer > 0)
	{
		for (i = 0; i < count; i++)
		{
			index = (cur_idx + i) % CYCLE_NODE;
			ret_val = pop_a_expire_timer(index, layer, data);
			CHECK_VALUE(ret_val);
		}
	}
	else
	{/* layer == 0, only pop current node, need not to travel all left node */
		ret_val = pop_a_expire_timer(cur_idx, 0, data);
		CHECK_VALUE(ret_val);
	}

	/* skip g_cur_node becase it has been popped */
	g_last_node = cur_idx;
	g_distance = 0;

	return ret_val;
}

static _int32 erase_from_timer_with_valid_index(void *comparator_data, data_comparator comp_fun, _int32 timer_index, void **data)
{
    sd_assert((timer_index >= 0) && (timer_index < CYCLE_NODE));
    _int32 ret_val = SUCCESS;
    
    _int32 idx = timer_index;
    CHECK_LOOP(g_timer[idx]);
    TIME_NODE *pnode = g_timer[idx];
    TIME_NODE *plastnode = pnode;
    for (; pnode; pnode = pnode->_next_node)
    {
        LIST_ITERATOR list_it = LIST_BEGIN(pnode->_data);
        for (; list_it != LIST_END(pnode->_data); list_it = LIST_NEXT(list_it))
        {
            if (comp_fun(comparator_data, LIST_VALUE(list_it)) == 0)
            {
                if (data)
                {
                    *data = LIST_VALUE(list_it);
                }
                list_erase(&pnode->_data, list_it);
                CHECK_LOOP(g_timer[idx]);
                if (list_size(&pnode->_data) == 0)
                {
                    if (pnode == g_timer[idx])
                    {
                        g_timer[idx] = pnode->_next_node;
                        if (g_timer[idx])
                        {
                            g_timer[idx]->_interval += pnode->_interval;
                        }
                    }
                    else
                    {
                        plastnode->_next_node = pnode->_next_node;
                        if (plastnode->_next_node)
                        {
                            plastnode->_next_node->_interval += pnode->_interval;
                        }
                    }

                    ret_val = mpool_free_slip(gp_timer_node_slab, pnode);
                    CHECK_VALUE(ret_val);
                }

                CHECK_LOOP(g_timer[idx]);
                return ret_val;
            }
        }
        plastnode = pnode;
    }
    
    return ret_val;
}

static _int32 erase_from_timer_with_timeout(void *comparator_data, data_comparator comp_fun, void **data)
{
    LIST_ITERATOR list_it = LIST_BEGIN(g_infinite_list);
    for (; list_it != LIST_END(g_infinite_list); list_it = LIST_NEXT(list_it))
    {
        if (comp_fun(comparator_data, LIST_VALUE(list_it)) == 0)
        {
            if (data)
            {
                *data = LIST_VALUE(list_it);
            }
            list_erase(&g_infinite_list, list_it);
            return SUCCESS;
        }
    }
    return INVALID_TIMER_INDEX;
}

static _int32 erase_from_timer_with_all_index(void *comparator_data, data_comparator comp_fun, void **data)
{
    _int32 ret_val = INVALID_TIMER_INDEX;
    _int32 idx = 0;
    for (idx = 0; idx < CYCLE_NODE; idx++)
    {
        ret_val = erase_from_timer_with_valid_index(comparator_data, comp_fun, idx, data);
        if (SUCCESS == ret_val)
        {
            if (data && (*data))
            {
                break;
            }
        }
    }

    if (CYCLE_NODE == idx)
    {
        ret_val = erase_from_timer_with_timeout(comparator_data, comp_fun, data);
    }

    return ret_val;
}

_int32 erase_from_timer(void *comparator_data, data_comparator comp_fun, _int32 timer_index, void **data)
{
	_int32 ret_val = INVALID_TIMER_INDEX;

	if (data)
	{
	    *data = NULL;
	}

	if ((timer_index >= 0) && (timer_index < CYCLE_NODE))
	{
	    ret_val = erase_from_timer_with_valid_index(comparator_data, comp_fun, timer_index, data);
	}
	else if (timer_index == TIMER_INDEX_NONE)
	{
	    ret_val = erase_from_timer_with_all_index(comparator_data, comp_fun, data);
	}
	else if (timer_index == TIMER_INDEX_INFINITE)
	{
	    ret_val = erase_from_timer_with_timeout(comparator_data, comp_fun, data);
	}

	return ret_val;
}

_u32 get_current_timestamp(void)
{
	return g_cur_timespot;
}
