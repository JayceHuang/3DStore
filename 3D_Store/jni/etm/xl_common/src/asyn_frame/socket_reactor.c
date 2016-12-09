#ifdef LINUX
#include <errno.h>
#endif

#ifdef SET_PRIORITY
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include "asyn_frame/socket_reactor.h"
#include "platform/sd_task.h"
#include "utility/map.h"
#include "utility/list.h"
#include "utility/mempool.h"
#include "utility/thread.h"
#include "utility/string.h"
#include "utility/utility.h"
#include "asyn_frame/device_handler.h"
#include "asyn_frame/selector.h"
#include "asyn_frame/timer.h"

/* log */
#define LOGID LOGID_SOCKET_REACTOR
#include "utility/logger.h"


#if defined( WINCE)
#include "platform/wm_interface/sd_wm_network_interface.h"
#elif defined(_ANDROID_LINUX)
#include "platform/android_interface/sd_android_network_interface.h"
#endif

static THREAD_STATUS g_socket_reactor_thread_status = INIT;

static _int32 g_socket_reactor_thread_id = 0;

/* the first member must be _fd, used for @FIND_NODE@ later*/
typedef struct
{
    _int32 _fd;
    _int32 _attachment_count;
    void *_write_list;
    void *_read_list;
    void *_write_list_tail;
    void *_read_list_tail;
    LIST _op_list; /* used for cancel socket-fd, do it simply for short-time, will optimaze in future */
} SOCKET_NODE;

static DEVICE_REACTOR g_socket_reactor;
static SET g_socket_set;

static SLAB *gp_socket_set_slab = NULL;

static _int32 g_socket_wait_handle = -1;

/* thread error handler */
static _int32 g_socket_reactor_error_exitcode = 0;
static void socket_handler_error_exit() 
{
    LOG_ERROR("****** ERROR OCCUR in socket-reactor thread(%u): %d !!!!", sd_get_self_taskid(), g_socket_reactor_error_exitcode);
    set_critical_error(g_socket_reactor_error_exitcode);
    finished_thread(&g_socket_reactor_thread_status);
}

#define EXIT(code)  {g_socket_reactor_error_exitcode = code; sd_assert(code==SUCCESS); socket_handler_error_exit();}


void socket_handler(void *args);


_int32 socket_node_comparator(void *E1, void *E2)
{
    return ((SOCKET_NODE*)E1)->_fd - ((SOCKET_NODE*)E2)->_fd;
}

static _int32 socket_reactor_timer_node_comparator(void *m1, void *m2)
{
    return (_int32)(((MSG*)m1)->_msg_id - ((MSG*)m2)->_msg_id);
}

static void init_socket_node(SOCKET_NODE *pnode)
{
    sd_memset(pnode, 0, sizeof(SOCKET_NODE));
    pnode->_read_list = pnode;
    pnode->_write_list = pnode;
    pnode->_read_list_tail = pnode;
    pnode->_write_list_tail = pnode;
}

static BOOL is_empty_socket_node_read_list(SOCKET_NODE *pnode)
{
    return (pnode->_read_list == pnode);
}

static BOOL is_empty_socket_node_write_list(SOCKET_NODE *pnode)
{
    return pnode->_write_list == pnode;
}

static void delete_socket_node(SOCKET_NODE *pnode)
{
    if (pnode && is_empty_socket_node_read_list(pnode) && is_empty_socket_node_write_list(pnode) && (pnode->_attachment_count == 0))
    {
        set_erase_node(&g_socket_set, (pnode));
         list_clear(&pnode->_op_list);
        mpool_free_slip(gp_socket_set_slab, (pnode));
        pnode = NULL;
    }
}

static void add_socket_node_read_list(SOCKET_NODE *pnode, MSG *pmsg)
{
    if (is_empty_socket_node_read_list(pnode))
    {
        pnode->_read_list = pmsg;
        pnode->_read_list_tail = pmsg;
    }
    else
    {
        ((MSG*)(pnode->_read_list_tail))->_inner_data = pmsg;
        pnode->_read_list_tail = pmsg;
    }
}

static void delete_socket_node_read_list(SOCKET_NODE *pnode)
{
    if (FALSE == is_empty_socket_node_read_list(pnode))
    {
        pnode->_read_list = ((MSG*)(pnode->_read_list))->_inner_data;
    }
}

static void add_socket_node_write_list(SOCKET_NODE *pnode, MSG *pmsg)
{
    if (is_empty_socket_node_write_list(pnode))
    {
        pnode->_write_list = pmsg;
        pnode->_write_list_tail = pmsg;
    }
    else
    {
        ((MSG*)(pnode->_write_list_tail))->_inner_data = pmsg;
        pnode->_write_list_tail = pmsg;
    }
}

static void delete_socket_node_write_list(SOCKET_NODE *pnode)
{
    if (FALSE == is_empty_socket_node_write_list(pnode))
    {
        pnode->_write_list = ((MSG*)(pnode->_write_list))->_inner_data;
    }
}

static MSG* read_msg_from_socket_node_read_list(SOCKET_NODE *pnode)
{
    return (MSG*)(pnode->_read_list);
}

