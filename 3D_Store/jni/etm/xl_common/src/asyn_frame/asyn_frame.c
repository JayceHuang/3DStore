#if defined(LINUX)
#include <signal.h>
#endif

#ifdef SET_PRIORITY
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include "utility/define.h"
#include "asyn_frame/asyn_frame.h"
#include "asyn_frame/msg_list.h"
#include "asyn_frame/msg_alloctor.h"
#include "asyn_frame/socket_reactor.h"
#include "asyn_frame/fs_reactor.h"
#include "asyn_frame/dns_reactor.h"
#include "asyn_frame/timer.h"
#include "utility/errcode.h"
#include "utility/map.h"
#include "utility/utility.h"
#include "utility/thread.h"
#include "platform/sd_task.h"
#include "asyn_frame/calc_hash_reactor.h"

#define LOGID LOGID_ASYN_FRAME
#include "utility/logger.h"

#include "platform/sd_network.h"

static init_handler g_asyn_frame_init_handler = NULL;
static void *g_asyn_frame_init_arg = NULL;
static uninit_handler g_asyn_frame_uninit_handler = NULL;
static void *g_asyn_frame_uninit_arg = NULL;

static THREAD_STATUS g_asyn_frame_thread_status = INIT;
static _int32 g_asyn_frame_thread_id = 0;

static _int32 handle_all_newmsgs(void);
static _int32 delete_msg(MSG *pmsg);
static _int32 handle_reactor_msg(MSG *pmsg);
static _int32 handle_all_event(void);
static _int32 handle_timeout_msg(MSG *pmsg);
static _int32 handle_all_timeout_msg(void);
static _int32 handle_new_msg(MSG *pnewmsg);
static _int32 handle_new_thread_msg(THREAD_MSG *pnewmsg);
static _int32 callback_msg(MSG *pmsg, _int32 msg_errcode, _u32 elapsed);
static _int32 handle_immediate_msg(MSG *pmsg);

static void reset_msg_notice_count_left(MSG *pmsg)
{
    pmsg->_notice_count_left = 0;
}

static void add_msg_notice_count_left(MSG *pmsg)
{
    if (pmsg->_notice_count_left != NOTICE_INFINITE)
    {
        pmsg->_notice_count_left++;
    }
}

static void del_msg_notice_count_left(MSG *pmsg)
{
    if ((pmsg->_notice_count_left != NOTICE_INFINITE) && (pmsg->_notice_count_left > 0))
    {
        pmsg->_notice_count_left--;
    }
}


#define DISPATCH_SOCKET_REACTOR     (100)
#define DISPATCH_FS_REACTOR         (10)
#define DISPATCH_DNS_REACTOR        (100)
#define DISPATCH_IMMEDIATE_MSG      (10)
#define DISPATCH_OTHER_THREAD       (10)


/* thread error handler */
static _int32 g_asyn_frame_error_exitcode = SUCCESS;
static void asyn_frame_handler_error_exit()
{
    LOG_ERROR("****** ERROR OCCUR in asyn-frame thread(%u): %d !!!!", sd_get_self_taskid(), g_asyn_frame_error_exitcode);
#ifdef _DEBUG
    sd_assert(FALSE);
#endif
    set_critical_error(g_asyn_frame_error_exitcode);
    finished_thread(&g_asyn_frame_thread_status);
}
#define EXIT(code)  {g_asyn_frame_error_exitcode = code; asyn_frame_handler_error_exit();}


static LIST g_asyn_frame_timeout_list;
static LIST g_immediate_msg_list;

static LIST g_lost_cancel_msg_id ;

static SUSPEND_DATA g_asyn_frame_suspend_data;

static _int32 asyn_frame_timer_node_comparator(void *m1, void *m2)
{
    return (_int32)(((MSG*)m1)->_msg_id - ((MSG*)m2)->_msg_id);
}

