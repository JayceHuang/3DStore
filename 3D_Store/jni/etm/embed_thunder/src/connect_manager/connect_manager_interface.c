#include "utility/define.h"
#include "utility/list.h"
#include "utility/url.h"
#include "utility/map.h"
#include "utility/settings.h"
#include "utility/peer_capability.h"
#include "utility/logid.h"
#define LOGID LOGID_CONNECT_MANAGER
#include "utility/logger.h"
#include "platform/sd_network.h"

#include "connect_manager_interface.h"
#include "connect_manager_imp.h"
#include "global_connect_dispatch.h"

#include "global_connect_dispatch.h"
#include "data_manager/file_info.h"
#include "http_data_pipe/http_data_pipe_interface.h"
#include "ftp_data_pipe/ftp_data_pipe_interface.h"
#include "p2p_data_pipe/p2p_pipe_interface.h"
#include "pipe_function_table_builder.h"
#include "data_manager/data_manager_interface.h"
#include "download_task/download_task.h"
#include "p2sp_download/p2sp_task.h"
#include "vod_data_manager/vod_data_manager_interface.h"

#ifdef ENABLE_BT 
#include "bt_download/bt_task/bt_task_interface.h"
#include "bt_download/bt_data_manager/bt_data_manager_interface.h"
#include "bt_download/bt_data_pipe/bt_pipe_interface.h"
#endif /* #ifdef ENABLE_BT  */

#ifdef ENABLE_EMULE
#include "emule/emule_interface.h"
#endif

#define CM_BASE_PIPE_NUM 30
#define CM_CONNECT_DISPATCH_INTERVAL 1000
#define CM_MAX_CREATE_COUNT 2

static _int32 cm_get_need_create_pipes_info(CONNECT_MANAGER* p_connect_manager
        , _int32* need_create_num);
static _int32 cm_inner_create_pipes( CONNECT_MANAGER *p_connect_manager );


_int32 init_connect_manager_module(void)
{
	_int32 ret_val = SUCCESS;

	build_globla_pipe_function_table();
	
	cm_init_connect_manager_setting();

	ret_val = gcm_init_struct();
	CHECK_VALUE( ret_val );
	
	return SUCCESS;
}

_int32 uninit_connect_manager_module(void)
{
	gcm_uninit_struct();
	
	cm_uninit_connect_manager_setting();

	clear_globla_pipe_function_table();
	
	return SUCCESS;
}

_int32 cm_init_connect_manager(CONNECT_MANAGER* p_connect_manager, struct _tag_data_manager *p_data_manager, can_destory_resource_call_back p_call_back)
{
	_int32 ret_val = SUCCESS;
	void **pp_func_table = pft_get_common_pipe_function_table();
	LOG_DEBUG( "cm_init_connect_manager.ptr:0x%x.", p_connect_manager );
	
	ret_val = cm_init_struct(p_connect_manager, (void *)p_data_manager, p_call_back);
	CHECK_VALUE( ret_val );

 	pi_init_pipe_interface( &p_connect_manager->_pipe_interface, COMMON_PIPE_INTERFACE_TYPE, 
		INVALID_FILE_INDEX, NULL, pp_func_table );

	ret_val = gcm_register_connect_manager( p_connect_manager );
	CHECK_VALUE( ret_val );

	return SUCCESS;
}

_int32 cm_uninit_connect_manager(CONNECT_MANAGER* p_connect_manager )
{
	_int32 ret_val = SUCCESS;
	MAP_ITERATOR map_node = NULL;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;

	map_node = MAP_BEGIN( p_connect_manager->_sub_connect_manager_map );
	while( map_node != MAP_END( p_connect_manager->_sub_connect_manager_map ) )
	{
		p_sub_connect_manager = MAP_VALUE( map_node );
		cm_uninit_connect_manager( p_sub_connect_manager );
		/*------------------------------------  memory lost*/
		sd_free(p_sub_connect_manager);
		p_sub_connect_manager = NULL;
		map_node = MAP_NEXT( p_connect_manager->_sub_connect_manager_map, map_node );
	}

	map_clear( &p_connect_manager->_sub_connect_manager_map );	
	
	LOG_DEBUG( "cm_unit_connect_manager begin. connect_manager ptr:0x%x, cur pipe num:%u, cur res num:%u",
		p_connect_manager, p_connect_manager->_cur_pipe_num, p_connect_manager->_cur_res_num );

	ret_val = cm_uninit_struct( p_connect_manager );
	CHECK_VALUE( ret_val );

	ret_val = gcm_unregister_connect_manager( p_connect_manager );
	CHECK_VALUE( ret_val );
	return SUCCESS;
}

#ifdef ENABLE_BT

_int32 cm_init_bt_connect_manager( CONNECT_MANAGER* p_connect_manager, struct tagBT_DATA_MANAGER *p_bt_data_manager, can_destory_resource_call_back p_call_back )
{
	_int32 ret_val = SUCCESS;
	void *range_swticher_ptr = NULL;
	void **pp_func_table = pft_get_bt_pipe_function_table();
	
	LOG_DEBUG( "cm_init_bt_connect_manager.ptr:0x%x.", p_connect_manager );
 	
	range_swticher_ptr = bdm_get_range_switch( p_bt_data_manager );	
	
	ret_val = cm_init_struct( p_connect_manager, (void *)p_bt_data_manager, p_call_back );
	CHECK_VALUE( ret_val );

	pi_init_pipe_interface( &p_connect_manager->_pipe_interface, BT_PIPE_INTERFACE_TYPE, 
		INVALID_FILE_INDEX, range_swticher_ptr, pp_func_table );

	ret_val = gcm_register_connect_manager( p_connect_manager );
	CHECK_VALUE( ret_val );

	return SUCCESS;
}

_int32 cm_uninit_bt_connect_manager(CONNECT_MANAGER* p_connect_manager )
{
	LOG_DEBUG( "cm_uninit_bt_connect_manager.ptr:0x%x.", p_connect_manager );
	sd_assert( p_connect_manager->_pipe_interface._pipe_interface_type == BT_PIPE_INTERFACE_TYPE );
	return cm_uninit_connect_manager( p_connect_manager );
}

_int32 cm_create_sub_connect_manager( CONNECT_MANAGER *p_connect_manager, _u32 file_index )
{

	CONNECT_MANAGER *p_sub_connect_manager = NULL;
	_int32 ret_val = SUCCESS;
	struct tagBT_DATA_MANAGER *p_bt_data_manager = NULL;
	void *range_swticher_ptr = NULL;
	PAIR map_node;
	void **pp_func_table = pft_get_speedup_pipe_function_table();
	

	sd_assert( p_connect_manager->_pipe_interface._pipe_interface_type == BT_PIPE_INTERFACE_TYPE );
	p_bt_data_manager = ( struct tagBT_DATA_MANAGER *)p_connect_manager->_data_manager_ptr;

	range_swticher_ptr = bdm_get_range_switch( p_bt_data_manager );
		
	ret_val = sd_malloc( sizeof( CONNECT_MANAGER ), (void **)&p_sub_connect_manager );
	CHECK_VALUE( ret_val );
	LOG_DEBUG( "cm_create_sub_connect_manager.ptr:0x%x.file_index:%u, sub_connect_manager:0x%x", 
		p_connect_manager, file_index, p_sub_connect_manager );

	ret_val = cm_init_struct( p_sub_connect_manager, (void * )p_bt_data_manager, p_connect_manager->_call_back_ptr );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS )
	{
		SAFE_DELETE( p_sub_connect_manager );
		return ret_val;
	}
	
	p_sub_connect_manager->_main_task_ptr = p_connect_manager->_main_task_ptr;
	
	pi_init_pipe_interface( &p_sub_connect_manager->_pipe_interface, SPEEDUP_PIPE_INTERFACE_TYPE, 
		file_index, range_swticher_ptr, pp_func_table );


	map_node._key = (void *)file_index;
	map_node._value = (void *)p_sub_connect_manager;
	ret_val = map_insert_node( &p_connect_manager->_sub_connect_manager_map, &map_node );
	if( ret_val != SUCCESS )
	{
		SAFE_DELETE( p_sub_connect_manager );
		return ret_val;
	}

	ret_val = gcm_register_connect_manager( p_sub_connect_manager );
	sd_assert( ret_val == SUCCESS );

	return ret_val;

}

_int32 cm_init_bt_magnet_connect_manager( CONNECT_MANAGER* p_connect_manager, void *p_data_manager )
{
	_int32 ret_val = SUCCESS;

	LOG_DEBUG( "cm_init_bt_magnet_connect_manager.ptr:0x%x.", p_connect_manager );
	
	ret_val = cm_init_struct( p_connect_manager, (void *)p_data_manager, NULL );
	CHECK_VALUE( ret_val );
	p_connect_manager->_cm_level = INIT_CM_LEVEL;
	

	pi_init_pipe_interface( &p_connect_manager->_pipe_interface, BT_MAGNET_INTERFACE_TYPE, 
		INVALID_FILE_INDEX, NULL, NULL );

	return SUCCESS;
}

_int32 cm_uninit_bt_magnet_connect_manager(CONNECT_MANAGER* p_connect_manager )
{
	LOG_DEBUG( "cm_uninit_bt_magnet_connect_manager.ptr:0x%x.", p_connect_manager );
	sd_assert( p_connect_manager->_pipe_interface._pipe_interface_type == BT_MAGNET_INTERFACE_TYPE );
	return cm_uninit_struct( p_connect_manager );
}
#endif /* #ifdef ENABLE_BT  */

_int32 cm_delete_sub_connect_manager( CONNECT_MANAGER *p_connect_manager, _u32 file_index )
{
	_int32 ret_val = SUCCESS;
	MAP_ITERATOR cur_node;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;
	
	LOG_DEBUG( "cm_delete_sub_connect_manager. connect_manager ptr:0x%x, file_index:%u",
		p_sub_connect_manager, file_index );
	
	ret_val = map_find_iterator( &p_connect_manager->_sub_connect_manager_map, (void *)file_index, &cur_node );
	sd_assert( ret_val == SUCCESS );

	if( cur_node == MAP_END( p_connect_manager->_sub_connect_manager_map ) )
	{
		sd_assert( FALSE );
		return CM_LOGIC_ERROR;
	}

	p_sub_connect_manager = MAP_VALUE( cur_node );

	ret_val = map_erase_iterator( &p_connect_manager->_sub_connect_manager_map, cur_node );
	sd_assert( ret_val == SUCCESS );
	
	LOG_DEBUG( "cm_delete_sub_connect_manager begin. connect_manager ptr:0x%x, cur pipe num:%u, cur res num:%u",
		p_sub_connect_manager, p_sub_connect_manager->_cur_pipe_num, p_sub_connect_manager->_cur_res_num );

#ifdef EMBED_REPORT
	// 累计到主文件的conect manager上。
	range_list_add_range_list(&p_connect_manager->_report_para._res_normal_cdn_use_time_list,
		&p_sub_connect_manager->_report_para._res_normal_cdn_use_time_list);
	p_connect_manager->_report_para._res_normal_cdn_total += p_sub_connect_manager->_report_para._res_normal_cdn_total;
	p_connect_manager->_report_para._res_normal_cdn_valid += p_sub_connect_manager->_report_para._res_normal_cdn_valid;
	if (p_connect_manager->_report_para._res_normal_cdn_first_from_task_start != 0) {
		p_connect_manager->_report_para._res_normal_cdn_first_from_task_start = 
			MIN(p_connect_manager->_report_para._res_normal_cdn_first_from_task_start, 
				p_sub_connect_manager->_report_para._res_normal_cdn_first_from_task_start);
	}
	else {
		p_connect_manager->_report_para._res_normal_cdn_first_from_task_start = 
			p_sub_connect_manager->_report_para._res_normal_cdn_first_from_task_start;
	}
#endif

	ret_val = cm_uninit_struct( p_sub_connect_manager );
	CHECK_VALUE( ret_val );

	ret_val = gcm_unregister_connect_manager( p_sub_connect_manager );
	CHECK_VALUE( ret_val );

	/*------------------------------------  memory lost*/
	sd_free(p_sub_connect_manager);
	p_sub_connect_manager = NULL;
	return SUCCESS;
}


#ifdef ENABLE_EMULE
_int32 cm_init_emule_connect_manager( CONNECT_MANAGER* p_connect_manager, void* p_data_manager, can_destory_resource_call_back p_call_back,
											void** function_table)
{
	_int32 ret_val = SUCCESS;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;
	PAIR map_node;

	ret_val = cm_init_struct( p_connect_manager, p_data_manager, p_call_back );
	CHECK_VALUE( ret_val );
   
 	pi_init_pipe_interface( &p_connect_manager->_pipe_interface, EMULE_PIPE_INTERFACE_TYPE, 
		INVALID_FILE_INDEX, NULL, function_table);
	ret_val = gcm_register_connect_manager( p_connect_manager );
	CHECK_VALUE( ret_val );

 
	ret_val = sd_malloc( sizeof( CONNECT_MANAGER ), (void **)&p_sub_connect_manager );
	CHECK_VALUE( ret_val );
	LOG_DEBUG( "cm_init_emule_connect_manager.ptr:0x%x. sub_connect_manager:0x%x", 
		p_connect_manager, p_sub_connect_manager );

	ret_val = cm_init_struct( p_sub_connect_manager, (void * )p_connect_manager->_data_manager_ptr, p_connect_manager->_call_back_ptr );
	p_sub_connect_manager->_main_task_ptr = p_connect_manager->_main_task_ptr;
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS )
	{
		SAFE_DELETE( p_sub_connect_manager );
		return ret_val;
	}
	pi_init_pipe_interface( &p_sub_connect_manager->_pipe_interface, EMULE_SPEEDUP_PIPE_INTERFACE_TYPE, 
		0, NULL, function_table );
	
	ret_val = gcm_register_connect_manager( p_sub_connect_manager );
	sd_assert( ret_val == SUCCESS );

	map_node._key = (void *)0;
	map_node._value = (void *)p_sub_connect_manager;
	ret_val = map_insert_node( &p_connect_manager->_sub_connect_manager_map, &map_node );
	if( ret_val != SUCCESS )
	{
		SAFE_DELETE( p_sub_connect_manager );
		return ret_val;
	}   
	return SUCCESS;
}

_int32 cm_uninit_emule_connect_manager(CONNECT_MANAGER* p_connect_manager )
{
	_int32 ret_val = SUCCESS;
    LOG_DEBUG( "cm_uninit_emule_connect_manager.ptr:0x%x. ", p_connect_manager );
    cm_delete_sub_connect_manager(p_connect_manager, 0);
	ret_val = cm_uninit_struct( p_connect_manager );
	CHECK_VALUE( ret_val );
	ret_val = gcm_unregister_connect_manager( p_connect_manager );
	CHECK_VALUE( ret_val );
	return SUCCESS;
}

#endif


_int32 cm_add_server_resource( CONNECT_MANAGER* p_connect_manager, _u32 file_index, char *url, _u32 url_size, char* ref_url, _u32 ref_url_size,_u8 resource_priority )
{
	return cm_add_server_resource_ex(  p_connect_manager,  file_index, url, url_size,  ref_url, ref_url_size,resource_priority, NULL, FALSE, 0 );
}
_int32 cm_add_server_resource_ex( 
	CONNECT_MANAGER*		p_connect_manager,
	_u32					file_index,
	char *					url,
	_u32					url_size,
	char*					ref_url,
	_u32					ref_url_size,
	_u8						resource_priority,
	void*					v_p_task,
	BOOL					relation_res,
	_u32					relation_index )
{
	RESOURCE *p_server_res = NULL;
	_int32 ret_val = SUCCESS;
	enum SCHEMA_TYPE url_type;
	PAIR map_pair;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;
	_u32 hash_value = 0;
#if defined(ANDROID_HSC_TEST)||defined(ANDROID_CDN_TEST)
	return SUCCESS;
#endif

	if(resource_priority==106)
	{
#ifdef ENABLE_CDN
		return	cm_add_cdn_server_resource(p_connect_manager,file_index,url,  url_size, ref_url,  ref_url_size);
#endif
	}
	
    ret_val = cm_get_sub_connect_manager( p_connect_manager, file_index, &p_sub_connect_manager );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS ) return SUCCESS;
	
	cm_adjust_url_format(url,url_size);
	cm_adjust_url_format(ref_url,ref_url_size);

	if( cm_is_server_res_exist( p_sub_connect_manager, url, url_size, &hash_value ) )
	{
		return CM_ADD_SERVER_PEER_EXIST;
	}
	if( !gcm_is_need_more_res() && p_sub_connect_manager->_cur_res_num > cm_min_res_num() ) return SUCCESS;

	url_type = sd_get_url_type( url, url_size);

	if( !cm_is_enable_server_res( p_sub_connect_manager, url_type, FALSE ) ) return SUCCESS;
	
	LOG_DEBUG( "cm_add_server_resource: url = %s, ref_url = %s relation_res=%d, relation_index", 
		url, ref_url, relation_res, relation_index);

#ifdef EMBED_REPORT
	p_sub_connect_manager->_report_para._res_server_total++;
