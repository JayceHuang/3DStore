#include <stdio.h>
#include "platform/sd_fs.h"
#include "utility/logid.h"
#define LOGID EM_LOGID_DOWNLOAD_TASK
#include "utility/logger.h"
#include "utility/crc.h"

#include <ooc/exception.h>
#include "PersistentTaskImpl.h"

#include "implement/PersistentTaskImpl.h"
#include "Task.h"
#include "IoFile.h"
#include "MemoryFile.h"
#include "TaskAsyncSave.h"

#include "download_manager/download_task_store.h"

/* Class virtual function prototypes
 */
InterfaceRegister( CPersistentTaskImpl )
{
    AddInterface( CPersistentTaskImpl, IPersistentDocument )
};

AllocateClassWithInterface( CPersistentTaskImpl, CPersistentImpl );

static BOOL CPersistentTaskImpl_Serialize( CPersistentTaskImpl self, BOOL bRead, Object fileObj );
static BOOL CPersistentTaskImpl_Save( CPersistentTaskImpl self, const char *cszFileName);
static BOOL CPersistentTaskImpl_Read( CPersistentTaskImpl self, Object fileObj );
static BOOL CPersistentTaskImpl_Open( CPersistentTaskImpl self, const char *cszFileName);

static BOOL CPersistentTaskImpl_ConvertVersionFrom1To2( CPersistentTaskImpl self, Object fileObj);

static _u16 CPersistentTaskImpl_GetVersion( CPersistentTaskImpl self );
static void CPersistentTaskImpl_SetVersion( CPersistentTaskImpl self, _u16 ver );

static BOOL CPersistentTask_IsValid(CPersistentTaskImpl self);
static void CPersistentTask_SetValid(CPersistentTaskImpl self, BOOL bVal);

static _u32 CPersistentTaskImpl_GetLength( CPersistentTaskImpl self );
static void CPersistentTaskImpl_SetLength( CPersistentTaskImpl self, _u32 len );

static BOOL CPersistentTaskImpl_DetermineTaskType(CPersistentTaskImpl self, Object fileObj, EM_TASK_TYPE *type);



/* Class initializing
 */

static
void
CPersistentTaskImpl_initialize( Class this )
{
    CPersistentTaskImplVtable vtab = & CPersistentTaskImplVtableInstance;
    
    vtab->CPersistentImpl.IPersistent.Serialize    =     (BOOL (*)(Object, BOOL bRead, Object fileObj ))    CPersistentTaskImpl_Serialize;
    vtab->Read = ( BOOL ( * )( Object, Object fileObj ) )CPersistentTaskImpl_Read;
    vtab->IPersistentDocument.Open = ( BOOL ( * )( Object, const char *cszFileName) )CPersistentTaskImpl_Open;
    vtab->IPersistentDocument.Save = ( BOOL ( * )( Object, const char *cszFileName) )CPersistentTaskImpl_Save;

    vtab->ConvertVersionFrom1To2 = (BOOL (*)( Object, Object fileObj))CPersistentTaskImpl_ConvertVersionFrom1To2;

    vtab->GetVersion = ( _u16 (*)( Object ) )CPersistentTaskImpl_GetVersion;
    vtab->SetVersion = ( void (*)( Object, _u16 ) )CPersistentTaskImpl_SetVersion;

    vtab->IsValid = (BOOL (*)(Object)) CPersistentTask_IsValid;
    vtab->SetValid = (void (*)(Object, BOOL)) CPersistentTask_IsValid;

    vtab->GetLength = ( _u32 (*)( Object ) )CPersistentTaskImpl_GetLength;
    vtab->SetLength = ( void (*)( Object, _u32 ) )CPersistentTaskImpl_SetLength;

    vtab->DetermineTaskType = ( BOOL  ( * )( Object, Object fileObj, EM_TASK_TYPE *) )CPersistentTaskImpl_DetermineTaskType;

	
}

/* Class finalizing
 */

#ifndef OOC_NO_FINALIZE
static
void
CPersistentTaskImpl_finalize( Class this )
{
}
#endif


/* Constructor
 */

static
void
CPersistentTaskImpl_constructor( CPersistentTaskImpl self, const void * params )
{
    sd_assert( ooc_isInitialized( CPersistentTaskImpl ) );
    
    chain_constructor( CPersistentTaskImpl, self, NULL );
    self->m_uiVersion = ETM_STORE_FILE_VERSION;

}

/* Destructor
 */

