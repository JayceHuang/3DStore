#ifndef _RES_QUERY_INTERFACE_IMP_H_
#define _RES_QUERY_INTERFACE_IMP_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#include "platform/sd_socket.h"
#include "res_query_interface.h"
#include "res_query_cmd_define.h"

typedef struct tagRES_QUERY_CMD
{
	 char* _cmd_ptr;			//命令指针
	_u32 _cmd_len;			//命令长度
	void* _callback;
	void* _user_data;			
	_u16 _cmd_type;
	_u32 _retry_times;
	_u32 _cmd_seq;			/*check if command valid*/
	BOOL _not_add_res;
    void* _extra_data;        /*命令附加的额外数据，用于夹带命令私货*/
	BOOL _is_req_with_rsa;
	_u8 _aes_key[16];
}RES_QUERY_CMD;

//shub用来查询server资源，phub用来查询peer资源
typedef struct tagHUB_DEVICE
{
    HUB_TYPE _hub_type;			/*type of hub*/
    SOCKET _socket;			//套接字
	char* _recv_buffer;		//接收缓冲区
	_u32 _recv_buffer_len;	//接收缓冲区的长度
	_u32 _had_recv_len;		//已经接收到
	LIST _cmd_list;			/*command queue which node type is RES_QUERY_CMD*/ 
	RES_QUERY_CMD* _last_cmd;			/*last command which is processing*/
	_u32 _timeout_id;		/*when resource query failed, it will start a timer and retry later*/ 
	_u32 _last_cmd_born_time;
	BOOL _is_cmd_time_out;
    char _host[MAX_HOST_NAME_LEN];
    _u16 _port;
}HUB_DEVICE;

static _int32 _res_query_phub_helper(void* user_data, 
							  res_query_notify_phub_handler handler, 
							  const _u8* cid, 
							  const _u8* gcid, 
							  _u64 file_size, 
							  _int32 bonus_res_num, 
							  _u8 query_type,
							  QUERY_PEER_RES_CMD *cmd);

//////////////////////////////////////////////////////////////////////////
// 查询root节点，获得本地的父节点
typedef _int32 (*res_query_notify_dphub_root_handler)(_int32 errcode, _u16 result, _u32 retry_interval);
_int32 res_query_dphub_root(res_query_notify_dphub_root_handler handler, _u32 region_id,
                            _u32 parent_id, _u32 last_query_stamp);

// res_query_notify_dphub_root_handler 处理查询root后的通知
_int32 handle_res_query_notify_dphub_root(_int32 errcode, _u16 result, _u32 retry_interval);
//////////////////////////////////////////////////////////////////////////

void pop_device_from_dnode_list(HUB_DEVICE *device);

#ifdef __cplusplus
}
#endif

#endif

