#include "utility/define.h"
#include "utility/time.h"
#include "asyn_frame/msg.h"

#if defined(LINUX) 
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#elif defined(WINCE)
struct  hostent {
	char    * h_name;           /* official name of host */
	char    * * h_aliases;  /* alias list */
	short   h_addrtype;             /* host address type */
	short   h_length;               /* length of address */
	char    * * h_addr_list; /* list of addresses */
#define h_addr  h_addr_list[0]          /* address, for backward compat */
};
#include "platform/sd_socket.h"
#endif

#include "asyn_frame/dns_reactor.h"
#include "asyn_frame/dns_parser.h"

#include "platform/sd_task.h"
#include "utility/utility.h"
#include "utility/thread.h"
#include "utility/errcode.h"

#ifdef SET_PRIORITY
#include <sys/time.h>
#include <sys/resource.h>
#endif

/* log */
#define LOGID LOGID_DNS_REACTOR
#include "utility/logger.h"

#include "asyn_frame/device.h"
#include "asyn_frame/device_reactor.h"

static DEVICE_REACTOR g_dns_reactor;
static _int32 g_dns_waitable_container = 0;

static THREAD_STATUS g_dns_reactor_thread_status;
static _int32 g_dns_reactor_thread_id;

/* thread error handler */
static _int32 g_dns_reactor_error_exitcode = 0;
static void dns_reactor_error_exit()
{
    LOG_ERROR("ERROR OCCUR in dns-reactor thread(%d): %u\n", sd_get_self_taskid(), g_dns_reactor_error_exitcode);
    set_critical_error(g_dns_reactor_error_exitcode);
    finished_thread(&g_dns_reactor_thread_status);
}
#define EXIT(code)	{g_dns_reactor_error_exitcode = code; dns_reactor_error_exit();}
static _int32 DNS_TMP_BUFFER_LEN = 1024;

void dns_handler(void *args);

#ifdef DNS_ASYNC
	static DNS_PARSER g_dns_parser;
#else
	static DNS_CACHE g_dns_cache;
#endif

_int32 init_dns_reactor(_int32 *waitable_handle)
{
	_int32 ret_val = device_reactor_init(&g_dns_reactor);
	CHECK_VALUE(ret_val);

	if (waitable_handle)
	{
	    *waitable_handle = g_dns_reactor._out_queue._waitable_handle;
	}

	ret_val = create_waitable_container(&g_dns_waitable_container);
	CHECK_VALUE(ret_val);

	ret_val = add_notice_handle(g_dns_waitable_container, g_dns_reactor._in_queue._waitable_handle);
	CHECK_VALUE(ret_val);

#ifdef DNS_ASYNC
    ret_val = dns_parser_init(&g_dns_parser);
    CHECK_VALUE(ret_val);
#else
    ret_val = dns_cache_init(&g_dns_cache);
#endif

    _int32 stack_size = 256*1024;
#ifdef WINCE
    stack_size = 32*1024;
#endif

    g_dns_reactor_thread_status = INIT;
    ret_val = sd_create_task(dns_handler, stack_size, NULL, &g_dns_reactor_thread_id);
    CHECK_VALUE(ret_val);
    /* wait for thread running */
    while (g_dns_reactor_thread_status == INIT)
    {
        sd_sleep(20);
    }

    if (FALSE == IS_THREAD_RUN(g_dns_reactor_thread_status))
	{
		ret_val = sd_finish_task(g_dns_reactor_thread_id);
		g_dns_reactor_thread_id = 0;
		CHECK_VALUE(ret_val);
		sd_exit(ret_val);
	}
	
	return SUCCESS;
}

_int32 uninit_dns_reactor(void)
{
	_int32 ret_val = SUCCESS;

	STOP_THREAD(g_dns_reactor_thread_status);
	
	MSG *pmsg = NULL;
	do /* pop all remain msg */
	{
		ret_val = pop_complete_event(&g_dns_reactor, &pmsg);
		CHECK_VALUE(ret_val);
	} while(pmsg);

	wait_thread(&g_dns_reactor_thread_status, g_dns_reactor._in_queue._notice_handle);
	sd_finish_task(g_dns_reactor_thread_id);
	g_dns_reactor_thread_id = 0;

	/* destory other variables */
	ret_val = destory_waitable_container(g_dns_waitable_container);
	CHECK_VALUE(ret_val);

	ret_val = device_reactor_uninit(&g_dns_reactor);
	CHECK_VALUE(ret_val);

#ifdef DNS_ASYNC
    ret_val = dns_parser_uninit(&g_dns_parser);
    CHECK_VALUE(ret_val);
#else
    ret_val = dns_cache_uninit(&g_dns_cache);
#endif
    return SUCCESS;
}

