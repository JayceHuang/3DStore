#include "utility/mempool.h"
#include "utility/string.h"
#include "utility/bytebuffer.h"
#include "utility/md5.h"
#include "utility/aes.h"
#include "utility/utility.h"
#include "utility/define_const_num.h"
#include "reporter_setting.h"
#include "platform/sd_network.h"
#include "platform/sd_gz.h"
#include "utility/logid.h"
#define LOGID LOGID_REPORTER
#include "utility/logger.h"

#include "reporter_cmd_builder.h"
#include "reporter_device.h"
#ifdef UPLOAD_ENABLE
#include "upload_manager/rclist_manager.h"
#endif

#include "res_query/res_query_interface.h"
#include "res_query/res_query_cmd_builder.h"

	static _u32 seq = 0;
#ifdef UPLOAD_ENABLE
	static _u32 reporter_rc_seq = 0;
#endif

_int32 emb_reporter_package_stat_cmd(char** buffer, _u32* len);


_u32 reporter_get_seq()
{
	return seq++;
}

_int32 reporter_build_report_license_cmd(char** buffer, _u32* len, REPORT_LICENSE_CMD* cmd)
{
	static _u32 license_seq = 0;
	_int32 ret;
	char*	tmp_buf;
	_int32  tmp_len,fix_len=0,other_len=0;
	
#ifdef ADD_HTTP_HEADER_TO_LICENSE_REPORT
	REPORTER_SETTING *settings = NULL;
	char http_header[1024] = {0};
	_u32 http_header_len = 0;
#endif //ADD_HTTP_HEADER_TO_LICENSE_REPORT
	
	cmd->_header._version = REP_LICENSE_PROTOCOL_VER;
	cmd->_header._seq = license_seq++;

	/* begining from _cmd_type */
	fix_len=
	1+   //_u8	_cmd_type;
	4+   //_u32	_peerid_size;
	4+   //_u32	_items_count;
	4+   //_u32	partner_id_name_len;
	4+   // _u32	partner_id_len;
	4+   // _u32	product_flag_name_len;
	4+   // _u32	product_flag_len;
	4+   // _u32	license_name_len;
	4+   // _u32	license_len;
	4+   // _u32	ip_name_len;
	4+   // _u32	ip_len;
	4+   // _u32	os_name_len;
	4;   // _u32	os_len;

	other_len=
	cmd->_peerid_size+
	cmd->partner_id_name_len+
	cmd->partner_id_len+
	cmd->product_flag_name_len+
	cmd->product_flag_len+
	cmd->license_name_len+
	cmd->license_len+
	cmd->ip_name_len+
	cmd->ip_len+
	cmd->os_name_len+
	cmd->os_len;

	cmd->_header._cmd_len = fix_len+other_len;

	cmd->_cmd_type = REPORT_LICENSE_CMD_TYPE;
	
	*len = sizeof(REP_CMD_HEADER) + cmd->_header._cmd_len;

#ifdef ADD_HTTP_HEADER_TO_LICENSE_REPORT
	//modified by xietao @20110428,  build http header before the binary packet
	settings = get_reporter_setting();
	http_header_len = sd_snprintf(http_header, 1024, "POST http://%s:%u/ HTTP/1.1\r\nHost: %s:%u\r\nContent-Length: %u\r\nContent-Type: application/octet-stream\r\nConnection: Close\r\nUser-Agent: Mozilla/4.0\r\nAccept: */*\r\n\r\n",
				settings->_license_server_addr, settings->_license_server_port, settings->_license_server_addr, settings->_license_server_port,*len);
	*len += http_header_len;
#endif //ADD_HTTP_HEADER_TO_LICENSE_REPORT

	ret = sd_malloc(*len, (void**)buffer);  // why +16 ???  zyq mark 
	if(ret != SUCCESS)
	{
		LOG_DEBUG("reporter_build_report_license_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}

#ifdef ADD_HTTP_HEADER_TO_LICENSE_REPORT
	sd_memcpy(*buffer, http_header, http_header_len);
	LOG_DEBUG("reporter_build_report_license_cmd, http_header before cmd: %s, header_len: %u", http_header, http_header_len);
	tmp_buf = *buffer + http_header_len;
	tmp_len = (_int32)*len - http_header_len;
#else
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;
#endif //ADD_HTTP_HEADER_TO_LICENSE_REPORT

	//sd_set_bytes(&tmp_buf, &tmp_len, (char*)&cmd->_header, sizeof(REP_CMD_HEADER));
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);
	//sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_client_version);
	//sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_compress);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_cmd_type);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_items_count);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->partner_id_name_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->partner_id_name, (_int32)cmd->partner_id_name_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->partner_id_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->partner_id, (_int32)cmd->partner_id_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->product_flag_name_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->product_flag_name, (_int32)cmd->product_flag_name_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->product_flag_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->product_flag, (_int32)cmd->product_flag_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->license_name_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->license_name, (_int32)cmd->license_name_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->license_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->license, (_int32)cmd->license_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->ip_name_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->ip_name, (_int32)cmd->ip_name_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->ip_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->ip, (_int32)cmd->ip_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->os_name_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->os_name, (_int32)cmd->os_name_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->os_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->os, (_int32)cmd->os_len);


#ifdef REPORTER_DEBUG
		{
			char test_buffer[1024];
			sd_memset(test_buffer,0,1024);
			str2hex( (char*)(*buffer), *len, test_buffer, 1024);
			LOG_DEBUG( "reporter_build_report_license_cmd:*buffer[%u]=%s" 
						,((_u32)(*len)), test_buffer);
		}
#endif /* _DEBUG */
	
	//ret = aes_encrypt(*buffer, len);
	//CHECK_VALUE(ret);
	if(tmp_len != 0)
	{
		SAFE_DELETE(*buffer);
		return REPORTER_ERR_BUILD_LICENSE;
	}
	return SUCCESS;
}

_int32 reporter_build_report_dw_stat_cmd(char** buffer, _u32* len, REPORT_DW_STAT_CMD* cmd)
{
	_int32 ret;
	char*	tmp_buf;
	_int32  tmp_len,fix_len=0,other_len=0;
	char http_header[1024] = {0};
	_u32 http_header_len = 1024;
    _u32 encode_len = 0;
    
    cmd->_header._version = REP_DW_PROTOCOL_VER;
	cmd->_header._seq = seq++;

	/* begining from _client_version */
	fix_len=
	4+	2+	2+	4+	4+	4+	4+	4+	8+	8+	8+	8+	4+	4+	4+	1+	1+	1+	1+	1+	8+	8+	1+	1+	8+	8+	1+
	1+	8+	8+	1+	1+	8+	8+	1+	1+	8+	8+	4+	4+	4+	4+	8+	8+	4+	4+	4+	4+	4+	4+	1+	4+	4+	4+
	4+	4+	8+	8+	8+	8
#ifdef REP_DW_VER_54
	+4
#endif /* REP_DW_VER_54 */
	;

	other_len =	cmd->_peerid_size+	cmd->_cid_size+	cmd->_origin_url_size+	cmd->_refer_url_size+
		cmd->file_suffix_len+	cmd->thunder_version_len+	cmd->partner_id_len+	cmd->_redirect_url_size+
		cmd->_gcid_size+	cmd->_real_page_url_size+	cmd->_user_name_size+	cmd->_file_name_size
#ifdef REP_DW_VER_54
	+cmd->_task_info_size
#endif /* REP_DW_VER_54 */
	;

	cmd->_header._cmd_len = fix_len+other_len;
	cmd->_cmd_type = REPORT_DW_STAT_CMD_TYPE;
	*len = sizeof(REP_CMD_HEADER) + cmd->_header._cmd_len;

	//encode_len = *len+16;
	encode_len = (cmd->_header._cmd_len + 16) /16 * 16 + 12;
    
    ret = build_report_http_header(http_header, &http_header_len, encode_len,REPORTER_STAT_HUB);
	CHECK_VALUE(ret);
	ret = sd_malloc(*len + 16 + http_header_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("reporter_build_report_dw_stat_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	sd_memcpy(*buffer, http_header, http_header_len);
	tmp_buf = *buffer + http_header_len;
	tmp_len = (_int32)*len;

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_client_version);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_compress);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_cmd_type);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_cid, (_int32)cmd->_cid_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_origin_url_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_origin_url, (_int32)cmd->_origin_url_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_refer_url_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_refer_url, (_int32)cmd->_refer_url_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->url_flag);

	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_size);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->size_by_server);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->size_by_peer);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->size_by_other);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->fault_block_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->download_time);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->file_suffix_len); 
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->file_suffix, (_int32)cmd->file_suffix_len);

	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->total_server_res);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->valid_server_res);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->is_nated);

	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->total_N2I_peer_res);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->valid_N2I_peer_res);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->N2I_Download_bytes1);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->N2I_Download_bytes2);

	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->Total_N2N_peer_res);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->Valid_N2N_peer_res);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->N2N_Download_bytes1);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->N2N_Download_bytes2);

	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->Total_N2SameN_peer_res);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->Valid_N2SameN_peer_res);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->N2SameN_Download_bytes1);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->N2SameN_Download_bytes2);

	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->Total_I2I_peer_res);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->Valid_I2I_peer_res);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->I2I_Download_bytes1);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->I2I_Download_bytes2);

	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->Total_I2N_peer_res);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->Valid_I2N_peer_res);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->I2N_Download_bytes1);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->I2N_Download_bytes2);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->dw_comefrom);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->product_flag);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->thunder_version_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->thunder_version, (_int32)cmd->thunder_version_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->partner_id_len);  
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->partner_id, (_int32)cmd->partner_id_len);

	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->downloadbytes_from_seed_svr);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->savebytes_from_seed_svr);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_redirect_url_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_redirect_url, (_int32)cmd->_redirect_url_size);

	sd_set_bytes(&tmp_buf, &tmp_len, (char* )&cmd->download_ip, sizeof(_u32));
	sd_set_bytes(&tmp_buf, &tmp_len, (char* )&cmd->lan_ip, sizeof(_u32));

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->limit_upload);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->download_stat);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_gcid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_gcid, (_int32)cmd->_gcid_size);

	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->CDN_number);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_real_page_url_size);  
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_real_page_url, (_int32)cmd->_real_page_url_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->download_ur_info);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->recommend_download_stat);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_user_name_size);  
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_user_name, (_int32)cmd->_user_name_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_name_size);  
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_file_name, (_int32)cmd->_file_name_size);

	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->download_bytes_by_relay);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->download_bytes_from_DCache);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->bytes_by_bt);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->bytes_by_emule);

#ifdef REP_DW_VER_54
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_task_info_size);  
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_task_info, (_int32)cmd->_task_info_size);
#endif /* REP_DW_VER_54 */
#ifdef REPORTER_DEBUG
		{
			char test_buffer[1024];
			sd_memset(test_buffer,0,1024);
			str2hex( (char*)(*buffer+http_header_len), *len+16, test_buffer, 1024);
			LOG_DEBUG( "build_report_dw_stat_cmd:*buffer[%u]=%s" 
						,((_u32)(*len))+16, test_buffer);
		}
#endif /* _DEBUG */
		
	ret = xl_aes_encrypt(*buffer+http_header_len, len);
	if(ret != SUCCESS)
	{
		SAFE_DELETE(*buffer);
		CHECK_VALUE(ret);
	}
	
	if(tmp_len != 0)
	{
		SAFE_DELETE(*buffer);
		return REPORTER_ERR_BUILD_DW_STAT;
	}
    
    sd_assert( encode_len == *len);
    *len += http_header_len;
	return SUCCESS;
}

_int32 reporter_build_report_dw_fail_cmd(char** buffer, _u32* len, REPORT_DW_FAIL_CMD* cmd)
{
	//static _u32 seq = 0;
	_int32 ret;
	char*	tmp_buf;
	_int32  tmp_len,fix_len=0,other_len=0;
	char http_header[1024] = {0};
	_u32 http_header_len = 1024;
    _u32 encode_len = 0;

    cmd->_header._version = REP_DW_PROTOCOL_VER;
	cmd->_header._seq = seq++;

	/* begining from _client_version */
	fix_len=
	4+
	2+
	2+
	4+
	4+
	4+
	4+
	4+
	8+
	4+
	4+
	4+
	4+
	4+
	4+
	4+
	4+
	4+
	4+
	4+
	1+
	4+
	4+
	4;

	other_len=
	cmd->_peerid_size+
	cmd->_url_size+
	cmd->_page_url_size+
	cmd->_cid_size+
	cmd->thunder_version_len+
	cmd->partner_id_len+
	cmd->_redirect_url_size+
	cmd->_statistic_page_url_size;
		
	cmd->_header._cmd_len = fix_len+ other_len;
/*	4+
	2+
	2+
	4+
	cmd->_peerid_size+
	4+
	cmd->_url_size+
	4+
	4+
	cmd->_page_url_size+
	4+
	cmd->_cid_size+
	8+
	4+
	4+
	4+
	cmd->thunder_version_len+
	4+
	cmd->parter_id_len+
	4+
	cmd->_redirect_url_size+
	4+
	4+
	4+
	4+
	4+
	4+
	1+
	4+
	cmd->_statistic_page_url_size+
	4+
	4;
*/	
	cmd->_cmd_type = REPORT_DW_FAIL_CMD_TYPE;
	*len = sizeof(REP_CMD_HEADER) + cmd->_header._cmd_len;

	//encode_len = *len+16;
	encode_len = (cmd->_header._cmd_len + 16) /16 * 16 + 12;
    ret = build_report_http_header(http_header, &http_header_len, encode_len,REPORTER_STAT_HUB);
	CHECK_VALUE(ret);
	ret = sd_malloc(*len + 16 + http_header_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("reporter_build_report_dw_fail_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	sd_memcpy(*buffer, http_header, http_header_len);
	tmp_buf = *buffer + http_header_len;
	tmp_len = (_int32)*len;

    /*
	ret = sd_malloc(*len + 16, (void**)buffer);  // why +16 ???  zyq mark 
	if(ret != SUCCESS)
	{
		LOG_DEBUG("build_report_dw_fail_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;
	*/
	//sd_set_bytes(&tmp_buf, &tmp_len, (char*)&cmd->_header, sizeof(REP_CMD_HEADER));

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
       sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
       sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);
	   
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_client_version);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_compress);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_cmd_type);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_url_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_url, (_int32)cmd->_url_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->fail_code);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_page_url_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_page_url, (_int32)cmd->_page_url_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_cid, (_int32)cmd->_cid_size);

	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->dw_comefrom);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->product_flag);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->thunder_version_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->thunder_version, (_int32)cmd->thunder_version_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->partner_id_len);  
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->partner_id, (_int32)cmd->partner_id_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_redirect_url_size);  
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_redirect_url, (_int32)cmd->_redirect_url_size);

	//sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->download_ip);
	//sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->lan_ip);

       sd_set_bytes(&tmp_buf, &tmp_len, (char* )&cmd->download_ip, sizeof(_u32));
	sd_set_bytes(&tmp_buf, &tmp_len, (char* )&cmd->lan_ip, sizeof(_u32));

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->limit_upload);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->download_stat);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->server_rc_num);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->peer_rc_num);

	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->CDN_number);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_statistic_page_url_size); 
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_statistic_page_url, (_int32)cmd->_statistic_page_url_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->recommend_download_stat);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->download_ratio);

#ifdef REPORTER_DEBUG
		{
			char test_buffer[1024];
			sd_memset(test_buffer,0,1024);
			str2hex( (char*)(*buffer+http_header_len), *len+16, test_buffer, 1024);
			LOG_DEBUG( "build_report_dw_fail_cmd:*buffer[%u]=%s" 
						,((_u32)(*len))+16, test_buffer);
		}
#endif /* _DEBUG */
	
	ret = xl_aes_encrypt(*buffer+http_header_len, len);
	if(ret != SUCCESS)
	{
		SAFE_DELETE(*buffer);
		CHECK_VALUE(ret);
	}
	
	if(tmp_len != 0)
	{
		SAFE_DELETE(*buffer);
		return REPORTER_ERR_BUILD_DW_FAIL;
	}
    sd_assert( encode_len == *len);
    *len += http_header_len;
   
	return SUCCESS;
}

_int32 reporter_build_report_insertsres_cmd(char** buffer, _u32* len, REPORT_INSERTSRES_CMD* cmd)
{
	//static _u32 seq = 0;
	_int32 ret;
	char*	tmp_buf;
	_int32  tmp_len,fix_len=0,other_len=0;
	char http_header[1024] = {0};
	_u32 http_header_len = 1024;
    _u32 encode_len = 0;

      cmd->_header._version = 61;
	cmd->_header._seq = seq++;
	build_reservce_60ver(&cmd->_reserver_buffer, &cmd->_reserver_length);
	
	//cmd->_header._cmd_len = 52 + cmd->_url_or_gcid_size + cmd->_cid_size + cmd->_origin_url_size + cmd->_peerid_size + cmd->_refer_url_size;

	/* begining from _client_version */
	fix_len=
	4+	//_u32	_client_version;
	2+	//_u16	_compress;
	2+	//_u16	_cmd_type;	//2005
	4+	//_u32	_peerid_size;
	4+	//_u32	_origin_url_size;
	4+  //_u32      _origion_url_code_page
	4+	//_u32	_refer_url_size;
	4+   //_u32     refer_url_code_page
	4+	//_u32	_redirect_url_size;
	4+  // _u32      redirect_url_code_page
	8+	//_u64	_file_size;
	4+ 	//_u32	_cid_size;
	4+	//_u32	_gcid_size;
	4+	//_int32 	_gcid_level;
	4+ 	//_int32 	_gcid_part_size;
	4+	//_u32	_bcid_size;	
	4+	//_u32	file_suffix_len;
	4+	//_int32	download_status;
	1+	//_u8		have_password;
	4+	//_int32	download_ip;
	4+	//_int32	download_stat;
	4	//_int32	insert_course;
	#ifdef REP_DW_VER_54
	+4	//_u32	partner_id_len;
	+4	//_int32	product_flag
	+1	//_u8		_refer_url_type;
	#endif /* REP_DW_VER_54 */
	;
	
other_len=
	cmd->_peerid_size+
	cmd->_origin_url_size+
	cmd->_refer_url_size+
	cmd->_redirect_url_size+
	cmd->_cid_size+
	cmd->_gcid_size+
	cmd->_bcid_size+
	cmd->file_suffix_len
#ifdef REP_DW_VER_54
	+cmd->partner_id_len
#endif /* REP_DW_VER_54 */
	+cmd->_reserver_length
	;

	cmd->_header._cmd_len = fix_len+other_len;

	cmd->_cmd_type = REPORT_INSERTSRES_CMD_TYPE;
	*len = sizeof(REP_CMD_HEADER) + cmd->_header._cmd_len;
	//encode_len = *len+16;
	encode_len = (cmd->_header._cmd_len + 16) /16 * 16 + 12;
    ret = build_report_http_header(http_header, &http_header_len, encode_len,REPORTER_SHUB);
	CHECK_VALUE(ret);
	ret = sd_malloc(*len + 16 + http_header_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("reporter_build_report_insertsres_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	sd_memcpy(*buffer, http_header, http_header_len);
	tmp_buf = *buffer + http_header_len;
	tmp_len = (_int32)*len;


/*
    ret = sd_malloc(*len + 16, (void**)buffer);  // why +16 ???  zyq mark 
	if(ret != SUCCESS)
	{
		LOG_DEBUG("reporter_build_report_insertsres_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;
	*/
	//sd_set_bytes(&tmp_buf, &tmp_len, (char*)&cmd->_header, sizeof(REP_CMD_HEADER));
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_client_version);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_compress);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_reserver_buffer, cmd->_reserver_length);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_cmd_type);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_origin_url_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_origin_url, (_int32)cmd->_origin_url_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_origin_url_code_page);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_refer_url_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_refer_url, (_int32)cmd->_refer_url_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_refer_url_code_page);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_redirect_url_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_redirect_url, (_int32)cmd->_redirect_url_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_redirect_url_code_page);

	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_cid, (_int32)cmd->_cid_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_gcid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_gcid, (_int32)cmd->_gcid_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_gcid_level);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_gcid_part_size);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len,(_int32) cmd->_bcid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_bcid, (_int32)cmd->_bcid_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->file_suffix_len); 
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->file_suffix, (_int32)cmd->file_suffix_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->download_status);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->have_password);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->download_ip);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->download_stat);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->insert_course);

#ifdef REP_DW_VER_54
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->partner_id_len);  
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->partner_id, (_int32)cmd->partner_id_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->product_flag);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_refer_url_type);
#endif /* REP_DW_VER_54 */

#ifdef REPORTER_DEBUG
		{
			char test_buffer[1024];
			sd_memset(test_buffer,0,1024);
			str2hex( (char*)(*buffer+http_header_len), *len+16, test_buffer, 1024);
			LOG_DEBUG( "reporter_build_report_insertsres_cmd:*buffer[%u]=%s" 
						,((_u32)(*len))+16, test_buffer);
		}
#endif /* _DEBUG */
		
	ret = xl_aes_encrypt(*buffer+http_header_len, len);
	if(ret != SUCCESS)
	{
		SAFE_DELETE(*buffer);
		CHECK_VALUE(ret);
	}
	
	if(tmp_len != 0)
	{
		SAFE_DELETE(*buffer);
		return REPORTER_ERR_BUILD_INSERTSRES;
	}
    sd_assert( encode_len == *len);
    *len += http_header_len;
	return SUCCESS;
}

