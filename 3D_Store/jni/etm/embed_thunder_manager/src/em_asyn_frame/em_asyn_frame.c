#include "em_common/em_define.h"
#if defined(LINUX)
#include <signal.h>
#elif defined(MSTAR)
#include <MsTypes.h>
#include <apiSystemInit.h>
#include <MsOS.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <MsTypes.h>
#include <apiFS.h>
#endif

#include "em_asyn_frame/em_asyn_frame.h"
#include "em_asyn_frame/em_msg_list.h"
#include "em_asyn_frame/em_msg_alloctor.h"
#include "em_asyn_frame/em_timer.h"
#include "em_asyn_frame/em_device_reactor.h"
#include "em_common/em_errcode.h"
#include "em_common/em_map.h"
#include "em_common/em_thread.h"
#include "em_interface/em_task.h"
#include "em_asyn_frame/em_msg.h"

#if defined(__SYMBIAN32__)
#include "em_symbian_interface/em_symbian_interface.h"
#endif
#define EM_LOGID EM_LOGID_ASYN_FRAME
#include "em_common/em_logger.h"

#ifdef LINUX
#include <signal.h>
#endif


static init_handler g_em_init_handler = NULL;
static void *g_em_init_arg = NULL;
static uninit_handler g_em_uninit_handler = NULL;
static void *g_em_uninit_arg = NULL;

#if !defined(__SYMBIAN32__)
static THREAD_STATUS g_em_thread_status = INIT;
static _int32 g_em_thread_id = 0;
#endif
_int32 em_handle_all_newmsgs(void);
_int32 em_delete_msg(MSG *pmsg);
_int32 em_handle_reactor_msg(MSG *pmsg);
_int32 em_handle_all_event(void);
_int32 em_handle_timeout_msg(void);
_int32 em_handle_new_msg(MSG *pnewmsg);
_int32 em_handle_new_thread_msg(THREAD_MSG *pnewmsg);
_int32 em_callback_msg(MSG *pmsg, _int32 msg_errcode, _u32 elapsed);
_int32 em_handle_immediate_msg(MSG *pmsg);



#define DISPATCH_SOCKET_REACTOR		(100)
#define DISPATCH_FS_REACTOR			(10)
#define DISPATCH_DNS_REACTOR		(10)
#define DISPATCH_IMMEDIATE_MSG		(10)
#define DISPATCH_OTHER_THREAD		(10)

/*
#define DISPATCH_TIMEOUT			(10)
*/

/* thread error handler */
static _int32 g_em_error_exitcode = SUCCESS;
#define EM_EXIT(code)	{g_em_error_exitcode = code; goto ERROR_EXIT;}

#ifdef _DEBUG
#define EM_CHECK_THREAD_VALUE(code) CHECK_VALUE(code)
#else
#define EM_CHECK_THREAD_VALUE(code) {if(code != SUCCESS) EM_EXIT(code);}
#endif


static LIST g_em_timeout_list;
static LIST g_em_immediate_msg_list;

static _int32 em_timer_node_comparator(void *m1, void *m2)
{
	return (_int32)(((MSG*)m1)->_msg_id - ((MSG*)m2)->_msg_id);
}
#if defined(__SYMBIAN32__)

_int32 init_em_asyn_frame(void )
{
	_int32 ret_val = SUCCESS;
	_int32 wait_msg = 0;

	/* init all allocators */
	ret_val = em_msg_alloctor_init();
	CHECK_VALUE(ret_val);

	ret_val = em_msg_queue_init(&wait_msg);
	CHECK_VALUE(ret_val);

 	ret_val = init_em_symbian_scheduler();
	CHECK_VALUE(ret_val);


	/* init timer */
	ret_val = em_init_timer();
	CHECK_VALUE(ret_val);

	if(g_em_init_handler)
	{
		ret_val = g_em_init_handler(g_em_init_arg);
		if(ret_val != SUCCESS)
		{
			EM_LOG_ERROR("em_aysn_frame init failed, user-init return %d", ret_val);

			CHECK_VALUE(ret_val);
		}
	}


	/* init timeout list */
	em_list_init(&g_em_timeout_list);

	/* init immediate-msg-list */
	em_list_init(&g_em_immediate_msg_list);

	/* refresh timer firstly */
	ret_val = em_refresh_timer();

	//CHECK_VALUE(ret_val);


	return ret_val;
}