static MSG* read_msg_from_socket_node_write_list(SOCKET_NODE *pnode)
{
    return (MSG*)(pnode->_write_list);
}

#ifdef _WSA_EVENT_SELECT
#define MAX_EPOLL_EVENT (64)
#else
#define MAX_EPOLL_EVENT (256)
#endif

static void *gp_socket_selector = NULL;

_int32 init_socket_reactor(_int32 *waitable_handle)
{
    _int32 ret_val = device_reactor_init(&g_socket_reactor);
    CHECK_VALUE(ret_val);

    if (waitable_handle)
    {
        *waitable_handle = g_socket_reactor._out_queue._waitable_handle;
    }

    ret_val = mpool_create_slab(sizeof(SOCKET_NODE), MIN_SOCKET_COUNT, 0, &gp_socket_set_slab);
    CHECK_VALUE(ret_val);

    set_init(&g_socket_set, socket_node_comparator);

    ret_val = create_selector(MAX_EPOLL_EVENT, &gp_socket_selector);
    CHECK_VALUE(ret_val);

    g_socket_wait_handle = g_socket_reactor._in_queue._waitable_handle;
    CHANNEL_DATA channel_data;
    channel_data._fd = g_socket_wait_handle;
#ifdef WINCE
    ret_val = add_a_notice_channel(gp_socket_selector, g_socket_wait_handle, channel_data);
#else
    ret_val = add_a_channel(gp_socket_selector, g_socket_wait_handle, CHANNEL_READ, channel_data);
#endif
    CHECK_VALUE(ret_val);

    g_socket_reactor_thread_status = INIT;
    ret_val = sd_create_task(socket_handler, 0, NULL, &g_socket_reactor_thread_id);
    CHECK_VALUE(ret_val);
    while (INIT == g_socket_reactor_thread_status)
    {
        sd_sleep(20);
    }

    if (FALSE == IS_THREAD_RUN(g_socket_reactor_thread_status))
	{
		ret_val = sd_finish_task(g_socket_reactor_thread_id);
		g_socket_reactor_thread_id = 0;
		CHECK_VALUE(ret_val);
		sd_exit(ret_val);
	}

    return ret_val;
}

_int32 uninit_socket_reactor(void)
{
    _int32 ret_val = SUCCESS;

    STOP_THREAD(g_socket_reactor_thread_status);

    /* pop all remain msg */
    MSG *pmsg;
    do
    {
        /* mem leak */
        ret_val = pop_complete_event(&g_socket_reactor, &pmsg);
        CHECK_VALUE(ret_val);
    }
    while(pmsg);

    wait_thread(&g_socket_reactor_thread_status, g_socket_reactor._in_queue._notice_handle);

    sd_finish_task(g_socket_reactor_thread_id);

    if (0 != set_size(&g_socket_set))
    {
        SET_ITERATOR socket_it = SET_BEGIN(g_socket_set);
        for (; socket_it != SET_END(g_socket_set); socket_it = SET_NEXT(g_socket_set, socket_it))
        {
            sd_close_socket(((SOCKET_NODE*)socket_it->_data)->_fd);
        }

        set_clear(&g_socket_set);
    }

    ret_val = destory_selector(gp_socket_selector);
    CHECK_VALUE(ret_val);

    ret_val = mpool_destory_slab(gp_socket_set_slab);
    CHECK_VALUE(ret_val);
    gp_socket_set_slab = NULL;

    ret_val = device_reactor_uninit(&g_socket_reactor);
    CHECK_VALUE(ret_val);

    return ret_val;
}

_int32 register_socket_event(MSG *msg)
{
    _int32 ret_val = check_register(msg);
    CHECK_VALUE(ret_val);

    SOCKET_NODE *result_node = NULL;
    SOCKET_NODE find_node;
    find_node._fd = msg->_msg_info._device_id;
    set_find_node(&g_socket_set, &find_node, (void**)&result_node);

    if (NULL == result_node)
    {
        /* insert a new node */
        ret_val = mpool_get_slip(gp_socket_set_slab, (void**)&result_node);
        CHECK_VALUE(ret_val);

        init_socket_node(result_node);
        result_node->_fd = msg->_msg_info._device_id;

        /* init op_list */
        list_init(&result_node->_op_list);

        ret_val = set_insert_node(&g_socket_set, result_node);
    }

    sd_assert(result_node!=NULL);
    result_node->_attachment_count ++;
    msg->_inner_data = result_node;

    /* insert msg into op_list */
    ret_val = list_push(&result_node->_op_list, msg);
    CHECK_VALUE(ret_val);

    ret_val = register_event(&g_socket_reactor, msg, NULL);
    /* when failed to push queue(NOT failed to notice), reset the status. */
    if (SUCCESS != ret_val)
    {
        result_node->_attachment_count --;
        msg->_inner_data = NULL;
        delete_socket_node(result_node);
        LOG_DEBUG("fail to register socket-event(op:%d, fd:%d) of msg(id:%d) status(%d) msg addr:0x%x ret(%d)", msg->_msg_info._operation_type, msg->_msg_info._device_id, msg->_msg_id, msg->_cur_reactor_status, (void*)msg, ret_val);
        return ret_val;
    }
    
    LOG_DEBUG("queue_capacity(sock_reactor.in_queue)   size:%d, capacity:%d, actual_capacity:%d",
              QINT_VALUE(g_socket_reactor._in_queue._data_queue._queue_size),
              QINT_VALUE(g_socket_reactor._in_queue._data_queue._queue_capacity),
              QINT_VALUE(g_socket_reactor._in_queue._data_queue._queue_actual_capacity));

    LOG_DEBUG("queue_capacity(sock_reactor.out_queue)   size:%d, capacity:%d, actual_capacity:%d",
              QINT_VALUE(g_socket_reactor._out_queue._data_queue._queue_size),
              QINT_VALUE(g_socket_reactor._out_queue._data_queue._queue_capacity),
              QINT_VALUE(g_socket_reactor._out_queue._data_queue._queue_actual_capacity));

    LOG_DEBUG("success register socket-event(op:%d, fd:%d) of msg(id:%d) status(%d) msg addr:0x%x", msg->_msg_info._operation_type, msg->_msg_info._device_id, msg->_msg_id, msg->_cur_reactor_status, (void*)msg);

    return ret_val;
}

