#include "utility/mempool.h"
#include "utility/md5.h"
#include "utility/aes.h"
#include "utility/utility.h"
#include "utility/bytebuffer.h"
#include "utility/logid.h"
#define LOGID LOGID_REPORTER
#include "utility/logger.h"

#include "reporter_cmd_handler.h"
#include "res_query/res_query_interface.h"
#include "res_query/res_query_cmd_handler.h"
#include "reporter_cmd_define.h"
#include "reporter_cmd_extractor.h"

#include "upload_manager/upload_manager.h"


_int32 reporter_handle_recv_resp_cmd(char* buffer, _u32 len, REPORTER_DEVICE* device)
{
	_u16 cmd_type = 0;
	_int32 tmp_len = len;
	char* cmd_ptr =NULL;
	_int32 ret = SUCCESS;
    _u8 cmd_type_u8 = 0;
#ifdef EMBED_REPORT
    _u32 cmd_type_tmp = 0;
#endif
	LOG_DEBUG("reporter_handle_recv_resp_cmd...,  device->_type:%d ", device->_type);

#ifdef REPORTER_DEBUG
	{
		char test_buffer[1024];
		sd_memset(test_buffer,0,1024);
		str2hex( buffer, len, test_buffer, 1024);
		LOG_DEBUG( "reporter_handle_recv_resp_cmd:buffer[%u]=%s" 
					,len, test_buffer);
	}
#endif /* _DEBUG */
		

	if((device->_type == REPORTER_SHUB)||(device->_type == REPORTER_STAT_HUB)
#ifdef ENABLE_BT
		||(device->_type == REPORTER_BT_HUB)
#endif
#ifdef ENABLE_EMULE
        ||(device->_type == REPORTER_EMULE_HUB)
#endif

	)
	{
		ret = xl_aes_decrypt(buffer, &len);
		CHECK_VALUE(ret);
		tmp_len = len;



		_int32 shub_ver = 0;
		cmd_ptr = buffer;
		sd_get_int32_from_lt((char**)&(cmd_ptr), &tmp_len,  &shub_ver);	

		if(shub_ver >= 60)
		{
			dump_buffer(buffer, len);
			tmp_len = len;
			cmd_ptr = buffer + REPORTER_CMD_HEADER_LEN + 6;
			tmp_len -= (REPORTER_CMD_HEADER_LEN + 6);
			_int32 reserver_length = 0;
			sd_get_int32_from_lt((char**)&(cmd_ptr), &tmp_len,  &reserver_length);
			tmp_len -= reserver_length;
			cmd_ptr += reserver_length;
			sd_get_int16_from_lt((char**)&(cmd_ptr), &tmp_len, (_int16*)&cmd_type);	
		}
		else
		{
			tmp_len = len;
			cmd_ptr = buffer + REPORTER_CMD_HEADER_LEN + 6;
			tmp_len -= (REPORTER_CMD_HEADER_LEN + 6);
       		sd_get_int16_from_lt((char**)&(cmd_ptr), &tmp_len, (_int16*)&cmd_type);		
       	}
		
	}
	else
#ifdef UPLOAD_ENABLE
	if(device->_type == REPORTER_PHUB)
	{	
		ret = xl_aes_decrypt(buffer, &len);
		CHECK_VALUE(ret);
		// cmd_type = *(_u8*)(buffer + REPORTER_CMD_HEADER_LEN); 
		//cmd_type = *(_u16*)(buffer + REPORTER_CMD_HEADER_LEN + 6);
		cmd_ptr =(buffer + REPORTER_CMD_HEADER_LEN );
		tmp_len -= (REPORTER_CMD_HEADER_LEN);
		sd_get_int8(&(cmd_ptr), &tmp_len, (_int8*)&cmd_type_u8);
        cmd_type = (_u16)cmd_type_u8;
	}
	else
#endif
	if(device->_type == REPORTER_LICENSE )
	{
		//cmd_type = *(_u8*)(buffer + REPORTER_CMD_HEADER_LEN ); 
		cmd_ptr =(buffer + REPORTER_CMD_HEADER_LEN );
		tmp_len -= (REPORTER_CMD_HEADER_LEN);
		sd_get_int8(&(cmd_ptr), &tmp_len, (_int8*)&cmd_type_u8);
        cmd_type = (_u16)cmd_type_u8;
		//cmd_ptr = (buffer + REPORTER_CMD_HEADER_LEN );
	//	tmp_len -=  REPORTER_CMD_HEADER_LEN;
		// sd_get_int8(&(cmd_ptr), &tmp_len, (_int8*)&cmd_type);
	}
	
#ifdef EMBED_REPORT
	else if( device->_type == REPORTER_EMBED )
	{
		//cmd_type = *(_u32*)(buffer + REPORTER_CMD_HEADER_LEN );
		cmd_ptr =(buffer + REPORTER_CMD_HEADER_LEN );
		tmp_len -= (REPORTER_CMD_HEADER_LEN);
		sd_get_int32_from_lt(&(cmd_ptr), &tmp_len, (_int32*)&cmd_type_tmp);
        cmd_type = (_u16)cmd_type_tmp;
	}
#endif	

	else
	{
#ifdef _DEBUG
					CHECK_VALUE(1);
#endif
		return REPORTER_ERR_UNKNOWN_DEVICE_TYPE;
	}
	
	LOG_DEBUG("reporter_handle_recv_resp_cmd, command type is %d", (_int32)cmd_type);
	switch(cmd_type)
	{
	case REPORT_LICENSE_RESP_CMD_TYPE:
		return reporter_handle_report_license_resp(buffer, len, device->_last_cmd);
		break;
	case REPORT_DW_STAT_RESP_CMD_TYPE:
	case REPORT_DW_FAIL_RESP_CMD_TYPE:
	case REPORT_INSERTSRES_RESP_CMD_TYPE:
#ifdef ENABLE_BT
	case REPORT_BT_TASK_DL_STAT_RESP_CMD_TYPE:
	case REPORT_BT_DL_STAT_RESP_CMD_TYPE:
	case REPORT_BT_INSERT_RES_RESP_CMD_TYPE:
#endif /* ENABLE_BT */
#ifdef ENABLE_EMULE
	case REPORT_EMULE_INSERTSRES_RESP_CMD_TYPE:
    case REPORT_EMULE_DL_STAT_RESP_CMD_TYPE:
#endif /* ENABLE_EMULE */
#ifdef UPLOAD_ENABLE
	case EMB_REPORT_UPLOAD_STAT_RESP_CMD_TYPE:
#endif		
		return reporter_handle_report_dw_resp(buffer, len, device->_last_cmd);
		break;
#ifdef UPLOAD_ENABLE
	case REPORT_ISRC_ONLINE_RESP_CMD_TYPE:
	case REPORT_RC_LIST_RESP_CMD_TYPE:
	case REPORT_INSERT_RC_RESP_CMD_TYPE:
	case REPORT_DELETE_RC_RESP_CMD_TYPE:
		return reporter_handle_rc_resp(buffer, len, device->_last_cmd);
		break;
#endif

#ifdef EMBED_REPORT

	case EMB_REPORT_COMMON_TASK_DL_STAT_RESP_CMD_TYPE:
	case EMB_REPORT_BT_TASK_DL_STAT_RESP_CMD_TYPE:
	case EMB_REPORT_BT_FILE_DL_STAT_RESP_CMD_TYPE:
	case EMB_REPORT_COMMON_TASK_DL_FAIL_RESP_CMD_TYPE:
	case EMB_REPORT_BT_FILE_DL_FAIL_RESP_CMD_TYPE:
	case EMB_REPORT_DNS_ABNORMAL_RESP_CMD_TYPE:
	case EMB_REPORT_COMMON_STOP_TASK_RESP_CMD_TYPE:
	case EMB_REPORT_BT_STOP_TASK_RESP_CMD_TYPE:
	case EMB_REPORT_BT_STOP_FILE_RESP_CMD_TYPE:
    case EMB_REPORT_EMULE_STOP_TASK_RESP_CMD_TYPE:

#if defined(MOBILE_PHONE)
	case EMB_REPORT_MOBILE_USER_ACTION_RESP_CMD_TYPE:
	case EMB_REPORT_MOBILE_NETWORK_RESP_CMD_TYPE:
#endif
		return reporter_handle_emb_common_resp(buffer, len, device->_last_cmd);
		break;
	case EMB_REPORT_ONLINE_PEER_STATE_RESP_CMD_TYPE:
		return reporter_handle_emb_online_peer_stat_resp(buffer, len, device->_last_cmd);
		break;
#endif

	default:
		LOG_ERROR("reporter_handle_recv_resp_cmd, but cmd_type is invalid...");
		//sd_assert(FALSE);
#ifdef _DEBUG
					//CHECK_VALUE(1);
#endif
		return REPORTER_ERR_UNKNOWN_COMMAND_TYPE;
	
	}
}
_int32 reporter_handle_report_license_resp(char* buffer, _u32 len, REPORTER_CMD* last_cmd)
{


	REPORT_LICENSE_RESP_CMD resp_cmd;

	LOG_DEBUG("reporter_handle_report_license_resp...");

	sd_memset(&resp_cmd, 0, sizeof(REPORT_LICENSE_RESP_CMD));
	if(reporter_extract_report_license_resp_cmd(buffer, len, &resp_cmd) != SUCCESS)
	{
		LOG_ERROR("FATAL ERROR!reporter_extract_report_license_resp_cmd!=SUCCESS");
		//sd_assert(FALSE);		

	}
	else if(last_cmd->_cmd_seq == resp_cmd._header._seq)
	{
		LOG_DEBUG("last_cmd->_cmd_seq == resp_cmd._header._seq=%u",last_cmd->_cmd_seq);
	}
	else
	{
		LOG_ERROR("FATAL ERROR!last_cmd->_cmd_seq(%u) != resp_cmd._header._seq(%u)",last_cmd->_cmd_seq , resp_cmd._header._seq);
		//sd_assert(FALSE);		/*last_cmd->_cmd_seq != resp_cmd._header._seq command not correspond*/
	}
	return SUCCESS;


}


