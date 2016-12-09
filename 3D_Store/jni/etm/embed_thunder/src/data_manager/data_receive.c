#include "utility/mempool.h"
#include "utility/settings.h"
#include "utility/logid.h"
#define LOGID LOGID_DATA_RECEIVE
#include "utility/logger.h"
#include "utility/utility.h"

#include "data_receive.h"
#include "data_buffer.h"

static _int32 MAX_FLUSH_UNIT_NUM  = DATA_RECEIVE_MAX_FLUSH_UNIT_NUM;
static _u32 MIN_RANGE_DATA_BUFFER_NODE  = DATA_RECEIVE_MIN_RANGE_DATA_BUFFER_NODE;

static SLAB* g_range_data_buffer_node_slab = NULL;

_int32 get_data_receiver_cfg_parameter()
{
    _int32 ret_val = SUCCESS;

    ret_val = settings_get_int_item("data_receiver.max_flush_unit_num", (_int32*)&MAX_FLUSH_UNIT_NUM);
    LOG_DEBUG("get_data_receiver_cfg_parameter,  data_receiver.max_flush_unit_num: %u .", MAX_FLUSH_UNIT_NUM);

    ret_val = settings_get_int_item("data_receiver.min_alloc_range_data_buffer_num", (_int32*)&MIN_RANGE_DATA_BUFFER_NODE);
    LOG_DEBUG("get_data_receiver_cfg_parameter,  data_receiver.min_alloc_range_data_buffer_num : %u .", MIN_RANGE_DATA_BUFFER_NODE);

    return ret_val;
}

_int32 get_max_flush_unit_num()
{
    return MAX_FLUSH_UNIT_NUM;
}


_int32 init_range_data_buffer_slab()
{
    _int32 ret_val = SUCCESS;

    if(g_range_data_buffer_node_slab != NULL)
    {
        return ret_val;
    }

    ret_val = mpool_create_slab(sizeof(RANGE_DATA_BUFFER), MIN_RANGE_DATA_BUFFER_NODE, 0, &g_range_data_buffer_node_slab);
    return (ret_val);
}

_int32 destroy_range_data_buffer_slab()
{
    _int32 ret_val = SUCCESS;
    if(g_range_data_buffer_node_slab !=  NULL)
    {
        mpool_destory_slab(g_range_data_buffer_node_slab);
        g_range_data_buffer_node_slab = NULL;
    }

    return (ret_val);
}

_int32 alloc_range_data_buffer_node(RANGE_DATA_BUFFER** pp_node)
{
    return  mpool_get_slip(g_range_data_buffer_node_slab, (void**)pp_node);
}

_int32 free_range_data_buffer_node(RANGE_DATA_BUFFER* p_node)
{
    return  mpool_free_slip(g_range_data_buffer_node_slab, (void*)p_node);
}



void buffer_list_init(RANGE_DATA_BUFFER_LIST *  buffer_list)
{
    list_init(&buffer_list->_data_list);
}

_u32 buffer_list_size(RANGE_DATA_BUFFER_LIST *buffer_list)
{
    return list_size(&buffer_list->_data_list);
}

_int32 buffer_list_add(RANGE_DATA_BUFFER_LIST *buffer_list, RANGE*  range, char *data,  _u32 length, _u32 buffer_len)
{
    RANGE_DATA_BUFFER* new_node  = NULL;
    RANGE_DATA_BUFFER* cur_buffer = NULL;
    LIST_ITERATOR  cur_node = NULL;

    LOG_DEBUG("buffer_list_add, buffer_list:0x%x , range(%u,%u), buffer:0x%x, length:%u, buffer_len:%u .", buffer_list, range->_index, range->_num, data, length, buffer_len);

    //  sd_malloc(sizeof(RANGE_DATA_BUFFER) ,(void**)&new_node);

    alloc_range_data_buffer_node(&new_node);
    if(new_node == NULL)
    {
        LOG_ERROR("buffer_list_add, buffer_list:0x%x , malloc RANGE_DATA_BUFFER failure .", buffer_list);

        return OUT_OF_MEMORY;
    }

    new_node->_range = *range;
    new_node->_data_length = length;
    new_node->_data_ptr = data;
    new_node->_buffer_length = buffer_len;

    cur_node = LIST_BEGIN(buffer_list->_data_list);

    while(cur_node != LIST_END(buffer_list->_data_list))
    {
        cur_buffer = (RANGE_DATA_BUFFER*)cur_node->_data;

        if(cur_buffer->_range._index <= range->_index)
        {
            cur_node =  LIST_NEXT(cur_node);
        }
        else
        {
            break;
        }
    }

    if(cur_node != LIST_END(buffer_list->_data_list))
    {
        list_insert(&buffer_list->_data_list, new_node, cur_node);
    }
    else
    {
        list_push(&buffer_list->_data_list, new_node);
    }
    return SUCCESS;
}


