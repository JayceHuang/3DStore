#include "em_asyn_frame/em_msg_alloctor.h"
#include "em_common/em_errcode.h"
#include "em_common/em_mempool.h"
#include "em_interface/em_task.h"
#include "em_asyn_frame/em_msg.h"

static SLAB *gp_em_msg_slab = NULL;

/* the following for thread-msg */
EM_DECL_FIXED_MPOOL(THREAD_MSG, 16);
static TASK_LOCK g_em_thread_msg_lock;
#ifndef _MSG_POLL
static TASK_COND g_em_thread_msg_cond;
#endif

_int32 em_msg_alloctor_init(void)
{
	_int32 ret_val = SUCCESS;

	ret_val = em_mpool_create_slab(sizeof(MSG), EM_MIN_MSG_COUNT, 0, &gp_em_msg_slab);
	CHECK_VALUE(ret_val);


	/* init thread-lock */
	ret_val = em_init_task_lock(&g_em_thread_msg_lock);
	CHECK_VALUE(ret_val);

#ifndef _MSG_POLL
	ret_val = em_init_task_cond(&g_em_thread_msg_cond);
	CHECK_VALUE(ret_val);
#endif

	ret_val = EM_INIT_FIXED_MPOOL();
	CHECK_VALUE(ret_val);

	ret_val = em_init_post_msg();
	CHECK_VALUE(ret_val);

	return ret_val;
}

_int32 em_msg_alloctor_uninit(void)
{
	_int32 ret_val = SUCCESS;

	em_uninit_post_msg();

	/* uninit thread-lock */
	ret_val = em_uninit_task_lock(&g_em_thread_msg_lock);
	CHECK_VALUE(ret_val);
	
#ifndef _MSG_POLL
	ret_val = em_uninit_task_cond(&g_em_thread_msg_cond);
	CHECK_VALUE(ret_val);
#endif

	/* all slab */
	ret_val = em_mpool_destory_slab(gp_em_msg_slab);
	CHECK_VALUE(ret_val);
	gp_em_msg_slab = NULL;

	return ret_val;
}

_int32 em_msg_alloc(MSG **msg)
{
	_int32 ret_val = SUCCESS;

	ret_val = em_mpool_get_slip(gp_em_msg_slab, (void**)msg);
	CHECK_VALUE(ret_val);

	return ret_val;
}

_int32 em_msg_dealloc(MSG *msg)
{
	_int32 ret_val = SUCCESS;

	ret_val = em_mpool_free_slip(gp_em_msg_slab, msg);
	CHECK_VALUE(ret_val);

	return ret_val;
}

_int32 em_msg_thread_alloc(THREAD_MSG **ppmsg)
{
	_int32 ret_val = SUCCESS;

	/* lock */
	em_task_lock(&g_em_thread_msg_lock);

	while((ret_val = EM_GET_FIEXED_MPOOL_DATA(ppmsg)) == OUT_OF_FIXED_MEMORY)
	{
#ifdef _MSG_POLL
		em_task_unlock(&g_em_thread_msg_lock);
		em_sleep(WAIT_INTERVAL);
		em_task_lock(&g_em_thread_msg_lock);
#else
		ret_val = em_task_cond_wait(&g_em_thread_msg_cond, &g_em_thread_msg_lock);
		CHECK_VALUE(ret_val);
#endif
	}

	em_task_unlock(&g_em_thread_msg_lock);

	return ret_val;
}

_int32 em_msg_thread_dealloc(THREAD_MSG *pmsg)
{
	_int32 ret_val = SUCCESS;

	/* lock main thread of asyn_frame. we assume this case seldom happend */
	em_task_lock(&g_em_thread_msg_lock);

	pmsg->_handle = NULL;
	pmsg->_param = NULL;
	
	ret_val = EM_FREE_FIXED_MPOOL_DATA(pmsg);
	if(ret_val != SUCCESS)
	{
		em_task_unlock(&g_em_thread_msg_lock);
		return ret_val;
	}

#ifndef _MSG_POLL
	ret_val = em_task_cond_signal(&g_em_thread_msg_cond);
	if(ret_val != SUCCESS)
	{
		em_task_unlock(&g_em_thread_msg_lock);
		return ret_val;
	}
#endif

	em_task_unlock(&g_em_thread_msg_lock);

	return ret_val;
}
