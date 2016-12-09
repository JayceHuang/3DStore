#include <stdio.h>
#include "utility/logid.h"
#define LOGID EM_LOGID_DOWNLOAD_TASK
#include "utility/logger.h"

#include <ooc/exception.h>

#include "PersistentTaskEmuleImpl.h"
#include "implement/PersistentTaskEmuleImpl.h"


AllocateClass( CPersistentTaskEmuleImpl, CPersistentTaskP2spImpl );
/* Class virtual function prototypes
 */

static BOOL CPersistentTaskEmuleImpl_Serialize( CPersistentTaskEmuleImpl self, BOOL bRead, Object fileObj );

/* Class initializing
 */

static
void
CPersistentTaskEmuleImpl_initialize( Class this )
{
    CPersistentTaskEmuleImplVtable vtab = & CPersistentTaskEmuleImplVtableInstance;
    
    vtab->CPersistentTaskP2spImpl.CPersistentTaskImpl.CPersistentImpl.IPersistent.Serialize =   (BOOL (*)(Object, BOOL bRead, Object fileObj ))  CPersistentTaskEmuleImpl_Serialize;
}

/* Class finalizing
 */

#ifndef OOC_NO_FINALIZE
static
void
CPersistentTaskEmuleImpl_finalize( Class this )
{
}
#endif


/* Constructor
 */

static
void
CPersistentTaskEmuleImpl_constructor( CPersistentTaskEmuleImpl self, const void * params )
{
    assert( ooc_isInitialized( CPersistentTaskEmuleImpl ) );
    
    chain_constructor( CPersistentTaskEmuleImpl, self, params );
}

/* Destructor
 */

static
void
CPersistentTaskEmuleImpl_destructor( CPersistentTaskEmuleImpl self, CPersistentTaskEmuleImplVtable vtab )
{
}

/* Copy constuctor
 */

static
int
CPersistentTaskEmuleImpl_copy( CPersistentTaskEmuleImpl self, const CPersistentTaskEmuleImpl from )
{
    

    return OOC_COPY_DONE;
}

/*  =====================================================
    Class member functions
 */


CPersistentTaskEmuleImpl
CPersistentTaskEmuleImpl_new( void )
{
    ooc_init_class( CPersistentTaskEmuleImpl );

    return ooc_new( CPersistentTaskEmuleImpl, NULL );
}

static
BOOL
CPersistentTaskEmuleImpl_Serialize( CPersistentTaskEmuleImpl self, BOOL bRead, Object fileObj )
{
    sd_assert( ooc_isInstanceOf( self, CPersistentTaskEmuleImpl ) );
    
    LOG_DEBUG("CPersistentTaskEmuleImpl::Serialize begin");

    return CPersistentTaskEmuleImplParentVirtual( self )->CPersistentTaskImpl.CPersistentImpl.IPersistent.Serialize((Object)self, bRead, fileObj );
}
