/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2009/02/26
-----------------------------------------------------------------------------------------------------------*/
#ifndef _BT_DATA_PIPE_H_
#define	_BT_DATA_PIPE_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#ifdef ENABLE_BT 
#ifdef ENABLE_BT_PROTOCOL
#include "bt_download/bt_utility/bt_define.h"
#include "connect_manager/resource.h"
#include "connect_manager/data_pipe.h"
#include "utility/bitmap.h"
#include "utility/list.h"
#include "utility/map.h"

struct tagBT_DEVICE;

typedef struct tagBT_RESOURCE
{
	RESOURCE				_res_pub;
	_u32					_ip;
	_u16					_port;
}BT_RESOURCE;

typedef struct tagBT_PIECE_DATA
{
	_u32	_index;
	_u32	_len;
	_u32	_offset;
}BT_PIECE_DATA;

typedef struct tagBT_DATA_PIPE
{
	DATA_PIPE			 _data_pipe;
	struct tagBT_DEVICE* _device;
	char	_remote_peerid[BT_PEERID_LEN + 1];
	_u32	_last_send_package_time;	//最后一次发送数据包的时间
	_u32	_last_recv_package_time;	//最后一次接收数据包的时间
	_u32	_timeout_id;
	_u32	_piece_size;
	_u32	_total_piece;
	_u64	_total_size;
	BITMAP	_buddy_have_bitmap;
	BITMAP	_interested_bitmap;
	LIST	_request_piece_list;	
	LIST	_upload_piece_list;			//正在上传的piece
	BT_PIECE_DATA* _reading_piece;		//正在读取数据的piece
	RANGE	_uncomplete_range;			//未完成的range
	RANGE	_down_range;				//正在下载的range
	char*	_data_buffer;				//从data_manager获得的内存
	_u32	_data_buffer_len;
	_u32	_get_data_buffer_timeout_id;
	BOOL	_is_ever_unchoke;
#ifndef UPLOAD_ENABLE
	BOOL	_choke_remote;
#endif
    BOOL    _is_magnet_pipe;
}BT_DATA_PIPE;
#endif
#endif

#ifdef __cplusplus
}
#endif


#endif
