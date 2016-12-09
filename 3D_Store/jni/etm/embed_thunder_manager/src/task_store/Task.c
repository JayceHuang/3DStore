#include <stdio.h>
#include "utility/logid.h"
#define LOGID EM_LOGID_DOWNLOAD_TASK
#include "utility/logger.h"

#include <ooc/exception.h>

#include "Task.h"
#include "implement/Task.h"

#include "download_manager/download_task_store.h"
#include "download_manager/download_task_data.h"
#include "scheduler/scheduler.h"

#include "EmException.h"
#include "utility/mempool.h"

AllocateClass( CTask, Base );

/* Class virtual function prototypes
 */

static _u32 CTask_GetId(CTask self);
static TASK_STATE CTask_GetState(CTask self);
static EM_TASK *CTask_GetAttachedData(CTask self);

static void CTask_Attach( CTask, EM_TASK * );
static EM_TASK *CTask_Detach( CTask );

static BOOL CTask_CreateTaskInfo( CTask self, TASK_INFO ** );
static void CTask_FreeTaskInfo( CTask self, TASK_INFO * );

static BOOL CTask_CorrectLoadedData(CTask self);

static BOOL CTask_Run(CTask self);

//property get set
static char * CTask_GetUrl(CTask);
static void CTask_SetUrl(CTask, const char *url, _u32 len);

static char * CTask_GetRefUrl(CTask);
static void CTask_SetRefUrl(CTask, const char *url, _u32 len);

static char * CTask_GetName(CTask);
static void CTask_SetName(CTask, const char *name, _u32 len);

static char * CTask_GetPath(CTask);
static void CTask_SetPath(CTask, const char *path, _u32 len);
    
static char * CTask_GetTcid(CTask, _u32 *len);
static void CTask_SetTcid(CTask, const char *tcid, _u32 len);
    
static void * CTask_GetUserData(CTask, _u32 *len);
static void CTask_SetUserData(CTask, const void *data, _u32 len);


static char * CTask_GetTaskTag(CTask);
static void CTask_SetTaskTag(CTask, const char *name, _u32 len);

/* Class initializing
 */

static
void
CTask_initialize( Class this )
{
    CTaskVtable vtab = & CTaskVtableInstance;

    vtab->GetId = (_u32 (*)(Object)) CTask_GetId;
    vtab->GetState = (TASK_STATE (*)(Object)) CTask_GetState;
    vtab->GetAttachedData = ( EM_TASK * (*)(Object) )CTask_GetAttachedData;

    vtab->Attach = ( void (*)(Object, EM_TASK *) )CTask_Attach;
    vtab->Detach = ( EM_TASK * (*)(Object) )CTask_Detach;

    vtab->CreateTaskInfo = (BOOL (*)( Object, TASK_INFO ** ) )CTask_CreateTaskInfo;
    vtab->FreeTaskInfo = (void (*)( Object, TASK_INFO * ))CTask_FreeTaskInfo;
    LOG_DEBUG("CTask::initialize FreeTaskInfo:%x", (_u32)vtab->FreeTaskInfo);
    
    vtab->CorrectLoadedData = (BOOL (*)(Object)) CTask_CorrectLoadedData;

    vtab->GetUrl = ( char * (*)(Object) ) CTask_GetUrl;
    vtab->SetUrl = ( void (*)(Object, const char *url, _u32 len) ) CTask_SetUrl;
    
    vtab->GetRefUrl = ( char * (*)(Object) ) CTask_GetRefUrl;
    vtab->SetRefUrl = ( void (*)(Object, const char *url, _u32 len) ) CTask_SetRefUrl;
    
    vtab->GetName = ( char * (*)(Object) ) CTask_GetName;
    vtab->SetName = ( void (*)(Object, const char *url, _u32 len) ) CTask_SetName;

    vtab->GetPath = ( char * (*)(Object) ) CTask_GetPath;
    vtab->SetPath = ( void (*)(Object, const char *url, _u32 len) ) CTask_SetPath;
    
    vtab->GetTcid = ( char * (*)(Object, _u32 *len) ) CTask_GetTcid;
    vtab->SetTcid = ( void (*)(Object, const char *url, _u32 len) ) CTask_SetTcid;
    
    vtab->GetUserData = ( void * (*)(Object, _u32 *len) ) CTask_GetUserData;
    vtab->SetUserData = ( void (*)(Object, const void *url, _u32 len) ) CTask_SetUserData;

	vtab->GetTaskTag = ( char * (*)(Object) ) CTask_GetTaskTag;
    vtab->SetTaskTag = ( void (*)(Object, const char *taskTag, _u32 len) ) CTask_SetTaskTag;
    
    vtab->Run = ( BOOL (*)(Object ) )CTask_Run;
}

