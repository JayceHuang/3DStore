
#include "dispatcher_interface.h"

#include "utility/define.h"
#include "utility/list.h"
#include "utility/map.h"
#include "utility/mempool.h"
#include "utility/settings.h"
#include "asyn_frame/msg.h"
#include "p2p_data_pipe/p2p_pipe_interface.h"
#include "p2p_data_pipe/p2p_data_pipe.h"
#include "p2p_data_pipe/p2p_send_cmd.h"
#include "http_data_pipe/http_data_pipe_interface.h"
#include "ftp_data_pipe/ftp_data_pipe_interface.h"

#include "connect_manager/connect_manager_imp.h"

#include "utility/logid.h"
#define LOGID LOGID_DISPATCHER
#include "utility/logger.h"

#define DISPATCHER_TIMER  10
#define DISPATCHER_ONCE_TIMER  11
#define MIN_DISPATCHER_TIMER_MS  100
#define FAST_SPEED_LEVEL_FOR_VOD  (5 * 1024)
#define MAX_WAIT_TIME_FOR_NORMAL 15
#define MAX_WAIT_TIME_FOR_VOD 5

static _u32 DISPATCHER_TIMER_INTERVAL = 1;
static _u32 MIN_DISPATCH_BLOCK_NUM = 1;
static _u32 MIN_DISPATCH_ITEM_NODE =  DISPATCH_MIN_DISPATCH_ITEM_NODE;
static _u32 MAX_OVER_ALLOC_TIMES = 2;
static _u32 MIN_PIPE_NUM = 3;
static _u32 MIN_ALLOC_TIME = 10;
static _u32  VOD_FIRST_WAIT_TIME = 3 ;
static _u32 g_max_wait_time = MAX_WAIT_TIME_FOR_NORMAL;
static SLAB* g_dispatch_item_slab = NULL;
static _u32 SECTION_UNIT = 32;

_int32 ds_dispatch_at_vod(DISPATCHER* ptr_dispatch_interface);

typedef struct _tag_dispatch_item
{
    BOOL _is_pipe_alloc;
    BOOL _is_completed;
    _u32 _alloc_times;
    LIST _owned_pipes;
} DISPATCH_ITEM;

_int32 down_map_comparator(void *E1, void *E2)
{
    _u32 l = (_u32)E1;
    _u32 r = (_u32)E2;
    if(l<r)  return -1;
    else if(l==r)  return 0;
    else return 1;
}


// 重要函数，分块调度计算pipe分块大小。规则：
// 1. 根据pipe速度计算分块；
// 2. 若pipe无速度，则根据pipe类型计算；
//  （TODO）判断cdn，且合理调整块大小参数。
_u32 calc_assign_range_num_to_pipe(DATA_PIPE* p_pipe)
{
    _u32 calc_num = 0;

    if (p_pipe == NULL)
    {
        return 0;
    }
    if (p_pipe->_dispatch_info._speed)
    {
        calc_num = p_pipe->_dispatch_info._speed * MIN_ALLOC_TIME / get_data_unit_size();
        calc_num = MAX(calc_num, 1);
    } 
    else
    {
        if((get_resource_level(p_pipe->_p_resource) == MAX_RES_LEVEL))  /* 原始资源或CDN资源 */
        {
            calc_num = 8;
        }
        else if (p_pipe->_dispatch_info._is_support_range == TRUE && p_pipe->_dispatch_info._is_support_long_connect == TRUE)
        {
            calc_num = 4;
            if (p_pipe->_data_pipe_type == HTTP_PIPE || p_pipe->_data_pipe_type == FTP_PIPE)
            {
                calc_num *=2;
            }
        } 
        else if (p_pipe->_dispatch_info._is_support_range == TRUE)
        {
            if (p_pipe->_data_pipe_type == HTTP_PIPE || p_pipe->_data_pipe_type == FTP_PIPE) 
            {
                calc_num = 4;
            }
            else
            {
                calc_num = 2;
            }
        } 
        else
        {
            calc_num = 0;//MAX_U32
        }
    }
    return calc_num;
}

_int32 init_dispatcher_module()
{
    _int32 ret_val = SUCCESS;
    ret_val = get_dispatcher_cfg_parameter();
    CHECK_VALUE(ret_val);

    ret_val = init_dispatch_item_slab();
    CHECK_VALUE(ret_val);

    return ret_val;
}

_int32 uninit_dispatcher_module()
{
    _int32 ret_val = SUCCESS;
    ret_val = destroy_dispatch_item_slab();
    CHECK_VALUE(ret_val);

    return ret_val;
}

_int32 get_dispatcher_cfg_parameter()
{
    _int32 ret_val = SUCCESS;
    ret_val = settings_get_int_item("dispatcher.dispatcher_time_interval_s", (_int32*)&DISPATCHER_TIMER_INTERVAL);
    LOG_DEBUG("get_dispatcher_cfg_parameter,  dispatcher.dispatcher_time_interval_s: %u .", DISPATCHER_TIMER_INTERVAL);
    ret_val = settings_get_int_item("dispatcher.min_dispatch_block_num", (_int32*)&MIN_DISPATCH_BLOCK_NUM);
    LOG_DEBUG("get_dispatcher_cfg_parameter,  dispatcher.min_dispatch_block_num : %u .", MIN_DISPATCH_BLOCK_NUM);
    ret_val = settings_get_int_item("dispatcher.min_alloc_dispatch_item_num", (_int32*)&MIN_DISPATCH_ITEM_NODE);
    LOG_DEBUG("get_dispatcher_cfg_parameter,  dispatcher.min_alloc_dispatch_item_num : %u .", MIN_DISPATCH_ITEM_NODE);
    ret_val = settings_get_int_item("dispatcher.vod_once_alloc_time", (_int32*)&MIN_ALLOC_TIME);
    LOG_DEBUG("get_dispatcher_cfg_parameter,  dispatcher.vod_once_alloc_time : %u .", MIN_ALLOC_TIME);
    ret_val = settings_get_int_item("dispatcher.vod_first_wait_time", (_int32*)&VOD_FIRST_WAIT_TIME);
    LOG_DEBUG("get_dispatcher_cfg_parameter,  dispatcher.vod_first_wait_time : %u .", VOD_FIRST_WAIT_TIME);
    ret_val = settings_get_int_item("dispatcher.max_wait_time", (_int32*)&g_max_wait_time);
    LOG_DEBUG("get_dispatcher_cfg_parameter,  dispatcher.max_wait_time : %u .", g_max_wait_time);
    //	ret_val = settings_get_int_item("dispatcher.dispatch_mod", (_int32*)&dispatch_mod);
    //  LOG_DEBUG("get_dispatcher_cfg_parameter,  dispatcher.dispatch_mod : %u .", dispatch_mod);
    return ret_val;
}

_int32 init_dispatch_item_slab()
{
    _int32 ret_val = SUCCESS;

    if (g_dispatch_item_slab != NULL)
    {
        return ret_val;
    }

    ret_val = mpool_create_slab(sizeof(DISPATCH_ITEM), MIN_DISPATCH_ITEM_NODE, 0, &g_dispatch_item_slab);
    return (ret_val);
}

_int32 destroy_dispatch_item_slab()
{
    _int32 ret_val = SUCCESS;
    if(g_dispatch_item_slab !=  NULL)
    {
        mpool_destory_slab(g_dispatch_item_slab);
        g_dispatch_item_slab = NULL;
    }

    return (ret_val);
}

_int32 ds_alloc_dispatch_item_node(DISPATCH_ITEM** pp_node)
{
    return  mpool_get_slip(g_dispatch_item_slab, (void**)pp_node);
}

_int32 ds_free_dispatch_item_node(DISPATCH_ITEM* p_node)
{
    return  mpool_free_slip(g_dispatch_item_slab, (void*)p_node);
}

void init_dispatch_item(DISPATCH_ITEM*  ptr_dis_item,
                        BOOL is_pipe_alloc, BOOL is_complete, DATA_PIPE* ptr_pipe)
{
    LOG_DEBUG("init_dispatch_item");

    list_init(&ptr_dis_item->_owned_pipes);
    ptr_dis_item->_is_pipe_alloc = is_pipe_alloc;
    ptr_dis_item->_is_completed = is_complete;
    if(is_complete == TRUE)
        ptr_dis_item->_alloc_times = 1;
    else
        ptr_dis_item->_alloc_times = 0;
    if(ptr_pipe != NULL)
    {
        list_push(&ptr_dis_item->_owned_pipes, ptr_pipe);
    }
}

void dispatch_item_add_pipe(DISPATCH_ITEM*  ptr_dis_item,
    DATA_PIPE* ptr_pipe)
{
    //LIST_ITERATOR cur_it = NULL;
    //DATA_PIPE* cur_pipe = NULL;

    LOG_DEBUG("dispatch_item_add_pipe");

    if(ptr_pipe != NULL)
    {
        list_push(&ptr_dis_item->_owned_pipes,ptr_pipe);

        /*  if(ptr_pipe->_dispatch_info._overlap_range_list._node_size == 0)
          {
                list_push(&ptr_dis_item->_owned_pipes,ptr_pipe);
          }
        else
        {
               cur_it = LIST_BEGIN(ptr_dis_item->_owned_pipes);
           while(cur_it != LIST_END(ptr_dis_item->_owned_pipes))
           {
                  cur_pipe = LIST_VALUE(cur_it);

               if(cur_pipe->_dispatch_info._overlap_range_list._node_size> ptr_pipe->_dispatch_info._overlap_range_list._node_size)
               {
                    list_insert(&ptr_dis_item->_owned_pipes, ptr_pipe, cur_it);
                    return ;
               }
               cur_it = LIST_NEXT(cur_it);
           }

            list_push(&ptr_dis_item->_owned_pipes,ptr_pipe);
        }*/
    }
}

/*void ds_add_candidate_pipe(PIPE_LIST*  ptr_pipe_list, DATA_PIPE* ptr_pipe)
{
       LIST_ITERATOR cur_pipe_it = NULL;
    DATA_PIPE* cur_pipe;

       if(ptr_pipe == NULL || ptr_pipe->_dispatch_info._overlap_range_list._node_size == 0)
       {
             return ;
       }

    cur_pipe_it = LIST_BEGIN(ptr_pipe_list->_data_pipe_list) ;
    while(cur_pipe_it !=  LIST_END(ptr_pipe_list->_data_pipe_list))
    {
          cur_pipe = LIST_VALUE(cur_pipe_it);
          if(cur_pipe->_dispatch_info._overlap_range_list._head_node->_range._num > ptr_pipe->_dispatch_info._overlap_range_list._head_node->_range._num)
          {
                 break;
          }
          cur_pipe_it= LIST_NEXT(cur_pipe_it);
    }

    if(cur_pipe_it !=  LIST_END(ptr_pipe_list->_data_pipe_list))
    {
          list_insert(&ptr_pipe_list->_data_pipe_list,ptr_pipe,cur_pipe_it);
    }
    else
    {
          list_push(&ptr_pipe_list->_data_pipe_list,ptr_pipe);
    }

    return;
}*/


void init_download_map(MAP* down_map)
{
    LOG_DEBUG("init_download_map");

    map_init(down_map, down_map_comparator);
    return;
}

void unit_download_map(MAP* down_map)
{
    MAP_ITERATOR  cur_node = NULL;
    MAP_ITERATOR  earse_node = NULL;
    DISPATCH_ITEM* cur_dis_item = NULL;

    LOG_DEBUG("unit_download_map");

    cur_node = MAP_BEGIN(*down_map);
    while(cur_node != MAP_END(*down_map))
    {
        cur_dis_item = MAP_VALUE(cur_node);
        clear_pipe_list(&cur_dis_item->_owned_pipes);

        earse_node = cur_node;
        cur_node = MAP_NEXT(*down_map, cur_node);
        map_erase_iterator(down_map,earse_node);

        ds_free_dispatch_item_node(cur_dis_item);
    }

    return;
}

void clear_pipe_list(LIST*  p_pipe_list)
{
    DATA_PIPE* p_data_pipe = NULL;

    //LOG_DEBUG("clear_pipe_list");

    do
    {
        list_pop(p_pipe_list, (void**)&p_data_pipe);
        if(p_data_pipe == NULL)
        {
            break;
        }

    }
    while(1);

    return;
}

/*
    更新下载地图中的某一个节点，index作为查找key
*/
_int32 download_map_update_item(MAP* down_map, _u32 index,
                                BOOL is_pipe_alloc, BOOL is_completed, DATA_PIPE* ptr_data_pipe, BOOL force)
{
    _int32 ret_val = SUCCESS;

    DISPATCH_ITEM*  cur_item =  NULL;
    PAIR insert_node;
    MAP_ITERATOR  cur_node = NULL;

    LOG_DEBUG("download_map_update_item");

    map_find_iterator(down_map, (void *)index, &cur_node);

    if(cur_node == MAP_END(*down_map))
    {
        // 没有找到对应的下载地图节点
        ret_val = ds_alloc_dispatch_item_node(&cur_item);
        CHECK_VALUE(ret_val);

        init_dispatch_item(cur_item,is_pipe_alloc, is_completed, ptr_data_pipe);

        insert_node._key = (void*)index;
        insert_node._value= (void*)cur_item;
        map_insert_node(down_map, &insert_node);
    }
    else
    {
        // 找到了对应的下载地图节点
        cur_item = MAP_VALUE(cur_node);

        if(is_pipe_alloc == TRUE)
            cur_item->_is_pipe_alloc = is_pipe_alloc;

        if(force == TRUE)
        {
            cur_item->_is_completed = is_completed;
            if(is_completed == TRUE)
                cur_item->_alloc_times++;
        }

        if(ptr_data_pipe != NULL)
            dispatch_item_add_pipe(cur_item,  ptr_data_pipe);

    }

    return ret_val;
}

/*
    从下载地图中移除对应index节点中的pipe集合中的某个pipe
*/
_int32 download_map_erase_pipe(MAP* down_map,
                               _u32 index, DATA_PIPE* ptr_data_pipe)
{
    DISPATCH_ITEM*  cur_item =  NULL;

    MAP_ITERATOR  cur_node = NULL;
    LIST_ITERATOR  cur_pipe_node = NULL;
    DATA_PIPE* cur_data_pipe = NULL;

    LOG_DEBUG("download_map_erase_pipe, down_map:0x%x, index:%u, \
              ptr_data_pipe: 0x%x . ", down_map, index, ptr_data_pipe);

    map_find_iterator(down_map, (void *)index, &cur_node);
    if(cur_node == MAP_END(*down_map))
    {
        LOG_DEBUG("download_map_erase_pipe can not find index ,so return, \
                  down_map:0x%x, index:%u, ptr_data_pipe: 0x%x . ",
                  down_map, index, ptr_data_pipe);
        return SUCCESS;
    }
    else
    {
        cur_item = MAP_VALUE(cur_node);

        if(list_size(&cur_item->_owned_pipes) == 0)
        {
            LOG_DEBUG("download_map_erase_pipe find index,but has no pipe ,\
                      so return, down_map:0x%x, index:%u, ptr_data_pipe: 0x%x . ",
                      down_map, index, ptr_data_pipe);
            return SUCCESS;
        }

        cur_pipe_node = LIST_BEGIN(cur_item->_owned_pipes);
        while(cur_pipe_node != LIST_END(cur_item->_owned_pipes))
        {
            cur_data_pipe = LIST_VALUE(cur_pipe_node);
            if(cur_data_pipe == ptr_data_pipe)
            {
                LOG_DEBUG("download_map_erase_pipe find pipe, success erase ,\
                          so return, down_map:0x%x, index:%u, ptr_data_pipe: 0x%x . ",
                          down_map, index, ptr_data_pipe);

                list_erase(&cur_item->_owned_pipes, cur_pipe_node);
                return SUCCESS;

            }

            cur_pipe_node = LIST_NEXT(cur_pipe_node);
        }

        LOG_DEBUG("download_map_erase_pipe cannoit find pipe ,so return, \
                  down_map:0x%x, index:%u, ptr_data_pipe: 0x%x . ",
                  down_map, index, ptr_data_pipe);

        return SUCCESS;
    }
}

/*
    初始化调度中所用到的各种信息
*/
_int32 ds_init_dispatcher(DISPATCHER*  ptr_dispatch_interface,
                          DS_DATA_INTEREFACE*  ptr_data_interface,
                          CONNECT_MANAGER* ptr_connect_manager)
{
    LOG_DEBUG("ds_init_dispatcher");

    ptr_dispatch_interface->_data_interface._get_file_size
        = ptr_data_interface->_get_file_size;
    ptr_dispatch_interface->_data_interface._is_only_using_origin_res
        = ptr_data_interface->_is_only_using_origin_res;
    ptr_dispatch_interface->_data_interface._is_origin_resource
        = ptr_data_interface->_is_origin_resource;
    ptr_dispatch_interface->_data_interface._get_priority_range
        = ptr_data_interface->_get_priority_range;
    ptr_dispatch_interface->_data_interface._get_uncomplete_range
        = ptr_data_interface->_get_uncomplete_range;
    ptr_dispatch_interface->_data_interface._get_error_range_block_list
        = ptr_data_interface->_get_error_range_block_list;
    ptr_dispatch_interface->_data_interface._get_ds_is_vod_mode
        = ptr_data_interface->_get_ds_is_vod_mode;
    ptr_dispatch_interface->_data_interface._own_ptr
        = ptr_data_interface->_own_ptr;

    ptr_dispatch_interface->_p_connect_manager= ptr_connect_manager;
    ptr_dispatch_interface->_dispatch_mode = NORMAL;
    ptr_dispatch_interface->_dispatcher_timer = INVALID_MSG_ID;
    ptr_dispatch_interface->_immediate_dispatcher_timer = INVALID_MSG_ID;

    range_list_init(&ptr_dispatch_interface->_tmp_range_list);
    range_list_init(&ptr_dispatch_interface->_overalloc_range_list);
    range_list_init(&ptr_dispatch_interface->_overalloc_in_once_dispatch);
    range_list_init(&ptr_dispatch_interface->_cur_down_point);
    ptr_dispatch_interface->_cur_down_pipes = 0;
    ptr_dispatch_interface->_start_index = MAX_U32;
    ptr_dispatch_interface->_start_index_time_ms = MAX_U32;
    ptr_dispatch_interface->_last_dispatch_time_ms = MAX_U32;
    return SUCCESS;
}

_int32 ds_unit_dispatcher(DISPATCHER*  ptr_dispatch_interface)
{
    LOG_DEBUG("ds_unit_dispatcher");

    sd_memset(&ptr_dispatch_interface->_data_interface, 0 , sizeof(DS_DATA_INTEREFACE));
    ptr_dispatch_interface->_p_connect_manager= NULL;
    ptr_dispatch_interface->_dispatch_mode = NORMAL;
    ptr_dispatch_interface->_dispatcher_timer = INVALID_MSG_ID;
    ptr_dispatch_interface->_immediate_dispatcher_timer = INVALID_MSG_ID;

    range_list_clear(&ptr_dispatch_interface->_tmp_range_list);
    range_list_clear(&ptr_dispatch_interface->_overalloc_range_list);
    range_list_clear(&ptr_dispatch_interface->_overalloc_in_once_dispatch);
    range_list_clear(&ptr_dispatch_interface->_cur_down_point);
    ptr_dispatch_interface->_cur_down_pipes = 0;
    ptr_dispatch_interface->_start_index = MAX_U32;
    ptr_dispatch_interface->_start_index_time_ms = MAX_U32;
    ptr_dispatch_interface->_last_dispatch_time_ms = MAX_U32;
    return SUCCESS;
}


/*
    开始调度
*/
_int32 ds_start_dispatcher(DISPATCHER*  ptr_dispatch_interface)
{
    _int32 ret_val = SUCCESS;

    LOG_DEBUG("ds_start_dispatcher");

    // 定时器周期性的调度
    ret_val = start_timer((msg_handler)ds_dispatcher_timer_handler,
        NOTICE_INFINITE, DISPATCHER_TIMER_INTERVAL*1000,
        DISPATCHER_TIMER, ptr_dispatch_interface,
        &ptr_dispatch_interface->_dispatcher_timer);

    CHECK_VALUE(ret_val);

    // 直接调度
    ds_do_dispatch(ptr_dispatch_interface, TRUE);
    return ret_val;
}

_int32 ds_stop_dispatcher(DISPATCHER*  ptr_dispatch_interface)
{
    _int32 ret_val = SUCCESS;

    LOG_DEBUG("ds_stop_dispatcher");

    if(ptr_dispatch_interface->_dispatcher_timer != INVALID_MSG_ID)
    {
        ret_val = cancel_timer(ptr_dispatch_interface->_dispatcher_timer);
        ptr_dispatch_interface->_dispatcher_timer =  INVALID_MSG_ID;
    }

    if(ptr_dispatch_interface->_immediate_dispatcher_timer != INVALID_MSG_ID)
    {
        ret_val = cancel_timer(ptr_dispatch_interface->_immediate_dispatcher_timer);
        ptr_dispatch_interface->_immediate_dispatcher_timer = INVALID_MSG_ID;
    }

    return ret_val;
}


/*先把所有在下载erased_blocks的pipe的任务块调整到不和erased_blocks重叠，然后进行重新调度*/

/* dispatcher  需要通过pipe的指针访问pipe的dispatch info 信息(连接状态，  已下载块，   未下载块 等信息)*/

/*
    这里是真正的调度入口
*/

_int32 ds_do_dispatch(DISPATCHER* ptr_dispatch_interface, BOOL force)
{
    _u32 now_time;
    sd_assert(ptr_dispatch_interface != NULL);
    sd_time_ms(&now_time);
    if (force == FALSE)
    {
        if (TIME_SUBZ(now_time, ptr_dispatch_interface->_last_dispatch_time_ms) < MIN_DISPATCHER_TIMER_MS)
        {
            LOG_DEBUG("ds_do_dispatch too frequently and return. _last_dispatch_time_ms:%u", ptr_dispatch_interface->_last_dispatch_time_ms);
            return SUCCESS;
        }
    }

    LOG_DEBUG("ds_do_dispatch begin. force:%d", force);
    // 打开一个新连接
    ds_active_new_connect(ptr_dispatch_interface);

    if (DS_GET_FILE_SIZE(ptr_dispatch_interface->_data_interface) == 0)
    {
        LOG_DEBUG("ds_do_dispatch at no filesize !");
        // 无文件大小的调度策略
        ds_dispatch_at_no_filesize(ptr_dispatch_interface);
    }
    else if(DS_IS_VOD_MODE(ptr_dispatch_interface->_data_interface) == TRUE)
    {
        g_max_wait_time = MAX_WAIT_TIME_FOR_VOD;
        ds_dispatch_at_vod(ptr_dispatch_interface);
    }
    else if(cm_is_origin_mode(ptr_dispatch_interface->_p_connect_manager))
    {
        LOG_DEBUG("ds_do_dispatch at using origin resource !");
        // 只有原始资源的调度策略
        ds_dispatch_at_origin_res(ptr_dispatch_interface);
    }
    else
    {
        BOOL is_p2sp_task = cm_is_p2sptask(ptr_dispatch_interface->_p_connect_manager);
        BOOL is_bcid_valid  =is_p2sp_task ? cm_shubinfo_valid(ptr_dispatch_interface->_p_connect_manager):FALSE;
        LOG_DEBUG("ds_do_dispatch is_p2sp_task:%d, is_bcid_valid:%d", is_p2sp_task, is_bcid_valid);
        if(is_p2sp_task && !is_bcid_valid)
        {
            LOG_DEBUG("ds_do_dispatch at using origin resource, because cm_is_p2sptask but shubinfo is not valid, so only use origion mode..!");
            // 只有原始资源的调度策略
            ds_dispatch_at_origin_res(ptr_dispatch_interface);
        }
        else
        {
            LOG_DEBUG("ds_do_dispatch at muti resource !");
            // 多资源调度
            ds_dispatch_at_muti_res(ptr_dispatch_interface);
        }
    }
    ptr_dispatch_interface->_last_dispatch_time_ms = now_time;
    return SUCCESS;
}


_int32 ds_active_new_connect(DISPATCHER*  ptr_dispatch_interface)
{
    LOG_DEBUG("ds_active_new_connect");

    cm_create_pipes(ptr_dispatch_interface->_p_connect_manager);

    return SUCCESS;
}

_int32 ds_dispatch_at_no_filesize(DISPATCHER*  ptr_dispatch_interface)
{
    RANGE full_range;

    PIPE_LIST* p_list = NULL;
    LIST_ITERATOR  cur_node = NULL;
    DATA_PIPE* cur_pipe  = NULL;

    LOG_DEBUG("ds_dispatch_at_no_filesize");

    full_range._index = 0;
    full_range._num = MAX_U32;

    range_list_clear(&ptr_dispatch_interface->_tmp_range_list);
    range_list_clear(&ptr_dispatch_interface->_overalloc_range_list);
    ptr_dispatch_interface->_start_index = MAX_U32;
    ptr_dispatch_interface->_start_index_time_ms = MAX_U32;

    cm_get_working_server_pipe_list(ptr_dispatch_interface->_p_connect_manager,
        &p_list);

    if(p_list ==  NULL || list_size(&p_list->_data_pipe_list) == 0)
    {
        LOG_DEBUG("ds_dispatch_at_no_filesize cannot get working server pipe.");

        cm_get_connecting_server_pipe_list(ptr_dispatch_interface->_p_connect_manager,
            &p_list);
        if(p_list == NULL || list_size(&p_list->_data_pipe_list) == 0)
        {
            LOG_DEBUG("ds_dispatch_at_no_filesize cannot get connecting server \
                      pipe, cannot dispath.");
            return SUCCESS;
        }
    }

    //这个p_list既有可能是working_server_pipe也有可能是connecting_server_pipe
    cur_node =  LIST_BEGIN(p_list->_data_pipe_list);
    while(cur_node !=  LIST_END(p_list->_data_pipe_list))
    {
        cur_pipe = LIST_VALUE(cur_node);
        if(cur_pipe->_dispatch_info._uncomplete_ranges._node_size != 0)
        {
            // 如果有未完成块，意味着已经分配了某个pipe去下载了，则不再继续分配
            LOG_DEBUG("ds_dispatch_at_no_filesize , there is a runing data pipe\
                      so can not alloc range.");
            break;
        }
        cur_node = LIST_NEXT(cur_node);
    }

    // 说明此时没有pipe在下载
    // TODO 这里可以优化，用两个while循环没必要
    if(cur_node == LIST_END(p_list->_data_pipe_list))
    {
        cur_node =  LIST_BEGIN(p_list->_data_pipe_list);
        while(cur_node !=  LIST_END(p_list->_data_pipe_list))
        {
            cur_pipe = LIST_VALUE(cur_node);

            if((cur_pipe->_dispatch_info._pipe_state != PIPE_FAILURE)
                && cur_pipe->_dispatch_info._uncomplete_ranges._node_size == 0)
            {
                // 找到一个合适的pipe去下载
                LOG_DEBUG("ds_dispatch_at_no_filesize , alloc range (%u, %u) \
                          to data pipe: 0x%x",
                          full_range._index, full_range._num, cur_pipe);
                ds_assigned_range_to_pipe(cur_pipe, &full_range);
                break;
            }

            cur_node = LIST_NEXT(cur_node);
        }
    }

    return SUCCESS;
}

