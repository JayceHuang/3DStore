/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/10/23
-----------------------------------------------------------------------------------------------------------*/
#ifndef _PTL_CALLBACK_FUNC_H_
#define _PTL_CALLBACK_FUNC_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "p2p_transfer_layer/udt/udt_device.h"
#include "utility/define.h"

struct tagPTL_DEVICE;

typedef _int32 (*ptl_device_connect_callback)(_int32 errcode, struct tagPTL_DEVICE* device);
typedef _int32 (*ptl_device_send_callback)(_int32 errcode, struct tagPTL_DEVICE* device, _u32 had_send);
typedef _int32 (*ptl_device_recv_cmd_callback)(_int32 errcode, struct tagPTL_DEVICE* device, _u32 len);
typedef _int32 (*ptl_device_recv_data_callback)(_int32 errcode, struct tagPTL_DEVICE* device, _u32 len);
typedef _int32 (*ptl_device_recv_discard_data_callback)(_int32 errcode, struct tagPTL_DEVICE* device, _u32 len);
typedef _int32 (*ptl_device_notify_close)(struct tagPTL_DEVICE* device);

_int32	ptl_connect_callback(_int32 errcode, struct tagPTL_DEVICE* device);

_int32	ptl_send_callback(_int32 errcode, struct tagPTL_DEVICE* device, _u32 had_send);

_int32	ptl_recv_cmd_callback(_int32 errcode, struct tagPTL_DEVICE* device, _u32 len);

_int32	ptl_recv_data_callback(_int32 errcode, struct tagPTL_DEVICE* device, _u32 len);

_int32	ptl_recv_discard_data_callback(_int32 errcode, struct tagPTL_DEVICE* device, _u32 len);

_int32	ptl_udt_recv_callback(_int32 errcode, RECV_TYPE type, struct tagPTL_DEVICE* device, _u32 len);

_int32	ptl_notify_close_device(struct tagPTL_DEVICE* device);
#ifdef __cplusplus
}
#endif
#endif
