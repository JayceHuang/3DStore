/*****************************************************************************
 *
 * Filename: 			connect_manager.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2008/08/08
 *	
 * Purpose:				Define the basic structs of connect manager.
 *
 *****************************************************************************/

#if !defined(__CONNECT_MANAGER_20080808)
#define __CONNECT_MANAGER_20080808

#ifdef __cplusplus
extern "C" 
{
#endif

#include "connect_manager/resource.h"
#include "connect_manager/data_pipe.h"
#include "connect_manager/connect_filter_manager.h"
#include "connect_manager/pipe_interface.h"

#include "utility/map.h"

typedef BOOL (*can_destory_resource_call_back)( void* p_data_manager, RESOURCE *p_abandon_res );

struct tagPEER_PIPE_INFO_ARRAY;
struct tagSERVER_PIPE_INFO_ARRAY;
struct _tag_task;

typedef enum tagCONNECT_MANAGER_LEVEL
{
	INIT_CM_LEVEL = 0,
	UNKNOWN_CM_LEVEL,
	LOW_CM_LEVEL,
	NORMAL_CM_LEVEL,
	HIGH_CM_LEVEL
} CONNECT_MANAGER_LEVEL;

#ifdef EMBED_REPORT
typedef struct tagCM_REPORT_PARA
{
	_u32  _res_server_total;			 //server��Դ����																	
	_u32  _res_server_valid;			 //��Чserver��Դ����																
	_u32  _res_cdn_total;				 //CDN��Դ����																		
	_u32  _res_cdn_valid;				 //��ЧCDN��Դ����		
	_u32  _res_partner_cdn_total;		//partner_CDN��Դ����																		
	_u32  _res_partner_cdn_valid;		//��Чpartner_CDN��Դ����	
		
	_u32  _cdn_res_max_num; 			 //ͬʱʹ�����Cdn��Դ����															
	_u32  _cdn_connect_failed_times;	 //Cdn ʹ��ʧ�ܴ��� 	
	
	_u32  _res_n2i_total;				 //���ص�������������������Դ��															
	_u32  _res_n2i_valid;				 //��Ч����������������Դ�� 	
	_u32  _res_n2n_total;				 //���ص�������������������Դ�� (����ͬ����)													
	_u32  _res_n2n_valid;				 //��Ч����������������Դ�� (����ͬ����)														
	_u32  _res_n2s_total;				 //����������ͬ������������Դ�� 													
	_u32  _res_n2s_valid;				 //��Ч��������ͬ������������Դ��													
	_u32  _res_i2i_total;				 //���ص�������������������Դ�� 													
	_u32  _res_i2i_valid;				 //��Ч����������������Դ�� 														
	_u32  _res_i2n_total;				 //������������������Դ��															
	_u32  _res_i2n_valid;				 //��Ч����������������Դ�� 	
	
	_u32  _hub_res_total;					 //���ϵ�Hub��Դ����																
	_u32  _hub_res_valid;					 //���ϵ�Hub��Ч��Դ����															
	_u32  _active_tracker_res_total;	 //���ϵ�Tracker������Դ����														
	_u32  _active_tracker_res_valid;	 //���ϵ�Tracker��Ч������Դ����		
	_u32  _res_normal_cdn_total;		// ������ͨcdn����Դ����
	_u32  _res_normal_cdn_valid;		// �������ϵ���ͨcdn����Դ����
	
	_u32  _res_peer_total;				// peer��Դ������

	_u32  _res_normal_cdn_start_time;	// ��ʼʹ����ͨcdn��ʱ��
	_u32  _res_normal_cdn_close_time;	// �ر�ʹ����ͨcdn��ʱ��
	_u32  _res_normal_cdn_sum_use_time;	// ����ʹ����ͨcdn��ʱ��
	RANGE_LIST _res_normal_cdn_use_time_list;	// ʹ����ͨcdn��ʱ��μ���
	_u32  _res_normal_cdn_first_from_task_start;	// ��һ��ʹ��cdn�������������ʱ��
} CM_REPORT_PARA;
#endif

/************************************************************************
 * The struct contains pipes list, resource list, and control parameters.
 ************************************************************************/
typedef struct tagCONNECT_MANAGER 
{
	void *_data_manager_ptr;

	RESOURCE_LIST  _idle_server_res_list;
	RESOURCE_LIST  _idle_peer_res_list;
	
	RESOURCE_LIST  _using_server_res_list;
	RESOURCE_LIST  _using_peer_res_list;
	
	RESOURCE_LIST  _candidate_server_res_list;//filtered low speed server resource
	RESOURCE_LIST  _candidate_peer_res_list;//filtered low speed peer resource
	
	RESOURCE_LIST  _retry_server_res_list;//server resource needed to retry which pipe is failture.
	RESOURCE_LIST  _retry_peer_res_list;//peer resource needed to retry which pipe is failture.
	
	RESOURCE_LIST  _discard_server_res_list;//server resources discarded which pipe is failture and not need retry.
	RESOURCE_LIST  _discard_peer_res_list;//peer resources discarded which pipe is failture and not need retry.

	
	PIPE_LIST _connecting_server_pipe_list;
	PIPE_LIST _connecting_peer_pipe_list;

	PIPE_LIST _working_server_pipe_list;	
	PIPE_LIST _working_peer_pipe_list;	

	CONNECT_FILTER_MANAGER _connect_filter_manager;
	
	_u32 _cur_res_num;	
	_u32 _cur_pipe_num;	
	_u32 _max_pipe_num;
	BOOL _is_only_using_origin_server;
	BOOL _is_idle_status;
	RESOURCE *_origin_res_ptr;
	_u32 _status_ticks;
	_u32 _res_type_switch;
	MAP _server_hash_value_map;
	MAP _peer_hash_value_map;
	MAP _destoryed_server_res_hash_value_map;
	MAP _destoryed_peer_res_hash_value_map;
	CONNECT_MANAGER_LEVEL _cm_level;
	BOOL _is_need_new_server_res;
	BOOL _is_need_new_peer_res;
	BOOL _is_fast_speed;
	_u32 _cur_server_speed;//sum of current server pipes speed
	_u32 _cur_peer_speed;//sum of current peer pipes speed
	_u32 _cur_peer_upload_speed;

	_u32 _max_server_speed;
	_u32 _max_peer_speed;
	_u32 _clear_server_hash_map_ticks;
	_u32 _clear_peer_hash_map_ticks;

	_u32 _idle_assign_num;
	_u32 _retry_assign_num;
	_u32 _start_time;
	BOOL _is_err_get_buffer_status;

	MAP _sub_connect_manager_map;
	PIPE_INTERFACE _pipe_interface;
	can_destory_resource_call_back _call_back_ptr;
	

#ifdef _CONNECT_DETAIL
    _u32 _server_destroy_num;
    _u32 _peer_destroy_num;
    struct tagPEER_PIPE_INFO_ARRAY _peer_pipe_info;
    
    struct tagSERVER_PIPE_INFO_ARRAY _server_pipe_info;
#endif

#ifdef ENABLE_CDN
	RESOURCE_LIST  _cdn_res_list;
	PIPE_LIST _cdn_pipe_list;
	BOOL _cdn_mode;
	_u32 _max_cdn_speed;
	BOOL _cdn_ok;
#endif

#ifdef EMBED_REPORT
	CM_REPORT_PARA _report_para;
#endif

	BOOL _pause_pipes;  // ��ֹͣ������������ͣ����,Ŀ���Ǳ�����vod����������
	BOOL enable_high_speed_channel;
	_u32 _last_dispatcher_time;
	struct _tag_task* _main_task_ptr;
}CONNECT_MANAGER;


#ifdef __cplusplus
}
#endif

#endif // !defined(__CONNECT_MANAGER_20080808)



