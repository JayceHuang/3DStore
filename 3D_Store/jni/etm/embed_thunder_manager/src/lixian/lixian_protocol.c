/*--------------------------------------------------------------------------*/
/*                               IDENTIFICATION                             */
/*--------------------------------------------------------------------------*/
/*     Filename  : lixian_protocol.c                                         */
/*     Author     : ZengYuqing                                              */
/*     Project    : ipad kankan                                        */
/*     Module     : lixian													*/
/*     Version    : 1.0  													*/
/*--------------------------------------------------------------------------*/
/*                  Shenzhen XunLei Networks			                    */
/*--------------------------------------------------------------------------*/
/*                  - C (copyright) - www.xunlei.com -		     		    */
/*                                                                          */
/*      This document is the proprietary of XunLei                          */
/*                                                                          */
/*      All rights reserved. Integral or partial reproduction               */
/*      prohibited without a written authorization from the                 */
/*      permanent of the author rights.                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
/*                         FUNCTIONAL DESCRIPTION                           */
/*--------------------------------------------------------------------------*/
/* This file contains the procedure of lixian                       */
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
/*                              HISTORY                                     */
/*--------------------------------------------------------------------------*/
/*   Date     |    Author   | Modification                                  */
/*--------------------------------------------------------------------------*/
/* 2011.11.02 | ZengYuqing  | Creation                                      */
/*--------------------------------------------------------------------------*/

#include "utility/define.h"

#ifdef ENABLE_LIXIAN_ETM
#include "lixian/lixian_interface.h"
#include "lixian/lixian_impl.h"
#include "lixian/lixian_protocol.h"

#include "utility/mempool.h"
#include "utility/bytebuffer.h"
#include "utility/string.h"
#include "utility/md5.h"
#include "utility/aes.h"
#include "utility/utility.h"
#include "utility/sd_iconv.h"

#include "platform/sd_gz.h"

#include "torrent_parser/torrent_parser_interface.h"
#include "et_interface/et_interface.h"

#include "em_common/em_utility.h"
#include "em_common/em_url.h"
#include "em_interface/em_fs.h"
#include "em_common/em_time.h"
#include "em_common/em_logid.h"

#include "xml_service.h"

#ifdef EM_LOGID
#undef EM_LOGID
#endif

#define EM_LOGID LOGID_LIXIAN

#define UINT32_LEN sizeof(_u32)

#include "em_common/em_logger.h"

/*--------------------------------------------------------------------------*/
/*                Global declarations				        */
/*--------------------------------------------------------------------------*/
static _int32 node_count=0;//���ڼ������Ѿ������˵Ľڵ����
static LX_TASK_INFO_EX * gp_task_info = NULL;
static LX_FILE_INFO * gp_file_info = NULL;
static MAP * gp_bt_sub_file_map = NULL;
/*--------------------------------------------------------------------------*/
/*                Interfaces provid for other module				        */
/*--------------------------------------------------------------------------*/

_int32 lx_build_req_task_ls(LX_BASE * p_base,LX_PT_TASK_LS * p_req)
{
	_int32 ret_val = SUCCESS;
    
	EM_LOG_DEBUG("lx_build_req_task_ls: offset = %d, num= %d", p_req->_req._offset, p_req->_req._max_task_num);
    
	/*fill request package*/
	p_req->_action._req_data_len =sd_snprintf(p_req->_action._req_buffer, p_req->_action._req_buffer_len, 
		"%s<LixianProtocol Version=\"1.0\">\r\n\
		<Command id=\"lxtasklist_req\">\r\n\
		<user id=\"%llu\" name=\"%s\" newno=\"%s\" vip_level=\"%d\" net=\"%d\" ip=\"%u\" key=\"\" from=\"%d\"/>\r\n\
		<info offset=\"%d\" num=\"%d\" type=\"%d\" fileattribute =\"%d\"/>\r\n\
		</Command>\r\n\
		</LixianProtocol>\r\n", 
		XML_HEADER, p_base->_userid, p_base->_user_name_old, p_base->_user_name, p_base->_vip_level, p_base->_net, p_base->_local_ip, p_base->_from, p_req->_req._offset, p_req->_req._max_task_num, p_req->_req._file_status, p_req->_req._file_type);

	#if 0 //def _DEBUG
		URGENT_TO_FILE(" tasklist request xml [%u]: \n%s\n", p_req->_action._req_data_len,p_req->_action._req_buffer);
	#endif/*_DEBUG*/

	ret_val = lx_build_req_zip_and_aes((LX_PT *) p_req);
	CHECK_VALUE(ret_val);

	return SUCCESS;
}
_int32 lx_parse_resp_task_list(LX_PT_TASK_LS* p_resp)
{
	EM_LOG_DEBUG("lx_parse_resp_task_list" );
	return lx_parse_xml_file((LX_PT *)p_resp, lx_task_list_xml_node_proc, lx_task_list_xml_node_end_proc, lx_task_list_xml_attr_proc,lx_task_list_xml_value_proc);
}

_int32 lx_build_req_bt_task_ls(LX_BASE * p_base, LX_PT_BT_LS * p_req)
{
	_int32 ret_val = SUCCESS;

	EM_LOG_DEBUG("lx_build_req_bt_task_ls" );

	/*fill request package*/
	p_req->_action._req_data_len =sd_snprintf(p_req->_action._req_buffer, p_req->_action._req_buffer_len, 
		"%s<LixianProtocol Version=\"1.0\">\r\n\
		<Command id=\"getbtlist_req\">\r\n\
		<user id=\"%llu\" name=\"%s\" newno=\"%s\" vip_level=\"%d\" net=\"%d\" ip=\"%u\" key=\"\" from=\"%d\"/>\r\n\
		<bttask>\r\n\
		<maintask id=\"%llu\" filter=\"%d\" type=\"%d\" offset=\"%d\" num=\"%d\" fileattribute =\"%d\"/>\r\n\
		</bttask>\r\n\
		</Command>\r\n\
		</LixianProtocol>\r\n", 
		XML_HEADER, p_base->_userid, p_base->_user_name_old, p_base->_user_name, p_base->_vip_level, p_base->_net, p_base->_local_ip, p_base->_from, p_req->_req._task_id, 0, p_req->_req._file_status, p_req->_req._offset, p_req->_req._max_file_num, p_req->_req._file_type);

	#ifdef _DEBUG
		EM_LOG_DEBUG("\n bt tasklist request xml [%u]: \n%s\n", p_req->_action._req_data_len,p_req->_action._req_buffer);
	#endif/*_DEBUG*/

	ret_val = lx_build_req_zip_and_aes((LX_PT *) p_req);
	CHECK_VALUE(ret_val);

	return SUCCESS;
}
_int32 lx_parse_resp_bt_task_list(LX_PT_BT_LS* p_resp)
{
	return lx_parse_xml_file((LX_PT *)p_resp, lx_bt_task_list_xml_node_proc, lx_bt_task_list_xml_node_end_proc, lx_bt_task_list_xml_attr_proc, lx_bt_task_list_xml_value_proc);
}

_int32 lx_build_req_screenshot(LX_BASE * p_base,LX_PT_SS * p_req)
{
	_int32 ret_val = SUCCESS;
	_u32 index = 0,gcid_num = 0;
	char gcid_str[64]={0},*gcid_list = NULL;
	_u8 * p_gcid = NULL;

	EM_LOG_DEBUG("lx_build_req_screenshot" );
	
	gcid_num = p_req->_need_dl_num;
	sd_assert(gcid_num!=0);
    
	if(p_req->_action._req_buffer_len<gcid_num*LX_PT_GCID_NODE_LEN)
	{
		return -1;
	}
	
	ret_val = sd_malloc(gcid_num*LX_PT_GCID_NODE_LEN, (void**)&gcid_list);
	CHECK_VALUE(ret_val);
	sd_memset(gcid_list,0x00,gcid_num*LX_PT_GCID_NODE_LEN);
    
	for(index = 0;index <gcid_num;index++ )
	{
		p_gcid = lx_get_next_need_dl_gcid(p_req,p_gcid);
		str2hex((char*)p_gcid, CID_SIZE, gcid_str, 64);
		gcid_str[40] = '\0';
		sd_snprintf(gcid_list+sd_strlen(gcid_list), LX_PT_GCID_NODE_LEN, "<Gcid  g=\"%s\" />\r\n", gcid_str);
	}
    
	/*fill request package*/
	p_req->_action._req_data_len =sd_snprintf(p_req->_action._req_buffer, p_req->_action._req_buffer_len, 
		"%s<LixianProtocol Version=\"1.0\">\r\n\
		<Command id=\"screenshot_req\">\r\n\
		<user id=\"%llu\" name=\"%s\" newno=\"%s\" vip_level=\"%d\" net=\"%d\" ip=\"%u\" key=\"\" from=\"%d\"/>\r\n\
		<Content section_type=\"%d\"/>\r\n\
		<Gcidlist>\r\n\%s</Gcidlist>\r\n\
		</Command>\r\n\
		</LixianProtocol>\r\n", 
		XML_HEADER, p_base->_userid, p_base->_user_name_old, p_base->_user_name, p_base->_vip_level, p_base->_net, p_base->_local_ip, p_base->_from, p_base->_section_type,gcid_list);

	#ifdef _DEBUG
		printf("\n screenshot request xml [%u]: \n%s\n", p_req->_action._req_data_len,p_req->_action._req_buffer);
	#endif/*_DEBUG*/

	SAFE_DELETE(gcid_list);

	ret_val = lx_build_req_zip_and_aes((LX_PT *) p_req);
	CHECK_VALUE(ret_val);

	return SUCCESS;
}
_int32 lx_parse_resp_screenshot(LX_PT_SS* p_resp)
{
	return lx_parse_xml_file((LX_PT *)p_resp, lx_screensoht_xml_node_proc, lx_screensoht_xml_node_end_proc, lx_screensoht_xml_attr_proc,lx_screensoht_xml_value_proc);
}

_int32 lx_build_req_vod_url(LX_BASE * p_base, LX_PT_VOD_URL * p_req)
{
	_int32 ret_val = SUCCESS;
	char gcid_str[44] = {0};
	char cid_str[44] = {0}; 
    
	EM_LOG_DEBUG("lx_build_req_vod_url" );
	
	str2hex((char*)p_req->_req._gcid, CID_SIZE, gcid_str, 44);
	gcid_str[40] = '\0';
    
	str2hex((char*)p_req->_req._cid, CID_SIZE, cid_str, 44);
	cid_str[40] = '\0';

	/*fill request package*/
	p_req->_action._req_data_len =sd_snprintf(p_req->_action._req_buffer, p_req->_action._req_buffer_len, 
		"%s<LixianProtocol Version=\"1.0\">\r\n\
		<Command id=\"getplayurl_req\">\r\n\
		<user id=\"%llu\" name=\"%s\" newno=\"%s\" vip_level=\"%d\" net=\"%d\" ip=\"%u\" key=\"\" from=\"%d\"/>\r\n\
		<content gcid=\"%s\" cid=\"%s\" dev_width=\"%u\" dev_hight=\"%u\" section_type=\"%d\" filesize=\"%llu\" max_fpfile_num=\"%u\" video_type=\"%u\" />\r\n\
		</Command>\r\n\
		</LixianProtocol>\r\n", 
		XML_HEADER, p_base->_userid, p_base->_user_name_old, p_base->_user_name, p_base->_vip_level, p_base->_net, p_base->_local_ip,p_base->_from, gcid_str, cid_str, p_req->_req._device_width, p_req->_req._device_height, p_base->_section_type, p_req->_req._size, p_req->_req._max_fpfile_num, p_req->_req._video_type);

	#ifdef _DEBUG
		EM_LOG_DEBUG("\n vod url request xml [%u]: \n%s\n", p_req->_action._req_data_len,p_req->_action._req_buffer);
	#endif/*_DEBUG*/

	ret_val = lx_build_req_zip_and_aes((LX_PT *) p_req);
	CHECK_VALUE(ret_val);

	return SUCCESS;
}
_int32 lx_parse_resp_vod_url(LX_PT_VOD_URL* p_resp)
{
	lx_parse_xml_file((LX_PT *)p_resp, lx_vod_url_xml_node_proc, lx_vod_url_xml_node_end_proc, lx_vod_url_xml_attr_proc,lx_vod_url_xml_value_proc);
	return SUCCESS;
}

_int32 lx_build_req_get_task_list_new(LX_PT_TASK_LS_NEW * p_req)
{
	_int32 ret_val = SUCCESS;
	char * tmp_buf = p_req->_action._req_buffer;
	_int32 tmp_len = (_int32)p_req->_action._req_buffer_len;
	
	EM_LOG_DEBUG("lx_build_req_get_task_list_new" );
    
	/* �������ͷ */
	ret_val = lx_pt_set_header(&tmp_buf, &tmp_len, &p_req->_req._cmd_header);
    
	/* ����û���Ϣ */
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)p_req->_req._jump_key_len);
	sd_set_bytes(&tmp_buf, &tmp_len, p_req->_req._jump_key, (_int32)p_req->_req._jump_key_len);
	
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)p_req->_req._userid);
	
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)p_req->_req._vip_level);
    
	/* ����Э�������ʣ���ֶ�
	[info_struct] 
	offset=uint32   ����ƫ���������ĸ�����ʼ
	num=uint32      �����������
	type=uint32     �������������0-all, 1-down, 2-complete, 4-all+�ѹ���
	fileattribute=uint32  ����һ���ļ����� 0-all, 1-��Ƶ��(����bt�к�����Ƶ������)
	sort=uint32     0- �ύʱ�併�� ������
	*/	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)5*sizeof(_int32));
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len,  (_int32)p_req->_req._task_list._offset);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len,  (_int32)p_req->_req._task_list._max_task_num);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len,  (_int32)p_req->_req._task_list._file_status);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len,  (_int32)p_req->_req._task_list._file_type);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len,  (_int32)0);
    
	/********************************************************************************************/
	/* Э��������ϣ�����action�е����󳤶Ⱥ�ͷ������� */
	p_req->_action._req_data_len = p_req->_action._req_buffer_len - tmp_len;
    p_req->_req._cmd_header._len = p_req->_action._req_data_len - sizeof(p_req->_req._cmd_header);
    
	/*aes ����*/
	tmp_len = p_req->_action._req_data_len;
	ret_val = lx_aes_encrypt(p_req->_action._req_buffer, (_u32*)&tmp_len);
	if (ret_val!= SUCCESS)
	{
		sd_assert(FALSE);
		return ret_val;
	}
    
	/* ���ܺ�Ҫ��������data len !!!!*/
	p_req->_action._req_data_len = tmp_len;
	
	return SUCCESS;	
}

_int32  lx_parse_get_task_list_new_user_info(char *tmp_buf, _int32 tmp_len, LX_TASK_LIST *task_list)
{
	_int32 ret_val = SUCCESS;
	_int32 msg_len = 0;
	char* cmd_msg = NULL;
    _int32 date_len = 0;
    _int32 cookie_len = 0;
	char * zlib_buf = NULL;
	_int32 tmp_data = 0;
	_int64 tmp_data64 = 0;
	_int32 i = 0;
	
	/* ��ȡ����������Ϣ */
	// max_task_num=uint64
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&task_list->_total_task_num);
	// max_store=uint64
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&task_list->_total_space); 
    // vip_store=uint64
    sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&tmp_data64); 
    // buy_store=uint64
    sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&tmp_data64); 
	// xz_store=uint64
    sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&tmp_data64); 
    // buy_num_task=uint32
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&tmp_data); 
	// buy_num_connection=uint32
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&tmp_data); 
	// buy_bandwidth=uint32
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&tmp_data); 
	// buy_task_live_time=uint32
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&tmp_data); 
	// experience_expire_date=string2, expire_date =string
	for(i = 0; i < 2; i++)
	{
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, &msg_len);
		if (msg_len > 0)
		{
			cmd_msg = tmp_buf;
			tmp_buf += msg_len;
			tmp_len -= msg_len;
		}
	}
	// available_space=uint64
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&task_list->_available_space);
	// total_num=uint32
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&task_list->_total_task_num); 
	//history_task_total_num=uint32
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&tmp_data); 
	//suspending_num=uint32
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&tmp_data); 
	//downloading_num=uint32
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&tmp_data); 
	//waiting_num=uint32
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&tmp_data); 
	//complete_num=uint32 
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&tmp_data); 
	//store_period=uint32
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&tmp_data); 
	//cookie=string
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &msg_len);
	if (msg_len > 0)
	{
		cmd_msg = tmp_buf;
		tmp_buf += msg_len;
		tmp_len -= msg_len;
	}
	//vip_level=uint8
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&tmp_data); 
	//user_type=uint32
	// goldbean_num=uint64
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&tmp_data64);
	//silverbean_num=uint64
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&tmp_data64);
	//convert_flag=uint8
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&tmp_data); 
	//special_net=uint32
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&tmp_data); 
	//total_filter_num=uint32
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&tmp_data); 

	return SUCCESS;
}


_int32  lx_parse_get_task_list_new_task_info(char *tmp_buf, _int32 tmp_len)
{
	_int32 ret_val = SUCCESS;
	_int32 tmp_data = 0;
	_int64 tmp_data64 = 0;
	_int32 msg_len = 0;
	char* cmd_msg = NULL;
	char cidbuf[41] = {0};
	char tmp_long_url[2*MAX_URL_LEN] = {0};
	char tmp_tmp_url[2*MAX_URL_LEN] = {0};
	char tmp_url[MAX_URL_LEN] = {0};
	_int32 i = 0;
	char utf8[MAX_FILE_NAME_BUFFER_LEN] = {0};
	_u32 utf8_len = MAX_FILE_NAME_BUFFER_LEN;
	char utf8url[2*MAX_URL_LEN] = {0};
	_u32 utf8urllen = 2*MAX_URL_LEN;
	/* ���ڹ����ļ����ضϵĴ���*/
	char dec_str[MAX_FILE_NAME_BUFFER_LEN] = {0};
	char task_name[MAX_FILE_NAME_BUFFER_LEN] = {0};
	char* p_suffix = NULL;
	_u32 suffix_len = 0, name_len = 0, attr_len = 0; 
	
 	LX_TASK_INFO_EX *task_info = NULL;
	ret_val = lx_malloc_ex_task(&task_info);
	if(ret_val != SUCCESS)
		return ret_val;

	// ����id
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&task_info->_task_id); 
	// flag=uint32
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&tmp_data); 
	// database=uint32
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&tmp_data); 
	// class_value=uint64
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&tmp_data64); 
	// global_id=uint64
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&tmp_data64); 
	// ��������
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&task_info->_type); 
	// �����С
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&task_info->_size); 
	// �����׺��
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_data); 
	ret_val = lx_pt_file_type_to_file_suffix(tmp_data, task_info->_file_suffix);
    sd_assert(ret_val == SUCCESS);
	// ����cid
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_data);
	if( tmp_data != 0)
	{
		sd_get_bytes(&tmp_buf, &tmp_len, cidbuf, tmp_data);
		ret_val = sd_string_to_cid((char *)cidbuf, task_info->_cid);
	}
	// ����gcid
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_data);
	if( tmp_data != 0)
	{
		sd_get_bytes(&tmp_buf, &tmp_len, cidbuf, tmp_data);
		ret_val = sd_string_to_cid((char *)cidbuf, task_info->_gcid);
	}
	// ��������
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_data);
	if( tmp_data >= MAX_FILE_NAME_BUFFER_LEN)
	{
		EM_LOG_DEBUG("lx_parse_get_task_list_new_task_info, filename_len = %u", tmp_data );
		tmp_buf += tmp_data;
		tmp_len -= tmp_data;
	}
	else
	{	
		sd_get_bytes(&tmp_buf, &tmp_len, tmp_url, tmp_data);
		
		ret_val = em_url_object_decode_ex(tmp_url, dec_str, MAX_FILE_NAME_BUFFER_LEN); 
		EM_LOG_DEBUG("lx_parse_get_task_list_new_task_info, filename:ret_val=%d,tmp=%s,dec_str=%s",ret_val, tmp_url ,dec_str);
		/* �����ļ����ضϵĴ���*/
	    if((ret_val == -1) && ((attr_len = sd_strlen(tmp_url))> MAX_FILE_NAME_BUFFER_LEN))
	    {
	         p_suffix = sd_strrchr((char*)tmp_url, '.');
	         if(p_suffix != NULL && (suffix_len = sd_strlen(p_suffix))<10 )
	         {
	                        /* �к�׺���ҺϷ����ض�ʱ�����ú�׺ */
	             name_len = sd_get_sub_utf8_str_len((char*)tmp_url, MAX_FILE_NAME_BUFFER_LEN - 1 - suffix_len);
	             sd_assert(name_len + suffix_len<MAX_FILE_NAME_BUFFER_LEN);	
	             sd_strncpy(task_name, tmp_url, name_len);
	             sd_strncpy(task_name + name_len, p_suffix, suffix_len);
	             ret_val = em_url_object_decode_ex(task_name, dec_str, MAX_FILE_NAME_BUFFER_LEN); 
	         }
	         else
	         {
	                        /* �޺�׺�����׺�����Ϸ��� ֱ�ӽض� */
	             name_len = sd_get_sub_utf8_str_len((char*)tmp_url, MAX_FILE_NAME_BUFFER_LEN-1);
	             sd_strncpy(task_name, tmp_url, name_len);
	             ret_val = em_url_object_decode_ex(task_name, dec_str, MAX_FILE_NAME_BUFFER_LEN); 
	         }
	    }
	    sd_assert(sd_strlen(dec_str)<MAX_FILE_NAME_BUFFER_LEN);					
	    sd_strncpy(task_info->_name,dec_str, MAX_FILE_NAME_BUFFER_LEN-1);           
	    /* check is Bad file name ! */
	    sd_get_valid_name(task_info->_name,NULL);
		// ���߷���������������ת����utf-8��ʽ
		sd_memset(utf8, 0x00, sizeof(utf8));
		ret_val = sd_any_format_to_utf8(task_info->_name, sd_strlen(task_info->_name), utf8, &utf8_len);
		EM_LOG_DEBUG("lx_parse_get_task_list_new_task_info,utf8 filename:ret_val=%d,name=%s,utf8=%s",ret_val, task_info->_name ,utf8);
		if(ret_val == SUCCESS || sd_strlen(utf8) != 0 )
		{
			// ת����utf8������ֵ����-1������ʵ���Ѿ�ת���ɹ�(������һ�£��ȼ���)
			sd_memset(task_info->_name, 0x00, sizeof(task_info->_name));
			sd_strncpy(task_info->_name, utf8, MAX_FILE_NAME_BUFFER_LEN);
		}
	}
	// ��������״̬ download_status=uint32
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_data); 
	task_info->_state = lx_pt_download_status_to_task_state_int(tmp_data);
	// ���������ٶȺͽ���
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_data); 
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &task_info->_progress); 
	// used_time=uint32
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_data); 
	// ����ʣ��ʱ��
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&tmp_data); 
    if(tmp_data>0 && tmp_data<=(24*60*60))
    {
        /* ����һ�� */
        task_info->_left_live_time = 1;
    }
    else
    {
        tmp_data = tmp_data/(24*60*60);
        task_info->_left_live_time = (_u32)tmp_data;
    }
	// ����lixian_url
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_data);
	if( tmp_data >= 2*MAX_URL_LEN)
	{
		EM_LOG_DEBUG("lx_parse_get_task_list_new_task_info, lixian_url_len = %u", tmp_data );
		tmp_buf += tmp_data;
		tmp_len -= tmp_data;
	}
	else
	{
		sd_get_bytes(&tmp_buf, &tmp_len, tmp_tmp_url, tmp_data);
		if(sd_strlen(tmp_tmp_url) > 0)
	    {	
		    ret_val = em_url_object_decode_ex(tmp_tmp_url, tmp_long_url, 2*MAX_URL_LEN-1); 
		EM_LOG_DEBUG("lx_parse_get_task_list_new_task_info,lixian_url:ret_val=%d,tmp_url=%s,long_url=%s",ret_val,tmp_tmp_url ,tmp_long_url);
            sd_assert(ret_val != -1);
            ret_val = lx_handle_pubnet_url(tmp_long_url);
            sd_assert(ret_val == SUCCESS);
			sd_assert(sd_strlen(tmp_long_url)<MAX_URL_LEN);
			sd_strncpy(task_info->_url,tmp_long_url, MAX_URL_LEN-1);
		EM_LOG_DEBUG("lx_parse_get_task_list_new_task_info,lixian_url=%s",task_info->_url);
		}
	}
	// ����ԭʼ��ַurl
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_data);
	if( tmp_data >= 2*MAX_URL_LEN)
	{	
		EM_LOG_DEBUG("lx_parse_get_task_list_new_task_info, url_len = %u", tmp_data );
		tmp_buf += tmp_data;
		tmp_len -= tmp_data;
	}
	else
	{
		sd_memset(tmp_tmp_url, 0x00, sizeof(tmp_tmp_url));
		sd_get_bytes(&tmp_buf, &tmp_len, tmp_tmp_url, tmp_data);
		if(sd_strlen(tmp_tmp_url) > 0)
	    {	
		    ret_val = em_url_object_decode_ex(tmp_tmp_url, tmp_long_url, 2*MAX_URL_LEN-1); 
		EM_LOG_DEBUG("lx_parse_get_task_list_new_task_info,decode_origin_url:ret_val=%d,tmp_url=%s,long_url=%s",ret_val,tmp_tmp_url,tmp_long_url);
		    sd_assert(ret_val != -1);
	    	// ԭʼurlǿ��ת��Ϊutf-8��ʽ
			ret_val = sd_any_format_to_utf8(tmp_long_url, sd_strlen(tmp_long_url), utf8url, &utf8urllen);
		EM_LOG_DEBUG("lx_parse_get_task_list_new_task_info,origin_url to utf8:ret_val=%d,utf8url=%s",ret_val,utf8url);
		    sd_assert(ret_val != -1);
		    if(ret_val != -1)
		    {
			sd_strncpy(task_info->_origin_url,utf8url, MAX_URL_LEN-1);
		         task_info->_origin_url_len = sd_strlen(task_info->_origin_url);
		         ret_val = em_get_url_hash_value( task_info->_origin_url,task_info->_origin_url_len, &task_info->_origin_url_hash);
		         sd_assert(ret_val == SUCCESS);
		    }
			else
			{
				sd_strncpy(task_info->_origin_url,tmp_long_url, MAX_URL_LEN-1);
			}
		EM_LOG_DEBUG("lx_parse_get_task_list_new_task_info,origin_url=%s",task_info->_origin_url);
		}
	}
	// �����ض�λ��ַurl
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_data);
	sd_memset(tmp_url, 0x00, sizeof(tmp_url));
	sd_get_bytes(&tmp_buf, &tmp_len, tmp_url, tmp_data);
	// ����cookies
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_data);
	if( tmp_data >= MAX_URL_LEN)
	{	
		EM_LOG_DEBUG("lx_parse_get_task_list_new_task_info, cookies_len = %u", tmp_data );
		tmp_buf += tmp_data;
		tmp_len -= tmp_data;
	}
	else
		sd_get_bytes(&tmp_buf, &tmp_len, task_info->_cookie, tmp_data);
	// ����vod
	tmp_data = 0;
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_data);
	if(tmp_data != 0)
		task_info->_vod = TRUE;
	else
		task_info->_vod = FALSE;
	// status=uint32
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_data);
	// message=string, dt_committed, dt_deleted
	for(i = 0; i < 3; i++)
	{
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_data);
		if (tmp_data > 0)
		{
			tmp_buf += tmp_data;
			tmp_len -= tmp_data;
		}
	}

	// list_sum=uint32
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32)&task_info->_sub_file_num);
	// finish_sum=uint32
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32)&task_info->_finished_file_num);

	// flag_killed_in_a_second=uint32
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_data);
	// res_count=uint32
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_data);

	// using_res_count=uint32
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_data);
	// verify_flag=uint64
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, &tmp_data64);

	// verify_flag=uint64
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_data);
	sd_get_bytes(&tmp_buf, &tmp_len, tmp_url, tmp_data);
	
	ret_val = lx_add_task_to_map(task_info);
	if(MAP_DUPLICATE_KEY == ret_val)
		SAFE_DELETE(task_info);
	return SUCCESS;
}


_int32 lx_parse_resp_get_task_list_new(LX_PT_TASK_LS_NEW* p_resp)
{
	_int32 ret_val = SUCCESS;
	char * tmp_buf = p_resp->_action._resp_buffer;
	_int32 tmp_len = (_int32)p_resp->_action._resp_data_len;
	char * databuf = NULL;
	_u32 readsize = 0;
	LX_CMD_HEADER cmd_header = {0};
	_int32 msg_len = 0;
	char* cmd_msg = NULL;
	_int32 result = 0;
	_int32 user_info_len = 0;
	_int32 task_num = 0;
	_int32 task_len = 0;
	_int32 i;
	char * zlib_buf = NULL;
	char *transfer_buf = NULL;
	_int32 transfer_len = 0;
	LX_TASK_LIST *task_list = &p_resp->_resp;
	
	EM_LOG_DEBUG("lx_parse_resp_get_task_list_new" );
	// ���������б�Ƚϳ������ݿ��ܴ����ļ�������ļ��ж�ȡ����
	if( tmp_len >= MAX_POST_RESP_BUFFER_LEN )
	{
		if( p_resp->_action._file_id == 0)
		{
			if(sd_file_exist(p_resp->_action._file_path))
			{
				ret_val = sd_open_ex(p_resp->_action._file_path, O_FS_RDONLY, &p_resp->_action._file_id);
				CHECK_VALUE(ret_val);
				ret_val = sd_malloc(tmp_len, (void **)&databuf);
				CHECK_VALUE(ret_val);
				sd_memset(databuf, 0x00, tmp_len);

				ret_val = sd_pread(p_resp->_action._file_id, databuf, tmp_len, 0, &readsize);
				CHECK_VALUE(ret_val);

				if( readsize != tmp_len )
				{
					ret_val = LXE_READ_FILE_WRONG;
					goto ErrHandle;
				}
				else
					tmp_buf = databuf;
				sd_close_ex(p_resp->_action._file_id);
				p_resp->_action._file_id = 0;
			}
			else
				return LXE_FILE_NOT_EXIST;
		}
		else
		{
			return -1;
		}
	}
	
	ret_val = lx_aes_decrypt(tmp_buf, (u32*)&tmp_len);
	CHECK_VALUE(ret_val);
	/* ��ȡ����ͷ */
	lx_pt_get_header(&tmp_buf, &tmp_len, &cmd_header);

	/* ѹ����־Ϊ256���ʾ���������ص���Ϣ����ѹ�� (�߰汾�ķ��������Զ�ѹ��) */
	lx_pt_zlib_uncompress(cmd_header._compress_flag, &tmp_buf, &tmp_len, &zlib_buf);
	
	// �������� result=uint32
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&result);

	if (result != 0) 
	{	
		ret_val = result;
		EM_LOG_DEBUG("lx_parse_resp_get_task_list_new, result = %d", result );
		goto ErrHandle;
	}
	// message=string
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &msg_len);

	if (msg_len > 0)
	{
		cmd_msg = tmp_buf;
		tmp_buf += msg_len;
		tmp_len -= msg_len;
	}

	/* ��ȡ�û���Ϣ */

	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &user_info_len); 

	transfer_buf = tmp_buf;
	transfer_len = tmp_len;
	
	ret_val = lx_parse_get_task_list_new_user_info(transfer_buf, transfer_len, task_list);

	if (user_info_len != 0) 
    {   
    	tmp_buf += user_info_len;
		tmp_len -= user_info_len;
	}

	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &task_num);
	task_list->_task_num = task_num;
	for(i = 0; i< task_num; i++)
	{
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, &task_len); 

		transfer_buf = tmp_buf;
		transfer_len = tmp_len;
		
		ret_val = lx_parse_get_task_list_new_task_info(transfer_buf, transfer_len);
		if( SUCCESS != ret_val)
		{	
			EM_LOG_DEBUG("lx_parse_get_task_list_new_task_info, ret_val = %d", ret_val );
			break;
		}
		if (task_len != 0) 
	    {   
	    	tmp_buf += task_len;
			tmp_len -= task_len;
		}
	}
	
	lx_pt_zlib_uncompress_free(zlib_buf);

ErrHandle:	
	SAFE_DELETE(databuf);
	return ret_val;
}


_int32 lx_build_req_get_overdue_or_deleted_task_list(LX_PT_OD_OR_DEL_TASK * p_req)
{
	_int32 ret_val = SUCCESS;
	char * tmp_buf = p_req->_action._req_buffer;
	_int32 tmp_len = (_int32)p_req->_action._req_buffer_len;
	
	EM_LOG_DEBUG("lx_build_req_get_overdue_or_deleted_task_list" );
    
	/* �������ͷ */
	ret_val = lx_pt_set_header(&tmp_buf, &tmp_len, &p_req->_req._cmd_header);
    
	/* ����û���Ϣ */
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)p_req->_req._jump_key_len);
	sd_set_bytes(&tmp_buf, &tmp_len, p_req->_req._jump_key, (_int32)p_req->_req._jump_key_len);
	
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)p_req->_req._userid);
	
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)p_req->_req._vip_level);
    
	/* ����Э�������ʣ���ֶ�*/
	sd_set_int32_to_lt(&tmp_buf, &tmp_len,  (_int32)p_req->_req._list_info._page_offset);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len,  (_int32)p_req->_req._list_info._max_task_num);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len,  (_int32)p_req->_req._list_info._task_type);
    
	/********************************************************************************************/
	/* Э��������ϣ�����action�е����󳤶Ⱥ�ͷ������� */
	p_req->_action._req_data_len = p_req->_action._req_buffer_len - tmp_len;
    p_req->_req._cmd_header._len = p_req->_action._req_data_len - sizeof(p_req->_req._cmd_header);
    
	/*aes ����*/
	tmp_len = p_req->_action._req_data_len;
	ret_val = lx_aes_encrypt(p_req->_action._req_buffer, (_u32*)&tmp_len);
	if (ret_val!= SUCCESS)
	{
		sd_assert(FALSE);
		return ret_val;
	}
    
	/* ���ܺ�Ҫ��������data len !!!!*/
	p_req->_action._req_data_len = tmp_len;
	
	return SUCCESS;	
}

