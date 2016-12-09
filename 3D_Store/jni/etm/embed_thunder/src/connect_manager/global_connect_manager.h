/*****************************************************************************
 *
 * Filename: 			global_connect_manager.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2008/11/6
 *	
 * Purpose:				Global_connect_manager struct.
 *
 *****************************************************************************/

/* 全局调度目标:(提高当前总体任务速度)
 * 1. 当用户指定的pipe充足时，在不影响速度的前提下减少pipe数。
 * 2. 当用户指定的pipe不充足时，保证使用的尽可能是快的资源。
 * 3. 当用户限速时，动态调整pipe数。
 *

 * 全局调度
 * 1. 由外层指定_global_max_pipe_num和_global_max_connecting_pipe_num;由全局定时驱动
 * 2. 任务有调度豁免时间，即在开始前一段时间不进行全局调度，每个任务有固定的max_pipe_num
 *    和max_connecting_pipe_num.

 * 全局调度删除pipe策略:
 * 1. 删除pipe时，讲当前正在使用 pipe排序，从低到高删除，删除pipe后总速度不低于当前速度 的95%。
 * 2. 进行单个任务，按照第一步是否删除进行删除，保证每个任务pipe个数不低于min_pipe_num.
 *
 * 全局调度添加pipe策略:
 * 1. 全局调度时优先使用从未使用资源，给任务安照当前速度评级，级别高的任务打开更多的未使用资源。
 * 2. 对server资源打开多个pipe(单个server资源的pipe个数不超过5，并且当pipe个数增加，资源总 的下载速度没有增加时，不增加pipe。
 * 3. 未使用资源用完后，从待选资源中选择下个资源,其中2/3概率按照历史速度从大到小选择.
 *    1/3的概率按照时间顺序选择。
 * 4. 使用发生过错误的pipe，选用顺序根据任务评级，级别高的任务打开更多的pipe。
 */


#if !defined(__GLOBAL_CONNECT_MANAGER_20081106)
#define __GLOBAL_CONNECT_MANAGER_20081106

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/list.h"

#include "connect_manager/connect_filter_manager.h"
#include "connect_manager/connect_manager.h"

struct tagCONNECT_MANAGER;
enum tagCONNECT_MANAGER_LEVEL;

typedef struct tagPIPE_WRAP
{
	DATA_PIPE *_data_pipe_ptr;
	BOOL _is_deleted_by_pipe;
} PIPE_WRAP;

typedef struct tagRES_WRAP
{
	RESOURCE *_res_ptr;
	BOOL _is_deleted;
} RES_WRAP;

typedef struct tagGLOBAL_CONNECT_MANAGER
{
	_u32 _global_max_pipe_num;
	_u32 _global_test_speed_pipe_num;
	_u32 _global_max_filter_pipe_num;
	//_u32 _global_pipe_low_limit;
	_u32 _global_cur_pipe_num;// sum of connect_manager pipe num which is in global dispatch mode
	_u32 _assign_max_pipe_num;
	_u32 _global_max_connecting_pipe_num;
	LIST _connect_manager_ptr_list;
	LIST _using_pipe_list;
	LIST _candidate_res_list;
	BOOL _is_use_global_strategy;
	CONNECT_FILTER_MANAGER _filter_manager;
	_u32 _global_dispatch_num;
	_u32 _global_switch_num;
	_u32 _global_start_ticks;
	_u32 _total_res_num;
} GLOBAL_CONNECT_MANAGER;

_int32 gcm_update_connect_status(void);
_int32 gcm_check_cm_level(void);
_int32 gcm_register_using_pipes( struct tagCONNECT_MANAGER *p_connect_manager );
_int32 gcm_register_candidate_res_list( struct tagCONNECT_MANAGER *p_connect_manager );
_int32 gcm_register_list_pipes( struct tagCONNECT_MANAGER *p_connect_manager, PIPE_LIST *p_pipe_list, BOOL is_working_list );

_int32 gcm_calc_global_connect_level(void);
_int32 gcm_filter_low_speed_pipe(void);
BOOL gcm_is_need_filter(void);

_int32 gcm_select_pipe_to_create(void);
_int32 gcm_create_pipes(void);

_int32 gcm_set_global_idle_res_num(void);
//_int32 gcm_set_each_manager_idle_assign_num( enum tagCONNECT_MANAGER_LEVEL cm_level );
_int32 gcm_select_using_res_to_create_pipe(void);
_int32 gcm_select_candidate_res_to_create_pipe(void);
_int32 gcm_set_global_retry_res_num(void);
_int32 gcm_set_retry_res_assign_num( enum tagCONNECT_MANAGER_LEVEL cm_level );


_int32 gcm_create_pipes_from_idle_res( struct tagCONNECT_MANAGER *p_connect_manager );
_int32 gcm_create_pipes_from_retry_res( struct tagCONNECT_MANAGER *p_connect_manager );

_int32 gcm_create_pipes_from_candidate_res( struct tagCONNECT_MANAGER *p_connect_manager );
_int32 gcm_create_pipes_from_candidate_server_res_list( struct tagCONNECT_MANAGER *p_connect_manager );
_int32 gcm_create_pipes_from_candidate_peer_res_list( struct tagCONNECT_MANAGER *p_connect_manager );

_int32 gcm_filter_pipes( struct tagCONNECT_MANAGER *p_connect_manager );
_int32 gcm_create_manager_pipes( struct tagCONNECT_MANAGER *p_connect_manager );


//pipe_wrap malloc/free wrap
_int32 init_pipe_wrap_slabs(void);
_int32 uninit_pipe_wrap_slabs(void);
_int32 gcm_malloc_pipe_wrap(PIPE_WRAP **pp_node);
_int32 gcm_free_pipe_wrap(PIPE_WRAP *p_node);


//res_wrap malloc/free wrap
_int32 init_res_wrap_slabs(void);
_int32 uninit_res_wrap_slabs(void);
_int32 gcm_malloc_res_wrap(RES_WRAP **pp_node);
_int32 gcm_free_res_wrap(RES_WRAP *p_node);

#ifdef __cplusplus
}
#endif

#endif // !defined(__GLOBAL_CONNECT_MANAGER_20081106)


