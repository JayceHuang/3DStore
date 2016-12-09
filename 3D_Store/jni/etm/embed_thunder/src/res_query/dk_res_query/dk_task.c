#include "utility/define.h"
#ifdef _DK_QUERY

#include "dk_task.h"
#include "routing_table.h"
#include "dk_setting.h"

#include "utility/logid.h"
#define LOGID LOGID_DK_QUERY
#include "utility/logger.h"

#include "utility/mempool.h"

#define DK_LOGIC_CREATE 0
#define DK_LOGIC_INIT 1
#define DK_LOGIC_UPDATE 2
#define DK_LOGIC_UNINIT 3
#define DK_LOGIC_DESTORY 4
#define DK_LOGIC_FUNC_COUNT 5

static void *g_dk_logic_func[DK_TYPE_COUNT][DK_LOGIC_FUNC_COUNT] = { { NULL, NULL, NULL }, { NULL, NULL, NULL } };

_int32 dk_task_set_logic_func( _u32 dk_type, dk_task_logic_create logic_create, dk_task_logic_init logic_init, 
	dk_task_logic_update logic_update, dk_task_logic_uninit logic_uninit, dk_task_logic_destory logic_destory )
{
	if( dk_type >= DK_TYPE_COUNT )
	{
		sd_assert( FALSE );
		return -1;
	}
	
	g_dk_logic_func[dk_type][DK_LOGIC_CREATE] = logic_create;
	g_dk_logic_func[dk_type][DK_LOGIC_INIT] = logic_init;
	g_dk_logic_func[dk_type][DK_LOGIC_UPDATE] = logic_update;
	g_dk_logic_func[dk_type][DK_LOGIC_UNINIT] = logic_uninit;
	g_dk_logic_func[dk_type][DK_LOGIC_DESTORY] = logic_destory;
	return SUCCESS;
}

_int32 dk_create_task( _u32 dk_type, _u8 *p_key, _u32 key_len, void *p_user, DK_TASK **pp_dk_task )
{
	_int32 ret_val = SUCCESS;
	DK_TASK *p_dk_task = NULL;
	dk_task_logic_create logic_create = g_dk_logic_func[dk_type][DK_LOGIC_CREATE];
	
	LOG_DEBUG( "dk_create_task, dk_type:%u", dk_type );

	*pp_dk_task = NULL;

	ret_val = logic_create( &p_dk_task );
	CHECK_VALUE( ret_val ); 

	ret_val = sd_malloc( key_len, (void **)&p_dk_task->_target_ptr );
	if( ret_val != SUCCESS )
	{
		SAFE_DELETE( p_dk_task );
		CHECK_VALUE( ret_val );
	}

	sd_memcpy( p_dk_task->_target_ptr, p_key, key_len );
	
	p_dk_task->_dk_type = dk_type;
	p_dk_task->_target_len = key_len;
	p_dk_task->_user_ptr = p_user;
	p_dk_task->_idle_ticks = 0;		
	
	dk_set_task_status( p_dk_task, DK_TASK_STOPPED );
	*pp_dk_task = p_dk_task;
	
	return SUCCESS;
}

_int32 dk_task_start( DK_TASK *p_dk_task )
{
	_int32 ret_val = SUCCESS;

	dk_task_logic_init logic_init = g_dk_logic_func[p_dk_task->_dk_type][DK_LOGIC_INIT];

	LOG_DEBUG( "dk_task_start, p_dk_task:0x%x", p_dk_task );

	if( dk_get_task_status( p_dk_task ) == DK_TASK_RUNNING ) return SUCCESS;

	ret_val = logic_init( p_dk_task );
	CHECK_VALUE( ret_val );	
	
	dk_set_task_status( p_dk_task, DK_TASK_RUNNING );

	return SUCCESS;
}

_int32 dk_task_timeout( DK_TASK *p_dk_task )
{	
	dk_task_logic_update logic_update = g_dk_logic_func[p_dk_task->_dk_type][DK_LOGIC_UPDATE];
	dk_task_logic_update logic_uninit = g_dk_logic_func[p_dk_task->_dk_type][DK_LOGIC_UNINIT];

	LOG_DEBUG( "dk_task_timeout, p_dk_task:0x%x", p_dk_task );

	if( dk_get_task_status( p_dk_task ) ==  DK_TASK_RUNNING )
	{
		logic_update( p_dk_task );
	}
	if( dk_get_task_status( p_dk_task ) ==  DK_TASK_FINISHED )
	{
		if( p_dk_task->_idle_ticks == 0 )
		{
			logic_uninit( p_dk_task );
		}
		p_dk_task->_idle_ticks++;
		if( p_dk_task->_idle_ticks > dk_res_query_interval() )
		{
			dk_task_start( p_dk_task );
			p_dk_task->_idle_ticks = 0;
		}
	}	
	return SUCCESS;
}


_int32 dk_task_stop( DK_TASK *p_dk_task  )
{
	dk_task_logic_update logic_uninit = g_dk_logic_func[p_dk_task->_dk_type][DK_LOGIC_UNINIT];

	LOG_DEBUG( "dk_task_stop, p_dk_task:0x%x", p_dk_task );
	
	if( dk_get_task_status( p_dk_task ) == DK_TASK_RUNNING )
	{
		logic_uninit( p_dk_task );
	}
	dk_set_task_status( p_dk_task, DK_TASK_STOPPED );
	return SUCCESS;
}

_int32 dk_task_destory( DK_TASK *p_dk_task )
{
	dk_task_logic_destory logic_destory = g_dk_logic_func[p_dk_task->_dk_type][DK_LOGIC_DESTORY];
	
	LOG_DEBUG( "dk_task_destory, p_dk_task:0x%x", p_dk_task );

	if( dk_get_task_status( p_dk_task ) == DK_TASK_RUNNING )
	{
		dk_task_stop( p_dk_task );
	}
	SAFE_DELETE( p_dk_task->_target_ptr );
	logic_destory( p_dk_task );
	return SUCCESS;
}

void dk_set_task_status( DK_TASK *p_dk_task, DK_TASK_STATUE status )
{
	p_dk_task->_status = status;
 	LOG_DEBUG( "dk_set_task_status, p_dk_task:0x%x, new_status:%d", p_dk_task, status );   
}

DK_TASK_STATUE dk_get_task_status( DK_TASK *p_dk_task )
{
	LOG_DEBUG( "dk_get_task_status, p_dk_task:0x%x, status:%d", p_dk_task, p_dk_task->_status );
	return p_dk_task->_status;
}

void *dk_get_user_ptr( DK_TASK *p_dk_task )
{
	return p_dk_task->_user_ptr;
}

_int32 dk_get_target( DK_TASK *p_dk_task, _u8 **pp_target, _u32 *p_target_len )
{
	LOG_DEBUG( "dk_get_target, p_dk_task:0x%x", p_dk_task );
	if( p_dk_task->_target_len == 0 || p_dk_task->_target_ptr == NULL )
	{
		sd_assert( FALSE );
		return -1;
	}
	*pp_target = p_dk_task->_target_ptr;
	*p_target_len = p_dk_task->_target_len;
	return SUCCESS;
}

#endif

