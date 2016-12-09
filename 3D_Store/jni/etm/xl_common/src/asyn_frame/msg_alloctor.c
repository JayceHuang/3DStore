#include "asyn_frame/msg_alloctor.h"
#include "utility/errcode.h"
#include "utility/mempool.h"
#include "platform/sd_task.h"

static SLAB *gp_msg_slab = NULL;
static SLAB *gp_dns_slab = NULL;
static SLAB *gp_accept_slab = NULL;
static SLAB *gp_csrw_slab = NULL;
static SLAB *gp_ncsrw_slab = NULL;
static SLAB *gp_fso_slab = NULL;
static SLAB *gp_fsrw_slab = NULL;
static SLAB *gp_sockaddr_slab = NULL;
static SLAB *gp_calc_hash_slab = NULL;

/* the following for thread-msg */
DECL_FIXED_MPOOL(THREAD_MSG, 16);
static TASK_LOCK g_thread_msg_lock;
static TASK_COND g_thread_msg_cond;

_int32 msg_alloctor_init(void)
{
	_int32 ret_val = mpool_create_slab(sizeof(MSG), MIN_MSG_COUNT, 0, &gp_msg_slab);
	CHECK_VALUE(ret_val);

	/* OP Parameter */
	ret_val = mpool_create_slab(sizeof(OP_PARA_DNS), MIN_DNS_COUNT, 0, &gp_dns_slab);
	CHECK_VALUE(ret_val);

	ret_val = mpool_create_slab(sizeof(OP_PARA_ACCEPT), MIN_ACCEPT_COUNT, 0, &gp_accept_slab);
	CHECK_VALUE(ret_val);

	ret_val = mpool_create_slab(sizeof(OP_PARA_CONN_SOCKET_RW), MIN_CONN_SOCKET_RW_COUNT, 0, &gp_csrw_slab);
	CHECK_VALUE(ret_val);

	ret_val = mpool_create_slab(sizeof(OP_PARA_NOCONN_SOCKET_RW), MIN_NOCONN_SOCKET_RW_COUNT, 0, &gp_ncsrw_slab);
	CHECK_VALUE(ret_val);

	ret_val = mpool_create_slab(sizeof(OP_PARA_FS_OPEN), MIN_FS_OPEN_COUNT, 0, &gp_fso_slab);
	CHECK_VALUE(ret_val);

	ret_val = mpool_create_slab(sizeof(OP_PARA_FS_RW), MIN_FS_RW_COUNT, 0, &gp_fsrw_slab);
	CHECK_VALUE(ret_val);

	ret_val = mpool_create_slab(sizeof(SD_SOCKADDR), MIN_SOCKADDR_COUNT, 0, &gp_sockaddr_slab);
	CHECK_VALUE(ret_val);

    ret_val = mpool_create_slab(sizeof(OP_PARA_CACL_HASH), 32, 0, &gp_calc_hash_slab);
    CHECK_VALUE(ret_val);

	/* init thread-lock */
	ret_val = sd_init_task_lock(&g_thread_msg_lock);
	CHECK_VALUE(ret_val);

	ret_val = sd_init_task_cond(&g_thread_msg_cond);
	CHECK_VALUE(ret_val);

	ret_val = INIT_FIXED_MPOOL();
	CHECK_VALUE(ret_val);

	return ret_val;
}

_int32 msg_alloctor_uninit(void)
{
	_int32 ret_val = sd_uninit_task_lock(&g_thread_msg_lock);
	CHECK_VALUE(ret_val);
	
	ret_val = sd_uninit_task_cond(&g_thread_msg_cond);
	CHECK_VALUE(ret_val);

	/* all slab */
	ret_val = mpool_destory_slab(gp_msg_slab);
	CHECK_VALUE(ret_val);
	gp_msg_slab = NULL;

	ret_val = mpool_destory_slab(gp_dns_slab);
	CHECK_VALUE(ret_val);
	gp_dns_slab = NULL;

	ret_val = mpool_destory_slab(gp_accept_slab);
	CHECK_VALUE(ret_val);
	gp_accept_slab = NULL;

	ret_val = mpool_destory_slab(gp_csrw_slab);
	CHECK_VALUE(ret_val);
	gp_csrw_slab = NULL;

	ret_val = mpool_destory_slab(gp_ncsrw_slab);
	CHECK_VALUE(ret_val);
	gp_ncsrw_slab = NULL;

	ret_val = mpool_destory_slab(gp_fso_slab);
	CHECK_VALUE(ret_val);
	gp_fso_slab = NULL;

	ret_val = mpool_destory_slab(gp_fsrw_slab);
	CHECK_VALUE(ret_val);
	gp_fsrw_slab = NULL;

	ret_val = mpool_destory_slab(gp_sockaddr_slab);
	CHECK_VALUE(ret_val);
	gp_sockaddr_slab = NULL;

    ret_val = mpool_destory_slab(gp_calc_hash_slab);
    CHECK_VALUE(ret_val);
    gp_calc_hash_slab = NULL;

	return ret_val;
}

_int32 msg_alloc(MSG **msg)
{
    _int32 ret_val = mpool_get_slip(gp_msg_slab, (void**)msg);
    CHECK_VALUE(ret_val);
    return ret_val;
}

