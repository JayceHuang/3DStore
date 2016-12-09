#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
#include "emule_server_encryptor.h"
#include "../emule_server/emule_server_device.h"
#include "../emule_utility/emule_opcodes.h"
#include "utility/mempool.h"
#include "utility/md5.h"
#include "utility/time.h"
#include "utility/utility.h"
#include "utility/bytebuffer.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"
#include "utility/rc4.h"

/*
Diffie-Hellman密钥交换协议如下：  
首先，Alice和Bob双方约定2个大整数n和g,其中1<g<p，这两个整数无需保密，然后，执行下面的过程 
1) Alice随机选择一个大整数x(保密)，并计算X=gx mod p  
2) Bob随机选择一个大整数y(保密)，并计算Y=gy mod p
3) Alice把X发送给B,B把Y发送给ALICE  
4) Alice计算K=Yx mod n  
5) Bob计算K=Xy mod n  
K即是共享的密钥,监听者Oscar在网络上只能监听到X和Y，但无法通过X，Y计算出x和y，
因此，Oscar无法计算出K= gxy mod p。 
在这里约定g =  2

通信包发送如下:
Client: <SemiRandomNotProtocolMarker 1[Unencrypted]><G^A 96 [Unencrypted]><RandomBytes 0-15 [Unencrypted]>    
Server: <G^B 96 [Unencrypted]><MagicValue 4><EncryptionMethodsSupported 1><EncryptionMethodPreferred 1><PaddingLen 1><RandomBytes PaddingLen>
Client: <MagicValue 4><EncryptionMethodsSelected 1><PaddingLen 1><RandomBytes PaddingLen> (Answer delayed till first payload to save a frame)
*/

#define	DHAGREEMENT_A_BITS		128
#define	PRIMESIZE_BYTES			96
#define	MAGICVALUE_REQUESTER		34							// modification of the requester-send and server-receive key
#define	MAGICVALUE_SERVER			203							// modification of the server-send and requester-send key
#define	MAGICVALUE_SYNC			0x835E6FC4					// value to check if we have a working encrypted stream 
#define	ENM_OBFUSCATION         		0

static NN_DIGIT dh768_p[] =
{
	0x6AF70DF3, 0x548B5F43, 0x8F05150F, 0x42FB4268,
	0xF88B2891, 0x0F762387, 0x28105F34, 0x7DF77818,
	0x5AD5E074, 0x1CCEAED4, 0x5255D4F6, 0xB207CBAA,
	0x86DA4A91, 0xCE259F22, 0x66535BDB, 0x21FC2863,
	0x43AFFCE7, 0xC73A41A6, 0xC34CB40D, 0x6217A33E,
	0xE886EB3C, 0x5371A936, 0x5F587ADD, 0xF2BF52C5,
};

_int32 emule_server_encryptor_build_handshake(EMULE_SERVER_ENCRYPTOR* encryptor, char** buffer, _int32* len)
{
	_int32 ret = SUCCESS;
	char* tmp_buf = NULL;
	_int32 tmp_len = 0, i = 0;
	R_RANDOM_STRUCT random;
	NN_DIGIT g[24] = {0};
	NN_DIGIT result[24] = {0};
	_u8 padding_len = emule_generate_random_not_protocol_marker() % 16;
	*len = 2 + PRIMESIZE_BYTES + padding_len;
	ret = sd_malloc(*len, (void**)buffer);
	CHECK_VALUE(ret);
	tmp_buf = *buffer;
	tmp_len = *len;
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)emule_generate_random_not_protocol_marker());
	/* Generate the private value of key agreement. */
	R_RandomCreate(&random);
	R_GenerateBytes((_u8*)encryptor->_a, 16, &random);
	g[0] = 2;
	/* Compute y = g^x mod p. */
	NN_ModExp(result, g, encryptor->_a, 4, dh768_p, 24);
	NN_Encode((_u8*)tmp_buf, PRIMESIZE_BYTES, result, 24);
	tmp_buf += PRIMESIZE_BYTES;
	tmp_len -= PRIMESIZE_BYTES;
	sd_set_int8(&tmp_buf, &tmp_len, padding_len);
	for(i = 0; i < padding_len; ++i)
	{
		ret = sd_set_int8(&tmp_buf, &tmp_len, (_int8)emule_generate_random_not_protocol_marker());
	}
	sd_assert(ret == SUCCESS && tmp_len == 0);
	return ret;
}

