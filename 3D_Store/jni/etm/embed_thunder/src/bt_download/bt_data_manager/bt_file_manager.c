#include "utility/define.h"
#ifdef ENABLE_BT 


#include "bt_file_manager.h"
#include "torrent_parser/torrent_parser_interface.h"
#include "bt_data_manager_imp.h"

#include "data_manager/file_manager_interface.h"
#include "data_manager/file_info.h"
#include "data_manager/data_receive.h"
#include "data_manager/et_utility.h"

#include "utility/mempool.h"
#include "utility/settings.h"
#include "utility/time.h"

#include "utility/utility.h"
#include "platform/sd_fs.h"

#include "bt_data_manager_interface.h"

#include "p2p_data_pipe/p2p_pipe_interface.h"
#include "http_data_pipe/http_resource.h"

#include "utility/logid.h"
#define LOGID LOGID_BT_FILE_MANAGER
#include "utility/logger.h"
#include "utility/define_const_num.h"

#include "reporter/reporter_interface.h"

static _u32 g_bt_sub_file_idle_ticks = 600;
static _u32 g_bt_sub_file_max_num = 3;
static _u64 g_bt_max_cur_downloading_size = 1024 * 1024 * 1024;

void bfm_init_module(void)
{
	
	g_bt_max_cur_downloading_size = MAX_CUR_DOWNLOADING_SIZE;

	settings_get_int_item( "bt.sub_file_idle_ticks", (_int32 *)&g_bt_sub_file_idle_ticks );
	settings_get_int_item( "bt.sub_file_max_num", (_int32 *)&g_bt_sub_file_max_num );
	settings_get_int_item( "bt.max_cur_downloading_size", (_int32 *)&g_bt_max_cur_downloading_size );
}


_int32 bfm_init_struct( BT_FILE_MANAGER *p_bt_file_manager, 
	struct tagBT_DATA_MANAGER *p_bt_data_manager, struct tagTORRENT_PARSER *p_torrent_parser,
	_u32 *p_need_download_file_array, _u32 need_download_file_num,
	char *p_data_path, _u32 data_path_len )
{
	_int32 ret_val = SUCCESS;
	_u32 file_pos = 0;
	_u64 file_size = 0;
	MAP_ITERATOR cur_node = NULL;
	BT_SUB_FILE *p_bt_sub_file = NULL;
	BOOL is_success = TRUE;
	
	ret_val = sd_strncpy( p_bt_file_manager->_data_path, p_data_path, MAX_FILE_PATH_LEN );
	CHECK_VALUE( ret_val );

	p_bt_file_manager->_is_start_finished_task = FALSE;
	p_bt_file_manager->_is_report_task = FALSE;

	p_bt_file_manager->_data_path_len = data_path_len;
	p_bt_file_manager->_cur_file_info_num = 0;

	p_bt_file_manager->_file_total_download_bytes = 0;
	p_bt_file_manager->_file_total_writed_bytes = 0;
	
	p_bt_file_manager->_bt_data_manager_ptr = p_bt_data_manager;
	p_bt_file_manager->_torrent_parser_ptr = p_torrent_parser;
	
	p_bt_file_manager->_close_need_call_back = FALSE;
	p_bt_file_manager->_is_closing = FALSE;
	p_bt_file_manager->_cur_range_step = 0;
	p_bt_file_manager->_big_sub_file_ptr = NULL;
	p_bt_file_manager->_server_dl_bytes = 0;
	p_bt_file_manager->_peer_dl_bytes = 0;
	p_bt_file_manager->_cdn_dl_bytes = 0;
	p_bt_file_manager->_normal_cdn_bytes = 0;
    p_bt_file_manager->_origin_dl_bytes = 0;
    p_bt_file_manager->_lixian_dl_bytes = 0;

    
	range_list_init(&p_bt_file_manager->_3part_cid_list);


	ret_val = bfm_create_bt_sub_file_struct( p_bt_file_manager, p_torrent_parser, p_need_download_file_array, need_download_file_num );
	if( ret_val != SUCCESS )
	{
		bfm_destroy_bt_sub_file_struct( p_bt_file_manager );
		return ret_val;
	}

	sd_assert( p_torrent_parser != NULL );
	if( p_torrent_parser == NULL ) return BT_DATA_MANAGER_LOGIC_ERROR;

	p_bt_file_manager->_file_total_size = tp_get_file_total_size( p_torrent_parser );

	p_bt_file_manager->_need_download_size = 0;
	p_bt_file_manager->_cur_downloading_file_size = 0;

	for( file_pos = 0; file_pos < need_download_file_num; file_pos++ )
	{
		file_size = 0;
		ret_val = tp_get_sub_file_size( p_torrent_parser, p_need_download_file_array[file_pos], &file_size );
		CHECK_VALUE( ret_val );

		p_bt_file_manager->_need_download_size += file_size;

		LOG_DEBUG( "bfm_init_struct. need_download_file:%u.",
			p_need_download_file_array[file_pos] );
	}
	
	LOG_DEBUG( "bfm_init_struct. need_download_file_num:%d, p_data_path:%s, data_path_len:%d, need_download_size:%llu ",
		need_download_file_num, p_data_path, data_path_len, p_bt_file_manager->_need_download_size );

	//ret_val = bfm_start_create_common_file_info( p_bt_file_manager );
	//sd_assert( ret_val == SUCCESS );

	cur_node = MAP_BEGIN( p_bt_file_manager->_bt_sub_file_map );

	while( cur_node != MAP_END( p_bt_file_manager->_bt_sub_file_map ) )
	{
		p_bt_sub_file = (BT_SUB_FILE *)MAP_VALUE( cur_node );
		LOG_DEBUG( " bfm_init_struct, file_index:%u, _file_status:%d, err_code:%d",
			p_bt_sub_file->_file_index, p_bt_sub_file->_file_status, p_bt_sub_file->_err_code );

		if( p_bt_sub_file->_file_status != BT_FILE_FINISHED )
		{
			is_success = FALSE;
		}

		cur_node = MAP_NEXT( p_bt_file_manager->_bt_sub_file_map, cur_node);
	}
	
	p_bt_file_manager->_is_start_finished_task = is_success;
	LOG_DEBUG( " bfm_init_struct,  _is_start_finished_task:%d",
		p_bt_file_manager->_is_start_finished_task );

	return SUCCESS;
}

_int32 bfm_uninit_struct( BT_FILE_MANAGER *p_bt_file_manager )
{
	_int32 ret_val = SUCCESS;

	MSG_INFO msg = { };
	_u32 msg_id = 0;
	LOG_DEBUG( " bfm_uninit_struct begin");

	bfm_handle_file_err_code( p_bt_file_manager );
	
	msg._device_id = (_u32)p_bt_file_manager;
	msg._user_data = 0;
	msg._device_type = DEVICE_NONE;
	msg._operation_type = OP_NONE;

	ret_val = post_message(&msg, bfm_notify_can_close, NOTICE_ONCE, 0, &msg_id);
	p_bt_file_manager->_close_need_call_back = TRUE;

	ret_val = bfm_stop_all_bt_sub_file_struct( p_bt_file_manager );
	sd_assert( ret_val == SUCCESS );
	p_bt_file_manager->_is_closing = TRUE;
	LOG_DEBUG( " bfm_uninit_struct end");

	
	return SUCCESS;
}


_int32 bfm_notify_can_close( const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid )
{
	BT_FILE_MANAGER *p_bt_file_manager = ( BT_FILE_MANAGER * )msg_info->_device_id;

    LOG_URGENT("bfm_notify_can_close, p_bt_file_manager : 0x%x  , msg id: %u  , errcode:%d.", p_bt_file_manager, msgid, errcode ); 
	p_bt_file_manager->_close_need_call_back = FALSE;
	
	bfm_nofity_file_close_success( p_bt_file_manager );

	return SUCCESS;
}

_int32 bfm_set_cid( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, _u8 cid[CID_SIZE] )
{
	FILEINFO *p_file_info = NULL;
	_int32 ret_val = SUCCESS;
	
	LOG_DEBUG("bfm_set_cid .file_index:%d", file_index );
	ret_val = bfm_get_file_info_ptr( p_bt_file_manager, file_index, &p_file_info );
	if( ret_val != SUCCESS )
	{
		return ret_val;
	}

	ret_val = file_info_set_cid( p_file_info, cid );
	CHECK_VALUE( ret_val );
	
	return SUCCESS;
}

BOOL bfm_get_cid( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, _u8 cid[CID_SIZE] )
{
	FILEINFO *p_file_info = NULL;
	_int32 ret_val = SUCCESS;
	BOOL ret_bool = FALSE; 
#ifdef _LOGGER  
	char cid_str[CID_SIZE*2+1];   
#endif 	  	

	ret_val = bfm_get_file_info_ptr( p_bt_file_manager, file_index, &p_file_info );
	if( ret_val != SUCCESS )
	{
		return FALSE;
	}

	LOG_DEBUG("bfm_get_cid .");

	if( file_info_get_shub_cid( p_file_info, cid ) == TRUE ) 
	{
#ifdef _LOGGER  
		str2hex((char*)cid, CID_SIZE, cid_str, CID_SIZE*2);
		cid_str[CID_SIZE*2] = '\0';   

		LOG_DEBUG( "bfm_get_cid, cid is valid, cid:%s.", cid_str );	 
#endif 				  
		return TRUE;	  
	}

    if(FALSE == range_list_is_contained(&p_file_info->_writed_range_list, &p_bt_file_manager->_3part_cid_list) 
        || FALSE == range_list_is_contained(&p_file_info->_done_ranges, &p_bt_file_manager->_3part_cid_list) )
    {
        return FALSE;
    }

	ret_bool = get_file_3_part_cid( p_file_info, cid, NULL );
	if( ret_bool )
	{		  
#ifdef _LOGGER  
		str2hex((char*)cid, CID_SIZE, cid_str, CID_SIZE*2);
		cid_str[CID_SIZE*2] = '\0';   
		 
		LOG_DEBUG("dm_get_cid, calc cid success, cid:%s.", cid_str);	 
#endif 			   
		file_info_set_cid( p_file_info, cid ) ; 

	}
	else
	{
		LOG_ERROR("dm_get_cid, calc cid failure."); 
	}

	return ret_bool;

}

BOOL bfm_get_bcids( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, _u32 *p_bcid_num, _u8 **pp_bcid_buffer )
{
	FILEINFO *p_file_info = NULL;
	_int32 ret_val = SUCCESS;
	BOOL ret_bool = FALSE;
	_u32  bcid_block_num = 0;
	_u8*  p_bcid_buffer = NULL;
	
	LOG_DEBUG("bfm_get_bcids .file_index:%d", file_index );
	
	ret_val = bfm_get_file_info_ptr( p_bt_file_manager, file_index, &p_file_info );
	if( ret_val != SUCCESS )
	{
		return FALSE;
	}

	if(file_info_bcid_valid( p_file_info ) == TRUE )
	{
		bcid_block_num = file_info_get_bcid_num( p_file_info );
		p_bcid_buffer =  file_info_get_bcid_buffer( p_file_info );
		ret_bool = TRUE;
		LOG_DEBUG("bfm_get_bcids, because bcid is valid , so  blocksize : %u , bcid buffer:0x%x .", bcid_block_num, p_bcid_buffer);
	}
	else
	{
		if( file_info_is_all_checked( p_file_info ) == FALSE )
		{
			LOG_DEBUG("bfm_get_bcids, because can not finish download, so no bcid .");
			ret_bool = FALSE;  
		}
		else
		{
			bcid_block_num =  file_info_get_bcid_num( p_file_info );
			p_bcid_buffer =  file_info_get_bcid_buffer( p_file_info );
			ret_bool = TRUE;
			LOG_DEBUG("bfm_get_bcids, because bcid is invalid and has finished download, so  blocksize : %u , bcid buffer:0x%x .", bcid_block_num, p_bcid_buffer);	
		}
	}

	*p_bcid_num = bcid_block_num;
	*pp_bcid_buffer = p_bcid_buffer;

	return ret_bool;	

}


_int32 bfm_set_hub_return_info2( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, _int32 gcid_type,  
							   unsigned char block_cid[], _u32 block_count, _u32 block_size )
{
	FILEINFO *p_file_info = NULL;
	_int32 ret_val = SUCCESS;
	BT_SUB_FILE *p_bt_sub_file = NULL;
	LOG_DEBUG("bfm_set_hub_return_info2 .file_index:%d, block_count:%u, block_size:%u",
		file_index, block_count, block_size );

	ret_val = bfm_get_bt_sub_file_ptr( p_bt_file_manager, file_index, &p_bt_sub_file );
	if( ret_val != SUCCESS ) return ret_val;

	p_file_info = p_bt_sub_file->_file_info_ptr;

	if( p_file_info == NULL )
	{
		return BT_FILE_INFO_NULL;
	}
	else if( p_bt_sub_file->_sub_file_info_status == FILE_INFO_CLOSING )
	{
		return SUCCESS;
	}

	ret_val = file_info_set_hub_return_info2( p_file_info, gcid_type, block_cid,
		block_count, block_size );
	CHECK_VALUE( ret_val );

	return SUCCESS;

}

_int32 bfm_set_hub_return_info( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, _int32 gcid_type,  
	unsigned char block_cid[], _u32 block_count, _u32 block_size, _u64 filesize )
{
	FILEINFO *p_file_info = NULL;
	_int32 ret_val = SUCCESS;
	BT_SUB_FILE *p_bt_sub_file = NULL;
	LOG_DEBUG("bfm_set_hub_return_info .file_index:%d, block_count:%u, block_size:%u",
		file_index, block_count, block_size );

	ret_val = bfm_get_bt_sub_file_ptr( p_bt_file_manager, file_index, &p_bt_sub_file );
	if( ret_val != SUCCESS ) return ret_val;

	p_file_info = p_bt_sub_file->_file_info_ptr;

	if( p_file_info == NULL )
	{
		return BT_FILE_INFO_NULL;
	}
	else if( p_bt_sub_file->_sub_file_info_status == FILE_INFO_CLOSING )
	{
		return SUCCESS;
	}

	ret_val = file_info_set_hub_return_info( p_file_info, filesize, gcid_type, block_cid,
		block_count, block_size );
	CHECK_VALUE( ret_val );
	p_bt_sub_file->_has_bcid = TRUE;

	start_check_blocks( p_file_info );

	return SUCCESS;
	
}

_int32 bfm_set_gcid( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, _u8 gcid[CID_SIZE] )
{
	FILEINFO *p_file_info = NULL;
	_int32 ret_val = SUCCESS;
	
	LOG_DEBUG("bfm_set_gcid .file_index:%d", file_index );
	ret_val = bfm_get_file_info_ptr( p_bt_file_manager, file_index, &p_file_info );
	if( ret_val != SUCCESS )
	{
		return ret_val;
	}


	ret_val = file_info_set_gcid( p_file_info, gcid );
	CHECK_VALUE( ret_val );
	
	return SUCCESS;
}

enum BT_FILE_STATUS bfm_get_file_status ( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index )
{
	BT_SUB_FILE *p_bt_sub_file = NULL;
	_int32 ret_val = SUCCESS;
	
	ret_val = bfm_get_bt_sub_file_ptr( p_bt_file_manager, file_index, &p_bt_sub_file );
	CHECK_VALUE( ret_val );
	
	LOG_DEBUG("bfm_get_file_status .file_index:%d, file_status:%d", 
		file_index, p_bt_sub_file->_file_status );
	
	sd_assert( p_bt_sub_file != NULL );
	if( p_bt_sub_file == NULL ) return BT_FILE_IDLE;
	return p_bt_sub_file->_file_status;
}

_u32  bfm_get_total_file_percent( BT_FILE_MANAGER *p_bt_file_manager )
{
	_u64 total_file_size = p_bt_file_manager->_need_download_size;
	_u32 percent = 0;
	//sd_assert( total_file_size > 0 );
	if( total_file_size == 0 ) return 100;
	sd_assert( p_bt_file_manager->_file_total_writed_bytes <= p_bt_file_manager->_need_download_size );
	percent = (_u64)p_bt_file_manager->_file_total_writed_bytes * 100 / total_file_size;

	LOG_DEBUG("bfm_get_total_file_percent .percent:%d", percent );

	return percent;
}