_int32 unregister_socket(MSG *msg, _int32 reason)
{
    _int32 ret_val = check_unregister(msg);
    CHECK_VALUE(ret_val);

    SOCKET_NODE *result_node = NULL;
    SOCKET_NODE find_node;
    find_node._fd = msg->_msg_info._device_id;
    set_find_node(&g_socket_set, &find_node, (void**)&result_node);

    if (NULL == result_node)
    {
#ifdef KANKAN_PROJ
        /* iPad看看3.11 频繁出现该错误,暂时山寨一下,稍后仔细查找原因 */
        URGENT_TO_FILE("unregister_socket:REACTOR_LOGIC_ERROR_1,device_id=%u,device_type=0x%X,operation_type=0x%X,pending_op_count=%u",msg->_msg_info._device_id,msg->_msg_info._device_type,msg->_msg_info._operation_type,msg->_msg_info._pending_op_count);
        PRINT_STRACE_TO_FILE;
        return SUCCESS;
#else
        return REACTOR_LOGIC_ERROR_1;
#endif
    }

    result_node->_attachment_count ++;

    ret_val = unregister_event(&g_socket_reactor, msg, reason);
    /* when failed to push queue(NOT failed to notice), reset the status. */
    if (SUCCESS != ret_val)
    {
        result_node->_attachment_count --;
        delete_socket_node(result_node);
        return ret_val;
    }

    LOG_DEBUG("success unregister socket-event(op:%d, fd:%d) of msg(id:%d) because reason(%d)", msg->_msg_info._operation_type, msg->_msg_info._device_id, msg->_msg_id, reason);

    return ret_val;
}

_int32 get_complete_socket_msg(MSG **msg)
{
    *msg = NULL;
    _int32 ret_val = pop_complete_event(&g_socket_reactor, msg);
	if (NULL == (*msg))
	{
		return ret_val;
	}
    SOCKET_NODE *psock_node = (SOCKET_NODE*)((*msg)->_inner_data);
    
#ifdef MACOS
    while (NULL == psock_node)
    {
        ret_val = pop_complete_event(&g_socket_reactor, msg);
		if (NULL == (*msg))
		{
			return ret_val;
		}
		psock_node = (SOCKET_NODE*)((*msg)->_inner_data);
    }
#endif

    if (NULL == psock_node)
    {
        sd_assert(FALSE);
        return REACTOR_LOGIC_ERROR_5;
    }
    
    psock_node->_attachment_count --;

    if ((*msg)->_op_count == 0)
    {
        LIST_ITERATOR list_it = LIST_BEGIN(psock_node->_op_list);
        for (; list_it != LIST_END(psock_node->_op_list); list_it = LIST_NEXT(list_it))
        {
            if (LIST_VALUE(list_it) == *msg)
            {
                list_erase(&psock_node->_op_list, list_it);
                break;
            }
        }
    }

    /* pending operation count */
    (*msg)->_msg_info._pending_op_count = list_size(&psock_node->_op_list);

    LOG_DEBUG("get a completed op(%d) of msg(id:%d) about socket(fd:%d, pending_op:%d), and msginfo(reactor_status:%d, op_err:%d, op_left:%d)",
              (*msg)->_msg_info._operation_type, (*msg)->_msg_id, (*msg)->_msg_info._device_id, (*msg)->_msg_info._pending_op_count, (*msg)->_cur_reactor_status, (*msg)->_op_errcode, (*msg)->_op_count);

    delete_socket_node(psock_node);

    return SUCCESS;
}

