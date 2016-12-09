/*****************************************************************************
 *
 * Filename: 			kad_task.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2009/11/15
 *	
 * Purpose:				Define the basic structs of dht_task.
 *
 *****************************************************************************/


#if !defined(__kad_task_20091115)
#define __kad_task_20091115

#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#ifdef _DK_QUERY
#ifdef ENABLE_EMULE

#include "res_query/dk_res_query/dk_task.h"
#include "res_query/dk_res_query/find_source_handler.h"



typedef struct tagKAD_TASK
{
	DK_TASK _dk_task;
	FIND_SOURCE_HANDLER _find_source_handler;
}KAD_TASK;

_int32 kad_task_logic_create( DK_TASK **pp_dk_task );
_int32 kad_task_logic_init( DK_TASK *p_dk_task );
_int32 kad_task_logic_update( DK_TASK *p_dk_task );
_int32 kad_task_logic_uninit( DK_TASK *p_dk_task );
_int32 kad_task_logic_destory( DK_TASK *p_dk_task );
void kad_task_set_file_size( KAD_TASK *p_kad_task, _u64 file_size );
#endif
#endif


#ifdef __cplusplus
}
#endif

#endif // !defined(__kad_task_20091115)







