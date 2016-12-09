#include <stdio.h>
#include "utility/logid.h"
#define LOGID EM_LOGID_DOWNLOAD_TASK
#include "utility/logger.h"

#include "MemoryFile.h"
#include "implement/MemoryFile.h"

#include <ooc/exception.h>


//�Ѿ���CIoFile�ж���
//AllocateInterface( IFile );

/* Allocating the class description table and the vtable
 */

InterfaceRegister( CMemoryFile )
{
    AddInterface( CMemoryFile, IFile )
};

AllocateClassWithInterface( CMemoryFile, Base );

/* Class virtual function prototypes
 */

static unsigned char* CMemoryFile_GetBuffer(CMemoryFile, BOOL bDetachBuffer);
static BOOL CMemoryFile_Open( CMemoryFile );
static BOOL CMemoryFile_Close( CMemoryFile ) ;
static size_t CMemoryFile_Read( CMemoryFile,  void *buffer, size_t size, size_t count) ;
static size_t CMemoryFile_Write( CMemoryFile,  const void *buffer, size_t size, size_t count);
static size_t CMemoryFile_PRead( CMemoryFile,  void *buffer, size_t size, size_t count, _int32 read_pos) ;
static size_t CMemoryFile_PWrite( CMemoryFile,  const void *buffer, size_t size, size_t count, _int32 write_pos);
static BOOL CMemoryFile_Seek( CMemoryFile,  _int32 offset, int origin) ;
static _int64 CMemoryFile_Tell( CMemoryFile) ;
static _int64 CMemoryFile_Size( CMemoryFile );
static BOOL CMemoryFile_Flush( CMemoryFile );
static BOOL CMemoryFile_Eof( CMemoryFile);
static _int32 CMemoryFile_Error( CMemoryFile);

static BOOL CMemoryFile_Alloc(CMemoryFile self,  _u32 dwNewLen);
static void CMemoryFile_Free(CMemoryFile self);


/* Class initializing
 */

static
void
CMemoryFile_initialize( Class this )
{
    CMemoryFileVtable vtab = & CMemoryFileVtableInstance;

    vtab->GetBuffer = ( unsigned char* (*)(Object, BOOL bDetachBuffer) )CMemoryFile_GetBuffer;
    vtab->Open = ( BOOL (*)( Object ) )CMemoryFile_Open;
    vtab->IFile.Close = ( BOOL (*)( Object )  )CMemoryFile_Close;
    vtab->IFile.Read = (size_t (*)( Object,  void *buffer, size_t size, size_t count) )CMemoryFile_Read;
    vtab->IFile.Write = ( size_t (*)( Object,  const void *buffer, size_t size, size_t count) )CMemoryFile_Write;
    vtab->IFile.PRead = (size_t (*)( Object,  void *buffer, size_t size, size_t count, _int32 read_pos) )CMemoryFile_PRead;
    vtab->IFile.PWrite = ( size_t (*)( Object,  const void *buffer, size_t size, size_t count, _int32 write_pos) )CMemoryFile_PWrite;
    vtab->IFile.Seek = ( BOOL (*)( Object,  _int32 offset, int origin) )CMemoryFile_Seek;
    vtab->IFile.Tell = ( _int64 (*)( Object) )CMemoryFile_Tell;
    vtab->IFile.Size = ( _int64 (*)( Object ) )CMemoryFile_Size;
    vtab->IFile.Flush = ( BOOL (*)( Object ) )CMemoryFile_Flush;
    vtab->IFile.Eof = ( BOOL (*)( Object) )CMemoryFile_Eof;
    vtab->IFile.Error = ( _int32 (*)( Object) )CMemoryFile_Error;
}

/* Class finalizing
 */

#ifndef OOC_NO_FINALIZE
static
void
CMemoryFile_finalize( Class this )
{
}
#endif


/* Constructor
 */