/////////////////////////
#ifdef ENABLE_BT
//INSERTBTRES
_int32 reporter_build_report_bt_insert_res_cmd(char** buffer, _u32* len, REPORT_INSERTBTRES_CMD* cmd)
{
	//static _u32 seq = 0;
	_int32 ret;
	char*	tmp_buf;
	_int32  tmp_len,fix_len=0,other_len=0;
	char http_header[1024] = {0};
	_u32 http_header_len = 1024;
    _u32 encode_len = 0;

    cmd->_header._version = REP_DW_PROTOCOL_VER;
	cmd->_header._seq = seq++;

	/* begining from _client_version */
	fix_len=
	4+		//_u32	_client_version;
	2+		//_u16	_compress;
	2+		//_u16	_cmd_type;	//4003
	4+		//_u32	_peerid_size;
	4+		//_u32	_info_id_size;
	4+		//_u32	_file_index;
	4+		//_int32	_cource;
	8+		//_u64	_file_size;
	4+		//_u32	_cid_size;
	4+		//_u32	_gcid_size;
	4+		//_int32	_gcid_level;
	4+		//_u32	_gcid_part_size;     // bcid block size
	4+		//_u32	_bcid_size;
	8+		//_u64	_file_total_size;
	8+		//_u64	_start_offset;
	4		//_int32 	_block_size;     // Piece size
#ifdef REP_DW_VER_54
	+4		//_u32	partner_id_len;
	+4		//_int32	product_flag;
#endif /* REP_DW_VER_54 */
	;

other_len=
	cmd->_peerid_size+
	cmd->_info_id_size+
	cmd->_cid_size+
	cmd->_gcid_size+
	cmd->_bcid_size
#ifdef REP_DW_VER_54
	+cmd->partner_id_len
#endif /* REP_DW_VER_54 */
	;

	cmd->_header._cmd_len = fix_len+other_len;
	cmd->_cmd_type = REPORT_BT_INSERT_RES_CMD_TYPE;
	*len = sizeof(REP_CMD_HEADER) + cmd->_header._cmd_len;

	//encode_len = *len+16;
	encode_len = (cmd->_header._cmd_len + 16) /16 * 16 + 12;
    
    ret = build_report_http_header(http_header, &http_header_len, encode_len,REPORTER_BT_HUB);
	CHECK_VALUE(ret);
	ret = sd_malloc(*len + 16 + http_header_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("reporter_build_report_bt_insert_res_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	sd_memcpy(*buffer, http_header, http_header_len);
	tmp_buf = *buffer + http_header_len;
	tmp_len = (_int32)*len;

    /*
	ret = sd_malloc(*len + 16, (void**)buffer);  // why +16 ???  zyq mark 
	if(ret != SUCCESS)
	{
		LOG_DEBUG("reporter_build_report_bt_insert_res_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;
	*/
	//sd_set_bytes(&tmp_buf, &tmp_len, (char*)&cmd->_header, sizeof(REP_CMD_HEADER));
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_client_version);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_compress);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_cmd_type);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_info_id_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_info_id, (_int32)cmd->_info_id_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_index);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_cource);
	
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_size);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_cid, (_int32)cmd->_cid_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_gcid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_gcid, (_int32)cmd->_gcid_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_gcid_level);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_gcid_part_size);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_bcid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_bcid, (_int32)cmd->_bcid_size);

	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_total_size);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_start_offset);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_block_size);
	
#ifdef REP_DW_VER_54
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->partner_id_len);  
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->partner_id, (_int32)cmd->partner_id_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->product_flag);
#endif /* REP_DW_VER_54 */

#ifdef REPORTER_DEBUG
		{
			char test_buffer[1024];
			sd_memset(test_buffer,0,1024);
			str2hex( (char*)(*buffer+http_header_len), *len+16, test_buffer, 1024);
			LOG_DEBUG( "reporter_build_report_bt_insert_res_cmd:*buffer[%u]=%s" 
						,((_u32)(*len))+16, test_buffer);
		}
#endif /* _DEBUG */
	
	ret = xl_aes_encrypt(*buffer+http_header_len, len);
	if(ret != SUCCESS)
	{
		SAFE_DELETE(*buffer);
		CHECK_VALUE(ret);
	}
	
	if(tmp_len != 0)
	{
		SAFE_DELETE(*buffer);
		return REPORTER_ERR_BUILD_BT_INSERT_RES;
	}
    sd_assert( encode_len == *len);
    *len += http_header_len;
	return SUCCESS;
}
_int32 reporter_build_report_bt_download_stat_cmd(char** buffer, _u32* len, REPORT_BT_DL_STAT_CMD* cmd)
{
	//static _u32 seq = 0;
	_int32 ret;
	char*	tmp_buf;
	_int32  tmp_len,fix_len=0,other_len=0;
	char http_header[1024] = {0};
	_u32 http_header_len = 1024;
    _u32 encode_len = 0;


    cmd->_header._version = REP_DW_PROTOCOL_VER;
	cmd->_header._seq = seq++;

	/* begining from _client_version */
	fix_len=
	4+		//_u32	_client_version;
	2+		//_u16	_compress;
	2+		//_u16	_cmd_type;	//4007
	4+		//_u32	_peerid_size;
	4+		//_u32	_info_id_size;
	4+		//_u32	_file_index;
	4+		//_u32	_title_size;
	8+		//_u64	_file_size;
	4+		//_u32	_cid_size;
	4+		//_u32	_gcid_size;
	4+		//_u32	_file_name_size;
	8+		//_u64	_bt_dl_size;
	8+		//_u64	_server_dl_size;
	8+		//_u64	_peer_dl_size;
	4+		//_u32 	_duration;
	4+		//_u32     _avg_speed;
	4+		//_int32	product_flag;
	4+		//_u32	thunder_version_len;
	4+		//_u32	partner_id_len;
	8+		//_u64	_file_total_size;
	8+		//_u64	_start_offset;
	4+		//_int32 	_block_size;     // Piece size
	8+		//_u64	_data_from_oversi;
	8		//_u64	_bytes_by_emule;
#ifdef REP_DW_VER_60
	+8		//_u64	_download_bytes_from_dcache;
#endif /* REP_DW_VER_60 */
	;

other_len=
	cmd->_peerid_size+
	cmd->_info_id_size+
	cmd->_title_size+
	cmd->_cid_size+
	cmd->_gcid_size+
	cmd->_file_name_size+
	cmd->thunder_version_len+
	cmd->partner_id_len
	;

	cmd->_header._cmd_len = fix_len+other_len;
	cmd->_cmd_type = REPORT_BT_DL_STAT_CMD_TYPE;
	*len = sizeof(REP_CMD_HEADER) + cmd->_header._cmd_len;
	encode_len = (cmd->_header._cmd_len + 16) /16 * 16 + 12;
	//encode_len = *len+16;
    ret = build_report_http_header(http_header, &http_header_len, encode_len,REPORTER_STAT_HUB);
	CHECK_VALUE(ret);
	ret = sd_malloc(*len + 16 + http_header_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("reporter_build_report_bt_download_stat_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	sd_memcpy(*buffer, http_header, http_header_len);
	tmp_buf = *buffer + http_header_len;
	tmp_len = (_int32)*len;

/*
    ret = sd_malloc(*len + 16, (void**)buffer);  // why +16 ???  zyq mark 
	if(ret != SUCCESS)
	{
		LOG_DEBUG("reporter_build_report_bt_download_stat_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;
	*/
	//sd_set_bytes(&tmp_buf, &tmp_len, (char*)&cmd->_header, sizeof(REP_CMD_HEADER));
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_client_version);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_compress);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_cmd_type);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_info_id_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_info_id, (_int32)cmd->_info_id_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_index);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_title_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_title, (_int32)cmd->_title_size);

	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_size);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_cid, (_int32)cmd->_cid_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_gcid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_gcid, (_int32)cmd->_gcid_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_name_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_file_name, (_int32)cmd->_file_name_size);

	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_bt_dl_size);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_server_dl_size);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_peer_dl_size);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_duration);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_avg_speed);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->product_flag);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->thunder_version_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->thunder_version, (_int32)cmd->thunder_version_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->partner_id_len);  
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->partner_id, (_int32)cmd->partner_id_len);

	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_total_size);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_start_offset);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_block_size);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_data_from_oversi);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_bytes_by_emule);

#ifdef REP_DW_VER_60
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_download_bytes_from_dcache);  
#endif /* REP_DW_VER_60 */

#ifdef REPORTER_DEBUG
		{
			char test_buffer[1024];
			sd_memset(test_buffer,0,1024);
			str2hex( (char*)(*buffer+http_header_len), *len+16, test_buffer, 1024);
			LOG_DEBUG( "reporter_build_report_bt_download_stat_cmd:*buffer[%u]=%s" 
						,((_u32)(*len))+16, test_buffer);
		}
#endif /* _DEBUG */
	
	ret = xl_aes_encrypt(*buffer+http_header_len, len);
	if(ret != SUCCESS)
	{
		SAFE_DELETE(*buffer);
		CHECK_VALUE(ret);
	}
	
	if(tmp_len != 0)
	{
		SAFE_DELETE(*buffer);
		return REPORTER_ERR_BUILD_BT_DL_STAT;
	}
    
    sd_assert( encode_len == *len);
    *len += http_header_len;
	return SUCCESS;
}

_int32 reporter_build_report_bt_task_download_stat_cmd(char** buffer, _u32* len, REPORT_BT_TASK_DL_STAT_CMD* cmd)
{
	//static _u32 seq = 0;
	_int32 ret;
	char*	tmp_buf;
	_int32  tmp_len,fix_len=0,other_len=0;
	char http_header[1024] = {0};
	_u32 http_header_len = 1024;
    _u32 encode_len = 0;

	cmd->_header._version = REP_DW_PROTOCOL_VER;
	cmd->_header._seq = seq++;

	/* begining from _client_version */
	fix_len=
	4+		//_u32	_client_version;
	2+		//_u16	_compress;
	2+		//_u16	_cmd_type;	//4011
	4+		//_u32	_peerid_size;
	4+		//_u32	_info_id_size;
	4+		//_u32	_title_size;
	4+		//_int32 	_file_number;
	8+		//_u64	_file_size;
	4+		//_int32 	_block_size;     // Piece size
	4+		//_u32 	_duration;
	4+		//_u32       _avg_speed;
	4+		//_u32	_seed_url_size;
	4+		//_u32	_page_url_size;
	4+		//_u32	_statistic_page_url_size;
	4+		//_int32	product_flag;
	4+		//_u32	thunder_version_len;
	4+		//_u32	partner_id_len;
	4		//_int32     _download_flag;
#ifdef REP_DW_VER_54
	+4		//_int32 	dw_comefrom;
#endif /* REP_DW_VER_54 */
	;

other_len=
	cmd->_peerid_size+
	cmd->_info_id_size+
	cmd->_title_size+
	cmd->_seed_url_size+
	cmd->_page_url_size+
	cmd->_statistic_page_url_size+
	cmd->thunder_version_len+
	cmd->partner_id_len
	;

	cmd->_header._cmd_len = fix_len+other_len;
	cmd->_cmd_type = REPORT_BT_TASK_DL_STAT_CMD_TYPE;
	*len = sizeof(REP_CMD_HEADER) + cmd->_header._cmd_len;
    

	//encode_len = *len+16;
	encode_len = (cmd->_header._cmd_len + 16) /16 * 16 + 12;
    ret = build_report_http_header(http_header, &http_header_len, encode_len,REPORTER_STAT_HUB);
	CHECK_VALUE(ret);
	ret = sd_malloc(*len + 16 + http_header_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("reporter_build_report_bt_task_download_stat_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	sd_memcpy(*buffer, http_header, http_header_len);
	tmp_buf = *buffer + http_header_len;
	tmp_len = (_int32)*len;

/*
	ret = sd_malloc(*len + 16, (void**)buffer);  // why +16 ???  zyq mark 
	if(ret != SUCCESS)
	{
		LOG_DEBUG("reporter_build_report_bt_task_download_stat_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;
	*/
	//sd_set_bytes(&tmp_buf, &tmp_len, (char*)&cmd->_header, sizeof(REP_CMD_HEADER));
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_client_version);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_compress);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_cmd_type);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_info_id_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_info_id, (_int32)cmd->_info_id_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_title_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_title, (_int32)cmd->_title_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_file_number);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_block_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_duration);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_avg_speed);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_seed_url_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_seed_url, (_int32)cmd->_seed_url_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_page_url_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_page_url, (_int32)cmd->_page_url_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_statistic_page_url_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_statistic_page_url, (_int32)cmd->_statistic_page_url_size);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->product_flag);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->thunder_version_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->thunder_version, (_int32)cmd->thunder_version_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->partner_id_len);  
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->partner_id, (_int32)cmd->partner_id_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_download_flag);

#ifdef REP_DW_VER_54
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->dw_comefrom);  
#endif /* REP_DW_VER_54 */

#ifdef REPORTER_DEBUG
		{
			char test_buffer[1024];
			sd_memset(test_buffer,0,1024);
			str2hex( (char*)(*buffer+http_header_len), *len+16, test_buffer, 1024);
			LOG_DEBUG( "reporter_build_report_bt_task_download_stat_cmd:*buffer[%u]=%s" 
						,((_u32)(*len))+16, test_buffer);
		}
#endif /* _DEBUG */
		
	ret = xl_aes_encrypt(*buffer+http_header_len, len);
	if(ret != SUCCESS)
	{
		SAFE_DELETE(*buffer);
		CHECK_VALUE(ret);
	}
	
	if(tmp_len != 0)
	{
		SAFE_DELETE(*buffer);
		return REPORTER_ERR_BUILD_BT_TASK_DL_STAT;
	}
    sd_assert( encode_len == *len);
    *len += http_header_len;
	return SUCCESS;
}


#endif /* ENABLE_BT */

//////////////////////////

#ifdef ENABLE_EMULE
_int32 reporter_build_emule_report_insertsres_cmd(char** buffer, _u32* len, REPORT_INSERT_EMULE_RES_CMD* cmd)
{
	//static _u32 seq = 0;
	_int32 ret;
	char*	tmp_buf;
	_int32  tmp_len,fix_len=0,other_len=0;
	char http_header[1024] = {0};
	_u32 http_header_len = 1024;
    _u32 encode_len = 0;

    cmd->_header._version = REP_DW_PROTOCOL_VER;
	cmd->_header._seq = seq++;
	//cmd->_header._cmd_len = 52 + cmd->_url_or_gcid_size + cmd->_cid_size + cmd->_origin_url_size + cmd->_peerid_size + cmd->_refer_url_size;

	/* begining from _client_version */
	fix_len=
        4+	//_u32	_client_version;
        2+	//_u16	_compress;
        2+	//_u16	_cmd_type;	//2005
        4+	//_u32	_peerid_size;
        4+	//_u32	_fileid_len;
        8+	//_u64	_file_size;
        4+	//_int32	insert_course;
        4+  //_u32  file_name_len;
        4+  //_u32  _aich_hash_len;
        4+  //_u32  _part_hash_len;
        4+ 	//_u32	_cid_size;
        4+	//_u32	_gcid_size;
        4+	//_int32 	_gcid_level;
        4+ 	//_int32 	_gcid_part_size;
        4+  //_u32  _bcid_size; 
        4  //_u32  _md5_size; 
#ifdef REP_DW_VER_54
        +4	//_u32	partner_id_len;
        +4	//_int32	product_flag
#endif /* REP_DW_VER_54 */
	;
	
other_len=
	cmd->_peerid_size+
	cmd->_fileid_len+

	cmd->_file_name_len+
	cmd->_aich_hash_len+
	cmd->_part_hash_len+

	cmd->_cid_size+
	cmd->_gcid_size+
	cmd->_bcid_size+

	cmd->_md5_size

#ifdef REP_DW_VER_54
	+cmd->_partner_id_len
#endif /* REP_DW_VER_54 */
	;

	cmd->_header._cmd_len = fix_len+other_len;

	cmd->_cmd_type = REPORT_EMULE_INSERTSRES_CMD_TYPE;
	*len = sizeof(REP_CMD_HEADER) + cmd->_header._cmd_len;

	//encode_len = *len+16;
    
	encode_len = (cmd->_header._cmd_len + 16) /16 * 16 + 12;
    ret = build_report_http_header(http_header, &http_header_len, encode_len,REPORTER_EMULE_HUB);
	CHECK_VALUE(ret);
	ret = sd_malloc(*len + 16 + http_header_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("reporter_build_emule_report_insertsres_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	sd_memcpy(*buffer, http_header, http_header_len);
	tmp_buf = *buffer + http_header_len;
	tmp_len = (_int32)*len;

/*
    ret = sd_malloc(*len + 16, (void**)buffer);  // why +16 ???  zyq mark 
	if(ret != SUCCESS)
	{
		LOG_DEBUG("reporter_build_emule_report_insertsres_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;
	*/
	//sd_set_bytes(&tmp_buf, &tmp_len, (char*)&cmd->_header, sizeof(REP_CMD_HEADER));
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_client_version);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_compress);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_cmd_type);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_fileid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_fileid, (_int32)cmd->_fileid_len);

	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_cource);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_name_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_file_name, (_int32)cmd->_file_name_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_aich_hash_len);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_aich_hash_ptr, (_int32)cmd->_aich_hash_len);

    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_part_hash_len);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_part_hash_ptr, (_int32)cmd->_part_hash_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_cid, (_int32)cmd->_cid_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_gcid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_gcid, (_int32)cmd->_gcid_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_gcid_level);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_gcid_part_size);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len,(_int32) cmd->_bcid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_bcid, (_int32)cmd->_bcid_size);

    sd_set_int32_to_lt(&tmp_buf, &tmp_len,(_int32) cmd->_md5_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_md5, (_int32)cmd->_md5_size);

#ifdef REP_DW_VER_54
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_partner_id_len);  
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_partner_id, (_int32)cmd->_partner_id_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_product_flag);
#endif /* REP_DW_VER_54 */

#ifdef REPORTER_DEBUG
	{
		char test_buffer[1024];
		sd_memset(test_buffer,0,1024);
		str2hex( (char*)(*buffer + http_header_len), *len+16, test_buffer, 1024);
		LOG_DEBUG( "reporter_build_emule_report_insertsres_cmd:*buffer[%u]=%s" 
					,((_u32)(*len))+16, test_buffer);
	}
#endif /* _DEBUG */
	
	ret = xl_aes_encrypt(*buffer + http_header_len, len);
	if(ret != SUCCESS)
	{
		SAFE_DELETE(*buffer);
		CHECK_VALUE(ret);
	}
	
	if(tmp_len != 0)
	{
      
        sd_assert( FALSE );
		SAFE_DELETE(*buffer);
		return REPORTER_ERR_BUILD_INSERTSRES;
	}
    sd_assert( encode_len == *len);
     *len += http_header_len;
     return SUCCESS;
/*
    LOG_DEBUG( "reporter_build_emule_report_insertsres_cmd test" );

    char *p_http_header = sd_strstr(*buffer, "\r\n\r\n", 0);
    _u32 parse_http_header_len = p_http_header - *buffer + 4;
    sd_assert(parse_http_header_len == http_header_len);
    char* recv_buffer = *buffer+parse_http_header_len;
    _u32 buffer_len = *len - parse_http_header_len;
    _u32 version, sequence, body_len,client_version;
    sd_get_int32_from_lt(&recv_buffer, (_int32*)&buffer_len, (_int32*)&version);
    sd_get_int32_from_lt(&recv_buffer, (_int32*)&buffer_len, (_int32*)&sequence);
    
    sd_assert(version ==cmd->_header._version );
    sd_assert(sequence ==cmd->_header._seq );
    
    ret = aes_decrypt(*buffer+parse_http_header_len, &encode_len);
    sd_assert(ret== SUCCESS);

    sd_get_int32_from_lt(&recv_buffer, (_int32*)&buffer_len, (_int32*)&body_len);
    sd_get_int32_from_lt(&recv_buffer, (_int32*)&buffer_len, (_int32*)&client_version);


    sd_assert(body_len ==cmd->_header._cmd_len );
    sd_assert(client_version ==cmd->_client_version );
    */
}

