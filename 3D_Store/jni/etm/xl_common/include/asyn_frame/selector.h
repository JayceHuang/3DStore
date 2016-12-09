#ifndef _SD_SELECTOR_H_00138F8F2E70_200809021219
#define _SD_SELECTOR_H_00138F8F2E70_200809021219

#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"

#define CHANNEL_READ	(1)
#define CHANNEL_WRITE	(2)

typedef union {
	void *_ptr;
	_u32 _fd;
} CHANNEL_DATA;


_int32 create_selector(_int32 channel_count, void **selector);
_int32 destory_selector(void *selector);

BOOL is_channel_full(void *selector);

/* add a socket channel, do not check this socket whether if exist */
_int32 add_a_channel(void *selector, _u32 channel_fd, _u32 channel_event, CHANNEL_DATA channel_data);
/* modify a socket channel */
_int32 modify_a_channel(void *selector, _int32 channel_idx, _u32 channel_fd, _u32 channel_event, CHANNEL_DATA channel_data);

_int32 del_a_channel(void *selector, _int32 channel_idx, _u32 channel_fd);


#define CHANNEL_IDX_FIND_BEGIN	(-1)
_int32 get_next_channel(void *selector, _int32 *channel_idx);

BOOL is_channel_error(void *selector, const _int32 channel_idx);
_int32 get_channel_error(void *selector, const _int32 channel_idx);
BOOL is_channel_readable(void *selector, const _int32 channel_idx);
BOOL is_channel_writable(void *selector, const _int32 channel_idx);

_int32 get_channel_data(void *selector, const _int32 channel_idx, CHANNEL_DATA *channel_data);

_int32 selector_wait(void *selector, _int32 timeout);
#ifdef __cplusplus
}
#endif
#endif