_int32  lx_parse_get_overdue_or_deleted_task_info(char *tmp_buf, _int32 tmp_len, _int32 task_type)
{
	_int32 ret_val = SUCCESS;
	_int32 tmp_data = 0;
	_u64 tmp_data64 = 0;
	_int32 msg_len = 0;
	char* cmd_msg = NULL;
	char cidbuf[41] = {0};
	char tmp_long_url[2*MAX_URL_LEN] = {0};
	char tmp_tmp_url[2*MAX_URL_LEN] = {0};
	char tmp_url[MAX_URL_LEN] = {0};
	_int32 i = 0;
	char utf8[MAX_FILE_NAME_BUFFER_LEN] = {0};
	_u32 utf8_len = MAX_FILE_NAME_BUFFER_LEN;
	/* ���ڹ����ļ����ضϵĴ���*/
	char dec_str[MAX_FILE_NAME_BUFFER_LEN] = {0};
	char task_name[MAX_FILE_NAME_BUFFER_LEN] = {0};
	char* p_suffix = NULL;
	_u32 suffix_len = 0, name_len = 0, attr_len = 0; 
	
 	LX_TASK_INFO_EX *task_info = NULL;
	ret_val = lx_malloc_ex_task(&task_info);
	if(ret_val != SUCCESS)
		return ret_val;
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_data); 
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &msg_len);
	if (msg_len > 0)
	{
		cmd_msg = tmp_buf;
		tmp_buf += msg_len;
		tmp_len -= msg_len;
	}
	// ����id
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&task_info->_task_id); 
	//ret_val = lx_add_task_id_to_list(&task_info->_task_id);
	// ����global_id
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&tmp_data64); 
	// ��������
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&task_info->_type); 
	// �����С
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&task_info->_size); 
	// �����׺��
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_data); 
	ret_val = lx_pt_file_type_to_file_suffix(tmp_data, task_info->_file_suffix);
    sd_assert(ret_val == SUCCESS);
	// ����cid
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_data);
	if( tmp_data != 0)
	{
		sd_get_bytes(&tmp_buf, &tmp_len, cidbuf, tmp_data);
		ret_val = sd_string_to_cid((char *)cidbuf, task_info->_cid);
	}
	// ����gcid
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_data);
	if( tmp_data != 0)
	{
		sd_get_bytes(&tmp_buf, &tmp_len, cidbuf, tmp_data);
		ret_val = sd_string_to_cid((char *)cidbuf, task_info->_gcid);
	}
	// ��������
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_data);
	if( tmp_data >= MAX_FILE_NAME_BUFFER_LEN)
	{
		EM_LOG_DEBUG("lx_parse_get_overdue_or_deleted_task_info, filename_len = %u", tmp_data);
		tmp_buf += tmp_data;
		tmp_len -= tmp_data;
	}
	else
	{	
		sd_get_bytes(&tmp_buf, &tmp_len, tmp_url, tmp_data);
		ret_val = em_url_object_decode_ex(tmp_url, dec_str, MAX_FILE_NAME_BUFFER_LEN); 
		/* �����ļ����ضϵĴ���*/
	    if((ret_val == -1) && ((attr_len = sd_strlen(tmp_url))> MAX_FILE_NAME_BUFFER_LEN))
	    {
	         p_suffix = sd_strrchr((char*)tmp_url, '.');
	         if(p_suffix != NULL && (suffix_len = sd_strlen(p_suffix))<10 )
	         {
	                        /* �к�׺���ҺϷ����ض�ʱ�����ú�׺ */
	             name_len = sd_get_sub_utf8_str_len((char*)tmp_url, MAX_FILE_NAME_BUFFER_LEN - 1 - suffix_len);
	             sd_assert(name_len + suffix_len<MAX_FILE_NAME_BUFFER_LEN);	
	             sd_strncpy(task_name, tmp_url, name_len);
	             sd_strncpy(task_name + name_len, p_suffix, suffix_len);
	             ret_val = em_url_object_decode_ex(task_name, dec_str, MAX_FILE_NAME_BUFFER_LEN); 
	         }
	         else
	         {
	                        /* �޺�׺�����׺�����Ϸ��� ֱ�ӽض� */
	             name_len = sd_get_sub_utf8_str_len((char*)tmp_url, MAX_FILE_NAME_BUFFER_LEN-1);
	             sd_strncpy(task_name, tmp_url, name_len);
	             ret_val = em_url_object_decode_ex(task_name, dec_str, MAX_FILE_NAME_BUFFER_LEN); 
	         }
	    }
	    sd_assert(sd_strlen(dec_str)<MAX_FILE_NAME_BUFFER_LEN);					
	    sd_strncpy(task_info->_name,dec_str, MAX_FILE_NAME_BUFFER_LEN-1);
	                
	    /* check is Bad file name ! */
	    sd_get_valid_name(task_info->_name,NULL);
		// ���߷���������������ת����utf-8��ʽ
		ret_val = sd_any_format_to_utf8(task_info->_name, sd_strlen(task_info->_name), utf8, &utf8_len);
		if(ret_val == SUCCESS)
		{
			sd_memset(task_info->_name, 0x00, sizeof(task_info->_name));
			sd_strncpy(task_info->_name, utf8, utf8_len);
		}
	}
	// ��������״̬
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_data); 
	//sd_snprintf(tbuf, sizeof(tbuf), "%d", tmp_data);
	if(task_type)
		task_info->_state = LXS_OVERDUE;//lx_pt_download_status_to_task_state(tbuf);
	else
		task_info->_state = LXS_DELETED;
	// ���������ٶȺͽ���
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_data); 
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &task_info->_progress); 
	
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_data); 
	// ����ʣ��ʱ��
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&tmp_data64); 
	task_info->_left_live_time = 0;
	// ����lixian_url
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_data);
	if( tmp_data >= 2*MAX_URL_LEN)
	{
		EM_LOG_DEBUG("lx_parse_get_overdue_or_deleted_task_info, lixian_url_len = %u", tmp_data );
		tmp_buf += tmp_data;
		tmp_len -= tmp_data;
	}
	else
	{
		sd_get_bytes(&tmp_buf, &tmp_len, tmp_tmp_url, tmp_data);
		if(sd_strlen(tmp_tmp_url) > 0)
	    {	
		    ret_val = em_url_object_decode_ex(tmp_tmp_url, tmp_long_url, 2*MAX_URL_LEN-1); 
            sd_assert(ret_val != -1);
            ret_val = lx_handle_pubnet_url(tmp_long_url);
            sd_assert(ret_val == SUCCESS);
			sd_assert(sd_strlen(tmp_long_url)<MAX_URL_LEN);
			sd_strncpy(task_info->_url,tmp_long_url, MAX_URL_LEN-1);
		}
	}
	// ����ԭʼ��ַurl
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_data);
	if( tmp_data >= 2*MAX_URL_LEN)
	{	
		EM_LOG_DEBUG("lx_parse_get_overdue_or_deleted_task_info, url_len = %u", tmp_data );
		tmp_buf += tmp_data;
		tmp_len -= tmp_data;
	}
	else
	{
		sd_memset(tmp_tmp_url, 0x00, sizeof(tmp_tmp_url));
		sd_get_bytes(&tmp_buf, &tmp_len, tmp_tmp_url, tmp_data);
		if(sd_strlen(tmp_tmp_url) > 0)
	    {	
		    ret_val = em_url_object_decode_ex(tmp_tmp_url, tmp_long_url, 2*MAX_URL_LEN-1); 
		    sd_assert(ret_val != -1);
			sd_strncpy(task_info->_origin_url,tmp_long_url, MAX_URL_LEN-1);
		    if(ret_val != -1)
		    {
		         task_info->_origin_url_len = sd_strlen(task_info->_origin_url);
		         ret_val = em_get_url_hash_value( task_info->_origin_url,task_info->_origin_url_len, &task_info->_origin_url_hash);
		         sd_assert(ret_val == SUCCESS);
		    }
		}
	}

	// �����ض�λ��ַurl
	tmp_data = 0;
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_data);
	if (tmp_data > 0)
	{
		tmp_buf += tmp_data;
		tmp_len -= tmp_data;
	}
	// ����cookies
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_data);
	if( tmp_data >= MAX_URL_LEN)
	{	
		EM_LOG_DEBUG("lx_parse_get_overdue_or_deleted_task_info, cookies_len = %u", tmp_data );
		tmp_buf += tmp_data;
		tmp_len -= tmp_data;
	}
	else
		sd_get_bytes(&tmp_buf, &tmp_len, task_info->_cookie, tmp_data);
	// ����vod
	tmp_data = 0;
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_data);
	if(tmp_data != 0)
		task_info->_vod = TRUE;
	else
		task_info->_vod = FALSE;
	for(i = 0; i < 2; i++)
	{
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_data);
		if (tmp_data > 0)
		{
			tmp_buf += tmp_data;
			tmp_len -= tmp_data;
		}
	}
	ret_val = lx_add_task_to_map(task_info);
	if(MAP_DUPLICATE_KEY == ret_val)
		SAFE_DELETE(task_info);
	return SUCCESS;
}

_int32 lx_parse_resp_get_overdue_or_deleted_task_list(LX_PT_OD_OR_DEL_TASK* p_resp)
{
	_int32 ret_val = SUCCESS;
	char * tmp_buf = p_resp->_action._resp_buffer;
	_int32 tmp_len = (_int32)p_resp->_action._resp_data_len;
	char * databuf = NULL;
	_u32 readsize = 0;
	LX_CMD_HEADER cmd_header = {0};
	_int32 msg_len = 0;
	char* cmd_msg = NULL;
	_int32 result = 0;
	_int32 task_num = 0;
	_int32 task_len = 0;
	_int32 i;
	_int32 task_type = p_resp->_req._list_info._task_type;
	char * zlib_buf = NULL;
	char *transfer_buf = NULL;
	_int32 transfer_len = 0;
	
	EM_LOG_DEBUG("lx_parse_resp_get_overdue_or_deleted_task_list" );
	// ���������б�Ƚϳ������ݿ��ܴ����ļ�������ļ��ж�ȡ����
	if( tmp_len >= MAX_POST_RESP_BUFFER_LEN )
	{
		if( p_resp->_action._file_id == 0)
		{
			if(sd_file_exist(p_resp->_action._file_path))
			{
				ret_val = sd_open_ex(p_resp->_action._file_path, O_FS_RDONLY, &p_resp->_action._file_id);
				CHECK_VALUE(ret_val);
				ret_val = sd_malloc(tmp_len, (void **)&databuf);
				CHECK_VALUE(ret_val);
				sd_memset(databuf, 0x00, tmp_len);

				ret_val = sd_pread(p_resp->_action._file_id, databuf, tmp_len, 0, &readsize);
				CHECK_VALUE(ret_val);

				if( readsize != tmp_len )
				{
					ret_val = LXE_READ_FILE_WRONG;
					goto ErrHandle;
				}
				else
					tmp_buf = databuf;
				sd_close_ex(p_resp->_action._file_id);
				p_resp->_action._file_id = 0;
			}
			else
				return LXE_FILE_NOT_EXIST;
		}
		else
		{
			return -1;
		}
	}
	
	ret_val = lx_aes_decrypt(tmp_buf, (u32*)&tmp_len);
	CHECK_VALUE(ret_val);
	/* ��ȡ����ͷ */
	lx_pt_get_header(&tmp_buf, &tmp_len, &cmd_header);

	/* ѹ����־Ϊ256���ʾ���������ص���Ϣ����ѹ�� (�߰汾�ķ��������Զ�ѹ��) */
	lx_pt_zlib_uncompress(cmd_header._compress_flag, &tmp_buf, &tmp_len, &zlib_buf);
		
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&result);

	if (result != 0) 
	{	
		ret_val = result;
		goto ErrHandle;
	}
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &msg_len);

	if (msg_len > 0)
	{
		cmd_msg = tmp_buf;
		tmp_buf += msg_len;
		tmp_len -= msg_len;
	}

	/* ��ȡ�ѹ���������Ϣ */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&p_resp->_resp._task_num);

	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &task_num);
	for(i = 0; i< task_num; i++)
	{
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, &task_len); 

		transfer_buf = tmp_buf;
		transfer_len = tmp_len;
		
		ret_val = lx_parse_get_overdue_or_deleted_task_info(transfer_buf, transfer_len, task_type);
		if( SUCCESS != ret_val)
			break;
		
		if (task_len != 0) 
	    {   
	    	tmp_buf += task_len;
			tmp_len -= task_len;
		}
	}
	
	lx_pt_zlib_uncompress_free(zlib_buf);

ErrHandle:	
	SAFE_DELETE(databuf);
	return ret_val;
}


_int32 lx_build_req_commit_task(LX_PT_COMMIT_TASK* p_req)
{
	_int32 ret_val = SUCCESS;
	char * tmp_buf = p_req->_action._req_buffer;
	_int32 tmp_len = (_int32)p_req->_action._req_buffer_len;
	char * task_tmp_buf = NULL;  //��ʱָ��ʼ������񳤶ȵĵط�
	_int32 task_tmp_len = 0;     //��ʱ���񳤶ȣ������������Ϣ�����
	char gbk_taskname[MAX_URL_LEN] = {0},url_buffer[MAX_URL_LEN] = {0};
	_u32 gbk_len = MAX_URL_LEN;
	_int32 tmp = 0;
	BOOL is_cid_mode = FALSE;
	ctx_md5	md5;
	_u8 url_mid[16];
	char cid_buffer[CID_SIZE*2+4] = {0},gcid_buffer[CID_SIZE*2+4] = {0},fs_buffer[32] = {0},url_mid_buffer[64]={0};
	//LIST_ITERATOR cur = NULL, end = NULL;
	LX_CREATE_TASK_INFO*p_task_info = &p_req->_req._task_info;
	ret_val = p_req->_req._vip_level;
	EM_LOG_DEBUG("lx_build_req_commit_task:j_key[%u]=%s,userid=%llu,vip_level=%d,auto_charge=%d,fs=%llu,url_len=%d",p_req->_req._jump_key_len,p_req->_req._jump_key,p_req->_req._userid, ret_val,p_task_info->_is_auto_charge,p_task_info->_file_size,sd_strlen(p_task_info->_url));

	/* �������ͷ */
	ret_val = lx_pt_set_header(&tmp_buf, &tmp_len, &p_req->_req._cmd_header);

	/* ����û���Ϣ */
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)p_req->_req._jump_key_len);
	sd_set_bytes(&tmp_buf, &tmp_len, p_req->_req._jump_key, (_int32)p_req->_req._jump_key_len);
	
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)p_req->_req._userid);
	
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)p_req->_req._vip_level);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)p_task_info->_is_auto_charge);

	/*����������Ϊ1*/
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, 1);

	/* ���������Ϣ */
	
	task_tmp_buf = tmp_buf;
	task_tmp_len = tmp_len;

	//������task cmd len Ϊ0
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, 0);

	if(sd_strlen(p_task_info->_url)!=0)
	{
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)sd_strlen(p_task_info->_url));
		sd_set_bytes(&tmp_buf, &tmp_len, p_task_info->_url,  (_int32)sd_strlen(p_task_info->_url));
		EM_LOG_DEBUG("lx_build_req_commit_task:url[%d]=%s",sd_strlen(p_task_info->_url),p_task_info->_url);
		/* �������߷�����Ŀǰ��utf-8 �ַ�����֧�������⣬��ת��GBK �ύ*/
		/*
		ret_val = sd_any_format_to_gbk(p_task_info->_url, sd_strlen(p_task_info->_url), (_u8*)gbk_taskname, &gbk_len);
		if(ret_val!=SUCCESS)
		{
			sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)sd_strlen(p_task_info->_url));
			sd_set_bytes(&tmp_buf, &tmp_len, p_task_info->_url,  (_int32)sd_strlen(p_task_info->_url));
			EM_LOG_DEBUG("lx_build_req_commit_task:url[%d]=%s",sd_strlen(p_task_info->_url),p_task_info->_url);
		}
		else
		{
			sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)gbk_len);
			sd_set_bytes(&tmp_buf, &tmp_len, gbk_taskname, (_int32)gbk_len);
			EM_LOG_DEBUG("lx_build_req_commit_task:gbk_url[%d]=%s",gbk_len,gbk_taskname);
		}
		sd_memset(gbk_taskname,0x00,MAX_URL_LEN);
		gbk_len = MAX_URL_LEN;
		*/
	}
	else
	{
		/*
		pubnet.sandai.net
		�㲥url��ʽ����/�ָ��ֶ�
		�汾��0
		http://host:port/version/gcid/cid/file_size/head_size/index_size/bitrate/packet_size/packet_count/tail_pos/url_mid/filename
		�汾��1
		http://host:port/version/gcid/cid/file_size/head_size/index_size/bitrate/packet_size/packet_count/tail_pos/��չ���б�/url_mid/filename
		http://pubnet.sandai.net:8080/6/eedbe2982a4258fe93df0eb5a0613666a16edad6/3cc69aadfe137adfb37b8c93ec8b3ad40b43719b/5e812e6/200000/0/4afb9/0/0/5e812e6/0/e963f6ad9096758a261562414b4aed7f/e1fc688bd05b89350b4838afa9bf8c33/3cc69aadfe137adfb37b8c93ec8b3ad40b43719b_1.flv
		*/
		
		if(sd_is_cid_valid(p_task_info->_cid))
		{
			str2hex((char*)p_task_info->_cid, CID_SIZE, cid_buffer, CID_SIZE*2);
			EM_LOG_DEBUG("lx_build_req_commit_task:cid=%s",cid_buffer);
			sd_strtolower(cid_buffer);
		}
		else
		{
			CHECK_VALUE(INVALID_TCID);
		}
		
		if(sd_is_cid_valid(p_task_info->_gcid))
		{
			str2hex((char*)p_task_info->_gcid, CID_SIZE, gcid_buffer, CID_SIZE*2);
			EM_LOG_DEBUG("lx_build_req_commit_task:gcid=%s",gcid_buffer);
			sd_strtolower(gcid_buffer);
		}
		else
		{
			CHECK_VALUE(INVALID_GCID);
		}
		
		sd_u64toa(p_task_info->_file_size, fs_buffer, 32, 16);
		EM_LOG_DEBUG("lx_build_req_commit_task:fs=%s",fs_buffer);
		sd_strtolower(fs_buffer);

		md5_initialize(&md5);
		md5_update(&md5, (const unsigned char*)p_task_info->_gcid, CID_SIZE);
		md5_update(&md5, (const unsigned char*)p_task_info->_cid, CID_SIZE);
		md5_update(&md5, (const unsigned char*)&(p_task_info->_file_size), sizeof(_u64));
		md5_finish(&md5, url_mid);
		str2hex((char*)url_mid, 16, url_mid_buffer, CID_SIZE*2);
		EM_LOG_DEBUG("lx_build_req_commit_task:mid=%s",url_mid_buffer);
		sd_strtolower(url_mid_buffer);
	
		
		ret_val = sd_snprintf(url_buffer, MAX_URL_LEN-1,"http://pubnet.sandai.net:8080/0/%s/%s/%s/200000/0/4afb9/0/0/%s/%s/%s",gcid_buffer,cid_buffer,fs_buffer,fs_buffer,url_mid_buffer,p_task_info->_task_name);
		ret_val = sd_any_format_to_gbk(url_buffer, sd_strlen(url_buffer), (_u8*)gbk_taskname, &gbk_len);
		if(ret_val!=SUCCESS)
		{
			sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)sd_strlen(url_buffer));
			sd_set_bytes(&tmp_buf, &tmp_len, url_buffer,  (_int32)sd_strlen(url_buffer));
			EM_LOG_ERROR("lx_build_req_commit_task:url_buffer[%u]=%s",sd_strlen(url_buffer),url_buffer);
		}
		else
		{
			sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)gbk_len);
			sd_set_bytes(&tmp_buf, &tmp_len, gbk_taskname, (_int32)gbk_len);
			EM_LOG_ERROR("lx_build_req_commit_task:gbk_url[%u]=%s",gbk_len,gbk_taskname);
		}
	
		is_cid_mode = TRUE;
		sd_memset(gbk_taskname,0x00,MAX_URL_LEN);
		gbk_len = MAX_URL_LEN;
	}
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)sd_strlen(p_task_info->_ref_url));
	sd_set_bytes(&tmp_buf, &tmp_len, p_task_info->_ref_url,  (_int32)sd_strlen(p_task_info->_ref_url));

	/*cookie*/
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, 0);
	/*sd_set_bytes(&tmp_buf, &tmp_len, p_task_info->_cookies,  (_int32)sd_strlen(p_task_info->_cookies));*/

	/* �������߷�����Ŀǰ��utf-8 �ļ�����֧�������⣬��ת��GBK �ύ*/
	ret_val = sd_any_format_to_gbk(p_task_info->_task_name, sd_strlen(p_task_info->_task_name), (_u8*)gbk_taskname, &gbk_len);
	if(ret_val!=SUCCESS)
	{
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)sd_strlen(p_task_info->_task_name));
		sd_set_bytes(&tmp_buf, &tmp_len, p_task_info->_task_name, (_int32)sd_strlen(p_task_info->_task_name));
	}
	else
	{
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)gbk_len);
		sd_set_bytes(&tmp_buf, &tmp_len, gbk_taskname, (_int32)gbk_len);
	}

	if(sd_is_cid_valid(p_task_info->_cid))
	{
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, CID_SIZE*2);
		str2hex((char*)p_task_info->_cid, CID_SIZE, cid_buffer, CID_SIZE*2);
		sd_set_bytes(&tmp_buf, &tmp_len, (char*)cid_buffer,  CID_SIZE*2);
	}
	else
	{
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, 0);
	}

	if(sd_is_cid_valid(p_task_info->_gcid))
	{
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, CID_SIZE*2);
		str2hex((char*)p_task_info->_gcid, CID_SIZE, cid_buffer, CID_SIZE*2);
		sd_set_bytes(&tmp_buf, &tmp_len, (char*)cid_buffer,  CID_SIZE*2);
	}
	else
	{
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, 0);
	}
	
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)p_task_info->_file_size);

	/*file type*/
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, 1);

	//��������task cmd len
	tmp = sizeof(_u32);
	sd_set_int32_to_lt(&task_tmp_buf, &tmp, task_tmp_len -tmp_len-sizeof(_u32));


	/********************************************************************************************/
	/* Э��������� */
	p_req->_action._req_data_len = p_req->_action._req_buffer_len-tmp_len;
    	p_req->_req._cmd_header._len = p_req->_action._req_data_len - sizeof(p_req->_req._cmd_header);

	/*aes ���� p_req->_action._req_buffer����ǰ���ͷ����Ϣ�г��ȵ��ֶ���lx_aes_encrypt����д*/
	tmp_len = p_req->_action._req_data_len;
	ret_val = lx_aes_encrypt(p_req->_action._req_buffer, (_u32*)&tmp_len);
	if (ret_val!= SUCCESS)
	{
		sd_assert(FALSE);
		return ret_val;
	}

	/* ���ܺ�����������data len !!! */
	p_req->_action._req_data_len = tmp_len;
	/*tmp_buf = p_req->_action._req_buffer + 2*sizeof(_u32);
	tmp = sizeof(_u32);
	sd_set_int32_to_lt(&tmp_buf, (_int32*)&tmp, (_int32)(tmp_len - LX_PKG_CLEARTEXT_LEN));*/
	
	return SUCCESS;	
}

_int32 lx_parse_resp_commit_task(LX_PT_COMMIT_TASK* p_resp)
{
	_int32 ret_val = SUCCESS;
	char * tmp_buf = p_resp->_action._resp_buffer;
	_int32 tmp_len = (_int32)p_resp->_action._resp_data_len;
	LX_CMD_HEADER cmd_header = {0};
	_int32 msg_len = 0;
	char* cmd_msg = NULL;
	_int32 result = 0,download_status = 0;
	_int32 task_num = 0;
	_int32 task_len = 0;
	_int32 tmp_msg_len = 0;
	_u32 file_type = 0;
	_int32 i;
	char buf[MAX_URL_LEN] = {0};
	char msginfo[64] = {0};
	char * zlib_buf = NULL;

	EM_LOG_DEBUG("lx_parse_resp_commit_task" );
	
    //set_customed_interface(ET_ZLIB_UNCOMPRESS,zlib_uncompress );
	ret_val = lx_aes_decrypt(tmp_buf, (u32*)&tmp_len);
	CHECK_VALUE(ret_val);

	/* ��ȡ����ͷ */
	lx_pt_get_header(&tmp_buf, &tmp_len, &cmd_header);
	/* ѹ����־Ϊ256���ʾ���������ص���Ϣ����ѹ�� (�߰汾�ķ��������Զ�ѹ��) */
	lx_pt_zlib_uncompress(cmd_header._compress_flag, &tmp_buf, &tmp_len, &zlib_buf);

	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&result);

	if (result != 0) return result;
	
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &msg_len);
	if (msg_len > 0)
	{
		cmd_msg = tmp_buf;
		tmp_buf += msg_len;
		tmp_len -= msg_len;
	}

	/* ��ȡ�����û�������Ϣ */
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._available_space);
	
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._max_space);
	
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._max_task_num);
	
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._current_task_num);
	
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&p_resp->_resp._is_goldbean_converted);
	
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._goldbean_total_num);
	
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._goldbean_need_num);
	
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._goldbean_get_space);
	
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._silverbean_total_num);
	
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._silverbaen_need_num);
    
	/* ��ȡÿ��������Ϣ */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &task_num);
    
	sd_assert(task_num==1);
	
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &task_len);
	
	//status: uint32, 0:success 3:task is not exist
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&p_resp->_resp._result);
	if(p_resp->_resp._result!=0)
	{
        EM_LOG_ERROR("resp._result = %d,%s\n", p_resp->_resp._result,tmp_buf+4);
		return p_resp->_resp._result;
	}
	//message: string
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_msg_len);
	if(tmp_msg_len > 0)
	{
		sd_get_bytes(&tmp_buf, &tmp_len, msginfo, tmp_msg_len);
		if( 0 == sd_strncmp(msginfo, "the same task", sd_strlen("the same task")) )
			return TASK_ALREADY_EXIST;
	}

	//Taskid: uint64
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._task_id);
	//url: string
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_msg_len);
	if(tmp_msg_len >= MAX_URL_LEN)
		return -1;
	sd_get_bytes(&tmp_buf, &tmp_len, buf, tmp_msg_len);

	if( sd_strlen(buf) > 5 )
	{
		if(sd_strncmp(buf, "ed2k://", sd_strlen("ed2k://"))==0)
		{
			p_resp->_resp._type = LXT_EMULE;
		}
		else if(sd_strncmp(buf, "ftp://", sd_strlen("ftp://"))==0)
		{
			p_resp->_resp._type = LXT_FTP;
		}
		else if(sd_strncmp(buf, "http://", sd_strlen("http://"))==0)
			p_resp->_resp._type = LXT_HTTP;
		else
			p_resp->_resp._type = LXT_UNKNOWN;
	}
	else
		p_resp->_resp._type = LXT_UNKNOWN;
	//taskname: string, cid: string, gcid: string
	for(i = 0; i < 3; i++)
	{
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_msg_len);
		if (tmp_msg_len > 0)
		{
			tmp_buf += tmp_msg_len;
			tmp_len -= tmp_msg_len;
		}
	}
	//filesize: uint64
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._file_size);
	//filetype: uint32
	sd_get_int32_from_lt(&tmp_buf, &tmp_len,(_int32*) &file_type);
#ifdef _DEBUG
	printf("file_type = %d\n", file_type);
#endif
	/*download_status: uint32_t
		DownloadStatus: ����״̬
		0�� ������
		1�� ������
		2�� ���
		3�� ʧ��
		4�� ��ʱû��
		5�� ��ͣ	*/
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &download_status);
	switch(download_status)
	{
		case 0:
			p_resp->_resp._status = LXS_WAITTING;
			break;
		case 1:
			p_resp->_resp._status = LXS_RUNNING;
			break;
		case 2:
			p_resp->_resp._status = LXS_SUCCESS;
			break;
		case 3:
			p_resp->_resp._status = LXS_FAILED;
			break;
		case 5:
			p_resp->_resp._status = LXS_PAUSED;
			break;
		default:
			sd_assert(FALSE);
			p_resp->_resp._status = LXS_FAILED;
			break;
	}

	//progress: uint32_t
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&p_resp->_resp._progress);
	/*lixian_url: string, ref_url: string, cookies:string
	for(i = 0; i < 3; i++)
	{
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_msg_len);
		if (tmp_msg_len > 0)
		{
			tmp_buf += tmp_msg_len;
			tmp_len -= tmp_msg_len;
		}
	}
	*/

	lx_pt_zlib_uncompress_free(zlib_buf);
	
	return SUCCESS;
}


_int32 lx_parse_resp_commit_task_info(LX_PT_COMMIT_TASK* p_resp, LX_TASK_INFO_EX *task_info)
{
	_int32 ret_val = SUCCESS;
	char * tmp_buf = p_resp->_action._resp_buffer;
	_int32 tmp_len = (_int32)p_resp->_action._resp_data_len;
	LX_CMD_HEADER cmd_header = {0};
	_int32 msg_len = 0;
	char* cmd_msg = NULL;
	_int32 result = 0,download_status = 0;
	_int32 task_num = 0;
	_int32 task_len = 0;
	_int32 tmp_msg_len = 0;
	_int64 tmp_data64 = 0;
	_u32 file_type = 0;
	char buf[MAX_URL_LEN] = {0};
	char msginfo[64] = {0};
	char utf8[MAX_FILE_NAME_BUFFER_LEN] = {0};
	_u32 utf8_len = MAX_FILE_NAME_BUFFER_LEN;
	_int64 left_live_time = 0;
    _int32 left_day = 0;
	char * zlib_buf = NULL;
	EM_LOG_DEBUG("lx_parse_resp_commit_task" );

	ret_val = lx_aes_decrypt(tmp_buf, (u32*)&tmp_len);
	CHECK_VALUE(ret_val);

	/* ��ȡ����ͷ */
	lx_pt_get_header(&tmp_buf, &tmp_len, &cmd_header);
    /* ѹ����־Ϊ256���ʾ���������ص���Ϣ����ѹ�� (�߰汾�ķ��������Զ�ѹ��) */
	lx_pt_zlib_uncompress(cmd_header._compress_flag, &tmp_buf, &tmp_len, &zlib_buf);
	
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&result);

	if (result != 0) return result;
	
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &msg_len);

	if (msg_len > 0)
	{
		cmd_msg = tmp_buf;
		tmp_buf += msg_len;
		tmp_len -= msg_len;
	}

	/* ��ȡ�����û�������Ϣ */
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._available_space);
	
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._max_space);
	
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._max_task_num);
	
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._current_task_num);
	
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&p_resp->_resp._is_goldbean_converted);
	
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._goldbean_total_num);
	
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._goldbean_need_num);
	
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._goldbean_get_space);
	
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._silverbean_total_num);
	
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._silverbaen_need_num);
    
	/* ��ȡÿ��������Ϣ */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &task_num);
    
	sd_assert(task_num==1);
	
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &task_len);
	
	//status: uint32, 0:success 3:task is not exist
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&p_resp->_resp._result);
	
#ifdef _DEBUG
	printf("p_resp->_resp._result = %d\n", p_resp->_resp._result);
#endif
	if(p_resp->_resp._result!=0)
	{
		return p_resp->_resp._result;
	}
	//message: string
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_msg_len);
	if(tmp_msg_len > 0)
	{
		sd_get_bytes(&tmp_buf, &tmp_len, msginfo, tmp_msg_len);
		if( 0 == sd_strncmp(msginfo, "the same task", sd_strlen("the same task")) )
			return TASK_ALREADY_EXIST;
	}

	//Taskid: uint64
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._task_id);
	task_info->_task_id = p_resp->_resp._task_id;
	//url
	sd_memset(buf, 0x00, sizeof(buf));
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_msg_len);
	if(tmp_msg_len >= MAX_URL_LEN)
		return -1;
	sd_get_bytes(&tmp_buf, &tmp_len, buf, tmp_msg_len);
	//sd_strncpy(task_info->_origin_url, buf, MAX_URL_LEN - 1);
	// ������������֮�󷵻ص��Ǳ������utf8��ʽ������Ҫǿ�ƽ��롣������̽�Ƚ�url��
	ret_val = em_url_object_decode_ex(buf, task_info->_origin_url, MAX_URL_LEN-1); 
    sd_assert(ret_val != -1);
	ret_val = sd_any_format_to_utf8(task_info->_origin_url, sd_strlen(task_info->_origin_url), utf8, &utf8_len);
	if(ret_val == SUCCESS)
	{
		sd_memset(task_info->_origin_url, 0x00, sizeof(task_info->_origin_url));
		sd_strncpy(task_info->_origin_url, utf8, utf8_len);
	}
     task_info->_origin_url_len = sd_strlen(task_info->_origin_url);
     ret_val = em_get_url_hash_value( task_info->_origin_url,task_info->_origin_url_len, &task_info->_origin_url_hash);
     sd_assert(ret_val == SUCCESS);

	if( sd_strlen(task_info->_origin_url) > 5 )
	{
		if(sd_strncmp(task_info->_origin_url, "ed2k://", sd_strlen("ed2k://"))==0)
		{
			task_info->_type = LXT_EMULE;
		}
		else if(sd_strncmp(task_info->_origin_url, "ftp://", sd_strlen("ftp://"))==0)
		{
			task_info->_type = LXT_FTP;
		}
		else if(sd_strncmp(task_info->_origin_url, "http://", sd_strlen("http://"))==0)
			task_info->_type = LXT_HTTP;
		else
			task_info->_type = LXT_UNKNOWN;
	}
	else
		task_info->_type = LXT_UNKNOWN;
	p_resp->_resp._type = task_info->_type;
	//taskname
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_msg_len);
	if(tmp_msg_len >= MAX_URL_LEN)
		return -1;
	sd_get_bytes(&tmp_buf, &tmp_len, task_info->_name, tmp_msg_len);
	// ���߷���������������ת����utf-8��ʽ
	ret_val = sd_any_format_to_utf8(task_info->_name, sd_strlen(task_info->_name), utf8, &utf8_len);
	if(ret_val == SUCCESS)
	{
		sd_memset(task_info->_name, 0x00, sizeof(task_info->_name));
		sd_strncpy(task_info->_name, utf8, utf8_len);
	}
	//cid
	sd_memset(buf, 0x00, sizeof(buf));
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_msg_len);
	if(tmp_msg_len >= MAX_URL_LEN)
		return -1;
	sd_get_bytes(&tmp_buf, &tmp_len, buf, tmp_msg_len);
	sd_string_to_cid(buf, task_info->_cid);
	//gcid
	sd_memset(buf, 0x00, sizeof(buf));
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_msg_len);
	if(tmp_msg_len >= MAX_URL_LEN)
		return -1;
	sd_get_bytes(&tmp_buf, &tmp_len, buf, tmp_msg_len);
	sd_string_to_cid(buf, task_info->_gcid);
	
	//filesize: uint64
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._file_size);
	task_info->_size = p_resp->_resp._file_size;
	//filetype: uint32
	sd_get_int32_from_lt(&tmp_buf, &tmp_len,(_int32*) &file_type);
	lx_pt_file_type_to_file_suffix(file_type, task_info->_file_suffix);
#ifdef _DEBUG
	printf("file_type = %d\n", file_type);
#endif
	/*download_status: uint32_t
		DownloadStatus: ����״̬
		0�� ������
		1�� ������
		2�� ���
		3�� ʧ��
		4�� ��ʱû��
		5�� ��ͣ	*/
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &download_status);
	switch(download_status)
	{
		case 0:
			p_resp->_resp._status = LXS_WAITTING;
			break;
		case 1:
			p_resp->_resp._status = LXS_RUNNING;
			break;
		case 2:
			p_resp->_resp._status = LXS_SUCCESS;
			break;
		case 3:
			p_resp->_resp._status = LXS_FAILED;
			break;
		case 5:
			p_resp->_resp._status = LXS_PAUSED;
			break;
		default:
			sd_assert(FALSE);
			p_resp->_resp._status = LXS_FAILED;
			break;
	}
	task_info->_state = p_resp->_resp._status;

	//progress: uint32_t
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&p_resp->_resp._progress);
	task_info->_progress = p_resp->_resp._progress;

	// lixian_url: string
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_msg_len);
	if(tmp_msg_len >= MAX_URL_LEN)
		return -1;
	sd_get_bytes(&tmp_buf, &tmp_len, task_info->_url, tmp_msg_len);
	ret_val = lx_handle_pubnet_url(task_info->_url);
	
	/*ref_url: string*/

	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_msg_len);
	if (tmp_msg_len > 0)
	{
		tmp_buf += tmp_msg_len;
		tmp_len -= tmp_msg_len;
	}
	/*cookies:string*/
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &tmp_msg_len);
	if(tmp_msg_len >= MAX_URL_LEN)
		return -1;
	sd_get_bytes(&tmp_buf, &tmp_len, task_info->_cookie, tmp_msg_len);

	/*classValue:uint64*/
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&tmp_data64);
	/*leftLiveTime:uint32*/
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&left_live_time);
	left_day = (_int32)left_live_time/(60*60*24); 
	task_info->_left_live_time = left_day;
	
	lx_pt_zlib_uncompress_free(zlib_buf);
	return SUCCESS;
}