_int32 reporter_build_report_emule_dl_cmd(char** buffer, _u32* len, REPORT_EMULE_DL_STAT_CMD* cmd)
{
	//static _u32 seq = 0;
	_int32 ret;
	char*	tmp_buf;
	_int32  tmp_len,fix_len=0,other_len=0;
	char http_header[1024] = {0};
	_u32 http_header_len = 1024;
    _u32 encode_len = 0;


    cmd->_header._version = REP_DW_PROTOCOL_VER;
	cmd->_header._seq = seq++;
	//cmd->_header._cmd_len = 52 + cmd->_url_or_gcid_size + cmd->_cid_size + cmd->_origin_url_size + cmd->_peerid_size + cmd->_refer_url_size;

	/* begining from _client_version */
	fix_len=
        4+	//_u32	_client_version;
        2+	//_u16	_compress;
        2+	//_u16	_cmd_type;	//2005
        4+	//_u32	_peerid_size;
        4+	//_u32	_fileid_len;
        8+	//_u64	_file_size;
        4+  //_u32  file_name_len;
        4+  //_u32  _aich_hash_len;
        4+  //_u32  _part_hash_len;
        4+ 	//_u32	_cid_size;
        4+	//_u32	_gcid_size;
        
        8+	//_u64	Emule dl size;
        8+	//_u64	Server dl size;
        8+	//_u64	Peer dl size;

        4+	//_u32 	Duration;
        4+ 	//_u32 	Avg speed;
        4+  //_u32  product_flag; 

        4+  //_u32  Thunder version len; 
        4+ //_u32  partner_id_len;
        4+ //_u32  Page url_len;
        4+ //_u32  Statistic page url_len;
        
        4+ //_u32  Download flag;
        
        8+	//_u64	bt dl size;
#ifdef REP_DW_VER_54
        4+	//_int32	dw_comefrom
#endif /* REP_DW_VER_54 */
        8+  //_u64  cdn dl size;
        1+  //Speed up
        4 //_u32  Band width;
	;
	
other_len=
	cmd->_peerid_size+
	cmd->_fileid_len+

	cmd->_file_name_len+
	cmd->_aich_hash_len+
	cmd->_part_hash_len+

	cmd->_cid_size+
	cmd->_gcid_size+

	cmd->_thunder_version_len+
	cmd->_partner_id_len+
	cmd->_page_url_size+
	cmd->_statistic_page_url_size
	;

	cmd->_header._cmd_len = fix_len+other_len;

	cmd->_cmd_type = REPORT_EMULE_DL_STAT_CMD_TYPE;
	*len = sizeof(REP_CMD_HEADER) + cmd->_header._cmd_len;
    
	//encode_len = *len+16;
	encode_len = (cmd->_header._cmd_len + 16) /16 * 16 + 12;
    ret = build_report_http_header(http_header, &http_header_len, encode_len,REPORTER_STAT_HUB);
	CHECK_VALUE(ret);
	ret = sd_malloc(*len + 16 + http_header_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("reporter_build_report_emule_dl_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	sd_memcpy(*buffer, http_header, http_header_len);
	tmp_buf = *buffer + http_header_len;
	tmp_len = (_int32)*len;


/*
    ret = sd_malloc(*len + 16, (void**)buffer);  // why +16 ???  zyq mark 
	if(ret != SUCCESS)
	{
		LOG_DEBUG("reporter_build_report_emule_dl_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;
	*/
	//sd_set_bytes(&tmp_buf, &tmp_len, (char*)&cmd->_header, sizeof(REP_CMD_HEADER));
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_client_version);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_compress);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_cmd_type);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_fileid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_fileid, (_int32)cmd->_fileid_len);

	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_name_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_file_name, (_int32)cmd->_file_name_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_aich_hash_len);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_aich_hash_ptr, (_int32)cmd->_aich_hash_len);

    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_part_hash_len);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_part_hash_ptr, (_int32)cmd->_part_hash_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_cid, (_int32)cmd->_cid_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_gcid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_gcid, (_int32)cmd->_gcid_size);

	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_emule_dl_bytes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_server_dl_bytes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_peer_dl_bytes);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_duration);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_avg_speed);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_product_flag);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_thunder_version_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_thunder_version, (_int32)cmd->_thunder_version_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_partner_id_len);  
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_partner_id, (_int32)cmd->_partner_id_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_page_url_size);  
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_page_url, (_int32)cmd->_page_url_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_statistic_page_url_size);  
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_statistic_page_url, (_int32)cmd->_statistic_page_url_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_download_flag);

	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_bt_dl_bytes);

#ifdef REP_DW_VER_54
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_dw_comefrom);
#endif /* REP_DW_VER_54 */

    sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_cdn_dl_bytes);
    sd_set_int8(&tmp_buf, &tmp_len, cmd->_is_speedup);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_band_width);

#ifdef REPORTER_DEBUG
	{
		char test_buffer[1024];
		sd_memset(test_buffer,0,1024);
		str2hex( (char*)(*buffer+http_header_len), *len+16, test_buffer, 1024);
		LOG_DEBUG( "reporter_build_report_emule_dl_cmd:*buffer[%u]=%s" 
					,((_u32)(*len))+16, test_buffer);
	}
#endif /* _DEBUG */
		
	ret = xl_aes_encrypt(*buffer+http_header_len, len);
	if(ret != SUCCESS)
	{
		SAFE_DELETE(*buffer);
		CHECK_VALUE(ret);
	}
	
	if(tmp_len != 0)
	{
        sd_assert( FALSE );
		SAFE_DELETE(*buffer);
		return REPORTER_ERR_BUILD_INSERTSRES;
	}
    sd_assert( encode_len == *len);

    *len += http_header_len;
	return SUCCESS;
}


#endif


#ifdef UPLOAD_ENABLE
_int32 reporter_build_isrc_online_cmd(char** buffer, _u32* len, REPORT_RC_ONLINE_CMD* cmd)
{
	_int32 ret;
	char*	tmp_buf;
	_int32  tmp_len,fix_len=0,other_len=0;
	char http_header[1024] = {0};
	_u32 http_header_len = 1024;
    _u32 encode_len = 0;


    cmd->_header._version = REP_RCLIST_PROTOCOL_VER;
	cmd->_header._seq = reporter_rc_seq++;

	/* begining from _cmd_type */
	fix_len=
//	4+   //_u32	_client_version;
//	2+   //_u16	_compress;
	1+   //_u8	_cmd_type;
	4;   //_u32	_peerid_size;

	other_len=
	cmd->_peerid_size;

	cmd->_header._cmd_len = fix_len+other_len;

	cmd->_cmd_type = REPORT_ISRC_ONLINE_CMD_TYPE;
	*len = sizeof(REP_CMD_HEADER) + cmd->_header._cmd_len;

	//encode_len = *len+16;
	encode_len = (cmd->_header._cmd_len + 16) /16 * 16 + 12;
    ret = build_report_http_header(http_header, &http_header_len, encode_len,REPORTER_PHUB);
	CHECK_VALUE(ret);
	ret = sd_malloc(*len + 16 + http_header_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("reporter_build_isrc_online_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	sd_memcpy(*buffer, http_header, http_header_len);
	tmp_buf = *buffer + http_header_len;
	tmp_len = (_int32)*len;

/*
    ret = sd_malloc(*len + 16, (void**)buffer);  // why +16 ???  zyq mark 
	if(ret != SUCCESS)
	{
		LOG_DEBUG("reporter_build_isrc_online_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;
	*/
	//sd_set_bytes(&tmp_buf, &tmp_len, (char*)&cmd->_header, sizeof(REP_CMD_HEADER));
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);
	//sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_client_version);
	//sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_compress);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_cmd_type);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_size);

#ifdef REPORTER_DEBUG
		{
			char test_buffer[1024];
			sd_memset(test_buffer,0,1024);
			str2hex( (char*)(*buffer+http_header_len), *len+16, test_buffer, 1024);
			LOG_DEBUG( "reporter_build_isrc_online_cmd:*buffer[%u]=%s" 
						,((_u32)(*len))+16, test_buffer);
		}
#endif /* _DEBUG */
	
	ret = xl_aes_encrypt(*buffer+ http_header_len, len);
	if(ret != SUCCESS)
	{
		SAFE_DELETE(*buffer);
		CHECK_VALUE(ret);
	}
	
	if(tmp_len != 0)
	{
		SAFE_DELETE(*buffer);
		return REPORTER_ERR_BUILD_RC_ONLINE;
	}
    sd_assert( encode_len == *len);
    *len += http_header_len;
	return SUCCESS;
}

_int32 reporter_build_rc_list_cmd(char** buffer, _u32* len, REPORT_RC_LIST_CMD* cmd,LIST_NODE ** pp_start_node,_int32 * start_node_index)
{
	_int32 ret;
	char*	tmp_buf;
	_int32  tmp_len,fix_len=0,other_len=0,_list_size = 0,item_len = sizeof(_int32)+CID_SIZE+sizeof(_u64)+sizeof(_int32)+CID_SIZE;
	LIST_NODE * p_list_node = NULL;
	ID_RC_INFO * p_rc_info = NULL;
	char http_header[1024] = {0};
	_u32 http_header_len = 1024;
    _u32 encode_len = 0;
    
	cmd->_header._version = REP_RCLIST_PROTOCOL_VER;
	cmd->_header._seq = reporter_rc_seq++;

	if((*pp_start_node)!=LIST_END(*(cmd->_rc_list)))
	{
		_list_size = list_size(cmd->_rc_list)-(*start_node_index);
		LOG_DEBUG("reporter_build_rc_list_cmd start:_list_size=%d,* pp_start_node=0x%X,* start_node_index=%d",_list_size,*pp_start_node,*start_node_index);
		if(_list_size>REPORT_MAX_RECORD_NUM_EVERY_CMD)
		{
			_list_size = REPORT_MAX_RECORD_NUM_EVERY_CMD;
			(*start_node_index)+=REPORT_MAX_RECORD_NUM_EVERY_CMD;
		}
	}
	else
	{
		*start_node_index = -1;
		LOG_DEBUG("reporter_build_rc_list_cmd 1:(*pp_start_node)==LIST_END(*(cmd->_rc_list)).");
	}
	
	/* begining from _cmd_type */
	fix_len=
//	4+   //_u32	_client_version;
//	2+   //_u16	_compress;
	1+   //_u8	_cmd_type;
	4+   //_u32	_peerid_size;
	4+   //_int32	list_size;
	4;   //_u32	_p2p_capability;

	other_len= cmd->_peerid_size+_list_size*(sizeof(item_len)+item_len);

	cmd->_header._cmd_len = fix_len+other_len;

	cmd->_cmd_type = REPORT_RC_LIST_CMD_TYPE;
	*len = sizeof(REP_CMD_HEADER) + cmd->_header._cmd_len;

	//encode_len = *len+16;
	encode_len = (cmd->_header._cmd_len + 16) /16 * 16 + 12;

    ret = build_report_http_header(http_header, &http_header_len, encode_len,REPORTER_PHUB);
	CHECK_VALUE(ret);
	ret = sd_malloc(*len + 16 + http_header_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("reporter_build_rc_list_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	sd_memcpy(*buffer, http_header, http_header_len);
	tmp_buf = *buffer + http_header_len;
	tmp_len = (_int32)*len;	

/*
    ret = sd_malloc(*len + 16, (void**)buffer);  // why +16 ???  zyq mark 
	if(ret != SUCCESS)
	{
		LOG_DEBUG("reporter_build_rc_list_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;
	*/
	//sd_set_bytes(&tmp_buf, &tmp_len, (char*)&cmd->_header, sizeof(REP_CMD_HEADER));
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);
//	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_client_version);
//	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_compress);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_cmd_type);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, _list_size);

	for(p_list_node =  *pp_start_node;(p_list_node !=  LIST_END(*(cmd->_rc_list)))&&(0!=_list_size--);p_list_node =  LIST_NEXT(p_list_node))
	{
		p_rc_info = (ID_RC_INFO * )LIST_VALUE(p_list_node);
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, item_len);
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)CID_SIZE);
		sd_set_bytes(&tmp_buf, &tmp_len, (char*)p_rc_info->_tcid, (_int32)CID_SIZE);
		sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)p_rc_info->_size);
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)CID_SIZE);
		sd_set_bytes(&tmp_buf, &tmp_len, (char*)p_rc_info->_gcid, (_int32)CID_SIZE);
		LOG_DEBUG("reporter_build_rc_list_cmd 2:_list_size=%d,p_list_node=0x%X,_path=%s",_list_size,p_list_node,p_rc_info->_path);
	}
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_p2p_capability);
#ifdef REPORTER_DEBUG
		{
			char test_buffer[1024];
			sd_memset(test_buffer,0,1024);
			str2hex( (char*)(*buffer+http_header_len), *len+16, test_buffer, 1024);
			LOG_DEBUG( "reporter_build_rc_list_cmd:*buffer[%u]=%s" 
						,((_u32)(*len))+16, test_buffer);
		}
#endif /* _DEBUG */
	
	ret = xl_aes_encrypt(*buffer+http_header_len, len);
	if(ret != SUCCESS)
	{
		SAFE_DELETE(*buffer);
		CHECK_VALUE(ret);
	}
	
	if(*start_node_index != -1)
		*pp_start_node = p_list_node;
	
	LOG_DEBUG("reporter_build_rc_list_cmd end:_list_size=%d,* pp_start_node=0x%X,* start_node_index=%d,is_end=%d",_list_size,*pp_start_node,*start_node_index,((*pp_start_node)==LIST_END(*(cmd->_rc_list)) ? TRUE : FALSE));
	
	if(tmp_len != 0)
	{
		SAFE_DELETE(*buffer);
		return REPORTER_ERR_BUILD_RC_LIST;
	}
    sd_assert( encode_len == *len);
    *len += http_header_len;
	return SUCCESS;

}


_int32 reporter_build_insert_rc_cmd(char** buffer, _u32* len, REPORT_INSERT_RC_CMD* cmd)
{
	_int32 ret;
	char*	tmp_buf;
	_int32  tmp_len,fix_len=0,other_len=0;
	char http_header[1024] = {0};
	_u32 http_header_len = 1024;
    _u32 encode_len = 0;
	
	cmd->_header._version = REP_RCLIST_PROTOCOL_VER;
	cmd->_header._seq = reporter_rc_seq++;
	

	/* begining from _client_version */
	fix_len=
//	4+   //_u32	_client_version;
//	2+   //_u16	_compress;
	1+   //_u8	_cmd_type;
	4+   //_u32	_peerid_size;
	4+   //_u32	cid_size;
	8+   //_u64	file_size;
	4+   //_u32	gcid_size;
	4;   //_u32	_p2p_capability;

	other_len= cmd->_peerid_size+cmd->_cid_size+cmd->_gcid_size;

	cmd->_header._cmd_len = fix_len+other_len;

	cmd->_cmd_type = REPORT_INSERT_RC_CMD_TYPE;
	*len = sizeof(REP_CMD_HEADER) + cmd->_header._cmd_len;


	//encode_len = *len+16;
	encode_len = (cmd->_header._cmd_len + 16) /16 * 16 + 12;
    ret = build_report_http_header(http_header, &http_header_len, encode_len,REPORTER_PHUB);
	CHECK_VALUE(ret);
	ret = sd_malloc(*len + 16 + http_header_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("reporter_build_insert_rc_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	sd_memcpy(*buffer, http_header, http_header_len);
	tmp_buf = *buffer + http_header_len;
	tmp_len = (_int32)*len;

/*
    ret = sd_malloc(*len + 16, (void**)buffer);  // why +16 ???  zyq mark 
	if(ret != SUCCESS)
	{
		LOG_DEBUG("reporter_build_insert_rc_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;
	*/
	//sd_set_bytes(&tmp_buf, &tmp_len, (char*)&cmd->_header, sizeof(REP_CMD_HEADER));
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);
//	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_client_version);
//	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_compress);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_cmd_type);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_cid, (_int32)cmd->_cid_size);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_gcid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_gcid, (_int32)cmd->_gcid_size);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_p2p_capability);

#ifdef REPORTER_DEBUG
		{
			char test_buffer[1024];
			sd_memset(test_buffer,0,1024);
			str2hex( (char*)(*buffer+http_header_len), *len+16, test_buffer, 1024);
			LOG_DEBUG( "reporter_build_insert_rc_cmd:*buffer[%u]=%s" 
						,((_u32)(*len))+16, test_buffer);
		}
#endif /* _DEBUG */
	
	ret = xl_aes_encrypt(*buffer+http_header_len, len);
	if(ret != SUCCESS)
	{
		SAFE_DELETE(*buffer);
		CHECK_VALUE(ret);
	}
	
	if(tmp_len != 0)
	{
		SAFE_DELETE(*buffer);
		return REPORTER_ERR_BUILD_INSERT_RC;
	}
    sd_assert( encode_len == *len);
    *len += http_header_len;
	return SUCCESS;
}


_int32 reporter_build_delete_rc_cmd(char** buffer, _u32* len, REPORT_DELETE_RC_CMD* cmd)
{
	_int32 ret;
	char*	tmp_buf;
	_int32  tmp_len,fix_len=0,other_len=0;
	char http_header[1024] = {0};
	_u32 http_header_len = 1024;
    _u32 encode_len = 0;
     
	cmd->_header._version = REP_RCLIST_PROTOCOL_VER;
	cmd->_header._seq = reporter_rc_seq++;
	

	/* begining from _client_version */
	fix_len=
//	4+   //_u32	_client_version;
//	2+   //_u16	_compress;
	1+   //_u8	_cmd_type;
	4+   //_u32	_peerid_size;
	4+   //_u32	cid_size;
	8+   //_u64	file_size;
	4;   //_u32	gcid_size;

	other_len= cmd->_peerid_size+cmd->_cid_size+cmd->_gcid_size;

	cmd->_header._cmd_len = fix_len+other_len;

	cmd->_cmd_type = REPORT_DELETE_RC_CMD_TYPE;
	*len = sizeof(REP_CMD_HEADER) + cmd->_header._cmd_len;


	//encode_len = *len+16;
	encode_len = (cmd->_header._cmd_len + 16) /16 * 16 + 12;
    ret = build_report_http_header(http_header, &http_header_len, encode_len,REPORTER_PHUB);
	CHECK_VALUE(ret);
	ret = sd_malloc(*len + 16 + http_header_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("reporter_build_delete_rc_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	sd_memcpy(*buffer, http_header, http_header_len);
	tmp_buf = *buffer + http_header_len;
	tmp_len = (_int32)*len;

/*
    ret = sd_malloc(*len + 16, (void**)buffer);  // why +16 ???  zyq mark 
	if(ret != SUCCESS)
	{
		LOG_DEBUG("reporter_build_delete_rc_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;
	*/
	//sd_set_bytes(&tmp_buf, &tmp_len, (char*)&cmd->_header, sizeof(REP_CMD_HEADER));
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);
//	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_client_version);
//	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_compress);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_cmd_type);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_cid, (_int32)cmd->_cid_size);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_gcid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_gcid, (_int32)cmd->_gcid_size);
	
#ifdef REPORTER_DEBUG
		{
			char test_buffer[1024];
			sd_memset(test_buffer,0,1024);
			str2hex( (char*)(*buffer+http_header_len), *len+16, test_buffer, 1024);
			LOG_DEBUG( "reporter_build_delete_rc_cmd:*buffer[%u]=%s" 
						,((_u32)(*len))+16, test_buffer);
		}
#endif /* _DEBUG */	
	ret = xl_aes_encrypt(*buffer+http_header_len, len);
	if(ret != SUCCESS)
	{
		SAFE_DELETE(*buffer);
		CHECK_VALUE(ret);
	}
	
	if(tmp_len != 0)
	{
		SAFE_DELETE(*buffer);
		return REPORTER_ERR_BUILD_DELETE_RC;
	}
    sd_assert( encode_len == *len);
    *len += http_header_len;
	return SUCCESS;
}
#endif

#ifdef EMBED_REPORT

_int32 emb_reporter_build_common_task_download_stat_cmd(char** buffer, _u32* len, EMB_REPORT_COMMON_TASK_DL_STAT_CMD* cmd)
{
	//static _u32 seq = 0;
	_int32 ret;
	char*	tmp_buf;
	_int32  tmp_len,fix_len=0,other_len=0;

    cmd->_header._version = EMB_REPORT_PROTOCOL_VER;
	cmd->_header._seq = seq++;

	/* begining from _client_version */
	fix_len=	
	4+		//_u32	_record_count; 条数
	4+		//_u32	_zip_type;	压缩方式
	4+		//_u32	_record_len;	单条记录长度
	
	4+		//_u32	_cmd_type;	
	4+		//_u32	thunder_version_len;
	4+		//_u32	partner_id_len;
	4+		//_u32	_peerid_size;
	4+		//_u32	_task_create_type;
	4+		//_u32	_url_len;
	4+		//_u32	_ref_url_len;
	4+		//_u32	_cid_size;
	4+		//_u32	_gcid_size;
	8+		//_u64	_file_size;
	4+		//_u32	_block_size;
	4+		//_u32	_file_name_len;
	4+		//_u32	_duration;
	4+		//_u32	_avg_speed;
	4		//_u32	_download_state;
	;

other_len=
	
	cmd->_thunder_version_len+
	cmd->_partner_id_len+
	cmd->_peerid_len+
	cmd->_url_len+
	cmd->_ref_url_len+
	cmd->_cid_len+
	cmd->_gcid_len+
	cmd->_file_name_len
	;

	cmd->_header._cmd_len = fix_len+other_len;
	cmd->_cmd_type = EMB_REPORT_COMMON_TASK_DL_STAT_CMD_TYPE;
	*len = sizeof(REP_CMD_HEADER) + cmd->_header._cmd_len;

	ret = sd_malloc(*len , (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("emb_reporter_build_common_task_download_stat_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);//压缩后重写长度

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)1); //条数
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)REPORTER_CMD_COMPRESS_TYPE_NONE); //压缩类型
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)(cmd->_header._cmd_len-12));//单条记录长度

	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cmd_type);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_thunder_version_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_thunder_version, (_int32)cmd->_thunder_version_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_partner_id_len);  
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_partner_id, (_int32)cmd->_partner_id_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_len);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_task_create_type);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_url_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_url, (_int32)cmd->_url_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_ref_url_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_ref_url, (_int32)cmd->_ref_url_len);	

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_cid, (_int32)cmd->_cid_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_gcid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_gcid, (_int32)cmd->_gcid_len);

	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_size);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_block_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_name_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_file_name, (_int32)cmd->_file_name_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_duration);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_avg_speed);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_download_stat);

	sd_assert( tmp_len == 0 );
	if(tmp_len != 0)
	{
		SAFE_DELETE(*buffer);
		return REPORTER_ERR_BUILD_EMB_REPORT;
	}
	
	ret = emb_reporter_package_stat_cmd(buffer, len);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("emb_reporter_build_common_task_download_stat_cmd, http package failed.");
		CHECK_VALUE(ret);
	}

