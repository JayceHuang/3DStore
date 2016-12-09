#include <stdio.h>

#include "platform/sd_fs.h"
#include "utility/crc.h"
#include "utility/mempool.h"
#include "utility/logid.h"
#define LOGID EM_LOGID_DOWNLOAD_TASK
#include "utility/logger.h"

#include <ooc/exception.h>

#include "PersistentTaskManagerImpl.h"
#include "implement/PersistentTaskManagerImpl.h"
#include "PersistentTaskImpl.h"
#include "PersistentTaskP2spImpl.h"
#include "download_manager/download_task_store.h"
#include "Task.h"
#include "TaskBt.h"
#include "TaskP2sp.h"
#include "TaskManager.h"
#include "IoFile.h"
#include "MemoryFile.h"
#include "TaskAsyncSave.h"

/* Allocating the class description table and the vtable
 */

InterfaceRegister( CPersistentTaskManagerImpl )
{
    AddInterface( CPersistentTaskManagerImpl, IPersistentDocument )
};

AllocateClassWithInterface( CPersistentTaskManagerImpl, CPersistentImpl );


/* Class virtual function prototypes
 */

static BOOL CPersistentTaskManagerImpl_Serialize( CPersistentTaskManagerImpl self, BOOL bRead, Object fileObj );
static BOOL CPersistentTaskManagerImpl_Save( CPersistentTaskManagerImpl self, const char *cszFileName);
static BOOL CPersistentTaskManagerImpl_Open( CPersistentTaskManagerImpl self, const char *cszFileName);

static _u16 CPersistentTaskManagerImpl_GetVersion( CPersistentTaskManagerImpl self );
static void CPersistentTaskManagerImpl_SetVersion( CPersistentTaskManagerImpl self, _u16 ver );

static BOOL CPersistentTaskManagerImpl_IsNeedLoadTasks(CPersistentTaskManagerImpl self);
static void CPersistentTaskManagerImpl_SetNeedLoadTasks(CPersistentTaskManagerImpl self, BOOL val);

/* Class initializing
 */

static
void
CPersistentTaskManagerImpl_initialize( Class this )
{
    CPersistentTaskManagerImplVtable vtab = & CPersistentTaskManagerImplVtableInstance;
    
    vtab->CPersistentImpl.IPersistent.Serialize =   (BOOL (*)(Object, BOOL bRead, Object fileObj ))  CPersistentTaskManagerImpl_Serialize;
    vtab->IPersistentDocument.Open = ( BOOL ( * )( Object, const char *cszFileName) )CPersistentTaskManagerImpl_Open;
    vtab->IPersistentDocument.Save = ( BOOL ( * )( Object, const char *cszFileName) )CPersistentTaskManagerImpl_Save;
    vtab->GetVersion = ( _u16 (*)( Object ) )CPersistentTaskManagerImpl_GetVersion;
    vtab->SetVersion = ( void (*)( Object, _u16 ) )CPersistentTaskManagerImpl_SetVersion;

    vtab->IsNeedLoadTasks = (BOOL (*)(Object) )CPersistentTaskManagerImpl_IsNeedLoadTasks;
    vtab->SetNeedLoadTasks = (void (*)(Object, BOOL val))CPersistentTaskManagerImpl_SetNeedLoadTasks;
    
}

/* Class finalizing
 */

#ifndef OOC_NO_FINALIZE
static
void
CPersistentTaskManagerImpl_finalize( Class this )
{
}
#endif


/* Constructor
 */

static
void
CPersistentTaskManagerImpl_constructor( CPersistentTaskManagerImpl self, const void * params )
{
    sd_assert( ooc_isInitialized( CPersistentTaskManagerImpl ) );
    
    chain_constructor( CPersistentTaskManagerImpl, self, params );
    self->m_uiVersion = ETM_STORE_FILE_VERSION;
    self->m_bLoadTasks = FALSE;
}

/* Destructor
 */

static
void
CPersistentTaskManagerImpl_destructor( CPersistentTaskManagerImpl self, CPersistentTaskManagerImplVtable vtab )
{
}

/* Copy constuctor
 */