_int32 buffer_list_splice(RANGE_DATA_BUFFER_LIST *l_buffer_list, RANGE_DATA_BUFFER_LIST *r_buffer_list)
{
    RANGE_DATA_BUFFER* l_data_buffer  = NULL;
    RANGE_DATA_BUFFER* r_data_buffer = NULL;
    LIST_ITERATOR  l_cur_node = NULL;
    LIST_ITERATOR  r_cur_node = NULL;

    LOG_DEBUG("buffer_list_splice,before splice, l_buffer_list:0x%x ,size:%u,  r_buffer_list:0x%x, size:%u.", l_buffer_list, list_size(&l_buffer_list->_data_list),r_buffer_list, list_size(&r_buffer_list->_data_list));


    l_cur_node = LIST_BEGIN(l_buffer_list->_data_list);
    r_cur_node = LIST_BEGIN(r_buffer_list->_data_list);

    while(r_cur_node != LIST_END(r_buffer_list->_data_list))
    {
        if(l_cur_node == LIST_END(l_buffer_list->_data_list))
        {
            r_data_buffer = LIST_VALUE(r_cur_node);

            list_push(&l_buffer_list->_data_list, r_data_buffer);

            r_cur_node =  LIST_NEXT(r_cur_node);
            continue;
        }

        l_data_buffer = LIST_VALUE(l_cur_node);
        r_data_buffer = LIST_VALUE(r_cur_node);

        if(l_data_buffer->_range._index < r_data_buffer->_range._index)
        {
            l_cur_node =  LIST_NEXT(l_cur_node);
        }
        else
        {
            list_insert(&l_buffer_list->_data_list, r_data_buffer, l_cur_node);
            r_cur_node =  LIST_NEXT(r_cur_node);
        }
    }

    LOG_DEBUG("buffer_list_splice, after splice, l_buffer_list:0x%x ,size:%u,  r_buffer_list:0x%x, size:%u.", l_buffer_list, list_size(&l_buffer_list->_data_list),r_buffer_list, list_size(&r_buffer_list->_data_list));

    return SUCCESS;
}

_int32 buffer_list_delete(RANGE_DATA_BUFFER_LIST *buffer_list, RANGE*  range, RANGE_LIST* ret_range_list)
{
    RANGE_DATA_BUFFER* cur_buffer =  NULL;
    LIST_ITERATOR  erase_node = NULL;

    LIST_ITERATOR  cur_node = LIST_BEGIN(buffer_list->_data_list);

    LOG_DEBUG("buffer_list_delete, buffer_list:0x%x , delete range(%u,%u) .", buffer_list, range->_index, range->_num);

    while(cur_node != LIST_END(buffer_list->_data_list))
    {
        cur_buffer = (RANGE_DATA_BUFFER*)cur_node->_data;

        if(cur_buffer->_range._index + cur_buffer->_range._num <= range->_index)
        {
            cur_node =  LIST_NEXT(cur_node);
        }
        else  if(cur_buffer->_range._index >= range->_index + range->_num)
        {
            break;
        }
        else
        {
            erase_node = cur_node;
            cur_node =  LIST_NEXT(cur_node);
            list_erase(&buffer_list->_data_list, erase_node);

            range_list_add_range(ret_range_list, &cur_buffer->_range, NULL, NULL);

            dm_free_buffer_to_data_buffer(cur_buffer->_buffer_length, &(cur_buffer->_data_ptr));
            //sd_free(cur_buffer);
            free_range_data_buffer_node(cur_buffer);
        }
    }

    return SUCCESS;
}