/* Return      TRUE: find   FALSE: not found */
static BOOL erase_msg_from_list(void **list_head, void **list_tail, MSG *pmsg, SOCKET_NODE *pnode)
{
    BOOL ret_val = FALSE;

    MSG *ploopmsg = NULL;
    sd_assert(pnode!=NULL);
    sd_assert(pmsg->_inner_data!=NULL);
    if(*list_head == pmsg)
    {
        *list_head = (void*)pmsg->_inner_data;
        if(pmsg->_inner_data == pnode)
            *list_tail = pnode;

        pmsg->_inner_data = pnode;
        ret_val = TRUE;
    }
    else
    {
        for(ploopmsg = *list_head; (void*)ploopmsg->_inner_data != pnode; ploopmsg = (void*)ploopmsg->_inner_data)
        {
            if(ploopmsg->_inner_data == pmsg)
            {
                ploopmsg->_inner_data = pmsg->_inner_data;
                if(pmsg->_inner_data == pnode)
                    *list_tail = ploopmsg;

                pmsg->_inner_data = pnode;

                ret_val = TRUE;
                break;
            }
        }
    }
    sd_assert(pmsg->_inner_data!=NULL);
    return ret_val;
}


static _int32 handle_socket_node(SOCKET_NODE *pnode, _int32 idx, BOOL modify_channel)
{
    _int32 ret_val = SUCCESS;

    _u32 event = 0;
    if (!is_empty_socket_node_read_list(pnode))
    {
        event |= CHANNEL_READ;
    }
    if (!is_empty_socket_node_write_list(pnode))
    {
        event |= CHANNEL_WRITE;
    }
    LOG_DEBUG("ready to change event to %d of socket(%d)", event, pnode->_fd);
    

    if (event == 0)
    {
        ret_val = del_a_channel(gp_socket_selector, idx, pnode->_fd);
    }
    else
    {
        CHANNEL_DATA data;
        data._ptr = pnode;
        if (modify_channel)
        {
            ret_val = modify_a_channel(gp_socket_selector, idx, pnode->_fd, event, data);
        }
        else
        {
            ret_val = add_a_channel(gp_socket_selector, pnode->_fd, event, data);
        }
    }

    /* Amlogic 避免线程退出，特别添加的 */
    if (ret_val == BAD_POLL_ARUMENT)
    {
        ret_val = SUCCESS;
    }
    
    return ret_val;
}

static _int32 socket_handler_with_channel_error(SOCKET_NODE *pnode, _int32 channel_idx)
{    
    LOG_DEBUG("a common socket event in is_channel_error coming (fd:%d), read_list:0x%x, read_list_tail:0x%x, write_list:0x%x, write_list_tail:0x%x", pnode->_fd,  (void*)pnode->_read_list, (void*)pnode->_read_list_tail, (void*)pnode->_write_list, (void*)pnode->_write_list_tail);

    if (is_empty_socket_node_read_list(pnode) && is_empty_socket_node_write_list(pnode))
    {
        return REACTOR_LOGIC_ERROR_2;
    }

    _int32 ret_val = SUCCESS;
    _int32 handle_errno = get_channel_error(gp_socket_selector, channel_idx);

    _int32 read_handle_errno = handle_errno;
    _int32 write_handle_errno = handle_errno;

    MSG *read_pmsg = NULL;
    if (FALSE == is_empty_socket_node_read_list(pnode))
    {
        read_pmsg = read_msg_from_socket_node_read_list(pnode);
        delete_socket_node_read_list(pnode);
        LOG_DEBUG("remove op(%d) of socket(%d) about msg(id:%d) from read-list because of an error", 
            read_pmsg->_msg_info._operation_type, read_pmsg->_msg_info._device_id, read_pmsg->_msg_id);

        if (SUCCESS == read_handle_errno)
        {
            read_handle_errno = get_socket_error(read_pmsg->_msg_info._device_id);
            if (SUCCESS == read_handle_errno)
            {
                /*  socket 无错误,有可能是对方发完数据后马上把连接断开了(如http短连接) ,这种情况需要把数据读出来*/
                BOOL completed = FALSE;
                read_handle_errno = handle_msg(read_pmsg, &completed);
                sd_assert(completed == TRUE);
                if ((completed == FALSE) && (read_handle_errno == SUCCESS))
                {
                    read_handle_errno = SOCKET_CLOSED;
                }
            }
            LOG_DEBUG("socket(%d) about msg(id:%d) error: %d",read_pmsg->_msg_info._device_id, read_pmsg->_msg_id, read_handle_errno);
        }
    }

    MSG *write_pmsg = NULL;
    if (FALSE == is_empty_socket_node_write_list(pnode))
    {
        write_pmsg = read_msg_from_socket_node_write_list(pnode);
        delete_socket_node_write_list(pnode);
        LOG_DEBUG("remove op(%d) of socket(%d) about msg(id:%d) from write-list because of an error",
            write_pmsg->_msg_info._operation_type, write_pmsg->_msg_info._device_id, write_pmsg->_msg_id);

        if (SUCCESS == write_handle_errno)
        {
            write_handle_errno = get_socket_error(write_pmsg->_msg_info._device_id);
            if (SUCCESS == write_handle_errno)
            {
                write_handle_errno = SOCKET_CLOSED;
            }
            LOG_DEBUG("socket(%d) about msg(id:%d) error: %d",write_pmsg->_msg_info._device_id, write_pmsg->_msg_id, write_handle_errno);
        }
    }

    ret_val = handle_socket_node(pnode, channel_idx, TRUE);
    CHECK_THREAD_VALUE(ret_val);

    if (NULL != read_pmsg)
    {
        read_pmsg->_inner_data = pnode;
        sd_assert(read_pmsg->_inner_data != NULL);
        ret_val = notice_complete_event(&g_socket_reactor, (_u16)read_handle_errno, read_pmsg);
        CHECK_THREAD_VALUE(ret_val);
    }

    if (NULL != write_pmsg)
    {
        write_pmsg->_inner_data = pnode;
        sd_assert(write_pmsg->_inner_data != NULL);
        ret_val = notice_complete_event(&g_socket_reactor, (_u16)write_handle_errno, write_pmsg);
        CHECK_THREAD_VALUE(ret_val);
    }

    return ret_val;
}

