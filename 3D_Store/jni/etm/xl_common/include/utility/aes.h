/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/08/20
-----------------------------------------------------------------------------------------------------------*/
#ifndef _AES_H_
#define _AES_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"

#define Bits128				16  /*密钥长度*/
#define Bits192				24
#define Bits256				32
#define ENCRYPT_BLOCK_SIZE	16

typedef struct  
{
	_int32 Nb;         /* block size in 32-bit words.  Always 4 for AES.  (128 bits). */
	_int32 Nk;         /* key size in 32-bit words.  4, 6, 8.  (128, 192, 256 bits). */
	_int32 Nr;         /* number of rounds. 10, 12, 14. */
	_u8	   State[4][4];/* State matrix */
	_u8	   key[32];    /* the seed key. size will be 4 *keySize from ctor. */
	_u8    w[16*15];   /* key schedule array. (Nb*(Nr+1))*4 */
}ctx_aes;

void aes_init(ctx_aes* aes, int keySize, _u8* keyBytes);
void aes_cipher(ctx_aes* aes, _u8* input, _u8* output);  /* encipher 16-bit input */
void aes_invcipher(ctx_aes* aes, _u8* input, _u8* output);
//
void SetNbNkNr(ctx_aes* aes, _int32 keyS);
void AddRoundKey(ctx_aes* aes, _int32 round);
void SubBytes(ctx_aes* aes);
void InvSubBytes(ctx_aes* aes);
void ShiftRows(ctx_aes* aes);
void InvShiftRows(ctx_aes* aes);
void MixColumns(ctx_aes* aes);
void InvMixColumns(ctx_aes* aes);
_u8  gfmultby01(_u8 b);
_u8 gfmultby02(_u8 b);
_u8 gfmultby03(_u8 b);
unsigned char gfmultby09(unsigned char b);
unsigned char gfmultby0b(unsigned char b);
unsigned char gfmultby0d(unsigned char b);
unsigned char gfmultby0e(unsigned char b);
void KeyExpansion(ctx_aes* aes);
void SubWord(_u8 *word, _u8 *result);
void RotWord(_u8 *word, _u8 *result);

_int32 gen_aes_key_by_ptr(const void *ptr, _u8 *p_aeskey);
_int32 aes_encrypt_with_known_key(char* buffer,_u32* len, _u8 *key);
_int32 aes_decrypt_with_known_key(char* p_data_buff, _u32* p_data_buff_len, _u8 *key);

//////////////////////////////////
//对src中的数据进行AES(128Bytes)加密，加密后数据保存在des中
//aes_key:用于加密的key,但是注意，这个key还需要进行MD5运算，用算出的MD5值作为加密解密的密钥!
//src:待加密数据
//src_len:待加密数据长度
//des:加密后的数据
//des_len:输入输出参数，传进去的的是des的内存最大长度,传出来的是加密后的数据长度
_int32 sd_aes_encrypt( const char* aes_key,const _u8 * src, _u32 src_len,_u8 * des, _u32 * des_len);

//////////////////////////////////
//对src中的数据进行AES(128Bytes)解密，解密后数据保存在des中
//aes_key:用于解密的key,但是注意，这个key还需要进行MD5运算，用算出的MD5值作为加密解密的密钥!
//src:待解密数据
//src_len:待解密数据长度
//des:解密后的数据
//des_len:输入输出参数，传进去的的是des的内存最大长度,传出来的是解密后的数据长度
_int32 sd_aes_decrypt(const  char* aes_key,const _u8 * src, _u32 src_len,_u8 * des, _u32 * des_len);

//////////////////////////////////
//对整个文件进行AES(128Bytes)解密，并用解密后的文件替换解密前的文件
//aes_key:用于解密的key,但是注意，这个key还需要进行MD5运算，用算出的MD5值作为加密解密的密钥!
//file_name:待解密文件全名
_int32 sd_aes_decrypt_file(const  char* aes_key,const char * file_name);


int32_t em_aes_encrypt(char* in_buffer ,uint32_t in_len, char* out_buffer, uint32_t* out_len, uint8_t key[16]);
int32_t em_aes_decrypt(char* in_buffer ,uint32_t in_len, char* out_buffer, uint32_t* out_len, uint8_t key[16]);

#ifdef __cplusplus
}
#endif

#endif