_int32 uninit_em_asyn_frame(void )
{
	_int32 ret_val = SUCCESS;
	_int32 wait_msg = 0;


	if(g_em_uninit_handler)
	{
		g_em_uninit_handler(g_em_uninit_arg);
	}


	ret_val = em_uninit_timer();
	CHECK_VALUE(ret_val);

	ret_val = uninit_em_symbian_scheduler();
	CHECK_VALUE(ret_val);
	
	ret_val = em_msg_queue_uninit();
	CHECK_VALUE(ret_val);


	ret_val = em_msg_alloctor_uninit();
	CHECK_VALUE(ret_val);

	return ret_val;


}

_int32 em_asyn_frame_handler(void *arglist )
{
	_int32 ret_val = SUCCESS;

       /* handle new msg*/
	ret_val = em_handle_all_newmsgs();
	CHECK_VALUE(ret_val);


	/* handle timeout msg */
	ret_val = em_handle_timeout_msg();
	CHECK_VALUE(ret_val);

	/* handle all event(all reactor-msg and timeout-msg) */

	ret_val = em_handle_all_event();
	CHECK_VALUE(ret_val);

	return ret_val;


}

BOOL em_asyn_frame_has_msg_to_do(void)
{
      if(em_msg_has_new_msg() )
     {
           return TRUE; 
     }
     else
     {
           return FALSE;
     }
     
}

#else


