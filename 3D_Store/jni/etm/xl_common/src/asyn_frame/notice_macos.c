#ifdef LINUX
#ifdef MACOS

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#ifdef _EPOOL
#include <sys/epoll.h>
#endif

#include "asyn_frame/notice.h"
#include "asyn_frame/selector.h"
#include "utility/errcode.h"
#include "utility/string.h"
#include "platform/sd_fs.h"
#include "platform/sd_socket.h"
#include "platform/sd_task.h"
#include "utility/mempool.h"

#include "utility/logid.h"
#define LOGID LOGID_COMMON
#include "utility/logger.h"

#define MAX_WAIT_EVENT	(64)

static _u32 sem_value = 0;
static TASK_LOCK * gp_event_lock = NULL;

_int32 init_event_module(void)
{
	_int32 ret_val = sd_malloc(sizeof(TASK_LOCK), (void**)&gp_event_lock);
	CHECK_VALUE(ret_val);
	sd_memset(gp_event_lock, 0x00, sizeof(TASK_LOCK));
	ret_val = sd_init_task_lock(gp_event_lock);
	if (ret_val != SUCCESS)
	{
		SAFE_DELETE(gp_event_lock);
	}
	return ret_val;
}
_int32 uninit_event_module(void)
{
	sd_uninit_task_lock(gp_event_lock);
	SAFE_DELETE(gp_event_lock);
	return SUCCESS;
}
static _int32 get_named(_u32 sem_index, sem_t **sem, _int32 val) 
{
	/* 用getpid()参与生成信号量的名字的目的是避免同时运行多个下载库时发成信号量名字冲突! */
	char sem_name[32] = {0};
	sd_snprintf(sem_name, sizeof(sem_name), "sd_%X_%u", getpid(), sem_index);
	
	while (((*sem = sem_open(sem_name, O_CREAT, S_IRUSR|S_IWUSR, val)) == SEM_FAILED) &&
           (errno == EINTR)) ;
	
	if (*sem != SEM_FAILED)
	{
		//printf("\n get_named 1, index:%d sem_t %d", sem_index, (int)(*sem));
		return SUCCESS;
	}
	
	_int32 err = errno;
	if (errno != EEXIST)
	{
		sd_assert(FALSE);
		return -1;
	}
	
	while (((*sem = sem_open(sem_name, 0)) == SEM_FAILED) && (errno == EINTR)) ;
	if (*sem != SEM_FAILED)
	{
		//printf("\n get_named 2, index:%d sem_t %d", sem_index, (int)(*sem));
		return SUCCESS;
	}
	sd_assert(FALSE);
	return -1;
}

_int32 notice_init(void)
{
    return SUCCESS;
}

_int32 create_notice_handle(_int32 *notice_handle, _int32 *waitable_handle)
{
	if ((NULL == notice_handle) || (NULL == waitable_handle))
    {
        return INVALID_ARGUMENT;
    }

	/* set nonblock */
	_int32 fds[2] = {0};
    _int32 ret_val = pipe(fds);
	if (ret_val < 0)
	{
		ret_val = errno;
		return ret_val;
	}

    _int32 flags = fcntl(fds[0], F_GETFL);
	ret_val = fcntl(fds[0], F_SETFL, flags | O_NONBLOCK);
	if (ret_val < 0)
	{
		ret_val = errno;
		return ret_val;
	}
	
    flags = fcntl(fds[1], F_GETFL);
	ret_val = fcntl(fds[1], F_SETFL, flags | O_NONBLOCK);

	if (ret_val < 0)
	{
		ret_val = errno;
		return ret_val;
	}

	*notice_handle = fds[1];
	*waitable_handle = fds[0];
	
	ret_val = SUCCESS;
	return ret_val;
}

_int32 create_waitable_container(_int32 *waitable_container)
{
    return create_selector(MAX_WAIT_EVENT, (void**)waitable_container);
}

_int32 add_notice_handle(_int32 waitable_container, _int32 waitable_handle)
{
	_u32 event = CHANNEL_READ;
	CHANNEL_DATA data;
	data._fd = waitable_handle;
	return add_a_channel((void*)waitable_container, waitable_handle, event, data);	
}

_int32 del_notice_handle(_int32 waitable_container, _int32 waitable_handle)
{
	return del_a_channel((void*)waitable_container, -1, waitable_handle);
}