_int32 drop_buffer_list(RANGE_DATA_BUFFER_LIST *buffer_list)
{
    RANGE_DATA_BUFFER* cur_buffer = NULL;
    LOG_DEBUG("drop_buffer_list, drop buffer_list:0x%x .", buffer_list);


    do
    {
        list_pop(&buffer_list->_data_list, (void**)&cur_buffer);
        if(cur_buffer == NULL)
        {
            break;
        }
        else
        {
            dm_free_buffer_to_data_buffer(cur_buffer->_buffer_length, &(cur_buffer->_data_ptr));
            // sd_free(cur_buffer);
            free_range_data_buffer_node(cur_buffer);
            cur_buffer = NULL;
        }


    }
    while(1);

    return SUCCESS;

}

_int32 drop_buffer_list_without_buffer(RANGE_DATA_BUFFER_LIST *buffer_list)
{
    RANGE_DATA_BUFFER* cur_buffer = NULL;
    LOG_DEBUG("drop_buffer_list, drop buffer_list:0x%x .", buffer_list);


    do
    {
        list_pop(&buffer_list->_data_list, (void**)&cur_buffer);
        if(cur_buffer == NULL)
        {
            break;
        }
        else
        {

            free_range_data_buffer_node(cur_buffer);
            cur_buffer = NULL;
        }


    }
    while(1);

    return SUCCESS;

}


_int32 drop_buffer_from_range_buffer(RANGE_DATA_BUFFER *p_range_buffer)
{
    if(p_range_buffer == NULL)  return SUCCESS;

    LOG_DEBUG("drop_buffer_from_range_buffer, range_buffer:0x%x, buffer:0x%x , range (%u,%u), data_len:%u, buffer_len:%u.",
              p_range_buffer, p_range_buffer->_data_ptr, p_range_buffer->_range._index, p_range_buffer->_range._num,
              p_range_buffer->_data_length,p_range_buffer->_buffer_length);

    dm_free_buffer_to_data_buffer(p_range_buffer->_buffer_length, &(p_range_buffer->_data_ptr));

    return SUCCESS;

}

void data_receiver_init(DATA_RECEIVER *data_receive)
{
    LOG_DEBUG("data_receiver_init .");
    list_init(&(data_receive->_buffer_list._data_list));
    range_list_init(&(data_receive->_total_receive_range));
    range_list_init(&(data_receive->_cur_cache_range));
    data_receive->_block_size = 0;
}

void data_receiver_unit(DATA_RECEIVER *data_receive)
{
    LOG_DEBUG("data_receiver_unit .");
    drop_buffer_list(&data_receive->_buffer_list);
    range_list_clear(&(data_receive->_total_receive_range));
    range_list_clear(&(data_receive->_cur_cache_range));
    data_receive->_block_size = 0;
}

void data_receiver_set_blocksize(DATA_RECEIVER *data_receive, _u32  block_size)
{
    LOG_DEBUG("data_receiver_set_blocksize, block_size:%u .", block_size);
    data_receive->_block_size = block_size;

}

BOOL data_receiver_is_empty(DATA_RECEIVER *data_receive)
{
    LOG_DEBUG("data_receiver_is_empty, cur_cache node size:%u .", data_receive->_cur_cache_range._node_size);
    return    data_receive->_cur_cache_range._node_size == 0? TRUE:FALSE;
}


_int32 data_receiver_add_buffer(DATA_RECEIVER *data_receive, RANGE*  range, char *data,  _u32 length, _u32 buffer_len)
{
    int ret_val = SUCCESS;

    LOG_DEBUG("data_receiver_add_buffer, data_receive:0x%x , range(%u,%u), buffer:0x%x, length:%u, buffer_len:%u .", data_receive, range->_index, range->_num, data, length, buffer_len);

    ret_val = buffer_list_add(&(data_receive->_buffer_list), range, data, length, buffer_len);

    CHECK_VALUE(ret_val);

    ret_val = range_list_add_range(&(data_receive->_total_receive_range),range,NULL, NULL);

    ret_val = range_list_add_range(&(data_receive->_cur_cache_range),range,NULL, NULL);

    return ret_val;
}

