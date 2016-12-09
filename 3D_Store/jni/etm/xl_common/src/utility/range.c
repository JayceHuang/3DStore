
#include "utility/errcode.h"
#include "utility/mempool.h"
#include "utility/utility.h"
#include "utility/range.h"
#include "utility/settings.h"

#include "utility/logid.h"
#define LOGID LOGID_RANGE_LIST
#include "utility/logger.h"


static _u32 DATA_UNIT_SIZE  =  RANGE_DATA_UNIT_SIZE;
static _u32 MIN_RANGE_LIST_NODE =  RANGE_MIN_RANGE_LIST_NODE;
static _u32 MIN_RANGE_LIST  = RANGE_MIN_RANGE_LIST;

static SLAB* g_range_list_node_slab = NULL;

static SLAB* g_range_list_slab = NULL;

_u32 init_range_module(void)
{
    _int32 ret_val = SUCCESS;

    ret_val = get_range_cfg_parameter();
    CHECK_VALUE(ret_val);

    ret_val = init_range_slabs();

    return ret_val;
}

_u32 uninit_range_module(void)
{
    _int32 ret_val = SUCCESS;

    ret_val = destroy_range_slabs();
    CHECK_VALUE(ret_val);
    return ret_val;
}


_int32 get_range_cfg_parameter(void)
{
    _int32 ret_val = SUCCESS;

    ///ret_val = settings_get_int_item("range.data_unit_size",(_int32*)&DATA_UNIT_SIZE);

    LOG_DEBUG("get_range_cfg_parameter,  range.data_unit_size : %u .", DATA_UNIT_SIZE);

    ret_val = settings_get_int_item("range.min_range_node_num",(_int32*)&MIN_RANGE_LIST_NODE);

    LOG_DEBUG("get_range_cfg_parameter,  range.min_range_node_num : %u .", MIN_RANGE_LIST_NODE);

    ret_val = settings_get_int_item("range.min_range_list_num",(_int32*)&MIN_RANGE_LIST);

    LOG_DEBUG("get_range_cfg_parameter,  range.min_range_list_num : %u .", MIN_RANGE_LIST);

    return ret_val;

}

_int32 init_range_slabs(void)
{
    _int32 ret_val = SUCCESS;

    if(g_range_list_node_slab == NULL)
    {
        ret_val = mpool_create_slab(sizeof(RANGE_LIST_NODE), MIN_RANGE_LIST_NODE, 0, &g_range_list_node_slab);
        CHECK_VALUE(ret_val);
    }

    if(g_range_list_slab == NULL)
    {
        ret_val = mpool_create_slab(sizeof(RANGE_LIST), MIN_RANGE_LIST, 0, &g_range_list_slab);
        CHECK_VALUE(ret_val);
    }

    return (ret_val);
}

_int32 destroy_range_slabs(void)
{
    _int32 ret_val = SUCCESS;
    if(g_range_list_node_slab !=  NULL)
    {
        mpool_destory_slab(g_range_list_node_slab);
        g_range_list_node_slab = NULL;
    }

    if(g_range_list_slab !=  NULL)
    {
        mpool_destory_slab(g_range_list_slab);
        g_range_list_slab = NULL;
    }

    return (ret_val);
}

_int32 range_list_alloc_node(RANGE_LIST_NODE** pp_node)
{
    return  mpool_get_slip(g_range_list_node_slab, (void**)pp_node);
}

_int32 range_list_free_node(RANGE_LIST_NODE* p_node)
{
    return  mpool_free_slip(g_range_list_node_slab, (void*)p_node);
}

_int32 alloc_range_list(RANGE_LIST** pp_node)
{
    return  mpool_get_slip(g_range_list_slab, (void**)pp_node);
}

_int32 free_range_list(RANGE_LIST* p_node)
{
    return  mpool_free_slip(g_range_list_slab, (void*)p_node);
}


_int32 range_list_init(RANGE_LIST* range_list)
{
    _int32 ret_val = SUCCESS;

    range_list->_head_node = NULL;
    range_list->_tail_node = NULL;
    range_list->_node_size = 0;

    return ret_val;

}

_int32 exact_range_list_init(EXACT_RANGE_LIST* range_list)
{
    _int32 ret_val = SUCCESS;
    range_list->_head_node = NULL;
    range_list->_tail_node = NULL;
    range_list->_node_size = 0;

    return ret_val;
}


_int32 range_list_clear(RANGE_LIST* range_list)
{
    _int32 ret_val = SUCCESS;

    RANGE_LIST_NODE*  tmp_node = range_list->_head_node;

    while (tmp_node != NULL)
    {
        range_list->_head_node = tmp_node->_next_node;
        /*sd_free(tmp_node);*/
        range_list_free_node(tmp_node);
        tmp_node = range_list->_head_node;
    }

    range_list->_head_node = NULL;
    range_list->_tail_node = NULL;
    range_list->_node_size = 0;

    return ret_val;

}

_int32 range_list_size(RANGE_LIST* range_list)
{
    return range_list->_node_size;
}

_int32  range_list_erase(RANGE_LIST* range_list, RANGE_LIST_NODE*  erase_node)
{
    if(erase_node->_prev_node != NULL)
    {
        erase_node->_prev_node->_next_node = erase_node->_next_node;
    }
    else
    {
        range_list->_head_node = erase_node->_next_node;
    }


    if(erase_node->_next_node != NULL)
    {
        erase_node->_next_node->_prev_node = erase_node->_prev_node;
    }
    else
    {
        range_list->_tail_node = erase_node->_prev_node;
    }

    if(range_list->_node_size > 1)
    {
        range_list->_node_size--;
    }
    else
    {
        range_list->_node_size = 0;

    }

    /*sd_free(erase_node);*/

    range_list_free_node(erase_node);

    return SUCCESS;
}