#endif

	if(( url_type == HTTP )||( url_type == HTTPS ))
	{
		if(relation_res)
		{
			TASK* p_task = (TASK*)v_p_task;
			sd_assert(p_task != NULL);
			sd_assert(p_task->_task_type == P2SP_TASK_TYPE);
			P2SP_TASK* p2sp_task = (P2SP_TASK*)p_task;
			sd_assert(p2sp_task->_relation_file_num > relation_index);

			P2SP_RELATION_FILEINFO* relation_file = p2sp_task->_relation_fileinfo_arr[relation_index];

			sd_assert(NULL != relation_file);

			extern void dump_relation_file_info(char* pos,_u32 i, P2SP_RELATION_FILEINFO* p_relation_info);
			dump_relation_file_info("cm_add_server_resource", relation_index,p2sp_task->_relation_fileinfo_arr[relation_index]);

			ret_val = http_resource_create_ex( url, url_size, ref_url, ref_url_size, FALSE, TRUE, 
				relation_file->_block_info_num, relation_file->_p_block_info_arr,
				relation_file->_file_size, &p_server_res );
		}
		else
		{
			ret_val = http_resource_create( url, url_size, ref_url, ref_url_size, FALSE,&p_server_res );
		}

		if( ret_val != SUCCESS ) return SUCCESS;
		LOG_DEBUG( "http_resource_create: resource ptr = 0x%x", p_server_res );
		sd_assert( p_server_res != NULL );	

	}
	else if( url_type == FTP && !relation_res)  // ftp 暂时不支持关联资源
	{
		ret_val = ftp_resource_create( url, url_size, FALSE, &p_server_res );
		//sd_assert( ret_val == SUCCESS );
		if( ret_val != SUCCESS ) return SUCCESS;
		sd_assert( p_server_res != NULL );
		LOG_DEBUG( "ftp_resource_create: resource ptr = 0x%x", p_server_res );
	}
	else
	{
		LOG_ERROR( "cm_add_server_unknown_resource" );
		return SUCCESS;
	}
	map_pair._key = (void *)hash_value;
	map_pair._value = (void *)p_server_res;
	map_insert_node( &p_sub_connect_manager->_server_hash_value_map, &map_pair );
	
	p_sub_connect_manager->_cur_res_num++;
	ret_val = list_push( &(p_sub_connect_manager->_idle_server_res_list._res_list), p_server_res );
	CHECK_VALUE( ret_val );
	LOG_DEBUG("task=0x%x [dispatch_stat] resource 0x%x add %s %d %d", p_connect_manager->_main_task_ptr, p_server_res, url, p_server_res->_resource_type, p_server_res->_level);

	gcm_add_res_num();
	return SUCCESS;
}
#ifdef ENABLE_LIXIAN
_int32 cm_add_lixian_server_resource( CONNECT_MANAGER* p_connect_manager, _u32 file_index, char *url, _u32 url_size, char* ref_url, _u32 ref_url_size, char* cookie, _u32 cookie_size,_u32 resource_priority )
{
	RESOURCE *p_server_res = NULL;
	_int32 ret_val = SUCCESS;
	enum SCHEMA_TYPE url_type;
	PAIR map_pair;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;
	_u32 hash_value = 0;

	ret_val = cm_get_sub_connect_manager( p_connect_manager, file_index, &p_sub_connect_manager );
	//sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS ) return ret_val;
	
	cm_adjust_url_format(url,url_size);
	cm_adjust_url_format(ref_url,ref_url_size);

	if( cm_is_server_res_exist( p_sub_connect_manager, url, url_size, &hash_value ) )
	{
		return CM_ADD_SERVER_PEER_EXIST;
	}

	url_type = sd_get_url_type( url, url_size);


	if(( url_type == HTTP )||( url_type == HTTPS ))
	{
		ret_val = http_resource_create( url, url_size, ref_url, ref_url_size, FALSE,&p_server_res );
		if( ret_val != SUCCESS ) return ret_val;
		p_server_res->_level = resource_priority;

		LOG_DEBUG( "http_resource_create: resource ptr = 0x%x", p_server_res );
		sd_assert( p_server_res != NULL );	
		 #ifdef ENABLE_COOKIES
		 http_resource_set_cookies((HTTP_SERVER_RESOURCE *) p_server_res,cookie, cookie_size);
		#endif
		http_resource_set_lixian((HTTP_SERVER_RESOURCE *) p_server_res,TRUE);
	}
	else if( url_type == FTP )
	{
		ret_val = ftp_resource_create( url, url_size, FALSE, &p_server_res );
		if( ret_val != SUCCESS ) return SUCCESS;
		sd_assert( p_server_res != NULL );
		LOG_DEBUG( "ftp_resource_create: resource ptr = 0x%x", p_server_res );
	}
	else
	{
		LOG_ERROR( "cm_add_server_unknown_resource" );
		return SUCCESS;
	}
	map_pair._key = (void *)hash_value;
	map_pair._value = (void *)p_server_res;
	map_insert_node( &p_sub_connect_manager->_server_hash_value_map, &map_pair );
	
	ret_val = list_push( &(p_sub_connect_manager->_cdn_res_list._res_list), p_server_res);
	if( ret_val !=SUCCESS)
	{
		if(( url_type == HTTP )||( url_type == HTTPS ))
		{
			http_resource_destroy( &p_server_res );
		}
		else if( url_type == FTP )
		{
			ftp_resource_destroy( &p_server_res );
		}
		CHECK_VALUE( ret_val );	
	}
	LOG_DEBUG("task=0x%x [dispatch_stat] resource 0x%x add %s %d %d", p_connect_manager->_main_task_ptr, p_server_res, url, p_server_res->_resource_type, p_server_res->_level);

	/* Create the pipe */
	ret_val =cm_create_single_cdn_pipe( p_sub_connect_manager, (RESOURCE *)p_server_res );
	CHECK_VALUE( ret_val );	

	return SUCCESS;
}
#endif /* ENABLE_LIXIAN */

_int32 cm_add_origin_server_resource( CONNECT_MANAGER* p_connect_manager, _u32 file_index, char *url, _u32 url_size, char* ref_url, _u32 ref_url_size, RESOURCE **pp_origin_res )
{
	RESOURCE *p_server_res = NULL;
	_int32 ret_val = SUCCESS;
	enum SCHEMA_TYPE url_type;
	PAIR map_pair;
	_u32 hash_value = 0;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;

	LOG_DEBUG( "cm_add_server_resource: url = %s, ref_url = %s", url, ref_url );
    ret_val = cm_get_sub_connect_manager( p_connect_manager, file_index, &p_sub_connect_manager );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS ) 
	{	
		LOG_DEBUG( "cm_add_server_resource: cm_get_sub_connect_manager ERROR");
		return SUCCESS;
	}
	
	cm_adjust_url_format(url,url_size);
	cm_adjust_url_format(ref_url,ref_url_size);
	
	if( cm_is_server_res_exist( p_sub_connect_manager, url, url_size, &hash_value ) )
	{
		LOG_DEBUG( "cm_add_server_resource: cm_is_server_res_exist ERROR");
		return CM_ADD_SERVER_PEER_EXIST;
	}

	url_type = sd_get_url_type( url, sd_strlen( url ) );

	if( !cm_is_enable_server_res( p_sub_connect_manager, url_type, TRUE ) ) return SUCCESS;
	
	LOG_DEBUG( "cm_add_server_resource: url = %s, ref_url = %s", url, ref_url );

#ifdef EMBED_REPORT
		p_sub_connect_manager->_report_para._res_server_total++;
#endif

	if(( url_type == HTTP )||( url_type == HTTPS ))
	{
		ret_val = http_resource_create( url, url_size, ref_url, ref_url_size, TRUE,&p_server_res );
		//sd_assert( ret_val == SUCCESS );
		if( ret_val != SUCCESS ) return ret_val;
		//CHECK_VALUE( ret_val );
		LOG_DEBUG( "http_resource_create: resource ptr = 0x%x", p_server_res );
		sd_assert( p_server_res != NULL );	
	}
	else if( url_type == FTP )
	{
		ret_val = ftp_resource_create( url, url_size, TRUE, &p_server_res );
		//sd_assert( ret_val == SUCCESS );
		//if( ret_val != SUCCESS ) return SUCCESS;
		CHECK_VALUE( ret_val );
		sd_assert( p_server_res != NULL );
		LOG_DEBUG( "ftp_resource_create: resource ptr = 0x%x", p_server_res );
	}
	else
	{
		LOG_ERROR( "cm_add_server_unknown_resource" );
		ret_val = DT_ERR_INVALID_URL;
		CHECK_VALUE( ret_val );
		
		//return SUCCESS;
	}
	map_pair._key = (void *)hash_value;
	map_pair._value = (void *)p_server_res;
	map_insert_node( &p_sub_connect_manager->_server_hash_value_map, &map_pair );
	
	p_sub_connect_manager->_cur_res_num++;
	ret_val = list_push( &(p_sub_connect_manager->_idle_server_res_list._res_list), p_server_res );
	CHECK_VALUE( ret_val );

	p_sub_connect_manager->_origin_res_ptr = p_server_res;
	*pp_origin_res = p_server_res;
	gcm_add_res_num();

	LOG_DEBUG("task=0x%x [dispatch_stat] resource 0x%x add %s %d %d", p_connect_manager->_main_task_ptr, p_server_res, url, p_server_res->_resource_type, p_server_res->_level);

	cm_create_pipes(p_connect_manager);
	return SUCCESS;
}

_int32 cm_add_active_peer_resource( CONNECT_MANAGER *p_connect_manager, _u32 file_index
    , char *peer_id, _u8 *gcid, _u64 file_size, _u32 peer_capability
    , _u32 host_ip, _u16 tcp_port, _u16 udp_port
    , _u8 res_level, _u8 res_from , _u8 res_priority)
{
	RESOURCE *p_peer_res;
	_int32 ret_val = SUCCESS;
	PAIR map_pair;
	LIST_ITERATOR cur_node = NULL;
	_u8 list_res_level = 0;
	_u8 list_res_priority = 0;
	_u32 hash_value = 0;
	_u16 list_res_level_multi_priority = 0;
	_u16 peer_level_multi_priority = res_level * res_priority;

	
	CONNECT_MANAGER *p_sub_connect_manager = NULL;
#ifdef _LOGGER
    char gcid_str[CID_SIZE*2+1];
    char addr[24] = {0};

	LOG_DEBUG("cm_add_active_peer_resource, file_index:%u, peer_id:%s, file_size:%llu, peer_capability:%u, res_from:%u.",
		file_index, peer_id, file_size, peer_capability, res_from);
#endif
#if defined(ANDROID_CDN_TEST)
	if(res_from==P2P_FROM_HUB&&is_cdn(peer_capability))
	{
		return	cm_add_cdn_peer_resource(p_connect_manager
		    , file_index,peer_id,gcid,file_size, peer_capability, host_ip,tcp_port,udp_port,  res_level,res_from);
	}
	else
	return SUCCESS;
#endif

#ifdef ANDROID_HSC_TEST
	if(res_from==P2P_FROM_VIP_HUB)
	{
#ifdef ENABLE_HSC
		return	cm_add_cdn_peer_resource(p_connect_manager,file_index,peer_id,gcid,file_size, peer_capability, host_ip,tcp_port,udp_port,  res_level,res_from);
#else
		return SUCCESS;
#endif
	}
	else
	return SUCCESS;
#endif

	if(res_from==P2P_FROM_PARTNER_CDN)
	{
#ifdef ENABLE_CDN
		return	cm_add_cdn_peer_resource(p_connect_manager,file_index,peer_id,gcid,file_size, peer_capability, host_ip,tcp_port,udp_port,  res_level,res_from);
#else
		return SUCCESS;
#endif
	}
    else
	if(res_from==P2P_FROM_VIP_HUB)
	{
#ifdef ENABLE_HSC
		return	cm_add_cdn_peer_resource(p_connect_manager,file_index,peer_id,gcid,file_size, peer_capability, host_ip,tcp_port,udp_port,  res_level,res_from);
#else
		return SUCCESS;
#endif
	}
	else
	if(is_cdn(peer_capability))
	{
		/* 来自phub的cdn资源 */
#ifdef ENABLE_CDN
		return	cm_add_cdn_peer_resource(p_connect_manager,file_index,peer_id,gcid,file_size, peer_capability, host_ip,tcp_port,udp_port,  res_level,res_from);
#endif
	}

    ret_val = cm_get_sub_connect_manager( p_connect_manager, file_index, &p_sub_connect_manager );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS ) return SUCCESS;

	if( cm_is_active_peer_res_exist( p_sub_connect_manager, peer_id, PEER_ID_SIZE, host_ip, tcp_port, &hash_value ) )
	{
	    LOG_DEBUG("cm_is_active_peer_res_exist, hash value = %d", hash_value);
		return CM_ADD_ACTIVE_PEER_EXIST;
	}
	
#ifdef _LOGGER
 //   char gcid_str[CID_SIZE*2+1]; 
  //  char addr[24] = {0};
    ret_val = str2hex((char*)gcid, CID_SIZE, gcid_str, CID_SIZE*2);
	CHECK_VALUE( ret_val );
	gcid_str[CID_SIZE*2] = '\0';

    ret_val = sd_inet_ntoa(host_ip, addr, 24);
    sd_assert( ret_val == SUCCESS ); 

	LOG_DEBUG( "cm_add_active_peer_resource: from = %u, peer_id = %s, gcid = %s, file_size = %llu, peer_ability = %u, host = %s, port = %u, udp_port:%u, hash_value = %d",
		res_from, peer_id, gcid_str, file_size, peer_capability, addr, tcp_port, udp_port, hash_value );
#endif

    if( !cm_is_enable_peer_res( p_sub_connect_manager, peer_capability ) ) 
    {
        LOG_DEBUG("cm_is_enable_peer_res is false");
        return SUCCESS;
    }
	if( !gcm_is_need_more_res() && p_sub_connect_manager->_cur_res_num > cm_min_res_num() ) 
	{
	    LOG_DEBUG("do not need more res");
	    return SUCCESS;
    }

#ifdef EMBED_REPORT
	cm_add_peer_res_report_para( p_sub_connect_manager, peer_capability, res_from);
#endif

	ret_val = p2p_resource_create( &p_peer_res, peer_id, gcid
	    , file_size, peer_capability, host_ip, tcp_port, udp_port
	    , res_level, res_from,  res_priority);
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS ) return SUCCESS;
	sd_assert( p_peer_res != NULL );
	LOG_DEBUG( "p2p_resource_create: resource ptr = 0x%x", p_peer_res );

	map_pair._key = (void *)hash_value;
	map_pair._value = (void *)p_peer_res;
	map_insert_node( &p_sub_connect_manager->_peer_hash_value_map, &map_pair );
	
	p_sub_connect_manager->_cur_res_num++;

	cur_node = LIST_BEGIN( p_sub_connect_manager->_idle_peer_res_list._res_list );
	while( cur_node != LIST_END( p_sub_connect_manager->_idle_peer_res_list._res_list ) )
	{
		RESOURCE *p_list_res = LIST_VALUE( cur_node );
        sd_assert( p_list_res->_resource_type == PEER_RESOURCE );
		list_res_level = p2p_get_res_level( p_list_res );
		list_res_priority = p2p_get_res_priority( p_list_res );
		list_res_level_multi_priority = list_res_level * list_res_priority;
		if( peer_level_multi_priority > list_res_level_multi_priority ) break;
		cur_node = LIST_NEXT( cur_node );
	}

	ret_val = list_insert( &(p_sub_connect_manager->_idle_peer_res_list._res_list), p_peer_res, cur_node );
	CHECK_VALUE( ret_val );	
	LOG_DEBUG("task=0x%x [dispatch_stat] resource 0x%x add %s %d %d", p_connect_manager->_main_task_ptr, p_peer_res, peer_id, p_peer_res->_resource_type, p_peer_res->_level);
	gcm_add_res_num();

	return SUCCESS;
}

_int32 cm_add_passive_peer( CONNECT_MANAGER* p_connect_manager, _u32 file_index, RESOURCE *p_peer_res, DATA_PIPE *p_peer_data_pipe )
{
	_int32 ret_val = SUCCESS;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;
	
	LOG_DEBUG("cm_add_passive_peer: p_connect_manager:0x%x, file_index:%u, resource_ptr = 0x%x, data_pipe_ptr = 0x%x"
        "cm_add_passive_peer: cm_is_enable_peer_download:%d, cm_is_enable_p2p_download:%d ", 
		p_connect_manager, file_index, p_peer_res, p_peer_data_pipe, cm_is_enable_peer_download(), 
        cm_is_enable_p2p_download());

	if ( !cm_is_enable_peer_download() ) return CM_ADD_PASSIVE_PEER_WRONG_TIME;
	if ( !cm_is_enable_p2p_download() ) return CM_ADD_PASSIVE_PEER_WRONG_TIME;
	if ( p_connect_manager == NULL ) return CM_LOGIC_ERROR;		

	if ( cm_is_origin_mode(p_connect_manager) ) 
	    return CM_ADD_PASSIVE_PEER_WRONG_TIME;	

#if defined(ANDROID_HSC_TEST)||defined(ANDROID_CDN_TEST)
	return SUCCESS;
#endif
    ret_val = cm_get_sub_connect_manager( p_connect_manager, file_index, &p_sub_connect_manager );

	if( ret_val != SUCCESS ) return CM_ADD_PASSIVE_PEER_WRONG_TIME;
	if( !gcm_is_need_more_res() && p_sub_connect_manager->_cur_res_num > cm_min_res_num() ) return CM_ADD_PASSIVE_PEER_WRONG_TIME;

	ret_val = cm_handle_passive_peer( p_sub_connect_manager, p_peer_res );
	if( ret_val != SUCCESS )
	{
		LOG_DEBUG( "cm_handle_passive_peer err:%u.", ret_val );
		return ret_val;
	}
	
	dp_set_pipe_interface( p_peer_data_pipe, &p_sub_connect_manager->_pipe_interface );

	p_sub_connect_manager->_cur_res_num++;
	p_sub_connect_manager->_cur_pipe_num++;
	p_peer_res->_pipe_num++;
	p_peer_data_pipe->_p_data_manager = p_sub_connect_manager->_data_manager_ptr;
	p_peer_data_pipe->_p_resource = p_peer_res;
	
	ret_val = list_push( &(p_sub_connect_manager->_using_peer_res_list._res_list), p_peer_res );
	CHECK_VALUE( ret_val );
	
	ret_val = list_push( &(p_sub_connect_manager->_working_peer_pipe_list._data_pipe_list), p_peer_data_pipe );
	CHECK_VALUE( ret_val );

	ret_val = gcm_register_pipe( p_sub_connect_manager, p_peer_data_pipe );
	CHECK_VALUE( ret_val );
    
	ret_val = gcm_register_working_pipe( p_sub_connect_manager, p_peer_data_pipe );
	CHECK_VALUE( ret_val );
	LOG_DEBUG("task=0x%x [dispatch_stat] resource 0x%x add passive_peer %d %d", p_connect_manager->_main_task_ptr, p_peer_res, p_peer_res->_resource_type, p_peer_res->_level);
	gcm_add_res_num();
	return SUCCESS;
}
/*
#ifdef UPLOAD_ENABLE

_int32 cm_notify_have_block( CONNECT_MANAGER *p_connect_manager, _u32 file_index )
{
	LIST_ITERATOR cur_node = NULL;
	DATA_PIPE *p_data_pipe = NULL;
	_int32 ret_val = SUCCESS;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;
	
	LOG_DEBUG( "cm_notify_have_block: file_index:%u", file_index );

    ret_val = cm_get_sub_connect_manager( p_connect_manager, file_index, &p_sub_connect_manager );
	//sd_assert( ret_val == SUCCESS ); //当查回来bcid时,但是这个文件没有被加速,这时ret_val会是CM_LOGIC_ERROR
	if( ret_val != SUCCESS ) return SUCCESS;
	
	cur_node = LIST_BEGIN( p_sub_connect_manager->_working_peer_pipe_list._data_pipe_list );
	while( cur_node != LIST_END( p_sub_connect_manager->_working_peer_pipe_list._data_pipe_list ) )
	{
		p_data_pipe = (DATA_PIPE *)LIST_VALUE( cur_node );
		
		LOG_DEBUG( "cm_notify_have_block: pipe_ptr:0x%x, pipe_type:%u, pipe_state:%u", 
			p_data_pipe, p_data_pipe->_data_pipe_type, p_data_pipe->_dispatch_info._pipe_state );
		if( p_data_pipe->_data_pipe_type != P2P_PIPE )
		{
			cur_node = LIST_NEXT( cur_node );
			continue;
		}
			
		if(	p_data_pipe->_dispatch_info._pipe_state == PIPE_DOWNLOADING )
		{
			LOG_DEBUG( "cm_notify_have_block: pipe_ptr:0x%x, file_index:%u", 
				p_data_pipe, file_index );
#ifdef UPLOAD_ENABLE
			ret_val = p2p_pipe_notify_range_checked( p_data_pipe );
			sd_assert( ret_val == SUCCESS );
#endif
		}
		cur_node = LIST_NEXT( cur_node );
	}

	return SUCCESS;
}
#endif
*/
#ifdef ENABLE_BT  
#ifdef ENABLE_BT_PROTOCOL

