#include "utility/logid.h"
#define  LOGID	LOGID_P2P_PIPE
#include "utility/logger.h"
#include "utility/errcode.h"
#include "utility/settings.h"

#include "p2p_sending_queue.h"

#include "p2p_cmd_define.h"
#include "p2p_transfer_layer/ptl_interface.h"

static SLAB*	g_p2p_sending_queue_slab = NULL;
static SLAB*	g_p2p_sending_cmd_slab = NULL;

#define  PIPE_UPLOAD_NUM	18
static _u32 g_p2p_upload_data_num = PIPE_UPLOAD_NUM;

_int32 init_p2p_sending_queue_mpool()
{
	_int32 ret = SUCCESS;
	LOG_DEBUG("init_p2p_sending_queue_mpool");
	
	settings_get_int_item("p2p.p2p_upload_data_num", (_int32*)&(g_p2p_upload_data_num));
	
	ret = mpool_create_slab(sizeof(P2P_SENDING_QUEUE), P2P_SENDING_QUEUE_SLAB_SIZE, 0, &g_p2p_sending_queue_slab);
	CHECK_VALUE(ret);
	
	ret = mpool_create_slab(sizeof(P2P_SENDING_CMD), P2P_SENDING_CMD_SLAB_SIZE, 0, &g_p2p_sending_cmd_slab);
	CHECK_VALUE(ret);
	return ret;
}

_int32 uninit_p2p_sending_queue_mpool()
{
	_int32 ret = SUCCESS;
	
	if(g_p2p_sending_queue_slab != NULL)
	{
		ret = mpool_destory_slab(g_p2p_sending_queue_slab);
		CHECK_VALUE(ret);
		g_p2p_sending_queue_slab = NULL;
	}
	
	if(g_p2p_sending_cmd_slab != NULL)
	{
		ret = mpool_destory_slab(g_p2p_sending_cmd_slab);
		CHECK_VALUE(ret);
		g_p2p_sending_cmd_slab = NULL;
	}
	
	return ret;
}

_int32 create_p2p_sending_queue(P2P_SENDING_QUEUE** sending_queue)
{
	_u32 i = 0;
	_int32 ret = mpool_get_slip(g_p2p_sending_queue_slab, (void**)sending_queue);
	CHECK_VALUE(ret);
	for (i = 0; i < P2P_SENDING_QUEUE_SIZE; ++i)
	{
	    list_init(&(*sending_queue)->_cmd_queue[i]);
	}
	return SUCCESS;
}

_int32 close_p2p_sending_queue(P2P_SENDING_QUEUE* sending_queue)
{
	P2P_SENDING_CMD* cmd = NULL;
	while (p2p_pop_sending_cmd(sending_queue, &cmd) == SUCCESS)
	{
		if (cmd == NULL)
		{
		    break;
		}
			
		ptl_free_cmd_buffer(cmd->buffer);
		p2p_free_sending_cmd(cmd);
	}
	return mpool_free_slip(g_p2p_sending_queue_slab, sending_queue);
}

