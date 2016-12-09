// Linux 2.3内核，不支持epoll
//NOTE:jieouy android1.5对应的linux内核为2.6.
#include <errno.h>
#include <unistd.h>

#include "asyn_frame/selector.h"
#include "asyn_frame/poll_wrap.h"
#include "utility/errcode.h"
#include "utility/mempool.h"
#include "utility/string.h"

#define LOGID	LOGID_ASYN_FRAME
#include "utility/logger.h"

#ifdef _POLL

_int32 create_selector(_int32 channel_count, void **selector)
{
    *selector = NULL;
    POLL_SELECTOR *ppoll = NULL;
	_int32 ret_val = sd_malloc(sizeof(POLL_SELECTOR), (void**)&ppoll);
	if (ret_val != SUCCESS)
	{
	    return ret_val;
	}

	ret_val = sd_malloc(channel_count * sizeof(struct pollfd), (void**)&ppoll->_channel_events);
	if (ret_val != SUCCESS)
	{
		sd_free(ppoll);
		return ret_val;
	}

	ret_val = sd_malloc(channel_count * sizeof(CHANNEL_DATA), (void**)&ppoll->_channel_data);
	if (ret_val != SUCCESS)
	{
		sd_free(ppoll->_channel_events);
		ppoll->_channel_events = NULL;
		sd_free(ppoll);
		return ret_val;
	}

	_int32 i = 0;
	for (i = 0; i < channel_count; i++)
	{
		ppoll->_channel_events[i].fd = -1;
		ppoll->_channel_events[i].events = 0;
		ppoll->_channel_events[i].revents = 0;
		
		ppoll->_channel_data[i]._ptr = NULL;
	}

	ppoll->_selector_size = channel_count;
	ppoll->_first_free_idx = 0;
	ppoll->_max_available_idx = -1;

	*selector= ppoll;

	return ret_val;
}

_int32 destory_selector(void *selector)
{
	_int32 ret_val = SUCCESS;
	POLL_SELECTOR *ppoll = selector;

	ret_val = sd_free(ppoll->_channel_data);
	ppoll->_channel_data = NULL;
	CHECK_VALUE(ret_val);

	ret_val = sd_free(ppoll->_channel_events);
	ppoll->_channel_events = NULL;
	CHECK_VALUE(ret_val);

	ret_val = sd_free(selector);
	selector = NULL;
	CHECK_VALUE(ret_val);

	return ret_val;
}

BOOL is_channel_full(void *selector)
{
	POLL_SELECTOR *ppoll = selector;
	if (ppoll->_first_free_idx >= ppoll->_selector_size)
	{
	    return TRUE;
	}
	else
	{
	    return FALSE;
	}
}

/* add a socket channel, do not check this socket whether if exist */
_int32 add_a_channel(void *selector, _u32 channel_fd, _u32 channel_event, CHANNEL_DATA channel_data)
{
    if (is_channel_full(selector))
    {
        return NO_ROOM_OF_POLL;
    }
    
	_int32 ret_val = SUCCESS, i = 0;
	POLL_SELECTOR *ppoll = selector;

	ppoll->_channel_events[ppoll->_first_free_idx].events = 0;
	if(channel_event & CHANNEL_READ)
		ppoll->_channel_events[ppoll->_first_free_idx].events |= POLLIN;
	if(channel_event & CHANNEL_WRITE)
		ppoll->_channel_events[ppoll->_first_free_idx].events |= POLLOUT;

	ppoll->_channel_events[ppoll->_first_free_idx].revents = 0;
	ppoll->_channel_events[ppoll->_first_free_idx].fd = channel_fd;

	sd_memcpy((void*)(ppoll->_channel_data + ppoll->_first_free_idx), &channel_data, sizeof(CHANNEL_DATA));

	if(ppoll->_first_free_idx > ppoll->_max_available_idx)
		ppoll->_max_available_idx = ppoll->_first_free_idx;

	/* find next free idx */
	for(i = ppoll->_first_free_idx; i < ppoll->_selector_size; i++)
	{
		if(ppoll->_channel_events[i].fd == -1)
		{
			break;
		}
	}
	ppoll->_first_free_idx = i;

	return ret_val;
}