#ifdef REPORTER_DEBUG
		{
			char test_buffer[1024];
			sd_memset(test_buffer,0,1024);
			str2hex( (char*)(*buffer), *len, test_buffer, 1024);
			LOG_DEBUG( "emb_reporter_build_common_task_download_stat_cmd:*buffer[%u]=%s" 
						,((_u32)(*len)), test_buffer);
		}
#endif /* REPORTER_DEBUG */

	return SUCCESS;
}

#ifdef ENABLE_BT

_int32 emb_reporter_build_bt_task_download_stat_cmd(char** buffer, _u32* len, EMB_REPORT_BT_TASK_DL_STAT_CMD* cmd)
{
	//static _u32 seq = 0;
	_int32 ret;
	char*	tmp_buf;
	_int32  tmp_len,fix_len=0,other_len=0;

	cmd->_header._version = EMB_REPORT_PROTOCOL_VER;
	cmd->_header._seq = seq++;

	/* begining from _client_version */
	fix_len=
	4+		//_u32	_record_count; 条数
	4+		//_u32	_zip_type;	压缩方式
	4+		//_u32	_record_len;	单条记录长度	
	4+		//_u32	_cmd_type;	
	4+		//_u32	thunder_version_len;
	4+		//_u32	partner_id_len;
	4+		//_u32	_peerid_size;
	4+		//_u32	_info_hash_size;
	4+		//_u32	_title_size;
	4+		//_u32	_file_num;
	8+		//_u64	_file_size;
	4+		//_u32	_piece_size;
	4+		//_u32	_duration;
	4		//_u32	_avg_speed;
	;

other_len=
	
	cmd->_thunder_version_len+
	cmd->_partner_id_len+
	cmd->_peerid_len+
	cmd->_info_hash_len+
	cmd->_file_title_len
	;

	cmd->_header._cmd_len = fix_len+other_len;
	cmd->_cmd_type = EMB_REPORT_BT_TASK_DL_STAT_CMD_TYPE;
	*len = sizeof(REP_CMD_HEADER) + cmd->_header._cmd_len;

	
	ret = sd_malloc(*len, (void**)buffer);  
	if(ret != SUCCESS)
	{
		LOG_DEBUG("emb_reporter_build_bt_task_download_stat_cmd, malloc cmd buffer failed.");
		CHECK_VALUE(ret);
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len); //压缩后重写长度

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)1); //条数
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)REPORTER_CMD_COMPRESS_TYPE_NONE); //压缩类型
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)(cmd->_header._cmd_len-12));//单条记录长度

	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cmd_type);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_thunder_version_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_thunder_version, (_int32)cmd->_thunder_version_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_partner_id_len);  
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_partner_id, (_int32)cmd->_partner_id_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_info_hash_len);
	sd_set_bytes(&tmp_buf, &tmp_len, (char *)cmd->_info_hash, (_int32)cmd->_info_hash_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_title_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_file_title, (_int32)cmd->_file_title_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_num);

	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_total_size);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_piece_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_duration);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_avg_speed);
   
	sd_assert( tmp_len == 0 );
	if(tmp_len != 0)
	{
		SAFE_DELETE(*buffer);
		return REPORTER_ERR_BUILD_EMB_REPORT;
	}
	
	ret = emb_reporter_package_stat_cmd(buffer, len);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("emb_reporter_build_bt_task_download_stat_cmd, http package failed.");
		CHECK_VALUE(ret);
	}


#ifdef REPORTER_DEBUG
		{
			char test_buffer[1024];
			sd_memset(test_buffer,0,1024);
			str2hex( (char*)(*buffer), *len, test_buffer, 1024);
			LOG_DEBUG( "emb_reporter_build_bt_task_download_stat_cmd:*buffer[%u]=%s" 
						,((_u32)(*len)), test_buffer);
		}
#endif /* REPORTER_DEBUG */
	return SUCCESS;
}


_int32 emb_reporter_build_bt_file_download_stat_cmd(char** buffer, _u32* len, EMB_REPORT_BT_FILE_DL_STAT_CMD* cmd)
{
	//static _u32 seq = 0;
	_int32 ret;
	char*	tmp_buf;
	_int32  tmp_len,fix_len=0,other_len=0;

    cmd->_header._version = EMB_REPORT_PROTOCOL_VER;
	cmd->_header._seq = seq++;

	/* begining from _client_version */
	fix_len=
	4+		//_u32	_record_count; 条数
	4+		//_u32	_zip_type;	压缩方式
	4+		//_u32	_record_len;	单条记录长度
	4+		//_u32	_cmd_type;	
	4+		//_u32	thunder_version_len;
	4+		//_u32	partner_id_len;
	4+		//_u32	_peerid_size;
	4+		//_u32	_info_hash_size;
	4+		//_u32	_title_size;
	4+		//_u32	_file_num;
	8+		//_u64	_file_total_size;
	4+		//_u32	_piece_size;
	4+		//_u32	_file_index;
	8+		//_u64	_file_offset;
	4+		//_u32	_cid_size;
	4+		//_u32	_gcid_size;
	8+		//_u64	_file_size;
	4+		//_u64	_file_name_len;
	4+		//_u32	_duration;
	4+		//_u32	_avg_speed;
	4		//_u32	_download_state;
	;

other_len=
	
	cmd->_thunder_version_len+
	cmd->_partner_id_len+
	cmd->_peerid_len+
	cmd->_info_hash_len+
	cmd->_file_title_len+
	cmd->_cid_len+
	cmd->_gcid_len+
	cmd->_file_name_len
	;

	cmd->_header._cmd_len = fix_len+other_len;
	cmd->_cmd_type = EMB_REPORT_BT_FILE_DL_STAT_CMD_TYPE;
	*len = sizeof(REP_CMD_HEADER) + cmd->_header._cmd_len;

	ret = sd_malloc(*len, (void**)buffer);  
	if(ret != SUCCESS)
	{
		LOG_DEBUG("emb_reporter_build_bt_file_download_stat_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);//压缩后重写长度

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)1); //条数
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)REPORTER_CMD_COMPRESS_TYPE_NONE); //压缩类型
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)(cmd->_header._cmd_len-12));//单条记录长度

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cmd_type);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_thunder_version_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_thunder_version, (_int32)cmd->_thunder_version_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_partner_id_len);  
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_partner_id, (_int32)cmd->_partner_id_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_info_hash_len);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_info_hash, (_int32)cmd->_info_hash_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_title_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_file_title, (_int32)cmd->_file_title_len);


	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_num);

	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_total_size);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_piece_size);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_index );
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_offset );

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_cid, (_int32)cmd->_cid_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_gcid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_gcid, (_int32)cmd->_gcid_len);


	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_name_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_file_name, (_int32)cmd->_file_name_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_duration);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_avg_speed);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_download_stat);
	
  	sd_assert( tmp_len == 0 );
	if(tmp_len != 0)
	{
		SAFE_DELETE(*buffer);
		return REPORTER_ERR_BUILD_EMB_REPORT;
	}
	
	ret = emb_reporter_package_stat_cmd(buffer, len);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("emb_reporter_build_bt_file_download_stat_cmd, http package failed.");
		CHECK_VALUE(ret);
	}


#ifdef REPORTER_DEBUG
		{
			char test_buffer[1024];
			sd_memset(test_buffer,0,1024);
			str2hex( (char*)(*buffer), *len, test_buffer, 1024);
			LOG_DEBUG( "emb_reporter_build_bt_file_download_stat_cmd:*buffer[%u]=%s" 
						,((_u32)(*len)), test_buffer);
		}
#endif /* REPORTER_DEBUG */
	return SUCCESS;
}
#endif

_int32 emb_reporter_build_common_task_download_fail_cmd(char** buffer, _u32* len, EMB_REPORT_COMMON_TASK_DL_FAIL_CMD* cmd)
{
	//static _u32 seq = 0;
	_int32 ret;
	char*	tmp_buf;
	_int32	tmp_len,fix_len=0,other_len=0;

    cmd->_header._version = EMB_REPORT_PROTOCOL_VER;
	cmd->_header._seq = seq++;

	/* begining from _client_version */
	fix_len=
	4+		//_u32	_record_count; 条数
	4+		//_u32	_zip_type;	压缩方式
	4+		//_u32	_record_len;	单条记录长度
	
	4+		//_u32	_cmd_type;	
	4+		//_u32	thunder_version_len;
	4+		//_u32	partner_id_len;
	4+		//_u32	_peerid_size;
	4+		//_u32	_task_create_type;
	4+		//_u32	_url_len;
	4+		//_u32	_ref_url_len;
	4+		//_u32	_cid_size;
	4+		//_u32	_gcid_size;
	8+		//_u64	_file_size;
	4+		//_u32	_block_size;
	4+		//_u32	_file_name_len;
	4+		//_u32	_fail_code;
	4+		//_u32	_percent;
	4+		//_u32	_ip_len;
	4+		//_u32	_duration;
	4+		//_u32	_avg_speed;
	4		//_u32	_download_state;
	;

other_len=
	
	cmd->_thunder_version_len+
	cmd->_partner_id_len+
	cmd->_peerid_len+
	cmd->_url_len+
	cmd->_ref_url_len+
	cmd->_cid_len+
	cmd->_gcid_len+
	cmd->_file_name_len+
	cmd->_ip_len
	;

	cmd->_header._cmd_len = fix_len+other_len;
	cmd->_cmd_type = EMB_REPORT_COMMON_TASK_DL_FAIL_CMD_TYPE;
	*len = sizeof(REP_CMD_HEADER) + cmd->_header._cmd_len;

	ret = sd_malloc(*len, (void**)buffer);	
	if(ret != SUCCESS)
	{
		LOG_DEBUG("emb_reporter_build_common_task_download_fail_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);//压缩后重写长度

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)1); //条数
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)REPORTER_CMD_COMPRESS_TYPE_NONE); //压缩类型
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)(cmd->_header._cmd_len-12));//单条记录长度

	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cmd_type);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_thunder_version_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_thunder_version, (_int32)cmd->_thunder_version_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_partner_id_len);  
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_partner_id, (_int32)cmd->_partner_id_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_len);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_task_create_type);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_url_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_url, (_int32)cmd->_url_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_ref_url_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_ref_url, (_int32)cmd->_ref_url_len); 

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_cid, (_int32)cmd->_cid_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_gcid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_gcid, (_int32)cmd->_gcid_len);

	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_size);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_block_size);


	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_name_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_file_name, (_int32)cmd->_file_name_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_fail_code);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_percent);


	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_ip_len);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_local_ip, (_int32)cmd->_ip_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_duration);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_avg_speed);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_download_stat);
   
	sd_assert( tmp_len == 0 );
	if(tmp_len != 0)
	{
		SAFE_DELETE(*buffer);
		return REPORTER_ERR_BUILD_EMB_REPORT;
	}
	
	ret = emb_reporter_package_stat_cmd(buffer, len);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("emb_reporter_build_common_task_download_fail_cmd, http package failed.");
		CHECK_VALUE(ret);
	}


#ifdef REPORTER_DEBUG
		{
			char test_buffer[1024];
			sd_memset(test_buffer,0,1024);
			str2hex( (char*)(*buffer), *len, test_buffer, 1024);
			LOG_DEBUG( "emb_reporter_build_common_task_download_fail_cmd:*buffer[%u]=%s" 
						,((_u32)(*len)), test_buffer);
		}
#endif /* REPORTER_DEBUG */
	return SUCCESS;

}

#ifdef ENABLE_BT
_int32 emb_reporter_build_bt_file_download_fail_cmd(char** buffer, _u32* len, EMB_REPORT_BT_FILE_DL_FAIL_CMD* cmd)
{
	//static _u32 seq = 0;
	_int32 ret;
	char*	tmp_buf;
	_int32  tmp_len,fix_len=0,other_len=0;

	cmd->_header._version = EMB_REPORT_PROTOCOL_VER;
	cmd->_header._seq = seq++;

	/* begining from _client_version */
	fix_len=
	4+		//_u32	_record_count; 条数
	4+		//_u32	_zip_type;	压缩方式
	4+		//_u32	_record_len;	单条记录长度
	4+		//_u32	_cmd_type;	
	4+		//_u32	thunder_version_len;
	4+		//_u32	partner_id_len;
	4+		//_u32	_peerid_size;
	4+		//_u32	_info_hash_size;
	4+		//_u32	_title_size;
	4+		//_u32	_file_num;
	8+		//_u64	_file_total_size;
	4+		//_u32	_piece_size;
	4+		//_u32	_file_index;
	8+		//_u64	_file_offset;
	4+		//_u32	_cid_size;
	4+		//_u32	_gcid_size;
	8+		//_u64	_file_size;
	4+		//_u32	_file_name_len;

	4+		//_u32	_fail_code;
	4+		//_u32	_percent;
	4+		//_u32	_ip_len;

	4+		//_u32	_duration;
	4+		//_u32	_avg_speed;
	4		//_u32	_download_state;
	;

other_len=
	
	cmd->_thunder_version_len+
	cmd->_partner_id_len+
	cmd->_peerid_len+
	cmd->_info_hash_len+
	cmd->_file_title_len+
	cmd->_cid_len+
	cmd->_gcid_len+
	cmd->_file_name_len+
	cmd->_ip_len
	;

	cmd->_header._cmd_len = fix_len+other_len;
	cmd->_cmd_type = EMB_REPORT_BT_FILE_DL_FAIL_CMD_TYPE;
	*len = sizeof(REP_CMD_HEADER) + cmd->_header._cmd_len;

	ret = sd_malloc(*len, (void**)buffer);  
	if(ret != SUCCESS)
	{
		LOG_DEBUG("emb_reporter_build_bt_file_download_fail_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);//压缩后重写长度

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)1); //条数
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)REPORTER_CMD_COMPRESS_TYPE_NONE); //压缩类型
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)(cmd->_header._cmd_len-12));//单条记录长度

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cmd_type);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_thunder_version_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_thunder_version, (_int32)cmd->_thunder_version_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_partner_id_len);  
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_partner_id, (_int32)cmd->_partner_id_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_info_hash_len);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_info_hash, (_int32)cmd->_info_hash_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_title_len);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_file_title, (_int32)cmd->_file_title_len);


	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_num);

	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_total_size);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_piece_size);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_index );
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_offset );

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_cid, (_int32)cmd->_cid_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_gcid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_gcid, (_int32)cmd->_gcid_len);


	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_name_len);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_file_name, (_int32)cmd->_file_name_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_fail_code);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_percent);


	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_ip_len);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_local_ip, (_int32)cmd->_ip_len);


	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_duration);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_avg_speed);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_download_stat);
	
	sd_assert( tmp_len == 0 );
	if(tmp_len != 0)
	{
		SAFE_DELETE(*buffer);
		return REPORTER_ERR_BUILD_EMB_REPORT;
	}
	
	ret = emb_reporter_package_stat_cmd(buffer, len);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("emb_reporter_build_bt_file_download_fail_cmd, http package failed.");
		CHECK_VALUE(ret);
	}


#ifdef REPORTER_DEBUG
		{
			char test_buffer[1024];
			sd_memset(test_buffer,0,1024);
			str2hex( (char*)(*buffer), *len, test_buffer, 1024);
			LOG_DEBUG( "emb_reporter_build_bt_file_download_fail_cmd:*buffer[%u]=%s" 
						,((_u32)(*len)), test_buffer);
		}
#endif /* REPORTER_DEBUG */
	return SUCCESS;
}
#endif

_int32 emb_reporter_build_dns_abnormal_cmd(char** buffer, _u32* len, EMB_DNS_ABNORMAL_CMD* cmd)
{
	//static _u32 seq = 0;
	_int32 ret = SUCCESS;
	char*	tmp_buf;
	_int32  tmp_len=0;
	_u32 fix_len=0,other_len=0,list_len=0, list_index=0;
	LIST_ITERATOR cur_node = NULL;
	_u32 dns_ip_list_size = 0, parse_ip_list_size = 0;
	_u32 ip = 0;
	
	cmd->_header._version = EMB_REPORT_PROTOCOL_VER;
	cmd->_header._seq = seq++;

	dns_ip_list_size = list_size( cmd->_dns_ip_list_ptr );
	dns_ip_list_size = MIN( DNS_SERVER_IP_COUNT_MAX, dns_ip_list_size );
	list_len += dns_ip_list_size * ( sizeof(_u32) ) + 4;

	parse_ip_list_size = list_size( cmd->_parse_ip_list_ptr );
	parse_ip_list_size = MIN( DNS_CONTENT_IP_COUNT_MAX, parse_ip_list_size );
	list_len += parse_ip_list_size * ( sizeof(_u32) ) + 4;


	/* begining from _client_version */
	fix_len=
	4+		//_u32	_record_count; 条数
	4+		//_u32	_zip_type;	压缩方式
	4+		//_u32	_record_len;	单条记录长度
	4+		//_u32	_cmd_type;	
	4+		//_u32	thunder_version_len;
	4+		//_u32	partner_id_len;
	4+		//_u32	_peerid_size;
	4+		//_u32	_err_code;

	4		//_u32	_hub_domain;
	;

other_len=
	
	cmd->_thunder_version_len+
	cmd->_partner_id_len+
	cmd->_peerid_len+
	cmd->_hub_domain_len
	;

	cmd->_header._cmd_len = fix_len+other_len + list_len;
	cmd->_cmd_type = EMB_REPORT_DNS_ABNORMAL_CMD_TYPE;
	*len = sizeof(REP_CMD_HEADER) + cmd->_header._cmd_len;

	ret = sd_malloc(*len, (void**)buffer); 
	if(ret != SUCCESS)
	{
		LOG_DEBUG("emb_reporter_build_dns_abnormal_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);//压缩后重写长度

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)1); //条数
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)REPORTER_CMD_COMPRESS_TYPE_NONE); //压缩类型
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)(cmd->_header._cmd_len-12));//单条记录长度

	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cmd_type);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_thunder_version_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_thunder_version, (_int32)cmd->_thunder_version_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_partner_id_len);  
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_partner_id, (_int32)cmd->_partner_id_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_err_code);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, dns_ip_list_size);

	cur_node = LIST_BEGIN( *cmd->_dns_ip_list_ptr );
	list_index = 0;
	while( cur_node != LIST_END( *cmd->_dns_ip_list_ptr ) 
		&& list_index < dns_ip_list_size )
	{
		ip = (_u32)LIST_VALUE( cur_node );	
		LOG_DEBUG("emb_reporter_build_dns_abnormal_cmd, dns_ip:%u.", ip);
		
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, ip );
		list_index++;
		cur_node = LIST_NEXT( cur_node );
	}
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_domain_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_hub_domain_str, (_int32)cmd->_hub_domain_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, parse_ip_list_size);

	cur_node = LIST_BEGIN( *cmd->_parse_ip_list_ptr );
	list_index = 0;
	while( cur_node != LIST_END( *cmd->_parse_ip_list_ptr ) 
		&& list_index < parse_ip_list_size )
	{
		ip = (_u32)LIST_VALUE( cur_node );		
		
		LOG_DEBUG("emb_reporter_build_dns_abnormal_cmd, parse_ip:%u.", ip);
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, ip );
		list_index++;
		cur_node = LIST_NEXT( cur_node );
	}
   
	sd_assert( tmp_len == 0 );
	if(tmp_len != 0)
	{
		SAFE_DELETE(*buffer);
		return REPORTER_ERR_BUILD_EMB_REPORT;
	}
	
	ret = emb_reporter_package_stat_cmd(buffer, len);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("emb_reporter_build_dns_abnormal_cmd, http package failed.");
		CHECK_VALUE(ret);
	}


#ifdef REPORTER_DEBUG
		{
			char test_buffer[1024];
			sd_memset(test_buffer,0,1024);
			str2hex( (char*)(*buffer), *len, test_buffer, 1024);
			LOG_DEBUG( "emb_reporter_build_dns_abnormal_cmd:*buffer[%u]=%s" 
						,((_u32)(*len)), test_buffer);
		}
#endif /* REPORTER_DEBUG */
	return SUCCESS;
}


