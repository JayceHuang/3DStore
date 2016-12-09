#include <stdio.h>
#include "utility/logid.h"
#define LOGID EM_LOGID_DOWNLOAD_TASK
#include "utility/logger.h"

#include <ooc/exception.h>

#include "Task.h"
#include "implement/Task.h"
#include "TaskP2sp.h"
#include "task_store/EmException.h"
#include "implement/TaskP2sp.h"

#include "download_manager/download_task_store.h"
#include "download_manager/download_task_data.h"

AllocateClass( CTaskP2sp, CTask );

/* Class virtual function prototypes
 */

static BOOL CTaskP2sp_CreateTaskInfo( CTaskP2sp, TASK_INFO ** );
static void CTaskP2sp_FreeTaskInfo( CTaskP2sp, TASK_INFO * );

static char * CTaskP2sp_GetUrl(CTaskP2sp);
static void CTaskP2sp_SetUrl(CTaskP2sp, const char *url, _u32 len);

static char * CTaskP2sp_GetRefUrl(CTaskP2sp);
static void CTaskP2sp_SetRefUrl(CTaskP2sp, const char *url, _u32 len);

static char * CTaskP2sp_GetName(CTaskP2sp);
static void CTaskP2sp_SetName(CTaskP2sp, const char *name, _u32 len);

static char * CTaskP2sp_GetPath(CTaskP2sp);
static void CTaskP2sp_SetPath(CTaskP2sp, const char *path, _u32 len);
    
static char * CTaskP2sp_GetTcid(CTaskP2sp, _u32 *len);
static void CTaskP2sp_SetTcid(CTaskP2sp, const char *tcid, _u32 len);
    
static void * CTaskP2sp_GetUserData(CTaskP2sp, _u32 *len);
static void CTaskP2sp_SetUserData(CTaskP2sp, const void *data, _u32 len);

static char * CTaskP2sp_GetTaskTag(CTaskP2sp);
static void CTaskP2sp_SetTaskTag(CTaskP2sp, const char *taskTag, _u32 len);


/* Class initializing
 */

static
void
CTaskP2sp_initialize( Class this )
{
    CTaskP2spVtable vtab = & CTaskP2spVtableInstance;

    vtab->CTask.CreateTaskInfo = (BOOL (*)( Object, TASK_INFO ** ) )CTaskP2sp_CreateTaskInfo;
    vtab->CTask.FreeTaskInfo = (void (*)( Object, TASK_INFO * ))CTaskP2sp_FreeTaskInfo;
    LOG_DEBUG("CTaskP2sp::initialize FreeTaskInfo:%x", (_u32)vtab->CTask.FreeTaskInfo);
    
    vtab->CTask.GetUrl = ( char * (*)(Object) ) CTaskP2sp_GetUrl;
    vtab->CTask.SetUrl = ( void (*)(Object, const char *url, _u32 len) ) CTaskP2sp_SetUrl;
    
    vtab->CTask.GetRefUrl = ( char * (*)(Object) ) CTaskP2sp_GetRefUrl;
    vtab->CTask.SetRefUrl = ( void (*)(Object, const char *url, _u32 len) ) CTaskP2sp_SetRefUrl;
    
    vtab->CTask.GetName = ( char * (*)(Object) ) CTaskP2sp_GetName;
    vtab->CTask.SetName = ( void (*)(Object, const char *url, _u32 len) ) CTaskP2sp_SetName;

    vtab->CTask.GetPath = ( char * (*)(Object) ) CTaskP2sp_GetPath;
    vtab->CTask.SetPath = ( void (*)(Object, const char *url, _u32 len) ) CTaskP2sp_SetPath;
    
    vtab->CTask.GetTcid = ( char * (*)(Object, _u32 *len) ) CTaskP2sp_GetTcid;
    vtab->CTask.SetTcid = ( void (*)(Object, const char *url, _u32 len) ) CTaskP2sp_SetTcid;
    
    vtab->CTask.GetUserData = ( void * (*)(Object, _u32 *len) ) CTaskP2sp_GetUserData;
    vtab->CTask.SetUserData = ( void (*)(Object, const void *url, _u32 len) ) CTaskP2sp_SetUserData;


	vtab->CTask.GetTaskTag = ( char * (*)(Object) ) CTaskP2sp_GetTaskTag;
    vtab->CTask.SetTaskTag = ( void (*)(Object, const char *taskTag, _u32 len) ) CTaskP2sp_SetTaskTag;
    
}

/* Class finalizing
 */

#ifndef OOC_NO_FINALIZE
static
void
CTaskP2sp_finalize( Class this )
{
}
#endif


