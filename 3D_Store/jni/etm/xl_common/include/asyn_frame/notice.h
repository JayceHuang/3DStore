#ifndef _SD_NOTICE_H_00138F8F2E70_200807101812
#define _SD_NOTICE_H_00138F8F2E70_200807101812

#include "utility/define.h"

#include "asyn_frame/notice_linux.h"
#include "asyn_frame/notice_macos.h"
#include "asyn_frame/notice_wince.h"

_int32 notice_init(void);

_int32 create_notice_handle(_int32 *notice_handle, _int32 *waitable_handle);

/* wait weakly, that means the caller need not depend to this signal. so we can change to msg-poll easily */
_int32 create_waitable_container(_int32 *waitable_container);

_int32 add_notice_handle(_int32 waitable_container, _int32 waitable_handle);
_int32 del_notice_handle(_int32 waitable_container, _int32 waitable_handle);

#define notice(notice_handle)  notice_impl(notice_handle)
_int32 notice_impl(_int32 notice_handle);


_int32 wait_for_notice(_int32 waitable_container, _int32 expect_handle_count, _int32 *noticed_handle, _u32 timeout);
_int32 reset_notice(_int32 waitable_handle);

_int32 destory_notice_handle(_int32 notice_handle, _int32 waitable_handle);
_int32 destory_waitable_container(_int32 waitable_container);

_int32 init_simple_event(SEVENT_HANDLE *handle);
_int32 wait_sevent_handle(SEVENT_HANDLE *handle);
_int32 signal_sevent_handle(SEVENT_HANDLE *handle);
_int32 uninit_simple_event(SEVENT_HANDLE *handle);

#endif
