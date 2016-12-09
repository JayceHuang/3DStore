#ifndef SD_PEERID_H_00138F8F2E70_2008007171747
#define SD_PEERID_H_00138F8F2E70_2008007171747
#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"

 _int32 get_physical_address(char *buffer, _int32 *bufsize);

_int32 get_peerid(char *buffer, _int32 bufsize);

_int32 set_peerid(const char* buff, _int32 length);

BOOL is_set_peerid(void);

void uninit_peerid(void);

#ifdef __cplusplus
}
#endif
#endif