_int32 lx_build_bt_req_commit_task(LX_PT_COMMIT_BT_TASK* p_req)
{
	_int32 ret_val = SUCCESS;
	char * tmp_buf = p_req->_action._req_buffer;  //ָ����Э���Ŀ�껺����
	_int32 tmp_len = (_int32)p_req->_action._req_buffer_len; //����Ŀ�껺������ǰ���ó���

	char info_hash_hex[HEX_INFO_HASH_LEN] = {0};
	_u32 info_hash_len = HEX_INFO_HASH_LEN;
	_u32 file_id = 0,file_num = 0;
	_u64 bt_file_size = 0;
	_u32 bt_read_size = 0;
	char *bt_file_data = NULL,*p_title = NULL;
	char utf8_title[MAX_FILE_NAME_BUFFER_LEN] = {0};
	_u32 utf8_len = MAX_FILE_NAME_BUFFER_LEN;
	char gbk_title[MAX_FILE_NAME_BUFFER_LEN] = {0};
	_u32 gbk_len = MAX_FILE_NAME_BUFFER_LEN;
	_u32 tmp = 0;
	LX_CREATE_BT_TASK_INFO* p_task_info = &p_req->_req._task_info;
	TORRENT_SEED_INFO *bt_seed_info;
	TORRENT_FILE_INFO *p_file_info = NULL;
	const char *padding_str = "_____padding_file";
	EM_EIGENVALUE eigenvalue;
	_u64 task_id = 0;
	
	EM_LOG_DEBUG("lx_build_req_commit_task" );

	/* �������ͷ */
	ret_val = lx_pt_set_header(&tmp_buf, &tmp_len, &p_req->_req._cmd_header);

	/* ����û���Ϣ */
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)p_req->_req._jump_key_len);
	sd_set_bytes(&tmp_buf, &tmp_len, p_req->_req._jump_key, (_int32)p_req->_req._jump_key_len);
	
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)p_req->_req._userid);
	
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)p_req->_req._vip_level);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)p_task_info->_is_auto_charge);

	/*���bt������Ϣ*/
	if(p_task_info->_seed_file_full_path!=NULL &&sd_strlen(p_task_info->_seed_file_full_path)>0)
	{
		/* ��Torrent �ļ�����bt���� */
		ret_val = tp_get_seed_info(p_task_info->_seed_file_full_path, UTF_8_SWITCH, &bt_seed_info);
		if( SUCCESS != ret_val )
			CHECK_VALUE(ret_val);

		// ����bt������������ص���Ϣ��ȫ�����˴���ȡ������Ϣ����
		sd_strncpy((char*)p_req->_req._info_hash, (const char*)bt_seed_info->_info_hash, INFO_HASH_LEN);
		if(NULL != p_title)
			sd_strncpy(p_req->_req._bt_title_name, bt_seed_info->_title_name, MAX_FILE_NAME_LEN - 1);
		
		str2hex((char*)bt_seed_info->_info_hash, INFO_HASH_LEN, info_hash_hex, info_hash_len);
		
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, info_hash_len);
		sd_set_bytes(&tmp_buf, &tmp_len, info_hash_hex, info_hash_len);

		eigenvalue._type = TT_BT;
		eigenvalue._url = NULL;
		eigenvalue._url_len = 0;
		sd_memcpy(eigenvalue._eigenvalue, info_hash_hex, 40);
		lx_get_task_id_by_eigenvalue(&eigenvalue ,&task_id);
		if(task_id!=0) 
		{
			tp_release_seed_info(bt_seed_info);
			return TASK_ALREADY_EXIST;
		}

		if(NULL != p_task_info->_title)
		{
			ret_val = sd_any_format_to_gbk(p_task_info->_title, sd_strlen(p_task_info->_title),(_u8*) gbk_title, &gbk_len);
			if(ret_val!=SUCCESS)
			{
				sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)sd_strlen(p_task_info->_title));
				sd_set_bytes(&tmp_buf, &tmp_len, p_task_info->_title, (_int32)sd_strlen(p_task_info->_title));
			}
			else
			{
				sd_set_int32_to_lt(&tmp_buf, &tmp_len,(_int32) gbk_len);
				sd_set_bytes(&tmp_buf, &tmp_len, gbk_title, (_int32)gbk_len);
			}
		}
		else
		{
			ret_val = sd_utf8_2_gbk(bt_seed_info->_title_name, bt_seed_info->_title_name_len, gbk_title, &gbk_len);
			if(ret_val!=SUCCESS)
			{
				sd_set_int32_to_lt(&tmp_buf, &tmp_len,(_int32) bt_seed_info->_title_name_len);
				sd_set_bytes(&tmp_buf, &tmp_len, bt_seed_info->_title_name, (_int32)bt_seed_info->_title_name_len);
			}
			else
			{
				sd_set_int32_to_lt(&tmp_buf, &tmp_len,(_int32) gbk_len);
				sd_set_bytes(&tmp_buf, &tmp_len, gbk_title, (_int32)gbk_len);
			}
		}
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, 0);
		//sd_set_bytes(&tmp_buf, &tmp_len, p_task_info->_ref_url, (_int32)sd_strlen(p_task_info->_ref_url));

		ret_val = sd_open_ex(p_task_info->_seed_file_full_path, O_FS_RDONLY, &file_id);
		if( SUCCESS != ret_val)
		{
			tp_release_seed_info(bt_seed_info);
			CHECK_VALUE(ret_val);
		}
		ret_val = sd_filesize(file_id, &bt_file_size);
		if( SUCCESS != ret_val)
		{
			tp_release_seed_info(bt_seed_info);
			sd_close_ex(file_id);
			CHECK_VALUE(ret_val);
		}
        
        if(bt_file_size==0)
        {
			tp_release_seed_info(bt_seed_info);
			sd_close_ex(file_id);
            CHECK_VALUE(EM_INVALID_FILE_SIZE);
        }
        
		if(((p_task_info->_file_num!=0)&& (p_task_info->_download_file_index_array == NULL))
           ||(p_task_info->_file_num>bt_seed_info->_file_num))
		{
			tp_release_seed_info(bt_seed_info);
			sd_close_ex(file_id);
			CHECK_VALUE(INVALID_ARGUMENT);
		}
        
		if(file_num==0)
		{
			file_num = bt_seed_info->_file_num;
		}
        
		if(bt_file_size+sizeof(_u32)*(file_num+2) < tmp_len)
		{
			ret_val = sd_malloc(bt_file_size, (void **)&bt_file_data);
			if( SUCCESS != ret_val)
			{
				tp_release_seed_info(bt_seed_info);
				sd_close_ex(file_id);
				CHECK_VALUE(ret_val);
			}
			ret_val = sd_read(file_id, bt_file_data, bt_file_size, &bt_read_size);
			if( (SUCCESS != ret_val) ||  (bt_read_size != bt_file_size) )
			{
				sd_free(bt_file_data);
				sd_close_ex(file_id);
				tp_release_seed_info(bt_seed_info);	
				CHECK_VALUE(INVALID_READ_SIZE);
			}
			
			sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)bt_read_size);
			sd_set_bytes(&tmp_buf, &tmp_len, bt_file_data, (_int32)bt_read_size);
			sd_free(bt_file_data);
		}
		else
		{
			sd_set_int32_to_lt(&tmp_buf, &tmp_len, 0);
		}
		
		sd_close_ex(file_id);
        
		if( file_num * sizeof(_u32) >= tmp_len)
		{
			tp_release_seed_info(bt_seed_info);
			CHECK_VALUE(NOT_ENOUGH_BUFFER);
		}
        
		if( p_task_info->_download_file_index_array != NULL)
		{
			/* ���ؽ���ָ�����ļ� */
			sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)p_task_info->_file_num);
			sd_set_bytes(&tmp_buf, &tmp_len, (char*)p_task_info->_download_file_index_array,(_int32)( p_task_info->_file_num * sizeof(_u32)));
		}
		else
		{
			/* �������п������ļ� */
			char *p_array_len = tmp_buf;
            
			file_num = 0;
			p_file_info = *(bt_seed_info->_file_info_array_ptr);
			
			tmp_buf+=sizeof(_u32);
			tmp_len-=sizeof(_u32);
			
			for(tmp=0;tmp<bt_seed_info->_file_num;tmp++)
			{
				if(( p_file_info->_file_size > MIN_BT_FILE_SIZE) && (0!=sd_strncmp( p_file_info->_file_name, padding_str, sd_strlen(padding_str) ) ))
				{
					sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)p_file_info->_file_index);
					file_num++;
				}
				p_file_info++;
			}
            
			if(file_num!=0)
			{
				tmp = sizeof(_u32);
				sd_set_int32_to_lt(&p_array_len, (_int32*)&tmp, (_int32)file_num);
			}
			else
			{
				tp_release_seed_info(bt_seed_info);
				CHECK_VALUE(INVALID_SEED_FILE);
			}
		}
		tp_release_seed_info(bt_seed_info);
	}
	else if(p_task_info->_magnet_url!=NULL &&sd_strlen(p_task_info->_magnet_url)>0)
    {
        /* �ô���������bt���� */
        _u8 info_hash[INFO_HASH_LEN]={0};
		// �������ؽӿ�ʹ��(�����������ѹ���������bt�����ԭʼ�������Ӹ�ʽΪbt://info_hash�ַ���)
		if( sd_strncmp(p_task_info->_magnet_url, "bt://", sd_strlen("bt://")) == 0 )
		{
			//sd_string_to_cid(p_task_info->_magnet_url + sd_strlen("bt://"), info_hash);
			sd_strncpy(info_hash_hex, p_task_info->_magnet_url + sd_strlen("bt://"), info_hash_len);
			sd_string_to_cid(info_hash_hex, info_hash);
		}
        else
		{	
			ret_val = em_parse_magnet_url(p_task_info->_magnet_url ,info_hash,utf8_title,NULL);
            CHECK_VALUE(ret_val);
			// �鿴�����Ƿ�����������MAP�� (�������ز���Ҫȥ��֤����ΪMAP���ܰ������ѹ��������б�)
			str2hex((const char*)info_hash, INFO_HASH_LEN, info_hash_hex, HEX_INFO_HASH_LEN);	
            eigenvalue._type = TT_BT;
            eigenvalue._url = NULL;
            eigenvalue._url_len = 0;
            sd_memcpy(eigenvalue._eigenvalue, info_hash_hex, 40);
            lx_get_task_id_by_eigenvalue(&eigenvalue ,&task_id);
            if(task_id!=0) 
            {
                return TASK_ALREADY_EXIST;
            }
        }
		
        /* Intfo_hash: string */
        sd_set_int32_to_lt(&tmp_buf, &tmp_len, info_hash_len);
        sd_set_bytes(&tmp_buf, &tmp_len, info_hash_hex, info_hash_len);
        
        /* Bt_title: string */
        if(NULL != p_task_info->_title)
        {
            p_title = p_task_info->_title;
        }
        else
        {
            p_title = utf8_title;
        }
        
        utf8_len = sd_strlen(p_title);
        ret_val = sd_any_format_to_gbk(p_title, utf8_len, (_u8*)gbk_title, &gbk_len);
        if(ret_val==SUCCESS)
        {
            sd_set_int32_to_lt(&tmp_buf, &tmp_len,(_int32) gbk_len);
            sd_set_bytes(&tmp_buf, &tmp_len, gbk_title, (_int32)gbk_len);
        }
        else
        {
            sd_set_int32_to_lt(&tmp_buf, &tmp_len,(_int32) utf8_len);
            sd_set_bytes(&tmp_buf, &tmp_len, p_title, (_int32)utf8_len);
        }

		// ����bt������������ص���Ϣ��ȫ�����˴���ȡ������Ϣ���沢�ڴ����ɹ������
		sd_strncpy((char*)p_req->_req._info_hash, (const char*)info_hash, INFO_HASH_LEN);
		if(NULL != p_title)
			sd_strncpy(p_req->_req._bt_title_name, p_title, MAX_FILE_NAME_LEN - 1);
        /* Ref_url: string */
        sd_set_int32_to_lt(&tmp_buf, &tmp_len, 0);
        //sd_set_bytes(&tmp_buf, &tmp_len, p_task_info->_ref_url, (_int32)sd_strlen(p_task_info->_ref_url));
        
        /* Data: string */
        sd_set_int32_to_lt(&tmp_buf, &tmp_len, 0);
        
        /* File_list : uint32[]  */
        if( p_task_info->_file_num * sizeof(_u32) >= tmp_len)
        {
            CHECK_VALUE(NOT_ENOUGH_BUFFER);
        }
        
        if( p_task_info->_download_file_index_array != NULL)
        {
            /* ���ؽ���ָ�����ļ� */
            sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)p_task_info->_file_num);
            sd_set_bytes(&tmp_buf, &tmp_len, (char*)p_task_info->_download_file_index_array,(_int32)( p_task_info->_file_num * sizeof(_u32)));
        }
        else
        {
            file_num = (tmp_len-sizeof(_u32)-32)/sizeof(_u32);
            if(file_num>LX_MAX_TASK_NUM_EACH_REQ)
            {
                file_num=LX_MAX_TASK_NUM_EACH_REQ;
            }
            
            sd_set_int32_to_lt(&tmp_buf, &tmp_len, file_num);
            for(tmp=0;tmp<file_num;tmp++)
            {
                sd_set_int32_to_lt(&tmp_buf, &tmp_len, tmp);
            }
        }
    }
	/********************************************************************************************/
	/* Э��������ϸ�����Ӧ�İ����ȵ���Ϣ */
	p_req->_action._req_data_len = p_req->_action._req_buffer_len - tmp_len;
    p_req->_req._cmd_header._len = p_req->_action._req_data_len - sizeof(p_req->_req._cmd_header);
    
	/*
     _int32 tmplen;
     p_req->_action->_req_buffer + sizeof(_u32) * 2;
     sd_set_int32_to_lt(&(p_req->_action->_req_buffer + sizeof(_u32) * 2), &tmplen, p_req->_req._cmd_header._len); 
     */
	/*aes ���� */
	tmp_len = p_req->_action._req_data_len;
	ret_val = lx_aes_encrypt(p_req->_action._req_buffer, (_u32*)&tmp_len);
	if (ret_val!= SUCCESS)
	{
		sd_assert(FALSE);
		return ret_val;
	}
	
	/* ���ܺ�����������data len !!! */
	p_req->_action._req_data_len = tmp_len;
    
	
	/*tmp_buf = p_req->_action._req_buffer + 2*sizeof(_u32);
     tmp = sizeof(_u32);
     sd_set_int32_to_lt(&tmp_buf, (_int32*)&tmp, (_int32)(tmp_len - LX_PKG_CLEARTEXT_LEN));*/
	
	return SUCCESS;	
}

_int32 lx_parse_bt_resp_commit_task_info(LX_PT_COMMIT_BT_TASK* p_resp, LX_TASK_INFO_EX *task_info)
{
	_int32 ret_val = SUCCESS;
	char * tmp_buf = p_resp->_action._resp_buffer; //ָ��������������ݵ�Ŀ�껺����
	_int32 tmp_len = (_int32)p_resp->_action._resp_data_len; //����Ŀ�껺������ǰ���ó���
	LX_CMD_HEADER cmd_header = {0};
	_int32 msg_len = 0;
	_int32 info_hash_len = 0;
	char* cmd_msg = NULL;
	char* info_hash_msg = NULL;
    char msginfo[64] = {0};
	char utf8url[MAX_URL_LEN] = {0};
	_u32 utf8urllen = MAX_URL_LEN;
	_u32 child_id_num;
	_u32 child_id_list_len;
	_u32 i;
	_u32 child_task_list_len;
	_u32 child_task_list_num;
	_int32 len;
	
	_int32 download_status;
	_u32 progress;
    
	_u32 num_waiting = 0;
	_u32 num_running = 0;
	_u32 num_success = 0;
	_u32 num_failed = 0;
	_u32 num_paused = 0;
	_u32 total_progress = 0;
	BOOL need_free_memory = FALSE;
	char * p_new_buffer = NULL;
	char * zlib_buf = NULL;
	//char info_hash_buf[HEX_INFO_HASH_LEN] = {0};
	
	EM_LOG_DEBUG("lx_parse_bt_resp_commit_task_info" );
    
	if(p_resp->_action._resp_data_len>p_resp->_action._resp_buffer_len)
	{
		_u32 file_id = 0,readsize = 0;
		ret_val = sd_malloc(p_resp->_action._resp_data_len, (void**)&tmp_buf);
		CHECK_VALUE(ret_val);
		
		p_new_buffer = tmp_buf;
		
		ret_val = em_open_ex(p_resp->_action._file_path, O_FS_RDONLY, &file_id);
		if(ret_val!=SUCCESS)
		{
			SAFE_DELETE(p_new_buffer);
			CHECK_VALUE(ret_val);
		}
        
		ret_val = em_read(file_id, tmp_buf,(_int32) p_resp->_action._resp_data_len,&readsize);
		em_close_ex(file_id);
		if(ret_val!=SUCCESS)
		{
			SAFE_DELETE(p_new_buffer);
			CHECK_VALUE(ret_val);
		}
		sd_assert(readsize==p_resp->_action._resp_data_len);
		need_free_memory = TRUE;
	}
	
	ret_val = lx_aes_decrypt(tmp_buf, (u32*)&tmp_len);
	if(ret_val!=SUCCESS)
	{
		if(need_free_memory)
		{
			SAFE_DELETE(p_new_buffer);
		}
		CHECK_VALUE(ret_val);
	}
    
	/* ��ȡ����ͷ */
	lx_pt_get_header(&tmp_buf, &tmp_len, &cmd_header);
	/* ѹ����־Ϊ256���ʾ���������ص���Ϣ����ѹ�� (�߰汾�ķ��������Զ�ѹ��) */
	lx_pt_zlib_uncompress(cmd_header._compress_flag, &tmp_buf, &tmp_len, &zlib_buf);
	/* ��ȡ����ͳ�����Ϣ��������������λ�� */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &p_resp->_resp._result);
	if (p_resp->_resp._result != 0) 
	{
		if(need_free_memory)
		{
			SAFE_DELETE(p_new_buffer);
		}
		return p_resp->_resp._result;
	}
    
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &msg_len);
    if(msg_len > 0)
	{
		sd_get_bytes(&tmp_buf, &tmp_len, msginfo, msg_len);
		if( 0 == sd_strncmp(msginfo, "the same task", sd_strlen("the same task")) )
			return TASK_ALREADY_EXIST;
	}
    
	/* ��ȡinfo_hashֵ */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &info_hash_len);
	if (info_hash_len > 0)
	{
		info_hash_msg = tmp_buf;
		tmp_buf += info_hash_len;
		tmp_len -= info_hash_len;
	}

	//sd_get_bytes(&tmp_buf, &tmp_len, info_hash_buf, HEX_INFO_HASH_LEN);
	
	/* ��ȡ�����û�������Ϣ */
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._available_space);
	
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._max_space);
    
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._file_size);
	
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._max_task_num);
	
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._current_task_num);
	
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&p_resp->_resp._is_goldbean_converted);
	
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._goldbean_total_num);
	
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._goldbean_need_num);
	
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._goldbean_get_space);
	
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._silverbean_total_num);
	
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._silverbaen_need_num);
    
	/*����������Ϣ Main_taskid: uint64*/
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._task_id);

	// Taskid_list: uint64[]
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*) &child_id_num);
    child_id_list_len = child_id_num * sizeof(_u64);
	if (child_id_list_len > 0)
	{
		tmp_buf += child_id_list_len;
		tmp_len -= child_id_list_len;
	}
    // Tasklist: tasklist[]
	sd_get_int32_from_lt(&tmp_buf, &tmp_len,  (_int32*)&child_task_list_num);
	//printf("child_task_list_len = %d\n", child_task_list_num);
	sd_assert(child_id_num!=0);
	sd_assert(child_id_num==child_task_list_num);
	
	p_resp->_resp._status= LXS_SUCCESS;
	p_resp->_resp._progress = 10000;
	if(child_task_list_num==0) 
	{
		if(need_free_memory)
		{
			SAFE_DELETE(p_new_buffer);
		}
		return SUCCESS;
	}
	
	for(i = 0; i < child_task_list_num; i++)
	{
		sd_get_int32_from_lt(&tmp_buf, &tmp_len,  (_int32*)&child_task_list_len);
		/*
			DownloadStatus: ����״̬
			0�� ������
			1�� ������
			2�� ���
			3�� ʧ��
			4�� ��ʱû��
			5�� ��ͣ
		*/
		
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, &download_status);
		switch(download_status)
		{
			case 0:
				num_waiting++;
				break;
			case 1:
				num_running++;
				break;
			case 2:
				num_success++;
				break;
			case 3:
				num_failed++;
				break;
			case 5:
				num_paused++;
				break;
			default:
				sd_assert(FALSE);
				num_failed++;
				break;
		}
		
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&progress);
		total_progress+=progress;
        
		len = child_task_list_len - 2 *sizeof(_u32);
		if ( len > 0)
		{
			tmp_buf += len;
			tmp_len -= len;
		}
		else
		{
			break;
		}
		
	}	
	// bt�����������״̬ = ��һ��������״̬Ϊ���У�ʧ�ܻ���ͣ��������Ϊ��Ӧ״̬�����гɹ����ǳɹ�״̬������ǵȴ�״̬
	if( num_running != 0 )
		p_resp->_resp._status = LXS_RUNNING;
	else
	if( num_failed != 0 )
		p_resp->_resp._status = LXS_FAILED;
	else
	if( num_paused != 0 )
		p_resp->_resp._status = LXS_PAUSED;
	else
	if( num_success == child_task_list_num )
		p_resp->_resp._status = LXS_SUCCESS;
	else
		p_resp->_resp._status = LXS_WAITTING;
	// bt������Ľ���= ������������ȵ�ƽ��ֵ
	p_resp->_resp._progress = total_progress/child_task_list_num;
	
	if(need_free_memory)
	{
		SAFE_DELETE(p_new_buffer);
	}

	task_info->_task_id = p_resp->_resp._task_id; 
	task_info->_type = LXT_BT_ALL; 
	p_resp->_resp._type = LXT_BT_ALL;
	task_info->_state = p_resp->_resp._status; 
	//task_info->_name = ; 
	task_info->_size = p_resp->_resp._file_size; 
	task_info->_progress = p_resp->_resp._progress; 

	// ������ʱ��ȡ������Ϣ���cid,origin_url��title_name��������Ϣ�����ⵥ������ѯ�ӿ�
	sd_strncpy((char*)task_info->_cid, (const char*)p_resp->_req._info_hash, INFO_HASH_LEN); 
	
	sd_memset(task_info->_origin_url, 0x00, sizeof(task_info->_origin_url));
	sd_memset(msginfo, 0x00, sizeof(msginfo));
	sd_strncpy(msginfo, (const char*)p_resp->_req._info_hash, sizeof(msginfo));
	sd_snprintf(task_info->_origin_url, MAX_URL_LEN, "bt://%s", msginfo); 

	sd_strncpy(task_info->_name, p_resp->_req._bt_title_name, MAX_FILE_NAME_LEN - 1);
	// ����BT����֮��ǿ��ת����UTF-8��ʽ
	ret_val = url_get_encode_mode(task_info->_origin_url, sd_strlen(task_info->_origin_url));
    if(ret_val == UEM_ASCII)
    {
    	//ASCII���ַ����⴦��
       ret_val = sd_unicode_2_utf8(task_info->_origin_url, sd_strlen(task_info->_origin_url), utf8url, &utf8urllen);
    }
    else
        ret_val = sd_any_format_to_utf8(task_info->_origin_url, sd_strlen(task_info->_origin_url), utf8url, &utf8urllen);

    if(ret_val == SUCCESS)
	{
		sd_memset(task_info->_origin_url, 0x00, sd_strlen(task_info->_origin_url));
		sd_strncpy(task_info->_origin_url, utf8url, utf8urllen);
	}

    task_info->_origin_url_len = sd_strlen(task_info->_origin_url);
    ret_val = em_get_url_hash_value( task_info->_origin_url,task_info->_origin_url_len, &task_info->_origin_url_hash);
    sd_assert(ret_val == SUCCESS);
	//task_info->_gcid = ; 
	//task_info->_vod = ; 
	//task_info->_url = ; 
	//task_info->_cookie = ; 
	task_info->_sub_file_num = child_task_list_num;
	task_info->_finished_file_num = num_success;
	//task_info->_left_live_time;
	//task_info->_origin_url_hash = ;
	
	lx_pt_zlib_uncompress_free(zlib_buf);
	return SUCCESS;
}

_int32 lx_parse_bt_resp_commit_task(LX_PT_COMMIT_BT_TASK* p_resp)
{
	_int32 ret_val = SUCCESS;
	char * tmp_buf = p_resp->_action._resp_buffer; //ָ��������������ݵ�Ŀ�껺����
	_int32 tmp_len = (_int32)p_resp->_action._resp_data_len; //����Ŀ�껺������ǰ���ó���
	LX_CMD_HEADER cmd_header = {0};
	_int32 msg_len = 0;
	_int32 info_hash_len = 0;
	char* cmd_msg = NULL;
	char* info_hash_msg = NULL;
    char msginfo[64] = {0};
	
	_u32 child_id_num;
	_u32 child_id_list_len;
	_u32 i;
	_u32 child_task_list_len;
	_u32 child_task_list_num;
	_int32 len;
	
	_int32 download_status;
	_u32 progress;
    
	_u32 num_waiting = 0;
	_u32 num_running = 0;
	_u32 num_success = 0;
	_u32 num_failed = 0;
	_u32 num_paused = 0;
	_u32 total_progress = 0;
	BOOL need_free_memory = FALSE;
	char * p_new_buffer = NULL;
	char * zlib_buf = NULL;
	
	EM_LOG_DEBUG("lx_parse_bt_resp_commit_task" );
    
	if(p_resp->_action._resp_data_len>p_resp->_action._resp_buffer_len)
	{
		_u32 file_id = 0,readsize = 0;
		ret_val = sd_malloc(p_resp->_action._resp_data_len, (void**)&tmp_buf);
		CHECK_VALUE(ret_val);
		
		p_new_buffer = tmp_buf;
		
		ret_val = em_open_ex(p_resp->_action._file_path, O_FS_RDONLY, &file_id);
		if(ret_val!=SUCCESS)
		{
			SAFE_DELETE(p_new_buffer);
			CHECK_VALUE(ret_val);
		}
        
		ret_val = em_read(file_id, tmp_buf,(_int32) p_resp->_action._resp_data_len,&readsize);
		em_close_ex(file_id);
		if(ret_val!=SUCCESS)
		{
			SAFE_DELETE(p_new_buffer);
			CHECK_VALUE(ret_val);
		}
		sd_assert(readsize==p_resp->_action._resp_data_len);
		need_free_memory = TRUE;
	}
	
	ret_val = lx_aes_decrypt(tmp_buf, (u32*)&tmp_len);
	if(ret_val!=SUCCESS)
	{
		if(need_free_memory)
		{
			SAFE_DELETE(p_new_buffer);
		}
		CHECK_VALUE(ret_val);
	}
    
	/* ��ȡ����ͷ */
	lx_pt_get_header(&tmp_buf, &tmp_len, &cmd_header);
	/* ѹ����־Ϊ256���ʾ���������ص���Ϣ����ѹ�� (�߰汾�ķ��������Զ�ѹ��) */
	lx_pt_zlib_uncompress(cmd_header._compress_flag, &tmp_buf, &tmp_len, &zlib_buf);
		
	/* ��ȡ����ͳ�����Ϣ��������������λ�� */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &p_resp->_resp._result);
	if (p_resp->_resp._result != 0) 
	{
		if(need_free_memory)
		{
			SAFE_DELETE(p_new_buffer);
		}
		return p_resp->_resp._result;
	}
    
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &msg_len);
    if(msg_len > 0)
	{
		sd_get_bytes(&tmp_buf, &tmp_len, msginfo, msg_len);
		if( 0 == sd_strncmp(msginfo, "the same task", sd_strlen("the same task")) )
			return TASK_ALREADY_EXIST;
	}
    
	/* ��ȡinfo_hashֵ */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &info_hash_len);
	if (info_hash_len > 0)
	{
		info_hash_msg = tmp_buf;
		tmp_buf += info_hash_len;
		tmp_len -= info_hash_len;
	}
	//sd_get_bytes(&tmp_buf, &tmp_len, &p_resp->_resp._info_hash, p_resp->_resp._info_hash_len);
	
	/* ��ȡ�����û�������Ϣ */
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._available_space);
	
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._max_space);
    
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._file_size);
	
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._max_task_num);
	
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._current_task_num);
	
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&p_resp->_resp._is_goldbean_converted);
	
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._goldbean_total_num);
	
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._goldbean_need_num);
	
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._goldbean_get_space);
	
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._silverbean_total_num);
	
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._silverbaen_need_num);
    
	/*����������Ϣ*/
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._task_id);
    
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*) &child_id_num);
    child_id_list_len = child_id_num * sizeof(_u64);
	if (child_id_list_len > 0)
	{
		tmp_buf += child_id_list_len;
		tmp_len -= child_id_list_len;
	}
    
	sd_get_int32_from_lt(&tmp_buf, &tmp_len,  (_int32*)&child_task_list_num);
	//printf("child_task_list_len = %d\n", child_task_list_num);
	sd_assert(child_id_num!=0);
	sd_assert(child_id_num==child_task_list_num);
	
	p_resp->_resp._status= LXS_SUCCESS;
	p_resp->_resp._type = LXT_BT_ALL;
	p_resp->_resp._progress = 10000;
	if(child_task_list_num==0) 
	{
		if(need_free_memory)
		{
			SAFE_DELETE(p_new_buffer);
		}
		return SUCCESS;
	}
	
	for(i = 0; i < child_task_list_num; i++)
	{
		sd_get_int32_from_lt(&tmp_buf, &tmp_len,  (_int32*)&child_task_list_len);
		/*
			DownloadStatus: ����״̬
			0�� ������
			1�� ������
			2�� ���
			3�� ʧ��
			4�� ��ʱû��
			5�� ��ͣ
		*/
		
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, &download_status);
		switch(download_status)
		{
			case 0:
				num_waiting++;
				break;
			case 1:
				num_running++;
				break;
			case 2:
				num_success++;
				break;
			case 3:
				num_failed++;
				break;
			case 5:
				num_paused++;
				break;
			default:
				sd_assert(FALSE);
				num_failed++;
				break;
		}
		
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&progress);
		total_progress+=progress;
        
		len = child_task_list_len - 2 *sizeof(_u32);
		if ( len > 0)
		{
			tmp_buf += len;
			tmp_len -= len;
		}
		else
		{
			break;
		}
		
	}	
	
	if( num_running != 0 )
		p_resp->_resp._status = LXS_RUNNING;
	else
	if( num_failed != 0 )
		p_resp->_resp._status = LXS_FAILED;
	else
	if( num_paused != 0 )
		p_resp->_resp._status = LXS_PAUSED;
	else
	if( num_success == child_task_list_num )
		p_resp->_resp._status = LXS_SUCCESS;
	else
		p_resp->_resp._status = LXS_WAITTING;

	p_resp->_resp._progress = total_progress/child_task_list_num;
	
	if(need_free_memory)
	{
		SAFE_DELETE(p_new_buffer);
	}

	lx_pt_zlib_uncompress_free(zlib_buf);
	
	return SUCCESS;
}


_int32 lx_build_req_delete_task(LX_PT_DELETE_TASK* p_req)
{
	_int32 ret_val = SUCCESS;
	char * tmp_buf = p_req->_action._req_buffer;
	_int32 tmp_len = (_int32)p_req->_action._req_buffer_len;
	
	EM_LOG_DEBUG("lx_build_req_delete_task" );
    
	/* �������ͷ */
	ret_val = lx_pt_set_header(&tmp_buf, &tmp_len, &p_req->_req._cmd_header);
    
	/* ����û���Ϣ */
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)p_req->_req._jump_key_len);
	sd_set_bytes(&tmp_buf, &tmp_len, p_req->_req._jump_key, (_int32)p_req->_req._jump_key_len);
	
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)p_req->_req._userid);
	
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)p_req->_req._vip_level);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)p_req->_req._flag);
    
	/* �������id  ��Ϊ1*/
	sd_set_int32_to_lt(&tmp_buf, &tmp_len,  1);
    
	/* �������id */
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)p_req->_req._task_id);
    
	/********************************************************************************************/
	/* Э��������� */
	p_req->_action._req_data_len = p_req->_action._req_buffer_len-tmp_len;
    p_req->_req._cmd_header._len = p_req->_action._req_data_len - sizeof(p_req->_req._cmd_header);
    
	/*aes ����*/
	tmp_len = p_req->_action._req_data_len;
	ret_val = lx_aes_encrypt(p_req->_action._req_buffer, (_u32*)&tmp_len);
	if (ret_val!= SUCCESS)
	{
		sd_assert(FALSE);
		return ret_val;
	}
    
	/* ���ܺ�Ҫ��������data len !!!!*/
	p_req->_action._req_data_len = tmp_len;
	/* ���ܺ�����������cmd_len */
	/*tmp_buf = p_req->_action._req_buffer + 2*sizeof(_u32);
     tmp = sizeof(_u32);
     sd_set_int32_to_lt(&tmp_buf, (_int32*)&tmp, (_int32)(tmp_len - LX_PKG_CLEARTEXT_LEN));*/
	
	return SUCCESS;	
}

_int32 lx_parse_resp_delete_task(LX_PT_DELETE_TASK* p_resp)
{
	_int32 ret_val = SUCCESS;
	_int32 result = 0;
	char * tmp_buf = p_resp->_action._resp_buffer;
	_int32 tmp_len = (_int32)p_resp->_action._resp_data_len;
	LX_CMD_HEADER cmd_header = {0};
	_int32 task_num = 0,task_info_len = 0,task_status = 0;
	char msgbuf[64] = {0};
	_int32 msglen = 0;
	char * zlib_buf = NULL;
	
	EM_LOG_DEBUG("lx_parse_resp_delete_task" );
    
	ret_val = lx_aes_decrypt(tmp_buf, (u32*)&tmp_len);
	CHECK_VALUE(ret_val);
    
	/* ��ȡ����ͷ */
	lx_pt_get_header(&tmp_buf, &tmp_len, &cmd_header);
	/* ѹ����־Ϊ256���ʾ���������ص���Ϣ����ѹ�� (�߰汾�ķ��������Զ�ѹ��) */
    lx_pt_zlib_uncompress(cmd_header._compress_flag, &tmp_buf, &tmp_len, &zlib_buf);
	
	sd_get_int32_from_lt(&tmp_buf, &tmp_len,&task_num);
	sd_assert(task_num==1);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len,&task_info_len);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len,&task_status);
	
	if(task_status!=0) 
	{
		return task_status;
	}

	sd_get_int32_from_lt(&tmp_buf, &tmp_len,&result);
	if(result != 0)
	{	
		sd_get_int32_from_lt(&tmp_buf, &tmp_len,&msglen);
		if(msglen > 0)
		{
			sd_get_bytes(&tmp_buf, &tmp_len, msgbuf, msglen);
			EM_LOG_DEBUG("lx_parse_resp_delete_task: error msg[%s]", msgbuf);
		}
		lx_pt_zlib_uncompress_free(zlib_buf);
		return result;
	}
	
	lx_pt_zlib_uncompress_free(zlib_buf);
	
	return SUCCESS;
}
_int32 lx_build_req_delete_tasks(LX_PT_DELETE_TASKS* p_req)
{
	_int32 ret_val = SUCCESS,i=0;
	char * tmp_buf = p_req->_action._req_buffer;
	_int32 tmp_len = (_int32)p_req->_action._req_buffer_len;
	_u64 *p_task_id = p_req->_req._p_task_ids;
	
	EM_LOG_DEBUG("lx_build_req_delete_tasks" );
    
	/* �������ͷ */
	ret_val = lx_pt_set_header(&tmp_buf, &tmp_len, &p_req->_req._cmd_header);
    
	/* ����û���Ϣ */
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)p_req->_req._jump_key_len);
	sd_set_bytes(&tmp_buf, &tmp_len, p_req->_req._jump_key, (_int32)p_req->_req._jump_key_len);
	
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)p_req->_req._userid);
	
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)p_req->_req._vip_level);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)p_req->_req._flag);
    
	/* �������id  ��*/
	sd_set_int32_to_lt(&tmp_buf, &tmp_len,  p_req->_req._task_num);
    
	/* �������id */
	for(i=0;i<p_req->_req._task_num;i++)
	{
		if(tmp_len<sizeof(_int64)) return INVALID_MEMORY;
		sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)(*p_task_id));
		p_task_id++;
	}
	/********************************************************************************************/
	/* Э��������� */
	p_req->_action._req_data_len = p_req->_action._req_buffer_len-tmp_len;
    p_req->_req._cmd_header._len = p_req->_action._req_data_len - sizeof(p_req->_req._cmd_header);
    
	/*aes ����*/
	tmp_len = p_req->_action._req_data_len;
	ret_val = lx_aes_encrypt(p_req->_action._req_buffer, (_u32*)&tmp_len);
	if (ret_val!= SUCCESS)
	{
		sd_assert(FALSE);
		return ret_val;
	}
    
	/* ���ܺ�Ҫ��������data len !!!!*/
	p_req->_action._req_data_len = tmp_len;
	/* ���ܺ�����������cmd_len */
	/*tmp_buf = p_req->_action._req_buffer + 2*sizeof(_u32);
     tmp = sizeof(_u32);
     sd_set_int32_to_lt(&tmp_buf, (_int32*)&tmp, (_int32)(tmp_len - LX_PKG_CLEARTEXT_LEN));*/
	
	return SUCCESS;	
}

_int32 lx_parse_resp_delete_tasks(LX_PT_DELETE_TASKS* p_resp)
{
	_int32 ret_val = SUCCESS;
	char * tmp_buf = p_resp->_action._resp_buffer;
	_int32 tmp_len = (_int32)p_resp->_action._resp_data_len;
	LX_CMD_HEADER cmd_header = {0};
	_int32 task_num = 0,task_info_len = 0,*task_status = p_resp->_resp._p_results,i = 0,msg_len = 0,final_result = 0;
	char* cmd_msg = NULL;
	_u64 task_id = 0,*p_task_id = p_resp->_resp._p_task_ids;
	char * zlib_buf = NULL;
	
	EM_LOG_DEBUG("lx_parse_resp_delete_tasks" );
    
	ret_val = lx_aes_decrypt(tmp_buf, (u32*)&tmp_len);
	CHECK_VALUE(ret_val);
    
	/* ��ȡ����ͷ */
	lx_pt_get_header(&tmp_buf, &tmp_len, &cmd_header);
	/* ѹ����־Ϊ256���ʾ���������ص���Ϣ����ѹ�� (�߰汾�ķ��������Զ�ѹ��) */
    lx_pt_zlib_uncompress(cmd_header._compress_flag, &tmp_buf, &tmp_len, &zlib_buf);
	
	sd_get_int32_from_lt(&tmp_buf, &tmp_len,&task_num);
	sd_assert(task_num==p_resp->_req._task_num);
	for(i=0;i<task_num;i++)
	{
		sd_get_int32_from_lt(&tmp_buf, &tmp_len,&task_info_len);
		sd_get_int32_from_lt(&tmp_buf, &tmp_len,task_status);
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, &msg_len);
		if (msg_len > 0)
		{
			cmd_msg = tmp_buf;
			EM_LOG_DEBUG("lx_parse_resp_delete_tasks:task_id=%llu,task_status=%d,cmd_msg=%s",*p_task_id,*task_status,cmd_msg );
			tmp_buf += msg_len;
			tmp_len -= msg_len;
		}
		sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&task_id);
		if(*task_status==0)
		{
			sd_assert(task_id==*p_task_id);
		}
		p_task_id++;
		task_status++;
	}
	
	sd_get_int32_from_lt(&tmp_buf, &tmp_len,&final_result);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &msg_len);
	if (msg_len > 0)
	{
		cmd_msg = tmp_buf;
		EM_LOG_DEBUG("lx_parse_resp_delete_tasks:final_result=%d,cmd_msg=%s",final_result,cmd_msg );
	}
	
	lx_pt_zlib_uncompress_free(zlib_buf);
	
	return SUCCESS;
}