/* Class finalizing
 */

#ifndef OOC_NO_FINALIZE
static
void
CTask_finalize( Class this )
{
}
#endif


/* Constructor
 */

static
void
CTask_constructor( CTask self, const void * params )
{
//  VectorIndex i;

    sd_assert( ooc_isInitialized( CTask ) );
    
    chain_constructor( CTask, self, NULL );

    EM_TASK *p_task = (EM_TASK *)params;
    
    if (!p_task)
    {
        
        _int32 ret_val = dt_task_malloc(&p_task);
        sd_assert(p_task!=NULL);
        if (SUCCESS!=ret_val || !p_task) 
        {
            ooc_throw( em_exception_new((int)err_out_of_memory) );
        }
        LOG_DEBUG("CTask::constructor p_task : %x ", (_u32)p_task);
        
        TASK_INFO *p_task_info = NULL;
        BOOL bRet = CTaskVirtual(self)->CreateTaskInfo((Object)self, &p_task_info);
        sd_assert(bRet);
        if(!bRet)
        {
            dt_task_free(p_task);
            sd_assert (SUCCESS!=ret_val ) ;
            ooc_throw( em_exception_new((int)err_out_of_memory) );
        }
        p_task->_task_info = p_task_info;
        
    }
    
    self->p_task = (EM_TASK *)p_task;
}

/* Destructor
 */

static
void
CTask_destructor( CTask self, CTaskVtable vtab )
{
    EM_TASK *pFree = self->p_task;
    LOG_DEBUG("CTask::destructor p_task : %x ", (_u32)pFree);
    if (pFree)
    {
        //所有的任务加载时，都根据它的类型分配自己的内存
        if (pFree->_task_info)
        {
            LOG_DEBUG("CTask::destructor p_task : %x before free task info:%x, fun:%x", (_u32)pFree, (_u32)pFree->_task_info, (_u32)vtab->FreeTaskInfo);
            vtab->FreeTaskInfo((Object)self, pFree->_task_info);
            pFree->_task_info = NULL;
        }
        dt_task_free(pFree);
        self->p_task = NULL;
    }
}

/* Copy constuctor
 */

static
int
CTask_copy( CTask self, const CTask from )
{
    self->p_task  = from->p_task;

    return OOC_COPY_DONE;
}

/*  =====================================================
    Class member functions
 */


CTask
CTask_new( void )
{
    ooc_init_class( CTask );

    return ooc_new( CTask, NULL );
}

_u32 CTask_GetId(CTask self)
{
    if (self->p_task && self->p_task->_task_info) return self->p_task->_task_info->_task_id;
    return 0;
}

static 
TASK_STATE CTask_GetState(CTask self)
{
    if (self->p_task && self->p_task->_task_info) return self->p_task->_task_info->_state;
    return TS_TASK_DELETED;
}

static EM_TASK *CTask_GetAttachedData(CTask self)
{
    return self->p_task;
}

static 
BOOL 
CTask_CreateTaskInfo( CTask self, TASK_INFO ** pp_task_info)
{
    LOG_DEBUG("CTask::CreateTaskInfo ");
    ooc_throw( exception_new(err_undefined_virtual) );
}

static 
void 
CTask_FreeTaskInfo( CTask self, TASK_INFO * p_task_info)
{
    LOG_DEBUG("CTask::FreeTaskInfo p_task_info:%x", (_u32)p_task_info);
    ooc_throw( exception_new(err_undefined_virtual) );
}

static void CTask_Attach( CTask self, EM_TASK * p_task)
{
   LOG_DEBUG("CTask::Attach task id : %u ", p_task->_task_info->_task_id);
    EM_TASK *pFree = self->p_task;
    self->p_task= p_task;
    if (pFree)
    {
        /*if ( pFree && pFree->_task_info )
        {
            dt_remove_task_eigenvalue(pFree->_task_info->_type, pFree->_task_info->_eigenvalue);
            if(pFree->_task_info->_file_name_eigenvalue!=0)
            {
                dt_remove_file_name_eigenvalue(pFree->_task_info->_file_name_eigenvalue);
            }
            // remove from all_tasks_map 
            dt_remove_task_from_map(pFree);
            // disable from task_file 
            dt_disable_task_in_file(pFree);
        }
        else */if (pFree->_task_info)
        {
            dt_task_info_free(pFree->_task_info);
        }

        dt_task_free(pFree);
        self->p_task = NULL;
    }
    
}