static _int32 asyn_frame_handler_init(_int32 *wait_msg, _int32 *wait_sock, 
    _int32 *wait_fs, _int32 *wait_slow_fs, _int32 *wait_dns, _int32* wait_calc_hash, _int32 *waitable_container)
{
    _int32 ret_val = msg_alloctor_init();
    CHECK_THREAD_VALUE(ret_val);
    
    ret_val = msg_queue_init(wait_msg);
    CHECK_THREAD_VALUE(ret_val);

    /* init all reactor */    
    ret_val = init_socket_reactor(wait_sock);
    CHECK_THREAD_VALUE(ret_val);
    
    ret_val = init_fs_reactor(wait_fs, wait_slow_fs);
    CHECK_THREAD_VALUE(ret_val);
    
    ret_val = init_dns_reactor(wait_dns);
    CHECK_THREAD_VALUE(ret_val);

    ret_val = init_calc_hash_reactor(wait_calc_hash);
    CHECK_THREAD_VALUE(ret_val);


    /* init timer */
    ret_val = init_timer();
    CHECK_THREAD_VALUE(ret_val);

    /* init waitable container */    
    ret_val = create_waitable_container(waitable_container);
    CHECK_THREAD_VALUE(ret_val);

    ret_val = add_notice_handle(*waitable_container, *wait_msg);
    CHECK_THREAD_VALUE(ret_val);

    ret_val = add_notice_handle(*waitable_container, *wait_sock);
    CHECK_THREAD_VALUE(ret_val);

    ret_val = add_notice_handle(*waitable_container, *wait_fs);
    CHECK_THREAD_VALUE(ret_val);

    ret_val = add_notice_handle(*waitable_container, *wait_slow_fs);
    CHECK_THREAD_VALUE(ret_val);

    ret_val = add_notice_handle(*waitable_container, *wait_dns);
    CHECK_THREAD_VALUE(ret_val);

    ret_val = add_notice_handle(*waitable_container, *wait_calc_hash);
    CHECK_THREAD_VALUE(ret_val);


    /* init timeout list */
    list_init(&g_asyn_frame_timeout_list);

    /* init immediate-msg-list */
    list_init(&g_immediate_msg_list);

    /* init cancel-msg-list */
    list_init(&g_lost_cancel_msg_id);

    if(g_asyn_frame_init_handler)
    {
        ret_val = g_asyn_frame_init_handler(g_asyn_frame_init_arg);
        if (ret_val != SUCCESS)
        {
            LOG_ERROR("aysn_frame init failed, user-init return %d", ret_val);
            CHECK_THREAD_VALUE(ret_val);
            return ret_val;
        }
    }

    /* refresh timer firstly */
    ret_val = refresh_timer();
    CHECK_THREAD_VALUE(ret_val);

    return ret_val;
}

static _int32 asyn_frame_handler_uninit(_int32 waitable_container)
{
    if (g_asyn_frame_uninit_handler)
    {
        g_asyn_frame_uninit_handler(g_asyn_frame_uninit_arg);
    }

    /* uninit all allocators */
    _int32 ret_val = destory_waitable_container(waitable_container);
    CHECK_THREAD_VALUE(ret_val);

    ret_val = uninit_timer();
    CHECK_THREAD_VALUE(ret_val);

    ret_val = uninit_calc_hash_reactor();
    CHECK_THREAD_VALUE(ret_val);

    ret_val = uninit_dns_reactor();
    CHECK_THREAD_VALUE(ret_val);

    ret_val = uninit_fs_reactor();
    CHECK_THREAD_VALUE(ret_val);

    ret_val = uninit_socket_reactor();
    CHECK_THREAD_VALUE(ret_val);

    ret_val = msg_queue_uninit();
    CHECK_THREAD_VALUE(ret_val);

    ret_val = msg_alloctor_uninit();
    CHECK_THREAD_VALUE(ret_val);

    finished_thread(&g_asyn_frame_thread_status);
    
    return ret_val;
}

static _int32 asyn_frame_handler_run(_int32 wait_msg, _int32 wait_sock, 
    _int32 wait_fs, _int32 wait_slow_fs, _int32 wait_dns, _int32 wait_calc_hash, _int32 waitable_container)
{
    thread_check_suspend(&g_asyn_frame_suspend_data);

    /* handle new msg*/
    _int32 ret_val = handle_all_newmsgs();
    CHECK_THREAD_VALUE(ret_val);

    /* handle timeout msg */
    ret_val = handle_all_timeout_msg();
    CHECK_THREAD_VALUE(ret_val);

    /* handle all event(all reactor-msg and timeout-msg) */
    ret_val = handle_all_event();
    CHECK_THREAD_VALUE(ret_val);

    /* wait for notice */
    _int32 tmp = 0;
    ret_val = wait_for_notice(waitable_container, 1, &tmp, GRANULARITY);
    CHECK_THREAD_VALUE(ret_val);

    /* reset all event */
    ret_val = reset_notice(wait_msg);
    CHECK_THREAD_VALUE(ret_val);
    ret_val = reset_notice(wait_sock);
    CHECK_THREAD_VALUE(ret_val);
    ret_val = reset_notice(wait_fs);
    CHECK_THREAD_VALUE(ret_val);
    ret_val = reset_notice(wait_slow_fs);
    CHECK_THREAD_VALUE(ret_val);
    ret_val = reset_notice(wait_dns);
    CHECK_THREAD_VALUE(ret_val);
    ret_val = reset_notice(wait_calc_hash);
    CHECK_THREAD_VALUE(ret_val);

    return ret_val;
}