_int32 lx_build_req_delay_task(LX_PT_DELAY_TASK* p_req)
{
	_int32 ret_val = SUCCESS;
	char * tmp_buf = p_req->_action._req_buffer;
	_int32 tmp_len = (_int32)p_req->_action._req_buffer_len;
	
	EM_LOG_DEBUG("lx_build_req_delay_task" );
    
	/* �������ͷ */
	ret_val = lx_pt_set_header(&tmp_buf, &tmp_len, &p_req->_req._cmd_header);
    
	/* ����û���Ϣ */
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)p_req->_req._jump_key_len);
	sd_set_bytes(&tmp_buf, &tmp_len, p_req->_req._jump_key, (_int32)p_req->_req._jump_key_len);
	
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)p_req->_req._userid);
	
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)p_req->_req._vip_level);
    
	/* �������id  ��Ϊ1*/
	sd_set_int32_to_lt(&tmp_buf, &tmp_len,  1);
    
	/* �������id */
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)p_req->_req._task_id);
    
	/********************************************************************************************/
	/* Э��������� */
	p_req->_action._req_data_len = p_req->_action._req_buffer_len - tmp_len;
    p_req->_req._cmd_header._len = p_req->_action._req_data_len - sizeof(p_req->_req._cmd_header);
    
	/*aes ����*/
	tmp_len = p_req->_action._req_data_len;
	ret_val = lx_aes_encrypt(p_req->_action._req_buffer, (_u32*)&tmp_len);
	if (ret_val!= SUCCESS)
	{
		sd_assert(FALSE);
		return ret_val;
	}
    
	/* ���ܺ�Ҫ��������data len !!!!*/
	p_req->_action._req_data_len = tmp_len;
	/* ���ܺ�����������cmd_len */
	/*tmp_buf = p_req->_action._req_buffer + 2*sizeof(_u32);
     tmp = sizeof(_u32);
     sd_set_int32_to_lt(&tmp_buf, (_int32*)&tmp, (_int32)(tmp_len - LX_PKG_CLEARTEXT_LEN));*/
	
	return SUCCESS;	
}

_int32 lx_parse_resp_delay_task(LX_PT_DELAY_TASK* p_resp)
{
	_int32 ret_val = SUCCESS;
	char * tmp_buf = p_resp->_action._resp_buffer;
	_int32 tmp_len = (_int32)p_resp->_action._resp_data_len;
	LX_CMD_HEADER cmd_header = {0};
	_int32 msg_len = 0;
	char* cmd_msg = NULL;
	_int32 task_num;
	_int32 task_len;
	_int64 task_id;
	_int64 left_live_time = 0;
    _int32 left_day = 0;
	char * zlib_buf = NULL;
	
	EM_LOG_DEBUG("lx_parse_resp_delay_task" );
    
	ret_val = lx_aes_decrypt(tmp_buf, (u32*)&tmp_len);
	CHECK_VALUE(ret_val);
    
	/* ��ȡ����ͷ */
	lx_pt_get_header(&tmp_buf, &tmp_len, &cmd_header);
	/* ѹ����־Ϊ256���ʾ���������ص���Ϣ����ѹ�� (�߰汾�ķ��������Զ�ѹ��) */
	lx_pt_zlib_uncompress(cmd_header._compress_flag, &tmp_buf, &tmp_len, &zlib_buf);
	
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&p_resp->_resp._result);
    
	if (p_resp->_resp._result != 0) return p_resp->_resp._result;
    
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &msg_len);
	if (msg_len > 0)
	{
		cmd_msg = tmp_buf;
		tmp_buf += msg_len;
		tmp_len -= msg_len;
	}
    
	
	/* ��ȡ����������Ϣ */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &task_num);
	
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &task_len); 
    
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, &task_id); 
	//printf("task_id = %lld\n", task_id);
	
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, &left_live_time); 
    left_day = (_int32)left_live_time/60/60/24; 
	p_resp->_resp._left_live_time = left_day;

	lx_pt_zlib_uncompress_free(zlib_buf);
	
	return SUCCESS;
}

//////////////////////////////  ��װ���񾫼��ѯ��Э���ʽ  ///////////////////////////////////
_int32 lx_build_req_miniquery_task(LX_PT_MINIQUERY_TASK* p_req)
{
	_int32 ret_val = SUCCESS;
	char * tmp_buf = p_req->_action._req_buffer;
	_int32 tmp_len = (_int32)p_req->_action._req_buffer_len;
	
	EM_LOG_DEBUG("lx_build_req_miniquery_task" );
    
	/* �������ͷ */
	ret_val = lx_pt_set_header(&tmp_buf, &tmp_len, &p_req->_req._cmd_header);
    
	/* ����û���Ϣ */
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)p_req->_req._jump_key_len);
	sd_set_bytes(&tmp_buf, &tmp_len, p_req->_req._jump_key, (_int32)p_req->_req._jump_key_len);
	
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)p_req->_req._userid);
	
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)p_req->_req._vip_level);
    
	/* �������id  ��Ϊ1*/
	sd_set_int32_to_lt(&tmp_buf, &tmp_len,  1);
    
	/* �������id */
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)p_req->_req._task_id);
    
	/********************************************************************************************/
	/* Э��������� */
	p_req->_action._req_data_len = p_req->_action._req_buffer_len - tmp_len;
    p_req->_req._cmd_header._len = p_req->_action._req_data_len - sizeof(p_req->_req._cmd_header);
    
	/*aes ����*/
	tmp_len = p_req->_action._req_data_len;
	ret_val = lx_aes_encrypt(p_req->_action._req_buffer, (_u32*)&tmp_len);
	if (ret_val!= SUCCESS)
	{
		sd_assert(FALSE);
		return ret_val;
	}
    
	/* ���ܺ�Ҫ��������data len !!!!*/
	p_req->_action._req_data_len = tmp_len;
	/* ���ܺ�����������cmd_len */
	/*tmp_buf = p_req->_action._req_buffer + 2*sizeof(_u32);
     tmp = sizeof(_u32);
     sd_set_int32_to_lt(&tmp_buf, (_int32*)&tmp, (_int32)(tmp_len - LX_PKG_CLEARTEXT_LEN));*/
	
	return SUCCESS;	
}

//////////////////////////////  �������񾫼��ѯ���ص����ݸ�ʽ  ///////////////////////////////////
_int32 lx_parse_resp_miniquery_task(LX_PT_MINIQUERY_TASK* p_resp)
{
	_int32 ret_val = SUCCESS;
	char * tmp_buf = p_resp->_action._resp_buffer;
	_int32 tmp_len = (_int32)p_resp->_action._resp_data_len;
	LX_CMD_HEADER cmd_header = {0};
	_int32 msg_len = 0;
	char* cmd_msg = NULL;
	_int32 task_num;
	_int32 task_len;
	_int64 task_id;
	_int64 left_live_time = 0;
	char task_name[254] = {0};
	char * zlib_buf = NULL;
	
	EM_LOG_DEBUG("lx_parse_resp_miniquery_task" );
    
	ret_val = lx_aes_decrypt(tmp_buf, (u32*)&tmp_len);
	CHECK_VALUE(ret_val);
    
	/* ��ȡ����ͷ */
	lx_pt_get_header(&tmp_buf, &tmp_len, &cmd_header);
	
    /* ѹ����־Ϊ256���ʾ���������ص���Ϣ����ѹ�� (�߰汾�ķ��������Զ�ѹ��) */
	lx_pt_zlib_uncompress(cmd_header._compress_flag, &tmp_buf, &tmp_len, &zlib_buf);
	
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&p_resp->_resp._result);
    
	if (p_resp->_resp._result != 0) return p_resp->_resp._result;
    
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &msg_len);
	if (msg_len > 0)
	{
		cmd_msg = tmp_buf;
		tmp_buf += msg_len;
		tmp_len -= msg_len;
	}
    
	
	/* Tasklist: task[] ������� */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &task_num);
	/* Tasklist: task[] ��һ������ĳ��� */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &task_len); 

    /* Status: uint32_t */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&p_resp->_resp._state); 
	/* Message: string */	
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &msg_len);
	if (msg_len > 0)
	{
		cmd_msg = tmp_buf;
		tmp_buf += msg_len;
		tmp_len -= msg_len;
	}
	/* Taskid: uint64_t */
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, &task_id);
	
    /* Taskname: string */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &msg_len);
	sd_get_bytes(&tmp_buf, &tmp_len, task_name, msg_len);
	
	/* Commit_time: uint64_t */
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._commit_time); 
	
	/* Left_live_time: uint64_t */
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, &left_live_time); 
    p_resp->_resp._left_live_time = (_u32)left_live_time/60/60/24; 

	lx_pt_zlib_uncompress_free(zlib_buf);
	return SUCCESS;
}


_int32 lx_build_req_get_user_info_task(LX_PT_GET_USER_INFO_TASK* p_req)
{
	_int32 ret_val = SUCCESS;
	char * tmp_buf = p_req->_action._req_buffer;
	_int32 tmp_len = (_int32)p_req->_action._req_buffer_len;
	
	EM_LOG_DEBUG("lx_build_req_get_user_info_task" );
    
	/* �������ͷ */
	ret_val = lx_pt_set_header(&tmp_buf, &tmp_len, &p_req->_req._cmd_header);
    
	/* ����û���Ϣ */
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)p_req->_req._jump_key_len);
	sd_set_bytes(&tmp_buf, &tmp_len, p_req->_req._jump_key, (_int32)p_req->_req._jump_key_len);
	
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)p_req->_req._userid);
	
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)p_req->_req._vip_level);
    
	p_req->_action._req_data_len = p_req->_action._req_buffer_len - tmp_len;
    p_req->_req._cmd_header._len = p_req->_action._req_data_len - sizeof(p_req->_req._cmd_header);
    
	/*aes ����*/
	tmp_len = p_req->_action._req_data_len;
	ret_val = lx_aes_encrypt(p_req->_action._req_buffer, (_u32*)&tmp_len);
	if (ret_val!= SUCCESS)
	{
		sd_assert(FALSE);
		return ret_val;
	}
    
	/* ���ܺ�Ҫ��������data len !!!!*/
	p_req->_action._req_data_len = tmp_len;
	/* ���ܺ�����������cmd_len */
	
	return SUCCESS;	
}

_int32 lx_parse_resp_get_user_info_task(LX_PT_GET_USER_INFO_TASK* p_resp)
{
    _int32 ret_val = SUCCESS;
	char * tmp_buf = p_resp->_action._resp_buffer;
	_int32 tmp_len = (_int32)p_resp->_action._resp_data_len;
	LX_CMD_HEADER cmd_header = {0};
	_int32 msg_len = 0;
	char* cmd_msg = NULL;
    _int32 date_len = 0;
    _int32 cookie_len = 0;
	char * zlib_buf = NULL;
	_int32 tmp_data = 0;
	_int64 tmp_data64 = 0;
	
	EM_LOG_DEBUG("lx_parse_resp_get_user_info_task" );
    
	ret_val = lx_aes_decrypt(tmp_buf, (u32*)&tmp_len);
	CHECK_VALUE(ret_val);
    
	/* ��ȡ����ͷ */
	lx_pt_get_header(&tmp_buf, &tmp_len, &cmd_header);
    /* ѹ����־Ϊ256���ʾ���������ص���Ϣ����ѹ�� (�߰汾�ķ��������Զ�ѹ��) */
	lx_pt_zlib_uncompress(cmd_header._compress_flag, &tmp_buf, &tmp_len, &zlib_buf);
		
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&p_resp->_resp._result);
    
	if (p_resp->_resp._result != 0) 
        return p_resp->_resp._result;
    
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &msg_len);
	if (msg_len > 0)
	{
		cmd_msg = tmp_buf;
		tmp_buf += msg_len;
		tmp_len -= msg_len;
	}
    
	/* ��ȡ����������Ϣ */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&p_resp->_resp._max_task_num);
	
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._max_space); 
    
    sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&tmp_data64); 
    
    sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&tmp_data64); 
    
    sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&tmp_data); 
    
    sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&tmp_data);
    
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&tmp_data);
    
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&tmp_data);
    
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, &date_len);
    if (date_len > 0)
	{
		tmp_buf += date_len;
		tmp_len -= date_len;
	}
    //sd_get_bytes(&tmp_buf, &tmp_len, p_resp->_resp._expire_data, date_len);
    
    sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._available_space);
    
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&p_resp->_resp._max_task_num);
    
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&tmp_data);
    
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&tmp_data);
    
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&tmp_data);
    
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&tmp_data);
    
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&tmp_data);
    
    sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&tmp_data64);
    
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, &cookie_len);
    if (cookie_len > 0)
	{
		tmp_buf += cookie_len;
		tmp_len -= cookie_len;
	}
    //sd_get_bytes(&tmp_buf, &tmp_len, p_resp->_resp._cookie, cookie_len);
    
    sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&p_resp->_resp._vip_level); 
    
    sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&p_resp->_resp._user_type);
    
    sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&tmp_data64);
    
    sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&tmp_data64);

	lx_pt_zlib_uncompress_free(zlib_buf);
	return SUCCESS;
}
#if 0
_int32 lx_build_req_query_task_info(LX_PT_QUERY_TASK_INFO* p_req)
{
	_int32 ret_val = SUCCESS;
	char * tmp_buf = p_req->_action._req_buffer;
	_int32 tmp_len = (_int32)p_req->_action._req_buffer_len;
	
	EM_LOG_DEBUG("lx_build_req_query_task_info" );
    
	/* �������ͷ */
	ret_val = lx_pt_set_header(&tmp_buf, &tmp_len, &p_req->_req._cmd_header);
    
	/* ����û���Ϣ */
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)p_req->_req._jump_key_len);
	sd_set_bytes(&tmp_buf, &tmp_len, p_req->_req._jump_key, (_int32)p_req->_req._jump_key_len);

	// userid
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)p_req->_req._userid);
	// taskid[]
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, 1);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)p_req->_req._task_id);

    //sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)0);
	//sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)0);
	p_req->_action._req_data_len = p_req->_action._req_buffer_len - tmp_len;
    p_req->_req._cmd_header._len = p_req->_action._req_data_len - sizeof(p_req->_req._cmd_header);
    
	/*aes ����*/
	tmp_len = p_req->_action._req_data_len;
	ret_val = lx_aes_encrypt(p_req->_action._req_buffer, (_u32*)&tmp_len);
	if (ret_val!= SUCCESS)
	{
		sd_assert(FALSE);
		return ret_val;
	}
    
	/* ���ܺ�Ҫ��������data len !!!!*/
	p_req->_action._req_data_len = tmp_len;
	/* ���ܺ�����������cmd_len */
	
	return SUCCESS;	
}

_int32 lx_parse_resp_query_task_info(LX_PT_QUERY_TASK_INFO* p_resp)
{
    _int32 ret_val = SUCCESS;
	char * tmp_buf = p_resp->_action._resp_buffer;
	_int32 tmp_len = (_int32)p_resp->_action._resp_data_len;
	LX_CMD_HEADER cmd_header = {0};
	_int32 msg_len = 0;
	char* cmd_msg = NULL;
	_int64 data_len_64 = 0;
    _int32 data_len = 0;
	_int32 task_num = 0;
	_int32 totol_len = 0;
	_int32 download_status = 0;
	char buf[MAX_URL_LEN] = {0};
	char * zlib_buf = NULL;
	
	EM_LOG_DEBUG("lx_parse_resp_query_task_info" );
    
	ret_val = lx_aes_decrypt(tmp_buf, (u32*)&tmp_len);
	CHECK_VALUE(ret_val);
    
	/* ��ȡ����ͷ */
	lx_pt_get_header(&tmp_buf, &tmp_len, &cmd_header);
	/* ѹ����־Ϊ256���ʾ���������ص���Ϣ����ѹ�� (�߰汾�ķ��������Զ�ѹ��) */
	lx_pt_zlib_uncompress(cmd_header._compress_flag, &tmp_buf, &tmp_len, &zlib_buf);
		
	// TaskInfos : DownloadTaskInfo[]  �������
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &task_num);
	// Tasklist: task[] ��һ������ĳ���
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&totol_len);
	// Status : uint32_t
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&p_resp->_resp._result);  
	if (p_resp->_resp._result != 0) 
        return p_resp->_resp._result;
    // Message : string
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &msg_len);
	if (msg_len > 0)
	{
		cmd_msg = tmp_buf;
		tmp_buf += msg_len;
		tmp_len -= msg_len;
	}
    
	/* taskid */
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, &data_len_64);
	if( data_len_64 != p_resp->_req._task_id)
		return LXE_TASK_ID_NOT_FOUND;
	/* taskname */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &msg_len);  
	if(msg_len >= MAX_FILE_NAME_BUFFER_LEN)
		return -1;
	sd_memset(buf, 0x00, sizeof(buf));
    sd_get_bytes(&tmp_buf, &tmp_len, buf, msg_len);
	/* downloadstatus */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&download_status); 
	p_resp->_resp._download_status = lx_pt_download_status_to_task_state_int(download_status);

	/* filesize */
    sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&data_len_64); 
    /* filetype */
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&data_len);
    /* speed */
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&data_len);
	/* progress */
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&p_resp->_resp._progress);
	/* UsedTime */
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&msg_len);
	/* LixianUrl */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &data_len);
	if(data_len >= MAX_URL_LEN)
		return -1;
	sd_memset(buf, 0x00, sizeof(buf));
    sd_get_bytes(&tmp_buf, &tmp_len, buf, data_len);
	/* referUrl */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &msg_len);
    if (msg_len > 0)
	{
		cmd_msg = tmp_buf;
		tmp_buf += msg_len;
		tmp_len -= msg_len;
	}
	/* Cookies */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &data_len);
	if(data_len >= MAX_URL_LEN)
		return -1;
	sd_memset(buf, 0x00, sizeof(buf));
    sd_get_bytes(&tmp_buf, &tmp_len, buf, data_len);
	//cid
	sd_memset(buf, 0x00, sizeof(buf));
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &data_len);
	if(data_len >= 128)
		return -1;
	sd_get_bytes(&tmp_buf, &tmp_len, buf, data_len);
	//sd_string_to_cid(buf, p_task_array->_cid);
	//gcid
	sd_memset(buf, 0x00, sizeof(buf));
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &data_len);
	if(data_len >= 128)
		return -1;
	sd_get_bytes(&tmp_buf, &tmp_len, buf, data_len);
	//sd_string_to_cid(buf, p_task_array->_gcid);
	/* LeftLiveTime */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &data_len);

	lx_pt_zlib_uncompress_free(zlib_buf);
	return SUCCESS;
}
#endif

_int32 lx_build_req_query_bt_task_info(LX_PT_QUERY_TASK_INFO* p_req)
{
	_int32 ret_val = SUCCESS;
	char * tmp_buf = p_req->_action._req_buffer;
	_int32 tmp_len = (_int32)p_req->_action._req_buffer_len;
	_int32 i = 0;
	EM_LOG_DEBUG("lx_build_req_query_bt_task_info" );
    
	/* �������ͷ */
	ret_val = lx_pt_set_header(&tmp_buf, &tmp_len, &p_req->_req._cmd_header);
    
	/* ����û���Ϣ */
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)p_req->_req._jump_key_len);
	sd_set_bytes(&tmp_buf, &tmp_len, p_req->_req._jump_key, (_int32)p_req->_req._jump_key_len);

	// userid
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)p_req->_req._userid);
	// taskid[]
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, p_req->_req._task_num);
	for(i = 0; i < p_req->_req._task_num; i++)
	{
		//printf("need query taskid = %llu\n", *(p_req->_req._task_id + i));
		sd_set_int64_to_lt(&tmp_buf, &tmp_len, *(p_req->_req._task_id + i));
	}

	p_req->_action._req_data_len = p_req->_action._req_buffer_len - tmp_len;
    p_req->_req._cmd_header._len = p_req->_action._req_data_len - sizeof(p_req->_req._cmd_header);
    
	/*aes ����*/
	tmp_len = p_req->_action._req_data_len;
	ret_val = lx_aes_encrypt(p_req->_action._req_buffer, (_u32*)&tmp_len);
	if (ret_val!= SUCCESS)
	{
		sd_assert(FALSE);
		return ret_val;
	}
    
	/* ���ܺ�Ҫ��������data len !!!!*/
	p_req->_action._req_data_len = tmp_len;
	/* ���ܺ�����������cmd_len */
	
	return SUCCESS;	
}


_int32 lx_parse_get_query_bt_task_info(char *tmp_buf, _int32 tmp_len)
{
	_int32 ret_val = 0;
	_int32 result = 0;
	_int32 msg_len = 0;
	char* cmd_msg = NULL;
	_int64 data_len_64 = 0;
    _int32 data_len = 0;
	_int32 download_status = 0;
	char cidbuf[41] = {0};
	char tmp_url[MAX_URL_LEN] = {0};
	_int32 i = 0;
	_u64 task_id = 0;
	LX_TASK_INFO_EX *task_info = NULL;
	char utf8[MAX_FILE_NAME_BUFFER_LEN] = {0};
	_u32 utf8_len = MAX_FILE_NAME_BUFFER_LEN;
	char info_hash_hex[HEX_INFO_HASH_LEN] = {0};
	_u32 info_hash_len = HEX_INFO_HASH_LEN;
	/* ���ڹ����ļ����ضϵĴ���*/
	char dec_str[MAX_FILE_NAME_BUFFER_LEN] = {0};
	char task_name[MAX_FILE_NAME_BUFFER_LEN] = {0};
	char* p_suffix = NULL;
	_u32 suffix_len = 0, name_len = 0, attr_len = 0; 


	// BTMainTaskId : uint64_t
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int32*)&task_id);  
	if (task_id != 0) 
	{
		task_info = lx_get_task_from_map(task_id);
		if( NULL == task_info)
			return -1;
	}
	task_info->_type = LXT_BT_ALL;
	// Status : uint32_t
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&result);  
	if (result != 0) 
        return result;
    // Message : string
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &msg_len);
	if (msg_len > 0)
	{
		cmd_msg = tmp_buf;
		tmp_buf += msg_len;
		tmp_len -= msg_len;
	}
    /* DownloadStatus : uint32_t */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&download_status); 
	task_info->_state = lx_pt_download_status_to_task_state_int(download_status);
	/* Speed : uint32_t  */
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&data_len);
	/* Progress : uint32_t */
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&task_info->_progress);
	/* UsedTime : uint32_t */
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&data_len);
	/* committime: uint64_t */
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, &data_len_64);
	/* taskname */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &data_len);  
	if(data_len >= MAX_FILE_NAME_BUFFER_LEN)
	{
		tmp_buf += data_len;
		tmp_len -= data_len;
	}
	sd_memset(tmp_url, 0x00, sizeof(tmp_url));
    sd_get_bytes(&tmp_buf, &tmp_len, tmp_url, data_len);
	ret_val = em_url_object_decode_ex(tmp_url, dec_str, MAX_FILE_NAME_BUFFER_LEN); 
	/* �����ļ����ضϵĴ���*/
    if((ret_val == -1) && ((attr_len = sd_strlen(tmp_url))> MAX_FILE_NAME_BUFFER_LEN))
    {
         p_suffix = sd_strrchr((char*)tmp_url, '.');
         if(p_suffix != NULL && (suffix_len = sd_strlen(p_suffix))<10 )
         {
            /* �к�׺���ҺϷ����ض�ʱ�����ú�׺ */
             name_len = sd_get_sub_utf8_str_len((char*)tmp_url, MAX_FILE_NAME_BUFFER_LEN - 1 - suffix_len);
             sd_assert(name_len + suffix_len<MAX_FILE_NAME_BUFFER_LEN);	
             sd_strncpy(task_name, tmp_url, name_len);
             sd_strncpy(task_name + name_len, p_suffix, suffix_len);
             ret_val = em_url_object_decode_ex(task_name, dec_str, MAX_FILE_NAME_BUFFER_LEN); 
         }
         else
         {
                        /* �޺�׺�����׺�����Ϸ��� ֱ�ӽض� */
             name_len = sd_get_sub_utf8_str_len((char*)tmp_url, MAX_FILE_NAME_BUFFER_LEN-1);
             sd_strncpy(task_name, tmp_url, name_len);
             ret_val = em_url_object_decode_ex(task_name, dec_str, MAX_FILE_NAME_BUFFER_LEN); 
         }
    }
    sd_assert(sd_strlen(dec_str)<MAX_FILE_NAME_BUFFER_LEN);		
	sd_memset(task_info->_name, 0x00, sizeof(task_info->_name));
    sd_strncpy(task_info->_name,dec_str, MAX_FILE_NAME_BUFFER_LEN-1);         
    /* check is Bad file name ! */
    sd_get_valid_name(task_info->_name,NULL);
	// ����������ת����utf-8��ʽ
	ret_val = sd_any_format_to_utf8(task_info->_name, sd_strlen(task_info->_name), utf8, &utf8_len);
	if(ret_val == SUCCESS)
	{
		sd_memset(task_info->_name, 0x00, sizeof(task_info->_name));
		sd_strncpy(task_info->_name, utf8, utf8_len);
	}
	/* Filesize: uint64_t */
    sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&task_info->_size); 
	/* Left_live_time: uint32_t */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&data_len);
	if(data_len>0 && data_len<=(24*60*60))
    {
        /* ����һ�� */
        task_info->_left_live_time = 1;
    }
    else
    {
        data_len = data_len/(24*60*60);
        task_info->_left_live_time = (_u32)data_len;
    }

	/* url: string */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &data_len);
	if( data_len >= MAX_URL_LEN)
	{
		tmp_buf += data_len;
		tmp_len -= data_len;
	}
	else
	{
		sd_memset(tmp_url, 0x00, sizeof(tmp_url));
		sd_get_bytes(&tmp_buf, &tmp_len, tmp_url, data_len);
		if(sd_strlen(tmp_url) > 0)
	    {	
	    	ret_val = em_url_object_decode_ex(tmp_url, task_info->_origin_url, MAX_URL_LEN - 1); 
	    	sd_assert(ret_val != -1);
			if(ret_val != -1)
		    {
		         task_info->_origin_url_len = sd_strlen(task_info->_origin_url);
		         ret_val = em_get_url_hash_value( task_info->_origin_url,task_info->_origin_url_len, &task_info->_origin_url_hash);
		         sd_assert(ret_val == SUCCESS);
		    }
			// bt�������ԭʼurl��ʽbt://info_hash�ַ���,���н�����infohash��䵽cid�ֶ�
			if( sd_strncmp(task_info->_origin_url, "bt://", sd_strlen("bt://")) == 0 )
			{
				sd_strncpy(info_hash_hex, task_info->_origin_url + sd_strlen("bt://"), info_hash_len);
				sd_memset(task_info->_cid, 0x00, sizeof(task_info->_cid));
				sd_string_to_cid(info_hash_hex, task_info->_cid);
			}
		}
	}	
	/* class_value: uint64 */
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&data_len_64); 
	/* file_attr: uint32 �Ƿ���Բ���  */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &task_info->_vod);
	
	return ret_val;
}


_int32 lx_parse_resp_query_bt_task_info(LX_PT_QUERY_TASK_INFO* p_resp)
{
    _int32 ret_val = SUCCESS;
	char * tmp_buf = p_resp->_action._resp_buffer;
	_int32 tmp_len = (_int32)p_resp->_action._resp_data_len;
	LX_CMD_HEADER cmd_header = {0};
	char * databuf = NULL;
	_u32 readsize = 0;
	_int32 i = 0;
	char *transfer_buf = NULL;
	_int32 transfer_len = 0;
	_int32 task_num = 0;
	_int32 total_len = 0;
	char * zlib_buf = NULL;
	
	EM_LOG_DEBUG("lx_parse_resp_query_bt_task_info" );

	if( tmp_len >= MAX_POST_RESP_BUFFER_LEN )
	{
		if( p_resp->_action._file_id == 0)
		{
			if(sd_file_exist(p_resp->_action._file_path))
			{
				ret_val = sd_open_ex(p_resp->_action._file_path, O_FS_RDONLY, &p_resp->_action._file_id);
				CHECK_VALUE(ret_val);
				ret_val = sd_malloc(tmp_len, (void **)&databuf);
				CHECK_VALUE(ret_val);
				sd_memset(databuf, 0x00, tmp_len);

				ret_val = sd_pread(p_resp->_action._file_id, databuf, tmp_len, 0, &readsize);
				CHECK_VALUE(ret_val);

				if( readsize != tmp_len )
					return LXE_READ_FILE_WRONG;
				else
					tmp_buf = databuf;
				sd_close_ex(p_resp->_action._file_id);
			}
			else
				return LXE_FILE_NOT_EXIST;
		}
		else
		{
			return -1;
		}
	}
	ret_val = lx_aes_decrypt(tmp_buf, (u32*)&tmp_len);
	CHECK_VALUE(ret_val);
    
	/* ��ȡ����ͷ */
	lx_pt_get_header(&tmp_buf, &tmp_len, &cmd_header);
	/* ѹ����־Ϊ256���ʾ���������ص���Ϣ����ѹ�� (�߰汾�ķ��������Զ�ѹ��) */
	lx_pt_zlib_uncompress(cmd_header._compress_flag, &tmp_buf, &tmp_len, &zlib_buf);
	// TaskInfos : DownloadTaskInfo[]  �������
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &task_num);
	for(i = 0; i < task_num; i++)
	{
		// Tasklist: task[] ��һ������ĳ���
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&total_len);

		transfer_buf = tmp_buf;
		transfer_len = tmp_len;
		
		lx_parse_get_query_bt_task_info(transfer_buf, transfer_len);
		
		if (total_len != 0) 
	    {   
	    	tmp_buf += total_len;
			tmp_len -= total_len;
		}

	}
	// CommandStatus:uint16
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &p_resp->_resp._result);
	
	SAFE_DELETE(databuf);
	lx_pt_zlib_uncompress_free(zlib_buf);
	return SUCCESS;
	
}


_int32 lx_build_req_batch_query_task_info(LX_PT_BATCH_QUERY_TASK_INFO* p_req)
{
	_int32 ret_val = SUCCESS;
	char * tmp_buf = p_req->_action._req_buffer;
	_int32 tmp_len = (_int32)p_req->_action._req_buffer_len;
	_int32 i = 0;
	EM_LOG_DEBUG("LX_PT_BATCH_QUERY_TASK_INFO" );
    
	/* �������ͷ */
	ret_val = lx_pt_set_header(&tmp_buf, &tmp_len, &p_req->_req._cmd_header);
    
	/* ����û���Ϣ */
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)p_req->_req._jump_key_len);
	sd_set_bytes(&tmp_buf, &tmp_len, p_req->_req._jump_key, (_int32)p_req->_req._jump_key_len);

	// userid
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)p_req->_req._userid);
	// taskid[]
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, p_req->_req._task_num);
	for(i = 0; i < p_req->_req._task_num; i++)
	{
		//printf("need query taskid = %llu\n", *(p_req->_req._task_id + i));
		sd_set_int64_to_lt(&tmp_buf, &tmp_len, *(p_req->_req._task_id + i));
	}
	// Peerid: string
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, 0);
	//QueryIndexTaskids : uint64_t[]
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, 0);
	
	/********************************************************************************************/
	/* Э��������� */
	p_req->_action._req_data_len = p_req->_action._req_buffer_len-tmp_len;
    p_req->_req._cmd_header._len = p_req->_action._req_data_len - sizeof(p_req->_req._cmd_header);

	/*aes ���� p_req->_action._req_buffer����ǰ���ͷ����Ϣ�г��ȵ��ֶ���lx_aes_encrypt����д*/
	tmp_len = p_req->_action._req_data_len;
	ret_val = lx_aes_encrypt(p_req->_action._req_buffer, (_u32*)&tmp_len);
	if (ret_val!= SUCCESS)
	{
		sd_assert(FALSE);
		return ret_val;
	}

	/* ���ܺ�����������data len !!! */
	p_req->_action._req_data_len = tmp_len;
	return SUCCESS;	
}

_int32 lx_parse_get_batch_query_task_info(char *tmp_buf, _int32 tmp_len)
{
	_int32 ret_val = 0;
	_int32 result = 0;
	_int32 msg_len = 0;
	char* cmd_msg = NULL;
	_int64 data_len_64 = 0;
    _int32 data_len = 0;
	_int32 download_status = 0;
	char cidbuf[41] = {0};
	char tmp_url[MAX_URL_LEN] = {0};
	_int32 i = 0;
	_u64 task_id = 0;
	LX_TASK_INFO_EX *task_info = NULL;
	char utf8[MAX_FILE_NAME_BUFFER_LEN] = {0};
	_u32 utf8_len = MAX_FILE_NAME_BUFFER_LEN;
	/* ���ڹ����ļ����ضϵĴ���*/
	char dec_str[MAX_FILE_NAME_BUFFER_LEN] = {0};
	char task_name[MAX_FILE_NAME_BUFFER_LEN] = {0};
	char* p_suffix = NULL;
	_u32 suffix_len = 0, name_len = 0, attr_len = 0; 
	
	// Status : uint32_t
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&result);  
	if (result != 0) 
        return result;
    // Message : string
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &msg_len);
	if (msg_len > 0)
	{
		cmd_msg = tmp_buf;
		tmp_buf += msg_len;
		tmp_len -= msg_len;
	}
    
	/* taskid */
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&task_id);
	task_info = lx_get_task_from_map(task_id);
	sd_assert(task_info!=NULL);
	if(task_info==NULL)
	{
		return -1;
		/*jump_len = total_len - msg_len - sizeof(_int32) - sizeof(_int64);
		tmp_buf += jump_len;
		tmp_len -= jump_len;
		continue;
		*/
	}
	/* taskname */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &data_len);  
	if(data_len >= MAX_FILE_NAME_BUFFER_LEN)
	{
		tmp_buf += data_len;
		tmp_len -= data_len;
	}
	sd_memset(tmp_url, 0x00, sizeof(tmp_url));
    sd_get_bytes(&tmp_buf, &tmp_len, tmp_url, data_len);
	ret_val = em_url_object_decode_ex(tmp_url, dec_str, MAX_FILE_NAME_BUFFER_LEN); 
	/* �����ļ����ضϵĴ���*/
    if((ret_val == -1) && ((attr_len = sd_strlen(tmp_url))> MAX_FILE_NAME_BUFFER_LEN))
    {
         p_suffix = sd_strrchr((char*)tmp_url, '.');
         if(p_suffix != NULL && (suffix_len = sd_strlen(p_suffix))<10 )
         {
            /* �к�׺���ҺϷ����ض�ʱ�����ú�׺ */
             name_len = sd_get_sub_utf8_str_len((char*)tmp_url, MAX_FILE_NAME_BUFFER_LEN - 1 - suffix_len);
             sd_assert(name_len + suffix_len<MAX_FILE_NAME_BUFFER_LEN);	
             sd_strncpy(task_name, tmp_url, name_len);
             sd_strncpy(task_name + name_len, p_suffix, suffix_len);
             ret_val = em_url_object_decode_ex(task_name, dec_str, MAX_FILE_NAME_BUFFER_LEN); 
         }
         else
         {
                        /* �޺�׺�����׺�����Ϸ��� ֱ�ӽض� */
             name_len = sd_get_sub_utf8_str_len((char*)tmp_url, MAX_FILE_NAME_BUFFER_LEN-1);
             sd_strncpy(task_name, tmp_url, name_len);
             ret_val = em_url_object_decode_ex(task_name, dec_str, MAX_FILE_NAME_BUFFER_LEN); 
         }
    }
    sd_assert(sd_strlen(dec_str)<MAX_FILE_NAME_BUFFER_LEN);					
    sd_strncpy(task_info->_name,dec_str, MAX_FILE_NAME_BUFFER_LEN-1);         
    /* check is Bad file name ! */
    sd_get_valid_name(task_info->_name,NULL);
	// ����������ת����utf-8��ʽ
	ret_val = sd_any_format_to_utf8(task_info->_name, sd_strlen(task_info->_name), utf8, &utf8_len);
	if(ret_val == SUCCESS)
	{
		sd_memset(task_info->_name, 0x00, sizeof(task_info->_name));
		sd_strncpy(task_info->_name, utf8, utf8_len);
	}
	/* downloadstatus */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&download_status); 
	task_info->_state = lx_pt_download_status_to_task_state_int(download_status);
	/* filesize */
    sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&task_info->_size); 
    /* filetype */
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&data_len);
	ret_val = lx_pt_file_type_to_file_suffix(data_len,task_info->_file_suffix);
    /* speed */
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&data_len);
	/* progress */
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&task_info->_progress);
	/* UsedTime */
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&data_len);
	/* LixianUrl */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &data_len);
	if( data_len >= MAX_URL_LEN)
	{
		tmp_buf += data_len;
		tmp_len -= data_len;
	}
	else
	{
		sd_memset(tmp_url, 0x00, sizeof(tmp_url));
		sd_get_bytes(&tmp_buf, &tmp_len, tmp_url, data_len);
		if(sd_strlen(tmp_url) > 0)
	    {	
	    	ret_val = em_url_object_decode_ex(tmp_url, task_info->_url, MAX_URL_LEN - 1); 
	    	sd_assert(ret_val != -1);
			ret_val = lx_handle_pubnet_url(task_info->_url);
			sd_assert(ret_val == SUCCESS);
		}
	}
	/* referUrl */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &msg_len);
    if (msg_len > 0)
	{
		cmd_msg = tmp_buf;
		tmp_buf += msg_len;
		tmp_len -= msg_len;
	}
	/* Cookies */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &data_len);
	if( data_len >= MAX_URL_LEN)
	{
		tmp_buf += data_len;
		tmp_len -= data_len;
	}
	else
	{
		sd_memset(task_info->_cookie, 0x00, sizeof(task_info->_cookie));
    	sd_get_bytes(&tmp_buf, &tmp_len, task_info->_cookie, data_len);
	}
	//cid
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &data_len);
	if( data_len != 0)
	{
		sd_get_bytes(&tmp_buf, &tmp_len, cidbuf, data_len);
		ret_val = sd_string_to_cid((char *)cidbuf, task_info->_cid);
	}
	//gcid
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &data_len);
	if( data_len != 0)
	{
		sd_memset(cidbuf, 0x00, sizeof(cidbuf));
		sd_get_bytes(&tmp_buf, &tmp_len, cidbuf, data_len);
		ret_val = sd_string_to_cid((char *)cidbuf, task_info->_gcid);
	}
	/* LeftLiveTime */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&data_len);
	if(data_len>0 && data_len<=(24*60*60))
    {
        /* ����һ�� */
        task_info->_left_live_time = 1;
    }
    else
    {
        data_len = data_len/(24*60*60);
        task_info->_left_live_time = (_u32)data_len;
    }
	/* ExtLixianUrllist: string[] */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &msg_len);
    if (msg_len > 0)
	{
		cmd_msg = tmp_buf;
		tmp_buf += msg_len;
		tmp_len -= msg_len;
	}
	/* TaskPeerList��TaskPeerInfo[] */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &data_len);
    if (data_len != 0)
	{
		for(i = 0; i < data_len; i++)
		{
			sd_get_int32_from_lt(&tmp_buf, &tmp_len, &msg_len);
			if(msg_len != 0)
			{
				tmp_buf += msg_len;
				tmp_len -= msg_len;
			}
		}
	}
	/* url: string */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &data_len);
	if( data_len >= MAX_URL_LEN)
	{
		tmp_buf += data_len;
		tmp_len -= data_len;
	}
	else
	{
		sd_memset(tmp_url, 0x00, sizeof(tmp_url));
		sd_get_bytes(&tmp_buf, &tmp_len, tmp_url, data_len);
		if(sd_strlen(tmp_url) > 0)
	    {	
	    	ret_val = em_url_object_decode_ex(tmp_url, task_info->_origin_url, MAX_URL_LEN - 1); 
	    	sd_assert(ret_val != -1);
			if(ret_val != -1)
		    {
		         task_info->_origin_url_len = sd_strlen(task_info->_origin_url);
		         ret_val = em_get_url_hash_value( task_info->_origin_url,task_info->_origin_url_len, &task_info->_origin_url_hash);
		         sd_assert(ret_val == SUCCESS);
		    }
			//ret_val = lx_handle_pubnet_url(task_info->_origin_url);
			//sd_assert(ret_val == SUCCESS);
		}
	}	
	/* committime: uint64_t */
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, &data_len_64);
	return ret_val;
}