_int32 emb_reporter_build_online_peer_state_cmd(char** buffer, _u32* len, EMB_REPORT_ONLINE_PEER_STATE_CMD* cmd)
{
	//static _u32 seq = 0;
	_int32 ret;
	char*	tmp_buf;
	_int32  tmp_len,fix_len=0,other_len=0;
	//char http_header[1024] = {0};

    cmd->_header._version = EMB_REPORT_PROTOCOL_VER;
	cmd->_header._seq = seq++;

	/* begining from _client_version */
	fix_len=
	4+		//_u32	_record_count; 条数
	4+		//_u32	_zip_type;	压缩方式
	4+		//_u32	_record_len;	单条记录长度
	4+		//_u32	_cmd_type;	
	4+		//_u32	thunder_version_len;
	4+		//_u32	partner_id_len;
	4+		//_u32	_peerid_size;
	4+		//_u32	_ip_len;
	4+		//_u32	_mempool_size;
	4+		//_u32	_os_type;
	4+		//_u32	_download_speed;
	4+		//_u32	_download_max_speed;
	4+		//_u32	_upload_speed;
	4+		//_u32	_upload_max_speed;
	4+		//_u32	_res_num;
	4		//_u32	_pipe_num;
	;

other_len=
	
	cmd->_thunder_version_len+
	cmd->_partner_id_len+
	cmd->_peerid_len+
	cmd->_ip_name_len+
	cmd->_os_name_len
	;

	cmd->_header._cmd_len = fix_len+other_len;
	cmd->_cmd_type = EMB_REPORT_ONLINE_PEER_STATE_CMD_TYPE;
	*len = sizeof(REP_CMD_HEADER) + cmd->_header._cmd_len;

	ret = sd_malloc(*len, (void**)buffer); 
	if(ret != SUCCESS)
	{
		LOG_DEBUG("emb_reporter_build_online_peer_state_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);//压缩后重写长度

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)1); //条数
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)REPORTER_CMD_COMPRESS_TYPE_NONE); //压缩类型
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)(cmd->_header._cmd_len-12));//单条记录长度

	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cmd_type);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_thunder_version_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_thunder_version, (_int32)cmd->_thunder_version_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_partner_id_len);  
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_partner_id, (_int32)cmd->_partner_id_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_len);
	

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_ip_name_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_ip_name, (_int32)cmd->_ip_name_len);


	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_memory_pool_size);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_os_name_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_os_name, (_int32)cmd->_os_name_len);


	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_download_speed);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_download_max_speed);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_upload_speed);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_upload_max_speed);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_num);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_pipe_num);
	
	sd_assert( tmp_len == 0 );
	if(tmp_len != 0)
	{
		SAFE_DELETE(*buffer);
		return REPORTER_ERR_BUILD_EMB_REPORT;
	}
	
	ret = emb_reporter_package_stat_cmd(buffer, len);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("emb_reporter_build_online_peer_state_cmd, http package failed.");
		CHECK_VALUE(ret);
	}


#ifdef REPORTER_DEBUG
		{
			char test_buffer[1024];
			sd_memset(test_buffer,0,1024);
			str2hex( (char*)(*buffer), *len, test_buffer, 1024);
			LOG_DEBUG( "emb_reporter_build_online_peer_state_cmd:*buffer[%u]=%s" 
						,((_u32)(*len)), test_buffer);
		}
#endif /* REPORTER_DEBUG */

	return SUCCESS;
}



_int32 emb_reporter_build_common_stop_task_cmd(char** buffer, _u32* len, EMB_REPORT_COMMON_STOP_TASK_CMD* cmd)
{
	//static _u32 seq = 0;
	_int32 ret;
	char*	tmp_buf;
	_int32  tmp_len,fix_len=0,other_len=0;
	//char http_header[1024] = {0};

    cmd->_header._version = EMB_REPORT_PROTOCOL_VER;
	cmd->_header._seq = seq++;

	/* begining from _client_version */
	fix_len=
	4+		//_u32	_record_count; 条数
	4+		//_u32	_zip_type;	压缩方式
	4+		//_u32	_record_len;	单条记录长度
	4+		//_u32	_cmd_type;	
	4+		//_u32	thunder_version_len;
	4+		//_u32	partner_id_len;
	4+		//_u32	_peerid_size;
	
	4+		//_u32	_task_create_type;
	4+		//_u32	_url_len;
	4+		//_u32	_ref_url_len;
	4+		//_u32	_cid_size;
	4+		//_u32	_gcid_size;
	8+		//_u64	_file_size;
	4+		//_u32	_block_size;
	4+		//_u32	_file_name_len;
	4+		//_u32	_duration;
	4+		//_u32	_avg_speed;

	4+		//_u32	_task_type;
	
	4+		//_u32	_vod_play_time;
	4+		//_u32	_vod_first_buffer_time;
	4+		//_u32	_vod_interrupt_times;
	4+		//_u32	_min_interrupt_time;
	4+		//_u32	_max_interrupt_time;
	4+		//_u32	_avg_interrupt_time;

	8+		//_u64	_server_dl_bytes;
	8+		//_u64	_peer_dl_bytes;
	8+		//_u64	_cdn_dl_bytes;
	
	4+		//_u32	_task_status;
	4+		//_u32	_failed_code;
	4+		//_u32	_dl_percent;

//add new
//////////////////vod_report

	4+      //_u32 _bit_rate;			        //比特率
	4+      //_u32 _vod_total_buffer_time;    //总的播放缓冲时间（秒）
	4+      //_u32 _vod_total_drag_wait_time;	//播放拖动后的等待播放时间（秒）
	4+      //_u32 _vod_drag_num;		        //拖动次数
	4+      //_u32 _play_interrupt_1;         // 1分钟内中断
	4+      //_u32 _play_interrupt_2;	        // 2分钟内中断
	4+      //_u32 _play_interrupt_3;		    //6分钟内中断
	4+      //_u32 _play_interrupt_4;		    //10分钟内中断
	4+      //_u32 _play_interrupt_5;		    //15分钟内中断
	4+      //_u32 _play_interrupt_6;		    //15分钟以上中断
	4+      //_u32 _cdn_stop_times;		    //关闭cdn次数
	
//////file_info_report
	8+		 //_down_exist; 				 //断点续传前数据量 																
	8+		 //_overlap_bytes;				 //重叠数据大小 	
	
	8+		 //_down_n2i_no;				 //内网从外网非边下边传下载字节数													
	8+		 //_down_n2i_yes;				 //内网从外网边下边传下载字节数 													
	8+		 //_down_n2n_no;				 //内网从内网非边下边传下载字节数	(包括同子网)												
	8+		 //_down_n2n_yes;				 //内网从内网边下边传下载字节数 	(包括同子网)												
	8+		 //_down_n2s_no;				 //同子网内网非边下边传下载字节数													
	8+		 //_down_n2s_yes;				 //同子网内网边下边传下载字节数 													
	8+		 //_down_i2i_no;				 //外网从外网非边下边传下载字节数													
	8+		 //_down_i2i_yes;				 //外网从外网边下边传下载字节数 													
	8+		 //_down_i2n_no;				 //外网从内网非边下边传下载字节数													
	8+		 //_down_i2n_yes;				 //外网从内网边下边传下载字节数 	
	
	8+		 //_down_by_partner_cdn;		      //partner Cdn下载字节		
	8+		 //_down_err_by_cdn;			 //Cdn纠错字节		
	8+		 //_down_err_by_partner_cdn;		//partner Cdn纠错字节		
	8+		 //_down_err_by_peer;			 //Peer纠错字节 																	
	8+		 //_down_err_by_svr;			 //Server纠错字节	

	//////cm_report
	4+		 //_res_server_total;			 //server资源总数																	
	4+		 //_res_server_valid;			 //有效server资源总数																
	4+		 //_res_cdn_total;				 //CDN资源总数																		
	4+		 //_res_cdn_valid;				 //有效CDN资源总数		
	4+		 //_res_partner_cdn_total;		//partner_CDN资源总数																		
	4+		 //_res_partner_cdn_valid;		//有效partner_CDN资源总数		
	4+		 //_cdn_res_max_num;			 //同时使用最大Cdn资源个数															
	4+		 //_cdn_connect_failed_times;	 //Cdn 使用失败次数 	

	
	4+		 //_res_n2i_total;				 //返回的所有内网到外网的资源数 														
	4+		 //_res_n2i_valid;				 //有效的内网到外网的资源数 	
	4+		 //_res_n2n_total;				 //返回的所有内网到内网的资源数 (包括同子网)													
	4+		 //_res_n2n_valid;				 //有效的内网到内网的资源数 (包括同子网)														
	4+		 //_res_n2s_total;				 //所有内网到同子网内网的资源数 													
	4+		 //_res_n2s_valid;				 //有效的内网到同子网内网的资源数													
	4+		 //_res_i2i_total;				 //返回的所有外网到外网的资源数 													
	4+		 //_res_i2i_valid;				 //有效的外网到外网的资源数 														
	4+		 //_res_i2n_total;				 //所有外网到内网的资源数															
	4+		 //_res_i2n_valid;				 //有效的外网到内网的资源数 	
	
	4+		 //_hub_res_total;					 //连上的Hub资源总数																
	4+		 //_hub_res_valid;					 //连上的Hub有效资源总数															
	4+		 //_active_tracker_res_total;	 //连上的Tracker主动资源总数														
	4+		 //_active_tracker_res_valid;	 //连上的Tracker有效主动资源总数													

//////res_query_report
	
	4+		 //_hub_s_max;					 //Shub查询的最大时间(s)															
	4+		 //_hub_s_min;					 //Shub查询的最小时间(s)															
	4+		 //_hub_s_avg;					 //Shub查询的平均时间(s)															
	4+		 //_hub_s_succ; 				 //Shub查询成功次数 																
	4+		 //_hub_s_fail; 				 //Shub查询失败次数 	
	
	4+		 //_hub_p_max;					 //Phub查询的最大时间(s)															
	4+		 //_hub_p_min;					 //Phub查询的最小时间(s)															
	4+		 //_hub_p_avg;					 //Phub查询的平均时间(s)															
	4+		 //_hub_p_succ; 				 //Phub查询成功次数 																
	4+		 //_hub_p_fail; 				 //Phub查询失败次数 	
	
	4+		 //_hub_t_max;					 //Tracker查询的最大时间(s) 														
	4+		 //_hub_t_min;					 //Tracker查询的最小时间(s) 														
	4+		 //_hub_t_avg;					 //Tracker查询的平均时间(s) 														
	4+		 //_hub_t_succ; 				 //Tracker查询成功次数																
	4+		 //_hub_t_fail; 				 //Tracker查询失败次数	
	4+		 //_network_info;				 //当前的网络信息
	8+           // appacc_dl_bytes
	8             // appacc_err_down_bytes
	;

other_len=
	cmd->_thunder_version_len+
	cmd->_partner_id_len+
	cmd->_peerid_len+
	cmd->_url_len+
	cmd->_ref_url_len+
	cmd->_cid_len+
	cmd->_gcid_len+
	cmd->_file_name_len
	;

	cmd->_header._cmd_len = fix_len+other_len;
	cmd->_cmd_type = EMB_REPORT_COMMON_STOP_TASK_CMD_TYPE;
	*len = sizeof(REP_CMD_HEADER) + cmd->_header._cmd_len;

	ret = sd_malloc(*len, (void**)buffer);  
	if(ret != SUCCESS)
	{
		LOG_DEBUG("emb_reporter_build_common_stop_task_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);//压缩后重写长度

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)1); //条数
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)REPORTER_CMD_COMPRESS_TYPE_NONE); //压缩类型
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)(cmd->_header._cmd_len-12));//单条记录长度

	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cmd_type);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_thunder_version_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_thunder_version, (_int32)cmd->_thunder_version_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_partner_id_len);  
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_partner_id, (_int32)cmd->_partner_id_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_task_create_type);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_url_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_url, (_int32)cmd->_url_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_ref_url_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_ref_url, (_int32)cmd->_ref_url_len);	

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_cid, (_int32)cmd->_cid_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_gcid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_gcid, (_int32)cmd->_gcid_len);

	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_size);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_block_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_name_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_file_name, (_int32)cmd->_file_name_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_duration);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_avg_speed);
	

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_task_type);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_vod_play_time);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_vod_first_buffer_time);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_vod_interrupt_times);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_min_interrupt_time);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_max_interrupt_time);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_avg_interrupt_time);

	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_server_dl_bytes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_peer_dl_bytes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_cdn_dl_bytes);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_task_status);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_failed_code);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_dl_percent);

///add new
//////////////////vod_report
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_bit_rate);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_vod_total_buffer_time);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_vod_total_drag_wait_time);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_vod_drag_num);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_play_interrupt_1);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_play_interrupt_2);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_play_interrupt_3);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_play_interrupt_4);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_play_interrupt_5);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_play_interrupt_6);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cdn_stop_times);

//////file_info_report
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_exist);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_overlap_bytes);
	
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_n2i_no);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_n2i_yes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_n2n_no);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_n2n_yes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_n2s_no);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_n2s_yes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_i2i_no);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_i2i_yes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_i2n_no);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_i2n_yes);
	
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_by_partner_cdn);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_err_by_cdn);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_err_by_partner_cdn);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_err_by_peer);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_err_by_svr);

//////cm_report
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_server_total);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_server_valid);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_cdn_total);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_cdn_valid);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_partner_cdn_total);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_partner_cdn_valid);	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cdn_res_max_num);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cdn_connect_failed_times);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_n2i_total);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_n2i_valid);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_n2n_total);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_n2n_valid);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_n2s_total);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_n2s_valid);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_i2i_total);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_i2i_valid);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_i2n_total);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_i2n_valid);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_res_total);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_res_valid);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_active_tracker_res_total);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_active_tracker_res_valid);
	
//////res_query_report
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_s_max);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_s_min);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_s_avg);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_s_succ);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_s_fail);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_p_max);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_p_min);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_p_avg);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_p_succ);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_p_fail);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_t_max);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_t_min);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_t_avg);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_t_succ);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_t_fail);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_network_info);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_appacc_dl_bytes);
	ret = sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_err_by_appacc);	
	
	sd_assert( ret == SUCCESS );
    
	sd_assert( tmp_len == 0 );
	if(tmp_len != 0)
	{
		SAFE_DELETE(*buffer);
		return REPORTER_ERR_BUILD_EMB_REPORT;
	}

		
	ret = emb_reporter_package_stat_cmd(buffer, len);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("emb_reporter_build_common_stop_task_cmd, http package failed.");
		CHECK_VALUE(ret);
	}

#ifdef REPORTER_DEBUG
		{
			char test_buffer[1024];
			sd_memset(test_buffer,0,1024);
			str2hex( (char*)(*buffer), *len, test_buffer, 1024);
			LOG_DEBUG( "emb_reporter_build_common_stop_task_cmd:*buffer[%u]=%s" 
						,((_u32)(*len)), test_buffer);
		}
#endif /* REPORTER_DEBUG */
	return SUCCESS;
}


#ifdef ENABLE_BT

_int32 emb_reporter_build_bt_stop_task_cmd(char** buffer, _u32* len, EMB_REPORT_BT_STOP_TASK_CMD* cmd)
{
	//static _u32 seq = 0;
	_int32 ret;
	char*	tmp_buf;
	_int32  tmp_len,fix_len=0,other_len=0;

    cmd->_header._version = EMB_REPORT_PROTOCOL_VER;
	cmd->_header._seq = seq++;

	/* begining from _client_version */
	fix_len=
	4+		//_u32	_record_count; 条数
	4+		//_u32	_zip_type;	压缩方式
	4+		//_u32	_record_len;	单条记录长度
		4+		//_u32	_cmd_type;	
		4+		//_u32	thunder_version_len;
		4+		//_u32	partner_id_len;
		4+		//_u32	_peerid_size;
		4+		//_u32	_info_hash_size;
		4+		//_u32	_title_size;
		4+		//_u32	_file_num;
		8+		//_u64	_file_size;
		4+		//_u32	_piece_size;
		4+		//_u32	_duration;
		4+		//_u32	_avg_speed;
		
		4+		//_u32	_task_type;
	
		8+		//_u64	_server_dl_bytes;
		8+		//_u64	_peer_dl_bytes;
		8+		//_u64	_cdn_dl_bytes;
		
		4+		//_u32	_task_status;
		4+		//_u32	_failed_code;
		4+		//_u32	_dl_percent	
		4		//_u32  _network_info
		;


other_len=
	cmd->_thunder_version_len+
	cmd->_partner_id_len+
	cmd->_peerid_len+
	cmd->_info_hash_len+
	cmd->_file_title_len
	;

	cmd->_header._cmd_len = fix_len+other_len;
	cmd->_cmd_type = EMB_REPORT_BT_STOP_TASK_CMD_TYPE;
	*len = sizeof(REP_CMD_HEADER) + cmd->_header._cmd_len;

	ret = sd_malloc(*len, (void**)buffer);  
	if(ret != SUCCESS)
	{
		LOG_DEBUG("emb_reporter_build_bt_stop_task_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);//压缩后重写长度

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)1); //条数
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)REPORTER_CMD_COMPRESS_TYPE_NONE); //压缩类型
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)(cmd->_header._cmd_len-12));//单条记录长度

	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cmd_type);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_thunder_version_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_thunder_version, (_int32)cmd->_thunder_version_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_partner_id_len);  
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_partner_id, (_int32)cmd->_partner_id_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_info_hash_len);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_info_hash, (_int32)cmd->_info_hash_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_title_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_file_title, (_int32)cmd->_file_title_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_num);

	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_total_size);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_piece_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_duration);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_avg_speed);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_task_type);
	
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_server_dl_bytes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_peer_dl_bytes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_cdn_dl_bytes);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_task_status);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_failed_code);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_dl_percent);	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_network_info);	
    
	sd_assert( tmp_len == 0 );
	if(tmp_len != 0)
	{
		SAFE_DELETE(*buffer);
		return REPORTER_ERR_BUILD_EMB_REPORT;
	}

		
	ret = emb_reporter_package_stat_cmd(buffer, len);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("emb_reporter_build_bt_stop_task_cmd, http package failed.");
		CHECK_VALUE(ret);
	}

#ifdef REPORTER_DEBUG
		{
			char test_buffer[1024];
			sd_memset(test_buffer,0,1024);
			str2hex( (char*)(*buffer), *len, test_buffer, 1024);
			LOG_DEBUG( "emb_reporter_build_bt_stop_task_cmd:*buffer[%u]=%s" 
						,((_u32)(*len)), test_buffer);
		}
#endif /* REPORTER_DEBUG */
	return SUCCESS;
}


