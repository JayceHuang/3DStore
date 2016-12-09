#include "reporter/reporter_cmd_extractor.h"
#include "utility/bytebuffer.h"
#include "utility/utility.h"
#include "connect_manager/connect_manager_interface.h"
#ifdef ENABLE_BT
#include "bt_download/bt_task/bt_task_interface.h"
#endif

#ifdef ENABLE_EMULE
#include "emule/emule_interface.h"
#endif

#include "task_manager/task_manager.h"
#ifdef UPLOAD_ENABLE
#include "upload_manager/upload_manager.h"
#endif

#include "reporter_setting.h"

#include "utility/logid.h"
#define LOGID	LOGID_REPORTER
#include "utility/logger.h"

#include "vod_data_manager/vod_data_manager_interface.h"

#include "reporter_impl.h"

_int32 reporter_extract_report_license_resp_cmd(char* buffer, _u32 len, REPORT_LICENSE_RESP_CMD* cmd)
{
	char* tmp_buf = buffer;
	_int32  tmp_len = (_int32)len /* ,ret_val=SUCCESS */ ;
	_int32 report_interval = DEFAULT_LICENSE_REPORT_INTERVAL,expire_time = 0,rule = 0,err_code=0;
	_int32 para_name_len =0,i=0;
	char para_name_buffer[256];

	
	LOG_DEBUG("reporter_extract_report_license_resp_cmd");
#ifdef REPORTER_DEBUG
		{
			char test_buffer[1024];
			sd_memset(test_buffer,0,1024);
			str2hex( buffer, len, test_buffer, 1024);
			LOG_DEBUG( "reporter_extract_report_license_resp_cmd:buffer[%u]=%s" 
						,len, test_buffer);
		}
#endif /* _DEBUG */

	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._version);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._seq);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._cmd_len);
	//sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_client_version);
	//sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_compress);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_cmd_type);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_result);

	switch(cmd->_result)
	{
		case LICENSE_OK:
			LOG_DEBUG("The license is ok [version=%u]!", cmd->_header._version);
		break;	
		case LICENSE_FREEZE:
			LOG_DEBUG("The license is frozen [version=%u,result=%d]!", cmd->_header._version,cmd->_result);
		break;	
		case LICENSE_EXPIRE:
			LOG_DEBUG("The license is expired [version=%u,result=%d]!", cmd->_header._version,cmd->_result);
		break;	
		case LICENSE_RECLAIM:
			LOG_DEBUG("The license is reclaimed [version=%u,result=%d]!", cmd->_header._version,cmd->_result);
		break;
		case LICENSE_USED:
			LOG_DEBUG("The license is held by another client [version=%u,result=%d]!", cmd->_header._version,cmd->_result);
		break;
		case LICENSE_SERVER_BUSY:
			LOG_DEBUG("The license server is busy now,please retry later ![version=%u,result=%d]!", cmd->_header._version,cmd->_result);
		break;
		case LICENSE_INVALID:
		default:
			LOG_DEBUG("The license is a invalid license [version=%u,result=%d]!", cmd->_header._version,cmd->_result);
	}

	if(cmd->_result!=LICENSE_SERVER_BUSY)
	{
	/*  _items_count  */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_items_count);
	if(cmd->_items_count < 3)
	{
		LOG_ERROR("ERROR: failed, [version = %u] cmd->_items_count = %u", cmd->_header._version, cmd->_items_count);
		err_code = REPORTER_ERR_INVALID_ITEMS_COUNT;
		goto ErrHandler; 
	}

	for(i=0;i<cmd->_items_count ;i++)
	{
		para_name_len=0;
		sd_get_int32_from_lt(&tmp_buf, &tmp_len,&para_name_len);
		if(para_name_len!=0)
		{
			sd_memset(para_name_buffer, 0, 256);
			sd_get_bytes(&tmp_buf, &tmp_len, para_name_buffer, para_name_len);
			/*  rule  */
			if( sd_strcmp("rule",para_name_buffer)==0)
			{
				sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->rule_len);
				if((cmd->rule_len == 0)||(cmd->rule_len > 16))
				{
					LOG_ERROR("ERROR: failed, [version = %u] cmd->rule_len = %u", cmd->_header._version, cmd->rule_len);
					err_code = REPORTER_ERR_INVALID_RULE_LEN;
					goto ErrHandler; 
				}
				sd_get_bytes(&tmp_buf, &tmp_len, (char*)cmd->rule, cmd->rule_len);

				rule = sd_atoi(cmd->rule);
			}
			else /*  expire_time  */
			if( sd_strcmp("expire_time",para_name_buffer)==0)
			{
				sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->expire_time_len);
				if((cmd->expire_time_len == 0)||(cmd->expire_time_len > 16))
				{
					LOG_ERROR("ERROR: failed, [version = %u] cmd->expire_time_len = %u", cmd->_header._version, cmd->expire_time_len);
					err_code = REPORTER_ERR_INVALID_EXPIRE_TIME_LEN;
					goto ErrHandler; 
				}
				sd_get_bytes(&tmp_buf, &tmp_len, (char*)cmd->expire_time, cmd->expire_time_len);

				expire_time = sd_atoi(cmd->expire_time);
			}
			else  	/*  report_interval  */
			if( sd_strcmp("report_interval",para_name_buffer)==0)
			{
				sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->report_interval_len);
				if((cmd->report_interval_len == 0)||(cmd->report_interval_len > 16))
				{
					LOG_ERROR("ERROR: failed, [version = %u] cmd->report_interval_len = %u", cmd->_header._version, cmd->report_interval_len);
					err_code = REPORTER_ERR_INVALID_INTERVAL_LEN;
					goto ErrHandler; 
				}
				sd_get_bytes(&tmp_buf, &tmp_len, (char*)cmd->report_interval, cmd->report_interval_len);

				report_interval = sd_atoi(cmd->report_interval);
			}
			else
			{
				para_name_len=0;
				sd_get_int32_from_lt(&tmp_buf, &tmp_len, &para_name_len);
				if(para_name_len!=0)
				{
					sd_memset(para_name_buffer, 0, 256);
					sd_get_bytes(&tmp_buf, &tmp_len, para_name_buffer, para_name_len);
					LOG_DEBUG("WARNING: unknown parameter [version = %u] para_name = %s", cmd->_header._version, para_name_buffer);
				}
			}
		}
	}
	}
	tm_notify_license_report_result(cmd->_result,   report_interval,  expire_time);

	if((rule&LICENSE_DISABLE_P2S)!=0)
	 {
	 	cm_p2s_set_enable(FALSE);
	 }
	 else
	 {
	 	cm_p2s_set_enable(TRUE);
	 }

     if((rule&LICENSE_DISABLE_VOD)!=0)
      {
        LOG_DEBUG("reporter_vdm_disable_vod, rule:%d", rule);
         vdm_enable_vod(FALSE);
      }
      else
      {
        LOG_DEBUG("reporter_vdm_enable_vod, rule:%d", rule);
         vdm_enable_vod(TRUE);
      }


	 if((rule&LICENSE_DISABLE_P2P)!=0)
	 {
	 	cm_p2p_set_enable(FALSE);
	 }
	 else
	 {
	 	cm_p2p_set_enable(TRUE);
	 }

