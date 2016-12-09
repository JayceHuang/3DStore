#include <stdio.h>
#include "utility/mempool.h"
#include "utility/logid.h"
#define LOGID EM_LOGID_DOWNLOAD_TASK
#include "utility/logger.h"
#include "utility/string.h"

#include <ooc/exception.h>

#include "Task.h"
#include "implement/Task.h"
#include "TaskBt.h"
#include "task_store/EmException.h"
#include "implement/TaskBt.h"

#include "download_manager/download_task_store.h"
#include "download_manager/download_task_data.h"


AllocateClass( CTaskBt, CTask );

/* Class virtual function prototypes
 */
static BOOL CTaskBt_CreateTaskInfo( CTaskBt, TASK_INFO ** );
static void CTaskBt_FreeTaskInfo( CTaskBt, TASK_INFO * );

static char * CTaskBt_GetName(CTaskBt);
static void CTaskBt_SetName(CTaskBt, const char *name, _u32 len);

static char * CTaskBt_GetPath(CTaskBt);
static void CTaskBt_SetPath(CTaskBt, const char *path, _u32 len);

static void * CTaskBt_GetUserData(CTaskBt, _u32 *len);
static void CTaskBt_SetUserData(CTaskBt, const void *data, _u32 len);

static _u16 * CTaskBt_GetNeedDlFile(CTaskBt, _u16 *len);
static void CTaskBt_SetNeedDlFile(CTaskBt, const _u16 *data, _u16 len);

static char * CTaskBt_GetSeedFile(CTaskBt);
static void CTaskBt_SetSeedFile(CTaskBt, const char *seed_file, _u32 len);

static BT_FILE * CTaskBt_GetSubFile(CTaskBt self, _u16 index);
static void CTaskBt_SetSubFile(CTaskBt self, _u16 index, BT_FILE *sub_file);

static char * CTaskBt_GetTaskTag(CTaskBt);
static void CTaskBt_SetTaskTag(CTaskBt, const char *name, _u32 len);


/* Class initializing
 */

static
void
CTaskBt_initialize( Class this )
{
    CTaskBtVtable vtab = & CTaskBtVtableInstance;
    
    vtab->CTask.CreateTaskInfo = (BOOL (*)( Object, TASK_INFO ** ) )CTaskBt_CreateTaskInfo;
    vtab->CTask.FreeTaskInfo = (void (*)( Object, TASK_INFO * ))CTaskBt_FreeTaskInfo;
    
    vtab->CTask.GetName = ( char * (*)(Object) ) CTaskBt_GetName;
    vtab->CTask.SetName = ( void (*)(Object, const char *url, _u32 len) ) CTaskBt_SetName;

    vtab->CTask.GetPath = ( char * (*)(Object) ) CTaskBt_GetPath;
    vtab->CTask.SetPath = ( void (*)(Object, const char *url, _u32 len) ) CTaskBt_SetPath;

    vtab->GetSeedFile = ( char * (*)(Object) ) CTaskBt_GetSeedFile;
    vtab->SetSeedFile = ( void (*)(Object, const char *url, _u32 len) ) CTaskBt_SetSeedFile;

    vtab->CTask.GetUserData = ( void * (*)(Object, _u32 *len) ) CTaskBt_GetUserData;
    vtab->CTask.SetUserData = ( void (*)(Object, const void *url, _u32 len) ) CTaskBt_SetUserData;
    
    vtab->GetNeedDlFile = ( _u16 * (*)(Object, _u16 *len) )CTaskBt_GetNeedDlFile;
    vtab->SetNeedDlFile = ( void (*)(Object, const _u16 *data, _u16 len))CTaskBt_SetNeedDlFile;

    vtab->GetSubFile = ( BT_FILE * (*)(Object, _u16 index) )CTaskBt_GetSubFile;
    vtab->SetSubFile = (void (*)(Object, _u16 index, BT_FILE *sub_file) )CTaskBt_SetSubFile;

	vtab->CTask.GetTaskTag = ( char * (*)(Object) ) CTaskBt_GetTaskTag;
    vtab->CTask.SetTaskTag = ( void (*)(Object, const char *url, _u32 len) ) CTaskBt_SetTaskTag;
}

