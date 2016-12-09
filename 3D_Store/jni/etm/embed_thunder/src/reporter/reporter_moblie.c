//Name : reporter_mobile.c

#if defined(MOBILE_PHONE)

#include "reporter_mobile.h"
#include "platform/sd_gz.h"
#include "utility/define_const_num.h"
#include "utility/bytebuffer.h"
#include "utility/string.h"
#include "utility/errcode.h"
#include "utility/time.h"
#include "utility/local_ip.h"
#include "utility/version.h"
#include "utility/peerid.h"
#include "utility/mempool.h"
#include "platform/sd_fs.h"
#include "reporter_cmd_define.h"
#include "reporter_cmd_builder.h"
#include "reporter_impl.h"
#include "task_manager/task_manager.h"
#include "platform/sd_network.h"
#include "embed_thunder_version.h"

#include "utility/logid.h"
#define  LOGID LOGID_REPORTER
#include "utility/logger.h"

REPORTER_DEVICE	* get_embed_hub();
_int32 reporter_commit_cmd(REPORTER_DEVICE* device, _u16 cmd_type, 
		const char* buffer, _u32 len, void* user_data, _u32 cmd_seq);

#define REPORTER_MAX_CMD_SIZE (2048)
#define REPORT_PROTOCOL_VER_SOLE_RECORD (1)
#define REPORT_PROTOCOL_VER_MUTI_RECORD (1000)
#define REPORT_PROTOCOL_VER REPORT_PROTOCOL_VER_MUTI_RECORD
#define REPORT_MAX_RECORD_COUNT (10)


#define REPORT_MOBILE_USER_ACTION_CMD_TYPE	        (EMB_REPORT_MOBILE_USER_ACTION_CMD_TYPE)
#define REPORT_MOBILE_USER_ACTION_RESP_CMD_TYPE     (EMB_REPORT_MOBILE_USER_ACTION_RESP_CMD_TYPE)

#define REPORT_MOBILE_NETWORK_CMD_TYPE	            (EMB_REPORT_MOBILE_NETWORK_CMD_TYPE)
#define REPORT_MOBILE_NETWORK_RESP_CMD_TYPE         (EMB_REPORT_MOBILE_NETWORK_RESP_CMD_TYPE)

// user action
#define REPORTER_FILE_ACTION "uac"

// REPORTER_FILE_ACTION 文件最大值。
#define REPORTER_ACTION_FILE_SIZE_MAX (1024 * 1000)

// REPORTER_FILE_ACTION 文件中最开始的整数大小(用于记录已上报的位置、下次上报位置)。(=sizeof(_u64)*2)
#define REPORTER_ACTION_FILE_POS_LEN (16)


typedef struct tagREPORT_CMD_HEADER
{
	_u32 _version;
	_u32 _seq;
	_u32 _cmd_len;
}REPORT_CMD_HEADER;

typedef struct tagREPORT_MOBILE_ACTION_CMD
{
	REPORT_CMD_HEADER _header;
	_u32 _cmd_type;	
	
	_u32 _thunder_version_len;
	char _thunder_version[MAX_VERSION_LEN];
	
	_u32 _peerid_len;
	char _peerid[PEER_ID_SIZE];
	
	_u32 _time_stamp;
	_u32 _action_value;
	_u32 _action_type;
	char *_p_data;
	_u32 _data_len;
	_u32 _partner_id; 
}REPORT_MOBILE_ACTION_CMD;

typedef struct tagREPORT_MOBILE_NETWORK_CMD
{
	REPORT_CMD_HEADER _header;
	_u32 _cmd_type;	
	
	_u32 _thunder_version_len;
	char _thunder_version[MAX_VERSION_LEN];
	
	_u32 _peerid_len;
	char _peerid[PEER_ID_SIZE];
	
	_u32 _time_stamp;
	_u32 _ip;

	_u32 _connect_type;
	_u64 _mobile_number_1;
	_u64 _mobile_number_2;
}REPORT_MOBILE_NETWORK_CMD;

/*****************************************************************************/

static char g_cmd_buffer[REPORTER_MAX_CMD_SIZE];

static char g_reporter_dir[MAX_FILE_PATH_LEN];

static char g_ui_version[MAX_VERSION_LEN] = "0.0.0";
static _int32 g_product = 0;  //产品编号
static _int32 g_partner_id = 0;
/*****************************************************************************/


_int32 get_ui_version(char *buffer, _int32 bufsize)
{
	//都用完整的版本号
	get_version(buffer, bufsize);
	return SUCCESS;
}

static _int32 get_action_filename(char *filename, _u32 size)
{
	if (filename == NULL || size > MAX_FILE_PATH_LEN)
	{
		return -1;
	}
	sd_strncpy(filename, g_reporter_dir, sd_strlen(g_reporter_dir) + 1);
	sd_strcat(filename, REPORTER_FILE_ACTION, sd_strlen(REPORTER_FILE_ACTION));
	return SUCCESS;
}