_int32 lx_parse_resp_batch_query_task_info(LX_PT_BATCH_QUERY_TASK_INFO* p_resp)
{
    _int32 ret_val = SUCCESS;
	char * tmp_buf = p_resp->_action._resp_buffer;
	_int32 tmp_len = (_int32)p_resp->_action._resp_data_len;
	LX_CMD_HEADER cmd_header = {0};
	_int32 task_num = 0;
	_int32 i = 0;
	_int32 total_len = 0;
	char * databuf = NULL;
	_u32 readsize = 0;
	char *transfer_buf = NULL;
	_int32 transfer_len = 0;
	char * zlib_buf = NULL;
	EM_LOG_DEBUG("lx_parse_resp_batch_query_task_info" );

	if( tmp_len >= MAX_POST_RESP_BUFFER_LEN )
	{
		if( p_resp->_action._file_id == 0)
		{
			if(sd_file_exist(p_resp->_action._file_path))
			{
				ret_val = sd_open_ex(p_resp->_action._file_path, O_FS_RDONLY, &p_resp->_action._file_id);
				CHECK_VALUE(ret_val);
				ret_val = sd_malloc(tmp_len, (void **)&databuf);
				CHECK_VALUE(ret_val);
				sd_memset(databuf, 0x00, tmp_len);

				ret_val = sd_pread(p_resp->_action._file_id, databuf, tmp_len, 0, &readsize);
				CHECK_VALUE(ret_val);

				if( readsize != tmp_len )
					return LXE_READ_FILE_WRONG;
				else
					tmp_buf = databuf;
				sd_close_ex(p_resp->_action._file_id);
				p_resp->_action._file_id = 0;
			}
			else
				return LXE_FILE_NOT_EXIST;
		}
		else
		{
			return -1;
		}
	}
	
	ret_val = lx_aes_decrypt(tmp_buf, (u32*)&tmp_len);
	CHECK_VALUE(ret_val);
    
	/* ��ȡ����ͷ */
	lx_pt_get_header(&tmp_buf, &tmp_len, &cmd_header);
	/* ѹ����־Ϊ256���ʾ���������ص���Ϣ����ѹ�� (�߰汾�ķ��������Զ�ѹ��) */
	lx_pt_zlib_uncompress(cmd_header._compress_flag, &tmp_buf, &tmp_len, &zlib_buf);
	// TaskInfos : DownloadTaskInfo[]  �������
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &task_num);
	printf("batch_query_task_info: task_num = %u\n", task_num);
for(i = 0; i < task_num; i++)
{
	// Tasklist: task[] ��һ������ĳ���
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&total_len);

	transfer_buf = tmp_buf;
	transfer_len = tmp_len;
	
	lx_parse_get_batch_query_task_info(transfer_buf, transfer_len);
	
	if (total_len != 0) 
    {   
    	tmp_buf += total_len;
		tmp_len -= total_len;
	}

}
	SAFE_DELETE(databuf);
	lx_pt_zlib_uncompress_free(zlib_buf);
	return SUCCESS;
}

_int32 lx_build_req_query_task_id_list(LX_PT_TASK_ID_LS* p_req)
{
	_int32 ret_val = SUCCESS;
	char * tmp_buf = p_req->_action._req_buffer;
	_int32 tmp_len = (_int32)p_req->_action._req_buffer_len;

	EM_LOG_DEBUG("lx_build_req_query_task_id_list" );

	/* �������ͷ */
	ret_val = lx_pt_set_header(&tmp_buf, &tmp_len, &p_req->_req._cmd_header);

	/* ����û���Ϣ */
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)p_req->_req._jump_key_len);
	sd_set_bytes(&tmp_buf, &tmp_len, p_req->_req._jump_key, (_int32)p_req->_req._jump_key_len);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)p_req->_req._userid);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)p_req->_req._vip_level);

	/*����ύʱ���*/
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, p_req->_req._commit_time);
	/* ����ѯ��ʼ����id */
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, p_req->_req._task_id);
	/* ����ѯ��ʼ��ҳ��С */
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, p_req->_req._task_list._max_task_num);
	/* ����Ƿ��ѯ�ѹ��������ʶ */
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, p_req->_req._flag);
	/* task_flag: uint32 */
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, 0);
	/* type: uint32 */
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, 0);
	/********************************************************************************************/
	/* Э��������� */
	p_req->_action._req_data_len = p_req->_action._req_buffer_len-tmp_len;
    p_req->_req._cmd_header._len = p_req->_action._req_data_len - sizeof(p_req->_req._cmd_header);

	/*aes ���� p_req->_action._req_buffer����ǰ���ͷ����Ϣ�г��ȵ��ֶ���lx_aes_encrypt����д*/
	tmp_len = p_req->_action._req_data_len;
	ret_val = lx_aes_encrypt(p_req->_action._req_buffer, (_u32*)&tmp_len);
	if (ret_val!= SUCCESS)
	{
		sd_assert(FALSE);
		return ret_val;
	}

	/* ���ܺ�����������data len !!! */
	p_req->_action._req_data_len = tmp_len;
	
	return SUCCESS;	
}


_int32 lx_parse_get_query_task_id_list_info(char *tmp_buf, _int32 tmp_len, LX_TASK_INFO_EX * task_info, _u64 *commit_time)
{
	_int64 data_len_64 = 0;
    _int32 data_len = 0;
	_int32 status = 0;
	_int8  download_status = 0;
	
	// Status : uint32_t
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&status);  
	if (status != 0) 
    {   
    	return -1;
	}
	/* taskid */
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&task_info->_task_id);
	/* downloadstatus */
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&download_status); 
	task_info->_state = lx_pt_download_status_to_task_state_int(download_status);

	/* commitTime */
    sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)commit_time);

	/* Res_type */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&task_info->_type);
	/* class_value;uint64 */
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, &data_len_64);
	/* database;uint32	 */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &data_len);
	/* delete_time;uint64	 */
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, &data_len_64);
	/* flag;uint32 */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &data_len);

	return 0;
}

_int32 lx_parse_resp_query_task_id_list(LX_PT_TASK_ID_LS* p_resp, _u64 *commit_time, _u64 *last_task_id)
{
    _int32 ret_val = SUCCESS;
	char * tmp_buf = p_resp->_action._resp_buffer;
	_int32 tmp_len = (_int32)p_resp->_action._resp_data_len;
	LX_CMD_HEADER cmd_header = {0};
	_int32 msg_len = 0;
	char* cmd_msg = NULL;
	_int32 total_len = 0;
	_int32 i = 0;
	char * databuf = NULL;
	_u32 readsize = 0;
	LX_TASK_INFO_EX * task_info = NULL;
	char *transfer_buf = NULL;
	_int32 transfer_len = 0;
	char * zlib_buf = NULL;
	EM_LOG_DEBUG("lx_parse_resp_query_task_id_list" );
	
    if( tmp_len >= MAX_POST_RESP_BUFFER_LEN )
	{
		if( p_resp->_action._file_id == 0)
		{
			if(sd_file_exist(p_resp->_action._file_path))
			{
				ret_val = sd_open_ex(p_resp->_action._file_path, O_FS_RDONLY, &p_resp->_action._file_id);
				CHECK_VALUE(ret_val);
				ret_val = sd_malloc(tmp_len, (void **)&databuf);
				CHECK_VALUE(ret_val);
				sd_memset(databuf, 0x00, tmp_len);

				ret_val = sd_pread(p_resp->_action._file_id, databuf, tmp_len, 0, &readsize);
				CHECK_VALUE(ret_val);

				if( readsize != tmp_len )
					return LXE_READ_FILE_WRONG;
				else
					tmp_buf = databuf;
				sd_close_ex(p_resp->_action._file_id);
				p_resp->_action._file_id = 0;
			}
			else
				return LXE_FILE_NOT_EXIST;
		}
		else
		{
			return -1;
		}
	}
	ret_val = lx_aes_decrypt(tmp_buf, (u32*)&tmp_len);
	CHECK_VALUE(ret_val);
    
	/* ��ȡ����ͷ */
	lx_pt_get_header(&tmp_buf, &tmp_len, &cmd_header);
	/* ѹ����־Ϊ256���ʾ���������ص���Ϣ����ѹ�� (�߰汾�ķ��������Զ�ѹ��) */
	lx_pt_zlib_uncompress(cmd_header._compress_flag, &tmp_buf, &tmp_len, &zlib_buf);
	/* ��ѯ�������Ϣ */
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&p_resp->_resp._result);
    
	if (p_resp->_resp._result != 0) 
        return p_resp->_resp._result;
    
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &msg_len);
	if (msg_len > 0)
	{
		cmd_msg = tmp_buf;
		tmp_buf += msg_len;
		tmp_len -= msg_len;
	}
	/* ��ѯ�������б� */
	
	// Tasklist: tasklist[] �������
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&p_resp->_resp._task_num);
	p_resp->_resp._total_task_num = p_resp->_resp._task_num;
	ret_val = lx_get_space(&p_resp->_resp._total_space, &p_resp->_resp._available_space);
	if(ret_val != SUCCESS)
	{
		//lx_get_user_info_req();
	}
	for(i = 0; i < p_resp->_resp._task_num; i++)	
	{
		ret_val = lx_malloc_ex_task(&task_info);
		sd_assert(ret_val == SUCCESS);
		
		// Tasklist: task[] ��һ������ĳ���
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&total_len);
		
		transfer_buf = tmp_buf;
		transfer_len = tmp_len;
		lx_parse_get_query_task_id_list_info(transfer_buf, transfer_len, task_info, commit_time);

		if (total_len != 0) 
	    {   
	    	tmp_buf += total_len;
			tmp_len -= total_len;
		}
		ret_val = lx_add_task_to_map(task_info);
		if(MAP_DUPLICATE_KEY == ret_val)
		{
			SAFE_DELETE(task_info);
		}
		else
			*last_task_id = task_info->_task_id;
	}
	
	
	SAFE_DELETE(databuf);
	lx_pt_zlib_uncompress_free(zlib_buf);
	return SUCCESS;
}


_int32 lx_parse_detect_regex_xml(LX_PT_GET_REGEX* p_resp)
{
	return lx_parse_xml_file((LX_PT *)p_resp, lx_detect_regex_xml_node_proc, lx_detect_regex_xml_node_end_proc, lx_detect_regex_xml_attr_proc, lx_detect_regex_xml_value_proc);
}

_int32 lx_parse_detect_string_xml(LX_PT_GET_DETECT_STRING* p_resp)
{
	return lx_parse_xml_file((LX_PT *)p_resp, lx_detect_string_xml_node_proc, lx_detect_string_xml_node_end_proc, lx_detect_string_xml_attr_proc, lx_detect_string_xml_value_proc);
}

/*--------------------------------------------------------------------------*/
/*                Internal functions				        */
/*--------------------------------------------------------------------------*/

const char * lx_get_aes_key_without_prefix( char * aes_key)
{
	char *key = sd_strstr(aes_key, "key=", 0);
	if(NULL == key||*(key+4) == '\0')//'='����Ĳ���key
	{
		return NULL;
	}
	else
	{
		key+=4;
	}
	return key;
}
/* ���߶����Ʒ���������Ĵ�����ת�������ؿⶨ��Ĵ����� */
_int32 lx_pt_server_errcode_to_em_errcode(_int32 server_errcode)
{
	_int32 errcode = 0;
	switch(server_errcode)
	{
	
		case 10:
		case 28:
			errcode = LXE_LACK_SPACE;
			break;		
		case 8:
			errcode = LXE_PERMISSION_DENIED;
			break;
		default:
			errcode = server_errcode;
			break;
	}
	return errcode;
}

/* ���߶����Ʒ���������Ĵ�����ת�������ؿⶨ��Ĵ����� */
_int32 lx_pt_high_speed_errcode_to_em_errcode(_int32 server_errcode)
{
	_int32 errcode = 0;
	switch(server_errcode)
	{
		case 101:
			errcode = LXE_HS_LACK_FLUX;
			break;		
		case 500:
			errcode = LXE_HS_SERVER_INNER_ERROR;
			break;
		case 501:
			errcode = LXE_HS_FILE_NOT_EXIST;
			break;
		case 502:
			errcode = LXE_HS_PARAMETER_ERROR;
			break;
		case 503:
			errcode = LXE_HS_TASK_NOT_EXIST;
			break;
		default:
			errcode = server_errcode;
			break;
	}
	return errcode;
}

_int32 lx_pt_file_type_to_file_suffix(_int32 type,char suffix[16])
{
	switch(type)
	{
	//��Ƶ��ʽ��׺
		case LPFT_RMVB:
			sd_strncpy(suffix, "rmvb", 15);
			break;
		case LPFT_RM:
			sd_strncpy(suffix, "rm", 15);
			break;
		case LPFT_AVI:
			sd_strncpy(suffix, "avi", 15);
			break;
		case LPFT_MKV:
			sd_strncpy(suffix, "mkv", 15);
			break;
		case LPFT_WMV:
			sd_strncpy(suffix, "wmv", 15);
			break;
		case LPFT_MP4:
			sd_strncpy(suffix, "mp4", 15);
			break;
		case LPFT_3GP:
			sd_strncpy(suffix, "3gp", 15);
			break;
		case LPFT_M4V:
			sd_strncpy(suffix, "m4v", 15);
			break;
		case LPFT_FLV:
			sd_strncpy(suffix, "flv", 15);
			break;
		case LPFT_TS:
			sd_strncpy(suffix, "ts", 15);
			break;	
		case LPFT_XV:
			sd_strncpy(suffix, "xv", 15);
			break;
		case LPFT_MOV:
			sd_strncpy(suffix, "mov", 15);
			break;
		case LPFT_MPG:
			sd_strncpy(suffix, "mpg", 15);
			break;
		case LPFT_MPEG:
			sd_strncpy(suffix, "mpeg", 15);
			break;
		case LPFT_ASF:
			sd_strncpy(suffix, "asf", 15);
			break;
		case LPFT_SWF:
			sd_strncpy(suffix, "swf", 15);
			break;
	//��Ƶ��ʽ��׺
		case LPFT_MP3:
			sd_strncpy(suffix, "mp3", 15);
			break;
		case LPFT_WMA:
			sd_strncpy(suffix, "wma", 15);
			break;
		case LPFT_WAV:
			sd_strncpy(suffix, "wav", 15);
			break;
	// �ı���ʽ��׺
		case LPFT_JPG:
			sd_strncpy(suffix, "jpg", 15);
			break;
		case LPFT_JPEG:
			sd_strncpy(suffix, "jpeg", 15);
			break;
		case LPFT_GIF:
			sd_strncpy(suffix, "gif", 15);
			break;
		case LPFT_PNG:
			sd_strncpy(suffix, "png", 15);
			break;
		case LPFT_BMP:
			sd_strncpy(suffix, "bmp", 15);
			break;
	//ѹ������ʽ��׺
		case LPFT_RAR:
			sd_strncpy(suffix, "rar", 15);
			break;
		case LPFT_ZIP:
			sd_strncpy(suffix, "zip", 15);
			break;
		case LPFT_ISO:
			sd_strncpy(suffix, "iso", 15);
			break;	
	//�ı���ʽ��׺
		case LPFT_TXT:
			sd_strncpy(suffix, "txt", 15);
			break;
		case LPFT_PPT:
			sd_strncpy(suffix, "ppt", 15);
			break;
		case LPFT_PPTX:
			sd_strncpy(suffix, "pptx", 15);
			break;
		case LPFT_XLS:
			sd_strncpy(suffix, "xls", 15);
			break;	
		case LPFT_XLSX:
			sd_strncpy(suffix, "xlsx", 15);
			break;
		case LPFT_PDF:
			sd_strncpy(suffix, "pdf", 15);
			break;
		case LPFT_DOC:
			sd_strncpy(suffix, "doc", 15);
			break;
		case LPFT_DOCX:
			sd_strncpy(suffix, "docx", 15);
			break;
		case LPFT_CPP:
			sd_strncpy(suffix, "cpp", 15);
			break;
		case LPFT_C:
			sd_strncpy(suffix, "c", 15);
			break;
		case LPFT_H:
			sd_strncpy(suffix, "h", 15);
			break;
	//Ӧ�ó����ʽ��׺
		case LPFT_EXE:
			sd_strncpy(suffix, "exe", 15);
			break;
		case LPFT_MSI:
			sd_strncpy(suffix, "msi", 15);
			break;
		case LPFT_APK:
			sd_strncpy(suffix, "apk", 15);
			break;
	//�����ļ���׺
		case LPFT_TORRENT:
			sd_strncpy(suffix, "torrent", 15);
			break;
	// �����׺
		case LPFT_OTHERS:
		default:
			break;
	}
	return SUCCESS;
}
LX_TASK_STATE lx_pt_download_status_to_task_state_int(_int32 download_status)
{
	LX_TASK_STATE state;
	switch(download_status)
	{
		case 0:
			state = LXS_WAITTING;
			break;
		case 1:
			state = LXS_RUNNING;
			break;
		case 2:
			state = LXS_SUCCESS;
			break;
		case 3:
			state = LXS_FAILED;
			break;
		case 5:
			state = LXS_PAUSED;
			break;
		default:
			sd_assert(FALSE);
			state = LXS_FAILED;
			break;
	}
	return state;
}

LX_TASK_STATE lx_pt_download_status_to_task_state(char * download_status)
{
	_int32 status = sd_atoi(download_status);
	LX_TASK_STATE state;
    
	switch(status)
	{
		case LPFS_TASK_START:
			state = LXS_WAITTING;
			break;
		case LPFS_TASK_PROCESS:
			state = LXS_RUNNING;
			break;
		case LPFS_TASK_COMPLETE:
			state = LXS_SUCCESS;
			break;
		case LPFS_TASK_FAILED:
			state = LXS_FAILED;
			break;
		case LPFS_TASK_SUSPEND:
			state = LXS_PAUSED;
			break;
		case LPFS_TASK_BT_FLAG:
		default:
			sd_assert(FALSE);
			state = LXS_FAILED;
			break;
	}
	return state;
}
_int32 lx_build_req_zip_and_aes(LX_PT * p_req)
{
	_int32 ret_val=0;
	_u32 des_len = 0;
    
	EM_LOG_DEBUG("lx_build_req_zip_and_aes" );
    
    
#ifdef ENABLE_LX_XML_ZIP
	if(TRUE==p_req->_is_compress)
	{
		//ѹ��
		des_len = p_req->_req_buffer_len;
		ret_val=sd_zip_data((const _u8*)p_req->_req_buffer, p_req->_req_data_len,( _u8*)p_req->_req_buffer, &des_len);
		CHECK_VALUE(ret_val);
		p_req->_req_data_len = des_len;
	}
#endif/* ENABLE_LX_XML_ZIP */
	
#ifdef ENABLE_LX_XML_AES
	if(TRUE==p_req->_is_aes)
	{
		des_len = p_req->_req_buffer_len;
		ret_val=sd_aes_encrypt( lx_get_aes_key_without_prefix(p_req->_aes_key), (const _u8*)p_req->_req_buffer, p_req->_req_data_len, ( _u8*)p_req->_req_buffer, &des_len);
		CHECK_VALUE(ret_val);
		
		p_req->_req_data_len = des_len;
	}
#endif/*ENABLE_LX_XML_AES*/
    
	return ret_val;
}

_int32 lx_parse_xml_file(LX_PT * p_resp, xml_node_proc node_proc, xml_node_end_proc node_end_proc, xml_attr_proc attr_proc, xml_value_proc value_proc)
{
	_int32 ret_val=0;
	//_u32 des_len = 0;
	char * file_name = p_resp->_file_path;
	
	EM_LOG_DEBUG("lx_parse_xml_file" );
    
    
#ifdef ENABLE_LX_XML_AES
	if(TRUE==p_resp->_is_aes)
	{
		/*����*/
		ret_val=sd_aes_decrypt_file(lx_get_aes_key_without_prefix(p_resp->_aes_key),file_name );
		CHECK_VALUE(ret_val);
	}
#endif/*ENABLE_LX_XML_AES*/
    
#ifdef ENABLE_LX_XML_ZIP
	if(TRUE==p_resp->_is_compress)
	{
		//��ѹ
		ret_val=sd_unzip_file(file_name);
		CHECK_VALUE(ret_val);
	}
#endif/* ENABLE_LX_XML_ZIP */
    
#if 0//def _DEBUG
    {
        _u32 file_id=0, size_read=0, size_printf=0;
        _u64 file_size=0;
        char read_buffer[MAX_POST_RESP_BUFFER_LEN]={0};
        ret_val = sd_open_ex(file_name, O_FS_RDONLY, &file_id);
        if(ret_val == SUCCESS)
        {
            sd_filesize(file_id, &file_size);
            printf("\n task list response xml [%llu]:\n ", file_size);
            while(file_size>size_printf)
            {
                sd_memset(read_buffer, 0, MAX_POST_RESP_BUFFER_LEN);
                sd_read(file_id, read_buffer, MAX_POST_RESP_BUFFER_LEN-1, &size_read);
                URGENT_TO_FILE("%s",read_buffer);
                size_printf+=size_read;
            }
            sd_close_ex( file_id);
        }
        ret_val = SUCCESS;
    }
#endif/*_DEBUG*/
    
	node_count=0;
	XML_SERVICE xml_service = NULL;
	xml_service = create_xml_service();
	read_xml_file( xml_service, 			//xml������
		file_name, 					//xml�ļ���
		sd_strlen(file_name), 		//xml�ļ�������
		p_resp,			//���ڴ洢ֵ��������ַ,user_data
		node_proc,		//�ڵ�ͷ��������
		node_end_proc,	//�ڵ�β��������
		attr_proc,			//�ڵ����Դ�������
		value_proc);		//�ڵ�ֵ��������
	destroy_xml_service(xml_service);
    
	return ret_val;
	
}

/*******************************************************************************************************************************************/

