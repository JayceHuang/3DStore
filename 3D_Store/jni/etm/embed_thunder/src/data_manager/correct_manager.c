#include "utility/mempool.h"
#include "utility/settings.h"
#include "utility/logid.h"
#define LOGID LOGID_CORRECT_MANAGER
#include "utility/logger.h"

#include "correct_manager.h"
#include "data_manager_interface.h"

static _u32 ORIGIN_MAX_CORRECT_NUM = 5;
static _u32 SERVER_MAX_CORRECT_NUM = 5;
static _u32 PEER_MAX_CORRECT_NUM = 4;
static _u32 MAX_CORRECT_TIMES = 5;

static _u32 MIN_ERROR_BLOCK_NODE = CORRECT_MANAGER_MIN_ERROR_BLOCK_NODE;

static SLAB* g_error_block_node_slab = NULL;


_int32 get_correct_manager_cfg_parameter()
{
    _int32 ret_val = SUCCESS;

    ret_val = settings_get_int_item("correct_manager.origin_res_correct_num", (_int32*)&ORIGIN_MAX_CORRECT_NUM);
    LOG_DEBUG("get_correct_manager_cfg_parameter,  correct_manager.origin_res_correct_num: %u .", ORIGIN_MAX_CORRECT_NUM);

    ret_val = settings_get_int_item("correct_manager.server_max_correct_num", (_int32*)&SERVER_MAX_CORRECT_NUM);
    LOG_DEBUG("get_correct_manager_cfg_parameter,  correct_manager.server_max_correct_num : %u .", SERVER_MAX_CORRECT_NUM);

    ret_val = settings_get_int_item("correct_manager.peer_max_correct_num", (_int32*)&PEER_MAX_CORRECT_NUM);
    LOG_DEBUG("get_correct_manager_cfg_parameter,  correct_manager.peer_max_correct_num : %u .", PEER_MAX_CORRECT_NUM);

    ret_val = settings_get_int_item("correct_manager.max_correct_times", (_int32*)&MAX_CORRECT_TIMES);
    LOG_DEBUG("get_correct_manager_cfg_parameter,  correct_manager.max_correct_times: %u .", MAX_CORRECT_TIMES);

    ret_val = settings_get_int_item("correct_manager.min_alloc_error_block_num", (_int32*)&MIN_ERROR_BLOCK_NODE);
    LOG_DEBUG("get_correct_manager_cfg_parameter,  correct_manager.min_alloc_error_block_num: %u .", MIN_ERROR_BLOCK_NODE);

    return ret_val;

}

_int32 init_error_block_slab()
{
    _int32 ret_val = SUCCESS;

    if(g_error_block_node_slab != NULL)
    {
        return ret_val;
    }

    ret_val = mpool_create_slab(sizeof(ERROR_BLOCK), MIN_ERROR_BLOCK_NODE, 0, &g_error_block_node_slab);
    return (ret_val);
}

_int32 destroy_error_block_slab()
{
    _int32 ret_val = SUCCESS;
    if(g_error_block_node_slab !=  NULL)
    {
        mpool_destory_slab(g_error_block_node_slab);
        g_error_block_node_slab = NULL;
    }

    return (ret_val);
}

_int32 alloc_error_block_node(ERROR_BLOCK** pp_node)
{
    return  mpool_get_slip(g_error_block_node_slab, (void**)pp_node);
}

_int32 free_error_block_node(ERROR_BLOCK* p_node)
{
    return  mpool_free_slip(g_error_block_node_slab, (void*)p_node);
}


_int32 init_correct_manager(CORRECT_MANAGER*  correct_m)
{
    _int32 ret_val = SUCCESS;

    LOG_DEBUG("init_correct_manager");

    list_init(&(correct_m->_error_ranages._error_block_list));

    ret_val =  range_list_init(&(correct_m->_prority_ranages));
    CHECK_VALUE(ret_val);

    correct_m->_origin_res = NULL;
    return ret_val;
}