static
void
CMemoryFile_constructor( CMemoryFile self, const void * params )
{
    assert( ooc_isInitialized( CMemoryFile ) );
    
    chain_constructor( CMemoryFile, self, NULL );

    self->m_pBuffer = NULL;
    self->m_Position = 0;
    self->m_Size = self->m_Edge = 0;
    self->m_bFreeOnClose = TRUE;

}

/* Destructor
 */

static
void
CMemoryFile_destructor( CMemoryFile self, CMemoryFileVtable vtab )
{
    CMemoryFile_Close(self);
}

/* Copy constuctor
 */

static
int
CMemoryFile_copy( CMemoryFile self, const CMemoryFile from )
{
    sd_assert( ooc_isInstanceOf( self, CMemoryFile ) );

    return OOC_NO_COPY;
}

/*  =====================================================
    Class member functions
 */


CMemoryFile
CMemoryFile_new( void )
{
    ooc_init_class( CMemoryFile );

    return ooc_new( CMemoryFile, NULL );
}

static 
unsigned char* 
CMemoryFile_GetBuffer(CMemoryFile self, BOOL bDetachBuffer)
{
    sd_assert( ooc_isInstanceOf( self, CMemoryFile ) );
    
    //can only detach, avoid inadvertantly attaching to
    // memory that may not be ours [Jason De Arte]
    if( bDetachBuffer )
    	self->m_bFreeOnClose = FALSE;
    return self->m_pBuffer;
}

static
BOOL 
CMemoryFile_Open( CMemoryFile self )
{
    sd_assert( ooc_isInstanceOf( self, CMemoryFile ) );
    
    if (self->m_pBuffer) return FALSE;	// Can't re-open without closing first

    self->m_Position = self->m_Size = self->m_Edge = 0;
    self->m_pBuffer=(unsigned char*)malloc(1);
    self->m_bFreeOnClose = TRUE;

    return (self->m_pBuffer!=0);
}

static 
BOOL 
CMemoryFile_Close( CMemoryFile self) 
{
    //sd_assert( ooc_isInstanceOf( self, CMemoryFile ) );
    
    if ( (self->m_pBuffer) && (self->m_bFreeOnClose) ){
    	free(self->m_pBuffer);
    	self->m_pBuffer = NULL;
    	self->m_Size = 0;
    }
    return TRUE;
}

static 
size_t 
CMemoryFile_Read( CMemoryFile self,  void *buffer, size_t size, size_t count) 
{
    sd_assert( ooc_isInstanceOf( self, CMemoryFile ) );
    
    if (buffer==NULL) return 0;

    if (self->m_pBuffer==NULL) return 0;
    if (self->m_Position >= (_u64)self->m_Size) return 0;

    _u64 nCount = (_u64)(count*size);
    if (nCount == 0) return 0;

    _u64 nRead;
    if (self->m_Position + nCount > (_u64)self->m_Size)
    	nRead = (self->m_Size - self->m_Position);
    else
    	nRead = nCount;

    memcpy(buffer, self->m_pBuffer + self->m_Position, nRead);
    self->m_Position += nRead;

    return (size_t)(nRead/size);
}

static 
size_t 
CMemoryFile_Write( CMemoryFile self,  const void *buffer, size_t size, size_t count)
{
    sd_assert( ooc_isInstanceOf( self, CMemoryFile ) );
    
    if (self->m_pBuffer==NULL) return 0;
    if (buffer==NULL) return 0;

    _u64 nCount = (_u64)(count*size);
    if (nCount == 0) return 0;

    if (self->m_Position + nCount > self->m_Edge){
    	if (!CMemoryFile_Alloc(self, self->m_Position + nCount)){
    		return FALSE;
    	}
    }

    memcpy(self->m_pBuffer + self->m_Position, buffer, nCount);

    self->m_Position += nCount;

    if (self->m_Position > (_u64)self->m_Size) self->m_Size = self->m_Position;

    return count;

}

