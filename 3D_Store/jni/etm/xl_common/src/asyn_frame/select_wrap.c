// 通用平台
#include "asyn_frame/selector.h"
#include "asyn_frame/select_wrap.h"
#include "utility/errcode.h"
#include "utility/mempool.h"
#include "utility/string.h"

#ifdef _SELECT

#if defined(WINCE)
//#include <sys/types.h>
#include <winsock2.h>
#define POLLIN		  1
#define POLLPRI		  2
#define POLLOUT		  4
#define POLLERR		  8
#define POLLHUP		 16
#define POLLNVAL	 32
#define POLLRDNORM	 64
#define POLLWRNORM	POLLOUT
#define POLLRDBAND	128
#define POLLWRBAND	256
#define POLLMSG		0x0400

#elif defined(LINUX)
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/select.h>

#define POLLIN		  1
#define POLLPRI		  2
#define POLLOUT		  4
#define POLLERR		  8
#define POLLHUP		 16
#define POLLNVAL	 32
#define POLLRDNORM	 64
#define POLLWRNORM	POLLOUT
#define POLLRDBAND	128
#define POLLWRBAND	256
#define POLLMSG		0x0400

#endif


#define LOGID	LOGID_ASYN_FRAME
#include "utility/logger.h"


_int32 create_selector(_int32 channel_count, void **selector)
{
	_int32 ret_val = SUCCESS;
	SELECT_SELECTOR *ppoll = NULL;
	_int32 i = 0;

	ret_val = sd_malloc(sizeof(SELECT_SELECTOR), selector);
	if(ret_val != SUCCESS) return ret_val;

	ppoll = *selector;

	ret_val = sd_malloc(channel_count * sizeof(struct pollfd), (void**)&ppoll->_channel_events);
	if(ret_val != SUCCESS)
	{
		sd_free(*selector);
		*selector = NULL;
		return ret_val;
	}

	ret_val = sd_malloc(channel_count * sizeof(CHANNEL_DATA), (void**)&ppoll->_channel_data);
	if(ret_val != SUCCESS)
	{
		sd_free(ppoll->_channel_events);
		ppoll->_channel_events = NULL;
		sd_free(*selector);
		*selector = NULL;
		return ret_val;
	}

	for(i = 0; i < channel_count; i++)
	{
		ppoll->_channel_events[i].fd = -1;
		ppoll->_channel_events[i].events = 0;
		ppoll->_channel_events[i].revents = 0;

		ppoll->_channel_data[i]._ptr = NULL;
	}

	ppoll->_selector_size = channel_count;
	ppoll->_first_free_idx = 0;
	ppoll->_max_aviable_idx = -1;

	return ret_val;
}

_int32 destory_selector(void *selector)
{
	_int32 ret_val = SUCCESS;
	SELECT_SELECTOR *ppoll = selector;

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
	SELECT_SELECTOR *ppoll = selector;

	if(ppoll->_first_free_idx >= ppoll->_selector_size)
		return TRUE;
	
	return FALSE;
}

