/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/11/20
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_PIPE_WAIT_ACCEPT_H_
#define _EMULE_PIPE_WAIT_ACCEPT_H_
#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL


#include "emule/emule_pipe/emule_pipe_interface.h"

struct tagEMULE_RESOURCE;

void emule_init_wait_accept_pipe(void);

void emule_uninit_wait_accept_pipe(void);

_int32 emule_add_wait_accept_pipe(EMULE_DATA_PIPE* emule_pipe);

_int32 emule_remove_wait_accept_pipe(EMULE_DATA_PIPE* emule_pipe);

EMULE_DATA_PIPE* emule_find_wait_accept_pipe(_u8 user_id[USER_ID_SIZE], 
                                             _u32 client_id, 
                                             _u32 server_ip, 
                                             _u16 server_port);

BOOL emule_resource_compare(struct tagEMULE_RESOURCE* res, _u32 client_id, _u32 server_ip, _u16 server_port);

#endif
#endif
#endif