#ifdef ENABLE_BT
	 if((rule&LICENSE_DISABLE_BT)!=0)
	 {
	 	bt_set_enable(FALSE);
	 }
	 else
	 {
	 	bt_set_enable(TRUE);
	 }
#endif /* #ifdef ENABLE_BT */

#ifdef ENABLE_EMULE
	 if((rule&LICENSE_DISABLE_EMULE)!=0)
	 {
	 	emule_set_enable(FALSE);
	 }
	 else
	 {
	 	emule_set_enable(TRUE);
	 }
#endif /* #ifdef ENABLE_BT */

/*
	 if((rule&LICENSE_DISABLE_EMULE)!=0)
	 {
	 	cm_emule_set_enable(FALSE);
	 }
	 else
	 {
	 	cm_emule_set_enable(TRUE);
	 }
*/
	LOG_DEBUG("report_license_response: [version=%u,result=%u,report_interval=%u,expire_time=%u,rule=%d]!", cmd->_header._version,cmd->_result,(_u32)report_interval,(_u32)expire_time,rule);
	return SUCCESS;
	
ErrHandler:
	tm_notify_license_report_result(-1,   err_code,  0);
	return SUCCESS;
		
}


_int32 reporter_extract_report_dw_resp_cmd(char* buffer, _u32 len, REPORT_DW_RESP_CMD* cmd)
{
	char* tmp_buf = buffer;
	_int32  tmp_len = (_int32)len;

	LOG_DEBUG("reporter_extract_report_dw_resp_cmd");
#ifdef REPORTER_DEBUG
		{
			char test_buffer[1024];
			sd_memset(test_buffer,0,1024);
			str2hex( buffer, len, test_buffer, 1024);
			LOG_DEBUG( "reporter_extract_report_dw_resp_cmd:buffer[%u]=%s" 
						,len, test_buffer);
		}
#endif /* _DEBUG */
	
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._version);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._seq);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._cmd_len);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_client_version);
	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_compress);
	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_cmd_type);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_result);
	if(cmd->_result == 0)	
	{	
		LOG_DEBUG("The dw report is not accepted by shub[version=%u,_seq=%u,_cmd_type=%d]!", cmd->_header._version,cmd->_header._seq,(_int32)cmd->_cmd_type);
	}
	else
	{
		LOG_DEBUG("The dw report is accepted by shub[version=%u,_seq=%u,_cmd_type=%d]!", cmd->_header._version,cmd->_header._seq,(_int32)cmd->_cmd_type);
	}

	return SUCCESS;
}

