#include "utility/define.h"
//#ifdef XUNLEI_MODE

#include "file_manager_imp_xl.h"
#include "file_manager_interface_xl.h"
#include "utility/range.h"
#include "utility/string.h"
#include "utility/mempool.h"
#include "utility/utility.h"

#include "utility/logid.h"
#define LOGID LOGID_FILE_MANAGER
#include "utility/logger.h"

#include "platform/sd_fs.h"
#include "data_buffer.h"

#ifdef _WRITE_BUFFER_MERGE_TEST
_u32 static g_write_num = 0;
_u32 static g_write_num_unmerge = 0; //未合并的range
_u32 static g_write_num_unnormal = 0;//未合并的range num不为1的

_u32 static g_write_num_2 = 0;
_u32 static g_write_num_3 = 0;
_u32 static g_write_num_4 = 0;
_u32 static g_write_num_5 = 0;
_u32 static g_write_num_6 = 0;
_u32 static g_write_num_7 = 0;
_u32 static g_write_num_8 = 0;

#endif


#ifdef _WRITE_BUFFER_MERGE
#include "platform/sd_mem.h"

_int64 static g_malloc_range_size = 0;
_int64 static g_max_malloc_range_size = 1024*1024*32;
#endif

_u32 static g_max_pending_write_op_count = 5;
static void fm_merge_write_range_list_ext(struct tagTMP_FILE* p_file_struct, LIST_ITERATOR* list_node);
static _int32 fm_get_can_merge_num(struct tagTMP_FILE* p_file_struct, LIST_ITERATOR* list_node);
static void fm_merge_wlist_process(struct tagTMP_FILE* p_file_struct, LIST_ITERATOR* list_node, _u32 unit_num);


_int32 fm_callback_xl(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid);

_int32 fm_create_and_init_struct_xl( const char *p_filename, const char *p_filepath, _u64 file_size, struct tagTMP_FILE **pp_file_struct )
{
    _int32 ret_val = SUCCESS;
    struct tagTMP_FILE *p_file_struct = NULL;

    ret_val = tmp_file_malloc_wrap( &p_file_struct );
    CHECK_VALUE( ret_val );

    LOG_DEBUG( "fm_init_struct: file_struct ptr:0x%x, file name:%s, file path:%s",
               p_file_struct, p_filename, p_filepath );

    p_file_struct->_file_name_len = sd_strlen( p_filename );
    ret_val = sd_memcpy( p_file_struct->_file_name, p_filename, p_file_struct->_file_name_len );
    CHECK_VALUE( ret_val );

    p_file_struct->_file_name[p_file_struct->_file_name_len] = '\0';

    p_file_struct->_file_path_len = sd_strlen( p_filepath );
    ret_val = sd_memcpy( p_file_struct->_file_path, p_filepath, p_file_struct->_file_path_len );
    CHECK_VALUE( ret_val );

    p_file_struct->_file_path[p_file_struct->_file_path_len] = '\0';
    p_file_struct->_device_id = INVALID_FILE_ID;
    p_file_struct->_file_size = file_size;
    p_file_struct->_new_file_size = 0;

    list_init( &(p_file_struct->_asyn_read_range_list) );
    list_init( &(p_file_struct->_write_range_list) );


    p_file_struct->_close_para_ptr = NULL;
    p_file_struct->_open_msg_id = INVALID_MSG_ID;

    p_file_struct->_is_closing = FALSE;
    p_file_struct->_is_reopening = FALSE;
    p_file_struct->_is_opening = FALSE;
    p_file_struct->_is_sending_read_op = FALSE;
    p_file_struct->_is_sending_write_op = FALSE;
    p_file_struct->_on_writing_op_count = 0;
    p_file_struct->_is_sending_close_op = FALSE;

    p_file_struct->_is_file_size_valid = FALSE;
    p_file_struct->_is_file_created = FALSE;

    *pp_file_struct = p_file_struct;
    return SUCCESS;
}

_int32 fm_handle_create_file_xl( TMP_FILE *p_file_struct, void *p_user, notify_file_create p_call_back )
{
    OP_PARA_FS_OPEN open_para;
    _int32 ret_val = SUCCESS;
    char file_full_path[MAX_FILE_NAME_LEN + MAX_FILE_PATH_LEN];
    MSG_FILE_CREATE_PARA *p_user_para = NULL;
    LOG_DEBUG( "fm_handle_create_file. user_ptr:0x%x, call_back_ptr:0x%x.", p_user, p_call_back);
    sd_assert( !p_file_struct->_is_closing );

    if( p_file_struct->_is_closing ) return FM_LOGIC_ERROR;

    ret_val = msg_file_create_para_malloc_wrap( &p_user_para );// delete in call_back function
    CHECK_VALUE( ret_val );
    p_user_para->_callback_func_ptr = p_call_back;
    p_user_para->_file_struct_ptr = p_file_struct;
    p_user_para->_user_ptr = p_user;

    ret_val = fm_get_file_full_path( p_file_struct, file_full_path, MAX_FILE_NAME_LEN + MAX_FILE_PATH_LEN );
    CHECK_VALUE( ret_val );

    open_para._filepath = file_full_path;
    open_para._filepath_buffer_len = p_file_struct->_file_path_len + p_file_struct->_file_name_len;
    sd_assert( open_para._filepath_buffer_len < MAX_FILE_NAME_LEN + MAX_FILE_PATH_LEN );
    open_para._open_option = O_DEVICE_FS_CREATE;
    open_para._file_size = p_file_struct->_file_size;

    p_file_struct->_is_opening = TRUE;
    ret_val = fm_op_open_xl( &open_para,  p_user_para, &p_file_struct->_open_msg_id );
    CHECK_VALUE( ret_val );

    return SUCCESS;
}

_int32 fm_op_open_xl( OP_PARA_FS_OPEN *p_fso_para, MSG_FILE_CREATE_PARA *p_user_para, _u32 *msg_id )
{
	MSG_INFO msginfo = {};
    LOG_DEBUG( "fm_op_open." );

    msginfo._device_id = 0;
    msginfo._device_type = DEVICE_FS;
    msginfo._msg_priority = MSG_PRIORITY_NORMAL;
    msginfo._operation_type = OP_OPEN;
    msginfo._user_data = p_user_para;
    msginfo._operation_parameter = p_fso_para;

    post_message(&msginfo, fm_callback_xl, NOTICE_ONCE, fm_open_file_timeout(), msg_id);
    return SUCCESS;
}

_int32 fm_cancel_open_msg_xl(TMP_FILE *p_file_struct)
{
    _int32 ret_val = SUCCESS;

    LOG_DEBUG( "fm_cancel_open_msg. msd_id:%d", p_file_struct->_open_msg_id );
    sd_assert( p_file_struct->_is_opening );

    if( p_file_struct->_open_msg_id != INVALID_MSG_ID )
    {
        ret_val = cancel_message_by_msgid( p_file_struct->_open_msg_id );
        sd_assert( ret_val == SUCCESS );
    }

    p_file_struct->_open_msg_id = INVALID_MSG_ID;
    return SUCCESS;
}