_u32  bfm_get_sub_file_percent( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index )
{
	BT_SUB_FILE *p_bt_sub_file = NULL;
	FILEINFO *p_file_info = NULL;
	_int32 ret_val = SUCCESS;
	_u32 file_percent = 0;
	
	ret_val = bfm_get_bt_sub_file_ptr( p_bt_file_manager, file_index, &p_bt_sub_file );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS ) return 0;

	p_file_info = p_bt_sub_file->_file_info_ptr;
	if( p_file_info == NULL )
	{
		sd_assert( p_bt_sub_file->_file_status != BT_FILE_DOWNLOADING );
		if( p_bt_sub_file->_file_size == 0 )
		{
			file_percent = 100;
		}
		else
		{
			file_percent = p_bt_sub_file->_file_writed_bytes * 100 / p_bt_sub_file->_file_size;
		}
	}
	else
	{
		file_percent = file_info_get_file_percent( p_file_info );
	}
	LOG_DEBUG("bfm_get_sub_file_percent .file_index:%u, file_percent:%d",
		p_bt_sub_file->_file_index, file_percent );
	return file_percent;		
	
}	

_u32 bfm_get_downloading_time( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index )
{
	BT_SUB_FILE *p_bt_sub_file = NULL;
	FILEINFO *p_file_info = NULL;
	_int32 ret_val = SUCCESS;
	_u32 downloading_time = 0;
	_u32 cur_time = 0;
	
	ret_val = bfm_get_bt_sub_file_ptr( p_bt_file_manager, file_index, &p_bt_sub_file );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS ) return 0;

	p_file_info = p_bt_sub_file->_file_info_ptr;
	downloading_time = p_bt_sub_file->_downloading_time;
	sd_time( &cur_time );
	
	if( p_file_info != NULL 
		&& cur_time > p_bt_sub_file->_start_time )
	{
		downloading_time += cur_time - p_bt_sub_file->_start_time;
	}
	LOG_DEBUG("bfm_get_downloading_time .file_index:%u, downloading_time:%d",
		p_bt_sub_file->_file_index, downloading_time );
	return downloading_time;	

}

_u32 bfm_get_sub_task_start_time(BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index)
{
    BT_SUB_FILE* p_bt_sub_file = NULL;
    _int32 ret_val = SUCCESS;
    ret_val = bfm_get_bt_sub_file_ptr(p_bt_file_manager, file_index, &p_bt_sub_file);
    sd_assert(ret_val == SUCCESS);
    if(ret_val != SUCCESS)
    {
        return SUCCESS;
    }
    return p_bt_sub_file->_sub_task_start_time;
}


_u64  bfm_get_total_file_download_data_size( BT_FILE_MANAGER *p_bt_file_manager )
{
	LOG_DEBUG("bfm_get_total_file_download_data_size ." );
	return p_bt_file_manager->_file_total_download_bytes;
}

_u64 bfm_get_sub_file_download_data_size( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index )
{
	BT_SUB_FILE *p_bt_sub_file = NULL;
	FILEINFO *p_file_info = NULL;
	_int32 ret_val = SUCCESS;
	_u64 download_data_size = 0;
	_u64 add_data_size = 0;
    _u32 now_time = 0;
	sd_time(&now_time);
	ret_val = bfm_get_bt_sub_file_ptr( p_bt_file_manager, file_index, &p_bt_sub_file );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS ) return 0;

	p_file_info = p_bt_sub_file->_file_info_ptr;
	if( p_file_info == NULL )
	{
		sd_assert( p_bt_sub_file->_file_status != BT_FILE_DOWNLOADING );
	}
	else
	{
		download_data_size = file_info_get_download_data_size( p_file_info );

        if(download_data_size >= p_bt_sub_file->_file_size 
            && 0 == p_bt_sub_file->_percent100_time)
        {
            p_bt_sub_file->_percent100_time = now_time;
        }

		if( download_data_size > p_bt_sub_file->_file_download_bytes )
		{
            LOG_DEBUG("bfm_get_sub_file_download_data_size, file_index = %d, download_data_size = %llu, p_bt_sub_file->_file_download_bytes = %llu"
                ,file_index ,download_data_size, p_bt_sub_file->_file_download_bytes);
            // 记录起速时间
            if(p_bt_sub_file->_first_zero_speed_duration == 0)
            {
                p_bt_sub_file->_first_zero_speed_duration = now_time - p_bt_sub_file->_sub_task_start_time;
                LOG_DEBUG("file_index = %d, p_bt_sub_file->_first_zero_speed_duration = %u"
                        , file_index, p_bt_sub_file->_first_zero_speed_duration );
            }

			add_data_size = download_data_size - p_bt_sub_file->_file_download_bytes;
			p_bt_file_manager->_file_total_download_bytes += add_data_size;
			add_speed_record( &p_bt_sub_file->_file_download_speed, add_data_size );
			p_bt_sub_file->_file_download_bytes = download_data_size;
        }// 没有收到数据
        else if(p_bt_sub_file->_last_stat_time)    
        {
            p_bt_sub_file->_other_zero_speed_duraiton += (now_time - p_bt_sub_file->_last_stat_time);
            LOG_DEBUG("file_index = %d, p_bt_sub_file->_other_zero_speed_duraiton= %u"
                , file_index, p_bt_sub_file->_other_zero_speed_duraiton);
        }
        
	}
    if(p_bt_sub_file->_first_zero_speed_duration != 0) // 起速之后开始统计_other_zero_speed_duraiton
        p_bt_sub_file->_last_stat_time = now_time;

	LOG_DEBUG("bfm_get_sub_file_download_data_size:file_index:%u, download_byte:%llu",
		file_index, p_bt_sub_file->_file_download_bytes );	
	return p_bt_sub_file->_file_download_bytes;

}		

void bfm_get_sub_file_stat_data(BT_FILE_MANAGER *p_bt_file_manager
                                , _u32 file_index
                                , _u32* first_zero_duration
                                , _u32* other_zero_duration
                                , _u32* percent100_time)

{
    BT_SUB_FILE *p_bt_sub_file = NULL;
    _int32 ret_val = SUCCESS;

    ret_val = bfm_get_bt_sub_file_ptr( p_bt_file_manager, file_index, &p_bt_sub_file );
    sd_assert( ret_val == SUCCESS );
    if (ret_val != SUCCESS)
    {
        return ;
    }
    *first_zero_duration = p_bt_sub_file->_first_zero_speed_duration;
    *other_zero_duration = p_bt_sub_file->_other_zero_speed_duraiton;
    *percent100_time = p_bt_sub_file->_percent100_time;
}

_u64  bfm_get_total_file_writed_data_size( BT_FILE_MANAGER *p_bt_file_manager )
{
	LOG_DEBUG("bfm_get_total_file_writed_data_size: %llu",
		p_bt_file_manager->_file_total_writed_bytes );	
	return p_bt_file_manager->_file_total_writed_bytes;
}

_u64  bfm_get_sub_file_writed_data_size( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index )
{
	BT_SUB_FILE *p_bt_sub_file = NULL;
	FILEINFO *p_file_info = NULL;
	_int32 ret_val = SUCCESS;
	_u64 writed_data_size = 0;
	
	ret_val = bfm_get_bt_sub_file_ptr( p_bt_file_manager, file_index, &p_bt_sub_file );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS ) return 0;

	p_file_info = p_bt_sub_file->_file_info_ptr;
	if( p_file_info == NULL )
	{
		sd_assert( p_bt_sub_file->_file_status != BT_FILE_DOWNLOADING );
	}
	else
	{
		writed_data_size = file_info_get_writed_data_size( p_file_info );
		if( writed_data_size > p_bt_sub_file->_file_writed_bytes )
		{
			p_bt_file_manager->_file_total_writed_bytes += writed_data_size - p_bt_sub_file->_file_writed_bytes;
			sd_assert( p_bt_file_manager->_file_total_writed_bytes <= p_bt_file_manager->_need_download_size );
			p_bt_sub_file->_file_writed_bytes = writed_data_size;		
		}
	}
	LOG_DEBUG("bfm_get_sub_file_writed_data_size:file_index:%u, writed_size: %llu.",
		file_index, p_bt_sub_file->_file_writed_bytes );		
	return p_bt_sub_file->_file_writed_bytes;

}	

BOOL bfm_is_file_info_opening( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index )
{
	BT_SUB_FILE *p_bt_sub_file = NULL;
	_int32 ret_val = SUCCESS;
	BOOL ret_bool = TRUE;
	
	ret_val = bfm_get_bt_sub_file_ptr( p_bt_file_manager, file_index, &p_bt_sub_file );
	if( ret_val != SUCCESS )
	{
		ret_bool = FALSE;	
	}			
	else if( p_bt_sub_file->_sub_file_info_status == FILE_INFO_OPENING )
	{
		ret_bool = TRUE;	
	}
	else
	{
		ret_bool = FALSE;	
	}
	LOG_DEBUG("bfm_is_file_info_opening, file_index:%u, is_opening: %d", file_index, ret_bool );	
	return ret_bool;
}	

BOOL bfm_bcid_is_valid( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index )
{
	FILEINFO *p_file_info = NULL;
	_int32 ret_val = SUCCESS;
	BOOL ret_bool = TRUE;
	
	ret_val = bfm_get_file_info_ptr( p_bt_file_manager, file_index, &p_file_info );
	LOG_DEBUG("bfm_bcid_is_valid: FALSE" );	
	
	if( ret_val != SUCCESS ) return FALSE;
			
	ret_bool = file_info_bcid_valid( p_file_info );
	
	LOG_DEBUG("bfm_bcid_is_valid: %d", ret_bool );	
	return ret_bool;
}	

/*
void bfm_erase_err_range( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, RANGE *p_range )
{
	FILEINFO *p_file_info = NULL;
	_int32 ret_val = SUCCESS;
	
	ret_val = bfm_get_file_info_ptr( p_bt_file_manager, file_index, &p_file_info );
	
	if( ret_val != SUCCESS ) return;
			
	LOG_DEBUG("bfm_erase_err_range: file_index:%u, range:[%u,%u]",
		file_index, p_range->_index, p_range->_num );	
	ret_val = file_info_erase_error_range( p_file_info, p_range );
	sd_assert( ret_val == SUCCESS );
}
*/

BOOL bfm_get_shub_gcid( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, _u8 gcid[CID_SIZE] )	
{
	FILEINFO *p_file_info = NULL;
	_int32 ret_val = SUCCESS;
	BOOL ret_bool = TRUE;
	
#ifdef _LOGGER  
	char  gcid_str[CID_SIZE*2+1];	  
#endif 			
	LOG_DEBUG("bfm_get_shub_gcid: %u.", file_index );	

	ret_val = bfm_get_file_info_ptr( p_bt_file_manager, file_index, &p_file_info );
	if( ret_val != SUCCESS ) return FALSE;

	if( file_info_get_shub_gcid( p_file_info, gcid ) == TRUE )		
	{
#ifdef _LOGGER  
		str2hex((char*)gcid, CID_SIZE, gcid_str, CID_SIZE*2);
		gcid_str[CID_SIZE*2] = '\0';	

		LOG_DEBUG("bfm_get_shub_gcid success, gcid:%s.", gcid_str); 	 
#endif
		ret_bool = TRUE;	 
	}
	else
	{
		LOG_DEBUG("bfm_get_shub_gcid,gcid is invalid, so can not get gcid.");
		ret_bool = FALSE;
	}	 
	LOG_DEBUG("bfm_get_shub_gcid: %u.",ret_bool );	
	return ret_bool;

}	

BOOL bfm_get_calc_gcid( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, _u8 gcid[CID_SIZE] )	
{
	FILEINFO *p_file_info = NULL;
	_int32 ret_val = SUCCESS;

	ret_val = bfm_get_file_info_ptr( p_bt_file_manager, file_index, &p_file_info );
	if( ret_val != SUCCESS ) return FALSE;

	LOG_DEBUG("bfm_get_shub_gcid .");	  
	return get_file_gcid( p_file_info, gcid );
}	

_int32 bfm_notify_create_speedup_file_info( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index )
{
	_int32 ret_val = SUCCESS;
	BT_SUB_FILE *p_bt_sub_file = NULL;

	ret_val = bfm_get_bt_sub_file_ptr( p_bt_file_manager, file_index, &p_bt_sub_file );
	CHECK_VALUE( ret_val );
/*	
	sd_assert( p_bt_sub_file->_file_status == BT_FILE_DOWNLOADING );
	sd_assert( p_bt_sub_file->_sub_file_info_status == FILE_INFO_OPENING
		|| p_bt_sub_file->_sub_file_info_status == FILE_INFO_OPENED );
*/
	LOG_DEBUG( " bfm_notify_create_speedup_file_info begin:file_index:%d, p_bt_sub_file->_sub_file_type:%d",
		file_index, p_bt_sub_file->_sub_file_type );
	/*
	if( p_bt_sub_file->_sub_file_type == IDLE_SUB_FILE_TYPE )
	{
		sd_assert( p_bt_sub_file->_file_status == BT_FILE_DOWNLOADING );
		sd_assert( p_bt_sub_file->_sub_file_info_status == FILE_INFO_OPENING
			|| p_bt_sub_file->_sub_file_info_status == FILE_INFO_OPENED );
		
		ret_val = bfm_create_file_info( p_bt_file_manager, p_bt_sub_file );
		CHECK_VALUE( ret_val );

		p_bt_sub_file->_sub_file_type = SPEEDUP_SUB_FILE_TYPE;
		p_bt_sub_file->_file_status = BT_FILE_DOWNLOADING;
		
		p_bt_file_manager->_cur_speedup_file_num++;

		sd_assert( p_bt_sub_file->_has_bcid );
		
		sd_assert( p_bt_sub_file->_file_info_ptr != NULL );
		p_range_list = file_info_get_recved_range_list( p_bt_sub_file->_file_info_ptr );
		bdm_file_manager_notify_start_file( p_bt_file_manager->_bt_data_manager_ptr, file_index, p_range_list, p_bt_sub_file->_has_bcid );
	}
	sd_assert( p_bt_sub_file->_sub_file_type == COMMON_SUB_FILE_TYPE );

	sd_assert( p_bt_sub_file->_file_status == BT_FILE_DOWNLOADING );
	sd_assert( p_bt_sub_file->_sub_file_info_status == FILE_INFO_OPENING
		|| p_bt_sub_file->_sub_file_info_status == FILE_INFO_OPENED );
	
	p_bt_sub_file->_sub_file_type = SPEEDUP_SUB_FILE_TYPE;
	sd_assert( p_bt_file_manager->_cur_common_file_num > 0 );
	p_bt_file_manager->_cur_common_file_num--;
	p_bt_file_manager->_cur_speedup_file_num++;

	LOG_DEBUG( " bfm_notify_create_speedup_file_info end: cur_common_file_num:%d, cur_speedup_file_num:%d",
		p_bt_file_manager->_cur_common_file_num, p_bt_file_manager->_cur_speedup_file_num );	
	*/
	if( p_bt_sub_file->_file_status != BT_FILE_DOWNLOADING )
	{
		//sd_assert( FALSE );
		return BT_DATA_MANAGER_LOGIC_ERROR;
	}
	if( p_bt_sub_file->_sub_file_info_status != FILE_INFO_OPENING
		&& p_bt_sub_file->_sub_file_info_status != FILE_INFO_OPENED )
	{
		return BT_DATA_MANAGER_LOGIC_ERROR;
	}

	return SUCCESS;
}

_int32 bfm_notify_delete_speedup_file_info( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index )
{
	_int32 ret_val = SUCCESS;
	BT_SUB_FILE *p_bt_sub_file = NULL;
	LOG_DEBUG( " bfm_notify_delete_speedup_file_info begin:file_index:%d", file_index );

	ret_val = bfm_get_bt_sub_file_ptr( p_bt_file_manager, file_index, &p_bt_sub_file );
	CHECK_VALUE( ret_val );

	p_bt_sub_file->_is_speedup_failed = TRUE;
	
	/*
	p_bt_sub_file = (BT_SUB_FILE *)MAP_VALUE( cur_node );
	sd_assert( p_bt_sub_file->_sub_file_type == SPEEDUP_SUB_FILE_TYPE );
	
	if( p_bt_sub_file->_sub_file_type != SPEEDUP_SUB_FILE_TYPE )
	{
		return SUCCESS;
	}
	if( p_bt_sub_file->_file_status != BT_FILE_DOWNLOADING )
	{
		return SUCCESS;
	}	


	ret_val = bfm_stop_sub_file( p_bt_file_manager, p_bt_sub_file );
	sd_assert( ret_val == SUCCESS );
	p_bt_sub_file->_sub_file_type = IDLE_SUB_FILE_TYPE;

	
	p_bt_sub_file->_sub_file_type = COMMON_SUB_FILE_TYPE;
	sd_assert( p_bt_file_manager->_cur_speedup_file_num > 0 );
	p_bt_file_manager->_cur_speedup_file_num--;
	if( p_bt_sub_file->_has_bt_completed_handled )
	{
		p_bt_sub_file->_file_status = BT_FILE_FAILURE;
	}
	else
	{
		p_bt_sub_file->_file_status = BT_FILE_IDLE;
	}
	LOG_DEBUG( " bfm_notify_delete_speedup_file_info begin:file_idex:%d,_has_bt_completed_handled:%d, _file_status:%d", 
		file_index,  p_bt_sub_file->_has_bt_completed_handled,p_bt_sub_file->_file_status);
	*/

	return SUCCESS;

}