/* modify a socket channel */
_int32 modify_a_channel(void *selector, _int32 channel_idx, _u32 channel_fd, _u32 channel_event, CHANNEL_DATA channel_data)
{
	_int32 ret_val = SUCCESS, i = 0;
	POLL_SELECTOR *ppoll = selector;

	if(channel_idx == -1)
	{/* find idx by channel_fd */
		for(i = 0; i <= ppoll->_max_available_idx; i++)
		{
			if(channel_fd == ppoll->_channel_events[i].fd)
			{
				channel_idx = i;
				break;
			}
		}
	}
	if(channel_idx == -1)
		return BAD_POLL_ARUMENT;

	ppoll->_channel_events[channel_idx].events = 0;

	if(channel_event & CHANNEL_READ)
		ppoll->_channel_events[channel_idx].events |= POLLIN;
	if(channel_event & CHANNEL_WRITE)
		ppoll->_channel_events[channel_idx].events |= POLLOUT;

	ppoll->_channel_events[channel_idx].revents = 0;
/*
	ppoll->_channel_events[channel_idx].fd = channel_fd;
*/

	sd_memcpy((void*)(ppoll->_channel_data + channel_idx), &channel_data, sizeof(CHANNEL_DATA));

	return ret_val;
}

_int32 del_a_channel(void *selector, _int32 channel_idx, _u32 channel_fd)
{
	_int32 ret_val = SUCCESS, i = 0;
	POLL_SELECTOR *ppoll = selector;

	if(channel_idx == -1)
	{/* find idx by channel_fd */
		for(i = 0; i <= ppoll->_max_available_idx; i++)
		{
			if(channel_fd == ppoll->_channel_events[i].fd)
			{
				channel_idx = i;
				break;
			}
		}
	}
	if(channel_idx == -1)
		return BAD_POLL_ARUMENT;

	ppoll->_channel_events[channel_idx].fd = -1;
	ppoll->_channel_events[channel_idx].events = 0;
	ppoll->_channel_events[channel_idx].revents = 0;
	ppoll->_channel_data[channel_idx]._ptr = NULL;

	if(ppoll->_first_free_idx > channel_idx)
		ppoll->_first_free_idx = channel_idx;

	if(ppoll->_max_available_idx == channel_idx)
	{/* find next max_avialbe_fd*/
		for(i = ppoll->_max_available_idx; i >= ppoll->_first_free_idx; i--)
		{
			if(ppoll->_channel_events[i].fd > 0)
				break;
		}
		ppoll->_max_available_idx = i;
	}

	return ret_val;
}

_int32 get_next_channel(void *selector, _int32 *channel_idx)
{
	_int32 i = 0;
	POLL_SELECTOR *ppoll = selector;

	for(i = (*channel_idx) + 1; i <= ppoll->_max_available_idx; i++)
	{
		if(ppoll->_channel_events[i].fd > 0 && ppoll->_channel_events[i].revents > 0)
		{
			*channel_idx = i;
			return SUCCESS;
		}
	}

	return NOT_FOUND_POLL_EVENT;
}

BOOL is_channel_error(void *selector, const _int32 channel_idx)
{
	return ((POLL_SELECTOR*)selector)->_channel_events[channel_idx].revents & (POLLERR | POLLHUP | POLLNVAL);
}

_int32 get_channel_error(void *selector, const _int32 channel_idx)
{
	return SUCCESS;
}


BOOL is_channel_readable(void *selector, const _int32 channel_idx)
{
	return ((POLL_SELECTOR*)selector)->_channel_events[channel_idx].revents & (POLLIN);
}

BOOL is_channel_writable(void *selector, const _int32 channel_idx)
{
	return ((POLL_SELECTOR*)selector)->_channel_events[channel_idx].revents & (POLLOUT);
}

_int32 get_channel_data(void *selector, const _int32 channel_idx, CHANNEL_DATA *channel_data)
{
	_int32 ret_val = SUCCESS;
	POLL_SELECTOR *ppoll = selector;

	sd_memcpy(channel_data, (void*)(ppoll->_channel_data + channel_idx), sizeof(CHANNEL_DATA));

	return ret_val;
}

_int32 selector_wait(void *selector, _int32 timeout)
{
	POLL_SELECTOR *ppoll = selector;
	_int32 tmp_count = 0;

	do{
		tmp_count = poll(ppoll->_channel_events, ppoll->_max_available_idx + 1, timeout);
	}while (tmp_count < 0 && errno == EINTR);

	return tmp_count;

}

#endif