_int32 fm_generate_range_list( struct tagTMP_FILE *p_file_struct
                               , RANGE_DATA_BUFFER *p_range_buffer
                               , void *p_call_back_buffer
                               , void *p_call_back
                               , void *p_user
                               , _u16 op_type )
{
    _int32 ret_val = SUCCESS;
    _u32 range_num = 0;
    _u64 range_pos = 0;
    _u32 data_unit_size;
    RW_DATA_BUFFER *p_rw_data_buffer = NULL;

    if( op_type != OP_OPEN)
    {
        sd_assert( p_range_buffer != NULL );
        data_unit_size = get_data_unit_size();
        range_num = p_range_buffer->_range._num;
        range_pos = p_range_buffer->_range._index;
        if( range_pos * data_unit_size + p_range_buffer->_data_length != p_file_struct->_file_size 
            && p_file_struct->_file_size != 0 )
        {
            //LOG_URGENT("p_range_buffer->_data_length = %d, range_num * data_unit_size = %llu"
            //   , p_range_buffer->_data_length, (_u64)(range_num * data_unit_size) );
            sd_assert( p_range_buffer->_data_length == range_num * data_unit_size );
        }
        sd_assert( range_num != 0 );

        LOG_DEBUG( "##########fm_generate_range_list, range_buffer_ptr:0x%x, range pos:%llu, range_num:%u, pos:%llu, range length:%u, range_op_type:%u",
                   p_range_buffer, range_pos, range_num, range_pos * data_unit_size, p_range_buffer->_data_length, (_u32)op_type);
    }


    ret_val = rw_data_buffer_malloc_wrap( &p_rw_data_buffer );
    CHECK_VALUE( ret_val );

    p_rw_data_buffer->_range_buffer_ptr = p_range_buffer;
    p_rw_data_buffer->_call_back_buffer_ptr = p_call_back_buffer;
    p_rw_data_buffer->_call_back_func_ptr = p_call_back;
    p_rw_data_buffer->_user_ptr = p_user;
    p_rw_data_buffer->_try_num = 0;
    p_rw_data_buffer->_operation_type = op_type;
    p_rw_data_buffer->_is_call_back = FALSE;
    p_rw_data_buffer->_is_cancel = FALSE;
    p_rw_data_buffer->_error_code = SUCCESS;
#ifdef _WRITE_BUFFER_MERGE
    p_rw_data_buffer->_is_merge = FALSE;
#endif

    if( op_type == OP_WRITE || op_type == OP_OPEN )
    {
        LOG_DEBUG( "fm_generate_range_list, p_rw_data_buffer:0x%x, op_type = OP_WRITE && OP_OPEN ", p_rw_data_buffer );
        ret_val = list_push( &p_file_struct->_write_range_list, (void*)p_rw_data_buffer );
        CHECK_VALUE( ret_val );
    }
    else if ( op_type == OP_READ )
    {
        LOG_DEBUG( "fm_generate_range_list, p_rw_data_buffer:0x%x, op_type = OP_READ ", p_rw_data_buffer);
        ret_val = list_push( &p_file_struct->_asyn_read_range_list, (void*)p_rw_data_buffer );
        CHECK_VALUE( ret_val );
    }

    return SUCCESS;
}


_int32 fm_get_can_merge_num(struct tagTMP_FILE* p_file_struct
                            , LIST_ITERATOR* list_node)
{
#ifdef _DEBUG
    LOG_DEBUG("before merge, write range list ... ");
    LIST_ITERATOR cur_iter_before_merge = *list_node;
    LIST_ITERATOR end_iter_before_merge = LIST_END(p_file_struct->_write_range_list);
    for (; cur_iter_before_merge != end_iter_before_merge; cur_iter_before_merge=LIST_NEXT(cur_iter_before_merge) )
    {
        RW_DATA_BUFFER* cur_value = (RW_DATA_BUFFER*)LIST_VALUE(cur_iter_before_merge);
        LOG_DEBUG("merge_write_range_list 000, range[%d, %d], is_callback : %d"
                  , cur_value->_range_buffer_ptr->_range._index
                  , cur_value->_range_buffer_ptr->_range._num
                  , cur_value->_is_call_back);
    }
#endif
    _int32 ret = SUCCESS;
    _int32 can_merge_count = 1;
    LIST_ITERATOR start_iter = *list_node;
    LIST_ITERATOR end_iter = LIST_END( p_file_struct->_write_range_list );
    RW_DATA_BUFFER* cur_value = NULL;
    cur_value = (RW_DATA_BUFFER*)LIST_VALUE(start_iter);
    if (cur_value->_is_call_back || cur_value->_operation_type != OP_WRITE || cur_value->_is_cancel)  // 如果第一个元素就是callback，就不需要进行合并操作了， 先对这个callback元素单独进行操作
    {
        return 0;
    }
    RANGE cur_range = cur_value->_range_buffer_ptr->_range;
    LIST_ITERATOR cur_iter = LIST_NEXT(start_iter);

    while ( cur_iter != end_iter )
    {
        cur_value = (RW_DATA_BUFFER*)LIST_VALUE(cur_iter);

        cur_iter = LIST_NEXT(cur_iter);

        // 是需要回调的结点，跳过这个元素
        if (cur_value->_is_call_back || cur_value->_operation_type != OP_WRITE || cur_value->_is_cancel)
        {
            continue;
        }

        if (cur_range._index + cur_range._num == cur_value->_range_buffer_ptr->_range._index)
        {
            cur_range._num = cur_range._num + cur_value->_range_buffer_ptr->_range._num;
            can_merge_count += 1;
            // 可以合并的元素总数超过最大限制
            if ( can_merge_count == fm_max_merge_range_num() )
            {
                break;
            }
        }
    }
    return cur_range._num;
}

void fm_merge_write_range_list_ext(struct tagTMP_FILE* p_file_struct
                                   , LIST_ITERATOR* list_node)
{
    _int32 unit_num = fm_get_can_merge_num(p_file_struct, list_node);
    LOG_DEBUG("unit_num = %d", unit_num);
    if (unit_num == 0)
    {
#ifdef _DEBUG
        LOG_DEBUG("need callback ... ");
        LIST_ITERATOR cur_iter_before_merge = *list_node;
        LIST_ITERATOR end_iter_before_merge = LIST_END(p_file_struct->_write_range_list);
        for (; cur_iter_before_merge != end_iter_before_merge; cur_iter_before_merge=LIST_NEXT(cur_iter_before_merge) )
        {
            RW_DATA_BUFFER* cur_value = (RW_DATA_BUFFER*)LIST_VALUE(cur_iter_before_merge);
            LOG_DEBUG("merge_write_range_list callback, range[%d, %d], is_callback : %d"
                      , cur_value->_range_buffer_ptr->_range._index
                      , cur_value->_range_buffer_ptr->_range._num
                      , cur_value->_is_call_back);
        }
#endif
        return;
    }

    return fm_merge_wlist_process(p_file_struct, list_node, unit_num);
}

