#include "asyn_frame/fs_reactor.h"
#include "asyn_frame/msg.h"
#include "asyn_frame/device_handler.h"
#include "platform/sd_task.h"
#include "platform/sd_fs.h"
#include "asyn_frame/device_reactor.h"

/* log */
#define LOGID LOGID_FS_REACTOR
#include "utility/logger.h"

#include "utility/thread.h"

#ifdef SET_PRIORITY
#include <sys/time.h>
#include <sys/resource.h>
#endif

static DEVICE_REACTOR g_calc_hash_reactor;
static _int32 g_in_queue_waitable_container;
static THREAD_STATUS g_thread_status;
static _int32 g_thread_id;
//static NOTICE_QUEUE g_in_queue;


static void calc_thread(void* args);

_int32 init_calc_hash_reactor(_int32* waitable_handle)
{

    _int32 ret = SUCCESS;
    device_reactor_init(&g_calc_hash_reactor);
    *waitable_handle = g_calc_hash_reactor._out_queue._waitable_handle;
    create_waitable_container(&g_in_queue_waitable_container);
    add_notice_handle(g_in_queue_waitable_container, g_calc_hash_reactor._in_queue._waitable_handle);
    g_thread_status = INIT;
    sd_create_task(calc_thread, 32* 1024, NULL, &g_thread_id);
    while (g_thread_status == INIT)
    {
        sd_sleep(20);
    }
    return SUCCESS;
}

_int32 uninit_calc_hash_reactor()
{
    _int32 ret_val = SUCCESS;
    MSG* pmsg = NULL;
    /*stop slop-op thread*/
    STOP_THREAD(g_thread_status);

    /* pop all remain msg */
    do 
    {
        /* mem leak */
        ret_val = pop_complete_event(&g_calc_hash_reactor, &pmsg);
        CHECK_VALUE(ret_val);
    } while(pmsg);

    wait_thread(&g_thread_status, g_calc_hash_reactor._in_queue._notice_handle);

    sd_finish_task(g_thread_id);
    g_thread_id = 0;

    /* destory other variables */
    ret_val = destory_waitable_container(g_in_queue_waitable_container);
    CHECK_VALUE(ret_val);

    ret_val = device_reactor_uninit(&g_calc_hash_reactor);
    CHECK_VALUE(ret_val);
    return ret_val;
}

void calc_thread(void* args)
{
    _int32 notice_handle;
    _int32 errono = SUCCESS;
    MSG* pmsg = NULL;
    RUN_THREAD(g_thread_status);
    BEGIN_THREAD_LOOP(g_thread_status)
    wait_for_notice(g_in_queue_waitable_container, 1, &notice_handle, WAIT_INFINITE);
    reset_notice(notice_handle);
    pop_register_event_with_lock(&g_calc_hash_reactor, (void**)&pmsg);
    while(pmsg && IS_THREAD_RUN(g_thread_status) )
    {
        
        // handle
        BOOL completed = TRUE;
        _int32 handle_errno = SUCCESS;

        handle_errno = handle_msg(pmsg, &completed);

        notice_complete_event(&g_calc_hash_reactor, errono, pmsg);

        pop_register_event_with_lock(&g_calc_hash_reactor, (void**)&pmsg);
    }
    END_THREAD_LOOP(g_thread_status);
    finished_thread(&g_thread_status);
}

_int32 get_complete_calc_hash_msg(MSG **msg)
{
    *msg = NULL;

    _int32 ret_val = pop_complete_event(&g_calc_hash_reactor, msg);
    CHECK_VALUE(ret_val);

    return ret_val;
}

_int32 regist_calc_hash_event(MSG* msg)
{
    return register_event(&g_calc_hash_reactor, msg, NULL);
}

_int32 unregister_calc_hash(MSG *msg, _int32 reason)
{
    return unregister_event_immediately(&g_calc_hash_reactor, msg, reason, (void**)msg->_inner_data);
}