_int32 bfm_notify_delete_range_data( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, RANGE *p_range )
{
	FILEINFO *p_file_info = NULL;
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( " bfm_notify_delete_range_data");

	ret_val = bfm_get_file_info_ptr( p_bt_file_manager, file_index, &p_file_info );
	if( ret_val != SUCCESS ) return SUCCESS;

	LOG_DEBUG("bfm_notify_delete_range_data: file_index:%u, range:[%u,%u]",
		file_index, p_range->_index, p_range->_num );

	ret_val = file_info_delete_range( p_file_info, p_range );
	CHECK_VALUE( ret_val );

	return SUCCESS;

}

_u32  bfm_get_sub_file_block_size( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index )
{
	FILEINFO *p_file_info = NULL;
	_int32 ret_val = SUCCESS;
	_u32 block_size = 0;

	ret_val = bfm_get_file_info_ptr( p_bt_file_manager, file_index, &p_file_info );
	sd_assert( ret_val == SUCCESS );

	if( ret_val == SUCCESS )
	{
		block_size = file_info_get_blocksize( p_file_info );
	}

	LOG_DEBUG("bfm_get_sub_file_block_size .file_index:%u, block_size:%d",
		file_index, block_size );
	return block_size;		
}	

_int32 bfm_put_data( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, const RANGE *p_range, 
	char **pp_data_buffer, _u32 data_len,_u32 buffer_len, RESOURCE *p_resource  )
{
	BT_SUB_FILE *p_bt_sub_file = NULL;
	_int32 ret_val = SUCCESS;

	LOG_DEBUG( "bfm_put_data: file_index:%u, range:[%u,%u],data_len:%u, buffer_len:%u", 
		file_index, p_range->_index, p_range->_num, data_len, buffer_len );	
	
	ret_val = bfm_get_bt_sub_file_ptr( p_bt_file_manager, file_index, &p_bt_sub_file );
	if( ret_val != SUCCESS )
	{
		return BT_ERROR_FILE_INDEX;	
	}
	else if( p_bt_sub_file->_sub_file_info_status == FILE_INFO_CLOSING
		|| p_bt_sub_file->_sub_file_info_status == FILE_INFO_CLOSED )
	{
		return BT_DATA_MANAGER_ERR_TIME;
	}
	else
	{
		sd_assert( p_bt_sub_file->_file_info_ptr != NULL );
		if( p_bt_sub_file->_file_info_ptr == NULL )
		{
			return BT_FILE_INFO_NULL;
		}
		
		ret_val = file_info_put_data( p_bt_sub_file->_file_info_ptr, 
                    p_range, pp_data_buffer, data_len, buffer_len );
		if( ret_val != SUCCESS )
		{
			LOG_ERROR( "bfm_put_data: file_index:%u, range:[%u,%u],data_len:%u, buffer_len:%u ERR!!!:%u", 
				file_index, p_range->_index, 
				p_range->_num, data_len, buffer_len, ret_val );
#ifdef EMBED_REPORT
			file_info_add_overloap_date( p_bt_sub_file->_file_info_ptr, data_len);
#endif
			return ret_val;
		}	
		else
		{
#ifdef EMBED_REPORT
			 file_info_handle_valid_data_report( p_bt_sub_file->_file_info_ptr, p_resource, data_len );
#endif			
		}
	}	

    // 资源数据统计
	if( p_resource == NULL ) return SUCCESS;
    
	LOG_DEBUG("bfm_put_data , the resource type = %d , is_cdn_resource = %d",p_resource->_resource_type, p2p_is_cdn_resource(p_resource));
	if( p_resource->_resource_type == HTTP_RESOURCE
		|| p_resource->_resource_type == FTP_RESOURCE )
	{
		if((p_resource->_resource_type == HTTP_RESOURCE)
            && http_resource_is_lixian((HTTP_SERVER_RESOURCE *)p_resource))
		{
			p_bt_sub_file->_lixian_dl_bytes+= data_len;
            p_bt_file_manager->_lixian_dl_bytes += data_len;
		}
		else
		{
			p_bt_sub_file->_server_dl_bytes += data_len;
            p_bt_file_manager->_server_dl_bytes += data_len;
		}
		//以后要将离线下载的数据从_server_dl_bytes中区分开来
	}
	else if( p_resource->_resource_type == PEER_RESOURCE )
	{
		LOG_DEBUG("bfm_put_data , p2p_get_from =%d", p2p_get_from(p_resource));
		if( p2p_get_from(p_resource) == P2P_FROM_CDN
          || p2p_get_from(p_resource) == P2P_FROM_PARTNER_CDN )
		{
			p_bt_sub_file->_peer_dl_bytes += data_len;
			p_bt_file_manager->_peer_dl_bytes += data_len;
		}
#ifdef ENABLE_HSC
		else if (p2p_get_from(p_resource) == P2P_FROM_VIP_HUB)
		{
			p_bt_file_manager->_bt_data_manager_ptr->_task_ptr->_hsc_info._dl_bytes += data_len;
			p_bt_file_manager->_cdn_dl_bytes += data_len;
			p_bt_sub_file->_cdn_dl_bytes += data_len;

		}
#endif
		else if (p2p_get_from(p_resource) == P2P_FROM_LIXIAN)
		{
			p_bt_sub_file->_lixian_dl_bytes+= data_len;
			
			//以后要将离线下载的数据从_peer_dl_bytes中区分开来			
			//p_bt_sub_file->_peer_dl_bytes += data_len;
			p_bt_file_manager->_lixian_dl_bytes += data_len;
			p_bt_file_manager->_bt_data_manager_ptr->_task_ptr->_main_task_lixian_info._dl_bytes += data_len;
		}
		else if(p2p_get_from(p_resource) == P2P_FROM_ASSIST_DOWNLOAD)
		{
			p_bt_file_manager->_peer_dl_bytes += data_len;
			p_bt_sub_file->_peer_dl_bytes += data_len;
			p_bt_file_manager->_assist_peer_dl_bytes += data_len;
		}
		else if (p2p_get_from(p_resource) == P2P_FROM_NORMAL_CDN)
		{
			p_bt_file_manager->_normal_cdn_bytes += data_len;
			p_bt_sub_file->_normal_cdn_bytes += data_len;
		}
		else
		{
			p_bt_sub_file->_peer_dl_bytes += data_len;
			p_bt_file_manager->_peer_dl_bytes += data_len;
		}
	}
	else if( p_resource->_resource_type == BT_RESOURCE_TYPE )
	{
		//p_bt_sub_file->_peer_dl_bytes += data_len;
		//p_bt_file_manager->_peer_dl_bytes += data_len;
       	p_bt_file_manager->_origin_dl_bytes += data_len;
       	p_bt_sub_file->_origion_dl_bytes += data_len;
	}
    else
    {
        p_bt_sub_file->_peer_dl_bytes += data_len;
        p_bt_file_manager->_peer_dl_bytes += data_len;
    }

	return SUCCESS;
}

_int32  bfm_read_data_buffer( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, 
	RANGE_DATA_BUFFER *p_data_buffer, notify_read_result p_call_back, void *p_user )
{
	BT_SUB_FILE *p_bt_sub_file = NULL;
	_int32 ret_val = SUCCESS;

	LOG_DEBUG( "bfm_read_data_buffer: file_index:%u, range:[%u,%u], data_len:%u, buffer_len:%u, p_data_buffer:0x%x", 
		file_index, p_data_buffer->_range._index, p_data_buffer->_range._num, 
		p_data_buffer->_data_length, p_data_buffer->_buffer_length, p_data_buffer->_data_ptr );	
	
	ret_val = bfm_get_bt_sub_file_ptr( p_bt_file_manager, file_index, &p_bt_sub_file );
	if( ret_val != SUCCESS )
	{
		return BT_ERROR_FILE_INDEX; 
	}
	else if( p_bt_sub_file->_sub_file_info_status != FILE_INFO_OPENED )
	{
		return BT_DATA_MANAGER_ERR_TIME;
	}
	else
	{
		sd_assert( p_bt_sub_file->_file_info_ptr != NULL );
		if( p_bt_sub_file->_file_info_ptr == NULL )
		{
			return BT_FILE_INFO_NULL;
		}
		ret_val =  file_info_asyn_read_data_buffer( p_bt_sub_file->_file_info_ptr, p_data_buffer, p_call_back, p_user );
		if( ret_val != SUCCESS )
		{
			LOG_ERROR( "bfm_read_data_buffer err!!!" );			
			return ret_val;
		}	
	}	

	return SUCCESS;
}

_int32 bfm_notify_flush_data( BT_FILE_MANAGER *p_bt_file_manager )
{
	MAP_ITERATOR cur_node = MAP_BEGIN( p_bt_file_manager->_bt_sub_file_map );
	BT_SUB_FILE *p_bt_sub_file = NULL;
	_int32 ret_val = SUCCESS;
	FILEINFO *p_file_info = NULL;

	LOG_DEBUG( " bfm_notify_flush_data");
	while( cur_node != MAP_END( p_bt_file_manager->_bt_sub_file_map ) )
	{
		p_bt_sub_file = (BT_SUB_FILE *)MAP_VALUE( cur_node );
		if( p_bt_sub_file->_file_status == BT_FILE_DOWNLOADING
			&& (p_bt_sub_file->_sub_file_info_status == FILE_INFO_OPENING
			|| p_bt_sub_file->_sub_file_info_status == FILE_INFO_OPENED) )
		{
			p_file_info = p_bt_sub_file->_file_info_ptr;
			sd_assert( p_file_info != NULL );
			if( p_file_info == NULL ) continue;
			
			ret_val = file_info_flush_data_to_file( p_file_info, FALSE );
			sd_assert( ret_val == SUCCESS );
		}
		cur_node = MAP_NEXT( p_bt_file_manager->_bt_sub_file_map, cur_node);
	}

	return SUCCESS;
}

_u64 bfm_get_total_file_size( BT_FILE_MANAGER *p_bt_file_manager )
{
	struct tagTORRENT_PARSER *p_torrent_parser = p_bt_file_manager->_torrent_parser_ptr;
	_u64 file_size = tp_get_file_total_size( p_torrent_parser );
	sd_assert( p_torrent_parser != NULL );
	if( p_torrent_parser == NULL )
	{
		return 0;
	}
	LOG_DEBUG( "bfm_get_total_file_size: file_size:%u", file_size);	
	return file_size;
}

_u64 bfm_get_sub_file_size( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index )
{
	_int32 ret_val = SUCCESS;
	BT_SUB_FILE *p_bt_sub_file = NULL;
	ret_val = bfm_get_bt_sub_file_ptr( p_bt_file_manager, file_index, &p_bt_sub_file );
	//sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS ) return 0;
	
	LOG_DEBUG( "bfm_get_sub_file_size: file_index = %u. file_size:%u", 
		file_index, p_bt_sub_file->_file_size );
	return p_bt_sub_file->_file_size;
}		

_u64 bfm_get_file_size( BT_FILE_MANAGER *p_bt_file_manager )
{
	LOG_DEBUG( "bfm_get_file_size. file_size:%u", p_bt_file_manager->_file_total_size );
	return p_bt_file_manager->_file_total_size;
}		

enum tagPIPE_FILE_TYPE bfm_get_sub_file_type( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index )
{
	_int32 ret_val = SUCCESS;
	char file_name[MAX_FILE_NAME_LEN];
	_u32 file_name_len = MAX_FILE_NAME_LEN;
	ret_val = tp_get_file_name( p_bt_file_manager->_torrent_parser_ptr, file_index, file_name, &file_name_len );
	sd_assert( ret_val == SUCCESS );
	LOG_DEBUG( " bfm_get_sub_file_type");

	if( ret_val != SUCCESS ) return PIPE_FILE_UNKNOWN_TYPE;
		
	if( sd_is_binary_file( file_name, NULL ) == TRUE )
	{
		return PIPE_FILE_BINARY_TYPE;
	}
	else
	{
		return PIPE_FILE_HTML_TYPE;
	}
}	

_int32 bfm_get_sub_file_cache_data( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, const RANGE *p_sub_file_range, struct _tag_range_data_buffer_list *p_ret_range_buffer )
{
	FILEINFO *p_file_info = NULL;
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "bfm_get_sub_file_cache_data begin.file_index:%u, range:[%u,%u], p_ret_range_buffer:0x%x",
		file_index, p_sub_file_range->_index, p_sub_file_range->_num, p_ret_range_buffer );
		
	ret_val = bfm_get_file_info_ptr( p_bt_file_manager, file_index, &p_file_info );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS )
	{
		return ret_val;
	}
	
	ret_val = file_info_get_range_data_buffer( p_file_info, *p_sub_file_range, p_ret_range_buffer );
	//CHECK_VALUE( ret_val );
	
	LOG_DEBUG( "bfm_get_sub_file_cache_data end.file_index:%u, range:[%u,%u], p_ret_range_buffer:0x%x",
		file_index, p_sub_file_range->_index, p_sub_file_range->_num, p_ret_range_buffer );
	
	return ret_val;
}


BOOL bfm_range_is_check( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, RANGE *p_sub_file_range )
{
	BT_SUB_FILE *p_bt_sub_file = NULL;
	_int32 ret_val = SUCCESS;
	BOOL ret_bool = TRUE;
	
	ret_val = bfm_get_bt_sub_file_ptr( p_bt_file_manager, file_index, &p_bt_sub_file );
	if( ret_val != SUCCESS )
	{
		ret_bool = FALSE;	
	}
	else if( p_bt_sub_file->_sub_file_info_status != FILE_INFO_OPENED )
	{
		ret_bool = FALSE;	
	}
	else
	{
		sd_assert( p_bt_sub_file->_file_info_ptr != NULL );
		ret_bool = file_info_range_is_check( p_bt_sub_file->_file_info_ptr, p_sub_file_range );
	}	
	
	LOG_DEBUG( "bfm_range_is_check, file_index:%d, range:[%u,%u], ret:%d", 
		file_index, p_sub_file_range->_index, p_sub_file_range->_num, ret_bool );
	return ret_bool;
}

BOOL bfm_range_is_can_read( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, RANGE *p_sub_file_range )
{
	BT_SUB_FILE *p_bt_sub_file = NULL;
	_int32 ret_val = SUCCESS;
	BOOL ret_bool = TRUE;
	
	ret_val = bfm_get_bt_sub_file_ptr( p_bt_file_manager, file_index, &p_bt_sub_file );
	if( ret_val != SUCCESS )
	{
		ret_bool = FALSE;	
	}
	else if( p_bt_sub_file->_sub_file_info_status != FILE_INFO_OPENED
		|| p_bt_sub_file->_has_bcid )
	{
		ret_bool = FALSE;	
		LOG_DEBUG( "bfm_range_is_can_read, file_index:%d, file_info:0x%x, has_bcid:%d, range:[%u,%u], ret:%d", 
			file_index, p_bt_sub_file->_file_info_ptr, p_bt_sub_file->_has_bcid,
			p_sub_file_range->_index, p_sub_file_range->_num, ret_bool );
	}
	LOG_DEBUG( "bfm_range_is_can_read, file_index:%d, range:[%u,%u], ret:%d", 
		file_index, p_sub_file_range->_index, 
		p_sub_file_range->_num, ret_bool );

	return ret_bool;
}