void fm_merge_wlist_process(struct tagTMP_FILE* p_file_struct
                            , LIST_ITERATOR* list_node
                            , _u32 unit_num)
{

#ifdef _DEBUG
    LOG_DEBUG("before merge, write range list ... ");
    LIST_ITERATOR cur_iter_before_merge = *list_node;
    LIST_ITERATOR end_iter_before_merge = LIST_END(p_file_struct->_write_range_list);
    for (; cur_iter_before_merge != end_iter_before_merge; cur_iter_before_merge=LIST_NEXT(cur_iter_before_merge) )
    {
        RW_DATA_BUFFER* cur_value = (RW_DATA_BUFFER*)LIST_VALUE(cur_iter_before_merge);
        LOG_DEBUG("merge_write_range_list 111, range[%d, %d], is_callback : %d"
                  , cur_value->_range_buffer_ptr->_range._index
                  , cur_value->_range_buffer_ptr->_range._num
                  , cur_value->_is_call_back);
    }
#endif

    _int32 ret = SUCCESS;
    LIST_ITERATOR tmp = NULL;
    LIST_ITERATOR start_iter = *list_node;
    LIST_ITERATOR end_iter = LIST_END( p_file_struct->_write_range_list );
    RW_DATA_BUFFER* cur_value = NULL;
    cur_value = (RW_DATA_BUFFER*)LIST_VALUE(start_iter);

    if (unit_num == cur_value->_range_buffer_ptr->_range._num) //直接不需要进行处理
    {
        LOG_DEBUG("unit_num == cur_value->_range_buffer_ptr->_range._num, do not need process");
        return;
    }

    // start_iter前面插入新的结点
    RW_DATA_BUFFER* new_rw_node = NULL;
    RANGE_DATA_BUFFER* new_data_buffer = NULL;
    ret = rw_data_buffer_malloc_wrap(&new_rw_node);
    if (ret != SUCCESS)
    {
        return;
    }
    sd_memset(new_rw_node, 0, sizeof(RW_DATA_BUFFER) );
    ret = alloc_range_data_buffer_node(&new_data_buffer);
    if (ret != SUCCESS)
    {
        rw_data_buffer_free_wrap(new_rw_node);
        return;
    }
    sd_memset(new_data_buffer, 0, sizeof(RANGE_DATA_BUFFER) );

    new_data_buffer->_range._index = cur_value->_range_buffer_ptr->_range._index;
    new_data_buffer->_range._num = 0;
    new_data_buffer->_buffer_length = unit_num * get_data_unit_size();
    new_data_buffer->_data_length = 0;

    ret = sd_get_mem_from_os(new_data_buffer->_buffer_length, (void**)&new_data_buffer->_data_ptr);
    if (ret != SUCCESS)
    {
        rw_data_buffer_free_wrap(new_rw_node);
        free_range_data_buffer_node(new_data_buffer);
        return;
    }
    g_malloc_range_size += new_data_buffer->_buffer_length;

    list_insert(&p_file_struct->_write_range_list, new_rw_node, start_iter);

    *list_node = LIST_PRE(start_iter);
    //开始遍历处理
    LIST_ITERATOR cur_iter = start_iter;
    while ( cur_iter != end_iter )
    {
        cur_value = (RW_DATA_BUFFER*)LIST_VALUE(cur_iter);

        // 是需要回调的结点，跳过这个元素
        if (cur_value->_is_call_back || cur_value->_operation_type != OP_WRITE || cur_value->_is_cancel)
        {
            cur_iter = LIST_NEXT(cur_iter);
            continue;
        }

        if (new_data_buffer->_range._index + new_data_buffer->_range._num == cur_value->_range_buffer_ptr->_range._index)
        {
            // 满足合并要求，从write_range_list里面删除，插入到tmp_nodes_list里面
            sd_memcpy(new_data_buffer->_data_ptr + new_data_buffer->_range._num * get_data_unit_size()
                      , cur_value->_range_buffer_ptr->_data_ptr
                      , cur_value->_range_buffer_ptr->_range._num * get_data_unit_size() );
            new_data_buffer->_data_length += cur_value->_range_buffer_ptr->_range._num * get_data_unit_size();
            new_data_buffer->_range._num = new_data_buffer->_range._num + cur_value->_range_buffer_ptr->_range._num;
            LOG_DEBUG("new_data_buffer.range.num = %d", new_data_buffer->_range._num );
            dm_free_buffer_to_data_buffer(cur_value->_range_buffer_ptr->_buffer_length
                    , &(cur_value->_range_buffer_ptr->_data_ptr) );
            sd_memcpy(new_rw_node, cur_value, sizeof(RW_DATA_BUFFER) );
            tmp = cur_iter;
            cur_iter = LIST_NEXT(cur_iter);
            list_erase(&p_file_struct->_write_range_list, tmp);
            //free_range_data_buffer_node(cur_value->_range_buffer_ptr);
            rw_data_buffer_free_wrap(cur_value);
            // 可以合并的元素总数超过最大限制
            if ( new_data_buffer->_range._num == unit_num )
            {
                break;
            }
        }
        else
        {
            cur_iter = LIST_NEXT(cur_iter);
        }
    }
    new_rw_node->_range_buffer_ptr = new_data_buffer;
    new_rw_node->_is_merge = TRUE;



#ifdef _DEBUG
    LOG_DEBUG("after merge, write range list ... ");
    LIST_ITERATOR cur_iter_after_merge = *list_node;
    LIST_ITERATOR end_iter_after_merge = LIST_END(p_file_struct->_write_range_list);
    for (; cur_iter_after_merge != end_iter_after_merge; cur_iter_after_merge=LIST_NEXT(cur_iter_after_merge) )
    {
        RW_DATA_BUFFER* cur_value = (RW_DATA_BUFFER*)LIST_VALUE(cur_iter_after_merge);
        LOG_DEBUG("merge_write_range_list 222, range[%d, %d], is_callback : %d"
                  , cur_value->_range_buffer_ptr->_range._index
                  , cur_value->_range_buffer_ptr->_range._num
                  , cur_value->_is_call_back);
    }
#endif
}


_int32 fm_handle_write_range_list( struct tagTMP_FILE *p_file_struct )
{
    _int32 ret_val = SUCCESS;
    RW_DATA_BUFFER *p_rw_data_buffer = NULL;
    _u32 write_list_size = 0;
    BOOL is_cancel;
    LIST_ITERATOR cur_node, next_node, tmp_node;
    BOOL is_merge_execute = FALSE;
    BOOL is_callback = FALSE;

#ifdef _LOGGER
    write_list_size = list_size( &p_file_struct->_write_range_list );
    LOG_DEBUG( "fm_handle_write_range_list begin. list_size:%d ", write_list_size );
#endif

    if( p_file_struct->_is_opening )
    {
        LOG_DEBUG("fm_handle_write_range_list, now in opening");
        return SUCCESS;
    }

    if(p_file_struct->_on_writing_op_count > g_max_pending_write_op_count)
    {
        LOG_DEBUG("p_file_struct->_on_writing_op_count > 5");
        return SUCCESS;
    }
    
    cur_node = LIST_BEGIN( p_file_struct->_write_range_list );

    while( cur_node != LIST_END( p_file_struct->_write_range_list ) )
    {
        p_rw_data_buffer = (RW_DATA_BUFFER *)LIST_VALUE( cur_node );
        if( p_rw_data_buffer->_operation_type == OP_OPEN )
        {
            if(p_file_struct->_is_sending_write_op)
            {
                LOG_DEBUG("p_rw_data_buffer->_operation_type == OP_OPEN && p_file_struct->_is_sending_write_op");
                break;
            }
            sd_assert( p_file_struct->_is_reopening );
            sd_assert( !p_file_struct->_is_sending_write_op );
            //sd_assert( !p_file_struct->_is_sending_write_op && !p_file_struct->_is_sending_read_op );
            LOG_DEBUG( "fm_handle_write_range_list:_operation_type OP_OPEN. _new_file_size:%d, _file_size:%d ",
                       p_file_struct->_new_file_size, p_file_struct->_file_size );

            if( p_file_struct->_new_file_size < p_file_struct->_file_size )
            {
                fm_syn_close_and_delete_file( p_file_struct );
            }
            p_file_struct->_file_size = p_file_struct->_new_file_size;

            ret_val = fm_handle_create_file_xl( p_file_struct
                                                , p_rw_data_buffer->_user_ptr
                                                , (notify_file_create)p_rw_data_buffer->_call_back_func_ptr );
            CHECK_VALUE( ret_val );
            break;
        }
        else
        {
            is_cancel = p_rw_data_buffer->_is_cancel;
            if( !is_cancel )
            {
#ifdef _WRITE_BUFFER_MERGE
                fm_merge_write_range_list_ext( p_file_struct, &cur_node );
                p_rw_data_buffer = (RW_DATA_BUFFER *)LIST_VALUE( cur_node );
#endif
                
                /*
                if(p_rw_data_buffer->_is_call_back)
                {    
                    RANGE_DATA_BUFFER_LIST* buffer_list = NULL;
                    ret_val = range_data_buffer_list_malloc_wrap(&buffer_list);
                    CHECK_VALUE(ret_val);
                    list_init(&buffer_list->_data_list);

                    ret_val = buffer_list_add(buffer_list
                        , &(p_rw_data_buffer->_range_buffer_ptr->_range)
                        , p_rw_data_buffer->_range_buffer_ptr->_data_ptr
                        , p_rw_data_buffer->_range_buffer_ptr->_data_length
                        , p_rw_data_buffer->_range_buffer_ptr->_buffer_length);
                    CHECK_VALUE(ret_val);
                    p_rw_data_buffer->_call_back_buffer_ptr = (void*)buffer_list;
                }
                */
                if(p_rw_data_buffer->_is_call_back)
                {
                    is_callback = TRUE;
                }


                ret_val = fm_asyn_handle_range_data( p_file_struct, p_rw_data_buffer );
                p_file_struct->_is_sending_write_op = TRUE;
                p_file_struct->_on_writing_op_count += 1;
                CHECK_VALUE( ret_val );

                next_node = LIST_NEXT( cur_node );
                list_erase( &p_file_struct->_write_range_list, cur_node );
                cur_node = next_node;

                if(is_callback)
                {
                    break;
                }

            }
            else
            {
                fm_handle_cancel_rw_buffer( p_file_struct, p_rw_data_buffer );
                next_node = LIST_NEXT( cur_node );
                list_erase( &p_file_struct->_write_range_list, cur_node );

                //sd_assert( p_file_struct->_is_sending_write_op == FALSE );
                cur_node = next_node;
            }
        }
    }

    write_list_size = list_size( &p_file_struct->_write_range_list );
    LOG_DEBUG( "fm_handle_write_range_list end. list_size:%d ", write_list_size );

    if( p_file_struct->_is_closing && write_list_size == 0 )
    {
        ret_val = fm_handle_close_file_xl( p_file_struct );
        CHECK_VALUE( ret_val );
    }

    return SUCCESS;
}

