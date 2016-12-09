#include "platform/sd_zlib.h"
#include "utility/errcode.h"
#include "utility/mempool.h"

#include "platform/sd_customed_interface.h"

#define Z_OK            0

_int32 sd_zlib_uncompress( char *p_src, _u32 src_len, char **pp_dest, _u32 *p_dest_len )
{
    _int32 ret_val = SUCCESS;
 	_u32 buff_size= UNZLIB_BUF_SIZE(src_len);
    char *p_buffer = NULL;   
    
	// —πÀı–≠“È

    if(!is_available_ci(CI_ZLIB_UNCOMPRESS)) return DK_UNSUPPORT_ZLIB;
    
    ret_val = sd_malloc( buff_size, (void **)&p_buffer );
    CHECK_VALUE( ret_val );

    ret_val = ((et_zlib_uncompress)ci_ptr(CI_ZLIB_UNCOMPRESS))( (unsigned char *)p_buffer, (_int32 *)&buff_size, (unsigned char *)p_src, src_len );
    if( ret_val != Z_OK )
    {
        SAFE_DELETE( p_buffer );
        return ret_val;
    }

    *pp_dest = p_buffer;
    *p_dest_len = buff_size;
    return SUCCESS;
}
