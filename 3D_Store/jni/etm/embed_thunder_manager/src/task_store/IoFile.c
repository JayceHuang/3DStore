#include <stdio.h>
#include "utility/logid.h"
#define LOGID EM_LOGID_DOWNLOAD_TASK
#include "utility/logger.h"

#include "IoFile.h"
#include "implement/IoFile.h"


#include <ooc/exception.h>


AllocateInterface( IFile );

/* Allocating the class description table and the vtable
 */

InterfaceRegister( CIoFile )
{
    AddInterface( CIoFile, IFile )
};

AllocateClassWithInterface( CIoFile, Base );

/* Class virtual function prototypes
 */

static BOOL CIoFile_Open( CIoFile, const char * filename, const char * mode);
static BOOL CIoFile_Close( CIoFile ) ;
static size_t CIoFile_Read( CIoFile,  void *buffer, size_t size, size_t count) ;
static size_t CIoFile_Write( CIoFile,  const void *buffer, size_t size, size_t count);
static size_t CIoFile_PRead( CIoFile,  void *buffer, size_t size, size_t count, _int32 read_pos) ;
static size_t CIoFile_PWrite( CIoFile,  const void *buffer, size_t size, size_t count, _int32 write_pos);
static BOOL CIoFile_Seek( CIoFile,  _int32 offset, int origin) ;
static _int64 CIoFile_Tell( CIoFile) ;
static _int64 CIoFile_Size( CIoFile );
static BOOL CIoFile_Flush( CIoFile );
static BOOL CIoFile_Eof( CIoFile);
static _int32 CIoFile_Error( CIoFile);


/* Class initializing
 */

static
void
CIoFile_initialize( Class this )
{
    CIoFileVtable vtab = & CIoFileVtableInstance;

    vtab->Open = ( BOOL (*)( Object, const char * filename, const char * mode) )CIoFile_Open;
    vtab->IFile.Close = ( BOOL (*)( Object )  )CIoFile_Close;
    vtab->IFile.Read = (size_t (*)( Object,  void *buffer, size_t size, size_t count) )CIoFile_Read;
    vtab->IFile.Write = ( size_t (*)( Object,  const void *buffer, size_t size, size_t count) )CIoFile_Write;
    vtab->IFile.PRead = (size_t (*)( Object,  void *buffer, size_t size, size_t count, _int32 read_pos) )CIoFile_PRead;
    vtab->IFile.PWrite = ( size_t (*)( Object,  const void *buffer, size_t size, size_t count, _int32 write_pos) )CIoFile_PWrite;    
    vtab->IFile.Seek = ( BOOL (*)( Object,  _int32 offset, int origin) )CIoFile_Seek;
    vtab->IFile.Tell = ( _int64 (*)( Object) )CIoFile_Tell;
    vtab->IFile.Size = ( _int64 (*)( Object ) )CIoFile_Size;
    vtab->IFile.Flush = ( BOOL (*)( Object ) )CIoFile_Flush;
    vtab->IFile.Eof = ( BOOL (*)( Object) )CIoFile_Eof;
    vtab->IFile.Error = ( _int32 (*)( Object) )CIoFile_Error;
}

/* Class finalizing
 */

#ifndef OOC_NO_FINALIZE
static
void
CIoFile_finalize( Class this )
{
}
#endif


/* Constructor
 */

static
void
CIoFile_constructor( CIoFile self, const void * params )
{
    assert( ooc_isInitialized( CIoFile ) );
    
    chain_constructor( CIoFile, self, NULL );

    self->m_fp = NULL;
    self->m_bCloseFile = FALSE;
}

/* Destructor
 */

static
void
CIoFile_destructor( CIoFile self, CIoFileVtable vtab )
{
    CIoFile_Close(self);
}

/* Copy constuctor
 */

static
int
CIoFile_copy( CIoFile self, const CIoFile from )
{
    sd_assert( ooc_isInstanceOf( self, CIoFile ) );
    //self->m_fp = from->m_fp;
    //self->m_bCloseFile = from->m_bCloseFile;

    return OOC_NO_COPY;
}