BOOL bfm_range_is_cached( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, RANGE *p_sub_file_range )
{
	BT_SUB_FILE *p_bt_sub_file = NULL;
	_int32 ret_val = SUCCESS;
	BOOL ret_bool = TRUE;
	
	ret_val = bfm_get_bt_sub_file_ptr( p_bt_file_manager, file_index, &p_bt_sub_file );
	if( ret_val != SUCCESS )
	{
		ret_bool = FALSE;	
	}
	else if( p_bt_sub_file->_sub_file_info_status != FILE_INFO_OPENED )
	{
		ret_bool = FALSE;	
	}
	else
	{
		sd_assert( p_bt_sub_file->_file_info_ptr != NULL );
		ret_bool = file_info_range_is_cached( p_bt_sub_file->_file_info_ptr, p_sub_file_range );
	}

	LOG_DEBUG( "bfm_range_is_cached, file_index:%d, range:[%u,%u], ret:%d", 
		file_index, p_sub_file_range->_index, p_sub_file_range->_num, ret_bool );
	return ret_bool;
}

void bfm_get_dl_size( BT_FILE_MANAGER *p_bt_file_manager, _u64 *p_server_dl_bytes, _u64 *p_peer_dl_bytes, _u64 *p_cdn_dl_bytes , _u64* p_origion_bytes)
{
	*p_server_dl_bytes = p_bt_file_manager->_server_dl_bytes;
	*p_peer_dl_bytes = p_bt_file_manager->_peer_dl_bytes;
	*p_cdn_dl_bytes = p_bt_file_manager->_cdn_dl_bytes;
	*p_origion_bytes = p_bt_file_manager->_origin_dl_bytes;
}

_u64 bfm_get_normal_cdn_size(BT_FILE_MANAGER *p_bt_file_manager)
{
	return p_bt_file_manager->_normal_cdn_bytes;
}

void bfm_get_sub_file_dl_size( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, _u64 *p_server_dl_bytes, _u64 *p_peer_dl_bytes, _u64 *p_cdn_dl_bytes , _u64 *p_lixian_dl_bytes, _u64* p_origion_dl_bytes)
{

	_int32 ret_val = SUCCESS;
	BT_SUB_FILE *p_bt_sub_file = NULL;
	ret_val = bfm_get_bt_sub_file_ptr( p_bt_file_manager, file_index, &p_bt_sub_file );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS ) return;
	
	LOG_DEBUG( "bfm_get_sub_file_server_dl_size: file_index = %u.", file_index );

	if(p_server_dl_bytes)
		*p_server_dl_bytes = p_bt_sub_file->_server_dl_bytes;

	if(p_peer_dl_bytes)
		*p_peer_dl_bytes = p_bt_sub_file->_peer_dl_bytes;

	if(p_cdn_dl_bytes)
		*p_cdn_dl_bytes = p_bt_sub_file->_cdn_dl_bytes;

	if(p_lixian_dl_bytes)
		*p_lixian_dl_bytes = p_bt_sub_file->_lixian_dl_bytes;

    if(p_origion_dl_bytes)
	    *p_origion_dl_bytes = p_bt_sub_file->_origion_dl_bytes;
}

_u64 bfm_get_sub_file_normal_cdn_size(BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index)
{
	_int32 ret_val = SUCCESS;
	BT_SUB_FILE *p_bt_sub_file = NULL;
	
	ret_val = bfm_get_bt_sub_file_ptr(p_bt_file_manager, file_index, &p_bt_sub_file);
	sd_assert(ret_val == SUCCESS);
	if (ret_val != SUCCESS) return -1;

	LOG_DEBUG( "bfm_get_sub_file_server_dl_size: file_index = %u.", file_index );

	return p_bt_sub_file->_normal_cdn_bytes;
}

BOOL bfm_is_waited_success_close_file(BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index)
{
	_int32 ret_val = SUCCESS;
	BT_SUB_FILE *p_bt_sub_file = NULL;
	ret_val = bfm_get_bt_sub_file_ptr( p_bt_file_manager, file_index, &p_bt_sub_file );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS ) return FALSE;
	
	LOG_DEBUG( "bfm_is_waited_success_close_file: file_index = %u.waited_success_close_file:%d", 
		p_bt_sub_file->_file_index, p_bt_sub_file->_waited_success_close_file );	
	return p_bt_sub_file->_waited_success_close_file;
}

BOOL  bfm_range_is_recv( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, RANGE *p_range )
{
	BT_SUB_FILE *p_bt_sub_file = NULL;
	_int32 ret_val = SUCCESS;
	BOOL ret_bool = TRUE;
	
	ret_val = bfm_get_bt_sub_file_ptr( p_bt_file_manager, file_index, &p_bt_sub_file );
	if( ret_val != SUCCESS )
	{
		ret_bool = FALSE;	
	}
	else if( p_bt_sub_file->_sub_file_info_status != FILE_INFO_OPENED )
	{
		ret_bool = FALSE;	
	}
	else
	{
		sd_assert( p_bt_sub_file->_file_info_ptr != NULL );
		ret_bool = file_info_range_is_recv( p_bt_sub_file->_file_info_ptr, p_range );
	}	
	
	LOG_DEBUG( "bfm_range_is_recv, file_index:%d, range:[%u,%u], ret:%d", 
		file_index, p_range->_index, p_range->_num, ret_bool );
	
	return ret_bool;
	
}

RANGE_LIST *bfm_get_resved_range_list( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index )
{
	FILEINFO *p_file_info = NULL;
	RANGE_LIST *p_range_list = NULL;
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "bfm_get_resved_range_list, file_index:%u", file_index );
		
	ret_val = bfm_get_file_info_ptr( p_bt_file_manager, file_index, &p_file_info );
	if( ret_val == SUCCESS )
	{
		p_range_list = file_info_get_recved_range_list( p_file_info );
	}

	return p_range_list;
}

RANGE_LIST *bfm_get_checked_range_list( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index )
{
	FILEINFO *p_file_info = NULL;
	RANGE_LIST *p_range_list = NULL;
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "bfm_get_checked_range_list, file_index:%u", file_index );
		
	ret_val = bfm_get_file_info_ptr( p_bt_file_manager, file_index, &p_file_info );
	if( ret_val == SUCCESS )
	{
		p_range_list = file_info_get_checked_range_list( p_file_info );
	}

	return p_range_list;
}


BOOL  bfm_range_is_write( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, RANGE *p_range )
{
	BT_SUB_FILE *p_bt_sub_file = NULL;
	_int32 ret_val = SUCCESS;
	BOOL ret_bool = TRUE;
	
	ret_val = bfm_get_bt_sub_file_ptr( p_bt_file_manager, file_index, &p_bt_sub_file );
	if( ret_val != SUCCESS )
	{
		ret_bool = FALSE;	
	}
	else if( p_bt_sub_file->_sub_file_info_status != FILE_INFO_OPENED )
	{
		ret_bool = FALSE;	
	}
	else
	{
		sd_assert( p_bt_sub_file->_file_info_ptr != NULL );
		ret_bool = file_info_range_is_write( p_bt_sub_file->_file_info_ptr, p_range );
	}

	LOG_DEBUG( "bfm_range_is_write. file_index:%d, padding_range:[%u, %u], ret_bool:%d", 
		file_index, p_range->_index, p_range->_num, ret_bool );

	return ret_bool;
	
}

_int32 bfm_set_bt_sub_file_err_code( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, _int32 err_code )
{
	_int32 ret_val = SUCCESS;
	BT_SUB_FILE *p_bt_sub_file = NULL;
	
	LOG_DEBUG( "bfm_set_bt_sub_file_err_code. file_index:%d, err_code:%d", 
		file_index, err_code );
		
	ret_val = bfm_get_bt_sub_file_ptr( p_bt_file_manager, file_index, &p_bt_sub_file );
	if( ret_val != SUCCESS ) return SUCCESS;

	if( err_code != SUCCESS )
	{
		ret_val = bfm_handle_file_failture( p_bt_file_manager, p_bt_sub_file, err_code );
		
		sd_assert( ret_val == SUCCESS );	
	}
	else 
	{
		LOG_DEBUG( "bfm_set_bt_sub_file_err_code. file_index:%d, err_code:%d, sub_file_err_code:%d", 
			file_index, err_code, p_bt_sub_file->_err_code );	
		sd_assert( p_bt_sub_file->_err_code == BT_FILE_DOWNLOAD_SUCCESS_WAIT_CHECK );

		ret_val = bfm_handle_file_success( p_bt_file_manager, p_bt_sub_file );
		sd_assert( ret_val == SUCCESS );	
	}

	return SUCCESS;
	
}


void  bfm_notify_file_create_result( void *p_user_data, struct _tag_file_info *p_file_info, _int32 creat_result )
{
	BT_SUB_FILE *p_bt_sub_file = ( BT_SUB_FILE * )p_user_data;
	_int32 ret_val = SUCCESS;
	BT_FILE_MANAGER *p_bt_file_manager = p_bt_sub_file->_bt_file_manager_ptr;

	LOG_DEBUG( "bfm_notify_file_create_result. file_index:%u, result:%d ", 
		p_bt_sub_file->_file_index, creat_result );
	
	sd_assert( p_bt_sub_file->_sub_file_info_status == FILE_INFO_OPENING
		|| p_bt_sub_file->_sub_file_info_status == FILE_INFO_CLOSING );
	if( p_bt_sub_file->_sub_file_info_status == FILE_INFO_CLOSING ) return;

	if( creat_result != SUCCESS )
	{
		ret_val = bfm_handle_file_failture( p_bt_file_manager, p_bt_sub_file, creat_result );
		sd_assert( ret_val == SUCCESS );
	}
	else
	{
		bfm_enter_file_info_status( p_bt_sub_file, FILE_INFO_OPENED );
	}

}

void  bfm_notify_file_close_result( void *p_user_data, struct _tag_file_info *p_file_info, _int32 close_result )
{	
	BT_SUB_FILE *p_bt_sub_file = ( BT_SUB_FILE * )p_user_data;
	BT_FILE_MANAGER *p_bt_file_manager = ( BT_FILE_MANAGER * )p_bt_sub_file->_bt_file_manager_ptr;
	_u32 cur_time = 0;
	
	LOG_DEBUG( "bfm_notify_file_close_result.close_result:%u,file_index:%u, file_status:%u ", 
		close_result, p_bt_sub_file->_file_index, p_bt_sub_file->_file_status );

	if(	p_bt_sub_file->_waited_success_close_file )
	{
		sd_assert( p_bt_sub_file->_file_status == BT_FILE_DOWNLOADING );
		
		file_info_change_td_name( p_bt_sub_file->_file_info_ptr );

		LOG_DEBUG("bfm_notify_file_close_result, delete cfg file.");

		file_info_delete_cfg_file( p_bt_sub_file->_file_info_ptr ); 

		file_info_decide_finish_filename( p_bt_sub_file->_file_info_ptr );		   
		p_bt_sub_file->_waited_success_close_file = FALSE;
		
		bfm_enter_file_status( p_bt_sub_file, BT_FILE_FINISHED );
	}

	bfm_uninit_file_info( p_bt_file_manager, p_bt_sub_file );
	
	bfm_enter_file_info_status( p_bt_sub_file, FILE_INFO_CLOSED );

	sd_time( &cur_time );

	if( cur_time > p_bt_sub_file->_start_time )
	{

		p_bt_sub_file->_downloading_time += cur_time - p_bt_sub_file->_start_time;
	}
	p_bt_sub_file->_start_time = 0;
	
	file_info_free_node( p_bt_sub_file->_file_info_ptr );	
	p_bt_sub_file->_file_info_ptr = NULL;

	if( p_bt_file_manager->_is_closing )
	{
		bfm_nofity_file_close_success( p_bt_file_manager );
	}

}

void bfm_notify_file_block_check_result( void *p_user_data, struct _tag_file_info *p_file_info, RANGE *p_range, BOOL  check_result )
{
	BT_SUB_FILE *p_bt_sub_file = ( BT_SUB_FILE * )p_user_data;
	LOG_DEBUG( "bfm_notify_file_block_check_result,file_index:%u, range:[%u, %u],check_result:%d, has_bcid:%d ", 
		p_bt_sub_file->_file_index, p_range->_index, p_range->_num,
		check_result, p_bt_sub_file->_has_bcid );
    if( p_bt_sub_file->_bt_file_manager_ptr->_is_closing ) return;

	if( p_bt_sub_file->_has_bcid )
	{
		bfm_notify_range_check_result( p_bt_sub_file->_bt_file_manager_ptr, 
			p_bt_sub_file->_file_index, p_range, check_result );	
	}
}


void bfm_notify_file_result( void* p_user_data, struct _tag_file_info *p_file_info, _u32 file_result )
{
	BT_SUB_FILE *p_bt_sub_file = ( BT_SUB_FILE * )p_user_data;
	BT_FILE_MANAGER *p_bt_file_manager = p_bt_sub_file->_bt_file_manager_ptr;
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "bfm_notify_file_result.file_index:%u, file_result:%u ",
		p_bt_sub_file->_file_index, file_result );
#ifdef UPLOAD_DEBUG
	return ;
#endif
    if( p_bt_file_manager->_is_closing ) return;
	if( file_result == SUCCESS )
	{		
		if( p_bt_sub_file->_has_bcid )
		{
			ret_val = bfm_handle_file_success( p_bt_file_manager, p_bt_sub_file );
			sd_assert( ret_val == SUCCESS );
		}
		else
		{
			bfm_enter_file_err_code( p_bt_sub_file, BT_FILE_DOWNLOAD_SUCCESS_WAIT_CHECK );
			ret_val = bdm_check_if_download_success( p_bt_file_manager->_bt_data_manager_ptr, p_bt_sub_file->_file_index );
			sd_assert( ret_val == SUCCESS );
		}
		//if( !bdm_check_if_file_can_close( p_bt_file_manager->_bt_data_manager_ptr, p_bt_sub_file->_file_index) ) return SUCCESS;
		//bfm_handle_file_success( p_bt_file_manager, p_bt_sub_file );
	}
	else if( file_result == DATA_ALLOC_BCID_BUFFER_ERROR )
	{
		bfm_enter_file_err_code( p_bt_sub_file, BT_DATA_CHECK_BCID_NO_BUFFER );
	}
	else
	{
		bfm_handle_file_failture( p_bt_file_manager, p_bt_sub_file, file_result );
	}
}

