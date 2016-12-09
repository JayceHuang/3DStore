
#include "utility/define.h"
#ifdef ENABLE_BT

#include "bt_task.h"
#include "bt_query.h"
#include "bt_accelerate.h"
#include "../bt_utility/bt_define.h"
#include "task_manager/task_manager.h"
#include "res_query/res_query_interface.h"
#include "../bt_data_pipe/bt_pipe_interface.h"
#include "../bt_magnet/bt_magnet_task.h"
#include "reporter/reporter_interface.h"
#include "data_manager/data_manager_interface.h"

#include "utility/settings.h"
#include "utility/utility.h"
#include "utility/errcode.h"
#include "utility/string.h"
#include "utility/sd_assert.h"
#include "utility/time.h"
#include "utility/mempool.h"
#include "platform/sd_fs.h"

#include "utility/logid.h"

#define LOGID LOGID_BT_DOWNLOAD
#include "utility/logger.h"
#ifdef ENABLE_HSC
#include "high_speed_channel/hsc_info.h"
#endif
#include "../bt_magnet/bt_magnet_task.h"
#include "platform/sd_task.h"
static BOOL g_enable_bt_download = TRUE;

_int32 init_bt_download_module(void)
{
	_int32 ret_val = SUCCESS;
	LOG_ERROR( "init_bt_download_module." );
	
	ret_val = init_bt_task_slabs();
	LOG_ERROR( "init_bt_download_module:1:%d.", ret_val);
	CHECK_VALUE( ret_val );
	
	ret_val = init_bt_file_info_slabs();
	LOG_ERROR( "init_bt_download_module:2:%d.", ret_val);
	if( ret_val!=SUCCESS ) goto ErrorHanle;

	ret_val = init_bt_file_task_info_slabs();
	LOG_ERROR( "init_bt_download_module:3:%d.", ret_val);
	if( ret_val!=SUCCESS ) goto ErrorHanle;

	ret_val = init_bt_query_para_slabs();
	LOG_ERROR( "init_bt_download_module:4:%d.", ret_val);
	if( ret_val!=SUCCESS ) goto ErrorHanle;

	ret_val = init_tp_module();
	LOG_ERROR( "init_bt_download_module:5:%d.", ret_val);
	if( ret_val!=SUCCESS ) goto ErrorHanle;

	ret_val = init_bc_slabs();
	if (ret_val != SUCCESS) goto ErrorHanle;

	settings_get_bool_item( "bt.enable_bt_download", &g_enable_bt_download );
	LOG_ERROR( "init_bt_download_module:6:%d.", ret_val);
	
#ifdef ENABLE_BT_PROTOCOL
	ret_val = res_query_register_add_bt_res_handler(bt_add_bt_peer_resource);
	LOG_ERROR( "init_bt_download_module:7:%d.", ret_val);
	if( ret_val!=SUCCESS ) goto ErrorHanle;
#endif

#ifdef _DK_QUERY
	ret_val = res_query_register_dht_res_handler(bt_add_bt_peer_resource);
	LOG_ERROR( "init_bt_download_module:8:%d.", ret_val);
	if( ret_val!=SUCCESS ) goto ErrorHanle;
#endif

	ret_val = bdm_init_module();
	LOG_ERROR( "init_bt_download_module:9:%d.", ret_val);
	if( ret_val!=SUCCESS ) goto ErrorHanle;
	
	ret_val = init_bt_data_pipe();
	LOG_ERROR( "init_bt_download_module:10:%d.", ret_val);
	if( ret_val!=SUCCESS ) goto ErrorHanle;

	LOG_ERROR( "init_bt_download_module:end!");
	return SUCCESS;
	
ErrorHanle:
	uninit_bt_download_module();
	return ret_val;
}

_int32 uninit_bt_download_module(void)
{	
	LOG_DEBUG( "uninit_bt_download_module." );
	
	uninit_bt_data_pipe();
	
	bdm_uninit_module();

	uninit_tp_module();

	uninit_bc_slabs();

	uninit_bt_query_para_slabs();

	uninit_bt_file_task_info_slabs();

	uninit_bt_file_info_slabs();

	uninit_bt_task_slabs();
	

	return SUCCESS;
}

void bt_set_enable( BOOL is_enable )
{
	g_enable_bt_download = is_enable;
	settings_set_int_item( "bt.enable_bt_download", g_enable_bt_download );
	LOG_DEBUG( "bt_set_enable:%d", is_enable );
}

_int32 bt_create_task( TASK **pp_task )
{
	_int32 ret_val = SUCCESS;
	BT_TASK *p_bt_task = NULL;
	ret_val = bt_task_malloc_wrap( (BT_TASK **)pp_task );
	CHECK_VALUE( ret_val );
    sd_memset( *pp_task, 0, sizeof(BT_TASK) );
	p_bt_task = (BT_TASK*)*pp_task;

	map_init( &p_bt_task->_file_info_map, &file_task_info_map_compare );
	map_init( &p_bt_task->_file_task_info_map, &file_task_info_map_compare );
	list_init( &p_bt_task->_query_para_list );
	return SUCCESS;
}