#ifdef MSTAR
void em_asyn_frame_handler(_u32 *arglist, void* unused)
#else
void em_asyn_frame_handler(void *arglist)
#endif
{
	_int32 ret_val = SUCCESS;
	_int32 wait_msg = 0, /*wait_sock = 0, wait_fs = 0, wait_slow_fs = 0, wait_dns = 0,*/ waitable_container = 0;
	_int32 tmp = 0;

	EM_LOG_DEBUG("em_asyn_frame_handler begin");

#if defined(MSTAR)
	em_add_myself_to_taskids();
#endif

	em_ignore_signal();

	/* init all allocators */
	ret_val = em_msg_alloctor_init();
	EM_CHECK_THREAD_VALUE(ret_val);

	ret_val = em_msg_queue_init(&wait_msg);
	EM_CHECK_THREAD_VALUE(ret_val);

	/* init timer */
	ret_val = em_init_timer();
	EM_CHECK_THREAD_VALUE(ret_val);

	/* init waitable container */
	ret_val = em_create_waitable_container(&waitable_container);
	EM_CHECK_THREAD_VALUE(ret_val);

	ret_val = em_add_notice_handle(waitable_container, wait_msg);
	EM_CHECK_THREAD_VALUE(ret_val);


	if(g_em_init_handler)
	{
		ret_val = g_em_init_handler(g_em_init_arg);
		if(ret_val != SUCCESS)
		{
			EM_LOG_ERROR("aysn_frame init failed, user-init return %d", ret_val);
			EM_CHECK_THREAD_VALUE(ret_val);
			return;
		}
	}

	/* init timeout list */
	em_list_init(&g_em_timeout_list);

	/* init immediate-msg-list */
	em_list_init(&g_em_immediate_msg_list);

	/* refresh timer firstly */
	ret_val = em_refresh_timer();
	EM_CHECK_THREAD_VALUE(ret_val);

	RUN_THREAD(g_em_thread_status);

#if defined(MSTAR)
      em_sleep(100);
#endif

	BEGIN_THREAD_LOOP(g_em_thread_status)
	{

/*
		EM_LOG_DEBUG("a new asyn_frame loop coming");
*/

#if defined(MSTAR)
		g_log_asynframe++;
		g_log_asynframe_step = 1;
#endif

              /* handle new msg*/
		ret_val = em_handle_all_newmsgs();
		EM_CHECK_THREAD_VALUE(ret_val);

#if defined(MSTAR)
		g_log_asynframe++;
		g_log_asynframe_step = 2;
#endif
		/* handle timeout msg */
		ret_val = em_handle_timeout_msg();
		EM_CHECK_THREAD_VALUE(ret_val);

		/* handle all event(all reactor-msg and timeout-msg) */
#if defined(MSTAR)
		g_log_asynframe++;
		g_log_asynframe_step = 3;
#endif
		ret_val = em_handle_all_event();
		EM_CHECK_THREAD_VALUE(ret_val);

		/* wait for notice */
#if defined(MSTAR)
		g_log_asynframe++;
		g_log_asynframe_step = 4;		
#endif
		ret_val = em_wait_for_notice(waitable_container, 1, &tmp, GRANULARITY);
		EM_CHECK_THREAD_VALUE(ret_val);

		/* reset all event */
		ret_val = em_reset_notice(wait_msg);
		EM_CHECK_THREAD_VALUE(ret_val);
	}
	END_THREAD_LOOP(g_em_thread_status);

	if(g_em_uninit_handler)
	{
		g_em_uninit_handler(g_em_uninit_arg);
	}

	/* uninit all allocators */
	ret_val = em_destory_waitable_container(waitable_container);
	EM_CHECK_THREAD_VALUE(ret_val);


	ret_val = em_uninit_timer();

	EM_CHECK_THREAD_VALUE(ret_val);

	ret_val = em_msg_queue_uninit();
	EM_CHECK_THREAD_VALUE(ret_val);


	ret_val = em_msg_alloctor_uninit();
	EM_CHECK_THREAD_VALUE(ret_val);

	em_finished_thread(&g_em_thread_status);
	#ifndef _ANDROID_LINUX
	em_finish_task(em_get_self_taskid());
	#endif
#if defined(AMLOS)
	AVTaskDel(OS_ID_SELF);
#endif
	if(ret_val!=SUCCESS)
	{
		goto ERROR_EXIT;
	}
	return;

ERROR_EXIT:

	EM_LOG_ERROR("****** ERROR OCCUR in asyn-frame thread(%u): %d !!!!", em_get_self_taskid(), g_em_error_exitcode);

#ifdef _DEBUG
	sd_assert(FALSE);
#endif

	em_set_critical_error(g_em_error_exitcode);

#if defined(MSTAR)
	em_prn_msg("em_asyn_frame_handler exit failed!!!!!!!%d\n", g_em_error_exitcode);
#endif
	em_finished_thread(&g_em_thread_status);
	em_finish_task(em_get_self_taskid());
	

#if defined(MSTAR)
	SD_UNUSED(waitable_container);
	SD_UNUSED(tmp);
#endif

#if defined(AMLOS)
	AVTaskDel(OS_ID_SELF);
#endif	
}
#endif /* __SYMBIAN32__ */
_int32 em_start_asyn_frame(init_handler h_init, void *init_arg, uninit_handler h_uninit, void *uninit_arg)
{
	_int32 ret_val = SUCCESS;
#if defined(__SYMBIAN32__)
	g_em_init_handler = h_init;
	g_em_init_arg = init_arg;
	g_em_uninit_handler = h_uninit;
	g_em_uninit_arg = uninit_arg;

	return init_em_asyn_frame();
#else
	if((g_em_thread_status!=INIT)&&(g_em_thread_status!=STOPPED))
		return ALREADY_ET_INIT;
	
	g_em_thread_status = INIT;
	
	g_em_init_handler = h_init;
	g_em_init_arg = init_arg;
	g_em_uninit_handler = h_uninit;
	g_em_uninit_arg = uninit_arg;

#if defined(LINUX)
	ret_val = em_create_task(em_asyn_frame_handler, 256 * 1024, NULL, &g_em_thread_id);
#elif defined(AMLOS)
	AM_THREAD_INFO*	thread_info = em_get_am_thread_info(THREAD_TYPE_ASYN_FRAME);
	ret_val = em_create_task(em_asyn_frame_handler, thread_info, NULL, NULL);
#elif defined(MSTAR)
	ret_val = em_create_task((start_address)em_asyn_frame_handler, 256 * 1024, NULL, NULL);
#elif defined(WINCE)
	ret_val = em_create_task(em_asyn_frame_handler, 256 * 1024, NULL, &g_em_thread_id);
#endif
	CHECK_VALUE(ret_val);

	/* wait for thread running */
	while(g_em_thread_status == INIT)
		em_sleep(20);
	return g_em_error_exitcode;
#endif /* __SYMBIAN32__  */
}

_int32 em_stop_asyn_frame(void)
{
#if defined(__SYMBIAN32__)
	return uninit_em_asyn_frame();
#else
	STOP_THREAD(g_em_thread_status);
	em_wait_thread(&g_em_thread_status, 0);

	em_finish_task(g_em_thread_id);
	return SUCCESS;
#endif
}

_int32 em_cancel_from_reactor(MSG *pmsg, _int32 reason)
{
	_int32 ret_val = SUCCESS;

	return ret_val;
}

