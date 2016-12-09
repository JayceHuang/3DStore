#if !defined(__LIXIAN_PROTOCOL_H_20111102)
#define __LIXIAN_PROTOCOL_H_20111102
/*----------------------------------------------------------------------------------------------------------
author:		Zeng yuqing
created:	2011/11/02
-----------------------------------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" 
{
#endif

#include "em_common/em_define.h"

#ifdef ENABLE_LIXIAN_ETM

#include "lixian/lixian_interface.h"
#include "lixian/lixian_impl.h"
#include "et_interface/et_interface.h"
#include "xml_service.h"

/************************************************************************/
/*                    STRUCT DEFINE                                     */
/************************************************************************/

#define 	XML_HEADER "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n"
#define LX_PT_GCID_NODE_LEN 64
#define LX_PKG_CLEARTEXT_LEN 12
#define LX_HTTP_HEADER_MAX_LEN 1024
#define LX_ENCODE_PADDING_LEN 16

// �Ʋ���ĿЭ����ؼ��ܷ�ʽ�ļ�����Կ�涨��ֵ
#define CLOUD_PLAY_AES_KEY "xunlei_vip_android_cloudplay"

typedef enum t_lx_xml_node_type
{
	LXX_PROTOVOL=0,
	LXX_COMMAND, 	
	//LXX_REQUEST, 		//���Ӧ��ʱ������֣�������Э����������xml node typ�ģ���Ҳ�������ڴ˴�

	LXX_USERINFO,   //2
	LXX_TASK_LIST,
	LXX_TASK,
	
	LXX_GCID_LIST,  //5
	LXX_GCID,

	LXX_PLAY_URL,   //7
	LXX_TS_URL,
	LXX_MP4_URL,
	LXX_FPFILE_LIST,
    LXX_FILE,

	LXX_DETECT_WEBSITES_TOTAL,  //12
	LXX_DETECT_WEBSITE,
	LXX_DETECT_RULE,
    LXX_DETECT_STRING_START,
    LXX_DETECT_STRING_END,
	LXX_DETECT_DOWNLOAD_URL,
	LXX_DETECT_DOWNLOAD_URL_END,
	LXX_DETECT_NAME,
	LXX_DETECT_NAME_END,
	LXX_DETECT_NAME_APPEND,
	LXX_DETECT_NAME_APPEND_END,
	LXX_DETECT_SUFFIX,
	LXX_DETECT_SUFFIX_END,
	LXX_DETECT_FILE_SIZE,
	LXX_DETECT_FILE_SIZE_END,
	
} LX_XML_NODE_TYPE;


typedef enum t_lx_pt_file_type
{
	LPFT_OTHERS = 99,
	LPFT_RMVB   = 10,
	LPFT_RM     = 11,
	LPFT_AVI    = 12,
	LPFT_MKV    = 13,
	LPFT_WMV    = 14,
	LPFT_MP4    = 15,
	LPFT_3GP	 = 16,
	LPFT_M4V    = 17,
	LPFT_FLV    = 18,
	LPFT_TS     = 19,
	LPFT_XV     = 20,
	LPFT_MOV    = 21,
	LPFT_MPG    = 22,
	LPFT_MPEG   = 23,
	LPFT_ASF    = 24,
	LPFT_SWF    = 25,
	LPFT_XLMV   = 26,
	LPFT_VOB    = 27,
	LPFT_MPE    = 28,
	LPFT_DAT    = 29,
	LPFT_CLPI   = 30,

	LPFT_MP3    = 40,
	LPFT_WMA    = 44,
	LPFT_WAV    = 45,

	LPFT_JPG    = 50,
	LPFT_JPEG   = 51,
	LPFT_GIF    = 52,
	LPFT_PNG    = 53,
	LPFT_BMP    = 54,
	
	LPFT_RAR    = 60,
	LPFT_ZIP    = 61,
	LPFT_ISO    = 62,
	LPFT_IMG    = 63,
	LPFT_MDS    = 64,
	LPFT_MDF    = 65,
	LPFT_GZ     = 66,
	
	LPFT_TXT    = 70,
	LPFT_PPT    = 71,
	LPFT_PPTX   = 72,
	LPFT_XLS    = 73,
	LPFT_XLSX   = 74,
	LPFT_PDF    = 75,
	LPFT_DOC    = 81,
	LPFT_DOCX   = 82,
	LPFT_CPP    = 83,
	LPFT_C      = 84,
	LPFT_H      = 85,

	LPFT_EXE    = 90,
	LPFT_MSI    = 91,
	LPFT_APK    = 92,
	LPFT_TORRENT  = 95,
}LX_PT_FT;