_int32 cm_add_bt_resource( CONNECT_MANAGER *p_connect_manager, _u32 host_ip, _u16 port, _u8 info_hash[20] )
{
	RESOURCE *p_bt_res;
	_int32 ret_val = SUCCESS;
	PAIR map_pair;
	_u32 hash_value = 0;

#ifdef _LOGGER
        char addr[24] = {0};

#endif
	sd_assert( p_connect_manager->_pipe_interface._pipe_interface_type == BT_PIPE_INTERFACE_TYPE
	|| p_connect_manager->_pipe_interface._pipe_interface_type == BT_MAGNET_INTERFACE_TYPE);

	if( cm_is_bt_res_exist( p_connect_manager, host_ip, port, info_hash, &hash_value ) )
	{
		return CM_ADD_BT_EXIST;
	}

#ifdef _LOGGER
	//char addr[24] = {0};
	ret_val = sd_inet_ntoa(host_ip, addr, 24);
	sd_assert( ret_val == SUCCESS ); 

 	LOG_DEBUG( "cm_add_bt_resource: host_ip = %s, port = %u", addr, port );
#endif

	if( !cm_is_enable_bt_res( p_connect_manager ) ) return SUCCESS;
	
	if( !gcm_is_need_more_res() && p_connect_manager->_cur_res_num > cm_min_res_num() ) return SUCCESS;
	
	ret_val = bt_resource_create( &p_bt_res, host_ip, port );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS ) return SUCCESS;
	sd_assert( p_bt_res != NULL );
	LOG_DEBUG( "bt_resource_create: resource ptr = 0x%x", p_bt_res );

	map_pair._key = (void *)hash_value;
	map_pair._value = (void *)p_bt_res;
	map_insert_node( &p_connect_manager->_peer_hash_value_map, &map_pair );
	
	p_connect_manager->_cur_res_num++;

	ret_val = list_push( &(p_connect_manager->_idle_peer_res_list._res_list), p_bt_res );
	CHECK_VALUE( ret_val ); 
	gcm_add_res_num();
	LOG_DEBUG("task=0x%x [dispatch_stat] resource 0x%x add bt-peer:%s:%d %d %d",p_connect_manager->_main_task_ptr, p_bt_res, addr, port, p_bt_res->_resource_type, p_bt_res->_level);

	return SUCCESS;

}


_int32 cm_notify_have_piece( CONNECT_MANAGER *p_connect_manager, _u32 piece_index )
{
	LIST_ITERATOR cur_node = NULL;
	DATA_PIPE *p_data_pipe = NULL;
	_int32 ret_val = SUCCESS;
	
	LOG_DEBUG( "cm_notify_have_piece: piece_index:%u", piece_index );
	sd_assert( p_connect_manager->_pipe_interface._pipe_interface_type == BT_PIPE_INTERFACE_TYPE );
	cur_node = LIST_BEGIN( p_connect_manager->_working_peer_pipe_list._data_pipe_list );
	while( cur_node != LIST_END( p_connect_manager->_working_peer_pipe_list._data_pipe_list ) )
	{
		p_data_pipe = (DATA_PIPE *)LIST_VALUE( cur_node );
		
		LOG_DEBUG( "cm_notify_have_piece: pipe_ptr:0x%x, pipe_type:%u, pipe_state:%u", 
			p_data_pipe, p_data_pipe->_data_pipe_type, p_data_pipe->_dispatch_info._pipe_state );
		if( p_data_pipe->_data_pipe_type != BT_PIPE )
		{
			cur_node = LIST_NEXT( cur_node );
			continue;
		}
			
		if((	p_data_pipe->_dispatch_info._pipe_state == PIPE_DOWNLOADING
			|| p_data_pipe->_dispatch_info._pipe_state == PIPE_CONNECTED 
			|| p_data_pipe->_dispatch_info._pipe_state == PIPE_CHOKED )
#ifdef UPLOAD_ENABLE
			&&(UPLOAD_FAILED_STATE!= dp_get_upload_state(p_data_pipe ))
#endif
			)
		{
			LOG_DEBUG( "cm_notify_have_piece: pipe_ptr:0x%x, piece_index:%u", 
				p_data_pipe, piece_index );
			ret_val = bt_pipe_notify_have_piece( p_data_pipe, piece_index );
			sd_assert( ret_val == SUCCESS );
		}
		cur_node = LIST_NEXT( cur_node );
	}

	cur_node = LIST_BEGIN( p_connect_manager->_connecting_peer_pipe_list._data_pipe_list );
	while( cur_node != LIST_END( p_connect_manager->_connecting_peer_pipe_list._data_pipe_list ) )
	{
		p_data_pipe = (DATA_PIPE *)LIST_VALUE( cur_node );
		
		LOG_DEBUG( "cm_notify_have_piece: pipe_ptr:0x%x, pipe_type:%u, pipe_state:%u", 
			p_data_pipe, p_data_pipe->_data_pipe_type, p_data_pipe->_dispatch_info._pipe_state );
		if( p_data_pipe->_data_pipe_type != BT_PIPE )
		{
			cur_node = LIST_NEXT( cur_node );
			continue;
		}
			
		if((	p_data_pipe->_dispatch_info._pipe_state == PIPE_DOWNLOADING
			|| p_data_pipe->_dispatch_info._pipe_state == PIPE_CONNECTED 
			|| p_data_pipe->_dispatch_info._pipe_state == PIPE_CHOKED )
#ifdef UPLOAD_ENABLE
			&&(UPLOAD_FAILED_STATE!= dp_get_upload_state(p_data_pipe ))
#endif
			)
		{
			LOG_DEBUG( "cm_notify_have_piece: pipe_ptr:0x%x, piece_index:%u", 
				p_data_pipe, piece_index );
			ret_val = bt_pipe_notify_have_piece( p_data_pipe, piece_index );
			sd_assert( ret_val == SUCCESS );
		}
		cur_node = LIST_NEXT( cur_node );
	}
	
	return SUCCESS;
}

/*-----------------------------------*/
_int32 cm_update_pipe_can_download_range( CONNECT_MANAGER *p_connect_manager)
{
	LIST_ITERATOR cur_node = NULL;
	DATA_PIPE *p_data_pipe = NULL;
	
	sd_assert( p_connect_manager->_pipe_interface._pipe_interface_type == BT_PIPE_INTERFACE_TYPE );

	cur_node = LIST_BEGIN( p_connect_manager->_working_peer_pipe_list._data_pipe_list);

	while(cur_node != LIST_END(p_connect_manager->_working_peer_pipe_list._data_pipe_list))
	{
		p_data_pipe = (DATA_PIPE*)LIST_VALUE(cur_node);

		sd_assert(p_data_pipe->_data_pipe_type == BT_PIPE);	
		LOG_DEBUG( "cm_update_pipe_can_download_range: pipe_ptr:0x%x, pipe_type:%u, pipe_state:%u", 
			p_data_pipe, p_data_pipe->_data_pipe_type, p_data_pipe->_dispatch_info._pipe_state );

		if( p_data_pipe->_data_pipe_type == BT_PIPE 
			&& p_data_pipe->_dispatch_info._pipe_state == PIPE_DOWNLOADING )
		{
			bt_pipe_update_can_download_range(p_data_pipe);
		}
		cur_node = LIST_NEXT(cur_node);
	}
	
	cur_node = LIST_BEGIN( p_connect_manager->_connecting_peer_pipe_list._data_pipe_list);
	while(cur_node != LIST_END(p_connect_manager->_connecting_peer_pipe_list._data_pipe_list))
	{
		p_data_pipe = (DATA_PIPE*)LIST_VALUE(cur_node);

		sd_assert(p_data_pipe->_data_pipe_type == BT_PIPE);	
		LOG_DEBUG( "cm_update_pipe_can_download_range: pipe_ptr:0x%x, pipe_type:%u, pipe_state:%u", 
			p_data_pipe, p_data_pipe->_data_pipe_type, p_data_pipe->_dispatch_info._pipe_state );

		if( p_data_pipe->_data_pipe_type == BT_PIPE 
			&& p_data_pipe->_dispatch_info._pipe_state == PIPE_DOWNLOADING )
		{
			bt_pipe_update_can_download_range(p_data_pipe);
		}
		cur_node = LIST_NEXT(cur_node);
	}
	return SUCCESS;
}


_u32 cm_get_sub_manager_upload_speed( CONNECT_MANAGER *p_connect_manager )
{
    _u32 cur_sub_manager_upload_speed = 0;
	MAP_ITERATOR map_node = NULL;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;
	
	sd_assert( p_connect_manager->_pipe_interface._pipe_interface_type == BT_PIPE_INTERFACE_TYPE );

	map_node = MAP_BEGIN( p_connect_manager->_sub_connect_manager_map );
	while( map_node != MAP_END( p_connect_manager->_sub_connect_manager_map ) )
	{
		p_sub_connect_manager = MAP_VALUE( map_node );
		cur_sub_manager_upload_speed += cm_get_upload_speed( p_sub_connect_manager );
		map_node = MAP_NEXT( p_connect_manager->_sub_connect_manager_map, map_node );
	}
	LOG_DEBUG( "cm_get_sub_manager_upload_speed. cur_sub_manager_upload_speed:%u.", cur_sub_manager_upload_speed );
	
	return cur_sub_manager_upload_speed;
}


#endif /* #ifdef ENABLE_BT  */
#endif 

#ifdef ENABLE_EMULE
_int32 cm_add_emule_resource(CONNECT_MANAGER* connect_manager, _u32 ip, _u16 port, _u32 server_ip, _u16 server_port)
{
    _int32 ret = SUCCESS;

#ifdef ENABLE_EMULE_PROTOCOL
    RESOURCE* res = NULL;
	PAIR map_pair;
	_u32 key = 0;
    //EMULE_RESOURCE *emule_exist_res = NULL;

#ifdef _LOGGER
        char addr[24] = {0};
        sd_inet_ntoa(ip, addr, 24);
        LOG_DEBUG( "cm_add_emule_resource: ip = %s, port = %hu, server_ip = %u, server_port = %hu", 
            addr, port, server_ip, server_port);
#endif

	sd_assert(connect_manager->_pipe_interface._pipe_interface_type == EMULE_PIPE_INTERFACE_TYPE);

	if(gcm_is_need_more_res() == FALSE)
    {
        LOG_ERROR("The task don't need more resource.");
        return SUCCESS;
    }
    
    // 由于现在etm的emule连接只是支持了emule server反连，以及高id的直连
    // 所以其他情况都认为是无效peer
    if (IS_LOWID(ip) && ((server_ip == 0) || (server_port == 0)))
    {
        LOG_ERROR("This is an invalid peer resource.");
        return SUCCESS;
    }

	if(cm_is_emule_res_exist(connect_manager, ip, port, &key))
    {
        // 如果存在的话，需要更新下这个资源上的无效参数，比如server ip和server port
//         LOG_DEBUG("there is an exist emule resource, so update it now.");
//
//         ret = map_find_node(&(connect_manager->_peer_hash_value_map), (void *)key, (void **)&emule_exist_res);
//         CHECK_VALUE(ret);
// 
//         LOG_DEBUG("exist emule res is 0x%x.", emule_exist_res);
//         if ((emule_exist_res != NULL) 
//             && ((emule_exist_res->_server_ip == 0) || (emule_exist_res->_server_port == 0)))
//         {
//             emule_exist_res->_server_ip = server_ip;
//             emule_exist_res->_server_port = server_port;
//         }        

        return ERR_RESOURCE_EXIST;
    }

	ret = emule_resource_create(&res, ip, port, server_ip, server_port);
	CHECK_VALUE(ret);

	map_pair._key = (void *)key;
	map_pair._value = (void *)res;
	map_insert_node(&connect_manager->_peer_hash_value_map, &map_pair);
	connect_manager->_cur_res_num++;
	ret = list_push(&(connect_manager->_idle_peer_res_list._res_list), res);
	CHECK_VALUE(ret); 
	LOG_DEBUG("task=0x%x [dispatch_stat] resource 0x%x add emule-peer:%s:%d %d %d", connect_manager->_main_task_ptr, res, addr, port, res->_resource_type, res->_level);
	gcm_add_res_num();
#endif

	return ret;
}
#endif

_int32 cm_pause_pipes(CONNECT_MANAGER *p_connect_manager)
{
	_int32 ret_val = SUCCESS;
	MAP_ITERATOR map_node = NULL;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;
	if(cm_is_use_global_strategy() == TRUE)
	{
		/* 如果使用了global_strategy 就不能pause，否则会引起pipe数目过多的问题 */
		return -1;
	}
	
	map_node = MAP_BEGIN( p_connect_manager->_sub_connect_manager_map );
	while( map_node != MAP_END( p_connect_manager->_sub_connect_manager_map ) )
	{
		p_sub_connect_manager = MAP_VALUE( map_node );
		ret_val = cm_pause_pipes( p_sub_connect_manager );
		sd_assert( ret_val == SUCCESS );
		map_node = MAP_NEXT( p_connect_manager->_sub_connect_manager_map, map_node );
	}

	cm_destroy_all_pipes(p_connect_manager);

	/* 把所有正在使用的资源都移到候选资源列表中,等到下一次调度再使用 */
	cm_move_using_res_to_candidate_except_res(p_connect_manager,NULL);
	
	p_connect_manager->_pause_pipes = TRUE;
	return SUCCESS;
}

_int32 cm_resume_pipes(CONNECT_MANAGER *p_connect_manager)
{
    p_connect_manager->_pause_pipes = FALSE;
    cm_create_pipes(p_connect_manager);
    return SUCCESS;
}
BOOL cm_is_pause_pipes(CONNECT_MANAGER *p_connect_manager)
{
	return p_connect_manager->_pause_pipes;
}

_int32 cm_create_sub_manager_pipes( CONNECT_MANAGER *p_connect_manager, _u32 file_index )
{
	CONNECT_MANAGER *p_sub_connect_manager = NULL;
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "cm_create_sub_manager_pipes file_index:%d", file_index );

    ret_val = cm_get_sub_connect_manager( p_connect_manager, file_index, &p_sub_connect_manager );
	if( ret_val != SUCCESS ) return SUCCESS;

	return cm_create_pipes( p_sub_connect_manager );
}