/* insert a node before the insert_node*/
_int32 add_range_to_list(RANGE_LIST* range_list, const RANGE*  new_range, RANGE_LIST_NODE* insert_node)
{
    RANGE_LIST_NODE* new_node  = NULL;

    /*sd_malloc(sizeof(RANGE_LIST_NODE) ,(void**)&new_node);*/

    range_list_alloc_node(&new_node);

    sd_assert(new_node != NULL);

    if(new_node == NULL)
        return OUT_OF_MEMORY;

    new_node->_range = *new_range;

    if(insert_node == NULL)
    {
        new_node->_next_node = NULL;
        new_node->_prev_node = range_list->_tail_node;

        if(range_list->_tail_node != NULL)
        {
            range_list->_tail_node->_next_node =  new_node;
            range_list->_tail_node = new_node;
            range_list->_node_size++;
        }
        else
        {
            range_list->_head_node = new_node;
            range_list->_tail_node = new_node;
            range_list->_node_size = 1;
        }

        return SUCCESS;
    }

    new_node->_prev_node = insert_node->_prev_node;
    new_node->_next_node = insert_node;

    if(insert_node->_prev_node != NULL)
    {
        insert_node->_prev_node->_next_node = new_node;
    }
    else
    {
        range_list->_head_node = new_node;
    }

    insert_node->_prev_node = new_node;

    range_list->_node_size++;


    return SUCCESS;
}

_int32 range_list_add_range(RANGE_LIST* range_list, const RANGE*  new_range, RANGE_LIST_NODE* begin_node,RANGE_LIST_NODE** _p_ret_node)
{
    RANGE_LIST_NODE*  cur_node = NULL;
    RANGE_LIST_NODE*  tail_node = NULL;

    if(_p_ret_node != NULL)
        *_p_ret_node = NULL;

    if(new_range == NULL || new_range->_num == 0)
        return  SUCCESS;

    cur_node = range_list->_head_node;
    tail_node = range_list->_tail_node;

    if(cur_node == NULL || tail_node == NULL)
    {
        /*  insert the head*/
        add_range_to_list(range_list, new_range, NULL);

        if(_p_ret_node != NULL)
            *_p_ret_node = range_list->_head_node;

        return  SUCCESS;
    }


    if(tail_node->_range._index + tail_node->_range._num < new_range->_index )
    {
        /*  insert to the tail of list*/
        add_range_to_list(range_list, new_range, NULL);
        if(_p_ret_node != NULL)
            *_p_ret_node = range_list->_tail_node;

        return SUCCESS;
    }

    if(begin_node != NULL)
    {
        cur_node = begin_node;
    }

    while(cur_node != NULL)
    {
        if(cur_node->_range._index > (new_range->_index + new_range->_num))
        {
            add_range_to_list(range_list, new_range, cur_node);

            cur_node = cur_node->_prev_node;

            break;
        }
        else if((cur_node->_range._index  + cur_node->_range._num) <new_range->_index)
        {
            cur_node = cur_node->_next_node;
            /*continue;   */
        }
        else if(cur_node->_range._index <= new_range->_index)
        {
            if((cur_node->_range._index + cur_node->_range._num) >= (new_range->_index + new_range->_num))
            {
                break;
            }
            else  if(NULL == cur_node->_next_node ||
                     (cur_node->_next_node != NULL && (cur_node->_next_node->_range._index > (new_range->_index + new_range->_num) )))
            {
                cur_node->_range._num = new_range->_index + new_range->_num - cur_node->_range._index;
                break;
            }
            else
            {
                /*erase next node */
                cur_node->_range._num = cur_node->_next_node->_range._index + cur_node->_next_node->_range._num - cur_node->_range._index;
                range_list_erase(range_list, cur_node->_next_node);
                /*continue;*/
            }
        }
        else
        {
            if((cur_node->_range._index + cur_node->_range._num) >= (new_range->_index + new_range->_num))
            {
                cur_node->_range._num = cur_node->_range._index + cur_node->_range._num - new_range->_index;
                cur_node->_range._index = new_range->_index;
                break;
            }
            else  if(NULL == cur_node->_next_node ||
                     (cur_node->_next_node != NULL && (cur_node->_next_node->_range._index > (new_range->_index + new_range->_num) )))
            {
                cur_node->_range._num = new_range->_num;
                cur_node->_range._index = new_range->_index;
                break;
            }
            else
            {
                /*erase next node */
                cur_node->_range._num = cur_node->_next_node->_range._index + cur_node->_next_node->_range._num -  new_range->_index;
                cur_node->_range._index = new_range->_index;

                range_list_erase(range_list, cur_node->_next_node);
                /*continue;*/
            }
        }
    }

    if(_p_ret_node != NULL)
        *_p_ret_node = cur_node;

    return SUCCESS;

}

_int32 range_list_add_range_list(RANGE_LIST* range_list, RANGE_LIST* new_range_list)
{
    RANGE_LIST_NODE*  cur_node = new_range_list->_head_node;
    RANGE_LIST_NODE*  begin_node = NULL;

    while(cur_node != NULL)
    {
        range_list_add_range(range_list, &(cur_node->_range), begin_node, &begin_node);

        cur_node = cur_node->_next_node;
    }

    return SUCCESS;

}

_int32 exact_range_list_erase (EXACT_RANGE_LIST * range_list, EXACT_RANGE_LIST_NODE * erase_node)
{
    if (erase_node->_prev_node != NULL)
    {
        erase_node->_prev_node->_next_node = erase_node->_next_node;
    }
    else
    {
        range_list->_head_node = erase_node->_next_node;
    }


    if (erase_node->_next_node != NULL)
    {
        erase_node->_next_node->_prev_node = erase_node->_prev_node;
    }
    else
    {
        range_list->_tail_node = erase_node->_prev_node;
    }

    if (range_list->_node_size > 1)
    {
        range_list->_node_size--;
    }
    else
    {
        range_list->_node_size = 0;
    }

    sd_free(erase_node);

    return SUCCESS;
}