_int32 em_put_into_reactor(MSG *pmsg)
{
	_int32 ret_val = SUCCESS;

	return ret_val;
}

_int32 em_handle_new_msg(MSG *pnewmsg)
{
	_int32 ret_val = SUCCESS;
	_int32 time_index = 0;
	MSG *pmsg = NULL;
	LIST_ITERATOR list_it;

	if(pnewmsg)
	{
		/* check */
		if(pnewmsg->_notice_count_left <= 0 && pnewmsg->_notice_count_left != NOTICE_INFINITE && VALID_HANDLE((_int32)pnewmsg->_handler))
		{
			EM_LOG_ERROR("the msg(id:%d) notice_count is %d, drop it", pnewmsg->_msg_id, pnewmsg->_notice_count_left);

			ret_val = em_msg_dealloc(pnewmsg);
			CHECK_VALUE(ret_val);

			return SUCCESS;
		}

		if((_int32)pnewmsg->_handler == CANCEL_MSG_BY_MSGID)
		{/* cancel msg by msgid */
			EM_LOG_DEBUG("the msg(id:%d) is a cancel-msg", pnewmsg->_msg_id);
			/* find in timer */
			ret_val = em_erase_from_timer(pnewmsg, em_timer_node_comparator, TIMER_INDEX_NONE, (void**)&pmsg);
			CHECK_VALUE(ret_val);

			if(pmsg)
			{/* erase success */
				if(REACTOR_REGISTERED(pmsg->_cur_reactor_status))
				{/* cancel from reactor, _op_count > 0 */
					EM_LOG_DEBUG("the cancelled-msg(id:%d) is a reactor-msg, so cancel from reactor", pmsg->_msg_id);
					ret_val = em_cancel_from_reactor(pmsg, REACTOR_STATUS_CANCEL);
					CHECK_VALUE(ret_val);
				}
				else if(pmsg->_op_count == 0)
				{/* call back now */

					/* notice once for cancel msg */
					pmsg->_notice_count_left = 0;

					/* this msg has no matter of reactor, so use _cur_reactor_status flag to denote its status */
					pmsg->_cur_reactor_status = REACTOR_STATUS_CANCEL;

					EM_LOG_DEBUG("the msg(id:%d) will put into immediate-list for callback because it is cancelled", pmsg->_msg_id);

					ret_val = em_list_push(&g_em_immediate_msg_list, pmsg);
					CHECK_VALUE(ret_val);
				}
			}
			else
			{/* find in timeout-list */
				for(list_it = LIST_BEGIN(g_em_timeout_list); list_it != LIST_END(g_em_timeout_list); list_it = LIST_NEXT(list_it))
				{
					if(em_timer_node_comparator(pnewmsg, LIST_VALUE(list_it)) == 0)
					{/* find it */
						pmsg = LIST_VALUE(list_it);

						/* notice once for cancel msg */
						pmsg->_notice_count_left = 1;

						EM_LOG_DEBUG("the msg(id:%d) be cancelled and it is a timeout msg", pmsg->_msg_id);

						/* handle it at function(handle_timeout_msg) */
						pmsg->_msg_cannelled = 1;
						break;
					}
				}

				if(list_it == LIST_END(g_em_timeout_list))
				{/* find in immediate-list */
					for(list_it = LIST_BEGIN(g_em_immediate_msg_list); list_it != LIST_END(g_em_immediate_msg_list); list_it = LIST_NEXT(list_it))
					{
						if(em_timer_node_comparator(pnewmsg, LIST_VALUE(list_it)) == 0)
						{/* find it */
							pmsg = LIST_VALUE(list_it);

							EM_LOG_DEBUG("the msg(id:%d) be cancelled and it is a immediate-msg", pmsg->_msg_id);

							/* change status to cancel... */
							pmsg->_cur_reactor_status = REACTOR_STATUS_CANCEL;
							break;
						}
					}

//#ifdef _DEBUG
					/* debug */
					if(list_it == LIST_END(g_em_immediate_msg_list))
					{
						EM_LOG_DEBUG("cancelling a lost msg(id:%d) ....", pnewmsg->_msg_id);

						//CHECK_VALUE(1);
					}
//#endif
				}

			}

			/* free the cancel msg */
			ret_val = em_msg_dealloc(pnewmsg);
			//CHECK_VALUE(ret_val);
		}
		else
		{/* a new msg */

			/* init reactor status */
			pnewmsg->_cur_reactor_status = REACTOR_STATUS_READY;
			pnewmsg->_op_count = 0;
			pnewmsg->_op_errcode = SUCCESS;

			if(pnewmsg->_timeout == 0)
			{/* callback immediately*/
					/* notice once for this case */
					pnewmsg->_notice_count_left = 0;

					EM_LOG_DEBUG("put msg(id:%d) into immediate-list because it is a immediate-msg of timeout 0", pnewmsg->_msg_id);

					/* this msg has no matter of reactor, so use _cur_reactor_status flag to denote its status */
					pnewmsg->_cur_reactor_status = REACTOR_STATUS_TIMEOUT;

					ret_val = em_list_push(&g_em_immediate_msg_list, pnewmsg);
					//CHECK_VALUE(ret_val);
			}
			else
			{/* a normal msg */

				/* set msg-timestamp */
				pnewmsg->_timestamp = em_get_current_timestamp();

				EM_LOG_DEBUG("a normal msg(id:%d), set its timestamp(%u) ready to put into timer & reactor", pnewmsg->_msg_id, pnewmsg->_timestamp);
				/* put into timer */
				ret_val = em_put_into_timer(pnewmsg->_timeout, pnewmsg, &time_index);
				CHECK_VALUE(ret_val);

				pnewmsg->_timeout_index = (_int16)time_index;

				/* put into reactor */
				ret_val = em_put_into_reactor(pnewmsg);
				//CHECK_VALUE(ret_val);
			}
		}
	}

	return ret_val;
}