_int32 emb_reporter_build_bt_stop_file_cmd(char** buffer, _u32* len, EMB_REPORT_BT_STOP_FILE_CMD* cmd)
{
	//static _u32 seq = 0;
	_int32 ret;
	char*	tmp_buf;
	_int32  tmp_len,fix_len=0,other_len=0;

    cmd->_header._version = EMB_REPORT_PROTOCOL_VER;
	cmd->_header._seq = seq++;

	/* begining from _client_version */
	fix_len=
	4+		//_u32	_record_count; 条数
	4+		//_u32	_zip_type;	压缩方式
	4+		//_u32	_record_len;	单条记录长度
	4+		//_u32	_cmd_type;	
	4+		//_u32	thunder_version_len;
	4+		//_u32	partner_id_len;
	4+		//_u32	_peerid_size;
	4+		//_u32	_info_hash_size;
	4+		//_u32	_title_size;
	4+		//_u32	_file_num;
	8+		//_u64	_file_total_size;
	4+		//_u32	_piece_size;
	4+		//_u32	_file_index;
	8+		//_u64	_file_offset;
	4+		//_u32	_cid_size;
	4+		//_u32	_gcid_size;
	8+		//_u64	_file_size;
	4+		//_u64	_file_name_len;
	4+		//_u32	_duration;
	4+		//_u32	_avg_speed;
		
	4+		//_u32	_task_type;
	
	4+		//_u32	_vod_play_time;
	4+		//_u32	_vod_first_buffer_time;
	4+		//_u32	_vod_interrupt_times;
	4+		//_u32	_min_interrupt_time;
	4+		//_u32	_max_interrupt_time;
	4+		//_u32	_avg_interrupt_time;
	
	8+		//_u64	_server_dl_bytes;
	8+		//_u64	_peer_dl_bytes;
	8+		//_u64	_cdn_dl_bytes;
	
	4+		//_u32	_task_status;
	4+		//_u32	_failed_code;
	4+		//_u32	_dl_percent;
	
//add new
//////////////////vod_report

	4+		//_u32 _bit_rate;					//比特率
	4+		//_u32 _vod_total_buffer_time;	  //总的播放缓冲时间（秒）
	4+		//_u32 _vod_total_drag_wait_time;	//播放拖动后的等待播放时间（秒）
	4+		//_u32 _vod_drag_num;				//拖动次数
	4+		//_u32 _play_interrupt_1;		  // 1分钟内中断
	4+		//_u32 _play_interrupt_2;			// 2分钟内中断
	4+		//_u32 _play_interrupt_3;			//6分钟内中断
	4+		//_u32 _play_interrupt_4;			//10分钟内中断
	4+		//_u32 _play_interrupt_5;			//15分钟内中断
	4+		//_u32 _play_interrupt_6;			//15分钟以上中断
	4+		//_u32 _cdn_stop_times; 		//关闭cdn次数
	
//////file_info_report
	8+		 //_down_exist; 				 //断点续传前数据量 																
	8+		 //_overlap_bytes;				 //重叠数据大小 	
	
	8+		 //_down_n2i_no;				 //内网从外网非边下边传下载字节数													
	8+		 //_down_n2i_yes;				 //内网从外网边下边传下载字节数 													
	8+		 //_down_n2n_no;				 //内网从内网非边下边传下载字节数	(包括同子网)												
	8+		 //_down_n2n_yes;				 //内网从内网边下边传下载字节数 	(包括同子网)												
	8+		 //_down_n2s_no;				 //同子网内网非边下边传下载字节数													
	8+		 //_down_n2s_yes;				 //同子网内网边下边传下载字节数 													
	8+		 //_down_i2i_no;				 //外网从外网非边下边传下载字节数													
	8+		 //_down_i2i_yes;				 //外网从外网边下边传下载字节数 													
	8+		 //_down_i2n_no;				 //外网从内网非边下边传下载字节数													
	8+		 //_down_i2n_yes;				 //外网从内网边下边传下载字节数 	
	
	8+		 //_down_by_partner_cdn;			  //partner Cdn下载字节 	
	8+		 //_down_err_by_cdn;			 //Cdn纠错字节		
	8+		 //_down_err_by_partner_cdn;		//partner Cdn纠错字节		
	8+		 //_down_err_by_peer;			 //Peer纠错字节 																	
	8+		 //_down_err_by_svr;			 //Server纠错字节	

	//////cm_report
	4+		 //_res_server_total;			 //server资源总数																	
	4+		 //_res_server_valid;			 //有效server资源总数																
	4+		 //_res_cdn_total;				 //CDN资源总数																		
	4+		 //_res_cdn_valid;				 //有效CDN资源总数		
	4+		 //_res_partner_cdn_total;		//partner_CDN资源总数																		
	4+		 //_res_partner_cdn_valid;		//有效partner_CDN资源总数		
	4+		 //_cdn_res_max_num;			 //同时使用最大Cdn资源个数															
	4+		 //_cdn_connect_failed_times;	 //Cdn 使用失败次数 	
	
	4+		 //_res_n2i_total;				 //返回的所有内网到外网的资源数 														
	4+		 //_res_n2i_valid;				 //有效的内网到外网的资源数 	
	4+		 //_res_n2n_total;				 //返回的所有内网到内网的资源数 (包括同子网)													
	4+		 //_res_n2n_valid;				 //有效的内网到内网的资源数 (包括同子网)														
	4+		 //_res_n2s_total;				 //所有内网到同子网内网的资源数 													
	4+		 //_res_n2s_valid;				 //有效的内网到同子网内网的资源数													
	4+		 //_res_i2i_total;				 //返回的所有外网到外网的资源数 													
	4+		 //_res_i2i_valid;				 //有效的外网到外网的资源数 														
	4+		 //_res_i2n_total;				 //所有外网到内网的资源数															
	4+		 //_res_i2n_valid;				 //有效的外网到内网的资源数 	
	
	4+		 //_hub_res_total;					 //连上的Hub资源总数																
	4+		 //_hub_res_valid;					 //连上的Hub有效资源总数															
	4+		 //_active_tracker_res_total;	 //连上的Tracker主动资源总数														
	4+		 //_active_tracker_res_valid;	 //连上的Tracker有效主动资源总数													

//////res_query_report
	
	4+		 //_hub_s_max;					 //Shub查询的最大时间(s)															
	4+		 //_hub_s_min;					 //Shub查询的最小时间(s)															
	4+		 //_hub_s_avg;					 //Shub查询的平均时间(s)															
	4+		 //_hub_s_succ; 				 //Shub查询成功次数 																
	4+		 //_hub_s_fail; 				 //Shub查询失败次数 	
	
	4+		 //_hub_p_max;					 //Phub查询的最大时间(s)															
	4+		 //_hub_p_min;					 //Phub查询的最小时间(s)															
	4+		 //_hub_p_avg;					 //Phub查询的平均时间(s)															
	4+		 //_hub_p_succ; 				 //Phub查询成功次数 																
	4+		 //_hub_p_fail; 				 //Phub查询失败次数 	
	
	4+		 //_hub_t_max;					 //Tracker查询的最大时间(s) 														
	4+		 //_hub_t_min;					 //Tracker查询的最小时间(s) 														
	4+		 //_hub_t_avg;					 //Tracker查询的平均时间(s) 														
	4+		 //_hub_t_succ; 				 //Tracker查询成功次数																
	4+		 //_hub_t_fail; 				 //Tracker查询失败次数	
	4		 // _network_info
	;


other_len=
	
	cmd->_thunder_version_len+
	cmd->_partner_id_len+
	cmd->_peerid_len+
	cmd->_info_hash_len+
	cmd->_file_title_len+
	cmd->_cid_len+
	cmd->_gcid_len+
	cmd->_file_name_len
	;

	cmd->_header._cmd_len = fix_len+other_len;
	cmd->_cmd_type = EMB_REPORT_BT_STOP_FILE_CMD_TYPE;
	*len = sizeof(REP_CMD_HEADER) + cmd->_header._cmd_len;

	ret = sd_malloc(*len, (void**)buffer);  
	if(ret != SUCCESS)
	{
		LOG_DEBUG("emb_reporter_build_bt_stop_file_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);//压缩后重写长度

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)1); //条数
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)REPORTER_CMD_COMPRESS_TYPE_NONE); //压缩类型
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)(cmd->_header._cmd_len-12));//单条记录长度

	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cmd_type);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_thunder_version_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_thunder_version, (_int32)cmd->_thunder_version_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_partner_id_len);  
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_partner_id, (_int32)cmd->_partner_id_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_info_hash_len);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_info_hash, (_int32)cmd->_info_hash_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_title_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_file_title, (_int32)cmd->_file_title_len);


	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_num);

	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_total_size);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_piece_size);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_index );
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_offset );

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_cid, (_int32)cmd->_cid_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_gcid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_gcid, (_int32)cmd->_gcid_len);


	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_name_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_file_name, (_int32)cmd->_file_name_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_duration);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_avg_speed);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_task_type);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_vod_play_time);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_vod_first_buffer_time);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_vod_interrupt_times);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_min_interrupt_time);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_max_interrupt_time);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_avg_interrupt_time);

	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_server_dl_bytes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_peer_dl_bytes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_cdn_dl_bytes);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_task_status);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_failed_code);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_dl_percent);

///add new
//////////////////vod_report
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_bit_rate);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_vod_total_buffer_time);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_vod_total_drag_wait_time);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_vod_drag_num);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_play_interrupt_1);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_play_interrupt_2);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_play_interrupt_3);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_play_interrupt_4);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_play_interrupt_5);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_play_interrupt_6);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cdn_stop_times);

//////file_info_report
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_exist);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_overlap_bytes);
	
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_n2i_no);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_n2i_yes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_n2n_no);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_n2n_yes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_n2s_no);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_n2s_yes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_i2i_no);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_i2i_yes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_i2n_no);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_i2n_yes);
	
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_by_partner_cdn);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_err_by_cdn);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_err_by_partner_cdn);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_err_by_peer);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_err_by_svr);

//////cm_report
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_server_total);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_server_valid);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_cdn_total);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_cdn_valid);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_partner_cdn_total);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_partner_cdn_valid);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cdn_res_max_num);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cdn_connect_failed_times);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_n2i_total);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_n2i_valid);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_n2n_total);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_n2n_valid);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_n2s_total);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_n2s_valid);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_i2i_total);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_i2i_valid);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_i2n_total);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_i2n_valid);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_res_total);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_res_valid);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_active_tracker_res_total);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_active_tracker_res_valid);
	
//////res_query_report
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_s_max);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_s_min);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_s_avg);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_s_succ);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_s_fail);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_p_max);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_p_min);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_p_avg);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_p_succ);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_p_fail);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_t_max);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_t_min);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_t_avg);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_t_succ);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_t_fail);
	ret = sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_network_info);
	sd_assert( ret == SUCCESS );
	
	sd_assert( tmp_len == 0 );
	if(tmp_len != 0)
	{
		SAFE_DELETE(*buffer);
		return REPORTER_ERR_BUILD_EMB_REPORT;
	}

		
	ret = emb_reporter_package_stat_cmd(buffer, len);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("emb_reporter_build_bt_stop_file_cmd, http package failed.");
		CHECK_VALUE(ret);
	}

#ifdef REPORTER_DEBUG
		{
			char test_buffer[1024];
			sd_memset(test_buffer,0,1024);
			str2hex( (char*)(*buffer), *len, test_buffer, 1024);
			LOG_DEBUG( "emb_reporter_build_bt_stop_file_cmd:*buffer[%u]=%s" 
						,((_u32)(*len)), test_buffer);
		}
#endif /* REPORTER_DEBUG */
	return SUCCESS;
}


#endif


#ifdef ENABLE_EMULE

_int32 emb_reporter_build_emule_stop_task_cmd(char** buffer, _u32* len, EMB_REPORT_EMULE_STOP_TASK_CMD* cmd)
{
	//static _u32 seq = 0;
	_int32 ret;
	char*	tmp_buf;
	_int32  tmp_len,fix_len=0,other_len=0;

    cmd->_header._version = EMB_REPORT_PROTOCOL_VER;
	cmd->_header._seq = seq++;

	/* begining from _client_version */
	fix_len=
	4+		//_u32	_record_count; 条数
	4+		//_u32	_zip_type;	压缩方式
	4+		//_u32	_record_len;	单条记录长度
	4+		//_u32	_cmd_type;	
	4+		//_u32	thunder_version_len;
	4+		//_u32	partner_id_len;
	4+		//_u32	_peerid_size;
	
	4+		//_u32	_fileid_size;
	
	4+		//_u32	_aich_hash_len;
	4+		//_u32	_part_hash_len;
	
	4+		//_u32	_cid_size;
	4+		//_u32	_gcid_size;
	
	8+		//_u64	_file_size;
	4+		//_u32	_file_name_len;
	4+		//_u32	_duration;
	4+		//_u32	_avg_speed;

	4+		//_u32	_task_type;
	
	4+		//_u32	_vod_play_time;
	4+		//_u32	_vod_first_buffer_time;
	4+		//_u32	_vod_interrupt_times;
	4+		//_u32	_min_interrupt_time;
	4+		//_u32	_max_interrupt_time;
	4+		//_u32	_avg_interrupt_time;

	8+		//_u64	_server_dl_bytes;
	8+		//_u64	_peer_dl_bytes;
	8+		//_u64	_emule_dl_bytes;
	8+		//_u64	_cdn_dl_bytes;
	
	4+		//_u32	_task_status;
	4+		//_u32	_failed_code;
	4+		//_u32	_dl_percent;

//add new
//////////////////vod_report

	4+      //_u32 _bit_rate;			        //比特率
	4+      //_u32 _vod_total_buffer_time;    //总的播放缓冲时间（秒）
	4+      //_u32 _vod_total_drag_wait_time;	//播放拖动后的等待播放时间（秒）
	4+      //_u32 _vod_drag_num;		        //拖动次数
	4+      //_u32 _play_interrupt_1;         // 1分钟内中断
	4+      //_u32 _play_interrupt_2;	        // 2分钟内中断
	4+      //_u32 _play_interrupt_3;		    //6分钟内中断
	4+      //_u32 _play_interrupt_4;		    //10分钟内中断
	4+      //_u32 _play_interrupt_5;		    //15分钟内中断
	4+      //_u32 _play_interrupt_6;		    //15分钟以上中断
	4+      //_u32 _cdn_stop_times;		    //关闭cdn次数
	
//////file_info_report
	8+		 //_down_exist; 				 //断点续传前数据量 																
	8+		 //_overlap_bytes;				 //重叠数据大小 	
	
	8+		 //_down_n2i_no;				 //内网从外网非边下边传下载字节数													
	8+		 //_down_n2i_yes;				 //内网从外网边下边传下载字节数 													
	8+		 //_down_n2n_no;				 //内网从内网非边下边传下载字节数	(包括同子网)												
	8+		 //_down_n2n_yes;				 //内网从内网边下边传下载字节数 	(包括同子网)												
	8+		 //_down_n2s_no;				 //同子网内网非边下边传下载字节数													
	8+		 //_down_n2s_yes;				 //同子网内网边下边传下载字节数 													
	8+		 //_down_i2i_no;				 //外网从外网非边下边传下载字节数													
	8+		 //_down_i2i_yes;				 //外网从外网边下边传下载字节数 													
	8+		 //_down_i2n_no;				 //外网从内网非边下边传下载字节数													
	8+		 //_down_i2n_yes;				 //外网从内网边下边传下载字节数 	
	
	8+		 //_down_by_partner_cdn;		      //partner Cdn下载字节		
	8+		 //_down_err_by_cdn;			 //Cdn纠错字节		
	8+		 //_down_err_by_partner_cdn;		//partner Cdn纠错字节		
	8+		 //_down_err_by_peer;			 //Peer纠错字节 																	
	8+		 //_down_err_by_svr;			 //Server纠错字节	

	//////cm_report
	4+		 //_res_server_total;			 //server资源总数																	
	4+		 //_res_server_valid;			 //有效server资源总数																
	4+		 //_res_cdn_total;				 //CDN资源总数																		
	4+		 //_res_cdn_valid;				 //有效CDN资源总数		
	4+		 //_res_partner_cdn_total;		//partner_CDN资源总数																		
	4+		 //_res_partner_cdn_valid;		//有效partner_CDN资源总数		
	4+		 //_cdn_res_max_num;			 //同时使用最大Cdn资源个数															
	4+		 //_cdn_connect_failed_times;	 //Cdn 使用失败次数 	

	
	4+		 //_res_n2i_total;				 //返回的所有内网到外网的资源数 														
	4+		 //_res_n2i_valid;				 //有效的内网到外网的资源数 	
	4+		 //_res_n2n_total;				 //返回的所有内网到内网的资源数 (包括同子网)													
	4+		 //_res_n2n_valid;				 //有效的内网到内网的资源数 (包括同子网)														
	4+		 //_res_n2s_total;				 //所有内网到同子网内网的资源数 													
	4+		 //_res_n2s_valid;				 //有效的内网到同子网内网的资源数													
	4+		 //_res_i2i_total;				 //返回的所有外网到外网的资源数 													
	4+		 //_res_i2i_valid;				 //有效的外网到外网的资源数 														
	4+		 //_res_i2n_total;				 //所有外网到内网的资源数															
	4+		 //_res_i2n_valid;				 //有效的外网到内网的资源数 	
	
	4+		 //_hub_res_total;					 //连上的Hub资源总数																
	4+		 //_hub_res_valid;					 //连上的Hub有效资源总数															
	4+		 //_active_tracker_res_total;	 //连上的Tracker主动资源总数														
	4+		 //_active_tracker_res_valid;	 //连上的Tracker有效主动资源总数													

//////res_query_report
	
	4+		 //_hub_s_max;					 //Shub查询的最大时间(s)															
	4+		 //_hub_s_min;					 //Shub查询的最小时间(s)															
	4+		 //_hub_s_avg;					 //Shub查询的平均时间(s)															
	4+		 //_hub_s_succ; 				 //Shub查询成功次数 																
	4+		 //_hub_s_fail; 				 //Shub查询失败次数 	
	
	4+		 //_hub_p_max;					 //Phub查询的最大时间(s)															
	4+		 //_hub_p_min;					 //Phub查询的最小时间(s)															
	4+		 //_hub_p_avg;					 //Phub查询的平均时间(s)															
	4+		 //_hub_p_succ; 				 //Phub查询成功次数 																
	4+		 //_hub_p_fail; 				 //Phub查询失败次数 	
	
	4+		 //_hub_t_max;					 //Tracker查询的最大时间(s) 														
	4+		 //_hub_t_min;					 //Tracker查询的最小时间(s) 														
	4+		 //_hub_t_avg;					 //Tracker查询的平均时间(s) 														
	4+		 //_hub_t_succ; 				 //Tracker查询成功次数																
	4+		 //_hub_t_fail; 				 //Tracker查询失败次数	
	4		 //_network_info;
	;

other_len=
	cmd->_thunder_version_len+
	cmd->_partner_id_len+
	cmd->_peerid_len+
	cmd->_fileid_len+
	
	cmd->_aich_hash_len+
	cmd->_part_hash_len+
	
	cmd->_cid_size+
	cmd->_gcid_size+
	
	cmd->_file_name_len
	;

	cmd->_header._cmd_len = fix_len+other_len;
	cmd->_cmd_type = EMB_REPORT_EMULE_STOP_TASK_CMD_TYPE;
	*len = sizeof(REP_CMD_HEADER) + cmd->_header._cmd_len;

	ret = sd_malloc(*len, (void**)buffer);  
	if(ret != SUCCESS)
	{
		LOG_DEBUG("emb_reporter_build_emule_stop_task_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);//压缩后重写长度

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)1); //条数
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)REPORTER_CMD_COMPRESS_TYPE_NONE); //压缩类型
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)(cmd->_header._cmd_len-12));//单条记录长度
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cmd_type);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_thunder_version_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_thunder_version, (_int32)cmd->_thunder_version_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_partner_id_len);  
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_partner_id, (_int32)cmd->_partner_id_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_len);

    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_fileid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_fileid, (_int32)cmd->_fileid_len);

    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_part_hash_len);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_part_hash_ptr, (_int32)cmd->_part_hash_len);

    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_aich_hash_len);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_aich_hash_ptr, (_int32)cmd->_aich_hash_len);

    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_cid, (_int32)cmd->_cid_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_gcid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_gcid, (_int32)cmd->_gcid_size);

	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_size);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_name_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_file_name, (_int32)cmd->_file_name_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_duration);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_avg_speed);
	

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_task_type);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_vod_play_time);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_vod_first_buffer_time);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_vod_interrupt_times);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_min_interrupt_time);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_max_interrupt_time);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_avg_interrupt_time);

	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_server_dl_bytes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_peer_dl_bytes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_emule_dl_bytes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_cdn_dl_bytes);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_task_status);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_failed_code);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_dl_percent);

///add new
//////////////////vod_report
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_bit_rate);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_vod_total_buffer_time);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_vod_total_drag_wait_time);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_vod_drag_num);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_play_interrupt_1);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_play_interrupt_2);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_play_interrupt_3);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_play_interrupt_4);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_play_interrupt_5);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_play_interrupt_6);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cdn_stop_times);

//////file_info_report
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_exist);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_overlap_bytes);
	
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_n2i_no);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_n2i_yes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_n2n_no);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_n2n_yes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_n2s_no);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_n2s_yes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_i2i_no);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_i2i_yes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_i2n_no);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_i2n_yes);
	
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_by_partner_cdn);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_err_by_cdn);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_err_by_partner_cdn);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_err_by_peer);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_err_by_svr);

//////cm_report
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_server_total);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_server_valid);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_cdn_total);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_cdn_valid);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_partner_cdn_total);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_partner_cdn_valid);	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cdn_res_max_num);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cdn_connect_failed_times);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_n2i_total);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_n2i_valid);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_n2n_total);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_n2n_valid);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_n2s_total);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_n2s_valid);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_i2i_total);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_i2i_valid);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_i2n_total);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_i2n_valid);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_res_total);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_res_valid);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_active_tracker_res_total);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_active_tracker_res_valid);
	
//////res_query_report
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_s_max);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_s_min);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_s_avg);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_s_succ);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_s_fail);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_p_max);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_p_min);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_p_avg);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_p_succ);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_p_fail);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_t_max);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_t_min);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_t_avg);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_t_succ);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_hub_t_fail);
	ret = sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_network_info);
	sd_assert( ret == SUCCESS );

	sd_assert( tmp_len == 0 );
	if(tmp_len != 0)
	{
		SAFE_DELETE(*buffer);
		return REPORTER_ERR_BUILD_EMB_REPORT;
	}

		
	ret = emb_reporter_package_stat_cmd(buffer, len);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("emb_reporter_build_emule_stop_task_cmd, http package failed.");
		CHECK_VALUE(ret);
	}

#ifdef REPORTER_DEBUG
		{
			char test_buffer[1024];
			sd_memset(test_buffer,0,1024);
			str2hex( (char*)(*buffer), *len, test_buffer, 1024);
			LOG_DEBUG( "emb_reporter_build_emule_stop_task_cmd:*buffer[%u]=%s" 
						,((_u32)(*len)), test_buffer);
		}
#endif /* REPORTER_DEBUG */
	return SUCCESS;
}

#endif


_int32 build_report_http_header(char* buffer, _u32* len, _u32 data_len, enum REPORTER_DEVICE_TYPE report_type)
{
	REPORTER_SETTING* setting = get_reporter_setting();
    char *addr = NULL;
    _u16 port = 0;
    
    switch(report_type)
    {
        case REPORTER_SHUB:
            {
                addr = setting->_shub_addr;
                port = setting->_shub_port;
                break;
            }

        case REPORTER_STAT_HUB:
            {
                addr = setting->_stat_hub_addr;
                port = setting->_stat_hub_port;
                break;
            }

                
#ifdef ENABLE_BT
        case REPORTER_BT_HUB:
            {
                addr = setting->_bt_hub_addr;
                port = setting->_bt_hub_port;
                break;
            }
                
#endif

#ifdef UPLOAD_ENABLE
        case REPORTER_PHUB:
            {
                addr = setting->_phub_addr;
                port = setting->_phub_port;
                break;
            }
#endif

#ifdef ENABLE_EMULE
        case REPORTER_EMULE_HUB:
            {
                addr = setting->_emule_hub_addr;
                port = setting->_emule_hub_port;
                break;
            }
#endif

#ifdef EMBED_REPORT
        case REPORTER_EMBED:
            {
                addr = setting->_emb_hub_addr;
                port = setting->_emb_hub_port;
                break;
            }
#endif
        

        default:
            {
                sd_assert(FALSE);
            }
    }
    LOG_DEBUG("build_report_http_header, cmd:%d,  addr= %s,port=%u", report_type, addr, port);

#if defined(MOBILE_PHONE)
	/*--------------- X-Online-Host ----------------------*/
	if(NT_GPRS_WAP & sd_get_net_type())
	{
		*len = sd_snprintf(buffer, *len, "POST http://%s:%u/ HTTP/1.1\r\nX-Online-Host: %s:%u\r\nContent-Length: %u\r\nContent-Type: application/octet-stream\r\nConnection: Close\r\n\r\n",
					addr, port,addr, port,  data_len);
	}
	else
#endif /* MOBILE_PHONE */
	{
		*len = sd_snprintf(buffer, *len, "POST http://%s:%u/ HTTP/1.1\r\nContent-Length: %u\r\nContent-Type: application/octet-stream\r\nConnection: Close\r\n\r\n",
				addr, port, data_len);
	}
	sd_assert(*len < 1024);

	return SUCCESS;
}