// 设置ver.
_int32 reporter_set_version(const char *ver, _int32 size,_int32 product,_int32 partner_id)
{
	_int32 ret_val = SUCCESS;
	char os_name[MAX_OS_LEN];
	char filename[MAX_FILE_PATH_LEN];

	g_product = product;
//	g_partner_id = partner_id;
	sd_memset(g_ui_version, 0, MAX_VERSION_LEN);
	size = (size < MAX_VERSION_LEN - 1)? size: MAX_VERSION_LEN - 1;
	ret_val = sd_get_os(os_name, MAX_OS_LEN); 
	CHECK_VALUE(ret_val);
	sd_snprintf(g_ui_version,MAX_VERSION_LEN-1,"%d_%s@%s",product,ver,os_name);
	set_et_version(g_ui_version);
	get_action_filename(filename, MAX_FILE_PATH_LEN);
	/*if (! sd_file_exist(filename))
	{
		reporter_mobile_user_action_to_file(REPORTER_USER_ACTION_TYPE_LOGIN, 1, NULL, 0);
	}
		
	reporter_mobile_user_action_to_file(REPORTER_USER_ACTION_TYPE_LOGIN, 2, NULL, 0);*/

	return SUCCESS;
}

_int32 reporter_new_install(BOOL is_install)
{
	char filename[MAX_FILE_PATH_LEN];
	get_action_filename(filename, MAX_FILE_PATH_LEN);

	if (! sd_file_exist(filename) || is_install)
	{
		//全新安装或者覆盖安装
		reporter_mobile_user_action_to_file(REPORTER_USER_ACTION_TYPE_LOGIN, 1, NULL, 0);
	}
		
	reporter_mobile_user_action_to_file(REPORTER_USER_ACTION_TYPE_LOGIN, 2, NULL, 0);
}

_int32 reporter_init(const char *record_dir, _u32 record_dir_len)
{
	if (record_dir_len > MAX_FILE_PATH_LEN - 20)
	{
		return -1;
	}
	sd_strncpy(g_reporter_dir, record_dir, record_dir_len);
	if (g_reporter_dir[record_dir_len - 1] != 0x2F || g_reporter_dir[record_dir_len - 1] != 0x5C)
	{
		g_reporter_dir[record_dir_len] = DIR_SPLIT_CHAR;
		g_reporter_dir[record_dir_len + 1] = 0;
	}
	return SUCCESS;
}

_int32 reporter_uninit()
{
	if(sd_strlen(g_reporter_dir)!=0)
	{
		reporter_mobile_user_action_to_file(REPORTER_USER_ACTION_TYPE_LOGIN, 3, NULL, 0);
	}
	return SUCCESS;
}

_int32 reporter_get_peerid(char *peer_id, _int32 size)
{
	// peer id 此时已经初始化，可直接从下载库中获得。
	return get_peerid(peer_id, size);
}

_int32 reporter_set_peerid(char *peer_id, _u32 size)
{
	return set_peerid(peer_id, size);
}

static _int32 reporter_build_mobile_user_action_cmd_by_one_record(char** buffer, _u32* len, REPORT_MOBILE_ACTION_CMD* cmd)
{
	char*	tmp_buf;
	_int32  tmp_len,fix_len=0,other_len=0;
	//文件里面保存记录格式使用老版本，以便兼容已有记录
	cmd->_header._version = REPORT_PROTOCOL_VER_SOLE_RECORD;
	
	/* begining from _client_version */
	fix_len=0
	+4		//_u32	_cmd_type;	
	+4		//_u32	thunder_version_len;
	+4		//_u32	_peerid_size;
	+4		//_u32	_time_stamp;
	+4		//_u32	_action_value;
	+4		//_u32	_action_type;
	+4		//_u32	_data_size;
	+4		//_u32 _partner_id;
	;

	other_len=0
	+ cmd->_thunder_version_len
	+ cmd->_peerid_len
	+ cmd->_data_len
	;

	sd_memset(g_cmd_buffer,0,sizeof(g_cmd_buffer));

	cmd->_header._cmd_len = fix_len + other_len;
	cmd->_cmd_type = REPORT_MOBILE_USER_ACTION_CMD_TYPE;
	*len = sizeof(REPORT_CMD_HEADER) + cmd->_header._cmd_len;
	if(sizeof(g_cmd_buffer)<=*len)
	{
		sd_assert(FALSE);
		return -1;
	}
	*buffer = g_cmd_buffer;
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cmd_type);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_thunder_version_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_thunder_version, (_int32)cmd->_thunder_version_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_len);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_time_stamp);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_action_value);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_action_type);

	if (cmd->_p_data != NULL && cmd->_data_len > 0)
	{
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_data_len);
		sd_set_bytes(&tmp_buf, &tmp_len, cmd->_p_data, (_int32)cmd->_data_len);
	}
	else
	{
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, 0);
	}
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_partner_id);
	sd_assert( tmp_len == 0 );
	if(tmp_len != 0)
	{
		return -1;
	}
	return SUCCESS;
}