#if 0
_int32 bt_create_task( TM_BT_TASK *p_task_para, TASK **pp_task )
{
	_int32 ret_val = SUCCESS;
	BT_TASK *p_bt_task = NULL;
	TASK * p_task=NULL;
	_u8 * info_hash=NULL;
	BOOL is_torrent_parser_created = FALSE,is_data_manager_inited = FALSE,is_file_info_inited = FALSE,is_connect_manager_inited = FALSE;
	DS_DATA_INTEREFACE _ds_data_intere;
	SET download_file_index;
	_u32 file_count = 0;
	_u32 file_num = 0;
	char seed_title[ MAX_FILE_PATH_LEN ];
	_u32 title_len = MAX_FILE_PATH_LEN;
	enum ENCODING_MODE seed_encoding;

	if( !g_enable_bt_download ) return BT_DOWNLOAD_DISABLE;
		
	LOG_DEBUG( "bt_create_new_task:%s, data_path:%s ",p_task_para->_seed_full_path, p_task_para->_data_path );
       *pp_task = NULL;

	  if(!sd_file_exist(p_task_para->_data_path))
	  	return DT_ERR_INVALID_FILE_PATH;
	   
	ret_val = bt_task_malloc_wrap( &p_bt_task );
	CHECK_VALUE( ret_val );

	p_task=(TASK *) p_bt_task;

	sd_assert( p_task_para->_data_path_len <= MAX_FILE_PATH_LEN );
	sd_assert( p_task_para->_seed_full_path_len <= MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN );	

	sd_memset(p_bt_task,0,sizeof(BT_TASK));
	
	p_bt_task->_task.task_status = TASK_S_IDLE;
	p_bt_task->_task._task_type = BT_TASK_TYPE;

	set_init( &download_file_index, file_index_set_compare );
	for( file_count = 0; file_count < p_task_para->_file_num; file_count++ )
	{
		ret_val = set_insert_node( &download_file_index, (void *)p_task_para->_download_file_index_array[file_count]);
		if( ret_val != SUCCESS )
		{
			ret_val = BT_ERROR_FILE_INDEX;
			goto ErrorHanle;
		}
	}
	
	ret_val = tp_pre_parser_seed( p_task_para->_seed_full_path, p_task_para->_encoding_switch_mode, 
		&seed_encoding, &file_num, seed_title, &title_len );
	//sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS ) goto ErrorHanle;
	LOG_DEBUG( "tp_create SUCCESS." );

	ret_val = tp_create( &p_bt_task->_torrent_parser_ptr, p_task_para->_encoding_switch_mode, &download_file_index );
	if( ret_val!=SUCCESS ) goto ErrorHanle;
	is_torrent_parser_created=TRUE;

	tp_set_exist_seed_title_name( p_bt_task->_torrent_parser_ptr, seed_title, title_len );
	tp_force_set_seed_encoding( p_bt_task->_torrent_parser_ptr, seed_encoding );

	ret_val = tp_task_parse_seed( p_bt_task->_torrent_parser_ptr, p_task_para->_seed_full_path );
	if( ret_val!=SUCCESS ) goto ErrorHanle;

       ret_val = tp_get_file_info_hash(  p_bt_task->_torrent_parser_ptr, &info_hash );
	if( ret_val!=SUCCESS ) goto ErrorHanle;
	
	if(tm_is_task_exist_by_info_hash(info_hash) == TRUE)
       {
            ret_val = DT_ERR_ORIGIN_RESOURCE_EXIST;
	     goto ErrorHanle;
       }


	map_init( &p_bt_task->_file_info_map, &file_task_info_map_compare );
	
	map_init( &p_bt_task->_file_task_info_map, &file_task_info_map_compare );

       list_init( &p_bt_task->_query_para_list );
            
       //list_init( &p_bt_task->_tracker_url_list );
       p_bt_task->_file_num=p_task_para->_file_num;
	ret_val = bt_file_info_for_user_malloc_wrap(&(p_bt_task->_file_info_for_user), p_task_para->_file_num,p_task_para->_download_file_index_array);
	if( ret_val!=SUCCESS ) goto ErrorHanle;
	
	ret_val = init_customed_rw_sharebrd(p_bt_task->_file_info_for_user, &p_bt_task->_brd_ptr);
	if( ret_val!=SUCCESS ) goto ErrorHanle;
           
	ret_val = bt_init_tracker_url_list( p_bt_task );
	if( ret_val!=SUCCESS ) goto ErrorHanle;
	
	ret_val = bt_init_file_info( p_bt_task, p_task_para );
	if( ret_val!=SUCCESS ) goto ErrorHanle;
	is_file_info_inited=TRUE;
	
 	ret_val = cm_init_bt_connect_manager(  &(p_task->_connect_manager),&(p_bt_task->_data_manager),bdm_can_abandon_resource);
	if( ret_val!=SUCCESS ) goto ErrorHanle;
	is_connect_manager_inited=TRUE;

	ret_val = bdm_init_struct(  &(p_bt_task->_data_manager),p_task,p_bt_task->_torrent_parser_ptr,
		p_task_para->_download_file_index_array,p_task_para->_file_num,p_task_para->_data_path,p_task_para->_data_path_len);
	if( ret_val!=SUCCESS ) goto ErrorHanle;
	is_data_manager_inited=TRUE;
	
      sd_memset(&_ds_data_intere, 0 , sizeof(DS_DATA_INTEREFACE));
       ret_val = bdm_get_dispatcher_data_info(&(p_bt_task->_data_manager), &_ds_data_intere);
	CHECK_VALUE(ret_val);
	
	ret_val = ds_init_dispatcher(  &(p_task->_dispatcher),&_ds_data_intere,&(p_task->_connect_manager));
	if( ret_val!=SUCCESS ) goto ErrorHanle;

	//bt_update_file_info( p_bt_task );
	
#ifdef EMBED_REPORT
	sd_memset( &p_task->_res_query_report_para, 0, sizeof(RES_QUERY_REPORT_PARA) );
#endif		

	*pp_task = (TASK *)p_bt_task;
#ifdef ENABLE_HSC
	hsc_init_info(&p_task->_hsc_info);
#endif
	set_clear(&download_file_index);
	
	LOG_DEBUG( "bt_create_new_task:%s, data_path:%s SUCCESS!!",p_task_para->_seed_full_path, p_task_para->_data_path );
	return SUCCESS;



ErrorHanle:
	if(ret_val !=SUCCESS)
	{
		if(is_torrent_parser_created)
			tp_destroy( p_bt_task->_torrent_parser_ptr);
		
		bt_clear_tracker_url_list( p_bt_task );

		if(p_bt_task->_file_info_for_user!=NULL)
			bt_file_info_for_user_free_wrap(p_bt_task->_file_info_for_user);

		if(p_bt_task->_brd_ptr!=NULL)
			uninit_customed_rw_sharebrd(p_bt_task->_brd_ptr);

		if(is_file_info_inited)
			bt_uninit_file_info( p_bt_task );
		
		//ds_unit_dispatcher( &(p_task->_dispatcher));
		if(is_connect_manager_inited)
			cm_uninit_bt_connect_manager( &(p_task->_connect_manager) );
		
		if(is_data_manager_inited == TRUE)
		{
			bdm_uninit_struct(&(p_bt_task->_data_manager));
			/* Return but do not release the p_bt_task until the bt_notify_file_closing_result_event is called */
#ifndef IPAD_KANKAN
			*pp_task =(TASK*)p_bt_task;
#endif /* IPAD_KANKAN */
		}
		else
		{
			bt_task_free_wrap(p_bt_task);
		}
	}

	set_clear(&download_file_index);
	LOG_DEBUG( "bt_create_new_task:%s, data_path:%s Error!! ret_val=%d,is_data_manager_inited=%d",p_task_para->_seed_full_path, p_task_para->_data_path ,ret_val,is_data_manager_inited);
	return ret_val;

}
#endif

