
#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
#include "emule_pipe_wait_accept.h"
#include "../emule_interface.h"
#include "utility/list.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"

static	LIST		g_wait_accept_pipe;

void emule_init_wait_accept_pipe(void)
{
	list_init(&g_wait_accept_pipe);
}

void emule_uninit_wait_accept_pipe(void)
{
	sd_assert(list_size(&g_wait_accept_pipe) == 0);
}

_int32 emule_add_wait_accept_pipe(EMULE_DATA_PIPE* emule_pipe)
{
	return list_push(&g_wait_accept_pipe, emule_pipe);
}

_int32 emule_remove_wait_accept_pipe(EMULE_DATA_PIPE* emule_pipe)
{
	LIST_ITERATOR iter;
	for(iter = LIST_BEGIN(g_wait_accept_pipe); iter != LIST_END(g_wait_accept_pipe); iter = LIST_NEXT(iter))
	{
		if(LIST_VALUE(iter) == emule_pipe)
			return list_erase(&g_wait_accept_pipe, iter);
	}
	return -1;
}

EMULE_DATA_PIPE* emule_find_wait_accept_pipe(_u8 user_id[USER_ID_SIZE], _u32 client_id, _u32 server_ip, 
                                             _u16 server_port)
{
	EMULE_DATA_PIPE* emule_pipe;
	EMULE_RESOURCE* res = NULL;
	LIST_ITERATOR iter;

    for(iter = LIST_BEGIN(g_wait_accept_pipe); iter != LIST_END(g_wait_accept_pipe); iter = LIST_NEXT(iter))
	{
		emule_pipe = (EMULE_DATA_PIPE*)LIST_VALUE(iter);
		res = (EMULE_RESOURCE*)emule_pipe->_data_pipe._p_resource;

        //目前没有比较user_id，应该用user_id来比较，这样结果可行度高
        if(emule_resource_compare(res, client_id, server_ip, server_port) == TRUE) return emule_pipe;
	}

	return NULL;
}

BOOL emule_resource_compare(EMULE_RESOURCE* res, _u32 client_id, _u32 server_ip, _u16 server_port)
{
	if(IS_LOWID(res->_client_id))
	{
        // client_id很有可能是0
		sd_assert(client_id != 0 && server_ip != 0 && server_port != 0);
		if(res->_client_id == client_id && res->_server_ip == server_ip && res->_server_port == server_port)
			return TRUE;		//Both have the same lowID, Same serverIP and Port..
		else 
			return FALSE;
	}
	sd_assert(FALSE);
	return FALSE;
}


#endif /* ENABLE_EMULE */
#endif /* ENABLE_EMULE */

