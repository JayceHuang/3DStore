#include "utility/errcode.h"
#include "utility/string.h"

#include "udt_cmd_sender.h"

#include "udt_cmd_builder.h"
#include "udt_cmd_define.h"
#include "udt_utility.h"
#include "../ptl_udp_device.h"
#include "../ptl_cmd_define.h"

_int32 udt_send_syn_cmd(BOOL is_syn_ack, _u32 seq, _u32 ack, _u32 wnd, _u32 source_port, _u32 target_port, _u32 ip, _u16 udp_port)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL;
	_u32 len = 0;
	SYN_CMD cmd;
	sd_memset(&cmd, 0, sizeof(SYN_CMD));
	cmd._version = PTL_PROTOCOL_VERSION;
	cmd._cmd_type = SYN;
	if(is_syn_ack == FALSE)
		cmd._flags = 0x0;
	else
		cmd._flags = 0x1;
	cmd._source_port = source_port;
	cmd._target_port = target_port;
	cmd._peerid_hashcode = udt_local_peerid_hashcode();
	cmd._seq_num = seq;
	cmd._ack_num = ack;
	cmd._window_size = wnd;
	cmd._udt_version = 0;
	ret = udt_build_syn_cmd(&buffer, &len, &cmd);
	if(ret != SUCCESS)
		return ret;
	ret = ptl_udp_sendto(buffer, len, ip, udp_port);
	CHECK_VALUE(ret);
	return ret;
}