BOOL ds_dispatch_pipe_is_origin(DISPATCHER* ptr_dispatch_interface,
                                DATA_PIPE* p_pipe)
{
    return DS_IS_ORIGIN_RESOURCE(ptr_dispatch_interface->_data_interface,
        p_pipe->_p_resource);
}

_int32 ds_dispatch_at_origin_res(DISPATCHER* ptr_dispatch_interface)
{
    PIPE_LIST* p_working_list = NULL;
    PIPE_LIST* p_connecting_list = NULL;

    _u64 file_size = 0;
    RANGE full_r ;
    RANGE_LIST uncomplete_range_list;
    PIPE_LIST origin_pipe_list ;
    PIPE_LIST origin_downing_long_pipe_list;
    PIPE_LIST origin_downing_range_pipe_list;
    PIPE_LIST origin_downing_last_pipe_list;

    PIPE_LIST origin_new_long_pipe_list;
    PIPE_LIST origin_new_range_pipe_list;
    PIPE_LIST origin_new_last_pipe_list;

    RANGE_LIST downing_range_list;
    RANGE_LIST pri_range_list;
    RANGE_LIST_ITEROATOR cur_range_node = NULL;

    MAP download_map;

    RANGE_LIST * dispatch_range_list = NULL;

    _u32 uncomplete_range_size = 0;
    _u32 pipe_num = 0;

    _u32 dowing_no_range_pipe_num = 0;
    _u32 new_no_range_pipe_num = 0;

    _u32 range_num_per_pipe = 0;

    _int32  cur_index =0;
    _int32  next_index =0;
    RANGE cur_range;
    MAP_ITERATOR cur_map_node = NULL;
    MAP_ITERATOR next_map_node = NULL;
    LIST_ITERATOR cur_pipe = NULL;
    DISPATCH_ITEM* cur_dispatch_item = NULL;

    BOOL no_pipe = FALSE;

    DATA_PIPE* cur_data_pipe = NULL;

    LOG_DEBUG("ds_dispatch_at_origin_res");

    range_list_clear(&ptr_dispatch_interface->_tmp_range_list);
    range_list_clear(&ptr_dispatch_interface->_overalloc_range_list);
    ptr_dispatch_interface->_start_index = MAX_U32;
    ptr_dispatch_interface->_start_index_time_ms = MAX_U32;

    cm_get_working_server_pipe_list(ptr_dispatch_interface->_p_connect_manager,
        &p_working_list);
    LOG_DEBUG("ds_dispatch_at_origin_res, get %u working server data pipe",
        list_size(&p_working_list->_data_pipe_list));

    cm_get_connecting_server_pipe_list(ptr_dispatch_interface->_p_connect_manager,
        &p_connecting_list);
    LOG_DEBUG("ds_dispatch_at_origin_res, get %u connecting server data pipe",
        list_size(&p_connecting_list->_data_pipe_list));

    file_size = DS_GET_FILE_SIZE(ptr_dispatch_interface->_data_interface);
    LOG_DEBUG("ds_dispatch_at_origin_res, get filesize %llu", file_size);

    full_r = pos_length_to_range(0, file_size, file_size);

    list_init(&origin_pipe_list._data_pipe_list);
    list_init(&origin_downing_long_pipe_list._data_pipe_list);
    list_init(&origin_downing_range_pipe_list._data_pipe_list);
    list_init(&origin_downing_last_pipe_list._data_pipe_list);
    list_init(&origin_new_long_pipe_list._data_pipe_list);
    list_init(&origin_new_range_pipe_list._data_pipe_list);
    list_init(&origin_new_last_pipe_list._data_pipe_list);

    /* init uncomplete map */
    init_download_map(&download_map);

    range_list_init(&downing_range_list);
    range_list_init(&pri_range_list);
    range_list_init(&uncomplete_range_list);


    /*get priority range list*/
    DS_GET_PRIORITY_RANGE(ptr_dispatch_interface->_data_interface, &pri_range_list);

    if(pri_range_list._node_size == 0)
    {
        /*get uncomplet range list*/
        DS_GET_UNCOMPLETE_RANGE(ptr_dispatch_interface->_data_interface,
            &uncomplete_range_list);
        dispatch_range_list = &uncomplete_range_list;
    }
    else
    {
        dispatch_range_list = &pri_range_list;
    }

#ifndef DISPATCH_RANDOM_MODE
    // BT任务独立处理
    if(!cm_is_bttask(ptr_dispatch_interface->_p_connect_manager))
    {
        // 滤掉前面的32M
        ds_filter_range_list_from_begin(dispatch_range_list, SECTION_UNIT*1024/16);
    }
#endif

    if(dispatch_range_list->_node_size == 0)
    {
        LOG_DEBUG("ds_dispatch_at_origin_res, has no uncomplete range so return !");
        goto origin_res_dis_done;
    }

    ds_dispatch_get_origin_pipe_list(ptr_dispatch_interface, p_working_list,
        &origin_pipe_list);
    ds_dispatch_get_origin_pipe_list(ptr_dispatch_interface, p_connecting_list,
        &origin_pipe_list);

    LOG_DEBUG("ds_dispatch_at_origin_res, get %u origin server data pipe",
        list_size(&origin_pipe_list._data_pipe_list));

    /* build uncomplete map */
    ds_build_uncomplete_map(&download_map, &uncomplete_range_list);

    ds_build_pipe_list_map(&download_map, NULL, &origin_pipe_list, &uncomplete_range_list,
        &origin_downing_long_pipe_list,&origin_downing_range_pipe_list,
        &origin_downing_last_pipe_list,&origin_new_long_pipe_list,
        &origin_new_range_pipe_list,&origin_new_last_pipe_list,NULL,
        NULL, &downing_range_list, 0);

    LOG_DEBUG("ds_dispatch_at_origin_res, get %u origin long working data pipe",
        list_size(&origin_downing_long_pipe_list._data_pipe_list));
    LOG_DEBUG("ds_dispatch_at_origin_res, get %u origin range working data pipe",
        list_size(&origin_downing_range_pipe_list._data_pipe_list));
    LOG_DEBUG("ds_dispatch_at_origin_res, get %u origin last working data pipe",
        list_size(&origin_downing_last_pipe_list._data_pipe_list));
    LOG_DEBUG("ds_dispatch_at_origin_res, get %u origin long connecting data pipe",
        list_size(&origin_new_long_pipe_list._data_pipe_list));
    LOG_DEBUG("ds_dispatch_at_origin_res, get %u origin range connecting data pipe",
        list_size(&origin_new_range_pipe_list._data_pipe_list));
    LOG_DEBUG("ds_dispatch_at_origin_res, get %u origin last connecting data pipe",
        list_size(&origin_new_last_pipe_list._data_pipe_list));

    uncomplete_range_size = range_list_get_total_num(&uncomplete_range_list);

    pipe_num += list_size(&origin_downing_long_pipe_list._data_pipe_list);
    pipe_num += list_size(&origin_downing_range_pipe_list._data_pipe_list);
    pipe_num += list_size(&origin_new_long_pipe_list._data_pipe_list);
    pipe_num += list_size(&origin_new_range_pipe_list._data_pipe_list);

    dowing_no_range_pipe_num = list_size(&origin_downing_last_pipe_list._data_pipe_list);
    new_no_range_pipe_num =  list_size(&origin_new_last_pipe_list._data_pipe_list);

    if(pipe_num == 0 && new_no_range_pipe_num == 0 )
    {
        LOG_ERROR("ds_dispatch_at_origin_res, has no pipes, so return !");
        goto origin_res_dis_done;
    }

    if(new_no_range_pipe_num != 0 && dowing_no_range_pipe_num == 0)
    {
        list_pop(&origin_new_last_pipe_list._data_pipe_list, (void**)&cur_data_pipe);
        if(cur_data_pipe != NULL)
        {
            if(ds_assigned_range_to_pipe(cur_data_pipe, &full_r) == TRUE)
            {
                LOG_DEBUG("ds_dispatch_at_origin_res , success assign range (%u, %u) \
                          to no range data pipe: 0x%x.",
                          full_r._index, full_r._num, cur_data_pipe);
                goto origin_res_dis_done;
            }
            else
            {
                LOG_ERROR("ds_dispatch_at_origin_res ,failure to assign range (%u, %u) \
                          to no range data pipe: 0x%x .",
                          full_r._index, full_r._num, cur_data_pipe);
            }
        }

    }

    range_num_per_pipe = uncomplete_range_size/pipe_num;

    if(range_num_per_pipe <= MIN_DISPATCH_BLOCK_NUM)
        range_num_per_pipe = MIN_DISPATCH_BLOCK_NUM;

    LOG_DEBUG("ds_dispatch_at_origin_res, uncomplete_range_size: %u, pipe_num: %u,  \
              range_num_per_pipe: %u", uncomplete_range_size, pipe_num, range_num_per_pipe);

    cur_map_node = MAP_BEGIN(download_map);
    while(cur_map_node != MAP_END(download_map))
    {
        cur_index = (_u32)MAP_KEY(cur_map_node);
        cur_dispatch_item = (DISPATCH_ITEM*)MAP_VALUE(cur_map_node);

        // 获取当前节点的下一个节点
        next_map_node = MAP_NEXT(download_map, cur_map_node);
        if(next_map_node != MAP_END(download_map))
        {
            next_index = (_u32)MAP_KEY(next_map_node);
        }
        else
        {
            next_index = full_r._index + full_r._num;
        }

        // 如果当前这个index已经搞定了，就下载其他的index
        if(cur_dispatch_item->_is_completed)
        {
            cur_map_node = next_map_node;
            continue;
        }

        // 有pipe可以用来下载
        if(list_size(&cur_dispatch_item->_owned_pipes) != 0)
        {
            LOG_DEBUG("ds_dispatch_at_origin_res , cur_index:%u , next_index: %u, \
                      owned_pipes: %u",
                      cur_index,next_index, list_size(&cur_dispatch_item->_owned_pipes));
            cur_pipe = LIST_BEGIN(cur_dispatch_item->_owned_pipes);
            while(cur_pipe != LIST_END(cur_dispatch_item->_owned_pipes))
            {
                cur_data_pipe = (DATA_PIPE*)LIST_VALUE(cur_pipe);
                if(cur_index + range_num_per_pipe < next_index)
                {
                    cur_range._index = cur_index;
                    cur_range._num = range_num_per_pipe;
                    if(ds_assigned_range_to_pipe(cur_data_pipe, &cur_range) == TRUE)
                    {
                        LOG_DEBUG("ds_dispatch_at_origin_res , assign range (%u, %u) \
                                  to data pipe: 0x%x",
                                  cur_range._index, cur_range._num, cur_data_pipe);
                        cur_index += range_num_per_pipe;
                    }
                    else
                    {
                        LOG_ERROR("ds_dispatch_at_origin_res , failure to assign range \
                                  (%u, %u) to data pipe: 0x%x",
                                  cur_range._index, cur_range._num, cur_data_pipe);
                    }
                }
                else if(cur_index < next_index)
                {
                    cur_range._index = cur_index;
                    cur_range._num = next_index - cur_index;
                    if(ds_assigned_range_to_pipe(cur_data_pipe, &cur_range) == TRUE)
                    {
                        LOG_DEBUG("ds_dispatch_at_origin_res , assign range (%u, %u) to \
                                  data pipe: 0x%x",
                                  cur_range._index, cur_range._num, cur_data_pipe);
                        break;
                    }
                    else
                    {
                        LOG_ERROR("ds_dispatch_at_origin_res , failure to assign range \
                                  (%u, %u) to data pipe: 0x%x",
                                  cur_range._index, cur_range._num, cur_data_pipe);
                    }
                }
                else
                {
                    break;
                }
                cur_pipe = LIST_NEXT(cur_pipe);
            }
        }
        no_pipe = FALSE;

        LOG_DEBUG("ds_dispatch_at_origin_res , cur_index:%u , next_index: %u .",
            cur_index,next_index);

        while(cur_index < next_index)
        {
            if(cur_index + range_num_per_pipe < next_index)
            {
                cur_range._index = cur_index;
                cur_range._num = range_num_per_pipe;
            }
            else
            {
                cur_range._index = cur_index;
                cur_range._num = next_index - cur_index;
            }

            cur_data_pipe
                = ds_get_data_pipe(&origin_new_long_pipe_list,
                &origin_new_range_pipe_list,NULL, NULL);
            if(cur_data_pipe == NULL)
            {
                no_pipe= TRUE;
                LOG_DEBUG("ds_dispatch_at_origin_res , cannot  get data_pipe!");
                break;
            }

            if(ds_assigned_range_to_pipe(cur_data_pipe, &cur_range) == FALSE)
            {
                LOG_ERROR("ds_dispatch_at_origin_res , failure to assign range (%u, %u) \
                          to data pipe: 0x%x", cur_range._index, cur_range._num, cur_data_pipe);
                continue;
            }
            else
            {
                LOG_DEBUG("ds_dispatch_at_origin_res , assign range (%u, %u) \
                          to data pipe: 0x%x", cur_range._index, cur_range._num, cur_data_pipe);
            }

            cur_index += cur_range._num;
        }

        if(no_pipe == TRUE)
        {
            LOG_DEBUG("ds_dispatch_at_origin_res , no pipes, cur_index:%u, next_index:%u,\
                      cur_assign_range (%u, %u) !",
                      cur_index, next_index, cur_range._index, cur_range._num);
        }

        cur_map_node = next_map_node;
    }

    LOG_DEBUG("ds_dispatch_at_origin_res , finish process cur_index: %u, next_index: %u.",
        cur_index, next_index);

    if(list_size(&origin_new_long_pipe_list._data_pipe_list) != 0
        ||list_size(&origin_new_range_pipe_list._data_pipe_list) != 0 )
    {
        LOG_DEBUG("ds_dispatch_at_origin_res ,because after assign, there is also pipes ,\
                  so assign dowing ranges.");

        range_list_get_head_node(&downing_range_list, &cur_range_node);
        while(cur_range_node != NULL)
        {
            cur_data_pipe = ds_get_data_pipe(&origin_new_long_pipe_list,
                &origin_new_range_pipe_list,NULL, NULL);
            if(cur_data_pipe == NULL)
            {
                LOG_DEBUG("ds_dispatch_at_origin_res dowing assign , \
                          can not get data pipe to assign dowing range (%u, %u).",
                          cur_range_node->_range._index,  cur_range_node->_range._num);
                no_pipe= TRUE;
                break;
            }
            if(ds_assigned_range_to_pipe(cur_data_pipe, &cur_range_node->_range) == FALSE)
            {
                LOG_ERROR("ds_dispatch_at_origin_res dowing assign ,  failure to assign \
                          dowing range (%u, %u) to data pipe: 0x%x",
                          cur_range_node->_range._index, cur_range_node->_range._num,
                          cur_data_pipe);
                continue;
            }
            else
            {
                LOG_DEBUG("ds_dispatch_at_origin_res dowing assign , success to assign \
                          dowing range (%u, %u) to data pipe: 0x%x",
                          cur_range_node->_range._index, cur_range_node->_range._num,
                          cur_data_pipe);
            }
            range_list_get_next_node(&downing_range_list, cur_range_node, &cur_range_node);
        }
    }
    else
    {
        LOG_DEBUG("ds_dispatch_at_origin_res , because no pipe, \
                  so can not assign dowing ranges.");
    }


origin_res_dis_done:

    unit_download_map(&download_map);
    range_list_clear(&uncomplete_range_list);
    range_list_clear(&downing_range_list);
    range_list_clear(&pri_range_list);

    clear_pipe_list( &origin_pipe_list._data_pipe_list) ;

    clear_pipe_list( &origin_downing_long_pipe_list._data_pipe_list);
    clear_pipe_list( &origin_downing_range_pipe_list._data_pipe_list);
    clear_pipe_list( &origin_downing_last_pipe_list._data_pipe_list);
    clear_pipe_list( &origin_new_long_pipe_list._data_pipe_list);
    clear_pipe_list( &origin_new_range_pipe_list._data_pipe_list);
    clear_pipe_list( &origin_new_last_pipe_list._data_pipe_list);
    return SUCCESS;

}

void ds_dispatch_get_origin_pipe_list(
    DISPATCHER*  ptr_dispatch_interface, 
    PIPE_LIST* src_pipe_list, PIPE_LIST* origin_pipe_list)
{
    LIST_ITERATOR  cur_node = NULL;

    LOG_DEBUG("ds_dispatch_get_origin_pipe_list");

    if(src_pipe_list == NULL || origin_pipe_list == NULL)
        return;

    cur_node = LIST_BEGIN(src_pipe_list->_data_pipe_list);
    while(cur_node !=  LIST_END(src_pipe_list->_data_pipe_list))
    {
        DATA_PIPE* cur_pipe = LIST_VALUE(cur_node);

        if(ds_dispatch_pipe_is_origin(ptr_dispatch_interface, cur_pipe) == TRUE)
        {
            list_push(&origin_pipe_list->_data_pipe_list, cur_pipe);
        }

        cur_node = LIST_NEXT(cur_node);
    }
}

