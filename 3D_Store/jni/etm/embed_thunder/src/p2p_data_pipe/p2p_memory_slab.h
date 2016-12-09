/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/08/30
-----------------------------------------------------------------------------------------------------------*/
#ifndef _P2P_MEMORY_SLAB_H_
#define _P2P_MEMORY_SLAB_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "p2p_data_pipe/p2p_cmd_define.h"

struct tagP2P_DATA_PIPE;
struct tagP2P_RESOURCE;
struct tagSOCKET_DEVICE;
struct _tag_range;

//目前发现最长的p2p命令为intersted_resp,长度最大达到205
#define	RECV_CMD_BUFFER_LEN		256

_int32	init_p2p_memory_slab(void);
_int32	uninit_p2p_memory_slab(void);

_int32	p2p_malloc_p2p_data_pipe(struct tagP2P_DATA_PIPE** pointer);
_int32	p2p_free_p2p_data_pipe(struct tagP2P_DATA_PIPE* pointer);

_int32	p2p_malloc_p2p_resource(struct tagP2P_RESOURCE** pointer);
_int32	p2p_free_p2p_resource(struct tagP2P_RESOURCE* pointer);

_int32	p2p_malloc_socket_device(struct tagSOCKET_DEVICE** pointer);
_int32	p2p_free_socket_device(struct tagSOCKET_DEVICE* pointer);

_int32	p2p_malloc_recv_cmd_buffer(char** pointer);
_int32	p2p_free_recv_cmd_buffer(char* pointer);

_int32	p2p_malloc_range(struct _tag_range** pointer);
_int32	p2p_free_range(struct _tag_range* pointer);

#ifdef UPLOAD_ENABLE
struct tagP2P_UPLOAD_DATA;
_int32 p2p_malloc_p2p_upload_data(struct tagP2P_UPLOAD_DATA** pointer);
_int32 p2p_free_p2p_upload_data(struct tagP2P_UPLOAD_DATA* pointer);
#endif
#ifdef __cplusplus
}
#endif
#endif


