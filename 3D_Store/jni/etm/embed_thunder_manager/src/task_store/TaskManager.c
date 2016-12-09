#include <stdio.h>
#include "utility/logid.h"
#define LOGID EM_LOGID_DOWNLOAD_TASK
#include "utility/logger.h"
#include "utility/mempool.h"

#include <ooc/exception.h>

#include "TaskManager.h"
#include "implement/TaskManager.h"

#include "scheduler/scheduler.h"
#include "download_manager/download_task_data.h"
#include "download_manager/download_task_impl.h"

AllocateClass( CTaskManager, Base );

/* Class virtual function prototypes
 */

static _u32 CTaskManager_GetMaxTaskId( CTaskManager );
static _u32* CTaskManager_GetOrderList( CTaskManager, _u32 *len );
static _u32* CTaskManager_GetRunningList ( CTaskManager );
static _u32 CTaskManager_GetRunningCount ( CTaskManager );
static BOOL CTaskManager_GetLoaded ( CTaskManager );
static BOOL CTaskManager_GetTasksLoaded ( CTaskManager );

static void CTaskManager_SetMaxTaskId ( CTaskManager, _u32 );
static void CTaskManager_SetOrderList( CTaskManager, _u32*, _u32 len );
static void CTaskManager_SetRunningList( CTaskManager, _u32*, _u32 len );
static void CTaskManager_SetLoaded ( CTaskManager, BOOL  );
static void CTaskManager_SetTasksLoaded ( CTaskManager, BOOL  );

static void CTaskManager_RunTask( CTaskManager,  CTask pTask );

static BOOL CTaskManager_AddTask( CTaskManager self,  CTask pTask );


/* Class initializing
 */

static
void
CTaskManager_initialize( Class this )
{
    CTaskManagerVtable vtab = & CTaskManagerVtableInstance;
    
    vtab->GetMaxTaskId      =   (_u32 (*)( Object ))    CTaskManager_GetMaxTaskId;
    vtab->GetOrderList  =   (_u32* (*)( Object, _u32 * ))   CTaskManager_GetOrderList;
    vtab->GetRunningList = (_u32* (*)( Object ))CTaskManager_GetRunningList;
    vtab->GetRunningCount = (_u32 (*)( Object ))CTaskManager_GetRunningCount;

    vtab->SetMaxTaskId = (  void (* )( Object, _u32 )  ) CTaskManager_SetMaxTaskId;
    vtab->SetOrderList = (  void (* )( Object, _u32*, _u32 ) ) CTaskManager_SetOrderList;
    vtab->SetRunningList = (  void (* )( Object, _u32*, _u32 ) ) CTaskManager_SetRunningList;
    
    vtab->GetLoaded = ( BOOL (* )( Object ) )CTaskManager_GetLoaded ;
    vtab->SetLoaded = ( void (* ) ( Object, BOOL ) )CTaskManager_SetLoaded;

    vtab->GetTasksLoaded = ( BOOL (* )( Object ) )CTaskManager_GetTasksLoaded ;
    vtab->SetTasksLoaded = ( void (* ) ( Object, BOOL ) )CTaskManager_SetTasksLoaded;

    vtab->RunTask = (void (*)( Object, CTask ))CTaskManager_RunTask;
    
    vtab->AddTask   =   (BOOL (*)( Object, CTask )) CTaskManager_AddTask;
}

/* Class finalizing
 */

#ifndef OOC_NO_FINALIZE
static
void
CTaskManager_finalize( Class this )
{
}
#endif


/* Constructor
 */

static
void
CTaskManager_constructor( CTaskManager self, const void * params )
{
//  VectorIndex i;

    sd_assert( ooc_isInitialized( CTaskManager ) );

    chain_constructor( CTaskManager, self, NULL );

    memset(self->running_tasks_array, 0, sizeof(self->running_tasks_array));

    self->_dling_task_order = NULL;
    self->task_num = 0;
    self->max_task_id = 0;
    self->loaded = FALSE;

    //self->p_task = NULL;
}

/* Destructor
 */

static
void
CTaskManager_destructor( CTaskManager self, CTaskManagerVtable vtab )
{
}

/* Copy constuctor
 */

static
int
CTaskManager_copy( CTaskManager self, const CTaskManager from )
{
    //self->p_task  = from->p_task;

    return OOC_COPY_DONE;
}

/*  =====================================================
    Class member functions
 */


CTaskManager
CTaskManager_new( void )
{
    ooc_init_class( CTaskManager );

    return ooc_new( CTaskManager, NULL );
}

static 
_u32 
CTaskManager_GetMaxTaskId( CTaskManager self)
{
    return self->max_task_id;
}

static 
_u32* 
CTaskManager_GetOrderList( CTaskManager self, _u32 *len )
{
    *len = self->task_num;
    return self->_dling_task_order;
}

static 
_u32* CTaskManager_GetRunningList ( CTaskManager self )
{
    return self->running_tasks_array;
}

static 
_u32 CTaskManager_GetRunningCount ( CTaskManager self )
{
    return MAX_ALOW_TASKS;
}

static BOOL CTaskManager_GetLoaded ( CTaskManager self)
{
    return self->loaded;
}

static BOOL CTaskManager_GetTasksLoaded ( CTaskManager self)
{
    return self->tasks_loaded;
}