static _int32 reporter_build_mobile_network_cmd(char** buffer, _u32* len, REPORT_MOBILE_NETWORK_CMD* cmd)
{
	_int32 ret = SUCCESS;
	char*	tmp_buf;
	_int32	tmp_len,fix_len=0,other_len=0;
	
	_u32 zip_type = REPORTER_CMD_COMPRESS_TYPE_NONE;
	_u8 gzed_buf[1024];
	_u32 gzed_len = 1024;
	
	cmd->_header._version = REPORT_PROTOCOL_VER;
	cmd->_header._seq = reporter_get_seq();

	sd_memset(g_cmd_buffer,0,sizeof(g_cmd_buffer));
	sd_memset(gzed_buf,0,sizeof(gzed_buf));
	
	/* begining from _client_version */
	fix_len=0
	+4		//_u32	_record_count; 条数
	+4		//_u32	_zip_type;	压缩方式
	+4		//_u32	_record_len;	单条记录长度
	+4		//_u32	_cmd_type;	
	+4		//_u32	thunder_version_len;
	+4		//_u32	_peerid_size;
	+4		//_u32	_time_stamp;
	+4		//_u32	_ip;
	+4		//_u32	_connect_type;
	+8		//_u64  _mobile_number_1;
	+8		//_u64	_mobile_number_2;
	;

	other_len = 0
	+ cmd->_thunder_version_len
	+ cmd->_peerid_len
	;

	cmd->_header._cmd_len = fix_len+other_len;
	cmd->_cmd_type = REPORT_MOBILE_NETWORK_CMD_TYPE;
	*len = sizeof(REPORT_CMD_HEADER) + cmd->_header._cmd_len ;
	if(sizeof(g_cmd_buffer)<=*len)
	{
		sd_assert(FALSE);
		return -1;
	}
	*buffer = g_cmd_buffer;
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);//压缩后重写长度

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)1); //条数
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)zip_type); //压缩类型
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)(cmd->_header._cmd_len-12));//单条记录长度

	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cmd_type);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_thunder_version_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_thunder_version, (_int32)cmd->_thunder_version_len);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_len);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_time_stamp);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_ip);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_connect_type);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_mobile_number_1);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_mobile_number_2);

	sd_assert( tmp_len == 0 );
	if(tmp_len != 0)
	{
		return -1;
	}

	//使用gz压缩数据 
	tmp_buf = *buffer+CMD_MUTI_RECORD_HEADER_LEN;//指向协议内容的第3字节，从这里开始压缩
	tmp_len = cmd->_header._cmd_len-CMD_DATA_HEADER_LEN;//需要压缩内容的长度(CMD_DATA_HEADER_LEN为len of record_count&gzip_type)
	 
	ret = sd_gz_encode_buffer((_u8 *)tmp_buf, (_u32)tmp_len, gzed_buf, 1024, &gzed_len);
	//仅压缩成功，且有效果，才使用压缩
	if((SUCCESS == ret)&&(gzed_len < tmp_len))
	{
		LOG_DEBUG("succ gz encode buffer, len_s=%u, len_e=%u", tmp_len, gzed_len);
		zip_type = REPORTER_CMD_COMPRESS_TYPE_GZIP;
		sd_memcpy(tmp_buf, gzed_buf, gzed_len);
		*len = *len - tmp_len + gzed_len;//新的buffer长度

		// 重填充数据头:<协议长度>  (条数)(压缩方式)  12字节
		tmp_buf = *buffer+8;//指向<协议长度>
		tmp_len = 12;
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)gzed_len+CMD_DATA_HEADER_LEN);//len (条数、压缩方式)=8
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)1);
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)zip_type);
	}
	return SUCCESS;
}

