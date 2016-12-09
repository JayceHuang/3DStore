// windows mobile 平台

#include "asyn_frame/wsa_event_select_wrap.h"
#include "utility/errcode.h"
#include "utility/mempool.h"
#include "utility/string.h"

#ifdef _WSA_EVENT_SELECT

#define LOGID	LOGID_ASYN_FRAME
#include "utility/logger.h"

_int32 create_selector(_int32 channel_count, void **selector)
{
	_int32 ret_val = SUCCESS;
	WES_SELECTOR *ppoll = NULL;
	_int32 i = 0;

	if(WSA_MAXIMUM_WAIT_EVENTS<channel_count)
		return NO_ROOM_OF_POLL;
	
	ret_val = sd_malloc(sizeof(WES_SELECTOR), selector);
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

	ret_val = sd_malloc(channel_count * sizeof(WSAEVENT), (void**)&ppoll->_wsa_events);
	if(ret_val != SUCCESS)
	{
		sd_free(ppoll->_channel_data);
		ppoll->_channel_data = NULL;
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
		ppoll->_wsa_events[i] = NULL;

		ppoll->_channel_data[i]._ptr = NULL;
	}

	ppoll->_selector_size = channel_count;
	ppoll->_first_free_idx = 0;
	ppoll->_max_available_idx = -1;
	ppoll->_min_event_idx = 0;

	return ret_val;
}

_int32 destory_selector(void *selector)
{
	_int32 ret_val = SUCCESS;
	WES_SELECTOR *ppoll = selector;

	ret_val = sd_free(ppoll->_wsa_events);
	ppoll->_wsa_events = NULL;
	CHECK_VALUE(ret_val);

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
	WES_SELECTOR *ppoll = selector;

	if(ppoll->_first_free_idx >= ppoll->_selector_size)
		return TRUE;
	
	return FALSE;
}

/* add a socket channel, do not check this socket whether if exist */
_int32 add_a_channel(void *selector, _u32 channel_fd, _u32 channel_event, CHANNEL_DATA channel_data)
{
	_int32 ret_val = SUCCESS, i = 0;
	WES_SELECTOR *ppoll = selector;

	if(ppoll->_first_free_idx >= ppoll->_selector_size)
		return NO_ROOM_OF_POLL;

	for(i = 0; i <= ppoll->_max_available_idx; i++)
	{
		if(channel_fd == ppoll->_channel_events[i].fd)
		{
			/* FATAL ERROR!!! */
			CHECK_VALUE(BAD_POLL_ARUMENT);
		}
	}
	
	ppoll->_wsa_events[ppoll->_first_free_idx] = WSACreateEvent();
	
	ppoll->_channel_events[ppoll->_first_free_idx].events = 0;
	if(channel_event & CHANNEL_READ)
		ppoll->_channel_events[ppoll->_first_free_idx].events |= FD_READ|FD_ACCEPT|FD_CLOSE;
	if(channel_event & CHANNEL_WRITE)
		ppoll->_channel_events[ppoll->_first_free_idx].events |= FD_WRITE|FD_CONNECT|FD_CLOSE;

	ppoll->_channel_events[ppoll->_first_free_idx].revents = 0;
	ppoll->_channel_events[ppoll->_first_free_idx].fd = channel_fd;
	
	WSAEventSelect(channel_fd, ppoll->_wsa_events[ppoll->_first_free_idx], ppoll->_channel_events[ppoll->_first_free_idx].events);
	
	sd_memcpy((void*)(ppoll->_channel_data + ppoll->_first_free_idx), &channel_data, sizeof(CHANNEL_DATA));

	if(ppoll->_first_free_idx > ppoll->_max_available_idx)
		ppoll->_max_available_idx = ppoll->_first_free_idx;

	/* find next free idx */
	ppoll->_first_free_idx++;

	return ret_val;
}
/* add a socket channel, do not check this socket whether if exist */
_int32 add_a_notice_channel(void *selector, _int32 notice_handle, CHANNEL_DATA channel_data)
{
	_int32 ret_val = SUCCESS, i = 0;
	WES_SELECTOR *ppoll = selector;

	if(ppoll->_first_free_idx >= ppoll->_selector_size)
		return NO_ROOM_OF_POLL;

	for(i = 0; i <= ppoll->_max_available_idx; i++)
	{
		if(notice_handle == ppoll->_channel_events[i].fd)
		{
			/* FATAL ERROR!!! */
			CHECK_VALUE(BAD_POLL_ARUMENT);
		}
	}
	
	ppoll->_wsa_events[ppoll->_first_free_idx] = notice_handle;
	
	ppoll->_channel_events[ppoll->_first_free_idx].events = FD_ALL_EVENTS;

	ppoll->_channel_events[ppoll->_first_free_idx].revents = 0;
	ppoll->_channel_events[ppoll->_first_free_idx].fd = notice_handle;
	
	//WSAEventSelect(channel_fd, ppoll->_wsa_events[ppoll->_first_free_idx], ppoll->_channel_events[ppoll->_first_free_idx].events);
	
	sd_memcpy((void*)(ppoll->_channel_data + ppoll->_first_free_idx), &channel_data, sizeof(CHANNEL_DATA));

	if(ppoll->_first_free_idx > ppoll->_max_available_idx)
		ppoll->_max_available_idx = ppoll->_first_free_idx;

	/* find next free idx */
	ppoll->_first_free_idx++;

	return ret_val;
}