static
int
CPersistentTaskManagerImpl_copy( CPersistentTaskManagerImpl self, const CPersistentTaskManagerImpl from )
{
    
    return OOC_COPY_DONE;
}

/*  =====================================================
    Class member functions
 */


CPersistentTaskManagerImpl
CPersistentTaskManagerImpl_new( void )
{
    ooc_init_class( CPersistentTaskManagerImpl );

    return ooc_new( CPersistentTaskManagerImpl, NULL );
}

static _u16 CPersistentTaskManagerImpl_GetVersion( CPersistentTaskManagerImpl self )
{
    return self->m_uiVersion;
}

static void CPersistentTaskManagerImpl_SetVersion( CPersistentTaskManagerImpl self, _u16 ver)
{
    self->m_uiVersion = ver;
}

static BOOL CPersistentTaskManagerImpl_IsNeedLoadTasks(CPersistentTaskManagerImpl self)
{
    return self->m_bLoadTasks;
}

static void CPersistentTaskManagerImpl_SetNeedLoadTasks(CPersistentTaskManagerImpl self, BOOL val)
{
    self->m_bLoadTasks = val;
}

static BOOL CaculateFileLenAndCrc(Object fileObj, _u32 begin_pos, _u32 *len, _u16 *crc)
{
    IFile pFile = ooc_get_interface(fileObj, IFile);
    
    _int32 readSize = 0;
    *len = begin_pos;

   LOG_DEBUG("CaculateFileLenAndCrc: %d, begin pos:%d", fileObj, begin_pos);

   _int64 size = pFile->Size((Object)fileObj);
   pFile->Seek((Object)fileObj, begin_pos, SEEK_SET);

   size -= begin_pos;
    while ( size>0 )
    {
        char buf[512];
        _int32 read_len = size>sizeof(buf)?sizeof(buf):size;
        readSize = pFile->Read((Object)fileObj, buf, 1, read_len);
        sd_assert(readSize==read_len);

        *len += readSize;
        *crc = sd_add_crc16(*crc, buf, readSize);
        size -= readSize;
    }
    pFile->Seek((Object)fileObj, begin_pos, SEEK_SET);
    
    
   LOG_DEBUG("CaculateFileLenAndCrc end file handle: %d", fileObj);
    return TRUE;
}

static BOOL UpdateTaskHead(CPersistentTaskManagerImpl self, Object fileObj)
{
    IFile pFile = ooc_get_interface(fileObj, IFile);
    
    _int32 writesize = 0;
    _u16 crc = 0xffff;
    _u32 total_len = 0;
    EM_STORE_HEAD head;

   LOG_DEBUG("UpdateTaskHead: %d", fileObj);

    _u32 data_pos = sizeof(EM_STORE_HEAD);
    CaculateFileLenAndCrc(fileObj, data_pos, &total_len, &crc);

    head._ver = self->m_uiVersion;
    head._len = total_len;
    head._crc = crc;

    writesize = pFile->PWrite((Object)fileObj, (char *)&head, 1, sizeof(head), 0);
    sd_assert(writesize>0 && writesize==sizeof(head));
    if (writesize<0  || writesize!=sizeof(head) )
    {
        LOG_ERROR("UpdateTaskHead: %d, result:%d", fileObj, writesize);
    }

    BOOL bRet = ( writesize==sizeof(head));
   LOG_DEBUG("UpdateTaskHead: %d,  end:%d", fileObj, bRet);
    return bRet;
}