typedef enum t_lx_pt_ds
{
	LPFS_TASK_START = 0,
	LPFS_TASK_PROCESS = 1,
	LPFS_TASK_COMPLETE = 2,
	LPFS_TASK_FAILED = 3,
	LPFS_TASK_BT_FLAG = 4,
	LPFS_TASK_SUSPEND = 5
}LX_PT_DS;

typedef enum t_lx_pt_server_errcode
{
	LPSE_PERMISSION_DENIED = 8,
}LX_PT_S_ERRCODE;

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for other module				        */
/*--------------------------------------------------------------------------*/

_int32 lx_build_req_task_ls(LX_BASE * p_base,LX_PT_TASK_LS * p_req);
_int32 lx_parse_resp_task_list(LX_PT_TASK_LS* p_resp);
_int32 lx_build_req_get_overdue_or_deleted_task_list(LX_PT_OD_OR_DEL_TASK * p_req);
_int32 lx_parse_resp_get_overdue_or_deleted_task_list(LX_PT_OD_OR_DEL_TASK* p_resp);
_int32 lx_build_req_bt_task_ls(LX_BASE * p_base, LX_PT_BT_LS * p_req);
_int32 lx_parse_resp_bt_task_list(LX_PT_BT_LS* p_resp);
_int32 lx_build_req_screenshot(LX_BASE * p_base,LX_PT_SS * p_req);
_int32 lx_parse_resp_screenshot(LX_PT_SS* p_resp);
_int32 lx_build_req_vod_url(LX_BASE * p_base, LX_PT_VOD_URL * p_req);
_int32 lx_parse_resp_vod_url(LX_PT_VOD_URL* p_resp);
_int32 lx_build_req_commit_task(LX_PT_COMMIT_TASK* p_req);
_int32 lx_parse_resp_commit_task(LX_PT_COMMIT_TASK* p_resp);
_int32 lx_build_bt_req_commit_task(LX_PT_COMMIT_BT_TASK* p_req);
_int32 lx_parse_bt_resp_commit_task(LX_PT_COMMIT_BT_TASK* p_resp);
_int32 lx_build_req_delete_task(LX_PT_DELETE_TASK* p_req);
_int32 lx_parse_resp_delete_task(LX_PT_DELETE_TASK* p_resp);
_int32 lx_build_req_delete_tasks(LX_PT_DELETE_TASKS* p_req);
_int32 lx_parse_resp_delete_tasks(LX_PT_DELETE_TASKS* p_resp);
_int32 lx_build_req_miniquery_task(LX_PT_MINIQUERY_TASK* p_req);
_int32 lx_parse_resp_miniquery_task(LX_PT_MINIQUERY_TASK* p_resp);
//_int32 lx_build_req_query_task_info(LX_PT_QUERY_TASK_INFO* p_req);
//_int32 lx_parse_resp_query_task_info(LX_PT_QUERY_TASK_INFO* p_resp);
_int32 lx_build_req_query_bt_task_info(LX_PT_QUERY_TASK_INFO* p_req);
_int32 lx_parse_resp_query_bt_task_info(LX_PT_QUERY_TASK_INFO* p_resp);
_int32 lx_build_req_delay_task(LX_PT_DELAY_TASK* p_req);
_int32 lx_parse_resp_delay_task(LX_PT_DELAY_TASK* p_resp);
_int32 lx_build_req_get_user_info_task(LX_PT_GET_USER_INFO_TASK* p_req);
_int32 lx_parse_resp_get_user_info_task(LX_PT_GET_USER_INFO_TASK* p_resp);

_int32 lx_build_req_query_task_id_list(LX_PT_TASK_ID_LS* p_req);
_int32 lx_parse_resp_query_task_id_list(LX_PT_TASK_ID_LS* p_resp, _u64 *commit_time, _u64 *last_task_id);
_int32 lx_build_req_batch_query_task_info(LX_PT_BATCH_QUERY_TASK_INFO* p_req);
_int32 lx_parse_resp_batch_query_task_info(LX_PT_BATCH_QUERY_TASK_INFO* p_resp);

_int32 lx_parse_detect_regex_xml(LX_PT_GET_REGEX* p_resp);
_int32 lx_parse_detect_string_xml(LX_PT_GET_DETECT_STRING* p_resp);