void asyn_frame_handler(void *arglist)
{
    sd_ignore_signal();

#ifdef LINUX
#ifdef SET_PRIORITY
    setpriority(PRIO_PROCESS, 0, 5);
#endif
#endif

    _int32 wait_msg = 0;
    _int32 wait_sock = 0;
    _int32 wait_fs = 0;
    _int32 wait_slow_fs = 0;
    _int32 wait_dns = 0;
    _int32 wait_calc_hash = 0;
    _int32 waitable_container = 0;
    
    asyn_frame_handler_init(&wait_msg, &wait_sock, &wait_fs, &wait_slow_fs, &wait_dns, &wait_calc_hash, &waitable_container);

    RUN_THREAD(g_asyn_frame_thread_status);

    BEGIN_THREAD_LOOP(g_asyn_frame_thread_status)
    {
       asyn_frame_handler_run(wait_msg, wait_sock, wait_fs, wait_slow_fs, wait_dns, wait_calc_hash, waitable_container);
    }
    END_THREAD_LOOP(g_asyn_frame_thread_status);

    asyn_frame_handler_uninit(waitable_container);
}

_int32 start_asyn_frame(init_handler h_init, void *init_arg, uninit_handler h_uninit, void *uninit_arg)
{
    if ((INIT != g_asyn_frame_thread_status) && (STOPPED != g_asyn_frame_thread_status))
    {
        return ALREADY_ET_INIT;
    }

    _int32 ret_val = SUCCESS;
    g_asyn_frame_thread_status = INIT;

    g_asyn_frame_init_handler = h_init;
    g_asyn_frame_init_arg = init_arg;
    g_asyn_frame_uninit_handler = h_uninit;
    g_asyn_frame_uninit_arg = uninit_arg;

    thread_suspend_init(&g_asyn_frame_suspend_data);

    ret_val = sd_create_task(asyn_frame_handler, 256 * 1024, NULL, &g_asyn_frame_thread_id);
    CHECK_VALUE(ret_val);

    /* wait for thread running */
    while (INIT == g_asyn_frame_thread_status)
    {
        sd_sleep(20);
    }

    if (FALSE == IS_THREAD_RUN(g_asyn_frame_thread_status))
	{
		ret_val = sd_finish_task(g_asyn_frame_thread_id);
		g_asyn_frame_thread_id = 0;
		CHECK_VALUE(ret_val);
		//NODE:jieouy 先平加上退出处理
		sd_exit(ret_val);
	}

    return g_asyn_frame_error_exitcode;
}

BOOL is_asyn_frame_running(void)
{
    return IS_THREAD_RUN(g_asyn_frame_thread_status);
}

_int32 stop_asyn_frame(void)
{
    thread_do_resume(&g_asyn_frame_suspend_data);
    
    STOP_THREAD(g_asyn_frame_thread_status);
    wait_thread(&g_asyn_frame_thread_status, 0);

    sd_finish_task(g_asyn_frame_thread_id);
    g_asyn_frame_thread_id = 0;

    thread_suspend_uninit(&g_asyn_frame_suspend_data);

    return SUCCESS;
}

static _int32 cancel_from_reactor(MSG *pmsg, _int32 reason)
{
    _int32 ret_val = SUCCESS;

    _int32 device_type = pmsg->_msg_info._device_type;
    _int32 op_type = pmsg->_msg_info._operation_type;

    if ((device_type == DEVICE_SOCKET_TCP) || (device_type == DEVICE_SOCKET_UDP))
    {
        ret_val = unregister_socket(pmsg, reason);
        CHECK_VALUE(ret_val);
    }
    else if (device_type == DEVICE_FS)
    {
        ret_val = unregister_fs(pmsg, reason);
        CHECK_VALUE(ret_val);
    }
    else if (op_type == OP_DNS_RESOLVE)
    {
        ret_val = unregister_dns(pmsg, reason);
        CHECK_VALUE(ret_val);
    }
    else if (op_type == OP_CALC_HASH )
    {
        ret_val = unregister_calc_hash(pmsg, reason);
        CHECK_VALUE(ret_val);
    }
    else
    {
        LOG_DEBUG("msg(%d) not a reactor-msg", pmsg->_msg_id);
    }

    return ret_val;
}

static _int32 put_into_reactor(MSG *pmsg)
{
    _int32 ret_val = SUCCESS;

    _int32 device_type = pmsg->_msg_info._device_type;
    _int32 op_type = pmsg->_msg_info._operation_type;

    if ((device_type == DEVICE_SOCKET_TCP) || (device_type == DEVICE_SOCKET_UDP))
    {
        ret_val = register_socket_event(pmsg);
        CHECK_VALUE(ret_val);
    }
    else if (device_type == DEVICE_FS)
    {
        ret_val = register_fs_event(pmsg);
        CHECK_VALUE(ret_val);
    }
    else if (op_type == OP_DNS_RESOLVE)
    {
        ret_val = register_dns_event(pmsg);
        CHECK_VALUE(ret_val);
    }
    else if( op_type == OP_CALC_HASH)
    {
        ret_val = regist_calc_hash_event(pmsg);
        CHECK_VALUE(ret_val);
    }
    else
    {
        LOG_DEBUG("msg(%d) not a reactor-msg when put into reactor ", pmsg->_msg_id);
    }

    return ret_val;
}