_int32 clear_error_block_list(ERROR_BLOCKLIST *error_block_list)
{
    ERROR_BLOCK* e_block = NULL;

    LOG_DEBUG("clear_error_block_list");

    do
    {
        list_pop(&error_block_list->_error_block_list, (void**)&e_block);
        if(e_block == NULL)
        {
            break;
        }
        else
        {
            correct_manager_clear_res_list(&e_block->_error_resources);
            /* sd_free(e_block);*/
            free_error_block_node(e_block);
            e_block = NULL;
        }


    }
    while(1);

    return SUCCESS;

}


_int32 unit_correct_manager(CORRECT_MANAGER*  correct_m)
{
    _int32 ret_val = SUCCESS;

    LOG_DEBUG("unit_correct_manager");

    ret_val =  clear_error_block_list(&(correct_m->_error_ranages));
    CHECK_VALUE(ret_val);

    ret_val =  range_list_clear(&(correct_m->_prority_ranages));
    CHECK_VALUE(ret_val);

    correct_m->_origin_res = NULL;

    return ret_val;
}

void correct_manager_set_origin_res(CORRECT_MANAGER*  correct_m,  RESOURCE*  origin_res)
{
    correct_m->_origin_res = origin_res;
}

_int32 correct_manager_add_prority_range(CORRECT_MANAGER*  correct_m,  RANGE* p_r)
{
    _int32 ret_val =  SUCCESS;

    LOG_DEBUG("correct_manager_add_prority_range");

    ret_val = range_list_add_range(&(correct_m->_prority_ranages),p_r,NULL, NULL);

    return  ret_val;
}

_int32 correct_manager_del_prority_range(CORRECT_MANAGER*  correct_m,  RANGE* p_r)
{
    _int32 ret_val =  SUCCESS;

    LOG_DEBUG("correct_manager_del_prority_range");

    if(correct_m->_prority_ranages._node_size == 0)
        return ret_val;

    ret_val = range_list_delete_range(&(correct_m->_prority_ranages),p_r,NULL, NULL);

    return  ret_val;


}

_int32 correct_manager_add_prority_range_list(CORRECT_MANAGER*  correct_m,  RANGE_LIST* p_list)
{
    _int32 ret_val =  SUCCESS;

    LOG_DEBUG("correct_manager_add_prority_range_list");

    ret_val = range_list_add_range_list(&(correct_m->_prority_ranages),p_list);

    return  ret_val;
}

_int32 correct_manager_del_prority_range_list(CORRECT_MANAGER*  correct_m,  RANGE_LIST* p_list)
{
    _int32 ret_val =  SUCCESS;

    LOG_DEBUG("correct_manager_del_prority_range_list");

    if(correct_m->_prority_ranages._node_size == 0)
        return ret_val;

    ret_val = range_list_delete_range_list(&(correct_m->_prority_ranages),p_list);

    return  ret_val;
}

_int32 correct_manager_clear_prority_range_list(CORRECT_MANAGER*  correct_m)
{
    range_list_clear(&(correct_m->_prority_ranages));
    return SUCCESS;
}

/*
       0   no error
     -1  peer/server  correct
     -2  one resource  correct
     -3  origin correct
     -4  can not correct
*/