/* Constructor
 */

static
void
CTaskP2sp_constructor( CTaskP2sp self, const void * params )
{
//  VectorIndex i;

    sd_assert( ooc_isInitialized( CTaskP2sp ) );
    EM_TASK *p_task = (EM_TASK *)params;    
    chain_constructor( CTaskP2sp, self, p_task );
}

/* Destructor
 */

static
void
CTaskP2sp_destructor( CTaskP2sp self, CTaskP2spVtable vtab )
{
}

/* Copy constuctor
 */

static
int
CTaskP2sp_copy( CTaskP2sp self, const CTaskP2sp from )
{
    

    return OOC_COPY_DONE;
}

/*  =====================================================
    Class member functions
 */


CTaskP2sp
CTaskP2sp_new( void )
{
    ooc_init_class( CTaskP2sp );

    return ooc_new( CTaskP2sp, NULL );
}

static 
BOOL 
CTaskP2sp_CreateTaskInfo( CTaskP2sp self, TASK_INFO ** pp_task_info)
{
    _int32 ret_val = dt_p2sp_task_malloc((EM_P2SP_TASK **)pp_task_info);
    if (ret_val!=SUCCESS)
    {
        sd_assert (SUCCESS!=ret_val ) ;
        ooc_throw(em_exception_new(err_out_of_memory));
    }
    LOG_DEBUG("CTaskP2sp::CreateTaskInfo result:%d, task address:%x", ret_val, (_u32)(*pp_task_info));
    return TRUE;
}

static 
void 
CTaskP2sp_FreeTaskInfo( CTaskP2sp self, TASK_INFO * p_task_info)
{
    if (p_task_info)
    {
        LOG_DEBUG("CTaskP2sp::FreeTaskInfo task info address:%x", (_u32)p_task_info);
        dt_uninit_p2sp_task_info((EM_P2SP_TASK *) p_task_info);
    }
}


static 
char * CTaskP2sp_GetUrl(CTaskP2sp self)
{
    CTask pTask = (CTask )self;
    EM_P2SP_TASK *p_p2sp_task = (EM_P2SP_TASK *)pTask->p_task->_task_info;
    return p_p2sp_task->_url;
}

static 
void CTaskP2sp_SetUrl(CTaskP2sp self, const char *url, _u32 len)
{
    CTask pTask = (CTask )self;
    EM_TASK *p_task = (EM_TASK *)pTask->p_task;
    EM_P2SP_TASK *p_p2sp_task = (EM_P2SP_TASK *)p_task->_task_info;

    _u32 _url_len = p_task->_task_info->_url_len_or_need_dl_num;
    _int32 ret_val = SetStrValue(&p_p2sp_task->_url, &_url_len, 
        url, len);
    p_task->_task_info->_url_len_or_need_dl_num = _url_len;

    sd_assert(ret_val==SUCCESS);
    return ;
}

static 
char * CTaskP2sp_GetRefUrl(CTaskP2sp self)
{
    CTask pTask = (CTask )self;
    EM_P2SP_TASK *p_p2sp_task = (EM_P2SP_TASK *)pTask->p_task->_task_info;
    return p_p2sp_task->_ref_url;
}

static 
void CTaskP2sp_SetRefUrl(CTaskP2sp self, const char *url, _u32 len)
{
    CTask pTask = (CTask )self;
    EM_TASK *p_task = (EM_TASK *)pTask->p_task;
    EM_P2SP_TASK *p_p2sp_task = (EM_P2SP_TASK *)p_task->_task_info;

    _u32 _url_len = p_task->_task_info->_ref_url_len_or_seed_path_len;
    _int32 ret_val = SetStrValue(&p_p2sp_task->_ref_url, &_url_len, 
        url, len);
    p_task->_task_info->_ref_url_len_or_seed_path_len = _url_len;
    
    if (SUCCESS == ret_val) p_task->_task_info->_have_ref_url= TRUE;
    
    sd_assert(ret_val==SUCCESS);
    return ;
}

static 
char * CTaskP2sp_GetName(CTaskP2sp self)
{
    CTask pTask = (CTask )self;
    EM_P2SP_TASK *p_p2sp_task = (EM_P2SP_TASK *)pTask->p_task->_task_info;
    return p_p2sp_task->_file_name;
}