_int32 reporter_handle_report_dw_resp(char* buffer, _u32 len, REPORTER_CMD* last_cmd)
{


	REPORT_DW_RESP_CMD resp_cmd;

	LOG_DEBUG("reporter_handle_report_dw_resp...");
	if(last_cmd->_user_data == NULL)		
	{
		return SUCCESS;		/*command had been cancel*/
	}
	sd_memset(&resp_cmd, 0, sizeof(REPORT_DW_RESP_CMD));
	if(reporter_extract_report_dw_resp_cmd(buffer, len, &resp_cmd) != SUCCESS)
	{
		LOG_ERROR("FATAL ERROR!reporter_extract_report_dw_resp_cmd != SUCCESS");
		//sd_assert(FALSE);		

	}
	else if(last_cmd->_cmd_seq == resp_cmd._header._seq)
	{
		LOG_DEBUG("last_cmd->_cmd_seq == resp_cmd._header._seq=%u",last_cmd->_cmd_seq);
#ifdef UPLOAD_ENABLE
		if(EMB_REPORT_UPLOAD_STAT_RESP_CMD_TYPE == resp_cmd._cmd_type)
		{
			ulm_stat_handle_report_response(resp_cmd._header._seq,resp_cmd._result);
		}
#endif		
		
	}
	else
	{
		LOG_ERROR("FATAL ERROR!last_cmd->_cmd_seq(%u) != resp_cmd._header._seq(%u)",last_cmd->_cmd_seq , resp_cmd._header._seq);
		//sd_assert(FALSE);		/*last_cmd->_cmd_seq != resp_cmd._header._seq command not correspond*/
	}
	return SUCCESS;


}