#ifdef UPLOAD_ENABLE
_int32 reporter_extract_rc_resp_cmd(char* buffer, _u32 len,_u32* cmd_seq)
{
	char* tmp_buf = buffer;
	_int32  tmp_len = (_int32)len /* ,ret_val=SUCCESS */ ;
	REPORT_RC_RESP_CMD resp_cmd;
	
	LOG_DEBUG("reporter_extract_rc_resp_cmd");
#ifdef REPORTER_DEBUG
		{
			char test_buffer[1024];
			sd_memset(test_buffer,0,1024);
			str2hex( buffer, len, test_buffer, 1024);
			LOG_DEBUG( "reporter_extract_rc_resp_cmd:buffer[%u]=%s" 
						,len, test_buffer);
		}
#endif /* _DEBUG */
	sd_memset(&resp_cmd, 0, sizeof(REPORT_RC_RESP_CMD));
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&resp_cmd._header._version);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&resp_cmd._header._seq);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&resp_cmd._header._cmd_len);
	//sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&resp_cmd._client_version);
	//sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&resp_cmd._compress);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&resp_cmd._cmd_type);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&resp_cmd._result);

	switch(resp_cmd._cmd_type)
	{
		case REPORT_ISRC_ONLINE_RESP_CMD_TYPE:
			sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&resp_cmd.should_report_rclist);
			LOG_DEBUG("REPORT_ISRC_ONLINE_RESP_CMD_TYPE ,_result = %d,should_report_rclist=%d [version=%u]!",resp_cmd._result,resp_cmd.should_report_rclist, resp_cmd._header._version);
			ulm_isrc_online_resp((resp_cmd._result==0 ? TRUE : FALSE), resp_cmd.should_report_rclist);
		break;	
		case REPORT_RC_LIST_RESP_CMD_TYPE:
			LOG_DEBUG("REPORT_RC_LIST_RESP_CMD_TYPE ,_result = %d[version=%u]!",resp_cmd._result,resp_cmd._header._version);
			ulm_report_rclist_resp((resp_cmd._result==0 ? TRUE : FALSE));
		break;	
		case REPORT_INSERT_RC_RESP_CMD_TYPE:
			LOG_DEBUG("REPORT_INSERT_RC_RESP_CMD_TYPE ,_result = %d[version=%u]!",resp_cmd._result,resp_cmd._header._version);
			ulm_insert_rc_resp((resp_cmd._result==0 ? TRUE : FALSE));
		break;	
		case REPORT_DELETE_RC_RESP_CMD_TYPE:
			LOG_DEBUG("REPORT_DELETE_RC_RESP_CMD_TYPE ,_result = %d[version=%u]!",resp_cmd._result,resp_cmd._header._version);
			ulm_delete_rc_resp((resp_cmd._result==0 ? TRUE : FALSE));
		break;
		default:
			LOG_DEBUG("UNKNOWN RESPONSE [_cmd_type=%d,version=%u,result=%d]!",resp_cmd._cmd_type,resp_cmd._header._version,resp_cmd._result);
	}

	*cmd_seq = resp_cmd._header._seq;
	return SUCCESS;
			
}
#endif