_int32 add_exact_range_to_list (EXACT_RANGE_LIST * range_list, const EXACT_RANGE * new_range,
                                EXACT_RANGE_LIST_NODE * insert_node)
{
    EXACT_RANGE_LIST_NODE *new_node = NULL;

    if (SUCCESS != sd_malloc(sizeof(EXACT_RANGE_LIST_NODE), (void**)&new_node))
        return OUT_OF_MEMORY;

    sd_memcpy(&new_node->_exact_range, new_range, sizeof(EXACT_RANGE));

    if (insert_node == NULL)
    {
        new_node->_next_node = NULL;
        new_node->_prev_node = range_list->_tail_node;

        if (range_list->_tail_node != NULL)
        {
            range_list->_tail_node->_next_node = new_node;
            range_list->_tail_node = new_node;
            range_list->_node_size++;
        }
        else
        {
            range_list->_head_node = new_node;
            range_list->_tail_node = new_node;
            range_list->_node_size = 1;
        }

        return SUCCESS;
    }

    new_node->_prev_node = insert_node->_prev_node;
    new_node->_next_node = insert_node;

    if (insert_node->_prev_node != NULL)
    {
        insert_node->_prev_node->_next_node = new_node;
    }
    else
    {
        range_list->_head_node = new_node;
    }

    insert_node->_prev_node = new_node;

    range_list->_node_size++;


    return SUCCESS;
}

_int32 exact_range_list_delete_range (EXACT_RANGE_LIST * range_list,
                                      const EXACT_RANGE * del_range,
                                      EXACT_RANGE_LIST_NODE * begin_node,
                                      EXACT_RANGE_LIST_NODE ** _p_ret_node)
{
    EXACT_RANGE_LIST_NODE *cur_node = NULL;
    EXACT_RANGE_LIST_NODE *tail_node = NULL;
    EXACT_RANGE h_range;
    EXACT_RANGE t_range;
    EXACT_RANGE_LIST_NODE *erase_node = NULL;

    if (_p_ret_node != NULL)
        *_p_ret_node = NULL;

    if (del_range == NULL || del_range->_length == 0)
        return SUCCESS;

    cur_node = range_list->_head_node;
    tail_node = range_list->_tail_node;

    if (cur_node == NULL || tail_node == NULL)
    {
        if (_p_ret_node != NULL)
            *_p_ret_node = NULL;
        return SUCCESS;
    }


    if (tail_node->_exact_range._pos + tail_node->_exact_range._length <=
        del_range->_pos)
    {
        if (_p_ret_node != NULL)
            *_p_ret_node = range_list->_tail_node;
        return SUCCESS;
    }

    if (begin_node != NULL)
    {
        cur_node = begin_node;
    }

    while (cur_node != NULL)
    {
        if (cur_node->_exact_range._pos >= (del_range->_pos + del_range->_length))
        {
            cur_node = cur_node->_prev_node;
            break;
        }
        else if ((cur_node->_exact_range._pos + cur_node->_exact_range._length) <=
                 del_range->_pos)
        {
            cur_node = cur_node->_next_node;
            /*continue;   */
        }
        else if (cur_node->_exact_range._pos <= del_range->_pos)
        {
            if ((cur_node->_exact_range._pos + cur_node->_exact_range._length) >=
                (del_range->_pos + del_range->_length))
            {
                h_range._pos = cur_node->_exact_range._pos;
                h_range._length = del_range->_pos - cur_node->_exact_range._pos;
                t_range._pos = del_range->_pos + del_range->_length;
                t_range._length =
                    cur_node->_exact_range._pos + cur_node->_exact_range._length -
                    t_range._pos;

                if (h_range._length == 0 && t_range._length == 0)
                {
                    erase_node = cur_node;
                    cur_node = cur_node->_next_node;
                    exact_range_list_erase (range_list, erase_node);
                }
                else if (h_range._length == 0)
                {
                    cur_node->_exact_range._length = t_range._length;
                    cur_node->_exact_range._pos = t_range._pos;
                }
                else if (t_range._length == 0)
                {
                    cur_node->_exact_range._length = h_range._length;
                    cur_node->_exact_range._pos = h_range._pos;
                }
                else
                {
                    cur_node->_exact_range._length = t_range._length;
                    cur_node->_exact_range._pos = t_range._pos;

                    add_exact_range_to_list (range_list, &h_range, cur_node);
                }

                break;
            }
            else
            {
                /*erase next node */

                h_range._pos = cur_node->_exact_range._pos;
                h_range._length = del_range->_pos - cur_node->_exact_range._pos;

                if (h_range._length == 0)
                {
                    erase_node = cur_node;
                    cur_node = cur_node->_next_node;
                    exact_range_list_erase (range_list, erase_node);
                }
                else
                {
                    cur_node->_exact_range._length = h_range._length;
                }
                /*continue; */
            }
        }
        else
        {
            if ((cur_node->_exact_range._pos + cur_node->_exact_range._length) >=
                (del_range->_pos + del_range->_length))
            {
                t_range._pos = del_range->_pos + del_range->_length;
                t_range._length =
                    cur_node->_exact_range._pos + cur_node->_exact_range._length -
                    t_range._pos;

                if (t_range._length == 0)
                {
                    erase_node = cur_node;
                    cur_node = cur_node->_prev_node;
                    exact_range_list_erase (range_list, erase_node);
                }
                else
                {
                    cur_node->_exact_range._length = t_range._length;
                    cur_node->_exact_range._pos = t_range._pos;
                }

                break;
            }
            else
            {
                /*erase cur node */
                erase_node = cur_node;
                cur_node = cur_node->_next_node;
                exact_range_list_erase (range_list, erase_node);
                /*continue; */
            }
        }
    }

    if (_p_ret_node != NULL)
        *_p_ret_node = cur_node;

    return SUCCESS;

}