/*
    多资源调度策略
*/
_int32 ds_dispatch_at_muti_res_nomal(DISPATCHER*  ptr_dispatch_interface)
{
    PIPE_LIST* p_server_connected_list = NULL;
    PIPE_LIST* p_server_connecting_list = NULL;

    PIPE_LIST* p_peer_connected_list = NULL;
    PIPE_LIST* p_peer_connecting_list = NULL;

    PIPE_LIST* p_cdn_pipe_list = NULL;

    PIPE_LIST dised_server_long_connect_list ;
    PIPE_LIST dised_server_range_list ;
    PIPE_LIST dised_server_last_list;

    PIPE_LIST new_server_long_connect_list ;
    PIPE_LIST new_server_range_list ;
    PIPE_LIST new_server_last_list;

    PIPE_LIST dised_peer_list ;
    PIPE_LIST new_peer_list;

    PIPE_LIST partial_pipe_list;

    PIPE_LIST cadidate_list;

    MAP download_map;
    MAP partial_map;

    RANGE_LIST * dispatch_range_list = NULL;

    ERROR_BLOCKLIST* p_error_block_list = NULL;

    _u32 uncomplete_range_size = 0;
    _u32 pipe_num = 0;
    _u32 dowing_no_range_pipe_num = 0;
    _u32 new_no_range_pipe_num = 0;
    _u32 range_num_per_pipe = 0;

    _int32  cur_index =0;
    _int32  next_index =0;
    _int32  uncomplete_num =0;
    RANGE cur_range;
    MAP_ITERATOR cur_map_node = NULL;
    MAP_ITERATOR next_map_node = NULL;
    LIST_ITERATOR cur_pipe = NULL;

    BOOL no_pipe = FALSE;
    DATA_PIPE* cur_data_pipe = NULL;

    DISPATCH_ITEM* cur_dispatch_item = NULL;

    _u64 file_size = DS_GET_FILE_SIZE(ptr_dispatch_interface->_data_interface);

    RANGE full_r = pos_length_to_range(0, file_size, file_size);

    RANGE_LIST uncomplete_range_list;
    RANGE_LIST pri_range_list;
    RANGE_LIST dispatched_correct_range_list;
    RANGE_LIST undispatched_correct_range_list;
    RANGE_LIST cannotdispatched_correct_range_list;
    RANGE_LIST downing_range_list;
    RANGE_LIST tmp_range_list;

    RANGE_LIST partial_alloc_range_list;

    RANGE_LIST_ITEROATOR cur_range_node = NULL;
    RANGE_LIST_ITEROATOR cur_add_node = NULL;
    RANGE_LIST_ITEROATOR cur_index_add_node = NULL;
    RANGE_LIST_ITEROATOR cur_index_add_node2 = NULL;

    LIST_ITERATOR cur_error_block_node = NULL;
    ERROR_BLOCK*   cur_error_block = NULL;
    BOOL bool_ret = FALSE;
    _u32 over_alloc_time = 0;
    LIST connect_manager_list ;
    LIST_ITERATOR connect_manager_it = NULL;
    CONNECT_MANAGER*  cur_connect_manager = NULL;
    _u32 start_index = MAX_U32;
    BOOL is_in_checking = FALSE;
    LOG_DEBUG("ds_dispatch_at_muti_res");

    range_list_init(&uncomplete_range_list);
    range_list_init(&pri_range_list);
    range_list_init(&dispatched_correct_range_list);
    range_list_init(&undispatched_correct_range_list);
    range_list_init(&cannotdispatched_correct_range_list);
    range_list_init(&downing_range_list);
    range_list_init(&tmp_range_list);

    range_list_init(&partial_alloc_range_list);

    list_init(&dised_server_long_connect_list._data_pipe_list);
    list_init(&dised_server_range_list._data_pipe_list);
    list_init(&dised_server_last_list._data_pipe_list);
    list_init(&new_server_long_connect_list._data_pipe_list);
    list_init(&new_server_range_list._data_pipe_list);
    list_init(&new_server_last_list._data_pipe_list);
    list_init(&dised_peer_list._data_pipe_list);
    list_init(&new_peer_list._data_pipe_list);

    list_init(&cadidate_list._data_pipe_list);
    list_init(&partial_pipe_list._data_pipe_list);

    list_init(&connect_manager_list);

    LOG_DEBUG("ds_dispatch_at_muti_res, get filesize %llu", file_size);

#ifdef VVVV_VOD_DEBUG
    LOG_DEBUG("ds_dispatch_at_muti_res,  gloabal_recv_data %llu, \
        gloabal_invalid_data:%llu , gloabal_discard_data:%llu, invalid  ratio :%u , \
        task_speed:%u, vv_speed:%u.",
        gloabal_recv_data,
        gloabal_invalid_data,
        gloabal_discard_data,
        (_u32)(gloabal_recv_data==0?0:(gloabal_invalid_data+gloabal_discard_data)*100/(gloabal_invalid_data+gloabal_discard_data+gloabal_recv_data)),
        gloabal_task_speed, gloabal_vv_speed);
#endif

    /* init uncomplete map */
    init_download_map(&download_map);
    init_download_map(&partial_map);

    range_list_clear(&ptr_dispatch_interface->_tmp_range_list);
    range_list_clear(&ptr_dispatch_interface->_overalloc_range_list);
    range_list_clear(&ptr_dispatch_interface->_overalloc_in_once_dispatch);
    range_list_clear(&ptr_dispatch_interface->_cur_down_point);

    ptr_dispatch_interface->_cur_down_pipes = 0; // 正在下载数据的pipe的个数

    ptr_dispatch_interface->_start_index = MAX_U32;
    ptr_dispatch_interface->_start_index_time_ms = MAX_U32;

    DS_GET_UNCOMPLETE_RANGE(ptr_dispatch_interface->_data_interface, &uncomplete_range_list);
    dispatch_range_list = &uncomplete_range_list;

    // 过滤前32M数据让pipe去下载，
    // 防止pipe下载后面的数据导致在sdcard上创建整个文件，创建文件
    // 非常耗时，导致速度一直为0

#ifndef DISPATCH_RANDOM_MODE
    // BT任务独立处理
    if(!cm_is_bttask(ptr_dispatch_interface->_p_connect_manager))
    {
	  ds_filter_range_list_from_begin(dispatch_range_list, SECTION_UNIT*1024/16);
    }
#endif
    LOG_DEBUG("after ds_filter_range_list_from_begin, uncomplete range list : ");
    out_put_range_list(dispatch_range_list);

    // 错误块
    DS_GET_ERROR_RANGE_BLOCK_LIST(ptr_dispatch_interface->_data_interface,
        &p_error_block_list);

    // 连接管理池
    cm_get_connect_manager_list(ptr_dispatch_interface->_p_connect_manager,
        &connect_manager_list);
    if(list_size(&connect_manager_list) == 0)
    {
        LOG_DEBUG("ds_dispatch_at_muti_res, has no connect manager !");
        goto  muti_res_dis_done;
    }

    connect_manager_it = LIST_BEGIN(connect_manager_list);
    while(connect_manager_it != LIST_END(connect_manager_list))
    {
        cur_connect_manager = (CONNECT_MANAGER*)LIST_VALUE(connect_manager_it);

        /*get pipe lists*/
        cm_get_working_server_pipe_list(cur_connect_manager, &p_server_connected_list);
        cm_get_connecting_server_pipe_list(cur_connect_manager, &p_server_connecting_list);
        cm_get_working_peer_pipe_list(cur_connect_manager, &p_peer_connected_list);
        cm_get_connecting_peer_pipe_list(cur_connect_manager, &p_peer_connecting_list);

#ifdef ENABLE_CDN
        cm_get_cdn_pipe_list(cur_connect_manager, &p_cdn_pipe_list);
#endif

        /*build pipe maps*/
        ds_build_pipe_list_map(&download_map
                , &partial_map
                , p_server_connected_list   // 已经连接上的server pipe队列
                , dispatch_range_list
                , &dised_server_long_connect_list
                , &dised_server_range_list
                , &dised_server_last_list
                , &new_server_long_connect_list
                , &new_server_range_list
                , &new_server_last_list
                , &partial_pipe_list
                , &dispatched_correct_range_list
                , &downing_range_list
                , 0);

        ds_build_pipe_list_map(&download_map
                , &partial_map
                , p_server_connecting_list  // 正在连接的server pipe队列
                , dispatch_range_list
                , &dised_server_long_connect_list
                , &dised_server_range_list
                , &dised_server_last_list
                , &new_server_long_connect_list
                , &new_server_range_list
                , &new_server_last_list
                , &partial_pipe_list
                , &dispatched_correct_range_list
                , &downing_range_list
                , 0);

        ds_build_pipe_list_map(&download_map
                , &partial_map
                , p_peer_connected_list
                , dispatch_range_list
                , &dised_peer_list
                , NULL
                , NULL
                , &new_peer_list
                , NULL
                , NULL
                , &partial_pipe_list
                , &dispatched_correct_range_list
                , &downing_range_list
                , 0);

        ds_build_pipe_list_map(&download_map
                , &partial_map
                , p_peer_connecting_list
                , dispatch_range_list
                , &dised_peer_list
                , NULL
                , NULL
                , &new_peer_list
                , NULL
                , NULL
                , &partial_pipe_list
                , &dispatched_correct_range_list
                , &downing_range_list
                , 0);

        ds_build_pipe_list_map(&download_map
                , &partial_map
                , p_cdn_pipe_list
                , dispatch_range_list
                , &dised_peer_list
                , NULL
                , NULL
                , &new_peer_list
                , NULL
                , NULL
                , &partial_pipe_list
                , &dispatched_correct_range_list
                , &downing_range_list, 0);


        p_server_connected_list = NULL;
        p_server_connecting_list = NULL;
        p_peer_connected_list = NULL;
        p_peer_connecting_list = NULL;
        p_cdn_pipe_list = NULL;

        connect_manager_it = LIST_NEXT(connect_manager_it);
    }

    if(dispatch_range_list->_node_size == 0)
    {
        LOG_DEBUG("ds_dispatch_at_muti_res, has no uncomplete range so return !");

        goto  muti_res_dis_done;
    }


    LOG_DEBUG("ds_dispatch_at_muti_res, get %u downing server data pipe",
        list_size(&p_server_connected_list->_data_pipe_list));
    LOG_DEBUG("ds_dispatch_at_muti_res, get %u conecting server data pipe",
        list_size(&p_server_connecting_list->_data_pipe_list));
    LOG_DEBUG("ds_dispatch_at_muti_res, get %u downing peer data pipe",
        list_size(&p_peer_connected_list->_data_pipe_list));
    LOG_DEBUG("ds_dispatch_at_muti_res, get %u connecting peer data pipe",
        list_size(&p_peer_connecting_list->_data_pipe_list));
    LOG_DEBUG("========================================================================");
    LOG_DEBUG("ds_dispatch_at_muti_res, get %u downing long server data pipe", list_size(&dised_server_long_connect_list._data_pipe_list));
    LOG_DEBUG("ds_dispatch_at_muti_res, get %u downing range server data pipe", list_size(&dised_server_range_list._data_pipe_list));
    LOG_DEBUG("ds_dispatch_at_muti_res, get %u downing last server data pipe", list_size(&dised_server_last_list._data_pipe_list));
    LOG_DEBUG("ds_dispatch_at_muti_res, get %u connecting long server data pipe", list_size(&new_server_long_connect_list._data_pipe_list));
    LOG_DEBUG("ds_dispatch_at_muti_res, get %u connecting range server data pipe", list_size(&new_server_range_list._data_pipe_list));
    LOG_DEBUG("ds_dispatch_at_muti_res, get %u connecting last server data pipe", list_size(&new_server_last_list._data_pipe_list));
    LOG_DEBUG("ds_dispatch_at_muti_res, get %u downing peer data pipe", list_size(&dised_peer_list._data_pipe_list));
    LOG_DEBUG("ds_dispatch_at_muti_res, get %u connecting peer data pipe", list_size(&new_peer_list._data_pipe_list));

    LOG_DEBUG("ds_dispatch_at_muti_res, get %u partial_pipe_list data pipe", list_size(&partial_pipe_list._data_pipe_list));


    if( range_list_size(&dispatched_correct_range_list) > 0 )
    {
        is_in_checking = TRUE;
    }
    /*assign error range to pipe*/
    if(p_error_block_list != NULL
       && list_size(&p_error_block_list->_error_block_list) != 0)
    {
        LOG_DEBUG("ds_dispatch_at_muti_res, get %u error blocks.",
            list_size(&p_error_block_list->_error_block_list));

        cur_error_block_node = LIST_BEGIN(p_error_block_list->_error_block_list);
        while(cur_error_block_node != LIST_END(p_error_block_list->_error_block_list))
        {
            cur_error_block = (ERROR_BLOCK*)LIST_VALUE(cur_error_block_node);

            LOG_DEBUG("ds_dispatch_at_muti_res, process error block, (%u,%u), \
                      level: %u, error times: %u ."
                      ,cur_error_block->_r._index ,cur_error_block->_r._num,
                      cur_error_block->_valid_resources, cur_error_block->_error_count);

            if(range_list_is_include(&dispatched_correct_range_list, &cur_error_block->_r) == FALSE
                && range_list_is_relevant(dispatch_range_list, &cur_error_block->_r) == TRUE
                && !is_in_checking )
            {
                // 只使用原始资源进行纠错
                if(cur_error_block->_valid_resources == 2)
                {
                    bool_ret = ds_handle_correct_range_using_origin_res(ptr_dispatch_interface
                            , cur_error_block
                            , &new_server_long_connect_list
                            , &new_server_range_list
                            , NULL
                            , &new_peer_list
                            , NULL);
                    if(bool_ret == TRUE)
                    {
                        LOG_DEBUG("ds_dispatch_at_muti_res, process error block, (%u,%u),  level: %u, error times: %u  assign to connecting data pipe.", cur_error_block->_r._index,  cur_error_block->_r._num,
                                  cur_error_block->_valid_resources, cur_error_block->_error_count);
                        range_list_add_range(&dispatched_correct_range_list
                            , &cur_error_block->_r
                            , NULL
                            , NULL);

                    }
                    else
                    {
                        bool_ret = ds_handle_correct_range_using_origin_res(ptr_dispatch_interface
                                , cur_error_block
                                , &dised_server_long_connect_list
                                , &dised_server_range_list
                                , NULL
                                , &dised_peer_list
                                , &download_map);
                        if(bool_ret == TRUE)
                        {
                            LOG_DEBUG("ds_dispatch_at_muti_res, process error block, \
                                (%u,%u),  level: %u, error times: %u  \
                                assign to connected data pipe.",
                                cur_error_block->_r._index,  cur_error_block->_r._num,
                                cur_error_block->_valid_resources,
                                cur_error_block->_error_count);

                            range_list_add_range(&undispatched_correct_range_list,
                                &cur_error_block->_r, NULL, NULL);
                        }
                        else
                        {
                            LOG_ERROR("ds_dispatch_at_muti_res, process error block,\
                                (%u,%u),  level: %u, error times: %u  error.",
                                cur_error_block->_r._index,  cur_error_block->_r._num,
                                cur_error_block->_valid_resources,
                                cur_error_block->_error_count);

                            range_list_add_range(&cannotdispatched_correct_range_list,
                                &cur_error_block->_r, NULL, NULL);
                        }
                    }
                }
                else
                {
                    bool_ret = ds_handle_correct_range(cur_error_block
                        , &new_server_long_connect_list
                        , &new_server_range_list
                        , NULL
                        , &new_peer_list
                        , NULL);
                    if(bool_ret == TRUE)
                    {
                        LOG_DEBUG("ds_dispatch_at_muti_res, process error block, (%u,%u),  level: %u, error times: %u  assign to connecting data pipe.", cur_error_block->_r._index,  cur_error_block->_r._num,
                                  cur_error_block->_valid_resources, cur_error_block->_error_count);
                        range_list_add_range(&dispatched_correct_range_list
                            , &cur_error_block->_r
                            , NULL
                            , NULL);
                    }
                    else
                    {
                        bool_ret = ds_handle_correct_range(cur_error_block
                            , &dised_server_long_connect_list
                            , &dised_server_range_list
                            , NULL
                            , &dised_peer_list
                            , &download_map);
                        if(bool_ret == TRUE)
                        {
                            LOG_DEBUG("ds_dispatch_at_muti_res, process error block, (%u,%u),  level: %u, error times: %u  assign to connected data pipe.", cur_error_block->_r._index,  cur_error_block->_r._num,
                                      cur_error_block->_valid_resources, cur_error_block->_error_count);
                            range_list_add_range(&dispatched_correct_range_list
                                , &cur_error_block->_r
                                , NULL
                                , NULL);
                        }
                        else
                        {
                            bool_ret = ds_handle_correct_range(cur_error_block
                                    , &partial_pipe_list
                                    , NULL
                                    , NULL
                                    , NULL
                                    , NULL);
                            if(bool_ret == TRUE)
                            {
                                LOG_DEBUG("ds_dispatch_at_muti_res, process error block, (%u,%u),  level: %u, error times: %u  assign to partial_pipe_list data pipe.", cur_error_block->_r._index,  cur_error_block->_r._num,
                                          cur_error_block->_valid_resources, cur_error_block->_error_count);
                                range_list_add_range(&dispatched_correct_range_list
                                        , &cur_error_block->_r
                                        , NULL
                                        , NULL);
                            }
                            else
                            {
                                LOG_ERROR("ds_dispatch_at_muti_res, process error block, (%u,%u),  level: %u, error times: %u  error.", cur_error_block->_r._index,  cur_error_block->_r._num,
                                          cur_error_block->_valid_resources, cur_error_block->_error_count);

                                range_list_add_range(&cannotdispatched_correct_range_list
                                        , &cur_error_block->_r
                                        , NULL
                                        , NULL);

                            }
                        }
                    }
                }
                if(bool_ret)
                {
                    is_in_checking = TRUE;
                }
            }
            else
            {
                LOG_DEBUG("ds_dispatch_at_muti_res, error block, (%u,%u),  level: %u, error times: %u  has assgin before or has downed!.", cur_error_block->_r._index,  cur_error_block->_r._num,
                          cur_error_block->_valid_resources, cur_error_block->_error_count);

                range_list_add_range(&cannotdispatched_correct_range_list
                                        , &cur_error_block->_r
                                        , NULL
                                        , NULL);
            }

            cur_error_block_node = LIST_NEXT(cur_error_block_node);
        }
    }


    if(dispatched_correct_range_list._node_size != 0)
    {
        LOG_DEBUG("ds_dispatch_at_muti_res ,  dispatched_correct_range_list:.");
        out_put_range_list(&dispatched_correct_range_list);

        range_list_delete_range_list(dispatch_range_list, &dispatched_correct_range_list);

        LOG_DEBUG("ds_dispatch_at_muti_res , after del dispatched_correct_range_list , un_complete_range_list:.");
        out_put_range_list(dispatch_range_list);
    }

    if(cannotdispatched_correct_range_list._node_size != 0)
    {
        LOG_DEBUG("ds_dispatch_at_muti_res ,  cannotdispatched_correct_range_list.");
        out_put_range_list(&cannotdispatched_correct_range_list);

        /*目前没有纠错块正在纠错，所以cannotdispatched_correct_range_list 中是没
              //有资源可以纠错的块，这些块需要重新下载一次导致纠错失败。*/
        if( dispatched_correct_range_list._node_size == 0 )
        {
            LOG_DEBUG("ds_dispatch_at_muti_res , only  cannotdispatched_correct_range_list so set cannot correct error range.");
            range_list_clear(&cannotdispatched_correct_range_list);
        }
        else
        {
            range_list_delete_range_list(dispatch_range_list, &cannotdispatched_correct_range_list);

            LOG_DEBUG("ds_dispatch_at_muti_res , after del cannotdispatched_correct_range_list , un_complete_range_list:.");
            out_put_range_list(dispatch_range_list);
        }
    }

    if(dispatch_range_list->_node_size == 0)
    {
        LOG_DEBUG("ds_dispatch_at_muti_res, 2  has no uncomplete range so return !");

        goto  overlap_alloc;
    }

    // 开始下载未完成块
    range_list_get_head_node(dispatch_range_list, &cur_range_node);
    if(cur_range_node != NULL)
    {
        start_index = cur_range_node->_range._index;
        cur_range_node = NULL;
    }

    // 重用这个局部pipe
    if(list_size(&partial_pipe_list._data_pipe_list) != 0)
    {
        LOG_DEBUG("ds_dispatch_at_muti_res assign partial pipe.");
        cur_data_pipe = NULL;
        cur_pipe = LIST_BEGIN(partial_pipe_list._data_pipe_list);

        while(cur_pipe != LIST_END(partial_pipe_list._data_pipe_list))
        {
            cur_data_pipe = (DATA_PIPE*)LIST_VALUE(cur_pipe);

            get_range_list_overlap_list(&cur_data_pipe->_dispatch_info._can_download_ranges
                    , dispatch_range_list
                    , &tmp_range_list);

            // 这个pipe没有我们需要的range
            if(tmp_range_list._node_size == 0)
            {
                LOG_DEBUG("ds_dispatch_at_muti_res, data pipe :0x%x  \
                    can not down uncomplete range.", cur_data_pipe);
                cur_pipe = LIST_NEXT(cur_pipe);
                continue;
            }

            // partial_alloc_range_list 没作用
            range_list_delete_range_list(&tmp_range_list, &partial_alloc_range_list);

            // 这个也没有意义
            if(tmp_range_list._node_size == 0)
            {
                get_range_list_overlap_list(
                    &cur_data_pipe->_dispatch_info._can_download_ranges,
                    dispatch_range_list, &tmp_range_list);
            }

            range_list_get_max_node(&tmp_range_list, &cur_range_node);
            if(cur_range_node == NULL)
            {
                LOG_ERROR("ds_dispatch_at_muti_res,  ptr_data_pipe: 0x%x , \
                    2 can not get rand range node of uncomplete_range_list, so return.",
                    cur_data_pipe);
                cur_pipe = LIST_NEXT(cur_pipe);
                continue;
            }

            cur_range._num = cur_range_node->_range._num/2;
            if(cur_range._num < 1)
            {
                cur_range._index = cur_range_node->_range._index ;
                cur_range._num = cur_range_node->_range._num ;
            }
            else
            {
                cur_range._index
                    = cur_range_node->_range._index + cur_range_node->_range._num - cur_range_node->_range._num/2;
                cur_range._num = cur_range_node->_range._num/2;
            }

            if(cur_range._num != 0
                && ds_assigned_range_to_pipe(cur_data_pipe, &cur_range) == TRUE)
            {
                LOG_DEBUG("ds_dispatch_at_muti_res,  \
                          2 assign range (%u, %u) to data pipe: 0x%x",
                          cur_range._index, cur_range._num, cur_data_pipe);
                range_list_add_range(&downing_range_list, &cur_range, NULL, NULL);
                cur_range._num = 1;
                range_list_add_range(&ptr_dispatch_interface->_tmp_range_list,
                    &cur_range, NULL, NULL);
                if(cur_range._index == start_index)
                {
                    ptr_dispatch_interface ->_start_index = start_index;
                    sd_time_ms(&ptr_dispatch_interface->_start_index_time_ms);
                }
            }
            else
            {
                LOG_DEBUG("ds_dispatch_at_muti_res,  failure to assign range (%u, %u) \
                          to data pipe: 0x%x", cur_range._index, cur_range._num, cur_data_pipe);
            }
            cur_pipe = LIST_NEXT(cur_pipe);
        }

        range_list_clear(&tmp_range_list);

        LOG_DEBUG("ds_dispatch_at_muti_res assign partial pipe end.");
    }

    uncomplete_range_size = range_list_get_total_num(dispatch_range_list);

    pipe_num = list_size(&dised_server_long_connect_list._data_pipe_list);
    pipe_num += list_size(&dised_server_range_list._data_pipe_list);
    pipe_num += list_size(&new_server_long_connect_list._data_pipe_list);
    pipe_num += list_size(&new_server_range_list._data_pipe_list);
    pipe_num += list_size(&dised_peer_list._data_pipe_list);
    pipe_num += list_size(&new_peer_list._data_pipe_list);

    dowing_no_range_pipe_num =  list_size(&dised_server_last_list._data_pipe_list);
    new_no_range_pipe_num =  list_size(&new_server_last_list._data_pipe_list);

    if(pipe_num == 0 && new_no_range_pipe_num == 0)
    {
        LOG_DEBUG("ds_dispatch_at_muti_res, has no pipes, so return !");

        goto  muti_res_dis_done;
    }

    // 不支持range的pipe
    if( dowing_no_range_pipe_num == 0
        && new_no_range_pipe_num != 0
        && pipe_num <= MIN_PIPE_NUM )
    {
        list_pop(&new_server_last_list._data_pipe_list, (void**)&cur_data_pipe);
        if(cur_data_pipe  !=  NULL)
        {
            if(ds_assigned_range_to_pipe(cur_data_pipe, &full_r) == TRUE)
            {
                LOG_DEBUG("ds_dispatch_at_muti_res , success assign range (%u, %u) \
                          to no range data pipe: 0x%x, so from %u start assign.", \
                          full_r._index, full_r._num, cur_data_pipe, start_index);
            }
            else
            {
                LOG_ERROR("ds_dispatch_at_muti_res ,failure to assign range (%u, %u) \
                          to no range data pipe: 0x%x .",
                          full_r._index, full_r._num, cur_data_pipe);
            }
        }
    }

    ds_adjust_pipe_list(&new_server_long_connect_list);
    ds_adjust_pipe_list(&new_server_range_list);
    ds_adjust_pipe_list(&new_peer_list);

    /* build uncomplete map */
    ds_build_uncomplete_map(&download_map, dispatch_range_list);

    range_num_per_pipe = (uncomplete_range_size )/(pipe_num );

    if(range_num_per_pipe <= MIN_DISPATCH_BLOCK_NUM)
        range_num_per_pipe = MIN_DISPATCH_BLOCK_NUM;


    LOG_DEBUG("ds_dispatch_at_muti_res, uncomplete_range_size: %u, pipe_num: %u,  \
        range_num_per_pipe: %u", uncomplete_range_size, pipe_num, range_num_per_pipe);


    cur_add_node = NULL;
    cur_index_add_node = NULL;
    cur_index_add_node2 = NULL;
    cur_map_node = MAP_BEGIN(download_map);

    while(cur_map_node != MAP_END(download_map))
    {
        cur_index =  (_u32)MAP_KEY(cur_map_node);
        cur_dispatch_item = (DISPATCH_ITEM* )MAP_VALUE(cur_map_node);

        next_map_node = MAP_NEXT(download_map, cur_map_node);
        if(next_map_node != MAP_END(download_map))
        {
            next_index = (_u32)MAP_KEY(next_map_node);
        }
        else
        {
            next_index = full_r._index+full_r._num;
        }

        LOG_DEBUG("ds_dispatch_at_muti_res, cur_index:%u, next_index:%u, is_complete:%u", \
            cur_index, next_index, cur_dispatch_item->_is_completed);

        // 已经分发完成
        if(cur_dispatch_item->_is_completed == TRUE)
        {
            if(cur_dispatch_item ->_is_pipe_alloc == TRUE)
            {
                if(ptr_dispatch_interface ->_start_index == MAX_U32
                    && cur_index == start_index)
                {
                    ptr_dispatch_interface ->_start_index = start_index;
                    sd_time_ms(&ptr_dispatch_interface->_start_index_time_ms);
                }

                cur_range._index = cur_index;
                cur_range._num = 1;
                range_list_add_range(&ptr_dispatch_interface->_tmp_range_list,
                    &cur_range, cur_index_add_node, &cur_index_add_node);
                if(cur_dispatch_item->_alloc_times > MAX_OVER_ALLOC_TIMES)
                {
                    range_list_add_range(&ptr_dispatch_interface->_overalloc_range_list,
                        &cur_range, cur_index_add_node2, &cur_index_add_node2);
                }
            }

            //如果当前块已经由某个pipe在下载了，就不让其他pipe重复下载
            if(list_size(&cur_dispatch_item->_owned_pipes) != 0)
            {
                cur_pipe = LIST_BEGIN(cur_dispatch_item->_owned_pipes);

                while(cur_pipe != LIST_END(cur_dispatch_item->_owned_pipes))
                {
                    cur_data_pipe = (DATA_PIPE*)LIST_VALUE(cur_pipe);
                    range_list_clear(&cur_data_pipe->_dispatch_info._uncomplete_ranges);
                    LOG_DEBUG("ds_dispatch_at_muti_res , cur_index:%u , next_index: %u, \
                        cur_data_pipe:0x%x 's uncomplete range list need clear.",
                        cur_index,next_index, cur_data_pipe);
                    cur_pipe = LIST_NEXT(cur_pipe);
                }

            }

            cur_map_node = next_map_node;
            continue;
        }

        // 下载本块的pipe数量不等于0
        if(list_size(&cur_dispatch_item->_owned_pipes) != 0)
        {
            LOG_DEBUG("ds_dispatch_at_muti_res , cur_index:%u , next_index: %u, owned_pipes: %u, per_pipe_range_num: %u",
                cur_index,next_index, list_size(&cur_dispatch_item->_owned_pipes), range_num_per_pipe);

            cur_pipe = LIST_BEGIN(cur_dispatch_item->_owned_pipes);
            while(cur_pipe != LIST_END(cur_dispatch_item->_owned_pipes))
            {
                cur_data_pipe = (DATA_PIPE*)LIST_VALUE(cur_pipe);
                // 如果有纠错块
                if(cur_data_pipe->_dispatch_info._correct_error_range._num != 0)
                {
                    /*process last time assigned range */
                    LOG_DEBUG("ds_dispatch_at_muti_res , cur_index:%u , \
                              next_index: %u, cur_data_pipe:0x%x, is correct pipe.",
                              cur_index, next_index, cur_data_pipe);
                    if((cur_data_pipe->_dispatch_info._down_range._index
                        + cur_data_pipe->_dispatch_info._down_range._num)
                        < (cur_data_pipe->_dispatch_info._correct_error_range._index
                        + cur_data_pipe->_dispatch_info._correct_error_range._num))
                    {
                        // 去下纠错块后面的部分
                        cur_range._index
                            = cur_data_pipe->_dispatch_info._down_range._index
                            + cur_data_pipe->_dispatch_info._down_range._num;
                        cur_range._num
                            = (cur_data_pipe->_dispatch_info._correct_error_range._index
                            + cur_data_pipe->_dispatch_info._correct_error_range._num)
                            - (cur_data_pipe->_dispatch_info._down_range._index
                            + cur_data_pipe->_dispatch_info._down_range._num);
                        if(ds_assigned_range_to_pipe(cur_data_pipe, &cur_range) == TRUE)
                        {
                            LOG_DEBUG("ds_dispatch_at_muti_res , cur_index:%u , \
                                      next_index: %u, cur_data_pipe:0x%x, is correct pipe, \
                                      so assign correct range (%u, %u).",
                                      cur_index, next_index, cur_data_pipe,
                                      cur_range._index, cur_range._num);
                            cur_index
                                = cur_data_pipe->_dispatch_info._correct_error_range._index
                                + cur_data_pipe->_dispatch_info._correct_error_range._num;
                            cur_pipe = LIST_NEXT(cur_pipe);
                            continue;
                        }
                        else
                        {
                            LOG_ERROR("ds_dispatch_at_muti_res , cur_index:%u , \
                                      next_index: %u, cur_data_pipe:0x%x, is correct pipe, \
                                      assign correct range (%u, %u) failue so clear correct \
                                      range.",
                                      cur_index, next_index, cur_data_pipe,
                                      cur_range._index, cur_range._num);
                            cur_data_pipe->_dispatch_info._correct_error_range._index =0 ;
                            cur_data_pipe->_dispatch_info._correct_error_range._num =0 ;
                        }
                    }
                    else
                    {
                        LOG_ERROR("ds_dispatch_at_muti_res , cur_index:%u , next_index: %u, cur_data_pipe:0x%x, is correct pipe, downing range (%u,%u) is not in the assign correct range (%u, %u)  so clear correct range.",
                            cur_index,next_index, cur_data_pipe, cur_data_pipe->_dispatch_info._down_range._index, cur_data_pipe->_dispatch_info._down_range._num,
                            cur_data_pipe->_dispatch_info._correct_error_range._index, cur_data_pipe->_dispatch_info._correct_error_range._num);
                        cur_data_pipe->_dispatch_info._correct_error_range._index =0 ;
                        cur_data_pipe->_dispatch_info._correct_error_range._num =0 ;
                    }
                }

                // 针对需要重新分配的pipe，现在的逻辑是按pipe数量平分，如果pipe从少到多，这个cur_range
                // 会越来越小。应该根据速度来分配。
                if(cur_index + range_num_per_pipe < next_index)
                {
                    cur_range._index = cur_index;
                    cur_range._num = range_num_per_pipe;
                    if(ds_assigned_range_to_pipe(cur_data_pipe, &cur_range) == TRUE)
                    {
                        LOG_DEBUG("ds_dispatch_at_muti_res , 1 assign range (%u, %u) \
                                  to data pipe: 0x%x",
                                  cur_range._index, cur_range._num, cur_data_pipe);
                        cur_index += range_num_per_pipe;
                        if(cur_data_pipe->_dispatch_info._down_range._num < range_num_per_pipe)
                        {
                            range_list_add_range(&downing_range_list,
                                &cur_range, cur_add_node, &cur_add_node);
                        }
                    }
                    else
                    {
                        LOG_ERROR("ds_dispatch_at_muti_res ,1 failure to assign range \
                                  (%u, %u) to data pipe: 0x%x",
                                  cur_range._index, cur_range._num, cur_data_pipe);
                    }
                }
                else if(cur_index < next_index)
                {
                    cur_range._index = cur_index;
                    cur_range._num = next_index - cur_index;
                    cur_index = next_index;
                    if(ds_assigned_range_to_pipe(cur_data_pipe, &cur_range) == TRUE)
                    {
                        LOG_DEBUG("ds_dispatch_at_muti_res , 2 assign range (%u, %u) \
                                  to data pipe: 0x%x",
                                  cur_range._index, cur_range._num, cur_data_pipe);
                        range_list_add_range(&downing_range_list, &cur_range,
                            cur_add_node, &cur_add_node);
                    }
                    else
                    {
                        LOG_ERROR("ds_dispatch_at_muti_res , 2 failure to assign range \
                                  (%u, %u) to data pipe: 0x%x",
                                  cur_range._index, cur_range._num, cur_data_pipe);
                    }
                }
                else
                {
                    LOG_DEBUG("ds_dispatch_at_muti_res , because assign finished so put  \
                              data pipe: 0x%x",  cur_data_pipe);
                }
                cur_pipe = LIST_NEXT(cur_pipe);
            }
        }
        cur_map_node = next_map_node;
    }


    // alloc  can down full pipes

    LOG_DEBUG("ds_dispatch_at_muti_res, alloc full pipes");

    cur_add_node = NULL;
    cur_index_add_node = NULL;
    no_pipe= FALSE;

    out_put_range_list(dispatch_range_list);

    out_put_range_list(&downing_range_list);

    range_list_delete_range_list(dispatch_range_list, &downing_range_list);

    out_put_range_list(dispatch_range_list);
    // 把未完成的第一块分给一个合适的pipe
    if(dispatch_range_list->_node_size != 0)
    {
        range_list_get_head_node(dispatch_range_list, &cur_range_node);
        while(cur_range_node != NULL)
        {
            cur_index = cur_range_node->_range._index;
            next_index =cur_range_node->_range._index + cur_range_node->_range._num;
            while(cur_index < next_index)
            {
                if(cur_index + range_num_per_pipe< next_index)
                {
                    cur_range._index = cur_index;
                    cur_range._num = range_num_per_pipe;
                }
                else
                {
                    cur_range._index = cur_index;
                    cur_range._num = next_index - cur_index;
                }
                cur_data_pipe
                    = ds_get_data_pipe(&new_server_long_connect_list,
                    &new_server_range_list, NULL, &new_peer_list);
                if(cur_data_pipe == NULL)
                {
                    LOG_DEBUG("ds_dispatch_at_muti_res , can not get data pipe to \
                              assign range (%u, %u) .", cur_range._index, cur_range._num);
                    no_pipe= TRUE;
                    uncomplete_num += cur_range._num;
                    break;
                }
                if(ds_assigned_range_to_pipe(cur_data_pipe, &cur_range) == FALSE)
                {
                    LOG_ERROR("ds_dispatch_at_muti_res , 3 failure to assign range \
                              (%u, %u) to data pipe: 0x%x",
                              cur_range._index, cur_range._num, cur_data_pipe);
                    continue;
                }
                else
                {
                    cur_index += cur_range._num;
                    LOG_DEBUG("ds_dispatch_at_muti_res , \
                              3 success to assign range (%u, %u) to data pipe: 0x%x",
                              cur_range._index, cur_range._num, cur_data_pipe);
                    range_list_add_range(&downing_range_list, &cur_range,
                        cur_add_node, &cur_add_node);
                    if(ptr_dispatch_interface ->_start_index == MAX_U32
                        && cur_index == start_index)
                    {
                        ptr_dispatch_interface ->_start_index = start_index;
                        sd_time_ms(&ptr_dispatch_interface->_start_index_time_ms);
                    }
                    cur_range._num = 1;
                    range_list_add_range(&ptr_dispatch_interface->_tmp_range_list,
                        &cur_range, cur_index_add_node, &cur_index_add_node);
                }
                if(no_pipe == TRUE)
                {
                    LOG_DEBUG("ds_dispatch_at_muti_res , no pipes, \
                              cur_index:%u, next_index:%u, cur_assign_range (%u, %u) !",
                              cur_index, next_index, cur_range._index, cur_range._num);
                }

            }
            if(no_pipe == TRUE)
            {
                LOG_DEBUG("ds_dispatch_at_muti_res , no pipes, so break !");
                break;
            }
            range_list_get_next_node(dispatch_range_list, cur_range_node, &cur_range_node);
        }
    }

overlap_alloc:

    LOG_DEBUG("ds_dispatch_at_muti_res , finish process cur_index: %u, next_index: %u.",
        cur_index, next_index);

    if(list_size(&new_server_long_connect_list._data_pipe_list) != 0
       || list_size(&new_server_range_list._data_pipe_list) != 0
       || list_size(&new_peer_list._data_pipe_list) != 0 )
    {
        over_alloc_time = 0;
        do
        {
            out_put_range_list(&downing_range_list);
            out_put_range_list(& ptr_dispatch_interface->_tmp_range_list);

            range_list_delete_range_list(&downing_range_list,
                &ptr_dispatch_interface->_tmp_range_list);
            if(downing_range_list._node_size != 0)
            {
                LOG_DEBUG("ds_dispatch_at_muti_res , because after assign, there is also pipes , so  assign dowing ranges.");

                dispatch_range_list = &downing_range_list;
            }
            else
            {
                // dispatch_range_list = &partial_alloc_range_list;
                range_list_delete_range_list(&ptr_dispatch_interface->_tmp_range_list, & ptr_dispatch_interface->_overalloc_range_list);

                dispatch_range_list = &ptr_dispatch_interface->_tmp_range_list;
            }

            out_put_range_list(dispatch_range_list);

            LOG_DEBUG("ds_dispatch_at_muti_res , because after assign, there is also pipes , so  assign dowing ranges.");

            uncomplete_range_size = range_list_get_total_num(dispatch_range_list);

            pipe_num = list_size(&new_server_long_connect_list._data_pipe_list);
            pipe_num += list_size(&new_server_range_list._data_pipe_list);
            pipe_num += list_size(&new_peer_list._data_pipe_list);

            if(pipe_num == 0 || uncomplete_range_size == 0 || over_alloc_time >= MAX_OVER_ALLOC_TIMES)
            {
                break;
            }

            //range_num_per_pipe = uncomplete_range_size/(pipe_num + dispatch_range_list->_node_size);
            range_num_per_pipe = uncomplete_range_size/(pipe_num + range_list_get_total_num(&ptr_dispatch_interface->_tmp_range_list));
            if(range_num_per_pipe <= MIN_DISPATCH_BLOCK_NUM)
                range_num_per_pipe = MIN_DISPATCH_BLOCK_NUM;

            cur_index_add_node = NULL;
            range_list_get_head_node(dispatch_range_list, &cur_range_node);
            while(cur_range_node != NULL)
            {
                if(no_pipe == TRUE)
                    break;
                cur_index = cur_range_node->_range._index;
                next_index =cur_range_node->_range._index + cur_range_node->_range._num;
                if(cur_range_node->_range._num > range_num_per_pipe)
                {
                    cur_index =   cur_index + range_num_per_pipe;
                }

                while(cur_index < next_index)
                {
                    if(cur_index + range_num_per_pipe < next_index)
                    {
                        cur_range._index = cur_index;
                        cur_range._num = range_num_per_pipe;
                    }
                    else
                    {
                        cur_range._index = cur_index;
                        cur_range._num = next_index - cur_index;
                    }

                    cur_data_pipe = ds_get_data_pipe(&new_server_long_connect_list,&new_server_range_list,NULL, &new_peer_list);
                    if(cur_data_pipe == NULL)
                    {
                        LOG_DEBUG("ds_dispatch_at_muti_res dowing assign , can not get data pipe to assign dowing range (%u, %u) .", cur_range_node->_range._index,  cur_range_node->_range._num);
                        no_pipe= TRUE;
                        break;
                    }
                    if(ds_assigned_range_to_pipe(cur_data_pipe, &cur_range) == FALSE)
                    {
                        // ds_put_data_pipe(cur_data_pipe,&new_server_long_connect_list,&new_server_range_list,&new_server_last_list, &new_peer_list);
                        LOG_ERROR("ds_dispatch_at_muti_res dowing assign ,  failure to assign dowing range (%u,%u)  of  (%u, %u) to data pipe: 0x%x", cur_range._index, cur_range._num, cur_range_node->_range._index, cur_range_node->_range._num, cur_data_pipe);
                        continue;
                    }
                    else
                    {
                        LOG_DEBUG("ds_dispatch_at_muti_res dowing assign ,  success to assign dowing range (%u,%u)  of  (%u, %u) to data pipe: 0x%x", cur_range._index, cur_range._num, cur_range_node->_range._index, cur_range_node->_range._num, cur_data_pipe);
                        cur_index +=   cur_range._num;
                        cur_range._num = 1;
                        //range_list_add_range(&partial_alloc_range_list, &cur_range, cur_index_add_node, &cur_index_add_node);
                        range_list_add_range(&ptr_dispatch_interface->_tmp_range_list, &cur_range, cur_index_add_node, &cur_index_add_node);
                    }
                }


                range_list_get_next_node(dispatch_range_list, cur_range_node, &cur_range_node);
            }

            over_alloc_time++;
        }
        while(no_pipe == FALSE);
    }
    else
    {
        LOG_DEBUG("ds_dispatch_at_muti_res , because no pipe, so can not assign dowing ranges.");

        if(uncomplete_num*2 > uncomplete_range_size
            && list_size(&new_server_last_list._data_pipe_list) != 0)
        {
            LOG_DEBUG("ds_dispatch_at_muti_res , because uncomplete num %u is too large uncomplet range %u ,so assign no range pipe.", uncomplete_num, uncomplete_range_size);

            list_pop(&new_server_last_list._data_pipe_list, (void**)&cur_data_pipe);
            if(cur_data_pipe  !=  NULL)
            {
                if(ds_assigned_range_to_pipe(cur_data_pipe, &full_r) == TRUE)
                {
                    //start_index = range_num_per_pipe;
                    LOG_DEBUG("ds_dispatch_at_muti_res , 2 success assign range (%u, %u) to no range data pipe: 0x%x, so from %u start assign.", cur_range._index, cur_range._num, cur_data_pipe, start_index);
                }
                else
                {
                    LOG_ERROR("ds_dispatch_at_muti_res ,2 failure to assign range (%u, %u) to no range data pipe: 0x%x .", cur_range._index, cur_range._num, cur_data_pipe);
                }
            }
        }
    }


muti_res_dis_done:

    LOG_DEBUG("ds_dispatch_at_muti_res end .");

    unit_download_map(&download_map);
    unit_download_map(&partial_map);

    range_list_clear(&uncomplete_range_list);
    range_list_clear(&pri_range_list);
    range_list_clear(&dispatched_correct_range_list);
    range_list_clear(&undispatched_correct_range_list);
    range_list_clear(&cannotdispatched_correct_range_list);
    range_list_clear(&downing_range_list);
    range_list_clear(&tmp_range_list);
    range_list_clear(&partial_alloc_range_list);


    clear_pipe_list( &dised_server_long_connect_list._data_pipe_list) ;
    clear_pipe_list( &dised_server_range_list._data_pipe_list);
    clear_pipe_list( &dised_server_last_list._data_pipe_list);
    clear_pipe_list( &new_server_long_connect_list._data_pipe_list) ;
    clear_pipe_list( &new_server_range_list._data_pipe_list);
    clear_pipe_list( &new_server_last_list._data_pipe_list);
    clear_pipe_list( &dised_peer_list._data_pipe_list) ;
    clear_pipe_list( &new_peer_list._data_pipe_list);
    clear_pipe_list( &cadidate_list._data_pipe_list);
    clear_pipe_list( &partial_pipe_list._data_pipe_list);

    list_clear(&connect_manager_list);

    return SUCCESS;
}