static _int32 socket_handler_with_channel_read(SOCKET_NODE *pnode, _int32 channel_idx)
{
    _int32 ret_val = SUCCESS;
    if (is_channel_readable(gp_socket_selector, channel_idx))/* readable */
    {
        LOG_DEBUG("a common socket in is_channel_readable event coming (fd:%d), read_list:0x%x, read_list_tail:0x%x, write_list:0x%x, write_list_tail:0x%x", 
            pnode->_fd,  (void*)pnode->_read_list, (void*)pnode->_read_list_tail, (void*)pnode->_write_list, (void*)pnode->_write_list_tail);
        if (is_empty_socket_node_read_list(pnode))
        {
            return REACTOR_LOGIC_ERROR_2;
        }
        BOOL completed = FALSE;
        MSG *pmsg = read_msg_from_socket_node_read_list(pnode);
        _int32 handle_errno = handle_msg(pmsg, &completed);
        LOG_DEBUG("handle socket event read op(%d) on socket(%d) about msg(id:%d) errno(%d)",
            pmsg->_msg_info._operation_type, pmsg->_msg_info._device_id, pmsg->_msg_id, handle_errno);

        if (completed || REACTOR_UNREGISTERED(pmsg->_cur_reactor_status))
        {
            LOG_DEBUG("remove op(%d) of socket(%d) about msg(id:%d)(cur_status:%d) from read-list since completed or ungistered",
                pmsg->_msg_info._operation_type, pmsg->_msg_info._device_id, pmsg->_msg_id, pmsg->_cur_reactor_status);
            delete_socket_node_read_list(pnode);

            ret_val = handle_socket_node(pnode, channel_idx, TRUE);
            CHECK_THREAD_VALUE(ret_val);

            pmsg->_inner_data = pnode;
            sd_assert(pmsg->_inner_data != NULL);
            ret_val = notice_complete_event(&g_socket_reactor, (_u16)handle_errno, pmsg);
            CHECK_THREAD_VALUE(ret_val);
        }
    }

    return ret_val;
}

static _int32 socket_handler_with_channel_write(SOCKET_NODE *pnode, _int32 channel_idx)
{
    _int32 ret_val = SUCCESS;
    if (is_channel_writable(gp_socket_selector, channel_idx))/* writable */
    {
        LOG_DEBUG("a common socket event in is_channel_writable coming (fd:%d), read_list:0x%x, read_list_tail:0x%x, write_list:0x%x, write_list_tail:0x%x",
            pnode->_fd,  (void*)pnode->_read_list, (void*)pnode->_read_list_tail, (void*)pnode->_write_list, (void*)pnode->_write_list_tail);

        if (is_empty_socket_node_write_list(pnode))
        {
            return REACTOR_LOGIC_ERROR_2;
        }
        /* distinguish the status of connect(first connect or connecting) */
        BOOL completed = FALSE;
        MSG *pmsg = read_msg_from_socket_node_write_list(pnode);
        _int32 handle_errno = handle_msg(pmsg, &completed);
        LOG_DEBUG("handle socket event write op(%d) on socket(%d) about msg(id:%d) errno(%d)",
            pmsg->_msg_info._operation_type, pmsg->_msg_info._device_id, pmsg->_msg_id, handle_errno);

        if (completed || REACTOR_UNREGISTERED(pmsg->_cur_reactor_status))
        {
            LOG_DEBUG("remove op(%d) of socket(%d) about msg(id:%d)(cur_status:%d) from write-list since completed or ungistered",
                pmsg->_msg_info._operation_type, pmsg->_msg_info._device_id, pmsg->_msg_id, pmsg->_cur_reactor_status);
            delete_socket_node_write_list(pnode);
            ret_val = handle_socket_node(pnode, channel_idx, TRUE);
            CHECK_THREAD_VALUE(ret_val);
            pmsg->_inner_data = pnode;
            sd_assert(pmsg->_inner_data != NULL);
            ret_val = notice_complete_event(&g_socket_reactor, (_u16)handle_errno, pmsg);
            CHECK_THREAD_VALUE(ret_val);
        }
    }
    return ret_val;
}