_int32 data_receiver_del_buffer(DATA_RECEIVER *data_receive, RANGE*  range)
{
    int ret_val = SUCCESS;

    RANGE_LIST erase_range_list;

    range_list_init(&erase_range_list);

    LOG_DEBUG("data_receiver_del_buffer, data_receive:0x%x , delete range(%u,%u) .", data_receive, range->_index, range->_num);

    ret_val = buffer_list_delete(&(data_receive->_buffer_list), range, &erase_range_list);

    CHECK_VALUE(ret_val);

    out_put_range_list(&erase_range_list);

    ret_val = range_list_delete_range_list(&(data_receive->_total_receive_range),&erase_range_list);

    ret_val = range_list_delete_range_list(&(data_receive->_cur_cache_range),&erase_range_list);

    range_list_clear(&erase_range_list);
    return ret_val;
}

void data_receiver_destroy(DATA_RECEIVER *data_receive)
{
    LOG_DEBUG("data_receiver_destroy .");
    data_receiver_unit(data_receive);
}

void data_receiver_cp_buffer(DATA_RECEIVER *data_receive, RANGE_DATA_BUFFER_LIST* need_flush_data)
{
    LOG_DEBUG("data_receiver_cp_buffer .");
    list_cat_and_clear(&need_flush_data->_data_list, &data_receive->_buffer_list._data_list);
    range_list_clear(&data_receive->_cur_cache_range) ;

    return ;
}

_int32 range_list_erase_buffer_range_list(RANGE_LIST* range_list, RANGE_DATA_BUFFER_LIST* buffer_list)
{

    RANGE_LIST_NODE*  begin_node = NULL;
    RANGE_DATA_BUFFER* cur_buffer = NULL;

    LIST_ITERATOR  cur_node = LIST_BEGIN(buffer_list->_data_list);

    LOG_DEBUG("range_list_erase_buffer_range_list .");

    while(cur_node != LIST_END(buffer_list->_data_list))
    {
        cur_buffer = (RANGE_DATA_BUFFER*)LIST_VALUE(cur_node);

        range_list_delete_range(range_list, &(cur_buffer->_range), begin_node, &begin_node);

        cur_node =  LIST_NEXT(cur_node);
    }

    return SUCCESS;
}

_int32 range_list_add_buffer_range_list(RANGE_LIST* range_list, RANGE_DATA_BUFFER_LIST* buffer_list)
{

    RANGE_LIST_NODE*  begin_node = NULL;
    RANGE_DATA_BUFFER* cur_buffer = NULL;

    LIST_ITERATOR  cur_node = LIST_BEGIN(buffer_list->_data_list);

    LOG_DEBUG("range_list_add_buffer_range_list .");

    while(cur_node != LIST_END(buffer_list->_data_list))
    {
        cur_buffer = (RANGE_DATA_BUFFER*)LIST_VALUE(cur_node);

        range_list_add_range(range_list, &(cur_buffer->_range), begin_node, &begin_node);

        cur_node =  LIST_NEXT(cur_node);
    }

    return SUCCESS;
}


_int32 data_receiver_erase_range(DATA_RECEIVER *data_receive, RANGE*  del_range)
{
    int ret_val = SUCCESS;

    LOG_DEBUG("data_receiver_erase_range, erase range (%u, %u) .",del_range->_index, del_range->_num);

    if(del_range->_num ==0)
        return ret_val;

    ret_val = range_list_delete_range(&(data_receive->_total_receive_range),del_range,NULL, NULL);

    return ret_val;
}