_int32 cm_inner_create_pipes( CONNECT_MANAGER *p_connect_manager )
{
    LOG_DEBUG("cm_inner_create_pipes, p_sub_connect_manager = %x", p_connect_manager);

	_int32 ret_val = SUCCESS;
    _u64 filesize = 0;
    _u32 max_working_pipe_num = CM_BASE_PIPE_NUM;
    _u32 working_pipe_num = 0;
    DATA_MANAGER* dm = NULL;
    _u32 now_time = 0;
    _u32 new_res_max_create_count = CM_MAX_CREATE_COUNT;
    _u32 real_create_count = 0;

    _u32 max_create_count = CM_MAX_CREATE_COUNT;
    _u32 new_create_use_using_pipes = 0;

    _int32 need_create_count = 0;

    _u32 total_create_count = 0;

    _u32 working_server_num = 0;
    _u32 working_peer_num = 0;
    _u32 connecting_server_num = 0;
    _u32 connecting_peer_num = 0;

    _u32 net_type = NT_WLAN;
    
    if( p_connect_manager == NULL )
    {
        sd_assert( FALSE );
        return SUCCESS;
    }

    sd_time_ms(&now_time);
    if(now_time - p_connect_manager->_last_dispatcher_time < CM_CONNECT_DISPATCH_INTERVAL)
	{
		LOG_DEBUG("can't be creat pipe for time is so short, now_time:%u, last_dispatch_time:%u.",
			now_time, p_connect_manager->_last_dispatcher_time);
        return SUCCESS;
	}

    if(is_pause_global_pipes() || cm_is_pause_pipes(p_connect_manager))
	{
		LOG_DEBUG("needn't create pipe now.");
		return SUCCESS;
	}

#ifdef ENABLE_CDN
    cm_update_cdn_pipes( p_connect_manager );
#endif

    ret_val = cm_update_connect_status( p_connect_manager );
    CHECK_VALUE( ret_val );

    cm_filter_pipes( p_connect_manager );
    CHECK_VALUE( ret_val );

	if (cm_can_close_normal_cdn_pipe(p_connect_manager) >= 0)
	{
		LOG_DEBUG("now we should close some normal cdn pipes.");
		ret_val = cm_destroy_normal_cdn_pipes(p_connect_manager);
	}

    // 开始开pipe
    sd_time_ms(&now_time);

    LOG_DEBUG( "cm_create_pipes, new server res num :  %u.",
               list_size(&p_connect_manager->_idle_server_res_list._res_list) );

    cm_create_special_pipes(p_connect_manager);

    if(p_connect_manager->_main_task_ptr)
    {
        filesize = p_connect_manager->_main_task_ptr->file_size;
    }
    if(filesize > 0)
    {
        max_working_pipe_num = filesize / (10*1024*1024) + max_working_pipe_num;
    }

    LOG_DEBUG("get_file_size = %lld, max_working_pipe_num = %d", filesize, max_working_pipe_num);
    working_server_num = list_size(&(p_connect_manager->_working_server_pipe_list._data_pipe_list));
    working_peer_num = list_size(&(p_connect_manager->_working_peer_pipe_list._data_pipe_list));
    working_pipe_num = working_server_num +working_peer_num;
    connecting_server_num = list_size(&(p_connect_manager->_connecting_server_pipe_list._data_pipe_list));
    connecting_peer_num = list_size(&(p_connect_manager->_connecting_peer_pipe_list._data_pipe_list)); 
    
    if ( ( working_pipe_num >= max_working_pipe_num/2 && 
        ( connecting_server_num/2 >= (max_working_pipe_num - working_pipe_num) || connecting_peer_num/2 >= (max_working_pipe_num - working_pipe_num) ) )
        || working_pipe_num >= max_working_pipe_num )
    {
        LOG_DEBUG("cm_create_pipes, arrive max pipe num");
        return SUCCESS;
    }

#ifdef _DEBUG
    _u32 task_max_conn_cn = max_working_pipe_num;
    _u32 task_now_inuse_conn_cn = 0;
    task_now_inuse_conn_cn += connecting_server_num;
    task_now_inuse_conn_cn += working_server_num;
    task_now_inuse_conn_cn += connecting_peer_num;
    task_now_inuse_conn_cn += working_peer_num;
    LOG_DEBUG("cm_inner_create_pipes.p_sub_connect_manager = %x, cm_create_pipes now all pipe count : %d, ws:%d, cs:%d, wp:%d, cp:%d, task max pipe count : %d"
              , p_connect_manager
              , task_now_inuse_conn_cn
              , working_server_num
              , connecting_server_num
              , working_peer_num
              , connecting_peer_num
              , max_working_pipe_num);
#endif

#if defined(MOBILE_PHONE)
    if(p_connect_manager->_main_task_ptr->_task_type == P2SP_TASK_TYPE)
    {
        P2SP_TASK* p2sp_task = (P2SP_TASK*)(p_connect_manager->_main_task_ptr);
        DATA_MANAGER* dm = NULL;
        dm = (DATA_MANAGER*)p_connect_manager->_data_manager_ptr;
        if(p2sp_task->_connect_dispatch_mode == TASK_DISPATCH_MODE_LITTLE_FILE)
        {
            _u64 filesize = 0;
            BOOL p2sp_use_multi_pipes = FALSE;
            
            filesize = dm_get_file_size(dm);
            LOG_DEBUG("now_time - p_connect_manager->_start_time = %d, dm_has_recv_data = %d, filesize = %llu"
                , now_time - p_connect_manager->_start_time
                , dm_has_recv_data(dm)
                , filesize );  

            if ( (now_time - p_connect_manager->_start_time >= 5000 && FALSE == dm_has_recv_data(dm)) 
                || filesize > p2sp_task->_filesize_to_little_file  ) 
            {
                p2sp_use_multi_pipes = TRUE;
            }

            if(p2sp_use_multi_pipes)  // 多资源下载
            {
                cm_set_small_file_mode(p_connect_manager, FALSE);  
            }
            else
            {
                cm_set_small_file_mode(p_connect_manager, TRUE);  // 小文件模式是只用单资源的模式
            }
        }
        if( cm_is_use_multires(p_connect_manager) )
        {
            dm_set_check_mode(dm, TRUE);
        }
        else
        {
            dm_set_check_mode(dm, FALSE);
        }
    }
 
 #endif

    

    /*
    1.新资源要优先打开，每个周期最多开2个新资源pipe，
    2.5s后优先从已经在使用的pipe所属资源上打开新pipe，每个周期最多2个
    3.如果第二步没有新开pipe，那么从peer和其他废弃server和重试server上开pipe
    */
    ret_val = cm_create_pipes_from_server_res_list( p_connect_manager,
              &p_connect_manager->_idle_server_res_list,
              new_res_max_create_count,
              &real_create_count );
    CHECK_VALUE( ret_val );

    LOG_DEBUG( "cm_create_pipes.cm_create_server_pipes from new res list end, created pipe num = %u.", real_create_count );

#if defined(_ANDROID_LINUX)
    net_type = sd_get_global_net_type();
    LOG_DEBUG("get_global_net_type, net_type = 0x%X", net_type);
    if( net_type == NT_3G)
    {
        LOG_DEBUG("get_global_net_type, type = 3G, so destroy not support range res");
        cm_destroy_not_support_range_speed_up_res(p_connect_manager);
    }
#endif

    if( now_time - p_connect_manager->_start_time >= 5000 || 0 == real_create_count ) 
    {
        if(cm_is_use_multires(p_connect_manager))
        {
	        // 因为只有一个资源，所以要不在idle和retry里面要不在using里面。
	        ret_val = cm_create_pipes_from_server_using_pipes(p_connect_manager
	                , max_create_count
	                , &new_create_use_using_pipes);
	        
	        LOG_DEBUG("cm_create_pipes.new_create_use_using_pipes : %d", new_create_use_using_pipes);
        }

        if(new_create_use_using_pipes == 0)
        {
            cm_get_need_create_pipes_info(p_connect_manager, &need_create_count);
            LOG_DEBUG("cm_create_pipes.get_need_create_pipes_infor, need_create_count : %d", need_create_count);
            total_create_count = cm_create_new_pipes(p_connect_manager, need_create_count);
            LOG_DEBUG("cm_create_pipes.create_pipes from other res count : %d", total_create_count);
        }
    }

#ifdef _LOGGER
    cm_print_info( p_connect_manager );
#endif

    sd_time_ms(&now_time);
    p_connect_manager->_last_dispatcher_time = now_time;

    return SUCCESS;
}

_int32 cm_create_pipes(CONNECT_MANAGER *p_connect_manager)
{
    LOG_DEBUG("p_connect_manager = %x", p_connect_manager);
	CONNECT_MANAGER *p_sub_connect_manager = NULL;
	cm_inner_create_pipes(p_connect_manager);
    MAP_ITERATOR map_node = MAP_BEGIN(p_connect_manager->_sub_connect_manager_map);
    if (map_size(&(p_connect_manager->_sub_connect_manager_map)) != 0)
    {
        while (map_node != MAP_END(p_connect_manager->_sub_connect_manager_map))
        {
            p_sub_connect_manager = MAP_VALUE(map_node);
            if (p_sub_connect_manager != NULL)
            {
                cm_inner_create_pipes( p_sub_connect_manager );
            }
            map_node = MAP_NEXT( p_connect_manager->_sub_connect_manager_map, map_node );
        }
    }
	return SUCCESS;
}

_int32 cm_get_need_create_pipes_info(CONNECT_MANAGER* p_connect_manager
                                     , _int32* need_create_num)
{
    _int32 ret = SUCCESS;

    _int32 t_num = 0;
    _u32 cur_speed = 0;

    cur_speed = p_connect_manager->_cur_server_speed + p_connect_manager->_cur_peer_speed;
    LOG_DEBUG("cm_get_need_create_pipes_info : current_task_speed %d", cur_speed);
    if(cur_speed < 1024*1024)
    {
        t_num = 5;
    }


    *need_create_num = t_num;

	TASK* p_task = p_connect_manager->_main_task_ptr;
	if(p_task && p_task->_p_assist_pipe &&
		((p_task->_p_assist_pipe->_dispatch_info._pipe_state == PIPE_DOWNLOADING)
		 /* || (p_task->_p_assist_pipe->_dispatch_info._pipe_state == PIPE_CONNECTED)*/) )
	{
		p_connect_manager->_max_pipe_num = MAX_PIPE_NUM_WHEN_ASSISTANT_ON;
	}
	else
	{
		p_connect_manager->_max_pipe_num = 0;
	}

	if(p_connect_manager->_max_pipe_num > 0 
		&& p_connect_manager->_max_pipe_num <= p_connect_manager->_cur_pipe_num)
	{
		*need_create_num = 0;
	}

    return ret;
}

_int32 cm_get_connect_manager_list(CONNECT_MANAGER *p_connect_manager, LIST *p_connect_manager_list)
{
	MAP_ITERATOR map_node = NULL;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;
	_int32 ret_val = list_push( p_connect_manager_list, (void *)p_connect_manager );
	CHECK_VALUE( ret_val );

	map_node = MAP_BEGIN( p_connect_manager->_sub_connect_manager_map );
	while( map_node != MAP_END( p_connect_manager->_sub_connect_manager_map ) )
	{
		p_sub_connect_manager = MAP_VALUE( map_node );
		ret_val = list_push( p_connect_manager_list, (void *)p_sub_connect_manager );
		CHECK_VALUE( ret_val );
		map_node = MAP_NEXT( p_connect_manager->_sub_connect_manager_map, map_node );
	}
	return SUCCESS;
}

_int32 cm_get_idle_server_res_list( CONNECT_MANAGER *p_connect_manager, RESOURCE_LIST **pp_res_list )
{
	LOG_DEBUG( "cm_get_idle_server_res_list." );
	*pp_res_list = &(p_connect_manager->_idle_server_res_list);
	return SUCCESS;
}

_int32 cm_get_using_server_res_list( CONNECT_MANAGER *p_connect_manager, RESOURCE_LIST **pp_res_list )
{
	LOG_DEBUG( "cm_get_using_server_res_list." );
	*pp_res_list = &(p_connect_manager->_using_server_res_list);
	return SUCCESS;
}

_int32 cm_get_idle_peer_res_list( CONNECT_MANAGER *p_connect_manager, RESOURCE_LIST **pp_res_list )
{
	LOG_DEBUG( "cm_get_idle_peer_res_list." );
	*pp_res_list = &(p_connect_manager->_idle_peer_res_list);
	return SUCCESS;
}

_int32 cm_get_using_peer_res_list( CONNECT_MANAGER *p_connect_manager, RESOURCE_LIST **pp_res_list )
{
	LOG_DEBUG( "cm_get_using_peer_res_list." );
	*pp_res_list = &(p_connect_manager->_using_peer_res_list);
	return SUCCESS;
}

_int32 cm_get_connecting_server_pipe_list( CONNECT_MANAGER *p_connect_manager, PIPE_LIST **pp_pipe_list )
{
	LOG_DEBUG( "cm_get_connecting_server_pipe_list." );
	*pp_pipe_list = &(p_connect_manager->_connecting_server_pipe_list);
	return SUCCESS;
}

_int32 cm_get_working_server_pipe_list( CONNECT_MANAGER *p_connect_manager, PIPE_LIST **pp_pipe_list )
{
	LOG_DEBUG( "cm_get_working_server_pipe_list." );
	*pp_pipe_list = &(p_connect_manager->_working_server_pipe_list);
	return SUCCESS;
}

_int32 cm_get_connecting_peer_pipe_list( CONNECT_MANAGER *p_connect_manager, PIPE_LIST **pp_pipe_list )
{
	LOG_DEBUG( "cm_get_connecting_peer_pipe_list." );
	*pp_pipe_list = &(p_connect_manager->_connecting_peer_pipe_list);
	return SUCCESS;
}

_int32 cm_get_working_peer_pipe_list( CONNECT_MANAGER *p_connect_manager, PIPE_LIST **pp_pipe_list )
{
	LOG_DEBUG( "cm_get_working_peer_pipe_list." );
	*pp_pipe_list = &(p_connect_manager->_working_peer_pipe_list);
	return SUCCESS;
}

_int32 cm_get_connecting_server_pipe_num( CONNECT_MANAGER *p_connect_manager)
{
    _u32 connecting_server_pipe_num = 0;
	MAP_ITERATOR map_node = NULL;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;

	connecting_server_pipe_num = list_size(&(p_connect_manager->_connecting_server_pipe_list._data_pipe_list));
	map_node = MAP_BEGIN( p_connect_manager->_sub_connect_manager_map );
	while( map_node != MAP_END( p_connect_manager->_sub_connect_manager_map ) )
	{
		p_sub_connect_manager = MAP_VALUE( map_node );
		connecting_server_pipe_num += cm_get_connecting_server_pipe_num( p_sub_connect_manager );
		map_node = MAP_NEXT( p_connect_manager->_sub_connect_manager_map, map_node );
	}
	
	return connecting_server_pipe_num;

}

_int32 cm_get_working_server_pipe_num( CONNECT_MANAGER *p_connect_manager)
{
    _u32 working_server_pipe_num = 0;
	MAP_ITERATOR map_node = NULL;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;

	working_server_pipe_num = list_size(&(p_connect_manager->_working_server_pipe_list._data_pipe_list));
	map_node = MAP_BEGIN( p_connect_manager->_sub_connect_manager_map );
	while( map_node != MAP_END( p_connect_manager->_sub_connect_manager_map ) )
	{
		p_sub_connect_manager = MAP_VALUE( map_node );
		working_server_pipe_num += cm_get_working_server_pipe_num( p_sub_connect_manager );
		map_node = MAP_NEXT( p_connect_manager->_sub_connect_manager_map, map_node );
	}
	
	return working_server_pipe_num;

}

_int32 cm_get_connecting_peer_pipe_num( CONNECT_MANAGER *p_connect_manager)
{
    _u32 connecting_peer_pipe_num = 0;
	MAP_ITERATOR map_node = NULL;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;

	connecting_peer_pipe_num = list_size(&(p_connect_manager->_connecting_peer_pipe_list._data_pipe_list));
	map_node = MAP_BEGIN( p_connect_manager->_sub_connect_manager_map );
	while( map_node != MAP_END( p_connect_manager->_sub_connect_manager_map ) )
	{
		p_sub_connect_manager = MAP_VALUE( map_node );
		connecting_peer_pipe_num += cm_get_connecting_peer_pipe_num( p_sub_connect_manager );
		map_node = MAP_NEXT( p_connect_manager->_sub_connect_manager_map, map_node );
	}
	
	return connecting_peer_pipe_num;

}

_int32 cm_get_working_peer_pipe_num( CONNECT_MANAGER *p_connect_manager )
{
    _u32 working_peer_pipe_num = 0;
	MAP_ITERATOR map_node = NULL;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;

	working_peer_pipe_num = list_size(&(p_connect_manager->_working_peer_pipe_list._data_pipe_list));
	map_node = MAP_BEGIN( p_connect_manager->_sub_connect_manager_map );
	while( map_node != MAP_END( p_connect_manager->_sub_connect_manager_map ) )
	{
		p_sub_connect_manager = MAP_VALUE( map_node );
		working_peer_pipe_num += cm_get_working_peer_pipe_num( p_sub_connect_manager );
		map_node = MAP_NEXT( p_connect_manager->_sub_connect_manager_map, map_node );
	}
	
	return working_peer_pipe_num;
}

_u32 cm_get_upload_speed( CONNECT_MANAGER *p_connect_manager )
{
    _u32 cur_upload_speed = 0;
	cur_upload_speed = p_connect_manager->_cur_peer_upload_speed;
	
	return cur_upload_speed;
}

_u32 cm_get_task_bt_download_speed( CONNECT_MANAGER *p_connect_manager )
{
    _u32 cur_bt_speed = 0;
	
	sd_assert( p_connect_manager->_pipe_interface._pipe_interface_type == BT_PIPE_INTERFACE_TYPE );
	cur_bt_speed = p_connect_manager->_cur_peer_speed;
	
	return cur_bt_speed;
}

#ifdef _CONNECT_DETAIL

void cm_get_working_peer_type_info( CONNECT_MANAGER *p_connect_manager, _u32 *p_tcp_pipe_num, _u32 *p_udp_pipe_num, _u32 *p_tcp_pipe_speed, _u32 *p_udp_pipe_speed )
{
    _u32 tcp_pipe_num = 0, udp_pipe_num = 0;
    _u32 tcp_pipe_speed = 0, udp_pipe_speed = 0;
    LIST_ITERATOR cur_node = LIST_BEGIN(p_connect_manager->_working_peer_pipe_list._data_pipe_list );
    while( cur_node != LIST_END(p_connect_manager->_working_peer_pipe_list._data_pipe_list) )
    {
        DATA_PIPE *p_data_pipe = LIST_VALUE( cur_node );
        if( p_data_pipe->_data_pipe_type == BT_PIPE )
        {
            tcp_pipe_num++;
            tcp_pipe_speed += p_data_pipe->_dispatch_info._speed;
        }
        else if( p_data_pipe->_data_pipe_type == EMULE_PIPE )
        {
            
        }
        else if( p_data_pipe->_data_pipe_type == P2P_PIPE )
        {
            if( p2p_pipe_is_transfer_by_tcp( p_data_pipe ) )
        {
            tcp_pipe_num++;
            tcp_pipe_speed += p_data_pipe->_dispatch_info._speed;
        }
        else
        {
            udp_pipe_num++;
            udp_pipe_speed += p_data_pipe->_dispatch_info._speed;            
        }
        }
        cur_node = LIST_NEXT( cur_node );
    }
    *p_tcp_pipe_num = tcp_pipe_num;
    *p_udp_pipe_num = udp_pipe_num;
    *p_tcp_pipe_speed = tcp_pipe_speed;
    *p_udp_pipe_speed = udp_pipe_speed;
    
}


_int32 cm_get_idle_server_res_num( CONNECT_MANAGER *p_connect_manager )
{
    _u32 idle_res_num = 0;
	MAP_ITERATOR map_node = NULL;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;

	idle_res_num = list_size(&(p_connect_manager->_idle_server_res_list._res_list) );
	map_node = MAP_BEGIN( p_connect_manager->_sub_connect_manager_map );
	while( map_node != MAP_END( p_connect_manager->_sub_connect_manager_map ) )
	{
		p_sub_connect_manager = MAP_VALUE( map_node );
		idle_res_num += list_size(&(p_sub_connect_manager->_idle_server_res_list._res_list) );
		map_node = MAP_NEXT( p_connect_manager->_sub_connect_manager_map, map_node );
	}
	LOG_DEBUG( "cm_get_idle_server_res_num: idle_server_res = %d ",idle_res_num );
	return idle_res_num;
}

_int32 cm_get_using_server_res_num( CONNECT_MANAGER *p_connect_manager )
{
	_u32 using_res_num = 0;
	MAP_ITERATOR map_node = NULL;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;

	using_res_num = list_size(&(p_connect_manager->_using_server_res_list._res_list) );
	map_node = MAP_BEGIN( p_connect_manager->_sub_connect_manager_map );
	while( map_node != MAP_END( p_connect_manager->_sub_connect_manager_map ) )
	{
		p_sub_connect_manager = MAP_VALUE( map_node );
		using_res_num += list_size(&(p_sub_connect_manager->_using_server_res_list._res_list) );
		map_node = MAP_NEXT( p_connect_manager->_sub_connect_manager_map, map_node );
	}
	LOG_DEBUG( "cm_get_using_server_res_num: using_server_res = %d ",using_res_num );
	return using_res_num;
}

_int32 cm_get_candidate_server_res_num( CONNECT_MANAGER *p_connect_manager )
{
	return list_size(&(p_connect_manager->_candidate_server_res_list._res_list));
}

