/*****************************************************************************
 *
 * Filename: 			dk_task.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2009/10/15
 *	
 * Purpose:				Define the basic structs of dk_task.
 *
 *****************************************************************************/


#if !defined(__dk_task_20091015)
#define __dk_task_20091015

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#ifdef _DK_QUERY

typedef enum tagDK_TASK_STATUE
{
	DK_TASK_STOPPED,
	DK_TASK_RUNNING,
		DK_TASK_FINISHED
}DK_TASK_STATUE;

struct tagDK_TASK;

typedef _int32 (*dk_task_logic_create)( struct tagDK_TASK **pp_dk_task );
typedef _int32 (*dk_task_logic_init)( struct tagDK_TASK *p_dk_task );
typedef _int32 (*dk_task_logic_update)( struct tagDK_TASK *p_dk_task );
typedef _int32 (*dk_task_logic_uninit )( struct tagDK_TASK *p_dk_task );
typedef _int32 (*dk_task_logic_destory )( struct tagDK_TASK *p_dk_task );


typedef struct tagDK_TASK
{
	_u32 _dk_type;
	_u8 *_target_ptr;
	_u32 _target_len;
	void *_user_ptr;
	DK_TASK_STATUE _status;
	_u32 _idle_ticks;
}DK_TASK;

_int32 dk_task_set_logic_func( _u32 dk_type, dk_task_logic_create logic_create, dk_task_logic_init logic_init, 
	dk_task_logic_update logic_update, dk_task_logic_uninit logic_uninit, dk_task_logic_destory logic_destory );

//platform for dk_manager
_int32 dk_create_task( _u32 dk_type, _u8 *p_key, _u32 key_len, void *p_user, DK_TASK **pp_dk_task );
_int32 dk_task_start( DK_TASK *p_dk_task );
_int32 dk_task_timeout( DK_TASK *p_dk_task );
_int32 dk_task_stop( DK_TASK *p_dk_task  );
_int32 dk_task_destory( DK_TASK *p_dk_task );


void dk_set_task_status( DK_TASK *p_dk_task, DK_TASK_STATUE status );
DK_TASK_STATUE dk_get_task_status( DK_TASK *p_dk_task );
void *dk_get_user_ptr( DK_TASK *p_dk_task );
_int32 dk_get_target( DK_TASK *p_dk_task, _u8 **pp_target, _u32 *p_target_len );

#endif

#ifdef __cplusplus
}
#endif

#endif // !defined(__dk_task_20091015)