_int32 range_list_delete_range(RANGE_LIST* range_list, const RANGE*  del_range, RANGE_LIST_NODE* begin_node,RANGE_LIST_NODE** _p_ret_node)
{
    RANGE_LIST_NODE*  cur_node = NULL;
    RANGE_LIST_NODE*  tail_node = NULL;
    RANGE  h_range;
    RANGE  t_range;
    RANGE_LIST_NODE*  erase_node  = NULL;

    if(_p_ret_node != NULL)
        *_p_ret_node = NULL;

    if(del_range == NULL || del_range->_num == 0)
        return  SUCCESS;

    cur_node = range_list->_head_node;
    tail_node = range_list->_tail_node;

    if(cur_node == NULL || tail_node == NULL)
    {
        if(_p_ret_node != NULL)
            *_p_ret_node = NULL;
        return  SUCCESS;
    }


    if(tail_node->_range._index + tail_node->_range._num <= del_range->_index )
    {
        if(_p_ret_node != NULL)
            *_p_ret_node = range_list->_tail_node;
        return SUCCESS;
    }

    if(begin_node != NULL)
    {
        cur_node = begin_node;
    }

    while(cur_node != NULL)
    {
        if(cur_node->_range._index >= (del_range->_index + del_range->_num))
        {
            cur_node = cur_node->_prev_node;
            break;
        }
        else if((cur_node->_range._index  + cur_node->_range._num) <= del_range->_index)
        {
            cur_node = cur_node->_next_node;
            /*continue;   */
        }
        else if(cur_node->_range._index <= del_range->_index)
        {
            if((cur_node->_range._index + cur_node->_range._num) >= (del_range->_index + del_range->_num))
            {
                h_range._index = cur_node->_range._index;
                h_range._num = del_range->_index - cur_node->_range._index;
                t_range._index = del_range->_index + del_range->_num;
                t_range._num = cur_node->_range._index + cur_node->_range._num - t_range._index;

                if(h_range._num == 0 && t_range._num == 0)
                {
                    erase_node = cur_node;
                    cur_node = cur_node->_next_node;
                    range_list_erase(range_list, erase_node);
                }
                else if(h_range._num == 0)
                {
                    cur_node->_range._num =  t_range._num;
                    cur_node->_range._index =  t_range._index;
                }
                else if( t_range._num == 0)
                {
                    cur_node->_range._num =  h_range._num;
                    cur_node->_range._index = h_range._index;
                }
                else
                {
                    cur_node->_range._num =  t_range._num;
                    cur_node->_range._index =  t_range._index;

                    add_range_to_list(range_list, &h_range, cur_node);
                }

                break;
            }
            else
            {
                /*erase next node */

                h_range._index = cur_node->_range._index;
                h_range._num = del_range->_index - cur_node->_range._index;

                if(h_range._num == 0)
                {
                    erase_node = cur_node;
                    cur_node = cur_node->_next_node;
                    range_list_erase(range_list, erase_node);
                }
                else
                {
                    cur_node->_range._num = h_range._num ;
                }
                /*continue;*/
            }
        }
        else
        {
            if((cur_node->_range._index + cur_node->_range._num) >= (del_range->_index + del_range->_num))
            {
                t_range._index = del_range->_index + del_range->_num;
                t_range._num = cur_node->_range._index + cur_node->_range._num - t_range._index;

                if(t_range._num == 0)
                {
                    erase_node = cur_node;
                    cur_node = cur_node->_prev_node;
                    range_list_erase(range_list, erase_node);
                }
                else
                {
                    cur_node->_range._num =  t_range._num;
                    cur_node->_range._index = t_range._index;
                }

                break;
            }
            else
            {
                /*erase cur node */
                erase_node = cur_node;
                cur_node = cur_node->_next_node;
                range_list_erase(range_list, erase_node);
                /*continue;*/
            }
        }
    }

    if(_p_ret_node != NULL)
        *_p_ret_node = cur_node;

    return SUCCESS;

}

_int32 range_list_delete_range_list(RANGE_LIST* range_list, RANGE_LIST* del_range_list)
{
    RANGE_LIST_NODE*  cur_node = del_range_list->_head_node;
    RANGE_LIST_NODE*  begin_node = NULL;

    while(cur_node != NULL)
    {
        range_list_delete_range(range_list, &(cur_node->_range), begin_node, &begin_node);

        cur_node = cur_node->_next_node;
    }

    return SUCCESS;
}

_int32 out_put_range_list(RANGE_LIST* range_list)
{
#ifdef _LOGGER

	RANGE_LIST_NODE*  cur_node = range_list->_head_node;
	_u32 total_num = 0;
    if(range_list->_node_size == 0)
    {
        LOG_DEBUG("out_put_range_list range_list size is 0, so return.");
        return SUCCESS;
    }
    while(cur_node != NULL)
    {
        LOG_DEBUG("node(%u, %u).", cur_node->_range._index, cur_node->_range._num);
		total_num += cur_node->_range._num;
        cur_node = cur_node->_next_node;
    }
	LOG_DEBUG("out_put_range_list size=%u, total_num=%u.", range_list->_node_size, total_num);
#endif
    return SUCCESS;

}

_int32 force_out_put_range_list(RANGE_LIST* range_list)
{
    return out_put_range_list(range_list);
}

RANGE range_list_get_first_overlap_range(RANGE_LIST* range_list, const RANGE* r)
{
    RANGE_LIST_NODE*  cur_node = NULL;
    RANGE  ret_r ;

    ret_r._index = 0;
    ret_r._num = 0;

    if(r == NULL || r->_num == 0)
        return  ret_r;

    cur_node = range_list->_head_node;

    while(cur_node != NULL)
    {
        if(cur_node->_range._index >= (r->_index + r->_num))
        {
            break;
        }
        else if((cur_node->_range._index  + cur_node->_range._num) <=r->_index)
        {
            cur_node = cur_node->_next_node;
            /*continue;   */
        }
        else //if(cur_node->_range._index <= r->_index)
        {
            if(cur_node->_range._index <= r->_index)
            {
                ret_r._index = cur_node->_range._index;
            }
            else
            {
                ret_r._index = r->_index;
            }


            if(cur_node->_range._index + cur_node->_range._num <=  r->_index + r->_num)
            {
                ret_r._num = cur_node->_range._index + cur_node->_range._num - ret_r._index;
            }
            else
            {
                ret_r._num = r->_index + r->_num - ret_r._index;
            }

            return ret_r;
        }


        // {
        //       break;
        // }

    }

    return ret_r;

}
BOOL range_list_is_relevant(RANGE_LIST* range_list, const RANGE* r)
{
    RANGE_LIST_NODE*  cur_node = NULL;
    if(r == NULL || r->_num == 0)
        return  FALSE;

    cur_node = range_list->_head_node;

    while(cur_node != NULL)
    {
        if(cur_node->_range._index >= (r->_index + r->_num))
        {
            break;
        }
        else if((cur_node->_range._index  + cur_node->_range._num) <=r->_index)
        {
            cur_node = cur_node->_next_node;
            /*continue;   */
        }
        else //if(cur_node->_range._index <= r->_index)
        {
            return TRUE;
        }


        // {
        //       break;
        // }

    }

    return FALSE;

}