static BOOL handle_new_msg_cancel_timer(MSG *pnewmsg)
{    
    MSG *pmsg = NULL;
    erase_from_timer(pnewmsg, asyn_frame_timer_node_comparator, TIMER_INDEX_NONE, (void**)&pmsg);
    BOOL ret_val = (NULL != pmsg);
    
    if (pmsg)
    {
        if (REACTOR_REGISTERED(pmsg->_cur_reactor_status))
        {
            /* cancel from reactor, _op_count > 0 */
            LOG_DEBUG("the cancelled-msg(id:%d) is a reactor-msg, so cancel from reactor", pmsg->_msg_id);
            ret_val = cancel_from_reactor(pmsg, REACTOR_STATUS_CANCEL);
            CHECK_VALUE(ret_val);
        }
        else if (pmsg->_op_count == 0)
        {
            /* call back now */
            pmsg->_notice_count_left = 0;

            /* this msg has no matter of reactor, so use _cur_reactor_status flag to denote its status */
            pmsg->_cur_reactor_status = REACTOR_STATUS_CANCEL;
            LOG_DEBUG("the msg(id:%d) will put into immediate-list for callback because it is cancelled", pmsg->_msg_id);
            ret_val = list_push(&g_immediate_msg_list, pmsg);
            CHECK_VALUE(ret_val);
        }
    }
    return ret_val;
}

static BOOL handle_new_msg_cancel_timeout(MSG *pnewmsg)
{
    BOOL ret_val = FALSE;

    LIST_ITERATOR list_it = LIST_BEGIN(g_asyn_frame_timeout_list);
    for (; list_it != LIST_END(g_asyn_frame_timeout_list); list_it = LIST_NEXT(list_it))
    {
        if (asyn_frame_timer_node_comparator(pnewmsg, LIST_VALUE(list_it)) == 0)
        {
            MSG *pmsg = LIST_VALUE(list_it);
            /* notice once for cancel msg */
            pmsg->_notice_count_left = 1;
            LOG_DEBUG("the msg(id:%d) be cancelled and it is a timeout msg", pmsg->_msg_id);
            /* handle it at function(handle_timeout_msg) */
            pmsg->_msg_cannelled = 1;
            ret_val = TRUE;
            break;
        }
    }
    return ret_val;
}

static BOOL handle_new_msg_cancel_immediate(MSG *pnewmsg)
{
    BOOL ret_val = FALSE;
    LIST_ITERATOR list_it = LIST_BEGIN(g_immediate_msg_list);
    for (; list_it != LIST_END(g_immediate_msg_list); list_it = LIST_NEXT(list_it))
    {
        if (asyn_frame_timer_node_comparator(pnewmsg, LIST_VALUE(list_it)) == 0)
        {
            MSG * pmsg = LIST_VALUE(list_it);
            LOG_DEBUG("the msg(id:%d) be cancelled and it is a immediate-msg", pmsg->_msg_id);
            /* change status to cancel... */
            pmsg->_cur_reactor_status = REACTOR_STATUS_CANCEL;
            ret_val = TRUE;
            break;
        }
    }
    return ret_val;
}

static _int32 handle_new_msg_cancel_msg(MSG *pnewmsg)
{
    LOG_DEBUG("the msg(id:%d) is a cancel-msg", pnewmsg->_msg_id);
    BOOL bHandle = handle_new_msg_cancel_timer(pnewmsg);
    if (FALSE == bHandle)
    {
        bHandle = handle_new_msg_cancel_timeout(pnewmsg);
        if (FALSE == bHandle)
        {
            bHandle = handle_new_msg_cancel_immediate(pnewmsg);
            if (FALSE == bHandle)
            {
                LOG_DEBUG("cancelling a lost msg(id:%d) ....", pnewmsg->_msg_id);
                list_push(&g_lost_cancel_msg_id, (void*)(pnewmsg->_msg_id));
            }
        }
    }

    msg_dealloc(pnewmsg);
    return SUCCESS;
}

static _int32 handle_new_msg_cancel_device(MSG *pnewmsg)
{
    _int32 ret_val = SUCCESS;
    /* cancel msg by deviceid, only support socket now */
    if ((pnewmsg->_msg_info._device_type != DEVICE_SOCKET_TCP)
        && (pnewmsg->_msg_info._device_type != DEVICE_SOCKET_UDP))
    {
        LOG_ERROR("not support device-type(%d) when cancel-msg-by-deviceid", pnewmsg->_msg_info._device_type);
        return NOT_IMPLEMENT;
    }

    LOG_DEBUG("ready to cancel socket(%d) from reactor", pnewmsg->_msg_info._device_id);
    ret_val = cancel_socket(pnewmsg->_msg_info._device_id);
    CHECK_VALUE(ret_val);

    ret_val = msg_dealloc(pnewmsg);
    return ret_val;
}

