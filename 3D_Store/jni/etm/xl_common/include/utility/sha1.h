
#if !defined(__SHA1_20080718)
#define __SHA1_20080718

#ifdef __cplusplus
extern "C" 
{
#endif


typedef struct _tag_ctx_sha1
{
       unsigned int _state[5];
	unsigned int _count[2];
	unsigned char _inner_data[64];
	 
}ctx_sha1, * p_ctx_sha1;


void sha1_initialize(ctx_sha1* p_ctx);
void sha1_update(ctx_sha1* p_ctx, const unsigned char *pdata, unsigned int count);
void sha1_finish(ctx_sha1* p_ctx,unsigned char cid[20]);

void sha1_handle(unsigned  int*state, const unsigned char block[64]);



#ifdef __cplusplus
}
#endif

#endif // !defined(__SHA1_20080718)