#ifdef _WRITE_BUFFER_MERGE
void fm_merge_write_range_list( struct tagTMP_FILE *p_file_struct, LIST_ITERATOR *list_node )
{
    _int32 ret_val = SUCCESS;
    RW_DATA_BUFFER *p_rw_data_buffer = NULL, *p_new_rw_data_buffer = NULL;
    RANGE_DATA_BUFFER *p_node_data = NULL, *p_next_node_data = NULL, *p_new_node_data = NULL;
    _u32 range_num = 0;
    RANGE new_range;
    _u32 range_pos = 0, dest_pos = 0, src_pos = 0, copy_num = 0;
    LIST_ITERATOR  cur_node = NULL, next_node = NULL, tmp_node = NULL;

    LOG_DEBUG("fm_merge_write_range_list!!!!!!!, p_file_struct:0x%x .", p_file_struct );

#ifdef _DEBUG
    cur_node = *list_node;
    while( cur_node != LIST_END( p_file_struct->_write_range_list ) )
    {
        p_rw_data_buffer = (RW_DATA_BUFFER *)LIST_VALUE( cur_node );
        p_node_data = p_rw_data_buffer->_range_buffer_ptr;
        LOG_DEBUG("fm_merge_write_range_list before, range[%d, %d].",
                  p_node_data->_range._index, p_node_data->_range._num );
        cur_node = LIST_NEXT( cur_node );
    }
#endif

    cur_node = *list_node;
    p_rw_data_buffer = (RW_DATA_BUFFER *)LIST_VALUE( cur_node );

    sd_assert( p_rw_data_buffer->_is_cancel == FALSE );
    if( p_rw_data_buffer->_operation_type != OP_WRITE ) return;
    if( p_rw_data_buffer->_is_call_back ) return;
    p_node_data = p_rw_data_buffer->_range_buffer_ptr;
    next_node = LIST_NEXT( cur_node );
    new_range._index = p_node_data->_range._index;
    new_range._num = p_node_data->_range._num;

    while( next_node != LIST_END(p_file_struct->_write_range_list)  )
    {
        p_rw_data_buffer = (RW_DATA_BUFFER *)LIST_VALUE( next_node );
        p_next_node_data = p_rw_data_buffer->_range_buffer_ptr;

        if( p_rw_data_buffer->_is_cancel
            || p_rw_data_buffer->_operation_type != OP_WRITE ) break;
        sd_assert( new_range._index <= p_next_node_data->_range._index );
        if( new_range._index + new_range._num < p_next_node_data->_range._index )
        {
            LOG_DEBUG("fm_merge_write_range_list, unmerge buffer:range:%u, num:%u .",
                      p_next_node_data->_range._index, p_next_node_data->_range._num );
            break;
        }
        else if( new_range._index + new_range._num >= p_next_node_data->_range._index + p_next_node_data->_range._num)
        {
            LOG_DEBUG("fm_merge_write_range_list, delete overlap tail buffer:range:%u, num:%u .",
                      p_next_node_data->_range._index, p_next_node_data->_range._num );
            if( p_rw_data_buffer->_is_call_back ) break;
            dm_free_buffer_to_data_buffer(p_next_node_data->_buffer_length, &(p_next_node_data->_data_ptr));
            //free_range_data_buffer_node(p_next_node_data);
            rw_data_buffer_free_wrap(p_rw_data_buffer);
            tmp_node = next_node;
            next_node = LIST_NEXT( next_node );
            list_erase( &p_file_struct->_write_range_list, tmp_node );
            continue;
        }

        sd_assert( p_next_node_data->_range._index + p_next_node_data->_range._num > new_range._index );
        range_num = p_next_node_data->_range._index + p_next_node_data->_range._num - new_range._index;
        if(  range_num > fm_max_merge_range_num() ) break;
        new_range._num = range_num;
        if( p_rw_data_buffer->_is_call_back ) break;

        next_node = LIST_NEXT( next_node );
    }

    sd_assert( new_range._index == p_node_data->_range._index );
    if( new_range._num == p_node_data->_range._num )
    {
        return;
    }

    alloc_range_data_buffer_node( &p_new_node_data );
    if(p_new_node_data == NULL)
    {
        LOG_ERROR("fm_merge_write_range_list , malloc RANGE_DATA_BUFFER failure .");
        return;
    }

    sd_memset(p_new_node_data, 0, sizeof(RANGE_DATA_BUFFER));

    sd_assert( new_range._num <= fm_max_merge_range_num() );

    LOG_DEBUG("fm_merge_write_range_list, merge buffer!!!:range:%u, num:%u .",
              new_range._index, new_range._num );

    p_new_node_data->_range = new_range;
    p_new_node_data->_buffer_length = new_range._num * get_data_unit_size();


    if( g_malloc_range_size >= g_max_malloc_range_size )
    {
        LOG_ERROR("fm_merge_write_range_list , malloc_range_size too big :%llu", g_malloc_range_size );

        free_range_data_buffer_node(p_new_node_data);
        return;
    }
    ret_val = sd_get_mem_from_os( p_new_node_data->_buffer_length, (void**)&p_new_node_data->_data_ptr );
    if(ret_val != SUCCESS)
    {
        free_range_data_buffer_node(p_new_node_data);
        LOG_ERROR("fm_merge_write_range_list , dm_get_buffer_from_data_buffer failure .");
        return;
    }

    ret_val = rw_data_buffer_malloc_wrap( &p_new_rw_data_buffer );
    if(ret_val != SUCCESS)
    {
        sd_free_mem_to_os( p_new_node_data->_data_ptr, p_new_node_data->_buffer_length );
        free_range_data_buffer_node(p_new_node_data);
        LOG_ERROR("fm_merge_write_range_list , dm_get_buffer_from_data_buffer failure .");
        return;
    }
    sd_memset( p_new_rw_data_buffer, 0, sizeof(RW_DATA_BUFFER) );

    ret_val = list_insert( &p_file_struct->_write_range_list, p_new_rw_data_buffer, cur_node );
    if(ret_val != SUCCESS)
    {
        sd_free_mem_to_os( p_new_node_data->_data_ptr, p_new_node_data->_buffer_length );
        free_range_data_buffer_node(p_new_node_data);
        rw_data_buffer_free_wrap( p_new_rw_data_buffer );
        LOG_ERROR("fm_merge_write_range_list , list_insert failure .");
        return;
    }
    *list_node = LIST_PRE( cur_node );
    g_malloc_range_size += p_new_node_data->_buffer_length;
    LOG_ERROR("fm_merge_write_range_list , g_malloc_range_size :%llu", g_malloc_range_size );

    range_pos = p_new_node_data->_range._index;
    next_node =  cur_node;
    while( next_node != LIST_END(p_file_struct->_write_range_list) )
    {
        p_rw_data_buffer = (RW_DATA_BUFFER *)LIST_VALUE( next_node );
        p_next_node_data = p_rw_data_buffer->_range_buffer_ptr;
        if( range_pos >= p_new_node_data->_range._index + p_new_node_data->_range._num
            || range_pos < p_next_node_data->_range._index )
            break;
        if( range_pos < p_next_node_data->_range._index + p_next_node_data->_range._num )
        {
            src_pos = (range_pos-p_next_node_data->_range._index) * get_data_unit_size();
            dest_pos = (range_pos-p_new_node_data->_range._index) * get_data_unit_size();
            copy_num = MIN( p_next_node_data->_range._index + p_next_node_data->_range._num - range_pos,
                            p_new_node_data->_range._index +p_new_node_data->_range._num - range_pos );

            sd_memcpy( p_new_node_data->_data_ptr + dest_pos, p_next_node_data->_data_ptr + src_pos, copy_num*get_data_unit_size() );

            if( p_next_node_data->_data_length % get_data_unit_size() == 0 )
            {
                p_new_node_data->_data_length += copy_num * get_data_unit_size();
            }
            else
            {
                p_new_node_data->_data_length += p_next_node_data->_data_length % get_data_unit_size() + (copy_num-1) * get_data_unit_size();
            }
            range_pos += copy_num;

            LOG_DEBUG("fm_merge_write_range_list, merge buffer delete:range:%u, num:%u .",
                      p_next_node_data->_range._index, p_next_node_data->_range._num );

            dm_free_buffer_to_data_buffer(p_next_node_data->_buffer_length, &(p_next_node_data->_data_ptr));
            //free_range_data_buffer_node(p_next_node_data);  //写成功会统一删掉
            sd_memcpy(p_new_rw_data_buffer, p_rw_data_buffer, sizeof(RW_DATA_BUFFER));
            rw_data_buffer_free_wrap( p_rw_data_buffer );
            tmp_node = next_node;
            next_node = LIST_NEXT( next_node );
            list_erase( &p_file_struct->_write_range_list, tmp_node );
        }
        else
        {
            sd_assert( FALSE );//前面已经把前一个包含后一个的过滤了
            break;
        }
    }

    p_new_rw_data_buffer->_range_buffer_ptr = p_new_node_data;
    p_new_rw_data_buffer->_is_merge = TRUE;

#ifdef _DEBUG
    cur_node = *list_node;
    while( cur_node != LIST_END( p_file_struct->_write_range_list ) )
    {
        p_rw_data_buffer = (RW_DATA_BUFFER *)LIST_VALUE( cur_node );
        p_node_data = p_rw_data_buffer->_range_buffer_ptr;
        LOG_DEBUG("fm_merge_write_range_list after, range[%d, %d].",
                  p_node_data->_range._index, p_node_data->_range._num );

        cur_node = LIST_NEXT( cur_node );
    }
#endif

    return;
}
#endif