static 
void CTaskP2sp_SetName(CTaskP2sp self, const char *name, _u32 len)
{
    CTask pTask = (CTask )self;
    EM_TASK *p_task = (EM_TASK *)pTask->p_task;
    EM_P2SP_TASK *p_p2sp_task = (EM_P2SP_TASK *)p_task->_task_info;

    _u32 name_len = p_task->_task_info->_file_name_len;
    _int32 ret_val = SetStrValue(&p_p2sp_task->_file_name, &name_len, 
        name, len);
    if (SUCCESS == ret_val) p_task->_task_info->_have_name = TRUE;

    p_task->_task_info->_file_name_len = name_len;
    
    sd_assert(ret_val==SUCCESS);
    return ;
}

static 
char * CTaskP2sp_GetPath(CTaskP2sp self)
{
    CTask pTask = (CTask )self;
    EM_P2SP_TASK *p_p2sp_task = (EM_P2SP_TASK *)pTask->p_task->_task_info;
    return p_p2sp_task->_file_path;
}

static 
void CTaskP2sp_SetPath(CTaskP2sp self, const char *path, _u32 len)
{
    CTask pTask = (CTask )self;
    EM_TASK *p_task = (EM_TASK *)pTask->p_task;
    EM_P2SP_TASK *p_p2sp_task = (EM_P2SP_TASK *)p_task->_task_info;

    _u32 path_len = p_task->_task_info->_file_path_len;
    _int32 ret_val = SetStrValue(&p_p2sp_task->_file_path, &path_len, 
        path, len);

    p_task->_task_info->_file_path_len = path_len;

    sd_assert(ret_val==SUCCESS);
    return ;
}

static 
char * CTaskP2sp_GetTcid(CTaskP2sp self, _u32 *len)
{
    CTask pTask = (CTask )self;
    EM_P2SP_TASK *p_p2sp_task = (EM_P2SP_TASK *)pTask->p_task->_task_info;
    *len = CID_SIZE;
    return p_p2sp_task->_tcid;
}

static 
void CTaskP2sp_SetTcid(CTaskP2sp self, const char *tcid, _u32 len)
{
    CTask pTask = (CTask )self;
    EM_TASK *p_task = (EM_TASK *)pTask->p_task;
    EM_P2SP_TASK *p_p2sp_task = (EM_P2SP_TASK *)p_task->_task_info;

    p_task->_task_info->_have_tcid = TRUE;
    if (len<=CID_SIZE)
    {
        sd_memset(p_p2sp_task->_tcid, 0, CID_SIZE);
        sd_strncpy(p_p2sp_task->_tcid, tcid, len);
        return ;
    }

    return ;
}
    
static 
void * CTaskP2sp_GetUserData(CTaskP2sp self, _u32 *len)
{
    CTask pTask = (CTask )self;
    EM_TASK *p_task = (EM_TASK *)pTask->p_task;
    EM_P2SP_TASK *p_p2sp_task = (EM_P2SP_TASK *)pTask->p_task->_task_info;
    *len = p_task->_task_info->_user_data_len;
    return p_p2sp_task->_user_data;
}

static 
void CTaskP2sp_SetUserData(CTaskP2sp self, const void *data, _u32 len)
{
    CTask pTask = (CTask )self;
    EM_TASK *p_task = (EM_TASK *)pTask->p_task;
    EM_P2SP_TASK *p_p2sp_task = (EM_P2SP_TASK *)p_task->_task_info;
    
    _int32 ret_val = SetStrValue((char **)&p_p2sp_task->_user_data, &p_task->_task_info->_user_data_len, 
        data, len);

    if (SUCCESS == ret_val) p_task->_task_info->_have_user_data= TRUE;
    
    sd_assert(ret_val==SUCCESS);
    return ;
}


static 
char * CTaskP2sp_GetTaskTag(CTaskP2sp self)
{
    CTask pTask = (CTask )self;
    EM_P2SP_TASK *p_p2sp_task = (EM_P2SP_TASK *)pTask->p_task->_task_info;
    return p_p2sp_task->_task_tag;
}

static 
void CTaskP2sp_SetTaskTag(CTaskP2sp self, const char *taskTag, _u32 len)
{
    CTask pTask = (CTask )self;
    EM_TASK *p_task = (EM_TASK *)pTask->p_task;
    EM_P2SP_TASK *p_p2sp_task = (EM_P2SP_TASK *)p_task->_task_info;

    _u32 task_tag_len = p_p2sp_task->_task_tag_len;
    _int32 ret_val = SetStrValue(&p_p2sp_task->_task_tag, &task_tag_len, 
        taskTag, len);
    //if (SUCCESS == ret_val) p_task->_task_info->_have_name = TRUE;

    //p_task->_task_info->_file_name_len = name_len;
    
    sd_assert(ret_val==SUCCESS);
    return ;
}