static
BOOL
CPersistentTaskManagerImpl_Serialize( CPersistentTaskManagerImpl self, BOOL bRead, Object fileObj )
{
    sd_assert( ooc_isInstanceOf( self, CPersistentTaskManagerImpl ) );
    
    IFile pFile = ooc_get_interface(fileObj, IFile);
    
    _int32 ret_val = 0;
    _u32 _max_task_id = 0, _dling_task_num = 0, running_count = 0;
    _u32 writeSize, readSize;
    _u32 running_tasks_array[MAX_ALOW_TASKS] = {0};
    _u32 *_dling_task_order = NULL;

   LOG_DEBUG("CPersistentTaskManagerImpl::Serialize file: %d, bRead:%u, version:%d", fileObj, bRead, self->m_uiVersion);

    IPersistent pIntf = ooc_get_interface(self, IPersistent);
    CTaskManager pTaskManager = (CTaskManager )pIntf->GetDataObj((Object)self);
    
    EM_STORE_HEAD head;

    if (bRead)
    {
        //if ( CTaskManagerVirtual(pTaskManager)->GetLoaded((Object)pTaskManager) )
        //{
        //    return TRUE;
        //}
        pFile->Seek((Object)fileObj, 0, SEEK_SET);

        ret_val = Util_Read(fileObj, (void *)&head, sizeof(head), -1);
        if ( SUCCESS != ret_val ) 
        {
            LOG_ERROR("CPersistentTaskManagerImpl::Serialize file: %d, read fail, file size is less than head size", fileObj);
            return FALSE;
        }

        if (head._ver > ETM_STORE_FILE_VERSION || head._ver==0)
        {
            LOG_ERROR("CPersistentTaskManagerImpl::Serialize file: %d, read fail, version in head is not valid", fileObj);
            return FALSE;
        }
        
        self->m_uiVersion = head._ver;
        if (self->m_uiVersion>=2)
        {
            _u32 data_pos = sizeof(EM_STORE_HEAD);
            _u32 total_len = 0;
            _u16 crc = 0xffff;
            CaculateFileLenAndCrc(fileObj, data_pos, &total_len, &crc);
            if (head._crc!=crc || head._len!=total_len )
            {
                //sd_assert(FALSE);
                LOG_ERROR("CPersistentTaskManagerImpl::Serialize crc or file len check fail:version=%u, file len:%u, crc:%u", head._ver, total_len, crc);
                return FALSE;
            }
        }

        ret_val = Util_Read(fileObj, (void *)&_max_task_id, sizeof(_max_task_id), -1);
        if (ret_val!=SUCCESS)
        {
            LOG_ERROR("CPersistentTaskManagerImpl::Serialize file: %d, read max task id fail", fileObj);
            return FALSE;
        }
        ret_val = Util_Read(fileObj, (void *)running_tasks_array, sizeof(_u32)*MAX_ALOW_TASKS, -1);
        if (ret_val!=SUCCESS)
        {
            LOG_ERROR("CPersistentTaskManagerImpl::Serialize file: %d, read running_tasks_array fail", fileObj);
            return FALSE;
        }
        ret_val = Util_Read(fileObj, (void *)&_dling_task_num, sizeof(_dling_task_num), -1);
        if (ret_val!=SUCCESS)
        {
            LOG_ERROR("CPersistentTaskManagerImpl::Serialize file: %d, read _dling_task_num fail", fileObj);
            return FALSE;
        }
        if (_dling_task_num<MAX_TASK_NUM)
        {
            Util_AllocAndRead(fileObj, (void **)&_dling_task_order, _dling_task_num*sizeof(_u32), _dling_task_num*sizeof(_u32), -1);
            sd_assert(ret_val == SUCCESS);
            if (ret_val!=SUCCESS)
            {
                LOG_ERROR("CPersistentTaskManagerImpl::Serialize file: %d, read _dling_task_order fail", fileObj);
                return FALSE;
            }
        }
        else
        {
            LOG_ERROR("CPersistentTaskManagerImpl::Serialize file: %d, _dling_task_num is not valid", fileObj);
            return FALSE;
        }

        CTaskManagerVirtual(pTaskManager)->SetMaxTaskId((Object)pTaskManager, _max_task_id);
        if (_dling_task_order)
        {
            CTaskManagerVirtual(pTaskManager)->SetOrderList((Object)pTaskManager, _dling_task_order, _dling_task_num);
            SAFE_DELETE(_dling_task_order);
        }
        CTaskManagerVirtual(pTaskManager)->SetRunningList((Object)pTaskManager, running_tasks_array, MAX_ALOW_TASKS);
        
        
    }
    else
    {
        head._crc = 0;
        head._len = 0;
        head._ver = self->m_uiVersion;
        writeSize = 0;
        _max_task_id = CTaskManagerVirtual(pTaskManager)->GetMaxTaskId((Object)pTaskManager);
        _u32 *task_order_ids = CTaskManagerVirtual(pTaskManager)->GetOrderList((Object)pTaskManager, &_dling_task_num);
        _u32 *running_task_ids = CTaskManagerVirtual(pTaskManager)->GetRunningList((Object)pTaskManager);

        running_count = CTaskManagerVirtual(pTaskManager)->GetRunningCount((Object)pTaskManager);

        Util_Write(fileObj, (void *)&head, sizeof(head), -1);
        Util_Write(fileObj, (void *)&_max_task_id, sizeof(_max_task_id), -1);
        Util_Write(fileObj, (void *)running_task_ids, sizeof(_u32)*MAX_ALOW_TASKS, -1);
        Util_Write(fileObj, (void *)&_dling_task_num, sizeof(_dling_task_num), -1);
        Util_Write(fileObj, (void *)task_order_ids, _dling_task_num*sizeof(_u32), -1);
        
    }

   LOG_DEBUG("CPersistentTaskManagerImpl::Serialize file: %d, bRead:%u, version:%d finished", fileObj, bRead, self->m_uiVersion);

    return TRUE;
}

