/*****************************************************************************
 *
 * Filename: 			connect_manager_imp.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2008/08/08
 *	
 * Purpose:				Implement the connect manager's functions.
 *
 *****************************************************************************/


#if !defined(__CONNECT_MANAGER_IMP_20080808)
#define __CONNECT_MANAGER_IMP_20080808

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/url.h"

#include "connect_manager/connect_manager.h"
#include "connect_manager/connect_manager_setting.h"


typedef BOOL (*distroy_filter_fun)(CONNECT_MANAGER *manager, DATA_PIPE *p_data_pipe );

/*****************************************************************************
 * cm init/uninit.
 *****************************************************************************/
_int32 cm_init_struct(CONNECT_MANAGER* p_connect_manager, void *p_data_manager, can_destory_resource_call_back p_call_back);
_int32 cm_uninit_struct(CONNECT_MANAGER *p_connect_manager);

/*****************************************************************************
 * Used in creating resources to avoid repeated resource.
 *****************************************************************************/
BOOL cm_is_server_res_exist(CONNECT_MANAGER* p_connect_manager, char *url, _u32 url_size, _u32 *p_hash_value);

_int32 cm_value_compare(void *E1, void *E2);
void cm_adjust_url_format(char *p_url, _u32 url_size);

BOOL cm_is_enable_server_res(CONNECT_MANAGER* p_connect_manager, enum SCHEMA_TYPE url_type, BOOL is_origin);
BOOL cm_is_active_peer_res_exist(CONNECT_MANAGER *p_connect_manager, char *p_peer_id, _u32 peer_id_len, _u32 host_ip, _u16 port, _u32 *p_hash_value);

BOOL cm_is_enable_peer_res(CONNECT_MANAGER *p_connect_manager, _u32 peer_capability);

#ifdef ENABLE_BT 
BOOL cm_is_enable_bt_res(CONNECT_MANAGER *p_connect_manager);
BOOL cm_is_bt_res_exist(CONNECT_MANAGER *p_connect_manager, _u32 host_ip, _u16 port, _u8 info_hash[20], _u32 *p_hash_value);
#endif /* #ifdef ENABLE_BT  */

#ifdef ENABLE_EMULE
BOOL cm_is_emule_res_exist( CONNECT_MANAGER *p_connect_manager, _u32 ip, _u16 port, _u32 *p_hash_value );
#endif /* #ifdef ENABLE_EMULE  */

/*****************************************************************************
 * Handle passive peer.
 *****************************************************************************/
_int32 cm_remove_peer_res_from_list(CONNECT_MANAGER *p_connect_manager, RESOURCE_LIST *p_res_list, RESOURCE *p_res);
_int32 cm_handle_passive_peer(CONNECT_MANAGER *p_connect_manager, RESOURCE *p_peer_res);
_int32 cm_handle_pasive_valid_peer(CONNECT_MANAGER *p_connect_manager, RESOURCE *p_peer_res);


/*****************************************************************************
 * Create server and peer pipes. Update lists according to pipe's information.
 *****************************************************************************/
 _int32 cm_update_server_connect_status(CONNECT_MANAGER *p_connect_manager);
 void cm_update_server_hash_map(CONNECT_MANAGER *p_connect_manager);
 void cm_update_server_pipe_speed(CONNECT_MANAGER *p_connect_manager, BOOL *is_server_err_get_buffer);
 void cm_update_server_pipe_score(CONNECT_MANAGER *p_connect_manager);

 _int32 cm_update_connect_status(CONNECT_MANAGER *p_connect_manager);
 
 _int32 cm_create_server_pipes(CONNECT_MANAGER *p_connect_manager);
 /* @Simple Function@ */
_u32 cm_get_new_server_pipe_num(CONNECT_MANAGER *p_connect_manager);
_int32 cm_create_pipes_from_server_res_list(CONNECT_MANAGER *p_connect_manager, RESOURCE_LIST *p_res_list, _u32 max_pipe_num, _u32 *p_create_pipe_num);
_int32 cm_create_pipes_from_server_using_pipes(CONNECT_MANAGER *p_connect_manager, _u32 max_pipe_num, _u32 *p_create_pipe_num);
_int32 cm_create_single_server_pipe(CONNECT_MANAGER *p_connect_manager, RESOURCE *p_res);
void cm_set_res_type_switch(CONNECT_MANAGER *p_connect_manager);

