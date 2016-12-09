/*****************************************************************************
 *
 * Filename: 			connect_filter_manager.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2008/08/25
 *	
 * Purpose:				Filter pipes for connect manager.
 *
 *****************************************************************************/

#if !defined(__CONNECT_FILTER_MANAGER_20080825)
#define __CONNECT_FILTER_MANAGER_20080825

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#include "connect_manager/data_pipe.h"

typedef _u32 (*cm_get_key_value)( LIST_ITERATOR list_iter );
typedef BOOL (*cm_is_iter_valid)( LIST_ITERATOR list_iter );
typedef _int32 (*cm_free_iterator)( LIST_ITERATOR list_iter );


struct tagCONNECT_MANAGER;

/************************************************************************
 * The struct contains filter control parameters.
 ************************************************************************/
typedef struct tagCONNECT_FILTER_MANAGER 
{
	_u32 _cur_total_speed;//sum of current pipes speed	
	_u32 _cur_speed_lower_limit;//to keep the speed bigger then the limit
	_u32 _cur_filter_dispatch_ms_time;
	_u32 _idle_ticks;
} CONNECT_FILTER_MANAGER;



void cm_init_filter_manager( CONNECT_FILTER_MANAGER *p_connect_filter_manager );
/*****************************************************************************
 * Do connect dispatch, destroy the low speed pipes.
 *****************************************************************************/
 
_int32 cm_filter_pipes( struct tagCONNECT_MANAGER *p_connect_manager );

_int32 cm_order_using_pipes( struct tagCONNECT_MANAGER *p_connect_manager );
_int32 cm_order_using_server_pipes( struct tagCONNECT_MANAGER *p_connect_manager );
_int32 cm_order_using_peer_pipes( struct tagCONNECT_MANAGER *p_connect_manager );
_int32 gcm_order_global_using_pipes(void);
_int32 gcm_order_global_candidate_res(void);
_int32 cm_get_order_list( LIST *p_src_list, 
	LIST *p_order_list, 
	cm_get_key_value p_get_value, 
	cm_is_iter_valid p_is_valid,
	cm_free_iterator p_free );

BOOL cm_is_normal_iter_valid( LIST_ITERATOR list_iter );
BOOL cm_is_res_wrap_iter_valid( LIST_ITERATOR list_iter );
_u32 cm_get_normal_dispatch_pipe_score( LIST_ITERATOR list_iter );

_u32 cm_get_global_dispatch_pipe_score( LIST_ITERATOR list_iter );
BOOL cm_is_pipe_wrap_iter_valid( LIST_ITERATOR list_iter );
_int32 cm_free_pipe_wrap_iterator( LIST_ITERATOR list_iter );
_int32 cm_free_res_wrap_iterator( LIST_ITERATOR list_iter );

_u32 cm_get_global_dispatch_res_max_speed( LIST_ITERATOR list_iter );

//_int32 cm_get_speed_order_list( PIPE_LIST *p_src_pipe_list, PIPE_LIST *p_order_pipe_list );

_int32 cm_do_filter_dispatch( struct tagCONNECT_MANAGER *p_connect_manager );
_int32 gcm_do_global_filter_dispatch(void);
//BOOL gcm_is_global_slow_pipe( CONNECT_FILTER_MANAGER *p_filter_manager, DATA_PIPE *p_data_pipe  );

_int32 cm_filter_speed_list( struct tagCONNECT_MANAGER *p_connect_manager,_u32 max_speed
    , RESOURCE* pres, PIPE_LIST *p_pipe_list, RESOURCE_LIST *p_using_res_list, RESOURCE_LIST *p_candidate_res_list, BOOL is_server );
_int32 cm_init_filter_para( CONNECT_FILTER_MANAGER *p_connect_filter_manager, _u32 cur_total_speed  );
BOOL cm_is_lower_speed_pipe( CONNECT_FILTER_MANAGER *p_connect_filter_manager, DATA_PIPE *p_data_pipe, _u32 max_speed
        , RESOURCE* pres );
BOOL cm_is_new_pipe( CONNECT_FILTER_MANAGER *p_connect_filter_manager, DATA_PIPE *p_data_pipe );


#ifdef __cplusplus
}
#endif

#endif // !defined(__CONNECT_FILTER_MANAGER_20080825)