/** 将cmd打包成http包
  * 成功返回0，buffer中保存http包
  * 失败返回非0， 传入buffer会被释放
  */
_int32 emb_reporter_package_stat_cmd(char** buffer, _u32* len)
{
	_int32 ret;
	char*	tmp_buf;
	_int32  tmp_len;
	char http_header[1024] = {0};	
 	_u32 http_header_len = 1024;
	
#ifdef ENABLE_EMBED_REPORT_GZIP
	_u32 zip_type = REPORTER_CMD_COMPRESS_TYPE_NONE;
	_u8* gzed_buf;
	_int32 gzed_len = 0;
#endif

#ifdef REPORTER_DEBUG
		{
			char test_buffer[1024];
			sd_memset(test_buffer,0,1024);
			str2hex( (char*)(*buffer), *len, test_buffer, 1024);
			LOG_DEBUG( "emb_reporter_package_stat_cmd:*buffer[%u]=%s" 
						,((_u32)(*len)), test_buffer);
		}
#endif
	
	////////////////使用gz压缩数据  有开关
#ifdef ENABLE_EMBED_REPORT_GZIP
	tmp_buf = *buffer+CMD_MUTI_RECORD_HEADER_LEN;//指向协议内容的第3字节，从这里开始压缩
	tmp_len = *len-CMD_MUTI_RECORD_HEADER_LEN;//需要压缩内容的长度
	// sd_gz_encode_buffer需要长度至少为tmp_len+18的buffer作为输出
	// 如果http_header_len>tmp_len+18,则借用buffer_http_header作为压缩buffer,否则sd_malloc
	gzed_buf = (_u8 *)http_header;
	if((_int32)http_header_len < tmp_len+18)
	{
		ret = sd_malloc(tmp_len+18, (void**)&gzed_buf);
		if(ret != SUCCESS)
		{
			gzed_buf = NULL;//不使用压缩
			LOG_DEBUG("emb_reporter_package_stat_cmd, malloc gz buf failed.");
		}
	}
	ret = -1;
	if(gzed_buf != NULL)
	{
		ret = sd_gz_encode_buffer((_u8 *)tmp_buf, (_u32)tmp_len, gzed_buf, (_u32)tmp_len+18, (_u32*)&gzed_len);
	}
	//仅压缩成功，且有效果，才使用压缩
	if((SUCCESS == ret)&&(gzed_len < tmp_len))
	{
		LOG_DEBUG("succ gz encode buffer: len_s=%u, len_e=%u", tmp_len, gzed_len);
		zip_type = REPORTER_CMD_COMPRESS_TYPE_GZIP;
		sd_memcpy(tmp_buf, gzed_buf, gzed_len);
		*len = *len - tmp_len + gzed_len;//新的buffer长度  
		if(gzed_buf != (_u8 *)http_header)
		{
			//gzed_buf是动态分配的,需要释放
			SAFE_DELETE(gzed_buf);
		}
		
		// 重填充数据头:<协议长度>  (条数)(压缩方式)
		tmp_buf = *buffer+8;//指向<协议长度>
		tmp_len = 12;
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)gzed_len+CMD_DATA_HEADER_LEN);//len of(条数、压缩方式)=CMD_DATA_HEADER_LEN
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)1);
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)zip_type);
	}
	else 
	{
		// 不使用压缩
		if( gzed_buf !=NULL && gzed_buf != (_u8 *)http_header)
		{
			//gzed_buf是动态分配的,需要释放
			SAFE_DELETE(gzed_buf);
		}
		LOG_DEBUG("fail gz encode buffer: len=%u", tmp_len);
	}

#endif
	
	////////////////构造http_header
    ret = build_report_http_header(http_header, &http_header_len, *len, REPORTER_EMBED);
	if (ret != SUCCESS)
	{
		// 返回前需要free掉传入的buffer
		SAFE_DELETE(*buffer);
		CHECK_VALUE(ret);
	}
	
	////////////////合并cmd和http_header
		
	tmp_buf = *buffer;//指向需要http封包的命令字
	tmp_len = *len;//cmd的长度

	ret = sd_malloc(tmp_len + http_header_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("emb_reporter_package_stat_cmd, malloc http package buf failed.");
		// 返回前需要free掉传入的buffer
		SAFE_DELETE(tmp_buf);
		CHECK_VALUE(ret);
	}
	sd_memcpy(*buffer, http_header, http_header_len);
	sd_memcpy(*buffer+http_header_len, tmp_buf, tmp_len);
	SAFE_DELETE(tmp_buf);
	*len += http_header_len;
	
	return SUCCESS;	
}

#endif

#ifdef ENABLE_BT

_int32 reporter_build_report_bt_normal_cdn_stat_cmd(char** buffer, 
													_u32* len, 
													BT_REPORT_NORMAL_CDN_STAT *cmd)
{
	_int32 ret = 0;
	char *tmp_buf = NULL;
	_int32 tmp_len = 0, fix_len = 0, other_len = 0;
	char http_header[1024] = {0};
	_u32 http_header_len = 1024;
	_u32 encode_len = 0;

	cmd->_header._version = EMB_REPORT_PROTOCOL_VER;
	cmd->_header._seq = seq++;

	// begin from _cmd_type
	fix_len = 2 + 4 + 4 + 8 + 4 + 4 + 4 + 4 + 8 + 8 + 4 + 4 + 4 + 4 + 4 + 4 + 8 + 4 + 8 + 4;
	other_len = cmd->_info_id_size + cmd->_user_id_size + cmd->_peerid_size;

	cmd->_header._cmd_len = fix_len+other_len;
	cmd->_cmd_type = EMB_REPORT_BT_NORMAL_CDN_STAT_TYPE;
	*len = sizeof(REP_CMD_HEADER) + cmd->_header._cmd_len;

	// 
	encode_len = (cmd->_header._cmd_len + 16) / 16 * 16 + 12;

	ret = build_report_http_header(http_header, &http_header_len, encode_len, REPORTER_STAT_HUB);
	CHECK_VALUE(ret);

	ret = sd_malloc(*len + 16 + http_header_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("malloc failed.");
		CHECK_VALUE(ret);
	}
	sd_memcpy(*buffer, http_header, http_header_len);
	tmp_buf = *buffer + http_header_len;
	tmp_len = (_int32)*len;

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);

	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_cmd_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_stat_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_info_id_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char *)cmd->_info_id, (_int32)cmd->_info_id_size);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_user_id_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char *)cmd->_user_id, (_int32)cmd->_user_id_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char *)cmd->_peerid, (_int32)cmd->_peerid_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_other_res_added);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_normal_cdn_cnt);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_total_down_bytes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_normal_cdn_down_bytes);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_normal_cdn_trans_time);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_normal_cdn_conn_time);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_time_span_from_0_to_nonzero);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_time_sum_of_all_zero_period);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_time_of_download);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_query_normal_cdn_fail_times);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_down_bytes_from_p2sp);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_task_magic_id);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_magic_id);
	ret = sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_random_id);

#ifdef REPORTER_DEBUG
	{
		char test_buffer[1024];
		sd_memset(test_buffer,0,1024);
		str2hex((char*)(*buffer+http_header_len), *len+16, test_buffer, 1024);
		LOG_DEBUG("current *buffer[%u]=%s" , ((_u32)(*len))+16, test_buffer);
	}
#endif /* _DEBUG */

	ret = xl_aes_encrypt(*buffer + http_header_len, len);
	if(ret != SUCCESS)
	{
		SAFE_DELETE(*buffer);
		CHECK_VALUE(ret);
	}

	if(tmp_len != 0)
	{
		SAFE_DELETE(*buffer);
		return REPORTER_ERR_BUILD_DW_STAT;
	}

	sd_assert(encode_len == *len);
	*len += http_header_len;
	return SUCCESS;
}

#endif

#ifdef ENABLE_EMULE

_int32 reporter_build_report_emule_normal_cdn_stat_cmd(char** buffer, 
													   _u32* len, 
													   EMULE_REPORT_NORMAL_CDN_STAT *cmd)
{
	_int32 ret = 0;
	char *tmp_buf = NULL;
	_int32 tmp_len = 0, fix_len = 0, other_len = 0;
	char http_header[1024] = {0};
	_u32 http_header_len = 1024;
	_u32 encode_len = 0;

	cmd->_header._version = EMB_REPORT_PROTOCOL_VER;
	cmd->_header._seq = seq++;

	// begin from _cmd_type
	fix_len = 2 + 4 + 4 + 8 + 4 + 4 + 4 + 4 + 4 + 4 + 8 + 8 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 8 + 4; 
	other_len = cmd->_file_id_size + cmd->_cid_size + cmd->_gcid_size + cmd->_user_id_size + cmd->_peerid_size;

	cmd->_header._cmd_len = fix_len + other_len;
	cmd->_cmd_type = EMB_REPORT_EMULE_NORMAL_CDN_STAT_TYPE;
	*len = sizeof(REP_CMD_HEADER) + cmd->_header._cmd_len;

	// 
	encode_len = (cmd->_header._cmd_len + 16) / 16 * 16 + 12;

	ret = build_report_http_header(http_header, &http_header_len, encode_len, REPORTER_STAT_HUB);
	CHECK_VALUE(ret);

	ret = sd_malloc(*len + 16 + http_header_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("malloc failed.");
		CHECK_VALUE(ret);
	}
	sd_memcpy(*buffer, http_header, http_header_len);
	tmp_buf = *buffer + http_header_len;
	tmp_len = (_int32)*len;

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);

	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_cmd_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_stat_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_id_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char *)cmd->_file_id, (_int32)cmd->_file_id_size);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char *)cmd->_cid, (_int32)cmd->_cid_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_gcid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char *)cmd->_gcid, (_int32)cmd->_gcid_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_user_id_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char *)cmd->_user_id, (_int32)cmd->_user_id_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char *)cmd->_peerid, (_int32)cmd->_peerid_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_other_res_added);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_normal_cdn_cnt);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_total_down_bytes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_normal_cdn_down_bytes);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_normal_cdn_trans_time);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_normal_cdn_conn_time);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_time_span_from_0_to_nonzero);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_time_sum_of_all_zero_period);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_time_of_download);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_query_normal_cdn_fail_times);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_task_magic_id);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_magic_id);
	ret = sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_random_id);

#ifdef REPORTER_DEBUG
	{
		char test_buffer[1024];
		sd_memset(test_buffer,0,1024);
		str2hex((char*)(*buffer+http_header_len), *len+16, test_buffer, 1024);
		LOG_DEBUG("current *buffer[%u]=%s" , ((_u32)(*len))+16, test_buffer);
	}
#endif /* _DEBUG */

	ret = xl_aes_encrypt(*buffer + http_header_len, len);
	if(ret != SUCCESS)
	{
		SAFE_DELETE(*buffer);
		CHECK_VALUE(ret);
	}

	if(tmp_len != 0)
	{
		SAFE_DELETE(*buffer);
		return REPORTER_ERR_BUILD_DW_STAT;
	}

	sd_assert(encode_len == *len);
	*len += http_header_len;
	return SUCCESS;
}

#endif

_int32 reporter_build_report_p2sp_normal_cdn_stat_cmd(char** buffer, 
													  _u32* len, 
													  P2SP_REPORT_NORMAL_CDN_STAT *cmd)
{
	_int32 ret = 0;
	char *tmp_buf = NULL;
	_int32 tmp_len = 0, fix_len = 0, other_len = 0;
	char http_header[1024] = {0};
	_u32 http_header_len = 1024;
	_u32 encode_len = 0;

	cmd->_header._version = EMB_REPORT_PROTOCOL_VER;
	cmd->_header._seq = seq++;

	// begin from _cmd_type
	fix_len = 2 + 4 + 4 + 8 + 4 + 4 + 4 + 4 + 4 + 4 + 8 + 8 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 8 + 4; 
	other_len = cmd->_cid_size + cmd->_gcid_size + cmd->_user_id_size + cmd->_peerid_size + cmd->_url_size;

	cmd->_header._cmd_len = fix_len + other_len;
	cmd->_cmd_type = EMB_REPORT_P2SP_NORMAL_CDN_STAT_TYPE;
	*len = sizeof(REP_CMD_HEADER) + cmd->_header._cmd_len;

	// 
	encode_len = (cmd->_header._cmd_len + 16) / 16 * 16 + 12;

	ret = build_report_http_header(http_header, &http_header_len, encode_len, REPORTER_STAT_HUB);
	CHECK_VALUE(ret);

	ret = sd_malloc(*len + 16 + http_header_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("malloc failed.");
		CHECK_VALUE(ret);
	}
	sd_memcpy(*buffer, http_header, http_header_len);
	tmp_buf = *buffer + http_header_len;
	tmp_len = (_int32)*len;

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);

	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_cmd_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_stat_type);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char *)cmd->_cid, (_int32)cmd->_cid_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_gcid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char *)cmd->_gcid, (_int32)cmd->_gcid_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_user_id_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char *)cmd->_user_id, (_int32)cmd->_user_id_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char *)cmd->_peerid, (_int32)cmd->_peerid_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_other_res_added);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_normal_cdn_cnt);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_total_down_bytes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_normal_cdn_down_bytes);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_normal_cdn_trans_time);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_normal_cdn_conn_time);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_time_span_from_0_to_nonzero);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_time_sum_of_all_zero_period);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_time_of_download);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_query_normal_cdn_fail_times);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_task_magic_id);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_url_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char *)cmd->_url, (_int32)cmd->_url_size);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_magic_id);
	ret = sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_random_id);

#ifdef REPORTER_DEBUG
	{
		char test_buffer[1024];
		sd_memset(test_buffer,0,1024);
		str2hex((char*)(*buffer+http_header_len), *len+16, test_buffer, 1024);
		LOG_DEBUG("current *buffer[%u]=%s" , ((_u32)(*len))+16, test_buffer);
	}
#endif /* _DEBUG */

	ret = xl_aes_encrypt(*buffer + http_header_len, len);
	if(ret != SUCCESS)
	{
		SAFE_DELETE(*buffer);
		CHECK_VALUE(ret);
	}

	if(tmp_len != 0)
	{
		SAFE_DELETE(*buffer);
		return REPORTER_ERR_BUILD_DW_STAT;
	}

	sd_assert(encode_len == *len);
	*len += http_header_len;
	return SUCCESS;
}

typedef _int32 ( *REPORTER_ENCODE_CMD_FUNC)(_u32 len,char * buffer,_u32 * p_encLen,void * cmd);

_int32 reporter_build_encode_stat_cmd(char** buffer, _u32* len,_u32 seqno,_u16 _cmd_type,void * p_cmd,REPORTER_ENCODE_CMD_FUNC  pf_encode_cmd)
{
    _int32 ret;
	char http_header[1024] = {0};
	_u32 http_header_len = 1024;
	_u32 cmd_len,plain_len=0,encode_len = 0;

	char*	tmp_buf;
	_int32	tmp_len;

	pf_encode_cmd(0,NULL,&cmd_len,p_cmd);	/*获取cmd 编码后的长度*/
	plain_len= 20+ cmd_len;					/*头部长度20字节,  refer to tagREPORT_STAT_HEADER*/
	encode_len = ( plain_len-12 + 16) /16 * 16 + 12;  //加密算法,跳过前12字节不加密 
	ret = build_report_http_header(http_header, &http_header_len, encode_len, REPORTER_STAT_HUB);
	CHECK_VALUE(ret);
	
	ret = sd_malloc(encode_len + http_header_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("reporter_build_report_task_p2sp_stat_cmd, malloc cmd buffer failed.");
		CHECK_VALUE(ret);
	}
	sd_memcpy(*buffer, http_header, http_header_len);
	tmp_buf=*buffer+http_header_len;
	tmp_len=(_int32)plain_len;
	/*编码头部*/
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)REP_DW_PROTOCOL_VER);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)seqno);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd_len+8);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)0);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)0);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)_cmd_type);
	/*编码cmd*/
	pf_encode_cmd(tmp_len,tmp_buf,&cmd_len,p_cmd);
	sd_assert(tmp_len == cmd_len);
	/*加密*/
    *len=plain_len;
	ret = xl_aes_encrypt(*buffer + http_header_len, len);
	if(ret != SUCCESS)
	{
		SAFE_DELETE(*buffer);
		CHECK_VALUE(ret);
	}

	sd_assert(encode_len == *len);

#ifdef REPORTER_DEBUG
	{
		char test_buffer[1024+1],* p_data_tmp;
		_int32  remain_len, len1;
		p_data_tmp=*buffer+http_header_len;
		remain_len=*len;
		do
		{
			len1=remain_len>512?512:remain_len;
			str2hex(p_data_tmp, len1,test_buffer, 1024);
			test_buffer[len1+len1]=0;
			remain_len -= len1;
		}while(remain_len>0);
	}
#endif /* _DEBUG */
    /*加上http头部长度*/
	*len += http_header_len;
	
    return SUCCESS;
}
 

void reporter_dump_report_task_common_stat(struct tagREPORT_TASK_COMMON_STAT * p_common_stat)
{
	char peerid[PEER_ID_SIZE+1];
	sd_memcpy(peerid,p_common_stat->_peerid,p_common_stat->_peerid_len);
	peerid[p_common_stat->_peerid_len]=0;
	LOG_DEBUG("task_id=%d create_time=%d start_time=%d finish_time=%d task_status=%d fail_code=%d ",
		p_common_stat->_task_id,p_common_stat->_create_time,p_common_stat->_start_time,
		p_common_stat->_finish_time,p_common_stat->_task_status,p_common_stat->_fail_code);
	LOG_DEBUG("peerid_len=%d  peerid=%s",p_common_stat->_peerid_len,peerid);
	LOG_DEBUG("thunder_version_len=%d  thunder_version=%s",p_common_stat->_thunder_version_len,p_common_stat->_thunder_version);
	LOG_DEBUG("partner_id_len=%d  partner_id=%s",p_common_stat->_partner_id_len,p_common_stat->_partner_id);
	LOG_DEBUG("product_flag=%d avg_speed=%d first_zero_speed_duration=%d other_zero_speed_duraiton=%d percent100_duration=%d percent=%d",
		p_common_stat->_product_flag,p_common_stat->_avg_speed,p_common_stat->_first_zero_speed_duration,
		p_common_stat->_other_zero_speed_duraiton,p_common_stat->_percent100_duration,p_common_stat->_percent);
}

_int32 _reporter_build_report_task_comon_stat(_u32 len,char * buffer,_u32 * p_encLen,struct tagREPORT_TASK_COMMON_STAT * p_common_stat)
{
    char * tmp_buf = buffer;
	_int32 tmp_len ;
    *p_encLen=24+ 4+p_common_stat->_peerid_len + 4+p_common_stat->_thunder_version_len + 4+p_common_stat->_partner_id_len + 24;
	if(len<*p_encLen)
	{
		return -1;
	}
	tmp_len=*p_encLen;

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, p_common_stat->_task_id);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, p_common_stat->_create_time);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, p_common_stat->_start_time);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, p_common_stat->_finish_time);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, p_common_stat->_task_status);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, p_common_stat->_fail_code);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)p_common_stat->_peerid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, p_common_stat->_peerid, (_int32)p_common_stat->_peerid_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)p_common_stat->_thunder_version_len);
	sd_set_bytes(&tmp_buf, &tmp_len, p_common_stat->_thunder_version, (_int32)p_common_stat->_thunder_version_len);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)p_common_stat->_partner_id_len);  
	sd_set_bytes(&tmp_buf, &tmp_len, p_common_stat->_partner_id, (_int32)p_common_stat->_partner_id_len);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, p_common_stat->_product_flag);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, p_common_stat->_avg_speed);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, p_common_stat->_first_zero_speed_duration); 
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, p_common_stat->_other_zero_speed_duraiton);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, p_common_stat->_percent100_duration);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, p_common_stat->_percent);	

	sd_assert(0==tmp_len);
	return 0;
}

