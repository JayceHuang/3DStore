#include "utility/define.h"
#ifdef _DK_QUERY
#ifdef ENABLE_EMULE

#include "kad_task.h"

#include "utility/logid.h"
#define LOGID LOGID_DK_QUERY
#include "utility/logger.h"
#include "utility/mempool.h"

_int32 kad_task_logic_create( DK_TASK **pp_dk_task )
{
	LOG_DEBUG( "kad_task_logic_create");
	return sd_malloc( sizeof( KAD_TASK ), (void **)pp_dk_task );
}

_int32 kad_task_logic_init( DK_TASK *p_dk_task )
{
    _int32 ret_val = SUCCESS;
	KAD_TASK *p_kad_task = (KAD_TASK *)p_dk_task;
	FIND_SOURCE_HANDLER *p_find_source_handler = &p_kad_task->_find_source_handler;
    _u8 *p_target = NULL;
    _u32 target_len = 0;
    
 	LOG_DEBUG( "kad_task_logic_init, task_ptr:0x%x", p_dk_task );

    // target ==> file_id ==> file_hash
    ret_val = dk_get_target( &p_kad_task->_dk_task, &p_target, &target_len );
    CHECK_VALUE( ret_val );	
    
	return find_source_init( p_find_source_handler, p_target, target_len, p_dk_task->_user_ptr );
}

_int32 kad_task_logic_update( DK_TASK *p_dk_task )
{
	KAD_TASK *p_kad_task = (KAD_TASK *)p_dk_task;
	FIND_SOURCE_HANDLER *p_find_source_handler = &p_kad_task->_find_source_handler;

 	LOG_DEBUG( "kad_task_logic_update, task_ptr:0x%x", p_dk_task );
	find_source_update( p_find_source_handler );	
	if( find_source_get_status( p_find_source_handler) == FIND_NODE_FINISHED )
	{
		dk_set_task_status( p_dk_task, DK_TASK_FINISHED);
	}
	return SUCCESS;
}	

_int32 kad_task_logic_uninit( DK_TASK *p_dk_task )
{
	KAD_TASK *p_kad_task = (KAD_TASK *)p_dk_task;
	FIND_SOURCE_HANDLER *p_find_source_handler = &p_kad_task->_find_source_handler;

 	LOG_DEBUG( "kad_task_logic_uninit, task_ptr:0x%x", p_dk_task );

	return find_source_uninit( p_find_source_handler );	

}

_int32 kad_task_logic_destory( DK_TASK *p_dk_task )
{
	KAD_TASK *p_kad_task = (KAD_TASK *)p_dk_task;
    
 	LOG_DEBUG( "kad_task_logic_destory, task_ptr:0x%x", p_dk_task );
	SAFE_DELETE( p_kad_task );
	return SUCCESS;
}

void kad_task_set_file_size( KAD_TASK *p_kad_task, _u64 file_size )
{
    find_source_set_file_size( &p_kad_task->_find_source_handler, file_size );
}

#endif
#endif