_int32 correct_manager_add_error_block(CORRECT_MANAGER*  correct_m,  RANGE* p_r,LIST* res_list)
{
    _int32 ret_val =  0;
    ERROR_BLOCK* cur_err_block = NULL;
    _u32  res_size  = 0;
    LIST_ITERATOR  res_node = NULL;
    ERROR_BLOCK* new_err_block = NULL;
    RESOURCE* p_cur_res = NULL;

    //LOG_DEBUG("correct_manager_add_error_block");

    LIST_ITERATOR  cur_node = LIST_BEGIN(correct_m->_error_ranages._error_block_list);
    LOG_DEBUG("correct_manager_add_error_block");
    while(cur_node != LIST_END(correct_m->_error_ranages._error_block_list))
    {
        cur_err_block = (ERROR_BLOCK*)cur_node->_data;

        if(cur_err_block->_r._index < p_r->_index)
        {
            cur_node = LIST_NEXT(cur_node);
        }
        else if(cur_err_block->_r._index == p_r->_index && cur_err_block->_r._num == p_r->_num)
        {
            cur_err_block->_error_count++;

            if(cur_err_block->_error_count > MAX_CORRECT_TIMES)
            {
                LOG_ERROR("correct_manager_add_error_block, range(%u,%u) error times: %u is to much, so ret -4", p_r->_index,p_r->_num, cur_err_block->_error_count);
                ret_val = -4;

                list_erase(&correct_m->_error_ranages._error_block_list,cur_node);

                correct_manager_clear_res_list(&cur_err_block->_error_resources);

                /* sd_free(cur_err_block);     */
                free_error_block_node(cur_err_block);


                return ret_val;
            }

            res_size  = list_size(res_list);
            if(res_size > 1)
            {
                cur_err_block->_valid_resources = 1;
                LOG_ERROR("correct_manager_add_error_block, range(%u,%u) , muti res,  ret -2", p_r->_index,p_r->_num);
                ret_val = -2;
                return ret_val;
            }

            res_node = LIST_BEGIN(*res_list);
            if(res_node != LIST_END(*res_list))
            {
                p_cur_res = (RESOURCE*) LIST_VALUE(res_node);
                //  if(dm_is_origin_resource(correct_m->_p_data_manager, p_cur_res) == FALSE)
                if(correct_manager_is_origin_resource(correct_m, p_cur_res) == FALSE)
                {
                    correct_manager_inc_res_error_times(p_cur_res);
                }

                if(cur_err_block->_valid_resources == 0)
                {
                    cur_err_block->_valid_resources = 1;
                    LOG_ERROR("correct_manager_add_error_block, range(%u,%u) , muti res corect error, ret -2", p_r->_index,p_r->_num);
                    ret_val = -2;
                }
                else if(cur_err_block->_valid_resources == 1)
                {
                    if((cur_err_block->_error_count >= MAX_CORRECT_TIMES/2) && (resource_is_useable(correct_m->_origin_res) == TRUE))
                    {
                        LOG_ERROR("correct_manager_add_error_block, range(%u,%u) , error times :%u ,  ret -2", p_r->_index,p_r->_num, cur_err_block->_error_count);

                        cur_err_block->_valid_resources = 2;
                        ret_val = -2;
                    }
                    else
                    {
                        LOG_ERROR("correct_manager_add_error_block, range(%u,%u) , error times :%u , ret -2", p_r->_index,p_r->_num, cur_err_block->_error_count);

                        cur_err_block->_valid_resources = 1;
                        ret_val = -2;
                    }
                }
                else if(cur_err_block->_valid_resources == 2)
                {
                    LOG_ERROR("correct_manager_add_error_block, range(%u,%u) , error times :%u ,origin res  ret -3", p_r->_index,p_r->_num, cur_err_block->_error_count);

                    cur_err_block->_valid_resources = 2;
                    ret_val = -3;

                }
            }
            else
            {
                LOG_ERROR("correct_manager_add_error_block, range(%u,%u) , but res_list size = 0", p_r->_index,p_r->_num);
                cur_err_block->_valid_resources = 1;
                ret_val = -2;
                // sd_assert(FALSE);
            }

            correct_manager_add_res_list(&cur_err_block->_error_resources, res_list);


            return ret_val;
        }
        else
        {
            break;
        }
    }


    /* ret_val = sd_malloc(sizeof(ERROR_BLOCK), (void**)&new_err_block);*/
    ret_val = alloc_error_block_node(&new_err_block);
    CHECK_VALUE(ret_val);

    new_err_block->_r = *p_r;
    new_err_block->_error_count = 1;

    list_init(&(new_err_block->_error_resources));

    res_size  = list_size(res_list);
    if(res_size > 1)
    {
        new_err_block->_valid_resources = 0;
        LOG_ERROR("correct_manager_add_error_block, range(%u,%u) , muti res  ret -1", p_r->_index,p_r->_num);

        ret_val = -1;
    }
    else if(res_size == 1 )
    {

        res_node = LIST_BEGIN(*res_list);
        if(res_node != LIST_END(*res_list))
        {
            p_cur_res = (RESOURCE*) LIST_VALUE(res_node);

            //if(dm_is_origin_resource(correct_m->_p_data_manager, p_cur_res) == TRUE)
            if(correct_manager_is_origin_resource(correct_m, p_cur_res) == TRUE)
            {
                LOG_ERROR("correct_manager_add_error_block, range(%u,%u) , origin res  ret -3", p_r->_index,p_r->_num);
                new_err_block->_valid_resources = 2;
                ret_val = -3;
            }
            else
            {
                LOG_ERROR("correct_manager_add_error_block, range(%u,%u) , not origin res  ret -2", p_r->_index,p_r->_num);
                correct_manager_inc_res_error_times(p_cur_res);
                new_err_block->_valid_resources = 1;
                ret_val = -2;
            }
        }

        correct_manager_add_res_list(&(new_err_block->_error_resources), res_list);

    }
    else
    {
        LOG_ERROR("correct_manager_add_error_block, range(%u,%u) , but res_list size = 0", p_r->_index,p_r->_num);
        new_err_block->_valid_resources = 1;
        ret_val = -2;
        //  sd_assert(FALSE);
    }


    if(cur_node !=  LIST_END(correct_m->_error_ranages._error_block_list))
    {
        list_insert(&correct_m->_error_ranages._error_block_list, new_err_block, cur_node);
    }
    else
    {
        list_push(&correct_m->_error_ranages._error_block_list, new_err_block);
    }

    return ret_val;;

}