_int32 ds_dispatch_at_vod(DISPATCHER*  ptr_dispatch_interface)
{
    PIPE_LIST* p_server_connected_list = NULL;
    PIPE_LIST* p_server_connecting_list = NULL;
    PIPE_LIST* p_peer_connected_list = NULL;
    PIPE_LIST* p_peer_connecting_list = NULL;
    PIPE_LIST* p_cdn_pipe_list = NULL;
    PIPE_LIST dised_server_long_connect_list;
    PIPE_LIST dised_server_range_list;
    PIPE_LIST dised_server_last_list;
    PIPE_LIST new_server_long_connect_list;
    PIPE_LIST new_server_range_list;
    PIPE_LIST new_server_last_list;
    PIPE_LIST dised_peer_list;
    PIPE_LIST new_peer_list;
    PIPE_LIST partial_pipe_list;
    PIPE_LIST cadidate_list;
    MAP download_map;
    MAP partial_map;
    RANGE_LIST * dispatch_range_list = NULL;
    ERROR_BLOCKLIST* p_error_block_list = NULL;
    _u32 uncomplete_range_size = 0;
    //  _u32 pri_range_size = 0;
    _u32 pipe_num = 0;
    _u32 dowing_no_range_pipe_num = 0;
    _u32 new_no_range_pipe_num = 0;
    _u32 range_num_per_pipe = 0;
    _int32  cur_index =0;
    _int32  next_index =0;
    // _int32  start_index =0;
    _int32  uncomplete_num =0;
    // _int32 cur_length =0;
    RANGE cur_range;
    MAP_ITERATOR cur_map_node = NULL;
    MAP_ITERATOR next_map_node = NULL;
    LIST_ITERATOR cur_pipe = NULL;
    BOOL no_pipe = FALSE;
    BOOL has_priority_range = FALSE;
    DATA_PIPE* cur_data_pipe = NULL;
    DISPATCH_ITEM* cur_dispatch_item = NULL;
    BOOL  is_over_alloc = FALSE;
    _u32 now_time_ms;
    // LIST* ptr_dis_pipe_list = NULL;
    //RANGE_LIST_ITEROATOR cur_range_node = NULL;

    /*get uncomplete range and build uncomplete range map*/
    //     _u64 file_size = dm_get_file_size(ptr_dispatch_interface->_p_data_manager);
    _u64 file_size = DS_GET_FILE_SIZE(ptr_dispatch_interface->_data_interface);
    RANGE full_r = pos_length_to_range(0, file_size, file_size);
    RANGE_LIST uncomplete_range_list;
    RANGE_LIST pri_range_list;
    RANGE_LIST dispatched_correct_range_list;
    RANGE_LIST undispatched_correct_range_list;
    RANGE_LIST cannotdispatched_correct_range_list;
    RANGE_LIST downing_range_list;
    RANGE_LIST tmp_range_list;
    // RANGE_LIST last_alloc_range_list;
    RANGE_LIST partial_alloc_range_list;
    RANGE_LIST_ITEROATOR cur_range_node = NULL;
    RANGE_LIST_ITEROATOR cur_add_node = NULL;
    RANGE_LIST_ITEROATOR cur_index_add_node = NULL;
    RANGE_LIST_ITEROATOR cur_index_add_node2 = NULL;
    LIST_ITERATOR cur_error_block_node = NULL;
    //LIST_ITERATOR cur_erase_pipe_node = NULL;
    ERROR_BLOCK*   cur_error_block = NULL;
    BOOL bool_ret = FALSE;
    _u32 over_alloc_time = 0;
    LIST connect_manager_list ;
    LIST_ITERATOR connect_manager_it = NULL;
    CONNECT_MANAGER*  cur_connect_manager = NULL;
    _u32 start_index = MAX_U32;
    //   LOG_ERROR("ds_dispatch_at_vod");
#ifdef _DEBUG
    _u32 p_server_connected_count = 0;
    _u32 p_server_connecting_count = 0;
    _u32 p_peer_connected_count = 0;
    _u32 p_peer_connecting_count = 0;
    _u32 p_cdn_pipe_count = 0;
#endif
    range_list_init(&uncomplete_range_list);
    range_list_init(&pri_range_list);
    range_list_init(&dispatched_correct_range_list);
    range_list_init(&undispatched_correct_range_list);
    range_list_init(&cannotdispatched_correct_range_list);
    range_list_init(&downing_range_list);
    range_list_init(&tmp_range_list);
    //range_list_init(&last_alloc_range_list);
    range_list_init(&partial_alloc_range_list);
    list_init(&dised_server_long_connect_list._data_pipe_list);
    list_init(&dised_server_range_list._data_pipe_list);
    list_init(&dised_server_last_list._data_pipe_list);
    list_init(&new_server_long_connect_list._data_pipe_list);
    list_init(&new_server_range_list._data_pipe_list);
    list_init(&new_server_last_list._data_pipe_list);
    list_init(&dised_peer_list._data_pipe_list);
    list_init(&new_peer_list._data_pipe_list);
    list_init(&cadidate_list._data_pipe_list);
    list_init(&partial_pipe_list._data_pipe_list);
    list_init(&connect_manager_list);
    LOG_DEBUG("ds_dispatch_at_vod, get filesize %llu,  start_index:%u, start_time:%u", file_size, ptr_dispatch_interface->_start_index, ptr_dispatch_interface->_start_index_time_ms);

#ifdef VVVV_VOD_DEBUG
    LOG_ERROR("ds_dispatch_at_vod,  gloabal_recv_data %llu, gloabal_invalid_data:%llu , gloabal_discard_data:%llu, invalid  ratio :%u  , task_speed:%u, vv_speed:%u.",
        gloabal_recv_data,
        gloabal_invalid_data,
        gloabal_discard_data,
        (_u32)(gloabal_recv_data==0?0:(gloabal_invalid_data+gloabal_discard_data)*100/(gloabal_invalid_data+gloabal_discard_data+gloabal_recv_data)),
        gloabal_task_speed, gloabal_vv_speed);
#endif
    /* init uncomplete map */
    init_download_map(&download_map);
    init_download_map(&partial_map);
    // range_list_cp_range_list(&ptr_dispatch_interface->_tmp_range_list, &last_alloc_range_list);
    range_list_clear(&ptr_dispatch_interface->_tmp_range_list);

    range_list_clear(&ptr_dispatch_interface->_overalloc_range_list);
    range_list_clear(&ptr_dispatch_interface->_overalloc_in_once_dispatch);
    range_list_clear(&ptr_dispatch_interface->_cur_down_point);
    ptr_dispatch_interface->_cur_down_pipes = 0;
    sd_time_ms(&now_time_ms);
    // ptr_dispatch_interface->_start_index = MAX_U32;
    // ptr_dispatch_interface->_start_index_time_ms = MAX_U32;
    /*get priority range list*/
    DS_GET_PRIORITY_RANGE_VOD(ptr_dispatch_interface->_data_interface, &pri_range_list);
    if(pri_range_list._node_size == 0)
    {
#ifdef _VOD_NO_DISK
        LOG_ERROR("pri_range_list is empty. _VOD_NO_DISK and return.");
        return SUCCESS; //如果是无盘点播就直接返回，在没有优先块的情况下无需调度
#endif
        /*get uncomplet range list*/
        //range_list_add_range(&uncomplete_range_list, &full_r, NULL,NULL);
        //dm_get_uncomplete_range(ptr_dispatch_interface->_p_data_manager, &uncomplete_range_list);
        DS_GET_UNCOMPLETE_RANGE(ptr_dispatch_interface->_data_interface, &uncomplete_range_list);
        dispatch_range_list = &uncomplete_range_list;
    }
    else
    {
        dispatch_range_list = &pri_range_list;
        has_priority_range = TRUE;
    }
#ifndef DISPATCH_RANDOM_MODE
    // BT任务独立处理
    if(!cm_is_bttask(ptr_dispatch_interface->_p_connect_manager))
    {
        ds_filter_range_list_from_begin(dispatch_range_list, SECTION_UNIT*1024/16);
    }
#endif

    cm_get_connect_manager_list(ptr_dispatch_interface->_p_connect_manager, &connect_manager_list);
    if(list_size(&connect_manager_list) == 0)
    {
        LOG_ERROR("ds_dispatch_at_vod, has no connect manager !");
        goto  vod_dispatch_done;
    }

    connect_manager_it = LIST_BEGIN(connect_manager_list);
    while(connect_manager_it != LIST_END(connect_manager_list))
    {
        cur_connect_manager = (CONNECT_MANAGER*)LIST_VALUE(connect_manager_it);
        /*get pipe lists*/
        cm_get_working_server_pipe_list(cur_connect_manager, &p_server_connected_list);
        cm_get_connecting_server_pipe_list(cur_connect_manager, &p_server_connecting_list);
        cm_get_working_peer_pipe_list(cur_connect_manager, &p_peer_connected_list);
        cm_get_connecting_peer_pipe_list(cur_connect_manager, &p_peer_connecting_list);
#ifdef _DEBUG
        p_server_connected_count += list_size(&p_server_connected_list->_data_pipe_list);
        p_server_connecting_count += list_size(&p_server_connecting_list->_data_pipe_list);
        p_peer_connected_count += list_size(&p_peer_connected_list->_data_pipe_list);
        p_peer_connecting_count += list_size(&p_peer_connecting_list->_data_pipe_list);
#endif

#ifdef ENABLE_CDN
        cm_get_cdn_pipe_list(cur_connect_manager, &p_cdn_pipe_list);
#ifdef _DEBUG        
        p_cdn_pipe_count += list_size(&p_cdn_pipe_list->_data_pipe_list);
#endif
#endif /* ENABLE_CDN */
        /*build pipe maps*/
        ds_build_pipe_list_map(&download_map, &partial_map,p_server_connected_list, dispatch_range_list,
            &dised_server_long_connect_list, &dised_server_range_list,&dised_server_last_list,
            &new_server_long_connect_list, &new_server_range_list,&new_server_last_list,
            &partial_pipe_list, &dispatched_correct_range_list, &downing_range_list, now_time_ms);

        ds_build_pipe_list_map(&download_map, &partial_map,p_server_connecting_list, dispatch_range_list,
            &dised_server_long_connect_list, &dised_server_range_list,&dised_server_last_list,
            &new_server_long_connect_list, &new_server_range_list,&new_server_last_list,
            &partial_pipe_list,&dispatched_correct_range_list, &downing_range_list, now_time_ms);

        ds_build_pipe_list_map(&download_map, &partial_map,p_peer_connected_list, dispatch_range_list,
            &dised_peer_list,NULL,NULL,
            &new_peer_list,NULL,NULL,&partial_pipe_list,&dispatched_correct_range_list, &downing_range_list, now_time_ms);

        ds_build_pipe_list_map(&download_map, &partial_map,p_peer_connecting_list, dispatch_range_list,
            &dised_peer_list,NULL,NULL,
            &new_peer_list,NULL,NULL,&partial_pipe_list,&dispatched_correct_range_list, &downing_range_list, now_time_ms);
        ds_build_pipe_list_map(&download_map, &partial_map,p_cdn_pipe_list, dispatch_range_list,
            &dised_peer_list,NULL,NULL,
            &new_peer_list,NULL,NULL,&partial_pipe_list,&dispatched_correct_range_list, &downing_range_list, now_time_ms);

        p_server_connected_list = NULL;
        p_server_connecting_list = NULL;
        p_peer_connected_list = NULL;
        p_peer_connecting_list = NULL;
        p_cdn_pipe_list = NULL;
        connect_manager_it = LIST_NEXT(connect_manager_it);
    }
    if(dispatch_range_list->_node_size == 0)
    {
        LOG_DEBUG("ds_dispatch_at_vod, has no uncomplete range so return !");
        goto  vod_dispatch_done;
    }
    LOG_DEBUG("[vod_dispatch_analysis][pipe count] server_connected count=%u, server_connecting count=%u, peer_connected count=%u, peer_connecting count=%u, p_cdn count=%u, dised_server_long_connect count=%u\
              , dised_server_range count=%u, dised_server_last count=%u, new_server_long_connect count=%u, new_server_range count=%u, new_server_last count=%u, dised_peer count=%u, new_peer count=%u, partial_pipe count=%u"
              , p_server_connected_count	
              , p_server_connecting_count
              , p_peer_connected_count
              , p_peer_connecting_count
              , p_cdn_pipe_count
              , list_size(&dised_server_long_connect_list._data_pipe_list)
              , list_size(&dised_server_range_list._data_pipe_list)
              , list_size(&dised_server_last_list._data_pipe_list)
              , list_size(&new_server_long_connect_list._data_pipe_list)
              , list_size(&new_server_range_list._data_pipe_list)
              , list_size(&new_server_last_list._data_pipe_list)
              , list_size(&dised_peer_list._data_pipe_list)
              , list_size(&new_peer_list._data_pipe_list)
              , list_size(&partial_pipe_list._data_pipe_list));

#ifdef _VOD_NO_DISK
    //  无盘点播下载无需校验逻辑
#else
    DS_GET_ERROR_RANGE_BLOCK_LIST(ptr_dispatch_interface->_data_interface, &p_error_block_list);
    if(p_error_block_list != NULL && list_size(&p_error_block_list->_error_block_list) != 0)
    {
        LOG_ERROR("ds_dispatch_at_vod, get %u error blocks.", list_size(&p_error_block_list->_error_block_list));
        cur_error_block_node = LIST_BEGIN(p_error_block_list->_error_block_list);
        while(cur_error_block_node != LIST_END(p_error_block_list->_error_block_list))
        {
            cur_error_block = (ERROR_BLOCK*)LIST_VALUE(cur_error_block_node);
            LOG_ERROR("ds_dispatch_at_vod, process error block, (%u,%u),  level: %u, error times: %u .", cur_error_block->_r._index,  cur_error_block->_r._num,
                cur_error_block->_valid_resources, cur_error_block->_error_count);
            if(range_list_is_include(&dispatched_correct_range_list, &cur_error_block->_r) == FALSE
                &&  range_list_is_relevant(dispatch_range_list, &cur_error_block->_r) == TRUE)
            {
                if(cur_error_block->_valid_resources == 2)
                {
                    bool_ret = ds_handle_correct_range_using_origin_res(ptr_dispatch_interface,cur_error_block, &new_server_long_connect_list,&new_server_range_list,NULL, &new_peer_list, NULL);
                    if(bool_ret == TRUE)
                    {
                        LOG_ERROR("ds_dispatch_at_vod, process error block, (%u,%u),  level: %u, error times: %u  assign to connecting data pipe.", cur_error_block->_r._index,  cur_error_block->_r._num,
                            cur_error_block->_valid_resources, cur_error_block->_error_count);
                        range_list_add_range(&undispatched_correct_range_list, &cur_error_block->_r, NULL, NULL);
                    }
                    else
                    {
                        bool_ret = ds_handle_correct_range_using_origin_res(ptr_dispatch_interface,cur_error_block, &dised_server_long_connect_list,&dised_server_range_list,NULL, &dised_peer_list, &download_map);
                        if(bool_ret == TRUE)
                        {
                            LOG_ERROR("ds_dispatch_at_vod, process error block, (%u,%u),  level: %u, error times: %u  assign to connected data pipe.", cur_error_block->_r._index,  cur_error_block->_r._num,
                                cur_error_block->_valid_resources, cur_error_block->_error_count);
                            range_list_add_range(&undispatched_correct_range_list, &cur_error_block->_r, NULL, NULL);
                        }
                        else
                        {
                            LOG_ERROR("ds_dispatch_at_vod, process error block, (%u,%u),  level: %u, error times: %u  error.", cur_error_block->_r._index,  cur_error_block->_r._num,
                                cur_error_block->_valid_resources, cur_error_block->_error_count);
                            range_list_add_range(&cannotdispatched_correct_range_list, &cur_error_block->_r, NULL, NULL);
                        }
                    }
                }
                else
                {
                    bool_ret = ds_handle_correct_range(cur_error_block, &new_server_long_connect_list,&new_server_range_list,NULL, &new_peer_list, NULL);
                    if(bool_ret == TRUE)
                    {
                        LOG_ERROR("ds_dispatch_at_vod, process error block, (%u,%u),  level: %u, error times: %u  assign to connecting data pipe.", cur_error_block->_r._index,  cur_error_block->_r._num,
                            cur_error_block->_valid_resources, cur_error_block->_error_count);
                        range_list_add_range(&undispatched_correct_range_list, &cur_error_block->_r, NULL, NULL);
                    }
                    else
                    {
                        bool_ret = ds_handle_correct_range(cur_error_block, &dised_server_long_connect_list,&dised_server_range_list,NULL, &dised_peer_list, &download_map);
                        if(bool_ret == TRUE)
                        {
                            LOG_ERROR("ds_dispatch_at_vod, process error block, (%u,%u),  level: %u, error times: %u  assign to connected data pipe.", cur_error_block->_r._index,  cur_error_block->_r._num,
                                cur_error_block->_valid_resources, cur_error_block->_error_count);
                            range_list_add_range(&undispatched_correct_range_list, &cur_error_block->_r, NULL, NULL);
                        }
                        else
                        {
                            bool_ret = ds_handle_correct_range(cur_error_block, &partial_pipe_list,NULL,NULL, NULL, NULL);
                            if(bool_ret == TRUE)
                            {
                                LOG_ERROR("ds_dispatch_at_vod, process error block, (%u,%u),  level: %u, error times: %u  assign to partial_pipe_list data pipe.", cur_error_block->_r._index,  cur_error_block->_r._num,
                                    cur_error_block->_valid_resources, cur_error_block->_error_count);
                                range_list_add_range(&undispatched_correct_range_list, &cur_error_block->_r, NULL, NULL);
                            }
                            else
                            {
                                LOG_ERROR("ds_dispatch_at_vod, process error block, (%u,%u),  level: %u, error times: %u  error.", cur_error_block->_r._index,  cur_error_block->_r._num,
                                    cur_error_block->_valid_resources, cur_error_block->_error_count);
                                range_list_add_range(&cannotdispatched_correct_range_list, &cur_error_block->_r, NULL, NULL);
                            }
                        }
                    }
                }
            }
            else
            {
                LOG_ERROR("ds_dispatch_at_vod, error block, (%u,%u),  level: %u, error times: %u  has assgin before or has downed!.", cur_error_block->_r._index,  cur_error_block->_r._num,
                    cur_error_block->_valid_resources, cur_error_block->_error_count);
            }
            cur_error_block_node = LIST_NEXT(cur_error_block_node);
        }
    }

    if(dispatched_correct_range_list._node_size != 0)
    {
        LOG_ERROR("ds_dispatch_at_vod ,  dispatched_correct_range_list:.");
        out_put_range_list(&dispatched_correct_range_list);
        range_list_delete_range_list(dispatch_range_list, &dispatched_correct_range_list);
        //LOG_ERROR("ds_dispatch_at_vod , delete dispatched_correct_range_list , dispatch_range_list:.");
        //out_put_range_list(dispatch_range_list);
    }
    if(undispatched_correct_range_list._node_size != 0)
    {
        LOG_DEBUG("ds_dispatch_at_vod, undispatched_correct_range_list:");
        out_put_range_list(&undispatched_correct_range_list);
        range_list_delete_range_list(dispatch_range_list, &undispatched_correct_range_list);
        //LOG_DEBUG("ds_dispatch_at_vod, delete undispatched_correct_range_list, dispatch_range_list:");
        //out_put_range_list(dispatch_range_list);
    }
    if(cannotdispatched_correct_range_list._node_size != 0)
    {
        LOG_ERROR("ds_dispatch_at_vod ,  cannotdispatched_correct_range_list.");
        out_put_range_list(&cannotdispatched_correct_range_list);

        /*目前没有纠错块正在纠错，所以cannotdispatched_correct_range_list 中是没有资源可以纠错的块，这些块需要重新下载一次导致纠错失败。*/
        if(dispatched_correct_range_list._node_size == 0 && undispatched_correct_range_list._node_size == 0)
        {
            LOG_DEBUG("ds_dispatch_at_vod , only  cannotdispatched_correct_range_list so set cannot correct error range.");
            set_cannot_correct_error_block_list(p_error_block_list, &cannotdispatched_correct_range_list);
            range_list_clear(&cannotdispatched_correct_range_list);
        }
        else
        {
            range_list_delete_range_list(dispatch_range_list, &cannotdispatched_correct_range_list);
            //LOG_ERROR("ds_dispatch_at_vod , delete cannotdispatched_correct_range_list , un_complete_range_list:.");
            //out_put_range_list(dispatch_range_list);
        }
    }

    if(dispatch_range_list->_node_size == 0)
    {
        LOG_ERROR("ds_dispatch_at_vod, 2  has no uncomplete range so return !");
		return SUCCESS;
    }
    LOG_DEBUG("[vod_dispatch_analysis] dispatch_range_list print");
    out_put_range_list(dispatch_range_list);
#endif
    range_list_get_head_node(dispatch_range_list, &cur_range_node);
    if(cur_range_node != NULL)
    {
        start_index = cur_range_node->_range._index;
        cur_range_node = NULL;
        if(start_index != ptr_dispatch_interface->_start_index)
        {
            ptr_dispatch_interface->_start_index = MAX_U32;
            ptr_dispatch_interface->_start_index_time_ms = MAX_U32;
        }
    }
    LOG_DEBUG("ds_dispatch_at_vod,  real start_index :%u ,ds start_index:%u, start_time:%u", start_index,  ptr_dispatch_interface->_start_index, ptr_dispatch_interface->_start_index_time_ms);
    uncomplete_range_size = range_list_get_total_num(dispatch_range_list);
    pipe_num = list_size(&dised_server_long_connect_list._data_pipe_list);
    pipe_num += list_size(&dised_server_range_list._data_pipe_list);
    //pipe_num += list_size(&dised_server_last_list._data_pipe_list);
    pipe_num += list_size(&new_server_long_connect_list._data_pipe_list);
    pipe_num += list_size(&new_server_range_list._data_pipe_list);
    //pipe_num += list_size(&new_server_last_list._data_pipe_list);
    pipe_num += list_size(&dised_peer_list._data_pipe_list);
    pipe_num += list_size(&new_peer_list._data_pipe_list);
    ptr_dispatch_interface->_cur_down_pipes += pipe_num;
    dowing_no_range_pipe_num =  list_size(&dised_server_last_list._data_pipe_list);
    new_no_range_pipe_num =  list_size(&new_server_last_list._data_pipe_list);
    if(pipe_num == 0 && new_no_range_pipe_num == 0)
    {
        LOG_DEBUG("ds_dispatch_at_vod, has no pipes, so return !");
        goto  vod_dispatch_done;
    }
    if(dowing_no_range_pipe_num == 0 && new_no_range_pipe_num != 0 && pipe_num <= MIN_PIPE_NUM)
    {
        list_pop(&new_server_last_list._data_pipe_list, (void**)&cur_data_pipe);
        if(cur_data_pipe  !=  NULL)
        {
            if(ds_assigned_range_to_pipe(cur_data_pipe, &full_r) == TRUE)
            {
                //   start_index = range_num_per_pipe;
                LOG_DEBUG("ds_dispatch_at_vod , success assign range (%u, %u) to no range data pipe: 0x%x, so from %u start assign.", full_r._index, full_r._num, cur_data_pipe, start_index);
            }
            else
            {
                LOG_ERROR("ds_dispatch_at_vod ,failure to assign range (%u, %u) to no range data pipe: 0x%x .", full_r._index, full_r._num, cur_data_pipe);
            }
        }
    }
    ds_adjust_pipe_list(&new_server_long_connect_list);
    ds_adjust_pipe_list(&new_server_range_list);
    ds_adjust_pipe_list(&new_peer_list);

    /* build uncomplete map */
    ds_build_uncomplete_map(&download_map, dispatch_range_list);
    range_num_per_pipe = (uncomplete_range_size )/(pipe_num );
    if(range_num_per_pipe <= MIN_DISPATCH_BLOCK_NUM)
        range_num_per_pipe = MIN_DISPATCH_BLOCK_NUM;

    LOG_DEBUG("ds_dispatch_at_vod, uncomplete_range_size: %u, pipe_num: %u,  range_num_per_pipe: %u", uncomplete_range_size, pipe_num, range_num_per_pipe);
    cur_add_node = NULL;
    cur_index_add_node = NULL;
    cur_index_add_node2 = NULL;
    cur_map_node = MAP_BEGIN(download_map);
    while(cur_map_node != MAP_END(download_map))
    {
        cur_index =  (_u32)MAP_KEY(cur_map_node);
        cur_dispatch_item = (DISPATCH_ITEM* )MAP_VALUE(cur_map_node);
        next_map_node = MAP_NEXT(download_map, cur_map_node);
        if(next_map_node != MAP_END(download_map))
        {
            next_index = (_u32)MAP_KEY(next_map_node);
        }
        else
        {
            next_index = full_r._index+full_r._num;
        }
        LOG_DEBUG("[vod_dispatch_analysis] download_map, range(%u, %u), is_complete:%u", cur_index, next_index, cur_dispatch_item->_is_completed);
#ifdef _DEBUG
        if(list_size(&cur_dispatch_item->_owned_pipes) != 0)
        {
            cur_pipe = LIST_BEGIN(cur_dispatch_item->_owned_pipes);
            while(cur_pipe != LIST_END(cur_dispatch_item->_owned_pipes))
            {
                cur_data_pipe = (DATA_PIPE*)LIST_VALUE(cur_pipe);
                LOG_DEBUG("[vod_dispatch_analysis], cur_data_pipe:0x%x, down_range(%d,%d), assign_range(%d,%d), speed:%d.",
                    cur_data_pipe
                    , cur_data_pipe->_dispatch_info._down_range._index, cur_data_pipe->_dispatch_info._down_range._num
                    , cur_data_pipe->_dispatch_info._correct_error_range._index, cur_data_pipe->_dispatch_info._correct_error_range._num
                    , cur_data_pipe->_dispatch_info._speed );
                LOG_DEBUG("print _uncomplete_ranges:");
                out_put_range_list(&cur_data_pipe->_dispatch_info._uncomplete_ranges);
                cur_pipe = LIST_NEXT(cur_pipe);
            }
        }
#endif
        if(cur_dispatch_item->_is_completed  ==  TRUE)
        {
            if(cur_dispatch_item ->_is_pipe_alloc == TRUE)
            {
                if(ptr_dispatch_interface ->_start_index == MAX_U32  && cur_index == start_index)
                {
                    ptr_dispatch_interface ->_start_index = start_index;
                    sd_time_ms(&ptr_dispatch_interface->_start_index_time_ms);
                }
                cur_range._index = cur_index;
                cur_range._num = 1;
                range_list_add_range(&ptr_dispatch_interface->_cur_down_point, &cur_range, cur_index_add_node, &cur_index_add_node);
                if(cur_dispatch_item->_alloc_times > MAX_OVER_ALLOC_TIMES)
                {
                    range_list_add_range(&ptr_dispatch_interface->_overalloc_range_list, &cur_range, cur_index_add_node2, &cur_index_add_node2);
                }
            }

            if(list_size(&cur_dispatch_item->_owned_pipes) != 0)
            {
                cur_pipe = LIST_BEGIN(cur_dispatch_item->_owned_pipes);
                while(cur_pipe != LIST_END(cur_dispatch_item->_owned_pipes))
                {
                    cur_data_pipe = (DATA_PIPE*)LIST_VALUE(cur_pipe);
                    range_list_clear(&cur_data_pipe->_dispatch_info._uncomplete_ranges);
                    LOG_ERROR("ds_dispatch_at_vod , cur_index:%u , next_index: %u, cur_data_pipe:0x%x 's uncomplete range list need clear.",
                        cur_index,next_index, cur_data_pipe);
                    cur_pipe = LIST_NEXT(cur_pipe);
                }
            }
            cur_map_node = next_map_node;
            continue;
        }

        if(list_size(&cur_dispatch_item->_owned_pipes) != 0)
        {
            LOG_ERROR("ds_dispatch_at_vod , cur_index:%u , next_index: %u, owned_pipes: %u",cur_index,next_index, list_size(&cur_dispatch_item->_owned_pipes));

            cur_pipe = LIST_BEGIN(cur_dispatch_item->_owned_pipes);
            while(cur_pipe != LIST_END(cur_dispatch_item->_owned_pipes))
            {
                cur_data_pipe = (DATA_PIPE*)LIST_VALUE(cur_pipe);
                if(ptr_dispatch_interface ->_start_index != MAX_U32 && cur_data_pipe->_dispatch_info._down_range._index == start_index)
                {
                    ptr_dispatch_interface->_start_index_time_ms =  cur_data_pipe->_dispatch_info._last_recv_data_time ;
                }
                if(cur_data_pipe->_dispatch_info._correct_error_range._num != 0)
                {
                    /*process last time assigned range */
                    LOG_ERROR("ds_dispatch_at_vod , cur_index:%u , next_index: %u, cur_data_pipe:0x%x, is correct pipe.",cur_index,next_index, cur_data_pipe);
                    if((cur_data_pipe->_dispatch_info._down_range._index + cur_data_pipe->_dispatch_info._down_range._num)
                        <(cur_data_pipe->_dispatch_info._correct_error_range._index + cur_data_pipe->_dispatch_info._correct_error_range._num))
                    {
                        cur_range._index = cur_data_pipe->_dispatch_info._down_range._index + cur_data_pipe->_dispatch_info._down_range._num;
                        cur_range._num = (cur_data_pipe->_dispatch_info._correct_error_range._index + cur_data_pipe->_dispatch_info._correct_error_range._num)
                            - (cur_data_pipe->_dispatch_info._down_range._index + cur_data_pipe->_dispatch_info._down_range._num);
                        if(ds_assigned_range_to_pipe(cur_data_pipe, &cur_range) == TRUE)
                        {
                            LOG_ERROR("ds_dispatch_at_vod , cur_index:%u , next_index: %u, cur_data_pipe:0x%x, is correct pipe, so assign correct range (%u, %u).",
                                cur_index,next_index, cur_data_pipe, cur_range._index, cur_range._num);
                            cur_index = (cur_data_pipe->_dispatch_info._correct_error_range._index + cur_data_pipe->_dispatch_info._correct_error_range._num);
                            cur_pipe = LIST_NEXT(cur_pipe);
                            continue;
                        }
                        else
                        {
                            LOG_ERROR("ds_dispatch_at_vod , cur_index:%u , next_index: %u, cur_data_pipe:0x%x, is correct pipe, assign correct range (%u, %u) failue so clear correct range.",
                                cur_index,next_index, cur_data_pipe, cur_range._index, cur_range._num);
                            cur_data_pipe->_dispatch_info._correct_error_range._index =0 ;
                            cur_data_pipe->_dispatch_info._correct_error_range._num =0 ;
                        }
                    }
                    else
                    {
                        LOG_ERROR("ds_dispatch_at_vod , cur_index:%u , next_index: %u, cur_data_pipe:0x%x, is correct pipe, downing range (%u,%u) is not in the assign correct range (%u, %u)  so clear correct range.",
                            cur_index,next_index, cur_data_pipe, cur_data_pipe->_dispatch_info._down_range._index, cur_data_pipe->_dispatch_info._down_range._num,
                            cur_data_pipe->_dispatch_info._correct_error_range._index, cur_data_pipe->_dispatch_info._correct_error_range._num);
                        cur_data_pipe->_dispatch_info._correct_error_range._index =0 ;
                        cur_data_pipe->_dispatch_info._correct_error_range._num =0 ;
                    }
                }

                if(cur_index + range_num_per_pipe < next_index)
                {
                    cur_range._index = cur_index;
                    cur_range._num = range_num_per_pipe;
                    //分配策略要細化
                    cur_range._num = MIN(cur_range._num, calc_assign_range_num_to_pipe(cur_data_pipe));

                    if(ds_assigned_range_to_pipe(cur_data_pipe, &cur_range) == TRUE)
                    {
                        LOG_ERROR("ds_dispatch_at_vod , 1 assign range (%u, %u) to data pipe: 0x%x", cur_range._index, cur_range._num, cur_data_pipe);
                        cur_index += cur_range._num;
                        range_list_add_range(&downing_range_list, &cur_range, cur_add_node, &cur_add_node);
                    }
                    else
                    {
                        LOG_ERROR("ds_dispatch_at_vod ,1 failure to assign range (%u, %u) to data pipe: 0x%x", cur_range._index, cur_range._num, cur_data_pipe);
                        //  ds_put_data_pipe(cur_data_pipe,&new_server_long_connect_list,&new_server_range_list,NULL, &new_peer_list);
                    }
                }
                else if(cur_index < next_index)
                {
                    cur_range._index = cur_index;
                    cur_range._num = next_index - cur_index;
                    //分配策略要細化
                    cur_range._num = MIN(cur_range._num, calc_assign_range_num_to_pipe(cur_data_pipe));
                    if(ds_assigned_range_to_pipe(cur_data_pipe, &cur_range) == TRUE)
                    {
                        cur_index =  cur_range._index + cur_range._num;
                        LOG_ERROR("ds_dispatch_at_vod , 2 assign range (%u, %u) to data pipe: 0x%x", cur_range._index, cur_range._num, cur_data_pipe);
                        range_list_add_range(&downing_range_list, &cur_range, cur_add_node, &cur_add_node);
                    }
                    else
                    {
                        LOG_ERROR("ds_dispatch_at_vod , 2 failure to assign range (%u, %u) to data pipe: 0x%x", cur_range._index, cur_range._num, cur_data_pipe);
                    }
                }
                else
                {
                    LOG_DEBUG("ds_dispatch_at_vod , because assign finished so put  data pipe: 0x%x",  cur_data_pipe);
                }
                cur_pipe = LIST_NEXT(cur_pipe);
            }
        }
        cur_map_node = next_map_node;
    }

    // alloc partial range
    if(list_size(&partial_pipe_list._data_pipe_list) != 0)
    {
        LOG_ERROR("ds_dispatch_at_vod assign partial pipe.");
        cur_data_pipe = NULL;
        cur_pipe = LIST_BEGIN(partial_pipe_list._data_pipe_list);
        while(cur_pipe != LIST_END(partial_pipe_list._data_pipe_list))
        {
            cur_data_pipe = (DATA_PIPE*)LIST_VALUE(cur_pipe);
            get_range_list_overlap_list(&cur_data_pipe->_dispatch_info._can_download_ranges, dispatch_range_list, &tmp_range_list);
            range_list_delete_range_list(&tmp_range_list, &downing_range_list);
            if(tmp_range_list._node_size != 0)
            {// 改进：vod不考虑重叠下载，只需要考虑及时替换劣质pipe下载优先块。
                range_list_get_head_node(&tmp_range_list, &cur_range_node);
                if(cur_range_node != NULL)
                {
                    cur_range._index = cur_range_node->_range._index;
                    cur_range._num = MIN(cur_range._num, calc_assign_range_num_to_pipe(cur_data_pipe));
                    if(ds_assigned_range_to_pipe(cur_data_pipe, &cur_range) == TRUE)
                    {
                        LOG_DEBUG("ds_dispatch_at_vod,  2 assign range (%u, %u) to data pipe: 0x%x", cur_range._index, cur_range._num, cur_data_pipe);
                        range_list_add_range(&downing_range_list, &cur_range, NULL, NULL);
                        cur_range._num = 1;
                        range_list_add_range(&ptr_dispatch_interface->_cur_down_point, &cur_range, NULL, NULL);
                        if(ptr_dispatch_interface ->_start_index == MAX_U32 && cur_range._index == start_index)
                        {
                            ptr_dispatch_interface ->_start_index = start_index;
                            sd_time_ms(&ptr_dispatch_interface->_start_index_time_ms);
                        }
                    }
                    else
                    {
                        LOG_ERROR("ds_dispatch_at_vod,  failure to assign range (%u, %u) to data pipe: 0x%x", cur_range._index, cur_range._num, cur_data_pipe);
                    }

                } 
                else 
                {
                    LOG_ERROR("ds_dispatch_at_vod,  ptr_data_pipe: 0x%x , 2 can not get rand range node of uncomplete_range_list, so return.", cur_data_pipe);
                }
            }
            else
            {
                LOG_DEBUG("ds_dispatch_at_vod, data pipe :0x%x  can not down uncomplete range.", cur_data_pipe);
            }

            cur_pipe = LIST_NEXT(cur_pipe);
        }
        range_list_clear(&tmp_range_list);
        LOG_ERROR("ds_dispatch_at_vod assign partial pipe end.");
    }
    // alloc  can down full pipes
    LOG_DEBUG("ds_dispatch_at_vod, alloc full pipes");
    cur_add_node = NULL;
    cur_index_add_node = NULL;
    LOG_DEBUG("[vod_dispatch_analysis] downing_range_list print");
    out_put_range_list(&downing_range_list);
    range_list_delete_range_list(dispatch_range_list, &downing_range_list);
    uncomplete_range_size = range_list_get_total_num(dispatch_range_list);
    LOG_DEBUG("[vod_dispatch_analysis] range_list_delete_range_list(dispatch_range_list, &downing_range_list) uncomplete_range_size=%u, print", uncomplete_range_size);
    out_put_range_list(dispatch_range_list);
    pipe_num = list_size(&new_server_long_connect_list._data_pipe_list);
    pipe_num += list_size(&new_server_range_list._data_pipe_list);
    pipe_num += list_size(&new_peer_list._data_pipe_list);
    if(pipe_num != 0)
    {
        range_num_per_pipe = (uncomplete_range_size )/(pipe_num );
    }
    else
    {
        range_num_per_pipe = MIN_DISPATCH_BLOCK_NUM;
    }
    if(range_num_per_pipe <= MIN_DISPATCH_BLOCK_NUM)
        range_num_per_pipe = MIN_DISPATCH_BLOCK_NUM;
#if 1
    if(dispatch_range_list->_node_size != 0)
    {
        no_pipe = FALSE;
        range_list_get_head_node(dispatch_range_list, &cur_range_node);
        while(cur_range_node != NULL)
        {
            cur_index = cur_range_node->_range._index;
            next_index =cur_range_node->_range._index + cur_range_node->_range._num;
            while(cur_index < next_index)
            {
                if(cur_index + range_num_per_pipe< next_index)
                {
                    cur_range._index = cur_index;
                    cur_range._num = range_num_per_pipe;
                }
                else
                {
                    cur_range._index = cur_index;
                    cur_range._num = next_index - cur_index;
                }
                cur_data_pipe = ds_get_data_pipe(&new_server_long_connect_list, &new_server_range_list, NULL, &new_peer_list);
                if(cur_data_pipe == NULL)
                {
                    LOG_ERROR("ds_dispatch_at_vod , can not get data pipe to assign range (%u, %u) .", cur_range._index, cur_range._num);
                    no_pipe = TRUE;
                    uncomplete_num += cur_range._num;
                    break;
                }

                cur_range._num = MIN(cur_range._num, calc_assign_range_num_to_pipe(cur_data_pipe));
                if(ds_assigned_range_to_pipe(cur_data_pipe, &cur_range) == FALSE)
                {
                    // ds_put_data_pipe(cur_data_pipe,&new_server_long_connect_list,&new_server_range_list,&new_server_last_list, &new_peer_list);
                    LOG_ERROR("ds_dispatch_at_vod , 3 failure to assign range (%u, %u) to data pipe: 0x%x", cur_range._index, cur_range._num, cur_data_pipe);
                    continue;
                }
                else
                {
                    cur_index += cur_range._num;
                    LOG_ERROR("ds_dispatch_at_vod , 3 success to assign range (%u, %u) to data pipe: 0x%x", cur_range._index, cur_range._num, cur_data_pipe);
                    range_list_add_range(&downing_range_list, &cur_range, cur_add_node, &cur_add_node);
                    if(ptr_dispatch_interface ->_start_index == MAX_U32  && cur_index == start_index)
                    {
                        ptr_dispatch_interface ->_start_index = start_index;
                        sd_time_ms(&ptr_dispatch_interface->_start_index_time_ms);
                    }
                    cur_range._num = 1;
                    range_list_add_range(&ptr_dispatch_interface->_cur_down_point, &cur_range, cur_index_add_node, &cur_index_add_node);
                }

            }
            if(no_pipe == TRUE)
            {
                LOG_ERROR("ds_dispatch_at_vod , no pipes, so break !");
                break;
            }
            range_list_get_next_node(dispatch_range_list, cur_range_node, &cur_range_node);
        }
    }
#endif
    range_list_add_range_list(&ptr_dispatch_interface->_tmp_range_list, &downing_range_list);
    // TODO 改进：vod不考虑重叠下载，只需要考虑及时替换劣质pipe下载优先块。

vod_dispatch_done:
    LOG_DEBUG("ds_dispatch_at_vod end .");
    unit_download_map(&download_map);
    unit_download_map(&partial_map);
    range_list_clear(&uncomplete_range_list);
    range_list_clear(&pri_range_list);
    range_list_clear(&dispatched_correct_range_list);
    range_list_clear(&undispatched_correct_range_list);
    range_list_clear(&cannotdispatched_correct_range_list);
    range_list_clear(&downing_range_list);
    range_list_clear(&tmp_range_list);
    //range_list_clear(&last_alloc_range_list);
    range_list_clear(&partial_alloc_range_list);

    clear_pipe_list( &dised_server_long_connect_list._data_pipe_list) ;
    clear_pipe_list( &dised_server_range_list._data_pipe_list);
    clear_pipe_list( &dised_server_last_list._data_pipe_list);
    clear_pipe_list( &new_server_long_connect_list._data_pipe_list) ;
    clear_pipe_list( &new_server_range_list._data_pipe_list);
    clear_pipe_list( &new_server_last_list._data_pipe_list);
    clear_pipe_list( &dised_peer_list._data_pipe_list) ;
    clear_pipe_list( &new_peer_list._data_pipe_list);
    clear_pipe_list( &cadidate_list._data_pipe_list);
    clear_pipe_list( &partial_pipe_list._data_pipe_list);
    list_clear(&connect_manager_list);
    return SUCCESS;
}