_int32 lx_task_list_xml_node_proc(void *user_data, void *parent_node, _int32 parent_type, _int32 depth, char const *node_name, void **node, _int32 *node_type)
{
	_int32 ret_val = SUCCESS;
	LX_PT_TASK_LS* p_resp=(LX_PT_TASK_LS *)user_data;
	
	if(p_resp->_action._error_code!=0)
		return SUCCESS;
	
	switch(depth)
	{
		case 0:
			if(!sd_strcmp(node_name,"LixianProtocol"))
				break;
			else
			{
				p_resp->_action._error_code=LXE_WRONG_RESPONE;
				return SUCCESS;
			}
		case 1:
			if(!sd_strcmp(node_name,"Command"))
				*node_type=LXX_COMMAND;
			break;
		case 2:
			if(!sd_strcmp(node_name,"userinfo"))
				*node_type=LXX_USERINFO;
			else if(!sd_strcmp(node_name,"tasklist"))
				*node_type=LXX_TASK_LIST;
			break;
		case 3:
			if(!sd_strcmp(node_name,"task"))
			{
				*node_type=LXX_TASK;
				sd_assert(gp_task_info==NULL);
				//ret_val = sd_malloc(sizeof(LX_TASK_INFO_EX), (void**)&gp_task_info);
				ret_val = lx_malloc_ex_task(&gp_task_info);
				sd_assert(ret_val == SUCCESS);
				if(ret_val!=SUCCESS)
				{
					p_resp->_action._error_code = ret_val;
					gp_task_info = NULL;
				}
			}
			break;
		default:
			break;
	}
	return SUCCESS;
}
_int32 lx_task_list_xml_node_end_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *node_name)
{
	_int32 ret_val = SUCCESS;
	//LX_PT_TASK_LS * p_resp=(LX_PT_TASK_LS *)user_data;
	
	//if(p_resp->_action._error_code!=0)
	//	return SUCCESS;
	
	if(node_type==LXX_COMMAND)
	{
		//if(p_resp->_action._resp_status!=0)
		//	p_resp->_action._error_code=LXE_STATUS_NOT_FOUND;
	}
	else if(node_type==LXX_TASK)
	{
	#ifdef KANKAN_PROJ
		if(gp_task_info->_state!=LXS_SUCCESS)
		{
			/* ipad������Ŀֻ��Ҫ����������ϵ���Ƶ�ļ��������ڷ������������״̬Ϊ����ɣ�
			���������Ѿ�������ϵ���Ƶ�ļ���BT����Ҳ����,�ͻ�������ֱ��������ǿ��תΪ�ɹ�״̬!   ---Add by zyq @ 20120713 */
			//SAFE_DELETE(gp_task_info);
			//p_resp->_resp._task_num--;
			//return SUCCESS;
			gp_task_info->_state = LXS_SUCCESS;
		}
	#endif
		ret_val = lx_add_task_to_map(gp_task_info);
		if(ret_val!=SUCCESS)
		{
			if(MAP_DUPLICATE_KEY==ret_val)
			{
				/* �Ѵ��ڣ�����״̬ */
				LX_TASK_INFO_EX * p_task_in_map = lx_get_task_from_map(gp_task_info->_task_id);
				sd_assert(p_task_in_map!=NULL);
				if(p_task_in_map->_state<LXS_SUCCESS)
				{
					/* ֻ����LX_TASK_INFO ���֣�origin_url ��_bt_sub_files ���ֲ����� */
					sd_memcpy(p_task_in_map,gp_task_info,sizeof(LX_TASK_INFO));
				}
			}
			SAFE_DELETE(gp_task_info);
		}
		gp_task_info = NULL;
	}
	else if(node_type==LXX_TASK_LIST)
	{
		//if(p_resp->_action._error_code!=0)
		//	SAFE_DELETE(p_resp->_resp._task_array);
	}
    
	return SUCCESS;
}
_int32 lx_task_list_xml_attr_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *attr_name, char const *attr_value)
{
	_int32 ret_val = SUCCESS;
	LX_PT_TASK_LS* p_resp=(LX_PT_TASK_LS*)user_data;
	LX_TASK_INFO_EX * p_info =  gp_task_info; 
	char dec_str[MAX_FILE_NAME_BUFFER_LEN] = {0};
	char utf8[MAX_FILE_NAME_BUFFER_LEN] = {0};
	_u32 utf8_len = MAX_FILE_NAME_BUFFER_LEN;
	/* ���ڹ����ļ����ضϵı���*/
	char task_name[MAX_FILE_NAME_BUFFER_LEN] = {0};
	char* p_suffix = NULL;
	_u32 suffix_len = 0, name_len = 0, attr_len = 0; 
	_u64 left_live_time_in_second = 0;
    char tmp_buf[2048] = {0};
	char utf8url[MAX_URL_LEN] = {0};
	_u32 utf8urllen = MAX_URL_LEN;
	if(p_resp->_action._error_code!=0)
		return SUCCESS;
    
	switch(node_type)
	{
        case LXX_COMMAND:
            if(!sd_strcmp(attr_name,"id"))
            {
                if(sd_strcmp(attr_value,"lxtasklist_resp"))//&&sd_strcmp(attr_value, "unknown_resp"))
                {
                    p_resp->_action._error_code=LXE_WRONG_RESPONE;
                    break;
                }
            }
            else if(!sd_strcmp(attr_name,"result"))
            {
                p_resp->_action._resp_status=sd_atoi(attr_value);
                if(p_resp->_action._resp_status!=0)
                {
                    p_resp->_action._error_code=p_resp->_action._resp_status;
                    break;
                }
            }
            else if(!sd_strcmp(attr_name,"message"))
            {
                ;
            }
            break;
        case LXX_USERINFO:
            /*
             if(!sd_strcmp(attr_name,"id"))
             {
             }
             else if(!sd_strcmp(attr_name,"vip_level"))
             {
             }
             else if(!sd_strcmp(attr_name,"from"))
             {
             }
             */
            if(!sd_strcmp(attr_name,"max_store"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                ret_val = sd_str_to_u64(attr_value, sd_strlen(attr_value), &p_resp->_resp._total_space);
                sd_assert(ret_val == SUCCESS);
            }
            else if(!sd_strcmp(attr_name,"available_space"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                ret_val = sd_str_to_u64(attr_value, sd_strlen(attr_value), &p_resp->_resp._available_space);
                sd_assert(ret_val == SUCCESS);
            }
            break;
        case LXX_TASK_LIST:
            if(!sd_strcmp(attr_name,"list_num"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                p_resp->_resp._task_num = sd_atoi(attr_value);
            }
            else if(!sd_strcmp(attr_name,"total_num"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                p_resp->_resp._total_task_num = sd_atoi(attr_value);
            }
            break;
        case LXX_TASK:
            if(!sd_strcmp(attr_name,"id"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                ret_val = sd_str_to_u64(attr_value, sd_strlen(attr_value), &p_info->_task_id);
                sd_assert(ret_val == SUCCESS);
            }
            else if(!sd_strcmp(attr_name,"restype"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                p_info->_type = sd_atoi(attr_value);
            }
            else if(!sd_strcmp(attr_name,"filesize"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                ret_val = sd_str_to_u64(attr_value, sd_strlen(attr_value), &p_info->_size);
                sd_assert(ret_val == SUCCESS);
            }
            else if(!sd_strcmp(attr_name,"filetype"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                ret_val = lx_pt_file_type_to_file_suffix(sd_atoi(attr_value),p_info->_file_suffix);
                sd_assert(ret_val == SUCCESS);
            }
            else if(!sd_strcmp(attr_name,"cid"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    /*if(p_info->_type != LXT_BT_ALL)
                     {
                     p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                     }*/
                    break;
                }
                ret_val = sd_string_to_cid((char *)attr_value,p_info->_cid);
                sd_assert(ret_val == SUCCESS);
            }
            else if(!sd_strcmp(attr_name,"gcid"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    /*if(p_info->_type != LXT_BT_ALL)
                     {
                     p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                     }*/
                    break;
                }
                ret_val = sd_string_to_cid((char *)attr_value,p_info->_gcid);
                //sd_assert(ret_val == SUCCESS); /* û������ɵĿ���gcidΪ0 */
            }
            else if(!sd_strcmp(attr_name,"taskname"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    //p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                    break;
                }

		EM_LOG_DEBUG( "lx_task_list_xml_attr_proc :taskname  attr_value= %s", attr_value);
                ret_val = em_url_object_decode_ex(attr_value, dec_str, MAX_FILE_NAME_BUFFER_LEN); 
                if((ret_val == -1) && ((attr_len = sd_strlen(attr_value))> MAX_FILE_NAME_BUFFER_LEN))
                {
                    p_suffix = sd_strrchr((char*)attr_value, '.');
                    if(p_suffix != NULL && (suffix_len = sd_strlen(p_suffix))<10 )
                    {
                        /* �к�׺���ҺϷ����ض�ʱ�����ú�׺ */
                        name_len = sd_get_sub_utf8_str_len((char*)attr_value, MAX_FILE_NAME_BUFFER_LEN-1-suffix_len);
                        sd_assert(name_len + suffix_len<MAX_FILE_NAME_BUFFER_LEN);	
                        sd_strncpy(task_name, attr_value, name_len);
                        sd_strncpy(task_name+name_len, p_suffix, suffix_len);
                        ret_val = em_url_object_decode_ex(task_name, dec_str, MAX_FILE_NAME_BUFFER_LEN); 
                    }
                    else
                    {
                        /* �޺�׺�����׺�����Ϸ��� ֱ�ӽض� */
                        name_len = sd_get_sub_utf8_str_len((char*)attr_value, MAX_FILE_NAME_BUFFER_LEN-1);
                        sd_strncpy(task_name, attr_value, name_len);
                        ret_val = em_url_object_decode_ex(task_name, dec_str, MAX_FILE_NAME_BUFFER_LEN); 
                    }
                }
                sd_assert(sd_strlen(dec_str)<MAX_FILE_NAME_BUFFER_LEN);					
                sd_strncpy(p_info->_name,dec_str, MAX_FILE_NAME_BUFFER_LEN-1);
                
				/* ȥ���ļ���ĩβ�Ŀո� */
                sd_trim_postfix_lws( p_info->_name );
		
                /* check is Bad file name ! */
                sd_get_valid_name(p_info->_name,NULL);
				// �ļ���ǿ��ת����utf-8��ʽ
				ret_val = sd_any_format_to_utf8(p_info->_name, sd_strlen(p_info->_name), utf8, &utf8_len);
				if(ret_val == SUCCESS)
				{
					sd_memset(p_info->_name, 0x00, sizeof(p_info->_name));
					sd_strncpy(p_info->_name, utf8, utf8_len);
				}
 		EM_LOG_DEBUG( "lx_task_list_xml_attr_proc :taskname  utf8= %s", p_info->_name);
           }
            else if(!sd_strcmp(attr_name,"download_status"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                p_info->_state = lx_pt_download_status_to_task_state((char*)attr_value);
            }
            else if(!sd_strcmp(attr_name,"progress"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                p_info->_progress = sd_atoi(attr_value);
            }
            else if(!sd_strcmp(attr_name,"url"))
            {
                
                /*  ���ػ���ʱ������ԭʼ����url */
                if(sd_strlen(attr_value)==0)
                {
                    //p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                //sd_assert(sd_strlen(attr_value)<MAX_URL_LEN);
                if(sd_strlen(attr_value) >= MAX_URL_LEN)
                {
                    //����url������
                    break;
                }
 		EM_LOG_DEBUG( "lx_task_list_xml_attr_proc :url  attr_name= %s",attr_name);
				// ԭʼurlͳһת����utf-8��ʽ
				ret_val = sd_any_format_to_utf8(attr_value, sd_strlen(attr_value), utf8url, &utf8urllen);
                ret_val = em_url_object_decode_ex(utf8url, p_info->_origin_url, MAX_URL_LEN-1); 
                sd_assert(ret_val != -1);
                if(ret_val != -1)
                {
                    p_info->_origin_url_len = sd_strlen(p_info->_origin_url);
                    ret_val = em_get_url_hash_value( p_info->_origin_url,p_info->_origin_url_len, &p_info->_origin_url_hash);
                    sd_assert(ret_val == SUCCESS);
                }
 		EM_LOG_DEBUG( "lx_task_list_xml_attr_proc :_origin_url  utf8= %s",p_info->_origin_url);
            }
            else if(!sd_strcmp(attr_name,"lixian_url"))
            {
                /*  ��������ʱ��������������url */
                if(sd_strlen(attr_value)==0)
                {	
                    /*if(p_info->_type != LXT_BT_ALL)
                     {
                     p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                     }*/
                    break;
                }
				if(sd_strlen(attr_value) < MAX_URL_LEN)
				{
	                //sd_assert(sd_strlen(attr_value)<MAX_URL_LEN);
	                //sd_strncpy(p_info->_url,attr_value, MAX_URL_LEN-1);
	                ret_val = em_url_object_decode_ex(attr_value, p_info->_url, MAX_URL_LEN-1); 
	                sd_assert(ret_val != -1);
	                ret_val = lx_handle_pubnet_url(p_info->_url);
	                sd_assert(ret_val == SUCCESS);
				}
				else
				{
	                ret_val = em_url_object_decode_ex(attr_value, tmp_buf, MAX_URL_LEN-1); 
	                sd_assert(ret_val != -1);
	                ret_val = lx_handle_pubnet_url(tmp_buf);
	                sd_assert(ret_val == SUCCESS);
					sd_assert(sd_strlen(tmp_buf)<MAX_URL_LEN);
					sd_strncpy(p_info->_url,tmp_buf, MAX_URL_LEN-1);
				}
 		EM_LOG_DEBUG( "lx_task_list_xml_attr_proc :lixian_url= %s",p_info->_url);
            }
            else if(!sd_strcmp(attr_name,"vod"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                if(*attr_value == '1')
                {
                    p_info->_vod = TRUE;
                }
            }
            else if(!sd_strcmp(attr_name,"flag_killed_in_a_second"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                    break;
                }
            }
            else if(!sd_strcmp(attr_name,"finish_num"))
            {
                if(sd_strlen(attr_value)==0 && p_info->_type == LXT_BT_ALL)
                {
                    p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                p_info->_finished_file_num= sd_atoi(attr_value);
            }
            else if(!sd_strcmp(attr_name,"total_num"))
            {
                if(sd_strlen(attr_value)==0 && p_info->_type == LXT_BT_ALL)
                {
                    p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                p_info->_sub_file_num = sd_atoi(attr_value);
            }
            else if(!sd_strcmp(attr_name,"left_live_time"))
            {
                if(sd_strlen(attr_value)==0 )
                {
                    //p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                ret_val = sd_str_to_u64(attr_value, sd_strlen(attr_value), &left_live_time_in_second);
                sd_assert(ret_val == SUCCESS);
                if(left_live_time_in_second>0 && left_live_time_in_second<=(24*60*60))
                {
                    /* ����һ�� */
                    p_info->_left_live_time = 1;
                }
                else
                {
                    left_live_time_in_second = left_live_time_in_second/(24*60*60);
                    p_info->_left_live_time = (_u32)left_live_time_in_second;
                }
            }
            else if(!sd_strcmp(attr_name,"cookie"))
            {
                if(sd_strlen(attr_value)==0)
                {	
                    break;
                }
                /*  lixian url �У�������� dt=16 17 ���� userid=xxxx��û��dt�������� gdriveid=xxxx  */
                sd_assert(sd_strlen(attr_value)<MAX_URL_LEN);
                lx_set_download_cookie(attr_value);
                sd_snprintf(p_info->_cookie,MAX_URL_LEN-1, "Cookie: %s",attr_value);
            }
            break;
        default:	
            ;
	}
    
	return SUCCESS;
}
_int32 lx_task_list_xml_value_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *value)
{
	return SUCCESS;
}

/*******************************************************************************************************************************************/

_int32 lx_bt_task_list_xml_node_proc(void *user_data, void *parent_node, _int32 parent_type, _int32 depth, char const *node_name, void **node, _int32 *node_type)
{
	_int32 ret_val = SUCCESS;
	LX_PT_BT_LS* p_resp=(LX_PT_BT_LS *)user_data;
	
	if(p_resp->_action._error_code!=0)
		return SUCCESS;
	
	switch(depth)
	{
		case 0:
			if(!sd_strcmp(node_name,"LixianProtocol"))
				break;
			else
			{
				p_resp->_action._error_code=LXE_WRONG_RESPONE;
				return SUCCESS;
			}
		case 1:
			if(!sd_strcmp(node_name,"Command"))
				*node_type=LXX_COMMAND;
			break;
		case 2:
			if(!sd_strcmp(node_name,"tasklist"))
			{
				LX_TASK_INFO_EX * p_task_in_map = lx_get_task_from_map(p_resp->_req._task_id);
				sd_assert(p_task_in_map!=NULL);
				//sd_assert(gp_bt_sub_file_map==NULL);
				gp_bt_sub_file_map = &p_task_in_map->_bt_sub_files;
				
				*node_type=LXX_TASK_LIST;
			}
			break;
		case 3:
			if(!sd_strcmp(node_name,"task"))
			{
				*node_type=LXX_TASK;
				sd_assert(gp_file_info==NULL);
				ret_val = sd_malloc(sizeof(LX_FILE_INFO), (void**)&gp_file_info);
				sd_assert(ret_val == SUCCESS);
				if(ret_val!=SUCCESS)
				{
					p_resp->_action._error_code = ret_val;
					gp_file_info = NULL;
				}
			}
			break;
		default:
			break;
	}
	return SUCCESS;
}
_int32 lx_bt_task_list_xml_node_end_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *node_name)
{
	_int32 ret_val = SUCCESS;
	LX_PT_BT_LS * p_resp=(LX_PT_BT_LS *)user_data;
	
	//if(p_resp->_action._error_code!=0)
	//	return SUCCESS;
	
	if(node_type==LXX_COMMAND)
	{
		//if(p_resp->_action._resp_status!=0)
		//	p_resp->_action._error_code=LXE_STATUS_NOT_FOUND;
	}
	else if(node_type==LXX_TASK)
	{
		ret_val = lx_add_file_to_map(gp_bt_sub_file_map,gp_file_info);
		if(ret_val!=SUCCESS)
		{
			if(MAP_DUPLICATE_KEY==ret_val)
			{
				/* �Ѵ��ڣ�����״̬ */
				LX_FILE_INFO * p_file_in_map = lx_get_file_from_map(gp_bt_sub_file_map,gp_file_info->_file_id);
				sd_assert(p_file_in_map!=NULL);
				p_file_in_map->_state = gp_file_info->_state;
				p_file_in_map->_progress = gp_file_info->_progress;
			}
			SAFE_DELETE(gp_file_info);
		}
		
		if(node_count<p_resp->_resp._file_num)
		{
			node_count++;
		}
		gp_file_info = NULL;
	}
	else if(node_type==LXX_TASK_LIST)
	{
		//if(p_resp->_action._error_code!=0)
		//	SAFE_DELETE(p_resp->_resp._file_array);
		gp_bt_sub_file_map = NULL;
	}
    
	return SUCCESS;
}
_int32 lx_bt_task_list_xml_attr_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *attr_name, char const *attr_value)
{
	_int32 ret_val = SUCCESS;
	LX_PT_BT_LS* p_resp=(LX_PT_BT_LS*)user_data;
	LX_FILE_INFO * p_info =  gp_file_info;//p_resp->_resp._file_array + node_count; 
	char utf8[MAX_FILE_NAME_BUFFER_LEN] = {0};
	_u32 utf8_len = MAX_FILE_NAME_BUFFER_LEN;
	char dec_str[MAX_FILE_NAME_BUFFER_LEN]={0};
	_u32 attr_len = 0,suffix_len= 0 ,name_len = 0;
	char *p_suffix = NULL,*task_name = NULL,*p_file_index = NULL;
    
	if(p_resp->_action._error_code!=0)
		return SUCCESS;
    
	switch(node_type)
	{
        case LXX_COMMAND:
            if(!sd_strcmp(attr_name,"id"))
            {
                if(sd_strcmp(attr_value,"getbtlist_resp"))//&&sd_strcmp(attr_value, "unknown_resp"))
                {
                    p_resp->_action._error_code=LXE_WRONG_RESPONE;
                    break;
                }
            }
            else if(!sd_strcmp(attr_name,"result"))
            {
                p_resp->_action._resp_status=sd_atoi(attr_value);
                if(p_resp->_action._resp_status!=0)
                {
                    p_resp->_action._error_code=p_resp->_action._resp_status;
                    break;
                }
            }
            else if(!sd_strcmp(attr_name,"message"))
            {
                ;
            }
            break;
        case LXX_TASK_LIST:
            if(!sd_strcmp(attr_name,"list_num"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                p_resp->_resp._file_num = sd_atoi(attr_value);
                /*
                 if(p_resp->_resp._file_num!=0)
                 {
                 ret_val = sd_malloc(p_resp->_resp._file_num*sizeof(LX_FILE_INFO), (void**)&p_resp->_resp._file_array);
                 if(ret_val!=SUCCESS)
                 {
                 p_resp->_action._error_code = ret_val;
                 break;
                 }
                 sd_memset(p_resp->_resp._file_array,0x00,p_resp->_resp._file_num*sizeof(LX_FILE_INFO));
                 p_info = p_resp->_resp._file_array;//��p_info�����һ���������ӿɶ��ԣ�Ҳ�����״�7.28
                 }
                 */
            }
            else if(!sd_strcmp(attr_name,"total_num"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                p_resp->_resp._total_file_num = sd_atoi(attr_value);
            }
            break;
        case LXX_TASK:
            if(!sd_strcmp(attr_name,"id"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                ret_val = sd_str_to_u64(attr_value, sd_strlen(attr_value), &p_info->_file_id);
                sd_assert(ret_val == SUCCESS);
            }
            else if(!sd_strcmp(attr_name,"filesize"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                ret_val = sd_str_to_u64(attr_value, sd_strlen(attr_value), &p_info->_size);
                sd_assert(ret_val == SUCCESS);
            }
            else if(!sd_strcmp(attr_name,"filetype"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                ret_val = lx_pt_file_type_to_file_suffix(sd_atoi(attr_value),p_info->_file_suffix);
                sd_assert(ret_val == SUCCESS);
            }
            else if(!sd_strcmp(attr_name,"cid"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    //p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                ret_val = sd_string_to_cid((char *)attr_value,p_info->_cid);
                sd_assert(ret_val == SUCCESS);
            }
            else if(!sd_strcmp(attr_name,"gcid"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    //p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                ret_val = sd_string_to_cid((char *)attr_value,p_info->_gcid);
                sd_assert(ret_val == SUCCESS);
            }
            else if(!sd_strcmp(attr_name,"taskname"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    //p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                //sd_assert(sd_strlen(attr_value)<MAX_FILE_NAME_BUFFER_LEN);
                sd_strncpy(p_info->_name,attr_value, MAX_FILE_NAME_BUFFER_LEN-1);
                ///////////////////////////////////////
                ret_val = em_url_object_decode_ex(attr_value, dec_str, MAX_FILE_NAME_BUFFER_LEN); 
                if((ret_val == -1) && ((attr_len = sd_strlen(attr_value))> MAX_FILE_NAME_BUFFER_LEN))
                {
                    p_suffix = sd_strrchr((char*)attr_value, '.');
                    task_name = p_info->_name;
                    if(p_suffix != NULL && (suffix_len = sd_strlen(p_suffix))<10 )
                    {
                        /* �к�׺���ҺϷ����ض�ʱ�����ú�׺ */
                        name_len = sd_get_sub_utf8_str_len((char*)attr_value, MAX_FILE_NAME_BUFFER_LEN-1-suffix_len);
                        sd_assert(name_len + suffix_len<MAX_FILE_NAME_BUFFER_LEN);	
                        sd_strncpy(task_name, attr_value, name_len);
                        sd_strncpy(task_name+name_len, p_suffix, suffix_len);
                        ret_val = em_url_object_decode_ex(task_name, dec_str, MAX_FILE_NAME_BUFFER_LEN); 
                    }
                    else
                    {
                        /* �޺�׺�����׺�����Ϸ��� ֱ�ӽض� */
                        name_len = sd_get_sub_utf8_str_len((char*)attr_value, MAX_FILE_NAME_BUFFER_LEN-1);
                        sd_strncpy(task_name, attr_value, name_len);
                        ret_val = em_url_object_decode_ex(task_name, dec_str, MAX_FILE_NAME_BUFFER_LEN); 
                    }
                }
                sd_assert(sd_strlen(dec_str)<MAX_FILE_NAME_BUFFER_LEN);					
                sd_strncpy(p_info->_name,dec_str, MAX_FILE_NAME_BUFFER_LEN-1);
                /* check is Bad file name ! */
                sd_get_valid_name(p_info->_name,NULL);
				// �ļ���ǿ��ת����utf-8��ʽ
				/*ret_val = sd_any_format_to_utf8(p_info->_name, sd_strlen(p_info->_name), utf8, &utf8_len);
				if(ret_val == SUCCESS)
				{
					sd_memset(p_info->_name, 0x00, sizeof(p_info->_name));
					sd_strncpy(p_info->_name, utf8, utf8_len);
				}
				*/
            }
            else if(!sd_strcmp(attr_name,"download_status"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                p_info->_state = lx_pt_download_status_to_task_state((char*)attr_value);
            }
            else if(!sd_strcmp(attr_name,"progress"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                p_info->_progress = sd_atoi(attr_value);
            }
            else if(!sd_strcmp(attr_name,"url"))
            {
                
                /*  ��ȡ���ļ����*/
                 if(sd_strlen(attr_value)==0)
                 {
                 	//p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                	 sd_assert(FALSE);
                	 break;
                 }
                 sd_assert(sd_strlen(attr_value)<MAX_URL_LEN);
                 sd_assert(sd_strlen(attr_value)>CID_SIZE*2);
				p_file_index = sd_strrchr(attr_value, 0x2F);
				if(p_file_index!=NULL && p_file_index-attr_value>CID_SIZE*2)
				{
					p_file_index++;
					p_info->_file_index = sd_atoi(p_file_index);
				}
            }
            else if(!sd_strcmp(attr_name,"lixian_url"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    //p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                sd_assert(sd_strlen(attr_value)<MAX_URL_LEN);
                //sd_strncpy(p_info->_url,attr_value, MAX_URL_LEN-1);
                ret_val = em_url_object_decode_ex(attr_value, p_info->_url, MAX_URL_LEN-1); 
                sd_assert(ret_val != -1);
                ret_val = lx_handle_pubnet_url(p_info->_url);
                sd_assert(ret_val == SUCCESS);
            }
            else if(!sd_strcmp(attr_name,"vod"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                if(*attr_value == '1')
                {
                    p_info->_vod = TRUE;
                }
            }
            /*
             else if(!sd_strcmp(attr_name,"left_live_time"))
             {
             if(sd_strlen(attr_value)==0 )
             {
             //p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
             break;
             }
             }
             */
            else if(!sd_strcmp(attr_name,"cookie"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    //p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                /*  lixian url �У�������� dt=16 17 ���� userid=xxxx��û��dt�������� gdriveid=xxxx  */
                sd_assert(sd_strlen(attr_value)<MAX_URL_LEN);
                lx_set_download_cookie(attr_value);
                sd_snprintf(p_info->_cookie,MAX_URL_LEN-1, "Cookie: %s",attr_value);
            }
            break;
        default:	
            ;
	}
    
	return SUCCESS;
}
_int32 lx_bt_task_list_xml_value_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *value)
{
	return SUCCESS;
}

/*******************************************************************************************************************************************/

_int32 lx_screensoht_xml_node_proc(void *user_data, void *parent_node, _int32 parent_type, _int32 depth, char const *node_name, void **node, _int32 *node_type)
{
	LX_PT_SS* p_resp=(LX_PT_SS *)user_data;
	
	if(p_resp->_action._error_code!=0)
		return SUCCESS;
	
	switch(depth)
	{
		case 0:
			if(!sd_strcmp(node_name,"LixianProtocol"))
				break;
			else
			{
				p_resp->_action._error_code=LXE_WRONG_RESPONE;
				return SUCCESS;
			}
		case 1:
			if(!sd_strcmp(node_name,"Command"))
				*node_type=LXX_COMMAND;
			break;
		case 2:
			if(!sd_strcmp(node_name,"GcidInfoList"))
				*node_type=LXX_GCID_LIST;
			break;
		case 3:
			if(!sd_strcmp(node_name,"Gcid"))
				*node_type=LXX_GCID;
			break;
		default:
			break;
	}
	return SUCCESS;
}
_int32 lx_screensoht_xml_node_end_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *node_name)
{
	LX_PT_SS * p_resp=(LX_PT_SS *)user_data;
	
	if(p_resp->_action._error_code!=0)
		return SUCCESS;
	/*
     if(node_type==LXX_COMMAND)
     {
     }
     else 
     */
	if(node_type==LXX_GCID)
	{
		if(node_count<p_resp->_need_dl_num)
		{
			node_count++;
		}
	}
	else if(node_type==LXX_GCID_LIST)
	{
		if(p_resp->_action._error_code!=0)
			SAFE_DELETE(p_resp->_dl_array);
	}
    
	return SUCCESS;
}
_int32 lx_screensoht_xml_attr_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *attr_name, char const *attr_value)
{
	_int32 ret_val = SUCCESS;
	LX_PT_SS* p_resp=(LX_PT_SS*)user_data;
	LX_DL_FILE * p_info =  p_resp->_dl_array+node_count; 
	
    
	if(p_resp->_action._error_code!=0)
		return SUCCESS;
    
	switch(node_type)
	{
        case LXX_COMMAND:
            if(!sd_strcmp(attr_name,"id"))
            {
                if(sd_strcmp(attr_value,"screenshot_resp"))//&&sd_strcmp(attr_value, "unknown_resp"))
                {
                    p_resp->_action._error_code=LXE_WRONG_RESPONE;
                    break;
                }
            }
            else if(!sd_strcmp(attr_name,"result"))
            {
                p_resp->_action._resp_status=sd_atoi(attr_value);
                if(p_resp->_action._resp_status!=0)
                {
                    p_resp->_action._error_code=p_resp->_action._resp_status;
                    break;
                }
            }
            else if(!sd_strcmp(attr_name,"message"))
            {
                ;
            }
            break;
            /*
             case LXX_USERINFO:
             if(!sd_strcmp(attr_name,"id"))
             {
             }
             else if(!sd_strcmp(attr_name,"vip_level"))
             {
             }
             else if(!sd_strcmp(attr_name,"from"))
             {
             }
             break;
             */
        case LXX_GCID_LIST:
            if(!sd_strcmp(attr_name,"list_num"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                
                if(p_resp->_need_dl_num!= sd_atoi(attr_value))
                {
                    p_resp->_action._error_code = LXE_WRONG_GCID_LIST_NUM;
                    sd_assert(FALSE);
                    break;
                }
                
                ret_val = sd_malloc(p_resp->_need_dl_num*sizeof(LX_DL_FILE),(void**) &p_resp->_dl_array);
                if(ret_val!=SUCCESS)
                {
                    p_resp->_action._error_code = ret_val;
                    sd_assert(FALSE);
                    break;
                }
                
                sd_memset(p_resp->_dl_array,0x00,p_resp->_need_dl_num*sizeof(LX_DL_FILE));
                
                p_info = p_resp->_dl_array;//��p_info�����һ���������ӿɶ��ԣ�Ҳ�����״�7.28
            }
            break;
        case LXX_GCID:
            if(!sd_strcmp(attr_name,"ret"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    p_info->_action._error_code = LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                p_info->_action._error_code = sd_atoi(attr_value);
                //sd_assert(p_info->_action._error_code == 0);
            }
            else if(!sd_strcmp(attr_name,"g"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    p_info->_action._error_code = LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                /*ret_val = lx_check_gcid(p_resp,p_info,(char*)attr_value);
                 if(ret_val!=SUCCESS)
                 {
                 p_info->_action._error_code = LXE_WRONG_GCID;
                 sd_assert(FALSE);
                 break;
                 }*/
                sd_snprintf(p_info->_action._file_path, MAX_FULL_PATH_BUFFER_LEN-1, "%s/%s.jpg",p_resp->_resp._store_path,attr_value);
            }
            else if(!sd_strcmp(attr_name,"section"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    //p_info->_action._error_code = LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                //ret_val = sd_str_to_u64(attr_value, sd_strlen(attr_value), &p_info->_size);
                //sd_assert(ret_val == SUCCESS);
            }
            else if(!sd_strcmp(attr_name,"section_type"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    //p_info->_action._error_code = LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                p_info->_section_type = sd_atoi(attr_value);
            }
            else if(!sd_strcmp(attr_name,"smallshot_url"))
            {
                if(sd_strlen(attr_value)==0 && p_resp->_req._is_big== FALSE)
                {
                    //p_info->_action._error_code = LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                
                if(!p_resp->_req._is_big)
                {
                    sd_assert(sd_strlen(attr_value)<MAX_URL_LEN);
                    sd_strncpy(p_info->_url,attr_value, MAX_URL_LEN-1);
                }
            }
            else if(!sd_strcmp(attr_name,"bigshot_url"))
            {
                if(sd_strlen(attr_value)==0 && p_resp->_req._is_big == TRUE)
                {
                    //p_info->_action._error_code = LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                
                if(p_resp->_req._is_big)
                {
                    sd_assert(sd_strlen(attr_value)<MAX_URL_LEN);
                    sd_strncpy(p_info->_url,attr_value, MAX_URL_LEN-1);
                }
            }
            break;
        default:	
            ;
	}
    
	return SUCCESS;
}
_int32 lx_screensoht_xml_value_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *value)
{
	return SUCCESS;
}

/*******************************************************************************************************************************************/

_int32 lx_vod_url_xml_node_proc(void *user_data, void *parent_node, _int32 parent_type, _int32 depth, char const *node_name, void **node, _int32 *node_type)
{
	LX_PT_VOD_URL* p_resp=(LX_PT_VOD_URL *)user_data;
	
	if(p_resp->_action._error_code!=0)
		return SUCCESS;
	
	switch(depth)
	{
		case 0:
			if(!sd_strcmp(node_name,"LixianProtocol"))
				break;
			else
			{
				p_resp->_action._error_code=LXE_WRONG_RESPONE;
				return SUCCESS;
			}
		case 1:
			if(!sd_strcmp(node_name,"Command"))
				*node_type=LXX_COMMAND;
			break;
		case 2:
			if(!sd_strcmp(node_name,"play_url"))
				*node_type=LXX_PLAY_URL;
			if(!sd_strcmp(node_name,"ts_url"))
				*node_type=LXX_TS_URL;
			if(!sd_strcmp(node_name,"mp4_url"))
				*node_type=LXX_MP4_URL;
			else if(!sd_strcmp(node_name,"fpfilelist"))
				*node_type=LXX_FPFILE_LIST;
			break;
		case 3:
			if(!sd_strcmp(node_name,"file"))
				*node_type=LXX_FILE;
			break;
		default:
			break;
	}
	return SUCCESS;
}
_int32 lx_vod_url_xml_node_end_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *node_name)
{
	LX_PT_VOD_URL * p_resp=(LX_PT_VOD_URL *)user_data;
	
	if(p_resp->_action._error_code!=0)
		return SUCCESS;
	/*
     if(node_type==LXX_COMMAND)
     {
     }
     else 
     */
	if(node_type==LXX_COMMAND)
	{
		//if(p_resp->_action._resp_status!=0)
		//	p_resp->_action._error_code=LXE_STATUS_NOT_FOUND;
	}
	else if(node_type==LXX_FILE)
	{
		if(node_count<p_resp->_resp._fpfile_num)
		{
			node_count++;
		}
	}
	else if(node_type==LXX_FPFILE_LIST)
	{
		if(p_resp->_action._error_code!=0)
			SAFE_DELETE(p_resp->_resp._fp_array);
	}	
	else if(node_type==LXX_PLAY_URL)
	{
        
	}
    
	return SUCCESS;
}
_int32 lx_vod_url_xml_attr_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *attr_name, char const *attr_value)
{
	_int32 ret_val = SUCCESS;
	LX_PT_VOD_URL* p_resp=(LX_PT_VOD_URL*)user_data;
	LX_FP_INFO * p_info =  p_resp->_resp._fp_array + node_count; 
	
    
	if(p_resp->_action._error_code!=0)
		return SUCCESS;
    
	switch(node_type)
	{
        case LXX_COMMAND:
            if(!sd_strcmp(attr_name,"id"))
            {
                if(sd_strcmp(attr_value,"getplayurl_resp"))//&&sd_strcmp(attr_value, "unknown_resp"))
                {
                    p_resp->_action._error_code=LXE_WRONG_RESPONE;
                    break;
                }
            }
            else if(!sd_strcmp(attr_name,"result"))
            {
                p_resp->_action._resp_status=sd_atoi(attr_value);
                if(p_resp->_action._resp_status!=0)
                {
                    p_resp->_action._error_code=p_resp->_action._resp_status;
                    break;
                }
            }
            else if(!sd_strcmp(attr_name,"message"))
            {
                ;
            }
            break;
        case LXX_PLAY_URL:
            if(p_resp->_req._video_type!=2&&p_resp->_req._video_type!=4)
            {
            	// Ĭ��high_url����middle_url��䣬���middle_urlû�������high_url���
                if(!sd_strcmp(attr_name,"high_bit_rate"))
                {
                    if(sd_strlen(attr_value)==0)
                    {
                        break;
                    }
					if(p_resp->_resp._high_bit_rate == 0)
                    	p_resp->_resp._high_bit_rate = sd_atoi(attr_value);			
                }
                else if(!sd_strcmp(attr_name,"high_filesize"))
                {
                    if(sd_strlen(attr_value)==0)
                    {
                        break;
                    }
					if(p_resp->_resp._high_size == 0)
                    {
                    	ret_val = sd_str_to_u64(attr_value, sd_strlen(attr_value), &p_resp->_resp._high_size);
                    	sd_assert(ret_val == SUCCESS);
					}
                }
                else if(!sd_strcmp(attr_name,"high_url"))
                {
                    if(sd_strlen(attr_value)==0)
                    {
                        break;
                    }
					if( sd_strlen(p_resp->_resp._high_vod_url) < 9 )
					{
                    	sd_assert(sd_strlen(attr_value)<MAX_URL_LEN);
                    	sd_strncpy(p_resp->_resp._high_vod_url, attr_value, MAX_URL_LEN-1);
					}
                }
                if(!sd_strcmp(attr_name,"middle_bit_rate"))
                {
                    if(sd_strlen(attr_value)==0)
                    {
                        break;
                    }
                    p_resp->_resp._high_bit_rate = sd_atoi(attr_value);			
                }
                else if(!sd_strcmp(attr_name,"middle_filesize"))
                {
                    if(sd_strlen(attr_value)==0)
                    {
                        break;
                    }
                    ret_val = sd_str_to_u64(attr_value, sd_strlen(attr_value), &p_resp->_resp._high_size);
                    sd_assert(ret_val == SUCCESS);
                }
                else if(!sd_strcmp(attr_name,"middle_url"))
                {
                    if(sd_strlen(attr_value)==0)
                    {
                        break;
                    }
                    sd_assert(sd_strlen(attr_value)<MAX_URL_LEN);
                    sd_strncpy(p_resp->_resp._high_vod_url, attr_value, MAX_URL_LEN-1);
                }
                else if(!sd_strcmp(attr_name,"normal_bit_rate"))
                {
                    p_resp->_resp._normal_bit_rate = sd_atoi(attr_value);			
                }
                else if(!sd_strcmp(attr_name,"normal_filesize"))
                {
                    if(sd_strlen(attr_value)==0)
                    {
                        break;
                    }
                    ret_val = sd_str_to_u64(attr_value, sd_strlen(attr_value), &p_resp->_resp._normal_size);
                    sd_assert(ret_val == SUCCESS);
                }
                else if(!sd_strcmp(attr_name,"normal_url"))
                {
                    if(sd_strlen(attr_value)==0)
                    {
                        break;
                    }
                    sd_assert(sd_strlen(attr_value)<MAX_URL_LEN);
                    sd_strncpy(p_resp->_resp._normal_vod_url, attr_value, MAX_URL_LEN-1);
                }
				else if(!sd_strcmp(attr_name,"mobile1_bit_rate"))
                {
                    p_resp->_resp._fluency_bit_rate_1= sd_atoi(attr_value);			
                }
                else if(!sd_strcmp(attr_name,"mobile1_filesize"))
                {
                    if(sd_strlen(attr_value)==0)
                    {
                        break;
                    }
                    ret_val = sd_str_to_u64(attr_value, sd_strlen(attr_value), &p_resp->_resp._fluency_size_1);
                    sd_assert(ret_val == SUCCESS);
                }
                else if(!sd_strcmp(attr_name,"mobile1_url"))
                {
                    if(sd_strlen(attr_value)==0)
                    {
                        break;
                    }
                    sd_assert(sd_strlen(attr_value)<MAX_URL_LEN);
                    sd_strncpy(p_resp->_resp._fluency_vod_url_1, attr_value, MAX_URL_LEN-1);
                }
				else if(!sd_strcmp(attr_name,"mobile2_bit_rate"))
                {
                    p_resp->_resp._fluency_bit_rate_2= sd_atoi(attr_value);			
                }
                else if(!sd_strcmp(attr_name,"mobile2_filesize"))
                {
                    if(sd_strlen(attr_value)==0)
                    {
                        break;
                    }
                    ret_val = sd_str_to_u64(attr_value, sd_strlen(attr_value), &p_resp->_resp._fluency_size_2);
                    sd_assert(ret_val == SUCCESS);
                }
                else if(!sd_strcmp(attr_name,"mobile2_url"))
                {
                    if(sd_strlen(attr_value)==0)
                    {
                        break;
                    }
                    sd_assert(sd_strlen(attr_value)<MAX_URL_LEN);
                    sd_strncpy(p_resp->_resp._fluency_vod_url_2, attr_value, MAX_URL_LEN-1);
                }
                else if(!sd_strcmp(attr_name,"video_time"))
                {
                    if(sd_strlen(attr_value)==0)
                    {
                        break;
                    }
                    p_resp->_resp._video_time = sd_atoi(attr_value);			
                }
            }
            break;
        case LXX_TS_URL:
            if(p_resp->_req._video_type==2)
            {
                if(!sd_strcmp(attr_name,"high_bit_rate"))
                {
                    if(sd_strlen(attr_value)==0)
                    {
                        break;
                    }
                    p_resp->_resp._high_bit_rate = sd_atoi(attr_value);			
                }
                else if(!sd_strcmp(attr_name,"high_filesize"))
                {
                    if(sd_strlen(attr_value)==0)
                    {
                        break;
                    }
                    ret_val = sd_str_to_u64(attr_value, sd_strlen(attr_value), &p_resp->_resp._high_size);
                    sd_assert(ret_val == SUCCESS);
                }
                else if(!sd_strcmp(attr_name,"high_url"))
                {
                    if(sd_strlen(attr_value)==0)
                    {
                        break;
                    }
                    sd_assert(sd_strlen(attr_value)<MAX_URL_LEN);
                    sd_strncpy(p_resp->_resp._high_vod_url, attr_value, MAX_URL_LEN-1);
                }
                else if(!sd_strcmp(attr_name,"normal_bit_rate"))
                {
                    p_resp->_resp._normal_bit_rate = sd_atoi(attr_value);			
                }
                else if(!sd_strcmp(attr_name,"normal_filesize"))
                {
                    if(sd_strlen(attr_value)==0)
                    {
                        break;
                    }
                    ret_val = sd_str_to_u64(attr_value, sd_strlen(attr_value), &p_resp->_resp._normal_size);
                    sd_assert(ret_val == SUCCESS);
                }
                else if(!sd_strcmp(attr_name,"normal_url"))
                {
                    if(sd_strlen(attr_value)==0)
                    {
                        break;
                    }
                    sd_assert(sd_strlen(attr_value)<MAX_URL_LEN);
                    sd_strncpy(p_resp->_resp._normal_vod_url, attr_value, MAX_URL_LEN-1);
                }
                else if(!sd_strcmp(attr_name,"video_time"))
                {
                    if(sd_strlen(attr_value)==0)
                    {
                        break;
                    }
                    p_resp->_resp._video_time = sd_atoi(attr_value);			
                }
            }
            break;
        case LXX_MP4_URL:
            if(p_resp->_req._video_type==4)
            {
                if(!sd_strcmp(attr_name,"high_bit_rate"))
                {
                    if(sd_strlen(attr_value)==0)
                    {
                        break;
                    }
                    p_resp->_resp._high_bit_rate = sd_atoi(attr_value);			
                }
                else if(!sd_strcmp(attr_name,"high_filesize"))
                {
                    if(sd_strlen(attr_value)==0)
                    {
                        break;
                    }
                    ret_val = sd_str_to_u64(attr_value, sd_strlen(attr_value), &p_resp->_resp._high_size);
                    sd_assert(ret_val == SUCCESS);
                }
                else if(!sd_strcmp(attr_name,"high_url"))
                {
                    if(sd_strlen(attr_value)==0)
                    {
                        break;
                    }
                    sd_assert(sd_strlen(attr_value)<MAX_URL_LEN);
                    sd_strncpy(p_resp->_resp._high_vod_url, attr_value, MAX_URL_LEN-1);
                }
                else if(!sd_strcmp(attr_name,"normal_bit_rate"))
                {
                    p_resp->_resp._normal_bit_rate = sd_atoi(attr_value);			
                }
                else if(!sd_strcmp(attr_name,"normal_filesize"))
                {
                    if(sd_strlen(attr_value)==0)
                    {
                        break;
                    }
                    ret_val = sd_str_to_u64(attr_value, sd_strlen(attr_value), &p_resp->_resp._normal_size);
                    sd_assert(ret_val == SUCCESS);
                }
                else if(!sd_strcmp(attr_name,"normal_url"))
                {
                    if(sd_strlen(attr_value)==0)
                    {
                        break;
                    }
                    sd_assert(sd_strlen(attr_value)<MAX_URL_LEN);
                    sd_strncpy(p_resp->_resp._normal_vod_url, attr_value, MAX_URL_LEN-1);
                }
                else if(!sd_strcmp(attr_name,"video_time"))
                {
                    if(sd_strlen(attr_value)==0)
                    {
                        break;
                    }
                    p_resp->_resp._video_time = sd_atoi(attr_value);			
                }
            }
            break;
        case LXX_FPFILE_LIST:
            if(!sd_strcmp(attr_name,"list_num"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    break;
                }
                p_resp->_resp._fpfile_num = sd_atoi(attr_value);
                
                if(p_resp->_resp._fpfile_num != 0)
                {
                    ret_val = sd_malloc(p_resp->_resp._fpfile_num*sizeof(LX_FP_INFO), (void**)&p_resp->_resp._fp_array);
                    if(ret_val!=SUCCESS)
                    {
                        p_resp->_action._error_code = ret_val;
                        break;
                    }
                    sd_memset(p_resp->_resp._fp_array, 0x00, p_resp->_resp._fpfile_num*sizeof(LX_FP_INFO));
                    p_info = p_resp->_resp._fp_array;//��p_info�����һ���������ӿɶ��ԣ�Ҳ�����״�7.28
                }
            }
            break;
        case LXX_FILE:
            if(!sd_strcmp(attr_name,"gcid"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                ret_val = sd_string_to_cid((char *)attr_value,p_info->_gcid);
                sd_assert(ret_val == SUCCESS);
            }
            else if(!sd_strcmp(attr_name,"cid"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                ret_val = sd_string_to_cid((char *)attr_value, p_info->_cid);
                sd_assert(ret_val == SUCCESS);
            }
            else if(!sd_strcmp(attr_name,"filesize"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    p_resp->_action._error_code=LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                ret_val = sd_str_to_u64(attr_value, sd_strlen(attr_value), &p_info->_size);
                sd_assert(ret_val == SUCCESS);
            }
            else if(!sd_strcmp(attr_name,"section"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    break;
                }
            }
            else if(!sd_strcmp(attr_name,"bit_rate"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    p_resp->_action._error_code = LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                p_info->_bit_rate = sd_atoi(attr_value);			
            }		
            else if(!sd_strcmp(attr_name,"video_time"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    p_resp->_action._error_code = LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                p_info->_video_time = sd_atoi(attr_value);			
            }
            else if(!sd_strcmp(attr_name,"url"))
            {
                if(sd_strlen(attr_value)==0)
                {
                    p_resp->_action._error_code = LXE_KEYWORD_NOT_FOUND;
                    break;
                }
                sd_assert(sd_strlen(attr_value)<MAX_URL_LEN);
                sd_strncpy(p_info->_vod_url, attr_value, MAX_URL_LEN-1);
            }
            break;
        default:	
            ;
	}
    
	return SUCCESS;
}
_int32 lx_vod_url_xml_value_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *value)
{
	return SUCCESS;
}

_u8 * lx_get_next_need_dl_gcid(LX_PT_SS * p_ss,_u8 * p_current_gcid)
{
	_int32 index = 0;
	if(p_current_gcid!=NULL)
	{
		index = (p_current_gcid-p_ss->_req._gcid_array)/CID_SIZE + 1;
	}
	
	for(;index<p_ss->_req._file_num;index++)
	{
		if(*(p_ss->_resp._result_array+index)!=SUCCESS)
		{
			return p_ss->_req._gcid_array+(CID_SIZE*index);
		}
	}
	sd_assert(FALSE);
	return NULL;
}
_int32 lx_check_gcid(LX_PT_SS * p_ss,LX_DL_FILE * p_info, char * gcid_str)
{
	_int32 ret_val = SUCCESS,index = 0;
	_u8 gcid[CID_SIZE]={0},* p_gcid = NULL;
    
	ret_val = sd_string_to_cid(gcid_str,gcid);
	CHECK_VALUE(ret_val);
    
	p_gcid = p_ss->_req._gcid_array;
	
	for(;index<p_ss->_req._file_num;index++)
	{
		if(sd_is_cid_equal(gcid,p_gcid))
		{
			p_info->_gcid_index = index;
			return SUCCESS;
		}
		p_gcid+=CID_SIZE;
	}
    
	return -1;
}

_int32 lx_pt_set_header(char **buffer, _int32 *cur_buflen, LX_CMD_HEADER * p_header)
{
	sd_set_int32_to_lt(buffer, cur_buflen, (_int32)p_header->_version);
	sd_set_int32_to_lt(buffer, cur_buflen, (_int32)p_header->_seq);
	sd_set_int32_to_lt(buffer, cur_buflen, (_int32)p_header->_len);
	sd_set_int32_to_lt(buffer, cur_buflen, (_int32)p_header->_thunder_flag);
	sd_set_int16_to_lt(buffer, cur_buflen, (_int16)p_header->_compress_flag);
	sd_set_int16_to_lt(buffer, cur_buflen, (_int16)p_header->_cmd_type);
	return SUCCESS;
}

_int32 lx_pt_get_header(char **buffer, _int32 *cur_buflen, LX_CMD_HEADER * p_header)
{
	sd_get_int32_from_lt(buffer, cur_buflen, (_int32*)&p_header->_version);
	sd_get_int32_from_lt(buffer, cur_buflen, (_int32*)&p_header->_seq);
	sd_get_int32_from_lt(buffer, cur_buflen, (_int32*)&p_header->_len);
	sd_get_int32_from_lt(buffer, cur_buflen, (_int32*)&p_header->_thunder_flag);
	sd_get_int16_from_lt(buffer, cur_buflen, (_int16*)&p_header->_compress_flag);
	sd_get_int16_from_lt(buffer, cur_buflen, (_int16*)&p_header->_cmd_type);	
	return SUCCESS;
}

_int32 lx_aes_encrypt(char* buffer,_u32* len)
{
	_int32 ret;
	char *pOutBuff;
	char *tmp_buffer = NULL;
	_int32 tmplen = *len;
	_int32 nOutLen;
	_int32 nBeginOffset;
	_u8 szKey[16];
	ctx_md5	md5;
	ctx_aes	aes;
	int nInOffset;
	int nOutOffset;
	unsigned char inBuff[ENCRYPT_BLOCK_SIZE],ouBuff[ENCRYPT_BLOCK_SIZE];
    
	if (buffer == NULL)
	{
		return -1;
	}
	ret = sd_malloc(*len + 16, (void**)&pOutBuff);
	if(ret != SUCCESS)
	{
		CHECK_VALUE(ret);
	}
	nOutLen = 0;
	nBeginOffset = sizeof(_u32)*3;
	md5_initialize(&md5);
	md5_update(&md5, (const unsigned char*)buffer, sizeof(_u32) * 2);
	md5_finish(&md5, szKey);
	
	/*aes encrypt*/
	aes_init(&aes, 16, szKey);
	nInOffset = nBeginOffset; //���Ǵ�ͷ��ʼ����
	nOutOffset = 0;
	sd_memset(inBuff,0,ENCRYPT_BLOCK_SIZE);
	sd_memset(ouBuff,0,ENCRYPT_BLOCK_SIZE);
	while(TRUE)
	{
		if (*len - nInOffset >= ENCRYPT_BLOCK_SIZE)
		{
			sd_memcpy(inBuff,buffer+nInOffset,ENCRYPT_BLOCK_SIZE);
			aes_cipher(&aes, inBuff, ouBuff);
			sd_memcpy(pOutBuff+nOutOffset,ouBuff,ENCRYPT_BLOCK_SIZE);
			nInOffset += ENCRYPT_BLOCK_SIZE;
			nOutOffset += ENCRYPT_BLOCK_SIZE;
		}
		else
		{
			int nDataLen = *len - nInOffset;
			int nFillData = ENCRYPT_BLOCK_SIZE - nDataLen;
			sd_memset(inBuff,nFillData,ENCRYPT_BLOCK_SIZE);
			sd_memset(ouBuff,0,ENCRYPT_BLOCK_SIZE);
			if (nDataLen > 0)
			{
				sd_memcpy(inBuff,buffer+nInOffset,nDataLen);
				aes_cipher(&aes, inBuff, ouBuff);
				sd_memcpy(pOutBuff+nOutOffset,ouBuff,ENCRYPT_BLOCK_SIZE);
				nInOffset += nDataLen;
				nOutOffset += ENCRYPT_BLOCK_SIZE;
			}
			else
			{
				aes_cipher(&aes, inBuff, ouBuff);
				sd_memcpy(pOutBuff+nOutOffset,ouBuff,ENCRYPT_BLOCK_SIZE);
				nOutOffset += ENCRYPT_BLOCK_SIZE;
			}
			break;
		}
	}
	nOutLen = nOutOffset;
	sd_memcpy(buffer + nBeginOffset,pOutBuff, nOutLen);
	tmp_buffer = buffer + sizeof(_u32) * 2;
    sd_set_int32_to_lt(&tmp_buffer, &tmplen, nOutLen);     
	sd_free(pOutBuff);
	if(nOutLen + nBeginOffset > *len + 16)
		return -1;
	*len = nOutLen + nBeginOffset;
	return SUCCESS;
}

_int32 lx_aes_decrypt(char* databuff, _u32* bufflen)
{
	_int32 ret;
	int nBeginOffset;
	char *pOutBuff;
	int  nOutLen;
	unsigned char szKey[16];
	ctx_md5 md5;
	ctx_aes aes;
	int nInOffset;
	int nOutOffset;
	unsigned char inBuff[ENCRYPT_BLOCK_SIZE],ouBuff[ENCRYPT_BLOCK_SIZE];
	char * out_ptr;
	if (databuff == NULL)
	{
		return -1;
	}
	nBeginOffset = sizeof(_u32)*3;
	if ((*bufflen-nBeginOffset)%ENCRYPT_BLOCK_SIZE != 0)
	{
		return -1;
	}
	ret = sd_malloc(*bufflen + 16, (void**)&pOutBuff);
	if(ret != SUCCESS)
	{
		EM_LOG_ERROR("aes_decrypt, malloc failed.");
		CHECK_VALUE(ret);
	}
	nOutLen = 0;
	md5_initialize(&md5);
	md5_update(&md5, (unsigned char*)databuff,sizeof(_u32)*2);
	md5_finish(&md5, szKey);
	aes_init(&aes,16,(unsigned char*)szKey);
	nInOffset = nBeginOffset;
	nOutOffset = 0;
	sd_memset(inBuff,0,ENCRYPT_BLOCK_SIZE);
	sd_memset(ouBuff,0,ENCRYPT_BLOCK_SIZE);
	while(*bufflen - nInOffset > 0)
	{
		sd_memcpy(inBuff,databuff+nInOffset,ENCRYPT_BLOCK_SIZE);
		aes_invcipher(&aes, inBuff,ouBuff);
		sd_memcpy(pOutBuff+nOutOffset,ouBuff,ENCRYPT_BLOCK_SIZE);
		nInOffset += ENCRYPT_BLOCK_SIZE;
		nOutOffset += ENCRYPT_BLOCK_SIZE;
	}
	nOutLen = nOutOffset;
	sd_memcpy(databuff + nBeginOffset,pOutBuff,nOutLen);
	out_ptr = pOutBuff + nOutLen - 1;
	if (*out_ptr <= 0 || *out_ptr > ENCRYPT_BLOCK_SIZE)
	{
		ret = -1;
	}
	else
	{
		if(nBeginOffset + nOutLen - *out_ptr < *bufflen)
		{
			*bufflen = nBeginOffset + nOutLen - *out_ptr;
			ret = SUCCESS;
		}
		else
		{
			ret = -1;
		}
	}
	sd_free(pOutBuff);
	return ret;
}

_int32 lx_pt_zlib_uncompress(_u16 compress_flag, char **tmp_buf, _int32 *tmp_len, char **zlib_buf)
{
	_int32 ret_val = SUCCESS;
	_int32 zlib_len= 0;
	
	if( compress_flag == 0x0100 )
	{
		ret_val = sd_zlib_uncompress(*tmp_buf, *tmp_len, &zlib_buf, &zlib_len);
		if(ret_val != SUCCESS)
			return ret_val;
		*tmp_buf = zlib_buf;
		*tmp_len = zlib_len;
	}
	return SUCCESS;
}

void lx_pt_zlib_uncompress_free(char *zlib_buf)
{
	SAFE_DELETE(zlib_buf);
}

_int32 lx_detect_regex_xml_node_proc(void *user_data, void *parent_node, _int32 parent_type, _int32 depth, char const *node_name, void **node, _int32 *node_type)
{
	_int32 ret_val = SUCCESS;
	LX_PT_GET_REGEX* p_action = (LX_PT_GET_REGEX *)user_data;
    
	EM_DETECT_SPECIAL_WEB *p_detect_website = p_action->_detect_website;
	EM_DETECT_REGEX * p_detect_rule = p_action->_detect_rule;
    
//    printf("node name:%s\n", node_name);
    
	switch(depth)
	{
		case 0:
			if(!sd_strcmp(node_name,"LixianDetectConfig"))
            {
				break;
            }
			else
			{
				return SUCCESS;
			}
		case 1:
			if(!sd_strcmp(node_name, "special_websites_for_detect")){
				*node_type = LXX_DETECT_WEBSITES_TOTAL;
				// ����special_websites_for_detect�ڵ�ʱ����LIST
				ret_val = sd_malloc(sizeof(LIST), (void**)&p_action->_website_list);
                sd_assert(ret_val == SUCCESS);
                list_init(p_action->_website_list);
				
            }
			break;
		case 2:
			if(!sd_strcmp(node_name, "website")){
                *node_type = LXX_DETECT_WEBSITE;
                // ����website�ڵ�ʱ
				// Ϊwebsite ��Ϣ����ռ�
				ret_val = sd_malloc(sizeof(EM_DETECT_SPECIAL_WEB), (void**) &p_detect_website);
                sd_assert(ret_val == SUCCESS);
                sd_memset(p_detect_website,0x00,sizeof(EM_DETECT_SPECIAL_WEB));
                ret_val = sd_malloc(sizeof(LIST), (void**)&p_detect_website->_rule_list);
                sd_assert(ret_val == SUCCESS);
                list_init(p_detect_website->_rule_list);
				p_action->_detect_website = p_detect_website;// ��ָ�뱣�浽user_data��
            }
			break;
        case 3:
			if (!sd_strcmp(node_name, "rule")) {
                *node_type = LXX_DETECT_RULE;
                //����rule�ڵ�ʱΪrule ��Ϣ����ռ�
				ret_val = sd_malloc(sizeof(EM_DETECT_REGEX), (void**) &p_detect_rule);
                sd_assert(ret_val == SUCCESS);
				sd_memset(p_detect_rule,0x00,sizeof(EM_DETECT_REGEX));
				p_action->_detect_rule = p_detect_rule;// ��ָ�뱣�浽user_data��
			}
            break;
        case 4:
            if (!sd_strcmp(node_name, "start_string")) {
                *node_type = LXX_DETECT_STRING_START;
            }
            else if (!sd_strcmp(node_name, "end_string")) {
                *node_type = LXX_DETECT_STRING_END;
            }
            else if (!sd_strcmp(node_name, "download_url"))
                *node_type = LXX_DETECT_DOWNLOAD_URL;            
            else if(!sd_strcmp(node_name, "name"))
                *node_type = LXX_DETECT_NAME;
            else if(!sd_strcmp(node_name, "name_append"))
                *node_type = LXX_DETECT_NAME_APPEND;
            else if(!sd_strcmp(node_name, "suffix"))
                *node_type = LXX_DETECT_SUFFIX;
            else if(!sd_strcmp(node_name, "file_size"))
                *node_type = LXX_DETECT_FILE_SIZE;
            break;
        default:
            break;
	}
    
	return ret_val;
}

_int32 lx_detect_regex_xml_node_end_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *node_name)
{
	_int32 ret_val = SUCCESS;
	LX_PT_GET_REGEX* p_action = (LX_PT_GET_REGEX *)user_data;
    
	EM_DETECT_SPECIAL_WEB *p_detect_website = p_action->_detect_website;
	EM_DETECT_REGEX * p_detect_rule = p_action->_detect_rule;
    
//    printf("node name:/%s\n", node_name);
    
    switch (node_type) {
            // ��website��rule�ڵ������ʱ�򣬽��ṹ�����LIST������ָ����Ϊ��
        case LXX_DETECT_RULE:
            if(p_detect_website != NULL) {
				list_push(p_detect_website->_rule_list, p_detect_rule);//��rule��Ϣ����list
				p_action->_detect_rule = NULL;// ��user_data�е�ruleָ���ÿ�
            }
            break;
        case LXX_DETECT_WEBSITE:
            list_push(p_action->_website_list, p_detect_website);//��website��Ϣ����list
            p_detect_website = NULL;// ��user_data�е�websiteָ���ÿ�
            break;
        case LXX_DETECT_STRING_START:
            break;
        case LXX_DETECT_STRING_END:
            break;
        case LXX_DETECT_DOWNLOAD_URL:
            break;
        case LXX_DETECT_NAME:
            break;
        case LXX_DETECT_NAME_APPEND:
            break;
        case LXX_DETECT_SUFFIX:
            break;
        case LXX_DETECT_FILE_SIZE:
            break;
        default:
            break;
    }
    
	return ret_val;
}

_int32 lx_detect_regex_xml_attr_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *attr_name, char const *attr_value)
{
	_int32 ret_val = SUCCESS;
	LX_PT_GET_REGEX* p_action = (LX_PT_GET_REGEX *)user_data;
    
	EM_DETECT_SPECIAL_WEB *p_detect_website = p_action->_detect_website;
	EM_DETECT_REGEX * p_detect_rule = p_action->_detect_rule;
    
    switch (node_type) {
        case LXX_DETECT_RULE:
            if (!sd_strcmp(attr_name, "matching_scheme")) {
                p_detect_rule->_matching_scheme = sd_atoi(attr_value);
            }
            break;
        case LXX_DETECT_WEBSITE:
            if(!sd_strcmp(attr_name, "url") && sd_strlen(attr_value) < MAX_URL_LEN) {
                sd_strncpy(p_detect_website->_website, attr_value, sd_strlen(attr_value));// ����������վ����
            }
            break;
        case LXX_DETECT_STRING_START:
            break;
        case LXX_DETECT_STRING_END:
            break;
        case LXX_DETECT_DOWNLOAD_URL:
            break;
        case LXX_DETECT_NAME:
            break;
        case LXX_DETECT_NAME_APPEND:
            break;
        case LXX_DETECT_SUFFIX:
            break;
        case LXX_DETECT_FILE_SIZE:
            break;
        default:
            break;
    }
	
	return ret_val;
}

_int32 lx_detect_regex_xml_value_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *value)
{
	_int32 ret_val = SUCCESS;
	LX_PT_GET_REGEX* p_action = (LX_PT_GET_REGEX *)user_data;
    
    //	EM_DETECT_SPECIAL_WEB *p_detect_website = p_action->_detect_website;
	EM_DETECT_REGEX * p_detect_rule = p_action->_detect_rule;
    
//    printf("value:%s\n",value);
    
    switch (node_type) {
        case LXX_DETECT_STRING_START:
            if(sd_strlen(value)>0&&sd_strlen(value)<EM_REGEX_LEN){
                sd_strncpy(p_detect_rule->_start_string, value, sd_strlen(value));
            }
            break;
        case LXX_DETECT_STRING_END:
            if(sd_strlen(value)>0&&sd_strlen(value)<EM_REGEX_LEN){
                sd_strncpy(p_detect_rule->_end_string, value, sd_strlen(value));
            }
            break;
        case LXX_DETECT_DOWNLOAD_URL:
            if(sd_strlen(value)>0&&sd_strlen(value)<EM_REGEX_LEN){
                sd_strncpy(p_detect_rule->_download_url, value, sd_strlen(value));
            }
            break;
        case LXX_DETECT_NAME:
            if(sd_strlen(value)>0&&sd_strlen(value)<EM_REGEX_LEN){
                sd_strncpy(p_detect_rule->_name, value, sd_strlen(value));
            }
            break;
        case LXX_DETECT_NAME_APPEND:
            if(sd_strlen(value)>0&&sd_strlen(value)<EM_REGEX_LEN){
                sd_strncpy(p_detect_rule->_name_append, value, sd_strlen(value));
            }
            break;
        case LXX_DETECT_SUFFIX:
            if(sd_strlen(value)>0&&sd_strlen(value)<EM_REGEX_LEN){
                sd_strncpy(p_detect_rule->_suffix, value, sd_strlen(value));
            }
            break;
        case LXX_DETECT_FILE_SIZE:
            if(sd_strlen(value)>0&&sd_strlen(value)<EM_REGEX_LEN){
                sd_strncpy(p_detect_rule->_file_size, value, sd_strlen(value));
            }
            break;
        case LXX_DETECT_RULE:
            break;
        case LXX_DETECT_WEBSITE:
            break;
        default:
            break;
    }
    
	return ret_val;
}

_int32 lx_detect_string_xml_node_proc(void *user_data, void *parent_node, _int32 parent_type, _int32 depth, char const *node_name, void **node, _int32 *node_type)
{
	_int32 ret_val = SUCCESS;
	LX_PT_GET_DETECT_STRING* p_action = (LX_PT_GET_DETECT_STRING*)user_data;
    
	EM_DETECT_SPECIAL_WEB *p_detect_website = p_action->_detect_website;
	EM_DETECT_STRING * p_detect_rule = p_action->_detect_rule;
    
//    printf("node name:%s\n", node_name);
    
	switch(depth)
	{
		case 0:
			if(!sd_strcmp(node_name,"LixianDetectConfig"))
            {
				break;
            }
			else
			{
				return SUCCESS;
			}
		case 1:
			if(!sd_strcmp(node_name, "special_websites_for_detect")){
				*node_type = LXX_DETECT_WEBSITES_TOTAL;
				// ����special_websites_for_detect�ڵ�ʱ����LIST
				ret_val = sd_malloc(sizeof(LIST), (void**)&p_action->_website_list);
                sd_assert(ret_val == SUCCESS);
                list_init(p_action->_website_list);
				
            }
			break;
		case 2:
			if(!sd_strcmp(node_name, "website")){
                *node_type = LXX_DETECT_WEBSITE;
                // ����website�ڵ�ʱ
				// Ϊwebsite ��Ϣ����ռ�
				ret_val = sd_malloc(sizeof(EM_DETECT_SPECIAL_WEB), (void**) &p_detect_website);
                sd_assert(ret_val == SUCCESS);
                sd_memset(p_detect_website,0x00,sizeof(EM_DETECT_SPECIAL_WEB));
                ret_val = sd_malloc(sizeof(LIST), (void**)&p_detect_website->_rule_list);
                sd_assert(ret_val == SUCCESS);
                list_init(p_detect_website->_rule_list);
				p_action->_detect_website = p_detect_website;// ��ָ�뱣�浽user_data��
            }
			break;
        case 3:
			if (!sd_strcmp(node_name, "rule")) {
                *node_type = LXX_DETECT_RULE;
                //����rule�ڵ�ʱΪrule ��Ϣ����ռ�
				ret_val = sd_malloc(sizeof(EM_DETECT_STRING), (void**) &p_detect_rule);
                sd_assert(ret_val == SUCCESS);
				sd_memset(p_detect_rule,0x00,sizeof(EM_DETECT_STRING));
				p_action->_detect_rule = p_detect_rule;// ��ָ�뱣�浽user_data��
			}
            break;
        case 4:
            if (!sd_strcmp(node_name, "string_start")) {
                *node_type = LXX_DETECT_STRING_START;
            }
            else if (!sd_strcmp(node_name, "string_end")) {
                *node_type = LXX_DETECT_STRING_END;
            }
            else if (!sd_strcmp(node_name, "download_url_start"))
                *node_type = LXX_DETECT_DOWNLOAD_URL;            
            else if(!sd_strcmp(node_name, "name_start"))
                *node_type = LXX_DETECT_NAME;
            else if(!sd_strcmp(node_name, "name_append_start"))
                *node_type = LXX_DETECT_NAME_APPEND;
            else if(!sd_strcmp(node_name, "suffix_start"))
                *node_type = LXX_DETECT_SUFFIX;
            else if(!sd_strcmp(node_name, "file_size_start"))
                *node_type = LXX_DETECT_FILE_SIZE;
            else if (!sd_strcmp(node_name, "download_url_end"))
                *node_type = LXX_DETECT_DOWNLOAD_URL_END;            
            else if(!sd_strcmp(node_name, "name_end"))
                *node_type = LXX_DETECT_NAME_END;
            else if(!sd_strcmp(node_name, "name_append_end"))
                *node_type = LXX_DETECT_NAME_APPEND_END;
            else if(!sd_strcmp(node_name, "suffix_end"))
                *node_type = LXX_DETECT_SUFFIX_END;
            else if(!sd_strcmp(node_name, "file_size_end"))
                *node_type = LXX_DETECT_FILE_SIZE_END;
            break;
        default:
            break;
	}
    
	return ret_val;
}

_int32 lx_detect_string_xml_node_end_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *node_name)
{
	_int32 ret_val = SUCCESS;
	LX_PT_GET_DETECT_STRING* p_action = (LX_PT_GET_DETECT_STRING*)user_data;
    
	EM_DETECT_SPECIAL_WEB *p_detect_website = p_action->_detect_website;
	EM_DETECT_STRING * p_detect_rule = p_action->_detect_rule;
    
//    printf("node name:%s\n", node_name);
    
    switch (node_type) {
            // ��website��rule�ڵ������ʱ�򣬽��ṹ�����LIST������ָ����Ϊ��
        case LXX_DETECT_RULE:
            if(p_detect_website != NULL) {
				list_push(p_detect_website->_rule_list, p_detect_rule);//��rule��Ϣ����list
				p_action->_detect_rule = NULL;// ��user_data�е�ruleָ���ÿ�
            }
            break;
        case LXX_DETECT_WEBSITE:
            list_push(p_action->_website_list, p_detect_website);//��website��Ϣ����list
            p_detect_website = NULL;// ��user_data�е�websiteָ���ÿ�
            break;
        case LXX_DETECT_STRING_START:
            break;
        case LXX_DETECT_STRING_END:
            break;
        case LXX_DETECT_DOWNLOAD_URL:
            break;
        case LXX_DETECT_NAME:
            break;
        case LXX_DETECT_NAME_APPEND:
            break;
        case LXX_DETECT_SUFFIX:
            break;
        case LXX_DETECT_FILE_SIZE:
            break;
        case LXX_DETECT_DOWNLOAD_URL_END:
            break;
        case LXX_DETECT_NAME_END:
            break;
        case LXX_DETECT_NAME_APPEND_END:
            break;
        case LXX_DETECT_SUFFIX_END:
            break;
        case LXX_DETECT_FILE_SIZE_END:
            break;
        default:
            break;
    }
    
	return ret_val;
}

_int32 lx_detect_string_xml_attr_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *attr_name, char const *attr_value)
{
	_int32 ret_val = SUCCESS;
	LX_PT_GET_DETECT_STRING* p_action = (LX_PT_GET_DETECT_STRING*)user_data;
    
	EM_DETECT_SPECIAL_WEB *p_detect_website = p_action->_detect_website;
	EM_DETECT_STRING * p_detect_rule = p_action->_detect_rule;

//    printf("attr name:%s attr value:%s\n", attr_name, attr_value);
    
    switch (node_type) {
        case LXX_DETECT_RULE:
            if (!sd_strcmp(attr_name, "matching_scheme")) {
                p_detect_rule->_matching_scheme = sd_atoi(attr_value);
            }
            break;
        case LXX_DETECT_WEBSITE:
            if(!sd_strcmp(attr_name, "url") && sd_strlen(attr_value) < MAX_URL_LEN) {
                sd_strncpy(p_detect_website->_website, attr_value, sd_strlen(attr_value));// ����������վ����
            }
            break;
        case LXX_DETECT_STRING_START:
	     if(!sd_strcmp(attr_name, "include") && sd_strlen(attr_value) > 0) {
		 	p_detect_rule->_string_start_include = sd_atoi(attr_value);
	     } 	
            break;
        case LXX_DETECT_DOWNLOAD_URL:
	     if(!sd_strcmp(attr_name, "include") && sd_strlen(attr_value) > 0) {
		 	p_detect_rule->_download_url_start_include = sd_atoi(attr_value);
	     } 	
            break;
        case LXX_DETECT_NAME:
	     if(!sd_strcmp(attr_name, "include") && sd_strlen(attr_value) > 0) {
		 	p_detect_rule->_name_start_include= sd_atoi(attr_value);
	     } 	
            break;
        case LXX_DETECT_NAME_APPEND:
	     if(!sd_strcmp(attr_name, "include") && sd_strlen(attr_value) > 0) {
		 	p_detect_rule->_name_append_start_include= sd_atoi(attr_value);
	     } 	
            break;
        case LXX_DETECT_SUFFIX:
	     if(!sd_strcmp(attr_name, "include") && sd_strlen(attr_value) > 0) {
		 	p_detect_rule->_suffix_start_include= sd_atoi(attr_value);
	     } 	
            break;
        case LXX_DETECT_FILE_SIZE:
	     if(!sd_strcmp(attr_name, "include") && sd_strlen(attr_value) > 0) {
		 	p_detect_rule->_file_size_start_include= sd_atoi(attr_value);
	     } 	
            break;
        case LXX_DETECT_STRING_END:
	     if(!sd_strcmp(attr_name, "include") && sd_strlen(attr_value) > 0) {
		 	p_detect_rule->_string_end_include = sd_atoi(attr_value);
	     } 	
            break;
        case LXX_DETECT_DOWNLOAD_URL_END:
	     if(!sd_strcmp(attr_name, "include") && sd_strlen(attr_value) > 0) {
		 	p_detect_rule->_download_url_end_include= sd_atoi(attr_value);
	     } 	
            break;
        case LXX_DETECT_NAME_END:
	     if(!sd_strcmp(attr_name, "include") && sd_strlen(attr_value) > 0) {
		 	p_detect_rule->_name_end_include= sd_atoi(attr_value);
	     } 	
            break;
        case LXX_DETECT_NAME_APPEND_END:
	     if(!sd_strcmp(attr_name, "include") && sd_strlen(attr_value) > 0) {
		 	p_detect_rule->_name_append_end_include= sd_atoi(attr_value);
	     } 	
            break;
        case LXX_DETECT_SUFFIX_END:
	     if(!sd_strcmp(attr_name, "include") && sd_strlen(attr_value) > 0) {
		 	p_detect_rule->_suffix_end_include= sd_atoi(attr_value);
	     } 	
            break;
        case LXX_DETECT_FILE_SIZE_END:
	     if(!sd_strcmp(attr_name, "include") && sd_strlen(attr_value) > 0) {
		 	p_detect_rule->_file_size_end_include= sd_atoi(attr_value);
	     } 	
            break;
        default:
            break;
    }
	
	return ret_val;
}

_int32 lx_detect_string_xml_value_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *value)
{
	_int32 ret_val = SUCCESS;
	LX_PT_GET_DETECT_STRING* p_action = (LX_PT_GET_DETECT_STRING*)user_data;
    
	//EM_DETECT_SPECIAL_WEB *p_detect_website = p_action->_detect_website;
	EM_DETECT_STRING * p_detect_rule = p_action->_detect_rule;
    
//    printf("value:%s\n",value);
    
    switch (node_type) {
        case LXX_DETECT_STRING_START:
            if(sd_strlen(value)>0&&sd_strlen(value)<EM_REGEX_LEN){
                sd_strncpy(p_detect_rule->_string_start, value, sd_strlen(value));
            }
            break;
        case LXX_DETECT_STRING_END:
            if(sd_strlen(value)>0&&sd_strlen(value)<EM_REGEX_LEN){
                sd_strncpy(p_detect_rule->_string_end, value, sd_strlen(value));
            }
            break;
        case LXX_DETECT_DOWNLOAD_URL:
            if(sd_strlen(value)>0&&sd_strlen(value)<EM_REGEX_LEN){
                sd_strncpy(p_detect_rule->_download_url_start, value, sd_strlen(value));
            }
            break;
        case LXX_DETECT_NAME:
            if(sd_strlen(value)>0&&sd_strlen(value)<EM_REGEX_LEN){
                sd_strncpy(p_detect_rule->_name_start, value, sd_strlen(value));
            }
            break;
        case LXX_DETECT_NAME_APPEND:
            if(sd_strlen(value)>0&&sd_strlen(value)<EM_REGEX_LEN){
                sd_strncpy(p_detect_rule->_name_append_start, value, sd_strlen(value));
            }
            break;
        case LXX_DETECT_SUFFIX:
            if(sd_strlen(value)>0&&sd_strlen(value)<EM_REGEX_LEN){
                sd_strncpy(p_detect_rule->_suffix_start, value, sd_strlen(value));
            }
            break;
        case LXX_DETECT_FILE_SIZE:
            if(sd_strlen(value)>0&&sd_strlen(value)<EM_REGEX_LEN){
                sd_strncpy(p_detect_rule->_file_size_start, value, sd_strlen(value));
            }
            break;
        case LXX_DETECT_DOWNLOAD_URL_END:
            if(sd_strlen(value)>0&&sd_strlen(value)<EM_REGEX_LEN){
                sd_strncpy(p_detect_rule->_download_url_end, value, sd_strlen(value));
            }
            break;
        case LXX_DETECT_NAME_END:
            if(sd_strlen(value)>0&&sd_strlen(value)<EM_REGEX_LEN){
                sd_strncpy(p_detect_rule->_name_end, value, sd_strlen(value));
            }
            break;
        case LXX_DETECT_NAME_APPEND_END:
            if(sd_strlen(value)>0&&sd_strlen(value)<EM_REGEX_LEN){
                sd_strncpy(p_detect_rule->_name_append_end, value, sd_strlen(value));
            }
            break;
        case LXX_DETECT_SUFFIX_END:
            if(sd_strlen(value)>0&&sd_strlen(value)<EM_REGEX_LEN){
                sd_strncpy(p_detect_rule->_suffix_end, value, sd_strlen(value));
            }
            break;
        case LXX_DETECT_FILE_SIZE_END:
            if(sd_strlen(value)>0&&sd_strlen(value)<EM_REGEX_LEN){
                sd_strncpy(p_detect_rule->_file_size_end, value, sd_strlen(value));
            }
            break;
        case LXX_DETECT_RULE:
            break;
        case LXX_DETECT_WEBSITE:
            break;
        default:
            break;
    }
    
	return ret_val;
}

_int32 lx_handle_pubnet_url(char * pubnet_url)
{
	_int32 ret_val = SUCCESS;
	if(sd_strncmp(pubnet_url, "http://pubnet.sandai.net", sd_strlen("http://pubnet.sandai.net"))==0)
	{
		char * p_fid = sd_strstr(pubnet_url,"&fid=",0);
		EM_LOG_ERROR("lx_handle_pubnet_url start: %s",pubnet_url);
		if(p_fid==NULL)
		{
			p_fid = sd_strstr(pubnet_url,"?fid=",0);
		}
		if(p_fid!=NULL)
		{
			char * p_h = sd_strstr(p_fid,"&h=",0);
			char * p_f = sd_strstr(p_fid,"&f=",0);
			if(p_h!=NULL&&p_f!=NULL)
			{
				char new_url[MAX_URL_LEN] = "http://";
				char * pos = NULL;
				_u32 fid_len = 0,h_len = 0, f_len = 0;
				
				p_fid++;
				if(p_h<p_f)
				{
					fid_len = p_h-p_fid;
				}
				else
				{
					fid_len = p_f-p_fid;
				}
				
				p_h += sd_strlen("&h=");
				p_f += sd_strlen("&f=");
                
				pos = sd_strchr(p_h,'&',0);
				if(pos!=NULL)
				{
					h_len = pos - p_h;
				}
				else
				{
					h_len = sd_strlen(p_h);
				}
                
				pos = sd_strchr(p_f,'&',0);
				if(pos!=NULL)
				{
					f_len = pos - p_f;
				}
				else
				{
					f_len = sd_strlen(p_f);
				}
                
				sd_strncpy(new_url+sd_strlen(new_url), p_h, h_len);
				sd_strcat(new_url, "/", sd_strlen("/"));
				sd_strncpy(new_url+sd_strlen(new_url), p_f, f_len);
				sd_strcat(new_url, "?", sd_strlen("?"));
				sd_strncpy(new_url+sd_strlen(new_url), p_fid, fid_len);
                
				sd_memset(pubnet_url,0x00,MAX_URL_LEN);
				sd_strncpy(pubnet_url, new_url, MAX_URL_LEN);

				EM_LOG_ERROR("lx_handle_pubnet_url end: %s",pubnet_url);

				return SUCCESS;
			}
		}
	}
	return ret_val;
}

_int32 lx_build_req_bt_file_take_off_flux_from_high_speed(LX_PT_TAKE_OFF_FLUX* p_req, LX_FILE_INFO* p_file_info)
{
	_int32 ret_val = SUCCESS;
	char * tmp_buf = p_req->_action._req_buffer;
	_int32 tmp_len = (_int32)p_req->_action._req_buffer_len;
	_int32 peerid_len = 0;
	_int32 len = 0;
	char cid_buffer[CID_SIZE*2+4] = {0};
	char gcid_buffer[CID_SIZE*2+4] = {0};
	char * task_tmp_buf = NULL;  //��ʱָ��ʼ������񳤶ȵĵط�
	_int32 task_tmp_len = 0;     //��ʱ���񳤶ȣ������������Ϣ�����
	_int32 tmp = 0;
	
	EM_LOG_DEBUG("lx_build_req_task_off_flux_from_high_speed" );
    
	/* �������ͷ */
	ret_val = lx_pt_set_header(&tmp_buf, &tmp_len, &p_req->_req._cmd_header);
    
	/* ����û���Ϣ */
	// userid
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)p_req->_req._userid);
	// loginkey: jumpkey
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, p_req->_req._jump_key_len);
	sd_set_bytes(&tmp_buf, &tmp_len, p_req->_req._jump_key, p_req->_req._jump_key_len);
	// peerid
	peerid_len = sd_strlen(p_req->_req._peer_id);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, peerid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, p_req->_req._peer_id, peerid_len);
	// Tasksize : uint64_t  ����bt������������������С��
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)p_file_info->_size);
	// task[] : task_info  ����bt����Ӧ���Ǹ����� (�������񷵻ص���Ϣ)
	sd_set_int32_to_lt(&tmp_buf, &tmp_len,  1);
	//������task cmd len Ϊ0
	task_tmp_buf = tmp_buf;
	task_tmp_len = tmp_len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, 0);
	/* ���������Ϣ */
	//taskid : Uint64  // bt����дfileindex (����ط���Ҫȷ�������ߵ��߼�����һ��)
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, p_file_info->_file_id);
	//url : String  ����url
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)sd_strlen(p_file_info->_url));
	sd_set_bytes(&tmp_buf, &tmp_len, p_file_info->_url, (_int32)sd_strlen(p_file_info->_url));
	//refer : String
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, 0);
	//name : String
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)sd_strlen(p_file_info->_name));
	sd_set_bytes(&tmp_buf, &tmp_len, p_file_info->_name, (_int32)sd_strlen(p_file_info->_name));
	//path : String
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, 0);
	//cid : String
	if(sd_is_cid_valid(p_file_info->_cid))
	{
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, CID_SIZE*2);
		str2hex((char*)p_file_info->_cid, CID_SIZE, cid_buffer, CID_SIZE*2);
		sd_set_bytes(&tmp_buf, &tmp_len, (char*)cid_buffer,  CID_SIZE*2);
	}
	else
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, 0);
	//gcid : String
	if(sd_is_cid_valid(p_file_info->_gcid))
	{
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, CID_SIZE*2);
		str2hex((char*)p_file_info->_gcid, CID_SIZE, gcid_buffer, CID_SIZE*2);
		sd_set_bytes(&tmp_buf, &tmp_len, (char*)gcid_buffer,  CID_SIZE*2);
	}
	else
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, 0);
	//type : Uint32_t
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, LXT_BT_FILE);
	//speed : Uint32_t
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, 0);
	//avg_speed : Uint32_t
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, 0);
	//size : Uint64_t
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, p_file_info->_size);
	//create_time : Uint64_t
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, 0);
	//down_time : Uint64_t
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, 0);
	//remain_time : Uint64_t
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, 0);//p_task_info->_left_live_time);
	//complete_time : Uint64_t
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, 0);
	//status : Uint32_t
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, p_file_info->_state);
	//process : Uint32_t
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, p_file_info->_progress);

	//��������task cmd len
	tmp = sizeof(_u32);
	sd_set_int32_to_lt(&task_tmp_buf, &tmp, task_tmp_len -tmp_len - sizeof(_u32));

	//res_type: uint8_t   -----��Դ����(bt,��¿����ͨ)
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)LXT_BT_FILE);
	//   res_id:string      -----��ԴID ��bt��bt_info_hash��p2sp��cid��emule��emule_hash
	sd_memset(gcid_buffer, 0x00, sizeof(gcid_buffer));
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, CID_SIZE*2);
	str2hex((char*)p_file_info->_gcid, CID_SIZE, gcid_buffer, CID_SIZE*2);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)gcid_buffer,  CID_SIZE*2);
	EM_LOG_DEBUG("lx_build_req_task_off_flux_from_high_speed, gcid = %s", gcid_buffer);

	//task_name:string   ----������
	len = sd_strlen(p_file_info->_name);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, len);
	sd_set_bytes(&tmp_buf, &tmp_len, p_file_info->_name, len);
	//business_flag: uint32_t  --- Ѹ���ƶ�������17
	sd_set_int32_to_lt(&tmp_buf, &tmp_len,  p_req->_req._business_flag);
	//need_query_manager  --��Ҫ��ѯcdnmanger��
	sd_set_int32_to_lt(&tmp_buf, &tmp_len,  1);
    
	/********************************************************************************************/
	/* Э��������� */
	p_req->_action._req_data_len = p_req->_action._req_buffer_len - tmp_len;
    p_req->_req._cmd_header._len = p_req->_action._req_data_len - sizeof(p_req->_req._cmd_header);
    
	/*aes ����*/
	tmp_len = p_req->_action._req_data_len;
	ret_val = lx_aes_encrypt(p_req->_action._req_buffer, (_u32*)&tmp_len);
	if (ret_val!= SUCCESS)
	{
		sd_assert(FALSE);
		return ret_val;
	}
    
	/* ���ܺ�Ҫ��������data len !!!!*/
	p_req->_action._req_data_len = tmp_len;
	
	return SUCCESS;	
}