/*
     0  表示r 与range_list 中一项在头部重叠
     1  表示r 与range_list 中一项在尾部重叠
     2  表示r 与range_list 无重叠
*/
_u32 range_list_is_head_relevant(RANGE_LIST* range_list, const RANGE* r)
{
    RANGE_LIST_NODE*  cur_node = NULL;

    //_u32 first_index = 0;

    if(r == NULL || r->_num == 0)
        return  FALSE;

    cur_node = range_list->_head_node;

    while(cur_node != NULL)
    {
        if(cur_node->_range._index >= (r->_index + r->_num))
        {
            break;
        }
        else if((cur_node->_range._index  + cur_node->_range._num) <=r->_index)
        {
            cur_node = cur_node->_next_node;
            /*continue;   */
        }
        else if(r->_index  <cur_node->_range._index)
        {
			if (r->_index + r->_num <= cur_node->_range._index + cur_node->_range._num)
			{
				return 1;
			} 
			else
			{
				return 2;
			}
        }
        else if(cur_node->_range._index  + cur_node->_range._num > r->_index + r->_num)
        {
            return 0;
        }
        else
        {
            return 1;
        }
    }
    return 2;
}


/*
    lval与rval相等，返回0；
    两者交叉且lval在rval右边，返回1；
    两者交叉且lval在rval左边，返回-1；
    lval被rval包含，返回2；
    rval被lval包含，返回-2；
    两者无交集且lval在右边，返回3；
    两者无交集且rval在右边，返回-3；
    参数错误返回-4
***/
_int32 range_complete(const RANGE *lval, const RANGE *rval)
{
    _u32 lval_begin;
    _u32 rval_begin;
    _u32 lval_end;
    _u32 rval_end;

	//检验有效性
	if (lval == NULL || rval == NULL)
	{
		return -4;
	}
	lval_begin = lval->_index;
	rval_begin = rval->_index;
	lval_end = lval->_index + lval->_num;
	rval_end = rval->_index + rval->_num;

    if ((lval_begin == rval_begin) && (lval_end == rval_end))
    {
        return 0;
    }

    if (lval_end <= rval_begin)
    {
        return -3;
    }
    else if (lval_end <= rval_end)
    {
        if (lval_begin <= rval_begin)
        {
            return -1;
        }
        else
        {
            return 2;
        }
    }
    else
    {
        if (lval_begin <= rval_begin)
        {
            return -2;
        }
        else if (lval_begin >= rval_end)
        {
            return 3;
        }
        else
        {
            return 1;
        }
    }
}

/*
     -1 表示参数错误
     0  表示r 与range_list 中一项在头部重叠
     1  表示r 与range_list 中一项在尾部重叠
     2  表示r 与range_list 无重叠
*/
_int32 range_list_is_head_relevant2(RANGE_LIST* range_list, const RANGE* r)
{
    RANGE_LIST_NODE *cur_node = NULL;
    _int32 complete_res;
    // 检验有效性
    if(range_list == NULL || r == NULL || r->_num == 0)
        return  -1;

    cur_node = range_list->_head_node;
    while (cur_node != NULL)
    {
        complete_res = range_complete(&cur_node->_range, r);
        switch (complete_res)
        {
            case -3:
                cur_node = cur_node->_next_node;
                break;
            case -2:
            case -1:
            case 0:
                return 0;
            case 1:
            case 2:
                return 1;

                //case 3:
            default:
                return 2;
        }
    }
    return 2;
}

_int32 range_list_get_total_num(RANGE_LIST* range_list)
{
    _u32 total_num = 0;
    RANGE_LIST_NODE*  cur_node = range_list->_head_node;

    while(cur_node != NULL)
    {
        total_num += cur_node->_range._num;
        cur_node = cur_node->_next_node;
    }

    return total_num;
}

BOOL range_list_is_include(const RANGE_LIST* range_list, const  RANGE* r)
{
    RANGE_LIST_NODE*  cur_node = NULL;
    if(r == NULL || r->_num == 0)
        return  FALSE;

    cur_node = range_list->_head_node;

    while(cur_node != NULL)
    {
        if(cur_node->_range._index > (r->_index + r->_num))
        {
            break;
        }
        else if((cur_node->_range._index  + cur_node->_range._num) < r->_index)
        {
            cur_node = cur_node->_next_node;
            /*continue;   */
        }
        else if(cur_node->_range._index <= r->_index
                && (cur_node->_range._index  + cur_node->_range._num) >= (r->_index + r->_num))
        {
            return TRUE;
        }
        else
        {
            break;
        }


    }

    return FALSE;

}


BOOL range_is_overlap(RANGE* l, RANGE* r)
{
    if(l->_index < r->_index)
    {
        if(l->_index + l->_num > r->_index)
            return TRUE;
    }
    else if(l->_index  >= r->_index)
    {
        if(l->_index < r->_index + r->_num)
            return TRUE;
    }

    return FALSE;
}