/* Class finalizing
 */

#ifndef OOC_NO_FINALIZE
static
void
CTaskBt_finalize( Class this )
{
}
#endif


/* Constructor
 */

static
void
CTaskBt_constructor( CTaskBt self, const void * params )
{
//  VectorIndex i;

    assert( ooc_isInitialized( CTaskBt ) );
    EM_TASK *p_task = (EM_TASK *)params;
    
    /*
    _int32 ret_val= SUCCESS;

    if (!p_task)
    {
        
        ret_val = dt_task_malloc(&p_task);
        sd_assert(p_task!=NULL);
        if (SUCCESS!=ret_val || !p_task) 
        {
            ooc_throw( em_exception_new(err_out_of_memory) );
        }
        ret_val = dt_bt_task_malloc(&p_task_info);
        if(ret_val!=SUCCESS)
        {
            dt_task_free(p_task);
            sd_assert (SUCCESS!=ret_val ) ;
            ooc_throw( em_exception_new(err_out_of_memory) );
        }
        p_task->_task_info = p_task_info;
        
    }*/
    chain_constructor( CTaskBt, self, p_task );

}

/* Destructor
 */

static
void
CTaskBt_destructor( CTaskBt self, CTaskBtVtable vtab )
{
}

/* Copy constuctor
 */

static
int
CTaskBt_copy( CTaskBt self, const CTaskBt from )
{
    

    return OOC_COPY_DONE;
}

/*  =====================================================
    Class member functions
 */


CTaskBt
CTaskBt_new( void )
{
    ooc_init_class( CTaskBt );

    return ooc_new( CTaskBt, NULL );
}

static 
BOOL 
CTaskBt_CreateTaskInfo( CTaskBt self, TASK_INFO ** pp_task_info)
{
    _int32 ret_val = dt_bt_task_malloc((EM_BT_TASK **)pp_task_info);
    if(ret_val!=SUCCESS)
    {
        sd_assert (SUCCESS!=ret_val ) ;
        ooc_throw( em_exception_new(err_out_of_memory) );
    }
    LOG_DEBUG("CTaskBt::CreateTaskInfo result:%d, task address:%x", ret_val, (_u32)(*pp_task_info));
    return TRUE;
}

static 
void 
CTaskBt_FreeTaskInfo( CTaskBt self, TASK_INFO * p_task_info)
{
    if (p_task_info)
    {
        LOG_DEBUG("CTaskP2sp::FreeTaskInfo task info address:%x", (_u32)p_task_info);
        if (p_task_info->_type == TT_BT)
            dt_uninit_bt_task_info((EM_BT_TASK *) p_task_info);
        else if (p_task_info->_type == TT_BT)
            dt_uninit_bt_magnet_task_info((EM_BT_TASK *) p_task_info);
    }
}


static 
char * CTaskBt_GetName(CTaskBt self)
{
    CTask pTask = (CTask )self;
    EM_BT_TASK *p_bt_task = (EM_BT_TASK *)pTask->p_task->_task_info;
    return p_bt_task->_file_name;
}

static 
void CTaskBt_SetName(CTaskBt self, const char *name, _u32 len)
{
    CTask pTask = (CTask )self;
    EM_TASK *p_task = (EM_TASK *)pTask->p_task;
    EM_BT_TASK *p_bt_task = (EM_BT_TASK *)p_task->_task_info;

    _u32 _file_name_len = p_task->_task_info->_file_name_len;
    _int32 ret_val = SetStrValue(&p_bt_task->_file_name, &_file_name_len, 
        name, len);
    p_task->_task_info->_file_name_len = (_u8)_file_name_len;

    if (SUCCESS == ret_val) p_task->_task_info->_have_name = TRUE;
    
    sd_assert(ret_val==SUCCESS);
}

