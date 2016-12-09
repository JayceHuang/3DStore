#if !defined(__MD5_20080718)
#define __MD5_20080718

#ifdef __cplusplus
extern "C" 
{
#endif

typedef struct _tag_ctx_md5
{
       unsigned int  _state[4];		/* state (ABCD) */

	unsigned int  _count[2];		/* number of bits, modulo 2^64 (lsb first) */

	unsigned char _inner_data[64];  /* input buffer */
	
}ctx_md5,* p_ctx_md5;


void md5_initialize(ctx_md5* p_ctx);
void md5_update(ctx_md5* p_ctx, const unsigned char *pdata, unsigned int count);
void md5_finish(ctx_md5* p_ctx,unsigned char cid[16]);

void md5_handle(unsigned int *state, const unsigned char block[64]);

void md5_encode(unsigned char *output, const unsigned int *input, unsigned int len);

void md5_decode(unsigned int *output, const unsigned char *input, unsigned int len);



#ifdef __cplusplus
}
#endif

#endif // !defined(__MD5_20080718)

