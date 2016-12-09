
#if !defined(__SD_ZLIB_20100106)
#define __SD_ZLIB_20100106

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/utility.h"

#define EMBLOCKSIZE				184320
#define UNZLIB_BUF_SIZE(data_len) MAX(data_len*16,EMBLOCKSIZE)	// 计算ZLIB算法解压需要的缓冲区大小


_int32 sd_zlib_uncompress( char *p_src, _u32 src_len, char **pp_dest, _u32 *p_dest_len );

#ifdef __cplusplus
}
#endif

#endif // !defined(__SD_ZLIB_20100106)