_int32 fm_syn_close_and_delete_file( struct tagTMP_FILE *p_file_struct )
{
    _int32 ret_val = SUCCESS;
    char _file_full_path[MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN];

    LOG_DEBUG( "fm_syn_close_and_delete_file " );
    ret_val = sd_close_ex( p_file_struct->_device_id );
    CHECK_VALUE( ret_val );

    ret_val = sd_strncpy( _file_full_path, p_file_struct->_file_path, MAX_FILE_PATH_LEN );
    CHECK_VALUE( ret_val );

    ret_val = sd_strncpy( _file_full_path + p_file_struct->_file_path_len, p_file_struct->_file_name, MAX_FILE_NAME_LEN );
    CHECK_VALUE( ret_val );

    _file_full_path[p_file_struct->_file_path_len + p_file_struct->_file_name_len] = '\0';
    LOG_DEBUG( "fm_syn_close_and_delete_file, sd_delete_file:%s ", _file_full_path );

    ret_val = sd_delete_file( _file_full_path );
    CHECK_VALUE( ret_val );

    return SUCCESS;
}


_int32 fm_handle_asyn_read_range_list( struct tagTMP_FILE *p_file_struct )
{
    _int32 ret_val = SUCCESS;
    RW_DATA_BUFFER *p_rw_data_buffer = NULL;
    LIST_ITERATOR cur_node, next_node;
    _u32 asyn_read_list_size = 0;
    BOOL is_cancel;

#ifdef _LOGGER
    asyn_read_list_size = list_size( &p_file_struct->_asyn_read_range_list );
    LOG_DEBUG( "fm_handle_asyn_read_range_list begin.list_size:%d ", asyn_read_list_size );
#endif

    LOG_DEBUG( "fm_handle_asyn_read_range_list.is_sending:%d, is_reopening:%d, is_opening:%d",
        p_file_struct->_is_sending_read_op, p_file_struct->_is_reopening,p_file_struct->_is_opening );
    if( p_file_struct->_is_sending_read_op 
        || p_file_struct->_is_reopening 
        || p_file_struct->_is_opening)
    {      
        return SUCCESS;
    }

    cur_node = LIST_BEGIN( p_file_struct->_asyn_read_range_list );
    p_rw_data_buffer = (RW_DATA_BUFFER *)LIST_VALUE( cur_node );

    while( cur_node != LIST_END( p_file_struct->_asyn_read_range_list ) )
    {
        p_rw_data_buffer = (RW_DATA_BUFFER *)LIST_VALUE( cur_node );
        is_cancel = p_rw_data_buffer->_is_cancel;
        if( !is_cancel )
        {
            ret_val = fm_asyn_handle_range_data( p_file_struct, p_rw_data_buffer );
            CHECK_VALUE( ret_val );
            p_file_struct->_is_sending_read_op = TRUE;
            break;
        }

        fm_handle_cancel_rw_buffer( p_file_struct, p_rw_data_buffer );
        next_node = LIST_NEXT( cur_node );
        list_erase( &p_file_struct->_asyn_read_range_list, cur_node );

        sd_assert( p_file_struct->_is_sending_read_op == FALSE );
        cur_node = next_node;
    }

    asyn_read_list_size = list_size( &p_file_struct->_asyn_read_range_list );
    LOG_DEBUG( "fm_handle_asyn_read_range_list end.list_size:%d ", asyn_read_list_size );

    if( p_file_struct->_is_closing && asyn_read_list_size == 0 )
    {
        ret_val = fm_handle_close_file_xl( p_file_struct );
        CHECK_VALUE( ret_val );
    }

    return SUCCESS;
}

_int32 fm_handle_cancel_rw_buffer( struct tagTMP_FILE *p_file_struct, RW_DATA_BUFFER *p_rw_data_buffer )
{
    notify_write_result p_write_result = NULL;
    RANGE_DATA_BUFFER *p_data_buffer = NULL;
    _int32 ret_val = SUCCESS;

    LOG_DEBUG( "fm_handle_cancel_rw_buffer. " );
    sd_assert( p_rw_data_buffer->_is_cancel == TRUE );
    if( p_rw_data_buffer->_operation_type == OP_READ )
    {
        notify_read_result p_read_result = (notify_read_result)p_rw_data_buffer->_call_back_func_ptr;
        sd_assert( p_rw_data_buffer->_is_call_back == TRUE );
        p_read_result( p_file_struct, p_rw_data_buffer->_user_ptr,
                       p_rw_data_buffer->_call_back_buffer_ptr, FM_READ_CLOSE_EVENT, 0 );
    }
    else
    {
        p_data_buffer = p_rw_data_buffer->_range_buffer_ptr;
        sd_assert( p_rw_data_buffer->_operation_type == OP_WRITE );
        p_write_result = (notify_write_result)p_rw_data_buffer->_call_back_func_ptr;

#ifdef _WRITE_BUFFER_MERGE
        if( p_rw_data_buffer->_is_merge )
        {
            sd_free_mem_to_os( p_data_buffer->_data_ptr, p_data_buffer->_buffer_length );
            g_malloc_range_size -=  p_data_buffer->_buffer_length;
        }
        else
        {
            ret_val = drop_buffer_from_range_buffer( p_data_buffer );
        }
#else
        ret_val = drop_buffer_from_range_buffer( p_data_buffer );
#endif
        sd_assert( ret_val == SUCCESS );

        if( p_rw_data_buffer->_is_call_back == TRUE )
        {
            p_write_result( p_file_struct, p_rw_data_buffer->_user_ptr,
                            p_rw_data_buffer->_call_back_buffer_ptr, FM_WRITE_CLOSE_EVENT );
            range_data_buffer_list_free_wrap( p_rw_data_buffer->_call_back_buffer_ptr );
        }
    }
    rw_data_buffer_free_wrap( p_rw_data_buffer );
    return SUCCESS;
}