static _int32 handle_new_msg_normal_msg(MSG *pnewmsg)
{
    _int32 ret_val = SUCCESS;

    /* init reactor status */
    pnewmsg->_cur_reactor_status = REACTOR_STATUS_READY;
    pnewmsg->_op_count = 0;
    pnewmsg->_op_errcode = SUCCESS;

    if (pnewmsg->_timeout == 0)/* callback immediately*/
    {
        LOG_DEBUG("put msg(id:%d) into immediate-list because it is a immediate-msg of timeout 0", pnewmsg->_msg_id);
        reset_msg_notice_count_left(pnewmsg);

        /* this msg has no matter of reactor, so use _cur_reactor_status flag to denote its status */
        pnewmsg->_cur_reactor_status = REACTOR_STATUS_TIMEOUT;
        ret_val = list_push(&g_immediate_msg_list, pnewmsg);
    }
    else/* a normal msg */
    {
        pnewmsg->_timestamp = get_current_timestamp();
        LOG_DEBUG("a normal msg(id:%d), set its timestamp(%u) ready to put into timer & reactor", pnewmsg->_msg_id, pnewmsg->_timestamp);

        _int32 time_index = 0;
        ret_val = put_into_timer(pnewmsg->_timeout, pnewmsg, &time_index);
        CHECK_VALUE(ret_val);
        LOG_DEBUG("put_into_timer, pnewmsg:0x%x, pnewmsg->_timeout = %d, pnewmsg->_msg_id = %d, time_index = %d, device_id = %d"
            ,pnewmsg, pnewmsg->_timeout, pnewmsg->_msg_id, time_index, pnewmsg->_msg_info._device_id);
        pnewmsg->_timeout_index = (_int16)time_index;

        ret_val = put_into_reactor(pnewmsg);
    }
    return ret_val;
}

static _int32 handle_new_msg(MSG *pnewmsg)
{
    _int32 ret_val = SUCCESS;

    if (pnewmsg)
    {
        if ((pnewmsg->_notice_count_left <= 0) 
            && (pnewmsg->_notice_count_left != NOTICE_INFINITE) 
            && VALID_HANDLE((_int32)pnewmsg->_handler))
        {
            LOG_ERROR("the msg(id:%d) notice_count is %d, drop it", pnewmsg->_msg_id, pnewmsg->_notice_count_left);

            ret_val = msg_dealloc(pnewmsg);
            CHECK_VALUE(ret_val);

            return SUCCESS;
        }

        if ((_int32)pnewmsg->_handler == CANCEL_MSG_BY_MSGID)
        {
            ret_val = handle_new_msg_cancel_msg(pnewmsg);
        }
        else if((_int32)pnewmsg->_handler == CANCEL_MSG_BY_DEVICEID)
        {
            ret_val = handle_new_msg_cancel_device(pnewmsg);
        }
        else
        {
            ret_val = handle_new_msg_normal_msg(pnewmsg);
        }
    }

    return ret_val;
}

static _int32 handle_all_event(void)
{
    _int32 ret_val = SUCCESS;
    BOOL continued = FALSE;
    
    do
    {
        continued = FALSE;

        MSG *pmsg = NULL;
        _int32 idx = 0;

        /* socket reactor */
        for (idx = 0; idx < DISPATCH_SOCKET_REACTOR; idx++)
        {
            ret_val = get_complete_socket_msg(&pmsg);
            CHECK_VALUE(ret_val);
            if (NULL == pmsg)
            {
                break;
            }

            ret_val = handle_reactor_msg(pmsg);
            CHECK_VALUE(ret_val);

            continued = TRUE;
        }

        /* fs reactor */
        for (idx = 0; idx < DISPATCH_FS_REACTOR; idx++)
        {
            ret_val = get_complete_fs_msg(&pmsg);
            CHECK_VALUE(ret_val);
            if (NULL == pmsg)
            {
                break;
            }

            ret_val = handle_reactor_msg(pmsg);
            CHECK_VALUE(ret_val);

            continued = TRUE;
        }

        /* dns reactor */
        for (idx = 0; idx < DISPATCH_DNS_REACTOR; idx++)
        {
            ret_val = get_complete_calc_hash_msg(&pmsg);
            CHECK_VALUE(ret_val);
            if (NULL == pmsg)
            {
                break;
            }

            ret_val = handle_reactor_msg(pmsg);
            CHECK_VALUE(ret_val);

            continued = TRUE;
        }
        

        /* dns reactor */
        for (idx = 0; idx < DISPATCH_DNS_REACTOR; idx++)
        {
            ret_val = get_complete_dns_msg(&pmsg);
            CHECK_VALUE(ret_val);
            if (NULL == pmsg)
            {
                break;
            }

            ret_val = handle_reactor_msg(pmsg);
            CHECK_VALUE(ret_val);

            continued = TRUE;
        }

        /* immediate-msg */
        for (idx = 0; idx < DISPATCH_IMMEDIATE_MSG; idx++)
        {
            ret_val = list_pop(&g_immediate_msg_list, (void**)&pmsg);
            CHECK_VALUE(ret_val);
            if (NULL == pmsg)
            {
                break;
            }

            ret_val = handle_immediate_msg(pmsg);
            CHECK_VALUE(ret_val);

            continued = TRUE;
        }

        /* other-thread-msg */
        THREAD_MSG *pthread_msg = NULL;
        for (idx = 0; idx < DISPATCH_OTHER_THREAD; idx++)
        {
            ret_val = pop_msginfo_node_from_other_thread(&pthread_msg);
            CHECK_VALUE(ret_val);
            if (NULL == pthread_msg)
            {
                break;
            }

            ret_val = handle_new_thread_msg(pthread_msg);
            CHECK_VALUE(ret_val);

            continued = TRUE;
        }

        ret_val = handle_all_timeout_msg();
        CHECK_VALUE(ret_val);

    }
    while (continued);

    return ret_val;
}