_int32 cm_update_peer_connect_status(CONNECT_MANAGER *p_connect_manager);
void cm_update_peer_hash_map(CONNECT_MANAGER *p_connect_manager);

void cm_update_peer_pipe_speed(CONNECT_MANAGER *p_connect_manager, BOOL *is_peer_err_get_buffer);
void cm_update_peer_pipe_score(CONNECT_MANAGER *p_connect_manager);

_int32 cm_create_active_peer_pipes(CONNECT_MANAGER *p_connect_manager);
/* @Simple Function@ */
_u32 cm_get_new_peer_pipe_num(CONNECT_MANAGER *p_connect_manager);
_int32 cm_create_pipes_from_peer_res_list(CONNECT_MANAGER *p_connect_manager, RESOURCE_LIST *p_res_list, BOOL is_retry_list, _u32 max_pipe_num, _u32 *p_create_pipe_num);
_int32 cm_create_single_active_peer_pipe(CONNECT_MANAGER * p_connect_manager, RESOURCE * p_res);

void cm_update_list_pipe_speed(PIPE_LIST *p_pipe_list, _u32 *p_total_speed, BOOL *is_err_get_buffer);
void cm_update_list_pipe_score(PIPE_LIST *p_pipe_list, _u32 *p_total_speed);
void cm_update_list_pipe_upload_speed(PIPE_LIST *p_pipe_list, _u32 *p_total_speed);

void cm_update_data_pipe_speed(DATA_PIPE *p_data_pipe);
_u32 cm_get_choke_pipe_score(DATA_PIPE *p_data_pipe, _u32 cur_time_ms);

_u32 cm_idle_res_num(CONNECT_MANAGER *p_connect_manager);
_u32 cm_retry_res_num(CONNECT_MANAGER *p_connect_manager);

void cm_update_idle_status(CONNECT_MANAGER *p_connect_manager);

/* Generate retry and discard resources and update connecting pipes list */
_int32 cm_update_server_pipe_list(CONNECT_MANAGER *p_connect_manager);
_int32 cm_update_peer_pipe_list(CONNECT_MANAGER *p_connect_manager);

_int32 cm_update_failture_pipes(CONNECT_MANAGER *p_connect_manager,
							PIPE_LIST *p_working_pipe_list, 
							RESOURCE_LIST *p_using_res_list,
							RESOURCE_LIST *p_retry_res_list,
							RESOURCE_LIST *p_discard_res_list,
							BOOL is_connecting_list);
void cm_calc_res_retry_para( CONNECT_MANAGER *p_connect_manager, RESOURCE *p_res );
void cm_calc_res_retry_score( RESOURCE_LIST *p_retry_res_list,RESOURCE *p_res );

_int32 cm_update_to_connecting_pipes( CONNECT_MANAGER *p_connect_manager, PIPE_LIST *p_connecting_pipe_list, PIPE_LIST *p_working_pipe_list );
_int32 cm_move_res( RESOURCE_LIST *p_src_res_list, RESOURCE_LIST *p_dest_res_list, RESOURCE *p_target_res );
_int32 cm_move_res_to_order_list( RESOURCE_LIST *p_src_res_list, RESOURCE_LIST *p_dest_res_list, RESOURCE *p_target_res );


/*****************************************************************************
 * Destroy server and peer resources. 
 *****************************************************************************/
_int32 cm_destroy_all_discard_ress( CONNECT_MANAGER *p_connect_manager );

_int32 cm_destroy_all_ress( CONNECT_MANAGER *p_connect_manager );

_int32 cm_destroy_all_server_ress( CONNECT_MANAGER *p_connect_manager );
_int32 cm_destroy_all_peer_ress( CONNECT_MANAGER *p_connect_manager );
_int32 cm_destroy_discard_res_list( CONNECT_MANAGER *p_connect_manager, RESOURCE_LIST *p_res_list );

_int32 cm_destroy_res_list( CONNECT_MANAGER *p_connect_manager, RESOURCE_LIST *p_res_list );
_int32 cm_destroy_res( CONNECT_MANAGER *p_connect_manager, RESOURCE *p_res );
_int32 cm_move_hash_map_res( MAP *p_source_map, MAP *p_dest_map, RESOURCE *p_res );


/*****************************************************************************
 * Destroy server and peer pipes.
 *****************************************************************************/
_int32 cm_destroy_all_pipes( CONNECT_MANAGER* p_connect_manager );