//handle the read and write operation for the range data
_int32 fm_asyn_handle_range_data( struct tagTMP_FILE *p_file_struct, RW_DATA_BUFFER *p_rw_data_buffer )
{
    OP_PARA_FS_RW rw_para;
    MSG_FILE_RW_PARA *p_user_para;
    RANGE_DATA_BUFFER *p_range_data_buffer = NULL;
    _int32 ret_val;
    sd_assert( p_file_struct->_is_reopening == FALSE );
    LOG_DEBUG( "fm_asyn_handle_range_data: p_rw_data_buffer:0x%x.", p_rw_data_buffer );

    ret_val = msg_file_rw_para_slab_malloc_wrap( &p_user_para );
    CHECK_VALUE( ret_val );

    p_user_para->_file_struct_ptr = p_file_struct;
    p_user_para->_rw_data_buffer_ptr = p_rw_data_buffer;
    p_range_data_buffer = p_rw_data_buffer->_range_buffer_ptr;

    rw_para._buffer = p_range_data_buffer->_data_ptr;
    rw_para._expect_size = p_range_data_buffer->_data_length;
    rw_para._operating_offset = ( _u64 )p_range_data_buffer->_range._index * get_data_unit_size();
    rw_para._operated_size = 0;

    LOG_DEBUG( "fm_asyn_handle_range_data: buffer:0x%x, expect_size:%u, operating_offset:%llu, _operated_size = %u.",
               p_range_data_buffer->_data_ptr, rw_para._expect_size, rw_para._operating_offset, rw_para._operated_size );
    ret_val = fm_op_rw_xl( p_file_struct->_device_id, &rw_para, p_user_para );
    CHECK_VALUE( ret_val );

    return SUCCESS;
}

_int32 fm_op_rw_xl( _u32 device_id, OP_PARA_FS_RW *p_frw_para, MSG_FILE_RW_PARA *p_user_para )
{
	MSG_INFO msg_info = {};
    _u32 msgid;
    _int32 ret_val = SUCCESS;
    LOG_DEBUG( "fm_op_rw .p_user_para:0x%x.", p_user_para );

    msg_info._device_id = device_id;
    msg_info._device_type = DEVICE_FS;
    msg_info._msg_priority = MSG_PRIORITY_NORMAL;
    msg_info._operation_parameter = p_frw_para;
    msg_info._operation_type = p_user_para->_rw_data_buffer_ptr->_operation_type;
    msg_info._user_data = p_user_para;

    ret_val = post_message(&msg_info, fm_callback_xl, NOTICE_ONCE, fm_rw_file_timeout(), &msgid);
    CHECK_VALUE( ret_val );

    return SUCCESS;
}

_int32 fm_handle_close_file_xl( struct tagTMP_FILE *p_file_struct )
{
    _int32 list_size = 0;
    _int32 ret_val = SUCCESS;
    LOG_DEBUG( "fm_handle_close_file." );

    list_size += p_file_struct->_asyn_read_range_list._list_size;
    list_size += p_file_struct->_write_range_list._list_size;
    if( list_size != 0 )
    {
        LOG_DEBUG( "fm_handle_close_file. list_size is not zero!" );
        return SUCCESS;
    }
    if( p_file_struct->_is_sending_write_op > 0)  
    {
        LOG_DEBUG( "p_file_struct->_is_sending_write_op > 0" );
        return SUCCESS;     
    }
    if( p_file_struct->_is_opening )
    {
        ret_val = fm_cancel_open_msg_xl( p_file_struct );
        sd_assert( ret_val == SUCCESS );
        return SUCCESS;
    }

    if(p_file_struct->_is_sending_close_op == FALSE)
    {
        LOG_DEBUG( "fm implement close file ." );
        sd_assert( p_file_struct->_close_para_ptr != NULL );
        ret_val = fm_op_close_xl( p_file_struct->_device_id, p_file_struct->_close_para_ptr );
        CHECK_VALUE( ret_val );

        p_file_struct->_is_sending_close_op = TRUE;
    }

    return SUCCESS;
}

void fm_cancel_list_rw_op_xl( LIST *p_list )
{
    LIST_ITERATOR cur_node, end_node;
    cur_node = LIST_BEGIN( *p_list );
    end_node = LIST_END( *p_list );
    LOG_DEBUG( "fm_cancel_list_rw_op_xl ." );

    while( cur_node != end_node )
    {
        RW_DATA_BUFFER *p_list_rw_item = (RW_DATA_BUFFER *)LIST_VALUE( cur_node );
        p_list_rw_item->_is_cancel = TRUE;
        cur_node = LIST_NEXT( cur_node );
    }
}

_int32 fm_op_close_xl( _u32 device_id , MSG_FILE_CLOSE_PARA *p_close_rara )
{
	MSG_INFO msg_info = {};
    _u32 msgid;
    LOG_DEBUG( "fm_op_close ." );

    msg_info._operation_parameter = NULL;
    msg_info._user_data = p_close_rara;

    if( device_id == INVALID_FILE_ID )
    {
        msg_info._device_type = DEVICE_NONE;
        msg_info._operation_type = OP_NONE;
        post_message( &msg_info, fm_callback_xl, NOTICE_ONCE, 0, &msgid );
    }
    else
    {
        msg_info._device_id = device_id;
        msg_info._device_type = DEVICE_FS;
        msg_info._operation_type = OP_CLOSE;
        msg_info._msg_priority = MSG_PRIORITY_NORMAL;
        post_message( &msg_info, fm_callback_xl, NOTICE_ONCE, fm_close_file_timeout(), &msgid );
    }

    return SUCCESS;
}