_int32 range_list_get_head_node(RANGE_LIST *range_list, RANGE_LIST_ITEROATOR *head_node)
{
    RANGE_LIST_ITEROATOR  cur_node = range_list->_head_node;

    if(cur_node == NULL)
    {
        *head_node = NULL;
    }
    else
    {
        *head_node  =  cur_node;
    }

    return SUCCESS;

}

_int32 range_list_get_tail_node(RANGE_LIST *range_list, RANGE_LIST_ITEROATOR *tail_node)
{

    RANGE_LIST_ITEROATOR cur_node = range_list->_tail_node;

    /*  while(cur_node != NULL)
      {
           if(cur_node->_next_node == NULL)
            break;
           cur_node = cur_node->_next_node ;
      }  */

    if(cur_node == NULL)
    {
        *tail_node = NULL;
    }
    else
    {
        *tail_node  =  cur_node;
    }



    return SUCCESS;

}

_int32 range_list_get_next_node(RANGE_LIST *range_list, RANGE_LIST_ITEROATOR cur_node, RANGE_LIST_ITEROATOR *next_node)
{

    RANGE_LIST_ITEROATOR  tmp_next_node = NULL;
    if(cur_node == NULL)
        *next_node  =  NULL;

    tmp_next_node = cur_node->_next_node;

    *next_node = tmp_next_node;

    return SUCCESS;

}

/*调整 range_list 使得都在adjust_range_list 范围内部*/
_int32 range_list_adjust(RANGE_LIST* range_list, RANGE_LIST* adjust_range_list)
{
    RANGE_LIST_NODE*  cur_node = range_list->_head_node;
    //RANGE_LIST_NODE*  tail_node = range_list->_tail_node;

    RANGE_LIST_NODE*  ad_cur_node = adjust_range_list->_head_node;
    // RANGE_LIST_NODE*  ad_tail_node = adjust_range_list->_tail_node;
    RANGE_LIST_NODE*  erase_node = NULL;
    RANGE tmp_range;

    if(cur_node == NULL)
        return SUCCESS;

    if(ad_cur_node ==  NULL)
        return SUCCESS;

    while(cur_node != NULL || ad_cur_node != NULL)
    {
        if(cur_node == NULL)
            break;

        if(ad_cur_node ==  NULL)
        {
            erase_node = cur_node;
            cur_node = cur_node->_next_node;
            range_list_erase(range_list, erase_node);
            continue;
        }

        if(cur_node->_range._index >= (ad_cur_node->_range._index + ad_cur_node->_range._num))
        {
            ad_cur_node = ad_cur_node->_next_node;
        }
        else if((cur_node->_range._index  + cur_node->_range._num) <=ad_cur_node->_range._index)
        {
            erase_node = cur_node;
            cur_node = cur_node->_next_node;
            range_list_erase(range_list, erase_node);
            /*continue; */
        }
        else if(cur_node->_range._index < ad_cur_node->_range._index)
        {
            cur_node->_range._num = cur_node->_range._num - (ad_cur_node->_range._index-cur_node->_range._index);
            cur_node->_range._index = ad_cur_node->_range._index;

            if(cur_node->_range._index + cur_node->_range._num <= (ad_cur_node->_range._index + ad_cur_node->_range._num))
            {
                cur_node = cur_node->_next_node;
            }

            else
            {
                if(ad_cur_node->_next_node == NULL )
                {
                    cur_node->_range._num =  (ad_cur_node->_range._index + ad_cur_node->_range._num) -  cur_node->_range._index;
                    cur_node = cur_node->_next_node;
                    ad_cur_node = ad_cur_node->_next_node;
                }
                else if(ad_cur_node->_next_node->_range._index >=  cur_node->_range._index + cur_node->_range._num)
                {
                    cur_node->_range._num =  (ad_cur_node->_range._index + ad_cur_node->_range._num) -  cur_node->_range._index;
                    cur_node = cur_node->_next_node;
                    ad_cur_node = ad_cur_node->_next_node;
                }
                else
                {
                    tmp_range._index = ad_cur_node->_next_node->_range._index;
                    tmp_range._num = cur_node->_range._index + cur_node->_range._num - tmp_range._index;
                    cur_node->_range._num =  (ad_cur_node->_range._index + ad_cur_node->_range._num) -  cur_node->_range._index;
                    cur_node = cur_node->_next_node;
                    add_range_to_list( range_list, & tmp_range, cur_node);

                    if(cur_node != NULL)
                    {
                        cur_node = cur_node->_prev_node;
                    }
                    else
                    {
                        cur_node = range_list->_tail_node;
                    }

                    ad_cur_node = ad_cur_node->_next_node;
                }

            }
        }
        else if(cur_node->_range._index + cur_node->_range._num <= (ad_cur_node->_range._index + ad_cur_node->_range._num))
        {
            cur_node = cur_node->_next_node;
        }
        else
        {
            if(ad_cur_node->_next_node == NULL )
            {
                cur_node->_range._num =  (ad_cur_node->_range._index + ad_cur_node->_range._num) -  cur_node->_range._index;
                cur_node = cur_node->_next_node;
                ad_cur_node = ad_cur_node->_next_node;
            }
            else if(ad_cur_node->_next_node->_range._index >=  cur_node->_range._index + cur_node->_range._num)
            {
                cur_node->_range._num =  (ad_cur_node->_range._index + ad_cur_node->_range._num) -  cur_node->_range._index;
                cur_node = cur_node->_next_node;
                ad_cur_node = ad_cur_node->_next_node;
            }
            else
            {
                tmp_range._index = ad_cur_node->_next_node->_range._index;
                tmp_range._num = cur_node->_range._index + cur_node->_range._num - tmp_range._index;
                cur_node->_range._num =  (ad_cur_node->_range._index + ad_cur_node->_range._num) -  cur_node->_range._index;
                cur_node = cur_node->_next_node;
                add_range_to_list( range_list, & tmp_range, cur_node);

                if(cur_node != NULL)
                {
                    cur_node = cur_node->_prev_node;
                }
                else
                {
                    cur_node = range_list->_tail_node;
                }

                ad_cur_node = ad_cur_node->_next_node;
            }
        }

    }

    return SUCCESS;
}

