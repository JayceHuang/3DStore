#include "udt_cmd_builder.h"
#include "udt_cmd_define.h"
#include "utility/bytebuffer.h"
#include "utility/errcode.h"
#include "utility/mempool.h"
#include "utility/logid.h"
#define	LOGID	LOGID_P2P_TRANSFER_LAYER
#include "utility/logger.h"


_int32 udt_build_syn_cmd(char** buffer, _u32* len, SYN_CMD* cmd)
{
	_int32	ret;
	char*	tmp_buf;
	_int32	tmp_len;
	*len = 33;
	ret = sd_malloc(*len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("udt_build_p2p_syn_cmd failed, errcode = %d", ret);
		return ret;
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_version);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_cmd_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_flags);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_source_port);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_target_port);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_hashcode);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_seq_num);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_ack_num);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_window_size);
	ret = sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_udt_version);
	sd_assert(ret == SUCCESS);
	sd_assert(tmp_len == 0);
	if(ret != SUCCESS)
	{
		LOG_ERROR("udt_build_p2p_syn_cmd falied, errcode = %d", ret);
		sd_free(*buffer);
		*buffer = NULL;
	}
	return ret;
}