_int32 lx_build_req_take_off_flux_from_high_speed(LX_PT_TAKE_OFF_FLUX* p_req, LX_TASK_INFO_EX * p_task_info)
{
	_int32 ret_val = SUCCESS;
	char * tmp_buf = p_req->_action._req_buffer;
	_int32 tmp_len = (_int32)p_req->_action._req_buffer_len;
	_int32 peerid_len = 0;
	_int32 len = 0;
	char cid_buffer[CID_SIZE*2+4] = {0};
	char gcid_buffer[CID_SIZE*2+4] = {0};
	char * task_tmp_buf = NULL;  //��ʱָ��ʼ������񳤶ȵĵط�
	_int32 task_tmp_len = 0;     //��ʱ���񳤶ȣ������������Ϣ�����
	_int32 tmp = 0;
	
	EM_LOG_DEBUG("lx_build_req_task_off_flux_from_high_speed" );
    
	/* �������ͷ */
	ret_val = lx_pt_set_header(&tmp_buf, &tmp_len, &p_req->_req._cmd_header);
    
	/* ����û���Ϣ */
	//sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)p_req->_req._jump_key_len);
	//sd_set_bytes(&tmp_buf, &tmp_len, p_req->_req._jump_key, (_int32)p_req->_req._jump_key_len);
	// userid
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)p_req->_req._userid);
	// loginkey: jumpkey
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, p_req->_req._jump_key_len);
	sd_set_bytes(&tmp_buf, &tmp_len, p_req->_req._jump_key, p_req->_req._jump_key_len);
	// peerid
	peerid_len = sd_strlen(p_req->_req._peer_id);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, peerid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, p_req->_req._peer_id, peerid_len);
	// Tasksize : uint64_t  ����bt������������������С��
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)p_task_info->_size);
	// task[] : task_info  ����bt����Ӧ���Ǹ����� (�������񷵻ص���Ϣ)
	if( LXT_BT_ALL != p_task_info->_type)
	{
		sd_set_int32_to_lt(&tmp_buf, &tmp_len,  1);
		//������task cmd len Ϊ0
		task_tmp_buf = tmp_buf;
		task_tmp_len = tmp_len;
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, 0);
		/* ���������Ϣ */
		//taskid : Uint64  // bt����дfileindex (����ط���Ҫȷ�������ߵ��߼�����һ��)
		sd_set_int64_to_lt(&tmp_buf, &tmp_len, p_task_info->_task_id);
		//url : String  ����url
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)sd_strlen(p_task_info->_url));
		sd_set_bytes(&tmp_buf, &tmp_len, p_task_info->_url, (_int32)sd_strlen(p_task_info->_url));
		//refer : String
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, 0);
		//name : String
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)sd_strlen(p_task_info->_name));
		sd_set_bytes(&tmp_buf, &tmp_len, p_task_info->_name, (_int32)sd_strlen(p_task_info->_name));
		//path : String
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, 0);
		//cid : String
		if(sd_is_cid_valid(p_task_info->_cid))
		{
			sd_set_int32_to_lt(&tmp_buf, &tmp_len, CID_SIZE*2);
			str2hex((char*)p_task_info->_cid, CID_SIZE, cid_buffer, CID_SIZE*2);
			sd_set_bytes(&tmp_buf, &tmp_len, (char*)cid_buffer,  CID_SIZE*2);
		}
		else
			sd_set_int32_to_lt(&tmp_buf, &tmp_len, 0);
		//gcid : String
		if(sd_is_cid_valid(p_task_info->_gcid))
		{
			sd_set_int32_to_lt(&tmp_buf, &tmp_len, CID_SIZE*2);
			str2hex((char*)p_task_info->_gcid, CID_SIZE, gcid_buffer, CID_SIZE*2);
			sd_set_bytes(&tmp_buf, &tmp_len, (char*)gcid_buffer,  CID_SIZE*2);
		}
		else
			sd_set_int32_to_lt(&tmp_buf, &tmp_len, 0);
		//type : Uint32_t
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, p_task_info->_type);
		//speed : Uint32_t
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, 0);
		//avg_speed : Uint32_t
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, 0);
		//size : Uint64_t
		sd_set_int64_to_lt(&tmp_buf, &tmp_len, p_task_info->_size);
		//create_time : Uint64_t
		sd_set_int64_to_lt(&tmp_buf, &tmp_len, 0);
		//down_time : Uint64_t
		sd_set_int64_to_lt(&tmp_buf, &tmp_len, 0);
		//remain_time : Uint64_t
		sd_set_int64_to_lt(&tmp_buf, &tmp_len, p_task_info->_left_live_time);
		//complete_time : Uint64_t
		sd_set_int64_to_lt(&tmp_buf, &tmp_len, 0);
		//status : Uint32_t
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, p_task_info->_state);
		//process : Uint32_t
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, p_task_info->_progress);

		//��������task cmd len
		tmp = sizeof(_u32);
		sd_set_int32_to_lt(&task_tmp_buf, &tmp, task_tmp_len -tmp_len - sizeof(_u32));
	}
	//res_type: uint8_t   -----��Դ����(bt,��¿����ͨ)
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)p_task_info->_type);
	//   res_id:string      -----��ԴID ��bt��bt_info_hash��p2sp��cid��emule��emule_hash
	if( LXT_BT_ALL != p_task_info->_type)
	{
		sd_memset(gcid_buffer, 0x00, sizeof(gcid_buffer));
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, CID_SIZE*2);
		str2hex((char*)p_task_info->_gcid, CID_SIZE, gcid_buffer, CID_SIZE*2);
		sd_set_bytes(&tmp_buf, &tmp_len, (char*)gcid_buffer,  CID_SIZE*2);
		EM_LOG_DEBUG("lx_build_req_task_off_flux_from_high_speed, gcid = %s", gcid_buffer);
	}
	//task_name:string   ----������
	len = sd_strlen(p_task_info->_name);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, len);
	sd_set_bytes(&tmp_buf, &tmp_len, p_task_info->_name, len);
	//business_flag: uint32_t  --- Ѹ���ƶ�������17
	sd_set_int32_to_lt(&tmp_buf, &tmp_len,  p_req->_req._business_flag);
	//need_query_manager  --��Ҫ��ѯcdnmanger��
	sd_set_int32_to_lt(&tmp_buf, &tmp_len,  1);
    
	/********************************************************************************************/
	/* Э��������� */
	p_req->_action._req_data_len = p_req->_action._req_buffer_len - tmp_len;
    p_req->_req._cmd_header._len = p_req->_action._req_data_len - sizeof(p_req->_req._cmd_header);
    
	/*aes ����*/
	tmp_len = p_req->_action._req_data_len;
	ret_val = lx_aes_encrypt(p_req->_action._req_buffer, (_u32*)&tmp_len);
	if (ret_val!= SUCCESS)
	{
		sd_assert(FALSE);
		return ret_val;
	}
    
	/* ���ܺ�Ҫ��������data len !!!!*/
	p_req->_action._req_data_len = tmp_len;
	
	return SUCCESS;	
}