static 
char * CTaskBt_GetPath(CTaskBt self)
{
    CTask pTask = (CTask )self;
    EM_BT_TASK *p_bt_task = (EM_BT_TASK *)pTask->p_task->_task_info;
    return p_bt_task->_file_path;
}

static 
void CTaskBt_SetPath(CTaskBt self, const char *path, _u32 len)
{
    CTask pTask = (CTask )self;
    EM_TASK *p_task = (EM_TASK *)pTask->p_task;
    EM_BT_TASK *p_bt_task = (EM_BT_TASK *)p_task->_task_info;

    _u32 path_len = p_task->_task_info->_file_path_len;
    _int32 ret_val = SetStrValue(&p_bt_task->_file_path, &path_len, 
        path, len);

    p_task->_task_info->_file_path_len = path_len;

    sd_assert(ret_val==SUCCESS);
    return ;
}

static 
void * CTaskBt_GetUserData(CTaskBt self, _u32 *len)
{
    CTask pTask = (CTask )self;
    EM_TASK *p_task = (EM_TASK *)pTask->p_task;
    EM_BT_TASK *p_bt_task = (EM_BT_TASK *)pTask->p_task->_task_info;
    *len = p_task->_task_info->_user_data_len;
    return p_bt_task->_user_data;
}

static 
void CTaskBt_SetUserData(CTaskBt self, const void *data, _u32 len)
{
    CTask pTask = (CTask )self;
    EM_TASK *p_task = (EM_TASK *)pTask->p_task;
    EM_BT_TASK *p_bt_task = (EM_BT_TASK *)p_task->_task_info;
    
    _int32 ret_val = SetStrValue((char **)&p_bt_task->_user_data, &p_task->_task_info->_user_data_len, 
        data, len);

    if (SUCCESS == ret_val) p_task->_task_info->_have_user_data = TRUE;
    
    sd_assert(ret_val==SUCCESS);
}

_u16 * CTaskBt_GetNeedDlFile(CTaskBt self, _u16 *len)
{
    CTask pTask = (CTask )self;
    EM_TASK *p_task = (EM_TASK *)pTask->p_task;
    EM_BT_TASK *p_bt_task = (EM_BT_TASK *)pTask->p_task->_task_info;
    *len = p_task->_task_info->_url_len_or_need_dl_num;
    return p_bt_task->_dl_file_index_array;
}

void CTaskBt_SetNeedDlFile(CTaskBt self, const _u16 *data, _u16 len)
{
    CTask pTask = (CTask )self;
    EM_TASK *p_task = (EM_TASK *)pTask->p_task;
    EM_BT_TASK *p_bt_task = (EM_BT_TASK *)pTask->p_task->_task_info;

    if (p_bt_task->_dl_file_index_array==data)
    {
        p_task->_task_info->_url_len_or_need_dl_num = len;
        return;
    }
    _int32 ret_val = 0;
    if (p_task->_task_info->_url_len_or_need_dl_num<len)
    {
        sd_free( (void *)p_bt_task->_dl_file_index_array );
        p_bt_task->_dl_file_index_array = NULL;
        ret_val = sd_malloc(len*sizeof(_u16), (void **)&p_bt_task->_dl_file_index_array);
        sd_assert(ret_val==SUCCESS);
        
        if (SUCCESS != ret_val) return ;
    }
    sd_memcpy((void *)p_bt_task->_dl_file_index_array, (void *)data, len*sizeof(_u16) );

    if (len>p_task->_task_info->_url_len_or_need_dl_num)
    {
        BT_FILE *old_bt_file_array = p_bt_task->_file_array;
	ret_val = sd_malloc(len*(sizeof(BT_FILE)), (void**)&p_bt_task->_file_array);
	if(ret_val==SUCCESS)
	{
            sd_memset(p_bt_task->_file_array,0,len*(sizeof(BT_FILE)));
            sd_memcpy(p_bt_task->_file_array, old_bt_file_array,  p_task->_task_info->_url_len_or_need_dl_num*(sizeof(BT_FILE)));

            _int32 i = 0;
		for(i=0;i<len;i++)
		{
			p_bt_task->_file_array[i]._file_index = p_bt_task->_dl_file_index_array[i];
			//p_bt_task->_file_array[i]._need_download = TRUE;
			//p_bt_task->_file_array[i]._file_size = 1024;
		}
            
            SAFE_DELETE(old_bt_file_array);	
	}
    }
    
    p_task->_task_info->_url_len_or_need_dl_num = len;

    
    sd_assert(ret_val==SUCCESS);
}