_int32 msg_dealloc(MSG *msg)
{
    _int32 ret_val = mpool_free_slip(gp_msg_slab, msg);
    CHECK_VALUE(ret_val);
    return ret_val;
}

_int32 para_dns_alloc(void **para_dns)
{
    _int32 ret_val = mpool_get_slip(gp_dns_slab, para_dns);
    CHECK_VALUE(ret_val);
    return ret_val;
}

_int32 para_dns_dealloc(void *para_dns)
{
    _int32 ret_val = mpool_free_slip(gp_dns_slab, para_dns);
    CHECK_VALUE(ret_val);
    return ret_val;
}

_int32 para_accept_alloc(void **para_accept)
{
    _int32 ret_val = mpool_get_slip(gp_accept_slab, para_accept);
    CHECK_VALUE(ret_val);
    return ret_val;
}

_int32 para_accept_dealloc(void *para_accept)
{
    _int32 ret_val = mpool_free_slip(gp_accept_slab, para_accept);
    CHECK_VALUE(ret_val);
    return ret_val;
}

_int32 para_connsock_rw_alloc(void **para_cs_rw)
{
    _int32 ret_val = mpool_get_slip(gp_csrw_slab, para_cs_rw);
    CHECK_VALUE(ret_val);
    return ret_val;
}

_int32 para_connsock_rw_dealloc(void *para_cs_rw)
{
	_int32 ret_val = mpool_free_slip(gp_csrw_slab, para_cs_rw);
	CHECK_VALUE(ret_val);
	return ret_val;
}

_int32 para_noconnsock_rw_alloc(void **para_ncs_rw)
{
    _int32 ret_val = mpool_get_slip(gp_ncsrw_slab, para_ncs_rw);
    CHECK_VALUE(ret_val);
    return ret_val;
}

_int32 para_noconnsock_rw_dealloc(void *para_ncs_rw)
{
    _int32 ret_val = mpool_free_slip(gp_ncsrw_slab, para_ncs_rw);
    CHECK_VALUE(ret_val);
    return ret_val;
}

_int32 para_fsopen_alloc(void **para_fsopen)
{
	_int32 ret_val = mpool_get_slip(gp_fso_slab, para_fsopen);
	CHECK_VALUE(ret_val);
	return ret_val;
}

_int32 para_fsopen_dealloc(void *para_fsopen)
{
	_int32 ret_val = mpool_free_slip(gp_fso_slab, para_fsopen);
	CHECK_VALUE(ret_val);
	return ret_val;
}

_int32 para_fsrw_alloc(void **para_fsrw)
{
	_int32 ret_val = mpool_get_slip(gp_fsrw_slab, para_fsrw);
	CHECK_VALUE(ret_val);
	return ret_val;
}

_int32 para_fsrw_dealloc(void *para_fsrw)
{
	_int32 ret_val = mpool_free_slip(gp_fsrw_slab, para_fsrw);
	CHECK_VALUE(ret_val);
	return ret_val;
}


_int32 para_calc_hash_alloc(void **para_calc_hash)
{
    _int32 ret_val = mpool_get_slip(gp_calc_hash_slab, para_calc_hash);
    CHECK_VALUE(ret_val);
    return ret_val;
}

_int32 para_calc_hash_dealloc(void *para_calc_hash)
{
    _int32 ret_val = mpool_free_slip(gp_calc_hash_slab, para_calc_hash);
    CHECK_VALUE(ret_val);
    return ret_val;
}

_int32 para_sockaddr_alloc(void **para_sockaddr)
{
	_int32 ret_val = mpool_get_slip(gp_sockaddr_slab, para_sockaddr);
	CHECK_VALUE(ret_val);
	return ret_val;
}

_int32 para_sockaddr_dealloc(void *para_sockaddr)
{
	_int32 ret_val = mpool_free_slip(gp_sockaddr_slab, para_sockaddr);
	CHECK_VALUE(ret_val);
	return ret_val;
}

_int32 msg_thread_alloc(THREAD_MSG **ppmsg)
{
	_int32 ret_val = SUCCESS;

	/* lock */
	sd_task_lock(&g_thread_msg_lock);

	while((ret_val = GET_FIEXED_MPOOL_DATA(ppmsg)) == OUT_OF_FIXED_MEMORY)
	{
		ret_val = sd_task_cond_wait(&g_thread_msg_cond, &g_thread_msg_lock);
		CHECK_VALUE(ret_val);
	}

	sd_task_unlock(&g_thread_msg_lock);

	return ret_val;
}

_int32 msg_thread_dealloc(THREAD_MSG *pmsg)
{
	_int32 ret_val = SUCCESS;

	/* lock main thread of asyn_frame. we assume this case seldom happend */
	sd_task_lock(&g_thread_msg_lock);

	ret_val = FREE_FIXED_MPOOL_DATA(pmsg);
	if(ret_val != SUCCESS)
	{
		sd_task_unlock(&g_thread_msg_lock);
		return ret_val;
	}

	ret_val = sd_task_cond_signal(&g_thread_msg_cond);
	if(ret_val != SUCCESS)
	{
		sd_task_unlock(&g_thread_msg_lock);
		return ret_val;
	}

	sd_task_unlock(&g_thread_msg_lock);

	return ret_val;
}
