#ifndef _RES_QUERY_SECURITY_H_20110721
#define _RES_QUERY_SECURITY_H_20110721
/*
 * author: xietao
 * date: 20110721
 */

#ifdef _RSA_RES_QUERY

#include "utility/define.h"
#include "res_query_interface.h"

/* rsa encrypted */
#define PUB_KEY_LEN 140
#define RSA_ENCRYPT_HEADER_LEN				(4+4+4+128+4)	//【魔数】【密钥版本】【密钥长度】【密钥内容】【数据长度】
#define RES_QUERY_AES_CIPHER_LEN			128				//经RSA加密后的AES的密文长度
#define RSA_ENCRYPT_MAGIC_NUM				637753480
#define RSA_PUBKEY_VERSION_SHUB				10000
#define RSA_PUBKEY_VERSION_BTHUB			10000
#define RSA_PUBKEY_VERSION_EMULEHUB			10000
#define RSA_PUBKEY_VERSION_PHUB				40000
#define RSA_PUBKEY_VERSION_VIPHUB			60000
#define RSA_PUBKEY_VERSION_TRACKER			70000
#define RSA_PUBKEY_VERSION_PARTNER_CDN		80000
#define RSA_PUBKEY_VERSION_DPHUB			50000 

_int32 gen_aes_key_by_user_data(void *user_data, _u8 *p_aeskey);

_int32 res_query_rsa_pub_encrypt(_int32 len, _u8 *text, char *encrypted, _u32 *p_enc_len, _int32 key_ver);

#endif

#endif //_RES_QUERY_ENCRYPT_H_20110721