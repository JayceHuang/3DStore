#ifndef _SD_FS_REACTOR_H_00138F8F2E70_200807211101
#define _SD_FS_REACTOR_H_00138F8F2E70_200807211101

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#include "asyn_frame/msg.h"

_int32 init_fs_reactor(_int32 *waitable_handle, _int32 *slow_op_waitable_handle);
_int32 uninit_fs_reactor(void);

_int32 register_fs_event(MSG *msg);
_int32 unregister_fs(MSG *msg, _int32 reason);

_int32 get_complete_fs_msg(MSG **msg);

#ifdef __cplusplus
}
#endif

#endif