_int32 bt_init_task( TM_BT_TASK *p_task_para, TASK *p_task, BOOL *is_wait_close )
{
	_int32 ret_val = SUCCESS;
	BT_TASK *p_bt_task = NULL;
	_u8 * info_hash=NULL;
	BOOL is_torrent_parser_created = FALSE,is_data_manager_inited = FALSE,is_file_info_inited = FALSE,is_connect_manager_inited = FALSE;
	DS_DATA_INTEREFACE _ds_data_intere;
	SET download_file_index;
	_u32 file_count = 0;
	_u32 file_num = 0;
	char seed_title[ MAX_FILE_PATH_LEN ];
	_u32 title_len = MAX_FILE_PATH_LEN;
	enum ENCODING_MODE seed_encoding;

	if( !g_enable_bt_download ) return BT_DOWNLOAD_DISABLE;
		
	LOG_DEBUG( "bt_init_task:%s, data_path:%s ",p_task_para->_seed_full_path, p_task_para->_data_path );

	  if(!sd_file_exist(p_task_para->_data_path))
	  	return DT_ERR_INVALID_FILE_PATH;
    
	p_bt_task=(BT_TASK *) p_task;

	sd_assert( p_task_para->_data_path_len <= MAX_FILE_PATH_LEN );
	sd_assert( p_task_para->_seed_full_path_len <= MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN );	

	p_bt_task->_task.task_status = TASK_S_IDLE;
	p_bt_task->_task._task_type = BT_TASK_TYPE;
    p_bt_task->_task._extern_id = -1;
    p_bt_task->_task._create_time = 0;
    p_bt_task->_task._is_continue = FALSE;

	set_init( &download_file_index, file_index_set_compare );
	for( file_count = 0; file_count < p_task_para->_file_num; file_count++ )
	{
		ret_val = set_insert_node( &download_file_index, (void *)p_task_para->_download_file_index_array[file_count]);
		if( ret_val != SUCCESS )
		{
			ret_val = BT_ERROR_FILE_INDEX;
			goto ErrorHanle;
		}
	}
	

	p_bt_task->_torrent_parser_ptr = hptp_torrent_init(p_bt_task->_torrent_parser_ptr);
	ret_val = tp_task_parse_seed( p_bt_task->_torrent_parser_ptr, p_task_para->_seed_full_path );
	if( ret_val!=SUCCESS ) goto ErrorHanle;

       ret_val = tp_get_file_info_hash(  p_bt_task->_torrent_parser_ptr, &info_hash );
	if( ret_val!=SUCCESS ) goto ErrorHanle;
	
	if(!(p_bt_task->_is_magnet_task) && tm_is_task_exist_by_info_hash(info_hash) == TRUE)
       {
            ret_val = DT_ERR_ORIGIN_RESOURCE_EXIST;
        	goto ErrorHanle;
       }
        p_bt_task->_file_num=p_task_para->_file_num;
	ret_val = bt_file_info_for_user_malloc_wrap(&(p_bt_task->_file_info_for_user), p_task_para->_file_num,p_task_para->_download_file_index_array);
	if( ret_val!=SUCCESS ) goto ErrorHanle;
	
	ret_val = init_customed_rw_sharebrd(p_bt_task->_file_info_for_user, &p_bt_task->_brd_ptr);
	if( ret_val!=SUCCESS ) goto ErrorHanle;
           
	ret_val = bt_init_tracker_url_list( p_bt_task );
	if( ret_val!=SUCCESS ) goto ErrorHanle;
	
	ret_val = bt_init_file_info( p_bt_task, p_task_para );
	if( ret_val!=SUCCESS ) goto ErrorHanle;
	is_file_info_inited=TRUE;
	
 	ret_val = cm_init_bt_connect_manager(  &(p_task->_connect_manager),&(p_bt_task->_data_manager),bdm_can_abandon_resource);
	if( ret_val!=SUCCESS ) goto ErrorHanle;
	is_connect_manager_inited=TRUE;
	p_task->_connect_manager._main_task_ptr = p_task;

	ret_val = bdm_init_struct(  &(p_bt_task->_data_manager)
        , p_task
        , p_bt_task->_torrent_parser_ptr
        , p_task_para->_download_file_index_array
        , p_task_para->_file_num
        , p_task_para->_data_path
        , p_task_para->_data_path_len);
	if( ret_val!=SUCCESS ) goto ErrorHanle;
	is_data_manager_inited=TRUE;
	
      sd_memset(&_ds_data_intere, 0 , sizeof(DS_DATA_INTEREFACE));
       ret_val = bdm_get_dispatcher_data_info(&(p_bt_task->_data_manager), &_ds_data_intere);
	CHECK_VALUE(ret_val);
	
	ret_val = ds_init_dispatcher(  &(p_task->_dispatcher),&_ds_data_intere,&(p_task->_connect_manager));
	if( ret_val!=SUCCESS ) goto ErrorHanle;
	
#ifdef EMBED_REPORT
	sd_memset( &p_task->_res_query_report_para, 0, sizeof(RES_QUERY_REPORT_PARA) );
#endif		
	set_clear(&download_file_index);

	LOG_DEBUG( "bt_init_task:%s, data_path:%s SUCCESS!!",p_task_para->_seed_full_path, p_task_para->_data_path );
	return SUCCESS;

ErrorHanle:
	if(ret_val !=SUCCESS)
	{
		if(is_torrent_parser_created)
			tp_destroy( p_bt_task->_torrent_parser_ptr);
		
		bt_clear_tracker_url_list( p_bt_task );

		if(p_bt_task->_file_info_for_user!=NULL)
			bt_file_info_for_user_free_wrap(p_bt_task->_file_info_for_user);

		if(p_bt_task->_brd_ptr!=NULL)
			uninit_customed_rw_sharebrd(p_bt_task->_brd_ptr);

		if(is_file_info_inited)
			bt_uninit_file_info( p_bt_task );
		
		//ds_unit_dispatcher( &(p_task->_dispatcher));
		if(is_connect_manager_inited)
			cm_uninit_bt_connect_manager( &(p_task->_connect_manager) );
		
		if(is_data_manager_inited == TRUE)
		{
			bdm_uninit_struct(&(p_bt_task->_data_manager));
			/* Return but do not release the p_bt_task until the bt_notify_file_closing_result_event is called */
            *is_wait_close = TRUE;
		}
		else
		{
			bt_task_free_wrap(p_bt_task);
            *is_wait_close = FALSE;
		}
	}

	set_clear(&download_file_index);
	LOG_DEBUG( "bt_init_task:%s, data_path:%s Error!! ret_val=%d,is_data_manager_inited=%d",p_task_para->_seed_full_path, p_task_para->_data_path ,ret_val,is_data_manager_inited);
	return ret_val;

}