static
BOOL 
CPersistentTaskManagerImpl_Save( CPersistentTaskManagerImpl self, const char *cszFileName)
{
    sd_assert( ooc_isInstanceOf( self, CPersistentTaskManagerImpl ) );

    _u16 crc = 0;
    _u32 total_len = 0;

    _int32 ret_val = SUCCESS;
    BOOL bRet = FALSE;
    _u32 writesize = 0;

    const char *pathEnd = strrchr(cszFileName, DIR_SPLIT_CHAR);
    char szPath[MAX_FILE_PATH_LEN] = {0};
    _u32 pathLen = pathEnd - cszFileName;
    sd_strncpy(szPath, cszFileName, pathLen);

    if (sd_is_path_exist(szPath)!=SUCCESS)
    {
        ret_val = sd_mkdir(szPath);
        if (SUCCESS!=ret_val) return FALSE;
    }

    sprintf(szPath, "%s.tmp", cszFileName); 

    CMemoryFile memFile = ooc_new( CMemoryFile, NULL );
    Object fileObj = (Object)memFile;
    IFile pFile = ooc_get_interface(fileObj, IFile);
    bRet = CMemoryFileVirtual(memFile)->Open( (Object)fileObj);
    if ( bRet )
    {
        IPersistent pIntf = ooc_get_interface(self, IPersistent);
        bRet = pIntf->Serialize((Object)self, FALSE, fileObj);
        LOG_DEBUG("CPersistentTaskManagerImpl::Save Serialize result:%d", bRet);
        
        if (self->m_uiVersion>=2)
        {
            bRet = UpdateTaskHead(self, fileObj);
        }

        total_len = pFile->Size( (Object)fileObj );
        unsigned char *pFileData = CMemoryFileVirtual(memFile)->GetBuffer( (Object)fileObj, TRUE );
        output_task_to_file(0xFFFFFFFF, ASYNC_OP_WRITE, cszFileName, pFileData, total_len);
        /*
        if (bRet)
        {
            if (sd_file_exist(cszFileName))
            {
                ret_val = sd_delete_file(cszFileName);
                if (SUCCESS!=ret_val)
                {
                    LOG_ERROR("CPersistentTaskManagerImpl::Save delete file fail: %s", cszFileName);
                }
                sd_assert(SUCCESS==ret_val);
            }
            ret_val = sd_rename_file(szPath, cszFileName);
            LOG_DEBUG("CPersistentTaskManagerImpl::Save rename  file: %s to %s, result:%d", szPath, cszFileName, ret_val);
            
            if (SUCCESS!=ret_val)
            {
                LOG_ERROR("CPersistentTaskManagerImpl::Save rename  file fail: %s to %s", szPath, cszFileName);
            }
            sd_assert(SUCCESS==ret_val);
            bRet = ret_val==SUCCESS;
        }*/
    }
    ooc_delete((Object)fileObj);
    return bRet;
}