static _int32 handle_timeout_msg(MSG * pmsg)
{
    _int32 ret_val = SUCCESS;

    LOG_DEBUG("handle_timeout_msg, pmsg->_cur_reactor_status = %d,pmsg->_timeout_index = %d, pmsg->_handler = 0x%x\
              , pmsg->_inner_data = 0x%x, pmsg->_msg_cannelled = %d \
              , pmsg->_msg_id = %d, pmsg->_msg_info._device_id = %d, pmsg->_msg_info._device_type = %d \
              , pmsg->_msg_info._operation_type = %d, pmsg->_msg_info._pending_op_count = %d \
              , pmsg->_msg_info._user_data = 0x%x, pmsg->_notice_count_left = %d \
              , pmsg->_op_count = %d, pmsg->_op_errcode = %d, pmsg->_timeout = %d"
        , pmsg->_cur_reactor_status
        , pmsg->_timeout_index
        , (void*)pmsg->_handler
        , pmsg->_inner_data
        , pmsg->_msg_cannelled
        , pmsg->_msg_id
        , pmsg->_msg_info._device_id
        , pmsg->_msg_info._device_type
        , pmsg->_msg_info._operation_type
        , pmsg->_msg_info._pending_op_count
        , pmsg->_msg_info._user_data
        , pmsg->_notice_count_left
        , pmsg->_op_count
        , pmsg->_op_errcode
        , pmsg->_timeout 
        );
    if (REACTOR_REGISTERED(pmsg->_cur_reactor_status))
    {
        LOG_DEBUG("find a timeout msg(id:%d) but a reactor event,so unregist from reactor", pmsg->_msg_id);
        ret_val = cancel_from_reactor(pmsg, REACTOR_STATUS_TIMEOUT);
        CHECK_VALUE(ret_val);
    }
    else if (pmsg->_op_count == 0)
    {
        del_msg_notice_count_left(pmsg);
        _int32 errcode = 0;
        if (pmsg->_msg_cannelled)
        {
            LOG_DEBUG("ready to callback a timeout msg(id:%d) and it be cancelled", pmsg->_msg_id);
            errcode = MSG_CANCELLED;
        }
        else
        {
            LOG_DEBUG("ready to callback msg(id:%d) because of timeout", pmsg->_msg_id);
            errcode = MSG_TIMEOUT;
        }
        ret_val = callback_msg(pmsg, errcode, pmsg->_timeout);
        CHECK_VALUE(ret_val);
        LOG_DEBUG("finish to callback about in timeout-handler");
    }

    return ret_val;
}

static _int32 handle_all_timeout_msg(void)
{
    _int32 ret_val = refresh_timer();
    CHECK_VALUE(ret_val);

    ret_val = pop_all_expire_timer(&g_asyn_frame_timeout_list);
    CHECK_VALUE(ret_val);
    
    LIST_ITERATOR list_it = LIST_BEGIN(g_asyn_frame_timeout_list);
    for (; list_it != LIST_END(g_asyn_frame_timeout_list); )
    {
        MSG *pmsg = LIST_VALUE(list_it);
        list_it = LIST_NEXT(list_it);
        list_erase(&g_asyn_frame_timeout_list, LIST_PRE(list_it));
        handle_timeout_msg(pmsg);
    }

    return ret_val;
}