_int32 bt_init_magnet_task( TM_BT_MAGNET_TASK *p_task_para, TASK *p_task )
{
    _int32 ret_val = SUCCESS;
    BT_TASK *p_bt_task = NULL;

	LOG_DEBUG( "bt_init_magnet_task:_task_id=%u",p_task->_task_id);

    
	p_bt_task=(BT_TASK *) p_task;

	p_bt_task->_task.task_status = TASK_S_IDLE;
	p_bt_task->_task._task_type = BT_TASK_TYPE;
    p_bt_task->_encoding_switch_mode = p_task_para->_encoding_switch_mode;
    p_bt_task->_is_magnet_task = TRUE;
    
    ret_val = bm_create_task( p_task_para->_url, p_task_para->_data_path, p_task_para->_data_name
		, p_task_para->_bManual
        , &p_bt_task->_magnet_task_ptr 
        , p_task);
        
    return ret_val;
}

_int32 bt_init_magnet_core_task( TASK *p_task )
{
    TM_BT_TASK para;
    _int32 ret_val = SUCCESS;
    BT_TASK *p_bt_task = NULL;
    BOOL is_wait_close = FALSE;
    TORRENT_SEED_INFO *p_seed_info = NULL;
	TORRENT_FILE_INFO *p_fi_array = NULL;
    _u32 *file_index_array = NULL;
    _u32 i = 0;
    char *padding_str = "_____padding_file";
    DM_NOTIFY_FILE_NAME_CHANGED call_ptr = NULL;
        
	LOG_DEBUG( "bt_init_magnet_core_task:_task_id=%u",p_task->_task_id);
    
    sd_memset(&para, 0, sizeof(para));
	p_bt_task=(BT_TASK *) p_task;
    ret_val = bm_get_seed_full_path( p_bt_task->_magnet_task_ptr,
    para._seed_full_path, MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN );
    CHECK_VALUE( ret_val );

    para._seed_full_path_len = sd_strlen(para._seed_full_path);

    ret_val = bm_get_data_path( p_bt_task->_magnet_task_ptr,
    para._data_path, MAX_FILE_PATH_LEN );
    CHECK_VALUE( ret_val );

    para._data_path_len = sd_strlen(para._data_path);
    para._encoding_switch_mode = p_bt_task->_encoding_switch_mode;
    
	ret_val = tp_get_seed_info( para._seed_full_path, 
    para._encoding_switch_mode, &p_seed_info );
    CHECK_VALUE( ret_val );

    //notify magnet task name changed 
    dm_get_file_name_changed_callback(&call_ptr);

    if(NULL!=call_ptr && p_task->_extern_id!= -1)
    {
        LOG_DEBUG("bt_init_magnet_core_task - task id %d , title %s",
            p_task->_extern_id,p_seed_info->_title_name);
        
        call_ptr(p_task->_extern_id,p_seed_info->_title_name,p_seed_info->_title_name_len);          
	
	//ask etm to get the task name
	p_task->_file_create_status=FILE_CREATED_SUCCESS;
    }
    else
    {
        LOG_DEBUG("file name call back fail !!! call_ptr 0x%x, task id %d",
                    call_ptr,p_task->_extern_id);
    }
    //end notify magnet task name changed 
    
    ret_val = sd_malloc( p_seed_info->_file_num * sizeof(_u32), (void**)&file_index_array );
    if( ret_val != SUCCESS )
    {
        tp_release_seed_info(p_seed_info);
        return ret_val;
    }

    int iter = 0;
    for(p_fi_array = p_seed_info->_file_info_array_ptr[iter];
		iter< p_seed_info->_file_num
			&& i < BT_MAX_DOWNLOAD_FILES; 
		++iter)
    {
    	p_fi_array = p_seed_info->_file_info_array_ptr[iter];
		if(sd_strncmp (p_fi_array->_file_name, padding_str, sd_strlen (padding_str))
			 && p_fi_array->_file_size > MIN_BT_FILE_SIZE)
		{
        	file_index_array[i++] = p_fi_array->_file_index;
		}
    }
    
    para._download_file_index_array = file_index_array;
    para._file_num = i;
    
    ret_val = bt_init_task( &para, p_task, &is_wait_close );
    if( ret_val != SUCCESS )
    {
        p_task->task_status = TASK_S_FAILED;
        p_task->failed_code = BT_MAGNET_CORE_INIT_FAILED;
        //这里不去判断is_wait_close,需测试一下
    }

    tp_release_seed_info(p_seed_info);
    sd_free(file_index_array);
    
	return ret_val;
}

_int32 bt_start_task( struct _tag_task *p_task )
{
	struct tagBT_TASK *p_bt_task = NULL;
	
	LOG_DEBUG( "bt_start_task_wrap:_task_id=%u",p_task->_task_id);
	
	sd_assert( p_task->_task_type == BT_TASK_TYPE );
    
	sd_assert( p_task->task_status == TASK_S_IDLE );
    
	p_bt_task = (struct tagBT_TASK *)p_task;

    // 设置加速相关信息，自动就是多源加速的
	p_task->_connect_manager._main_task_ptr = p_task;
	p_task->_working_mode |= S_USE_NORMAL_MULTI_RES; 
	
    if( p_bt_task->_is_magnet_task && p_bt_task->_magnet_task_ptr)
    {
        p_bt_task->_task.task_status = TASK_S_RUNNING;
        return SUCCESS;
    }
    else if(g_enable_bt_download)
    {
        return bt_start_task_impl(p_task);
    }
    
	return BT_DOWNLOAD_DISABLE;
}

_int32 bt_start_task_impl( struct _tag_task *p_task )
{
	_int32 ret_val = SUCCESS,ret_val_info = SUCCESS,ret_val_tracker = SUCCESS,ret_val_dht = SUCCESS;
	struct tagBT_TASK *p_bt_task = NULL;
	
	LOG_DEBUG( "bt_start_task:_task_id=%u",p_task->_task_id);
	
	sd_assert( p_task->_task_type == BT_TASK_TYPE );
	p_bt_task = (struct tagBT_TASK *)p_task;
	
#ifdef ENABLE_BT_PROTOCOL
	ret_val_tracker = bt_start_res_query_bt_tracker( p_bt_task );
#endif
	
	ret_val_dht = bt_start_res_query_dht( p_bt_task );

	if(ret_val_info!=SUCCESS&&ret_val_tracker!=SUCCESS&&ret_val_dht!=SUCCESS)
		CHECK_VALUE(ret_val_tracker) ;
	
	ret_val = sd_time( &p_task->_start_time );
	if( ret_val!=SUCCESS ) goto ErrorHanle;

	ret_val = ds_start_dispatcher(&(p_task->_dispatcher));
	if( ret_val!=SUCCESS ) goto ErrorHanle;

	p_bt_task->_task.task_status = TASK_S_RUNNING;

	LOG_DEBUG( "bt_start_task:_task_id=%u SUCCESS!",p_task->_task_id);
	return SUCCESS;
	
ErrorHanle:
	bt_stop_query_hub( p_bt_task );
    
#ifdef ENABLE_BT_PROTOCOL
	bt_stop_res_query_bt_tracker(p_bt_task );
#endif

	bt_stop_res_query_dht( p_bt_task );
	dt_failure_exit( ( TASK*)p_bt_task,ret_val );
	LOG_DEBUG( "bt_start_task:_task_id=%u Error!,ret_val=%d",p_task->_task_id,ret_val);
	return ret_val;
}

