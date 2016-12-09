/*----------------------------------------------------------------------------------------------------------
author:		ZengYuqing
created:	2008/09/20
-----------------------------------------------------------------------------------------------------------*/
#ifndef _REPORTER_DEVICE_H_
#define _REPORTER_DEVICE_H_

#ifdef __cplusplus
extern "C" 
{
#endif


#include "utility/list.h"
#include "platform/sd_socket.h"

enum REPORTER_DEVICE_TYPE 
{ 
	REPORTER_LICENSE=0
	,REPORTER_SHUB
	,REPORTER_STAT_HUB
#ifdef ENABLE_BT
	,REPORTER_BT_HUB
#endif

#ifdef ENABLE_EMULE
	,REPORTER_EMULE_HUB
#endif

#ifdef UPLOAD_ENABLE
	,REPORTER_PHUB 
#endif

#ifdef EMBED_REPORT
	,REPORTER_EMBED
#endif	

} ;

typedef struct tagREPORTER_CMD
{
	const char*		_cmd_ptr;			//命令指针
	_u32			_cmd_len;			//命令长度
	void*			_user_data;
	_u16			_cmd_type;
	_u32			_retry_times;
	_u32			_cmd_seq;			/*check if command valid*/
}REPORTER_CMD;


typedef struct tagREPORTER_DEVICE
{
	enum REPORTER_DEVICE_TYPE		_type;			/*type of hub*/
	SOCKET			_socket;			//套接字
	char*			_recv_buffer;		//接收缓冲区
	_u32			_recv_buffer_len;	//接收缓冲区的长度
	_u32			_had_recv_len;		//已经接收到
	LIST			_cmd_list;			/*command queue which node type is REPORTER_CMD*/ 
	REPORTER_CMD*	_last_cmd;			/*last command which is processing*/
	_u32			_timeout_id;		/*when report failed, it will start a timer and retry later*/ 
}REPORTER_DEVICE;

#ifdef __cplusplus
}
#endif


#endif  /* _REPORTER_DEVICE_H_ */


