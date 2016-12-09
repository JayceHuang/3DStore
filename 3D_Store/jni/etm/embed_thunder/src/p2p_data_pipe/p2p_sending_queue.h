/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/07/2
-----------------------------------------------------------------------------------------------------------*/
#ifndef _P2P_SENDING_QUEUE_H_
#define _P2P_SENDING_QUEUE_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#include "utility/mempool.h"
#include "utility/list.h"

typedef struct tagP2P_SENDING_CMD
{
	char* buffer;
	_u32  len;	
	_u8	_cmd_type;
}P2P_SENDING_CMD;

#define P2P_SENDING_QUEUE_SIZE 8

typedef struct tagP2P_SENDING_QUEUE
{	
	/*
	_cmd_queue[0] -- CANCEL_CMD
	_cmd_queue[1] -- REQUEST_CMD
	_cmd_queue[2] -- INTERESTED_CMD
	_cmd_queue[3] -- NOT_INTERESTED_CMD
	_cmd_queue[4] -- CANCEL_RESPONSE_CMD
	_cmd_queue[5] -- REQUEST_RESPONSE_CMD
	_cmd_queue[6] -- INTERESTED_RESPONSE_CMD
	_cmd_queue[7] -- KEEP_ALIVE_CMD
	*/
	LIST	_cmd_queue[P2P_SENDING_QUEUE_SIZE];		
}P2P_SENDING_QUEUE;

_int32 init_p2p_sending_queue_mpool(void);

_int32 uninit_p2p_sending_queue_mpool(void);

/*
function : create a p2p sending queue
param	 : [out]sending_queue -- point to p2p sending queue address
return	 : 0	   -- success,and (*sending_queue) is point to a valid p2p sending queue
		   nonzero -- failed, and (*sending_queue) is set to NULL
*/
_int32 create_p2p_sending_queue(P2P_SENDING_QUEUE** sending_queue);

/*
function : free all p2p command node in sending queue and destroy the sending queue
param	 : [in]sending_queue -- a p2p sending queue pointer;
return	 : 0 -- success; nonzero -- failed
*/
_int32 close_p2p_sending_queue(P2P_SENDING_QUEUE* sending_queue);

/*
function : push a p2p command in sending queue
param	 : [in]sending_queue -- a p2p sending queue pointer; [in]cmd_node -- a p2p command node pointer;
		   [in]command_type	 -- the p2p command type
return	 : void
*/
_int32 p2p_push_sending_cmd(P2P_SENDING_QUEUE* sending_queue, P2P_SENDING_CMD* cmd_node, _u8 command_type);

/*
function : pop a p2p command in sending queue
param	 : [in]sending_queue -- a p2p sending queue pointer;	[out]cmd_node -- point to a p2p command node pointer;
return	 : 0	   -- success, if (*cmd_node) is set to NULL means no node in queue;
		   nonzero -- failed
*/
_int32 p2p_pop_sending_cmd(P2P_SENDING_QUEUE* sending_queue, P2P_SENDING_CMD** cmd_node);

/*
function : allocate a p2p command node
param	 : [out]cmd_node -- point to a p2p command node pointer;
		   [in]buffer -- a p2p command buffer;	[in]len -- the length of buffer
return	 : 0		-- success, and (*node) is point to a valid p2p command
		   nonzero	-- failed,  and (*node) is set to NULL
*/
_int32 p2p_alloc_sending_cmd(P2P_SENDING_CMD** cmd_node, const char* buffer, _u32 len);

/*
function : free the p2p command buffer and the p2p command node
param	 : [in]cmd_node -- the p2p command node to free
return   : 0 -- success;	nonzero -- failed
*/
_int32 p2p_free_sending_cmd(P2P_SENDING_CMD* cmd_node);


/*
function : clear p2p command queue in P2P_SENDING_QUEUE struct
param	 : [in]sending_queue -- a p2p sending queue pointer;	[in]index -- the index of p2p command queue
return	 : 0 -- success;	nonzero -- failed
*/
_int32 p2p_clear_cmd_queue(P2P_SENDING_QUEUE* sending_queue, _u32 index);

#ifdef __cplusplus
}
#endif
#endif
