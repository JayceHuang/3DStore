#ifndef EM_VERSION_H_00138F8F2E70_2008007261031
#define EM_VERSION_H_00138F8F2E70_2008007261031

#include "em_common/em_define.h"

#define ETM_INNER_VERSION ("1.9.0")

_int32 em_get_version(char *buffer, _int32 bufsize);
BOOL em_is_et_version_ok(void);


#endif
