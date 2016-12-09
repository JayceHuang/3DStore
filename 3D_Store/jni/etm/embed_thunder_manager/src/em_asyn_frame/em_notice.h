#ifndef _EM_NOTICE_H_00138F8F2E70_200807101812
#define _EM_NOTICE_H_00138F8F2E70_200807101812

#include "asyn_frame/notice.h"
#define   em_notice_init 					notice_init

#define   em_create_notice_handle 			create_notice_handle

/* wait weakly, that means the caller need not depend to this signal. so we can change to msg-poll easily */
#define   em_create_waitable_container 		create_waitable_container

#define   em_add_notice_handle 				add_notice_handle
#define   em_del_notice_handle 				del_notice_handle

#define   em_notice 							notice
#define   em_wait_for_notice 				wait_for_notice
#define   em_reset_notice 					reset_notice

#define   em_destory_notice_handle 			destory_notice_handle
#define   em_destory_waitable_container 	destory_waitable_container

#define   em_init_simple_event 				init_simple_event
#define   em_wait_sevent_handle 			wait_sevent_handle
#define   em_signal_sevent_handle 			signal_sevent_handle
#define   em_uninit_simple_event 			uninit_simple_event

#define   em_empty_impl empty_impl
#define   em_wait_instance wait_instance

#endif /* _EM_NOTICE_H_00138F8F2E70_200807101812 */

