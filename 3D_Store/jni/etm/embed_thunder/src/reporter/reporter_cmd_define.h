/*----------------------------------------------------------------------------------------------------------
author:		ZengYuqing
created:	2008/09/20
-----------------------------------------------------------------------------------------------------------*/
#ifndef _REPORTER_CMD_DEFINE_H_
#define _REPORTER_CMD_DEFINE_H_

#ifdef __cplusplus
extern "C" 
{
#endif


#include "utility/define.h"
#include "utility/errcode.h"
#include "utility/list.h"


#define REP_LICENSE_PROTOCOL_VER	1
#define REP_DW_PROTOCOL_VER	54
#define REP_DW_VER_54  
#ifdef UPLOAD_ENABLE
#define REP_RCLIST_PROTOCOL_VER	54
#endif



#define REPORTER_MAX_REC_BUFFER_LEN 1024
#define REPORTER_MAX_REC_DATA_LEN 1024

/*command type*/
#define REPORTER_CMD_HEADER_LEN		12

/* For license*/
#define REPORT_LICENSE_CMD_TYPE	0x01
#define REPORT_LICENSE_RESP_CMD_TYPE	0x02

/* For task status*/
#define REPORT_DW_STAT_CMD_TYPE	3009
#define REPORT_DW_STAT_RESP_CMD_TYPE	3010
#define REPORT_DW_FAIL_CMD_TYPE	3011
#define REPORT_DW_FAIL_RESP_CMD_TYPE	3012

/* For INSERTSRES*/
#define REPORT_INSERTSRES_CMD_TYPE	2005
#define REPORT_INSERTSRES_RESP_CMD_TYPE	2006

#ifdef ENABLE_EMULE
/* For EMULE_INSERTSRES*/
#define REPORT_EMULE_INSERTSRES_CMD_TYPE	5003
#define REPORT_EMULE_INSERTSRES_RESP_CMD_TYPE	5004

/* For EMULE_DL*/
#define REPORT_EMULE_DL_STAT_CMD_TYPE	5005
#define REPORT_EMULE_DL_STAT_RESP_CMD_TYPE	5006
#endif


#ifdef UPLOAD_ENABLE
/* For record list */
#define REPORT_ISRC_ONLINE_CMD_TYPE			51
#define REPORT_ISRC_ONLINE_RESP_CMD_TYPE		52
#define REPORT_RC_LIST_CMD_TYPE				53
#define REPORT_RC_LIST_RESP_CMD_TYPE			54
#define REPORT_INSERT_RC_CMD_TYPE				55
#define REPORT_INSERT_RC_RESP_CMD_TYPE		56
#define REPORT_DELETE_RC_CMD_TYPE			57
#define REPORT_DELETE_RC_RESP_CMD_TYPE		58

#define REPORT_MAX_RECORD_NUM_EVERY_CMD		10

#endif

#ifdef ENABLE_BT
#define REPORT_BT_TASK_DL_STAT_CMD_TYPE			4011
#define REPORT_BT_TASK_DL_STAT_RESP_CMD_TYPE	4012
#define REPORT_BT_DL_STAT_CMD_TYPE				4007
#define REPORT_BT_DL_STAT_RESP_CMD_TYPE			4008
#define REPORT_BT_INSERT_RES_CMD_TYPE			4003
#define REPORT_BT_INSERT_RES_RESP_CMD_TYPE		4004
#endif /* ENABLE_BT */


#ifdef EMBED_REPORT

#define EMB_REPORT_PROTOCOL_VER	1001

//���ݰ�ʹ�õ�ѹ����ʽ
#define REPORTER_CMD_COMPRESS_TYPE_NONE (0)
#define REPORTER_CMD_COMPRESS_TYPE_GZIP (1)

#define CMD_HEADER_LEN (12)
#define CMD_DATA_HEADER_LEN (8)
#define CMD_MUTI_RECORD_HEADER_LEN (CMD_HEADER_LEN+CMD_DATA_HEADER_LEN)


#define EMB_REPORT_COMMON_TASK_DL_STAT_CMD_TYPE		    2101
#define EMB_REPORT_COMMON_TASK_DL_STAT_RESP_CMD_TYPE	2102

#define EMB_REPORT_BT_TASK_DL_STAT_CMD_TYPE		        2103
#define EMB_REPORT_BT_TASK_DL_STAT_RESP_CMD_TYPE	    2104

#define EMB_REPORT_BT_FILE_DL_STAT_CMD_TYPE		        2105
#define EMB_REPORT_BT_FILE_DL_STAT_RESP_CMD_TYPE	    2106

#define EMB_REPORT_COMMON_TASK_DL_FAIL_CMD_TYPE		    2107
#define EMB_REPORT_COMMON_TASK_DL_FAIL_RESP_CMD_TYPE	2108

#define EMB_REPORT_BT_FILE_DL_FAIL_CMD_TYPE		        2109
#define EMB_REPORT_BT_FILE_DL_FAIL_RESP_CMD_TYPE	    2110

#define EMB_REPORT_DNS_ABNORMAL_CMD_TYPE	            3101
#define EMB_REPORT_DNS_ABNORMAL_RESP_CMD_TYPE	        3102

#define EMB_REPORT_ONLINE_PEER_STATE_CMD_TYPE	        4101
#define EMB_REPORT_ONLINE_PEER_STATE_RESP_CMD_TYPE	    4102

#define EMB_REPORT_COMMON_STOP_TASK_CMD_TYPE	        5101
#define EMB_REPORT_COMMON_STOP_TASK_RESP_CMD_TYPE	    5102

#define EMB_REPORT_BT_STOP_TASK_CMD_TYPE	            5103
#define EMB_REPORT_BT_STOP_TASK_RESP_CMD_TYPE	        5104

#define EMB_REPORT_BT_STOP_FILE_CMD_TYPE	            5105
#define EMB_REPORT_BT_STOP_FILE_RESP_CMD_TYPE	        5106

#define EMB_REPORT_EMULE_STOP_TASK_CMD_TYPE	            5107
#define EMB_REPORT_EMULE_STOP_TASK_RESP_CMD_TYPE	    5108

#define EMB_REPORT_BT_NORMAL_CDN_STAT_TYPE				4017
#define EMB_REPORT_BT_NORMAL_CDN_STAT_RESP_TYPE			4018

#define EMB_REPORT_BT_SUBT_NORMAL_CDN_STAT_TYPE			4019
#define EMB_REPORT_BT_SUBT_NORMAL_CDN_STAT_RESP_TYPE	4020

#define EMB_REPORT_EMULE_NORMAL_CDN_STAT_TYPE			5011
#define EMB_REPORT_EMULE_NORMAL_CDN_STAT_RESP_TYPE		5012

#define EMB_REPORT_P2SP_NORMAL_CDN_STAT_TYPE			3045
#define EMB_REPORT_P2SP_NORMAL_CDN_STAT_RESP_TYPE		3046

#define EMB_REPORT_P2SP_DOWNLOAD_STAT_CMD_TYPE          3053
#define EMB_REPORT_P2SP_DOWNLOAD_STAT_RESP_CMD_TYPE     3054

#define EMB_REPORT_TASK_CREATE_STAT_CMD_TYPE			3055
#define EMB_REPORT_TASK_CREATE_STAT_RESP_CMD_TYPE		3056

#define EMB_REPORT_UPLOAD_STAT_CMD_TYPE			3057
#define EMB_REPORT_UPLOAD_STAT_RESP_CMD_TYPE		3058

#define EMB_REPORT_EMULE_DOWNLOAD_STAT_CMD_TYPE         5017
#define EMB_REPORT_EMULE_DOWNLOAD_STAT_RESP_CMD_TYPE    5018

#define EMB_REPORT_BT_DOWNLOAD_STAT_CMD_TYPE            4025
#define EMB_REPORT_BT_DOWNLOAD_STAT_RESP_CMD_TYPE       4026

#define EMB_REPORT_BT_SUBTASK_DOWNLOAD_STAT_CMD_TYPE         4027
#define EMB_REPORT_BT_SUBTASK_DOWNLOAD_STAT_RESP_CMD_TYPE    4028


#if defined(MOBILE_PHONE)
#define EMB_REPORT_MOBILE_USER_ACTION_CMD_TYPE	        9101
#define EMB_REPORT_MOBILE_USER_ACTION_RESP_CMD_TYPE     9102

#define EMB_REPORT_MOBILE_NETWORK_CMD_TYPE	            9103
#define EMB_REPORT_MOBILE_NETWORK_RESP_CMD_TYPE         9104
#endif

#endif /* EMBED_REPORT */

/* url flag for task status*/
#define URL_FLAG_COMMON	0
#define URL_FLAG_THUNDER	1
#define URL_FLAG_BY_URL	0
#define URL_FLAG_BY_CID	2

/* Download url info for task status */
#define DUL_FILE_SIZE_OK	1
#define DUL_QUERY_SHUB_OK	2
#define DUL_ONLY_ORIGIN	4
#define DUL_QUERY_PHUB_OK	8
#define DUL_QUERY_TRACKER_OK	16

/* license response result for license */
#define LICENSE_OK			0
#define LICENSE_FREEZE		21000
#define LICENSE_EXPIRE		21001
#define LICENSE_RECLAIM		21002
#define LICENSE_USED		21003
#define LICENSE_INVALID		21004
#define LICENSE_SERVER_BUSY	21005

/* license response rule for license */
#define LICENSE_DISABLE_P2S	0x00000001
#define LICENSE_DISABLE_P2P	0x00000002
#define LICENSE_DISABLE_BT		0x00000004
#define LICENSE_DISABLE_EMULE	0x00000008
#define LICENSE_DISABLE_VOD	    0x00000010


typedef struct tagREP_CMD_HEADER
{
	_u32	_version;
	_u32	_seq;
	_u32	_cmd_len;
}REP_CMD_HEADER;

/*************************************************************************/
/* Structure for license*/
/*************************************************************************/
	/// licenseЭ��ͷ�ĸ�ʽΪ
/* 
	Э�鱨�� �� ProtocolVersion	+ // Э��汾�� (unsigned __int32)
				SeqNum			+ // �������к� (unsigned __int32)
				BodyLen			+ // ���峤��   (unsigned __int32)
				CMD_ID			+ // �����ʶ   (BYTE��
				Command
*/

//ReportLicense
typedef struct tagREPORT_LICENSE_CMD		
{
	REP_CMD_HEADER _header;
	//_u32	_client_version;
	//_u16	_compress;
	_u8	_cmd_type;	//0x01
	_u8   	 _U8_PACKING_1 ;
	_u16    	_U16_PACKING_1;
	
	_u32	_peerid_size;
	char		_peerid[PEER_ID_SIZE + 1];
	_u8    	_U8_PACKING_2 ;
	_u16    	_U16_PACKING_2;

	_u32	_items_count;
	
	_u32	partner_id_name_len;
	char		partner_id_name[16];//="partner_id";
	_u32	partner_id_len;
	char		partner_id[MAX_PARTNER_ID_LEN];
	//_int32	product_flag;
	_u32	product_flag_name_len;
	char		product_flag_name[16];//"product_flag"
	_u32	product_flag_len;
	char		product_flag[16];//"0x2000"
	_u32	license_name_len;
	char		license_name[8];//"license"
	_u32	license_len;
	char		license[MAX_LICENSE_LEN];
	//_int32	lan_ip;
	_u32	ip_name_len;
	char		ip_name[4]; //"ip"
	_u32	ip_len;
	char		ip[MAX_HOST_NAME_LEN];
	_u32	os_name_len;
	char		os_name[4];//"os"
	_u32	os_len;
	char		os[MAX_OS_LEN];
}REPORT_LICENSE_CMD;


typedef struct tagREPORT_LICENSE_RESP_CMD		
{
	REP_CMD_HEADER _header;
	//_u32	_client_version;
	//_u16	_compress;
	_u8	_cmd_type;	//0x02
	_u8   	 _U8_PACKING_1 ;
	_u16    	_U16_PACKING_1;
	_u32		_result;

	_u32	_items_count;
	
	_u32	report_interval_name_len;
	char		report_interval_name[16];//="report_interval";
	_u32	report_interval_len;
	char		report_interval[16];
	
	_u32	expire_time_name_len;
	char		expire_time_name[16];//="expire_time";
	_u32	expire_time_len;
	char		expire_time[16];
	
	_u32	rule_name_len;
	char		rule_name[8];//="rule";
	_u32	rule_len;
	char  	rule[16];
	
}REPORT_LICENSE_RESP_CMD;

/*************************************************************************/
/*  Structure for task status*/
/*************************************************************************/

//REPORTDWSTAT
typedef struct tagREPORT_DW_STAT_CMD		
{
	REP_CMD_HEADER _header;
	_u32	_client_version;
	_u16	_compress;
	_u16	_cmd_type;	//3009
	_u32	_peerid_size;
	char		_peerid[PEER_ID_SIZE + 1];
	_u8    _U8_PACKING_1 ;
	_u16    _U16_PACKING_1;
	_u32	_cid_size;						
	_u8		_cid[CID_SIZE];				
	_u32	_origin_url_size;
	char		_origin_url[MAX_URL_LEN];
	_u32	_refer_url_size;
	char		_refer_url[MAX_URL_LEN];
	_int32 	url_flag;			//URL��־(�Ƿ�Ѹ��ר����) b0��0����ͨurl ���أ�1��Ѹ��ר�������� b1��0�����Ǹ���cid���أ�1������cid����
	_u64	_file_size;					
	_u64	size_by_server;					
	_u64	size_by_peer;					
	_u64	size_by_other;
	_int32 	fault_block_size;
	_int32 	download_time;
	_u32	file_suffix_len;
	char 	file_suffix[MAX_SUFFIX_LEN];
	_u8		total_server_res;
	_u8		valid_server_res;
	_u8		is_nated;
	_u8    _U8_PACKING_2 ;
	
	_u8		total_N2I_peer_res;
	_u8		valid_N2I_peer_res;
	_u16    _U16_PACKING_2;
	_u64	N2I_Download_bytes1;
	_u64	N2I_Download_bytes2;
	
	_u8		Total_N2N_peer_res;
	_u8		Valid_N2N_peer_res;
	_u16    _U16_PACKING_3;
	_u64	N2N_Download_bytes1;
	_u64	N2N_Download_bytes2;

	_u8		Total_N2SameN_peer_res;
	_u8		Valid_N2SameN_peer_res;
	_u16    _U16_PACKING_4;
	_u64	N2SameN_Download_bytes1;
	_u64	N2SameN_Download_bytes2;

	_u8		Total_I2I_peer_res;
	_u8		Valid_I2I_peer_res;
	_u16    _U16_PACKING_5;
	_u64	I2I_Download_bytes1;
	_u64	I2I_Download_bytes2;

	_u8		Total_I2N_peer_res;
	_u8		Valid_I2N_peer_res;
	_u16    _U16_PACKING_6;
	_u64	I2N_Download_bytes1;
	_u64	I2N_Download_bytes2;

	_int32 	dw_comefrom;			//������Դ(���ID)
	_int32	product_flag;
	_u32	thunder_version_len;
	char		thunder_version[MAX_VERSION_LEN];
	_u32	partner_id_len;
	char		partner_id[MAX_PARTNER_ID_LEN];

	_u64	downloadbytes_from_seed_svr;
	_u64	savebytes_from_seed_svr;

	_u32	_redirect_url_size;
	char		_redirect_url[MAX_URL_LEN];

	_int32	download_ip;
	_int32	lan_ip;
	_int32	limit_upload;
	_int32	download_stat;		//λ������B0:�Ƿ������� B1:�Ƿ�ǿ�Ƴɹ� B2:�Ƿ���±߲� B3:�Ƿ����CIDʽ����

	_u32	_gcid_size;						
	_u8		_gcid[CID_SIZE];				

	_u8		CDN_number;
	_u8    _U8_PACKING_3 ;
	_u16    _U16_PACKING_7;

	_u32	_real_page_url_size;
	char		_real_page_url[MAX_URL_LEN];

	_int32	download_ur_info;		//����URL����Ϣ�� b0���������Ƿ񷵻��ļ���С(1��ʾ����) b1����ѯServer��Դ�Ƿ�ɹ�(1��ʾ�ɹ�) b2���û��Ƿ�ѡ��ֻ��ԭʼ��ַ����(1��ʾ��) b3����ѯphub״̬��1��ʾ�鵽peer��Դ��b4����ѯtracker״̬(1��ʾ�鵽peer��Դ)
	_int32 	recommend_download_stat;  //�ӽ����ã�0��δ�Ƽ�����ʧ�� n(n>0)���Ƽ�����ʧ�ܣ�n�ǵڼ����Ƽ�����ʧ��

	_u32	_user_name_size;
	char		_user_name[MAX_USER_NAME_LEN];
	_u32	_file_name_size;
	char		_file_name[MAX_FILE_NAME_LEN];

	_u64	download_bytes_by_relay;
	_u64	download_bytes_from_DCache;
	_u64	bytes_by_bt;
	_u64	bytes_by_emule;
#ifdef REP_DW_VER_54
	_u32	_task_info_size;
	char		_task_info[MAX_DES_LEN];    //Bencoding����
#endif /* REP_DW_VER_54 */
}REPORT_DW_STAT_CMD;

//REPORTDWFAIL
typedef struct tagREPORT_DW_FAIL_CMD		
{
	REP_CMD_HEADER _header;
	_u32	_client_version;
	_u16	_compress;
	_u16	_cmd_type;	//3011
	_u32	_peerid_size;
	char		_peerid[PEER_ID_SIZE + 1];
	_u8    _U8_PACKING_1 ;
	_u16    _U16_PACKING_1;
	_u32	_url_size;
	char		_url[MAX_URL_LEN];
	_int32 	fail_code;
	_u32	_page_url_size;
	char		_page_url[MAX_URL_LEN];
	_u32	_cid_size;						
	_u8		_cid[CID_SIZE];
	
	_u64	_file_size;	
	
	_int32 	dw_comefrom;		//������Դ
	_int32	product_flag;
	_u32	thunder_version_len;
	char		thunder_version[MAX_VERSION_LEN];
	_u32	partner_id_len;
	char		partner_id[MAX_PARTNER_ID_LEN];

	_u32	_redirect_url_size;
	char		_redirect_url[MAX_URL_LEN];

	_int32	download_ip;
	_int32	lan_ip;
	_int32	limit_upload;
	_int32	download_stat;	//λ������B0:�Ƿ������� B1:�Ƿ�ǿ�Ƴɹ� B3:�Ƿ���±߲�

	_int32	server_rc_num;
	_int32	peer_rc_num;
	_u8 		CDN_number;
	_u8    _U8_PACKING_2 ;
	_u16    _U16_PACKING_2;

	_u32	_statistic_page_url_size;
	char		_statistic_page_url[MAX_URL_LEN];

	_int32	recommend_download_stat;	//�ӽ����ã�0��δ�Ƽ�����ʧ�� n(n>0)���Ƽ�����ʧ�ܣ�n�ǵڼ����Ƽ�����ʧ��
	_int32	download_ratio;


}REPORT_DW_FAIL_CMD;

//INSERTSRES
typedef struct tagREPORT_INSERTSRES_CMD		
{
	REP_CMD_HEADER _header;
	_u32	_client_version;
	_u16	_compress;
	_u32  	_reserver_length;
	char* 	_reserver_buffer;
	_u16	_cmd_type;	//2005
	_u32	_peerid_size;
	char		_peerid[PEER_ID_SIZE + 1];
	_u8    _U8_PACKING_1 ;
	_u16    _U16_PACKING_1;
	
	_u32	_origin_url_size;
	char		_origin_url[MAX_URL_LEN];
	_u32        _origin_url_code_page;
	_u32	_refer_url_size;
	char		_refer_url[MAX_URL_LEN];
	_u32 	_refer_url_code_page;
	_u32	_redirect_url_size;
	char		_redirect_url[MAX_URL_LEN];
	_u32 	_redirect_url_code_page;
	_u64	_file_size;					
	_u32	_cid_size;						
	_u8		_cid[CID_SIZE];				
	_u32	_gcid_size;						
	_u8		_gcid[CID_SIZE];				
	_int32 	_gcid_level;		//ȫ��CID����1��Peer���� 10����Server���� 90��ԭʼURL����
	_int32 	_gcid_part_size;
	_u32	_bcid_size;						
	_u8*	_bcid;							
	_u32	file_suffix_len;
	char 	file_suffix[MAX_SUFFIX_LEN];
	_int32	download_status;  //������Ϊ״̬��(ÿһbit����һ״̬) b0: ԭʼURL���ص��ֽ���Ϊ0
	_u8		have_password;
	_u8    _U8_PACKING_2 ;
	_u16    _U16_PACKING_2;
	
	_int32	download_ip;
	_int32	download_stat; //λ������B0:�Ƿ������� B1:�Ƿ�ǿ�Ƴɹ� B3:�Ƿ���±߲�
	_int32	insert_course;  //����ԭ��λ������ B0:��������CID��Ϣ B1:GCID�������� B2:GCID�����仯��������CID,FileSize�ı䣩B3:δ����BCID B4:δ�����ļ����ͣ���׺��
#ifdef REP_DW_VER_54
	_u32	partner_id_len;
	char		partner_id[MAX_PARTNER_ID_LEN];
	_int32	product_flag;
	_u8		_refer_url_type;   //����ҳ��Դ���� 0��δ֪����ҳ���͡� 1��������ҳ�� 2����ʵ����ҳ�� 3���û�ͨ���½�����ֶ����õ�����ҳ�� 4�����ؿ���û������ҳʱ��������url���ϼ�Ŀ¼����Ϊ����ҳ��
	_u8    _U8_PACKING_3 ;
	_u16    _U16_PACKING_3;
#endif /* REP_DW_VER_54 */
}REPORT_INSERTSRES_CMD;

typedef struct tagREPORT_DW_RESP_CMD		/*p2s protocol version is 50*/
{
	REP_CMD_HEADER _header;
	_u32	_client_version;
	_u16	_compress;
	_u16	_cmd_type;	//3010 or 3012
	_u8		_result;
	_u8   	 _U8_PACKING_1 ;
	_u16    	_U16_PACKING_1;
}REPORT_DW_RESP_CMD;

#ifdef ENABLE_BT
//REPORTBTDOWNLOADSTAT
typedef struct tagREPORT_BT_DL_STAT_CMD		
{
	REP_CMD_HEADER _header;
	_u32	_client_version;
	_u16	_compress;
	_u16	_cmd_type;	//4007
	_u32	_peerid_size;
	char		_peerid[PEER_ID_SIZE + 1];
	_u8    _U8_PACKING_1 ;
	_u16    _U16_PACKING_1;
	
	_u32	_info_id_size;						
	_u8		_info_id[CID_SIZE];				
	_u32 	_file_index;
	_u32	_title_size;
	char		_title[MAX_FILE_NAME_BUFFER_LEN];
	_u64	_file_size;					
	_u32	_cid_size;						
	_u8		_cid[CID_SIZE];	
	_u32	_gcid_size;						
	_u8		_gcid[CID_SIZE];				
	_u32	_file_name_size;
	char		_file_name[MAX_FILE_NAME_BUFFER_LEN];
	_u64	_bt_dl_size;
	_u64	_server_dl_size;					
	_u64	_peer_dl_size;					
	_u32 	_duration;
	_u32       _avg_speed;
	_int32	product_flag;
	_u32	thunder_version_len;
	char		thunder_version[MAX_VERSION_LEN];
	_u32	partner_id_len;
	char		partner_id[MAX_PARTNER_ID_LEN];

	_u64	_file_total_size;					
	_u64	_start_offset;					
	_int32 	_block_size;       // Piece size
	_u64	_data_from_oversi;
	_u64	_bytes_by_emule;   //version 53
#ifdef REP_DW_VER_60
	_u64	_download_bytes_from_dcache;
#endif /* REP_DW_VER_60 */
}REPORT_BT_DL_STAT_CMD;

//REPORTBTTASKDOWNLOADSTAT
typedef struct tagREPORT_BT_TASK_DL_STAT_CMD		
{
	REP_CMD_HEADER _header;
	_u32	_client_version;
	_u16	_compress;
	_u16	_cmd_type;	//4011
	
	_u32	_peerid_size;
	char		_peerid[PEER_ID_SIZE + 1];
	_u8    _U8_PACKING_1 ;
	_u16    _U16_PACKING_1;
	_u32	_info_id_size;						
	_u8		_info_id[CID_SIZE];				
	_u32	_title_size;
	char		_title[MAX_FILE_NAME_BUFFER_LEN];
	_int32 	_file_number;
	_u64	_file_size;					
	_int32 	_block_size;     // Piece size
	_u32 	_duration;
	_u32       _avg_speed;
	_u32	_seed_url_size;
	char 	_seed_url[MAX_URL_LEN];
	_u32	_page_url_size;
	char 	_page_url[MAX_URL_LEN];
	_u32	_statistic_page_url_size;
	char 	_statistic_page_url[MAX_URL_LEN];
	_int32	product_flag;
	_u32	thunder_version_len;
	char		thunder_version[MAX_VERSION_LEN];
	_u32	partner_id_len;
	char		partner_id[MAX_PARTNER_ID_LEN];

	_int32     _download_flag;    //���ر�־(�Ƿ�Ѹ��ר����) 0: ��ͨurl ���� 1: Ѹ��ר��������
#ifdef REP_DW_VER_54
	_int32 	dw_comefrom;      //������Դ
#endif /* REP_DW_VER_54 */

}REPORT_BT_TASK_DL_STAT_CMD;

//INSERTBTRES
typedef struct tagREPORT_INSERTBTRES_CMD		
{
	REP_CMD_HEADER _header;
	_u32	_client_version;
	_u16	_compress;
	_u16	_cmd_type;	//4003
	_u32	_peerid_size;
	char	_peerid[PEER_ID_SIZE + 1];
	_u8    _U8_PACKING_1 ;
	_u16    _U16_PACKING_1;
	
	_u32	_info_id_size;						
	_u8		_info_id[CID_SIZE];				
	_u32 	_file_index;
	_int32	_cource;    //�ϱ�ԭ��b0������������Ϣ��b1��δ����gcid��b2��δ����bcid��b3��cid���㲻ͬ��b4��gcid���㲻ͬ��b5��filesize��ͬ��b6������ԭ��
	_u64	_file_size;					
	_u32	_cid_size;						
	_u8		_cid[CID_SIZE];	
	_u32	_gcid_size;						
	_u8		_gcid[CID_SIZE];	
	_int32	_gcid_level;			//ȫ��CID����1��Peer���� 10����Server���� 90��ԭʼURL����
	_u32	_gcid_part_size;     // bcid block size
	_u32	_bcid_size;						
	_u8*	_bcid;							
	_u64	_file_total_size;					
	_u64	_start_offset;					
	_int32 	_block_size;     // Piece size

#ifdef REP_DW_VER_54
	_u32	partner_id_len;
	char		partner_id[MAX_PARTNER_ID_LEN];
	_int32	product_flag;
#endif /* REP_DW_VER_54 */
}REPORT_INSERTBTRES_CMD;

typedef struct tagBT_REPORT_NORMAL_CDN_STAT 
{
	REP_CMD_HEADER		_header;
	_u16				_cmd_type;	// 4017
	_int32				_stat_type;	// ͳ�����ͣ�0������ϱ���1����ͣ�ϱ�
	_u32				_info_id_size;
	_u8					_info_id[INFO_HASH_LEN]; 
	_u64				_file_size;
	_u32				_user_id_size;
	_u8					_user_id[MAX_USER_NAME_LEN];
	_u32				_peerid_size;
	_u8					_peerid[PEER_ID_SIZE];
	_int32				_other_res_added;	// 0: û���ⲿ��Դ���룬1��������������Դ
	_int32				_normal_cdn_cnt;	// ��ͨcdn��Դ�ĸ���
	_u64				_total_down_bytes;	// ������������ֽ���
	_u64				_normal_cdn_down_bytes;	// ����ͨcdn�����ص��ֽ���
	_int32				_normal_cdn_trans_time;	// ʹ����ͨcdn��ʱ��
	_int32				_normal_cdn_conn_time;	// ������ͨcdn��ʱ��
	_int32				_time_span_from_0_to_nonzero;	// �ӽ����������ٶ�֮��ļ��
	_int32				_time_sum_of_all_zero_period;	// �������������ٶ�Ϊ0������֮��
	_int32				_time_of_download;	// ������ʱ��
	_int32				_query_normal_cdn_fail_times;	// ��ѯ��ͨcdnʧ�ܵĴ���
	_u64				_down_bytes_from_p2sp;	// �������p2sp���������ص��ֽ���
	_int32				_task_magic_id;	
	_u64				_magic_id;
	_u32				_random_id;	// 32λ�������ÿ����ͣƬ����������һ��random id
}BT_REPORT_NORMAL_CDN_STAT;

typedef struct tagBT_REPORT_NORMAL_CDN_STAT_RESP
{
	REP_CMD_HEADER		_header;
	_u8					_cmd_type;
	_u8					_result;
}BT_REPORT_NORMAL_CDN_STAT_RESP;

typedef struct tagBT_REPORT_SUB_TASK_NORMAL_CDN_STAT 
{
	REP_CMD_HEADER		_header;
	_u8					_cmd_type;
	_int32				_stat_type;	// ͳ�����ͣ�0������ϱ���1����ͣ�ϱ�
	_u32				_info_id_size;
	_u8*				_info_id; 
	_int32				_file_index;
	_int32				_file_size;
	_u32				_cid_size;
	_u8*				_cid;
	_u32				_gcid_size;
	_u8*				_gcid;
	_u32				_user_id_size;
	_u8*				_user_id;
	_u32				_peer_id_size;
	_int32				_other_res_added;	// 0: û���ⲿ��Դ���룬1��������������Դ
	_int32				_normal_cdn_cnt;	// ��ͨcdn��Դ�ĸ���
	_u64				_total_down_bytes;	// ������������ֽ���
	_u64				_normal_cdn_down_bytes;	// ����ͨcdn�����ص��ֽ���
	_int32				_normal_cdn_trans_time;	// ʹ����ͨcdn��ʱ��
	_int32				_normal_cdn_conn_time;	// ������ͨcdn��ʱ��
	_int32				_time_sum_of_all_zero_period;	// �������������ٶ�Ϊ0������֮��
	_int32				_time_of_download;	// ������ʱ��
	_int32				_query_normal_cdn_fail_times;	// ��ѯ��ͨcdnʧ�ܵĴ���
	_int32				_task_magic_id;	
	_u32				_random_id;	// 32λ�������ÿ����ͣƬ����������һ��random id
}BT_REPORT_SUB_TASK_NORMAL_CDN_STAT;

#endif /* ENABLE_BT */

#ifdef ENABLE_EMULE
typedef struct tagREPORT_INSERT_EMULE_RES_CMD		
{
	REP_CMD_HEADER _header;
	_u32	_client_version;
	_u16	_compress;
	_u16	_cmd_type;	//5003
	
	_u32	_peerid_size;
	char	_peerid[PEER_ID_SIZE];
    
	_u32	_fileid_len;
	char	_fileid[FILE_ID_SIZE];
    
    _u64	_file_size;					
	_int32	_cource;    //�ϱ�ԭ��b0������������Ϣ��b1��δ����gcid��b2��δ����bcid��b3��cid���㲻ͬ��b4��gcid���㲻ͬ��b5��filesize��ͬ��b6������ԭ��

    _u32	_file_name_len;
	char	_file_name[MAX_FILE_NAME_BUFFER_LEN];

	_u32	_aich_hash_len;
	_u8*	 _aich_hash_ptr;
    
	_u32	 _part_hash_len;
	_u8*	 _part_hash_ptr;
    
    _u32	_cid_size;						
	_u8		_cid[CID_SIZE];
    
	_u32	_gcid_size;						
	_u8		_gcid[CID_SIZE];
    
	_int32	_gcid_level;			//ȫ��CID����1��Peer���� 10����Server���� 90��ԭʼURL����
	_u32	_gcid_part_size;     // bcid block size
	
	_u32	_bcid_size;						
	_u8*	_bcid;
    
	_u32	_md5_size;						
	_u8*	_md5;   
    
#ifdef REP_DW_VER_54
	_u32	_partner_id_len;
	char	_partner_id[MAX_PARTNER_ID_LEN];
	_int32	_product_flag;
#endif /* REP_DW_VER_54 */
}REPORT_INSERT_EMULE_RES_CMD;

typedef struct tagREPORT_EMULE_DL_STAT_CMD		
{
	REP_CMD_HEADER _header;
	_u32	_client_version;
	_u16	_compress;
	_u16	_cmd_type;	//5005  
	
	_u32	_peerid_size;
	char	_peerid[PEER_ID_SIZE];
    
	_u32	_fileid_len;
	char	_fileid[FILE_ID_SIZE];
    
    _u64	_file_size;					

    _u32	_file_name_len;
	char	_file_name[MAX_FILE_NAME_BUFFER_LEN];

	_u32	_aich_hash_len;
	_u8*	 _aich_hash_ptr;
    
	_u32	 _part_hash_len;
	_u8*	 _part_hash_ptr;
    
    _u32	_cid_size;						
	_u8		_cid[CID_SIZE];
    
	_u32	_gcid_size;						
	_u8		_gcid[CID_SIZE];
    
    _u64    _emule_dl_bytes;
    _u64    _server_dl_bytes;
    _u64    _peer_dl_bytes;

	_u32	_duration;
	_u32	_avg_speed;
    
	_int32	_product_flag;
    
	_u32	_thunder_version_len;
	char	_thunder_version[MAX_VERSION_LEN];
    
	_u32	_partner_id_len;
	char	_partner_id[MAX_PARTNER_ID_LEN];

	_u32	_page_url_size;
	char 	_page_url[MAX_URL_LEN];
	_u32	_statistic_page_url_size;
	char 	_statistic_page_url[MAX_URL_LEN];

	_int32  _download_flag;    //���ر�־(�Ƿ�Ѹ��ר����) 0: ��ͨurl ���� 1: Ѹ��ר��������

    _u64    _bt_dl_bytes;
#ifdef REP_DW_VER_54
     _int32  _dw_comefrom;      //������Դ
#endif /* REP_DW_VER_54 */
    _u64    _cdn_dl_bytes;
    _u8     _is_speedup;
	_u32    _band_width;
}REPORT_EMULE_DL_STAT_CMD;

typedef struct tagEMULE_REPORT_NORMAL_CDN_STAT
{
	REP_CMD_HEADER		_header;
	_u16				_cmd_type;	// 4017
	_int32				_stat_type;	// ͳ�����ͣ�0������ϱ���1����ͣ�ϱ�
	_u32				_file_id_size;
	_u8					_file_id[FILE_ID_SIZE];
	_u64				_file_size;
	_u32				_cid_size;
	_u8					_cid[CID_SIZE];
	_u32				_gcid_size;
	_u8					_gcid[CID_SIZE];
	_u32				_user_id_size;
	_u8					_user_id[MAX_USER_NAME_LEN];
	_u32				_peerid_size;
	_u8					_peerid[PEER_ID_SIZE];
	_int32				_other_res_added;	// 0: û���ⲿ��Դ���룬1��������������Դ
	_int32				_normal_cdn_cnt;	// ��ͨcdn��Դ�ĸ���
	_u64				_total_down_bytes;	// ������������ֽ���
	_u64				_normal_cdn_down_bytes;	// ����ͨcdn�����ص��ֽ���
	_int32				_normal_cdn_trans_time;	// ʹ����ͨcdn��ʱ��
	_int32				_normal_cdn_conn_time;	// ������ͨcdn��ʱ��
	_int32				_time_span_from_0_to_nonzero;	// �ӽ����������ٶ�֮��ļ��
	_int32				_time_sum_of_all_zero_period;	// �������������ٶ�Ϊ0������֮��
	_int32				_time_of_download;	// ������ʱ��
	_int32				_query_normal_cdn_fail_times;	// ��ѯ��ͨcdnʧ�ܵĴ���
	_int32				_task_magic_id;	
	_u64				_magic_id;
	_u32				_random_id;	// 32λ�������ÿ����ͣƬ����������һ��random id
}EMULE_REPORT_NORMAL_CDN_STAT;

typedef struct tagBT_REPORT_NORMAL_CDN_STAT_RESP EMULE_REPORT_NORMAL_CDN_STAT_RESP;

#endif /* ENABLE_EMULE */

#ifdef UPLOAD_ENABLE
/*************************************************************************/
/* Structure for record list */
/*************************************************************************/
//isrc_online
typedef struct tagREPORT_RC_ONLINE_CMD		
{
	REP_CMD_HEADER _header;
	//_u32	_client_version;
	//_u16	_compress;
	_u8		_cmd_type;	
	_u8   	 _U8_PACKING_1 ;
	_u16    	_U16_PACKING_1;
	
	_u32	_peerid_size;
	char		_peerid[PEER_ID_SIZE + 1];
	_u8    	_U8_PACKING_2 ;
	_u16    	_U16_PACKING_2;
}REPORT_RC_ONLINE_CMD;


//reportRClist
typedef struct tagREPORT_RC_LIST_CMD		
{
	REP_CMD_HEADER _header;
	//_u32	_client_version;
	//_u16	_compress;
	_u8		_cmd_type;	
	_u8   	 _U8_PACKING_1 ;
	_u16    	_U16_PACKING_1;
	
	_u32	_peerid_size;
	char		_peerid[PEER_ID_SIZE + 1];
	_u8    	_U8_PACKING_2 ;
	_u16    	_U16_PACKING_2;

	LIST * 	_rc_list;
	_u32	_p2p_capability;
	
}REPORT_RC_LIST_CMD;


//insertRC
typedef struct tagREPORT_INSERT_RC_CMD		
{
	REP_CMD_HEADER _header;
	//_u32	_client_version;
	//_u16	_compress;
	_u8	_cmd_type;	
	_u8   	 _U8_PACKING_1 ;
	_u16    	_U16_PACKING_1;
	
	_u32	_peerid_size;
	char		_peerid[PEER_ID_SIZE + 1];
	_u8    	_U8_PACKING_2 ;
	_u16    	_U16_PACKING_2;

	_u32	_cid_size;						
	_u8		_cid[CID_SIZE];
	
	_u64	_file_size;
	
	_u32	_gcid_size;						
	_u8		_gcid[CID_SIZE];
	
	_u32	_p2p_capability;
}REPORT_INSERT_RC_CMD;

//deleteRC
typedef struct tagREPORT_DELETE_RC_CMD		
{
	REP_CMD_HEADER _header;
	//_u32	_client_version;
	//_u16	_compress;
	_u8	_cmd_type;	
	_u8   	 _U8_PACKING_1 ;
	_u16    	_U16_PACKING_1;
	
	_u32	_peerid_size;
	char		_peerid[PEER_ID_SIZE + 1];
	_u8    	_U8_PACKING_2 ;
	_u16    	_U16_PACKING_2;

	_u32	_cid_size;						
	_u8		_cid[CID_SIZE];
	
	_u64	_file_size;
	
	_u32	_gcid_size;						
	_u8		_gcid[CID_SIZE];
	
}REPORT_DELETE_RC_CMD;

/* record report response */
typedef struct tagREPORT_RC_RESP_CMD		
{
	REP_CMD_HEADER _header;
	//_u32	_client_version;
	//_u16	_compress;
	_u8		_cmd_type;

	_u8		_result;
	_u16    	_U16_PACKING_1;
	
	_int32 	should_report_rclist;  /* Just for REPORT_RC_ONLINE_CMD */
}REPORT_RC_RESP_CMD;
#endif

#ifdef EMBED_REPORT

//COMMONTASKDLSTAT	
typedef struct tagEMB_REPORT_COMMON_TASK_DL_STAT_CMD		
{
		REP_CMD_HEADER _header;
		_u32	_cmd_type;	//2101
		
		_u32	_thunder_version_len;
		char	_thunder_version[MAX_VERSION_LEN];

		_u32	_partner_id_len;
		char	_partner_id[MAX_PARTNER_ID_LEN];
		
		_u32	_peerid_len;
		char	_peerid[PEER_ID_SIZE];

/*
		���񴴽�����:
		0: url ������
		1: url �ϵ���������
		2: cid������
		3 cid�ϵ���������
		4 gcid����
*/
		_u32	_task_create_type;	  

		_u32	_url_len;
		char    _url[MAX_URL_LEN];
		
		_u32	_ref_url_len;
		char    _ref_url[MAX_URL_LEN];
		
		_u32	_cid_len;						
		_u8 	_cid[CID_SIZE]; 
	
		_u32	_gcid_len;						
		_u8		_gcid[CID_SIZE];	
		
		_u64	_file_size; 
		_u32	_block_size;
		
		_u32	_file_name_len;
		char	_file_name[MAX_FILE_NAME_BUFFER_LEN];

		_u32	_duration;
		_u32	_avg_speed;

/*
		λ������
		B0:�Ƿ�������
		B1:�Ƿ�ǿ�Ƴɹ�
		B3:�Ƿ���±߲�
*/
		_u32	_download_stat;	  
		
}EMB_REPORT_COMMON_TASK_DL_STAT_CMD;

//BTTASKDLSTAT	
typedef struct tagEMB_REPORT_BT_TASK_DL_STAT_CMD		
{
		REP_CMD_HEADER _header;
		_u32	_cmd_type;	//2103
		
		_u32	_thunder_version_len;
		char	_thunder_version[MAX_VERSION_LEN];

		_u32	_partner_id_len;
		char	_partner_id[MAX_PARTNER_ID_LEN];
		
		_u32	_peerid_len;
		char	_peerid[PEER_ID_SIZE];

		_u32	_info_hash_len;						
		_u8 	_info_hash[CID_SIZE]; 
	
		_u32	_file_title_len;
		char	_file_title[MAX_FILE_NAME_BUFFER_LEN];
		
		_u32	_file_num;
		_u64	_file_total_size; 
		_u32	_piece_size;

		_u32	_duration;
		_u32	_avg_speed;

}EMB_REPORT_BT_TASK_DL_STAT_CMD;

//BTFILEDLSTAT	
typedef struct tagEMB_REPORT_BT_FILE_DL_STAT_CMD		
{
		REP_CMD_HEADER _header;
		_u32	_cmd_type;	
		
		_u32	_thunder_version_len;
		char	_thunder_version[MAX_VERSION_LEN];

		_u32	_partner_id_len;
		char	_partner_id[MAX_PARTNER_ID_LEN];
		
		_u32	_peerid_len;
		char	_peerid[PEER_ID_SIZE];

		_u32	_info_hash_len;						
		_u8 	_info_hash[CID_SIZE]; 
	
		_u32	_file_title_len;
		char	_file_title[MAX_FILE_NAME_BUFFER_LEN];
		
		_u32	_file_num;
		_u64	_file_total_size; 
		_u32	_piece_size;

		_u32    _file_index;
		_u64    _file_offset;

		_u32	_cid_len;						
		_u8 	_cid[CID_SIZE]; 
	
		_u32	_gcid_len;						
		_u8		_gcid[CID_SIZE];	
		
		_u64    _file_size;

		_u32	_file_name_len;
		char	_file_name[MAX_FILE_NAME_BUFFER_LEN];
		
		_u32	_duration;
		_u32	_avg_speed;
		
		/*
				λ������
				B0:�Ƿ�������
				B1:�Ƿ�ǿ�Ƴɹ�
				B3:�Ƿ���±߲�
		*/
		_u32	_download_stat;   

}EMB_REPORT_BT_FILE_DL_STAT_CMD;

//COMMONTASKDLFAIL	
typedef struct tagEMB_REPORT_COMMON_TASK_DL_FAIL_CMD		
{
		REP_CMD_HEADER _header;
		_u32	_cmd_type;	
		
		_u32	_thunder_version_len;
		char	_thunder_version[MAX_VERSION_LEN];

		_u32	_partner_id_len;
		char	_partner_id[MAX_PARTNER_ID_LEN];
		
		_u32	_peerid_len;
		char	_peerid[PEER_ID_SIZE];

		/*
				���񴴽�����:
				0: url ������
				1: url �ϵ���������
				2: cid������
				3 cid�ϵ���������
				4 gcid����
		*/
		_u32	_task_create_type;	

		_u32	_url_len;
		char    _url[MAX_URL_LEN];
		
		_u32	_ref_url_len;
		char    _ref_url[MAX_URL_LEN];
		
		_u32	_cid_len;						
		_u8 	_cid[CID_SIZE]; 
	
		_u32	_gcid_len;						
		_u8		_gcid[CID_SIZE];	
		
		_u64	_file_size; 
		_u32	_block_size;
		
		_u32	_file_name_len;
		char	_file_name[MAX_FILE_NAME_BUFFER_LEN];
		
		_u32    _fail_code;
		_u32    _percent;

		_u32    _ip_len;
		char    _local_ip[MAX_HOST_NAME_LEN];
		
		_u32	_duration;
		_u32	_avg_speed;

/*
		λ������
		B0:�Ƿ�������
		B1:�Ƿ�ǿ�Ƴɹ�
		B3:�Ƿ���±߲�
*/
		_u32	_download_stat;	  
		
}EMB_REPORT_COMMON_TASK_DL_FAIL_CMD;

//BTFILEDLFAIL	
typedef struct tagEMB_REPORT_BT_FILE_DL_FAIL_CMD		
{
		REP_CMD_HEADER _header;
		_u32	_cmd_type;	
		
		_u32	_thunder_version_len;
		char	_thunder_version[MAX_VERSION_LEN];

		_u32	_partner_id_len;
		char	_partner_id[MAX_PARTNER_ID_LEN];
		
		_u32	_peerid_len;
		char	_peerid[PEER_ID_SIZE];

		_u32	_info_hash_len;						
		_u8 	_info_hash[CID_SIZE]; 
	
		_u32	_file_title_len;
		char	_file_title[MAX_FILE_NAME_BUFFER_LEN];
		
		_u32	_file_num;
		_u64	_file_total_size; 
		_u32	_piece_size;

		_u32    _file_index;
		_u64    _file_offset;

		_u32	_cid_len;						
		_u8 	_cid[CID_SIZE]; 

		_u32	_gcid_len;						
		_u8 	_gcid[CID_SIZE]; 

		_u64    _file_size;
		
		_u32	_file_name_len;
		char	_file_name[MAX_FILE_NAME_BUFFER_LEN];

		_u32    _fail_code;
		_u32    _percent;

		_u32    _ip_len;
		char    _local_ip[MAX_HOST_NAME_LEN];
		
		_u32	_duration;
		_u32	_avg_speed;

/*
		λ������
		B0:�Ƿ�������
		B1:�Ƿ�ǿ�Ƴɹ�
		B3:�Ƿ���±߲�
*/
		_u32	_download_stat;	  
		
}EMB_REPORT_BT_FILE_DL_FAIL_CMD;

typedef struct tagEMB_DNS_ABNORMAL_CMD
{
	REP_CMD_HEADER _header;
	_u32	_cmd_type;	
	
	_u32	_thunder_version_len;
	char	_thunder_version[MAX_VERSION_LEN];
	
	_u32	_partner_id_len;
	char	_partner_id[MAX_PARTNER_ID_LEN];
	
	_u32	_peerid_len;
	char	_peerid[PEER_ID_SIZE];
	
	_u32	_err_code;
	
	LIST*   _dns_ip_list_ptr;
	
	_u32   _hub_domain_len;
	char*  _hub_domain_str;

	LIST*  _parse_ip_list_ptr;
	
}EMB_DNS_ABNORMAL_CMD;

typedef struct tagEMB_REPORT_ONLINE_PEER_STATE_CMD
{
	REP_CMD_HEADER _header;
	_u32	_cmd_type;	
	
	_u32	_thunder_version_len;
	char	_thunder_version[MAX_VERSION_LEN];
	
	_u32	_partner_id_len;
	char	_partner_id[MAX_PARTNER_ID_LEN];
	
	_u32	_peerid_len;
	char	_peerid[PEER_ID_SIZE];
	
	_u32	_ip_name_len;
	char	_ip_name[MAX_HOST_NAME_LEN];
	
	_u32	_memory_pool_size;
	
	_u32	_os_name_len;
	char	_os_name[MAX_OS_LEN];
	
	_u32	_download_speed;
	_u32	_download_max_speed;

	_u32	_upload_speed;
	_u32	_upload_max_speed;
	
	_u32	_res_num;
	_u32	_pipe_num;
	
	
}EMB_REPORT_ONLINE_PEER_STATE_CMD;

typedef struct tagEMB_ONLINE_PEER_STAT_RESP_CMD
{
	REP_CMD_HEADER _header;
	_u32	_cmd_type;	
	_u32	_result;
	_u32	_interval;

}EMB_ONLINE_PEER_STAT_RESP_CMD;

typedef struct tagEMB_REPORT_COMMON_STOP_TASK_CMD
{
	REP_CMD_HEADER _header;
	_u32	_cmd_type;	
	
	_u32	_thunder_version_len;
	char	_thunder_version[MAX_VERSION_LEN];
	
	_u32	_partner_id_len;
	char	_partner_id[MAX_PARTNER_ID_LEN];
	
	_u32	_peerid_len;
	char	_peerid[PEER_ID_SIZE];

	/*
			���񴴽�����:
			0: url ������
			1: url �ϵ���������
			2: cid������
			3: cid�ϵ���������
			4: gcid����
			+8: ���������ϵ�4 λ(IPAD KANKAN)
	*/
	_u32	_task_create_type;	  

	_u32	_url_len;
	char	_url[MAX_URL_LEN];
	
	_u32	_ref_url_len;
	char	_ref_url[MAX_URL_LEN];
	
	_u32	_cid_len;						
	_u8 	_cid[CID_SIZE]; 

	_u32	_gcid_len;						
	_u8 	_gcid[CID_SIZE];	
	
	_u64	_file_size; 
	_u32	_block_size;
	
	_u32	_file_name_len;
	char	_file_name[MAX_FILE_NAME_BUFFER_LEN];

	_u32	_duration;
	_u32	_avg_speed;

	_u32    _task_type;
	
	_u32    _vod_play_time;
	_u32    _vod_first_buffer_time;
	
	_u32    _vod_interrupt_times;
	_u32    _min_interrupt_time;
	_u32    _max_interrupt_time;
	_u32    _avg_interrupt_time;
	_u64    _server_dl_bytes;
	_u64    _peer_dl_bytes;
	_u64    _cdn_dl_bytes;
	_u32    _task_status;
	_u32    _failed_code;
    _u32    _dl_percent;
	
///add new
//////////////////vod_report
	_u32 _bit_rate;			        //������
	_u32 _vod_total_buffer_time;    //�ܵĲ��Ż���ʱ�䣨�룩
	_u32 _vod_total_drag_wait_time;	//�����϶���ĵȴ�����ʱ�䣨�룩
	_u32 _vod_drag_num;		        //�϶�����
	_u32 _play_interrupt_1;         // 1�������ж�
	_u32 _play_interrupt_2;	        // 2�������ж�
	_u32 _play_interrupt_3;		    //6�������ж�
	_u32 _play_interrupt_4;		    //10�������ж�
	_u32 _play_interrupt_5;		    //15�������ж�
	_u32 _play_interrupt_6;		    //15���������ж�
	_u32 _cdn_stop_times;		    //�ر�cdn����
	
//////file_info_report

	_u64  _down_exist;					 //�ϵ�����ǰ������ 																
	_u64  _overlap_bytes;				 //�ص����ݴ�С 	
	
	_u64  _down_n2i_no; 				 //�����������Ǳ��±ߴ������ֽ���													
	_u64  _down_n2i_yes;				 //�������������±ߴ������ֽ��� 													
	_u64  _down_n2n_no; 				 //�����������Ǳ��±ߴ������ֽ���	(����ͬ����)												
	_u64  _down_n2n_yes;				 //�������������±ߴ������ֽ��� 	(����ͬ����)												
	_u64  _down_n2s_no; 				 //ͬ���������Ǳ��±ߴ������ֽ���													
	_u64  _down_n2s_yes;				 //ͬ�����������±ߴ������ֽ��� 													
	_u64  _down_i2i_no; 				 //�����������Ǳ��±ߴ������ֽ���													
	_u64  _down_i2i_yes;				 //�������������±ߴ������ֽ��� 													
	_u64  _down_i2n_no; 				 //�����������Ǳ��±ߴ������ֽ���													
	_u64  _down_i2n_yes;				 //�������������±ߴ������ֽ��� 	
	
	_u64  _down_by_partner_cdn; 			 //Cdn�����ֽ�		
	_u64  _down_err_by_cdn; 			 //Cdn�����ֽ�		
	_u64  _down_err_by_partner_cdn; 	//partner Cdn�����ֽ�		
	_u64  _down_err_by_peer;			 //Peer�����ֽ� 																	
	_u64  _down_err_by_svr; 			 //Server�����ֽ�	

	//////cm_report
	_u32  _res_server_total;			 //server��Դ����																	
	_u32  _res_server_valid;			 //��Чserver��Դ����																
	_u32  _res_cdn_total;				 //CDN��Դ����																		
	_u32  _res_cdn_valid;				 //��ЧCDN��Դ����		
	_u32  _res_partner_cdn_total;		//partner_CDN��Դ����																		
	_u32  _res_partner_cdn_valid;		//��Чpartner_CDN��Դ����	
	_u32  _cdn_res_max_num; 			 //ͬʱʹ�����Cdn��Դ����															
	_u32  _cdn_connect_failed_times;	 //Cdn ʹ��ʧ�ܴ��� 	
	
	_u32  _res_n2i_total;				 //���ص�������������������Դ��															
	_u32  _res_n2i_valid;				 //��Ч����������������Դ�� 	
	_u32  _res_n2n_total;				 //���ص�������������������Դ�� (����ͬ����)													
	_u32  _res_n2n_valid;				 //��Ч����������������Դ�� (����ͬ����)														
	_u32  _res_n2s_total;				 //����������ͬ������������Դ�� 													
	_u32  _res_n2s_valid;				 //��Ч��������ͬ������������Դ��													
	_u32  _res_i2i_total;				 //���ص�������������������Դ�� 													
	_u32  _res_i2i_valid;				 //��Ч����������������Դ�� 														
	_u32  _res_i2n_total;				 //������������������Դ��															
	_u32  _res_i2n_valid;				 //��Ч����������������Դ�� 	
	
	_u32  _hub_res_total;					 //���ϵ�Hub��Դ����																
	_u32  _hub_res_valid;					 //���ϵ�Hub��Ч��Դ����															
	_u32  _active_tracker_res_total;	 //���ϵ�Tracker������Դ����														
	_u32  _active_tracker_res_valid;	 //���ϵ�Tracker��Ч������Դ����													

//////res_query_report
	
	_u32  _hub_s_max;					 //Shub��ѯ�����ʱ��(ms)															
	_u32  _hub_s_min;					 //Shub��ѯ����Сʱ��(ms)															
	_u32  _hub_s_avg;					 //Shub��ѯ��ƽ��ʱ��(ms)															
	_u32  _hub_s_succ;					 //Shub��ѯ�ɹ����� 																
	_u32  _hub_s_fail;					 //Shub��ѯʧ�ܴ��� 	
	
	_u32  _hub_p_max;					 //Phub��ѯ�����ʱ��(ms)															
	_u32  _hub_p_min;					 //Phub��ѯ����Сʱ��(ms)															
	_u32  _hub_p_avg;					 //Phub��ѯ��ƽ��ʱ��(ms)															
	_u32  _hub_p_succ;					 //Phub��ѯ�ɹ����� 																
	_u32  _hub_p_fail;					 //Phub��ѯʧ�ܴ��� 	
	
	_u32  _hub_t_max;					 //Tracker��ѯ�����ʱ��(ms) 														
	_u32  _hub_t_min;					 //Tracker��ѯ����Сʱ��(ms) 														
	_u32  _hub_t_avg;					 //Tracker��ѯ��ƽ��ʱ��(ms) 														
	_u32  _hub_t_succ;					 //Tracker��ѯ�ɹ�����																
	_u32  _hub_t_fail;					 //Tracker��ѯʧ�ܴ���																
	_u32  _network_info;				 //��ǰ��������Ϣ
	_u64  _appacc_dl_bytes;			// Ӧ���г����������ֽ���
	_u64  _down_err_by_appacc;			// Ӧ���г��������ش����ֽ���
														   
}EMB_REPORT_COMMON_STOP_TASK_CMD;

typedef struct tagEMB_REPORT_BT_STOP_TASK_CMD
{
	REP_CMD_HEADER _header;
	_u32	_cmd_type;	
	
	_u32	_thunder_version_len;
	char	_thunder_version[MAX_VERSION_LEN];
	
	_u32	_partner_id_len;
	char	_partner_id[MAX_PARTNER_ID_LEN];
	
	_u32	_peerid_len;
	char	_peerid[PEER_ID_SIZE];

	_u32	_info_hash_len; 					
	_u8 	_info_hash[CID_SIZE]; 
	
	_u32	_file_title_len;
	char	_file_title[MAX_FILE_NAME_BUFFER_LEN];
	
	_u32	_file_num;
	_u64	_file_total_size; 
	_u32	_piece_size;
	
	_u32	_duration;
	_u32	_avg_speed;

	_u32    _task_type;
	
	_u64    _server_dl_bytes;
	_u64    _peer_dl_bytes;
	_u64    _cdn_dl_bytes;
	_u32    _task_status;
	_u32    _failed_code;
    _u32    _dl_percent;

	_u32  _network_info;				 //��ǰ��������Ϣ

}EMB_REPORT_BT_STOP_TASK_CMD;

typedef struct tagEMB_REPORT_BT_STOP_FILE_CMD
{
	REP_CMD_HEADER _header;
	_u32	_cmd_type;	
	
	_u32	_thunder_version_len;
	char	_thunder_version[MAX_VERSION_LEN];
	
	_u32	_partner_id_len;
	char	_partner_id[MAX_PARTNER_ID_LEN];
	
	_u32	_peerid_len;
	char	_peerid[PEER_ID_SIZE];

	
	_u32	_info_hash_len; 					
	_u8 	_info_hash[CID_SIZE]; 
	
	_u32	_file_title_len;
	char	_file_title[MAX_FILE_NAME_BUFFER_LEN];
	
	_u32	_file_num;
	_u64	_file_total_size; 
	_u32	_piece_size;
	
	_u32	_file_index;
	_u64	_file_offset;
	
	_u32	_cid_len;						
	_u8 	_cid[CID_SIZE]; 
	
	_u32	_gcid_len;						
	_u8 	_gcid[CID_SIZE];	
	
	_u64	_file_size;
	
	_u32	_file_name_len;
	char	_file_name[MAX_FILE_NAME_BUFFER_LEN];
	
	_u32	_duration;
	_u32	_avg_speed;

	_u32    _task_type;
	
	_u32    _vod_play_time;
	_u32    _vod_first_buffer_time;
	
	_u32    _vod_interrupt_times;
	_u32    _min_interrupt_time;
	_u32    _max_interrupt_time;
	_u32    _avg_interrupt_time;
	_u64    _server_dl_bytes;
	_u64    _peer_dl_bytes;
	_u64    _cdn_dl_bytes;
	_u32    _task_status;
	_u32    _failed_code;
    _u32    _dl_percent;
		
	///add new
	//////////////////vod_report
		_u32 _bit_rate; 				//������
		_u32 _vod_total_buffer_time;	//�ܵĲ��Ż���ʱ�䣨�룩
		_u32 _vod_total_drag_wait_time; //�����϶���ĵȴ�����ʱ�䣨�룩
		_u32 _vod_drag_num; 			//�϶�����
		_u32 _play_interrupt_1; 		// 1�������ж�
		_u32 _play_interrupt_2; 		// 2�������ж�
		_u32 _play_interrupt_3; 		//6�������ж�
		_u32 _play_interrupt_4; 		//10�������ж�
		_u32 _play_interrupt_5; 		//15�������ж�
		_u32 _play_interrupt_6; 		//15���������ж�
		_u32 _cdn_stop_times;			//�ر�cdn����
		
	//////file_info_report
	
		_u64  _down_exist;					 //�ϵ�����ǰ������ 																
		_u64  _overlap_bytes;				 //�ص����ݴ�С 	
		
		_u64  _down_n2i_no; 				 //�����������Ǳ��±ߴ������ֽ���													
		_u64  _down_n2i_yes;				 //�������������±ߴ������ֽ��� 													
		_u64  _down_n2n_no; 				 //�����������Ǳ��±ߴ������ֽ���	(����ͬ����)												
		_u64  _down_n2n_yes;				 //�������������±ߴ������ֽ��� 	(����ͬ����)												
		_u64  _down_n2s_no; 				 //ͬ���������Ǳ��±ߴ������ֽ���													
		_u64  _down_n2s_yes;				 //ͬ�����������±ߴ������ֽ��� 													
		_u64  _down_i2i_no; 				 //�����������Ǳ��±ߴ������ֽ���													
		_u64  _down_i2i_yes;				 //�������������±ߴ������ֽ��� 													
		_u64  _down_i2n_no; 				 //�����������Ǳ��±ߴ������ֽ���													
		_u64  _down_i2n_yes;				 //�������������±ߴ������ֽ��� 	
		
		_u64  _down_by_partner_cdn; 		//Cdn�����ֽ�		
		_u64  _down_err_by_cdn; 			 //Cdn�����ֽ�		
		_u64  _down_err_by_partner_cdn; 	//partner Cdn�����ֽ�		
		_u64  _down_err_by_peer;			 //Peer�����ֽ� 																	
		_u64  _down_err_by_svr; 			 //Server�����ֽ�	
	
		//////cm_report
		_u32  _res_server_total;			 //server��Դ����																	
		_u32  _res_server_valid;			 //��Чserver��Դ����																
		_u32  _res_cdn_total;				 //CDN��Դ����																		
		_u32  _res_cdn_valid;				 //��ЧCDN��Դ����		
		_u32  _res_partner_cdn_total;		//partner_CDN��Դ����																		
		_u32  _res_partner_cdn_valid;		//��Чpartner_CDN��Դ����	
		_u32  _cdn_res_max_num; 			 //ͬʱʹ�����Cdn��Դ����															
		_u32  _cdn_connect_failed_times;	 //Cdn ʹ��ʧ�ܴ��� 	
		
		_u32  _res_n2i_total;				 //���ص�������������������Դ�� 														
		_u32  _res_n2i_valid;				 //��Ч����������������Դ�� 	
		_u32  _res_n2n_total;				 //���ص�������������������Դ�� (����ͬ����)													
		_u32  _res_n2n_valid;				 //��Ч����������������Դ�� (����ͬ����)														
		_u32  _res_n2s_total;				 //����������ͬ������������Դ�� 													
		_u32  _res_n2s_valid;				 //��Ч��������ͬ������������Դ��													
		_u32  _res_i2i_total;				 //���ص�������������������Դ�� 													
		_u32  _res_i2i_valid;				 //��Ч����������������Դ�� 														
		_u32  _res_i2n_total;				 //������������������Դ��															
		_u32  _res_i2n_valid;				 //��Ч����������������Դ�� 	
		
		_u32  _hub_res_total;					 //���ϵ�Hub��Դ����																
		_u32  _hub_res_valid;					 //���ϵ�Hub��Ч��Դ����															
		_u32  _active_tracker_res_total;	 //���ϵ�Tracker������Դ����														
		_u32  _active_tracker_res_valid;	 //���ϵ�Tracker��Ч������Դ����													
	
	//////res_query_report
		
		_u32  _hub_s_max;					 //Shub��ѯ�����ʱ��(ms)															
		_u32  _hub_s_min;					 //Shub��ѯ����Сʱ��(ms)															
		_u32  _hub_s_avg;					 //Shub��ѯ��ƽ��ʱ��(ms)															
		_u32  _hub_s_succ;					 //Shub��ѯ�ɹ����� 																
		_u32  _hub_s_fail;					 //Shub��ѯʧ�ܴ��� 	
		
		_u32  _hub_p_max;					 //Phub��ѯ�����ʱ��(ms)															
		_u32  _hub_p_min;					 //Phub��ѯ����Сʱ��(ms)															
		_u32  _hub_p_avg;					 //Phub��ѯ��ƽ��ʱ��(ms)															
		_u32  _hub_p_succ;					 //Phub��ѯ�ɹ����� 																
		_u32  _hub_p_fail;					 //Phub��ѯʧ�ܴ��� 	
		
		_u32  _hub_t_max;					 //Tracker��ѯ�����ʱ��(ms)														
		_u32  _hub_t_min;					 //Tracker��ѯ����Сʱ��(ms)														
		_u32  _hub_t_avg;					 //Tracker��ѯ��ƽ��ʱ��(ms)														
		_u32  _hub_t_succ;					 //Tracker��ѯ�ɹ�����																
		_u32  _hub_t_fail;					 //Tracker��ѯʧ�ܴ���	

		_u32  _network_info;				 //��ǰ��������Ϣ
}EMB_REPORT_BT_STOP_FILE_CMD;

typedef struct tagEMB_REPORT_EMULE_STOP_TASK_CMD
{
	REP_CMD_HEADER _header;
	_u32	_cmd_type;	
	
	_u32	_thunder_version_len;
	char	_thunder_version[MAX_VERSION_LEN];
	
	_u32	_partner_id_len;
	char	_partner_id[MAX_PARTNER_ID_LEN];
	
	_u32	_peerid_len;
	char	_peerid[PEER_ID_SIZE];
    
	_u32	_fileid_len;
	char	_fileid[FILE_ID_SIZE];

	_u32	 _aich_hash_len;
	_u8*	 _aich_hash_ptr;

	_u32	 _part_hash_len;
	_u8*	 _part_hash_ptr;

	_u32	_cid_size;						
	_u8 	_cid[CID_SIZE]; 
	
	_u32	_gcid_size;						
	_u8 	_gcid[CID_SIZE];	

	_u64	_file_size; 
	
	_u32	_file_name_len;
	char	_file_name[MAX_FILE_NAME_BUFFER_LEN];

	_u32	_duration;
	_u32	_avg_speed;

	_u32    _task_type;
	
	_u32    _vod_play_time;
	_u32    _vod_first_buffer_time;
	
	_u32    _vod_interrupt_times;
	_u32    _min_interrupt_time;
	_u32    _max_interrupt_time;
	_u32    _avg_interrupt_time;
	_u64    _server_dl_bytes;
	_u64    _peer_dl_bytes;
	_u64    _emule_dl_bytes;
	_u64    _cdn_dl_bytes;
	_u32    _task_status;
	_u32    _failed_code;
    _u32    _dl_percent;
	
///add new
//////////////////vod_report
	_u32 _bit_rate;			        //������
	_u32 _vod_total_buffer_time;    //�ܵĲ��Ż���ʱ�䣨�룩
	_u32 _vod_total_drag_wait_time;	//�����϶���ĵȴ�����ʱ�䣨�룩
	_u32 _vod_drag_num;		        //�϶�����
	_u32 _play_interrupt_1;         // 1�������ж�
	_u32 _play_interrupt_2;	        // 2�������ж�
	_u32 _play_interrupt_3;		    //6�������ж�
	_u32 _play_interrupt_4;		    //10�������ж�
	_u32 _play_interrupt_5;		    //15�������ж�
	_u32 _play_interrupt_6;		    //15���������ж�
	_u32 _cdn_stop_times;		    //�ر�cdn����
	
//////file_info_report

	_u64  _down_exist;					 //�ϵ�����ǰ������ 																
	_u64  _overlap_bytes;				 //�ص����ݴ�С 	
	
	_u64  _down_n2i_no; 				 //�����������Ǳ��±ߴ������ֽ���													
	_u64  _down_n2i_yes;				 //�������������±ߴ������ֽ��� 													
	_u64  _down_n2n_no; 				 //�����������Ǳ��±ߴ������ֽ���	(����ͬ����)												
	_u64  _down_n2n_yes;				 //�������������±ߴ������ֽ��� 	(����ͬ����)												
	_u64  _down_n2s_no; 				 //ͬ���������Ǳ��±ߴ������ֽ���													
	_u64  _down_n2s_yes;				 //ͬ�����������±ߴ������ֽ��� 													
	_u64  _down_i2i_no; 				 //�����������Ǳ��±ߴ������ֽ���													
	_u64  _down_i2i_yes;				 //�������������±ߴ������ֽ��� 													
	_u64  _down_i2n_no; 				 //�����������Ǳ��±ߴ������ֽ���													
	_u64  _down_i2n_yes;				 //�������������±ߴ������ֽ��� 	
	
	_u64  _down_by_partner_cdn; 			 //Cdn�����ֽ�		
	_u64  _down_err_by_cdn; 			 //Cdn�����ֽ�		
	_u64  _down_err_by_partner_cdn; 	//partner Cdn�����ֽ�		
	_u64  _down_err_by_peer;			 //Peer�����ֽ� 																	
	_u64  _down_err_by_svr; 			 //Server�����ֽ�	

	//////cm_report
	_u32  _res_server_total;			 //server��Դ����																	
	_u32  _res_server_valid;			 //��Чserver��Դ����																
	_u32  _res_cdn_total;				 //CDN��Դ����																		
	_u32  _res_cdn_valid;				 //��ЧCDN��Դ����		
	_u32  _res_partner_cdn_total;		//partner_CDN��Դ����																		
	_u32  _res_partner_cdn_valid;		//��Чpartner_CDN��Դ����	
	_u32  _cdn_res_max_num; 			 //ͬʱʹ�����Cdn��Դ����															
	_u32  _cdn_connect_failed_times;	 //Cdn ʹ��ʧ�ܴ��� 	
	
	_u32  _res_n2i_total;				 //���ص�������������������Դ��															
	_u32  _res_n2i_valid;				 //��Ч����������������Դ�� 	
	_u32  _res_n2n_total;				 //���ص�������������������Դ�� (����ͬ����)													
	_u32  _res_n2n_valid;				 //��Ч����������������Դ�� (����ͬ����)														
	_u32  _res_n2s_total;				 //����������ͬ������������Դ�� 													
	_u32  _res_n2s_valid;				 //��Ч��������ͬ������������Դ��													
	_u32  _res_i2i_total;				 //���ص�������������������Դ�� 													
	_u32  _res_i2i_valid;				 //��Ч����������������Դ�� 														
	_u32  _res_i2n_total;				 //������������������Դ��															
	_u32  _res_i2n_valid;				 //��Ч����������������Դ�� 	
	
	_u32  _hub_res_total;					 //���ϵ�Hub��Դ����																
	_u32  _hub_res_valid;					 //���ϵ�Hub��Ч��Դ����															
	_u32  _active_tracker_res_total;	 //���ϵ�Tracker������Դ����														
	_u32  _active_tracker_res_valid;	 //���ϵ�Tracker��Ч������Դ����													

//////res_query_report
	
	_u32  _hub_s_max;					 //Shub��ѯ�����ʱ��(ms)															
	_u32  _hub_s_min;					 //Shub��ѯ����Сʱ��(ms)															
	_u32  _hub_s_avg;					 //Shub��ѯ��ƽ��ʱ��(ms)															
	_u32  _hub_s_succ;					 //Shub��ѯ�ɹ����� 																
	_u32  _hub_s_fail;					 //Shub��ѯʧ�ܴ��� 	
	
	_u32  _hub_p_max;					 //Phub��ѯ�����ʱ��(ms)															
	_u32  _hub_p_min;					 //Phub��ѯ����Сʱ��(ms)															
	_u32  _hub_p_avg;					 //Phub��ѯ��ƽ��ʱ��(ms)															
	_u32  _hub_p_succ;					 //Phub��ѯ�ɹ����� 																
	_u32  _hub_p_fail;					 //Phub��ѯʧ�ܴ��� 	
	
	_u32  _hub_t_max;					 //Tracker��ѯ�����ʱ��(ms) 														
	_u32  _hub_t_min;					 //Tracker��ѯ����Сʱ��(ms) 														
	_u32  _hub_t_avg;					 //Tracker��ѯ��ƽ��ʱ��(ms) 														
	_u32  _hub_t_succ;					 //Tracker��ѯ�ɹ�����																
	_u32  _hub_t_fail;					 //Tracker��ѯʧ�ܴ���																
	
	_u32  _network_info;				 //��ǰ��������Ϣ														   
}EMB_REPORT_EMULE_STOP_TASK_CMD;

typedef struct tagEMB_COMMON_RESP_CMD
{
	REP_CMD_HEADER _header;
	_u32	_cmd_type;	
	_u32	_result;

}EMB_COMMON_RESP_CMD;

typedef struct tagP2SP_REPORT_NORMAL_CDN_STAT
{
	REP_CMD_HEADER		_header;
	_u16				_cmd_type;	// 3045
	_int32				_stat_type;	// ͳ�����ͣ�0������ϱ���1����ͣ�ϱ�
	_u64				_file_size;
	_u32				_cid_size;
	_u8					_cid[CID_SIZE];
	_u32				_gcid_size;
	_u8					_gcid[CID_SIZE];
	_u32				_user_id_size;
	_u8					_user_id[MAX_USER_NAME_LEN];
	_u32				_peerid_size;
	_u8					_peerid[PEER_ID_SIZE];
	_int32				_other_res_added;	// 0: û���ⲿ��Դ���룬1��������������Դ
	_int32				_normal_cdn_cnt;	// ��ͨcdn��Դ�ĸ���
	_u64				_total_down_bytes;	// ������������ֽ���
	_u64				_normal_cdn_down_bytes;	// ����ͨcdn�����ص��ֽ���
	_int32				_normal_cdn_trans_time;	// ʹ����ͨcdn��ʱ��
	_int32				_normal_cdn_conn_time;	// ������ͨcdn��ʱ��
	_int32				_time_span_from_0_to_nonzero;	// �ӽ����������ٶ�֮��ļ��
	_int32				_time_sum_of_all_zero_period;	// �������������ٶ�Ϊ0������֮��
	_int32				_time_of_download;	// ������ʱ��
	_int32				_query_normal_cdn_fail_times;	// ��ѯ��ͨcdnʧ�ܵĴ���
	_int32				_task_magic_id;	
	_u32				_url_size;
	_u8					_url[MAX_URL_LEN];
	_u64				_magic_id;
	_u32				_random_id;	// 32λ�������ÿ����ͣƬ����������һ��random id
}P2SP_REPORT_NORMAL_CDN_STAT;

typedef struct tagBT_REPORT_NORMAL_CDN_STAT_RESP P2SP_REPORT_NORMAL_CDN_STAT_RESP;

struct tagREPORT_TASK_COMMON_STAT
{   /*ʱ��-ָͨ����1970��������� ; ʱ��,ָ��ʱ��ļ��*/
    _int32  _task_id; /*����id*/
    _int32  _create_time;/*���񴴽���ʱ��*/
	
    _int32  _start_time; /*����������ʱ��*/
    _int32  _finish_time;/*��ͣ,�ɹ���ʧ�ܵ�ʱ��*/

    _int32  _task_status;/*0-��ͣ,1-�ɹ�,2-ʧ��*/	
    _int32  _fail_code;	 /*ʧ��ԭ��*/

	_u32	_peerid_len;
	char	_peerid[PEER_ID_SIZE];
	
	_u32	_thunder_version_len;
	char	_thunder_version[MAX_VERSION_LEN];
	
	_u32	_partner_id_len;
	char	_partner_id[MAX_PARTNER_ID_LEN];

	_int32  _product_flag;
	
    _int32  _avg_speed;		/* Bytes/s*/
    _u32    _first_zero_speed_duration;/*����ʱ��*/
    _u32    _other_zero_speed_duraiton;/*����ʱ��*/
    _int32  _percent100_duration;/*�յ�ȫ����Ч���ݵ�ʱ��*/
    _int32  _percent; /*�յ���Ч���ݵĽ���,ȡֵ[0,100] */
};

struct tagREPORT_STAT_HEADER
{
	REP_CMD_HEADER		_header;
	_u32	_client_version;
	_u16	_compress;
	_u16	_cmd_type;
};

typedef struct tagEMB_REPORT_TASK_P2SP_STAT
{   
    struct tagREPORT_TASK_COMMON_STAT _common;

	_u32	_url_len;
	char	_url[MAX_URL_LEN];
	_u32	_ref_url_len;
	char	_ref_url[MAX_URL_LEN];
	_int32  _task_create_type;
	_u32	_cid_size;				/*���û��,������0*/		
	_u8 	_cid[CID_SIZE]; 
	_u32	_gcid_size;				/*���û��,������0*/				
	_u8 	_gcid[CID_SIZE];	
	_u64	_file_size; 
	_u32	_file_name_len;
	char	_file_name[MAX_FILE_NAME_BUFFER_LEN];
	
	_u64    _origin_bytes;
	_u64    _other_server_bytes;
	_u64    _peer_dl_bytes;
	_u64    _speedup_cdn_bytes;
	_u64    _lixian_cdn_bytes;
	_u64    _zero_cdn_dl_bytes;	
	
	_int32  _zero_cdn_use_time;/*����cdnʹ��ʱ��*/
	_int32  _zero_cdn_count;/*����cdn������Դ��Ŀ*/
    _int32  _zero_cdn_valid_count;  /* ��Ч������cdn ��Դ��Ŀ*/
    _int32  _first_use_zero_cdn_duration; /* ��һ��ʹ������cdn ��������ʼʱ��*/
}EMB_REPORT_TASK_P2SP_STAT;

typedef struct tagEMB_REPORT_TASK_EMULE_STAT
{   
    struct tagREPORT_TASK_COMMON_STAT _common;

	_u32	_fileid_len;
	char	_fileid[FILE_ID_SIZE];
	_u32	_cid_size;						
	_u8 	_cid[CID_SIZE];
	_u32	_gcid_size; 					
	_u8 	_gcid[CID_SIZE];
	_u64	_file_size; 		
	
	_u64	_emule_dl_bytes;
	_u64	_server_dl_bytes;
	_u64	_peer_dl_bytes;
	_u64	_speedup_cdn_bytes;
	_u64    _lixian_cdn_bytes;
	_u64    _zero_cdn_dl_bytes;	
	
	_int32  _zero_cdn_use_time;/*����cdnʹ��ʱ��*/
	_int32  _zero_cdn_count;/*����cdn������Դ��Ŀ*/
    _int32  _zero_cdn_valid_count;  /* ��Ч������cdn ��Դ��Ŀ*/
    _int32  _first_use_zero_cdn_duration; /* ��һ��ʹ������cdn ��������ʼʱ��*/
}EMB_REPORT_TASK_EMULE_STAT;

typedef struct tagEMB_REPORT_TASK_BT_STAT
{   
    struct tagREPORT_TASK_COMMON_STAT _common;

	_u32	_info_id_size;						
	_u8		_info_id[INFO_HASH_LEN];				
	_int32 	_file_number;/*���ļ���Ŀ*/
	_u64	_file_total_size; 				
	_int32  _piece_size;
	
	_u64	_server_dl_bytes;
	_u64	_bt_dl_bytes;
	_u64	_speedup_cdn_bytes;
	_u64    _lixian_cdn_bytes;
	_u64	_peer_dl_bytes;
	_u64    _zero_cdn_dl_bytes;
	
	_int32  _zero_cdn_use_time;/*����cdnʹ��ʱ��*/
	_int32  _zero_cdn_count;/*����cdn������Դ��Ŀ*/
    _int32  _zero_cdn_valid_count;  /* ��Ч������cdn ��Դ��Ŀ*/
    _int32  _first_use_zero_cdn_duration; /* ��һ��ʹ������cdn ��������ʼʱ��*/
}EMB_REPORT_TASK_BT_STAT;

typedef struct tagEMB_REPORT_TASK_BT_FILE_STAT
{   
    struct tagREPORT_TASK_COMMON_STAT _common;

	_u32	_info_id_size;						
	_u8		_info_id[INFO_HASH_LEN];				
	_int32 	_file_index;
	_u32	_cid_size;						
	_u8 	_cid[CID_SIZE];
	_u32	_gcid_size; 					
	_u8 	_gcid[CID_SIZE];
	_u64	_file_size; 		
	

	_u64	_server_dl_bytes;
	_u64	_bt_dl_bytes;
	_u64	_speedup_cdn_bytes;
	_u64    _lixian_cdn_bytes;
	_u64	_peer_dl_bytes;
	_u64    _zero_cdn_dl_bytes;

	_int32	_zero_cdn_use_time;/*����cdnʹ��ʱ��*/
	_int32	_zero_cdn_count;/*����cdn������Դ��Ŀ*/
    _int32  _zero_cdn_valid_count;
    _int32  _first_use_zero_cdn_duration;
}EMB_REPORT_TASK_BT_FILE_STAT;

typedef struct tagEMB_REPORT_TASK_CREATE_STAT
{
	_u32	_task_id;  // etm���ý������ϲ��id
	_u32	_create_time;
	_u32    _file_index;
	
	_int32  _task_status;/*3-��������*/	
	_u32	_peerid_len;
	char	_peerid[PEER_ID_SIZE];

	_u32	_thunder_version_len;
	char	_thunder_version[MAX_VERSION_LEN];

	_u32	_partner_id_len;
	char	_partner_id[MAX_PARTNER_ID_LEN];

	_int32	_product_flag;
	_u32    _task_type;

	_u32	_create_info_len;
	_u8	_create_info[MAX_URL_LEN];
} EMB_REPORT_TASK_CREATE_STAT;

typedef struct tagEMB_REPORT_UPLOAD_STAT
{
	_u32	_peerid_len;
	char	_peerid[PEER_ID_SIZE];

	_u32	_thunder_version_len;
	char	_thunder_version[MAX_VERSION_LEN];

	_u32	_partner_id_len;
	char	_partner_id[MAX_PARTNER_ID_LEN];

	_int32	_product_flag;

	_u32	_begin_time;/*ͳ�ƿ�ʼʱ��*/
	_u32    _end_time;/*ͳ�ƽ���ʱ��*/
	
	_u32    _peer_capacity;
	_u32    _up_duration;/*�豸����ʱ��*/

	_u32	_up_use_duration;/*�ϴ���ʱ*/
	_u64    _up_data_bytes;/*�ϴ��������ֽ���*/
	_u32    _up_pipe_num;/*�½�����������*/
	_u32    _up_passive_pipe_num;/*�½��ı�����������*/
} EMB_REPORT_UPLOAD_STAT;
#endif

#ifdef __cplusplus
}
#endif


#endif /* _REPORTER_CMD_DEFINE_H_ */