/*  =====================================================
    Class member functions
 */


CIoFile
CIoFile_new( void )
{
    ooc_init_class( CIoFile );

    return ooc_new( CIoFile, NULL );
}

static
BOOL CIoFile_Open( CIoFile self, const char * filename, const char * mode)
{
    sd_assert( ooc_isInstanceOf( self, CIoFile ) );
    
    if (self->m_fp) return FALSE;	// Can't re-open without closing first

    self->m_fp = fopen(filename, mode);
    if (!self->m_fp) return FALSE;

    self->m_bCloseFile = TRUE;

    return TRUE;
}

static 
BOOL 
CIoFile_Close( CIoFile self) 
{
    //sd_assert( ooc_isInstanceOf( self, CIoFile ) );
    
    _int32 iErr = 0;
    if ( (self->m_fp) && (self->m_bCloseFile) ){ 
    	iErr = fclose(self->m_fp);
    	self->m_fp = NULL;
    }
    return (BOOL)(iErr==0);

}

static 
size_t 
CIoFile_Read( CIoFile self,  void *buffer, size_t size, size_t count) 
{
    sd_assert( ooc_isInstanceOf( self, CIoFile ) );
    
    if (!self->m_fp) return 0;
    return fread(buffer, size, count, self->m_fp);
}

static 
size_t 
CIoFile_Write( CIoFile self,  const void *buffer, size_t size, size_t count)
{
    sd_assert( ooc_isInstanceOf( self, CIoFile ) );
    
    if (!self->m_fp) return 0;
    return fwrite(buffer, size, count, self->m_fp);
}

static 
size_t 
CIoFile_PRead( CIoFile self,  void *buffer, size_t size, size_t count, _int32 read_pos) 
{
    sd_assert( ooc_isInstanceOf( self, CIoFile ) );
    
    if (CIoFile_Seek(self, read_pos, SEEK_SET))
    {
        return CIoFile_Read(self,  buffer, size, count);
    }
    else
        return -1;
}

static 
size_t 
CIoFile_PWrite( CIoFile self,  const void *buffer, size_t size, size_t count, _int32 write_pos)
{
    sd_assert( ooc_isInstanceOf( self, CIoFile ) );
    
    if (CIoFile_Seek(self, write_pos, SEEK_SET))
    {
        return CIoFile_Write(self,  buffer, size, count);
    }
    else
        return -1;
}

static 
BOOL 
CIoFile_Seek( CIoFile self,  _int32 offset, int origin) 
{
    sd_assert( ooc_isInstanceOf( self, CIoFile ) );
    
    if (!self->m_fp) return FALSE;
    return (BOOL)(fseek(self->m_fp, offset, origin) == 0);
}

static 
_int64 
CIoFile_Tell( CIoFile self) 
{
    sd_assert( ooc_isInstanceOf( self, CIoFile ) );
    
    if (!self->m_fp) return 0;
    return ftell(self->m_fp);
}

static 
_int64 
CIoFile_Size( CIoFile  self)
{
    sd_assert( ooc_isInstanceOf( self, CIoFile ) );
    
    if (!self->m_fp) return -1;
    _int64 pos,size;
    pos = ftell(self->m_fp);
    fseek(self->m_fp, 0, SEEK_END);
    size = ftell(self->m_fp);
    fseek(self->m_fp, pos,SEEK_SET);
    return size;
}

static 
BOOL 
CIoFile_Flush( CIoFile self )
{
    sd_assert( ooc_isInstanceOf( self, CIoFile ) );
    
    if (!self->m_fp) return FALSE;
    return (BOOL)(fflush(self->m_fp) == 0);
}

static 
BOOL 
CIoFile_Eof( CIoFile self)
{
    sd_assert( ooc_isInstanceOf( self, CIoFile ) );
    
    if (!self->m_fp) return TRUE;
    return (BOOL)(feof(self->m_fp) != 0);
}

static 
_int32 
CIoFile_Error( CIoFile self)
{
    sd_assert( ooc_isInstanceOf( self, CIoFile ) );
    
    if (!self->m_fp) return -1;
    return ferror(self->m_fp);
}