_int32 em_handle_all_event(void)
{
	_int32 ret_val = SUCCESS;
	MSG *pmsg = NULL;
	THREAD_MSG *pthread_msg = NULL;
	BOOL continued = FALSE;
	_int32 idx = 0;

	do {
		continued = FALSE;


		for(idx = 0; idx < DISPATCH_IMMEDIATE_MSG; idx++)
		{
#if defined(MSTAR)
			g_log_asynframe++;
			g_log_asynframe_step = 103;
#endif
		
			/* immediate-msg */
			ret_val = em_list_pop(&g_em_immediate_msg_list, (void**)&pmsg);
			CHECK_VALUE(ret_val);
			if(!pmsg)
				break;

			ret_val = em_handle_immediate_msg(pmsg);
			CHECK_VALUE(ret_val);

			/* handle timeout msg */
/*
			ret_val = handle_timeout_msg();
			CHECK_VALUE(ret_val);
*/

			continued = TRUE;
		}

		for(idx = 0; idx < DISPATCH_OTHER_THREAD; idx++)
		{
#if defined(MSTAR)
			g_log_asynframe++;
			g_log_asynframe_step = 104;
#endif
		
			/* other-thread-msg */
			ret_val = em_pop_msginfo_node_from_other_thread(&pthread_msg);
			CHECK_VALUE(ret_val);
			if(!pthread_msg)
				break;

			ret_val = em_handle_new_thread_msg(pthread_msg);
			CHECK_VALUE(ret_val);

			/* handle timeout msg */
/*
			ret_val = handle_timeout_msg();
			CHECK_VALUE(ret_val);
*/

			continued = TRUE;
		}

		ret_val = em_handle_timeout_msg();
		CHECK_VALUE(ret_val);

	} while(continued);

	return ret_val;
}

