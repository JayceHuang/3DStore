#ifndef _XV_DEC_H
#define _XV_DEC_H

#ifdef __cplusplus
extern "C" 
{
#endif


#include "utility/define.h"

#ifdef ENABLE_XV_FILE_DEC

typedef struct tagXV_DEC_CONTEXT{
	unsigned int encoded_data_offset;
	unsigned int encoded_data_len;
	unsigned char key;
}XV_DEC_CONTEXT;

#define MAX_XV_FILE_HEAD_SIZE 2*1024*1024

XV_DEC_CONTEXT* create_xv_decoder();
int analyze_xv_head(XV_DEC_CONTEXT* ctx,unsigned char* head_buf,unsigned int len_of_head_buf, unsigned int* decode_data_offset);
int decode_xv_data(XV_DEC_CONTEXT* ctx, unsigned int data_offset,unsigned char* input_buffer,unsigned int len_of_input_buf);
void close_xv_decoder(XV_DEC_CONTEXT* ctx);

#endif /* ENABLE_XV_FILE_DEC */

#ifdef __cplusplus
}
#endif


#endif