_int32 reporter_mobile_user_action_to_file(_u32 action_type, _u32 action_value, void *data, _u32 data_len)
{
	char *buffer = NULL;
	_u32 buf_len = 0;
	REPORT_MOBILE_ACTION_CMD cmd;
	_int32 ret_val = SUCCESS;
	_u32 file_id = 0;
	_u64 filesize = 0;
	_u32 writesize = 0, readsize = 0;
	char filename[MAX_FILE_PATH_LEN];
	char buffer_pos[REPORTER_ACTION_FILE_POS_LEN];
	char *buffer_pos_ptr = NULL;
	_int32 buffer_pos_len = 0;

	//LOG_DEBUG("reporter_mobile_user_action_to_file \"%s\", action=%d.", filename, action);
	sd_memset(&cmd, 0, sizeof(REPORT_MOBILE_ACTION_CMD));
	
	/* 迅雷版本号 */
	ret_val = get_ui_version(cmd._thunder_version, MAX_VERSION_LEN-1); 
	if(ret_val!=SUCCESS) return ret_val;
	cmd._thunder_version_len = sd_strlen(cmd._thunder_version);

	/* Peer ID */
	// 暂时不获取peerid，用全'0'表示。
	cmd._peerid_len = PEER_ID_SIZE;
	sd_memset(cmd._peerid, 0, PEER_ID_SIZE);

	sd_time(&cmd._time_stamp);
	cmd._action_value = action_value;
	cmd._action_type = action_type;
	cmd._p_data = data;
	cmd._data_len = data_len;
	cmd._partner_id = g_partner_id;

	/*build command*/
	ret_val = reporter_build_mobile_user_action_cmd_by_one_record(&buffer, &buf_len, &cmd);
	CHECK_VALUE(ret_val);

	// 将上报内容写到文件。
	ret_val = get_action_filename(filename, MAX_FILE_PATH_LEN);
	CHECK_VALUE(ret_val);

	ret_val = sd_open_ex(filename, O_FS_WRONLY | O_FS_CREATE, &file_id);
	CHECK_VALUE(ret_val);

	ret_val = sd_filesize(file_id, &filesize);
	CHECK_VALUE(ret_val);

	if (filesize >= REPORTER_ACTION_FILE_SIZE_MAX)
	{
		// 文件超过一定大小，不再写入。
		sd_close_ex(file_id);
		return END_OF_FILE;
	}
	
	if (filesize <= REPORTER_ACTION_FILE_POS_LEN)
	{
		//非文件尾写文件超出文件长度可能出问题，先enlarge
		if (filesize < REPORTER_ACTION_FILE_POS_LEN)
		{
			// 扩充到文件大小为 REPORTER_ACTION_FILE_POS_LEN。
			ret_val = sd_enlarge_file(file_id, REPORTER_ACTION_FILE_POS_LEN, &filesize);
			CHECK_VALUE(ret_val);
		}

		// 将文件开始的 pos 置为 REPORTER_ACTION_FILE_POS_LEN。
		buffer_pos_ptr = buffer_pos;
		buffer_pos_len = REPORTER_ACTION_FILE_POS_LEN;
		sd_set_int64_to_lt(&buffer_pos_ptr, &buffer_pos_len, REPORTER_ACTION_FILE_POS_LEN);
		sd_set_int64_to_lt(&buffer_pos_ptr, &buffer_pos_len, 0);

		ret_val = sd_pwrite(file_id, buffer_pos, REPORTER_ACTION_FILE_POS_LEN, 0, &writesize);
		CHECK_VALUE(ret_val);
	}

	// 将buffer添加到文件尾。
	ret_val = sd_pwrite(file_id, buffer, buf_len, filesize, &writesize);
	CHECK_VALUE(ret_val);

	ret_val = sd_close_ex(file_id);
	CHECK_VALUE(ret_val);

	
	LOG_DEBUG("reporter_mobile_user_action_to_file writesize=%d", writesize);
	return SUCCESS;
}