static 
size_t 
CMemoryFile_PRead( CMemoryFile self,  void *buffer, size_t size, size_t count, _int32 read_pos) 
{
    sd_assert( ooc_isInstanceOf( self, CMemoryFile ) );
    
    if (CMemoryFile_Seek(self, read_pos, SEEK_SET))
    {
        return CMemoryFile_Read(self,  buffer, size, count);
    }
    else
        return -1;
}

static 
size_t 
CMemoryFile_PWrite( CMemoryFile self,  const void *buffer, size_t size, size_t count, _int32 write_pos)
{
    sd_assert( ooc_isInstanceOf( self, CMemoryFile ) );
    
    if (CMemoryFile_Seek(self, write_pos, SEEK_SET))
    {
        return CMemoryFile_Write(self,  buffer, size, count);
    }
    else
        return -1;
}


static 
BOOL 
CMemoryFile_Seek( CMemoryFile self,  _int32 offset, int origin) 
{
    sd_assert( ooc_isInstanceOf( self, CMemoryFile ) );
    
    if (self->m_pBuffer==NULL) return FALSE;
    _u64 lNewPos = self->m_Position;

    if (origin == SEEK_SET)		 lNewPos = offset;
    else if (origin == SEEK_CUR) lNewPos += offset;
    else if (origin == SEEK_END) lNewPos = self->m_Size + offset;
    else return FALSE;

	if ((double)lNewPos < 0) lNewPos = 0;

    self->m_Position = lNewPos;
    return TRUE;
}

static 
_int64 
CMemoryFile_Tell( CMemoryFile self) 
{
    sd_assert( ooc_isInstanceOf( self, CMemoryFile ) );
    
    if (self->m_pBuffer==NULL) return -1;
    return self->m_Position;
}

static 
_int64 
CMemoryFile_Size( CMemoryFile  self)
{
    sd_assert( ooc_isInstanceOf( self, CMemoryFile ) );
    
    if (self->m_pBuffer==NULL) return -1;
    return self->m_Size;
}

static 
BOOL 
CMemoryFile_Flush( CMemoryFile self )
{
    sd_assert( ooc_isInstanceOf( self, CMemoryFile ) );
    
    if (self->m_pBuffer==NULL) return FALSE;
    return TRUE;
}

static 
BOOL 
CMemoryFile_Eof( CMemoryFile self)
{
    sd_assert( ooc_isInstanceOf( self, CMemoryFile ) );
    
    if (self->m_pBuffer==NULL) return TRUE;
    return (self->m_Position >= (_int64)self->m_Size);
}

static 
_int32 
CMemoryFile_Error( CMemoryFile self)
{
    sd_assert( ooc_isInstanceOf( self, CMemoryFile ) );
    
    if (self->m_pBuffer==NULL) return -1;
    return (self->m_Position > (_int64)self->m_Size);
}

static BOOL CMemoryFile_Alloc(CMemoryFile self, _u32 dwNewLen)
{
    sd_assert( ooc_isInstanceOf( self, CMemoryFile ) );
    
    if (dwNewLen > (_u32)self->m_Edge)
    {
    	// find new buffer size
    	_u32 dwNewBufferSize = (_u32)(((dwNewLen>>16)+1)<<16);

    	// allocate new buffer
    	if (self->m_pBuffer == NULL) self->m_pBuffer = (unsigned char *)malloc(dwNewBufferSize);
    	else	 self->m_pBuffer = (unsigned char *)realloc(self->m_pBuffer, dwNewBufferSize);
    	// I own this buffer now (caller knows nothing about it)
    	self->m_bFreeOnClose = TRUE;

    	self->m_Edge = dwNewBufferSize;
    }
    return (self->m_pBuffer!=0);

}

static void CMemoryFile_Free(CMemoryFile self)
{
    sd_assert( ooc_isInstanceOf( self, CMemoryFile ) );
    
    CMemoryFile_Close(self);
}