void ds_adjust_range_list_by_index(RANGE_LIST* range_list)
{
    //int i,j;
    unsigned int size = range_list->_node_size;

    if(size<1 || range_list == NULL || range_list_size(range_list)==0 )
        return;

    RANGE_LIST_NODE *pos = range_list->_tail_node;

    RANGE_LIST_NODE *index = NULL;
    RANGE_LIST_NODE *max = NULL;
    unsigned int tmp;

    while(range_list->_head_node != pos)
    {
        index = range_list->_head_node;
        max = index;

        for(; index!=pos; index=index->_next_node)
        {
            if(index->_range._index > max->_range._index)
                max = index;
        }

        //swap the data
        if(max->_range._index > pos->_range._index)
        {
            tmp = max->_range._index;
            max->_range._index = pos->_range._index;
            pos->_range._index = tmp;

            tmp = max->_range._num;
            max->_range._num = pos->_range._num;
            pos->_range._num = tmp;
        }

        //move forward
        pos = pos->_prev_node;
    }

}

//过滤链表，从头删除超出block_num的range
void ds_filter_range_list_from_begin(RANGE_LIST* range_list, int block_num)
{
    LOG_DEBUG("ds_filter_range_list_from_begin, block_num =%d", block_num);

    if ((block_num < 1) || (range_list == NULL) || (range_list_size(range_list) == 0))
    {
        return;
    }

    ds_adjust_range_list_by_index(range_list);
    //we suppose the list was order by range._index
    RANGE_LIST_NODE *index = range_list->_head_node;
    block_num -= index->_range._num;
    if (block_num < 0)
    {
        LOG_DEBUG("filter range, num %d , cut size %d", index->_range._num, block_num);
        index->_range._num += block_num;
        LOG_DEBUG("filter range, num %d",index->_range._num);
    }
    for (index = index->_next_node; ((index != range_list->_head_node) && (index != NULL)); index = index->_next_node)
    {
        if (block_num > 0)
        {
            block_num -= index->_range._num;
            //not allow larger than block_num
            if (block_num < 0)
            {
                LOG_DEBUG("filter range, num %d , cut size %d", index->_range._num, block_num);
                index->_range._num += block_num;
                LOG_DEBUG("filter range, num %d", index->_range._num);
            }
        }
        else
        {
            index = index->_prev_node;
            range_list_delete_range(range_list, &(index->_next_node->_range), index, NULL);
        }
    }
}