_int32 bt_stop_task( struct _tag_task *p_task )
{
	struct tagBT_TASK *p_bt_task = NULL;
	
	LOG_DEBUG( "bt_stop_task:_task_id=%u",p_task->_task_id);
	
	sd_assert( p_task->_task_type == BT_TASK_TYPE );
    
	p_bt_task = (struct tagBT_TASK *)p_task;
	p_bt_task->_task.task_status = TASK_S_STOPPED;
    if( p_bt_task->_is_magnet_task && p_bt_task->_magnet_task_ptr)
    {
        return SUCCESS;
    }
    else if(g_enable_bt_download) // 如果bt被禁用，则不会进行初始化，所以不用去真正去stop task
    {
		return bt_stop_task_impl(p_task);
    }
    
	return SUCCESS;
}


_int32 bt_stop_task_impl( struct _tag_task *p_task )
{
	struct tagBT_TASK *p_bt_task = NULL;
	LOG_DEBUG( "bt_stop_task_impl:_task_id=%u",p_task->_task_id);
	
	sd_assert( p_task->_task_type == BT_TASK_TYPE );
	p_bt_task = (struct tagBT_TASK *)p_task;
	
#ifdef EMBED_REPORT
	emb_reporter_bt_stop_task(p_task );
    	bdm_stop_report_running_sub_file(&p_bt_task->_data_manager);
	reporter_task_bt_stat(p_task);
#endif

	bt_stop_query_hub( p_bt_task );

#ifdef ENABLE_BT_PROTOCOL
	bt_stop_res_query_bt_tracker(p_bt_task );
#endif

	bt_stop_res_query_dht( p_bt_task );


	bt_clear_accelerate( p_bt_task );
	
	ds_stop_dispatcher(&(p_task->_dispatcher));
	
	p_bt_task->_task.task_status = TASK_S_STOPPED;

	LOG_DEBUG("The task is stoped!_task_id=%u,_start_time=%u,_stop_time=%u",p_task->_task_id,p_task->_start_time,p_task->_finished_time);

	return SUCCESS;

}

_int32 bt_update_info( struct _tag_task *p_task )
{
    struct tagBT_TASK *p_bt_task = NULL;
    _int32 ret_val = SUCCESS;
    MAGNET_TASK_STATUS status;
    BOOL tried_to_commit_magnet = FALSE;
     
    LOG_DEBUG( "bt_update_info_wrap:_task_id=%u",p_task->_task_id);

    sd_assert( p_task->_task_type == BT_TASK_TYPE );

    p_bt_task = (struct tagBT_TASK *)p_task;
    if(p_bt_task->_is_magnet_task && p_bt_task->_magnet_task_ptr)
    {
        ret_val = bm_get_task_status( p_bt_task->_magnet_task_ptr, &status );
        CHECK_VALUE( ret_val );

        if( status == MAGNET_TASK_SUCCESS )
		{
			/*don't auto change to the core bt task*/
			if(p_bt_task->_magnet_task_ptr->_bManual)
			{
				if(FILE_CREATED_SUCCESS != p_task->_file_create_status)
				{
					_int32 res;
					char seed_full_path[MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN];
					_u32 file_id;
					seed_full_path[0]=0;
					res = bm_get_seed_full_path( p_bt_task->_magnet_task_ptr,seed_full_path,sizeof(seed_full_path));
					if(SUCCESS==res)
					{
						res=sd_open_ex(seed_full_path,0,&file_id);
						if(SUCCESS==res)
						{
							sd_filesize(file_id,&p_task->file_size);
							p_task->_downloaded_data_size = p_task->file_size;
							sd_close_ex(file_id);
						}
					}
					p_task->_file_create_status = FILE_CREATED_SUCCESS;
					p_task->task_status = TASK_S_SUCCESS;
				}
			}
			else
			{
				tried_to_commit_magnet = p_bt_task->_magnet_task_ptr->_tried_commit_to_lixian;
				ret_val = bt_init_magnet_core_task( p_task );
				LOG_ERROR("bt_update_info[0x%X], bt_init_magnet_core_task ret:%d",p_task, ret_val);
				if( ret_val != SUCCESS ) 
				{
					p_task->task_status = TASK_S_FAILED;
					p_task->failed_code = BT_MAGNET_CORE_INIT_FAILED;
				}
				else
				{
					ret_val = tm_add_bt_file_info_to_pool( p_task);
					CHECK_VALUE(ret_val);

					bm_destory_task( p_bt_task->_magnet_task_ptr );
					p_bt_task->_magnet_task_ptr = NULL;
					ret_val = bt_start_task_impl( p_task);
					LOG_ERROR("bt_update_info[0x%X], bt_start_task_impl ret:%d",p_task, ret_val);
					if(tried_to_commit_magnet)
					{
						sd_assert(FALSE);
						return -1;
						//lixian_commit_new_bt(p_bt_task);
					}
				}
			}
		}
        else if( status == MAGNET_TASK_FAIL )
        {
           p_task->task_status = TASK_S_FAILED;
           p_task->failed_code = BT_MAGNET_FAILED;
        }

        if( status == MAGNET_TASK_FAIL)
        {
           bm_destory_task( p_bt_task->_magnet_task_ptr );
           p_bt_task->_magnet_task_ptr = NULL;
        }
        return ret_val;
    }
    else
    {
        return bt_update_info_impl(p_task);
    }
    
	return SUCCESS;
}

_int32 bt_update_info_impl( struct _tag_task *p_task )
{
	_int32 ret_val = SUCCESS;
	struct tagBT_TASK *p_bt_task = NULL;
	sd_assert( p_task->_task_type == BT_TASK_TYPE );
	p_bt_task = (struct tagBT_TASK *)p_task;
	
	LOG_DEBUG( "bt_update_info:_task_id=%u",p_task->_task_id);
	ret_val = bt_update_task_info( p_bt_task );
	CHECK_VALUE( ret_val );

	return SUCCESS;
}