_int32 register_dns_event(MSG *msg)
{
	_int32 ret_val = register_event(&g_dns_reactor, msg, (void*)&msg->_inner_data);

	LOG_DEBUG("queue_capacity(dns_reactor.in_queue)   size:%d, capacity:%d, actual_capacity:%d",
		QINT_VALUE(g_dns_reactor._in_queue._data_queue._queue_size), 
		QINT_VALUE(g_dns_reactor._in_queue._data_queue._queue_capacity),
		QINT_VALUE(g_dns_reactor._in_queue._data_queue._queue_actual_capacity));

	LOG_DEBUG("queue_capacity(dns_reactor.out_queue)   size:%d, capacity:%d, actual_capacity:%d",
		QINT_VALUE(g_dns_reactor._out_queue._data_queue._queue_size), 
		QINT_VALUE(g_dns_reactor._out_queue._data_queue._queue_capacity),
		QINT_VALUE(g_dns_reactor._out_queue._data_queue._queue_actual_capacity));

	return ret_val;
}

_int32 unregister_dns(MSG *msg, _int32 reason)
{
    return unregister_event_immediately(&g_dns_reactor, msg, reason, (void**)msg->_inner_data);
}

_int32 get_complete_dns_msg(MSG **msg)
{
	_int32 ret_val = pop_complete_event(&g_dns_reactor, msg);
	CHECK_VALUE(ret_val);
	if (*msg)
	{
	    LOG_DEBUG("get_complete_dns_msg(id:%d)",(*msg)->_msg_id);
	}
	return ret_val;
}