static void CTaskManager_SetMaxTaskId ( CTaskManager self, _u32 num)
{
    self->max_task_id = num;
}

static void CTaskManager_SetOrderList( CTaskManager self, _u32* data, _u32 len )
{
	_int32 ret_val = 0;
    if (len>MAX_TASK_NUM) return;
    if (len>self->task_num)
    {
    	_u32 *new_buffer = NULL;
    	ret_val = sd_malloc(len*sizeof(_u32), (void**)&new_buffer);
    	sd_assert(ret_val==SUCCESS);
    	if (ret_val!=SUCCESS) return;

    	if (self->_dling_task_order)
    	{
    		sd_free(self->_dling_task_order);
    		self->_dling_task_order = NULL;
    	}
    	self->_dling_task_order = new_buffer;
    }
    sd_memcpy((void *)self->_dling_task_order, (void *)data, len*sizeof(_u32));
    self->task_num = len;
}

static void CTaskManager_SetRunningList( CTaskManager self, _u32* data, _u32 len )
{
    if (len>MAX_ALOW_TASKS) return;

    sd_memset((void *)self->running_tasks_array, 0, sizeof(_u32)*MAX_ALOW_TASKS);
    sd_memcpy((void *)self->running_tasks_array, (void *)data, len*sizeof(_u32));
}

static void CTaskManager_SetLoaded ( CTaskManager self, BOOL  bVal)
{
    self->loaded = bVal;
}

static void CTaskManager_SetTasksLoaded ( CTaskManager self, BOOL  bVal)
{
    self->tasks_loaded = bVal;
}

static void CTaskManager_RunTask( CTaskManager self, CTask pTask )
{
    _u32 _task_id = CTaskVirtual( pTask )->GetId((Object)pTask);
   LOG_DEBUG("CTaskManager::RunTask task id : %u ", _task_id);
    
    if((em_is_task_auto_start())&&(CTaskVirtual(pTask)->GetState((Object)pTask) ==TS_TASK_PAUSED))
    {
        _u32 *running_task_ids = CTaskManagerVirtual(self)->GetRunningList((Object)self);

        _int32 i = 0;
        if(running_task_ids!=NULL)
        {
            for(i=0; i<CTaskManagerVirtual(self)->GetRunningCount((Object)self); i++)
            {
                if(running_task_ids[i]==_task_id)
                {
                    CTaskVirtual(pTask)->Run((Object)pTask);
                }
            }
        }
    }
    
}

static BOOL CTaskManager_AddTask( CTaskManager self,  CTask pTask )
{

    CTaskVirtual(pTask)->CorrectLoadedData((Object)pTask);
    CTaskManager_RunTask(self, pTask);

    EM_TASK *p_task = CTaskVirtual(pTask)->GetAttachedData((Object)pTask);
    EM_TASK *p_task_tmp2 = NULL;
    TASK_INFO *p_task_info = p_task->_task_info;    
    _int32 ret_val = SUCCESS;

   LOG_DEBUG("CTaskManager::AddTask task id : %u, state:%u ", p_task->_task_info->_task_id, p_task->_task_info->_state);
    
    /* add to all_task_map */
    ret_val = dt_add_task_to_map(p_task);
    if(ret_val!=SUCCESS)
    {
        if(ret_val==MAP_DUPLICATE_KEY)
        {
            LOG_ERROR("Need delete the old one!");
            p_task_tmp2 = dt_get_task_from_map(p_task_info->_task_id);
            sd_assert(p_task_tmp2!=NULL);
            if(p_task_tmp2==NULL) return FALSE;
            dt_remove_task_eigenvalue(p_task_tmp2->_task_info->_type,p_task_tmp2->_task_info->_eigenvalue);
            if(p_task_tmp2->_task_info->_file_name_eigenvalue!=0)
            {
                dt_remove_file_name_eigenvalue(p_task_tmp2->_task_info->_file_name_eigenvalue);
            }
            /* remove from all_tasks_map */
            dt_remove_task_from_map(p_task_tmp2);
            /* delete from task_file */
            dt_detete_task_in_file(p_task_tmp2);

            dt_task_info_free(p_task_tmp2->_task_info);
            dt_task_free(p_task_tmp2);

            ret_val = dt_add_task_to_map(p_task);
            sd_assert(ret_val==SUCCESS);
        }
        else
        {
            return FALSE;
        }
    }

    /* add to eigenvalue map */
    ret_val = dt_add_task_eigenvalue(p_task_info->_type,p_task_info->_eigenvalue,p_task_info->_task_id);
    if(ret_val!=SUCCESS)
    {
        LOG_ERROR("dt_add_task_eigenvalue,FAILED!");
        /* remove from all_tasks_map */
        dt_remove_task_from_map(p_task);
        return FALSE;
    }

    if(p_task_info->_file_name_eigenvalue!=0)
    {
        dt_add_file_name_eigenvalue(p_task_info->_file_name_eigenvalue,p_task_info->_task_id);
    }

    //避免任务id自增值小于当前的任务id
    if ( CTaskManagerVirtual(self)->GetMaxTaskId((Object)self)< CTaskVirtual(pTask)->GetId((Object)pTask) )
    {
        CTaskManagerVirtual(self)->SetMaxTaskId((Object)self, CTaskVirtual(pTask)->GetId((Object)pTask) );
    }
    
    return TRUE;
}