_int32 em_handle_timeout_msg(void)
{
	_int32 ret_val = SUCCESS;
	LIST_ITERATOR list_it;
	LIST tmp_list;
	MSG *pmsg = NULL;
	_int32 /*count = 0,*/ errcode = 0;

	em_list_init(&tmp_list);

	ret_val = em_refresh_timer();
	CHECK_VALUE(ret_val);

	ret_val = em_pop_all_expire_timer(&g_em_timeout_list);
	CHECK_VALUE(ret_val);

/*
	ret_val = pop_all_expire_timer(&tmp_list);
	CHECK_VALUE(ret_val);

	list_cat_and_clear(&g_timeout_list, &tmp_list);

	for(list_it = LIST_BEGIN(g_timeout_list); list_it != LIST_END(g_timeout_list) && count < DISPATCH_TIMEOUT; count++)
*/
	for(list_it = LIST_BEGIN(g_em_timeout_list); list_it != LIST_END(g_em_timeout_list);)
	{
#if defined(MSTAR)
			g_log_asynframe++;
			g_log_asynframe_step = 110;
#endif
	
		pmsg = LIST_VALUE(list_it);
		list_it = LIST_NEXT(list_it);
		em_list_erase(&g_em_timeout_list, LIST_PRE(list_it));

		if(REACTOR_REGISTERED(pmsg->_cur_reactor_status))
		{/* cancel from reactor, _op_count > 0 */
			EM_LOG_DEBUG("find a timeout msg(id:%d) but a reactor event,so unregist from reactor", pmsg->_msg_id);

			ret_val = em_cancel_from_reactor(pmsg, REACTOR_STATUS_TIMEOUT);
			CHECK_VALUE(ret_val);
		}
		else if(pmsg->_op_count == 0)
		{/* call back now */

			if(pmsg->_notice_count_left != NOTICE_INFINITE && pmsg->_notice_count_left > 0)
				pmsg->_notice_count_left --;

			if(pmsg->_msg_cannelled)
			{
				EM_LOG_DEBUG("ready to callback a timeout msg(id:%d) and it be cancelled", pmsg->_msg_id);
				errcode = MSG_CANCELLED;
			}
			else
			{
				EM_LOG_DEBUG("ready to callback msg(id:%d) because of timeout", pmsg->_msg_id);
				errcode = MSG_TIMEOUT;
			}

			ret_val = em_callback_msg(pmsg, errcode, pmsg->_timeout);
			CHECK_VALUE(ret_val);

			EM_LOG_DEBUG("finish to callback about in timeout-handler");
		}
	}

	return ret_val;
}

_int32 em_handle_immediate_msg(MSG *pmsg)
{
	_int32 ret_val = SUCCESS;
	_int32 msg_errcode = 0;
	_u32 elapsed = 0;

	if(pmsg)
	{
		if(pmsg->_cur_reactor_status == REACTOR_STATUS_CANCEL)
		{
			elapsed = em_get_current_timestamp();
			if(elapsed > pmsg->_timestamp)
				elapsed -= pmsg->_timestamp;
			else
				elapsed = 0;

			msg_errcode = MSG_CANCELLED;

			EM_LOG_DEBUG("the handler of msg(id:%d) gotton from immediate-list will be callback because it is cancelled", pmsg->_msg_id);
		}
		else if(pmsg->_cur_reactor_status == REACTOR_STATUS_TIMEOUT)
		{
			elapsed = 0;
			msg_errcode = MSG_TIMEOUT;
			EM_LOG_DEBUG("the handler of msg(id:%d) gotton from immediate-list will be callback because it timeout(timeout is 0)", pmsg->_msg_id);
		}
		else
		{
			EM_LOG_ERROR("unexpected reactor status(%d) in immediate-msg-list", pmsg->_cur_reactor_status);
			return NOT_IMPLEMENT;
		}

		ret_val = em_callback_msg(pmsg, msg_errcode, elapsed);
		CHECK_VALUE(ret_val);

		EM_LOG_DEBUG("finish to callback in immediate-msg-handler");
	}

	return ret_val;
}

_int32 em_handle_reactor_msg(MSG *pmsg)
{
	_int32 ret_val = SUCCESS;

	return ret_val;
}