_int32 ds_dispatch_at_muti_res(DISPATCHER*  ptr_dispatch_interface)
{
    return ds_dispatch_at_muti_res_nomal(ptr_dispatch_interface);
}

BOOL ds_handle_correct_range(ERROR_BLOCK* p_error_block, PIPE_LIST* p_long_server_list,  PIPE_LIST* p_range_server_list, PIPE_LIST* p_last_server_list,
                             PIPE_LIST* p_peer_pipe_list, MAP* p_down_map)
{
    LOG_DEBUG("ds_handle_correct_range begin.");

    if(p_long_server_list != NULL)
    {
        LOG_DEBUG("ds_handle_correct_range, using p_long_server_list.");
        if(ds_assign_correct_range(p_error_block, p_long_server_list, p_down_map) == TRUE)
        {
            LOG_DEBUG("ds_handle_correct_range, using p_long_server_list success.");
            return TRUE;
        }
    }

    if(p_peer_pipe_list != NULL)
    {
        LOG_DEBUG("ds_handle_correct_range, using p_peer_pipe_list.");
        if(ds_assign_correct_range(p_error_block, p_peer_pipe_list, p_down_map) == TRUE)
        {
            LOG_DEBUG("ds_handle_correct_range, using p_peer_pipe_list success.");
            return TRUE;
        }
    }

    if(p_range_server_list != NULL)
    {
        LOG_DEBUG("ds_handle_correct_range, using p_range_server_list.");
        if(ds_assign_correct_range(p_error_block, p_range_server_list, p_down_map) == TRUE)
        {
            LOG_DEBUG("ds_handle_correct_range, using p_range_server_list success.");
            return TRUE;
        }
    }

    if(p_last_server_list != NULL)
    {
        LOG_DEBUG("ds_handle_correct_range, using p_last_server_list");
        if(ds_assign_correct_range(p_error_block, p_last_server_list, p_down_map) == TRUE)
        {
            LOG_DEBUG("ds_handle_correct_range, using p_last_server_list.");
            return TRUE;
        }
    }

    LOG_DEBUG("ds_handle_correct_range end.");
    return FALSE;

}

BOOL ds_assign_correct_range(ERROR_BLOCK* p_error_block, PIPE_LIST* p_pipe_list, MAP* p_down_map)
{
    RANGE* p_cur_range = &p_error_block->_r;

    _u32 erase_index = MAX_U32;
    /*LIST* p_cur_error_res_list = &p_error_block->_error_resources;*/

    DATA_PIPE* cur_data_pipe = NULL;
    LIST_ITERATOR cur_pipe_node = LIST_BEGIN(p_pipe_list->_data_pipe_list);

    LOG_DEBUG("ds_assign_correct_range begin.");

    while(cur_pipe_node != LIST_END(p_pipe_list->_data_pipe_list))
    {
        cur_data_pipe = (DATA_PIPE*)LIST_VALUE(cur_pipe_node);

        erase_index = MAX_U32;

        if(cur_data_pipe->_dispatch_info._correct_error_range._num == 0
            && ds_res_is_include(&p_error_block->_error_resources,cur_data_pipe->_p_resource) == FALSE)
        {
            if(cur_data_pipe->_dispatch_info._down_range._num != 0)
            {
                erase_index = cur_data_pipe->_dispatch_info._down_range._index
                    + cur_data_pipe->_dispatch_info._down_range._num;
            }

            if(ds_assigned_range_to_pipe(cur_data_pipe, p_cur_range) == TRUE)
            {
                /*set assign range for correct block*/
                LOG_DEBUG("ds_assign_correct_range,  data_pipe:0x%x, is assigned by error block (%u,%u)", cur_data_pipe, p_cur_range->_index, p_cur_range->_num);

                if(  p_down_map != NULL && erase_index != MAX_U32)
                {

                    download_map_erase_pipe(p_down_map, erase_index, cur_data_pipe);
                }
                cur_data_pipe->_dispatch_info._correct_error_range = *p_cur_range;
                list_erase(&p_pipe_list->_data_pipe_list, cur_pipe_node);
                return TRUE;
            }
            else
            {
                LOG_DEBUG("ds_assign_correct_range,  data_pipe:0x%x, is failure assigned by error block (%u,%u)", cur_data_pipe, p_cur_range->_index, p_cur_range->_num);
            }
        }
        else
        {
            LOG_DEBUG("ds_assign_correct_range,  data_pipe:0x%x, is in the res_list of error block (%u,%u)", cur_data_pipe, p_cur_range->_index, p_cur_range->_num);
        }
        cur_pipe_node = LIST_NEXT(cur_pipe_node);
    }

    LOG_DEBUG("ds_assign_correct_range false end .");
    return FALSE;
}

BOOL ds_handle_correct_range_using_origin_res(DISPATCHER*  ptr_dispatch_interface
    , ERROR_BLOCK* p_error_block
    , PIPE_LIST* p_long_server_list
    , PIPE_LIST* p_range_server_list
    , PIPE_LIST* p_last_server_list
    , PIPE_LIST* p_peer_pipe_list
    , MAP* p_down_map)
{
    LOG_DEBUG("ds_handle_correct_range_using_origin_res begin.");

    if(p_long_server_list != NULL)
    {
        LOG_DEBUG("ds_handle_correct_range_using_origin_res, using p_long_server_list.");
        if(ds_assign_correct_range_using_origin_res(ptr_dispatch_interface,p_error_block, p_long_server_list, p_down_map) == TRUE)
        {
            LOG_DEBUG("ds_handle_correct_range_using_origin_res, using p_long_server_list success.");
            return TRUE;
        }
    }

    if(p_peer_pipe_list != NULL)
    {
        LOG_DEBUG("ds_handle_correct_range_using_origin_res, using p_peer_pipe_list.");
        if(ds_assign_correct_range_using_origin_res(ptr_dispatch_interface,p_error_block, p_peer_pipe_list, p_down_map) == TRUE)
        {
            LOG_DEBUG("ds_handle_correct_range_using_origin_res, using p_peer_pipe_list success.");
            return TRUE;
        }
    }

    if(p_range_server_list != NULL)
    {
        LOG_DEBUG("ds_handle_correct_range_using_origin_res, using p_range_server_list.");
        if(ds_assign_correct_range_using_origin_res(ptr_dispatch_interface,p_error_block, p_range_server_list, p_down_map) == TRUE)
        {
            LOG_DEBUG("ds_handle_correct_range_using_origin_res, using p_range_server_list success.");
            return TRUE;
        }
    }

    if(p_last_server_list != NULL)
    {
        LOG_DEBUG("ds_handle_correct_range_using_origin_res, using p_last_server_list");
        if(ds_assign_correct_range_using_origin_res(ptr_dispatch_interface,p_error_block, p_last_server_list, p_down_map) == TRUE)
        {
            LOG_DEBUG("ds_handle_correct_range_using_origin_res, using p_last_server_list.");
            return TRUE;
        }
    }

    LOG_DEBUG("ds_handle_correct_range_using_origin_res end.");
    return FALSE;

}

BOOL ds_assign_correct_range_using_origin_res(DISPATCHER*  ptr_dispatch_interface,ERROR_BLOCK* p_error_block, PIPE_LIST* p_pipe_list, MAP* p_down_map)
{
    RANGE* p_cur_range = &p_error_block->_r;

    _u32 erase_index = 0;
    /*LIST* p_cur_error_res_list = &p_error_block->_error_resources;*/

    DATA_PIPE* cur_data_pipe = NULL;
    LIST_ITERATOR cur_pipe_node = LIST_BEGIN(p_pipe_list->_data_pipe_list);

    LOG_DEBUG("ds_assign_correct_range_using_origin_res begin.");

    while(cur_pipe_node != LIST_END(p_pipe_list->_data_pipe_list))
    {
        cur_data_pipe = (DATA_PIPE*)LIST_VALUE(cur_pipe_node);

        if(cur_data_pipe->_dispatch_info._correct_error_range._num == 0
            && ds_dispatch_pipe_is_origin(ptr_dispatch_interface,cur_data_pipe) == TRUE)
        {
            if(ds_assigned_range_to_pipe(cur_data_pipe, p_cur_range) == TRUE)
            {
                /*set assign range for correct block*/
                LOG_DEBUG("ds_assign_correct_range_using_origin_res,  data_pipe:0x%x, is assigned by error block (%u,%u)", cur_data_pipe, p_cur_range->_index, p_cur_range->_num);

                if(p_down_map != NULL)
                {
                    erase_index = cur_data_pipe->_dispatch_info._down_range._index 
                                + cur_data_pipe->_dispatch_info._down_range._num;
                    download_map_erase_pipe(p_down_map, erase_index, cur_data_pipe);
                }

                cur_data_pipe->_dispatch_info._correct_error_range = *p_cur_range;
                list_erase(&p_pipe_list->_data_pipe_list, cur_pipe_node);
                return TRUE;
            }
            else
            {
                LOG_DEBUG("ds_assign_correct_range_using_origin_res,  data_pipe:0x%x, is failure assigned by error block (%u,%u)", cur_data_pipe, p_cur_range->_index, p_cur_range->_num);
            }
        }
        else
        {
            LOG_DEBUG("ds_assign_correct_range_using_origin_res,  data_pipe:0x%x, is in the res_list of error block (%u,%u)", cur_data_pipe, p_cur_range->_index, p_cur_range->_num);
        }
        cur_pipe_node = LIST_NEXT(cur_pipe_node);
    }

    LOG_DEBUG("ds_assign_correct_range_using_origin_res false end .");
    return FALSE;
}


BOOL ds_res_is_include(LIST* res_list, RESOURCE* p_res)
{


    RESOURCE* cur_res = NULL;
    LIST_ITERATOR cur_res_node = NULL;

    LOG_DEBUG("ds_res_is_include");

    if(res_list == NULL || p_res== NULL)
    {
        return FALSE;
    }

    cur_res_node = LIST_BEGIN(*res_list);

    while(cur_res_node != LIST_END(*res_list))
    {
        cur_res = LIST_VALUE(cur_res_node);
        if(cur_res == p_res)
        {
            return TRUE;
        }
        cur_res_node = LIST_NEXT(cur_res_node);
    }

    return FALSE;
}

DATA_PIPE* ds_get_data_pipe( PIPE_LIST* p_long_server_list
                , PIPE_LIST* p_range_server_list
                , PIPE_LIST* p_last_server_list
                , PIPE_LIST* p_peer_pipe_list)
{
    DATA_PIPE* ret_data_pipe =  NULL;

    LOG_DEBUG("ds_get_data_pipe");

    if(p_long_server_list != NULL)
    {
        if(list_size(&p_long_server_list->_data_pipe_list) != 0)
        {
            list_pop(&p_long_server_list->_data_pipe_list, (void**)&ret_data_pipe);
            if(ret_data_pipe != NULL)
            {
                LOG_DEBUG("ds_get_data_pipe using p_long_server_list");
                return ret_data_pipe;
            }
        }
    }

    if(p_peer_pipe_list != NULL)
    {
        if(list_size(&p_peer_pipe_list->_data_pipe_list) != 0)
        {
            list_pop(&p_peer_pipe_list->_data_pipe_list, (void**)&ret_data_pipe);
            if(ret_data_pipe != NULL)
            {
                LOG_DEBUG("ds_get_data_pipe using p_peer_pipe_list");
                return ret_data_pipe;
            }
        }
    }

    if(p_range_server_list != NULL)
    {
        if(list_size(&p_range_server_list->_data_pipe_list) != 0)
        {
            list_pop(&p_range_server_list->_data_pipe_list, (void**)&ret_data_pipe);
            if(ret_data_pipe != NULL)
            {
                LOG_DEBUG("ds_get_data_pipe using p_range_server_list");
                return ret_data_pipe;
            }
        }
    }

    if(p_last_server_list != NULL)
    {
        if(list_size(&p_last_server_list->_data_pipe_list) != 0)
        {
            list_pop(&p_last_server_list->_data_pipe_list, (void**)&ret_data_pipe);
            if(ret_data_pipe != NULL)
            {
                LOG_DEBUG("ds_get_data_pipe using p_last_server_list");
                return ret_data_pipe;
            }
        }
    }


    return   ret_data_pipe;

}

void ds_put_data_pipe( DATA_PIPE* ptr_data_pipe,PIPE_LIST* p_long_server_list,  PIPE_LIST* p_range_server_list, PIPE_LIST* p_last_server_list,
                       PIPE_LIST* p_peer_pipe_list)
{
    DISPATCH_INFO* p_cur_dispatch_info = NULL;
    PIPE_TYPE  data_pipe_type;

    LOG_DEBUG("ds_put_data_pipe");

    if(ptr_data_pipe == NULL)
        return;

    p_cur_dispatch_info = &ptr_data_pipe->_dispatch_info;
    data_pipe_type = ptr_data_pipe->_data_pipe_type;

    if(data_pipe_type == P2P_PIPE)
    {
        if(p_peer_pipe_list != NULL)
        {
            LOG_DEBUG("ds_put_data_pipe using p_peer_pipe_list");

            list_push(&p_peer_pipe_list->_data_pipe_list,ptr_data_pipe);

        }
        return;
    }

    if(p_cur_dispatch_info->_is_support_long_connect == TRUE && p_cur_dispatch_info->_is_support_range == TRUE)
    {
        if(p_long_server_list != NULL)
        {
            LOG_DEBUG("ds_put_data_pipe using p_long_server_list");
            list_push(&p_long_server_list->_data_pipe_list,ptr_data_pipe);

        }
    }
    else if(p_cur_dispatch_info->_is_support_range == TRUE)
    {
        if(p_range_server_list != NULL)
        {
            LOG_DEBUG("ds_put_data_pipe using p_range_server_list");
            list_push(&p_range_server_list->_data_pipe_list,ptr_data_pipe);

        }
    }
    else
    {
        if(p_last_server_list != NULL)
        {
            LOG_DEBUG("ds_put_data_pipe using p_last_server_list");
            list_push(&p_last_server_list->_data_pipe_list,ptr_data_pipe);

        }
    }

    return;
}

/*
    给一个pipe分配range
*/
BOOL ds_assigned_range_to_pipe(DATA_PIPE* ptr_data_pipe, RANGE* ptr_range)
{
    PIPE_TYPE  data_pipe_type;
    DISPATCH_INFO* p_cur_dispatch_info = NULL;
    RANGE_LIST* p_cur_can_down_range = NULL;
    BOOL  cacel_flag = FALSE;

    if(ptr_data_pipe == NULL ||ptr_range == NULL || ptr_range->_num == 0)
    {
        LOG_DEBUG("ds_assigned_range_to_pipe return because invalid parameter");
        return FALSE;
    }

    p_cur_dispatch_info = &ptr_data_pipe->_dispatch_info;
    p_cur_can_down_range = &p_cur_dispatch_info->_can_download_ranges;
    data_pipe_type =   ptr_data_pipe->_data_pipe_type;

    if(p_cur_can_down_range->_node_size != 0)
    {
        if(range_list_is_include(p_cur_can_down_range, ptr_range) == FALSE)
        {
            LOG_DEBUG("ds_assigned_range_to_pipe, assign range(%u,%u) is not in the can down range of data pipe:0x%x .",
                ptr_range->_index, ptr_range->_num, ptr_data_pipe);
            return FALSE;
        }
    }
    else
    {
        LOG_DEBUG("ds_assigned_range_to_pipe, because p_cur_can_down_range is 0, \
                  so assign range(%u,%u) is assign to data pipe:0x%x  .", ptr_range->_index, ptr_range->_num, ptr_data_pipe);
    }

    if(p_cur_dispatch_info->_cancel_down_range == TRUE)
    {
        cacel_flag = TRUE;
        p_cur_dispatch_info->_cancel_down_range = FALSE;
    }

    LOG_DEBUG("ds_assigned_range_to_pipe [dispatch_stat] range %u_%u assign 0x%x (pipe), cancel flag: %u, cur_down_range(%u,%u), _correct_error_range(%u,%u), speed:%u, can_down_range->_node_size:%u.",
        ptr_range->_index, ptr_range->_num, ptr_data_pipe, (_u32)cacel_flag,
        ptr_data_pipe->_dispatch_info._down_range._index,
        ptr_data_pipe->_dispatch_info._down_range._num,
        ptr_data_pipe->_dispatch_info._correct_error_range._index,
        ptr_data_pipe->_dispatch_info._correct_error_range._num,
        ptr_data_pipe->_dispatch_info._speed,
        p_cur_can_down_range->_node_size);
    if(SUCCESS != pi_pipe_change_range(ptr_data_pipe, ptr_range, cacel_flag))
    {
        return FALSE;
    }

    return TRUE;

}

_int32 ds_build_uncomplete_map(MAP* down_map, 
    RANGE_LIST* uncomplete_range_list)
{
    RANGE_LIST_ITEROATOR cur_node =  NULL;
    _u32 cur_index = 0;

    LOG_DEBUG("ds_build_uncomplete_map");

    if(uncomplete_range_list ==  NULL)
        return SUCCESS;

    range_list_get_head_node(uncomplete_range_list, &cur_node);
    while(cur_node != NULL)
    {
        cur_index = cur_node->_range._index;
        LOG_DEBUG("ds_build_uncomplete_map: index: %u true, pipe alloc false ,force true."
            , cur_index + cur_node->_range._num);
        download_map_update_item(down_map, cur_index + cur_node->_range._num, 
            FALSE, TRUE, NULL, TRUE);
        range_list_get_next_node(uncomplete_range_list, cur_node, &cur_node);
    }

    return SUCCESS;
}

_int32 ds_build_pri_range_map(MAP* down_map, RANGE_LIST* pri_range_list)
{
    RANGE_LIST_ITEROATOR cur_node =  NULL;
    _u32 cur_index = 0;

    LOG_DEBUG("ds_build_pri_range_map");

    if(pri_range_list ==  NULL)
        return SUCCESS;

    range_list_get_head_node(pri_range_list, &cur_node);
    while(cur_node != NULL)
    {
        cur_index = cur_node->_range._index;
        LOG_DEBUG("ds_build_pri_range_map: index: %u  false, pipe alloc false ,force false .", cur_index);
        download_map_update_item(down_map, cur_index, FALSE, FALSE, NULL, FALSE);
        range_list_get_next_node(pri_range_list, cur_node, &cur_node);
    }

    return SUCCESS;
}

/*_int32 ds_build_error_map(MAP* down_map, ERROR_BLOCKLIST* p_error_block_list)
{
       ERROR_BLOCK* cur_err_block = NULL;
     _u32 cur_index = 0;
     LIST_ITERATOR cur_node = NULL;

     LOG_DEBUG("ds_build_error_map");

       if(p_error_block_list == NULL)
       {
             return SUCCESS;
       }

       cur_node = LIST_BEGIN(p_error_block_list->_error_block_list);
    while(cur_node != LIST_END(p_error_block_list->_error_block_list))
    {
          cur_err_block = ( ERROR_BLOCK*) LIST_VALUE(cur_node);

          cur_index = cur_err_block->_r._index;
          LOG_DEBUG("ds_build_error_map: index: %u  false, force true .", cur_index);
          download_map_update_item(down_map, cur_index, TRUE, FALSE, NULL, TRUE);
          cur_node = LIST_NEXT(cur_node);
    }
    return SUCCESS;
}*/

void ds_adjust_pipe_list(PIPE_LIST* p_pipe_list)
{

    LIST_ITERATOR l_it = NULL;
    LIST_ITERATOR r_it = NULL;
    LIST_ITERATOR e_it = NULL;
    DATA_PIPE* cur_pipe = NULL;

    if(p_pipe_list == NULL)
        return;

    if(list_size(&p_pipe_list->_data_pipe_list)  == 0)
    {
        return;

    }


    l_it = LIST_BEGIN(p_pipe_list->_data_pipe_list);
    r_it = LIST_RBEGIN(p_pipe_list->_data_pipe_list);

    while(l_it != r_it)
    {
        cur_pipe = (DATA_PIPE* )LIST_VALUE(l_it);


        if(cur_pipe->_dispatch_info._speed == 0)
        {
            e_it  = l_it;
            l_it = LIST_NEXT(l_it);

            list_erase(&p_pipe_list->_data_pipe_list, e_it);
            list_push(&p_pipe_list->_data_pipe_list,cur_pipe);

        }
        else
        {
            l_it = LIST_NEXT(l_it);
        }

    }

    return;
}


