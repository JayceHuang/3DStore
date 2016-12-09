#include "utility/define.h"
#ifdef ENABLE_DRM 

#include "openssl_interface.h"

#include "utility/errcode.h"

static BOOL g_is_available_openssl = FALSE;
static void* g_openssli_fun[OPENSSL_IDX_COUNT] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};

_int32 set_openssli_rsa_interface(_int32 fun_count, void *fun_ptr_table)
{
    _int32 fun_index = 0;
    void *fun = NULL;

    if( fun_count != OPENSSL_IDX_COUNT
        || fun_ptr_table == NULL )
    {
        return INVALID_CUSTOMED_INTERFACE_PTR;
    }

    for( ; fun_index < fun_count; fun_index++ )
    {
        fun = (void *)(*((_u32*)fun_ptr_table + fun_index));
        if( fun != NULL )
        {
            g_openssli_fun[fun_index] = fun;
        }
        else
        {
            return INVALID_CUSTOMED_INTERFACE_PTR;
        }
    }

    g_is_available_openssl = TRUE;
    return SUCCESS;
}

BOOL is_available_openssli(void)
{
    return g_is_available_openssl;
}


RSA_MY *openssli_d2i_RSAPublicKey(RSA_MY **a, _u8 **pp, _int32 length)
{
    RSA_MY *ret = NULL;
    ret = ((et_func_d2i_RSAPublicKey)g_openssli_fun[OPENSSL_D2I_RSAPUBLICKEY_IDX])(a, pp, length);
    return ret;
}

_int32	openssli_RSA_size(const RSA_MY *rsa)
{
    return ((et_func_openssli_RSA_size)g_openssli_fun[OPENSSL_RSA_SIZE_IDX])(rsa);
}


BIO_MY *openssli_BIO_new_mem_buf( void *buf, _int32 len )
{
    return ((et_func_BIO_new_mem_buf)g_openssli_fun[OPENSSL_BIO_NEW_MEM_BUF_IDX])(buf, len);
}    

RSA_MY *openssli_d2i_RSA_PUBKEY_bio(BIO_MY *bp,RSA_MY **rsa)
{
    return ((et_func_d2i_RSA_PUBKEY_bio)g_openssli_fun[OPENSSL_D2I_RSA_PUBKEY_BIO_IDX])(bp, rsa);
}

_int32	openssli_RSA_public_decrypt( _int32 flen, const _u8 *from, 
		_u8 *to, RSA_MY *rsa, _int32 padding)
{
    return ((et_func_RSA_public_decrypt)g_openssli_fun[OPENSSL_RSA_PUBLIC_DECRYPT_IDX])(flen, from, to, rsa, padding );
}

void openssli_RSA_free ( RSA_MY *r )
{
    return ((et_func_RSA_free)g_openssli_fun[OPENSSL_RSA_FREE_IDX])(r);
}

void openssli_BIO_free( BIO_MY *a )
{
    return ((et_func_BIO_free)g_openssli_fun[OPENSSL_BIO_FREE_IDX])(a);
}

#endif