/* modify a socket channel */
_int32 modify_a_channel(void *selector, _int32 channel_idx, _u32 channel_fd, _u32 channel_event, CHANNEL_DATA channel_data)
{
	_int32 ret_val = SUCCESS, i = 0;
	WES_SELECTOR *ppoll = selector;

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
		ppoll->_channel_events[channel_idx].events |= FD_READ|FD_ACCEPT|FD_CLOSE;
	if(channel_event & CHANNEL_WRITE)
		ppoll->_channel_events[channel_idx].events |= FD_WRITE|FD_CONNECT|FD_CLOSE;

	ppoll->_channel_events[channel_idx].revents = 0;


	WSAEventSelect(channel_fd, ppoll->_wsa_events[channel_idx], ppoll->_channel_events[channel_idx].events);

	sd_memcpy((void*)(ppoll->_channel_data + channel_idx), &channel_data, sizeof(CHANNEL_DATA));

	return ret_val;
}

_int32 del_a_channel(void *selector, _int32 channel_idx, _u32 channel_fd)
{
	_int32 ret_val = SUCCESS, i = 0;
	WES_SELECTOR *ppoll = selector;

	if(channel_idx == -1)
	{/* find idx by channel_fd */
		for(i = 0; i <= ppoll->_max_available_idx; i++)
		{
			if(channel_fd ==  ppoll->_channel_events[i].fd)
			{
				channel_idx = i;
				break;
			}
		}
	}
	if(channel_idx == -1)
		return BAD_POLL_ARUMENT;

       WSACloseEvent(ppoll->_wsa_events[channel_idx]);  
	if(channel_idx!=ppoll->_max_available_idx)
	{
		ppoll->_wsa_events[channel_idx] = ppoll->_wsa_events[ppoll->_max_available_idx];
		sd_memcpy((void*)(ppoll->_channel_events + channel_idx), (void*)(ppoll->_channel_events + ppoll->_max_available_idx), sizeof(struct pollfd));
		sd_memcpy((void*)(ppoll->_channel_data + channel_idx), (void*)(ppoll->_channel_data + ppoll->_max_available_idx), sizeof(CHANNEL_DATA));
		
		ppoll->_channel_events[ppoll->_max_available_idx].fd = -1;
		ppoll->_channel_events[ppoll->_max_available_idx].events = 0;
		ppoll->_channel_events[ppoll->_max_available_idx].revents = 0;
		ppoll->_wsa_events[ppoll->_max_available_idx] = NULL;

		ppoll->_channel_data[ppoll->_max_available_idx]._ptr = NULL;
	}
	else
	{
		ppoll->_channel_events[channel_idx].fd = -1;
		ppoll->_channel_events[channel_idx].events = 0;
		ppoll->_channel_events[channel_idx].revents = 0;
		ppoll->_wsa_events[channel_idx] = NULL;

		ppoll->_channel_data[channel_idx]._ptr = NULL;
	}
	ppoll->_max_available_idx--;
	ppoll->_first_free_idx--;
	sd_assert(ppoll->_first_free_idx==ppoll->_max_available_idx+1);
	
	return ret_val;
}
_int32 del_a_notice_channel(void *selector, _int32 channel_idx, _int32 notice_handle)
{
	_int32 ret_val = SUCCESS, i = 0;
	WES_SELECTOR *ppoll = selector;

	if(channel_idx == -1)
	{/* find idx by channel_fd */
		for(i = 0; i <= ppoll->_max_available_idx; i++)
		{
			if(notice_handle ==  ppoll->_channel_events[i].fd) 
			{
				channel_idx = i;
				break;
			}
		}
	}
	if(channel_idx == -1)
		return BAD_POLL_ARUMENT;

       WSACloseEvent(ppoll->_wsa_events[channel_idx]);  
	if(channel_idx!=ppoll->_max_available_idx)
	{
		ppoll->_wsa_events[channel_idx] = ppoll->_wsa_events[ppoll->_max_available_idx];
		sd_memcpy((void*)(ppoll->_channel_events + channel_idx), (void*)(ppoll->_channel_events + ppoll->_max_available_idx), sizeof(struct pollfd));
		sd_memcpy((void*)(ppoll->_channel_data + channel_idx), (void*)(ppoll->_channel_data + ppoll->_max_available_idx), sizeof(CHANNEL_DATA));
		
		ppoll->_channel_events[ppoll->_max_available_idx].fd = -1;
		ppoll->_channel_events[ppoll->_max_available_idx].events = 0;
		ppoll->_channel_events[ppoll->_max_available_idx].revents = 0;
		ppoll->_wsa_events[ppoll->_max_available_idx] = NULL;

		ppoll->_channel_data[ppoll->_max_available_idx]._ptr = NULL;
	}
	else
	{
		ppoll->_channel_events[channel_idx].fd = -1;
		ppoll->_channel_events[channel_idx].events = 0;
		ppoll->_channel_events[channel_idx].revents = 0;
		ppoll->_wsa_events[channel_idx] = NULL;

		ppoll->_channel_data[channel_idx]._ptr = NULL;
	}
	ppoll->_max_available_idx--;
	ppoll->_first_free_idx--;
	sd_assert(ppoll->_first_free_idx==ppoll->_max_available_idx+1);
	
	return ret_val;
}