static EM_TASK *CTask_Detach( CTask self )
{
   LOG_DEBUG("CTask::Detach task id : %u ", self->p_task->_task_info->_task_id);
    EM_TASK *pRelease = self->p_task;
    self->p_task= NULL;
    return pRelease;
}

static BOOL CTask_CorrectLoadedData(CTask self)
{
   LOG_DEBUG("CTask::CorrectLoadedData task id : %u, state:%u ", self->p_task->_task_info->_task_id, self->p_task->_task_info->_state);

    _int32 i = 0;
    //self->p_task->_task_info->_full_info= FALSE;
    if((self->p_task->_task_info->_state ==TS_TASK_RUNNING)||(self->p_task->_task_info->_state ==TS_TASK_WAITING))
    {
        LOG_DEBUG("CTask::CorrectLoadedData auto start flag:%d ", em_is_task_auto_start() );
        if(em_is_task_auto_start()==TRUE)
        {
            if(dt_is_vod_task(self->p_task))
            {
                self->p_task->_task_info->_state =TS_TASK_PAUSED;
                self->p_task->_change_flag|=CHANGE_STATE;
            }
            else if(self->p_task->_task_info->_state !=TS_TASK_WAITING)
            {
                self->p_task->_task_info->_state =TS_TASK_WAITING;
                dt_have_task_waitting();
                self->p_task->_change_flag|=CHANGE_STATE;
            }
        }
        else
        {
            self->p_task->_task_info->_state =TS_TASK_PAUSED;
            self->p_task->_change_flag|=CHANGE_STATE;
        }
    }

    if((self->p_task->_task_info->_state ==TS_TASK_PAUSED) && (self->p_task->_task_info->_file_size!=0 && 
        self->p_task->_task_info->_downloaded_data_size == self->p_task->_task_info->_file_size))
    {
        self->p_task->_task_info->_state = TS_TASK_SUCCESS;
    }
    return TRUE;
}

static 
BOOL CTask_Run(CTask self)
{
   LOG_DEBUG("CTask::Run task id : %u ", self->p_task->_task_info->_task_id);
   
    self->p_task->_task_info->_state =TS_TASK_WAITING;
    dt_have_task_waitting();
    self->p_task->_change_flag|=CHANGE_STATE;
        
    return TRUE;
}

_int32 SetStrValue(char **dest, _u32 *dest_len, const char *src, _u32 src_len)
{
    _int32 ret_val = SUCCESS;
    if (*dest == src)
    {
        *dest_len = src_len;
        return ret_val;
    }
    
    if (*dest_len <= src_len)
    {
        sd_free(*dest);
        *dest = NULL;
        ret_val = sd_malloc(src_len+1, (void **)dest);
        if (SUCCESS != ret_val) 
        {
            return -1;
        }
    }
    sd_strncpy(*dest, src, src_len);
    (*dest)[src_len] = '\0';
    *dest_len = src_len;
    
    return ret_val;
}


static 
char * CTask_GetUrl(CTask self)
{
    return NULL;
}

static 
void CTask_SetUrl(CTask self, const char *url, _u32 len)
{
}

static 
char * CTask_GetRefUrl(CTask self)
{
    return NULL;
}

static 
void CTask_SetRefUrl(CTask self, const char *url, _u32 len)
{
}

static 
char * CTask_GetName(CTask self)
{
    return NULL;
}

static 
void CTask_SetName(CTask self, const char *name, _u32 len)
{
}

static 
char * CTask_GetPath(CTask self)
{
    return NULL;
}

static 
void CTask_SetPath(CTask self, const char *path, _u32 len)
{
}

static 
char * CTask_GetTcid(CTask self, _u32 *len)
{
    return NULL;
}

static 
void CTask_SetTcid(CTask self, const char *tcid, _u32 len)
{
}
    
static 
void * CTask_GetUserData(CTask self, _u32 *len)
{
    return NULL;
}

static 
void CTask_SetUserData(CTask self, const void *data, _u32 len)
{
}

static 
char * CTask_GetTaskTag(CTask self)
{
    return NULL;
}

static 
void CTask_SetTaskTag(CTask self, const char *name, _u32 len)
{
}