_int32 em_callback_msg(MSG *pmsg, _int32 msg_errcode, _u32 elapsed)
{
	_int32 ret_val = SUCCESS;

	if(pmsg)
	{
		EM_LOG_DEBUG("will callback of msg(id:%d), (errcode:%d, notice_left:%d, elapsed:%u", pmsg->_msg_id, msg_errcode, pmsg->_notice_count_left, elapsed);


#if defined(MSTAR)
		g_log_asynframe++;
		g_log_asynframe_step =401;
		g_log_operation_type = pmsg->_msg_info._operation_type;
		g_log_msg_errcode = msg_errcode;
		g_log_msg_elapsed = elapsed;
#endif

		ret_val = pmsg->_handler(&pmsg->_msg_info, msg_errcode, pmsg->_notice_count_left, elapsed, pmsg->_msg_id);

#if defined(MSTAR)
		g_log_asynframe++;
		g_log_asynframe_step =402;
#endif		

		EM_LOG_DEBUG("callback return %d about msg(id:%d)", ret_val, pmsg->_msg_id);

	#ifdef _DEBUG
		CHECK_VALUE(ret_val);
	#endif

		if(pmsg->_notice_count_left == 0)
		{
			EM_LOG_DEBUG("try to delete msg(id:%d) because end of callback", pmsg->_msg_id);
#if defined(MSTAR)
		g_log_asynframe++;
		g_log_asynframe_step =403;
#endif			
			ret_val = em_delete_msg(pmsg);

#if defined(MSTAR)
		g_log_asynframe++;
		g_log_asynframe_step =404;
#endif			
			CHECK_VALUE(ret_val);
		}
		else
		{
			EM_LOG_DEBUG("ready to renew msg(id:%d) because its notice_count is %d", pmsg->_msg_id, pmsg->_notice_count_left);

#if defined(MSTAR)
		g_log_asynframe++;
		g_log_asynframe_step =405;
#endif			
			ret_val = em_handle_new_msg(pmsg);

#if defined(MSTAR)
		g_log_asynframe++;
		g_log_asynframe_step =406;
#endif

			CHECK_VALUE(ret_val);
		}

		/* Handle new msg to assure the cancelled msg callback with errcode MSG_CANCELLED 
			  since user can cancel msg in last callback. 
		   This can be optimazed by handle every type of msg one by one */

#if defined(MSTAR)
		g_log_asynframe++;
		g_log_asynframe_step =407;
#endif		
		ret_val = em_handle_all_newmsgs();

#if defined(MSTAR)
		g_log_asynframe++;
		g_log_asynframe_step =408;
#endif
		
		CHECK_VALUE(ret_val);
	}

	return ret_val;
}

_int32 em_handle_new_thread_msg(THREAD_MSG *pnewmsg)
{
	_int32 ret_val = SUCCESS;
	if(pnewmsg)
	{
		if( (_u32)(pnewmsg->_handle) < 1000 ||(_u32)( pnewmsg->_param) < 1000)
		{
			EM_LOG_URGENT("em_handle_new_thread_msg:pnewmsg=0x%X,handle=0x%X,param=0x%X!", pnewmsg,pnewmsg->_handle,pnewmsg->_param);
			//sd_assert(FALSE);
			ret_val = em_force_signal_sevent();
		}
		else
		{
				ret_val = pnewmsg->_handle(pnewmsg->_param);
		}
		EM_LOG_DEBUG("callback of thread-msg return : %d", ret_val);

		/* free the msg. 
		 * this operation will be locked, we assume this opeartion happened seldom.
		 */
		ret_val = em_msg_thread_dealloc(pnewmsg);
		CHECK_VALUE(ret_val);

		/* Handle new msg to assure the cancelled msg callback with errcode MSG_CANCELLED 
			  since user can cancel msg in upper callback. 
		   This can be optimazed by handle every type of msg one by one */
		ret_val = em_handle_all_newmsgs();
		CHECK_VALUE(ret_val);
	}
	return ret_val;
}

_int32 em_delete_msg(MSG *pmsg)
{
	_int32 ret_val = SUCCESS;
	if(pmsg->_notice_count_left == 0 && pmsg->_op_count == 0)
	{
		EM_LOG_DEBUG("will delete msg(id:%d)", pmsg->_msg_id);

		/* free the memory of msg */
		ret_val = em_dealloc_parameter(&pmsg->_msg_info);
		CHECK_VALUE(ret_val);

		ret_val = em_msg_dealloc(pmsg);
		CHECK_VALUE(ret_val);
	}
	return ret_val;
}

_int32 em_handle_all_newmsgs(void)
{
	_int32 ret_val = SUCCESS;
	MSG *pmsg = NULL;

	/* refresh timer for put new msg */
	ret_val = em_refresh_timer();
	CHECK_VALUE(ret_val);

	/* try to get a new msg */
	ret_val = em_pop_msginfo_node(&pmsg);
	CHECK_VALUE(ret_val);

	while(pmsg)
	{
		EM_LOG_DEBUG("ready to handle new msg(id:%d)", pmsg->_msg_id);
		ret_val = em_handle_new_msg(pmsg);
		CHECK_VALUE(ret_val);

		pmsg = NULL;

		ret_val = em_pop_msginfo_node(&pmsg);
		CHECK_VALUE(ret_val);
	}

	return ret_val;
}

_int32 em_peek_operation_count_by_device_id(_u32 device_id, _u32 device_type, _u32 *count)
{
	_int32 ret_val = SUCCESS;
	return ret_val;
}

