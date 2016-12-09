/* Inherited Exception for tests
 */
 
#include "EmException.h"

ClassMembers( EmException, Exception )
EndOfClassMembers;

Virtuals( EmException, Exception )
EndOfVirtuals;

AllocateClass( EmException, Exception );

static	void	EmException_initialize( Class this ) {}
#ifndef OOC_NO_FINALIZE
static	void	EmException_finalize( Class this ) {}
#endif

static	void	EmException_constructor( EmException self, const void * params )
{
	assert( ooc_isInitialized( EmException ) );
	chain_constructor( EmException, self, NULL );
	if( params )
		self->Exception.user_code = * ( (int*) params );
}

static	void	EmException_destructor( EmException self, EmExceptionVtable vtab ) {}
static	int		EmException_copy( EmException self, const EmException from ) { return OOC_COPY_DEFAULT; }

Exception em_exception_new( int em_error_code ) 
{ 
	ooc_init_class( EmException );
	return (Exception) ooc_new( EmException, & em_error_code );
}