_int32 bfm_handle_file_success( BT_FILE_MANAGER *p_bt_file_manager, BT_SUB_FILE *p_bt_sub_file )
{
	BOOL ret_bool = FALSE;
	_int32 ret_val = SUCCESS;
	_int32 errcode = 0;
	_u8 cid[CID_SIZE];
	
	LOG_DEBUG("bfm_handle_file_success, file_index:%u", p_bt_sub_file->_file_index );
	
	bfm_enter_file_err_code( p_bt_sub_file, SUCCESS );
	if( bdm_is_vod_mode( p_bt_file_manager->_bt_data_manager_ptr )
		&& bdm_get_vod_file_index( p_bt_file_manager->_bt_data_manager_ptr ) == p_bt_sub_file->_file_index )
	{
		bfm_enter_file_err_code( p_bt_sub_file, BT_VOD_WAIT_CLOSE );
		return SUCCESS;
	}

	if( p_bt_sub_file->_has_bcid )
	{
		BOOL read_file_error = FALSE;
		ret_bool = file_info_check_cid( p_bt_sub_file->_file_info_ptr, &errcode, &read_file_error, NULL);
		LOG_DEBUG("bfm_handle_file_success, file_index:%u,	cid is valid, errcode:%d", 
			p_bt_sub_file->_file_index, errcode );
		if( DM_CAL_CID_NO_BUFFER == errcode )
		{
			bfm_enter_file_err_code( p_bt_sub_file, BT_DATA_CHECK_CID_NO_BUFFER );
			return SUCCESS;
		}
		if( !ret_bool && read_file_error)
		{
			bfm_handle_file_failture( p_bt_file_manager, p_bt_sub_file, BT_DATA_CHECK_CID_ERROR );
			return SUCCESS;
		}
		
		LOG_DEBUG("bfm_handle_file_success, cid is valid, so check gcid");
		ret_bool = file_info_check_gcid( p_bt_sub_file->_file_info_ptr );
		if( ret_bool == FALSE )
		{
			LOG_ERROR("bfm_handle_file_success, gcid is valid, but check gcid fail!");	
			bfm_handle_file_failture( p_bt_file_manager, p_bt_sub_file, BT_DATA_CHECK_GCID_ERROR );
			return SUCCESS; 	   
		}  
	}
	else if( file_info_cid_is_valid( p_bt_sub_file->_file_info_ptr ) == FALSE )
	{			 
		LOG_ERROR("bfm_handle_file_success, cid is invalid");

		ret_bool = get_file_3_part_cid( p_bt_sub_file->_file_info_ptr, cid, &errcode);		 
		if( ret_bool == FALSE )
		{
			LOG_ERROR("bfm_handle_file_success, get cid failure");
			if( errcode == DM_CAL_CID_NO_BUFFER )
			{
				bfm_enter_file_err_code( p_bt_sub_file, BT_DATA_CHECK_CID_NO_BUFFER );
				return SUCCESS;
			}

			bfm_handle_file_failture( p_bt_file_manager, p_bt_sub_file, BT_DATA_GET_CID_ERROR );
			return SUCCESS;
		}

		LOG_DEBUG("bfm_handle_file_success, set cid"); 
		file_info_set_cid( p_bt_sub_file->_file_info_ptr, cid );  

	}

	p_bt_sub_file->_waited_success_close_file = TRUE;
	if( p_bt_sub_file->_file_writed_bytes < p_bt_sub_file->_file_size )
	{
		p_bt_file_manager->_file_total_writed_bytes += p_bt_sub_file->_file_size - p_bt_sub_file->_file_writed_bytes;
		sd_assert( p_bt_file_manager->_file_total_writed_bytes <= p_bt_file_manager->_need_download_size );
	}
	if( p_bt_sub_file->_file_download_bytes < p_bt_sub_file->_file_size )
	{
		p_bt_file_manager->_file_total_download_bytes += p_bt_sub_file->_file_size - p_bt_sub_file->_file_download_bytes;
	}	
	p_bt_sub_file->_file_writed_bytes = p_bt_sub_file->_file_size;
	p_bt_sub_file->_file_download_bytes = p_bt_sub_file->_file_size;
	LOG_DEBUG("bfm_handle_file_success, file_index:%u, file_write_byte:%llu, file_download_byte:%llu, total_write_byte:%llu, total_download_byte:%llu", 
		p_bt_sub_file->_file_index, p_bt_sub_file->_file_writed_bytes, p_bt_sub_file->_file_download_bytes,
		p_bt_file_manager->_file_total_writed_bytes, p_bt_file_manager->_file_total_download_bytes );
	bdm_report_single_file( p_bt_file_manager->_bt_data_manager_ptr, p_bt_sub_file->_file_index, TRUE );

#ifdef EMBED_REPORT
	reporter_task_bt_file_stat(p_bt_file_manager->_bt_data_manager_ptr->_task_ptr, p_bt_sub_file->_file_index); 
#endif

	ret_val = bfm_stop_sub_file( p_bt_file_manager, p_bt_sub_file );
	sd_assert( ret_val == SUCCESS );

	return SUCCESS; 	   
}

_int32 bfm_handle_file_failture( BT_FILE_MANAGER *p_bt_file_manager, BT_SUB_FILE *p_bt_sub_file, _int32 err_code )
{
	_int32 ret_val = SUCCESS;
	
	LOG_URGENT("bfm_handle_file_failture, file_index:%u, err_code:%d", 
		p_bt_sub_file->_file_index, err_code );
	
	bfm_enter_file_status( p_bt_sub_file, BT_FILE_FAILURE );
	
	bfm_enter_file_err_code( p_bt_sub_file, err_code );
	bdm_report_single_file( p_bt_file_manager->_bt_data_manager_ptr, p_bt_sub_file->_file_index, FALSE );
	#ifdef EMBED_REPORT
		reporter_task_bt_file_stat(p_bt_file_manager->_bt_data_manager_ptr->_task_ptr, p_bt_sub_file->_file_index); 
	#endif
	ret_val = bfm_stop_sub_file( p_bt_file_manager, p_bt_sub_file );
	sd_assert( ret_val == SUCCESS );

	return SUCCESS; 	   
}

_int32 bfm_create_bt_sub_file_struct( BT_FILE_MANAGER *p_bt_file_manager, struct tagTORRENT_PARSER *p_torrent_parser,
	_u32 *p_need_download_file_array, _u32 need_download_file_num )
{
	_int32 ret_val = SUCCESS;
	PAIR map_node;
	_u32 file_num = 0;
	_u32 file_index = 0;
	BT_SUB_FILE *p_bt_sub_file = NULL;
	map_init( &p_bt_file_manager->_bt_sub_file_map, bfm_bt_sub_file_map_compare );
	LOG_DEBUG( "bfm_create_bt_sub_file_struct." );

	for( ; file_num < need_download_file_num; file_num++ )
	{
		file_index = p_need_download_file_array[ file_num ];
		if( file_index >= tp_get_seed_file_num( p_torrent_parser ) )
		{
			return BT_ERROR_FILE_INDEX;
		}
		LOG_DEBUG( "bfm_create_bt_sub_file_struct: file_index:%u, file_num:%u", 
			file_index, need_download_file_num );
			
		ret_val = sd_malloc( sizeof( BT_SUB_FILE ), (void **)&p_bt_sub_file );
		CHECK_VALUE( ret_val );

		p_bt_sub_file->_file_index = file_index;
		bfm_enter_file_status( p_bt_sub_file, BT_FILE_IDLE );
		bfm_enter_file_err_code( p_bt_sub_file, SUCCESS );
		bfm_enter_file_info_status( p_bt_sub_file, FILE_INFO_CLOSED );
		
		p_bt_sub_file->_file_info_ptr = NULL;
		p_bt_sub_file->_sub_file_type = IDLE_SUB_FILE_TYPE;
		
		p_bt_sub_file->_bt_file_manager_ptr = p_bt_file_manager;
		
		p_bt_sub_file->_file_size = 0;
		p_bt_sub_file->_file_download_bytes = 0;
		p_bt_sub_file->_file_writed_bytes = 0;
		p_bt_sub_file->_has_bcid = FALSE;
		p_bt_sub_file->_is_speedup_failed = FALSE;
		p_bt_sub_file->_idle_ticks = 0;
		p_bt_sub_file->_start_time = 0;
		p_bt_sub_file->_downloading_time = 0;

		p_bt_sub_file->_waited_success_close_file = FALSE;
		
		p_bt_sub_file->_server_dl_bytes = 0;
		p_bt_sub_file->_peer_dl_bytes = 0;
		p_bt_sub_file->_cdn_dl_bytes = 0;
		p_bt_sub_file->_normal_cdn_bytes = 0;
		p_bt_sub_file->_origion_dl_bytes = 0;
		p_bt_sub_file->_lixian_dl_bytes = 0;	
		p_bt_sub_file->_is_ever_vod_file = FALSE;
        p_bt_sub_file->_sub_task_start_time = 0;
        p_bt_sub_file->_first_zero_speed_duration = 0;
        p_bt_sub_file->_last_stat_time = 0;
        p_bt_sub_file->_other_zero_speed_duraiton = 0;
        p_bt_sub_file->_percent100_time = 0;
        p_bt_sub_file->_is_continue_task = FALSE;
		
		ret_val = bfm_init_file_info( p_bt_sub_file, p_torrent_parser,
			p_bt_file_manager->_data_path, p_bt_file_manager->_data_path_len );
		if( ret_val != SUCCESS )
		{
			SAFE_DELETE( p_bt_sub_file );
		}
		CHECK_VALUE( ret_val );
	
		p_bt_file_manager->_torrent_parser_ptr = p_torrent_parser;

		map_node._key = (void *)file_index;
		map_node._value = (void *)p_bt_sub_file;

		ret_val = map_insert_node( &p_bt_file_manager->_bt_sub_file_map, &map_node );
		CHECK_VALUE( ret_val );
	}
	
	return SUCCESS;
}

_int32 bfm_init_file_info( BT_SUB_FILE *p_bt_sub_file, struct tagTORRENT_PARSER *p_torrent_parser, char *p_data_path, _u32 data_path_len )
{
	_int32 ret_val = SUCCESS;
	char cfg_full_path[MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN] = {0};
	char name_buffer[MAX_FILE_NAME_LEN] = {0};
	char *p_cfg_tail = ".rf.cfg",*p_td_tail = ".rf";
	_u32 full_path_buffer_len = 0,name_len=MAX_FILE_NAME_LEN,file_id=0;
	_u64 file_size = 0;
	BOOL is_td_exist=FALSE;
	BT_FILE_MANAGER *p_bt_file_manager = p_bt_sub_file->_bt_file_manager_ptr;

	if( data_path_len >= MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN )
	{
		return FILE_PATH_TOO_LONG;
	}

	ret_val = sd_strncpy( cfg_full_path, p_data_path, MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN );
	CHECK_VALUE( ret_val );

	full_path_buffer_len = MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN - data_path_len;
	ret_val = tp_get_file_info_detail( p_torrent_parser, p_bt_sub_file->_file_index, 
		cfg_full_path + data_path_len, &full_path_buffer_len, &p_bt_sub_file->_file_size );
	CHECK_VALUE( ret_val );
	/* Check if the file already exist */
	if (sd_file_exist( cfg_full_path ) )
	{
		LOG_DEBUG( "bfm_init_file_info, cfg_full_path:%s is exist", cfg_full_path );	
		ret_val = sd_get_file_size_and_modified_time(cfg_full_path,&file_size,NULL);
		CHECK_VALUE( ret_val );
		if(p_bt_sub_file->_file_size == 0
			|| file_size == p_bt_sub_file->_file_size)
		{
			/* file already exist  */
			bfm_enter_file_status( p_bt_sub_file, BT_FILE_FINISHED );

			p_bt_sub_file->_file_download_bytes = p_bt_sub_file->_file_size;
			p_bt_sub_file->_file_writed_bytes = p_bt_sub_file->_file_size;
			p_bt_file_manager->_file_total_download_bytes+=p_bt_sub_file->_file_size;
			p_bt_file_manager->_file_total_writed_bytes+=p_bt_sub_file->_file_size;
			LOG_DEBUG( "bfm_init_file_info: cfg_full_path:%s, download_byte:%d,writed_byte:%d ", 
				cfg_full_path, p_bt_sub_file->_file_download_bytes, p_bt_sub_file->_file_writed_bytes );		
			return SUCCESS;
		}
		else
		{
			/* Wrong file exist ,delete it */
			bfm_enter_file_status( p_bt_sub_file, BT_FILE_IDLE );
			p_bt_sub_file->_file_download_bytes = 0;
			p_bt_sub_file->_file_writed_bytes = 0;
			LOG_DEBUG( "bfm_init_file_info, delete file_full_path:%s", cfg_full_path );
			ret_val = sd_delete_file( cfg_full_path );
			LOG_DEBUG( "bfm_init_file_info, delete cfg_full_path:%s, ret_val:%d", 
				cfg_full_path, ret_val );	
			CHECK_VALUE( ret_val );
		}
	}
	
	/* Check if the .td file already exist */
	sd_assert(data_path_len + full_path_buffer_len+sd_strlen(p_td_tail)<MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN);
	ret_val = sd_strncpy( cfg_full_path + data_path_len + full_path_buffer_len, p_td_tail,MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN-(data_path_len + full_path_buffer_len) );
	CHECK_VALUE( ret_val );
	cfg_full_path[ data_path_len + full_path_buffer_len + sd_strlen(p_td_tail) ] = '\0';

	LOG_DEBUG( "bfm_init_file_info, td_full_path is exist?:%s ", cfg_full_path ); 
	if( sd_file_exist( cfg_full_path ) )
	{
		LOG_DEBUG( "bfm_init_file_info, td_full_path:%s is exist", cfg_full_path );	
		is_td_exist=TRUE;
	}

	/* Check if the .td.cfg file already exist */
	sd_assert(data_path_len + full_path_buffer_len+sd_strlen(p_cfg_tail)<MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN);
	ret_val = sd_strncpy( cfg_full_path + data_path_len + full_path_buffer_len, p_cfg_tail,MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN-(data_path_len + full_path_buffer_len) );
	CHECK_VALUE( ret_val );

	cfg_full_path[ data_path_len + full_path_buffer_len + sd_strlen(p_cfg_tail) ] = '\0';

	if( sd_file_exist( cfg_full_path ) &&(TRUE== is_td_exist))
	{
		LOG_DEBUG( "bfm_init_file_info, cfg and td exist" );			
		bfm_enter_file_status( p_bt_sub_file, BT_FILE_IDLE );
		p_bt_sub_file->_file_download_bytes = file_info_get_downsize_from_cfg_file( cfg_full_path );
		p_bt_sub_file->_file_writed_bytes = p_bt_sub_file->_file_download_bytes;	
		p_bt_file_manager->_file_total_download_bytes+=p_bt_sub_file->_file_download_bytes;	
		p_bt_file_manager->_file_total_writed_bytes+=p_bt_sub_file->_file_download_bytes;
        p_bt_sub_file->_is_continue_task = TRUE;
	}
	else
	{
		bfm_enter_file_status( p_bt_sub_file, BT_FILE_IDLE );
		p_bt_sub_file->_file_download_bytes = 0;
		p_bt_sub_file->_file_writed_bytes = 0;	
		ret_val = sd_delete_file( cfg_full_path );
		LOG_DEBUG( "bfm_init_file_info, delete cfg_full_path:%s, ret_val:%d, is_td_exist:%d", 
			cfg_full_path, ret_val, is_td_exist );	
		
		if(is_td_exist==TRUE)
		{
			ret_val = sd_strncpy( cfg_full_path + data_path_len + full_path_buffer_len, p_td_tail, MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN-(data_path_len + full_path_buffer_len) );
			CHECK_VALUE( ret_val );
			cfg_full_path[ data_path_len + full_path_buffer_len + sd_strlen(p_td_tail) ] = '\0';
			ret_val = sd_delete_file( cfg_full_path );
			LOG_DEBUG( "bfm_init_file_info, delete td_file_full_path:%s, ret_val:%d", 
				cfg_full_path, ret_val );
			
			CHECK_VALUE( ret_val );
		}
	}

	/* Check if the file is 0 */
	if(( p_bt_sub_file->_file_size == 0 )&&(p_bt_sub_file->_file_status!=BT_FILE_FINISHED))
	{
		
		LOG_DEBUG( "bfm_init_file_info, file_size = 0" );			
		full_path_buffer_len= MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN-data_path_len;
		ret_val = tp_get_file_path_and_name(p_torrent_parser,p_bt_sub_file->_file_index, cfg_full_path + data_path_len,  &full_path_buffer_len,name_buffer,&name_len);
		CHECK_VALUE( ret_val );
		cfg_full_path[data_path_len+full_path_buffer_len]='\0';
		ret_val = sd_mkdir( cfg_full_path );
		CHECK_VALUE( ret_val );

		ret_val = sd_strncpy( cfg_full_path + data_path_len + full_path_buffer_len, name_buffer, MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN-(data_path_len + full_path_buffer_len) );
		CHECK_VALUE( ret_val );
		cfg_full_path[data_path_len+full_path_buffer_len+name_len]='\0';
	
		LOG_DEBUG( "bfm_init_file_info.file_path:%s", cfg_full_path );
		ret_val = sd_open_ex( cfg_full_path, O_FS_CREATE, &file_id);
		CHECK_VALUE( ret_val );
			
		bfm_enter_file_status( p_bt_sub_file, BT_FILE_FINISHED );
		p_bt_sub_file->_file_download_bytes = 0;
		p_bt_sub_file->_file_writed_bytes = 0;	

		ret_val = sd_close_ex( file_id);
		CHECK_VALUE( ret_val );
	}

	LOG_DEBUG( "bfm_init_file_info: cfg_full_path:%s, download_byte:%llu, writed_byte:%llu ", 
		cfg_full_path, p_bt_sub_file->_file_download_bytes, p_bt_sub_file->_file_writed_bytes );		
	return SUCCESS;
}

_int32 bfm_bt_sub_file_map_compare( void *E1, void *E2 )
{
	_u32 value1 = (_u32)E1;
	_u32 value2 = (_u32)E2;
	return value1 > value2 ? 1 : ( value1 < value2 ? -1 : 0 );  
}

