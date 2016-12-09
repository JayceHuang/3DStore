#include "ptl_callback_func.h"
#include "utility/sd_assert.h"
#include "utility/errcode.h"

_int32 ptl_connect_callback(_int32 errcode, PTL_DEVICE* device)
{
	if(errcode != SUCCESS && errcode != ERR_PTL_TRY_UDT_CONNECT)	
		errcode = ERR_P2P_CONNECT_FAILED;		//如果不是连接成功或者换用udt连接，那么统一认为所有错误都是连接失败
	sd_assert(device != NULL);
	if( device == NULL ) return SUCCESS;
	if( device->_table== NULL ) return SUCCESS;
	sd_assert(device->_table[0] != NULL);
	if( device->_table[0] == NULL ) return SUCCESS;
	return ((ptl_device_connect_callback)device->_table[0])(errcode, device);
}

_int32 ptl_send_callback(_int32 errcode, PTL_DEVICE* device, _u32 had_send)
{
	return ((ptl_device_send_callback)device->_table[1])(errcode, device, had_send);
}

_int32 ptl_recv_cmd_callback(_int32 errcode, PTL_DEVICE* device, _u32 len)
{
	return ((ptl_device_recv_cmd_callback)device->_table[2])(errcode, device, len);
}

_int32 ptl_recv_data_callback(_int32 errcode, PTL_DEVICE* device, _u32 len)
{
	return ((ptl_device_recv_data_callback)device->_table[3])(errcode, device, len);
}

_int32 ptl_recv_discard_data_callback(_int32 errcode, PTL_DEVICE* device, _u32 len)
{
	return ((ptl_device_recv_discard_data_callback)device->_table[4])(errcode, device, len);
}

_int32 ptl_notify_close_device(PTL_DEVICE* device)
{
	return ((ptl_device_notify_close)device->_table[5])(device);
}

_int32	ptl_udt_recv_callback(_int32 errcode, RECV_TYPE type, PTL_DEVICE* device, _u32 len)
{
	switch(type)
	{
	case RECV_CMD_TYPE:
		{
			ptl_recv_cmd_callback(errcode, device, len);
			break;
		}
	case RECV_DATA_TYPE:
		{
			ptl_recv_data_callback(errcode, device, len);
			break;
		}
	case RECV_DISCARD_DATA_TYPE:
		{
			ptl_recv_discard_data_callback(errcode, device, len);
			break;
		}
	}
	return SUCCESS;
}