BOOL range_list_is_contained(const RANGE_LIST* range_list1, RANGE_LIST* range_list2)
{
    RANGE_LIST_ITEROATOR cur_node = NULL;
    if(range_list2->_node_size == 0)
        return TRUE;

    if(range_list1->_node_size == 0)
        return FALSE;

    range_list_get_head_node(range_list2, &cur_node);

    while(cur_node!= NULL)
    {
        if(range_list_is_include(range_list1, &cur_node->_range) == FALSE)
        {
            return FALSE;
        }

        range_list_get_next_node(range_list2, cur_node, &cur_node);
    }

    return TRUE;
}


/*clear the r_range_list  cp l_range_list to r_range_list*/
void range_list_cp_range_list(RANGE_LIST* l_range_list, RANGE_LIST* r_range_list)
{
    range_list_clear(r_range_list);
    /* RANGE_LIST_ITEROATOR cur_node = NULL;
    range_list_get_head_node(l_range_list, cur_node);
    while(cur_node != NULL)
    {
                  RANGE tem_r  =  cur_node->_range;


       range_list_get_next_node(l_range_list, cur_node, &cur_node);
    }*/

    range_list_add_range_list(r_range_list, l_range_list);
}


_u32 get_data_unit_size(void)
{
    return DATA_UNIT_SIZE;
}

_u32 compute_unit_num(_u64 block_size)
{
    return (block_size + DATA_UNIT_SIZE - 1)/(DATA_UNIT_SIZE);
}

_u64 get_data_pos_from_index(_u32 index)
{
    _u64 pos = index;

    return (pos *DATA_UNIT_SIZE);
}


_u64  range_to_length(RANGE* r,  _u64 file_size)
{
    _u64 pos = get_data_unit_size();
    _u64 len = get_data_unit_size();

    pos *= r->_index;
    len *= r->_num;

    if( 0 != file_size && file_size < pos + len )
    {
        len = file_size - pos;
    }

    return len;
}

/*返回包含pos-length的 最小range，注意返回的range的边界不一定=pos ，可能比pos小*/
RANGE  pos_length_to_range(_u64 pos, _u64 length,  _u64 file_size)
{
    RANGE ret_r;
    _u32 start_index = 0;
    _u32 end_index =0;

    ret_r._index = 0;
    ret_r._num = 0;
    if(pos >= file_size)
    {
        return ret_r;
    }

    if(pos+length > file_size)
    {
        length = file_size - pos;
    }

    start_index = pos/get_data_unit_size();
    end_index =  (pos+length -1)/get_data_unit_size();

    ret_r._index = start_index;
    ret_r._num = end_index- start_index +1;

    return ret_r;
}

/*返回pos-length的 包含最小完整range，返回的range 一定是完整的包括最后一块 !*/
RANGE  pos_length_to_range2(_u64 pos, _u64 length,  _u64 file_size)
{
    RANGE ret_r;
    _u32 start_index = 0;
    _u32 end_index =0;

    ret_r._index = 0;
    ret_r._num = 0;
    if(pos >= file_size)
    {
        return ret_r;
    }

    if(pos+length > file_size)
    {
        length = file_size - pos;
    }

    start_index = (pos  + get_data_unit_size()-1)/get_data_unit_size();

    if(pos+length != file_size)
    {
        end_index =  (pos+length)/get_data_unit_size();
    }
    else
    {
        end_index =  (pos+length + get_data_unit_size()-1)/get_data_unit_size();
    }

    ret_r._index = start_index;

    if(end_index<start_index)
    {
        ret_r._num = 0;
    }
    else
    {
        ret_r._num = end_index- start_index;
    }

    return ret_r;
}


/*return the max range node of the range list*/
_int32 range_list_get_max_node(RANGE_LIST *range_list, RANGE_LIST_ITEROATOR *ret_node)
{
    _u32 max_num = 0;
    RANGE_LIST_ITEROATOR max_node = NULL;
    RANGE_LIST_ITEROATOR cur_node = range_list->_head_node;

    while(cur_node != NULL)
    {
        if(cur_node->_range._num > max_num)
        {
            max_num = cur_node->_range._num;
            max_node  = cur_node;
        }

        cur_node = cur_node->_next_node;
    }

    *ret_node = max_node;
    return max_num;
}

_int32 range_list_get_rand_node(RANGE_LIST *range_list, RANGE_LIST_ITEROATOR *ret_node)
{
    _u32 ret_num = 0;

    _u32 rand = sd_rand();

    RANGE_LIST_ITEROATOR cur_node = range_list->_head_node;

    if(range_list->_node_size != 0)
    {
        rand = rand%range_list->_node_size;
    }
    else
    {
        *ret_node = NULL;
        return    ret_num;
    }

    while(cur_node != NULL)
    {
        if(rand == 0)
        {
            break;
        }
        else
        {
            rand--;
        }
        cur_node = cur_node->_next_node;
    }

    *ret_node = cur_node;
    if(cur_node != 0)
        ret_num = cur_node->_range._num;
    return ret_num;

}

_int32 range_list_get_lasted_node(RANGE_LIST *range_list, _u32 _index, RANGE_LIST_ITEROATOR *ret_node)
{
    _u32 ret_num = 0;

    RANGE_LIST_ITEROATOR cur_node = range_list->_head_node;
    while(cur_node != NULL)
    {
        if(cur_node->_range._index >= _index)
            break;
        cur_node = cur_node->_next_node;
    }

    *ret_node = cur_node;
    return ret_num;
}

RANGE relevant_range(RANGE* l, RANGE *r)
{
    RANGE ret_r ;
    ret_r._index = 0;
    ret_r._num = 0;

    if(l->_index + l->_num <= r->_index)
    {
        return ret_r;
    }
    else if(r->_index + r->_num <= l->_index)
    {
        return ret_r;
    }
    else
    {
        if(l->_index < r->_index)
        {
            ret_r._index = r->_index;
        }
        else
        {
            ret_r._index = l->_index;
        }

        if((l->_index + l->_num)<(r->_index + r->_num))
        {
            ret_r._num = (l->_index + l->_num) - ret_r._index;
        }
        else
        {
            ret_r._num = (r->_index + r->_num) - ret_r._index;
        }
    }

    return ret_r;

}