void reporter_dump_report_p2sp_stat_cmd(EMB_REPORT_TASK_P2SP_STAT *cmd)
{
    char cid_hex[CID_SIZE+CID_SIZE+1],gcid_hex[CID_SIZE+CID_SIZE+1];
	reporter_dump_report_task_common_stat(&cmd->_common);
	str2hex((char*)cmd->_cid, CID_SIZE,cid_hex, CID_SIZE+CID_SIZE);
	cid_hex[CID_SIZE+CID_SIZE]=0;
	
	str2hex((char*)cmd->_gcid, CID_SIZE,gcid_hex, CID_SIZE+CID_SIZE);
	gcid_hex[CID_SIZE+CID_SIZE]=0; 
	
	LOG_DEBUG("url=%s",cmd->_url);
	LOG_DEBUG("ref_url=%s",cmd->_ref_url);
	LOG_DEBUG("task_create_type=%d cid=%s gcid=%s",cmd->_task_create_type,cid_hex,gcid_hex);
	LOG_DEBUG("file_size=%llu file_name=%s",cmd->_file_size,cmd->_file_name);

	LOG_DEBUG("origin_bytes=%llu  other_server_bytes=%llu peer_dl_bytes=%llu speedup_cdn_bytes=%llu	lixian_cdn_bytes=%llu zero_cdn_dl_bytes=%llu",
		cmd->_origin_bytes,cmd->_other_server_bytes,cmd->_peer_dl_bytes,
		cmd->_speedup_cdn_bytes,cmd->_lixian_cdn_bytes,cmd->_zero_cdn_dl_bytes );

	LOG_DEBUG("zero_cdn_use_time=%d  zero_cdn_count=%d zero_cdn_valid_count=%d first_use_zero_cdn_duration=%d",
		cmd->_zero_cdn_use_time,cmd->_zero_cdn_count,cmd->_zero_cdn_valid_count,cmd->_first_use_zero_cdn_duration);
}

_int32 _reporter_encode_report_p2sp_stat_cmd(_u32 len,char * buffer,_u32 * p_encLen,EMB_REPORT_TASK_P2SP_STAT *cmd)
{
	char*	tmp_buf;
	_int32	tmp_len, com_len=0,fix_len=0,other_len=0;

	_reporter_build_report_task_comon_stat(0,NULL,(_u32*)&com_len,&cmd->_common);
	fix_len =96 ;/*4*10 + 8*7*/
	other_len = cmd->_url_len + cmd->_ref_url_len +  cmd->_cid_size + cmd->_gcid_size + cmd->_file_name_len;
	
	*p_encLen= com_len + fix_len + other_len;
	if(len<*p_encLen)
	{
		return -1;
	}

	tmp_buf = buffer ;
	tmp_len = (_int32)*p_encLen;

    _reporter_build_report_task_comon_stat(com_len,tmp_buf,(_u32*)&com_len,&cmd->_common);
	tmp_buf += com_len;
	tmp_len -= com_len;

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_url_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_url, (_int32)cmd->_url_len);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_ref_url_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_ref_url, (_int32)cmd->_ref_url_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_task_create_type);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_cid, (_int32)cmd->_cid_size);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_gcid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_gcid, (_int32)cmd->_gcid_size);
	
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_size);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_name_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_file_name, (_int32)cmd->_file_name_len);

	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_origin_bytes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_other_server_bytes);	
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_peer_dl_bytes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_speedup_cdn_bytes);	
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_lixian_cdn_bytes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_zero_cdn_dl_bytes);
		
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_zero_cdn_use_time);	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_zero_cdn_count);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_zero_cdn_valid_count);	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_first_use_zero_cdn_duration);

    sd_assert(0==tmp_len);
    return 0;
}

_int32 reporter_build_report_task_p2sp_stat_cmd(char** buffer, _u32* len,  EMB_REPORT_TASK_P2SP_STAT *cmd,_u32 * p_seq)
{
	_int32 ret;
	*p_seq=seq++;
	ret = reporter_build_encode_stat_cmd(buffer,len,*p_seq,EMB_REPORT_P2SP_DOWNLOAD_STAT_CMD_TYPE,
		cmd,(REPORTER_ENCODE_CMD_FUNC)_reporter_encode_report_p2sp_stat_cmd);

	reporter_dump_report_p2sp_stat_cmd(cmd);
	return ret;
}

void reporter_dump_report_emule_stat_cmd(EMB_REPORT_TASK_EMULE_STAT *cmd)
{
    char fileid[FILE_ID_SIZE+FILE_ID_SIZE+1],cid_hex[CID_SIZE+CID_SIZE+1],gcid_hex[CID_SIZE+CID_SIZE+1];
	reporter_dump_report_task_common_stat(&cmd->_common);

	str2hex((char*)cmd->_fileid, FILE_ID_SIZE,fileid, FILE_ID_SIZE+FILE_ID_SIZE);
	fileid[FILE_ID_SIZE+FILE_ID_SIZE]=0;
	
	str2hex((char*)cmd->_cid, CID_SIZE,cid_hex, CID_SIZE+CID_SIZE);
	cid_hex[CID_SIZE+CID_SIZE]=0;
	
	str2hex((char*)cmd->_gcid, CID_SIZE,gcid_hex, CID_SIZE+CID_SIZE);
	gcid_hex[CID_SIZE+CID_SIZE]=0; 
	
	LOG_DEBUG("_fileid=%s cid=%s gcid=%s",fileid,cid_hex,gcid_hex);
	LOG_DEBUG("file_size=%llu ",cmd->_file_size);

	LOG_DEBUG("emule_dl_bytes=%llu  server_dl_bytes=%llu peer_dl_bytes=%llu speedup_cdn_bytes=%llu lixian_cdn_bytes=%llu zero_cdn_dl_bytes=%llu",
		cmd->_emule_dl_bytes,cmd->_server_dl_bytes,cmd->_peer_dl_bytes,
		cmd->_speedup_cdn_bytes,cmd->_lixian_cdn_bytes,cmd->_zero_cdn_dl_bytes );

	LOG_DEBUG("zero_cdn_use_time=%d  zero_cdn_count=%d zero_cdn_valid_count=%d first_use_zero_cdn_duration=%d",
		cmd->_zero_cdn_use_time,cmd->_zero_cdn_count,cmd->_zero_cdn_valid_count,cmd->_first_use_zero_cdn_duration);

}

_int32 _reporter_encode_report_emule_stat_cmd(_u32 len,char * buffer,_u32 * p_encLen,EMB_REPORT_TASK_EMULE_STAT *cmd)
{
	char*	tmp_buf;
	_int32	tmp_len,com_len,fix_len=0,other_len=0;

	_reporter_build_report_task_comon_stat(0,NULL,(_u32*)&com_len,&cmd->_common);

	fix_len = 84;/*4*7+8*7*/
    other_len = cmd->_fileid_len + cmd->_cid_size + cmd->_gcid_size ;

	*p_encLen = com_len + fix_len + other_len;
	if(len<*p_encLen)
	{
		return -1;
	}

	tmp_buf = buffer ;
	tmp_len = (_int32)*p_encLen;
	
    _reporter_build_report_task_comon_stat(com_len,tmp_buf,(_u32*)&com_len,&cmd->_common);
	tmp_buf += com_len;
	tmp_len -=com_len;

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_fileid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_fileid, (_int32)cmd->_fileid_len);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_cid, (_int32)cmd->_cid_size);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_gcid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_gcid, (_int32)cmd->_gcid_size);
	
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_size);

	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_emule_dl_bytes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_server_dl_bytes);	
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_peer_dl_bytes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_speedup_cdn_bytes);	
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_lixian_cdn_bytes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_zero_cdn_dl_bytes);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_zero_cdn_use_time);	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_zero_cdn_count);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_zero_cdn_valid_count);	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_first_use_zero_cdn_duration);
	
	sd_assert(0==tmp_len);
    return SUCCESS;
}

_int32 reporter_build_report_task_emule_stat_cmd(char** buffer, _u32* len,  EMB_REPORT_TASK_EMULE_STAT *cmd,_u32 * p_seq)
{
	_int32 ret;
	*p_seq=seq++;	
	ret = reporter_build_encode_stat_cmd(buffer,len,*p_seq,EMB_REPORT_EMULE_DOWNLOAD_STAT_CMD_TYPE,
		cmd,(REPORTER_ENCODE_CMD_FUNC)_reporter_encode_report_emule_stat_cmd);
	
	reporter_dump_report_emule_stat_cmd(cmd);
	return ret;
}

void reporter_dump_report_bt_stat_cmd(EMB_REPORT_TASK_BT_STAT *cmd)
{
    char info_id[INFO_HASH_LEN+INFO_HASH_LEN+1];
	reporter_dump_report_task_common_stat(&cmd->_common);
		
	str2hex((char*)cmd->_info_id, INFO_HASH_LEN,info_id, INFO_HASH_LEN+INFO_HASH_LEN);
	info_id[INFO_HASH_LEN+INFO_HASH_LEN]=0;
	
	LOG_DEBUG("info_id=%s file_number=%d file_total_size=%llu piece_size=%d",
				info_id,cmd->_file_number,cmd->_file_total_size,cmd->_piece_size);

	LOG_DEBUG("server_dl_bytes=%llu  bt_dl_bytes=%llu peer_dl_bytes=%llu speedup_cdn_bytes=%llu	lixian_cdn_bytes=%llu zero_cdn_dl_bytes=%llu",
		cmd->_server_dl_bytes,cmd->_bt_dl_bytes,cmd->_peer_dl_bytes,
		cmd->_speedup_cdn_bytes,cmd->_lixian_cdn_bytes,cmd->_zero_cdn_dl_bytes );

	LOG_DEBUG("zero_cdn_use_time=%d  zero_cdn_count=%d zero_cdn_valid_count=%d first_use_zero_cdn_duration=%d",
		cmd->_zero_cdn_use_time,cmd->_zero_cdn_count,cmd->_zero_cdn_valid_count,cmd->_first_use_zero_cdn_duration);

}

_int32 _reporter_encode_report_bt_stat_cmd(_u32 len,char * buffer,_u32 * p_encLen,EMB_REPORT_TASK_BT_STAT *cmd)
{
	char*	tmp_buf;
	_int32	tmp_len,com_len,fix_len=0,other_len=0;

	_reporter_build_report_task_comon_stat(0,NULL,(_u32*)&com_len,&cmd->_common);

	fix_len = 84;/*4*7+8*7*/
    other_len = cmd->_info_id_size;
	
	*p_encLen = com_len + fix_len + other_len;
	if(len<*p_encLen)
	{
		return -1;
	}

	tmp_buf = buffer ;
	tmp_len = (_int32)*p_encLen;

    _reporter_build_report_task_comon_stat(com_len,tmp_buf,(_u32*)&com_len,&cmd->_common);
	tmp_buf+=com_len;
	tmp_len-=com_len;

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_info_id_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_info_id, (_int32)cmd->_info_id_size);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_number);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_total_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_piece_size);
	
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_server_dl_bytes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_bt_dl_bytes);	
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_speedup_cdn_bytes);	
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_lixian_cdn_bytes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_peer_dl_bytes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_zero_cdn_dl_bytes);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_zero_cdn_use_time);	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_zero_cdn_count);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_zero_cdn_valid_count);	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_first_use_zero_cdn_duration);
	
	sd_assert(0==tmp_len);
    return SUCCESS;
}

_int32 reporter_build_report_task_bt_stat_cmd(char** buffer, _u32* len,  EMB_REPORT_TASK_BT_STAT *cmd,_u32 * p_seq)
{
	_int32 ret;
	*p_seq=seq++;		
	ret = reporter_build_encode_stat_cmd(buffer,len,*p_seq,EMB_REPORT_BT_DOWNLOAD_STAT_CMD_TYPE,
		cmd,(REPORTER_ENCODE_CMD_FUNC)_reporter_encode_report_bt_stat_cmd);

	reporter_dump_report_bt_stat_cmd(cmd);
	return ret;
}

void reporter_dump_report_bt_file_stat_cmd(EMB_REPORT_TASK_BT_FILE_STAT *cmd)
{
    char info_id[INFO_HASH_LEN+INFO_HASH_LEN+1],cid_hex[CID_SIZE+CID_SIZE+1],gcid_hex[CID_SIZE+CID_SIZE+1];
	reporter_dump_report_task_common_stat(&cmd->_common);

	str2hex((char*)cmd->_info_id, INFO_HASH_LEN,info_id, INFO_HASH_LEN+INFO_HASH_LEN);
	info_id[INFO_HASH_LEN+INFO_HASH_LEN]=0;

	
	str2hex((char*)cmd->_cid, CID_SIZE,cid_hex, CID_SIZE+CID_SIZE);
	cid_hex[CID_SIZE+CID_SIZE]=0;
	
	str2hex((char*)cmd->_gcid, CID_SIZE,gcid_hex, CID_SIZE+CID_SIZE);
	gcid_hex[CID_SIZE+CID_SIZE]=0; 

	LOG_DEBUG("info_id=%s file_index=%d cid=%s gcid=%s file_size=%llu",
				info_id,cmd->_file_index,cid_hex,gcid_hex,cmd->_file_size);

	LOG_DEBUG("server_dl_bytes=%llu  bt_dl_bytes=%llu peer_dl_bytes=%llu speedup_cdn_bytes=%llu	lixian_cdn_bytes=%llu zero_cdn_dl_bytes=%llu",
		cmd->_server_dl_bytes,cmd->_bt_dl_bytes,cmd->_peer_dl_bytes,
		cmd->_speedup_cdn_bytes,cmd->_lixian_cdn_bytes,cmd->_zero_cdn_dl_bytes );

	LOG_DEBUG("zero_cdn_use_time=%d  zero_cdn_count=%d zero_cdn_valid_count=%d first_use_zero_cdn_duration=%d",
		cmd->_zero_cdn_use_time,cmd->_zero_cdn_count,cmd->_zero_cdn_valid_count,cmd->_first_use_zero_cdn_duration);

}

_int32 _reporter_encode_report_bt_file_stat_cmd(_u32 len,char * buffer,_u32 * p_encLen,EMB_REPORT_TASK_BT_FILE_STAT *cmd)
{
	char*	tmp_buf;
	_int32	tmp_len,com_len,fix_len=0,other_len=0;

	_reporter_build_report_task_comon_stat(0,NULL,(_u32*)&com_len,&cmd->_common);

	fix_len = 88;/*4*8+8*7*/
    other_len = cmd->_info_id_size + cmd->_cid_size + cmd->_gcid_size;
	
	*p_encLen = com_len + fix_len + other_len;
	if(len<*p_encLen)
	{
		return -1;
	}

	tmp_buf = buffer ;
	tmp_len = (_int32)*p_encLen;

    _reporter_build_report_task_comon_stat(com_len,tmp_buf,(_u32*)&com_len,&cmd->_common);
	tmp_buf+=com_len;
	tmp_len-=com_len;

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_info_id_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_info_id, (_int32)cmd->_info_id_size);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_index);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_cid, (_int32)cmd->_cid_size);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_gcid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_gcid, (_int32)cmd->_gcid_size);

	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_size);
	
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_server_dl_bytes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_bt_dl_bytes);	
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_speedup_cdn_bytes);	
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_lixian_cdn_bytes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_peer_dl_bytes);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_zero_cdn_dl_bytes);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_zero_cdn_use_time);	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_zero_cdn_count);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_zero_cdn_valid_count);	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_first_use_zero_cdn_duration);
	
	sd_assert(0==tmp_len);
    return SUCCESS;
}

_int32 reporter_build_report_task_bt_file_stat_cmd(char** buffer, _u32* len,  EMB_REPORT_TASK_BT_FILE_STAT *cmd,_u32 * p_seq)
{
	_int32 ret;
	*p_seq=seq++;	
	ret = reporter_build_encode_stat_cmd(buffer,len,*p_seq,EMB_REPORT_BT_SUBTASK_DOWNLOAD_STAT_CMD_TYPE,
		cmd,(REPORTER_ENCODE_CMD_FUNC)_reporter_encode_report_bt_file_stat_cmd);

	reporter_dump_report_bt_file_stat_cmd(cmd);	
	return ret;
}

void reporter_dump_report_create_stat_cmd(EMB_REPORT_TASK_CREATE_STAT *cmd,_u32 flag)
{
	char peerid[PEER_ID_SIZE+1],create_info[MAX_URL_LEN+MAX_URL_LEN+1];
	sd_memcpy(peerid,cmd->_peerid,cmd->_peerid_len);
	peerid[cmd->_peerid_len]=0;
	LOG_DEBUG("task_id=%d create_time=%d file_index=%d task_status=%d",
		cmd->_task_id,cmd->_create_time,cmd->_file_index,cmd->_task_status);
	LOG_DEBUG("peerid_len=%d  peerid=%s",cmd->_peerid_len,peerid);
	LOG_DEBUG("thunder_version_len=%d  thunder_version=%s",cmd->_thunder_version_len,cmd->_thunder_version);
	LOG_DEBUG("partner_id_len=%d  partner_id=%s",cmd->_partner_id_len,cmd->_partner_id);
	LOG_DEBUG("product_flag=%d task_type=%d ",cmd->_product_flag,cmd->_task_type);
	if(flag&0x01)
	{
		LOG_DEBUG("create_info=%s ",cmd->_create_info);		
	}
	else
	{
		str2hex((char*)cmd->_create_info, cmd->_create_info_len,create_info, MAX_URL_LEN+MAX_URL_LEN);
		create_info[cmd->_create_info_len+cmd->_create_info_len]=0;
		LOG_DEBUG("create_info=%s ",create_info);		
	}
}

_int32 _reporter_encode_report_create_stat_cmd(_u32 len,char * buffer,_u32 * p_encLen,EMB_REPORT_TASK_CREATE_STAT *cmd)
{
	char*	tmp_buf;
	_int32	tmp_len, fix_len=0,other_len=0;

	fix_len = 40;/*4*10*/
    other_len = cmd->_peerid_len + cmd->_thunder_version_len + cmd->_partner_id_len + cmd->_create_info_len;
	
	*p_encLen = fix_len + other_len;
	if(len<*p_encLen)
	{
		return -1;
	}

	tmp_buf = buffer ;
	tmp_len = (_int32)*p_encLen;

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_task_id);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_create_time);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_index);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_task_status);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_thunder_version_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_thunder_version, (_int32)cmd->_thunder_version_len);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_partner_id_len);  
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_partner_id, (_int32)cmd->_partner_id_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_product_flag);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_task_type);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_create_info_len);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_create_info, (_int32)cmd->_create_info_len);

	sd_assert(0==tmp_len);
    return SUCCESS;
}

_int32 reporter_build_report_task_create_stat_cmd(char** buffer, _u32* len,EMB_REPORT_TASK_CREATE_STAT *cmd,_u32 * p_seq,_u32 flag)
{
	_int32 ret;
	*p_seq=seq++;	
	ret = reporter_build_encode_stat_cmd(buffer,len,*p_seq,EMB_REPORT_TASK_CREATE_STAT_CMD_TYPE,
		cmd,(REPORTER_ENCODE_CMD_FUNC)_reporter_encode_report_create_stat_cmd);
	reporter_dump_report_create_stat_cmd(cmd,flag);
	return ret;
}

#ifdef UPLOAD_ENABLE
void reporter_dump_report_upload_stat_cmd(EMB_REPORT_UPLOAD_STAT *cmd)
{
	char peerid[PEER_ID_SIZE+1];
	sd_memcpy(peerid,cmd->_peerid,cmd->_peerid_len);
	peerid[cmd->_peerid_len]=0;
	LOG_DEBUG("peerid_len=%d  peerid=%s",cmd->_peerid_len,peerid);
	LOG_DEBUG("thunder_version_len=%d  thunder_version=%s",cmd->_thunder_version_len,cmd->_thunder_version);
	LOG_DEBUG("partner_id_len=%d  partner_id=%s",cmd->_partner_id_len,cmd->_partner_id);
	LOG_DEBUG("product_flag=%d peer_capacity=%d ",cmd->_product_flag,cmd->_peer_capacity);
	LOG_DEBUG("begin_time=%d end_time=%d up_duration=%d up_use_duration=%d",cmd->_begin_time,cmd->_end_time,cmd->_up_duration,cmd->_up_use_duration);
	LOG_DEBUG("up_data_bytes=%llu up_pipe_num=%d up_passive_pipe_num=%d",cmd->_up_data_bytes,cmd->_up_pipe_num,cmd->_up_passive_pipe_num);
}

_int32 _reporter_encode_report_upload_stat_cmd(_u32 len,char * buffer,_u32 * p_encLen,EMB_REPORT_UPLOAD_STAT *cmd)
{
	char*	tmp_buf;
	_int32	tmp_len, fix_len=0,other_len=0;

	fix_len = 52;/*4*11+8*1*/
    other_len = cmd->_peerid_len + cmd->_thunder_version_len + cmd->_partner_id_len;
	
	*p_encLen = fix_len + other_len;
	if(len<*p_encLen)
	{
		return -1;
	}

	tmp_buf = buffer ;
	tmp_len = (_int32)*p_encLen;

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_thunder_version_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_thunder_version, (_int32)cmd->_thunder_version_len);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_partner_id_len);  
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_partner_id, (_int32)cmd->_partner_id_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_product_flag);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_begin_time);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_end_time);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peer_capacity);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_up_duration);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_up_use_duration);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_up_data_bytes);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_up_pipe_num);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_up_passive_pipe_num);

	sd_assert(0==tmp_len);
    return SUCCESS;
}

_int32 reporter_build_report_upload_stat_cmd(char** buffer, _u32* len,EMB_REPORT_UPLOAD_STAT *cmd,_u32 * p_seq)
{
	_int32 ret;
	*p_seq=seq++;	

	ret = reporter_build_encode_stat_cmd(buffer,len,*p_seq,EMB_REPORT_UPLOAD_STAT_CMD_TYPE,
		cmd,(REPORTER_ENCODE_CMD_FUNC)_reporter_encode_report_upload_stat_cmd);
	reporter_dump_report_upload_stat_cmd(cmd);
	return ret;
}
#endif