_int32 lx_build_req_bt_file_take_off_flux_from_high_speed(LX_PT_TAKE_OFF_FLUX* p_req, LX_FILE_INFO* p_file_info);
_int32 lx_build_req_take_off_flux_from_high_speed(LX_PT_TAKE_OFF_FLUX* p_req, LX_TASK_INFO_EX * p_task_info);
_int32 lx_parse_resp_take_off_flux_from_high_speed(LX_PT_TAKE_OFF_FLUX* p_resp);

/*--------------------------------------------------------------------------*/
/*                Internal functions				        */
/*--------------------------------------------------------------------------*/

const char * lx_get_aes_key_without_prefix( char * aes_key);
_int32 lx_pt_file_type_to_file_suffix(_int32 type,char suffix[16]);
_int32 lx_pt_server_errcode_to_em_errcode(_int32 server_errcode);
LX_TASK_STATE lx_pt_download_status_to_task_state_int(_int32 download_status);
LX_TASK_STATE lx_pt_download_status_to_task_state(char * download_status);
_int32 lx_build_req_zip_and_aes(LX_PT * p_req);
_int32 lx_parse_xml_file(LX_PT * p_resp, xml_node_proc node_proc, xml_node_end_proc node_end_proc, xml_attr_proc attr_proc, xml_value_proc value_proc);
_int32 lx_task_list_xml_node_proc(void *user_data, void *parent_node, _int32 parent_type, _int32 depth, char const *node_name, void **node, _int32 *node_type);
_int32 lx_task_list_xml_node_end_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *node_name);
_int32 lx_task_list_xml_attr_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *attr_name, char const *attr_value);
_int32 lx_task_list_xml_value_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *value);
_int32 lx_bt_task_list_xml_node_proc(void *user_data, void *parent_node, _int32 parent_type, _int32 depth, char const *node_name, void **node, _int32 *node_type);
_int32 lx_bt_task_list_xml_node_end_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *node_name);
_int32 lx_bt_task_list_xml_attr_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *attr_name, char const *attr_value);
_int32 lx_bt_task_list_xml_value_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *value);
_int32 lx_screensoht_xml_node_proc(void *user_data, void *parent_node, _int32 parent_type, _int32 depth, char const *node_name, void **node, _int32 *node_type);
_int32 lx_screensoht_xml_node_end_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *node_name);
_int32 lx_screensoht_xml_attr_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *attr_name, char const *attr_value);
_int32 lx_screensoht_xml_value_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *value);
_int32 lx_vod_url_xml_node_proc(void *user_data, void *parent_node, _int32 parent_type, _int32 depth, char const *node_name, void **node, _int32 *node_type);
_int32 lx_vod_url_xml_node_end_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *node_name);
_int32 lx_vod_url_xml_attr_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *attr_name, char const *attr_value);
_int32 lx_vod_url_xml_value_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *value);
_u8 * lx_get_next_need_dl_gcid(LX_PT_SS * p_ss,_u8 * p_current_gcid);
_int32 lx_check_gcid(LX_PT_SS * p_ss,LX_DL_FILE * p_info, char * gcid_str);
_int32 lx_pt_set_header(char **buffer, _int32 *cur_buflen, LX_CMD_HEADER * p_header);
_int32 lx_pt_get_header(char **buffer, _int32 *cur_buflen, LX_CMD_HEADER * p_header);
_int32 lx_aes_encrypt(char* buffer,_u32* len);
_int32 lx_aes_decrypt(char* buffer, _u32* len);
_int32 lx_pt_zlib_uncompress(_u16 compress_flag, char **tmp_buf, _int32 *tmp_len, char **zlib_buf);
void lx_pt_zlib_uncompress_free(char *zlib_buf);

_int32 lx_detect_regex_xml_node_proc(void *user_data, void *parent_node, _int32 parent_type, _int32 depth, char const *node_name, void **node, _int32 *node_type);
_int32 lx_detect_regex_xml_node_end_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *node_name);
_int32 lx_detect_regex_xml_attr_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *attr_name, char const *attr_value);
_int32 lx_detect_regex_xml_value_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *value);
_int32 lx_detect_string_xml_node_proc(void *user_data, void *parent_node, _int32 parent_type, _int32 depth, char const *node_name, void **node, _int32 *node_type);
_int32 lx_detect_string_xml_node_end_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *node_name);
_int32 lx_detect_string_xml_attr_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *attr_name, char const *attr_value);
_int32 lx_detect_string_xml_value_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *value);



_int32 lx_handle_pubnet_url(char * pubnet_url);

#endif /* ENABLE_LIXIAN */

#ifdef __cplusplus
}
#endif


#endif // !defined(__LIXIAN_PROTOCOL_H_20111102)