_int32 notice_impl(_int32 notice_handle)
{
	_int32 ret_val = write(notice_handle, "0", 1);
	if (ret_val != 1)
	{
	    ret_val = errno;
	}
	else
	{
	    ret_val = SUCCESS;
	}

	return ret_val;
}

_int32 wait_for_notice(_int32 waitable_container, _int32 expect_handle_count, _int32 *noticed_handle, _u32 timeout)
{
	_int32 tmp_count = selector_wait((void*)waitable_container, (_int32)timeout);
	if (tmp_count > expect_handle_count)
	{
	    tmp_count = expect_handle_count;
	}

	_int32 channel_idx = CHANNEL_IDX_FIND_BEGIN;
	_int32 ret_val = SUCCESS;
	CHANNEL_DATA data;
	
	/* handle the event */
	_int32 index = 0;
	for (index = 0; index < tmp_count; index++)
	{
		ret_val = get_next_channel((void*)waitable_container, &channel_idx);
		if (ret_val == NOT_FOUND_POLL_EVENT)
		{
			LOG_DEBUG("the waitable_container dismatched with tmp_count. channel_idx: %d, tmp_count: %d, index_socket: %d", channel_idx, tmp_count, index);
			ret_val = SUCCESS;
			break;//NODE:jieouy总感觉这里应该是continue，以前人的代码写break
		}

		if (ret_val != SUCCESS)
		{
		    return ret_val;
		}

		ret_val = get_channel_data((void*)waitable_container, channel_idx, &data);
		if (ret_val != SUCCESS)
		{
		    return ret_val;
		}

		noticed_handle[index] = data._fd;
	}

	return ret_val;
}

_int32 reset_notice(_int32 waitable_handle)
{
	char buffer[4] = {0};
	read(waitable_handle, buffer, 1);

	return SUCCESS;
}

_int32 destory_notice_handle(_int32 notice_handle, _int32 waitable_handle)
{
	close(notice_handle);

	if (waitable_handle != notice_handle)
	{
	    close(waitable_handle);
	}

	return SUCCESS;
}

_int32 destory_waitable_container(_int32 waitable_container)
{
	return destory_selector((void*)waitable_container);
}

_int32 init_simple_event(SEVENT_HANDLE *handle)
{
    sd_task_lock(gp_event_lock);
    handle->_index = sem_value;
    handle->_value = 0;
	LOG_DEBUG("init_simple_event semaphore%u", sem_value);
	
    sem_t* sem = NULL;
	_int32 ret_val = get_named(sem_value, &sem, 0);
	if (ret_val == SUCCESS)
	{
		handle->sem = sem;
		sem_value++;
	}
	else
	{
	    ret_val = errno;
	    sd_assert(sem == SEM_FAILED);
		sd_assert(FALSE);
	}
	sd_task_unlock(gp_event_lock);
	LOG_DEBUG("init_simple_event ret handle = %d, errno=%d", sem, ret_val);
	return ret_val;
}

_int32 wait_sevent_handle(SEVENT_HANDLE *handle)
{
    sem_t* sem = handle->sem;

	_int32 ret_val = SUCCESS;
	do
	{
		ret_val = sem_wait(sem);
	} while ((ret_val < 0) && (errno == EINTR));

	if (ret_val < 0)
	{
		ret_val = errno;
		sd_assert(FALSE);
	}
	else
	{
		handle->_value--;
	}
	return ret_val;
}

_int32 signal_sevent_handle(SEVENT_HANDLE *handle)
{
	_int32 ret_val = SUCCESS;
    sem_t* sem = handle->sem;
    
	if (handle->_value <= 0 )
	{
		handle->_value++;
		ret_val = sem_post(sem);
		
		if (ret_val < 0)
		{
			sd_assert(FALSE);
			ret_val = errno;
			handle->_value--;
		}
	}

	return ret_val;
}

_int32 uninit_simple_event(SEVENT_HANDLE *handle)
{
	
    sem_t* sem = handle->sem;	
	LOG_DEBUG("uninit_simple_event  handle = %d ", handle->sem);

	_int32 ret_val = sem_close(sem);
	if (ret_val < 0)
	{
		ret_val = errno;
		sd_assert(FALSE);
	}
    char sem_name[32] = {0};
	sd_snprintf(sem_name, sizeof(sem_name), "sd_%X_%u", getpid(),handle->_index);
	sem_unlink(sem_name);
	LOG_DEBUG("uninit_simple_event  success handle = %d ", handle->sem);
	return ret_val;
}

#endif
#endif