_int32 reporter_set_version_handle(void * _param)
{
	TM_POST_PARA_3* _p_param = (TM_POST_PARA_3*)_param;
	_int32 ret = SUCCESS;	
	char* ui_version = (char*)_p_param->_para1;
	_int32 product = (_int32)_p_param->_para2;
	_int32 partner_id = (_int32)_p_param->_para3;
	_int32 size = sd_strlen(ui_version);
	set_partner_id(partner_id);
	ret = reporter_set_version(ui_version, size, product,partner_id);
	
	_p_param->_result = ret;
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 reporter_new_install_handle(void * _param)
{
	TM_POST_PARA_1* _p_param = (TM_POST_PARA_1*)_param;
	_int32 ret = SUCCESS;	
	BOOL is_install = (BOOL)_p_param->_para1;

	ret = reporter_new_install(is_install);
	
	_p_param->_result = ret;
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 reporter_mobile_user_action_to_file_handle( void * _param )
{
	TM_POST_PARA_4* _p_param = (TM_POST_PARA_4*)_param;
	_int32 ret = SUCCESS;	
	_u32 action_type = (_u32)_p_param->_para1;
	_u32 action_value = (_u32)_p_param->_para2;
	void* data = (void*)_p_param->_para3;
	_u32 data_len = (_u32)_p_param->_para4;

	ret = reporter_mobile_user_action_to_file(action_type, action_value, data, data_len);
	
	_p_param->_result = ret;
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 reporter_mobile_enable_user_action_report_handle(void * _param)
{
	TM_POST_PARA_1* _p_param = (TM_POST_PARA_1*)_param;
	_int32 ret = SUCCESS;	
	BOOL is_enable = (BOOL)_p_param->_para1;
	
	ret = reporter_set_enable_file_report(is_enable);
	
	_p_param->_result = ret;
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 reporter_mobile_network(void)
{
	char *buffer = NULL, *buffer_malloc = NULL;
	char buffer_http_header[1024];
	_u32 data_len = 0, header_len = 0;
	REPORT_MOBILE_NETWORK_CMD cmd;
	_int32 ret_val = SUCCESS;

	sd_memset(&cmd, 0, sizeof(REPORT_MOBILE_NETWORK_CMD));

	/* 版本号 */
	ret_val = get_ui_version(cmd._thunder_version, MAX_VERSION_LEN-1); 
	if(ret_val!=SUCCESS) return ret_val;
	cmd._thunder_version_len = sd_strlen(cmd._thunder_version);

	/* Peer ID */
	cmd._peerid_len = PEER_ID_SIZE;
	ret_val = reporter_get_peerid(cmd._peerid, PEER_ID_SIZE);
	CHECK_VALUE(ret_val);

	sd_time(&cmd._time_stamp);
	cmd._ip = sd_get_local_ip();
	cmd._connect_type = sd_get_net_type();
	cmd._mobile_number_1 = 0;
	cmd._mobile_number_2 = 0;
	
	/*build command*/
	ret_val = reporter_build_mobile_network_cmd(&buffer, &data_len, &cmd);
	CHECK_VALUE(ret_val);

	header_len = 1024;
	ret_val = build_report_http_header(buffer_http_header, &header_len, data_len, REPORTER_EMBED);
	CHECK_VALUE(ret_val);

	ret_val = sd_malloc(header_len + data_len, (void**)&buffer_malloc);
	CHECK_VALUE(ret_val);

	sd_memcpy(buffer_malloc, buffer_http_header, header_len);
	sd_memcpy(buffer_malloc + header_len, buffer, data_len);

	reporter_commit_cmd(get_embed_hub(), REPORT_MOBILE_NETWORK_CMD_TYPE, (const char*)buffer_malloc, header_len + data_len,
		NULL, cmd._header._seq);

	return SUCCESS;
}

/**
  *从文件中读取一条UAC cmd，按照命令MOBILE_USER_ACTION_STAT格式
  *写入到pp_buffer指定位置，pp_buffer、p_buffer_len会变化
  *
  *@para:
  * peer_id用户标识符， peer_id_len用户标识符长度
  * file_id 文件句柄，file_size文件大小
  * cmd_pos_in_file 文件中的记录偏移量
  * pp_buffer消息缓冲区指针 ，p_cmd_len剩余缓冲区的长度
  *
  *@return: 0-SUCCESS else-FAILURE
**/
_int32 reporter_fill_one_record_from_file(char *peer_id, _u32 peer_id_len,_u32 file_id, _u64 file_size,
		_u64 cmd_pos_in_file, char ** pp_buffer, _int32 *p_buffer_len)
{
	_int32 ret = 0;
	_u32 readsize = 0;
	_u32 cmd_body_len = 0;
	char cmd_header_buf[CMD_HEADER_LEN];
	char * tmp_buffer = NULL;
	_int32 tmp_buffer_len = 0;
	_int32 cmd_type = 0;

	if (file_size == cmd_pos_in_file)
	{
		// 已经读完文件，没有更多的内容
		
		LOG_DEBUG("reporter_fill_one_record_from_file: no more data have to report.");
		return SUCCESS;
	}

	if (cmd_pos_in_file < REPORTER_ACTION_FILE_POS_LEN || file_size < cmd_pos_in_file || file_size - cmd_pos_in_file <= CMD_HEADER_LEN)
	{
		// 读取位置不对或文件内容不足。
		LOG_DEBUG("reporter_fill_one_record_from_file: cmd_pos_in_file invalid");
		return ERROR_DATA_IS_WRITTING;
	}
	//读取包头字段
	ret = sd_pread(file_id, cmd_header_buf, CMD_HEADER_LEN, cmd_pos_in_file, &readsize);
	CHECK_VALUE(ret);
	
	if(readsize != CMD_HEADER_LEN)
		return ERROR_DATA_IS_WRITTING;

	tmp_buffer = cmd_header_buf + 8;
	tmp_buffer_len = readsize - 8;
//读取命令长度
	ret = sd_get_int32_from_lt(&tmp_buffer, &tmp_buffer_len, (_int32 *)&cmd_body_len);
	if(ret!=SUCCESS) return ret;

	if ( cmd_body_len > (file_size - cmd_pos_in_file -CMD_HEADER_LEN))
	{
		// cmd_body_len 太大，文件有错误
		LOG_DEBUG("reporter_fill_one_record_from_file: cmd_body_len invalid.");
		return ERROR_DATA_IS_WRITTING;
	}
	if (cmd_body_len+4 > (*p_buffer_len))
	{
		// 如果添加该记录则超出包长限制
		
		if(cmd_body_len > REPORTER_MAX_CMD_SIZE - 24) // 24=协议头12+数据头8+记录长度4
		{
			// 单条记录长度就已经超出限制，说明文件有错误
			LOG_DEBUG("reporter_fill_one_record_from_file: error, single cmd_body_len=%u", cmd_body_len);
			return ERROR_DATA_IS_WRITTING;
		}
		return OUT_OF_MEMORY;
	}
	
	// 开始填充缓冲区:长度
	ret = sd_memcpy(*pp_buffer, cmd_header_buf+8, 4);//直接cp网络序
	CHECK_VALUE(ret);
	*pp_buffer += 4;
	*p_buffer_len -= 4;
	
	// 填充缓冲区:内容
	ret = sd_pread(file_id, *pp_buffer , cmd_body_len, cmd_pos_in_file + CMD_HEADER_LEN, &readsize);
	CHECK_VALUE(ret);
	
	// UAC命令之前没有获取到 peer_id，在这里添加。
	tmp_buffer = *pp_buffer ;
	tmp_buffer_len = readsize;
	
	ret = sd_get_int32_from_lt(&tmp_buffer, &tmp_buffer_len, &cmd_type);
	if(ret!=SUCCESS) return ret;

	if (REPORT_MOBILE_USER_ACTION_CMD_TYPE == cmd_type)
	{
		_u32 ver_len = 0;
		ret = sd_get_int32_from_lt(&tmp_buffer, &tmp_buffer_len, (_int32 *)&ver_len);
		if(ret!=SUCCESS) return ret;

		// skip ver.
		tmp_buffer += ver_len;
		tmp_buffer_len -= ver_len;

		ret = sd_set_int32_to_lt(&tmp_buffer, &tmp_buffer_len, peer_id_len);
		if(ret!=SUCCESS) return ret;

		ret = sd_set_bytes(&tmp_buffer, &tmp_buffer_len, peer_id, peer_id_len);
		if(ret!=SUCCESS) return ret;
	}
	
	*pp_buffer += cmd_body_len;
	*p_buffer_len -= cmd_body_len;
	
	
	LOG_DEBUG("reporter_fill_one_record_from_file succ, len=%u", cmd_body_len+4);
	return SUCCESS;
}

/**
  *从文件中读取并发送UAC
  *1检查文件格式
  *2打包发送消息
  *@return: 0-SUCCESS else-FAILURE
**/
_int32 reporter_from_file()
{
	_int32 ret = 0;
	_u32 file_id = 0;
	_u64 filesize = 0;
	_u32 readsize = 0, writesize = 0;
	_u64 cmd_pos_in_file = 0;
	_u64 next_cmd_pos_in_file = 0;
	_u32 seq = 0;
	_u32 cmd_len = 0;//整个命令字长度，含协议头部
	_u32 zip_type = REPORTER_CMD_COMPRESS_TYPE_NONE;//命令的压缩类型，默认无压缩
	_int32 record_count = 0;//保存当前包含的记录条数
	char peer_id[PEER_ID_SIZE];
	char filename[MAX_FILE_PATH_LEN];
	char buffer_pos[REPORTER_ACTION_FILE_POS_LEN];
	char *buffer_ptr = NULL;
	_int32 buffer_len = 0;
	_int32 per_buffer_len = 0;

	char buffer_http_header[1024];
	_u32 header_len = 0;
	char * buffer_malloc = NULL;

	ret = reporter_get_peerid(peer_id, PEER_ID_SIZE);
	CHECK_VALUE(ret);
	
	ret = get_action_filename(filename, MAX_FILE_PATH_LEN);
	CHECK_VALUE(ret);

	ret = sd_open_ex(filename, O_FS_RDWR|O_FS_CREATE, &file_id);
	CHECK_VALUE(ret);
	
	ret = sd_filesize(file_id, &filesize);
	CHECK_VALUE(ret);

	if(filesize==0)
	{
		sd_close_ex(file_id);
		return SUCCESS;
	}
	
	ret = sd_pread(file_id, buffer_pos, REPORTER_ACTION_FILE_POS_LEN, 0, &readsize);
	CHECK_VALUE(ret);

	if(readsize==0)
	{
		sd_close_ex(file_id);
		return SUCCESS;
	}
	
	//读取当前偏移位置和未确定偏移
	buffer_ptr = buffer_pos;
	buffer_len = readsize;
	ret = sd_get_int64_from_lt(&buffer_ptr, &buffer_len, (_int64 *)&cmd_pos_in_file);
	CHECK_VALUE(ret);
	ret = sd_get_int64_from_lt(&buffer_ptr, &buffer_len, (_int64 *)&next_cmd_pos_in_file);
	CHECK_VALUE(ret);
	// 处理next_cmd_pos_in_file
	if(next_cmd_pos_in_file > cmd_pos_in_file)
	{
		//上一个发送操作未完成? 后面将从cmd_pos_in_file开始重发
		LOG_DEBUG("last report fail, next_pos=%llu, pos=%llu.", next_cmd_pos_in_file, cmd_pos_in_file);
	}
	//填写协议头的<协议版本><序列号> 
	sd_memset(g_cmd_buffer,0,sizeof(g_cmd_buffer));
	buffer_ptr = g_cmd_buffer;
	buffer_len = REPORTER_MAX_CMD_SIZE;
	
	ret = sd_set_int32_to_lt(&buffer_ptr, &buffer_len, REPORT_PROTOCOL_VER_MUTI_RECORD);
	CHECK_VALUE(ret);
	seq = reporter_get_seq();
	ret = sd_set_int32_to_lt(&buffer_ptr, &buffer_len, seq);
	CHECK_VALUE(ret);
	//预留数据头:<协议长度>  (条数)(压缩方式)
	buffer_ptr += 12;
	buffer_len -= 12;
	
	LOG_DEBUG("start read record from file. pos_in_file=%u",cmd_pos_in_file);
	next_cmd_pos_in_file = cmd_pos_in_file;
	//填写REPORT_MAX_RECORD_COUNT条记录
	do
	{
		per_buffer_len = buffer_len;
		ret = reporter_fill_one_record_from_file(peer_id, PEER_ID_SIZE,	file_id, filesize, 
				next_cmd_pos_in_file, &buffer_ptr, &buffer_len);
		
		if((OUT_OF_MEMORY == ret)||( SUCCESS == ret && per_buffer_len == buffer_len))
		{
			//超出包长限制或者到达文件结尾，不再添加记录，直接发送已添加记录
			break;
		}
		else if(ret != SUCCESS ) 
		{
			//文件格式错误，重置文件格式。
			LOG_DEBUG("reporter_from_file: file format error, reset it.");
			if (filesize > REPORTER_ACTION_FILE_POS_LEN)
			{
				ret = sd_close_ex(file_id);
				CHECK_VALUE(ret);
				
				ret = sd_delete_file(filename);
				CHECK_VALUE(ret);
				
				ret = sd_open_ex(filename, O_FS_RDWR | O_FS_CREATE, &file_id);
				CHECK_VALUE(ret);
				
				ret = sd_filesize(file_id, &filesize);
				CHECK_VALUE(ret);
			}

			ret = sd_enlarge_file(file_id, REPORTER_ACTION_FILE_POS_LEN, &filesize);
			CHECK_VALUE(ret);

			cmd_pos_in_file = REPORTER_ACTION_FILE_POS_LEN;
			next_cmd_pos_in_file = 0;

			buffer_ptr = buffer_pos;
			buffer_len = REPORTER_ACTION_FILE_POS_LEN;
			ret = sd_set_int64_to_lt(&buffer_ptr, &buffer_len, cmd_pos_in_file);
			CHECK_VALUE(ret);
			ret = sd_set_int64_to_lt(&buffer_ptr, &buffer_len, next_cmd_pos_in_file);
			CHECK_VALUE(ret);
			
			ret = sd_pwrite(file_id, buffer_pos, REPORTER_ACTION_FILE_POS_LEN, 0, &writesize);
			CHECK_VALUE(ret);

			//不再继续添加记录，直接发送已添加记录
			break;

		}
		
		//成功读取并添加新纪录
		record_count++;
		next_cmd_pos_in_file += per_buffer_len - buffer_len - 4 + CMD_HEADER_LEN;
		LOG_DEBUG("succ read one record, next_pos=%u", next_cmd_pos_in_file);
	}while(record_count < REPORT_MAX_RECORD_COUNT);
	
	// 计算当前命令长度
	cmd_len = REPORTER_MAX_CMD_SIZE - buffer_len ;
	
	// 记录下次上报位置到文件中
	if (next_cmd_pos_in_file > cmd_pos_in_file)//否则未成功读取到记录
	{
		sd_assert(record_count>0);
		
		buffer_ptr = buffer_pos + 8;
		buffer_len = REPORTER_ACTION_FILE_POS_LEN/2;
		ret = sd_set_int64_to_lt(&buffer_ptr, &buffer_len, next_cmd_pos_in_file);
		CHECK_VALUE(ret);
				
		ret = sd_pwrite(file_id, buffer_pos + 8, REPORTER_ACTION_FILE_POS_LEN/2, REPORTER_ACTION_FILE_POS_LEN/2, &writesize);
		CHECK_VALUE(ret);
	}
	
	sd_close_ex(file_id);

	if( 0 == record_count)
	{
		// 无记录
		LOG_DEBUG("reporter_from_file: no record in file.");
		return SUCCESS;
	}

	//使用gz压缩数据 ,暂借用buffer_http_header,header_len
	ret = sd_gz_encode_buffer((_u8 *)(g_cmd_buffer+CMD_MUTI_RECORD_HEADER_LEN), cmd_len-CMD_MUTI_RECORD_HEADER_LEN, (_u8 *)buffer_http_header, 1024, &header_len);
	//仅压缩成功，且有效果，才使用压缩
	if((SUCCESS == ret)&&(header_len < cmd_len-CMD_MUTI_RECORD_HEADER_LEN))
	{
		LOG_DEBUG("succ gz encode buffer, len_s=%u, len_e=%u", cmd_len-CMD_MUTI_RECORD_HEADER_LEN, header_len);
		zip_type = REPORTER_CMD_COMPRESS_TYPE_GZIP;
		sd_memcpy(g_cmd_buffer+CMD_MUTI_RECORD_HEADER_LEN, buffer_http_header, header_len);
		cmd_len = CMD_MUTI_RECORD_HEADER_LEN + header_len;
	}
	
	// 填充数据头:<协议长度>  (条数)(压缩方式)
	buffer_ptr = g_cmd_buffer + 8;
	buffer_len = 12;
	
	ret = sd_set_int32_to_lt(&buffer_ptr, &buffer_len, cmd_len - CMD_HEADER_LEN);
	CHECK_VALUE(ret);
	ret = sd_set_int32_to_lt(&buffer_ptr, &buffer_len, record_count);
 	CHECK_VALUE(ret);
	ret = sd_set_int32_to_lt(&buffer_ptr, &buffer_len, zip_type);
 	CHECK_VALUE(ret);
 	
	//发送已添加记录
	header_len = 1024;	// it is very important. 
	ret = build_report_http_header(buffer_http_header, &header_len, cmd_len, REPORTER_EMBED);
	CHECK_VALUE(ret);
	
	ret = sd_malloc(header_len + cmd_len, (void**)&buffer_malloc);
	CHECK_VALUE(ret);
	
	sd_memcpy(buffer_malloc, buffer_http_header, header_len);
	sd_memcpy(buffer_malloc + header_len, g_cmd_buffer, cmd_len);
	
	ret = reporter_commit_cmd(get_embed_hub(), REPORT_MOBILE_USER_ACTION_CMD_TYPE, 
			buffer_malloc, header_len + cmd_len, NULL, seq);
	LOG_DEBUG("reporter_from_file commit cmd: len=%u, record_count=%u, zip_type=%d", header_len + cmd_len, record_count, zip_type);
	return ret;
}

/**
  *UAC发送成功的回调函数
  *1标记发送成功
  *2继续发送剩余的消息
  *@return: 0-SUCCESS else-FAILURE
**/
_int32 reporter_from_file_callback()
{
	_int32 ret = 0;
	_u32 file_id = 0;
	_u64 filesize = 0;
	_u64 cmd_pos_in_file = 0;
	_u64 next_cmd_pos_in_file = 0;
	char *buffer = NULL;
	_u32 readsize = 0, writesize = 0;
	_u32 cmd_len = 0;
	_u32 cmd_type = 0, seq = 0;
	char filename[MAX_FILE_PATH_LEN];
	char buffer_pos[REPORTER_ACTION_FILE_POS_LEN];
	char *buffer_pos_ptr = NULL;
	_int32 buffer_pos_len = 0;

	LOG_ERROR("reporter_from_file_callback==============");

	ret = get_action_filename(filename, MAX_FILE_PATH_LEN);
	CHECK_VALUE(ret);

	ret = sd_open_ex(filename, O_FS_RDWR, &file_id);
	CHECK_VALUE(ret);

	ret = sd_filesize(file_id, &filesize);
	CHECK_VALUE(ret);
	// 读消息偏移位置
	ret = sd_pread(file_id, buffer_pos, REPORTER_ACTION_FILE_POS_LEN, 0, &readsize);
	CHECK_VALUE(ret);
	//转换成_int64
	buffer_pos_ptr = buffer_pos;
	buffer_pos_len = readsize;
	ret = sd_get_int64_from_lt(&buffer_pos_ptr, &buffer_pos_len, (_int64 *)&cmd_pos_in_file);
	CHECK_VALUE(ret);
	ret = sd_get_int64_from_lt(&buffer_pos_ptr, &buffer_pos_len, (_int64 *)&next_cmd_pos_in_file);
	CHECK_VALUE(ret);

	//检查文件格式并标记发送成功
	if ((next_cmd_pos_in_file >= filesize)
		|| (next_cmd_pos_in_file < cmd_pos_in_file + CMD_HEADER_LEN) )
	{
		LOG_DEBUG("reporter_from_file_callback reset uac file: filesize=%llu, cmd_pos_in_file=%llu, next_cmd_pos_in_file=%llu",filesize, cmd_pos_in_file, next_cmd_pos_in_file );
		//文件错误或者发送完成，重置文件
		ret = sd_close_ex(file_id);
		CHECK_VALUE(ret);

		ret = sd_delete_file(filename);
		CHECK_VALUE(ret);

		ret = sd_open_ex(filename, O_FS_RDWR | O_FS_CREATE, &file_id);
		CHECK_VALUE(ret);

		ret = sd_filesize(file_id, &filesize);
		CHECK_VALUE(ret);

		ret = sd_enlarge_file(file_id, REPORTER_ACTION_FILE_POS_LEN, &filesize);
		CHECK_VALUE(ret);
		
		cmd_pos_in_file = REPORTER_ACTION_FILE_POS_LEN;
		next_cmd_pos_in_file = 0;
	}

	cmd_pos_in_file = (next_cmd_pos_in_file>REPORTER_ACTION_FILE_POS_LEN) ? next_cmd_pos_in_file : REPORTER_ACTION_FILE_POS_LEN;
	next_cmd_pos_in_file = 0;		

	buffer_pos_ptr = buffer_pos;
	buffer_pos_len = REPORTER_ACTION_FILE_POS_LEN;
	ret = sd_set_int64_to_lt(&buffer_pos_ptr, &buffer_pos_len, cmd_pos_in_file);
	CHECK_VALUE(ret);
	ret = sd_set_int64_to_lt(&buffer_pos_ptr, &buffer_pos_len, next_cmd_pos_in_file);
	CHECK_VALUE(ret);
	
	ret = sd_pwrite(file_id, buffer_pos, REPORTER_ACTION_FILE_POS_LEN, 0, &writesize);
	CHECK_VALUE(ret);
	
	ret = sd_close_ex(file_id);
	CHECK_VALUE(ret);
	
	//继续发送剩余的消息
	return reporter_from_file();
}

#endif