/* add a socket channel, do not check this socket whether if exist */
_int32 add_a_channel(void *selector, _u32 channel_fd, _u32 channel_event, CHANNEL_DATA channel_data)
{
	_int32 ret_val = SUCCESS, i = 0;
	SELECT_SELECTOR *ppoll = selector;

	if(ppoll->_first_free_idx >= ppoll->_selector_size)
		return NO_ROOM_OF_POLL;

	ppoll->_channel_events[ppoll->_first_free_idx].events = 0;
	if(channel_event & (CHANNEL_READ | CHANNEL_WRITE))
	{
		if(channel_event & CHANNEL_READ)
			ppoll->_channel_events[ppoll->_first_free_idx].events |= POLLIN;
		if(channel_event & CHANNEL_WRITE)
			ppoll->_channel_events[ppoll->_first_free_idx].events |= POLLOUT;
	}
	else
	{
		LOG_ERROR("bad event of poll selector %d", channel_event);
		return BAD_POLL_EVENT;
	}

	ppoll->_channel_events[ppoll->_first_free_idx].revents = 0;
	ppoll->_channel_events[ppoll->_first_free_idx].fd = channel_fd;

	sd_memcpy((void*)(ppoll->_channel_data + ppoll->_first_free_idx), &channel_data, sizeof(CHANNEL_DATA));

	if(ppoll->_first_free_idx > ppoll->_max_aviable_idx)
		ppoll->_max_aviable_idx = ppoll->_first_free_idx;

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
	SELECT_SELECTOR *ppoll = selector;

	if(channel_idx == -1)
	{/* find idx by channel_fd */
		for(i = 0; i <= ppoll->_max_aviable_idx; i++)
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

	sd_memcpy((void*)(ppoll->_channel_data + channel_idx), &channel_data, sizeof(CHANNEL_DATA));

	return ret_val;
}

_int32 del_a_channel(void *selector, _int32 channel_idx, _u32 channel_fd)
{
	_int32 ret_val = SUCCESS, i = 0;
	SELECT_SELECTOR *ppoll = selector;

	if(channel_idx == -1)
	{/* find idx by channel_fd */
		for(i = 0; i <= ppoll->_max_aviable_idx; i++)
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

	if(ppoll->_max_aviable_idx == channel_idx)
	{/* find next max_avialbe_fd*/
		for(i = ppoll->_max_aviable_idx; i >= ppoll->_first_free_idx; i--)
		{
			if(ppoll->_channel_events[i].fd > 0)
				break;
		}
		ppoll->_max_aviable_idx = i;
	}

	return ret_val;
}

_int32 get_next_channel(void *selector, _int32 *channel_idx)
{
	_int32 i = 0;
	SELECT_SELECTOR *ppoll = selector;

	for(i = (*channel_idx) + 1; i <= ppoll->_max_aviable_idx; i++)
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
	return ((SELECT_SELECTOR*)selector)->_channel_events[channel_idx].revents & (POLLERR | POLLHUP | POLLNVAL);
}

_int32 get_channel_error(void *selector, const _int32 channel_idx)
{
	return SUCCESS;
}

BOOL is_channel_readable(void *selector, const _int32 channel_idx)
{
	return ((SELECT_SELECTOR*)selector)->_channel_events[channel_idx].revents & (POLLIN);
}

BOOL is_channel_writable(void *selector, const _int32 channel_idx)
{
	return ((SELECT_SELECTOR*)selector)->_channel_events[channel_idx].revents & (POLLOUT);
}

_int32 get_channel_data(void *selector, const _int32 channel_idx, CHANNEL_DATA *channel_data)
{
	_int32 ret_val = SUCCESS;
	SELECT_SELECTOR *ppoll = selector;

	sd_memcpy(channel_data, (void*)(ppoll->_channel_data + channel_idx), sizeof(CHANNEL_DATA));

	return ret_val;
}

_int32 selector_wait(void *selector, _int32 timeout)
{
	SELECT_SELECTOR *ppoll = selector;
	struct timeval to, *p_to;
	_int32 tmp_count = 0;
	_int32 maxfd = 0;
	fd_set rfds, wfds;
#if defined(WINCE)	
	fd_set xfds;
#endif
	_int32 i = 0 ;

	timeout = 10;
	
	if(timeout < 0)
		p_to = NULL;
	else
	{
		to.tv_sec = timeout/1000;
		to.tv_usec = (timeout % 1000) * 1000;

		p_to= &to;
	}


	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
#if defined(WINCE)	
	FD_ZERO(&xfds);
#endif
	
	for ( i = 0; i <= ppoll->_max_aviable_idx; ++i)
	{
		if(ppoll->_channel_events[i].fd > 0)
		{
			if(ppoll->_channel_events[i].events & POLLIN)
			{
				FD_SET(ppoll->_channel_events[i].fd, &rfds);
				LOG_DEBUG("select wait add event fd:%d  index:%d readop", ppoll->_channel_events[i].fd, i);
			}
			if(ppoll->_channel_events[i].events & POLLOUT)
			{
				FD_SET(ppoll->_channel_events[i].fd, &wfds);
				LOG_DEBUG("select wait add event fd:%d  index:%d writeop", ppoll->_channel_events[i].fd, i);
			}

#if defined(WINCE)	
			FD_SET(ppoll->_channel_events[i].fd, &xfds);
#endif

			ppoll->_channel_events[i].revents = 0;

			if(ppoll->_channel_events[i].fd > maxfd)
				maxfd = ppoll->_channel_events[i].fd;

#if defined(WINCE)	
			if(!(ppoll->_channel_events[i].events & POLLIN || ppoll->_channel_events[i].events & POLLOUT))
			{
				printf("a empty event, fd:%d\n", ppoll->_channel_events[i].fd);
			}
#endif
             }
	}


                   #if defined(WINCE)	
	                   tmp_count = select(maxfd+1, &rfds, &wfds, &xfds,p_to);
                  #endif
		if(tmp_count > 0)
		{
			for ( i = 0; i <= ppoll->_max_aviable_idx; ++i)
			{
				if(FD_ISSET(ppoll->_channel_events[i].fd, &rfds))
				{
				       LOG_DEBUG("select wait receive event fd:%d  index:%d POLLIN", ppoll->_channel_events[i].fd, i);
					ppoll->_channel_events[i].revents |= POLLIN;
				}
				if(FD_ISSET(ppoll->_channel_events[i].fd, &wfds))
				{
				       LOG_DEBUG("select wait receive event fd:%d  index:%d POLLOUT", ppoll->_channel_events[i].fd, i);
					ppoll->_channel_events[i].revents |= POLLOUT;
				}
#if defined(WINCE)	
				if(FD_ISSET(ppoll->_channel_events[i].fd, &xfds))
					ppoll->_channel_events[i].revents |= POLLERR;
#endif
                       }
		}

	return tmp_count;

}

#endif