static 
char * CTaskBt_GetSeedFile(CTaskBt self)
{
    CTask pTask = (CTask )self;
    EM_BT_TASK *p_bt_task = (EM_BT_TASK *)pTask->p_task->_task_info;
    return p_bt_task->_seed_file_path;
}

static 
void CTaskBt_SetSeedFile(CTaskBt self, const char *need_file, _u32 len)
{
    CTask pTask = (CTask )self;
    EM_TASK *p_task = (EM_TASK *)pTask->p_task;
    EM_BT_TASK *p_bt_task = (EM_BT_TASK *)p_task->_task_info;

    _u32 _seed_file_len = p_task->_task_info->_ref_url_len_or_seed_path_len;
    _int32 ret_val = SetStrValue(&p_bt_task->_seed_file_path, &_seed_file_len, 
        need_file, len);
    p_task->_task_info->_ref_url_len_or_seed_path_len = (_u16)_seed_file_len;

    sd_assert(ret_val==SUCCESS);
    return ;
}

static BT_FILE * CTaskBt_GetSubFile(CTaskBt self, _u16 index)
{
    CTask pTask = (CTask )self;
    EM_TASK *p_task = (EM_TASK *)pTask->p_task;
    EM_BT_TASK *p_bt_task = (EM_BT_TASK *)pTask->p_task->_task_info;
    if (p_bt_task->_file_array && index>=0 && index<p_task->_task_info->_bt_total_file_num)
    {
        return &p_bt_task->_file_array[index];
    }
    return NULL;
}

static void CTaskBt_SetSubFile(CTaskBt self, _u16 index, BT_FILE *sub_file)
{
    CTask pTask = (CTask )self;
    EM_TASK *p_task = (EM_TASK *)pTask->p_task;
    EM_BT_TASK *p_bt_task = (EM_BT_TASK *)pTask->p_task->_task_info;

    _int32 ret_val = 0;
    if (p_bt_task->_file_array && index>=0 && index<p_task->_task_info->_bt_total_file_num)
    {
        ret_val = sd_memcpy(&p_bt_task->_file_array[index], sub_file, sizeof(BT_FILE));
        sd_assert(ret_val==SUCCESS);
        
        if (SUCCESS != ret_val) return ;
    }
    
    sd_assert(ret_val==SUCCESS);
}


static 
char * CTaskBt_GetTaskTag(CTaskBt self)
{
    CTask pTask = (CTask )self;
    EM_BT_TASK *p_bt_task = (EM_BT_TASK *)pTask->p_task->_task_info;
    return p_bt_task->_task_tag;
}

static 
void CTaskBt_SetTaskTag(CTaskBt self, const char *name, _u32 len)
{
    CTask pTask = (CTask )self;
    EM_TASK *p_task = (EM_TASK *)pTask->p_task;
    EM_BT_TASK *p_bt_task = (EM_BT_TASK *)p_task->_task_info;

    _u32 _task_tag_len = p_bt_task->_task_tag_len;
    _int32 ret_val = SetStrValue(&p_bt_task->_task_tag, &_task_tag_len, 
        name, len);
    //p_task->_task_info->_task_tag_len = _task_tag_len;

    //if (SUCCESS == ret_val) p_task->_task_info->_have_name = TRUE;
    
    sd_assert(ret_val==SUCCESS);
}

  