_int32 set_cannot_correct_error_block(ERROR_BLOCKLIST* p_err_block_list,  RANGE* p_r)
{
    ERROR_BLOCK* cur_err_block = NULL;
    LIST_ITERATOR  cur_node = NULL;

    if(p_err_block_list == NULL || p_r == NULL)
    {
        return SUCCESS;
    }

    LOG_DEBUG("set_cannot_correct_error_block (%u,%u)", p_r->_index, p_r->_num);
    cur_node = LIST_BEGIN(p_err_block_list->_error_block_list);

    while(cur_node != LIST_END(p_err_block_list->_error_block_list))
    {
        cur_err_block = (ERROR_BLOCK*)cur_node->_data;

        if(cur_err_block->_r._index < p_r->_index)
        {
            cur_node = LIST_NEXT(cur_node);
        }
        else if(cur_err_block->_r._index >= p_r->_index
                && cur_err_block->_r._index  + cur_err_block->_r._num <= p_r->_index+ p_r->_num)
        {
            cur_err_block->_error_count = MAX_CORRECT_TIMES+1;
            LOG_DEBUG("set_cannot_correct_error_block (%u,%u)  of (%u,%u) success", cur_err_block->_r._index,cur_err_block->_r._num, p_r->_index, p_r->_num);
            cur_node = LIST_NEXT(cur_node);
        }
        else
        {
            break;
        }
    }

    LOG_DEBUG("set_cannot_correct_error_block (%u,%u)  failure", p_r->_index, p_r->_num);

    return SUCCESS;
}

_int32 set_cannot_correct_error_block_list(ERROR_BLOCKLIST* p_err_block_list,  RANGE_LIST* p_r_list)
{
    RANGE_LIST_ITEROATOR  r_it = NULL;

    if(p_r_list == NULL)
    {
        return SUCCESS;
    }

    range_list_get_head_node(p_r_list, &r_it);

    while(r_it != NULL)
    {
        set_cannot_correct_error_block(p_err_block_list, &(r_it->_range));
        range_list_get_next_node(p_r_list, r_it, &r_it);
    }

    return SUCCESS;
}


