#include <stdio.h>
#include "utility/logid.h"
#define LOGID EM_LOGID_DOWNLOAD_TASK
#include "utility/logger.h"

#include "PersistentImpl.h"
#include "implement/PersistentImpl.h"
#include "IoFile.h"
#include "MemoryFile.h"

#include "download_manager/download_task_store.h"


#include "utility/mempool.h"

#include <ooc/exception.h>


AllocateInterface( IPersistent );
AllocateInterface( IPersistentDocument );

/* Allocating the class description table and the vtable
 */

InterfaceRegister( CPersistentImpl )
{
    AddInterface( CPersistentImpl, IPersistent )
};

AllocateClassWithInterface( CPersistentImpl, Base );

/* Class virtual function prototypes
 */

static void CPersistent_SetDataObj( CPersistentImpl self, Object p_task );
static Object CPersistent_GetDataObj( CPersistentImpl self );

/* Class initializing
 */

static
void
CPersistentImpl_initialize( Class this )
{
    CPersistentImplVtable vtab = & CPersistentImplVtableInstance;
    
    vtab->IPersistent.SetDataObj    =   (void (*)(Object, Object p_task ))  CPersistent_SetDataObj;
    vtab->IPersistent.GetDataObj    =   (Object (*)(Object ))   CPersistent_GetDataObj;
}

/* Class finalizing
 */

#ifndef OOC_NO_FINALIZE
static
void
CPersistentImpl_finalize( Class this )
{
}
#endif


/* Constructor
 */

static
void
CPersistentImpl_constructor( CPersistentImpl self, const void * params )
{
    assert( ooc_isInitialized( CPersistentImpl ) );
    
    chain_constructor( CPersistentImpl, self, NULL );

    self->m_pData = NULL;
}

/* Destructor
 */

static
void
CPersistentImpl_destructor( CPersistentImpl self, CPersistentImplVtable vtab )
{
}

/* Copy constuctor
 */

static
int
CPersistentImpl_copy( CPersistentImpl self, const CPersistentImpl from )
{
    self->m_pData  = from->m_pData;

    return OOC_COPY_DONE;
}

/*  =====================================================
    Class member functions
 */


CPersistentImpl
CPersistentImpl_new( void )
{
    ooc_init_class( CPersistentImpl );

    return ooc_new( CPersistentImpl, NULL );
}

static 
void 
CPersistent_SetDataObj( CPersistentImpl self, Object p_data )
{
    assert( ooc_isInstanceOf( self, CPersistentImpl ) );
    self->m_pData = p_data;
}

static 
Object 
CPersistent_GetDataObj( CPersistentImpl self )
{
    assert( ooc_isInstanceOf( self, CPersistentImpl ) );
    return self->m_pData;
}

_int32 Util_Read(Object fileObj, void *buf, _u32 read_len, _int32 read_pos)
{
    _int32 ret_val = 0;
    _int32 readsize = 0;
    IFile pFile = ooc_get_interface(fileObj, IFile);

    LOG_DEBUG("Util_Read:file=%x, read pos:%d, read len:%d", fileObj, read_pos, read_len);

    if (0==read_len) return SUCCESS;

    if (read_pos>=0)
        readsize = pFile->PRead((Object)fileObj, (void *)buf, 1, read_len, read_pos);
    else
        readsize = pFile->Read((Object)fileObj, (void *)buf, 1, read_len);
    if (readsize<0)
    {
        ret_val = readsize;
        LOG_ERROR("Util_Read:file=%x, read pos:%d, read len:%d, fail result:%d", fileObj, read_pos, read_len, readsize);
        return ret_val;
    }
    if(readsize!=read_len)
    {
        LOG_ERROR("Util_Read:file=%x, read pos:%d, expect read len:%d, but:%d", fileObj, read_pos, read_len, readsize);
        ret_val = INVALID_READ_SIZE;
        return ret_val;
    }
    return SUCCESS;
}

_int32 Util_AllocAndRead(Object fileObj, void **buf, _u32 buf_need_len, _u32 read_len, _int32 read_pos)
{
    _int32 ret_val = 0;
    IFile pFile = ooc_get_interface(fileObj, IFile);

    if (0==read_len) return SUCCESS;

    ret_val = sd_malloc(buf_need_len, buf);
    sd_assert(SUCCESS==ret_val);
    if (SUCCESS != ret_val) return ret_val;
    sd_memset(*buf, 0, buf_need_len);

    ret_val = Util_Read(fileObj, *buf, read_len, read_pos);
    if (SUCCESS != ret_val)
    {
        SAFE_DELETE(*buf);
        return ret_val;
    }
    if (buf_need_len==read_len+1)
    {
        LOG_DEBUG("Util_AllocAndRead:file=%x, read content:%s", fileObj, *buf);    
    }
    return SUCCESS;
}

_int32 Util_Write(Object fileObj, void *buf, _u32 write_len, _int32 write_pos)
{
    _int32 ret_val = SUCCESS;
    _int32 writesize=0;
    IFile pFile = ooc_get_interface(fileObj, IFile);

    if (write_pos>=0)
        writesize = pFile->PWrite((Object)fileObj, (char *)buf, 1, write_len, write_pos);
    else
        writesize = pFile->Write((Object)fileObj, (char *)buf, 1, write_len);
    LOG_DEBUG("Util_Write:file=%u, write pos:%d, write len:%d, result:%d", fileObj, write_pos, write_len, writesize);
    sd_assert(writesize==write_len);
    if (writesize<0) return writesize; //fail
    if(writesize!=write_len)
    {
        return INVALID_WRITE_SIZE;
    }
    return ret_val;
}