static
void
CPersistentTaskImpl_destructor( CPersistentTaskImpl self, CPersistentTaskImplVtable vtab )
{
}

/* Copy constuctor
 */

static
int
CPersistentTaskImpl_copy( CPersistentTaskImpl self, const CPersistentTaskImpl from )
{

    return OOC_COPY_DONE;
}

/*    =====================================================
    Class member functions
 */


CPersistentTaskImpl
CPersistentTaskImpl_new( void )
{
    ooc_init_class( CPersistentTaskImpl );

    return ooc_new( CPersistentTaskImpl, NULL );
}

static _u16 CPersistentTaskImpl_GetVersion( CPersistentTaskImpl self )
{
    return self->m_uiVersion;
}

static void CPersistentTaskImpl_SetVersion( CPersistentTaskImpl self, _u16 ver)
{
    self->m_uiVersion = ver;
}

static
BOOL CPersistentTask_IsValid(CPersistentTaskImpl self)
{
    return self->m_bValid;
}

static
void CPersistentTask_SetValid(CPersistentTaskImpl self, BOOL bVal)
{
    self->m_bValid = bVal;
}

static _u32 CPersistentTaskImpl_GetLength( CPersistentTaskImpl self )
{
    return self->m_uiLen;
}

static void CPersistentTaskImpl_SetLength( CPersistentTaskImpl self, _u32 len)
{
    self->m_uiLen = len;
}

static BOOL CaculateFileLenAndCrc(Object fileObj, _int32 begin_pos, _u32 *len, _u16 *crc)
{
    //_int32 ret_val = 0;
    _int32 readSize = 0;
    *len = 0;
    IFile pFile = ooc_get_interface(fileObj, IFile);

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
    return TRUE;
}

static BOOL UpdateTaskHead(CPersistentTaskImpl self, Object fileObj)
{
    IFile pFile = ooc_get_interface(fileObj, IFile);
    _int32 writesize = 0;
    _u16 crc = 0xffff;
    _u32 total_len = 0;
    //_int32 ret_val = SUCCESS;
    DT_TASK_STORE_HEAD head;

    Object pData = ((CPersistentImpl)self)->m_pData;
    EM_TASK *p_task = CTaskVirtual((CTask)pData)->GetAttachedData(pData);

    _u32 data_pos = sizeof(DT_TASK_STORE_HEAD);
    CaculateFileLenAndCrc(fileObj, data_pos, &total_len, &crc);

    head._valid = self->m_uiVersion;
    head._len = total_len;
    head._crc = crc;

    writesize = pFile->PWrite((Object)fileObj, (char *)&head, 1, sizeof(head), p_task->_offset);
    sd_assert(writesize>0 && writesize==sizeof(head));
    return (writesize>0 && writesize==sizeof(head));
}

static
_u32 CaculateDataLength(CPersistentTaskImpl self)
{
    Object pData = ((CPersistentImpl)self)->m_pData;
    EM_TASK *p_task = CTaskVirtual((CTask)pData)->GetAttachedData(pData);
    TASK_INFO *p_task_info = (TASK_INFO *)p_task->_task_info;
    if (p_task_info)
    {
        //return p_task_info->
    }
}