_int32 bt_delete_task( struct _tag_task *p_task )
{
	struct tagBT_TASK *p_bt_task = NULL;
	
	LOG_URGENT( "bt_delete_task_wrap:_task_id=%u",p_task->_task_id);
	
	sd_assert( p_task->_task_type == BT_TASK_TYPE );
    
	p_bt_task = (struct tagBT_TASK *)p_task;
    if( p_bt_task->_is_magnet_task && p_bt_task->_magnet_task_ptr)
    {
        bm_destory_task( p_bt_task->_magnet_task_ptr );
        p_bt_task->_magnet_task_ptr = NULL;
        p_bt_task->_task.task_status = TASK_S_STOPPED;
        bt_task_free_wrap(p_bt_task);
        return SUCCESS;
    }
	else if (p_task->failed_code == BT_MAGNET_FAILED)
	{
		p_bt_task->_task.task_status = TASK_S_STOPPED;
        bt_task_free_wrap(p_bt_task);
		return SUCCESS;
	}
    else if(g_enable_bt_download)
    {
        return bt_delete_task_impl(p_task);
    }
    
	return SUCCESS;
}

_int32 bt_delete_task_impl( struct _tag_task *p_task )
{
    
	struct tagBT_TASK *p_bt_task = NULL;
	
	LOG_DEBUG( "bt_delete_task:_task_id=%u",p_task->_task_id);
	
	sd_assert( p_task->_task_type == BT_TASK_TYPE );
	p_bt_task = (struct tagBT_TASK *)p_task;
	
	sd_assert( p_bt_task->_task.task_status == TASK_S_STOPPED || p_bt_task->_task.task_status == TASK_S_IDLE );
	p_bt_task->_is_deleting_task = TRUE;

	bt_file_info_for_user_free_wrap(p_bt_task->_file_info_for_user);

	uninit_customed_rw_sharebrd(p_bt_task->_brd_ptr);
	
	bt_clear_tracker_url_list( p_bt_task );

	ds_unit_dispatcher( &(p_task->_dispatcher));
	
	cm_uninit_bt_connect_manager( &(p_task->_connect_manager) );
		
	bdm_uninit_struct(&(p_bt_task->_data_manager));

	LOG_DEBUG( "bt_delete_task return SUCCESS." );
	
	return TM_ERR_WAIT_FOR_SIGNAL;
}


BOOL bt_is_task_exist_by_info_hash(struct _tag_task *p_task,const _u8* info_hash)
{
	_u8 * my_info_hash = NULL;
	struct tagBT_TASK *p_bt_task = (struct tagBT_TASK *)p_task;
	_int32 ret_val= SUCCESS;
	LOG_DEBUG( "bt_is_task_exist_by_info_hash:_task_id=%u",p_task->_task_id);
	
       ret_val = tp_get_file_info_hash(  p_bt_task->_torrent_parser_ptr, &my_info_hash );
	CHECK_VALUE( ret_val );

	return sd_is_cid_equal(my_info_hash,info_hash);
}
BOOL bt_is_task_exist_by_gcid( struct _tag_task *p_task, const _u8* gcid)
{
	MAP_ITERATOR cur_node;
	struct tagBT_TASK *p_bt_task = NULL;
	_u32 file_index=0;
	_u8 my_gcid[CID_SIZE];
	
	LOG_DEBUG( "bt_is_task_exist_by_gcid:_task_id=%u",p_task->_task_id);
	
	sd_assert( p_task->_task_type == BT_TASK_TYPE );
	p_bt_task = (struct tagBT_TASK *)p_task;
	
	if(p_bt_task->_is_magnet_task && p_bt_task->_magnet_task_ptr) return FALSE;

	cur_node = MAP_BEGIN( p_bt_task->_file_info_map );

	while( cur_node != MAP_END( p_bt_task->_file_info_map ) )
	{
		file_index = (_u32)MAP_KEY( cur_node );
		if(TRUE==bdm_get_shub_gcid(&(p_bt_task->_data_manager),  file_index, my_gcid ))
		{
			if(TRUE==sd_is_cid_equal(my_gcid,gcid))
				return TRUE;
		}
		else
		if(TRUE==bdm_get_calc_gcid( &(p_bt_task->_data_manager),  file_index, my_gcid))
		{
			if(TRUE==sd_is_cid_equal(my_gcid,gcid))
				return TRUE;
		}
			
		cur_node = MAP_NEXT( p_bt_task->_file_info_map, cur_node );
	}
	
	return FALSE;	
}

BOOL bt_is_task_exist_by_cid( struct _tag_task *p_task, const _u8* cid)
{
	MAP_ITERATOR cur_node;
	struct tagBT_TASK *p_bt_task = NULL;
	_u32 file_index=0;
	_u8 my_cid[CID_SIZE];
	
	LOG_DEBUG( "bt_is_task_exist_by_cid:_task_id=%u",p_task->_task_id);
	
	sd_assert( p_task->_task_type == BT_TASK_TYPE );
	p_bt_task = (struct tagBT_TASK *)p_task;
	
	

	cur_node = MAP_BEGIN( p_bt_task->_file_info_map );

	while( cur_node != MAP_END( p_bt_task->_file_info_map ) )
	{
		file_index = (_u32)MAP_KEY( cur_node );
		if(TRUE== bdm_get_cid(&(p_bt_task->_data_manager),  file_index, my_cid ))
		{
			if(TRUE==sd_is_cid_equal(my_cid,cid))
				return TRUE;
		}
			
		cur_node = MAP_NEXT( p_bt_task->_file_info_map, cur_node );
	}
	
	return FALSE;	
}

CONNECT_MANAGER *  bt_get_task_connect_manager( struct _tag_task *p_task, const _u8* gcid,_u32 * file_index)
{
	MAP_ITERATOR cur_node;
	struct tagBT_TASK *p_bt_task = NULL;
	_u32 file_id=0;
	_u8 my_gcid[CID_SIZE];
	CONNECT_MANAGER * p_cm=NULL;
	
	LOG_DEBUG( "bt_get_task_connect_manager:_task_id=%u",p_task->_task_id);
	
	sd_assert( p_task->_task_type == BT_TASK_TYPE );
	p_bt_task = (struct tagBT_TASK *)p_task;
	
	

	cur_node = MAP_BEGIN( p_bt_task->_file_info_map );

	while( cur_node != MAP_END( p_bt_task->_file_info_map ) )
	{
		file_id = (_u32)MAP_KEY( cur_node );
		if(TRUE==bdm_get_shub_gcid(&(p_bt_task->_data_manager),  file_id, my_gcid ))
		{
			if(TRUE==sd_is_cid_equal(my_gcid,gcid))
			{
				p_cm=&(p_task->_connect_manager);
				*file_index=file_id;
				return p_cm;
			}
		}
		else
		if(TRUE==bdm_get_calc_gcid( &(p_bt_task->_data_manager),  file_id, my_gcid))
		{
			if(TRUE==sd_is_cid_equal(my_gcid,gcid))
			{
				p_cm=&(p_task->_connect_manager);
				*file_index=file_id;
				return p_cm;
			}
		}
			
		cur_node = MAP_NEXT( p_bt_task->_file_info_map, cur_node );
	}
	
	return NULL;	
}