/*返回包含pos-length 中包含的完整block对应的range 范围，不足一个block的返回range 中_num=0*/
RANGE  pos_length_to_valid_range(_u64 pos, _u64 length,  _u64 file_size, _u32 block_size)
{
    RANGE ret_range;
    _u64 valid_len =0;
    _u64 valid_offset =0;

    _u32 unit_num =0 ;

    ret_range._index = 0;
    ret_range._num = 0;

    if(file_size == 0 || block_size == 0 || pos >= file_size || block_size%DATA_UNIT_SIZE != 0)
    {
        return ret_range;
    }

    ret_range._index = (pos + block_size-1)/block_size ;

    valid_offset = (_u64)ret_range._index*(_u64)block_size;

    valid_len = valid_offset - pos;

    if(length <= valid_len)
    {
        ret_range._index = 0;
        ret_range._num = 0;
        return ret_range;
    }

    length -= valid_len;

    if(length + valid_offset < file_size)
    {
        ret_range._num = length/block_size;
        valid_len = (_u64)ret_range._num*(_u64)block_size;
    }
    else
    {
        ret_range._num = (length+block_size-1)/block_size;
        valid_len = length;
    }

    unit_num =  compute_unit_num(block_size);

    ret_range._index = ret_range._index*unit_num;
    ret_range._num = compute_unit_num(valid_len);

    return ret_range;

}


BOOL range_list_is_contained2(RANGE_LIST* range_list1, RANGE_LIST* range_list2)
{
    RANGE_LIST_ITEROATOR cur_node2 = NULL;
    RANGE_LIST_ITEROATOR cur_node1 = NULL;
    RANGE* r1 = NULL;
    RANGE* r2 = NULL;

    if(range_list2->_node_size == 0)
        return TRUE;

    if(range_list1->_node_size == 0)
        return FALSE;

    if(range_list1->_node_size == 1)
    {
        if(range_list2->_head_node->_range._index >= range_list1->_head_node->_range._index
           && (range_list2->_tail_node->_range._index + range_list2->_tail_node->_range._num)
           <= (range_list1->_head_node->_range._index +range_list1->_head_node->_range._num))
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }

    range_list_get_head_node(range_list2, &cur_node2);
    range_list_get_head_node(range_list1, &cur_node1);

    while(cur_node2 != NULL)
    {
        if(cur_node1 == NULL)
        {
            return FALSE;
        }

        r1 = &cur_node1->_range;
        r2 = &cur_node2->_range;

        if(r2->_index < r1->_index)
        {
            return FALSE;
        }
        else if(r2->_index + r2->_num <= r1->_index + r1->_num)
        {
            range_list_get_next_node(range_list2, cur_node2, &cur_node2);
        }
        else if(r2->_index  >=  r1->_index + r1->_num)
        {
            range_list_get_next_node(range_list1, cur_node1, &cur_node1);

        }
        else
        {
            return FALSE;
        }

    }

    return TRUE;
}


_int32 get_range_list_overlap_list(RANGE_LIST* range_list1, RANGE_LIST* range_list2, RANGE_LIST* ret_list)
{
    RANGE_LIST_ITEROATOR cur_node2 = NULL;
    RANGE_LIST_ITEROATOR cur_node1 = NULL;
    RANGE* r1 = NULL;
    RANGE* r2 = NULL;
    RANGE  overlap_r ;
    RANGE_LIST_NODE*  begin_node = NULL;

    if(range_list1 == NULL || range_list2 == NULL || ret_list == NULL)
    {
        return -1;
    }

    range_list_clear(ret_list);

    range_list_get_head_node(range_list2, &cur_node2);
    range_list_get_head_node(range_list1, &cur_node1);

    while(cur_node2 != NULL && cur_node1 != NULL)
    {

        r1 = &cur_node1->_range;
        r2 = &cur_node2->_range;

        if(r2->_index + r2->_num <= r1->_index)
        {
            range_list_get_next_node(range_list2, cur_node2, &cur_node2);
        }
        else if(r2->_index  >=  r1->_index + r1->_num)
        {
            range_list_get_next_node(range_list1, cur_node1, &cur_node1);

        }
        else
        {
            if(r1->_index <= r2->_index)
            {
                overlap_r._index = r2->_index;
            }
            else
            {
                overlap_r._index = r1->_index;
            }

            if(r1->_index + r1->_num <= r2->_index +r2->_num)
            {
                overlap_r._num = r1->_index + r1->_num - overlap_r._index;
                range_list_get_next_node(range_list1, cur_node1, &cur_node1);

            }
            else
            {
                overlap_r._num = r2->_index + r2->_num - overlap_r._index;
                range_list_get_next_node(range_list2, cur_node2, &cur_node2);
            }

            if(overlap_r._num != 0)
            {
                range_list_add_range(ret_list, &overlap_r, begin_node, &begin_node);
            }

        }

    }

    return SUCCESS;

}


_int32 range_list_pop_first_range(RANGE_LIST* range_list)
{
    //_int32 ret_val = SUCCESS;

    RANGE_LIST_NODE*  tmp_node = NULL;
    if(range_list == NULL)
    {
        return SUCCESS;
    }

    if(range_list->_node_size == 0)
    {
        return SUCCESS;

    }

    if(range_list->_node_size == 1)
    {
        tmp_node =  range_list->_head_node;
        range_list_free_node(tmp_node);
        range_list->_head_node = NULL;
        range_list->_tail_node = NULL;
        range_list->_node_size = 0;
    }
    else
    {
        tmp_node =  range_list->_head_node;
        range_list->_head_node = tmp_node->_next_node;
        range_list_free_node(tmp_node);
        range_list->_node_size-- ;
    }

    return SUCCESS;
}