#ifdef UPLOAD_ENABLE
_int32 reporter_handle_rc_resp(char* buffer, _u32 len, REPORTER_CMD* last_cmd)
{
	_u32 cmd_seq = 0;
	LOG_DEBUG("reporter_handle_rc_resp...");

	if(reporter_extract_rc_resp_cmd(buffer, len,&cmd_seq) != SUCCESS)
	{
		LOG_ERROR("FATAL ERROR!reporter_handle_rc_resp!=SUCCESS");
		//sd_assert(FALSE);		

	}
	else if(last_cmd->_cmd_seq == cmd_seq)
	{
		LOG_DEBUG("last_cmd->_cmd_seq == resp_cmd._header._seq=%u",last_cmd->_cmd_seq);
	}
	else
	{
		LOG_ERROR("FATAL ERROR!last_cmd->_cmd_seq(%u) != resp_cmd._header._seq(%u)",last_cmd->_cmd_seq , cmd_seq);
		//sd_assert(FALSE);		/*last_cmd->_cmd_seq != resp_cmd._header._seq command not correspond*/
	}
	return SUCCESS;


}
#endif


#ifdef EMBED_REPORT

_int32 reporter_handle_emb_common_resp(char* buffer, _u32 len, REPORTER_CMD* last_cmd)
{
	EMB_COMMON_RESP_CMD resp_cmd;

	LOG_DEBUG("reporter_handle_emb_common_resp...");

	sd_memset(&resp_cmd, 0, sizeof(EMB_COMMON_RESP_CMD));
	if(reporter_extract_emb_common_resp_cmd(buffer, len, &resp_cmd) != SUCCESS)
	{
		LOG_ERROR("FATAL ERROR!reporter_handle_emb_common_resp != SUCCESS");
		//sd_assert(FALSE);		

	}
	else if(last_cmd->_cmd_seq == resp_cmd._header._seq)
	{
		LOG_DEBUG("last_cmd->_cmd_seq == resp_cmd._header._seq=%u",last_cmd->_cmd_seq);
	}
	else
	{
		LOG_ERROR("FATAL ERROR!last_cmd->_cmd_seq(%u) != resp_cmd._header._seq(%u)",last_cmd->_cmd_seq , resp_cmd._header._seq);
		//sd_assert(FALSE);		/*last_cmd->_cmd_seq != resp_cmd._header._seq command not correspond*/
	}
	return SUCCESS;
}