_int32 correct_manager_inc_res_error_times(RESOURCE* res)
{
    RESOURCE_TYPE res_type =  get_resource_type(res);

    LOG_DEBUG("correct_manager_inc_res_error_times");

    if(res_type  == PEER_RESOURCE)
    {
        LOG_DEBUG("correct_manager_inc_res_error_times, p2p res:0x%x .", res);
        if(inc_resource_error_times(res) >= PEER_MAX_CORRECT_NUM)
        {
            LOG_DEBUG("correct_manager_inc_res_error_times, abandon p2p res:0x%x .", res);
            set_resource_state(res, ABANDON);
        }

    }
    else
    {
        LOG_DEBUG("correct_manager_inc_res_error_times, server res:0x%x .", res);
        if(inc_resource_error_times(res) >= SERVER_MAX_CORRECT_NUM)
        {
            LOG_DEBUG("correct_manager_inc_res_error_times,abandon server res:0x%x .", res);
            set_resource_state(res, ABANDON);
        }
    }

    return SUCCESS;

}

_int32 correct_manager_erase_error_block(CORRECT_MANAGER*  correct_m,  RANGE* p_r)
{
    _int32 ret_val =  SUCCESS;
    ERROR_BLOCK* cur_err_block = NULL;

    LIST_ITERATOR cur_node = LIST_BEGIN(correct_m->_error_ranages._error_block_list);

    LOG_DEBUG("correct_manager_erase_error_block");

    while(cur_node !=  LIST_END(correct_m->_error_ranages._error_block_list))
    {
        cur_err_block = (ERROR_BLOCK*)LIST_VALUE(cur_node);

        if(cur_err_block->_r._index < p_r->_index)
        {
            cur_node = LIST_NEXT(cur_node);
        }
        else if(cur_err_block->_r._index == p_r->_index && cur_err_block->_r._num == p_r->_num)
        {
            ret_val =  list_erase(&correct_m->_error_ranages._error_block_list,cur_node);

            correct_manager_clear_res_list(&cur_err_block->_error_resources);

            /* sd_free(cur_err_block);  */
            free_error_block_node(cur_err_block);

            return ret_val;
        }
        else
        {
            break;
        }
    }

    return SUCCESS;

}


_int32 correct_manager_add_res_list( LIST* res_list ,LIST* new_res_list)
{
    RESOURCE* cur_res = NULL;
    LIST_ITERATOR tmp_node = NULL;
    RESOURCE* tmp_res = NULL;

    LIST_ITERATOR cur_node = LIST_BEGIN(*new_res_list);

    LOG_DEBUG("correct_manager_add_res_list");

    while(cur_node != LIST_END(*new_res_list))
    {
        cur_res = (RESOURCE*)LIST_VALUE(cur_node);

        tmp_node = LIST_BEGIN(*res_list);

        while( tmp_node !=  LIST_END(*res_list))
        {
            tmp_res = (RESOURCE*)tmp_node->_data;
            if(tmp_res == cur_res)
            {
                break;
            }
            else
            {
                tmp_node =LIST_NEXT(tmp_node);
            }
        }

        if(tmp_node == LIST_END(*res_list))
        {
            list_push(res_list, cur_res);
        }

        cur_node =LIST_NEXT(cur_node);
    }

    return SUCCESS;
}


_int32 correct_manager_clear_res_list( LIST* res_list )
{
    RESOURCE* cur_res = NULL;

    LOG_DEBUG("correct_manager_clear_res_list");

    list_pop(res_list, (void**)&cur_res);

    while(cur_res != NULL)
    {
        list_pop(res_list, (void**)&cur_res);
    }

    return SUCCESS;
}



BOOL range_is_relate_error_block(ERROR_BLOCKLIST*  error_block_list,  RANGE* p_r)
{

    ERROR_BLOCK* cur_err_block = NULL;

    LIST_ITERATOR cur_node = LIST_BEGIN(error_block_list->_error_block_list);

    LOG_DEBUG("range_is_relate_error_block");

    while(cur_node !=  LIST_END(error_block_list->_error_block_list))
    {
        cur_err_block = (ERROR_BLOCK*)LIST_VALUE(cur_node);

        if(range_is_overlap(p_r, &cur_err_block->_r) == TRUE)
        {
            return TRUE;
        }

        cur_node = LIST_NEXT(cur_node);

    }

    return FALSE;

}

