#ifndef _MAPFILE_H_
#define _MAPFILE_H_

#include "utility/define.h"
#if defined (WINCE) && defined(_MEM_DEBUG)

#include "windows.h"
#include "string.h"
#include "stdlib.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" 
{
#endif

PSTR MapfileLookupAddress( IN  PWSTR wszModuleName, IN  DWORD dwAddress);
//
//  Return the function name at the specified address of the specified module if known.
//  Return "" if unknown.
//

#ifdef __cplusplus
}
#endif

#endif

#endif /* _MAPFILE_H_ */