_int32 reporter_handle_emb_online_peer_stat_resp(char* buffer, _u32 len, REPORTER_CMD* last_cmd)
{
	EMB_ONLINE_PEER_STAT_RESP_CMD resp_cmd;

	LOG_DEBUG("reporter_handle_emb_online_peer_stat_resp...");

	sd_memset(&resp_cmd, 0, sizeof(EMB_ONLINE_PEER_STAT_RESP_CMD));
	if(reporter_extract_emb_online_peer_stat_resp_cmd(buffer, len, &resp_cmd) != SUCCESS)
	{
		LOG_ERROR("FATAL ERROR!reporter_handle_emb_common_resp != SUCCESS");
		//sd_assert(FALSE);		

	}
	else if(last_cmd->_cmd_seq == resp_cmd._header._seq)
	{
		LOG_DEBUG("last_cmd->_cmd_seq == resp_cmd._header._seq=%u",last_cmd->_cmd_seq);
	}
	else
	{
		LOG_ERROR("FATAL ERROR!last_cmd->_cmd_seq(%u) != resp_cmd._header._seq(%u)",last_cmd->_cmd_seq , resp_cmd._header._seq);
		//sd_assert(FALSE);		/*last_cmd->_cmd_seq != resp_cmd._header._seq command not correspond*/
	}
	return SUCCESS;
}
#endif