_int32 data_receiver_syn_data_buffer(DATA_RECEIVER *data_receive)
{
    RANGE_DATA_BUFFER* cur_buffer = NULL;
    LIST_ITERATOR  next_node = NULL;

    LIST_ITERATOR  cur_node = LIST_BEGIN(data_receive->_buffer_list._data_list);

    LOG_DEBUG("data_receiver_syn_data_buffer .");

    while(cur_node != LIST_END(data_receive->_buffer_list._data_list))
    {
        cur_buffer = (RANGE_DATA_BUFFER*)LIST_VALUE(cur_node);

        if(TRUE == range_list_is_include(&data_receive->_total_receive_range, &cur_buffer->_range))
        {
            cur_node = LIST_NEXT(cur_node);
        }
        else
        {
            dm_free_buffer_to_data_buffer(cur_buffer->_buffer_length, &(cur_buffer->_data_ptr));

            range_list_delete_range(&(data_receive->_cur_cache_range),&cur_buffer->_range,NULL, NULL);

            next_node =  LIST_NEXT(cur_node);
            list_erase(&data_receive->_buffer_list._data_list, cur_node);
            cur_node = next_node;
        }
    }

    return SUCCESS;

}

BOOL data_receiver_range_is_recv(DATA_RECEIVER *data_receive, const RANGE* r)
{
    LOG_DEBUG("data_receiver_range_is_recv,  range (%u, %u) .",r->_index, r->_num);

    return range_list_is_include(&data_receive->_total_receive_range, r);
}

_int32 data_receiver_get_releate_data_buffer(DATA_RECEIVER *data_receive, RANGE* r,  RANGE_DATA_BUFFER_LIST* ret_range_buffer)
{
    LIST_ITERATOR  cur_node = NULL;
    RANGE_DATA_BUFFER* cur_buffer = NULL;

    if(data_receive == NULL || r == NULL || ret_range_buffer == NULL)
        return -1;

    cur_node = LIST_BEGIN(data_receive->_buffer_list._data_list);
    while(cur_node != LIST_END(data_receive->_buffer_list._data_list))
    {
        cur_buffer = (RANGE_DATA_BUFFER*)LIST_VALUE(cur_node);
        if(cur_buffer->_range._index >= r->_index + r->_num)
        {
            break;
        }
        else if(cur_buffer->_range._index  + cur_buffer->_range._num <= r->_index )
        {
            //cur_node = LIST_NEXT(cur_node);
        }
        else
        {
            list_push(&ret_range_buffer->_data_list, cur_buffer);
        }

        cur_node = LIST_NEXT(cur_node);
    }

    return SUCCESS;
}


_int32 get_range_list_from_buffer_list(RANGE_DATA_BUFFER_LIST *buffer_list,  RANGE_LIST* ret_range_list)
{
    RANGE_DATA_BUFFER* cur_buffer =  NULL;

    LIST_ITERATOR  cur_node = LIST_BEGIN(buffer_list->_data_list);

    LOG_DEBUG("get_range_list_from_buffer_list, buffer_list:0x%x  .", buffer_list );

    while(cur_node != LIST_END(buffer_list->_data_list))
    {
        cur_buffer = (RANGE_DATA_BUFFER*)cur_node->_data;

        range_list_add_range(ret_range_list, &cur_buffer->_range, NULL, NULL);

        cur_node = LIST_NEXT(cur_node );
    }

    out_put_range_list(ret_range_list);

    return SUCCESS;
}


_int32 get_releate_data_buffer_from_range_data_buffer_by_range(RANGE_DATA_BUFFER_LIST* src_range_buffer, RANGE* r,  RANGE_DATA_BUFFER_LIST* ret_range_buffer)
{
    LIST_ITERATOR  cur_node = NULL;
    RANGE_DATA_BUFFER* cur_buffer = NULL;

    if(src_range_buffer == NULL || r == NULL || ret_range_buffer == NULL)
        return -1;

    cur_node = LIST_BEGIN(src_range_buffer->_data_list);
    while(cur_node != LIST_END(src_range_buffer->_data_list))
    {
        cur_buffer = (RANGE_DATA_BUFFER*)LIST_VALUE(cur_node);
        if(cur_buffer->_range._index >= r->_index + r->_num)
        {
            break;
        }
        else if(cur_buffer->_range._index  + cur_buffer->_range._num <= r->_index )
        {
            //cur_node = LIST_NEXT(cur_node);
        }
        else
        {
            list_push(&ret_range_buffer->_data_list, cur_buffer);
        }

        cur_node = LIST_NEXT(cur_node);
    }

    return SUCCESS;
}