#ifdef EMBED_REPORT

_int32 reporter_extract_emb_common_resp_cmd(char* buffer, _u32 len, EMB_COMMON_RESP_CMD* cmd)
{
	char* tmp_buf = buffer;
	_int32  tmp_len = (_int32)len;

	LOG_DEBUG("reporter_extract_emb_common_resp_cmd");
#ifdef REPORTER_DEBUG
		{
			char test_buffer[1024];
			sd_memset(test_buffer,0,1024);
			str2hex( buffer, len, test_buffer, 1024);
			LOG_DEBUG( "reporter_extract_emb_common_resp_cmd:buffer[%u]=%s" 
						,len, test_buffer);
		}
#endif /* _DEBUG */
	
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._version);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._seq);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._cmd_len);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_cmd_type);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_result);

	if(cmd->_result == 0)	
	{	
		LOG_DEBUG("The emb_common_resp report is not accepted[version=%u,_seq=%u,_cmd_type=%d]!", cmd->_header._version,cmd->_header._seq,(_int32)cmd->_cmd_type);
	}
	else
	{
		LOG_DEBUG("The emb_common_resp report is accepted [version=%u,_seq=%u,_cmd_type=%d]!", cmd->_header._version,cmd->_header._seq,(_int32)cmd->_cmd_type);
	}

	return SUCCESS;
}


_int32 reporter_extract_emb_online_peer_stat_resp_cmd(char* buffer, _u32 len, EMB_ONLINE_PEER_STAT_RESP_CMD* cmd)
{
	char* tmp_buf = buffer;
	_int32  tmp_len = (_int32)len;
	REPORTER_SETTING* setting =NULL;

	LOG_DEBUG("reporter_extract_emb_online_peer_stat_resp_cmd");
#ifdef REPORTER_DEBUG
		{
			char test_buffer[1024];
			sd_memset(test_buffer,0,1024);
			str2hex( buffer, len, test_buffer, 1024);
			LOG_DEBUG( "reporter_extract_emb_online_peer_stat_resp_cmd:buffer[%u]=%s" 
						,len, test_buffer);
		}
#endif /* _DEBUG */
	
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._version);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._seq);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._cmd_len);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_cmd_type);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_result);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_interval);

	if(cmd->_result == 0)	
	{	
		LOG_DEBUG("The emb_common_resp report is not accepted[version=%u,_seq=%u,_cmd_type=%d]!report_interval=%u,", cmd->_header._version,cmd->_header._seq,(_int32)cmd->_cmd_type,cmd->_interval );
	}
	else
	{
		LOG_DEBUG("The emb_common_resp report is accepted [version=%u,_seq=%u,_cmd_type=%d]!report_interval=%u", cmd->_header._version,cmd->_header._seq,(_int32)cmd->_cmd_type,cmd->_interval);
	}
	
	setting = get_reporter_setting();	

	setting->_online_peer_report_interval = cmd->_interval;
	reporter_init_timeout_event(TRUE);

	return SUCCESS;
}


#endif