static
CTask CreateTask(EM_TASK_TYPE _type)
{
    CTask pTask = NULL;
    switch(_type)
        {
        case TT_BT:
        case TT_BT_MAGNET:
            pTask = (CTask)ooc_new(CTaskBt, NULL);
            break;
        default:
            pTask = (CTask)ooc_new(CTaskP2sp, NULL);
            break;            
        }
   LOG_DEBUG("CPersistentTaskManagerImpl::CreateTask address: %u ", (_u32)pTask);
    return pTask;
}

static
BOOL
CPersistentTaskManagerImpl_LoadTask1( CPersistentTaskManagerImpl self, CTaskManager pTaskManager, Object fileObj )
{
    IFile pFile = ooc_get_interface(fileObj, IFile);
    
    _int32 ret_val = SUCCESS;
    EM_TASK *p_task=NULL;
    BOOL bLoadFinish = FALSE;
    EM_TASK_TYPE task_type = 0;

   LOG_DEBUG("CPersistentTaskManagerImpl::LoadTask1 file : %d ", fileObj);

   BOOL bRet = pFile->Seek( (Object)fileObj,  POS_TASK_START, SEEK_SET);
   sd_assert(bRet);
   if (!bRet)
   {
	   LOG_ERROR("CPersistentTaskManagerImpl::LoadTask1 file : %d, set task data pos fail ", fileObj);
	   return FALSE;
   }
    while (!bLoadFinish)
    {

        CPersistentTaskImpl persistentTask = (CPersistentTaskImpl) ooc_new( CPersistentTaskImpl, NULL );

        if (!CPersistentTaskImplVirtual(persistentTask)->DetermineTaskType((Object)persistentTask, fileObj, &task_type))
        {
            //�ж��Ƿ��ļ�β
            DT_TASK_STORE_HEAD head;
            _int32 read_size = pFile->Read( (Object)fileObj,  &head, 1, sizeof(DT_TASK_STORE_HEAD));
            if (read_size!=0)
            {
                //û�е��ļ�β�����쳣
	        LOG_ERROR("CPersistentTaskManagerImpl::LoadTask1 file : %d, Determin Task Type fail ", fileObj);
                //sd_assert(FALSE);
            }
            else
            {
	        LOG_DEBUG("CPersistentTaskManagerImpl::LoadTask1 file : %d, Determin Task Type fail, reach file end ", fileObj);
            }
            ooc_delete((Object)persistentTask);
            break;
        }

        CTask pTask = CreateTask(task_type);
        sd_assert(pTask);
        if (!pTask)
        {
            ooc_delete((Object)persistentTask);
            break;
        }
        p_task = CTaskVirtual(pTask)->GetAttachedData((Object)pTask);
        sd_assert(p_task);
        
        IPersistent pPersistent = ooc_get_interface(persistentTask, IPersistent);
        pPersistent->SetDataObj( (Object)persistentTask, (Object)pTask );
        CPersistentTaskImplVirtual(persistentTask)->SetVersion((Object)persistentTask, 1);

        BOOL bReadTask = CPersistentTaskImplVirtual(persistentTask)->Read((Object)persistentTask, fileObj);
        if (bReadTask)
        {
            if (!CPersistentTaskImplVirtual(persistentTask)->IsValid((Object)persistentTask))
            {
	        LOG_DEBUG("CPersistentTaskManagerImpl::LoadTask1 file : %d, read an invalid task ", fileObj);
                bRet = pFile->Seek( (Object)fileObj,  p_task->_offset+CPersistentTaskImplVirtual(persistentTask)->GetLength((Object)persistentTask) , SEEK_SET);
                ooc_delete((Object)pTask);
                ooc_delete((Object)persistentTask);
                continue;
            }
            //
            CPersistentTaskImplVirtual(persistentTask)->ConvertVersionFrom1To2((Object)persistentTask, fileObj);
            bRet = pFile->Seek( (Object)fileObj,  p_task->_offset+CPersistentTaskImplVirtual(persistentTask)->GetLength((Object)persistentTask) , SEEK_SET);

            bRet = CTaskManagerVirtual(pTaskManager)->AddTask((Object)pTaskManager, pTask);
            if (bRet)
            {
                CTaskVirtual(pTask)->Detach((Object)pTask);
            }
        }
        else
            bLoadFinish = TRUE;
        
        ooc_delete((Object)pTask);
        ooc_delete((Object)persistentTask);
    }
    return ret_val == SUCCESS;
}