static _int32 handle_immediate_msg(MSG *pmsg)
{
    _int32 ret_val = SUCCESS;
    _int32 msg_errcode = 0;
    _u32 elapsed = 0;

    if (pmsg->_cur_reactor_status == REACTOR_STATUS_CANCEL)
    {
        elapsed = get_current_timestamp();
        if (elapsed > pmsg->_timestamp)
        {
            elapsed -= pmsg->_timestamp;
        }
        else
        {
            elapsed = 0;
        }

        msg_errcode = MSG_CANCELLED;
        LOG_DEBUG("the handler of msg(id:%d) gotton from immediate-list will be callback because it is cancelled", pmsg->_msg_id);
    }
    else if (pmsg->_cur_reactor_status == REACTOR_STATUS_TIMEOUT)
    {
        elapsed = 0;
        msg_errcode = MSG_TIMEOUT;
        LOG_DEBUG("the handler of msg(id:%d) gotton from immediate-list will be callback because it timeout(timeout is 0)", pmsg->_msg_id);
    }
    else
    {
        LOG_ERROR("unexpected reactor status(%d) in immediate-msg-list", pmsg->_cur_reactor_status);
        return NOT_IMPLEMENT;
    }

    ret_val = callback_msg(pmsg, msg_errcode, elapsed);
    CHECK_VALUE(ret_val);
    LOG_DEBUG("finish to callback in immediate-msg-handler");

    return ret_val;
}

static _int32 handle_lost_cancel_msg(MSG *pmsg)
{
    _int32 msg_errcode = MSG_TIMEOUT;
    if (0 != list_size(&g_lost_cancel_msg_id))
    {
        LIST_ITERATOR list_it = LIST_BEGIN(g_lost_cancel_msg_id);
        _u32 lost_msg_id = 0;
        for (; list_it != LIST_END(g_lost_cancel_msg_id); list_it = LIST_NEXT(list_it))
        {
            lost_msg_id = (_u32)LIST_VALUE(list_it);
            if (pmsg->_msg_id == lost_msg_id)
            {
                msg_errcode =  MSG_CANCELLED ;
                LOG_DEBUG("success process a  cancelling a lost msg(id:%d) ....", pmsg->_msg_id);
                list_erase(&g_lost_cancel_msg_id, list_it);
                break;
            }
        }
    }

    return msg_errcode;
}

static _int32 handle_reactor_msg(MSG *pmsg)
{
    _int32 ret_val = SUCCESS;
    _int32 msg_errcode = 0;

    LOG_DEBUG("msg._op_count = %d msg._cur_reactor_status = %d, msg._op_errcode = %d", pmsg->_op_count, pmsg->_cur_reactor_status, pmsg->_op_errcode );
    if (pmsg->_op_count == 0)
    {
        ret_val = erase_from_timer(pmsg, asyn_frame_timer_node_comparator, pmsg->_timeout_index, NULL);
        //CHECK_VALUE(ret_val);

        if (REACTOR_REGISTERED(pmsg->_cur_reactor_status))
        {
            del_msg_notice_count_left(pmsg);
            msg_errcode = pmsg->_op_errcode;
        }
        else if (pmsg->_cur_reactor_status == REACTOR_STATUS_CANCEL)
        {
            reset_msg_notice_count_left(pmsg);
            msg_errcode = MSG_CANCELLED;
        }
        else if (pmsg->_cur_reactor_status == REACTOR_STATUS_TIMEOUT)
        {
            del_msg_notice_count_left(pmsg);
            /*process the timeout msg which is timeout when it is canceling */
            msg_errcode = handle_lost_cancel_msg(pmsg);
        }
        else
        {
            LOG_ERROR("Error status(%d) in socket-reactor", pmsg->_cur_reactor_status);

            /* ready to delete it */
            reset_msg_notice_count_left(pmsg);
        }

        pmsg->_cur_reactor_status = REACTOR_STATUS_READY;

        _u32 elapsed = get_current_timestamp();
        if (elapsed > pmsg->_timestamp)
        {
            elapsed -= pmsg->_timestamp;
        }
        else
        {
            elapsed = 0;
        }

        LOG_DEBUG("callback of msg(id:%d) because a completed reactor of errcode(%d)", pmsg->_msg_id, msg_errcode);

        ret_val = callback_msg(pmsg, msg_errcode, elapsed);
        CHECK_VALUE(ret_val);

        LOG_DEBUG("finish to callback in reactor-handler");
    }
    else
    {
        LOG_DEBUG("can not  callback of msg(id:%d) because op_count:%d is no equal zero . ", pmsg->_msg_id, (_int32)pmsg->_op_count);
    }

    return ret_val;
}