static
BOOL CPersistentTaskImpl_Serialize( CPersistentTaskImpl self, BOOL bRead, Object fileObj )
{
    sd_assert( ooc_isInstanceOf( self, CPersistentTaskImpl ) );
    
    IFile pFile = ooc_get_interface(fileObj, IFile);

    _int32 writeSize, readSize;
    _int64 filePos = 0;
    _int64 fileSize = 0;
    _int32 ret_val = 0;

    DT_TASK_STORE_HEAD head;
    Object pData = ((CPersistentImpl)self)->m_pData;
    EM_TASK *p_task = CTaskVirtual((CTask)pData)->GetAttachedData(pData);
    filePos = pFile->Tell((Object)fileObj);
    p_task->_offset = (_u32)filePos;

    LOG_DEBUG("CPersistentTaskImpl::Serialize begin read:%d, task_id=%u, file handle:%d, offset:%u", bRead, p_task->_task_info->_task_id, fileObj, p_task->_offset);

    if (bRead)
    {
        fileSize = pFile->Size( (Object)fileObj );
        sd_assert(fileSize>0 );
        if (fileSize<=0) return FALSE;

        readSize = pFile->Read( (Object)fileObj, (void *)&head, 1, sizeof(head));
        self->m_bValid = head._valid!=0;
        self->m_uiLen = head._len+sizeof(head);
        if (readSize==0)
        {
            LOG_ERROR("CPersistentTaskImpl::Serialize file reach to end=%u when read task head", fileObj);
            return FALSE;
        }
        else if (head._len<sizeof(TASK_INFO))
        {
            LOG_ERROR("CPersistentTaskImpl::Serialize task len is tool small head._len=%u", head._len);
            sd_assert(FALSE);
            return FALSE;
        }
        else if (filePos+self->m_uiLen>fileSize)
        {
            LOG_ERROR("CPersistentTaskImpl::Serialize task len is tool long in head:head._len=%u", head._len);
            sd_assert(FALSE);
            return FALSE;
        }
        self->m_uiVersion = head._valid;
        if (self->m_uiVersion>=2)
        {
            self->m_bValid = TRUE;

            self->m_uiVersion = head._valid;

            _u32 data_pos = sizeof(DT_TASK_STORE_HEAD);
            _u32 total_len = 0;
            _u16 crc = 0xffff;
            CaculateFileLenAndCrc(fileObj, data_pos, &total_len, &crc);
            if (head._crc!=crc || head._len!=total_len )
            {
                //sd_assert(FALSE);
                LOG_DEBUG("CPersistentTaskImpl::Serialize crc or file len check fail:version=%u, file len:%u, crc:%u", head._valid, total_len, crc);
                return FALSE;
            }
        }

        //���û�б����أ��Ͷ�ȡ������Ѿ����أ��Ͳ�Ҫ�ټ���

		// _u32 version2to3_inc = CPersistentTaskImplVirtual((CPersistentTaskImpl)self)->GetTaskInfoIncLength(self, ETM_STORE_FILE_VERSION);

		 //LOG_DEBUG("CPersistentTaskImpl::Serialize version2to3_inc %d ",version2to3_inc );

		  if (p_task->_task_info->_task_id==0) {

            readSize = pFile->Read( (Object)fileObj, (char *)p_task->_task_info, 1, sizeof(TASK_INFO));
            p_task->_task_info->_full_info = FALSE;
		  }

		  //p_task->_task_info->_full_info = FALSE;

		 //LOG_DEBUG("CPersistentTaskImpl::Serialize p_task->_task_info->_task_tag_len: %d ",p_task->_task_info->_task_tag_len);
		 //LOG_DEBUG("CPersistentTaskImpl::Serialize sizeof test(TASK_INFO): %d ",sizeof(TASK_INFO));

		
        
    }
    else
    {
        head._valid = self->m_uiVersion;
        head._len = sizeof(TASK_INFO);
        head._crc = 0xffff;

        writeSize = pFile->Write( (Object)fileObj, (char *)&head, 1, sizeof(head));
        writeSize = pFile->Write( (Object)fileObj, (char *)p_task->_task_info, 1, sizeof(TASK_INFO) );
        
    }
    LOG_DEBUG("CPersistentTaskImpl::Serialize finish read:%d, task_id=%u, task len:%u", bRead, p_task->_task_info->_task_id, head._len);
    return TRUE;
    
}

static
BOOL
CPersistentTaskImpl_ConvertVersionFrom1To2( CPersistentTaskImpl self, Object fileObj )
{
    sd_assert( ooc_isInstanceOf( self, CPersistentTaskImpl ) );

    IFile pFile = ooc_get_interface(fileObj, IFile);
    
   LOG_DEBUG("CPersistentTaskImpl::ConvertVersionFrom1To2 file : %d ", fileObj);

    if (!fileObj ) return FALSE;
    _int32 ret_val = SUCCESS;
    int hFile2 = 0;
    char task_file_path[MAX_FILE_PATH_LEN] = {0};

    DT_TASK_STORE_HEAD head;
    Object pData = ((CPersistentImpl)self)->m_pData;
    EM_TASK *p_task = CTaskVirtual((CTask)pData)->GetAttachedData(pData);

    ret_val = dt_get_task_alone_store_file_path(p_task, task_file_path, MAX_FILE_PATH_LEN);
    sd_assert( SUCCESS == ret_val );

   LOG_DEBUG("CPersistentTaskImpl::ConvertVersionFrom1To2 file path: %s ", task_file_path);

    const char *pathEnd = strrchr(task_file_path, DIR_SPLIT_CHAR);
    char szPath[MAX_FILE_PATH_LEN] = {0};
    _u32 pathLen = pathEnd - task_file_path;
    sd_strncpy(szPath, task_file_path, pathLen);

    if (sd_is_path_exist(szPath)!=SUCCESS)
    {
        ret_val = sd_mkdir(szPath);
        sd_assert( SUCCESS == ret_val );
        if (SUCCESS != ret_val) return FALSE;
    }

    BOOL bRet = FALSE;
    ret_val = sd_open_ex(task_file_path, O_FS_CREATE, &hFile2);
    if (ret_val==SUCCESS)
    {
    	char buf[512];
    	_int32 readSize, writeSize;
       bRet = pFile->Seek( (Object)fileObj, p_task->_offset, SEEK_SET);
    	sd_assert(bRet);

       readSize = pFile->PRead( (Object)fileObj, (void *)&head, 1, sizeof(head), p_task->_offset);
    	sd_assert( readSize>0 );

    	head._valid = 1;
       ret_val = sd_write(hFile2, (char *)&head, sizeof(head), &writeSize);
    	sd_assert(SUCCESS==ret_val);

    	_int32 remainLen = head._len;
    	while (readSize = pFile->Read( (Object)fileObj, buf, 1, remainLen>512?512:remainLen ) && readSize>0)
    	{
    		ret_val = sd_write(hFile2, buf, readSize, &writeSize);
        	sd_assert(SUCCESS==ret_val);
        	sd_assert(readSize==writeSize);

    		remainLen-= readSize;
    	}
    	sd_close_ex(hFile2);
    }
    return ret_val==SUCCESS;
}