static _int32 socket_handle_read_op_register(MSG *pmsg, SOCKET_NODE *pnode, BOOL is_fd_exist)
{
    _int32 ret_val =SUCCESS;
    BOOL completed = FALSE;
    _int32 handle_errno = SUCCESS;

    if (is_empty_socket_node_read_list(pnode))
    {
        LOG_DEBUG("try to read of the msg(id:%d) since it is a first one", pmsg->_msg_id);
        completed = TRUE; /* distinguish the status of connect(first connect or connecting) */
        handle_errno = handle_msg(pmsg, &completed); /* try to handle */
    }

    if (completed || REACTOR_UNREGISTERED(pmsg->_cur_reactor_status))
    {
        LOG_DEBUG("succeed in handling(or ungistered) of socket(%d) about msg(id:%d)(cur_status:%d) with errcode(%d) immediately",
            pmsg->_msg_info._operation_type, pmsg->_msg_info._device_id, pmsg->_msg_id, pmsg->_cur_reactor_status, handle_errno);

        ret_val = notice_complete_event(&g_socket_reactor, (_u16)handle_errno, pmsg);
        CHECK_THREAD_VALUE(ret_val);
    }
    else
    {
        LOG_DEBUG("put op_count(%d) of socket(%d) about msg(id:%d) into read-list",
            pmsg->_op_count, pmsg->_msg_info._device_id, pmsg->_msg_id);

        add_socket_node_read_list(pnode, pmsg);
        ret_val = handle_socket_node(pnode, -1, is_fd_exist);
        CHECK_THREAD_VALUE(ret_val);
    }

    return ret_val;
}

static _int32 socket_handle_write_op_register(MSG *pmsg, SOCKET_NODE *pnode, BOOL is_fd_exist)
{
    _int32 ret_val = SUCCESS;
    BOOL completed = FALSE;
    _int32 handle_errno = SUCCESS;
    if (is_empty_socket_node_write_list(pnode))
    {
        LOG_DEBUG("try to write of the msg(id:%d) since it is a first one", pmsg->_msg_id);
        completed = TRUE; /* distinguish the status of connect(first connect or connecting) */
        handle_errno = handle_msg(pmsg, &completed); /* try to handle */
    }

    if (completed || REACTOR_UNREGISTERED(pmsg->_cur_reactor_status))
    {
        LOG_DEBUG("succeed in handling(or ungistered) of socket(%d) about msg(id:%d)(cur_status:%d) with errcode(%d) immediately 2",
            pmsg->_msg_info._operation_type, pmsg->_msg_info._device_id, pmsg->_msg_id, pmsg->_cur_reactor_status, handle_errno);

        ret_val = notice_complete_event(&g_socket_reactor, (_u16)handle_errno, pmsg);
        CHECK_THREAD_VALUE(ret_val);
    }
    else
    {
        LOG_DEBUG("put op(%d) of socket(%d) about msg(id:%d) into write-list",
            pmsg->_msg_info._device_id, pmsg->_msg_id);

        add_socket_node_write_list(pnode, pmsg);
        ret_val = handle_socket_node(pnode, -1, is_fd_exist);
        CHECK_THREAD_VALUE(ret_val);
    }

    return ret_val;
}

static _int32 socket_handler_register_msg(MSG *pmsg)
{
    _int32 ret_val = SUCCESS;
    _int32 op_type = pmsg->_msg_info._operation_type;
    sd_assert(pmsg->_inner_data != NULL);
    SOCKET_NODE *pnode = (SOCKET_NODE*)pmsg->_inner_data;
    LOG_DEBUG("get a register-notice of msg(id:%d) op_count(%d) op_type(%d)", pmsg->_msg_id, pmsg->_op_count, op_type);

    BOOL is_fd_exist = !(is_empty_socket_node_read_list(pnode) && is_empty_socket_node_write_list(pnode));
    if ((op_type == OP_ACCEPT) || (op_type == OP_RECV) || (op_type == OP_RECVFROM))
    {
        socket_handle_read_op_register(pmsg, pnode, is_fd_exist);
    }
    else if ((op_type == OP_SEND) || (op_type == OP_SENDTO) || (op_type == OP_CONNECT) || (op_type == OP_PROXY_CONNECT))
    {
        socket_handle_write_op_register(pmsg, pnode, is_fd_exist);
    }
    else
    {
        return REACTOR_LOGIC_ERROR_4;
    }

    return ret_val;
}

static _int32 socket_handler_read_op_unregister(MSG *pmsg, SOCKET_NODE *pnode)
{
    _int32 ret_val = SUCCESS;

    if (!is_empty_socket_node_read_list(pnode))
    {
        if (erase_msg_from_list((void**)&pnode->_read_list, (void **)&pnode->_read_list_tail, pmsg, (SOCKET_NODE*)pnode))
        {
            /* notice the msg about REGISTER-event */
            LOG_DEBUG("success remove of socket(%d) about msg(id:%d) from read-list since cancelled",
                pmsg->_msg_info._device_id, pmsg->_msg_id);

            if (is_empty_socket_node_read_list(pnode))
            {
                ret_val = handle_socket_node(pnode, -1, TRUE);
                CHECK_THREAD_VALUE(ret_val);
            }
            ret_val = notice_complete_event(&g_socket_reactor, REACTOR_E_UNREGISTER, pmsg);
            CHECK_THREAD_VALUE(ret_val);
        }
    }
    return ret_val;
}

