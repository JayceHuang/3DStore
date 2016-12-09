/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/11/13
-----------------------------------------------------------------------------------------------------------*/
#ifndef	_UDT_CMD_DEFINE_H_
#define	_UDT_CMD_DEFINE_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#include "utility/bitmap.h"

#define		UDT_HEADER_SIZE					29
#define		UDT_NEW_HEADER_SIZE				33			//新版本udt头部

//#define		DEFAULT_SEND_RECV_SPACE			65536
#define		DEFAULT_SEND_RECV_SPACE			327680

//command type
#define		RESET								7
#define		NAT_KEEPALIVE						8
#define		DATA_TRANSFER						9
#define		ADVANCED_ACK						17
#define		ADVANCED_DATA					18

//command define

typedef struct tagSYN_CMD
{
	_u32				_version;
	_u8					_cmd_type;
	_u32				_flags;
	_u16				_source_port;
	_u16				_target_port;
	_u32				_peerid_hashcode;
	_u32				_seq_num;
	_u32				_ack_num;
	_u32				_window_size;
	_u16				_udt_version;
}SYN_CMD;

typedef struct tagNAT_KEEPALIVE_CMD
{
	_u32				_version;
	_u8					_cmd_type;
	_u16				_source_port;
	_u16				_target_port;
	_u32				_peerid_hashcode; 
}NAT_KEEPALIVE_CMD;

typedef struct tagRESET_CMD
{
	_u32	_version;
	_u8		_cmd_type;
	_u16	_source_port;
	_u16	_target_port;
	_u32	_peerid_hashcode; 
}RESET_CMD;

typedef	struct tagDATA_TRANSFER_CMD 
{
	_u32	_version;
	_u8		_cmd_type;
	_u16	_source_port;
	_u16	_target_port;
	_u32	_peerid_hashcode;
	_u32	_seq_num;
	_u32	_ack_num;
	_u32	_window_size;
	_u32	_data_len;
	char*	_data;
}DATA_TRANSFER_CMD;

typedef struct tagADVANCED_DATA_CMD
{
	_u32	_version;
	_u8		_cmd_type;
	_u16	_source_port;
	_u16	_target_port;
	_u32	_peerid_hashcode;
	_u32	_seq_num;
	_u32	_ack_num;
	_u32	_window_size;
	_u32	_data_len;
	_u32	_package_seq;
	char*	_data;
}ADVANCED_DATA_CMD;

typedef struct tagADVANCED_ACK_CMD
{
	_u32	_version;
	_u8		_cmd_type;
	_u16	_source_port;
	_u16	_target_port;
	_u32	_peerid_hashcode;
	_u32	_window_size;
	_u32	_seq_num;
	_u32	_ack_num;
	_u32	_ack_seq;		//该确认是收到本Seq后发送的确认
	_u32	_bitmap_base;
	_u32	_bitmap_count;
	char*	_bit;
	_u32	_mem_size;
}ADVANCED_ACK_CMD;
#ifdef __cplusplus
}
#endif
#endif