static
BOOL 
DetermineTaskTypeInFile( CPersistentTaskImpl self, const char *cszFileName, EM_TASK_TYPE *type)
{
    BOOL bRet = FALSE;

   LOG_DEBUG("DetermineTaskTypeInFile::Open file path: %s ", cszFileName);

    CIoFile ioFile = ooc_new( CIoFile, NULL );
    Object fileObj = (Object)ioFile;
    IFile pFile = ooc_get_interface(fileObj, IFile);
    bRet = CIoFileVirtual(ioFile)->Open( (Object)fileObj, cszFileName, "rb");
    if (bRet)
    {
        bRet = CPersistentTaskImplVirtual(self)->DetermineTaskType((Object)self, fileObj,  type);
        pFile->Close( (Object)fileObj );
    }
    ooc_delete((Object)fileObj);
   LOG_DEBUG("DetermineTaskTypeInFile::Open file path: %s, result type:%u ", cszFileName, type);
    return bRet;
}

static
BOOL
CPersistentTaskManagerImpl_LoadTask2( CPersistentTaskManagerImpl self,  CTaskManager pTaskManager, const char *cszTaskPath, _u32 uiPathLen)
{
    _int32 ret_val = SUCCESS;
    EM_TASK *p_task=NULL;
    EM_TASK_TYPE task_type = 0;

    char szPath[MAX_FILE_PATH_LEN] = {0};
 
    sd_strncpy(szPath, cszTaskPath, uiPathLen);
    sd_append_path(szPath, MAX_FILE_PATH_LEN, "tasks");

   LOG_DEBUG("CPersistentTaskManagerImpl::LoadTask2 file path: %s ", szPath);

    if (!sd_dir_exist(szPath)) return TRUE;

    FILE_ATTRIB *p_sub_files = NULL;
    _u32 _sub_files_count = 0;
    ret_val = sd_get_sub_files(szPath, NULL, 0, &_sub_files_count);
    sd_assert(SUCCESS==ret_val);

    ret_val = sd_malloc(sizeof(FILE_ATTRIB)*_sub_files_count, (void **)&p_sub_files);
    sd_get_sub_files(szPath, p_sub_files, _sub_files_count, &_sub_files_count);

    _int32 i=0;
    for (i=0; i<_sub_files_count; i++)
    {
//      p_sub_files[i]._attrib
        if (!p_sub_files[i]._is_dir)
        {
            //�ж��ǲ�����.dat�ļ���������ļ�.dat.tmp
            char *file_ext = sd_strrchr(p_sub_files[i]._name, '.');
            if (!file_ext || sd_strncmp(file_ext, ".dat", 4)!=0) continue;            
        
            char szTaskFile[MAX_FILE_PATH_LEN] = {0};
            sd_strncpy(szTaskFile, szPath, sd_strlen(szPath));
            sd_append_path(szTaskFile, MAX_FILE_PATH_LEN, p_sub_files[i]._name);            
            
            CPersistentTaskImpl persistentTask = (CPersistentTaskImpl) ooc_new( CPersistentTaskImpl, NULL );
            if (!DetermineTaskTypeInFile(persistentTask, szTaskFile, &task_type))
            {
                sd_assert(FALSE);
                ooc_delete((Object)persistentTask);
                continue;
            }

            CTask pTask = CreateTask(task_type);
            sd_assert(pTask);
            if (!pTask)
            {
                ooc_delete((Object)persistentTask);
                continue;
            }
            p_task = CTaskVirtual(pTask)->GetAttachedData((Object)pTask);
            sd_assert(p_task);

            IPersistent pPersistent = ooc_get_interface(persistentTask, IPersistent);
            pPersistent->SetDataObj( (Object)persistentTask, (Object)pTask );
            IPersistentDocument pDoc = ooc_get_interface(persistentTask, IPersistentDocument);
            
            BOOL bReadTask = pDoc->Open((Object)persistentTask, szTaskFile);
            if (bReadTask)
            {
                BOOL bRet = CTaskManagerVirtual(pTaskManager)->AddTask((Object)pTaskManager, pTask);
                if (bRet)
                {
                    CTaskVirtual(pTask)->Detach((Object)pTask);
                }
            }
            else
            {
            }
            
            ooc_delete((Object)persistentTask);
            ooc_delete((Object)pTask);
        
        }
    }

    sd_free(p_sub_files);
    p_sub_files = NULL;
    
   LOG_DEBUG("CPersistentTaskManagerImpl::LoadTask2 file path: %s finish", szPath);
    
    return ret_val == SUCCESS;
}