void cm_destroy_all_pipes_fileter(CONNECT_MANAGER * p_connect_manager, distroy_filter_fun fun);

_int32 cm_destroy_all_pipes_except_cdn( CONNECT_MANAGER* p_connect_manager );

/* 销毁除了局域网http pipe 外的所有pipe */
_int32 cm_destroy_all_pipes_except_lan( CONNECT_MANAGER* p_connect_manager );


_int32 cm_destroy_all_server_pipes( CONNECT_MANAGER* p_connect_manager );

/* 销毁除了局域网http pipe 外的所有pipe */
_int32 cm_destroy_all_server_pipes_except_lan( CONNECT_MANAGER* p_connect_manager );

_int32 cm_destroy_all_peer_pipes( CONNECT_MANAGER* p_connect_manager );

_int32 cm_destroy_pipe_list( CONNECT_MANAGER *p_connect_manager, PIPE_LIST *p_pipe_list, BOOL is_connecting_list, distroy_filter_fun fun);

/* 销毁除了局域网http pipe 外的所有pipe */
_int32 cm_destroy_pipe_list_except_lan( CONNECT_MANAGER *p_connect_manager, PIPE_LIST *p_pipe_list, BOOL is_connecting_list );

_int32 cm_destroy_single_pipe( CONNECT_MANAGER *p_connect_manager, DATA_PIPE *p_data_pipe );


/*****************************************************************************
 * Discard server and peer resources.( used in origin download mode )
 *****************************************************************************/
_int32 cm_discard_all_server_res_except( CONNECT_MANAGER *p_connect_manager, RESOURCE *p_res );
_int32 cm_discard_all_peer_res( CONNECT_MANAGER *p_connect_manager );
_int32 cm_move_using_res_to_candidate_except_lan(CONNECT_MANAGER *p_connect_manager );
_int32 cm_move_using_res_to_candidate_except_res(CONNECT_MANAGER *p_connect_manager ,RESOURCE *p_except_res);
_int32 cm_move_using_res_list_to_candidate_res_list_except_res( CONNECT_MANAGER *p_connect_manager, RESOURCE_LIST *p_using_res_list, RESOURCE_LIST *p_candidate_res_list, RESOURCE *p_except_res );
_int32 cm_move_res_list_except_res( CONNECT_MANAGER *p_connect_manager, RESOURCE_LIST *p_src_res_list, RESOURCE_LIST *p_dest_res_list, RESOURCE *p_except_res );

_int32 cm_destroy_server_pipes_except( CONNECT_MANAGER *p_connect_manager, RESOURCE *p_res );
_int32 cm_destroy_pipe_list_except_owned_by_res( CONNECT_MANAGER *p_connect_manager, PIPE_LIST *p_pipe_list, RESOURCE *p_res ,BOOL is_connecting_list);


/*****************************************************************************
 * Get excellent filename.
 *****************************************************************************/
/*
BOOL cm_get_excellent_filename_from_pipe_list( CONNECT_MANAGER *p_connect_manager, PIPE_LIST *p_pipe_list, 
	char *p_str_excellent_name, _u32 excellent_name_buffer_len, _u32 *p_excellent_name_len, 
	char *p_str_normal_name, _u32 normal_name_buffer_len, _u32 *p_normal_name_len );
*/
BOOL cm_get_excellent_filename_from_res_list( CONNECT_MANAGER *p_connect_manager, RESOURCE_LIST *p_res_list, 
	char *p_str_excellent_name, _u32 excellent_name_buffer_len, _u32 *p_excellent_name_len, 
	char *p_str_normal_name, _u32 normal_name_buffer_len, _u32 *p_normal_name_len );

BOOL cm_get_excellent_filename_from_orgin_res( CONNECT_MANAGER *p_connect_manager, RESOURCE *p_orgin_res, 
	char *p_str_excellent_name, _u32 excellent_name_buffer_len, _u32 *p_excellent_name_len, 
	char *p_str_normal_name, _u32 normal_name_buffer_len, _u32 *p_normal_name_len );

void cm_set_need_new_server_res( CONNECT_MANAGER *p_connect_manager, BOOL is_need_new_res );
void cm_set_need_new_peer_res( CONNECT_MANAGER *p_connect_manager, BOOL is_need_new_res );
void cm_check_is_fast_speed( CONNECT_MANAGER *p_connect_manager );

/*****************************************************************************
 * sub connect_manager.
 *****************************************************************************/