_int32 cm_get_retry_server_res_num( CONNECT_MANAGER *p_connect_manager )
{
	return list_size(&(p_connect_manager->_retry_server_res_list._res_list));
}

_int32 cm_get_discard_server_res_num( CONNECT_MANAGER *p_connect_manager )
{
	return list_size(&(p_connect_manager->_discard_server_res_list._res_list));
}

_int32 cm_get_destroy_server_res_num( CONNECT_MANAGER *p_connect_manager )
{
	return p_connect_manager->_server_destroy_num;
}


_int32 cm_get_idle_peer_res_num( CONNECT_MANAGER *p_connect_manager )
{
	_u32 using_res_num = 0;
	MAP_ITERATOR map_node = NULL;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;

	using_res_num = list_size(&(p_connect_manager->_idle_peer_res_list._res_list) );
	map_node = MAP_BEGIN( p_connect_manager->_sub_connect_manager_map );
	while( map_node != MAP_END( p_connect_manager->_sub_connect_manager_map ) )
	{
		p_sub_connect_manager = MAP_VALUE( map_node );
		using_res_num += list_size(&(p_sub_connect_manager->_idle_peer_res_list._res_list) );
		map_node = MAP_NEXT( p_connect_manager->_sub_connect_manager_map, map_node );
	}
	LOG_DEBUG( "cm_get_idle_peer_res_num: idle_peer_res = %d ",using_res_num );
	return using_res_num;
}

_int32 cm_get_using_peer_res_num( CONNECT_MANAGER *p_connect_manager )
{
	_u32 using_res_num = 0;
	MAP_ITERATOR map_node = NULL;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;

	LOG_DEBUG("cm_get_using_peer_res_num p_connect_manager = 0x%x, _using_peer_res_list = 0x%x.", 
        p_connect_manager, &p_connect_manager->_using_peer_res_list);
	using_res_num = list_size(&(p_connect_manager->_using_peer_res_list._res_list) );
	map_node = MAP_BEGIN( p_connect_manager->_sub_connect_manager_map );
	while( map_node != MAP_END( p_connect_manager->_sub_connect_manager_map ) )
	{
		p_sub_connect_manager = MAP_VALUE( map_node );
		LOG_DEBUG("cm_get_using_peer_res_num p_sub_connect_manager = 0x%x, _using_peer_res_list = 0x%x.", 
			p_sub_connect_manager, &p_sub_connect_manager->_using_peer_res_list);
		using_res_num += list_size(&(p_sub_connect_manager->_using_peer_res_list._res_list) );
		map_node = MAP_NEXT( p_connect_manager->_sub_connect_manager_map, map_node );
	}
	LOG_DEBUG( "cm_get_using_peer_res_num: using_peer_res = %d ",using_res_num );
	return using_res_num;
}


_int32 cm_get_candidate_peer_res_num( CONNECT_MANAGER *p_connect_manager )
{
	return list_size(&(p_connect_manager->_candidate_peer_res_list._res_list));
}

_int32 cm_get_retry_peer_res_num( CONNECT_MANAGER *p_connect_manager )
{
	return list_size(&(p_connect_manager->_retry_peer_res_list._res_list));
}

_int32 cm_get_discard_peer_res_num( CONNECT_MANAGER *p_connect_manager )
{
	return list_size(&(p_connect_manager->_discard_peer_res_list._res_list));
}

_int32 cm_get_destroy_peer_res_num( CONNECT_MANAGER *p_connect_manager )
{
	return p_connect_manager->_peer_destroy_num;
}

_u32 cm_get_cm_level( CONNECT_MANAGER *p_connect_manager )
{
	return (_u32)p_connect_manager->_cm_level;
}

void *cm_get_peer_pipe_info_array( CONNECT_MANAGER *p_connect_manager )
{
    cm_update_peer_pipe_info( p_connect_manager );
    return (void *)&p_connect_manager->_peer_pipe_info;
}

void *cm_get_server_pipe_info_array( CONNECT_MANAGER *p_connect_manager )
{
    cm_update_server_pipe_info( p_connect_manager );
    return (void *)&p_connect_manager->_server_pipe_info;
}

#endif

/*
BOOL cm_get_excellent_file_suffix( CONNECT_MANAGER *p_connect_manager, char *p_str_name_buffer, _u32 buffer_len )
{
	char *file_suffix,*file_suffix_origin;
	RESOURCE_LIST *p_res_list = &p_connect_manager->_using_server_res_list;
	LIST_ITERATOR cur_node, begin_node, end_node;
	BOOL is_binary_file=FALSE;
	RESOURCE *p_res;
	
	LOG_DEBUG( "cm_get_excellent_file_suffix. res_list:0x%x.", p_res_list );

	LOG_DEBUG( " First,try to get file suffix from using resources ");
	begin_node = LIST_BEGIN( p_res_list->_res_list);
	end_node = LIST_END( p_res_list->_res_list );

	for( cur_node = begin_node; cur_node != end_node; cur_node = LIST_NEXT(cur_node) )
	{
		p_res = (RESOURCE *)LIST_VALUE( cur_node );		
		LOG_DEBUG( "cm_get_excellent_file_suffix. p_res:0x%X,type=%d.", p_res, p_res->_resource_type);
		if( p_res->_resource_type == HTTP_RESOURCE )
		{
			file_suffix=http_resource_get_file_suffix(p_res,&is_binary_file);
			if(( file_suffix!=NULL )&&(is_binary_file==TRUE))
			{
				sd_strncpy( p_str_name_buffer, file_suffix, buffer_len );
				LOG_DEBUG( "cm_get_excellent_file_suffix:%s", p_str_name_buffer );
				return TRUE;
			}
		}
		else if( p_res->_resource_type == FTP_RESOURCE )
		{
			file_suffix=ftp_resource_get_file_suffix(p_res,&is_binary_file);
			if(( file_suffix!=NULL )&&(is_binary_file==TRUE))
			{
				sd_strncpy( p_str_name_buffer, file_suffix, buffer_len );
				LOG_DEBUG( "cm_get_excellent_file_suffix:%s", p_str_name_buffer );
				return TRUE;
			}
		}
		
	}
	
	LOG_DEBUG( " Try to get file suffix from origin resource ");
	
	if(p_connect_manager->_origin_res_ptr==NULL)
	{
		LOG_DEBUG( " No origin resource,don't know what to do!  ");
		return FALSE;
	}
	
	p_res = p_connect_manager->_origin_res_ptr;
	
	if( p_res->_resource_type == HTTP_RESOURCE )
	{
		file_suffix_origin=http_resource_get_file_suffix(p_res,&is_binary_file);
		if(( file_suffix_origin!=NULL )&&(is_binary_file==TRUE))
		{
			sd_strncpy( p_str_name_buffer, file_suffix_origin, buffer_len );
			LOG_DEBUG( "cm_get_excellent_file_suffix:%s", p_str_name_buffer );
			return TRUE;
		}
	}
	else if( p_res->_resource_type == FTP_RESOURCE )
	{
		file_suffix_origin=ftp_resource_get_file_suffix(p_res,&is_binary_file);
		if(( file_suffix_origin!=NULL )&&(is_binary_file==TRUE))
		{
			sd_strncpy( p_str_name_buffer, file_suffix_origin, buffer_len );
			LOG_DEBUG( "cm_get_excellent_file_suffix:%s", p_str_name_buffer );
			return TRUE;
		}
	}
	
	LOG_DEBUG( "Can not get file suffix, use the suffix from origin resource as excellent ! ");
	if(file_suffix_origin!=NULL)
	{
		sd_strncpy( p_str_name_buffer, file_suffix_origin, buffer_len );
		LOG_DEBUG( "cm_get_excellent_file_suffix:%s", p_str_name_buffer );
		return TRUE;
	}

	LOG_DEBUG( " No file suffix ");
	return FALSE;	
}
*/

BOOL cm_get_excellent_filename( CONNECT_MANAGER *p_connect_manager, char *p_str_name_buffer, _u32 buffer_len )
{
	RESOURCE *p_res =NULL;
	RESOURCE_LIST *p_res_list = &p_connect_manager->_using_server_res_list;	
	LIST_ITERATOR cur_node, begin_node, end_node;
	
	LOG_DEBUG( "cm_get_excellent_filename. res_list:0x%x.", p_res_list );

	LOG_DEBUG( " First,try to get excellent file name from the using resources ");
	begin_node = LIST_BEGIN( p_res_list->_res_list);
	end_node = LIST_END( p_res_list->_res_list );

	for( cur_node = begin_node; cur_node != end_node; cur_node = LIST_NEXT(cur_node) )
	{
		p_res = (RESOURCE *)LIST_VALUE( cur_node );		

		LOG_DEBUG( "cm_get_excellent_filename. p_res:0x%X,type=%d.", p_res, p_res->_resource_type);
		if( p_res->_resource_type == HTTP_RESOURCE )
		{
			if( http_resource_get_file_name(p_res,p_str_name_buffer,buffer_len,FALSE)== TRUE )
			{
				return TRUE;
			}
		}
		else if( p_res->_resource_type == FTP_RESOURCE )
		{
			if( ftp_resource_get_file_name(p_res,p_str_name_buffer,buffer_len,FALSE)== TRUE )
			{
				return TRUE;
			}
		}
	}

	LOG_DEBUG( " Can not get excellent file name from using resource,try the origin resource ");
	
	if(p_connect_manager->_origin_res_ptr==NULL)
	{
		LOG_DEBUG( " No origin resource,don't know what to do!  ");
		return FALSE;
	}
	
	p_res = p_connect_manager->_origin_res_ptr;

	if( p_res->_resource_type == HTTP_RESOURCE )
	{
		return http_resource_get_file_name(p_res,p_str_name_buffer,buffer_len,FALSE);
	}
	else if( p_res->_resource_type == FTP_RESOURCE )
	{
		return ftp_resource_get_file_name(p_res,p_str_name_buffer,buffer_len,FALSE);			
	}
	
	LOG_DEBUG( " Can not get excellent file name ");
	return FALSE;
}

_u32 cm_get_current_task_speed( CONNECT_MANAGER *p_connect_manager )
{
    _u32 cur_speed = 0;
	BOOL is_server_err_get_buffer = FALSE;
	BOOL is_peer_err_get_buffer = FALSE;
	_u32 working_list_size = 0;
	
	MAP_ITERATOR map_node = NULL;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;


    cm_update_server_pipe_speed( p_connect_manager, &is_server_err_get_buffer );
    cm_update_peer_pipe_speed( p_connect_manager, &is_peer_err_get_buffer );
    
	cur_speed = p_connect_manager->_cur_server_speed + p_connect_manager->_cur_peer_speed;

	p_connect_manager->_is_err_get_buffer_status = FALSE; 

	working_list_size = list_size( &p_connect_manager->_working_server_pipe_list._data_pipe_list );
	working_list_size += list_size( &p_connect_manager->_working_peer_pipe_list._data_pipe_list );

	if( is_server_err_get_buffer 
		&& is_peer_err_get_buffer 
		&& working_list_size > 0 )
	{
		p_connect_manager->_is_err_get_buffer_status = TRUE; 
	}
	LOG_DEBUG( "cm_get_current_task_speed:%u, server_total_speed:%u, peer_total_speed:%u, _is_err_get_buffer_status:%d",
		cur_speed, p_connect_manager->_cur_server_speed, p_connect_manager->_cur_peer_speed, p_connect_manager->_is_err_get_buffer_status );

	map_node = MAP_BEGIN( p_connect_manager->_sub_connect_manager_map );
	while( map_node != MAP_END( p_connect_manager->_sub_connect_manager_map ) )
	{
		p_sub_connect_manager = MAP_VALUE( map_node );
		cur_speed += cm_get_current_task_speed( p_sub_connect_manager );
		map_node = MAP_NEXT( p_connect_manager->_sub_connect_manager_map, map_node );
	}
	
#ifdef ENABLE_CDN
	cur_speed+=cm_get_current_connect_manager_cdn_speed(p_connect_manager, TRUE);
#endif /* #ifdef ENABLE_CDN  */

	return cur_speed;
}

_u32 cm_get_current_task_server_speed( CONNECT_MANAGER *p_connect_manager )
{
    _u32 cur_server_speed = 0;
	MAP_ITERATOR map_node = NULL;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;

	cur_server_speed = p_connect_manager->_cur_server_speed;
	map_node = MAP_BEGIN( p_connect_manager->_sub_connect_manager_map );
	while( map_node != MAP_END( p_connect_manager->_sub_connect_manager_map ) )
	{
		p_sub_connect_manager = MAP_VALUE( map_node );
		cur_server_speed += cm_get_current_task_server_speed( p_sub_connect_manager );
		map_node = MAP_NEXT( p_connect_manager->_sub_connect_manager_map, map_node );
	}
	
	return cur_server_speed;
}

_u32 cm_get_current_task_peer_speed( CONNECT_MANAGER *p_connect_manager )
{
    _u32 cur_peer_speed = 0;
	MAP_ITERATOR map_node = NULL;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;

	cur_peer_speed = p_connect_manager->_cur_peer_speed;
	map_node = MAP_BEGIN( p_connect_manager->_sub_connect_manager_map );
	while( map_node != MAP_END( p_connect_manager->_sub_connect_manager_map ) )
	{
		p_sub_connect_manager = MAP_VALUE( map_node );
		cur_peer_speed += cm_get_current_task_peer_speed( p_sub_connect_manager );
		map_node = MAP_NEXT( p_connect_manager->_sub_connect_manager_map, map_node );
	}
	
	return cur_peer_speed;
}


_int32 cm_set_origin_download_mode( CONNECT_MANAGER *p_connect_manager, RESOURCE *p_orgin_res )
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "cm_set_origin_download_mode." );
	if( p_orgin_res == NULL ) return SUCCESS;
	
//	p_connect_manager->_is_only_using_origin_server = TRUE;
    cm_set_origin_mode(p_connect_manager, TRUE);
//	ret_val = cm_destroy_server_pipes_except( p_connect_manager, p_orgin_res );
//	CHECK_VALUE( ret_val );	

	ret_val = cm_discard_all_server_res_except( p_connect_manager, p_orgin_res );
	CHECK_VALUE( ret_val );		
	
//	ret_val = cm_destroy_all_peer_pipes( p_connect_manager );
//	CHECK_VALUE( ret_val );

	ret_val = cm_discard_all_peer_res( p_connect_manager );
	CHECK_VALUE( ret_val );	

	return SUCCESS;
}

_int32 cm_set_small_file_mode(CONNECT_MANAGER *p_connect_manager, BOOL is_samll_file)
{
    LOG_DEBUG( "cm_set_small_file_mode. is_small_file = %d", is_samll_file );
    DATA_MANAGER* dm = NULL;

    if(p_connect_manager->_main_task_ptr == NULL)
    {
		return -1;
    }
    else
    {
        if(is_samll_file)
        {
            p_connect_manager->_main_task_ptr->_working_mode |= S_SMALL_FILE_MODE;
        }
        else
        {
            p_connect_manager->_main_task_ptr->_working_mode &= ~S_SMALL_FILE_MODE; 
        }
    }
	return 0;
}

_int32 cm_set_origin_mode( CONNECT_MANAGER *p_connect_manager, BOOL origin_mode )
{
	LOG_DEBUG( "cm_set_origin_mode. set_mode = %d", origin_mode );
    DATA_MANAGER* dm = NULL;

    if(p_connect_manager->_main_task_ptr == NULL)
    {
       // CHECK_VALUE(1);
    }
    else
    {

        LOG_DEBUG("p_connect_manager->_main_task_ptr->_task_type = %d"
            , p_connect_manager->_main_task_ptr->_task_type );
        if(p_connect_manager->_main_task_ptr->_task_type == P2SP_TASK_TYPE)
        {
            dm = (DATA_MANAGER*)(p_connect_manager->_data_manager_ptr);
            if(dm != NULL && dm->_only_use_orgin)
            {
                LOG_DEBUG("cm_set_origin_mode, dm != NULL && dm->_only_use_orgin");
                return SUCCESS;
            }
        }
        
    	if(origin_mode)
    	{
    		 p_connect_manager->_main_task_ptr->_working_mode = 0;
    	}
		else
		{
        	 p_connect_manager->_main_task_ptr->_working_mode |=  S_USE_NORMAL_MULTI_RES; // 暂时先这样
		}
    }
    
    /*
	if(origin_mode)
	{
		p_connect_manager->_is_only_using_origin_server = TRUE;
	}
	else if(!cm_is_only_using_origin_server())
	{
		if(p_connect_manager->_is_only_using_origin_server)
		{
			p_connect_manager->_is_only_using_origin_server = FALSE;
		}
	}
	*/
	return SUCCESS;
}


BOOL cm_get_origin_mode( CONNECT_MANAGER *p_connect_manager )
{
    BOOL is_origin_mode = cm_is_origin_mode(p_connect_manager) ? TRUE : FALSE;

    LOG_DEBUG("cm_get_origin_mode origin_mode = %d", is_origin_mode);
    
    return is_origin_mode;
	//return	p_connect_manager->_is_only_using_origin_server ;
}


BOOL cm_is_need_more_server_res( CONNECT_MANAGER *p_connect_manager, _u32 file_index )
{
	if(p_connect_manager->_main_task_ptr
		&& p_connect_manager->_main_task_ptr->_use_assist_pc_res_only)
	{
		LOG_DEBUG( "cm_is_need_more_server_res: use pc assist res only");
		return FALSE;
	}
	
	BOOL is_need = FALSE;
	_int32 ret_val = SUCCESS;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;

    ret_val = cm_get_sub_connect_manager( p_connect_manager, file_index, &p_sub_connect_manager );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS ) return FALSE;
	
	is_need = !p_sub_connect_manager->_is_fast_speed 
		&& p_sub_connect_manager->_is_need_new_server_res
		&& !cm_is_origin_mode(p_connect_manager);

	LOG_DEBUG( "cm_is_need_more_server_res:%d", is_need );
	return is_need;
}

BOOL cm_is_need_more_peer_res( CONNECT_MANAGER *p_connect_manager, _u32 file_index )
{
	if(p_connect_manager->_main_task_ptr
		&& p_connect_manager->_main_task_ptr->_use_assist_pc_res_only)
	{
		LOG_DEBUG( "cm_is_need_more_peer_res: use pc assist res only");
		return FALSE;
	}
	
	BOOL is_need = FALSE;
	_int32 ret_val = SUCCESS;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;

    ret_val = cm_get_sub_connect_manager( p_connect_manager, file_index, &p_sub_connect_manager );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS ) return FALSE;
	
	is_need = !p_sub_connect_manager->_is_fast_speed 
		&& p_sub_connect_manager->_is_need_new_peer_res