#ifdef DNS_ASYNC
static _int32 dns_handler_async_run()
{
    _int32 noticed_handle = 0;
    _int32 ret_val = wait_for_notice(g_dns_waitable_container, 1, &noticed_handle, WAIT_INFINITE);
    CHECK_THREAD_VALUE(ret_val);

    ret_val = reset_notice(noticed_handle);
    CHECK_THREAD_VALUE(ret_val);

    /* handle this msg */
    while (IS_THREAD_RUN(g_dns_reactor_thread_status))
    {
        _int32 handle_errno = 0;
        _u32 ip_count = 0;
        OP_PARA_DNS *op_dns = NULL;
        _int32 query_result = 0;
        _int32 ack_result = 0;
        DNS_CONTENT_PACKAGE dns_content_package;
        _u32 query_count = 0;
        DNS_REQUEST_RECORD *precord = NULL;
        BOOL is_finished = FALSE;
    
        BOOL is_ready = FALSE;
        MSG *pmsg = NULL;
        /* is ready? */
        ret_val = dns_parser_is_ready(&g_dns_parser, &is_ready);
        CHECK_THREAD_VALUE(ret_val);

        if (is_ready)
        {
            ret_val = pop_register_event_with_lock(&g_dns_reactor, &pmsg);
            CHECK_THREAD_VALUE(ret_val);
        }
        else
        {
            pmsg = NULL;
            sd_sleep(20);
        }

        if (pmsg != NULL)
        {
            if (!REACTOR_REGISTERED(pmsg->_cur_reactor_status))
            {
                LOG_DEBUG("get a unregister-msg(id:%d) in thread %u(dns reactor), and its status(%d)",
                    pmsg->_msg_id, sd_get_self_taskid(), pmsg->_cur_reactor_status);

                handle_errno = DNS_INVALID_REQUEST;
                ret_val = notice_complete_event(&g_dns_reactor, (_u16)handle_errno, pmsg);
                CHECK_THREAD_VALUE(ret_val);
            }
            else
            {
                op_dns = pmsg->_msg_info._operation_parameter;
                ret_val = dns_parser_query(&g_dns_parser, op_dns->_host_name, op_dns->_host_name_buffer_len,
                    &query_result, (void *)pmsg, &dns_content_package);
                CHECK_THREAD_VALUE(ret_val);

                if (query_result == DNS_QUERY_RESULT_OK)
                {
                    LOG_DEBUG("msg(id:%d) call dns_parser_query (ok). %s, result=%d(%s), pmsg=0x%x", 
                        pmsg->_msg_id,op_dns->_host_name, dns_content_package._result,
                        ((dns_content_package._result == SUCCESS) ? "SUCCESS" :
                        (dns_content_package._result == DNS_NO_SERVER) ? "DNS_NO_SERVER" :
                        (dns_content_package._result == DNS_INVALID_ADDR) ? "DNS_INVALID_ADDR" :
                        (dns_content_package._result == DNS_NETWORK_EXCEPTION) ? "DNS_NETWORK_EXCEPTION" :
                        "UNKNOWN"), (_int32)pmsg);

                    handle_errno = dns_content_package._result;
                    ip_count = MIN(dns_content_package._ip_count, op_dns->_ip_count);
                    op_dns->_ip_count = ip_count;
                    _int32 i = 0;
                    for (; i < ip_count; i ++)
                    {
                        op_dns->_ip_list[i] = dns_content_package._ip_list[i];
                        LOG_DEBUG("msg(id:%d)  call dns_parser_query (ok, ip list): %s, ip_count=MIN[%d,%d], ip[%d]=0x%x",
                            pmsg->_msg_id,op_dns->_host_name, dns_content_package._ip_count, op_dns->_ip_count, i, op_dns->_ip_list[i]);
                    }

                    /* fill dns server ip. */
                    op_dns->_server_ip_count = 0;
                    dns_parser_dns_server_count(&g_dns_parser, &op_dns->_server_ip_count);
                    _int32 j = 0;
                    for (; j < op_dns->_server_ip_count; ++j)
                    {
                        dns_parser_dns_server_info(&g_dns_parser, j, &op_dns->_server_ip[j]);
                    }

                    ret_val = notice_complete_event(&g_dns_reactor, (_u16)handle_errno, pmsg);
                    CHECK_THREAD_VALUE(ret_val);
                    LOG_DEBUG("msg(id:%d)  dns_parser_query_complete!", pmsg->_msg_id);
                }
                else
                {
                    /* can not get the answer immediately */
                    LOG_DEBUG("call dns_parser_query (async_request). %s", op_dns->_host_name);
                    /* wait 20ms for dns server ack. */
                    sd_sleep(20);
                }
            }
        }
        else
        {
            /* wait 20ms  */
            sd_sleep(20);
        }

        /* read dns answer. */
        for (; IS_THREAD_RUN(g_dns_reactor_thread_status); )
        {
            ret_val = dns_parser_get(&g_dns_parser, &ack_result, (void **)&pmsg, &dns_content_package);
            CHECK_THREAD_VALUE(ret_val);

            if (DNS_ACK_RESULT_OK != ack_result)
            {
                break;
            }

            LOG_DEBUG("call dns_parser_get (ok). %s, result=%d(%s), pmsg=0x%x",
                dns_content_package._host_name, 
                dns_content_package._result,
                ((dns_content_package._result == SUCCESS) ? "SUCCESS" :
                (dns_content_package._result == DNS_NO_SERVER) ? "DNS_NO_SERVER" :
                (dns_content_package._result == DNS_INVALID_ADDR) ? "DNS_INVALID_ADDR" :
                (dns_content_package._result == DNS_NETWORK_EXCEPTION) ? "DNS_NETWORK_EXCEPTION" :
                "UNKNOWN"), (_int32)pmsg);

            op_dns = pmsg->_msg_info._operation_parameter;
            handle_errno = dns_content_package._result;
            ip_count = MIN(dns_content_package._ip_count, op_dns->_ip_count);
            op_dns->_ip_count = ip_count;
            _int32 i = 0;
            for(; i < ip_count; i ++)
            {
                op_dns->_ip_list[i] = dns_content_package._ip_list[i];
                LOG_DEBUG("call dns_parser_get (ok): %s, ip_count=MIN[%d,%d], ip[%d]=0x%x",
                    op_dns->_host_name, dns_content_package._ip_count, op_dns->_ip_count, i, op_dns->_ip_list[i]);
            }

            /* fill dns server ip. */
            op_dns->_server_ip_count = 0;
            dns_parser_dns_server_count(&g_dns_parser, &op_dns->_server_ip_count);
            _int32 j = 0;
            for (; j < op_dns->_server_ip_count; ++j)
            {
                dns_parser_dns_server_info(&g_dns_parser, j, &op_dns->_server_ip[j]);
            }

            ret_val = notice_complete_event(&g_dns_reactor, (_u16)handle_errno, pmsg);
            CHECK_THREAD_VALUE(ret_val);
        }

        /* remove the cancelled or timeout msg. */
        ret_val = dns_parser_request_count(&g_dns_parser, &query_count);
        CHECK_THREAD_VALUE(ret_val);

        _int32 i = query_count - 1;
        for (; (IS_THREAD_RUN(g_dns_reactor_thread_status)) && (i >= 0); --i)
        {
            ret_val = dns_parser_request_record_const(&g_dns_parser, i, &precord);
            CHECK_THREAD_VALUE(ret_val);

            pmsg = (MSG *)precord->_data;
            if (REACTOR_UNREGISTERED(pmsg->_cur_reactor_status))
            {
                /* remove from request queue. */
                ret_val = dns_parser_query_cancel(&g_dns_parser, i);
                CHECK_THREAD_VALUE(ret_val);

                handle_errno = DNS_INVALID_REQUEST;
                op_dns = pmsg->_msg_info._operation_parameter;
                op_dns->_ip_count = 0;

                /* fill dns server ip. */
                op_dns->_server_ip_count = 0;
                dns_parser_dns_server_count(&g_dns_parser, &op_dns->_server_ip_count);
                _int32 j = 0;
                for (; j < op_dns->_server_ip_count; ++j)
                {
                    dns_parser_dns_server_info(&g_dns_parser, j, &op_dns->_server_ip[j]);
                }

                ret_val = notice_complete_event(&g_dns_reactor, (_u16)handle_errno, pmsg);
                CHECK_THREAD_VALUE(ret_val);
            }
        }

        /* request queue is empty? */
        ret_val = dns_parser_is_finished(&g_dns_parser, &is_finished);
        CHECK_THREAD_VALUE(ret_val);

        if (is_finished)
        {
            break;
        }
    }

    return ret_val;
}
#else
static _int32 dns_handler_sync_run()
{
    _int32 noticed_handle = 0;
	_int32 ret_val = wait_for_notice(g_dns_waitable_container, 1, &noticed_handle, WAIT_INFINITE);
	CHECK_THREAD_VALUE(ret_val);

	ret_val = reset_notice(noticed_handle);
	CHECK_THREAD_VALUE(ret_val);

    MSG *pmsg = NULL;
	ret_val = pop_register_event_with_lock(&g_dns_reactor, &pmsg);
	CHECK_THREAD_VALUE(ret_val);

	_int32 handle_errno = 0;
   
#if defined(LINUX)    
    struct addrinfo hints;
	struct addrinfo *result = NULL;
	struct addrinfo *rp = NULL;
	struct sockaddr_in *sin_p = NULL;
#endif   
	
	/* handle this msg */
	while (pmsg && IS_THREAD_RUN(g_dns_reactor_thread_status))
	{
    	_int32 ip_count = 0;
	    if (REACTOR_REGISTERED(pmsg->_cur_reactor_status))
	    {
	        /* implement simplely now */
	        OP_PARA_DNS *op_dns = pmsg->_msg_info._operation_parameter;
	        DNS_CONTENT_PACKAGE dns_content;
	        /* cache */
	        ret_val = dns_cache_query(&g_dns_cache, op_dns->_host_name, &dns_content);
	        if (ret_val == SUCCESS)
	        {
	            handle_errno = SUCCESS;
	            ip_count = MIN(dns_content._ip_count, op_dns->_ip_count);
	            op_dns->_ip_count = ip_count;
	            _int32 index = 0;
	            for (index = 0; index < ip_count; index ++)
	            {
	                op_dns->_ip_list[index] = dns_content._ip_list[index];
	            }
	        }
	        else
	        {
	            sd_memset(&dns_content, 0, sizeof(DNS_CONTENT_PACKAGE));
	            dns_content._host_name_buffer_len = op_dns->_host_name_buffer_len;
	            sd_strncpy(dns_content._host_name, op_dns->_host_name, dns_content._host_name_buffer_len);
	            sd_time_ms(&dns_content._start_time);

	            struct hostent *phostent = NULL;
                LOG_DEBUG("ready to gethostbyname(%s) in thread %u", op_dns->_host_name, sd_get_self_taskid());

#if defined(LINUX)
                memset(&hints, 0, sizeof(struct addrinfo));

                hints.ai_family = AF_INET; /* only for ipv4 */

                ret_val = getaddrinfo(op_dns->_host_name, NULL, &hints, &result);
            	if (ret_val != 0 || !result) {
            		fprintf(stderr, "Fatal Error: can not parse dns info for [%s], error: %d.\n", op_dns->_host_name, ret_val);
            		handle_errno = DNS_INVALID_ADDR;
                    dns_content._result = DNS_INVALID_ADDR;
                    dns_content._ttl[0] = DNS_CHCHE_INVAIL_DNS_TTL;
            	}
                else
                {
                    handle_errno = SUCCESS;
                    _int32 index = 0;
                    for (rp = result; rp != NULL && index < op_dns->_ip_count; rp = rp->ai_next, index++)
                    {
                        sin_p = (struct sockaddr_in *)rp->ai_addr;
                        op_dns->_ip_list[index] = sin_p->sin_addr.s_addr;
                        LOG_DEBUG("get the %dst ip: %u", index, op_dns->_ip_list[index]);
                        dns_content._ip_list[index] = sin_p->sin_addr.s_addr;
                        dns_content._result = SUCCESS;
                        dns_content._ttl[index] = MAX_DNS_VALID_TIME; //3600 * 1000;
                    }
                    op_dns->_ip_count = index;
                    dns_content._ip_count = index;
                    freeaddrinfo(result);
                }
#elif defined(WINCE)
                phostent = (struct hostent *)sd_gethostbyname(op_dns->_host_name);

                if (phostent)
                {
                    handle_errno = SUCCESS;
                    ip_count = MIN(phostent->h_length / 4, op_dns->_ip_count);
                    LOG_DEBUG("get %d ip of hostname %s, and expect %d ip", phostent->h_length / 4, op_dns->_host_name, op_dns->_ip_count);
                    op_dns->_ip_count = ip_count;
                    dns_content._ip_count = ip_count;
                    _int32 index = 0;
                    for (index = 0; index < ip_count; index ++)
                    {
                        sd_memcpy((void*)&op_dns->_ip_list[index], (void*)((_u32*)phostent->h_addr_list[index]), sizeof(_u32));
                        LOG_DEBUG("get the %dst ip: %u", index, op_dns->_ip_list[index]);
                        dns_content._ip_list[index] = op_dns->_ip_list[index];
                        dns_content._result = SUCCESS;
                        dns_content._ttl[index] = MAX_DNS_VALID_TIME; //3600 * 1000;
                    }
                }
                else
                {
                    handle_errno = DNS_INVALID_ADDR;
                    dns_content._result = DNS_INVALID_ADDR;
                    dns_content._ttl[0] = DNS_CHCHE_INVAIL_DNS_TTL;
                }
#endif
                /* append to cache */
                ret_val = dns_cache_append(&g_dns_cache, &dns_content);
                CHECK_THREAD_VALUE(ret_val);
            }
        }
        else
        {
            LOG_DEBUG("get a unregister-msg(id:%d) in thread %u(dns reactor), and its status(%d)", pmsg->_msg_id, sd_get_self_taskid(), pmsg->_cur_reactor_status);
            handle_errno = REACTOR_E_UNREGISTER;
        }

        LOG_DEBUG("ready to notice complete-event of msg(id:%d) in dns reactor", pmsg->_msg_id);
        /*ignore completed*/

        ret_val = notice_complete_event(&g_dns_reactor, (_u16)handle_errno, pmsg);
        CHECK_THREAD_VALUE(ret_val);

        /* try to get new msg */
        ret_val = pop_register_event_with_lock(&g_dns_reactor, &pmsg);
        CHECK_THREAD_VALUE(ret_val);
    }

    return ret_val;
}
#endif

void dns_handler(void *args)
{
    sd_ignore_signal();
#ifdef SET_PRIORITY
    setpriority(PRIO_PROCESS, 0, 5);
#endif

    RUN_THREAD(g_dns_reactor_thread_status);
    BEGIN_THREAD_LOOP(g_dns_reactor_thread_status)
    
#ifdef DNS_ASYNC
    dns_handler_async_run();
#else
    dns_handler_sync_run();
#endif

	END_THREAD_LOOP(g_dns_reactor_thread_status);
	finished_thread(&g_dns_reactor_thread_status);
}