_int32 get_next_channel(void *selector, _int32 *channel_idx)
{
	_int32 i = 0,ret=0;
	WES_SELECTOR *ppoll = selector;
	WSANETWORKEVENTS event;   

	if(*channel_idx ==CHANNEL_IDX_FIND_BEGIN) *channel_idx= ppoll->_min_event_idx-1;

	for(i = (*channel_idx) + 1; i <= ppoll->_max_available_idx; i++)
	{
		if(ppoll->_min_event_idx!=i)
			ret = WSAWaitForMultipleEvents(1, &(ppoll->_wsa_events[i]), TRUE, 1000, FALSE);   
		else
			ret = ppoll->_min_event_idx;

		if(ret == WSA_WAIT_FAILED || ret == WSA_WAIT_TIMEOUT)   
		{   
			continue;   
		}   
		else 
		{   
			ppoll->_channel_events[i].err_code = 0;
			if(ppoll->_channel_events[i].events == FD_ALL_EVENTS)
			{
				ppoll->_channel_events[i].revents = FD_ALL_EVENTS;  
				*channel_idx = i;
				return SUCCESS;
			}

			WSAEnumNetworkEvents(ppoll->_channel_events[i].fd, ppoll->_wsa_events[i], &event);  
			if(event.lNetworkEvents & FD_ACCEPT)                // 处理FD_ACCEPT通知消息   
			{   
				if(event.iErrorCode[FD_ACCEPT_BIT] == 0)   
				{   
					ppoll->_channel_events[i].revents|=FD_ACCEPT;   
				} 
				else
				{
					ppoll->_channel_events[i].err_code = event.iErrorCode[FD_ACCEPT_BIT];
				}
			}   
			else if(event.lNetworkEvents & FD_CONNECT)         // 处理FD_CONNECT通知消息   
			{   
				if(event.iErrorCode[FD_CONNECT_BIT] == 0)   
				{   
					ppoll->_channel_events[i].revents|=FD_CONNECT;   
				}   
				else
				{
					ppoll->_channel_events[i].err_code = event.iErrorCode[FD_CONNECT_BIT];
				}
			}   
			else if(event.lNetworkEvents & FD_READ)         // 处理FD_READ通知消息   
			{   
				if(event.iErrorCode[FD_READ_BIT] == 0)   
				{   
					ppoll->_channel_events[i].revents|=FD_READ;   
				}   
				else
				{
					ppoll->_channel_events[i].err_code = event.iErrorCode[FD_ACCEPT_BIT];
				}
			}   
			else if(event.lNetworkEvents & FD_WRITE)        // 处理FD_WRITE通知消息   
			{   
				if(event.iErrorCode[FD_WRITE_BIT] == 0)   
				{   
					ppoll->_channel_events[i].revents|=FD_WRITE;   
				}   
				else
				{
					ppoll->_channel_events[i].err_code = event.iErrorCode[FD_WRITE_BIT];
				}
			}   
			else if(event.lNetworkEvents & FD_CLOSE)        // 处理FD_CLOSE通知消息   
			{   
					//ppoll->_channel_events[channel_idx].revents|=FD_READ;   
					ppoll->_channel_events[i].err_code = SOCKET_CLOSED;
			}   
		
			*channel_idx = i;
			return SUCCESS;
		}
	}
	
	ppoll->_min_event_idx = 0;
	
	return NOT_FOUND_POLL_EVENT;
}

