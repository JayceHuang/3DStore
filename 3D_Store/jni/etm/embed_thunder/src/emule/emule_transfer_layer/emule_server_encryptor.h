/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/11/27
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_SERVER_ENCRYPTOR_H_
#define _EMULE_SERVER_ENCRYPTOR_H_
#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
#include "rsa/rsaeuro.h"
#include "utility/rc4.h"


struct tagEMULE_SOCKET_DEVICE;

typedef enum tagENCRYPT_STATE
{
	ENCRYPT_NONE = 0, ENCRYPT_HANDSHAKE_START, ENCRYPT_HANDSHAKE_RECV_PADDING, ENCRYPT_HANDSHAKE_OK
}ENCRYPT_STATE;

typedef struct tagEMULE_SERVER_ENCRYPTOR
{
	ENCRYPT_STATE		_state;
	NN_DIGIT 			_a[4];
	RC4_KEY				_send_key;
	RC4_KEY				_recv_key;
	char					_buffer[128];
}EMULE_SERVER_ENCRYPTOR;

_int32 emule_server_encryptor_build_handshake(EMULE_SERVER_ENCRYPTOR* encryptor, char** buffer, _int32* len);

_int32 emule_server_encryptor_recv_key(EMULE_SERVER_ENCRYPTOR* encryptor, char* buffer, _int32 len, _u8* padding);

_int32 emule_server_encryptor_build_handshake_resp(EMULE_SERVER_ENCRYPTOR* encryptor, char** buffer, _int32* len);

void emule_server_encryptor_encode_data(EMULE_SERVER_ENCRYPTOR* encryptor, char* data, _int32 len);

void emule_server_encryptor_decode_data(EMULE_SERVER_ENCRYPTOR* encryptor, char* data, _int32 len);

_u8 emule_generate_random_not_protocol_marker(void);

#endif
#endif /* ENABLE_EMULE */
#endif