_int32 emule_server_encryptor_recv_key(EMULE_SERVER_ENCRYPTOR* encryptor, char* buffer, _int32 len, _u8* padding)
{
	_int32 ret = SUCCESS;
	ctx_md5	md5;
	_u32 magic = 0;
	_u8 encryp_methods_supported = 0, encryp_method_preferred = 0;
	_u8 md5_key_data[16] = {0};
	char encode_buffer[97] = {0};
	NN_DIGIT answer[24] = {0};				//16个字节的大整数a
	NN_DIGIT result[24] = {0};
	char* tmp_buf = buffer + PRIMESIZE_BYTES;
	_int32 tmp_len = len - PRIMESIZE_BYTES;
	NN_Decode(answer, 24, (_u8*)buffer, PRIMESIZE_BYTES);
	/* Compute y = g^x mod p. */
	NN_ModExp(result, answer, encryptor->_a, 4, dh768_p, 24);
	// create the keys
	NN_Encode((_u8*)encode_buffer, PRIMESIZE_BYTES, result, 24);
	// 根据md5值创建密钥
	encode_buffer[PRIMESIZE_BYTES] = MAGICVALUE_REQUESTER;
	md5_initialize(&md5);
	md5_update(&md5, (_u8*)encode_buffer, 97);
	md5_finish(&md5, md5_key_data);
	rc4_init_key(&encryptor->_send_key, md5_key_data, 16);
	// 根据md5值创建密钥
	encode_buffer[PRIMESIZE_BYTES] = MAGICVALUE_SERVER;
	md5_initialize(&md5);
	md5_update(&md5, (_u8*)encode_buffer, 97);
	md5_finish(&md5, md5_key_data);	
	rc4_init_key(&encryptor->_recv_key, md5_key_data, 16);
	//解析剩下的字段<MagicValue 4><EncryptionMethodsSupported 1><EncryptionMethodPreferred 1><PaddingLen 1>
	emule_server_encryptor_decode_data(encryptor, tmp_buf, tmp_len);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&magic);
	sd_assert(magic == MAGICVALUE_SYNC);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&encryp_methods_supported);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&encryp_method_preferred);
	ret = sd_get_int8(&tmp_buf, &tmp_len, (_int8*)padding);
	sd_assert(ret == SUCCESS && tmp_len == 0);
	return SUCCESS;
}

_int32 emule_server_encryptor_build_handshake_resp(EMULE_SERVER_ENCRYPTOR* encryptor, char** buffer, _int32* len)
{
	_int32 ret = SUCCESS;
	_u8 padding_len = (_u8)sd_rand() % 16, i = 0;
	char *tmp_buf = NULL;
	_int32 tmp_len = 0;
	*len = 6 + padding_len;
	ret = sd_malloc(*len, (void**)buffer);
	CHECK_VALUE(ret);
	tmp_buf = *buffer;
	tmp_len = *len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, MAGICVALUE_SYNC);
	sd_set_int8(&tmp_buf, &tmp_len, ENM_OBFUSCATION);
	if(padding_len == 0)
		padding_len = 1;
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)padding_len);
	for(i = 0; i < padding_len; ++i)
	{
		ret = sd_set_int8(&tmp_buf, &tmp_len, sd_rand()%128);
	}
	sd_assert(ret == SUCCESS && tmp_len == 0);
	emule_server_encryptor_encode_data(encryptor, *buffer, *len);
	return SUCCESS;
}

void emule_server_encryptor_encode_data(EMULE_SERVER_ENCRYPTOR* encryptor, char* data, _int32 len)
{
	sd_assert(FALSE);
	rc4_crypt((const _u8*)data, (_u8*)data, (_u32)len, &encryptor->_send_key);
}

void emule_server_encryptor_decode_data(EMULE_SERVER_ENCRYPTOR* encryptor, char* data, _int32 len)
{
	sd_assert(FALSE);
	rc4_crypt((const _u8*)data, (_u8*)data, (_u32)len, &encryptor->_recv_key);
}

_u8 emule_generate_random_not_protocol_marker(void)
{
	_u8 result = 0;
	_u32 i = 0, time = 0;
	sd_time(&time);
	sd_srand(time);
	for(i = 0; i < 128; ++i)
	{
		result = (_u8)sd_rand();
		if(result == OP_EDONKEYPROT || result == OP_PACKEDPROT || result == OP_EMULEPROT || result == 0)
			continue;
		else
			break;
	}
	return result;
}

#endif /* ENABLE_EMULE */

#endif /* ENABLE_EMULE */