#ifndef LITTLE_FILE_TEST2
		&& !cm_is_origin_mode(p_connect_manager)
#endif /* LITTLE_FILE_TEST2 */
		;
	
	LOG_DEBUG( "cm_is_need_more_peer_res:%d", is_need );
	return is_need;
}

BOOL cm_is_global_need_more_res(void)
{
	
	return gcm_is_need_more_res();
}


BOOL cm_is_idle_status( CONNECT_MANAGER *p_connect_manager, _u32 file_index )
{
	_int32 ret_val = SUCCESS;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;

    ret_val = cm_get_sub_connect_manager( p_connect_manager, file_index, &p_sub_connect_manager );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS ) return TRUE;
	LOG_DEBUG( "cm_is_idle_status:%d", p_sub_connect_manager->_is_idle_status );
	return p_sub_connect_manager->_is_idle_status;
}

void cm_set_not_idle_status( CONNECT_MANAGER *p_connect_manager, _u32 file_index )
{
	_int32 ret_val = SUCCESS;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;

    ret_val = cm_get_sub_connect_manager( p_connect_manager, file_index, &p_sub_connect_manager );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS ) return;
	LOG_DEBUG( "cm_set_not_idle_status file_index:%u", file_index );
	p_sub_connect_manager->_is_idle_status = FALSE;
	p_sub_connect_manager->_status_ticks = 0;
}

void cm_p2s_set_enable( BOOL is_enable )
{
	LOG_DEBUG( "cm_p2s_set_enable:%d", is_enable );
	cm_enable_p2s_download( is_enable );
}

void cm_p2p_set_enable( BOOL is_enable )
{	
	LOG_DEBUG( "cm_p2p_set_enable:%d", is_enable );
	cm_enable_p2p_download( is_enable );	
}

void cm_set_task_max_pipe( _u32 pipe_num )
{
	LOG_DEBUG( "cm_set_task_max_pipe:%d", pipe_num );
	
#ifdef MOBILE_PHONE
	if(sd_get_net_type()<NT_3G)
	{
		if(pipe_num>CONNECT_MANAGE_DEFAULT_MAX_CONNECTIONS_IN_GPRS) 
			pipe_num=CONNECT_MANAGE_DEFAULT_MAX_CONNECTIONS_IN_GPRS;
	}
	else
	if(pipe_num>CONNECT_MANAGE_DEFAULT_MAX_CONNECTIONS) 
		pipe_num=CONNECT_MANAGE_DEFAULT_MAX_CONNECTIONS;
#endif	
	
	if( gcm_is_global_strategy() )
	{
		gcm_set_max_pipe_num( pipe_num );
	}
	else
	{
		cm_set_settings_max_pipe( pipe_num );
	}
}

_u32 cm_get_task_max_pipe(void)
{
	LOG_DEBUG( "cm_get_task_max_pipe" );	
	if( gcm_is_global_strategy() )
	{
		return gcm_get_mak_pipe_num();
	}
	else
	{
		return cm_get_settings_max_pipe();
	}
}

_int32 cm_set_global_strategy( BOOL is_using )
{
	return gcm_set_global_strategy( is_using );
}

_int32 cm_set_global_max_pipe_num( _u32 max_pipe_num )
{
	return gcm_set_max_pipe_num( max_pipe_num );
}

_int32 cm_set_global_max_connecting_pipe_num( _u32 max_connecting_pipe_num )
{
	return gcm_set_max_connecting_pipe_num( max_connecting_pipe_num );
}

_int32 cm_do_global_connect_dispatch(void)
{
	return gcm_do_connect_dispatch();
}

_int32 cm_get_global_res_num(void)
{
	return gcm_get_res_num();
}

_int32 cm_get_global_pipe_num(void)
{
	return gcm_get_pipe_num();
}

_int32 cm_pause_all_global_pipes(void)
{
	return pause_all_global_pipes();
}
_int32 cm_resume_all_global_pipes(void)
{
	return resume_all_global_pipes();
}

_int32 cm_pause_global_pipes(void)
{
	if((sd_get_net_type()<NT_3G)||(0== vdm_get_vod_task_num ()))
	{
	return pause_global_pipes();
	}
	return SUCCESS;
}

_int32 cm_resume_global_pipes(void)
{
	return resume_global_pipes();
}


#ifdef ENABLE_CDN
BOOL cm_is_need_more_cdn_res(CONNECT_MANAGER *p_connect_manager, _u32 file_index)
{
	if(p_connect_manager->_main_task_ptr
		&& p_connect_manager->_main_task_ptr->_use_assist_pc_res_only)
	{
		LOG_DEBUG( "cm_is_need_more_cdn_res: use pc assist res only");
		return FALSE;
	}
	
	_int32 ret_val = SUCCESS,res_list_size=0,cdn_res_num=DEFAULT_CDN_RES_NUM;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;

    	ret_val = cm_get_sub_connect_manager( p_connect_manager, file_index, &p_sub_connect_manager );
	CHECK_VALUE( ret_val );
	
	settings_get_int_item( "connect_manager.cdn_res_num",(_int32*)&cdn_res_num);
	res_list_size = (_int32)list_size( &(p_sub_connect_manager->_cdn_res_list._res_list));
	
	LOG_DEBUG( "cm_is_need_more_cdn_res: file_index = %u,res_list_size=%d,max_cdn_res_num=%d",file_index, res_list_size,cdn_res_num );
	
	if(res_list_size<cdn_res_num)
	{
		return TRUE;
	}
	return FALSE;
}

_int32 cm_get_cdn_peer_count(RESOURCE_LIST * p_res_list, _u8 res_from)
{
	_int32 ret_count = 0;
	LIST_ITERATOR cur_node,end_node;
	cur_node = LIST_BEGIN(p_res_list->_res_list );
	end_node = LIST_END( p_res_list->_res_list);

	while( cur_node != end_node )
	{
		RESOURCE* p_res =(RESOURCE *) LIST_VALUE( cur_node );

		_u8 from = cm_get_resource_from(p_res) ;

		if(from == res_from)
		{
			ret_count++;
		}
		
		cur_node = LIST_NEXT( cur_node );
	}

	return ret_count;
}

_int32 cm_add_cdn_peer_resource( CONNECT_MANAGER *p_connect_manager, _u32 file_index, 
								char *peer_id, _u8 *gcid, _u64 file_size, _u32 peer_capability,  
								_u32 host_ip, _u16 tcp_port , _u16 udp_port, _u8 res_level, _u8 res_from )
{
	RESOURCE *p_peer_res=NULL;
	_u32 peer_capability_t=0;
	char peer_id_t[PEER_ID_SIZE + 1]={0};
	_int32 ret_val = SUCCESS,from_res_list_size=0,cdn_res_num=DEFAULT_CDN_RES_NUM;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;

	if(res_from == P2P_FROM_ASSIST_DOWNLOAD)
	{
		cm_set_use_pcres(p_connect_manager, TRUE);
	}
	
	if ( !cm_is_enable_peer_download() ) return SUCCESS;

	if ( !cm_is_enable_p2p_download() ) return SUCCESS;

    ret_val = cm_get_sub_connect_manager( p_connect_manager, file_index, &p_sub_connect_manager );
	//sd_assert( ret_val == SUCCESS );
    LOG_DEBUG("cm_add_cdn_peer_resource, cm_get_sub_connect_manager ret_val = %d", ret_val);
	if( ret_val != SUCCESS ) return SUCCESS;

	settings_get_int_item( "connect_manager.cdn_res_num",(_int32*)&cdn_res_num);
	from_res_list_size = cm_get_cdn_peer_count( &p_sub_connect_manager->_cdn_res_list, res_from);
	
	LOG_DEBUG( "cm_add_cdn_peer_resource: file_index = %u, res_from=%d,res_list_size=%d,max_cdn_res_num=%d",file_index,res_from, from_res_list_size,cdn_res_num );

	if(from_res_list_size>=cdn_res_num && (res_from != P2P_FROM_NORMAL_CDN))
	{
		LOG_DEBUG("cm_add_cdn_peer_resource from_res_list_size>=cdn_res_num just return...");
		return SUCCESS;
	}

	if( cm_is_cdn_peer_res_exist( p_sub_connect_manager,  host_ip, tcp_port ) )
	{
		LOG_DEBUG( "cm_add_cdn_peer_resource: this resource already exist,do not add again!" );
		return CM_ADD_ACTIVE_PEER_EXIST;
	}

	sd_memset(peer_id_t, 0,PEER_ID_SIZE+1);
	if((res_from==P2P_FROM_CDN)||(res_from==P2P_FROM_LIXIAN))
	{
		set_peer_capability(&peer_capability_t, FALSE, TRUE , FALSE, TRUE, TRUE, FALSE, FALSE);
		sd_strncpy(peer_id_t, "0000000000000000",PEER_ID_SIZE);
	}
	else
	{
		peer_capability_t = peer_capability;
		sd_strncpy(peer_id_t, peer_id,PEER_ID_SIZE);
	}

#if defined(_LOGGER)||defined(VOD_BUFFER_TESTING)
{
    char gcid_str[CID_SIZE*2+1];
    char addr[24] = {0};
    ret_val = str2hex((char*)gcid, CID_SIZE, gcid_str, CID_SIZE*2);
	CHECK_VALUE( ret_val );
	gcid_str[CID_SIZE*2] = '\0';

    ret_val = sd_inet_ntoa(host_ip, addr, 24);
    sd_assert( ret_val == SUCCESS ); 

	LOG_DEBUG( "cm_add_cdn_peer_resource: peer_id = %s, gcid = %s, file_size = %llu, peer_ability = %u, host = %s, tcp_port = %u,udp_port = %u,res_from=%d",
		peer_id_t, gcid_str, file_size, peer_capability_t, addr, tcp_port,udp_port ,res_from);
#if defined(IPAD_KANKAN)
	{
		_u32 time_stamp=0;
		sd_time_ms(&time_stamp);
	    printf("XXXXXXXX get cdn ok:%u,host = %s, tcp_port = %u,udp_port = %u\n", time_stamp,addr, tcp_port,udp_port); 
	}
#endif
}
#endif

#ifdef EMBED_REPORT
	cm_add_peer_res_report_para( p_sub_connect_manager, peer_capability_t, res_from);
#endif

	ret_val = p2p_resource_create( &p_peer_res, peer_id_t, gcid, file_size, peer_capability_t
	    , host_ip, tcp_port, udp_port, res_level, res_from,  MAX_PRIORITY);
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS ) return SUCCESS;
	sd_assert( p_peer_res != NULL );
	LOG_DEBUG( "p2p_resource_create: resource ptr = 0x%x", p_peer_res );
	
	set_resource_level( p_peer_res, MAX_RES_LEVEL);

	if (res_from == P2P_FROM_NORMAL_CDN || res_from == P2P_FROM_VIP_HUB)
	{
		// 普通cdn资源永不废弃
		set_resource_max_retry_time(p_peer_res, -1);
	}

	LOG_DEBUG("task=0x%x [dispatch_stat] resource 0x%x add %s %d %d %d", 
		p_connect_manager->_main_task_ptr, p_peer_res, peer_id, p_peer_res->_resource_type, p_peer_res->_level);
	ret_val = list_push( &(p_sub_connect_manager->_cdn_res_list._res_list), p_peer_res);
	if( ret_val !=SUCCESS)
	{
		ret_val = p2p_resource_destroy((RESOURCE **) &p_peer_res );
		CHECK_VALUE( ret_val );	
	}	

#ifdef EMBED_REPORT
    if(res_from == P2P_FROM_VIP_HUB)
    {
          from_res_list_size = cm_get_cdn_peer_count(&p_sub_connect_manager->_cdn_res_list, res_from);
    	   p_connect_manager->_report_para._cdn_res_max_num = MAX( p_connect_manager->_report_para._cdn_res_max_num, from_res_list_size );
    }
#endif

	if((p_sub_connect_manager->_cdn_mode==TRUE&&res_from==P2P_FROM_CDN)
		||(res_from==P2P_FROM_PARTNER_CDN)
		||(res_from==P2P_FROM_LIXIAN)
		||(res_from==P2P_FROM_ASSIST_DOWNLOAD)
		||(res_from == P2P_FROM_VIP_HUB && (p_sub_connect_manager->enable_high_speed_channel == TRUE))
        ||(res_from==P2P_FROM_HUB&&is_cdn(peer_capability))
		) //对于从phub返回的peer，也创建pipe jjxh added
	{
		/* Create the pipe */
		ret_val = cm_create_single_cdn_pipe(p_sub_connect_manager, p_peer_res);
		LOG_DEBUG( "Create the pipe cm_create_single_cdn_pipe: res_from = %d, ret_val = %d", res_from , ret_val);
		CHECK_VALUE( ret_val );	
	}

	// 如果其它pipe在用，立即断开
    cm_destroy_all_pipes_fileter(p_connect_manager,  cm_filter_pipes_res_exist_cdnres);
	
	return SUCCESS;
}

_int32 cm_add_cdn_server_resource( CONNECT_MANAGER* p_connect_manager, _u32 file_index, char *url, _u32 url_size, char* ref_url, _u32 ref_url_size )
{
	RESOURCE *p_server_res=NULL;
	_int32 ret_val = SUCCESS,res_list_size=0,cdn_res_num=DEFAULT_CDN_RES_NUM;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;
	PAIR map_pair;
	_u32 hash_value=0;
 	enum SCHEMA_TYPE url_type;

    ret_val = cm_get_sub_connect_manager( p_connect_manager, file_index, &p_sub_connect_manager );
	CHECK_VALUE( ret_val );	
	
	cm_adjust_url_format(url,url_size);
	cm_adjust_url_format(ref_url,ref_url_size);

	if( cm_is_server_res_exist( p_sub_connect_manager, url, url_size, &hash_value ) )
	{
		return CM_ADD_SERVER_PEER_EXIST;
	}
	
	url_type = sd_get_url_type( url, url_size);

#if !defined(MOBILE_PHONE)
	if( !cm_is_enable_server_res( p_sub_connect_manager, url_type, FALSE ) ) return SUCCESS;
#endif	

	settings_get_int_item( "connect_manager.cdn_res_num",(_int32*)&cdn_res_num);
	res_list_size = (_int32)list_size( &(p_sub_connect_manager->_cdn_res_list._res_list));
	
	LOG_DEBUG( "cm_add_cdn_server_resource: file_index = %u,res_list_size=%d,max_cdn_res_num=%d",file_index, res_list_size,cdn_res_num );

	if(res_list_size>=cdn_res_num)
	{
		return SUCCESS;
	}

	LOG_DEBUG( "cm_add_cdn_server_resource: url = %s, ref_url = %s", url, ref_url );

//#ifdef 0 //EMBED_REPORT
//	p_sub_connect_manager->_report_para._res_server_total++;
//#endif

	if(( url_type == HTTP )||( url_type == HTTPS ))
	{
		ret_val = http_resource_create( url, url_size, ref_url, ref_url_size, FALSE,&p_server_res );
		//sd_assert( ret_val == SUCCESS );
		if( ret_val != SUCCESS ) return SUCCESS;
		LOG_DEBUG( "http_resource_create: resource ptr = 0x%x", p_server_res );
		sd_assert( p_server_res != NULL );	
	}
	else if( url_type == FTP )
	{
		ret_val = ftp_resource_create( url, url_size, FALSE, &p_server_res );
		//sd_assert( ret_val == SUCCESS );
		if( ret_val != SUCCESS ) return SUCCESS;
		sd_assert( p_server_res != NULL );
		LOG_DEBUG( "ftp_resource_create: resource ptr = 0x%x", p_server_res );
	}
	else
	{
		LOG_ERROR( "cm_add_server_unknown_resource" );
		return SUCCESS;
	}
	
	set_resource_level( p_server_res, MAX_RES_LEVEL);
	LOG_DEBUG("task=0x%x [dispatch_stat] resource 0x%x add %s %d %d", p_connect_manager->_main_task_ptr ,p_server_res, url, p_server_res->_resource_type, p_server_res->_level);
	
	map_pair._key = (void *)hash_value;
	map_pair._value = (void *)p_server_res;
	map_insert_node( &p_sub_connect_manager->_server_hash_value_map, &map_pair );
	
	ret_val = list_push( &(p_sub_connect_manager->_cdn_res_list._res_list), p_server_res);
	if( ret_val !=SUCCESS)
	{
		if(( url_type == HTTP )||( url_type == HTTPS ))
		{
			http_resource_destroy( &p_server_res );
		}
		else if( url_type == FTP )
		{
			ftp_resource_destroy( &p_server_res );
		}
		CHECK_VALUE( ret_val );	
	}

	res_list_size = (_int32)list_size( &(p_sub_connect_manager->_cdn_res_list._res_list));

#ifdef EMBED_REPORT
    p_connect_manager->_report_para._cdn_res_max_num = MAX( p_connect_manager->_report_para._cdn_res_max_num, res_list_size );
#endif

	if((p_sub_connect_manager->_cdn_mode==TRUE)
#if defined(MOBILE_PHONE)
		||(NT_WLAN!=sd_get_net_type())
#endif
		)
	{
		/* Create the pipe */
		ret_val =cm_create_single_cdn_pipe( p_sub_connect_manager, (RESOURCE *)p_server_res );
		CHECK_VALUE( ret_val );	
	}
	return SUCCESS;
}

_int32 cm_set_cdn_mode(CONNECT_MANAGER *p_connect_manager, _u32 file_index,BOOL mode)
{
	_int32 ret_val = SUCCESS;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;

	LOG_ERROR( "cm_set_cdn_mode=%d.",mode );
    	ret_val = cm_get_sub_connect_manager( p_connect_manager, file_index, &p_sub_connect_manager );
	CHECK_VALUE( ret_val );	
	
	if(p_sub_connect_manager->_cdn_mode == mode) return SUCCESS;
	
	p_sub_connect_manager->_cdn_mode = mode;
	if(mode)
	{
		/* Create all the pipe of each resources */
		ret_val =cm_create_cdn_pipes( p_sub_connect_manager );
		//CHECK_VALUE( ret_val );	
	}
	else
	{
		/* Close all the pipe of each resources */
		ret_val =cm_close_all_cdn_manager_pipes( p_sub_connect_manager );
		//CHECK_VALUE( ret_val );	
	}
	return ret_val;
}

_int32 cm_get_cdn_pipe_list( CONNECT_MANAGER *p_connect_manager, PIPE_LIST **pp_pipe_list )
{
	//_int32 ret_val = SUCCESS;
	//CONNECT_MANAGER *p_sub_connect_manager = NULL;
	LOG_DEBUG( "cm_get_cdn_pipe_list." );

    	//ret_val = cm_get_sub_connect_manager( p_connect_manager, file_index, &p_sub_connect_manager );
	//CHECK_VALUE( ret_val );
	
	*pp_pipe_list = &(p_connect_manager->_cdn_pipe_list);
	return SUCCESS;
}