_int32 ds_build_pipe_list_map(MAP* down_map
                              , MAP* partial_map
                              , PIPE_LIST* p_pipe_list
                              , RANGE_LIST* uncomplete_range_list
                              , PIPE_LIST* p_long_pipe_list
                              , PIPE_LIST* p_range_pipe_list
                              , PIPE_LIST* p_last_pipe_list
                              , PIPE_LIST* p_new_long_pipe_list
                              , PIPE_LIST* p_new_range_pipe_list
                              , PIPE_LIST* p_new_last_pipe_list
                              , PIPE_LIST* p_partial_pipe_list
                              , RANGE_LIST*  correct_range_list
                              , RANGE_LIST*  p_dowing_range_list
                              , _u32 now_time)
{
    LIST_ITERATOR cur_node = NULL;
    DATA_PIPE* cur_data_pipe = NULL;
    DISPATCH_INFO* p_cur_dispatch_info = NULL;
    RANGE_LIST* p_cur_uncomplete_range = NULL;
    RANGE_LIST* p_cur_can_down_range = NULL;
    _int32 cur_index;
    RANGE* p_cur_range = NULL;
    RANGE* p_assign_range = NULL;
    RANGE assign_r;

    BOOL  _is_no_range = FALSE;

    _u32 head_overlap = 0;

    LOG_DEBUG("ds_build_pipe_list_map, task uncomplete rangelist : ");
    out_put_range_list(uncomplete_range_list);

    if(p_pipe_list == NULL)
        return SUCCESS;


    assign_r._index = 0;
    assign_r._num = 0;

    LOG_DEBUG("ds_build_pipe_list_map, pipe size = %u .",
        list_size(&p_pipe_list->_data_pipe_list));

    // 遍历所有的pipe列表信息
    cur_node = LIST_BEGIN(p_pipe_list->_data_pipe_list);
    while(cur_node != LIST_END(p_pipe_list->_data_pipe_list))
    {
        _is_no_range = FALSE;

        cur_data_pipe = (DATA_PIPE*) LIST_VALUE(cur_node);
		LOG_DEBUG("cur_data_pipe:0x%x, pipe_state:%d, down_range(%d, %d), assign_range(%d, %d), max_speed:%d, speed:%d, score:%d",
			cur_data_pipe,
			cur_data_pipe->_dispatch_info._pipe_state,
			cur_data_pipe->_dispatch_info._down_range._index, cur_data_pipe->_dispatch_info._down_range._num,
                  cur_data_pipe->_dispatch_info._correct_error_range._index, cur_data_pipe->_dispatch_info._correct_error_range._num,
			cur_data_pipe->_dispatch_info._max_speed,
			cur_data_pipe->_dispatch_info._speed,
			cur_data_pipe->_dispatch_info._score);
        LOG_DEBUG("pipe uncomplete range_list = ");
        out_put_range_list(&cur_data_pipe->_dispatch_info._uncomplete_ranges);

        // 如果pipe正在下载
        if(cur_data_pipe->_dispatch_info._pipe_state == PIPE_DOWNLOADING)
        {
            p_cur_dispatch_info = &cur_data_pipe->_dispatch_info;
            p_cur_uncomplete_range = &p_cur_dispatch_info->_uncomplete_ranges;
            p_cur_can_down_range = &p_cur_dispatch_info->_can_download_ranges;
            p_cur_range = &p_cur_dispatch_info->_down_range;
            p_assign_range = &p_cur_dispatch_info->_correct_error_range;
            // 判断正在下载的range是否跟未完成块有重叠
            if((p_assign_range->_num != 0) ||
				(uncomplete_range_list->_node_size == 0))
            {
                head_overlap = 0;
            }
            else
            {
                if(p_cur_range->_num == 0)
                {
                    head_overlap = 2;
                }
                else
                {
                    /*
                         -1 表示参数错误
                         0  表示r 与range_list 中一项在头部重叠
                         1  表示r 与range_list 中一项在尾部重叠
                         2  表示r 与range_list 无重叠
                    */
                    head_overlap = range_list_is_head_relevant2(uncomplete_range_list, p_cur_range);
                }
            }
            LOG_DEBUG("head_overlap = %d", head_overlap );
            // 有下载块，并且该块要在可下载块内
            if(p_cur_range->_num != 0 &&
				(p_cur_can_down_range->_node_size == 0|| range_list_is_include(p_cur_can_down_range, p_cur_range) == TRUE) &&
				(head_overlap != 2))
            {
                if(now_time != 0)
                {
                    // 若pipe接收数据超时，则过期不再使用这个pipe了
                    if(TIME_SUBZ(now_time,
						p_cur_dispatch_info->_last_recv_data_time) > g_max_wait_time * 1000)
                    { // TODO 是否需要清空已分配的下载块 uncompleted_range ？
                        cur_node = LIST_NEXT(cur_node);
                        LOG_ERROR("ds_build_pipe_list_map, data pipe:0x%x\
                            because cur_down range (%u,%u) is over time speed:%u,\
                            time_stamp:%u, so no using this pipe.",
                                  cur_data_pipe, p_cur_range->_index,
                                  p_cur_range->_num, p_cur_dispatch_info->_speed,
                                  p_cur_dispatch_info->_last_recv_data_time);
                        continue;
                    }
                }

                p_cur_dispatch_info->_cancel_down_range = FALSE;

                // 根据这个pipe所支持的不同的能力，将其插入到具体的pipe队列中
                if(p_cur_dispatch_info->_is_support_long_connect == TRUE
                    && p_cur_dispatch_info->_is_support_range == TRUE)
                {
                    // 长连接且支持range的pipe
                    if(p_long_pipe_list != NULL)
                    {
                        LOG_DEBUG("ds_build_pipe_list_map, data pipe :0x%x  \
                                  is push to p_long_pipe_list.", cur_data_pipe);
                        list_push(&p_long_pipe_list->_data_pipe_list,
                            cur_data_pipe);
                    }
                }
                else if(p_cur_dispatch_info->_is_support_range == TRUE)
                {
                    // 所有支持range的pipe
                    if(p_range_pipe_list != NULL)
                    {
                        LOG_DEBUG("ds_build_pipe_list_map, data pipe :0x%x  \
                                  is push to p_range_pipe_list.", cur_data_pipe);
                        list_push(&p_range_pipe_list->_data_pipe_list,
                            cur_data_pipe);
                    }
                }
                else
                {
                    // 所有不支持range的pipe
                    if(p_last_pipe_list != NULL)
                    {
                        LOG_DEBUG("ds_build_pipe_list_map, data pipe :0x%x  \
                                  is push to p_last_pipe_list.", cur_data_pipe);
                        list_push(&p_last_pipe_list->_data_pipe_list,
                            cur_data_pipe);

                    }
                    _is_no_range = TRUE;
                }

                //
                if(correct_range_list != NULL)
                {
                    if(p_assign_range->_num != 0)
                    {
                        // cur_range完全在纠错块里面
                        if((p_cur_range->_index >= p_assign_range->_index)
                            && ((p_cur_range->_index + p_cur_range->_num) <=
                            (p_assign_range->_index + p_assign_range->_num)))
                        {
                            LOG_DEBUG("ds_build_pipe_list_map, data pipe :0x%x \
                                      has correct range (%u,%u).",
                                      cur_data_pipe,
                                      p_assign_range->_index, p_assign_range->_num);
                            range_list_add_range(correct_range_list,
                                p_assign_range, NULL, NULL);
                        }
                        else
                        {
                            p_assign_range->_index = 0;
                            p_assign_range->_num = 0;
                        }
                    }
                }

                if(_is_no_range == FALSE)
                {
                    cur_index = p_cur_range->_index + p_cur_range->_num;

                    LOG_DEBUG("ds_build_pipe_list_map: index: %u  true, \
                              alloc true ,force true .", p_cur_range->_index);

                    // 已经分配到某个pipe上去下载了
                    download_map_update_item(down_map,
                        p_cur_range->_index, TRUE, TRUE, NULL, TRUE);

                    if(head_overlap == 0)
                    {
                        LOG_DEBUG("ds_build_pipe_list_map: index: %u  false, \
                                  alloc true, force false .", cur_index);
                        // 头部重叠，这个pipe优先下载cur_index开始的块
                        download_map_update_item(down_map,
                            cur_index, TRUE, FALSE, cur_data_pipe, FALSE);
                    }
                    else
                    {
                        range_list_clear(p_cur_uncomplete_range);
                    }
                }


                if(p_assign_range->_num == 0 && p_cur_range->_num != 0)
                {
                    LOG_DEBUG("ds_build_pipe_list_map: range (%u,%u) is put \
                              into  dowing range.", p_cur_range->_index, p_cur_range->_num);
                    range_list_add_range(p_dowing_range_list, p_cur_range, NULL, NULL);
                }

            }
            else
            {
                if( p_cur_range->_num != 0)
                {
                    p_cur_dispatch_info->_cancel_down_range = TRUE;
                }
                else
                {
                    p_cur_dispatch_info->_cancel_down_range = FALSE;
                }

                range_list_clear(p_cur_uncomplete_range);

                if(p_assign_range->_num != 0)
                {
                    LOG_DEBUG("ds_build_pipe_list_map, data pipe :0x%x  \
                              has correct range (%u,%u), but because cur_down \
                              range (%u,%u) is not in the range , so dop it.",
                              cur_data_pipe,
                              p_assign_range->_index, p_assign_range->_num,
                              p_cur_range->_index, p_cur_range->_num);
                    p_assign_range->_num = 0;
                    p_assign_range->_index = 0;
                }

                if( p_cur_dispatch_info->_cancel_down_range == TRUE)
                {
                    if(cur_data_pipe->_data_pipe_type == P2P_PIPE)
                    {
                        assign_r._index = 0;
                        assign_r._num = 0;
                        p_cur_dispatch_info->_cancel_down_range = FALSE;
                        p2p_pipe_change_ranges(cur_data_pipe, &assign_r, TRUE);
                        LOG_DEBUG("ds_build_pipe_list_map, data pipe :0x%x   \
                                  because cur_down range (%u,%u) is not in the range \
                                  , so cancel it .",
                                  cur_data_pipe,
                                  p_cur_range->_index, p_cur_range->_num);

                        cur_node = LIST_NEXT(cur_node);
                        continue;
                    }
                }

                // 部分可下载，边下边传 partial_pipe_list
                if(range_list_is_contained2(p_cur_can_down_range, uncomplete_range_list) == FALSE)
                {
                    if(p_partial_pipe_list != NULL)
                    {
                        LOG_DEBUG("ds_build_pipe_list_map, data pipe :0x%x  \
                                  is push to p_partial_pipe_list.", cur_data_pipe);
                        list_push(&p_partial_pipe_list->_data_pipe_list,
                            cur_data_pipe);
                    }

                    cur_node = LIST_NEXT(cur_node);
                    continue;
                }

                if( p_cur_dispatch_info->_is_support_long_connect == TRUE
                    && p_cur_dispatch_info->_is_support_range == TRUE )
                {
                    if(p_new_long_pipe_list != NULL)
                    {
                        LOG_DEBUG("ds_build_pipe_list_map, data pipe :0x%x  \
                                  is push to p_new_long_pipe_list.", cur_data_pipe);
                        list_push(&p_new_long_pipe_list->_data_pipe_list,
                            cur_data_pipe);
                    }
                }
                else if(p_cur_dispatch_info->_is_support_range == TRUE)
                {
                    if(p_new_range_pipe_list != NULL)
                    {
                        LOG_DEBUG("ds_build_pipe_list_map, data pipe :0x%x  \
                                  is push to p_new_range_pipe_list.", cur_data_pipe);
                        list_push(&p_new_range_pipe_list->_data_pipe_list,
                            cur_data_pipe);
                    }
                }
                else
                {
                    // 既不支持长连接又不支持range
                    if(p_new_last_pipe_list != NULL)
                    {
                        LOG_DEBUG("ds_build_pipe_list_map, data pipe :0x%x  \
                                  is push to p_new_last_pipe_list.", cur_data_pipe);
                        list_push(&p_new_last_pipe_list->_data_pipe_list,
                            cur_data_pipe);
                    }
                }
            }
        }
        else
        {
			// NOTE：这里的判断存在问题，因为此时p_cur_range为空并不一定表示该pipe来自new资源？
            LOG_DEBUG("ds_build_pipe_list_map, data pipe :0x%x  is not dowing  \
                status : %u, so no using dispatcher.",
                cur_data_pipe, cur_data_pipe->_dispatch_info._pipe_state);
        }
        cur_node = LIST_NEXT(cur_node);
    }

    return SUCCESS;
}

_int32 ds_dispatcher_timer_handler(const MSG_INFO *msg_info, 
    _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid)
{
    _u32 time_id = msg_info->_device_id;
    DISPATCHER* ptr_dispatcher = (DISPATCHER*)msg_info->_user_data;

    LOG_DEBUG("ds_dispatcher_timer_handler");

    if(errcode == MSG_CANCELLED)
    {
        LOG_DEBUG("ds_dispatcher_timer_handler, time id: %u, dispatcher:0x%x   \
                  return because canceled", time_id, ptr_dispatcher);
        return SUCCESS;
    }

    if(errcode == MSG_TIMEOUT && 
       (time_id == DISPATCHER_TIMER || time_id == DISPATCHER_ONCE_TIMER))
    {
        LOG_DEBUG("ds_dispatcher_timer_handler, time id: %u, dispatcher:0x%x   do dispatcher.", time_id, ptr_dispatcher);
        if(time_id == DISPATCHER_ONCE_TIMER)
        {
            ptr_dispatcher->_immediate_dispatcher_timer = INVALID_MSG_ID;
        }

        ds_do_dispatch(ptr_dispatcher, FALSE);
    }
    else
    {
        LOG_ERROR("ds_dispatcher_timer_handler, error callback return value, errcode:%u, time id: %d, dispatcher:0x%x   do dispatcher.", errcode, time_id, ptr_dispatcher);
    }

    return SUCCESS;
}


/*
    针对每一个pipe进行调度
*/
_int32 ds_dispatch_at_pipe_normal(DISPATCHER*  ptr_dispatch_interface, 
    DATA_PIPE*  ptr_data_pipe)
{
    _u64 file_size = DS_GET_FILE_SIZE(ptr_dispatch_interface->_data_interface);
    RANGE_LIST uncomplete_range_list;
    RANGE_LIST pri_range_list;
    RANGE_LIST err_range_list;

    RANGE_LIST * dispatch_range_list = NULL;

    RANGE_LIST_ITEROATOR rand_range_node = NULL;
    RANGE_LIST_ITEROATOR lasted_range_node = NULL;
    DISPATCH_INFO* cur_dispatch_info = NULL;
    RANGE assign_range;

    RANGE_LIST_ITEROATOR first_range_node = NULL;

    ERROR_BLOCKLIST* p_error_block_list = NULL;
    _int32 ret_val = SUCCESS;

    BOOL is_from_tmp_range;

    LOG_DEBUG("ds_dispatch_at_pipe, ptr_dispatch_interface:0x%x , ptr_data_pipe: 0x%x .", 
        ptr_dispatch_interface, ptr_data_pipe);

    assign_range._index = 0;
    assign_range._num = 0;

    if(ptr_data_pipe == NULL || file_size == 0)
    {
        LOG_DEBUG("ds_dispatch_at_pipe, ptr_dispatch_interface:0x%x , ptr_data_pipe: 0x%x\
            , because filesize:%llu is 0, so return.",
            ptr_dispatch_interface, ptr_data_pipe, file_size);
        return  SUCCESS;
    }

    LOG_DEBUG("ds_dispatch_at_pipe, ptr_dispatch_interface:0x%x , ptr_data_pipe: 0x%x , \
        get filesize :%llu.", ptr_dispatch_interface, ptr_data_pipe, file_size);
        
    cur_dispatch_info = &ptr_data_pipe->_dispatch_info;

    LOG_DEBUG("ds_dispatch_at_pipe, ptr_dispatch_interface:0x%x , ptr_data_pipe: 0x%x , \
        because dis parameter down_range(%u,%u) , uncomplete rangelist size :%u .",
        ptr_dispatch_interface, ptr_data_pipe, cur_dispatch_info->_down_range._index, 
        cur_dispatch_info->_down_range._num, 
        cur_dispatch_info->_uncomplete_ranges._node_size);

    if(cur_dispatch_info->_pipe_state == PIPE_FAILURE 
       || cur_dispatch_info->_is_support_range == FALSE)
    {
        LOG_DEBUG("ds_dispatch_at_pipe, ptr_dispatch_interface:0x%x , ptr_data_pipe: 0x%x\
            ,because dis parameter pipe state:%u, is_support_range:%u ,is invalid, so return.",
            ptr_dispatch_interface, ptr_data_pipe, 
            cur_dispatch_info->_pipe_state, cur_dispatch_info->_is_support_range);
        return  SUCCESS;
    }

    DS_GET_ERROR_RANGE_BLOCK_LIST(ptr_dispatch_interface->_data_interface, 
        &p_error_block_list);
    
    range_list_init(&uncomplete_range_list);
    range_list_init(&pri_range_list);
    range_list_init(&err_range_list);

    ret_val = DS_GET_PRIORITY_RANGE(ptr_dispatch_interface->_data_interface, 
        &pri_range_list);
    // 无优先块需要下载
    if(pri_range_list._node_size == 0)
    {
        // 这根本不会走到，难道没有优先块去下载吗
        if(ret_val == 1)
        {
            LOG_DEBUG("[DS_GET_PRIORITY_RANGE] ret_val == 1, start to dispatcher.");
            ds_start_dispatcher_immediate(ptr_dispatch_interface);
            ptr_dispatch_interface->_start_index = MAX_U32;
            ptr_dispatch_interface->_start_index_time_ms = MAX_U32;
        }

        //ret_val = dm_get_uncomplete_range(ptr_dispatch_interface->_p_data_manager, &uncomplete_range_list);
        ret_val = DS_GET_UNCOMPLETE_RANGE(ptr_dispatch_interface->_data_interface, 
                                          &uncomplete_range_list);
        if(ret_val == 1)
        {
            LOG_DEBUG("[DS_GET_UNCOMPLETE_RANGE] ret_val == 1, start to dispatcher.");
            ds_start_dispatcher_immediate(ptr_dispatch_interface);
            ptr_dispatch_interface->_start_index = MAX_U32;
            ptr_dispatch_interface->_start_index_time_ms = MAX_U32;
        }

        dispatch_range_list = &uncomplete_range_list;
    }
    else
    {
        dispatch_range_list = &pri_range_list;
    }


    // BT任务独立处理	
#ifndef DISPATCH_RANDOM_MODE
    if(!cm_is_bttask(ptr_dispatch_interface->_p_connect_manager))
    {
   	 // 过滤到32M
       ds_filter_range_list_from_begin(dispatch_range_list, SECTION_UNIT*1024/16);
    }
#endif

    if(dispatch_range_list->_node_size == 0)
    {
        LOG_DEBUG("ds_dispatch_at_pipe, ptr_dispatch_interface:0x%x , ptr_data_pipe: 0x%x \
            , uncompleted is 0, so return.", ptr_dispatch_interface, ptr_data_pipe);
        goto  pipe_dis_done;
    }

    if(p_error_block_list != NULL 
       && list_size(&p_error_block_list->_error_block_list) != 0)
    {
        // 有错误块需要下载
        get_error_block_range_list(p_error_block_list, &err_range_list);
        if(err_range_list._node_size != 0)
        {
            // 将错误块从调度块中移除
            range_list_delete_range_list(dispatch_range_list, &err_range_list);

            // 调度块为0，为么会去调度
            // 未完成块的个数为0，则该pipe完成了下载，再次请求去调度自己
            if(dispatch_range_list->_node_size == 0)
            {
                LOG_DEBUG("ds_dispatch_at_pipe, ptr_dispatch_interface:0x%x , \
                    ptr_data_pipe: 0x%x , only correct range is left, so return.",
                    ptr_dispatch_interface, ptr_data_pipe);
                ds_start_dispatcher_immediate(ptr_dispatch_interface);
                goto  pipe_dis_done;
            }
        }
    }

    out_put_range_list(&ptr_dispatch_interface->_tmp_range_list);
    out_put_range_list(&ptr_dispatch_interface->_overalloc_range_list);

    // _tmp_range_list当前正在下载的块
    range_list_adjust(&ptr_dispatch_interface->_tmp_range_list, dispatch_range_list);
    // _overalloc_range_list重复分配的块，即当超过一定的下载间隔，重新给这个pipe分配的块
    range_list_adjust(&ptr_dispatch_interface->_overalloc_range_list, dispatch_range_list);
    range_list_add_range_list(&ptr_dispatch_interface->_tmp_range_list, 
        &ptr_dispatch_interface->_overalloc_range_list);

    out_put_range_list(&ptr_dispatch_interface->_tmp_range_list);
    out_put_range_list(&ptr_dispatch_interface->_overalloc_range_list);

    if(cur_dispatch_info->_is_support_long_connect == FALSE)
    {
        if(cur_dispatch_info->_down_range._num != 0)
        {
            LOG_DEBUG("ds_dispatch_at_pipe, ptr_dispatch_interface:0x%x , \
                ptr_data_pipe: 0x%x , dowing range (%u, %u)  is not support long connect .",
                ptr_dispatch_interface, ptr_data_pipe, 
                cur_dispatch_info->_down_range._index, 
                cur_dispatch_info->_down_range._num);

            // 找紧挨着当前下载的那一块
            range_list_get_lasted_node(dispatch_range_list, 
                cur_dispatch_info->_down_range._index + cur_dispatch_info->_down_range._num, 
                &lasted_range_node);
            if(lasted_range_node != NULL)
            {
                // 下紧挨着的一块
                if((lasted_range_node->_range._index 
                   >= cur_dispatch_info->_down_range._num + cur_dispatch_info->_down_range._index)
                   && (lasted_range_node->_range._index 
                       <= cur_dispatch_info->_down_range._num + cur_dispatch_info->_down_range._index + 1))
                {
                    assign_range._index 
                        = cur_dispatch_info->_down_range._num + cur_dispatch_info->_down_range._index;
                    assign_range._num 
                        = lasted_range_node->_range._index + lasted_range_node->_range._num - assign_range._index;
                }

                LOG_DEBUG("ds_dispatch_at_pipe, ptr_dispatch_interface:0x%x , \
                          ptr_data_pipe: 0x%x , dowing range (%u, %u)  is not support \
                          long connect , get lasted range (u,%u).",
                          ptr_dispatch_interface, ptr_data_pipe, 
                          cur_dispatch_info->_down_range._index, 
                          cur_dispatch_info->_down_range._num,
                          lasted_range_node->_range._index, lasted_range_node->_range._num);
            }
        }
    }
    else
    {
        if(cur_dispatch_info->_down_range._num != 0)
        {
            LOG_DEBUG("ds_dispatch_at_pipe, ptr_dispatch_interface:0x%x , \
                ptr_data_pipe: 0x%x ,  support long connect , dowing range (%u, %u)  \
                is not zero so not alloc now .",
                ptr_dispatch_interface, ptr_data_pipe, 
                cur_dispatch_info->_down_range._index, 
                cur_dispatch_info->_down_range._num);

            goto pipe_dis_done;
        }
    }

    is_from_tmp_range = FALSE;

    if(assign_range._num != 0 
       && ds_assigned_range_to_pipe(ptr_data_pipe, &assign_range) == TRUE)
    {
        // pipe上已经有任务了，那就去下载吧
        LOG_DEBUG("ds_dispatch_at_pipe, 1 assign range (%u, %u) to data pipe: 0x%x",
                  assign_range._index, assign_range._num, ptr_data_pipe);
    }
    else
    {
        // 否则还需要申请下载任务
        LOG_DEBUG("ds_dispatch_at_pipe,  failure to assign range (%u, %u) \
            to data pipe: 0x%x", assign_range._index, assign_range._num, ptr_data_pipe);

        // 如果有可下载的块
        if(ptr_data_pipe->_dispatch_info._can_download_ranges._node_size != 0)
        {
            // 第一次给这个pipe分配数据下载
            if(ptr_dispatch_interface->_start_index == MAX_U32)
            {
                // 取未完成块的第一块，看是否需要下载
                range_list_get_head_node(dispatch_range_list, &first_range_node);
                if(first_range_node != NULL)
                {
                    if(range_list_is_include(
                            &ptr_data_pipe->_dispatch_info._can_download_ranges, 
                            &first_range_node->_range) == TRUE)
                    {   // 1, range包含在可下载块中，分配给pipe去下载就行了
#ifdef LITTLE_FILE_TEST
                        RANGE little_file_assign_range = {0};
                        if(first_range_node->_range._num>3 &&ds_should_use_little_file_assign_range(ptr_data_pipe))
                        {
                            little_file_assign_range._index = first_range_node->_range._index+(first_range_node->_range._num/2);
                            little_file_assign_range._num = first_range_node->_range._num-(little_file_assign_range._index-first_range_node->_range._index);
                            LOG_DEBUG( " enter[0x%X] should_use_little_file_assign_range:[%u,%u]->[%u,%u]",ptr_data_pipe, first_range_node->_range._index, first_range_node->_range._num, little_file_assign_range._index, little_file_assign_range._num );
                            if(ds_assigned_range_to_pipe(ptr_data_pipe, &little_file_assign_range) == TRUE)
                            {
                                ptr_dispatch_interface->_start_index =  little_file_assign_range._index;
                                LOG_DEBUG("ds_dispatch_at_pipe,  4.1 assign range (%u, %u) to data pipe: 0x%x", little_file_assign_range._index, little_file_assign_range._num, ptr_data_pipe);
                                goto pipe_dis_done;
                            }
                        }
#endif
                        if(ds_assigned_range_to_pipe(ptr_data_pipe, 
                                &first_range_node->_range) == TRUE)
                        {
                            ptr_dispatch_interface->_start_index 
                                = first_range_node->_range._index;
                            LOG_DEBUG("ds_dispatch_at_pipe, 4 assign range (%u, %u) \
                                to data pipe: 0x%x",
                                      first_range_node->_range._index,
                                      first_range_node->_range._num, ptr_data_pipe);
                            goto pipe_dis_done;
                        }
                    }
                    else
                    {
                        // 2, 不在可下载块内，什么情况，
                        // 2.1, 是否有重复的部分，有的话，分配pipe去下载这块重复
                        assign_range
                            = range_list_get_first_overlap_range(
                                  &ptr_data_pipe->_dispatch_info._can_download_ranges,
                                  &first_range_node->_range);
                        if(assign_range._num != 0
                                && assign_range._index == first_range_node->_range._index)
                        {
                            if(ds_assigned_range_to_pipe(ptr_data_pipe, &assign_range) == TRUE)
                            {
                                ptr_dispatch_interface->_start_index = assign_range._index;
                                LOG_DEBUG("ds_dispatch_at_pipe,  5 assign range (%u, %u) to data pipe: 0x%x", assign_range._index, assign_range._num, ptr_data_pipe);
                                goto pipe_dis_done;
                            }
                        }
                    }
                }
            }

            if(range_list_is_contained2(
                &ptr_data_pipe->_dispatch_info._can_download_ranges, 
                dispatch_range_list) == TRUE)
            {
                range_list_delete_range_list(dispatch_range_list, 
                    &ptr_dispatch_interface->_tmp_range_list);
                if(dispatch_range_list->_node_size == 0)
                {
                    range_list_delete_range_list(&ptr_dispatch_interface->_tmp_range_list, 
                        &ptr_dispatch_interface->_overalloc_range_list);
                    dispatch_range_list = &ptr_dispatch_interface->_tmp_range_list;
                    range_list_get_rand_node(dispatch_range_list, &rand_range_node);
                    is_from_tmp_range = TRUE;
                }
                else
                {

                    // range_list_get_rand_node(dispatch_range_list, &rand_range_node);
                    range_list_get_max_node(dispatch_range_list, &rand_range_node);
                }
                if(rand_range_node == NULL)
                {
                    LOG_DEBUG("ds_dispatch_at_pipe, ptr_dispatch_interface:0x%x , ptr_data_pipe: 0x%x , 1 can not get rand range node of uncomplete_range_list, so return.", ptr_dispatch_interface, ptr_data_pipe);

                    goto  pipe_dis_done;
                }

                LOG_DEBUG("ds_dispatch_at_pipe, ptr_dispatch_interface:0x%x , ptr_data_pipe: 0x%x , 1 get rand range node (%u, %u)  of uncomplete_range_list .",
                          ptr_dispatch_interface, ptr_data_pipe, rand_range_node->_range._index, rand_range_node->_range._num);

                if(is_from_tmp_range != TRUE)
                {
                    assign_range._num = rand_range_node->_range._num/2;
                    if(assign_range._num < 1)
                    {
                        assign_range._index = rand_range_node->_range._index ;
                        assign_range._num = rand_range_node->_range._num ;
                    }
                    else
                    {
                        // 取后一部分去下载
                        assign_range._index 
                            = rand_range_node->_range._index 
                              + rand_range_node->_range._num 
                              - rand_range_node->_range._num / 2;
                    }
                }
                else
                {
                    // 随机一块正在下载，但未重复分配的块去下载
                    if(rand_range_node->_range._num < 2)
                    {
                        assign_range._index = rand_range_node->_range._index;
                        assign_range._num = 1;
                    }
                    else
                    {
                        assign_range._index 
                            = rand_range_node->_range._index + sd_rand()%rand_range_node->_range._num;
                        assign_range._num = 1;
                    }
                }
            }
            else
            {
                get_range_list_overlap_list(
                    &ptr_data_pipe->_dispatch_info._can_download_ranges,
                    dispatch_range_list, 
                    &err_range_list);

                range_list_delete_range_list(&err_range_list, 
                    &ptr_dispatch_interface->_tmp_range_list);
                if(err_range_list._node_size == 0)
                {
                    get_range_list_overlap_list(&ptr_data_pipe->_dispatch_info._can_download_ranges,dispatch_range_list, &err_range_list) ;
                    range_list_delete_range_list(&err_range_list, &ptr_dispatch_interface->_overalloc_range_list);
                    range_list_get_rand_node(&err_range_list, &rand_range_node);
                    is_from_tmp_range = TRUE;
                }
                else
                {
                    range_list_get_max_node(&err_range_list, &rand_range_node);
                }

                if(rand_range_node == NULL)
                {
                    LOG_DEBUG("ds_dispatch_at_pipe, ptr_dispatch_interface:0x%x , \
                        ptr_data_pipe: 0x%x , 2 can not get rand range node \
                        of uncomplete_range_list, so return.",
                        ptr_dispatch_interface, ptr_data_pipe);
                    goto  pipe_dis_done;
                }

                LOG_DEBUG("ds_dispatch_at_pipe, ptr_dispatch_interface:0x%x , \
                    ptr_data_pipe: 0x%x , 2 get rand range node (%u, %u)  \
                    of uncomplete_range_list .",
                    ptr_dispatch_interface, ptr_data_pipe, 
                    rand_range_node->_range._index, rand_range_node->_range._num);

                if( is_from_tmp_range != TRUE)
                {
                    assign_range._num = rand_range_node->_range._num/2;
                    if(assign_range._num < 1)
                    {
                        assign_range._index = rand_range_node->_range._index ;
                        assign_range._num = rand_range_node->_range._num ;
                    }
                    else
                    {
                        assign_range._index = rand_range_node->_range._index + rand_range_node->_range._num - rand_range_node->_range._num/2;
                        assign_range._num = rand_range_node->_range._num/2;
                    }
                }
                else
                {
                    if(rand_range_node->_range._num < 2)
                    {
                        assign_range._index = rand_range_node->_range._index;
                        assign_range._num = 1;
                    }
                    else
                    {
                        assign_range._index = rand_range_node->_range._index + sd_rand()%rand_range_node->_range._num;
                        assign_range._num = 1;
                    }
                }
                
            }

            if(assign_range._num != 0 
               && ds_assigned_range_to_pipe(ptr_data_pipe, &assign_range) == TRUE)
            {
                LOG_DEBUG("ds_dispatch_at_pipe,  2 assign range (%u, %u) \
                    to data pipe: 0x%x",
                          assign_range._index, assign_range._num, ptr_data_pipe);
                assign_range._num = 1;
                if(is_from_tmp_range != TRUE)
                {
                    // _tmp_range_list 存储了当前所有pipe上分配的起始range
                    range_list_add_range(&ptr_dispatch_interface->_tmp_range_list, 
                        &assign_range, NULL, NULL);
                }
                else
                {
                }
            }
            else
            {
                LOG_DEBUG("ds_dispatch_at_pipe,  failure to assign range (%u, %u) to data pipe: 0x%x", assign_range._index, assign_range._num, ptr_data_pipe);
            }

        }
    }


pipe_dis_done:

    range_list_clear(&uncomplete_range_list);
    range_list_clear(&pri_range_list);
    range_list_clear(&err_range_list);
    return SUCCESS;
}

