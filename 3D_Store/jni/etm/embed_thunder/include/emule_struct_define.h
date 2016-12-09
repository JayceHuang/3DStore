#ifndef _EMULE_STRUCT_DEFINE_H_
#define _EMULE_STRUCT_DEFINE_H_
#include "utility/define.h"

#include "utility/list.h"
#include "utility/define_const_num.h"

#ifdef ENABLE_EMULE
#define	USER_ID_SIZE				16
#define	CONN_ID_SIZE				16
#define	PART_HASH_SIZE			16
#define	ROOT_HASH_SIZE			20
#define	AICH_HASH_LEN				20
#define	MAX_EMULE_FILE_SIZE		0x4000000000ull 	// = 2^38 = 256GB
#define	OLD_MAX_EMULE_FILE_SIZE	4290048000ull
#ifdef WINCE
#define	EMULE_PART_SIZE			9728000ull
#else
#define	EMULE_PART_SIZE			9728000llu
#endif
#define	EMULE_HEADER_SIZE			5
#define	EMULE_MAX_CMD_LEN		(800*1024)

#define	AES_ENCODE_PADDING_LEN	16
#define	EMULE_HUB_VERSION		54
#define	EMULE_HUB_HEADER_LEN		18

typedef struct tagEMULE_HUB_HEADER
{
	_u32	_version;
	_u32	_hub_seq;
	_u32	_cmd_len;
	_u32	_client_version;
	_u16	_compress;
}EMULE_HUB_HEADER;

#define IS_LOWID(id)					(id<16777216)		// 是否是LowID
#define IS_HIGHID(id)				(!IS_LOWID(id))

// 定义混杂选项
#define DEF_MISC_OPTION(name,offset,mask)			\
			enum{									\
			BIT_OFFSET_##name	= offset,				\
			BIT_MASK_##name=	mask,				\
			};

// 访问混杂选项
#define GET_MISC_OPTION(option,name)		(option>>BIT_OFFSET_##name)&BIT_MASK_##name
#define SET_MISC_OPTION(option,name,val)	option |= (((_u32)val<<BIT_OFFSET_##name))

// CT_EMULE_MISCOPTIONS1混合选项
DEF_MISC_OPTION(AICH,29,0x07)
DEF_MISC_OPTION(UNICODE,28,0x01)
DEF_MISC_OPTION(UDPVER,24,0x0f)
DEF_MISC_OPTION(COMPRESS,20,0x0f)
DEF_MISC_OPTION(SECUREIDENT,16,0x0f)
DEF_MISC_OPTION(SOURCEEXCHANGE,12,0x0f)
DEF_MISC_OPTION(EXTENDREQUEST,8,0x0f)
DEF_MISC_OPTION(COMMENT,4,0x0f)
DEF_MISC_OPTION(PEERCACHE,3,0x01)
DEF_MISC_OPTION(NOVIEWSHAREDFILE,2,0x01)
DEF_MISC_OPTION(MULTIPACKET,1,0x01)
DEF_MISC_OPTION(PREVIEW,0,0x01)

//CT_EMULE_MISCOPTIONS2混合选项
DEF_MISC_OPTION(REQUIRES_CRYPT_LAYER,9,0x01)
DEF_MISC_OPTION(REQUESTS_CRYPT_LAYER,8,0x01)
DEF_MISC_OPTION(SUPPORTS_CRYPT_LAYER,7,0x01)
DEF_MISC_OPTION(EXT_MULTI_PACKET,5,0x01)
DEF_MISC_OPTION(LARGE_FILES,4,0x01)
DEF_MISC_OPTION(KADVER,0,0x0f)

#define EMBLOCKSIZE					184320
#define UNZLIB_BUF_SIZE(data_len) 	MAX(data_len*16,EMBLOCKSIZE)	// 计算ZLIB算法解压需要的缓冲区大小

//////////////////////////////////////////////////////////////////////////
// 身份验证选项
//////////////////////////////////////////////////////////////////////////
enum {
	IS_UNAVAILABLE		= 0,
	IS_ALLREQUESTSSEND  = 0,
	IS_SIGNATURENEEDED	= 1,    // 需要发送签名
	IS_KEYANDSIGNEEDED	= 2,    // 需要发送公钥和签名
	IS_VERIFIED         = 3,    // 已验证
	IS_BADGUY           = 4,    // 验证失败
};

//////////////////////////////////////////////////////////////////////////
// emule客户端版本
//////////////////////////////////////////////////////////////////////////
enum {
	SO_EMULE			= 0,	// default
	SO_CDONKEY			= 1,	// ET_COMPATIBLECLIENT
	SO_XMULE			= 2,	// ET_COMPATIBLECLIENT
	SO_AMULE			= 3,	// ET_COMPATIBLECLIENT
	SO_SHAREAZA			= 4,	// ET_COMPATIBLECLIENT
	SO_MLDONKEY			= 10,	// ET_COMPATIBLECLIENT
	SO_LPHANT			= 20,	// ET_COMPATIBLECLIENT
	// other client types which are not identified with ET_COMPATIBLECLIENT
	SO_EDONKEYHYBRID	= 50,
	SO_EDONKEY,
	SO_OLDEMULE,
	SO_URL,
	SO_UNKNOWN
};

typedef struct tagEMULE_PEER_SOURCE
{
	_u32	_ip;
	_u16	_port;
}EMULE_PEER_SOURCE;

typedef struct tagED2K_LINK_INFO
{
	char		_file_name[MAX_FILE_NAME_LEN  + 1];
	_u64	_file_size;
	_u8		_file_id[FILE_ID_SIZE];
	_u8		_root_hash[ROOT_HASH_SIZE];
	LIST		_source_list;				//类型是EMULE_PEER_SOURCE
	_u8*	_part_hash;
	_u32	_part_hash_size;
	char		_http_url[MAX_URL_LEN + 1];
	
}ED2K_LINK_INFO;


// 解释2d2k链接字串
_int32 emule_extract_ed2k_link(char* ed2k_link, ED2K_LINK_INFO* ed2k_info);

//构造一个ed2k链接
_int32 emule_create_ed2k_link(const char* file_name, _u64 file_size, 
		const _u8 file_hash[16], char *ed2k_buf, _u32 *pbuflen);

#endif /* ENABLE_EMULE */
#endif


