#ifndef EM_EXCEPTION_H_
#define EM_EXCEPTION_H_

#include <ooc/exception.h>
#include <ooc/implement/exception.h>

/* Inherited Exception for tests
 */

DeclareClass( EmException, Exception );

Exception em_exception_new( int em_error_code );

#endif /*EM_EXCEPTION_H_*/