_int32 bt_get_file_info(  struct  tag_BT_FILE_INFO_POOL* p_bt_file_info_pool, _u32 file_index, void *p_file_info )
{
	_int32 ret_val = SUCCESS;
	
	LOG_DEBUG( "bt_get_file_info:_task_id=%u,file_index=%u." ,p_bt_file_info_pool->_task_id,file_index);
	
	ret_val = bt_file_info_for_user_read(p_bt_file_info_pool, file_index, p_file_info );
	//CHECK_VALUE( ret_val );

	return ret_val;
	
}
_int32 bt_get_download_file_index( struct _tag_task *p_task, _u32 *buffer_len, _u32 *file_index_buffer )
{
	_u32 need_download_file_num = 0;
	struct tagBT_TASK *p_bt_task = NULL;
	MAP_ITERATOR cur_node;
	_u32 file_index = 0;
	_u32 file_num = 0;
	
	sd_assert( p_task->_task_type == BT_TASK_TYPE );
	p_bt_task = (struct tagBT_TASK *)p_task;

	LOG_DEBUG( "bt_get_download_file_index:_task_id=%u",p_task->_task_id);
	need_download_file_num = map_size( &p_bt_task->_file_info_map );

	sd_memset(file_index_buffer,0,(*buffer_len)*sizeof(_u32));

	if( need_download_file_num > *buffer_len )
	{
		*buffer_len = need_download_file_num;
		return TM_ERR_BUFFER_NOT_ENOUGH;
	}
	else
	if( need_download_file_num < *buffer_len )
	{
		*buffer_len = need_download_file_num;
	}
		
	if( file_index_buffer == NULL )
	{
		return TM_ERR_INVALID_PARAMETER;
	}
	
	cur_node = MAP_BEGIN( p_bt_task->_file_info_map );
	while( cur_node != MAP_END( p_bt_task->_file_info_map ) && file_num < need_download_file_num )
	{
		file_index = (_u32)MAP_KEY( cur_node );
		
		LOG_DEBUG( "bt_get_download_file_index. file_index:0x%x", file_index);	
		file_index_buffer[ file_num ] = file_index;
		cur_node = MAP_NEXT( p_bt_task->_file_info_map, cur_node );
		file_num++;
	}	
	return SUCCESS;
	
}

_int32 bt_notify_file_closing_result_event(TASK* p_task)
{
	_int32 ret_val = SUCCESS;
	struct tagBT_TASK *p_bt_task = NULL;

	if(p_task==NULL) return DT_ERR_INVALID_DOWNLOAD_TASK;
	LOG_DEBUG("bt_notify_file_closing_result_event(_task_id=%u)",p_task->_task_id);

	p_bt_task = (BT_TASK *)p_task;
	
	LOG_URGENT("bt_notify_file_closing_result_event:_task_id=%u,need_remove_tmp_file=%d",p_task->_task_id,p_task->need_remove_tmp_file);
	/* Check if need to remove the temp files */
	if(p_task->need_remove_tmp_file == TRUE)
	{
		 bt_delete_tmp_file( p_bt_task );
	}
		bt_uninit_file_info( p_bt_task );
		
		tp_destroy( p_bt_task->_torrent_parser_ptr );
	
		p_bt_task->_torrent_parser_ptr = NULL;
	

		ret_val = bt_task_free_wrap(p_bt_task);
		if(ret_val!=SUCCESS)
		{
			LOG_DEBUG("Fatal Error calling: bt_task_free_wrap(p_bt_task)");

		}

		/* Release the signal sevent  */
		tm_signal_sevent_handle();
		
	return SUCCESS;
}


_int32 bt_notify_report_task_success(TASK* p_task)
{
	//struct tagBT_TASK *p_bt_task = NULL;

	if(p_task==NULL) return DT_ERR_INVALID_DOWNLOAD_TASK;
	LOG_DEBUG("bt_notify_file_closing_result_event(_task_id=%u)",p_task->_task_id);

	//p_bt_task = (BT_TASK *)p_task;
	
	LOG_DEBUG("bt_notify_file_closing_result_event:_task_id=%u,need_remove_tmp_file=%d",p_task->_task_id,p_task->need_remove_tmp_file);
		/* report the task information to shub */
		
#ifdef EMBED_REPORT
	reporter_bt_task_download_stat(p_task);
	emb_reporter_bt_task_download_stat(p_task);
#endif

	return SUCCESS;
}

_int32 bt_query_hub_for_single_file( TASK* p_task,_u32 file_index )
{
	_int32 ret_val = SUCCESS;
	BT_TASK *p_bt_task = (BT_TASK*)p_task;
	BT_FILE_INFO *p_file_info=NULL;

	if(p_task==NULL) return DT_ERR_INVALID_DOWNLOAD_TASK;
	LOG_DEBUG("bt_query_hub_for_single_file(_task_id=%u,file_index=%u)",p_task->_task_id,file_index);

	p_bt_task = (BT_TASK *)p_task;
	
	ret_val = map_find_node(&(p_bt_task->_file_info_map), (void*)file_index,(void**)&p_file_info);
	CHECK_VALUE( ret_val );
	sd_assert(p_file_info!=NULL);

	if((p_file_info->_query_result != RES_QUERY_REQING)&&(p_file_info->_query_result != RES_QUERY_FAILED))
	{
		ret_val = bt_start_query_hub_for_single_file( p_bt_task,p_file_info, file_index );
		CHECK_VALUE( ret_val );
	}
	
		
	return SUCCESS;
}

#ifdef ENABLE_BT_PROTOCOL
_int32 bt_notify_have_piece(TASK* p_task, _u32 piece_index )
{
	return cm_notify_have_piece( &(p_task->_connect_manager), piece_index );
}
#endif

void bt_get_file_info_pool(TASK* p_task, struct  tag_BT_FILE_INFO_POOL* p_bt_file_info_pool )
{
	BT_TASK *p_bt_task = (BT_TASK*)p_task;
	p_bt_file_info_pool->_task_id= p_bt_task->_task._task_id;
	p_bt_file_info_pool->_file_num = p_bt_task->_file_num;
	p_bt_file_info_pool->_brd_ptr= p_bt_task->_brd_ptr;
	p_bt_file_info_pool->_file_info_for_user= p_bt_task->_file_info_for_user;
}