_u32 cm_get_cdn_speed( CONNECT_MANAGER *p_connect_manager, BOOL is_contain_normal_cdn )
{
    _u32 cur_cdn_speed = 0;
	MAP_ITERATOR map_node = NULL;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;

	cur_cdn_speed += cm_get_current_connect_manager_cdn_speed(p_connect_manager, is_contain_normal_cdn);

	map_node = MAP_BEGIN( p_connect_manager->_sub_connect_manager_map );
	while( map_node != MAP_END( p_connect_manager->_sub_connect_manager_map ) )
	{
		p_sub_connect_manager = MAP_VALUE( map_node );
		cur_cdn_speed += cm_get_cdn_speed( p_sub_connect_manager, is_contain_normal_cdn );
		map_node = MAP_NEXT( p_connect_manager->_sub_connect_manager_map, map_node );
	}
	
	LOG_DEBUG( "cm_get_cdn_speed=%u, is_contain_normal_cdn=%d", cur_cdn_speed, is_contain_normal_cdn);
	p_connect_manager->_max_cdn_speed = MAX( cur_cdn_speed, p_connect_manager->_max_cdn_speed );
	return cur_cdn_speed;
}

_u32 cm_get_current_connect_manager_cdn_speed( CONNECT_MANAGER *p_connect_manager, BOOL is_contain_normal_cdn)
{
    _u32 cur_cdn_speed = 0;

	LIST_ITERATOR p_list_node=NULL ;
	DATA_PIPE *p_cdn_pipe=NULL;

	for(p_list_node = LIST_BEGIN(p_connect_manager->_cdn_pipe_list._data_pipe_list); 
		p_list_node != LIST_END(p_connect_manager->_cdn_pipe_list._data_pipe_list); 
		p_list_node = LIST_NEXT(p_list_node))
	{
		p_cdn_pipe = (DATA_PIPE * )LIST_VALUE(p_list_node);
		if(p_cdn_pipe->_dispatch_info._pipe_state==PIPE_DOWNLOADING)
		{
			if (is_contain_normal_cdn == FALSE)
			{
				if (p_cdn_pipe->_p_resource->_resource_type == PEER_RESOURCE)
				{
					if (p2p_get_from(p_cdn_pipe->_p_resource) == P2P_FROM_NORMAL_CDN)
					{
						LOG_DEBUG("such resource is p2p resource, so ignore it.");
						continue;
					}
				}
			}
			cm_update_data_pipe_speed( p_cdn_pipe );
			cur_cdn_speed += p_cdn_pipe->_dispatch_info._speed;
		}
		else
		{
			p_cdn_pipe->_dispatch_info._speed = 0;
		}
	}

	p_connect_manager->_max_cdn_speed = MAX( cur_cdn_speed, p_connect_manager->_max_cdn_speed );
	LOG_DEBUG( "cm_get_current_connect_manager_cdn_speed=%u, max_cdn_speed:%u, is_contain_normal_cdn:%d" ,
		cur_cdn_speed, p_connect_manager->_max_cdn_speed, is_contain_normal_cdn );

	return cur_cdn_speed;
}

#ifdef ENABLE_HSC
_u32 cm_get_vip_cdn_speed( CONNECT_MANAGER *p_connect_manager )
{
    _u32 cur_vip_cdn_speed = 0;
	MAP_ITERATOR map_node = NULL;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;

	cur_vip_cdn_speed += cm_get_current_connect_manager_vip_cdn_speed( p_connect_manager );

	map_node = MAP_BEGIN( p_connect_manager->_sub_connect_manager_map );
	while( map_node != MAP_END( p_connect_manager->_sub_connect_manager_map ) )
	{
		p_sub_connect_manager = MAP_VALUE( map_node );
		cur_vip_cdn_speed += cm_get_vip_cdn_speed( p_sub_connect_manager );
		map_node = MAP_NEXT( p_connect_manager->_sub_connect_manager_map, map_node );
	}
	
	LOG_DEBUG( "cm_get_vip_cdn_speed=%u" ,cur_vip_cdn_speed);
	p_connect_manager->_max_cdn_speed = MAX( cur_vip_cdn_speed, p_connect_manager->_max_cdn_speed );
	return cur_vip_cdn_speed;
}


_u32 cm_get_current_connect_manager_vip_cdn_speed( CONNECT_MANAGER *p_connect_manager )
{
    _u32 cur_vip_cdn_speed = 0;

	LIST_ITERATOR p_list_node=NULL ;
	DATA_PIPE *p_cdn_pipe=NULL;

	for(p_list_node = LIST_BEGIN(p_connect_manager->_cdn_pipe_list._data_pipe_list); p_list_node != LIST_END(p_connect_manager->_cdn_pipe_list._data_pipe_list); p_list_node = LIST_NEXT(p_list_node))
	{
		p_cdn_pipe = (DATA_PIPE * )LIST_VALUE(p_list_node);
		if(p_cdn_pipe->_p_resource->_resource_type != PEER_RESOURCE
			|| p2p_get_from(p_cdn_pipe->_p_resource) != P2P_FROM_VIP_HUB)
		{
			continue;
		}
		if(p_cdn_pipe->_dispatch_info._pipe_state==PIPE_DOWNLOADING)
		{
			cm_update_data_pipe_speed( p_cdn_pipe );
			cur_vip_cdn_speed+=p_cdn_pipe->_dispatch_info._speed;
		}
		else
		{
			p_cdn_pipe->_dispatch_info._speed = 0;
		}
	}

	p_connect_manager->_max_cdn_speed = MAX( cur_vip_cdn_speed, p_connect_manager->_max_cdn_speed );
	LOG_DEBUG( "cm_get_current_connect_manager_vip_cdn_speed=%u, max_cdn_speed:%u" ,
		cur_vip_cdn_speed, p_connect_manager->_max_cdn_speed );

	return cur_vip_cdn_speed;
}
#endif /* ENABLE_HSC */
#ifdef ENABLE_LIXIAN
_u32 cm_get_current_connect_manager_lixian_speed( CONNECT_MANAGER *p_connect_manager )
{
    _u32 cur_lixian_speed = 0;

	LIST_ITERATOR p_list_node=NULL ;
	DATA_PIPE *p_cdn_pipe=NULL;

	for(p_list_node = LIST_BEGIN(p_connect_manager->_cdn_pipe_list._data_pipe_list)
        ; p_list_node != LIST_END(p_connect_manager->_cdn_pipe_list._data_pipe_list)
        ; p_list_node = LIST_NEXT(p_list_node))
	{
		p_cdn_pipe = (DATA_PIPE * )LIST_VALUE(p_list_node);

		if(((p_cdn_pipe->_p_resource->_resource_type == PEER_RESOURCE)&&(p2p_get_from(p_cdn_pipe->_p_resource) == P2P_FROM_LIXIAN))
            || ((p_cdn_pipe->_p_resource->_resource_type == HTTP_RESOURCE)&&http_resource_is_lixian((HTTP_SERVER_RESOURCE *)p_cdn_pipe->_p_resource) ))
		{
			if(p_cdn_pipe->_dispatch_info._pipe_state==PIPE_DOWNLOADING)
			{
				cm_update_data_pipe_speed( p_cdn_pipe );
				cur_lixian_speed+=p_cdn_pipe->_dispatch_info._speed;
			}
			else
			{
				p_cdn_pipe->_dispatch_info._speed = 0;
			}
		}
	}

	p_connect_manager->_max_cdn_speed = MAX( cur_lixian_speed, p_connect_manager->_max_cdn_speed );
	LOG_DEBUG( "cm_get_current_connect_manager_lixian_speed=%u, max_cdn_speed:%u" ,
		cur_lixian_speed, p_connect_manager->_max_cdn_speed );

	return cur_lixian_speed;
}
_u32 cm_get_current_connect_manager_lixian_res_num( CONNECT_MANAGER *p_connect_manager )
{
	LIST_ITERATOR cur_node, end_node;
	RESOURCE *p_res=NULL;
	_u32 lixian_res_num = 0;	//资源个数



	cur_node = LIST_BEGIN( p_connect_manager->_cdn_res_list._res_list );
	end_node = LIST_END( p_connect_manager->_cdn_res_list._res_list);

	while( cur_node != end_node )
	{
		p_res =(RESOURCE *) LIST_VALUE( cur_node );
		if(((p_res->_resource_type == PEER_RESOURCE)&&(p2p_get_from(p_res) == P2P_FROM_LIXIAN))||
			((p_res->_resource_type == HTTP_RESOURCE)&&http_resource_is_lixian((HTTP_SERVER_RESOURCE *)p_res) ))
		{
			lixian_res_num++;
		}

		cur_node = LIST_NEXT( cur_node );
	}	
	
	return lixian_res_num;
}

_u32 cm_get_lixian_info(CONNECT_MANAGER *p_connect_manager, _u32 file_index,LIXIAN_INFO *p_lx_info)
{
	_int32 ret_val = SUCCESS;
	//CONNECT_MANAGER *p_sub_connect_manager = NULL;

 //   	ret_val = cm_get_sub_connect_manager( p_connect_manager, file_index, &p_sub_connect_manager );
	////sd_assert(ret_val == SUCCESS);
	//if( ret_val == SUCCESS )
	//{
	//	p_lx_info->_state = LS_RUNNING;
	//	p_lx_info->_res_num = cm_get_current_connect_manager_lixian_res_num(p_sub_connect_manager );
	//	p_lx_info->_speed= cm_get_current_connect_manager_lixian_speed( p_sub_connect_manager );
	//}
	
    p_lx_info->_state = LS_RUNNING;
    MAP_ITERATOR map_node = NULL;
    CONNECT_MANAGER *p_sub_connect_manager = NULL;

    p_lx_info->_speed += cm_get_current_connect_manager_lixian_speed( p_connect_manager );
    p_lx_info->_res_num += cm_get_current_connect_manager_lixian_res_num(p_connect_manager );

    map_node = MAP_BEGIN( p_connect_manager->_sub_connect_manager_map );
    while( map_node != MAP_END( p_connect_manager->_sub_connect_manager_map ) )
    {
        p_sub_connect_manager = MAP_VALUE( map_node );
        p_lx_info->_speed += cm_get_current_connect_manager_lixian_speed( p_sub_connect_manager );
        p_lx_info->_res_num += cm_get_current_connect_manager_lixian_res_num(p_sub_connect_manager );
        map_node = MAP_NEXT( p_connect_manager->_sub_connect_manager_map, map_node );
    }


	return ret_val;
}


_u32 cm_get_lixian_speed(CONNECT_MANAGER *p_connect_manager)
{
    _u32 cur_lixian_speed = 0;
	MAP_ITERATOR map_node = NULL;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;

	cur_lixian_speed += cm_get_current_connect_manager_lixian_speed( p_connect_manager );

	map_node = MAP_BEGIN( p_connect_manager->_sub_connect_manager_map );
	while( map_node != MAP_END( p_connect_manager->_sub_connect_manager_map ) )
	{
		p_sub_connect_manager = MAP_VALUE( map_node );
		cur_lixian_speed += cm_get_lixian_speed( p_sub_connect_manager );
		map_node = MAP_NEXT( p_connect_manager->_sub_connect_manager_map, map_node );
	}
	
	LOG_DEBUG( "cm_get_lixian_speed=%u" ,cur_lixian_speed);

	return cur_lixian_speed;
}
#endif /* ENABLE_LIXIAN */
_int32 cm_get_working_cdn_pipe_num( CONNECT_MANAGER *p_connect_manager)
{
    _u32 working_pipe_num = 0;
	MAP_ITERATOR map_node = NULL;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;

	working_pipe_num = list_size(&(p_connect_manager->_cdn_pipe_list._data_pipe_list));
	map_node = MAP_BEGIN( p_connect_manager->_sub_connect_manager_map );
	while( map_node != MAP_END( p_connect_manager->_sub_connect_manager_map ) )
	{
		p_sub_connect_manager = MAP_VALUE( map_node );
		working_pipe_num += cm_get_working_cdn_pipe_num( p_sub_connect_manager );
		map_node = MAP_NEXT( p_connect_manager->_sub_connect_manager_map, map_node );
	}
	
	return working_pipe_num;
}

_int32 cm_get_cdn_res_num( CONNECT_MANAGER *p_connect_manager)
{
    _u32 res_num = 0;
	MAP_ITERATOR map_node = NULL;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;

	res_num = list_size(&(p_connect_manager->_cdn_res_list._res_list));
	map_node = MAP_BEGIN( p_connect_manager->_sub_connect_manager_map );
	while( map_node != MAP_END( p_connect_manager->_sub_connect_manager_map ) )
	{
		p_sub_connect_manager = MAP_VALUE( map_node );
		res_num += cm_get_cdn_res_num( p_sub_connect_manager );
		map_node = MAP_NEXT( p_connect_manager->_sub_connect_manager_map, map_node );
	}
	
	return res_num;

}

#ifdef ENABLE_HSC
_int32 cm_enable_high_speed_channel(CONNECT_MANAGER * p_connect_manager, _u32 file_index, _int32 value)
{
    LOG_DEBUG("file_index:%u", file_index);
    _u32 sub_file_index = MAX_U32;
    CONNECT_MANAGER *p_sub_connection_manager = NULL;
    MAP_ITERATOR map_node = MAP_BEGIN(p_connect_manager->_sub_connect_manager_map);
    p_connect_manager->enable_high_speed_channel = value;
    while (map_node != MAP_END(p_connect_manager->_sub_connect_manager_map))
    {
        sub_file_index = (_u32)MAP_KEY(map_node);
        if (sub_file_index == file_index)
        {
            p_sub_connection_manager = MAP_VALUE(map_node);
            cm_enable_high_speed_channel(p_sub_connection_manager, file_index, value);
        }
		map_node = MAP_NEXT(p_connect_manager->_sub_connect_manager_map, map_node);
	}

	return SUCCESS;
}

_int32 cm_disable_high_speed_channel(CONNECT_MANAGER * p_connect_manager)
{
	CONNECT_MANAGER *p_sub_connection_manager = NULL;
	MAP_ITERATOR map_node = NULL;
	p_connect_manager->enable_high_speed_channel = FALSE;
	LOG_DEBUG("cm_disable_high_speed_channel");

	map_node = MAP_BEGIN(p_connect_manager->_sub_connect_manager_map);
	while(map_node != MAP_END(p_connect_manager->_sub_connect_manager_map))
	{
		p_sub_connection_manager = MAP_VALUE(map_node);
		cm_disable_high_speed_channel(p_sub_connection_manager);
		map_node = MAP_NEXT(p_connect_manager->_sub_connect_manager_map, map_node);
	}

	return SUCCESS;
}
#endif  /* ENABLE_HSC */
#endif /* ENABLE_CDN */

#ifdef EMBED_REPORT
struct tagCM_REPORT_PARA *cm_get_report_para( CONNECT_MANAGER *p_connect_manager, _u32 file_index )
{
	_int32 ret_val = SUCCESS;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;
	LOG_DEBUG( "cm_get_report_para, file_index:%u.",file_index );

    ret_val = cm_get_sub_connect_manager( p_connect_manager, file_index, &p_sub_connect_manager );
	if( ret_val != SUCCESS ) return NULL;
	
	return &p_sub_connect_manager->_report_para;
}

#endif

_int32 cm_get_bt_acc_file_idx_list(CONNECT_MANAGER *p_connect_manager, LIST *p_list)
{
	_int32 ret = SUCCESS;
	_u32 file_idx = 0;
	CONNECT_MANAGER *p_sub_cm = NULL;
	MAP_ITERATOR map_cur = NULL, map_end = NULL;

	map_cur = MAP_BEGIN(p_connect_manager->_sub_connect_manager_map);
	map_end = MAP_END(p_connect_manager->_sub_connect_manager_map);
	while(map_cur != map_end)
	{
		file_idx = (_u32)MAP_KEY(map_cur);
		ret = list_push(p_list, (void*)file_idx);
		CHECK_VALUE(ret);
		p_sub_cm = (CONNECT_MANAGER*)MAP_VALUE(map_cur);
		ret = cm_get_bt_acc_file_idx_list(p_sub_cm, p_list);
		CHECK_VALUE(ret);
		map_cur = MAP_NEXT(p_connect_manager->_sub_connect_manager_map, map_cur);
	}
	return ret;
}


_u32 cm_get_origin_resource_speed(CONNECT_MANAGER *p_connect_manager)
{
	_u32 cur_origin_speed = 0;

	LIST_ITERATOR p_list_node=NULL ;
	DATA_PIPE *p_server_pipe=NULL;

	for(p_list_node = LIST_BEGIN(p_connect_manager->_working_server_pipe_list._data_pipe_list); p_list_node != LIST_END(p_connect_manager->_working_server_pipe_list._data_pipe_list); p_list_node = LIST_NEXT(p_list_node))
	{
		if( NULL == p_list_node )
			return 0;
		p_server_pipe = (DATA_PIPE * )LIST_VALUE(p_list_node);
		LOG_DEBUG( "cm_get_origin_resource_speed: p_connect_manager->_origin_res_ptr=0x%x, p_server_pipe->_p_resource:0x%x" ,
		p_connect_manager->_origin_res_ptr, p_server_pipe->_p_resource );

		if(p_connect_manager->_origin_res_ptr!=NULL&&p_connect_manager->_origin_res_ptr==p_server_pipe->_p_resource)
		{
			if(p_server_pipe->_dispatch_info._pipe_state == PIPE_DOWNLOADING)
			{
				cm_update_data_pipe_speed( p_server_pipe );
				cur_origin_speed += p_server_pipe->_dispatch_info._speed;
			}
			else
			{
				p_server_pipe->_dispatch_info._speed = 0;
			}
		}
	}
	p_connect_manager->_max_cdn_speed = MAX( cur_origin_speed, p_connect_manager->_max_cdn_speed );

	LOG_DEBUG( "cm_get_current_connect_manager_assist_peer_speed=%u, max_cdn_speed:%u" ,
		cur_origin_speed, p_connect_manager->_max_cdn_speed );

	return cur_origin_speed;

}


