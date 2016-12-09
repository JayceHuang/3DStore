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

/* ȫ�ֵ���Ŀ��:(��ߵ�ǰ���������ٶ�)
 * 1. ���û�ָ����pipe����ʱ���ڲ�Ӱ���ٶȵ�ǰ���¼���pipe����
 * 2. ���û�ָ����pipe������ʱ����֤ʹ�õľ������ǿ����Դ��
 * 3. ���û�����ʱ����̬����pipe����
 *

 * ȫ�ֵ���
 * 1. �����ָ��_global_max_pipe_num��_global_max_connecting_pipe_num;��ȫ�ֶ�ʱ����
 * 2. �����е��Ȼ���ʱ�䣬���ڿ�ʼǰһ��ʱ�䲻����ȫ�ֵ��ȣ�ÿ�������й̶���max_pipe_num
 *    ��max_connecting_pipe_num.

 * ȫ�ֵ���ɾ��pipe����:
 * 1. ɾ��pipeʱ������ǰ����ʹ�� pipe���򣬴ӵ͵���ɾ����ɾ��pipe�����ٶȲ����ڵ�ǰ�ٶ� ��95%��
 * 2. ���е������񣬰��յ�һ���Ƿ�ɾ������ɾ������֤ÿ������pipe����������min_pipe_num.
 *
 * ȫ�ֵ������pipe����:
 * 1. ȫ�ֵ���ʱ����ʹ�ô�δʹ����Դ���������յ�ǰ�ٶ�����������ߵ�����򿪸����δʹ����Դ��
 * 2. ��server��Դ�򿪶��pipe(����server��Դ��pipe����������5�����ҵ�pipe�������ӣ���Դ�� �������ٶ�û������ʱ��������pipe��
 * 3. δʹ����Դ����󣬴Ӵ�ѡ��Դ��ѡ���¸���Դ,����2/3���ʰ�����ʷ�ٶȴӴ�Сѡ��.
 *    1/3�ĸ��ʰ���ʱ��˳��ѡ��
 * 4. ʹ�÷����������pipe��ѡ��˳�������������������ߵ�����򿪸����pipe��
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


