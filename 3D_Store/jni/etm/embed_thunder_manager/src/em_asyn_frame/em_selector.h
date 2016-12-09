#ifndef _EM_SELECTOR_H_00138F8F2E70_200809021219
#define _EM_SELECTOR_H_00138F8F2E70_200809021219

#include "asyn_frame/selector.h"
#define   em_create_selector 		create_selector
#define   em_destory_selector 		destory_selector

#define   em_is_channel_full 		is_channel_full
/* add a socket channel, do not check this socket whether if exist */
#define   em_add_a_channel 		add_a_channel
/* modify a socket channel */
#define   em_modify_a_channel 		modify_a_channel

#define   em_del_a_channel 			del_a_channel


#define   em_get_next_channel 		get_next_channel

#define   em_is_channel_error 		is_channel_error
#define  em_is_channel_readable 	is_channel_readable
#define  em_is_channel_writable 	is_channel_writable

#define  em_get_channel_data 		get_channel_data

#define  em_selector_wait 			selector_wait

#endif  /* _EM_SELECTOR_H_00138F8F2E70_200809021219 */