_int32 bfm_stop_all_bt_sub_file_struct( BT_FILE_MANAGER *p_bt_file_manager )
{
	MAP_ITERATOR cur_node = MAP_BEGIN( p_bt_file_manager->_bt_sub_file_map );
	BT_SUB_FILE *p_bt_sub_file = NULL;
	_int32 ret_val = SUCCESS;

	LOG_DEBUG( "bfm_stop_all_bt_sub_file_struct " );		
	while( cur_node != MAP_END( p_bt_file_manager->_bt_sub_file_map ) )
	{
		p_bt_sub_file = (BT_SUB_FILE *)MAP_VALUE( cur_node );
		LOG_DEBUG( "bfm_stop_all_bt_sub_file_struct, file_index:%u, sub_file_info_satus:%d ",
			p_bt_sub_file->_file_index, p_bt_sub_file->_sub_file_info_status );		
		
		if( p_bt_sub_file->_sub_file_info_status == FILE_INFO_OPENING
			|| p_bt_sub_file->_sub_file_info_status == FILE_INFO_OPENED )
		{
			ret_val = bfm_stop_sub_file( p_bt_file_manager,p_bt_sub_file );
			sd_assert( ret_val == SUCCESS );
		}
		
		cur_node = MAP_NEXT( p_bt_file_manager->_bt_sub_file_map, cur_node );
	}
	return SUCCESS;	
}

_int32 bfm_stop_report_running_sub_file( BT_FILE_MANAGER *p_bt_file_manager )
{
#ifdef EMBED_REPORT
	MAP_ITERATOR cur_node = MAP_BEGIN( p_bt_file_manager->_bt_sub_file_map );
	BT_SUB_FILE *p_bt_sub_file = NULL;
	_int32 ret_val = SUCCESS;

	LOG_DEBUG( "bfm_stop_all_bt_sub_file_struct " );		
	while( cur_node != MAP_END( p_bt_file_manager->_bt_sub_file_map ) )
	{
		p_bt_sub_file = (BT_SUB_FILE *)MAP_VALUE( cur_node );
		LOG_DEBUG( "bfm_stop_all_bt_sub_file_struct, file_index:%u, sub_file_info_satus:%d ",
			p_bt_sub_file->_file_index, p_bt_sub_file->_sub_file_info_status );		

		if( p_bt_sub_file->_sub_file_info_status == FILE_INFO_OPENING
			|| p_bt_sub_file->_sub_file_info_status == FILE_INFO_OPENED )
		{
			reporter_task_bt_file_stat(p_bt_file_manager->_bt_data_manager_ptr->_task_ptr, p_bt_sub_file->_file_index); 
		}

		cur_node = MAP_NEXT( p_bt_file_manager->_bt_sub_file_map, cur_node );
	}
	return SUCCESS;	
#endif
}

_int32 bfm_destroy_bt_sub_file_struct( BT_FILE_MANAGER *p_bt_file_manager )
{
	MAP_ITERATOR cur_node = MAP_BEGIN( p_bt_file_manager->_bt_sub_file_map );
	MAP_ITERATOR next_node;
	BT_SUB_FILE *p_bt_sub_file = NULL;
	_int32 ret_val = SUCCESS;
	
	LOG_DEBUG( "bfm_destroy_bt_sub_file_struct " );		
	while( cur_node != MAP_END( p_bt_file_manager->_bt_sub_file_map ) )
	{
		p_bt_sub_file = (BT_SUB_FILE *)MAP_VALUE( cur_node );
		
		sd_assert( p_bt_sub_file->_sub_file_info_status == FILE_INFO_CLOSED );
		sd_assert( p_bt_sub_file->_file_info_ptr == NULL );
		SAFE_DELETE( p_bt_sub_file );
		next_node = MAP_NEXT( p_bt_file_manager->_bt_sub_file_map, cur_node);

		ret_val = map_erase_iterator( &p_bt_file_manager->_bt_sub_file_map, cur_node );
		sd_assert( ret_val == SUCCESS );
		
		cur_node = next_node;
	}
	return SUCCESS;	
}

_int32 bfm_start_create_common_file_info( BT_FILE_MANAGER *p_bt_file_manager )
{
	_u32 created_file_info_num = 0;
	BOOL is_select = FALSE;
	_u32 max_file_info_num = bfm_get_max_file_info_num( p_bt_file_manager );
	LOG_DEBUG( "bfm_start_create_common_file_info " );		
	
	while( p_bt_file_manager->_cur_file_info_num < max_file_info_num
		&& p_bt_file_manager->_cur_downloading_file_size < g_bt_max_cur_downloading_size )
	{
		sd_assert( created_file_info_num < max_file_info_num );
		if( created_file_info_num >= max_file_info_num ) return BT_DATA_MANAGER_LOGIC_ERROR;
		is_select = bfm_select_file_download( p_bt_file_manager );
		if( !is_select ) return SUCCESS;
		created_file_info_num++;
		LOG_DEBUG( "bfm_start_create_common_file_info, cur_file_info_num:%u, created_file_info_num:%u, max_file_info_num:%u, _cur_downloading_file_size:%llu ",
			p_bt_file_manager->_cur_file_info_num, created_file_info_num, max_file_info_num, p_bt_file_manager->_cur_downloading_file_size );			
	}
	return SUCCESS;	
}

BOOL bfm_select_file_download( BT_FILE_MANAGER *p_bt_file_manager )
{
	MAP_ITERATOR cur_node = MAP_BEGIN( p_bt_file_manager->_bt_sub_file_map );
	BT_SUB_FILE *p_bt_sub_file = NULL;
	BT_SUB_FILE *p_bt_select_sub_file = NULL;
	_u32 file_index = -1;
	BOOL is_select = FALSE;
	_int32 ret_val = SUCCESS;
	
	LOG_DEBUG( "bfm_select_file_download " );		
	
	while( cur_node != MAP_END( p_bt_file_manager->_bt_sub_file_map ) )
	{
		p_bt_sub_file = (BT_SUB_FILE *)MAP_VALUE( cur_node );
		LOG_DEBUG( "bfm_select_file_download, file_index:%u, file_size:%llu, file_status:%d, sub_file_info_status:%d ",
			p_bt_sub_file->_file_index, p_bt_sub_file->_file_size, p_bt_sub_file->_file_status, p_bt_sub_file->_sub_file_info_status );		
		
		if( p_bt_sub_file->_file_status == BT_FILE_IDLE
			&& p_bt_sub_file->_sub_file_info_status == FILE_INFO_CLOSED )
		{
			if( p_bt_sub_file->_file_index < file_index )
			{
				p_bt_select_sub_file = p_bt_sub_file;
				file_index = p_bt_sub_file->_file_index;
			}
		}
		cur_node = MAP_NEXT( p_bt_file_manager->_bt_sub_file_map, cur_node);
	}
	
	if( p_bt_select_sub_file != NULL )
	{
		sd_assert( file_index != -1 );
		LOG_DEBUG( "bfm_select_file_download end, file_index:%u, file_size:%llu, file_status:%d, sub_file_info_status:%d ",
			p_bt_select_sub_file->_file_index, p_bt_select_sub_file->_file_size, p_bt_select_sub_file->_file_status, p_bt_select_sub_file->_sub_file_info_status ); 	
		
		ret_val = bfm_start_single_file_info( p_bt_file_manager, p_bt_select_sub_file );
		if( ret_val == SUCCESS )
		{
			is_select = TRUE;	
		}
		else
		{
			is_select = FALSE;
		}
	}
	else
	{
		is_select = FALSE;
	}
	LOG_DEBUG( "bfm_select_file_download ,is_select:%d ", is_select );		
	
	return is_select;
	
}

_int32 bfm_start_single_file_info( BT_FILE_MANAGER *p_bt_file_manager
    , BT_SUB_FILE *p_bt_sub_file )
{
	_int32 ret_val = SUCCESS;
	RANGE_LIST *p_range_list = NULL;
	BOOL is_success = FALSE;
	
	LOG_DEBUG( "bfm_start_single_file_info, create_file_info:file_index:%u, file_status:%d, sub_file_info_status:%d, cur_file_info_num:%u, has_bcid:%d ",
		p_bt_sub_file->_file_index, p_bt_sub_file->_file_status, p_bt_sub_file->_sub_file_info_status, 
		p_bt_file_manager->_cur_file_info_num, p_bt_sub_file->_has_bcid );		
	sd_time( &p_bt_sub_file->_start_time );
    p_bt_sub_file->_sub_task_start_time = p_bt_sub_file->_start_time;
	ret_val = bfm_create_file_info( p_bt_file_manager, p_bt_sub_file );
	if( ret_val != SUCCESS )
	{
		bfm_handle_file_failture( p_bt_file_manager, p_bt_sub_file, ret_val );
		return ret_val;
	}

	p_bt_sub_file->_sub_file_type = COMMON_SUB_FILE_TYPE;

	if( p_bt_sub_file->_file_status == BT_FILE_FINISHED )
	{
		is_success = TRUE;
	}

	bfm_enter_file_status( p_bt_sub_file, BT_FILE_DOWNLOADING );
	init_speed_calculator( &p_bt_sub_file->_file_download_speed, 10, 1000 );

	p_bt_file_manager->_cur_file_info_num++;
	p_bt_file_manager->_cur_downloading_file_size += p_bt_sub_file->_file_size;

	sd_assert( p_bt_sub_file->_file_info_ptr != NULL );
	p_range_list = file_info_get_recved_range_list( p_bt_sub_file->_file_info_ptr );
	
	bfm_add_file_downloading_range( p_bt_file_manager, p_bt_sub_file );
		
	bdm_file_manager_notify_start_file( p_bt_file_manager->_bt_data_manager_ptr, p_bt_sub_file->_file_index, p_range_list, p_bt_sub_file->_has_bcid );

#ifdef EMBED_REPORT
	if(!p_bt_sub_file->_is_continue_task)
	{
	    report_task_create(p_bt_file_manager->_bt_data_manager_ptr->_task_ptr, p_bt_sub_file->_file_index);
	}
#endif

	if( !p_bt_sub_file->_has_bcid && !is_success )
	{
		bdm_query_hub_for_single_file( p_bt_file_manager->_bt_data_manager_ptr, p_bt_sub_file->_file_index );
	}
	return SUCCESS;
}

void bfm_add_file_downloading_range( BT_FILE_MANAGER *p_bt_file_manager, BT_SUB_FILE *p_bt_sub_file )
{
	RANGE file_range;
	_u32 step_num = 0;
	_u64 file_range_num = 0;
	RANGE delete_file_range;

	sd_assert( p_bt_sub_file->_file_size > 0 );
	if( p_bt_sub_file->_file_size == 0 ) return;
	
	file_range_num = ( p_bt_sub_file->_file_size - 1 ) / get_data_unit_size() + 1;
	
	LOG_DEBUG( "bfm_add_file_downloading_range begin, file_size:%llu, _cur_range_step:%u, file_range_num:%u",
		p_bt_sub_file->_file_size, p_bt_file_manager->_cur_range_step, file_range_num );		

	if( p_bt_sub_file->_file_size > g_bt_max_cur_downloading_size
		&& !bdm_is_vod_mode( p_bt_file_manager->_bt_data_manager_ptr ) )
	{
		sd_assert( g_bt_max_cur_downloading_size % get_data_unit_size() == 0 );
		
		step_num = g_bt_max_cur_downloading_size / get_data_unit_size();

		file_range._index = step_num * p_bt_file_manager->_cur_range_step;
		
		//sd_assert( file_range._index <= file_range_num );
		if( file_range._index >= file_range_num ) return;
		
		file_range._num = MIN( step_num, file_range_num - file_range._index );	
		p_bt_file_manager->_big_sub_file_ptr = p_bt_sub_file;
		if( p_bt_file_manager->_cur_range_step > 0 )
		{
			delete_file_range._index = step_num * ( p_bt_file_manager->_cur_range_step - 1 );
			delete_file_range._num = step_num;
			bdm_file_manager_notify_delete_file_range( p_bt_file_manager->_bt_data_manager_ptr
			        , p_bt_sub_file->_file_index
			        , &delete_file_range );
		}
		p_bt_file_manager->_cur_range_step++;
	}
	else
	{
		file_range._index = 0;
		file_range._num = file_range_num;
	}

	bdm_file_manager_notify_add_file_range( p_bt_file_manager->_bt_data_manager_ptr, p_bt_sub_file->_file_index, &file_range );

	LOG_DEBUG( "bfm_add_file_downloading_range end, file_index:%u, file_range:[%u, %u]",
		p_bt_sub_file->_file_index, file_range._index, file_range._num );		

}

_int32 bfm_create_file_info( BT_FILE_MANAGER *p_bt_file_manager, BT_SUB_FILE *p_bt_sub_file )
{
	_int32 ret_val = SUCCESS;
	FILE_INFO_CALLBACK file_info_callback;
	char cfg_file_full_path[ MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN ] = {0};
	char data_file_path[ MAX_FILE_PATH_LEN ] = {0};
	char data_file_name[ MAX_FILE_NAME_BUFFER_LEN ] = {0};
	
	char *p_cfg_tail = ".rf.cfg";
	char *p_tmp_tail = ".rf";
	
	_u32 data_file_path_len = 0;
	_u32 file_index = p_bt_sub_file->_file_index;
	struct tagTORRENT_PARSER *p_torrent_parser = p_bt_file_manager->_torrent_parser_ptr;
	TORRENT_FILE_INFO *p_file_info = NULL;
	
	sd_assert( p_bt_sub_file->_file_info_ptr == NULL );
	if( p_bt_sub_file->_file_info_ptr != NULL ) return BT_DATA_MANAGER_LOGIC_ERROR;

	ret_val = file_info_alloc_node( &p_bt_sub_file->_file_info_ptr );
	CHECK_VALUE( ret_val );
	
	LOG_DEBUG( "bfm_create_file_info, create_file_info:file_index:%u, file_status:%d, sub_file_info_status:%d, file_info:0x%x, file_size:%llu ",
		p_bt_sub_file->_file_index, p_bt_sub_file->_file_status, 
		p_bt_sub_file->_sub_file_info_status, p_bt_sub_file->_file_info_ptr,
		p_bt_sub_file->_file_size );		

	ret_val = init_file_info( p_bt_sub_file->_file_info_ptr, p_bt_sub_file );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS )
	{
		goto ErrHandle0;
	}
	
	file_info_callback._p_notify_create_event = bfm_notify_file_create_result;
	file_info_callback._p_notify_close_event = bfm_notify_file_close_result;
	file_info_callback._p_notify_check_event = bfm_notify_file_block_check_result;
	file_info_callback._p_notify_file_result_event = bfm_notify_file_result;
	 file_info_callback._p_get_buffer_list_from_cache =  NULL ;	

	ret_val = file_info_register_callbacks( p_bt_sub_file->_file_info_ptr, &file_info_callback );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS )
	{
		goto ErrHandle1;
	}
	
	ret_val = tp_get_file_info( p_torrent_parser, file_index, &p_file_info );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS )
	{
		goto ErrHandle1;
	}
	data_file_path_len = p_bt_file_manager->_data_path_len + p_file_info->_file_path_len;

	if( data_file_path_len >= MAX_FILE_PATH_LEN )
	{
		sd_assert( FALSE );
		ret_val = FILE_PATH_TOO_LONG;
		goto ErrHandle1;
	}

	ret_val = sd_strncpy( data_file_path, p_bt_file_manager->_data_path, MAX_FILE_PATH_LEN );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS )
	{
		goto ErrHandle1;
	}

	ret_val = sd_strncpy( data_file_path + p_bt_file_manager->_data_path_len, p_file_info->_file_path, MAX_FILE_PATH_LEN - p_bt_file_manager->_data_path_len );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS )
	{
		goto ErrHandle1;
	}
	
	data_file_path[data_file_path_len] = '\0';

	if( p_file_info->_file_name_len > MAX_FILE_NAME_LEN )
	{
		ret_val = FILE_PATH_TOO_LONG;
		goto ErrHandle1;
	}

	ret_val = sd_strncpy( data_file_name, p_file_info->_file_name, MAX_FILE_NAME_BUFFER_LEN );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS )
	{
		goto ErrHandle1;
	}
	
	//注释by zxb, 测试要求改回如果目录存在同名文件，且大小一致，则认为已经完成
	//ret_val =get_unique_file_name_in_path(data_file_path, data_file_name, MAX_FILE_NAME_BUFFER_LEN);
	//sd_assert( ret_val == SUCCESS );
	//if( ret_val != SUCCESS )
	//{
	//	goto ErrHandle1;
	//}

       _u32 data_file_name_len = sd_strlen(data_file_name);
	ret_val = sd_strncpy( data_file_name + data_file_name_len, p_tmp_tail, MAX_FILE_NAME_BUFFER_LEN - data_file_name_len );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS )
	{
		goto ErrHandle1;
	}

	ret_val = sd_mkdir( data_file_path );
	//sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS )
	{
		LOG_ERROR( "bfm_create_file_info, mkdir err!! file_path:%s", data_file_path );			
		goto ErrHandle1;
	}

	ret_val = file_info_set_user_name( p_bt_sub_file->_file_info_ptr, data_file_name, data_file_path );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS )
	{
		goto ErrHandle1;
	}
	
	LOG_DEBUG( "bfm_create_file_info, file_path:%s, file_name:%s ", 
		data_file_path, data_file_name );		

	ret_val = file_info_set_final_file_name( p_bt_sub_file->_file_info_ptr, data_file_name );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS )
	{
		goto ErrHandle1;
	}	
	
	ret_val = file_info_set_filesize( p_bt_sub_file->_file_info_ptr, p_bt_sub_file->_file_size );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS )
	{
		goto ErrHandle1;
	}	

    compute_3part_range_list(p_bt_sub_file->_file_size, &p_bt_file_manager->_3part_cid_list);
	
	ret_val = sd_strncpy( cfg_file_full_path, data_file_path, MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS )
	{
		goto ErrHandle1;
	}	

	if( data_file_path_len + data_file_name_len + sd_strlen(p_cfg_tail) >= MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN )
	{
		sd_assert( FALSE );
		ret_val = FILE_PATH_TOO_LONG;	
		goto ErrHandle1;
	}
	ret_val = sd_strncpy( cfg_file_full_path + data_file_path_len, data_file_name, MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN - data_file_name_len );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS )
	{
		goto ErrHandle1;
	}	
	
	ret_val = sd_strncpy( cfg_file_full_path + data_file_path_len + data_file_name_len, p_cfg_tail, MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN - data_file_name_len - sd_strlen(p_cfg_tail) );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS )
	{
		goto ErrHandle1;
	}		
	ret_val = file_info_set_td_flag( p_bt_sub_file->_file_info_ptr );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS )
	{
		goto ErrHandle1;
	}	

	ret_val = bfm_open_continue_file_info(p_bt_sub_file, p_bt_sub_file->_file_info_ptr, cfg_file_full_path );
	LOG_DEBUG( "bfm_open_continue_file_info return:%d", ret_val );
	if( ret_val != SUCCESS )
	{
		goto ErrHandle1;
	}	

	bfm_enter_file_info_status( p_bt_sub_file, FILE_INFO_OPENING );

	return SUCCESS;