_int32 cm_get_sub_connect_manager( CONNECT_MANAGER *p_connect_manager, _u32 file_index, CONNECT_MANAGER **pp_sub_connect_manager );

_int32 cm_create_special_pipes(CONNECT_MANAGER* p_connect_manager);
#ifdef _LOGGER
/*****************************************************************************
 * Print information.
 *****************************************************************************/
void cm_print_info( CONNECT_MANAGER *p_connect_manager );
void cm_print_server_res_list_info( CONNECT_MANAGER *p_connect_manager, RESOURCE_LIST *p_res_list );
void cm_print_peer_res_list_info( CONNECT_MANAGER *p_connect_manager, RESOURCE_LIST *p_res_list );

#endif

#ifdef _CONNECT_DETAIL

void cm_update_peer_pipe_info( CONNECT_MANAGER *p_connect_manager );
void cm_update_server_pipe_info( CONNECT_MANAGER *p_connect_manager );


#endif

#ifdef ENABLE_CDN
/***************************************************************************************************/
/*   Internal functions   */
/***************************************************************************************************/

_int32 cm_create_cdn_pipes( CONNECT_MANAGER *p_connect_manager );
_int32 cm_create_single_cdn_pipe( CONNECT_MANAGER *p_connect_manager, RESOURCE *p_res );
_int32 cm_create_single_normal_cdn_pipe( CONNECT_MANAGER *p_connect_manager, RESOURCE *p_res );
_int32 cm_destroy_all_cdn_ress( CONNECT_MANAGER *p_connect_manager );
_int32 cm_destroy_cdn_res( CONNECT_MANAGER *p_connect_manager, RESOURCE *p_res );
_int32 cm_destroy_all_cdn_pipes( CONNECT_MANAGER* p_connect_manager );
_int32 cm_close_all_cdn_manager_pipes( CONNECT_MANAGER* p_connect_manager );
_int32 cm_destroy_single_cdn_pipe( CONNECT_MANAGER *p_connect_manager, DATA_PIPE *p_data_pipe );
_int32 cm_destroy_single_normal_cdn_pipe(CONNECT_MANAGER *p_connect_manager, DATA_PIPE *p_data_pipe);
// @param: p_connect_manager 当前连接调度器
// @param: is_destroy 该次调用是否用于销毁时统计
_int32 cm_stat_normal_cdn_use_time_helper(CONNECT_MANAGER *p_connect_manager, BOOL is_destroy);
_int32 cm_update_cdn_pipes( CONNECT_MANAGER *p_connect_manager );
BOOL cm_is_cdn_peer_res_exist( CONNECT_MANAGER *p_connect_manager, _u32 host_ip, _u16 port );
BOOL cm_is_cdn_peer_res_exist_by_peerid(void * p_connect_manager, const char* peerid);
BOOL cm_filter_pipes_res_exist_cdnres(CONNECT_MANAGER *manager, DATA_PIPE* data_pipe);

BOOL cm_filter_pipes_low_speed(CONNECT_MANAGER *manager, DATA_PIPE* data_pipe);

_int32 cm_destroy_normal_cdn_res(CONNECT_MANAGER *p_connect_manager, RESOURCE *p_res);
_int32 cm_destroy_normal_cdn_pipes(CONNECT_MANAGER *p_connect_manager);

#endif /* ENABLE_CDN */

#ifdef EMBED_REPORT
void cm_add_peer_res_report_para( CONNECT_MANAGER *p_connect_manager, _u8 capability, _u8 from );
void cm_valid_res_report_para( CONNECT_MANAGER *p_connect_manager, RESOURCE *p_res );
#endif

_u32 cm_create_new_pipes(CONNECT_MANAGER *p_connect_manager, _u32 sum);

_u32 cm_is_origin_mode(CONNECT_MANAGER* p_connect_manager);

_u32 cm_is_use_multires(CONNECT_MANAGER* p_connect_manager);

_u32 cm_is_use_pcres(CONNECT_MANAGER* p_connect_manager);

_u32 cm_set_use_pcres(CONNECT_MANAGER* p_connect_manager, BOOL isuse);

_u8 cm_get_resource_from(RESOURCE* res);

_int32 cm_destroy_not_support_range_speed_up_res(CONNECT_MANAGER* p_connect_manager);

#ifdef __cplusplus
}
#endif

#endif // !defined(__CONNECT_MANAGER_IMP_20080808)