_int32 lx_parse_resp_take_off_flux_from_high_speed(LX_PT_TAKE_OFF_FLUX* p_resp)
{
	_int32 ret_val = SUCCESS;
	char * tmp_buf = p_resp->_action._resp_buffer;
	_int32 tmp_len = (_int32)p_resp->_action._resp_data_len;
	LX_CMD_HEADER cmd_header = {0};
	_int32 msg_len = 0;
	_int32 result = 0;
	char msginfo[64] = {0};
	_int64 tmp_data_64 = 0;
	_int32 tmp_data_32 = 0;
	_int16 tmp_data_16 = 0;
	char buf[MAX_URL_LEN] = {0};

	EM_LOG_DEBUG("lx_parse_resp_take_off_flux_from_high_speed" );
	
	ret_val = lx_aes_decrypt(tmp_buf, (u32*)&tmp_len);
	CHECK_VALUE(ret_val);

	/* ��ȡ����ͷ */
	lx_pt_get_header(&tmp_buf, &tmp_len, &cmd_header);
	// result
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&result);
	if (result != 0) return result;
	
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &msg_len);
	if (msg_len > 0)
		sd_get_bytes(&tmp_buf, &tmp_len, msginfo, msg_len);
	//Capacity : uint64_t   �ܵ�����
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._all_flux);
	//Remain : uint64_t   �ܵ�ʣ������
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&p_resp->_resp._remain_flux);
	//Needed : uint64_t   ������������������
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&tmp_data_64);
	//Flags : uint32_t     ��������, ��ʾ���ٽ�����,0Ϊ����ʾ, 1Ϊ��ʾ
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&tmp_data_32);
	//goldbean_balance : uint64_t	ʣ�����(������Ϊ��λ���൱�ڽ���*1000)
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&tmp_data_64);
	//goldbean_needed : uint64_t	����+�� ����ʱ��������Ҫ�Ľ���(��λͬ��)
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&tmp_data_64);
	//solidify_capacity_remain	: uin64_t 	�̻�������ʣ������
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&tmp_data_64);
	//num_of_gb_deducted_cur_transaction : uin64_t���������¿۳��Ľ���Ŀ����λͬ�ϣ�
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&tmp_data_64);
	//free : uint16 �Ƿ����
	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int64*)&tmp_data_16);
	//cdn_exist: string  �Ƿ���ڸ���ͨ��
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, &msg_len);
	if(msg_len > 0)
	{
		sd_memset(msginfo, 0x00, sizeof(msginfo));
		sd_get_bytes(&tmp_buf, &tmp_len, msginfo, msg_len);
	}
	return SUCCESS;
}

#if 0
#include "json/json_lib/json.h"
typedef json_object LX_JSON_OBJECT;
_int32 lx_json_parse( const char *str, LX_JSON_OBJECT **pp_json_obj )
{
	*pp_json_obj = json_tokener_parse(str);
	if(is_error(*pp_json_obj)) 
		return JSON_LIB_ERR;
	return SUCCESS;
}

_int32 lx_json_release( LX_JSON_OBJECT *p_json_object )
{
	if( NULL == p_json_object ) return SUCCESS;
	json_object_put( p_json_object );
	return SUCCESS;
}

_int32 lx_build_http_req_zip_and_aes(LX_PT * p_req)
{
	_int32 ret_val=0;
	_u8* gzed_buf = NULL;
	_int32 gzed_len = 0;
	_u32 tmp_len = p_req->_req_data_len;//��Ҫѹ�����ݵĳ���
	
	_u32 des_len = 0;
	char os_type[MAX_OS_LEN] = {0};
	_int32 width, length;
	char key_string[MAX_KEY_STRING] = {0};//CLOUD_PLAY_AES_KEY;

	// �ȼ�ѹ�����
	if(p_req->_is_compress)
	{
		// sd_gz_encode_buffer��Ҫ��������Ϊtmp_len+18��buffer��Ϊ���
		tmp_len += 18;
		ret_val = sd_malloc(tmp_len, (void**)&gzed_buf);
		CHECK_VALUE(ret_val);
		sd_memset(gzed_buf, 0x00, tmp_len);

		ret_val = sd_gz_encode_buffer((_u8 *)p_req->_req_buffer, (_u32)p_req->_req_data_len, gzed_buf, (_u32)tmp_len, (_u32*)&gzed_len);

		//��ѹ���ɹ�������Ч������ʹ��ѹ��
		if((SUCCESS == ret_val)&&(gzed_len < tmp_len))
		{
			sd_memset(p_req->_req_buffer, 0x00, MAX_POST_REQ_BUFFER_LEN);
			sd_memcpy(p_req->_req_buffer, gzed_buf, gzed_len);
			p_req->_req_data_len = gzed_len;
			SAFE_DELETE(gzed_buf);
		}
		else
		{	
			ret_val = LXE_GZ_ENCODE_ERROR;
			SAFE_DELETE(gzed_buf);
			return ret_val;
		}
	}
	if(p_req->_is_aes)
	{
		des_len = p_req->_req_buffer_len;
		// ��ȡ����������Э�����ϵ���Կ��ʽ
		if( LPT_GET_HIGH_SPEED_FLUX  == p_req->_type)
		{
			// ���ܵ�key��Cookie: TE=xxxx&yyy&zzz;��xxxx���֡�(httpĬ����д��xxxx������--��������,�ֱ���)
			ret_val = sd_get_os(os_type, MAX_OS_LEN);
			CHECK_VALUE(ret_val);
			ret_val = sd_get_screen_size(&width, &length);
			CHECK_VALUE(ret_val);
			sd_memset(key_string, 0x00, MAX_KEY_STRING);
			sd_snprintf(key_string, MAX_KEY_STRING,"%s,%dx%d", os_type, width, length);
		}
		else 
		{
			// �µ�Э��ͳһ�ù̶���Կ��ʽ
			sd_memset(key_string, 0x00, sizeof(key_string));
			sd_strncpy(key_string, CLOUD_PLAY_AES_KEY, MAX_KEY_STRING -1 );
		}

		ret_val = sd_aes_encrypt(key_string, (const _u8*)p_req->_req_buffer, p_req->_req_data_len, ( _u8*)p_req->_req_buffer, &des_len);
		CHECK_VALUE(ret_val);
		if( SUCCESS != ret_val )
		{	
			ret_val = LXE_AES_ENCRYPT_ERROR;
			return ret_val;
		}
		p_req->_req_data_len = des_len;
	}
    
	return ret_val;
}

_int32 lx_parse_req_aes_and_zip(LX_PT * p_resp, char* buff, _u32* len)
{
	_int32 ret_val=0;

	_u32 des_len = 0;
	char os_type[MAX_OS_LEN] = {0};
	_int32 width, length;
	char key_string[MAX_KEY_STRING] = CLOUD_PLAY_AES_KEY;

	char* de_gzed_buf = NULL;
	_u32 de_gzed_len=0;

	// sd_gz_decode_buffer�������볤������Ϊlen *2��buffer��Ϊ���
	_u8* tmp_buf = (_u8*)buff;     
	_int32 tmp_len = (*len) * 2;   

	//�Ƚ��ܺ��ѹ
	if(p_resp->_is_aes)
	{
		des_len = p_resp->_resp_data_len;
		// ��ȡ����������Э�����ϵ���Կ��ʽ
		if( LPT_GET_HIGH_SPEED_FLUX  == p_resp->_type)
		{
			// ���ܵ�key��Cookie: TE=xxxx&yyy&zzz;��xxxx���֡�(httpĬ����д��xxxx������--��������,�ֱ���)
			ret_val = sd_get_os(os_type, MAX_OS_LEN);
			CHECK_VALUE(ret_val);
			ret_val = sd_get_screen_size(&width, &length);
			CHECK_VALUE(ret_val);
			sd_memset(key_string, 0x00, MAX_KEY_STRING);
			sd_snprintf(key_string, MAX_KEY_STRING,"%s,%dx%d", os_type, width, length);
		}
		else 
		{
			// �µ�Э��ͳһ�ù̶���Կ��ʽ
			sd_memset(key_string, 0x00, sizeof(key_string));
			sd_strncpy(key_string, CLOUD_PLAY_AES_KEY, MAX_KEY_STRING -1 );
		}

		ret_val = sd_aes_decrypt(key_string, (const _u8*)tmp_buf, *len, ( _u8*)tmp_buf, &des_len);
		CHECK_VALUE(ret_val);
		if( SUCCESS != ret_val )
		{	
			ret_val = LXE_AES_DECRYPT_ERROR;
			return ret_val;
		}
		
		*len = des_len;
	}
	
	if(p_resp->_is_compress)
	{
		ret_val = sd_malloc(tmp_len, (void**)&de_gzed_buf);
		CHECK_VALUE(ret_val);
		sd_memset(de_gzed_buf, 0x00, tmp_len);
		
		ret_val = sd_gz_decode_buffer((_u8 *)tmp_buf, *len, (_u8**)&de_gzed_buf, (_u32*)&tmp_len,&de_gzed_len);

		//����ѹ�ɹ�������Ч������ʹ�ý�ѹ
		if((SUCCESS == ret_val) ) //&&(de_gzed_len > *len)) //������������Сʱ����ѹ���������ݳ��ȷ�����С
		{
			sd_memset(buff, 0x00, *len);
			sd_memcpy(buff, de_gzed_buf, de_gzed_len);
			*len = de_gzed_len;
			SAFE_DELETE(de_gzed_buf);
		}
		else
		{	
			ret_val = LXE_GZ_DECODE_ERROR;
			SAFE_DELETE(de_gzed_buf);
			return ret_val;
		}
	}
	
	return ret_val;
}

//������ȡ��Ѳ��Ե��ض�json��ʽ����
_int32 lx_parse_resp_free_strategy(LX_PT_FREE_STRATEGY* p_resp, char *tmp_buf)
{
	_int32 ret_val = SUCCESS;
	_int32 rtn = 0;
	char *is_active_stage = NULL;
	LX_JSON_OBJECT *p_new_obj = NULL;
	lh_table *p_tmp_table = NULL;
	LX_JSON_OBJECT *p_tmp_obj = NULL;
	struct lh_entry *p_tmp_entry = NULL;

	ret_val = lx_json_parse( tmp_buf, &p_new_obj);
	if( ret_val != SUCCESS )
		goto ErrHandle;
	//��ȡhash��ṹ����
    p_tmp_table = json_object_get_object(p_new_obj);
    if( NULL == p_tmp_table)
    {
    	ret_val = JSON_TOKEN_PARSE_ERR;
		goto ErrHandle;
	}
	//��ȡ��һ������(����ֵ)�Ĳ���
    p_tmp_entry = p_tmp_table->head;

	while(p_tmp_entry != NULL)
	{
		if( !sd_strcmp(p_tmp_entry->k, "rtn_code") )
		{
			p_tmp_obj = (json_object*)p_tmp_entry->v;
	    	rtn = json_object_get_int(p_tmp_obj);
			if( SUCCESS != rtn )
			{	
				p_resp->_resp._result = rtn;
				goto ErrHandle;
			}
		}
		else if( !sd_strcmp(p_tmp_entry->k, "is_active_stage") )
		{
			p_tmp_obj = (json_object*)p_tmp_entry->v;
	    	is_active_stage = json_object_get_string(p_tmp_obj);
			if( NULL != is_active_stage)
			{
				if(!sd_strcmp(is_active_stage, "true"))
					p_resp->_resp._is_active_stage = TRUE;
				else
					p_resp->_resp._is_active_stage = FALSE;
			}
		}
		else if( !sd_strcmp(p_tmp_entry->k, "free_play_time") )
		{
			p_tmp_obj = (json_object*)p_tmp_entry->v;
	    	p_resp->_resp._free_play_time = json_object_get_int(p_tmp_obj);
		}
		else if( !sd_strcmp(p_tmp_entry->k, "user_id") )
		{
			p_tmp_obj = (json_object*)p_tmp_entry->v;
	    	p_resp->_resp._user_id = json_object_get_double(p_tmp_obj);
		}
		//��ȡ��һ������
		p_tmp_entry = p_tmp_entry->next;
		if( NULL == p_tmp_entry)  
			break;
	    p_tmp_obj = (json_object*)p_tmp_entry->v;
		if(p_tmp_obj == NULL)
			goto ErrHandle;

	}
	
	lx_json_release(p_new_obj);
	return SUCCESS;
ErrHandle:
	lx_json_release(p_new_obj);
	return ret_val;
}

_int32 lx_build_req_free_strategy(LX_PT_FREE_STRATEGY* p_req)
{
	_int32 ret_val = SUCCESS;

	EM_LOG_DEBUG("lx_build_req_free_strategy" );

	// ����û���Ϣ
	p_req->_action._req_data_len =sd_snprintf(p_req->_action._req_buffer, p_req->_action._req_buffer_len,
	"{\"XL_Android_Free_Strategy\":\"1.0\",\"Command_id\":\"Get_share_strategy_cmd\",\"user_id\":\"%llu\",\"session_id\":\"%s\"}", p_req->_req._userid, p_req->_req._jump_key);

	// �����ݼ�ѹ���Ҽ���
	ret_val = lx_build_http_req_zip_and_aes((LX_PT *)p_req);
	CHECK_VALUE(ret_val);

	return ret_val;
}

_int32 lx_parse_resp_is_free_strategy(LX_PT_FREE_STRATEGY* p_resp)
{
    _int32 ret_val = SUCCESS;
	char * tmp_buf = p_resp->_action._resp_buffer;
	_int32 tmp_len = (_int32)p_resp->_action._resp_data_len;

	char * databuf = NULL;
	_u32 readsize = 0;

	// ��������Ļ����ļ��ж�ȡ��ȫ����������
	if( tmp_len >= MAX_POST_RESP_BUFFER_LEN )
	{
		if( p_resp->_action._file_id == 0)
		{
			if(sd_file_exist(p_resp->_action._file_path))
			{
				ret_val = sd_open_ex(p_resp->_action._file_path, O_FS_RDONLY, &p_resp->_action._file_id);
				CHECK_VALUE(ret_val);
				ret_val = sd_malloc(tmp_len, (void **)&databuf);
				CHECK_VALUE(ret_val);
				sd_memset(databuf, 0x00, tmp_len);

				ret_val = sd_pread(p_resp->_action._file_id, databuf, tmp_len, 0, &readsize);
				CHECK_VALUE(ret_val);

				if( readsize != tmp_len )
					return LXE_READ_FILE_WRONG;
				else
					tmp_buf = databuf;
				sd_close_ex(p_resp->_action._file_id);
				p_resp->_action._file_id = 0;
			}
			else
				return LXE_FILE_NOT_EXIST;
		}
		else
		{
			return -1;
		}
	}
	// ���ܲ��ҽ�ѹ����
	ret_val = lx_parse_req_aes_and_zip((LX_PT *)p_resp, tmp_buf, &tmp_len);
	CHECK_VALUE(ret_val);
	if(ret_val != SUCCESS) 
		return ret_val;
	// ����json��ʽ������
	ret_val = lx_parse_resp_free_strategy(p_resp, tmp_buf);
	CHECK_VALUE(ret_val);
	
	return ret_val;
	
}

_int32 lx_build_req_free_experience_member(LX_PT_FREE_EXPERIENCE_MEMBER* p_req)
{
	_int32 ret_val = SUCCESS;

	EM_LOG_DEBUG("lx_build_req_free_experience_member" );

	// ����û����
	p_req->_action._req_data_len =sd_snprintf(p_req->_action._req_buffer, p_req->_action._req_buffer_len,
	"{\"XL_LocationProtocol\":\"1.0\",\"Command_id\":\"free_days_req\",\"user_id\":\"%llu\"}", p_req->_req._userid);

	// �����ݼ�ѹ���Ҽ���
	ret_val = lx_build_http_req_zip_and_aes((LX_PT *)p_req);
	CHECK_VALUE(ret_val);

	return SUCCESS;
}

_int32 lx_build_req_get_experience_member_remain_time(LX_PT_FREE_EXPERIENCE_MEMBER* p_req)
{
	_int32 ret_val = SUCCESS;
	char peer_id[PEER_ID_SIZE + 4] = {0};
	EM_LOG_DEBUG("lx_build_req_free_experience_member" );

	get_peerid(peer_id, PEER_ID_SIZE);
	// ����û���Ϣ
	p_req->_action._req_data_len =sd_snprintf(p_req->_action._req_buffer, p_req->_action._req_buffer_len,
	"{\"XL_LocationProtocol\":\"1.0\",\"Command_id\":\"get_all_free_days_req\",\"user_id\":\"%llu\",\"peer_id\":\"%s\"}", p_req->_req._userid, peer_id);

	// �����ݼ�ѹ���Ҽ���
	ret_val = lx_build_http_req_zip_and_aes((LX_PT *)p_req);
	CHECK_VALUE(ret_val);

	return SUCCESS;
}

//������ȡ��Ѳ��Ե��ض�json��ʽ����
_int32 lx_parse_resp_free_experience_member(LX_PT_FREE_EXPERIENCE_MEMBER* p_resp, char *tmp_buf)
{
	_int32 ret_val = SUCCESS;
	_int32 rtn = 0;
	char *tmp = NULL;
	LX_JSON_OBJECT *p_new_obj = NULL;
	lh_table *p_tmp_table = NULL;
	LX_JSON_OBJECT *p_tmp_obj = NULL;
	struct lh_entry *p_tmp_entry = NULL;

	ret_val = lx_json_parse( tmp_buf, &p_new_obj);
	if( ret_val != SUCCESS )
		goto ErrHandle;
	//��ȡhash��ṹ����
    p_tmp_table = json_object_get_object(p_new_obj);
    if( NULL == p_tmp_table)
    {
    	ret_val = JSON_TOKEN_PARSE_ERR;
		goto ErrHandle;
	}
	//��ȡ��һ������(����ֵ)�Ĳ���
    p_tmp_entry = p_tmp_table->head;

	while(p_tmp_entry != NULL)
	{
		if( !sd_strcmp(p_tmp_entry->k, "rtn_code") )
		{
			p_tmp_obj = (json_object*)p_tmp_entry->v;
	    	rtn = json_object_get_int(p_tmp_obj);
			// ��ȡ���������ֶ������ж��Ƿ��һ��ʹ��(Э�鶨�ƵĲ�����)
			if( LX_CS_RESP_GET_FREE_DAY == p_resp->_req._type  )
			{	
				p_resp->_resp._is_first_use = rtn;
			}
			else if( LX_CS_RESP_GET_REMAIN_DAY == p_resp->_req._type )
			{
				if( SUCCESS != rtn )
				{
					ret_val = rtn;
					goto ErrHandle;
				}
			}
		}
		else if( !sd_strcmp(p_tmp_entry->k, "free_days") )
		{
			p_tmp_obj = (json_object*)p_tmp_entry->v;
	    	p_resp->_resp._free_time= json_object_get_int(p_tmp_obj);
		}
		else if( !sd_strcmp(p_tmp_entry->k, "is_first_use") )
		{
			p_tmp_obj = (json_object*)p_tmp_entry->v;
	    	p_resp->_resp._is_first_use = json_object_get_int(p_tmp_obj);
		}
		//��ȡ��һ������
		p_tmp_entry = p_tmp_entry->next;
		if( NULL == p_tmp_entry)  /* �ﵽ�ļ�β */
			break;
	    p_tmp_obj = (json_object*)p_tmp_entry->v;
		if(p_tmp_obj == NULL)
			goto ErrHandle;

	}
	
	lx_json_release(p_new_obj);
	return SUCCESS;
ErrHandle:
	lx_json_release(p_new_obj);
	return ret_val;
}

_int32 lx_parse_resp_data_free_experience_member(LX_PT_FREE_EXPERIENCE_MEMBER* p_resp)
{
    _int32 ret_val = SUCCESS;
	char * tmp_buf = p_resp->_action._resp_buffer;
	_int32 tmp_len = (_int32)p_resp->_action._resp_data_len;

	char * databuf = NULL;
	_u32 readsize = 0;

	// ��������Ļ����ļ��ж�ȡ��ȫ����������
	if( tmp_len >= MAX_POST_RESP_BUFFER_LEN )
	{
		if( p_resp->_action._file_id == 0)
		{
			if(sd_file_exist(p_resp->_action._file_path))
			{
				ret_val = sd_open_ex(p_resp->_action._file_path, O_FS_RDONLY, &p_resp->_action._file_id);
				CHECK_VALUE(ret_val);
				ret_val = sd_malloc(tmp_len, (void **)&databuf);
				CHECK_VALUE(ret_val);
				sd_memset(databuf, 0x00, tmp_len);

				ret_val = sd_pread(p_resp->_action._file_id, databuf, tmp_len, 0, &readsize);
				CHECK_VALUE(ret_val);

				if( readsize != tmp_len )
					return LXE_READ_FILE_WRONG;
				else
					tmp_buf = databuf;
				sd_close_ex(p_resp->_action._file_id);
				p_resp->_action._file_id = 0;
			}
			else
				return LXE_FILE_NOT_EXIST;
		}
		else
		{
			return LXE_FILE_NOT_EXIST;
		}
	}
	// ���ܲ��ҽ�ѹ����
	ret_val = lx_parse_req_aes_and_zip((LX_PT *)p_resp, tmp_buf, &tmp_len);
	CHECK_VALUE(ret_val);
	if(ret_val != SUCCESS) 
		return ret_val;
	// ����json��ʽ������
	ret_val = lx_parse_resp_free_experience_member(p_resp, tmp_buf);
	CHECK_VALUE(ret_val);
	
	return ret_val;
	
}

_int32 lx_build_req_get_high_speed_flux(LX_PT_GET_HIGH_SPEED_FLUX* p_req)
{
	_int32 ret_val = SUCCESS;

	EM_LOG_DEBUG("lx_build_req_get_high_speed_flux" );

	/* ����û���Ϣ */
	p_req->_action._req_data_len =sd_snprintf(p_req->_action._req_buffer, p_req->_action._req_buffer_len,
	"{\"XL_LocationProtocol\":\"1.0\",\"Command_id\":\"add_flux_card_req\",\"user_id\":\"%llu\",\"flux\":\"%llu\",\"valid_time\":\"%u\",\"sequence\":\"%u\",\"property\":\"%u\",\"business_flag\":\"%u\"}", 
	p_req->_req._user_id, p_req->_req._flux, p_req->_req._valid_time, GET_HIGH_SPEED_FLUX_SEQUENCE, p_req->_req._property, LX_BUSINESS_TYPE);

	// �����ݼ�ѹ���Ҽ���
	ret_val = lx_build_http_req_zip_and_aes((LX_PT *)p_req);
	CHECK_VALUE(ret_val);

	return ret_val;
}

//
_int32 lx_parse_resp_get_high_speed_flux_result(LX_PT_GET_HIGH_SPEED_FLUX* p_resp, char *tmp_buf)
{
	_int32 ret_val = SUCCESS;
	_int32 rtn = 0;
	_int32 tmp = 0;
	LX_JSON_OBJECT *p_new_obj = NULL;
	lh_table *p_tmp_table = NULL;
	LX_JSON_OBJECT *p_tmp_obj = NULL;
	struct lh_entry *p_tmp_entry = NULL;

	ret_val = lx_json_parse( tmp_buf, &p_new_obj);
	if( ret_val != SUCCESS )
		goto ErrHandle;
	//��ȡhash��ṹ����
    p_tmp_table = json_object_get_object(p_new_obj);
    if( NULL == p_tmp_table)
    {
    	ret_val = JSON_TOKEN_PARSE_ERR;
		goto ErrHandle;
	}
	//��ȡ��һ������(����ֵ)�Ĳ���
    p_tmp_entry = p_tmp_table->head;

	while(p_tmp_entry != NULL)
	{
		if( !sd_strcmp(p_tmp_entry->k, "rtn_code") )
		{
			p_tmp_obj = (json_object*)p_tmp_entry->v;
	    	rtn = json_object_get_int(p_tmp_obj);
			if( SUCCESS != rtn )
			{	
				ret_val= rtn;
				goto ErrHandle;
			}
		}
		else if( !sd_strcmp(p_tmp_entry->k, "sequence") )
		{
			p_tmp_obj = (json_object*)p_tmp_entry->v;
	    	tmp = json_object_get_int(p_tmp_obj);
		}
		//��ȡ��һ������
		p_tmp_entry = p_tmp_entry->next;
		if( NULL == p_tmp_entry)  /* �ﵽ�ļ�β */
			break;
	    p_tmp_obj = (json_object*)p_tmp_entry->v;
		if(p_tmp_obj == NULL)
			goto ErrHandle;
	}
	
	lx_json_release(p_new_obj);
	return SUCCESS;
ErrHandle:
	lx_json_release(p_new_obj);
	return ret_val;
}

_int32 lx_parse_resp_get_high_speed_flux(LX_PT_GET_HIGH_SPEED_FLUX* p_resp)
{
    _int32 ret_val = SUCCESS;
	char * tmp_buf = p_resp->_action._resp_buffer;
	_int32 tmp_len = (_int32)p_resp->_action._resp_data_len;

	char * databuf = NULL;
	_u32 readsize = 0;

	// ��������Ļ����ļ��ж�ȡ��ȫ����������
	if( tmp_len >= MAX_POST_RESP_BUFFER_LEN )
	{
		if( p_resp->_action._file_id == 0)
		{
			if(sd_file_exist(p_resp->_action._file_path))
			{
				ret_val = sd_open_ex(p_resp->_action._file_path, O_FS_RDONLY, &p_resp->_action._file_id);
				CHECK_VALUE(ret_val);
				ret_val = sd_malloc(tmp_len, (void **)&databuf);
				CHECK_VALUE(ret_val);
				sd_memset(databuf, 0x00, tmp_len);

				ret_val = sd_pread(p_resp->_action._file_id, databuf, tmp_len, 0, &readsize);
				CHECK_VALUE(ret_val);

				if( readsize != tmp_len )
					return LXE_READ_FILE_WRONG;
				else
					tmp_buf = databuf;
				sd_close_ex(p_resp->_action._file_id);
				p_resp->_action._file_id = 0;
			}
			else
				return LXE_FILE_NOT_EXIST;
		}
		else
		{
			return -1;
		}
	}
	// ���ܲ��ҽ�ѹ����
	ret_val = lx_parse_req_aes_and_zip((LX_PT *)p_resp, tmp_buf, &tmp_len);
	CHECK_VALUE(ret_val);
	if(ret_val != SUCCESS) 
		return ret_val;
	// ����json��ʽ������
	ret_val = lx_parse_resp_get_high_speed_flux_result(p_resp, tmp_buf);
	
	return ret_val;
	
}
#else
_int32 lx_parse_resp_free_strategy(LX_PT_FREE_STRATEGY* p_resp, char *tmp_buf)
{

}

_int32 lx_build_req_free_strategy(LX_PT_FREE_STRATEGY* p_req)
{
	
}

_int32 lx_parse_resp_is_free_strategy(LX_PT_FREE_STRATEGY* p_resp)
{
  
	
}

_int32 lx_build_req_free_experience_member(LX_PT_FREE_EXPERIENCE_MEMBER* p_req)
{
	
}

_int32 lx_build_req_get_experience_member_remain_time(LX_PT_FREE_EXPERIENCE_MEMBER* p_req)
{
	
}

//������ȡ��Ѳ��Ե��ض�json��ʽ����
_int32 lx_parse_resp_free_experience_member(LX_PT_FREE_EXPERIENCE_MEMBER* p_resp, char *tmp_buf)
{
	
}

_int32 lx_parse_resp_data_free_experience_member(LX_PT_FREE_EXPERIENCE_MEMBER* p_resp)
{
   
	
}

_int32 lx_build_req_get_high_speed_flux(LX_PT_GET_HIGH_SPEED_FLUX* p_req)
{
	
}

//
_int32 lx_parse_resp_get_high_speed_flux_result(LX_PT_GET_HIGH_SPEED_FLUX* p_resp, char *tmp_buf)
{
	
}

_int32 lx_parse_resp_get_high_speed_flux(LX_PT_GET_HIGH_SPEED_FLUX* p_resp)
{
	
}

#endif

#endif /* ENABLE_LIXIAN */