static
BOOL 
CPersistentTaskImpl_Save( CPersistentTaskImpl self, const char *cszFileName)
{
    sd_assert( ooc_isInstanceOf( self, CPersistentTaskImpl ) );
    
    if (!cszFileName) return FALSE;
    _int32 ret_val = SUCCESS;

   LOG_DEBUG("CPersistentTaskImpl::Save begin file path: %s ", cszFileName);

    Object pData = ((CPersistentImpl)self)->m_pData;
    EM_TASK *p_task = CTaskVirtual((CTask)pData)->GetAttachedData(pData);    
    
    const char *pathEnd = strrchr(cszFileName, DIR_SPLIT_CHAR);
    char szPath[MAX_FILE_PATH_LEN] = {0};
    _u32 pathLen = pathEnd - cszFileName;
    sd_strncpy(szPath, cszFileName, pathLen);

    if (sd_is_path_exist(szPath)!=SUCCESS)
    {
        ret_val = sd_mkdir(szPath);
        sd_assert( SUCCESS == ret_val );
        if (SUCCESS != ret_val) return FALSE;
    }

    sprintf(szPath, "%s.tmp", cszFileName); 
    
    BOOL bRet = FALSE;
    CMemoryFile memFile = ooc_new( CMemoryFile, NULL );
    Object fileObj = (Object)memFile;
    IFile pFile = ooc_get_interface(fileObj, IFile);
    bRet = CMemoryFileVirtual(memFile)->Open( (Object)fileObj);    
    if (bRet)
    {
        IPersistent pIntf = ooc_get_interface(self, IPersistent);
        bRet = pIntf->Serialize((Object)self, FALSE, fileObj);

        if (self->m_uiVersion>=2)
        {
            bRet = UpdateTaskHead(self, fileObj);
        }

        _int64 total_len = pFile->Size( (Object)fileObj );
        unsigned char *pFileData = CMemoryFileVirtual(memFile)->GetBuffer( (Object)fileObj, TRUE );
        output_task_to_file(p_task->_task_info->_task_id, ASYNC_OP_WRITE, cszFileName, pFileData, total_len);
        /*
        pFile->Close( (Object)fileObj );
        if (bRet)
        {
            if (sd_file_exist(cszFileName))
            {
                sd_delete_file(cszFileName);
            }
            ret_val = sd_rename_file(szPath, cszFileName);
            bRet = ret_val==SUCCESS;
        }*/
    }
    ooc_delete( (Object)fileObj );
   LOG_DEBUG("CPersistentTaskImpl::Save finish file path: %s ", cszFileName);
    return bRet;
}