_u32 cm_get_current_connect_manager_assist_peer_speed( CONNECT_MANAGER *p_connect_manager )
{
    _u32 cur_lixian_speed = 0;

	LIST_ITERATOR p_list_node=NULL ;
	DATA_PIPE *p_cdn_pipe=NULL;

	for(p_list_node = LIST_BEGIN(p_connect_manager->_cdn_pipe_list._data_pipe_list); p_list_node != LIST_END(p_connect_manager->_cdn_pipe_list._data_pipe_list); p_list_node = LIST_NEXT(p_list_node))
	{
		p_cdn_pipe = (DATA_PIPE * )LIST_VALUE(p_list_node);

		if((p_cdn_pipe->_p_resource->_resource_type == PEER_RESOURCE)&&(p2p_get_from(p_cdn_pipe->_p_resource) == P2P_FROM_ASSIST_DOWNLOAD))
		{
			if(p_cdn_pipe->_dispatch_info._pipe_state == PIPE_DOWNLOADING)
			{
				cm_update_data_pipe_speed( p_cdn_pipe );
				cur_lixian_speed += p_cdn_pipe->_dispatch_info._speed;
			}
			else
			{
				p_cdn_pipe->_dispatch_info._speed = 0;
			}
		}
	}
	p_connect_manager->_max_cdn_speed = MAX( cur_lixian_speed, p_connect_manager->_max_cdn_speed );

	LOG_DEBUG( "cm_get_current_connect_manager_assist_peer_speed=%u, max_cdn_speed:%u" ,
		cur_lixian_speed, p_connect_manager->_max_cdn_speed );

	return cur_lixian_speed;
}
_u32 cm_get_current_connect_manager_assist_peer_res_num( CONNECT_MANAGER *p_connect_manager )
{
	LIST_ITERATOR cur_node, end_node;
	RESOURCE *p_res=NULL;
	_u32 assist_peer_res_num = 0;	//资源个数

	cur_node = LIST_BEGIN( p_connect_manager->_cdn_res_list._res_list );
	end_node = LIST_END( p_connect_manager->_cdn_res_list._res_list);

	while( cur_node != end_node )
	{
		p_res =(RESOURCE *) LIST_VALUE( cur_node );
		if((p_res->_resource_type == PEER_RESOURCE)&&(p2p_get_from(p_res) == P2P_FROM_ASSIST_DOWNLOAD))
		{
			assist_peer_res_num++;
		}

		cur_node = LIST_NEXT( cur_node );
	}	
	
	return assist_peer_res_num;
}

_u32 cm_get_assist_peer_info(CONNECT_MANAGER *p_connect_manager, _u32 file_index,ASSIST_PEER_INFO *p_lx_info)
{
	_int32 ret_val = SUCCESS;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;

    ret_val = cm_get_sub_connect_manager( p_connect_manager, file_index, &p_sub_connect_manager );
	if( ret_val == SUCCESS )
	{
		p_lx_info->_res_num = cm_get_current_connect_manager_assist_peer_res_num(p_sub_connect_manager );
		p_lx_info->_speed= cm_get_current_connect_manager_assist_peer_speed( p_sub_connect_manager );
	}
	
	return ret_val;
}



BOOL cm_is_p2sptask(CONNECT_MANAGER * p_connect_manager)
{
	BOOL ret = FALSE;

	if(p_connect_manager->_main_task_ptr != NULL && p_connect_manager->_main_task_ptr->_task_type == P2SP_TASK_TYPE)
	{
		ret = TRUE;
	}

	return ret;	
}

BOOL cm_is_bttask(CONNECT_MANAGER * p_connect_manager)
{
	BOOL ret = FALSE;

	if(p_connect_manager->_main_task_ptr != NULL && p_connect_manager->_main_task_ptr->_task_type == BT_TASK_TYPE)
	{
		ret = TRUE;
	}

	return ret;
}

BOOL cm_shubinfo_valid(CONNECT_MANAGER * p_connect_manager)
{
	BOOL ret = FALSE;

	if(p_connect_manager->_main_task_ptr != NULL && p_connect_manager->_main_task_ptr->_task_type == P2SP_TASK_TYPE)
	{
		P2SP_TASK* p2sp_task = (P2SP_TASK*)p_connect_manager->_main_task_ptr;

		if(file_info_gcid_is_valid(&p2sp_task->_data_manager._file_info)  && file_info_bcid_valid(&p2sp_task->_data_manager._file_info))
		{
			ret = TRUE;
		}
	}	

	return ret;
}

_int32 cm_can_open_normal_cdn_pipe(CONNECT_MANAGER *conn_manager)
{
	_int32 ret = SUCCESS;
	_u32 now_time = 0;
	_u32 cur_task_spd = 0;
	TASK *task = conn_manager->_main_task_ptr;
	_u32 default_task_start_span = 5, default_from_last_open_span = 10;
	_u32 default_min_speed = 5;

	settings_get_int_item("speedup_cdn_controller.start_interval", (_int32*)&default_task_start_span);
	settings_get_int_item("speedup_cdn_controller.open_interval", (_int32*)&default_from_last_open_span);
	settings_get_int_item("speedup_cdn_controller.high_speed", (_int32*)&default_min_speed);

	LOG_DEBUG("getting default value, speedup_cdn_controller.start_interval:%u, "
		"speedup_cdn_controller.open_interval:%u, speedup_cdn_controller.high_speed:%u."
		, default_task_start_span, default_from_last_open_span, default_min_speed);

#ifdef EMBED_REPORT	
	if (conn_manager->_report_para._res_normal_cdn_start_time != 0)
	{
		LOG_DEBUG("such connect manager have normal cdn pipe already, so return.");
		return -4;
	}
#endif

	// 1、满足开启时长超过5s
	ret = sd_time(&now_time);
	LOG_DEBUG("cm_dispatch_normal_cdn, now:%u, start task time:%u.", now_time, task->_start_time);
	if (now_time - task->_start_time < default_task_start_span) 
	{
		// 因为任务开始时长不够，不必开启norml cdn调度
		LOG_DEBUG("cm_dispatch_normal_cdn, task has started less than %u s.", 
			default_task_start_span);
		return -1;
	}

#ifdef EMBED_REPORT
	// 2、距离上一次normal cdn的时长需要超过10s
	// 这个策略感觉值得商榷，用意是为了避免开的太频繁，是不是只要避免关的时候太频繁就好了？
	LOG_DEBUG("conn_manager->_report_para._res_normal_cdn_close_time:%u.", 
		conn_manager->_report_para._res_normal_cdn_close_time);
	if ((conn_manager->_report_para._res_normal_cdn_close_time != 0) && 
		(now_time - conn_manager->_report_para._res_normal_cdn_close_time < default_from_last_open_span))
	{
		// 因为距离上次关闭时长太小而不必开启normal cdn调度
		LOG_DEBUG("cm_dispatch_normal_cdn, task has opened normal cdn pipe less than %u s.", 
			default_from_last_open_span);
		return -2;
	}
#endif

	// 3、非普通cdn提供的速度有多少
	// 如果超过了5k，则不开启，否则开启
	cur_task_spd = cm_get_current_task_speed_without_normal_cdn(conn_manager);
	LOG_DEBUG("cm_dispatch_normal_cdn, current task speed:%u.", cur_task_spd);
	
	if (cur_task_spd >= default_min_speed*1024)
	{
		// 因为任务的速度超过5k，不必开启normal cdn调度
		LOG_DEBUG("cm_dispatch_normal_cdn, task speed without normal cdn pipe over than %u k.", 
			default_min_speed);
		return -3;
	}

#ifdef EMBED_REPORT
	// 满足各种条件后，可以在normal cdn上开启链接
	LOG_DEBUG("start to open normal cdn pipe, sum_use_time:%u, last open time:%u, last close time:%u, can open.",
		conn_manager->_report_para._res_normal_cdn_sum_use_time,
		conn_manager->_report_para._res_normal_cdn_start_time,
		conn_manager->_report_para._res_normal_cdn_close_time);
#endif

	return SUCCESS;
}

_int32 cm_can_close_normal_cdn_pipe(CONNECT_MANAGER *conn_manager)
{
	_int32 ret = 0;
	_u32 cur_task_spd = 0;
	_u32 now_time = 0;
	_u32 default_task_start_span = 5, default_from_last_open_span = 10;
	_u32 default_min_speed = 5;

#ifdef EMBED_REPORT
	if (conn_manager->_report_para._res_normal_cdn_start_time == 0) {
		LOG_ERROR("conn_manager->_report_para._res_normal_cdn_start_time == 0");
		return -4;
	}
#endif

	settings_get_int_item("speedup_cdn_controller.start_interval", (_int32*)&default_task_start_span);
	settings_get_int_item("speedup_cdn_controller.open_interval", (_int32*)&default_from_last_open_span);
	settings_get_int_item("speedup_cdn_controller.high_speed", (_int32*)&default_min_speed);

	// 1、开启时间小于10s时长，为了避免不断的开关，小于10s不再关闭
	ret = sd_time(&now_time);

#ifdef EMBED_REPORT
	LOG_DEBUG("cm_can_close_normal_cdn_pipe, now:%u, last_open_normal_cdn_pipe:%u.",
		now_time, conn_manager->_report_para._res_normal_cdn_start_time);
	if (now_time - conn_manager->_report_para._res_normal_cdn_start_time < default_from_last_open_span) 
	{
		// normal cdn开启的时长不够，不必开启norml cdn调度
		LOG_DEBUG("cm_can_close_normal_cdn_pipe, task has started less than %u s.", default_from_last_open_span);
		return -1;
	}
#endif

	// 2、速度小于5k的话，不能关闭
	cur_task_spd = cm_get_current_task_speed_without_normal_cdn(conn_manager);
	LOG_DEBUG("cm_can_close_normal_cdn_pipe, cur_task_spd:%u.", cur_task_spd);

	if (cur_task_spd < default_min_speed*1024)
	{
		// 速度不足5k，不必关闭
		LOG_DEBUG("cm_can_close_normal_cdn_pipe, cur task sped less than %u k", default_min_speed);
		return -2;	
	}

#ifdef EMBED_REPORT
	LOG_DEBUG("start to close normal cdn pipe, sum_use_time:%u, last open time:%u, last close time:%u, can close.",
		conn_manager->_report_para._res_normal_cdn_sum_use_time,
		conn_manager->_report_para._res_normal_cdn_start_time,
		conn_manager->_report_para._res_normal_cdn_close_time);
#endif

	return SUCCESS;
}

_u32 cm_get_normal_cdn_speed(CONNECT_MANAGER *p_connect_manager)
{
	_u32 cur_normal_cdn_speed = 0;
	MAP_ITERATOR map_node = NULL;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;

	cur_normal_cdn_speed += cm_get_current_connect_manager_normal_cdn_speed( p_connect_manager );

	map_node = MAP_BEGIN( p_connect_manager->_sub_connect_manager_map );
	while( map_node != MAP_END( p_connect_manager->_sub_connect_manager_map ) )
	{
		p_sub_connect_manager = MAP_VALUE( map_node );
		cur_normal_cdn_speed += cm_get_normal_cdn_speed( p_sub_connect_manager );
		map_node = MAP_NEXT( p_connect_manager->_sub_connect_manager_map, map_node );
	}

	LOG_DEBUG( "connect manager:0x%x, normal cdn speed:%u", p_connect_manager, cur_normal_cdn_speed);

	p_connect_manager->_max_cdn_speed = MAX( cur_normal_cdn_speed, p_connect_manager->_max_cdn_speed );

	return cur_normal_cdn_speed;
}

_u32 cm_get_current_connect_manager_normal_cdn_speed(CONNECT_MANAGER *p_connect_manager)
{
	_u32 cur_normal_cdn_speed = 0;

	LIST_ITERATOR p_list_node=NULL ;
	DATA_PIPE *p_cdn_pipe=NULL;

	for(p_list_node = LIST_BEGIN(p_connect_manager->_cdn_pipe_list._data_pipe_list); 
		p_list_node != LIST_END(p_connect_manager->_cdn_pipe_list._data_pipe_list); 
		p_list_node = LIST_NEXT(p_list_node))
	{
		p_cdn_pipe = (DATA_PIPE * )LIST_VALUE(p_list_node);

		if (p_cdn_pipe->_p_resource->_resource_type == PEER_RESOURCE) 
		{
			LOG_DEBUG("cdn pipe:0x%x, cdn resource type:%u, cdn pipe from:%u.", p_cdn_pipe,
				p_cdn_pipe->_p_resource->_resource_type, p2p_get_from(p_cdn_pipe->_p_resource));

			if(p2p_get_from(p_cdn_pipe->_p_resource) == P2P_FROM_NORMAL_CDN) 
			{
				if(p_cdn_pipe->_dispatch_info._pipe_state == PIPE_DOWNLOADING)
				{
					cm_update_data_pipe_speed( p_cdn_pipe );
					cur_normal_cdn_speed += p_cdn_pipe->_dispatch_info._speed;
				}
				else
				{
					p_cdn_pipe->_dispatch_info._speed = 0;
				}
			}
		}		
	}

	LOG_DEBUG("current connect manager:0x%x, normal cdn speed:%u.", p_connect_manager, 
		cur_normal_cdn_speed);

	return cur_normal_cdn_speed;
}

_u32 cm_get_current_task_speed_without_normal_cdn(CONNECT_MANAGER *p_connect_manager)
{
	_u32 cur_spd = 0;
	_u32 cur_task_spd = 0, cur_task_normal_cdn_spd = 0;

	// task_spd - normal_cdn_spd
	cur_task_spd = cm_get_current_task_speed(p_connect_manager);
	cur_task_normal_cdn_spd = cm_get_normal_cdn_speed(p_connect_manager);

	cur_spd = cur_task_spd - cur_task_normal_cdn_spd;

	LOG_DEBUG("current task speed:%u, current task normal cdn speed:%u, non normal cdn speed:%u.",
		cur_task_spd, cur_task_normal_cdn_spd, cur_spd);

	return cur_spd;
}

_u32 cm_get_normal_cdn_connect_time(CONNECT_MANAGER *p_connect_manager)
{
	_u32 ret = 0;
	RESOURCE_LIST *res_list = NULL;
	LIST_ITERATOR it = NULL;
	_u32 now_time = 0;

	if (!p_connect_manager) return ret;
	
	sd_time(&now_time);

	res_list = &(p_connect_manager->_cdn_res_list);
	if (!res_list) return 0;

	for (it = LIST_BEGIN(res_list->_res_list); it != LIST_END(res_list->_res_list); it = LIST_NEXT(it))
	{
		RESOURCE *res = (RESOURCE *)LIST_VALUE(it);

		LOG_DEBUG("cur res:0x%x, open time:%u, close time:%u.", res, res->_open_normal_cdn_pipe_time, 
			res->_close_normal_cdn_pipe_time);

		if (res)
		{
			ret += res->_last_pipe_connect_time;

			if (res->_open_normal_cdn_pipe_time != 0)
			{
				if (res->_close_normal_cdn_pipe_time != 0)
				{
					if (res->_close_normal_cdn_pipe_time < res->_open_normal_cdn_pipe_time)
					{
						LOG_ERROR("res:0x%x, open time:%u, close time:%u.", res, res->_open_normal_cdn_pipe_time, 
							res->_close_normal_cdn_pipe_time);
						sd_assert(FALSE);
						continue;
					}
					ret += res->_close_normal_cdn_pipe_time - res->_open_normal_cdn_pipe_time;
				}
				else
				{
					ret += now_time - res->_open_normal_cdn_pipe_time;
				}
			}
		}
	}

	LOG_DEBUG("now pipe connect time on normal cdn is:%u.", ret);
	return ret;
}

_u32 cm_get_normal_cdn_stat_para_helper(CONNECT_MANAGER *p_connect_manager,
										_u32 *cdn_cnt_tmp, 
										_u32 *cdn_valid_cnt_tmp, 
										_u32 *first_zero_dura_tmp, 
										RANGE_LIST *sum_time_span)
{
	_u32 ret = 0;

#ifdef EMBED_REPORT	
	// 为了避免在上报的时候，所有的普通的cdn还没有使用完，即没有关闭，此时需要主动获取一下使用时间
	cm_stat_normal_cdn_use_time_helper(p_connect_manager, FALSE);

	ret = range_list_add_range_list(sum_time_span, 
		&(p_connect_manager->_report_para._res_normal_cdn_use_time_list));

	*cdn_cnt_tmp += p_connect_manager->_report_para._res_normal_cdn_total;

	*cdn_valid_cnt_tmp += p_connect_manager->_report_para._res_normal_cdn_valid;

	if (*first_zero_dura_tmp == 0) {
		*first_zero_dura_tmp = p_connect_manager->_report_para._res_normal_cdn_first_from_task_start;
	}
	else {
		*first_zero_dura_tmp 
			= MIN(*first_zero_dura_tmp, p_connect_manager->_report_para._res_normal_cdn_first_from_task_start);
	}
#endif

	return ret;
}

_u32 cm_get_normal_cdn_stat_para(CONNECT_MANAGER *p_connect_manager, BOOL is_using_sub_conn,
								_u32 *use_time, _u32 *cdn_cnt, _u32 *cdn_valid_cnt, _u32 *first_zero_dura)
{
	_u32 ret = 0;
	MAP_ITERATOR map_node = NULL;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;
	RANGE_LIST sum_time_span;
	_u32 cdn_cnt_tmp = 0, cdn_valid_cnt_tmp = 0, first_zero_dura_tmp = 0;
	
	range_list_init(&sum_time_span);

	cm_get_normal_cdn_stat_para_helper(p_connect_manager, 
		&cdn_cnt_tmp, &cdn_valid_cnt_tmp, &first_zero_dura_tmp, &sum_time_span);
	
	if (is_using_sub_conn) 
	{
		map_node = MAP_BEGIN( p_connect_manager->_sub_connect_manager_map );
		while( map_node != MAP_END( p_connect_manager->_sub_connect_manager_map ) )
		{
			p_sub_connect_manager = MAP_VALUE( map_node );

			cm_get_normal_cdn_stat_para_helper(p_sub_connect_manager, 
				&cdn_cnt_tmp, &cdn_valid_cnt_tmp, &first_zero_dura_tmp, &sum_time_span);

			map_node = MAP_NEXT( p_connect_manager->_sub_connect_manager_map, map_node );
		}
	}	

	*use_time = range_list_get_total_num(&sum_time_span);
	*cdn_cnt = cdn_cnt_tmp;
	*cdn_valid_cnt = cdn_valid_cnt_tmp;
	*first_zero_dura = first_zero_dura_tmp;

	range_list_clear(&sum_time_span);

	LOG_DEBUG( "connect manager:0x%x, normal cdn use time:%u, cdn count:%u, cdn valid count:%u, first zero:%u.", 
		p_connect_manager, *use_time, *cdn_cnt, *cdn_valid_cnt, *first_zero_dura);

	return ret;
}