BOOL is_channel_error(void *selector, const _int32 channel_idx)
{
	return ((WES_SELECTOR*)selector)->_channel_events[channel_idx].err_code!=0?TRUE:FALSE;
}

_int32 get_channel_error(void *selector, const _int32 channel_idx)
{
	return ((WES_SELECTOR*)selector)->_channel_events[channel_idx].err_code;
}


BOOL is_channel_readable(void *selector, const _int32 channel_idx)
{
	return ((WES_SELECTOR*)selector)->_channel_events[channel_idx].revents & (FD_READ|FD_ACCEPT);
}

BOOL is_channel_writable(void *selector, const _int32 channel_idx)
{
	return ((WES_SELECTOR*)selector)->_channel_events[channel_idx].revents & (FD_WRITE|FD_CONNECT);
}

_int32 get_channel_data(void *selector, const _int32 channel_idx, CHANNEL_DATA *channel_data)
{
	_int32 ret_val = SUCCESS;
	WES_SELECTOR *ppoll = selector;

	sd_memcpy(channel_data, (void*)(ppoll->_channel_data + channel_idx), sizeof(CHANNEL_DATA));

	return ret_val;
}

_int32 selector_wait(void *selector, _int32 timeout)
{
	WES_SELECTOR *ppoll = selector;
	_int32 tmp_count = 0,ret=0;   

	if(timeout==-1) timeout= WSA_INFINITE;

	ppoll->_min_event_idx = 0;
  
	ret = WSAWaitForMultipleEvents(ppoll->_max_available_idx + 1, ppoll->_wsa_events, FALSE, timeout, FALSE);   
	if (ret == WSA_WAIT_FAILED || ret == WSA_WAIT_TIMEOUT)   
	{   
		tmp_count = 0;  
		return tmp_count;
	}   
	ppoll->_min_event_idx = ret - WSA_WAIT_EVENT_0;  
	tmp_count = ppoll->_max_available_idx + 1 -ppoll->_min_event_idx;
	return 1; //tmp_count;
}

#endif /* _WSA_EVENT_SELECT */