ErrHandle1:
	unit_file_info( p_bt_sub_file->_file_info_ptr );
	
ErrHandle0:
	file_info_free_node( p_bt_sub_file->_file_info_ptr );
	
	p_bt_sub_file->_file_info_ptr = NULL;
	return ret_val;
	
}


_int32 bfm_open_continue_file_info(BT_SUB_FILE *p_bt_sub_file
                                   , struct _tag_file_info *p_file_info
                                   , char* cfg_file_path )
{
	_int32 ret_val =  SUCCESS;

	char  data_file_full_path [MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN] = {0};	
	_u32 cfg_len = 0;
	
	LOG_DEBUG( "bfm_open_continue_file_info,cfg_file_path:%s ", 
		cfg_file_path );	

	if( sd_file_exist(cfg_file_path) == FALSE )
	{
		LOG_DEBUG("bfm_open_continue_file_info, cfg file %s not exsist .", cfg_file_path);	
		ret_val = SUCCESS;  		
		return ret_val;		

	}

	cfg_len = sd_strlen(cfg_file_path);
	if( cfg_len > MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN )
	{
		LOG_DEBUG("bfm_open_continue_file_info, cfg file %s is too long :%u .", cfg_file_path, cfg_len);	
		ret_val = OPEN_OLD_FILE_FAIL;  		
		return ret_val;		


	}

	sd_strncpy( data_file_full_path, cfg_file_path, MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN );

	data_file_full_path[cfg_len-4] = '\0';

	if( sd_file_exist(data_file_full_path) == FALSE )
	{
		LOG_DEBUG("bfm_open_continue_file_info, data file %s not exsist .", data_file_full_path);	
		ret_val = OPEN_OLD_FILE_FAIL;  		
		return ret_val;		

	}

    

	if(file_info_load_from_cfg_file( p_file_info,cfg_file_path) == FALSE )
	{
		ret_val = OPEN_OLD_FILE_FAIL;
		LOG_ERROR("bfm_open_continue_file_info, cfg file %s load err!! .", cfg_file_path);	
		sd_delete_file(cfg_file_path);
		sd_delete_file(data_file_full_path);
		
		ret_val = SUCCESS;
	}

    p_bt_sub_file->_is_continue_task = TRUE;

	LOG_DEBUG("bfm_open_continue_file_info, ret : %d .", ret_val);	

	return ret_val;
}


_int32 bfm_uninit_file_info( BT_FILE_MANAGER *p_bt_file_manager, BT_SUB_FILE *p_bt_sub_file )
{
	LOG_DEBUG( "bfm_uninit_file_info, file_index:%u, cur_file_info_num ",
		p_bt_sub_file->_file_index, p_bt_file_manager->_cur_file_info_num );
	unit_file_info( p_bt_sub_file->_file_info_ptr );
	
	p_bt_sub_file->_has_bcid = FALSE;
	
	p_bt_file_manager->_cur_file_info_num--;
	

	if( p_bt_file_manager->_cur_downloading_file_size >= p_bt_sub_file->_file_size )
	{
		p_bt_file_manager->_cur_downloading_file_size -= p_bt_sub_file->_file_size;
	}
	else
	{
		sd_assert( FALSE );
	}

	if( p_bt_sub_file->_file_size > g_bt_max_cur_downloading_size )
	{
		p_bt_file_manager->_big_sub_file_ptr = NULL;
		p_bt_file_manager->_cur_range_step = 0;
	}
	
	uninit_speed_calculator( &p_bt_sub_file->_file_download_speed );

	return SUCCESS;
}


_int32 bfm_dispatch_common_file( BT_FILE_MANAGER *p_bt_file_manager )
{
	_int32 ret_val				= SUCCESS;
	_u32 last_error_code		= SUCCESS;
	MAP_ITERATOR cur_node		= NULL; 
	BT_SUB_FILE *p_bt_sub_file	= NULL;
	_u32 file_index				= 0;
	_u32 success_file_count		= 0;
	_u32 failed_file_count		= 0;
	BOOL is_vod_mode			= FALSE;
	BOOL keep_running			= FALSE;
	

	is_vod_mode = bdm_is_vod_mode( p_bt_file_manager->_bt_data_manager_ptr );

	LOG_DEBUG( " bfm_dispatch_common_file, cur_file_info_num:%u, is_vod_mode:%d, is_task_success:%d",
		p_bt_file_manager->_cur_file_info_num, is_vod_mode, p_bt_file_manager->_is_report_task );

	if( p_bt_file_manager->_is_closing ) return SUCCESS;

	cur_node = MAP_BEGIN( p_bt_file_manager->_bt_sub_file_map );
	while( cur_node != MAP_END( p_bt_file_manager->_bt_sub_file_map ) )
	{
		p_bt_sub_file = (BT_SUB_FILE *)MAP_VALUE( cur_node );
		file_index = p_bt_sub_file->_file_index;
		LOG_DEBUG( " bfm_dispatch_common_file, file_index:%u, _file_status:%d, err_code:%d",
			p_bt_sub_file->_file_index, p_bt_sub_file->_file_status, p_bt_sub_file->_err_code );

		if(p_bt_sub_file->_err_code != SUCCESS)
		{
			last_error_code = p_bt_sub_file->_err_code;
		}

		switch(p_bt_sub_file->_file_status)
		{
		case BT_FILE_IDLE:
			keep_running = TRUE;
			break;
		case BT_FILE_DOWNLOADING:
			keep_running = TRUE;
			if(( p_bt_sub_file->_err_code == SUCCESS || p_bt_sub_file->_err_code == BT_VOD_WAIT_CLOSE )
				&& bfm_is_slow_bt_sub_file(p_bt_sub_file))
			{
				sd_assert( p_bt_file_manager->_cur_file_info_num > 0 );
				ret_val = bfm_handle_file_failture( p_bt_file_manager, p_bt_sub_file, BT_COMMON_FILE_TOO_SLOW );
				sd_assert( ret_val == SUCCESS );
			}
			break;
		case BT_FILE_FINISHED:
			success_file_count++;
			break;
		case BT_FILE_FAILURE:
			failed_file_count++;
			break;
		default:
			LOG_ERROR("BT FILE STATUS ERROR: %d", p_bt_sub_file->_file_status);
			sd_assert(FALSE);
		}

		cur_node = MAP_NEXT( p_bt_file_manager->_bt_sub_file_map, cur_node);
	}

	if(keep_running)
	{ //还有子文件需要调度
		if( !is_vod_mode 
			&& p_bt_file_manager->_cur_file_info_num < bfm_get_max_file_info_num( p_bt_file_manager ) )
		{
			ret_val = bfm_start_create_common_file_info( p_bt_file_manager );
			sd_assert( ret_val == SUCCESS );
		}
		LOG_DEBUG(">>>>> continue dispatching >>>>>>");
		return SUCCESS;	
	}
	else
	{ //所有子文件调度完成
		if(0 == failed_file_count)
		{ //全部子文件成功
			if( !p_bt_file_manager->_is_start_finished_task
				&& !p_bt_file_manager->_is_report_task )
			{
				bdm_notify_report_task_success( p_bt_file_manager->_bt_data_manager_ptr );
				bdm_notify_del_tmp_file(p_bt_file_manager->_bt_data_manager_ptr);
				p_bt_file_manager->_is_start_finished_task = TRUE;
				p_bt_file_manager->_is_report_task = TRUE;
			}
			if(!is_vod_mode)
			{ //非vod任务通知成功，结束任务
				bfm_nofity_file_download_success( p_bt_file_manager );
				LOG_DEBUG("yyyyyyyyyy notify task success yyyyyyyyyy");
			}
			return SUCCESS;
		}
		else 
		{ 
			if(success_file_count > 0 && is_vod_mode)
			{//部分文件成功，且是vod任务，让任务保留
				return SUCCESS;
			}
			bfm_nofity_file_download_failture( p_bt_file_manager, last_error_code );
			LOG_DEBUG("xxxxxxxxx notify task failed last_errcode=%d xxxxxxxxxx", last_error_code);
			return SUCCESS;
		}
	}
}

//
//_int32 bfm_dispatch_common_file( BT_FILE_MANAGER *p_bt_file_manager )
//{
//	_int32 ret_val = SUCCESS;
//	MAP_ITERATOR cur_node = MAP_BEGIN( p_bt_file_manager->_bt_sub_file_map );
//	BT_SUB_FILE *p_bt_sub_file = NULL;
//	_u32 file_index = 0;
//	BOOL is_success = TRUE;
//	BOOL is_failture = TRUE;
//	BOOL is_vod_mode = FALSE;
//	BOOL is_task_success = TRUE;
//	_u32 error_code = SUCCESS;
//
//	is_vod_mode = bdm_is_vod_mode( p_bt_file_manager->_bt_data_manager_ptr );
//
//	LOG_DEBUG( " bfm_dispatch_common_file, cur_file_info_num:%u, is_vod_mode:%d, is_task_success:%d",
//		p_bt_file_manager->_cur_file_info_num, is_vod_mode, p_bt_file_manager->_is_report_task );
//	
//	//if( is_vod_mode ) return SUCCESS;
//	if( p_bt_file_manager->_is_closing ) return SUCCESS;
//	
//	while( cur_node != MAP_END( p_bt_file_manager->_bt_sub_file_map ) )
//	{
//		p_bt_sub_file = (BT_SUB_FILE *)MAP_VALUE( cur_node );
//		file_index = p_bt_sub_file->_file_index;
//		LOG_DEBUG( " bfm_dispatch_common_file, file_index:%u, _file_status:%d, err_code:%d",
//			p_bt_sub_file->_file_index, p_bt_sub_file->_file_status, p_bt_sub_file->_err_code );
//
//        if(p_bt_sub_file->_err_code != SUCCESS)
//        {
//            error_code = p_bt_sub_file->_err_code;
//        }
//
//		if( p_bt_sub_file->_file_status == BT_FILE_IDLE
//			|| p_bt_sub_file->_file_status == BT_FILE_DOWNLOADING )
//		{
//			is_failture = FALSE;
//		}
//		if( is_success && p_bt_sub_file->_file_status != BT_FILE_FINISHED )
//		{
//			is_success = FALSE;
//		}
//		if( is_task_success )
//		{
//			if( p_bt_sub_file->_file_status != BT_FILE_FINISHED 
//				&& p_bt_sub_file->_err_code != BT_VOD_WAIT_CLOSE )
//			{
//				is_task_success = FALSE;
//			}
//		}
//
//		if( p_bt_sub_file->_file_status == BT_FILE_DOWNLOADING
//			&& ( p_bt_sub_file->_err_code == SUCCESS || p_bt_sub_file->_err_code == BT_VOD_WAIT_CLOSE )
//			&& bfm_is_slow_bt_sub_file(p_bt_sub_file) )
//		{
//			sd_assert( p_bt_file_manager->_cur_file_info_num > 0 );
//			ret_val = bfm_handle_file_failture( p_bt_file_manager, p_bt_sub_file, BT_COMMON_FILE_TOO_SLOW );
//			sd_assert( ret_val == SUCCESS );
//		}
//
//		cur_node = MAP_NEXT( p_bt_file_manager->_bt_sub_file_map, cur_node);
//	}
//
//
//	if( is_task_success || is_success )
//	{
//		if( !p_bt_file_manager->_is_start_finished_task
//			&& !p_bt_file_manager->_is_report_task )
//		{
//			bdm_notify_report_task_success( p_bt_file_manager->_bt_data_manager_ptr );
//			bdm_notify_del_tmp_file(p_bt_file_manager->_bt_data_manager_ptr);
//			p_bt_file_manager->_is_start_finished_task = TRUE;
//			p_bt_file_manager->_is_report_task = TRUE;
//		}	
//	}
//	
//	if( is_success )
//	{
//		if( is_vod_mode )
//		{
//			return SUCCESS;
//		}
//		bfm_nofity_file_download_success( p_bt_file_manager );
//
//		return SUCCESS;
//	}
//
//	if( is_failture )
//	{
//		sd_assert( !is_success );
//		bfm_nofity_file_download_failture( p_bt_file_manager, error_code );
//		return SUCCESS;
//	}	
//	
//	if( is_vod_mode )
//	{
//		return SUCCESS;
//	}	
//	
//	if( p_bt_file_manager->_cur_file_info_num < bfm_get_max_file_info_num( p_bt_file_manager ) )
//	{
//		ret_val = bfm_start_create_common_file_info( p_bt_file_manager );
//		sd_assert( ret_val == SUCCESS );
//	}
//	return SUCCESS;	
//
//}

_int32 bfm_stop_sub_file( BT_FILE_MANAGER *p_bt_file_manager, BT_SUB_FILE *p_bt_sub_file )
{
	LOG_DEBUG( " bfm_stop_sub_file, file_index:%d, file_status:%d", 
		p_bt_sub_file->_file_index, p_bt_sub_file->_sub_file_info_status );

	bdm_file_manager_notify_stop_file( p_bt_file_manager->_bt_data_manager_ptr, p_bt_sub_file->_file_index, p_bt_sub_file->_is_ever_vod_file );

	if( p_bt_sub_file->_sub_file_info_status == FILE_INFO_CLOSING
		|| p_bt_sub_file->_sub_file_info_status == FILE_INFO_CLOSED )
	{
		return SUCCESS;
	}
	
	bfm_enter_file_info_status( p_bt_sub_file, FILE_INFO_CLOSING );
	
	file_info_close_tmp_file( p_bt_sub_file->_file_info_ptr );

	return SUCCESS;
}