_int32 fm_callback_xl(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid)
{

#ifdef _LOGGER
    _u16 device_type = msg_info->_device_type;
#endif

    _u16 operation_type = msg_info->_operation_type;
    _int32 ret_val = SUCCESS;
    struct tagTMP_FILE *p_file_struct;
    LOG_DEBUG( "fm_callback,op=%X,errcode=%d.",operation_type,errcode );

    if( operation_type == OP_OPEN )//handle open operation
    {
        MSG_FILE_CREATE_PARA *p_msg_para = (MSG_FILE_CREATE_PARA *)msg_info->_user_data;
#ifdef _LOGGER
        sd_assert( device_type == DEVICE_FS );
#endif
        p_file_struct = p_msg_para->_file_struct_ptr;
        if( errcode == SUCCESS ) p_file_struct->_is_file_created = TRUE;

        p_file_struct->_is_opening = FALSE;
        if( !p_file_struct->_is_reopening )
        {
            ret_val = fm_create_callback_xl( msg_info, errcode );
            CHECK_VALUE( ret_val );
        }
        else
        {
            ret_val = fm_reopen_callback( msg_info, errcode );
            CHECK_VALUE( ret_val );
        }

        if( p_file_struct->_is_closing
            && !p_file_struct->_is_sending_read_op
            && !p_file_struct->_is_sending_write_op )
        {
            ret_val = fm_handle_close_file_xl( p_file_struct );
            CHECK_VALUE( ret_val );
        }
        ret_val = msg_file_create_para_free_wrap( msg_info->_user_data );
        CHECK_VALUE( ret_val );

        //ret_val = fm_handle_write_range_list( p_file_struct );
        //CHECK_VALUE( ret_val );
    }
    else if( operation_type == OP_READ )
    {
        MSG_FILE_RW_PARA *p_msg_para = (MSG_FILE_RW_PARA *)msg_info->_user_data;
#ifdef _LOGGER
        sd_assert( device_type == DEVICE_FS );
#endif

        p_file_struct = p_msg_para->_file_struct_ptr;

        ret_val = fm_read_callback_xl( msg_info, errcode );
        CHECK_VALUE( ret_val );

        ret_val = msg_file_rw_para_slab_free_wrap( msg_info->_user_data );
        CHECK_VALUE( ret_val );
        p_file_struct->_is_sending_read_op = FALSE;

        ret_val = fm_handle_asyn_read_range_list( p_file_struct );
        CHECK_VALUE( ret_val );
    }
    else if( operation_type == OP_WRITE )
    {
        MSG_FILE_RW_PARA *p_msg_para = (MSG_FILE_RW_PARA *)msg_info->_user_data;
#ifdef _LOGGER
        sd_assert( device_type == DEVICE_FS );
#endif

        p_file_struct = p_msg_para->_file_struct_ptr;

        ret_val = fm_write_callback_xl( msg_info, errcode );
        CHECK_VALUE( ret_val );

        ret_val = msg_file_rw_para_slab_free_wrap( msg_info->_user_data );
        CHECK_VALUE( ret_val );
        p_file_struct->_on_writing_op_count -= 1;
        if(p_file_struct->_on_writing_op_count <= 0)
            p_file_struct->_is_sending_write_op = FALSE;

        ret_val = fm_handle_write_range_list( p_file_struct );
        CHECK_VALUE( ret_val );
    }
    else if( operation_type == OP_CLOSE || operation_type == OP_NONE )
    {
        if( operation_type == OP_NONE )
        {
            errcode = SUCCESS;
        }
        else
        {
#ifdef _LOGGER
            sd_assert( device_type == DEVICE_FS );
#endif
        }
        ret_val = fm_close_callback_xl( msg_info, errcode );
        CHECK_VALUE( ret_val );
        ret_val = msg_file_close_para_free_wrap( msg_info->_user_data );
        CHECK_VALUE( ret_val );
    }

    return SUCCESS;
}

_int32 fm_reopen_callback( const MSG_INFO *msg_info, _int32 errcode )
{
    _int32 ret_val;

    MSG_FILE_CREATE_PARA *p_user_para = NULL;
    struct tagTMP_FILE *p_file_struct = NULL;
    RW_DATA_BUFFER *p_list_rw_item = NULL;
    LIST_ITERATOR cur_node = NULL;
    notify_file_create p_create_call_back = NULL;

    p_user_para = (MSG_FILE_CREATE_PARA *)msg_info->_user_data;
    p_file_struct = p_user_para->_file_struct_ptr;
    p_create_call_back = p_user_para->_callback_func_ptr;

    p_file_struct->_device_id = msg_info->_device_id;

    /*
    if( errcode == SUCCESS )
    {
        p_file_struct->_device_id = msg_info->_device_id;
    }
    else
    {
        p_file_struct->_device_id = INVALID_FILE_ID;
    }
    */
    LOG_DEBUG( "fm_reopen_callback:errcode=%d. is_closing:%d,filename=%s/%s,filesize=%llu",errcode,p_file_struct->_is_closing,p_file_struct->_file_path,p_file_struct->_file_name,p_file_struct->_file_size);


    cur_node = LIST_BEGIN( p_file_struct->_write_range_list );
    p_list_rw_item = (RW_DATA_BUFFER *)LIST_VALUE( cur_node );
    sd_assert( p_list_rw_item->_operation_type == OP_OPEN );
    if( !p_list_rw_item->_is_cancel )
    {
        p_file_struct->_is_reopening = FALSE;
    }
    rw_data_buffer_free_wrap( p_list_rw_item );
    list_erase( &p_file_struct->_write_range_list, cur_node );

    if( p_file_struct->_is_closing )
    {
        ret_val = p_create_call_back( p_file_struct, p_user_para->_user_ptr, FM_CREAT_CLOSE_EVENT );
        CHECK_VALUE( ret_val );
    }
    else
    {
        ret_val = p_create_call_back( p_file_struct, p_user_para->_user_ptr, errcode );
        CHECK_VALUE( ret_val );
        if( errcode == SUCCESS )
        {
            ret_val = fm_handle_write_range_list( p_file_struct );
            CHECK_VALUE( ret_val );
            ret_val = fm_handle_asyn_read_range_list( p_file_struct );
            CHECK_VALUE( ret_val );
        }
    }

    return SUCCESS;
}

_int32 fm_create_callback_xl( const MSG_INFO *msg_info, _int32 errcode )
{
    MSG_FILE_CREATE_PARA *p_user_para = NULL;
    struct tagTMP_FILE *p_file_struct = NULL;
    notify_file_create p_create_call_back = NULL;
    _int32 ret_val = 0;

    p_user_para = (MSG_FILE_CREATE_PARA *)msg_info->_user_data;
    p_file_struct = p_user_para->_file_struct_ptr;
    p_create_call_back = p_user_para->_callback_func_ptr;

    p_file_struct->_device_id = msg_info->_device_id;

    LOG_DEBUG( "fm_create_callback:errcode=%d.filename=%s/%s,filesize=%llu",errcode,p_file_struct->_file_path,p_file_struct->_file_name,p_file_struct->_file_size);
    /*
    if( errcode == SUCCESS )
    {
        p_file_struct->_device_id = msg_info->_device_id;
    }
    else
    {
        p_file_struct->_device_id = INVALID_FILE_ID;
    }
    */

    //if error, not retry
    if( p_file_struct->_is_closing )
    {
        ret_val = p_create_call_back( p_file_struct, p_user_para->_user_ptr, FM_CREAT_CLOSE_EVENT );
        CHECK_VALUE( ret_val );
    }
    else
    {
        ret_val = p_create_call_back( p_file_struct, p_user_para->_user_ptr, errcode );
        CHECK_VALUE( ret_val );
        if( errcode == SUCCESS )
        {
            ret_val = fm_handle_write_range_list( p_file_struct );
            CHECK_VALUE( ret_val );
            ret_val = fm_handle_asyn_read_range_list( p_file_struct );
            CHECK_VALUE( ret_val );
        }
    }

    return SUCCESS;
}