static _int32 socket_handler_write_op_unregister(MSG *pmsg, SOCKET_NODE *pnode)
{
    _int32 ret_val = SUCCESS;
    if (!is_empty_socket_node_write_list(pnode))
    {
        if (erase_msg_from_list((void**)&pnode->_write_list, (void**)&pnode->_write_list_tail, pmsg, (SOCKET_NODE*)pnode))
        {
            /* notice the msg about REGISTER-event */
            LOG_DEBUG("success remove of socket(%d) about msg(id:%d) from write-list since cancelled",
                pmsg->_msg_info._device_id, pmsg->_msg_id);
            if (is_empty_socket_node_write_list(pnode))
            {
                ret_val = handle_socket_node(pnode, -1, TRUE);
                CHECK_THREAD_VALUE(ret_val);
            }
            ret_val = notice_complete_event(&g_socket_reactor, REACTOR_E_UNREGISTER, pmsg);
            CHECK_THREAD_VALUE(ret_val);
        }
    }
    return ret_val;
}

static _int32 socket_handler_unregister_msg(MSG *pmsg)
{
    _int32 ret_val = SUCCESS;

    MSG *ploopmsg = pmsg;
    /* @FIND_NODE@ find the pnode, the first member of SOCKET_NODE must be _fd and the first member of MSG is the handler(its value > 65536) */
    while (*((_u32*)ploopmsg->_inner_data) != ploopmsg->_msg_info._device_id)
    {
        ploopmsg = (MSG*)ploopmsg->_inner_data;
    }
    sd_assert(ploopmsg->_inner_data != NULL);
    SOCKET_NODE *pnode = (void*)ploopmsg->_inner_data;

    _int32 op_type = pmsg->_msg_info._operation_type;
    LOG_DEBUG("get a unregister-notice of msg(id:%d),op_count(%d), op_type(%d)", pmsg->_msg_id, pmsg->_op_count, op_type);

    /* erase this msg*/
    if ((op_type == OP_ACCEPT) || (op_type == OP_RECV) || (op_type == OP_RECVFROM))
    {
        socket_handler_read_op_unregister(pmsg, pnode);
    }
    else if ((op_type == OP_SEND) || (op_type == OP_SENDTO) || (op_type == OP_CONNECT) || (op_type == OP_PROXY_CONNECT))
    {
        socket_handler_write_op_unregister(pmsg, pnode);
    }
    else
    {
        return REACTOR_LOGIC_ERROR_4;
    }

    /* notice the msg about UNREGISTER-event */
    ret_val = notice_complete_event(&g_socket_reactor, REACTOR_E_UNREGISTER, pmsg);
    CHECK_THREAD_VALUE(ret_val);
    
    return ret_val;
}

static _int32 socket_handler_other_msg()
{
    MSG *pmsg = NULL;
    _int32 ret_val = SUCCESS;
    while (1)
    {
        ret_val = pop_register_event(&g_socket_reactor, &pmsg);
        CHECK_THREAD_VALUE(ret_val);
        if (pmsg && IS_THREAD_RUN(g_socket_reactor_thread_status))
        {
            LOG_DEBUG("REACTOR_LOGIC_ERROR_3 socket-event(op:%d, fd:%d) of msg(id:%d)  status=%d msg addr:0x%x", 
                pmsg->_msg_info._operation_type, pmsg->_msg_info._device_id, pmsg->_msg_id, pmsg->_cur_reactor_status, (void*)pmsg);
        }
        else
        {
            break;
        }
    };
    sd_assert(FALSE);

    return ret_val;
}

static _int32 socket_handler_selector_msg()
{
    _int32 ret_val = SUCCESS;
    MSG *pmsg = NULL;
    if (!is_channel_full(gp_socket_selector))
    {
        ret_val = pop_register_event(&g_socket_reactor, &pmsg);
        CHECK_THREAD_VALUE(ret_val);
    }

    while (pmsg && IS_THREAD_RUN(g_socket_reactor_thread_status))
    {
        LOG_DEBUG("get a notice of msg(id:%d) op_count(%d) status(%d) msg addr:0x%x", pmsg->_msg_id, pmsg->_op_count, pmsg->_cur_reactor_status, (void*)pmsg);
        if (REACTOR_REGISTERED(pmsg->_cur_reactor_status))
        {
            ret_val = socket_handler_register_msg(pmsg);
        }
        else if (REACTOR_UNREGISTERED(pmsg->_cur_reactor_status))
        {
            ret_val = socket_handler_unregister_msg(pmsg);
        }
        else
        {
            socket_handler_other_msg();
        }

        if (REACTOR_LOGIC_ERROR_4 == ret_val)
        {
            return ret_val;
        }

        pmsg = NULL;
        if (!is_channel_full(gp_socket_selector))
        {
            ret_val = pop_register_event(&g_socket_reactor, &pmsg);
            CHECK_THREAD_VALUE(ret_val);
        }
    }

    return ret_val;
}

