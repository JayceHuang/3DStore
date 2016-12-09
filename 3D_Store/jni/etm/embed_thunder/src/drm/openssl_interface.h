#ifndef SD_EMBEDTHUNDERFS_H_OPENSSL_INTERFACE_201009081666
#define SD_EMBEDTHUNDERFS_H_OPENSSL_INTERFACE_201009081666

#include "utility/define.h"
#ifdef ENABLE_DRM 

//#include "openssl/rsa.h"
//#include "openssl/bio.h"

#define OPENSSL_IDX_COUNT			    (7)

/* function index */
#define OPENSSL_D2I_RSAPUBLICKEY_IDX    (0)
#define OPENSSL_RSA_SIZE_IDX            (1)
#define OPENSSL_BIO_NEW_MEM_BUF_IDX     (2)
#define OPENSSL_D2I_RSA_PUBKEY_BIO_IDX  (3)
#define OPENSSL_RSA_PUBLIC_DECRYPT_IDX  (4)
#define OPENSSL_RSA_FREE_IDX            (5)
#define OPENSSL_BIO_FREE_IDX            (6)

typedef _int32 RSA_MY;
typedef _int32 BIO_MY;

typedef RSA_MY * (*et_func_d2i_RSAPublicKey)(RSA_MY **a, _u8 **pp, _int32 length);
typedef _int32 (*et_func_openssli_RSA_size)(const RSA_MY *rsa);
typedef BIO_MY *(*et_func_BIO_new_mem_buf)(void *buf, _int32 len);
typedef RSA_MY *(*et_func_d2i_RSA_PUBKEY_bio)(BIO_MY *bp,RSA_MY **rsa);
typedef _int32(*et_func_RSA_public_decrypt)(_int32 flen, const _u8 *from, _u8 *to, RSA_MY *rsa, _int32 padding);
typedef void(*et_func_RSA_free)( RSA_MY *r );
typedef void(*et_func_BIO_free)( BIO_MY *a );

_int32 set_openssli_rsa_interface(_int32 fun_count, void *fun_ptr_table);

BOOL is_available_openssli(void);

RSA_MY *openssli_d2i_RSAPublicKey(RSA_MY **a, _u8 **pp, _int32 length);

_int32	openssli_RSA_size(const RSA_MY *rsa);

BIO_MY *openssli_BIO_new_mem_buf( void *buf, _int32 len );

RSA_MY *openssli_d2i_RSA_PUBKEY_bio(BIO_MY *bp,RSA_MY **rsa);

_int32	openssli_RSA_public_decrypt( _int32 flen, const _u8 *from, 
		_u8 *to, RSA_MY *rsa, _int32 padding);

void openssli_RSA_free( RSA_MY *r );

void openssli_BIO_free( BIO_MY *a );

#endif
#endif