static _int32 callback_msg(MSG *pmsg, _int32 msg_errcode, _u32 elapsed)
{
    _int32 ret_val = SUCCESS;

    if (pmsg)
    {
        LOG_DEBUG("will callback of msg(id:%d), (errcode:%d, notice_left:%d, elapsed:%u", pmsg->_msg_id, msg_errcode, pmsg->_notice_count_left, elapsed);

        /* If critial error , stop doing anything ...*/
        if ((get_critical_error() != SUCCESS) && (msg_errcode == MSG_TIMEOUT))
        {
            ret_val = pmsg->_handler(&pmsg->_msg_info, get_critical_error(), pmsg->_notice_count_left, elapsed, pmsg->_msg_id);
        }
        else
        {
            if (pmsg->_handler)
            {
                ret_val = pmsg->_handler(&pmsg->_msg_info, msg_errcode, pmsg->_notice_count_left, elapsed, pmsg->_msg_id);
            }
            else
            {
                /* Fatal Error! need restart download_engine */
                sd_assert(FALSE);
                set_critical_error(INVALID_HANDLER);
                ret_val = INVALID_HANDLER;
            }
        }

        LOG_DEBUG("callback return %d about msg(id:%d)", ret_val, pmsg->_msg_id);

        if (pmsg->_notice_count_left == 0)
        {
            LOG_DEBUG("try to delete msg(id:%d) because end of callback", pmsg->_msg_id);
            ret_val = delete_msg(pmsg);
            CHECK_VALUE(ret_val);
        }
        else
        {
            LOG_DEBUG("ready to renew msg(id:%d) because its notice_count is %d", pmsg->_msg_id, pmsg->_notice_count_left);
            ret_val = handle_new_msg(pmsg);
            CHECK_VALUE(ret_val);
        }

        /* Handle new msg to assure the cancelled msg callback with errcode MSG_CANCELLED
              since user can cancel msg in last callback.
           This can be optimazed by handle every type of msg one by one */

        ret_val = handle_all_newmsgs();
        CHECK_VALUE(ret_val);
    }

    return ret_val;
}

static _int32 handle_new_thread_msg(THREAD_MSG *pnewmsg)
{
    _int32 ret_val = SUCCESS;
    if (pnewmsg)
    {
        ret_val = pnewmsg->_handle(pnewmsg->_param);

        LOG_DEBUG("callback of thread-msg return : %d", ret_val);

        /* free the msg.
         * this operation will be locked, we assume this opeartion happened seldom.
         */
        ret_val = msg_thread_dealloc(pnewmsg);
        CHECK_VALUE(ret_val);

        /* Handle new msg to assure the cancelled msg callback with errcode MSG_CANCELLED
              since user can cancel msg in upper callback.
           This can be optimazed by handle every type of msg one by one */
        ret_val = handle_all_newmsgs();
        CHECK_VALUE(ret_val);
    }
    return ret_val;
}

static _int32 delete_msg(MSG *pmsg)
{
    _int32 ret_val = SUCCESS;
    if ((pmsg->_notice_count_left == 0) && (pmsg->_op_count == 0))
    {
        LOG_DEBUG("will delete msg(id:%d)", pmsg->_msg_id);

        /* free the memory of msg */
        ret_val = dealloc_parameter(&pmsg->_msg_info);
        CHECK_VALUE(ret_val);

        ret_val = msg_dealloc(pmsg);
        CHECK_VALUE(ret_val);
    }
    return ret_val;
}

static _int32 handle_all_newmsgs(void)
{
    /* refresh timer for put new msg */
    _int32 ret_val = refresh_timer();
    CHECK_VALUE(ret_val);

    MSG *pmsg = NULL;
    /* try to get a new msg */
    ret_val = pop_msginfo_node(&pmsg);
    CHECK_VALUE(ret_val);

    while (pmsg)
    {
        LOG_DEBUG("ready to handle new msg(id:%d)", pmsg->_msg_id);
        ret_val = handle_new_msg(pmsg);
        CHECK_VALUE(ret_val);

        pmsg = NULL;
        ret_val = pop_msginfo_node(&pmsg);
        CHECK_VALUE(ret_val);
    }

    return ret_val;
}

_int32 peek_operation_count_by_device_id(_u32 device_id, _u32 device_type, _u32 *count)
{
    _int32 ret_val = SUCCESS;

    *count = 0;
    if (device_type != DEVICE_SOCKET_TCP && device_type != DEVICE_SOCKET_UDP)
    {
        return NOT_IMPLEMENT;
    }

    /* handle all new-msg to keep op-count just posted in a same call-stack */
    ret_val = handle_all_newmsgs();
    CHECK_VALUE(ret_val);

    ret_val = peek_op_count(device_id, count);
    CHECK_VALUE(ret_val);

    LOG_DEBUG("peek operation of socket(%d) is %d", device_id, *count);
    return ret_val;
}

BOOL asyn_frame_is_suspend()
{
    LOG_DEBUG("asyn_frame_is_suspend");
    return thread_is_suspend(&g_asyn_frame_suspend_data);
}

_int32 asyn_frame_suspend()
{
    LOG_DEBUG("asyn_frame_suspend");
    return thread_do_suspend(&g_asyn_frame_suspend_data);
}

_int32 asyn_frame_resume()
{
    LOG_DEBUG("asyn_frame_resume");
    return thread_do_resume(&g_asyn_frame_suspend_data);
}

