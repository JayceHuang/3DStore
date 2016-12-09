#if !defined(__ULM_PIPE_MANAGER_H_20090604)
#define __ULM_PIPE_MANAGER_H_20090604


#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"

#ifdef UPLOAD_ENABLE

#include "utility/list.h"
#include "utility/map.h"
#include "utility/errcode.h"
#include "connect_manager/data_pipe.h"
#include "asyn_frame/msg.h"

#ifdef _CONNECT_DETAIL
struct tagPEER_PIPE_INFO_ARRAY;
#endif

typedef enum tagULM_PIPE_TYPE 
{
    ULM_UNDEFINE = -1,
	ULM_D_AND_U_P2P_PIPE = 0,     // 普通p2p 边下边传pipe
	ULM_PU_P2P_PIPE,   			  // 普通p2p 纯上传pipe
#ifdef ENABLE_BT
	ULM_D_AND_U_BT_PIPE,		  // bt 边下边传pipe
	ULM_D_AND_U_BT_P2P_PIPE,   // bt加速的p2p 边下边传pipe
#endif
#ifdef ENABLE_EMULE
	ULM_D_AND_U_EMULE_PIPE,
	ULM_D_AND_U_EMULE_P2P_PIPE,	//emule加速的p2p边下边传pipe
#endif
} ULM_PIPE_TYPE;


typedef struct 
{
	DATA_PIPE	* _pipe;          
	_u8 _gcid[CID_SIZE];          
	ULM_PIPE_TYPE	_type;
	_u32  _score;
	_u32 _choke_time;
#ifdef ULM_DEBUG
	_u32  _ref_score;
	_u32  _file_score;
	_u32  _type_score;
	_u32  _t_s_score;
#endif
}UPM_PIPE_INFO;

typedef struct 
{
	LIST		_unchoke_pipe_list;           // list of UPM_PIPE_INFO
	LIST		_choke_pipe_list;           // list of UPM_PIPE_INFO
	MAP	_pipe_info_map;		// pipe指针地址索引 UPM_PIPE_INFO
	_u32 	_max_unchoke_pipes;
	_u32 	_max_choke_pipes;
	_u32 	_max_speed;
	_int32     	_error_code;
	_u32 	_high_score_choked_pipe_num;
	_u32 	_low_score_unchoked_pipe_num;
	BOOL	_wait_uninit;
#ifdef _CONNECT_DETAIL
    struct tagPEER_PIPE_INFO_ARRAY _peer_pipe_info;
#endif	
}ULM_PIPE_MANAGER;


/*--------------------------------------------------------------------------*/
/*                Interfaces provid for upload_manager				        */
/*--------------------------------------------------------------------------*/
_int32 init_upload_pipe_manager(void);
_int32 uninit_upload_pipe_manager(void);
_int32 upm_close_upload_pipes(void);
BOOL upm_is_pipe_full(void);
_u32 upm_get_cur_upload_speed( void);
_u32 upm_get_max_upload_speed( void);
_int32 upm_add_pipe(const _u8 *gcid  ,DATA_PIPE * p_pipe,  ULM_PIPE_TYPE type);
_int32 upm_remove_pipe( DATA_PIPE * p_pipe);
_u8* upm_get_gcid(DATA_PIPE * p_pipe);
  ULM_PIPE_TYPE upm_get_pipe_type(DATA_PIPE * p_pipe);
void upm_scheduler(void);
BOOL upm_is_pipe_unchoked(DATA_PIPE * p_pipe);
_int32 upm_notify_have_block( const _u8 *gcid);
#ifdef _CONNECT_DETAIL
void upm_update_pipe_info(void);
void* upm_get_pipe_info(void);
#endif

/*--------------------------------------------------------------------------*/
/*                Internal functions				        */
/*--------------------------------------------------------------------------*/
_int32 upm_del_pipes(const _u8 *gcid);
void upm_clear(void);
void upm_clear_failure_pipes(void);
_u32 upm_get_pipe_referrence_score(UPM_PIPE_INFO* p_pipe_info);
_u32 upm_get_pipe_speed(UPM_PIPE_INFO* p_pipe_info);
_u32 upm_get_pipe_speed_score(UPM_PIPE_INFO* p_pipe_info,_u32 average_speed);
_u32 upm_get_file_score(const _u8 * gcid,ULM_PIPE_TYPE	_type);
_u32 upm_get_pipe_type_score( ULM_PIPE_TYPE	_type);
_u32 upm_get_pipe_time_score(UPM_PIPE_INFO* p_pipe_info);
void upm_update_unchoked_pipe_score(UPM_PIPE_INFO* p_pipe_info,_u32 average_speed);
_int32 upm_sort_unchoked_pipes(void);
void upm_update_unchoked_pipes_score(void);
void upm_update_choked_pipe_score(UPM_PIPE_INFO* p_pipe_info);
void upm_update_choked_pipes_score(void);
_int32 upm_assign_pipes(void);
_int32 upm_failure_exit(_int32 err_code);
_int32 upm_map_compare_fun( void *E1, void *E2 );
_u32 upm_get_average_speed(void);
_int32 upm_get_pipe_list_by_gcid(const _u8* gcid,LIST * pipe_list);
_int32 upm_create_pipe_info(DATA_PIPE	* p_pipe,_u8* gcid,  ULM_PIPE_TYPE type,UPM_PIPE_INFO ** pp_pipe_info);
_int32 upm_delete_pipe_info(UPM_PIPE_INFO * p_pipe_info);

_int32 upm_close_pure_upload_pipes(); // 边下边传的pipe不能关闭

#endif /* UPLOAD_ENABLE */

#ifdef __cplusplus
}
#endif

#endif // !defined(__ULM_PIPE_MANAGER_H_20090604)
