// Linux 2.6,Ö§³Öepoll
#include <errno.h>
#include <unistd.h>

#include "utility/define.h"
#include "asyn_frame/selector.h"
#include "asyn_frame/epoll_wrap.h"
#include "utility/errcode.h"
#include "utility/mempool.h"
#include "utility/string.h"

#ifdef _EPOLL

#define LOGID	LOGID_ASYN_FRAME
#include "utility/logger.h"

_int32 create_selector(_int32 channel_count, void **selector)
{
    *selector = NULL;
	EPOLL_SELECTOR *pepoll = NULL;
	_int32 ret_val = sd_malloc(sizeof(EPOLL_SELECTOR), (void**)&pepoll);
	if (ret_val != SUCCESS)
	{
	    return ret_val;
	}

	pepoll->_selector_size = channel_count;
	pepoll->_epoll_fd = epoll_create(channel_count);
	if (pepoll->_epoll_fd == -1)
	{
		ret_val = errno;
		sd_free(pepoll);
		LOG_DEBUG("create epoll failed: %d", ret_val);
		return ret_val;
	}

	ret_val = sd_malloc((sizeof(struct epoll_event) * channel_count), (void*)&pepoll->_channel_events);
	if (ret_val != SUCCESS)
	{
		close(pepoll->_epoll_fd);
		sd_free(pepoll);
		return ret_val;
	}

	*selector = pepoll;
	return ret_val;
}

_int32 destory_selector(void *selector)
{
	EPOLL_SELECTOR *pepoll = selector;
	close(pepoll->_epoll_fd);

	_int32 ret_val = SUCCESS;
	ret_val = sd_free(pepoll->_channel_events);
	pepoll->_channel_events = NULL;
	CHECK_VALUE(ret_val);

	ret_val = sd_free(selector);
	selector = NULL;
	CHECK_VALUE(ret_val);

	return ret_val;
}
BOOL is_channel_full(void *selector)
{
	return FALSE;
}

_int32 add_a_channel(void *selector, _u32 channel_fd, _u32 channel_event, CHANNEL_DATA channel_data)
{
	_u32 event = 0;
	if (channel_event & CHANNEL_READ)
	{
	    event |= EPOLLIN;
	}
	if (channel_event & CHANNEL_WRITE)	
	{
	    event |= EPOLLOUT;
	}
	
	struct epoll_event ev;
	sd_memset(&ev, 0, sizeof(ev));
	ev.events = event;
	sd_memcpy(&ev.data, &channel_data, sizeof(CHANNEL_DATA));

	_int32 ret_val = SUCCESS;
	EPOLL_SELECTOR *pepoll = selector;
	if (epoll_ctl(pepoll->_epoll_fd, EPOLL_CTL_ADD, channel_fd, &ev) == -1)
	{
	    ret_val = errno;
	}

	return ret_val;
}

_int32 modify_a_channel(void *selector, _int32 channel_idx, _u32 channel_fd, _u32 channel_event, CHANNEL_DATA channel_data)
{
	_int32 ret_val = SUCCESS;
	_u32 event = 0;
	struct epoll_event ev;
	EPOLL_SELECTOR *pepoll = selector;

	if(channel_event & CHANNEL_READ)	event |= EPOLLIN;
	if(channel_event & CHANNEL_WRITE)	event |= EPOLLOUT;

	sd_memset(&ev, 0, sizeof(ev));
	ev.events = event;
	sd_memcpy(&ev.data, &channel_data, sizeof(CHANNEL_DATA));

	if(epoll_ctl(pepoll->_epoll_fd, EPOLL_CTL_MOD, channel_fd, &ev) == -1)
		ret_val = errno;

	return ret_val;
}

_int32 del_a_channel(void *selector, _int32 channel_idx, _u32 channel_fd)
{
    _int32 ret_val = SUCCESS;
	if (epoll_ctl(((EPOLL_SELECTOR*)selector)->_epoll_fd, EPOLL_CTL_DEL, channel_fd, NULL) == -1)
	{
	    ret_val = errno;
	}

	return ret_val;
}

_int32 get_next_channel(void *selector, _int32 *channel_idx)
{
	(*channel_idx) ++;
	return SUCCESS;
}

BOOL is_channel_error(void *selector, const _int32 channel_idx)
{
	return ((EPOLL_SELECTOR*)selector)->_channel_events[channel_idx].events & EPOLLERR;
}

_int32 get_channel_error(void *selector, const _int32 channel_idx)
{
	return SUCCESS;
}

BOOL is_channel_readable(void *selector, const _int32 channel_idx)
{
	return ((EPOLL_SELECTOR*)selector)->_channel_events[channel_idx].events & EPOLLIN;
}

BOOL is_channel_writable(void *selector, const _int32 channel_idx)
{
	return ((EPOLL_SELECTOR*)selector)->_channel_events[channel_idx].events & EPOLLOUT;
}

_int32 get_channel_data(void *selector, const _int32 channel_idx, CHANNEL_DATA *channel_data)
{
	sd_memcpy(channel_data, &(((EPOLL_SELECTOR*)selector)->_channel_events[channel_idx].data), sizeof(CHANNEL_DATA));
	return SUCCESS;
}

_int32 selector_wait(void *selector, _int32 timeout)
{
	EPOLL_SELECTOR *pepoll = selector;
	_int32 tmp_count = 0;

	do
	{
		tmp_count = epoll_wait(pepoll->_epoll_fd, pepoll->_channel_events, pepoll->_selector_size, timeout);
	} while (tmp_count < 0 && errno == EINTR);

	return tmp_count;
}

#endif
