// IOS ƽ̨
#include "asyn_frame/selector.h"
#include "asyn_frame/kqueue_wrap.h"
#include "utility/errcode.h"
#include "utility/mempool.h"
#include "utility/string.h"

#ifdef _KQUEUE

#define LOGID	LOGID_ASYN_FRAME
#include "utility/logger.h"


#include <errno.h>
#include <unistd.h>

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

_int32 create_selector(_int32 channel_count, void **selector)
{
	_int32 ret_val = SUCCESS;
	KQUEUE_SELECTOR *ppoll = NULL;
	_int32 kq;
	_int32 i = 0;

	ret_val = sd_malloc(sizeof(KQUEUE_SELECTOR), selector);
	if(ret_val != SUCCESS) return ret_val;

	ppoll = *selector;

       kq = kqueue();
       if (kq == -1){
               ret_val = errno;
		 sd_free(*selector);
		 *selector = NULL;
		 LOG_DEBUG("create kqueue failed %d", ret_val);
		 return ret_val;
       }
	ppoll->_kqueue_fd = kq;

	ret_val = sd_malloc(channel_count * sizeof(struct pollfd), (void**)&ppoll->_channel_events);
	if(ret_val != SUCCESS)
	{
	       close(ppoll->_kqueue_fd);
		sd_free(*selector);
		*selector = NULL;
		return ret_val;
	}

	ret_val = sd_malloc(channel_count * sizeof(CHANNEL_DATA), (void**)&ppoll->_channel_data);
	if(ret_val != SUCCESS)
	{
	       close(ppoll->_kqueue_fd);
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
	ppoll->_max_available_idx = -1;

	return ret_val;
}

_int32 destory_selector(void *selector)
{
	_int32 ret_val = SUCCESS;
	KQUEUE_SELECTOR *ppoll = selector;

	close(ppoll->_kqueue_fd);

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
	KQUEUE_SELECTOR *ppoll = selector;

	if(ppoll->_first_free_idx >= ppoll->_selector_size)
		return TRUE;
	
	return FALSE;
}

_int32 get_channel_error(void *selector, const _int32 channel_idx)
{
	return SUCCESS;
}

/* add a socket channel, do not check this socket whether if exist */
_int32 add_a_channel(void *selector, _u32 channel_fd, _u32 channel_event, CHANNEL_DATA channel_data)
{
	_int32 ret_val = SUCCESS, i = 0;
	KQUEUE_SELECTOR *ppoll = selector;
	struct kevent kev;
       struct timespec ts;
	LOG_DEBUG("add_a_channel  channel_fd:%d channel_event:%d channel_data.fd:%d channel_data.ptr:%x",channel_fd,  (_u32)channel_event, channel_data._fd, channel_data._ptr);

	if(ppoll->_first_free_idx >= ppoll->_selector_size)
		return NO_ROOM_OF_POLL;

	ppoll->_channel_events[ppoll->_first_free_idx].events = 0;
	if(channel_event & CHANNEL_READ)
		ppoll->_channel_events[ppoll->_first_free_idx].events |= POLLIN;
	if(channel_event & CHANNEL_WRITE)
		ppoll->_channel_events[ppoll->_first_free_idx].events |= POLLOUT;

	ppoll->_channel_events[ppoll->_first_free_idx].revents = 0;
	ppoll->_channel_events[ppoll->_first_free_idx].fd = channel_fd;

	sd_memcpy((void*)(ppoll->_channel_data + ppoll->_first_free_idx), &channel_data, sizeof(CHANNEL_DATA));

       ts.tv_sec=0; ts.tv_nsec=0;
	if(ppoll->_channel_events[ppoll->_first_free_idx].events & POLLIN)
	{
		EV_SET(&kev, ppoll->_channel_events[ppoll->_first_free_idx].fd, EVFILT_READ, EV_ADD|EV_ENABLE, 0, 0, 0);
		kevent(ppoll->_kqueue_fd, &kev, 1, 0, 0, &ts);
	}

	if(ppoll->_channel_events[ppoll->_first_free_idx].events & POLLOUT)
	{
		EV_SET(&kev, ppoll->_channel_events[ppoll->_first_free_idx].fd, EVFILT_WRITE, EV_ADD|EV_ENABLE, 0, 0, 0);
		kevent(ppoll->_kqueue_fd, &kev, 1, 0, 0, &ts);
	}

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
	KQUEUE_SELECTOR *ppoll = selector;
	struct kevent kev;
       struct timespec ts;

	LOG_DEBUG("modify_a_channel channel_idx:%d channel_fd:%d channel_event:%d channel_data.fd:%d channel_data.ptr:%x",channel_idx, channel_fd,  (_u32)channel_event, channel_data._fd, channel_data._ptr);
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

       ts.tv_sec=0; ts.tv_nsec=0;
	if(ppoll->_channel_events[channel_idx].events & POLLIN)
	{
	     EV_SET(&kev, ppoll->_channel_events[channel_idx].fd, EVFILT_READ , EV_ADD|EV_ENABLE, 0, 0, 0);
	     kevent(ppoll->_kqueue_fd, &kev, 1, 0, 0, &ts);
	}
	else
	{
	     EV_SET(&kev, ppoll->_channel_events[channel_idx].fd, EVFILT_READ , EV_DELETE, 0, 0, 0);
	     kevent(ppoll->_kqueue_fd, &kev, 1, 0, 0, &ts);
	}
	
	if(ppoll->_channel_events[channel_idx].events & POLLOUT)
	{
	     EV_SET(&kev, ppoll->_channel_events[channel_idx].fd, EVFILT_WRITE , EV_ADD|EV_ENABLE, 0, 0, 0);
	     kevent(ppoll->_kqueue_fd, &kev, 1, 0, 0, &ts);
	}
	else
	{
	     EV_SET(&kev, ppoll->_channel_events[channel_idx].fd, EVFILT_WRITE , EV_DELETE, 0, 0, 0);
	     kevent(ppoll->_kqueue_fd, &kev, 1, 0, 0, &ts);
	}
/*
	ppoll->_channel_events[channel_idx].fd = channel_fd;
*/

	sd_memcpy((void*)(ppoll->_channel_data + channel_idx), &channel_data, sizeof(CHANNEL_DATA));

	return ret_val;
}

_int32 del_a_channel(void *selector, _int32 channel_idx, _u32 channel_fd)
{
	_int32 ret_val = SUCCESS, i = 0;
	KQUEUE_SELECTOR *ppoll = selector;
	struct kevent kev;
       struct timespec ts;

	LOG_DEBUG("del_a_channel channel_idx:%d channel_fd:%d ",channel_idx, channel_fd);
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

       ts.tv_sec=0; ts.tv_nsec=0;
	EV_SET(&kev, ppoll->_channel_events[channel_idx].fd, EVFILT_READ , EV_DELETE, 0, 0, 0);
	kevent(ppoll->_kqueue_fd, &kev, 1, 0, 0, &ts);
	EV_SET(&kev, ppoll->_channel_events[channel_idx].fd, EVFILT_WRITE , EV_DELETE, 0, 0, 0);
	kevent(ppoll->_kqueue_fd, &kev, 1, 0, 0, &ts);


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
	KQUEUE_SELECTOR *ppoll = selector;

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
	return ((KQUEUE_SELECTOR*)selector)->_channel_events[channel_idx].revents & (POLLERR | POLLHUP | POLLNVAL);
}

BOOL is_channel_readable(void *selector, const _int32 channel_idx)
{
	return ((KQUEUE_SELECTOR*)selector)->_channel_events[channel_idx].revents & (POLLIN);
}

BOOL is_channel_writable(void *selector, const _int32 channel_idx)
{
	return ((KQUEUE_SELECTOR*)selector)->_channel_events[channel_idx].revents & (POLLOUT);
}

_int32 get_channel_data(void *selector, const _int32 channel_idx, CHANNEL_DATA *channel_data)
{
	_int32 ret_val = SUCCESS;
	KQUEUE_SELECTOR *ppoll = selector;

	sd_memcpy(channel_data, (void*)(ppoll->_channel_data + channel_idx), sizeof(CHANNEL_DATA));
	LOG_DEBUG("get_channel_data channel_idx:%d channel_fd:%d , channel_data._ptr:%x",channel_idx, channel_data->_fd, channel_data->_ptr);

	return ret_val;
}

_int32 update_selector_by_fd(void *selector, _int32 fd, _int16  event)
{
	   KQUEUE_SELECTOR *ppoll = selector;
	   _int32 i = 0;

	   
	    for ( i = 0; i <= ppoll->_max_available_idx; ++i)
	    {
			if(ppoll->_channel_events[i].fd > 0  && ppoll->_channel_events[i].fd == fd)
			{
				ppoll->_channel_events[i].revents |= event;
	             }
	    }
           return SUCCESS;
}

#define MAX_EVENTS (256)

_int32 selector_wait(void *selector, _int32 timeout)
{
	   _int32 f, kq, nev;
	   struct kevent changelist[MAX_EVENTS];
	   struct kevent eventlist[MAX_EVENTS];
	   
	   KQUEUE_SELECTOR *ppoll = selector;
	   struct timeval to, *p_to;
	   struct timespec  time_spec;
	   _int32 tmp_count = 0;
	   _int32 maxfd = 0;
	   _int32 i = 0 ;


           sd_memset(changelist, 0, sizeof(changelist) );
           sd_memset(eventlist, 0, sizeof(eventlist) );

	    kq = ppoll->_kqueue_fd;

		
	    if(timeout < 0)
	    {
		p_to = NULL;
		time_spec.tv_sec = 0; 
		time_spec.tv_nsec = 0; 
	    }
	    else
	    {
			to.tv_sec = timeout/1000;
			to.tv_usec = (timeout % 1000) * 1000;

			time_spec.tv_sec = timeout/1000;
			time_spec.tv_nsec = (timeout % 1000) * 1000*1000;

			p_to= &to;
	    }

           //kevent(kq, NULL, 0, NULL, 0, NULL);
		   
	    for ( i = 0; i <= ppoll->_max_available_idx; ++i)
	    {
			if(ppoll->_channel_events[i].fd > 0)
			{
				if(ppoll->_channel_events[i].events & POLLIN)
				{
					EV_SET(&changelist[tmp_count++], ppoll->_channel_events[i].fd, EVFILT_READ, EV_ADD|EV_CLEAR, 0, 0, 0);
				}
				else if(ppoll->_channel_events[i].events & POLLOUT)
				{
					EV_SET(&changelist[tmp_count++], ppoll->_channel_events[i].fd, EVFILT_WRITE, EV_ADD|EV_CLEAR, 0, 0, 0);
				}
				 
				ppoll->_channel_events[i].revents = 0;

				if(ppoll->_channel_events[i].fd > maxfd)
					maxfd = ppoll->_channel_events[i].fd;

				if(!(ppoll->_channel_events[i].events & POLLIN || ppoll->_channel_events[i].events & POLLOUT))
				{
					printf("an empty event, fd:%d\n", ppoll->_channel_events[i].fd);
				}
	             }
		}



               if(timeout<0)
               {
	                 nev = kevent(kq, 0, 0, eventlist, tmp_count, NULL);
	        }
               else
               {
                        nev = kevent(kq, 0, 0, eventlist, tmp_count, &time_spec);
               }	  
	  	if(nev > 0)
		{
		     for ( i = 0; i <= nev; ++i)
		     {

	               if (eventlist[i].ident > 0)
	               {
		
				if (eventlist[i].filter == EVFILT_READ) 
			       {
			               if (eventlist[i].flags & EV_EOF)
			               {
						update_selector_by_fd(selector, eventlist[i].ident , POLLERR);
			               }
				        else if (eventlist[i].flags & EV_ERROR)
			               {
						update_selector_by_fd(selector, eventlist[i].ident , POLLERR);
			               }
					 else
					 {
						//ppoll->_channel_events[i].revents |= POLLIN;
						update_selector_by_fd(selector, eventlist[i].ident , POLLIN);
					 }
				 }
				 else if (eventlist[i].filter ==EVFILT_WRITE)
				 {
					//ppoll->_channel_events[i].revents |= POLLOUT;
					update_selector_by_fd(selector, eventlist[i].ident , POLLOUT);
				 }
	               }
		     }
		}
		return nev;

}

#endif