static _int32 socket_handler_run()
{
    _int32 fd_count = selector_wait(gp_socket_selector, -1);

    _int32 channel_idx = CHANNEL_IDX_FIND_BEGIN;
    _int32 index = 0;
    _int32 ret_val = SUCCESS;
    /* handle the event */
    for (; index < fd_count && IS_THREAD_RUN(g_socket_reactor_thread_status); index++)
    {
        ret_val = get_next_channel(gp_socket_selector, &channel_idx);
        if (ret_val == NOT_FOUND_POLL_EVENT)
        {
            LOG_DEBUG("the selector-idx dismatched with fd-count. channel_idx: %d, fd_count: %d, index_socket: %d", channel_idx, fd_count, index);
            break;
        }
        CHECK_THREAD_VALUE(ret_val);

        CHANNEL_DATA channel_data;
        ret_val = get_channel_data(gp_socket_selector, channel_idx, &channel_data);

        if (channel_data._fd == (_u32)g_socket_wait_handle)
        {
            /* the address of SOCKET_NODE will be bigger than fd(0-65536), so we can indentify the waitable_fd*/
            LOG_DEBUG("a notice event coming (%d/%d)", index, fd_count);
            ret_val = reset_notice(g_socket_wait_handle);
        }
        else
        {
            /* common sokcet event */
            LOG_DEBUG("a common socket event coming (%d/%d)", index, fd_count);
            SOCKET_NODE *pnode = channel_data._ptr;

            if (is_channel_error(gp_socket_selector, channel_idx))
            {
                ret_val = socket_handler_with_channel_error(pnode, channel_idx);
                if (REACTOR_LOGIC_ERROR_2 == ret_val)
                {
                    return REACTOR_LOGIC_ERROR_2;
                }
                continue;
            }

            if (REACTOR_LOGIC_ERROR_2 == socket_handler_with_channel_read(pnode, channel_idx))
            {
                return REACTOR_LOGIC_ERROR_2;
            }

            if (REACTOR_LOGIC_ERROR_2 == socket_handler_with_channel_write(pnode, channel_idx))
            {
                return REACTOR_LOGIC_ERROR_2;
            }
        }
    }

    if (REACTOR_LOGIC_ERROR_4 == socket_handler_selector_msg())
    {
        return REACTOR_LOGIC_ERROR_4;
    }

    return ret_val;
}

void socket_handler(void *args)
{
    sd_ignore_signal();
#ifdef SET_PRIORITY
    setpriority(PRIO_PROCESS, 0, 5);
#endif

    RUN_THREAD(g_socket_reactor_thread_status);
    
    BEGIN_THREAD_LOOP(g_socket_reactor_thread_status)

    _int32 ret_val = socket_handler_run();
    if ((REACTOR_LOGIC_ERROR_2 == ret_val) || (REACTOR_LOGIC_ERROR_4 == ret_val))
    {
        EXIT(ret_val);
    }

    END_THREAD_LOOP(g_socket_reactor_thread_status);
    finished_thread(&g_socket_reactor_thread_status);
}

_int32 cancel_socket(_int32 socket_id)
{
    _int32 ret_val = SUCCESS;
    MSG *pmsg = NULL;
    SOCKET_NODE find_node, *result_node;

    /* ready for erase msg from op_list */
    LIST_ITERATOR list_it;

    find_node._fd = socket_id;
    set_find_node(&g_socket_set, &find_node, (void**)&result_node);

    if(result_node)
    {
        /* erase msg from op_list */
        for(list_it = LIST_BEGIN(result_node->_op_list)
            ; list_it != LIST_END(result_node->_op_list)
            ; list_it = LIST_NEXT(list_it) )
        {
            pmsg = LIST_VALUE(list_it);
            if(REACTOR_REGISTERED(pmsg->_cur_reactor_status))
            {
                /*cancel timer*/

                ret_val = erase_from_timer(pmsg, socket_reactor_timer_node_comparator
                    , pmsg->_timeout_index, NULL);
                LOG_DEBUG("cancel_socket, pmsg:0x%x, ret = %d, socket_id = %d, pmsg->_timeout_index = %d, pmsg->_msg_id = %d"
                    ,pmsg, ret_val, socket_id, pmsg->_timeout_index, pmsg->_msg_id);
                CHECK_VALUE(ret_val);

                ret_val = unregister_socket(pmsg, REACTOR_STATUS_CANCEL);
                CHECK_VALUE(ret_val);
            }
            else
            {
                sd_assert(REACTOR_UNREGISTERED(pmsg->_cur_reactor_status));

                pmsg->_cur_reactor_status = REACTOR_STATUS_CANCEL;
            }
        }
    }

    return ret_val;
}

_int32 peek_op_count(_int32 socket_id, _u32 *count)
{
    *count = 0;
    SOCKET_NODE *result_node = NULL;
    SOCKET_NODE find_node;
    find_node._fd = socket_id;
	set_find_node(&g_socket_set, &find_node, (void**)&result_node);
    if (result_node)
    {
        *count = list_size(&result_node->_op_list);
    }
    return SUCCESS;
}
