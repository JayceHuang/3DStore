#if !defined(__GLOBAL_CONNECT_DISPATCH_20081106)
#define __GLOBAL_CONNECT_DISPATCH_20081106

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/list.h"

#include "connect_manager/data_pipe.h"
#include "connect_manager/resource.h"

struct tagGLOBAL_CONNECT_MANAGER;

_int32 gcm_init_struct(void);
_int32 gcm_uninit_struct(void);
_int32 gcm_set_global_strategy(BOOL is_using);
BOOL gcm_is_global_strategy(void);

_int32 gcm_set_max_pipe_num(_u32 max_pipe_num);
_u32 gcm_get_mak_pipe_num(void);

_int32 gcm_set_max_connecting_pipe_num(_u32 max_connecting_pipe_num);

_int32 gcm_do_connect_dispatch(void);

_int32 gcm_register_connect_manager(struct tagCONNECT_MANAGER *p_connect_manager);
_int32 gcm_unregister_connect_manager(struct tagCONNECT_MANAGER *p_connect_manager);

_int32 pause_all_global_pipes(void);
_int32 resume_all_global_pipes(void);
_int32 pause_global_pipes(void);
_int32 resume_global_pipes(void);
BOOL is_pause_global_pipes(void);

_int32 gcm_register_pipe(struct tagCONNECT_MANAGER *p_connect_manager, DATA_PIPE *p_data_pipe);
_int32 gcm_unregister_pipe(struct tagCONNECT_MANAGER *p_connect_manager, DATA_PIPE *p_data_pipe);

_int32 gcm_register_working_pipe(struct tagCONNECT_MANAGER *p_connect_manager, DATA_PIPE *p_data_pipe);

_int32 gcm_register_candidate_res(struct tagCONNECT_MANAGER *p_connect_manager, RESOURCE *p_res);

BOOL gcm_is_need_more_res(void);
void gcm_add_res_num(void);
void gcm_sub_res_num(void);

_u32 gcm_get_res_num(void);
_u32 gcm_get_pipe_num(void);
BOOL gcm_is_pipe_full(void);

_u32 gcm_get_all_pipes_num();

struct tagGLOBAL_CONNECT_MANAGER* gcm_get_ptr(void);

#ifdef __cplusplus
}
#endif

#endif // !defined(__GLOBAL_CONNECT_DISPATCH_20081106)