_int32 p2p_push_sending_cmd(P2P_SENDING_QUEUE* sending_queue, P2P_SENDING_CMD* cmd_node, _u8 command_type)
{
	/*
	when push a command, follow rules:
	A. when push NOT_INTERESTED_CMD command, all INTERESTED_CMD command in queue must discard.
	B. when push INTERESTED_CMD command, all NOT_INTERESTED_CMD command in queue must discard.
	C. when push INTERESTED_RESPONSE_CMD command, all INTERESTED_RESPONSE_CMD command in queue must discard.
	D. when push CANCEL_RESPONSE_CMD command, all REQUEST_RESPONSE_CMD command in queue must discard.
	E. when push CANCEL_CMD command, all REQUEST_CMD command in queue must discard.
	F. when push any command, all KEEP_ALIVE_CMD command in queue must discard.

	发送队列中的不同类型的包有不同的优先级。优先级高的应该先发。
	发送完一个包后，才能开始发送另外的包。不同类型包的优先级顺序如下：
	（数字越小，优先级越高；数字相同，优先级相同，则先存放到发送队列中的包先发）
	sending_queue->_cmd_queue[0] -- CANCEL_CMD and FIN_CMD
	sending_queue->_cmd_queue[1] -- REQUEST_CMD and FIN_RESP_CMD
	sending_queue->_cmd_queue[2] -- INTERESTED_CMD and NOT_INTERESTED_CMD
	sending_queue->_cmd_queue[3] -- CANCEL_RESPONSE_CMD
	sending_queue->_cmd_queue[4] -- CHOKE_CMD and UNCHOKE_CMD
	sending_queue->_cmd_queue[5] -- REQUEST_RESPONSE_CMD
	sending_queue->_cmd_queue[6] -- INTERESTED_RESPONSE_CMD
	sending_queue->_cmd_queue[7] -- KEEP_ALIVE_CMD
	*/
	if(list_size(&sending_queue->_cmd_queue[7]) > 0)
	{
		p2p_clear_cmd_queue(sending_queue, 7);		/*clear all KEEP_ALIVE_CMD command*/
	}
	/*process each type of p2p command*/
	switch(command_type)
	{	/*HANDSHAKE_CMD command not push in sending_queue*/
	case INTERESTED:
		{
			if(list_size(&sending_queue->_cmd_queue[2]) > 0)
				p2p_clear_cmd_queue(sending_queue, 2);		/*clear all NOT_INTERESTED_CMD command*/
			return list_push(&sending_queue->_cmd_queue[2], cmd_node);
		}
	case INTERESTED_RESP:
		{
			if(list_size(&sending_queue->_cmd_queue[6]) > 0)
				p2p_clear_cmd_queue(sending_queue, 6);		/*clear all INTERESTED_RESPONSE_CMD command*/
			return list_push(&sending_queue->_cmd_queue[6], cmd_node);
		}
	case NOT_INTERESTED:
		{
			if(list_size(&sending_queue->_cmd_queue[2]) > 0)
				p2p_clear_cmd_queue(sending_queue, 2);		/*clear all INTERESTED_CMD command*/
			return list_push(&sending_queue->_cmd_queue[2], cmd_node);
		}
	case KEEPALIVE:
		{
			return list_push(&sending_queue->_cmd_queue[7], cmd_node);
		}
	case REQUEST:
		{
			return list_push(&sending_queue->_cmd_queue[1], cmd_node);
		}
	case REQUEST_RESP:
		{
			//if( list_size(&sending_queue->_cmd_queue[5])> g_p2p_upload_data_num ) return ERR_P2P_CONNECT_UPLOAD_SLOW;
			return list_push(&sending_queue->_cmd_queue[5], cmd_node);
		}
	case CANCEL:
		{
			if(list_size(&sending_queue->_cmd_queue[1]) > 0)
				p2p_clear_cmd_queue(sending_queue, 1);		/*clear all REQUEST_CMD command*/
			return list_push(&sending_queue->_cmd_queue[0], cmd_node);
		}
	case CANCEL_RESP:
		{
			if(list_size(&sending_queue->_cmd_queue[5]) > 0)
				p2p_clear_cmd_queue(sending_queue, 5);		/*clear all REQUEST_RESPONSE_CMD command*/
			return list_push(&sending_queue->_cmd_queue[3], cmd_node);
		}
	case CHOKE:
		{
			if(list_size(&sending_queue->_cmd_queue[5]) > 0)
				p2p_clear_cmd_queue(sending_queue, 5);		/*clear all REQUEST_RESPONSE_CMD command*/
			if(list_size(&sending_queue->_cmd_queue[4]) > 0)
				p2p_clear_cmd_queue(sending_queue, 4);		/*clear all UNCHOKE_CMD command*/
			return list_push(&sending_queue->_cmd_queue[4], cmd_node); 
		}
	case UNCHOKE:
		{
			if(list_size(&sending_queue->_cmd_queue[4]) > 0)
				p2p_clear_cmd_queue(sending_queue, 4);		/*clear all CHOKE_CMD command*/
			return list_push(&sending_queue->_cmd_queue[4], cmd_node);
		}
	case FIN_RESP:
		{
			if(list_size(&sending_queue->_cmd_queue[5]) > 0)
				p2p_clear_cmd_queue(sending_queue, 5);		/*clear all REQUEST_RESPONSE_CMD command*/
			return list_push(&sending_queue->_cmd_queue[1], cmd_node);
		}
			
	default:
		{
			sd_assert(FALSE);
		}
	}

	/*
	when process REQUEST_RESPONSE_CMD command, data manager will load data. This may cost some time.
	At this moment, if a REQUEST_CMD command is committed, it can't be send immediately.That means 
	upload affects download.
	To avoid this situation, just cancel REQUEST_RESPONSE_CMD, and push back to header of the queue,
	and then push REQUEST_CMD in queue. Since REQUEST_CMD command is priorityer than REQUEST_RESPONSE_CMD,
	REQUEST_CMD will be send first.
	This version not implement..
	*/
	return -1;
}

// 优先发送非数据包， 因为数据包会被限速。
_int32 p2p_pop_sending_cmd(P2P_SENDING_QUEUE* sending_queue, P2P_SENDING_CMD** cmd_node)
{
	_u32 i;
	for(i = 0; i < 5; ++i)
	{
		if(list_size(&sending_queue->_cmd_queue[i]) > 0)
		{
			return list_pop(&sending_queue->_cmd_queue[i], (void**)cmd_node);
		}
	}
    for(i = 6; i < P2P_SENDING_QUEUE_SIZE; ++i)
	{
		if(list_size(&sending_queue->_cmd_queue[i]) > 0)
		{
			return list_pop(&sending_queue->_cmd_queue[i], (void**)cmd_node);
		}
	}
    if(list_size(&sending_queue->_cmd_queue[5]) > 0)
	{
		return list_pop(&sending_queue->_cmd_queue[5], (void**)cmd_node);
	}
	*cmd_node = NULL;
	return SUCCESS;
}

_int32 p2p_alloc_sending_cmd(P2P_SENDING_CMD** cmd_node, const char* buffer, _u32 len)
{
	_int32 ret;
	*cmd_node = NULL;
	ret = mpool_get_slip(g_p2p_sending_cmd_slab, (void**)cmd_node);
	CHECK_VALUE(ret);
	(*cmd_node)->buffer = (char*)buffer;
	(*cmd_node)->len = len;
	return ret;
}

_int32 p2p_free_sending_cmd(P2P_SENDING_CMD* cmd_node)
{
	return mpool_free_slip(g_p2p_sending_cmd_slab, cmd_node);
}

_int32 p2p_clear_cmd_queue(P2P_SENDING_QUEUE* sending_queue, _u32 index)
{
	_int32 ret = SUCCESS;
	P2P_SENDING_CMD* cmd_node = NULL;
	sd_assert(index < 8);
	while(TRUE)
	{
		list_pop(&sending_queue->_cmd_queue[index], (void**)&cmd_node);
		if(cmd_node == NULL)
			break;
		ptl_free_cmd_buffer(cmd_node->buffer);
		ret = p2p_free_sending_cmd(cmd_node);
		CHECK_VALUE(ret);
	}
	return ret;
}