_int32 bt_get_task_file_path_and_name(TASK* p_task, _u32 file_index,char *file_path_buffer, _int32 *file_path_buffer_size, char *file_name_buffer, _int32* file_name_buffer_size)
{
	BT_TASK *p_bt_task = (BT_TASK*)p_task;
	LOG_DEBUG("bt_get_task_file_path_and_name");
	return tp_get_file_path_and_name(p_bt_task->_torrent_parser_ptr,file_index,file_path_buffer,(_u32*)file_path_buffer_size,file_name_buffer,(_u32*)file_name_buffer_size);
}

_int32 bt_get_tmp_file_path_name(TASK* p_task, char* tmp_file_path, char* tmp_file_name)
{
	BT_TASK *p_bt_task = (BT_TASK*)p_task;
    if(NULL!=p_bt_task->_magnet_task_ptr)
	{
		return TM_ERR_INVALID_FILE_PATH;
	}
	sd_strncpy(tmp_file_path, p_bt_task->_data_manager._bt_piece_check._tmp_file._file_path, MAX_FILE_PATH_LEN);
	sd_strncpy(tmp_file_name, p_bt_task->_data_manager._bt_piece_check._tmp_file._file_name,MAX_FILE_NAME_LEN);

	LOG_DEBUG("bt_get_tmp_file_path path:%s, %s", tmp_file_path, tmp_file_name);
	return 0;
}


#ifdef ENABLE_BT_PROTOCOL

_int32 bt_update_pipe_can_download_range( TASK* p_task)
{
	return cm_update_pipe_can_download_range( &(p_task->_connect_manager));
}
#endif

_int32 bt_report_single_file( TASK* p_task,_u32 file_index, BOOL is_success )
{
	_int32 ret_val = SUCCESS;
	BT_TASK *p_bt_task = (BT_TASK*)p_task;
	BT_FILE_INFO *p_file_info=NULL;
	
	LOG_DEBUG("bt_report_single_file..._p_task=0x%X,task_id=%u,file_index=%u",p_task,p_task->_task_id,file_index);

	ret_val = map_find_node(&(p_bt_task->_file_info_map), (void*)file_index,(void**)&p_file_info);
	CHECK_VALUE( ret_val );
	sd_assert(p_file_info!=NULL);
	if( is_success )
	{
		if(p_file_info->_has_record==FALSE)
		{
			LOG_DEBUG("New resource,report to server...");
			#ifdef EMBED_REPORT
				reporter_bt_insert_res(p_task, file_index);
			#endif
		}
			
#ifdef UPLOAD_ENABLE
		bt_add_record_to_upload_manager( p_task,file_index );
#endif

#ifdef EMBED_REPORT
		reporter_bt_download_stat(p_task, file_index);
		emb_reporter_bt_file_download_stat(p_task,file_index);
#endif	
	}
	else
	{
#ifdef EMBED_REPORT
		emb_reporter_bt_file_download_fail(p_task,file_index);
#endif		
	}

	return SUCCESS;
}

_int32 bt_get_sub_file_tcid( struct _tag_task *p_task, _u32 file_index, _u8 cid[CID_SIZE])
{
	struct tagBT_TASK *p_bt_task = NULL;
	
	LOG_DEBUG( "bt_get_sub_file_tcid:_task_id=%u,%u",p_task->_task_id,file_index);
	
	sd_assert( p_task->_task_type == BT_TASK_TYPE );
    
	p_bt_task = (struct tagBT_TASK *)p_task;

	if(bdm_get_cid(&p_bt_task->_data_manager,  file_index,  cid))
	{
		return SUCCESS;
	}
	
	return BT_DATA_GET_CID_ERROR;
}

//jjxh added...
_int32 bt_get_sub_file_gcid(struct _tag_task* p_task, _u32 file_index, _u8 gcid[CID_SIZE])
{

	struct tagBT_TASK *p_bt_task = NULL;

	LOG_DEBUG( "bt_get_sub_file_gcid:_task_id=%u,%u",p_task->_task_id,file_index);

	sd_assert( p_task->_task_type == BT_TASK_TYPE );

	p_bt_task = (struct tagBT_TASK *)p_task;

	if(TRUE == bdm_get_shub_gcid(&p_bt_task->_data_manager,  file_index,  gcid))
	{
		return SUCCESS;
	}
	else if (TRUE == bdm_get_calc_gcid(&p_bt_task->_data_manager, file_index, gcid))
	{
		return SUCCESS;
	}
	return BT_DATA_GET_CID_ERROR;
}


_int32 bt_get_sub_file_name( struct _tag_task *p_task, _u32 file_index, char * name_buffer,_u32 * buffer_size)
{
	_int32 ret_val = SUCCESS;
	char * p_file_name = NULL;
	struct tagBT_TASK *p_bt_task = NULL;
	
	LOG_DEBUG( "bt_get_sub_file_name:_task_id=%u,%u",p_task->_task_id,file_index);
	
	sd_assert( p_task->_task_type == BT_TASK_TYPE );
    
	p_bt_task = (struct tagBT_TASK *)p_task;

	ret_val = bdm_get_file_name( &p_bt_task->_data_manager,  file_index, &p_file_name );
	CHECK_VALUE(ret_val);

	if(sd_strlen(p_file_name)>=*buffer_size)
	{
		*buffer_size = sd_strlen(p_file_name);
		return BUFFER_OVERFLOW;
	}
	
	sd_strncpy(name_buffer, p_file_name, (*buffer_size)-1);
	
	return SUCCESS;
}

_int32 bt_get_seed_title_name(TASK* p_task, char *file_name_buffer, _int32* buffer_size)
{
    if( file_name_buffer == NULL || (p_task->_task_type != BT_TASK_TYPE && p_task->_task_type != BT_MAGNET_TASK_TYPE))
    {
            return TM_ERR_INVALID_PARAMETER;
    }

    BT_TASK *bt_task = (BT_TASK *)p_task;
    
    _int32 len = 0;
    char *name = NULL;
    
    if(bt_task->_torrent_parser_ptr != NULL)
    {
        tp_get_seed_title_name(bt_task->_torrent_parser_ptr,&name);

        if(name!=NULL)
        {
            len = sd_strlen(name);
            
            if( len > (*buffer_size) )
            {
                len = (_int32)(*buffer_size);
            }

            sd_strncpy(file_name_buffer,name,len);

            return SUCCESS;
        }           
    }
	else
	{
		if(bt_task->_is_magnet_task &&
			(NULL!=bt_task->_magnet_task_ptr)&&
			(bt_task->_magnet_task_ptr->_bManual))
		{
			if(sd_strlen(bt_task->_magnet_task_ptr->_data_manager._file_name) < (*buffer_size) )
			{
				sd_strncpy(file_name_buffer,bt_task->_magnet_task_ptr->_data_manager._file_name,*buffer_size);
				return SUCCESS;
			}
		}
	}
    return -1;   

}

#endif /* #ifdef ENABLE_BT */