void get_error_block_range_list(ERROR_BLOCKLIST*  error_block_list,  RANGE_LIST* p_range_list)
{
    ERROR_BLOCK* cur_err_block = NULL;
    RANGE_LIST_ITEROATOR begin_node = NULL;

    LIST_ITERATOR cur_node = LIST_BEGIN(error_block_list->_error_block_list);

    LOG_DEBUG("get_error_block_range_list");

    while(cur_node !=  LIST_END(error_block_list->_error_block_list))
    {
        cur_err_block = (ERROR_BLOCK*)LIST_VALUE(cur_node);

        range_list_add_range(p_range_list, &cur_err_block->_r, begin_node, &begin_node);

        cur_node = LIST_NEXT(cur_node);
    }

}


_int32 correct_manager_erase_abandon_res(CORRECT_MANAGER*  correct_m,  RESOURCE* abandon_res)
{


    ERROR_BLOCK* cur_err_block = NULL;

    LIST_ITERATOR cur_node = LIST_BEGIN(correct_m->_error_ranages._error_block_list);

    LIST_ITERATOR cur_res_node = NULL;
    LIST_ITERATOR erase_res_node = NULL;
    RESOURCE* cur_res = NULL;

    LOG_DEBUG("correct_manager_erase_abandon_res connect_manager:0x%x, abandon_res:0x%x", correct_m, abandon_res);

    while(cur_node !=  LIST_END(correct_m->_error_ranages._error_block_list))
    {
        cur_err_block = (ERROR_BLOCK*)LIST_VALUE(cur_node);

        cur_res_node = LIST_BEGIN(cur_err_block->_error_resources);
        while(cur_res_node != LIST_END(cur_err_block->_error_resources))
        {
            cur_res = (RESOURCE*)LIST_VALUE(cur_res_node);
            if(cur_res == abandon_res)
            {
                erase_res_node = cur_res_node;
                cur_res_node = LIST_NEXT(cur_res_node);
                list_erase(&cur_err_block->_error_resources, erase_res_node);
                break;

            }
            else
            {
                cur_res_node = LIST_NEXT(cur_res_node);
            }
        }

        cur_node = LIST_NEXT(cur_node);
    }

    return SUCCESS;

}

BOOL correct_manager_origin_resource_is_useable( CORRECT_MANAGER*  correct_m)
{

    return resource_is_useable(correct_m->_origin_res);
}

BOOL resource_is_useable( RESOURCE* p_res)
{
    DISPATCH_STATE res_state;
    LOG_DEBUG("resource_is_useable .");

    if(p_res  == NULL)
    {
        LOG_ERROR("resource_is_useable, origin res is NULL.");
        return FALSE;
    }

    res_state =  get_resource_state(p_res);

    if(res_state == ABANDON)
    {

        LOG_ERROR("resource_is_useable, because origin res 0x%x is abdon, so not use.", p_res);
        return FALSE;
    }
    else
    {
        LOG_DEBUG("resource_is_useable, because origin res 0x%x is %u, so can use.", p_res,res_state);
        return TRUE;
    }
}

BOOL correct_manager_is_origin_resource( CORRECT_MANAGER*  correct_m, RESOURCE*  res)
{
    LOG_DEBUG("correct_manager_is_origin_resource, res:0x%x .", res);

    if(correct_m->_origin_res  == NULL)
    {
        LOG_DEBUG("correct_manager_is_origin_resource, origin res is NULL.");
        return FALSE;
    }

    if(correct_m->_origin_res == res)
    {
        LOG_DEBUG("correct_manager_is_origin_resource,res:0x%x  is origin.", res);
        return TRUE;
    }
    else
    {
        LOG_DEBUG("correct_manager_is_origin_resource,res:0x%x  ,but origin is:0x%x .", res, correct_m->_origin_res);
        return FALSE;
    }
}

BOOL correct_manager_is_relate_error_block(CORRECT_MANAGER*  correct_m,  RANGE* p_r)
{
    return range_is_relate_error_block( &correct_m->_error_ranages, p_r );
}