BOOL bfm_is_slow_bt_sub_file( BT_SUB_FILE *p_bt_sub_file )
{
	_u32 speed = 0;
	_int32 ret_val = SUCCESS;
	BOOL is_slow = FALSE;

	sd_assert( p_bt_sub_file->_file_status == BT_FILE_DOWNLOADING );

	ret_val = calculate_speed( &p_bt_sub_file->_file_download_speed, &speed );
	sd_assert( ret_val == SUCCESS );

	if( speed == 0 )
	{
		p_bt_sub_file->_idle_ticks++;
	}
	else
	{
		p_bt_sub_file->_idle_ticks = 0;
	}

	if( p_bt_sub_file->_idle_ticks < g_bt_sub_file_idle_ticks )
	{
		is_slow = FALSE;
	}
	else
	{
		is_slow = TRUE;
	}
	/*
	if( !p_bt_sub_file->_is_speedup_failed && p_bt_sub_file->_has_bcid )
	{
		is_slow = FALSE;
		p_bt_sub_file->_idle_ticks = 0;
	}
	*/
	if( p_bt_sub_file->_file_size == p_bt_sub_file->_file_writed_bytes )
	{
		LOG_DEBUG( " bfm_is_slow_bt_sub_file: is_checking_file" );
		is_slow = FALSE;
		p_bt_sub_file->_idle_ticks = 0;
	}

	if( p_bt_sub_file->_sub_file_info_status == FILE_INFO_OPENING
		&& p_bt_sub_file->_file_info_ptr != NULL
		&& file_info_has_tmp_file( p_bt_sub_file->_file_info_ptr ) )
	{
		LOG_DEBUG( " bfm_is_slow_bt_sub_file: is_createing_file" );

		is_slow = FALSE;
		p_bt_sub_file->_idle_ticks = 0;
	}

	LOG_DEBUG( " bfm_is_slow_bt_sub_file: file_index:%u, speed:%u, ticks:%u, is_slow:%d, _is_speedup_failed:%u, file_size:%llu, writed_bytes:%llu",
		p_bt_sub_file->_file_index, speed, p_bt_sub_file->_idle_ticks, is_slow, p_bt_sub_file->_is_speedup_failed,
		p_bt_sub_file->_file_size, p_bt_sub_file->_file_writed_bytes );

	return is_slow;
}

_int32 bfm_get_bt_sub_file_ptr( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, BT_SUB_FILE **pp_bt_sub_file )
{
	MAP_ITERATOR cur_node = NULL;
	_int32 ret_val = SUCCESS;
	
	LOG_DEBUG( " bfm_get_bt_sub_file_ptr");
	ret_val = map_find_iterator( &p_bt_file_manager->_bt_sub_file_map, (void *)file_index, &cur_node );
	sd_assert( ret_val == SUCCESS );

	if( cur_node == MAP_END( p_bt_file_manager->_bt_sub_file_map ) )
	{
		LOG_ERROR( "bfm_get_bt_sub_file_ptr. file_index:%d,BT_ERROR_FILE_INDEX", 
			file_index );
		return BT_ERROR_FILE_INDEX;
	}

	*pp_bt_sub_file = MAP_VALUE( cur_node );
	return SUCCESS;
}

_int32 bfm_get_file_info_ptr( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, struct _tag_file_info **pp_file_info )
{
	MAP_ITERATOR cur_node = NULL;
	_int32 ret_val = SUCCESS;
	BT_SUB_FILE *p_bt_sub_file = NULL;
	LOG_DEBUG( " bfm_get_file_info_ptr, file_index:%u", file_index );

	ret_val = map_find_iterator( &p_bt_file_manager->_bt_sub_file_map, (void *)file_index, &cur_node );
	sd_assert( ret_val == SUCCESS );

	if( cur_node == MAP_END( p_bt_file_manager->_bt_sub_file_map ) )
	{
		LOG_ERROR( " bfm_get_file_info_ptr,file_index:%u, return:BT_ERROR_FILE_INDEX", file_index );
		return BT_ERROR_FILE_INDEX;
	}

	p_bt_sub_file = MAP_VALUE( cur_node );
	*pp_file_info = p_bt_sub_file->_file_info_ptr;
	if( *pp_file_info == NULL )
	{
		LOG_ERROR( " bfm_get_file_info_ptr.file_index:%u return:BT_FILE_INFO_NULL", file_index );
		return BT_FILE_INFO_NULL;
	}

	LOG_DEBUG( "bfm_get_file_info_ptr.file_index:%u, *pp_file_info:0x%x",
		file_index, *pp_file_info );
			
	return SUCCESS;
}


_u32 bfm_get_max_file_info_num( BT_FILE_MANAGER *p_bt_file_manager )
{
	return g_bt_sub_file_max_num;
}

void bfm_notify_range_check_result( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, RANGE *p_range, BOOL  check_result )
{
	LOG_DEBUG( " bfm_notify_range_check_result");
	
	bdm_notify_sub_file_range_check_finished( p_bt_file_manager->_bt_data_manager_ptr, file_index, p_range, check_result );
}

void bfm_nofity_file_close_success( BT_FILE_MANAGER *p_bt_file_manager )
{	
	MAP_ITERATOR cur_node = MAP_BEGIN( p_bt_file_manager->_bt_sub_file_map );
	BT_SUB_FILE *p_bt_sub_file = NULL;
	_int32 ret_val = SUCCESS;
	if( p_bt_file_manager->_close_need_call_back ) return;
	LOG_URGENT( "bfm_nofity_file_close_success" );		
	while( cur_node != MAP_END( p_bt_file_manager->_bt_sub_file_map ) )
	{
		p_bt_sub_file = (BT_SUB_FILE *)MAP_VALUE( cur_node );
		if( p_bt_sub_file->_sub_file_info_status != FILE_INFO_CLOSED )
		{
			return;
		}
		cur_node = MAP_NEXT( p_bt_file_manager->_bt_sub_file_map, cur_node );
	}
	sd_assert( p_bt_file_manager->_is_closing );
		
	ret_val = bfm_destroy_bt_sub_file_struct( p_bt_file_manager );
	
	sd_assert( p_bt_file_manager->_cur_file_info_num == 0 );
	bdm_notify_task_stop_success( p_bt_file_manager->_bt_data_manager_ptr );
}


void bfm_nofity_file_download_success( BT_FILE_MANAGER *p_bt_file_manager )
{	
	LOG_DEBUG( "bfm_nofity_file_download_success");		
	bdm_notify_task_download_success( p_bt_file_manager->_bt_data_manager_ptr );
}

void bfm_nofity_file_download_failture( BT_FILE_MANAGER *p_bt_file_manager, _int32 err_code )
{	
	LOG_URGENT( "bfm_nofity_file_download_failture, err_code:%d ", err_code );	
	bdm_notify_download_failed( p_bt_file_manager->_bt_data_manager_ptr, err_code );
}

void bfm_enter_file_info_status( BT_SUB_FILE *p_bt_sub_file, enum tagSUB_FILE_INFO_STATUS status )
{	
	LOG_DEBUG( "bfm_enter_file_info_status:file_index:%d, status: %d",
		p_bt_sub_file->_file_index, status );		
	p_bt_sub_file->_sub_file_info_status = status;
}

void bfm_enter_file_status( BT_SUB_FILE *p_bt_sub_file, enum BT_FILE_STATUS status )
{
	LOG_DEBUG( "bfm_enter_file_status:file_index:%d, status:%d",
		p_bt_sub_file->_file_index, status );		
	p_bt_sub_file->_file_status = status;
}

void bfm_enter_file_err_code( BT_SUB_FILE *p_bt_sub_file, _int32 err_code )
{
    LOG_DEBUG( "bfm_enter_file_err_code, file_index:%d, err_code:%d"
        , p_bt_sub_file->_file_index, err_code );					
	p_bt_sub_file->_err_code = err_code;
}

void bfm_handle_file_err_code( BT_FILE_MANAGER *p_bt_file_manager )
{
	MAP_ITERATOR cur_node = MAP_BEGIN( p_bt_file_manager->_bt_sub_file_map );
	BT_SUB_FILE *p_bt_sub_file = NULL;
	_int32 ret_val = SUCCESS;
	
	LOG_DEBUG( "bfm_handle_file_err_code " );		
	while( cur_node != MAP_END( p_bt_file_manager->_bt_sub_file_map ) )
	{
		p_bt_sub_file = (BT_SUB_FILE *)MAP_VALUE( cur_node );
		
		LOG_DEBUG( "bfm_handle_file_err_code:file_index:%u, err_code:%d", 
			p_bt_sub_file->_file_index, p_bt_sub_file->_err_code );		
		if( p_bt_sub_file->_err_code == BT_FILE_DOWNLOAD_SUCCESS_WAIT_CHECK )
		{
			sd_assert( p_bt_sub_file->_file_status == BT_FILE_DOWNLOADING );

			if( p_bt_sub_file->_has_bcid )
			{
				ret_val = bfm_handle_file_success( p_bt_file_manager, p_bt_sub_file );
				sd_assert( ret_val == SUCCESS );
			}
			else
			{
				ret_val = bdm_check_if_download_success( p_bt_file_manager->_bt_data_manager_ptr, p_bt_sub_file->_file_index );
				sd_assert( ret_val == SUCCESS );
			}
		}	
		else if( p_bt_sub_file->_err_code == BT_DATA_CHECK_CID_NO_BUFFER
			|| p_bt_sub_file->_err_code == BT_VOD_WAIT_CLOSE )
		{
			sd_assert( p_bt_sub_file->_file_status == BT_FILE_DOWNLOADING );
			bfm_handle_file_success( p_bt_file_manager, p_bt_sub_file );
		}	
		else if( p_bt_sub_file->_err_code == BT_DATA_CHECK_CID_NO_BUFFER )
		{
			sd_assert( p_bt_sub_file->_file_status == BT_FILE_DOWNLOADING );
			sd_assert( p_bt_sub_file->_file_info_ptr != NULL );
			bfm_enter_file_err_code( p_bt_sub_file, SUCCESS );
			start_check_blocks( p_bt_sub_file->_file_info_ptr );
		}	
		else if( p_bt_sub_file->_err_code == BT_VOD_WAIT_OPEN )
		{
			if( bdm_get_vod_file_index( p_bt_file_manager->_bt_data_manager_ptr ) == p_bt_sub_file->_file_index )
			{
				sd_assert( p_bt_sub_file->_sub_file_info_status != FILE_INFO_OPENING
					&& p_bt_sub_file->_sub_file_info_status != FILE_INFO_OPENED );
				if( p_bt_sub_file->_sub_file_info_status != FILE_INFO_CLOSING )
				{
					bfm_enter_file_err_code( p_bt_sub_file, SUCCESS );
				}	
				
				if( p_bt_sub_file->_sub_file_info_status == FILE_INFO_CLOSED )
				{
					//是点播文件,并且文件已经关闭,重新打开
					bfm_start_single_file_info( p_bt_file_manager, p_bt_sub_file );
				}
			}
			else
			{
				//已经不是点播文件,不处理
				bfm_enter_file_err_code( p_bt_sub_file, SUCCESS );
			}
		}
		cur_node = MAP_NEXT( p_bt_file_manager->_bt_sub_file_map, cur_node );
	}
}

_int32 bfm_vod_only_open_file_info( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index )
{
	MAP_ITERATOR cur_node = MAP_BEGIN( p_bt_file_manager->_bt_sub_file_map );
	BT_SUB_FILE *p_bt_sub_file = NULL;
	_int32 ret_val = SUCCESS;

	LOG_DEBUG( "bfm_vod_only_open_file_info, file_index:%u ", file_index );		
	ret_val = bfm_get_bt_sub_file_ptr( p_bt_file_manager, file_index, &p_bt_sub_file );
	if( ret_val != SUCCESS )
	{
		CHECK_VALUE( BT_ERROR_FILE_INDEX );	
	}

	if( p_bt_sub_file->_file_status == BT_FILE_FINISHED )
	{
		return BT_VOD_FILE_DOWNLODD_FINISHED;
	}
	
	p_bt_sub_file->_is_ever_vod_file = TRUE;

	while( cur_node != MAP_END( p_bt_file_manager->_bt_sub_file_map ) )
	{
		p_bt_sub_file = (BT_SUB_FILE *)MAP_VALUE( cur_node );
		LOG_DEBUG( "bfm_vod_only_open_file_info, file_index:%u, sub_file_info_satus:%d ",
			p_bt_sub_file->_file_index, p_bt_sub_file->_sub_file_info_status );		
		if( p_bt_sub_file->_file_index == file_index )
		{
			if( p_bt_sub_file->_sub_file_info_status == FILE_INFO_CLOSING )
			{
				bfm_enter_file_err_code( p_bt_sub_file, BT_VOD_WAIT_OPEN );
			}
			else if( p_bt_sub_file->_sub_file_info_status == FILE_INFO_CLOSED )
			{
				bfm_start_single_file_info( p_bt_file_manager, p_bt_sub_file );
			}
			cur_node = MAP_NEXT( p_bt_file_manager->_bt_sub_file_map, cur_node );
			continue;
		}

		sd_assert( p_bt_sub_file->_file_index != file_index );
		if( ( p_bt_sub_file->_sub_file_info_status == FILE_INFO_OPENING
			|| p_bt_sub_file->_sub_file_info_status == FILE_INFO_OPENED )
			&& p_bt_sub_file->_err_code != BT_VOD_WAIT_CLOSE )
		{
			bfm_enter_file_status( p_bt_sub_file, BT_FILE_IDLE );
			bfm_enter_file_err_code( p_bt_sub_file, SUCCESS );
			ret_val = bfm_stop_sub_file( p_bt_file_manager,p_bt_sub_file );
			sd_assert( ret_val == SUCCESS );
		}
		
		cur_node = MAP_NEXT( p_bt_file_manager->_bt_sub_file_map, cur_node );
	}
	return SUCCESS;
}

_int32 bfm_get_file_err_code( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index )
{
	BT_SUB_FILE *p_bt_sub_file = NULL;
	_int32 ret_val = SUCCESS;
	
	ret_val = bfm_get_bt_sub_file_ptr( p_bt_file_manager, file_index, &p_bt_sub_file );
	CHECK_VALUE( ret_val );
	
	LOG_DEBUG("bfm_get_file_err_code .file_index:%d, file_status:%d", 
		file_index, p_bt_sub_file->_err_code );
	
	sd_assert( p_bt_sub_file != NULL );
	if( p_bt_sub_file == NULL ) return BT_DATA_MANAGER_LOGIC_ERROR;
	return p_bt_sub_file->_err_code;
}

BOOL bfm_update_big_file_downloading_range( BT_FILE_MANAGER *p_bt_file_manager )
{
	LOG_DEBUG("bfm_update_big_file_downloading_range" );
	
	if( p_bt_file_manager->_big_sub_file_ptr == NULL ) return FALSE;
	
	bfm_add_file_downloading_range( p_bt_file_manager, p_bt_file_manager->_big_sub_file_ptr );
	return TRUE;
}

BOOL bfm_add_no_need_check_range( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, RANGE *p_sub_range )
{
	FILEINFO *p_file_info = NULL;
	_int32 ret_val = SUCCESS;
	BOOL ret_bool = TRUE;
	
	ret_val = bfm_get_file_info_ptr( p_bt_file_manager, file_index, &p_file_info );
	LOG_DEBUG("bfm_add_no_need_check_range" );	
	
	if( ret_val != SUCCESS ) return TRUE;
			
	ret_bool = file_info_add_no_need_check_range( p_file_info, p_sub_range );
	
	LOG_DEBUG("bfm_add_no_need_check_range: %d", ret_bool );	
	return ret_bool;
}


struct tagFILE_INFO_REPORT_PARA *bfm_get_report_para( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index )
{
#ifdef EMBED_REPORT
	FILEINFO *p_file_info = NULL;
	_int32 ret_val = SUCCESS;
	
	ret_val = bfm_get_file_info_ptr( p_bt_file_manager, file_index, &p_file_info );
	LOG_DEBUG("bfm_get_report_para" );	
	
	if( ret_val != SUCCESS ) return NULL;
	return file_info_get_report_para( p_file_info );
#endif
}

void bfm_handle_err_data_report( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, RESOURCE *p_res, _u32 data_len )
{
#ifdef EMBED_REPORT
	FILEINFO *p_file_info = NULL;
	_int32 ret_val = SUCCESS;
	
	ret_val = bfm_get_file_info_ptr( p_bt_file_manager, file_index, &p_file_info );
	LOG_DEBUG("bfm_get_report_para" );	
	
	if( ret_val != SUCCESS ) return;
	file_info_handle_err_data_report( p_file_info, p_res, data_len );
#endif
}

void bfm_get_assist_peer_dl_size( BT_FILE_MANAGER *p_bt_file_manager, _u64 *p_assist_peer_dl_bytes)
{
#ifdef EMBED_REPORT
	*p_assist_peer_dl_bytes = p_bt_file_manager->_assist_peer_dl_bytes;
	LOG_DEBUG("bfm_get_assist_peer_dl_size: _assist_peer_dl_bytes=%llu", *p_assist_peer_dl_bytes);	
#endif 
}


#endif /* #ifdef ENABLE_BT */