_int32 ds_dispatch_at_pipe_vod(DISPATCHER*  ptr_dispatch_interface, DATA_PIPE*  ptr_data_pipe)
{

    /*get uncomplete range and build uncomplete range map*/
    //     _u64 file_size = dm_get_file_size(ptr_dispatch_interface->_p_data_manager);
    _u64 file_size = DS_GET_FILE_SIZE(ptr_dispatch_interface->_data_interface);
    //  RANGE full_r = pos_length_to_range(0, file_size, file_size);
    RANGE_LIST uncomplete_range_list;
    RANGE_LIST pri_range_list;
    RANGE_LIST err_range_list;
    RANGE_LIST * dispatch_range_list = NULL;
    RANGE_LIST_ITEROATOR rand_range_node = NULL;
    RANGE_LIST_ITEROATOR lasted_range_node = NULL;
    DISPATCH_INFO* cur_dispatch_info = NULL;
    RANGE assign_range;
    RANGE_LIST_ITEROATOR first_range_node = NULL;
    ERROR_BLOCKLIST* p_error_block_list = NULL;
    _int32 ret_val = SUCCESS;
    // 判断该链接是否低速，针对低速连接则分块是从dispatch_range_list后面考虑的。
    BOOL is_slow_peer = FALSE;
    _u32 now_time_ms;
    BOOL has_priority_range = FALSE, is_cdn_pipe = FALSE;

    LOG_DEBUG("ds_dispatch_at_pipe_vod, ptr_dispatch_interface:0x%x , ptr_data_pipe: 0x%x, start_index:%u, start_time:%u .", ptr_dispatch_interface, ptr_data_pipe, ptr_dispatch_interface->_start_index, ptr_dispatch_interface->_start_index_time_ms);

    assign_range._index = 0;
    assign_range._num = 0;
    if(ptr_data_pipe == NULL || file_size == 0)
    {
        LOG_ERROR("ds_dispatch_at_pipe_vod, ptr_dispatch_interface:0x%x , ptr_data_pipe: 0x%x , because filesize:%llu is 0, so return.", ptr_dispatch_interface, ptr_data_pipe, file_size);
        return  SUCCESS;
    }
    LOG_DEBUG("ds_dispatch_at_pipe_vod, ptr_dispatch_interface:0x%x , ptr_data_pipe: 0x%x, get filesize:%llu.", ptr_dispatch_interface, ptr_data_pipe, file_size);
    cm_update_data_pipe_speed( ptr_data_pipe );
    sd_time_ms(&now_time_ms);
    cur_dispatch_info = &ptr_data_pipe->_dispatch_info;
    LOG_DEBUG("ds_dispatch_at_pipe_vod, ptr_dispatch_interface:0x%x , ptr_data_pipe: 0x%x , because dis parameter down_range(%u,%u) , uncomplete rangelist size :%u . speed:%u ,pipe_num:%u .",
        ptr_dispatch_interface, ptr_data_pipe, cur_dispatch_info->_down_range._index, cur_dispatch_info->_down_range._num,
        cur_dispatch_info->_uncomplete_ranges._node_size, cur_dispatch_info->_speed, ptr_dispatch_interface->_cur_down_pipes);

    if((get_resource_level(ptr_data_pipe->_p_resource) == MAX_RES_LEVEL))  /* 原始资源或CDN资源 */
    {
        if(cur_dispatch_info->_speed < FAST_SPEED_LEVEL_FOR_VOD + 1)
        { // 目的是为了尝试给优质资源分配更多块。实际意义有待研究？
            cur_dispatch_info->_speed = FAST_SPEED_LEVEL_FOR_VOD * 2;
        }
        is_cdn_pipe = TRUE;
    }

    if(cur_dispatch_info->_speed > FAST_SPEED_LEVEL_FOR_VOD)
    {
        is_slow_peer = FALSE;
    }
    else
    {
        /************ zyq add for testing @20101215 *******************/
        if((get_resource_level(ptr_data_pipe->_p_resource) == MAX_RES_LEVEL)  /* 原始资源或CDN资源 */
            &&(ptr_data_pipe->_p_resource->_last_failture_time == 0)      /* 从来没有失败过*/
            &&(ptr_data_pipe->_p_resource->_max_speed == 0))              /* 之前没有下载过数据*/
        {   /* 比如任务刚开始 */
            is_slow_peer = FALSE;
        }
        else
            /************ zyq add end *******************/
        {
            is_slow_peer = TRUE;
        }
    }
    if(cur_dispatch_info->_pipe_state == PIPE_FAILURE || cur_dispatch_info->_is_support_range == FALSE)
    {
        LOG_ERROR("ds_dispatch_at_pipe_vod, ptr_dispatch_interface:0x%x , ptr_data_pipe: 0x%x , because dis parameter pipe state:%u, is_support_range:%u ,is invalid, so return .",
            ptr_dispatch_interface, ptr_data_pipe, cur_dispatch_info->_pipe_state, cur_dispatch_info->_is_support_range);
        return  SUCCESS;
    }

    range_list_init(&uncomplete_range_list);
    range_list_init(&pri_range_list);
    range_list_init(&err_range_list);
    ret_val = DS_GET_PRIORITY_RANGE_VOD(ptr_dispatch_interface->_data_interface, &pri_range_list);
    //dm_ds_intereface_get_priority_range 获取优先range
    if(pri_range_list._node_size == 0)
    {
#ifdef _VOD_NO_DISK
        return SUCCESS; //如果是无盘点播就直接返回，在没有优先块的情况下无需调度
#endif
        /*get uncomplet range list*/
        //range_list_add_range(&uncomplete_range_list, &full_r, NULL,NULL);
        if(ret_val  == 1 )
        {
            ds_start_dispatcher_immediate(ptr_dispatch_interface);
            //goto  pipe_dis_done;
            ptr_dispatch_interface->_start_index = MAX_U32;
            ptr_dispatch_interface->_start_index_time_ms = MAX_U32;
        }
        //ret_val = dm_get_uncomplete_range(ptr_dispatch_interface->_p_data_manager, &uncomplete_range_list);
        ret_val = DS_GET_UNCOMPLETE_RANGE(ptr_dispatch_interface->_data_interface, &uncomplete_range_list);
        if(ret_val  == 1 )
        {
            ds_start_dispatcher_immediate(ptr_dispatch_interface);
            //goto  pipe_dis_done;
            ptr_dispatch_interface->_start_index = MAX_U32;
            ptr_dispatch_interface->_start_index_time_ms = MAX_U32;
        }

        dispatch_range_list = &uncomplete_range_list;
    }
    else
    {
        dispatch_range_list = &pri_range_list;
        has_priority_range = TRUE;
    }
#ifndef DISPATCH_RANDOM_MODE
    // BT任务独立处理
    if(!cm_is_bttask(ptr_dispatch_interface->_p_connect_manager))
    {
        ds_filter_range_list_from_begin(dispatch_range_list, SECTION_UNIT*1024/16);
    }
#endif

    //要下的现在在dispatch_range_list 里
    if(dispatch_range_list->_node_size == 0)
    {
        LOG_ERROR("ds_dispatch_at_pipe_vod, ptr_dispatch_interface:0x%x , ptr_data_pipe: 0x%x , uncompleted is 0, so return.", ptr_dispatch_interface, ptr_data_pipe);
        goto  pipe_dis_done;
    }

    DS_GET_ERROR_RANGE_BLOCK_LIST(ptr_dispatch_interface->_data_interface, &p_error_block_list);
    if(p_error_block_list != NULL && list_size(&p_error_block_list->_error_block_list) != 0)
    {
        get_error_block_range_list(p_error_block_list, &err_range_list);
        if(err_range_list._node_size != 0)
        {
            range_list_delete_range_list(dispatch_range_list, &err_range_list);
            if(dispatch_range_list->_node_size == 0)
            {
                LOG_ERROR("ds_dispatch_at_pipe_vod, ptr_dispatch_interface:0x%x , ptr_data_pipe: 0x%x , only correct range is left, so return.", ptr_dispatch_interface, ptr_data_pipe);
                ds_start_dispatcher_immediate(ptr_dispatch_interface);
                goto  pipe_dis_done;
            }
        }
    }

    range_list_adjust(&ptr_dispatch_interface->_tmp_range_list, dispatch_range_list);
    range_list_adjust(&ptr_dispatch_interface->_overalloc_range_list, dispatch_range_list);
    range_list_adjust(&ptr_dispatch_interface->_cur_down_point, dispatch_range_list);
    range_list_add_range_list(&ptr_dispatch_interface->_tmp_range_list, &ptr_dispatch_interface->_overalloc_range_list);

    LOG_DEBUG("[vod_dispatch_analysis]print dispatch_range_list");
    out_put_range_list(dispatch_range_list);
    LOG_DEBUG("[vod_dispatch_analysis]print _tmp_range_list");
    out_put_range_list(&ptr_dispatch_interface->_tmp_range_list);
    LOG_DEBUG("[vod_dispatch_analysis]print _overalloc_range_list");
    out_put_range_list(&ptr_dispatch_interface->_overalloc_range_list);
    LOG_DEBUG("[vod_dispatch_analysis]print _cur_down_point");
    out_put_range_list(&ptr_dispatch_interface->_cur_down_point);

    if(cur_dispatch_info->_is_support_long_connect == FALSE)
    {
        if(cur_dispatch_info->_down_range._num != 0)
        {
            LOG_DEBUG("ds_dispatch_at_pipe_vod, ptr_dispatch_interface:0x%x , ptr_data_pipe: 0x%x , dowing range (%u, %u)  is not support long connect .",
                ptr_dispatch_interface, ptr_data_pipe, cur_dispatch_info->_down_range._index, cur_dispatch_info->_down_range._num);
            range_list_get_lasted_node(dispatch_range_list,  cur_dispatch_info->_down_range._index +  cur_dispatch_info->_down_range._num, &lasted_range_node);
            if(lasted_range_node != NULL)
            {
                if(lasted_range_node->_range._index >= cur_dispatch_info->_down_range._num + cur_dispatch_info->_down_range._index
                    && lasted_range_node->_range._index <= cur_dispatch_info->_down_range._num + cur_dispatch_info->_down_range._index + 1)
                {
                    assign_range._index = cur_dispatch_info->_down_range._num + cur_dispatch_info->_down_range._index;
                    assign_range._num = lasted_range_node->_range._index + lasted_range_node->_range._num - assign_range._index;
                }
                LOG_ERROR("ds_dispatch_at_pipe_vod, ptr_dispatch_interface:0x%x , ptr_data_pipe: 0x%x , dowing range (%u, %u)  is not support long connect , get lasted range (u,%u).",
                    ptr_dispatch_interface, ptr_data_pipe, cur_dispatch_info->_down_range._index, cur_dispatch_info->_down_range._num,
                    lasted_range_node->_range._index, lasted_range_node->_range._num);
            }
        }
    }
    else
    {
        if(cur_dispatch_info->_down_range._num != 0)
            //if (cur_dispatch_info->_uncomplete_ranges._node_size != 0)
        {
            // TODO优化：若添加悬挂块逻辑，需要从这里考虑。
            LOG_ERROR("ds_dispatch_at_pipe_vod, ptr_dispatch_interface:0x%x , ptr_data_pipe: 0x%x ,  support long connect , dowing range (%u, %u)  is not zero so not alloc now .",
                ptr_dispatch_interface, ptr_data_pipe, cur_dispatch_info->_down_range._index, cur_dispatch_info->_down_range._num);
            goto pipe_dis_done;
        }
    }

    if(assign_range._num != 0 && ds_assigned_range_to_pipe(ptr_data_pipe, &assign_range) == TRUE)
    {
        LOG_ERROR("ds_dispatch_at_pipe_vod, 1 assign range (%u, %u) to data pipe: 0x%x", assign_range._index, assign_range._num, ptr_data_pipe);
    }
    else
    {
        LOG_ERROR("ds_dispatch_at_pipe_vod,  failure to assign range (%u, %u) to data pipe: 0x%x", assign_range._index, assign_range._num, ptr_data_pipe);
        if(ptr_data_pipe->_dispatch_info._can_download_ranges._node_size != 0)
        {
            if(has_priority_range && is_cdn_pipe)
            {
                range_list_get_head_node(dispatch_range_list, &first_range_node);
                if(first_range_node != NULL)
                {
                    // [junkun-20130802] 针对cdn pipe考虑加倍分块 
                    //assign_range._num = first_range_node->_range._num/2; // 原来的这样分块可能太大！
                    assign_range._index = first_range_node->_range._index ;
                    assign_range._num = MIN(first_range_node->_range._num, calc_assign_range_num_to_pipe(ptr_data_pipe));

                    if((assign_range._index != ptr_data_pipe->_dispatch_info._down_range._index) ||
                        (0 == ptr_data_pipe->_dispatch_info._down_range._num))
                    {
                        if(ds_assigned_range_to_pipe(ptr_data_pipe, &assign_range) == TRUE)
                        {
                            LOG_ERROR("ds_dispatch_at_pipe_vod,  cdn assign range (%u, %u) to data pipe: 0x%x", assign_range._index, assign_range._num, ptr_data_pipe);
                            if (ptr_dispatch_interface->_start_index == MAX_U32)
                            {
                                ptr_dispatch_interface->_start_index =  assign_range._index;
                                sd_time_ms(&ptr_dispatch_interface->_start_index_time_ms);
                            }
                            LOG_ERROR("ds_dispatch_at_pipe_vod,  cdn assign range (%u, %u) to data pipe: 0x%x", assign_range._index, assign_range._num, ptr_data_pipe);
                            range_list_add_range(&ptr_dispatch_interface->_tmp_range_list, &assign_range, NULL, NULL);
                            assign_range._num = 1;
                            range_list_add_range(&ptr_dispatch_interface->_cur_down_point, &assign_range, NULL, NULL);
                        }
                    }
                    goto pipe_dis_done;
                }
            }

            if(is_slow_peer == FALSE && 
                (ptr_dispatch_interface->_start_index == MAX_U32 ||
                TIME_SUBZ(now_time_ms, ptr_dispatch_interface->_start_index_time_ms) > VOD_FIRST_WAIT_TIME * 1000))
            {
                range_list_get_head_node(dispatch_range_list, &first_range_node);
                if(first_range_node != NULL)
                {
                    if(range_list_is_include(&ptr_data_pipe->_dispatch_info._can_download_ranges, &first_range_node->_range) == TRUE)
                    {
                        assign_range._index = first_range_node->_range._index ;
                        //按速度来分
                        assign_range._num = MIN(first_range_node->_range._num, calc_assign_range_num_to_pipe(ptr_data_pipe));
                        if(ds_assigned_range_to_pipe(ptr_data_pipe, &assign_range) == TRUE)
                        {
                            ptr_dispatch_interface->_start_index =  assign_range._index;
                            sd_time_ms(&ptr_dispatch_interface->_start_index_time_ms);
                            LOG_ERROR("ds_dispatch_at_pipe_vod,  4 assign range (%u, %u) to data pipe: 0x%x", assign_range._index, assign_range._num, ptr_data_pipe);
                            range_list_add_range(&ptr_dispatch_interface->_tmp_range_list, &assign_range, NULL, NULL);
                            assign_range._num = 1;
                            range_list_add_range(&ptr_dispatch_interface->_cur_down_point, &assign_range, NULL, NULL);
                            goto pipe_dis_done;
                        }
                    }
                    else
                    {
                        assign_range = range_list_get_first_overlap_range(&ptr_data_pipe->_dispatch_info._can_download_ranges, &first_range_node->_range);
                        if(assign_range._num !=0 && assign_range._index == first_range_node->_range._index)
                        {
                            assign_range._num = MIN(first_range_node->_range._num, calc_assign_range_num_to_pipe(ptr_data_pipe));
                            if(ds_assigned_range_to_pipe(ptr_data_pipe, &assign_range) == TRUE)
                            {
                                ptr_dispatch_interface->_start_index = assign_range._index;
                                sd_time_ms(&ptr_dispatch_interface->_start_index_time_ms);
                                LOG_ERROR("ds_dispatch_at_pipe_vod,  5 assign range (%u, %u) to data pipe: 0x%x", assign_range._index, assign_range._num, ptr_data_pipe);
                                range_list_add_range(&ptr_dispatch_interface->_tmp_range_list, &assign_range, NULL, NULL);
                                assign_range._num = 1;
                                range_list_add_range(&ptr_dispatch_interface->_cur_down_point, &assign_range, NULL, NULL);
                                goto pipe_dis_done;
                            }
                        }
                    }
                }
            }
            if(range_list_is_contained2(&ptr_data_pipe->_dispatch_info._can_download_ranges, dispatch_range_list) == TRUE)
            {
                range_list_delete_range_list(dispatch_range_list, &ptr_dispatch_interface->_tmp_range_list);
                if (dispatch_range_list->_node_size != 0)
                {
                    // range_list_get_rand_node(dispatch_range_list, &rand_range_node);
                    //range_list_get_max_node(dispatch_range_list, &rand_range_node);
                    if(is_slow_peer == TRUE)
                    {
                        range_list_get_tail_node(dispatch_range_list, &rand_range_node);
                    }
                    else
                    {
                        range_list_get_head_node(dispatch_range_list, &rand_range_node);
                    }
                }
                if(rand_range_node == NULL)
                {
                    LOG_ERROR("ds_dispatch_at_pipe_vod, ptr_dispatch_interface:0x%x , ptr_data_pipe: 0x%x , 1 can not get rand range node of uncomplete_range_list, so return.", ptr_dispatch_interface, ptr_data_pipe);
                    goto  pipe_dis_done;
                }
                LOG_DEBUG("ds_dispatch_at_pipe_vod, ptr_dispatch_interface:0x%x , ptr_data_pipe: 0x%x , 1 get first range node (%u, %u)  of uncomplete_range_list .",
                    ptr_dispatch_interface, ptr_data_pipe, rand_range_node->_range._index, rand_range_node->_range._num);

                assign_range._index = rand_range_node->_range._index ;
                assign_range._num = MIN(rand_range_node->_range._num, calc_assign_range_num_to_pipe(ptr_data_pipe));
            }
            else
            {
                get_range_list_overlap_list(&ptr_data_pipe->_dispatch_info._can_download_ranges,dispatch_range_list, &err_range_list);
                range_list_delete_range_list(&err_range_list, &ptr_dispatch_interface->_tmp_range_list);
                if(err_range_list._node_size != 0)
                {
                    if(is_slow_peer == TRUE)
                    {
                        range_list_get_tail_node(&err_range_list, &rand_range_node);
                    }
                    else
                    {
                        range_list_get_head_node(&err_range_list, &rand_range_node);
                    }
                }
                if(rand_range_node == NULL)
                {
                    LOG_ERROR("ds_dispatch_at_pipe_vod, ptr_dispatch_interface:0x%x , ptr_data_pipe: 0x%x , 2 can not get rand range node of uncomplete_range_list, so return.", ptr_dispatch_interface, ptr_data_pipe);
                    goto  pipe_dis_done;
                }
                LOG_DEBUG("ds_dispatch_at_pipe_vod, ptr_dispatch_interface:0x%x , ptr_data_pipe: 0x%x , 2 get first range node (%u, %u)  of uncomplete_range_list .",
                    ptr_dispatch_interface, ptr_data_pipe, rand_range_node->_range._index, rand_range_node->_range._num);
                assign_range._index = rand_range_node->_range._index ;
                assign_range._num = MIN(rand_range_node->_range._num, calc_assign_range_num_to_pipe(ptr_data_pipe));
            }
            if(assign_range._num != 0 && ds_assigned_range_to_pipe(ptr_data_pipe, &assign_range) == TRUE)
            {
                LOG_DEBUG("ds_dispatch_at_pipe_vod,  2 assign range (%u, %u) to data pipe: 0x%x", assign_range._index, assign_range._num, ptr_data_pipe);
                range_list_add_range(&ptr_dispatch_interface->_tmp_range_list, &assign_range, NULL, NULL);
                assign_range._num =  1;
                range_list_add_range(&ptr_dispatch_interface->_cur_down_point, &assign_range, NULL, NULL);
            }
            else
            {
                LOG_ERROR("ds_dispatch_at_pipe_vod,  failure to assign range (%u, %u) to data pipe: 0x%x", assign_range._index, assign_range._num, ptr_data_pipe);
            }
        }
    }

pipe_dis_done:
    range_list_clear(&uncomplete_range_list);
    range_list_clear(&pri_range_list);
    range_list_clear(&err_range_list);
    return SUCCESS;
}

_int32 ds_dispatch_at_pipe(DISPATCHER *ptr_dispatch_interface, 
    DATA_PIPE *ptr_data_pipe)
{
    if(DS_IS_VOD_MODE(ptr_dispatch_interface->_data_interface) == FALSE)
    {
        return ds_dispatch_at_pipe_normal(ptr_dispatch_interface, ptr_data_pipe);
    }
    else
    {
        return ds_dispatch_at_pipe_vod(ptr_dispatch_interface, ptr_data_pipe);
    }
}


_int32 ds_start_dispatcher_immediate(DISPATCHER*  ptr_dispatch_interface)
{
    _int32 ret_val = SUCCESS;

    LOG_DEBUG("ds_start_dispatcher_immediate");

    if(ptr_dispatch_interface->_immediate_dispatcher_timer == INVALID_MSG_ID)
    {
        ret_val = start_timer((msg_handler)ds_dispatcher_timer_handler, NOTICE_ONCE, 0, 
                    DISPATCHER_ONCE_TIMER, ptr_dispatch_interface, 
                    &ptr_dispatch_interface->_immediate_dispatcher_timer);

        CHECK_VALUE(ret_val);

        LOG_DEBUG("ds_start_dispatcher_immediate, timer :%u", 
            ptr_dispatch_interface->_immediate_dispatcher_timer);
    }
    else
    {
        LOG_DEBUG("ds_start_dispatcher_immediate, already has timer :%u",
            ptr_dispatch_interface->_immediate_dispatcher_timer);
    }

    return ret_val;

}

#ifdef LITTLE_FILE_TEST
#include "p2sp_download/p2sp_task.h"
BOOL ds_is_super_pipe(DATA_PIPE * ptr_data_pipe)
{
    if(ptr_data_pipe->_data_pipe_type!=HTTP_PIPE && ptr_data_pipe->_data_pipe_type!=FTP_PIPE)
    {
        return FALSE;
    }

    if(ptr_data_pipe->_data_pipe_type==HTTP_PIPE )
    {
        HTTP_DATA_PIPE * http_pipe = (HTTP_DATA_PIPE *)ptr_data_pipe;
        if(http_pipe->_super_pipe)
        {
            return TRUE;
        }
    }
    else
    {
        /*  FTP */
        return FALSE;
    }
    return FALSE;
}
BOOL ds_is_task_in_origin_mode(DATA_PIPE * ptr_data_pipe)
{
    TASK * p_task =  (TASK * )dp_get_task_ptr( ptr_data_pipe);
    return pt_is_origin_mode(p_task);
}
BOOL ds_should_use_little_file_assign_range(DATA_PIPE * ptr_data_pipe)
{
    LOG_DEBUG( " enter[0x%X] ds_should_use_little_file_assign_range()...",ptr_data_pipe );
    //PRINT_STRACE_TO_FILE;
    if(ptr_data_pipe->_data_pipe_type!=HTTP_PIPE && ptr_data_pipe->_data_pipe_type!=FTP_PIPE)
    {
        return FALSE;
    }

    if(ptr_data_pipe->_data_pipe_type==HTTP_PIPE )
    {
        HTTP_DATA_PIPE * http_pipe = (HTTP_DATA_PIPE *)ptr_data_pipe;
        HTTP_SERVER_RESOURCE * http_resource = (HTTP_SERVER_RESOURCE *)ptr_data_pipe->_p_resource;
        if(!http_resource->_b_is_origin)
        {
            return FALSE;
        }

        if(!ds_is_task_in_origin_mode( ptr_data_pipe))
        {
            return FALSE;
        }

        if(http_pipe->_super_pipe)
        {
            return FALSE;
        }
    }
    else
    {
        /*  FTP */
        return FALSE;
    }

    return TRUE;
}
#endif /* LITTLE_FILE_TEST */