_int32 fm_write_callback_xl( const MSG_INFO *msg_info, _int32 errcode )
{

    MSG_FILE_RW_PARA *p_user_para = (MSG_FILE_RW_PARA *)msg_info->_user_data;
    struct tagTMP_FILE *p_file_struct = p_user_para->_file_struct_ptr;
    OP_PARA_FS_RW *p_rw_para = NULL;

    RW_DATA_BUFFER *p_rw_data_buffer = p_user_para->_rw_data_buffer_ptr;
    notify_write_result p_write_call_back = (notify_write_result)p_rw_data_buffer->_call_back_func_ptr;
    void *p_user = p_rw_data_buffer->_user_ptr;
    RANGE_DATA_BUFFER *p_data_buffer = NULL;

    void *call_back_buffer_ptr = NULL;
    BOOL is_call_back = FALSE;
    _int32 ret_val = SUCCESS;

    p_rw_para = (OP_PARA_FS_RW *)msg_info->_operation_parameter;

    LOG_DEBUG( "fm_write_callback:errcode=%d.filename=%s/%s,filesize=%llu,offset=%llu\
               ,expect_size=%u,operated_size=%u",
               errcode,p_file_struct->_file_path,p_file_struct->_file_name
              ,p_file_struct->_file_size,p_rw_para->_operating_offset
              ,p_rw_para->_expect_size,p_rw_para->_operated_size);

    if( p_rw_data_buffer->_is_cancel ) return SUCCESS;

    //check asyn write op
    if( errcode == SUCCESS && p_rw_para->_expect_size != p_rw_para->_operated_size )
    {
        LOG_ERROR( "fm_write_callback error. expect_size:%u, operated_size:%u ",
                   p_rw_para->_expect_size, p_rw_para->_operated_size );
        errcode = FM_RW_ERROR;
    }

    if( errcode != SUCCESS )
    {
        p_rw_data_buffer->_try_num++;
        LOG_DEBUG( "fm_write_callback error, errcode:%d.", errcode );
        p_rw_data_buffer->_error_code = errcode;
        p_file_struct->_write_error = errcode;
    }

    p_data_buffer = p_rw_data_buffer->_range_buffer_ptr;
#ifdef _WRITE_BUFFER_MERGE
    if( p_rw_data_buffer->_is_merge )
    {
        sd_assert( p_data_buffer->_range._num > 1 );
        sd_free_mem_to_os( p_data_buffer->_data_ptr, p_data_buffer->_buffer_length );
        g_malloc_range_size -=  p_data_buffer->_buffer_length;
        free_range_data_buffer_node(p_data_buffer);
    }
    else
    {
        ret_val = drop_buffer_from_range_buffer( p_data_buffer );
    }
#else
    ret_val = drop_buffer_from_range_buffer( p_data_buffer );
#endif
    sd_assert( ret_val == SUCCESS );

    if( p_rw_data_buffer->_is_call_back 
        && !p_file_struct->_is_opening)
//        && !p_file_struct->_is_closing)

    {
        LOG_DEBUG( "fm_write_callback return SUCCESS.file_struct:0x%x, call_back_buffer_ptr:0x%x.",
                   p_file_struct, p_rw_data_buffer->_call_back_buffer_ptr );
        p_write_call_back( p_file_struct
                           , p_user
                           , p_rw_data_buffer->_call_back_buffer_ptr
                           , p_file_struct->_write_error );
        range_data_buffer_list_free_wrap( p_rw_data_buffer->_call_back_buffer_ptr );
    }

    rw_data_buffer_free_wrap( p_rw_data_buffer );

    return SUCCESS;
}


_int32 fm_read_callback_xl( const MSG_INFO *msg_info, _int32 errcode )
{
    MSG_FILE_RW_PARA *p_user_para = (MSG_FILE_RW_PARA *)msg_info->_user_data;
    struct tagTMP_FILE *p_file_struct = p_user_para->_file_struct_ptr;
    OP_PARA_FS_RW *p_rw_para = NULL;

    LIST_ITERATOR cur_node = NULL;
    RW_DATA_BUFFER *p_list_block_item = NULL;

    RW_DATA_BUFFER *p_rw_data_buffer = p_user_para->_rw_data_buffer_ptr;
    notify_read_result p_read_call_back = (notify_read_result)p_rw_data_buffer->_call_back_func_ptr;
    void *p_user = p_rw_data_buffer->_user_ptr;
    p_rw_para = (OP_PARA_FS_RW *)msg_info->_operation_parameter;

    LOG_DEBUG( "fm_read_callback:errcode=%d.filename=%s/%s,filesize=%llu,offset=%llu,expect_size=%u,operated_size=%u",
               errcode,p_file_struct->_file_path,p_file_struct->_file_name,p_file_struct->_file_size,p_rw_para->_operating_offset,p_rw_para->_expect_size,p_rw_para->_operated_size);

    if( p_rw_data_buffer->_is_cancel ) return SUCCESS;
    //check the read range list
    cur_node = LIST_BEGIN( p_file_struct->_asyn_read_range_list );
    p_list_block_item = (RW_DATA_BUFFER *)LIST_VALUE( cur_node );
    LOG_DEBUG( "fm_read_callback. p_list_block_item:0x%x, p_block_data_buffer:0x%x, p_user_para:0x%x/n",
               p_list_block_item, p_rw_data_buffer, p_user_para );
    sd_assert( p_list_block_item == p_rw_data_buffer );

    //check asyn read op
    if( errcode == SUCCESS && p_rw_para->_expect_size != p_rw_para->_operated_size )
    {
        LOG_ERROR( "fm_read_callback error. expect_size:%u, operated_size:%u ",
                   p_rw_para->_expect_size, p_rw_para->_operated_size );
        return FM_RW_ERROR;
    }

    LOG_DEBUG( "fm_read_callback." );
    sd_assert( p_file_struct->_is_closing == FALSE );
    if( errcode == SUCCESS )
    {
        list_erase( &p_file_struct->_asyn_read_range_list, cur_node );
        sd_assert( p_rw_data_buffer->_is_call_back == TRUE );
        LOG_DEBUG( "fm_read_callback return SUCCESS." );
        p_read_call_back( p_file_struct, p_user, p_rw_data_buffer->_call_back_buffer_ptr, SUCCESS, p_rw_data_buffer->_range_buffer_ptr->_data_length );

        rw_data_buffer_free_wrap( p_rw_data_buffer );
    }
    else
    {
        p_rw_data_buffer->_try_num++;
        if( p_rw_data_buffer->_try_num == FM_ERR_TRY_TIME )
        {
            list_erase( &p_file_struct->_asyn_read_range_list, cur_node );
            p_read_call_back( p_file_struct, p_user, p_rw_data_buffer->_call_back_buffer_ptr, errcode, 0 );
            rw_data_buffer_free_wrap( p_rw_data_buffer );
            LOG_ERROR( "fm_read_callback exceed try num." );
            return SUCCESS;
        }
    }

    sd_assert( p_file_struct->_is_sending_read_op );
    return SUCCESS;
}

_int32 fm_close_callback_xl( const MSG_INFO *msg_info, _int32 errcode )
{
    MSG_FILE_CLOSE_PARA *p_user_para = NULL;
    struct tagTMP_FILE *p_file_struct = NULL;
    notify_file_close p_callback_func = NULL;
    _int32 ret_val = SUCCESS;

    p_user_para = msg_info->_user_data;
    p_file_struct = p_user_para->_file_struct_ptr;
    p_callback_func = p_user_para->_callback_func_ptr;

    LOG_DEBUG( "fm_close_callback:errcode=%d.filename=%s/%s,filesize=%llu",errcode,p_file_struct->_file_path,p_file_struct->_file_name,p_file_struct->_file_size);
#ifdef _WRITE_BUFFER_MERGE_TEST
    LOG_DEBUG( "fm_close_callback. write_statistic: malloc_range_size:%llu, write_num:%u, unmerge num:%u, unnormal_num:%u, num_2:%u, num_3:%u, num_4:%u, num_5:%u, num_6:%u, num_7:%u, num_8:%u",
               g_malloc_range_size, g_write_num, g_write_num_unmerge, g_write_num_unnormal, g_write_num_2, g_write_num_3, g_write_num_4, g_write_num_5, g_write_num_6, g_write_num_7, g_write_num_8 );
#endif
    ret_val = p_callback_func( p_file_struct, p_user_para->_user_ptr, errcode );
    CHECK_VALUE( ret_val );

    if( p_file_struct != NULL )
    {
        ret_val = tmp_file_free_wrap( p_file_struct );
        CHECK_VALUE( ret_val );
    }

    return SUCCESS;
}

_int32 fm_get_file_full_path( struct tagTMP_FILE *p_file_struct, char *file_full_path, _u32 buffer_size )
{
    _int32 ret_val = SUCCESS;
    _u32 str_len = 0;

    str_len = p_file_struct->_file_path_len + p_file_struct->_file_name_len;
    sd_assert( str_len != 0 );
    sd_assert( buffer_size > str_len );

    ret_val = sd_memcpy( file_full_path, p_file_struct->_file_path, p_file_struct->_file_path_len);
    CHECK_VALUE( ret_val );

    ret_val = sd_memcpy( file_full_path + p_file_struct->_file_path_len, p_file_struct->_file_name, p_file_struct->_file_name_len );
    CHECK_VALUE( ret_val );

    file_full_path[ str_len ] = '\0';

    LOG_DEBUG( "cm_get_file_full_path:%s. path_len:%u", file_full_path, str_len );
    return SUCCESS;
}

//#endif