static
BOOL 
CPersistentTaskManagerImpl_Open( CPersistentTaskManagerImpl self, const char *cszFileName)
{
    sd_assert( ooc_isInstanceOf( self, CPersistentTaskManagerImpl ) );

    _u16 crc = 0;
    BOOL bRet = FALSE;
    EM_TASK *p_task=NULL;
    TASK_INFO * p_task_info = NULL;

   LOG_DEBUG("CPersistentTaskManagerImpl::Open file path: %s ", cszFileName);

    IPersistent pIntf = ooc_get_interface(self, IPersistent);
    CTaskManager pTaskManager = (CTaskManager )pIntf->GetDataObj((Object)self);
    if (!CTaskManagerVirtual(pTaskManager)->GetLoaded((Object)pTaskManager) ||
         !CTaskManagerVirtual(pTaskManager)->GetTasksLoaded((Object)pTaskManager))
    {
        CIoFile ioFile = ooc_new( CIoFile, NULL );
        Object fileObj = (Object)ioFile;
        IFile pFile = ooc_get_interface(fileObj, IFile);
        bRet = CIoFileVirtual(ioFile)->Open( (Object)fileObj, cszFileName, "rb");
        if (bRet)
        {
            bRet = pIntf->Serialize((Object)self, TRUE, fileObj);
            LOG_DEBUG("CPersistentTaskManagerImpl::read file path: %s, result:%s", cszFileName, bRet?"true":"false");
            //sd_assert(bRet);

            if (!bRet )
            {
                //��������б�����ļ�����ʧ�ܣ������е�tasks����
                pFile->Close( (Object)fileObj );
                sd_delete_file( cszFileName );
                dt_create_task_file();            
            }
            CTaskManagerVirtual(pTaskManager)->SetLoaded((Object)pTaskManager, TRUE);

            if (self->m_bLoadTasks)
            {
                if (bRet && CPersistentTaskManagerImplVirtual(self)->GetVersion((Object)self)<2 &&
                     !CTaskManagerVirtual(pTaskManager)->GetTasksLoaded((Object)pTaskManager))
                {
                    bRet = CPersistentTaskManagerImpl_LoadTask1(self, pTaskManager, fileObj);
                    CTaskManagerVirtual(pTaskManager)->SetTasksLoaded((Object)pTaskManager, TRUE);
                    
                    LOG_DEBUG("CPersistentTaskManagerImpl::LoadTask1 file path: %s, result:%d", cszFileName, bRet);
                    sd_assert(bRet);
                }
            }
            pFile->Close( (Object)fileObj );
            
            
        }
        ooc_delete((Object)fileObj);
    }

    if (CPersistentTaskManagerImplVirtual(self)->GetVersion((Object)self)>=2 && 
        !CTaskManagerVirtual(pTaskManager)->GetTasksLoaded((Object)pTaskManager))
    {
        const char *pathEnd = strrchr(cszFileName, DIR_SPLIT_CHAR);
        _u32 pathLen = pathEnd - cszFileName;

        bRet = CPersistentTaskManagerImpl_LoadTask2(self, pTaskManager, cszFileName, pathLen);
        CTaskManagerVirtual(pTaskManager)->SetTasksLoaded((Object)pTaskManager, TRUE);
        LOG_DEBUG("CPersistentTaskManagerImpl::LoadTask2 file path: %s, result:%d", cszFileName, bRet);
        sd_assert(bRet);
    }
   LOG_DEBUG("CPersistentTaskManagerImpl::Open file path: %s finished result:%d", cszFileName, bRet);
    return bRet;
}