static 
BOOL 
CPersistentTaskImpl_DetermineTaskType(CPersistentTaskImpl self, Object fileObj, EM_TASK_TYPE *type)
{
    sd_assert( ooc_isInstanceOf( self, CPersistentTaskImpl ) );

    IFile pFile = ooc_get_interface(fileObj, IFile);
    
    _int32 readSize;
    _int64 readPos = 0;
    _int64 orgPos = 0;

    DT_TASK_STORE_HEAD head;
    TASK_INFO _task_info;

    LOG_DEBUG("CPersistentTaskImpl::DetermineTaskType");
    

    readPos= pFile->Tell((Object)fileObj);
    sd_assert(readPos>=0);
    orgPos = readPos;
    LOG_DEBUG("CPersistentTaskImpl::DetermineTaskType begin file handle:%d, offset:%d", fileObj, (_int32)orgPos);
    
    if (!type) return FALSE;
        //sd_setfilepos(hFile, 0);
        
    readSize = pFile->Read((Object)fileObj, (void *)&head, 1, sizeof(head));
    if (readSize != sizeof(head)) 
    {
        if (readSize>0) pFile->Seek((Object)fileObj, orgPos, SEEK_SET);
        return FALSE;
    }

    readSize = 0;
    readSize = pFile->Read((Object)fileObj, (void *)&_task_info, 1, sizeof(TASK_INFO));
    if (readSize != sizeof(TASK_INFO)) 
    {
        pFile->Seek((Object)fileObj, orgPos, SEEK_SET);
        return FALSE;
    }
    pFile->Seek((Object)fileObj, orgPos, SEEK_SET);
    *type = _task_info._type;
    return TRUE;

}

static 
BOOL CPersistentTaskImpl_Read( CPersistentTaskImpl self, Object fileObj)
{
   LOG_DEBUG("CPersistentTaskImpl::Read file : %d ", fileObj);

   BOOL bRet = FALSE;
    IPersistent pIntf = ooc_get_interface(self, IPersistent);
    
    bRet = pIntf->Serialize((Object)self, TRUE, fileObj);
    return bRet;
}

static _u32 GetTaskIdFromFileName(const char *cszFileName)
{
    char *pos = sd_strrchr((char *)cszFileName, DIR_SPLIT_CHAR);
    _u32 id = 0;
    if (pos)
    {        
        id = strtoul(pos+1, 0, 0);
    }
    else
    {
        id = strtoul(cszFileName, 0, 0);
    }
   
    return id;
}

static
BOOL 
CPersistentTaskImpl_Open( CPersistentTaskImpl self, const char *cszFileName)
{
    BOOL bRet = FALSE;
    _int32 ret_val = 0;

   LOG_DEBUG("CPersistentTaskImpl::Open begin file path: %s ", cszFileName);

    _u32 task_id = GetTaskIdFromFileName(cszFileName);
    if (!task_id)
    {
        sd_delete_file( cszFileName );
        return bRet;
    }

    CIoFile ioFile = ooc_new( CIoFile, NULL );
    Object fileObj = (Object)ioFile;
    IFile pFile = ooc_get_interface(fileObj, IFile);
    bRet = CIoFileVirtual(ioFile)->Open( (Object)fileObj , cszFileName, "rb");
    if ( bRet )
    {
        bRet = CPersistentTaskImplVirtual(self)->Read((Object)self, fileObj );
        pFile->Close( (Object)fileObj );
        if (!bRet)
        {
                // ����ʧ�ܣ�����
            sd_delete_file( cszFileName );
            LOG_ERROR("CPersistentTaskImpl::Open task read fail task id:%u ", task_id);
            
        }
        else
        {
            //���task_id �Ƿ����ļ������
            Object pData = ((CPersistentImpl)self)->m_pData;
            _u32 read_task_id = CTaskVirtual((CTask)pData)->GetId(pData);
            
            if (task_id && task_id!=read_task_id)
            {
                LOG_DEBUG("CPersistentTaskImpl::Open task it's task id:%u is not equal to it in file name:%u", read_task_id, task_id);
                char task_file_path[MAX_FILE_PATH_LEN] = {0};
                EM_TASK *p_task = CTaskVirtual((CTask)pData)->GetAttachedData(pData);
                ret_val = dt_get_task_alone_store_file_path(p_task, task_file_path, MAX_FILE_PATH_LEN);
                sd_assert( SUCCESS == ret_val );
                if ( sd_file_exist(task_file_path))
                {
                    //�Ѿ�����һ��task_id��ͬ�������ļ������Զ���ǰ�ļ�
                    bRet = FALSE;
                    LOG_DEBUG("CPersistentTaskImpl::Open an other task is exist, it has the same task id: %u", read_task_id);
                }
                else
                {
                    ret_val = sd_rename_file(cszFileName, task_file_path);
                    bRet = (ret_val==SUCCESS);
                }
            }
        }
    }
    ooc_delete((Object)fileObj);
    
   LOG_DEBUG("CPersistentTaskImpl::Open finish file path: %s ", cszFileName);
    return bRet;
}




