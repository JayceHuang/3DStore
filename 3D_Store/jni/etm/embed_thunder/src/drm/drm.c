#include "utility/define.h"
#ifdef ENABLE_DRM 

#include "drm.h"

#include "platform/sd_fs.h"
#include "utility/define.h"
#include "utility/string.h"
#include "utility/errcode.h"

#include "openssl_interface.h"

#include "data_manager/data_manager_interface.h"
#include "utility/string.h"
#include "utility/utility.h"
#include "http_data_pipe/http_data_pipe_interface.h"

#include "utility/logid.h"
#define LOGID LOGID_DRM
#include "utility/logger.h"
#include "utility/md5.h"
#include "utility/aes.h"
#include "utility/peerid.h"
#include "utility/bytebuffer.h"
#include "utility/sd_iconv.h"
#include "utility/define_const_num.h"
#include "utility/settings.h"
#include "utility/version.h"
#include "reporter/reporter_interface.h"
#include "embed_thunder_version.h"

static DRM_MANAGER g_drm_manager;
static _u32 g_drm_id = 0;
static char g_certificate_path[MAX_FILE_PATH_LEN] = {0};
static void* g_drm_pipe_function_table[MAX_FUNC_NUM];
static PIPE_INTERFACE g_drm_pipe_interface;

#define XLV_ID  0x56444c58
#define XLV_VER 0x1000000

#define XLV_RSA_DATA_LEN 128
#define XLV_HEAD_LEN ( XLV_RSA_DATA_LEN+4+4 )

#define XLV_CERTIFICATE_HEAD "drm_certificate_"
#define XLV_CERTIFICATE_TAIL ".cfg"
#define XLV_DECODE_BUFFER_LEN 1024
#define DRM_DATA_BUFFRE_LEN (16*1024)

static char g_drm_verify_server_host[MAX_HOST_NAME_LEN] = {0};

_int32 drm_init_module()
{
    LOG_DEBUG( "drm_init_module" );
    sd_memset( g_drm_pipe_function_table, 0, MAX_FUNC_NUM );
    sd_memset( &g_drm_pipe_interface, 0, sizeof(PIPE_INTERFACE) );
    sd_memset( g_certificate_path, 0, MAX_FILE_PATH_LEN );
	g_drm_pipe_function_table[0] = (void*)drm_pipe_change_range;
	g_drm_pipe_function_table[1] = (void*)drm_pipe_get_dispatcher_range;
	g_drm_pipe_function_table[2] = (void*)drm_pipe_set_dispatcher_range;
	g_drm_pipe_function_table[3] = (void*)drm_pipe_get_file_size;
	g_drm_pipe_function_table[4] = (void*)drm_pipe_set_file_size;
	g_drm_pipe_function_table[5] = (void*)drm_pipe_get_data_buffer;
	g_drm_pipe_function_table[6] = (void*)drm_pipe_free_data_buffer;
	g_drm_pipe_function_table[7] =(void*) drm_pipe_put_data_buffer;
	g_drm_pipe_function_table[8] = (void*)drm_pipe_read_data;
	g_drm_pipe_function_table[9] = (void*)drm_pipe_notify_dispatch_data_finish;
	g_drm_pipe_function_table[10] = (void*)drm_pipe_get_file_type;
	g_drm_pipe_function_table[11] = NULL;
	g_drm_pipe_interface._file_index = 0;
	g_drm_pipe_interface._range_switch_handler = NULL;
	g_drm_pipe_interface._func_table_ptr = g_drm_pipe_function_table;
//   map_init( &g_drm_manager.drm_file_map, drm_comparator );
       list_init(&g_drm_manager.drm_file_list);
	   
	sd_memcpy( g_drm_verify_server_host, DRM_VERIFY_SERVER_HOST, MAX_HOST_NAME_LEN);
	settings_get_str_item("drm.verify_server_host", g_drm_verify_server_host );
    return SUCCESS;
}

_int32 drm_uninit_module()
{
    DRM_FILE *p_drm_file = NULL;

    LIST_ITERATOR list_it;

    LOG_DEBUG( "drm_uninit_module" );
    for(list_it = LIST_BEGIN(g_drm_manager.drm_file_list); list_it != LIST_END(g_drm_manager.drm_file_list);)
    {
		p_drm_file = LIST_VALUE(list_it);
              drm_destroy_drm_file(p_drm_file);
		list_it = LIST_NEXT(list_it);

		//list_erase(g_drm_manager.drm_file_list, LIST_PRE(list_it));
     }
    list_clear(&g_drm_manager.drm_file_list);
    
    sd_memset( g_drm_pipe_function_table, 0, MAX_FUNC_NUM );
    sd_memset( &g_drm_pipe_interface, 0, sizeof(PIPE_INTERFACE) );
    sd_memset( &g_drm_verify_server_host, 0, MAX_HOST_NAME_LEN );
    
    //sd_assert( map_size(&g_drm_manager.drm_file_map)==0 );
    
    return SUCCESS;
}

_int32 drm_set_certificate_path( const char file_full_path[] )
{
    _u32 path_len = 0;
    LOG_DEBUG( "drm_set_certificate_path:%s", file_full_path );
    if( !sd_file_exist(file_full_path) )
      return DT_ERR_INVALID_FILE_PATH;

    path_len = sd_strlen(file_full_path);
    
    if( path_len >= MAX_FILE_PATH_LEN )
    {
        return DT_ERR_INVALID_FILE_PATH;
    }

    sd_memset( g_certificate_path, 0, MAX_FILE_PATH_LEN );
    sd_strncpy( g_certificate_path, file_full_path, MAX_FILE_PATH_LEN );

    if( g_certificate_path[path_len-1] != '/' )
    {
        if( path_len+1 >= MAX_FILE_PATH_LEN )
        {
            sd_memset( g_certificate_path, 0, MAX_FILE_PATH_LEN );
            return DT_ERR_INVALID_FILE_PATH;
        }
        g_certificate_path[path_len] = '/';
        
        LOG_DEBUG( "drm_set_certificate_path:%s", file_full_path );
    }    
    return SUCCESS;
}

_int32 drm_open_file( const char file_full_path[], _u32 *p_drm_id, _u64 *p_origin_file_size )
{
    _int32 ret_val = SUCCESS;
    DRM_FILE *p_drm_file = NULL;
    char buffer[XLV_RSA_DATA_LEN] = {0};
    _u32 read_size = 0;
    _u32 id = 0, ver = 0;
    char *tmp = NULL;
    //_u16 cid_wchar[ CID_SIZE*2 ] = {0};
    //_u32 output_len = 0;
    _u64 xlmv_file_size = 0;

    _u8 out_buffer[XLV_DECODE_BUFFER_LEN] = {0};
    _u32 out_len = XLV_DECODE_BUFFER_LEN;
//    PAIR map_pair;
    
    LOG_DEBUG( "drm_open_file:%s", file_full_path );
        
    if( !sd_file_exist(file_full_path) )
    {
        LOG_ERROR( "drm_file_is_not_exist:%s", file_full_path );
        return DRM_NO_FILE;
    }

    ret_val = sd_malloc( sizeof(DRM_FILE), (void**)&p_drm_file );
    CHECK_VALUE( ret_val );

    sd_memset( p_drm_file, 0, sizeof(DRM_FILE) );
    p_drm_file->_file_id = INVALID_FILE_ID; 
    p_drm_file->_timeout_id = INVALID_MSG_ID;

    ret_val = sd_open_ex( (char*)file_full_path, O_FS_RDONLY, &p_drm_file->_file_id );
    if( ret_val != SUCCESS )
    {
        LOG_ERROR( "drm_open_file err, ret_val:%u", ret_val );
        ret_val = DRM_OPRN_ERR;
        goto ErrHandle;
    }

    ret_val = sd_read( p_drm_file->_file_id, (char *)&id, sizeof(id), &read_size );
    if( ret_val != SUCCESS || read_size != sizeof(id) )
    {
        LOG_ERROR( "sd_read id, ret_val:%u", ret_val );
        ret_val = DRM_READ_ERR;
        goto ErrHandle1;
    }
    
    ret_val = sd_read( p_drm_file->_file_id, (char *)&ver, sizeof(ver), &read_size );
    if( ret_val != SUCCESS || read_size != sizeof(ver) )
    {
        LOG_ERROR( "sd_read ver, ret_val:%u", ret_val );
        ret_val = DRM_READ_ERR;
        goto ErrHandle1;
    }


    ret_val = sd_read( p_drm_file->_file_id, buffer, XLV_RSA_DATA_LEN, &read_size );
    if( ret_val != SUCCESS || read_size != XLV_RSA_DATA_LEN )
    {
        LOG_ERROR( "sd_read rsa head, ret_val:%u", ret_val );
        ret_val = DRM_READ_ERR;
        goto ErrHandle1;
    }


    if( id != XLV_ID || ver != XLV_VER )
    {
        LOG_ERROR( "drm_file err format id:%u, ver:%u", id, ver );
        ret_val = DRM_FILE_FORMAT_ERR;
        goto ErrHandle1;
    }

    if( !is_available_openssli() )
    {
        LOG_ERROR( "drm_rsa_decode_xlv_head, openssli unsuport:%d", ret_val );
        ret_val = DRM_UNSUPPORT_OPENSSL;
        goto ErrHandle1;
    }
    
    ret_val = drm_rsa_decode_xlv_head( (_u8*)(buffer), XLV_RSA_DATA_LEN, (_u8*)out_buffer, &out_len );
    if( ret_val != SUCCESS )
    {
        LOG_ERROR( "drm_rsa_decode_xlv_head, ret_val:%d", ret_val );
        ret_val = DRM_FILE_FORMAT_ERR;
        goto ErrHandle1;
    }

    //sd_assert( out_len == 84 );
    tmp = (char *)out_buffer;
    
    ret_val = sd_get_int32_from_lt( &tmp, (_int32 *)&out_len, (_int32 *)&p_drm_file->_mid );
    if( ret_val != SUCCESS )
    {
        LOG_ERROR( "drm_rsa_decode_mid err, ret_val:%u", ret_val );
        ret_val = DRM_FILE_FORMAT_ERR;
        goto ErrHandle1;
    }
    
    ret_val = sd_get_bytes( &tmp, (_int32 *)&out_len, (char *)p_drm_file->_key_md5, 16 );
    if( ret_val != SUCCESS )
    {
        LOG_ERROR( "drm_rsa_decode_key_md5 err, ret_val:%u", ret_val );
        ret_val = DRM_FILE_FORMAT_ERR;
        goto ErrHandle1;
    }
     
    sd_memset( p_drm_file->_cid_hex, 0, CID_SIZE*2+1 );
    ret_val = sd_get_bytes( &tmp, (_int32 *)&out_len, (char *)p_drm_file->_cid_hex, CID_SIZE*2 );
    if( ret_val != SUCCESS )
    {
        LOG_ERROR( "drm_rsa_decode_cid err, ret_val:%u", ret_val );
        ret_val = DRM_FILE_FORMAT_ERR;
        goto ErrHandle1;
    }
    
    ret_val = sd_get_int32_from_lt( &tmp, (_int32 *)&out_len, (_int32 *)&p_drm_file->_append_data_len );
    if( ret_val != SUCCESS )
    {
        ret_val = DRM_FILE_FORMAT_ERR;
        goto ErrHandle1;
    } 
    sd_assert( out_len == 0 );

    /*
    output_len = CID_SIZE*2+1;
    ret_count = sd_unicode_2_gbk_str( cid_wchar, CID_SIZE*2, p_drm_file->_cid_hex, &output_len );
    if( ret_count < 0 )
    {
        LOG_ERROR( "drm_rsa_unicode_2_gbk err, ret_val:%u", ret_val );
        ret_val = DRM_FILE_FORMAT_ERR;
        goto ErrHandle1;
    }
    */
    LOG_DEBUG( "drm_rsa_decode mid:%d, cid:%s, append_data_len:%d", 
        p_drm_file->_mid, p_drm_file->_cid_hex, p_drm_file->_append_data_len );

    if( p_drm_file->_append_data_len != 0 )
    {
        sd_memset( out_buffer, 0, XLV_DECODE_BUFFER_LEN );
        ret_val = sd_read( p_drm_file->_file_id, (char*)out_buffer, p_drm_file->_append_data_len, &read_size );
        if( ret_val != SUCCESS || read_size != p_drm_file->_append_data_len )
        {
            LOG_ERROR( "sd_read rsa head, ret_val:%u", ret_val );
            ret_val = DRM_READ_ERR;
            goto ErrHandle1;
        }
        
        ret_val = drm_aes_decode_xlmv_append_data( p_drm_file, (char*)out_buffer, p_drm_file->_append_data_len );
        if( ret_val != SUCCESS )
        {
            ret_val = DRM_FILE_FORMAT_ERR;
            goto ErrHandle1;
        } 
    }


    *p_drm_id = ++g_drm_id;

/*	
    map_pair._key = (void *)g_drm_id;
    map_pair._value = p_drm_file;   
    ret_val = map_insert_node( &g_drm_manager.drm_file_map, &map_pair );
*/
	p_drm_file->_drm_id = *p_drm_id;

    ret_val = list_insert(&g_drm_manager.drm_file_list, (void*)p_drm_file, LIST_BEGIN(g_drm_manager.drm_file_list));
    if( ret_val != SUCCESS )
    {
        LOG_ERROR( "drm list_insert_node, ret_val:%u", ret_val );
        goto ErrHandle1;
    }

    ret_val = sd_filesize( p_drm_file->_file_id, &xlmv_file_size );
    if( ret_val != SUCCESS )
    {
        drm_enter_err( p_drm_file, ret_val );
        goto ErrHandle1;
    }
    
    if( xlmv_file_size > XLV_HEAD_LEN + p_drm_file->_append_data_len )
    {
        p_drm_file->_origin_file_size = xlmv_file_size - XLV_HEAD_LEN - p_drm_file->_append_data_len;
    }
    *p_origin_file_size = p_drm_file->_origin_file_size;

    
    drm_handle_certificate( p_drm_file );
    if( p_drm_file->_is_certificate_ok ) 
    {
        LOG_DEBUG( "drm_open_file:%s SUCCESS, drm_id:%d. _is_certificate_ok!!!, orgin_file_size:%llu", 
            file_full_path, *p_drm_id, p_drm_file->_origin_file_size );
        return SUCCESS;
    }
    
    ret_val = drm_download_certificate( p_drm_file );
    if( ret_val != SUCCESS )
    {
        drm_enter_err( p_drm_file, ret_val );
        goto ErrHandle1;
    }

    LOG_DEBUG( "drm_open_file:%s,need download_certificate!!!, drm_id:%d, orgin_file_size:%llu", 
        file_full_path, *p_drm_id, p_drm_file->_origin_file_size );
    
    return SUCCESS;
    
ErrHandle1:
    sd_close_ex( p_drm_file->_file_id );
    p_drm_file->_file_id = INVALID_FILE_ID;

ErrHandle:
    
    sd_free( p_drm_file );
	p_drm_file = NULL;
    return ret_val;
}

_int32 drm_is_certificate_ok( _u32 drm_id, BOOL *p_is_ok )
{
    _int32 ret_val = SUCCESS;
    DRM_FILE *p_drm_file = NULL;

    ret_val = drm_get_fille_ptr( drm_id, &p_drm_file, FALSE );
    if( ret_val != SUCCESS )
    {
        return ret_val;
    }
    if( p_drm_file->_err_code != SUCCESS )
    {
        return p_drm_file->_err_code;
    }
    
    *p_is_ok = drm_is_can_read(p_drm_file);

    LOG_DEBUG( "drm_is_certificate_ok:drm_id:%d, is_ok:%d,is_net_err:%d", 
        drm_id, *p_is_ok, p_drm_file->_is_net_err );
    return SUCCESS;
}

_int32 drm_read_file( _u32 drm_id, char *buf, _u32 size, _u64 start_pos, _u32 *p_read_size )
{
    _int32 ret_val = SUCCESS;
    DRM_FILE *p_drm_file = NULL;
    //_u32 index = 0, read_pos = 0;
    

    ret_val = drm_get_fille_ptr( drm_id, &p_drm_file, FALSE );
    if( ret_val != SUCCESS )
    {
        return ret_val;
    }
    
    if( p_drm_file->_err_code != SUCCESS )
    {
        return p_drm_file->_err_code;
    }
    
    if( !drm_is_can_read(p_drm_file) )
    {
        return DRM_CERTIFICATE_NOT_OK;
    }

    if( p_drm_file->_file_id == INVALID_FILE_ID )
    {
        sd_assert( FALSE );
        return DRM_LOGIC_ERR;
    }
    ret_val = sd_pread( p_drm_file->_file_id, buf, size, start_pos + XLV_HEAD_LEN + p_drm_file->_append_data_len, p_read_size );
    CHECK_VALUE( ret_val );

	drm_read_buffer( p_drm_file, buf, start_pos, *p_read_size );

/*
    index = start_pos % DRM_KEY_LEN;
    for( ; read_pos < *p_read_size; read_pos++ )
    {
        buf[read_pos] ^= p_drm_file->_key[index];
        index++;
        if( index >= DRM_KEY_LEN ) index = 0;
    }
 */
    LOG_DEBUG( "drm_read_file, drm_id:%d, size:%u, pos:%llu, read_size:%u ", 
        drm_id, size, start_pos, *p_read_size );
    return SUCCESS;
}

_int8 g_drm_Key[128] = {
		0xFF,0x55,0x41,0x13,0x83,0xAC,0x4F,0xBF,0xCA,0xCC,0x57,0x58,0x3E,0x08,0x4B,0x04,0xB2,0x26,0x7B,0xD4,0x7A,0x75,0x1D,0x79,0x99,0xD5,0x77,0xC9,0xFE,0x18,0x7C,0x60,0x53,0x78,0x47,0x2D,0xD2,0x81,0xB9,0xEB,0xAC,0x17,0x16,0xB9,0xF3,0x30,0x00,0xA0,0x5B,0xA1,0x13,0x23,0x35,0xDE,0x4A,0xE6,0x27,0x7A,0xDB,0xA5,0xB7,0x3B,0x8E,0x16,0x56,0xA0,0xFB,0xB5,0xB7,0x37,0x47,0xFA,0xDE,0x74,0x6C,0x41,0x74,0xE3,0x1D,0x7E,0xFC,0x75,0x0F,0xE2,0x6A,0xB1,0xA2,0x33,0xAE,0x42,0x15,0x04,0x1D,0xE4,0x67,0xCA,0xBA,0x4D,0x85,0xEC,0x0D,0xD9,0x84,0xA8,0x28,0xC4,0x8C,0xCB,0xDD,0x46,0x05,0xF2,0x06,0x30,0x66,0x0E,0xD3,0x03,0xDF,0x1E,0x0B,0xB8,0x7C,0xF3,0xD8,0xAB,0x20,0x10
};

//100% encode
//void Readv4_100(byte* pbBuffer,LONGLONG len,LONGLONG filepos)
void drm_read_buffer( DRM_FILE *p_drm_file, char *p_buffer, _u64 llpos, _u64 len )
{
       //transfer arg
       char* pbBuffer = p_buffer;
	_u64 filepos = llpos;
	char   key[DRM_KEY_LEN];
	char* m_pKey = (char*)key;
       _int32* m_pdwKey = (_int32*)key;
	char* pb = pbBuffer;
	//隔4字节加密，在这128一个周期中的哪一位
	_int32 pos = filepos%128;
	//在32个块中的哪一块，一块有4字节
	_int32 blockindex = pos/4;

	_int32 posInBlock = pos%4;

	char* end = pbBuffer + len;
	_int32* pdw = NULL;

	sd_memcpy(key, p_drm_file->_key, DRM_KEY_LEN);
	if (posInBlock != 0)
	{
		//if (posInBlock < 4)
		//{
			while (posInBlock != 4)
			{
				if (pb == end)
				{
					return;
				}
				*pb ^= m_pKey[blockindex*4+posInBlock];
				++pb;
				++posInBlock;
			}
			//pb+=4;
			//if (pb >= end)
			//{
			//	return;
			//}
		//}
		//else
		//{
		//	pb += ( 8 - posInBlock );
		//	if (pb >= (void*)end)
		//	{
		//		return;
		//	}
		//}
		++blockindex;
		if (blockindex == 32)
		{
			blockindex = 0;
		}
	}

	pos = (filepos + len)%128;
	posInBlock = pos%4;

	//if (posInBlock < 4)
	//{
		end -= posInBlock;
		while (	posInBlock != 0)
		{
			*(end + posInBlock - 1)	^=	m_pKey[pos/4*4+posInBlock - 1];
			--posInBlock;
		}
	//}	

	pdw = (_int32*)pb;

	while (pdw < (_int32*)end)
	{
		*pdw ^= m_pdwKey[blockindex];
		++blockindex;
		if (blockindex == 32)
		{
			blockindex = 0;
		}
		++pdw;
	}
}
#if 0
//void CXlvStream::Readv5(byte* pbBuffer,LONGLONG lllen,LONGLONG llfilepos)
void drm_read_buffer( DRM_FILE *p_drm_file, char *p_buffer, _u64 pos, _u64 len )
{
       //transfer arg
       char* pbBuffer = p_buffer;
	_u64 llfilepos = pos;
	_u64 lllen = len;
	char* m_pKey = (char*)g_drm_Key;
       _int32* m_pdwKey = (_int32*)g_drm_Key;
	   
	char* pb = pbBuffer;
	////在这1280一个周期中的哪一位
	_int32 dwpos = llfilepos%1280;
	////在32个块中的哪一块，一块有40字节，前4字节为加密，后36字节不加密
	_int32 dwblockindex = dwpos/40;
	_int32 dwposInBlock = dwpos%40;

	char* pbend = pbBuffer + lllen;

	if (dwposInBlock != 0)
	{
		if (dwposInBlock < 4)
		{
			while (dwposInBlock != 4)
			{
				if (pb == pbend)
				{
					return;
				}
				*pb ^= m_pKey[dwblockindex*4+dwposInBlock];
				++pb;
				++dwposInBlock;
			}
			pb+=36;
			if (pb >= pbend)
			{
				return;
			}
		}
		else
		{
			pb += ( 40 - dwposInBlock );
			if (pb >= pbend)
			{
				return;
			}
		}
		++dwblockindex;
		if (dwblockindex == 32)
		{
			dwblockindex = 0;
		}
	}

	dwpos = (llfilepos + lllen)%1280;
	dwposInBlock = dwpos%40;

	if (dwposInBlock < 4)
	{
		pbend -= dwposInBlock;
		while (	dwposInBlock != 0)
		{
			*(pbend + dwposInBlock - 1)	^=	m_pKey[dwpos/40*4+dwposInBlock - 1];
			--dwposInBlock;
		}
	}	

	_int32* pdw = (_int32*)pb;
	while (pdw < (void*)pbend)
	{

		*pdw ^= m_pdwKey[dwblockindex];
		++dwblockindex;
		if (dwblockindex == 32)
		{
			dwblockindex = 0;
		}
		pdw += 10;
	}
}
#endif
/*
void drm_read_buffer( DRM_FILE *p_drm_file, char *p_buffer, _u64 pos, _u64 len )
{
	char *p_tmp_buffer = p_buffer;
	_u32 byteReads = len;
	_u32 index = (_u32)(pos % DRM_KEY_LEN);
	_u32 bi = index/4;
	
	_u32 dwKey[DRM_KEY_LEN/4] = {0}; 
	_u32 remainbyte = 0;
	_u32 dwKeyLength = DRM_KEY_LEN/4;
	_u32 numb = 0;
	
	_u32* pdw = NULL;
	_u32 i = 0;
	
    LOG_DEBUG( "drm_read_buffer, len:%llu, pos:%llu ", len, pos );

	if( len== 0 ) return;
	
	sd_assert( DRM_KEY_LEN%4 == 0 );
	sd_memcpy( dwKey, p_drm_file->_key, DRM_KEY_LEN );
	
	if ((index)%sizeof(_u32))
	{
		remainbyte = 4 - (index)%4;
		byteReads -= remainbyte;
		for ( i = 0 ; i < remainbyte ; ++i)
		{
			*p_tmp_buffer ^= p_drm_file->_key[index];
			p_tmp_buffer++;
			index = (index + 1)%DRM_KEY_LEN;
		}
		bi = (bi + 1)%dwKeyLength;
	}

	numb = byteReads/sizeof(_u32);
	byteReads = byteReads%sizeof(_u32);

	pdw = (_u32*)p_tmp_buffer;
	for ( i = 0; i < numb ; ++i)
	{
		*pdw ^= dwKey[bi];
		bi = (bi + 1)%dwKeyLength;
		pdw++;
	}

	p_tmp_buffer = (char*)pdw;

	if (byteReads)
	{
		index =  (_u32)((pos + len - byteReads)% DRM_KEY_LEN);
		for ( i = 0 ; i < byteReads ; ++i)
		{
			*p_tmp_buffer ^= p_drm_file->_key[index];
			p_tmp_buffer++;
			index = (index + 1)%DRM_KEY_LEN;
		}
	}
}

*/
_int32 drm_close_file( _u32 drm_id )
{
    _int32 ret_val = SUCCESS;
    DRM_FILE *p_drm_file = NULL;
    
    LOG_DEBUG( "drm_close_file, drm_id:%d", drm_id );
    ret_val = drm_get_fille_ptr( drm_id, &p_drm_file, TRUE );
    if( ret_val != SUCCESS )
    {
        return ret_val;
    }
    drm_destroy_drm_file( p_drm_file );

    return SUCCESS;

}

BOOL drm_is_can_read( DRM_FILE *p_drm_file  )
{
    BOOL can_read = FALSE;
    if( p_drm_file->_is_certificate_ok ) 
    {
        can_read = TRUE;
    }
    else
    {
        if( p_drm_file->_is_net_err
            && p_drm_file->_is_key_valid )
        {
            can_read = TRUE;
        }
    }
    LOG_DEBUG( "drm_is_can_read, drm_file_id:%d, can_read:%d", p_drm_file->_drm_id, can_read );
    return can_read;
}

_int32 drm_destroy_drm_file( DRM_FILE *p_drm_file )
{
    
    LOG_DEBUG( "drm_destroy_drm_file, p_drm_file:0x%x", p_drm_file );
    if( p_drm_file->_file_id != INVALID_FILE_ID )
    {
        sd_close_ex( p_drm_file->_file_id );
    }
    
    if( p_drm_file->_http_pipe_ptr != NULL )
    {
        drm_destory_res_and_pips( p_drm_file );
    }

    if( p_drm_file->_timeout_id != INVALID_MSG_ID )
    {
        cancel_timer( p_drm_file->_timeout_id );
        p_drm_file->_timeout_id = INVALID_MSG_ID;
    }

    sd_free( p_drm_file );
	p_drm_file = NULL;

    return SUCCESS;
}

_int32 drm_comparator(void *E1, void *E2)
{
    _u32 l = (_u32)E1;
	_u32 r = (_u32)E2; 
	if(l<r)  return -1;
	else if(l==r)  return 0;
	else return 1;
		
}

_int32 drm_handle_certificate( DRM_FILE *p_drm_file )
{
    _int32 ret_val = SUCCESS;
     char cert_file_full_name[MAX_FULL_PATH_LEN] = {0};
    _u32 file_id = INVALID_FILE_ID;
    char *p_buffer = NULL;
    _u32 read_size = 0;
    _u64 file_size = 0;
    _u8 out_buffer[XLV_DECODE_BUFFER_LEN] = {0};
    _u32 out_len = XLV_DECODE_BUFFER_LEN;
    _u8 szKey[16];
    ctx_md5 md5;
    
    LOG_DEBUG( "drm_handle_certificate, p_drm_file:0x%x", p_drm_file );

    ret_val = drm_get_certificate_file_full_path( p_drm_file, cert_file_full_name );
    if( ret_val != SUCCESS )
    {
        goto ErrHandle;
    }

    if( !sd_file_exist(cert_file_full_name) )
    {
        ret_val = DRM_LOGIC_ERR;
        goto ErrHandle;
    }

    ret_val = sd_open_ex( cert_file_full_name, O_FS_RDONLY, &file_id );
    if( ret_val != SUCCESS )
    {
        goto ErrHandle;
    }

    ret_val = sd_filesize( file_id, &file_size );
    if( ret_val != SUCCESS )
    {
        goto ErrHandle1;
    }

    ret_val = sd_malloc( file_size, (void**)&p_buffer );
    if( ret_val != SUCCESS )
    {
        sd_assert( FALSE );
        goto ErrHandle1;
    }

    ret_val = sd_read( file_id, p_buffer, file_size, &read_size );
    if( ret_val != SUCCESS )
    {
        sd_assert( FALSE );
        goto ErrHandle2;
    }    
    
    if( !is_available_openssli() )
    {
        LOG_ERROR( "drm_handle_certificate, openssli unsuport:%d", ret_val );
        ret_val = DRM_UNSUPPORT_OPENSSL;
        goto ErrHandle2;
    }

    ret_val = drm_rsa_decode_certificate( (_u8*)p_buffer, read_size, out_buffer, &out_len );
    if( ret_val != SUCCESS )
    {
        //sd_assert( FALSE );
        sd_delete_file( cert_file_full_name );
        ret_val = DRM_CERTIFICATE_FORMAT_ERR;
        goto ErrHandle2;
    }   
    
    SAFE_DELETE( p_buffer );
    
    ret_val = drm_aes_decode_certificate( out_buffer, &out_len );
    if( ret_val != SUCCESS )
    {
        //sd_assert( FALSE );
        sd_delete_file( cert_file_full_name );
        ret_val = DRM_CERTIFICATE_FORMAT_ERR;
        goto ErrHandle1;
    }   

    if( out_len != DRM_KEY_LEN )
    {
        sd_assert( FALSE );
        goto ErrHandle1;
    }

    sd_memcpy( p_drm_file->_key, out_buffer, DRM_KEY_LEN );

    
    md5_initialize(&md5);
    md5_update(&md5, (unsigned char*)p_drm_file->_key,DRM_KEY_LEN);
    md5_finish(&md5, szKey);

    if( sd_memcmp( szKey, p_drm_file->_key_md5, KEY_MD5_LEN ) != 0 )
    {
        LOG_DEBUG( "drm_handle_certificate, _key_md5 err:0x%x", p_drm_file );
        ret_val = DRM_CERTIFICATE_FORMAT_ERR;
        goto ErrHandle1;
    }
    LOG_DEBUG( "drm_handle_certificate SUCCESS!!!, p_drm_file:0x%x", p_drm_file );
    p_drm_file->_is_certificate_ok = TRUE;
    p_drm_file->_is_key_valid = TRUE;
    sd_close_ex( file_id );

    return SUCCESS;
    
ErrHandle2:
    SAFE_DELETE( p_buffer );

ErrHandle1:
    sd_close_ex( file_id );

ErrHandle:
    LOG_DEBUG( "drm_handle_certificate err!!!, p_drm_file:0x%x, ret:%d", p_drm_file, ret_val );
    p_drm_file->_is_certificate_ok = FALSE;
    return ret_val;

}

_int32 drm_get_certificate_file_full_path( DRM_FILE *p_drm_file, char file_name[MAX_FULL_PATH_LEN] )
{
    _int32 ret_val = SUCCESS;
    char file_mid[10] = {0};
    
    if( sd_strlen(g_certificate_path) == 0 )
    {
        ret_val = DT_ERR_INVALID_FILE_PATH;
        return ret_val;
    }
    sd_u32_to_str( p_drm_file->_mid, file_mid, 10 );
    sd_strncpy( file_name, g_certificate_path, MAX_FILE_PATH_LEN );
    sd_strcat( file_name, XLV_CERTIFICATE_HEAD, MAX_FILE_PATH_LEN - sd_strlen(file_name) );
    sd_strcat( file_name, file_mid, MAX_FILE_PATH_LEN - sd_strlen(file_name) );
    sd_strcat( file_name, XLV_CERTIFICATE_TAIL, MAX_FILE_PATH_LEN - sd_strlen(file_name) );

    LOG_DEBUG( "drm_get_certificate_file_full_path, p_drm_file:0x%x, file_name:%s", 
        p_drm_file, file_name );

    return SUCCESS;
}

_int32 drm_download_certificate( DRM_FILE *p_drm_file )
{
    _int32 ret_val = SUCCESS;
    char url[MAX_URL_LEN] = {0};
    RANGE rg;
    _u32 url_len = 0;
    _u8 peer_id[PEER_ID_SIZE+1] = {0};
    char http_request[1024] = {0};
    _u32 request_len = 1024;
    char version[MAX_VERSION_LEN] = {0};
	char license[MAX_LICENSE_LEN] = {0};

    ret_val = get_peerid( (char *)peer_id, PEER_ID_SIZE+1);
    CHECK_VALUE( ret_val );
    
	ret_val = get_version( version, MAX_VERSION_LEN-1); 
	CHECK_VALUE(ret_val);
    
	ret_val = reporter_get_license( license, MAX_LICENSE_LEN); 
	CHECK_VALUE(ret_val);


    ///test
	settings_get_str_item("drm.peer_id", (char*)peer_id );
    ///test

    //http://auth.mall.xunlei.com/box/auth?v=1&pid=000C29A549F8000X&vid=2&c=3


	url_len = sd_snprintf( url, MAX_URL_LEN, "http://%s/box/auth?v=1&pid=%s&vid=%d&cid=%s&tid=2&c=%s&license=%s",
				g_drm_verify_server_host, peer_id, p_drm_file->_mid,  p_drm_file->_cid_hex, version, license );

    LOG_DEBUG( "drm_download_certificate, p_drm_file:0x%x, url:%s", p_drm_file, url );

    ret_val = http_resource_create( url, sd_strlen(url), NULL, 0, TRUE, &p_drm_file->_http_res_ptr );
    CHECK_VALUE( ret_val );

	ret_val = http_pipe_create( NULL, p_drm_file->_http_res_ptr, (DATA_PIPE**)&p_drm_file->_http_pipe_ptr );
    CHECK_VALUE( ret_val );

	dp_set_pipe_interface( p_drm_file->_http_pipe_ptr, &g_drm_pipe_interface );
	 p_drm_file->_http_pipe_ptr->_is_user_free = TRUE;//防止http模块不回调

	request_len = sd_snprintf( http_request, request_len, "GET /box/auth?v=1&pid=%s&vid=%d&cid=%s&tid=2&c=%s&license=%s HTTP/1.1\r\nHost: %s\r\nConnection: Keep-Alive\r\n\r\n",
				peer_id, p_drm_file->_mid, p_drm_file->_cid_hex, version, license, g_drm_verify_server_host );

    
    LOG_DEBUG( "drm_download_certificate, p_drm_file:0x%x, http_request:%s", p_drm_file, http_request );

	http_pipe_set_request( p_drm_file->_http_pipe_ptr, http_request, request_len);


	ret_val = http_pipe_open( p_drm_file->_http_pipe_ptr );
    CHECK_VALUE( ret_val );

	rg._index = 0;
	rg._num = MAX_U32;
	ret_val = pi_pipe_change_range( p_drm_file->_http_pipe_ptr, &rg, FALSE );
    CHECK_VALUE( ret_val );
    
    ret_val = start_timer( drm_handle_timeout, NOTICE_INFINITE, 1000, 0, p_drm_file, &p_drm_file->_timeout_id );
    CHECK_VALUE( ret_val );

    return SUCCESS;
}

void drm_enter_err( DRM_FILE *p_drm_file, _int32 err )
{
    LOG_DEBUG("drm_enter_err, errcode:%d.", err );
    p_drm_file->_err_code = err;
}

_int32 drm_handle_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid)
{
    _int32 ret_val = SUCCESS;
	DRM_FILE *p_drm_file = (DRM_FILE*)msg_info->_user_data;
    
    LOG_DEBUG("drm_handle_timeout, errcode:%d.", errcode );

	if(errcode != MSG_TIMEOUT)
	{
		return SUCCESS;
	}

    if( p_drm_file->_err_code != SUCCESS )
    {
		return SUCCESS;
    }

    if( p_drm_file->_is_write_file_ok && !p_drm_file->_is_certificate_ok )
    {
        ret_val = drm_handle_certificate( p_drm_file );
        if( ret_val != SUCCESS )
        {
            LOG_DEBUG("drm_handle_certificate err:%d", ret_val );
            drm_enter_err( p_drm_file, ret_val );
        }
    }

    if( p_drm_file->_http_pipe_ptr != NULL
        && dp_get_state(p_drm_file->_http_pipe_ptr) == PIPE_FAILURE )
    {
        LOG_DEBUG("drm_handle_timeout, pipe_err." );
        if( !p_drm_file->_is_key_valid )
        {
            drm_enter_err( p_drm_file, DRM_CERTIFICATE_DOWNLOAD_ERR );
        }
        drm_destory_res_and_pips( p_drm_file );
        p_drm_file->_is_net_err = TRUE;
        return SUCCESS;
    }
    
    if( p_drm_file->_is_write_file_ok && p_drm_file->_http_pipe_ptr != NULL )
    {
        LOG_DEBUG("drm_handle_timeout, _is_write_file_ok." );
        drm_destory_res_and_pips( p_drm_file );
    }

    if( p_drm_file->_is_certificate_ok )
    {
        cancel_timer( p_drm_file->_timeout_id );
        p_drm_file->_timeout_id = INVALID_MSG_ID;
    }
    return SUCCESS;
}

_int32 drm_destory_res_and_pips( DRM_FILE *p_drm_file )
{
    LOG_DEBUG( "drm_destory_res_and_pips, p_drm_file:0x%x", p_drm_file);
    http_pipe_close( p_drm_file->_http_pipe_ptr );
    p_drm_file->_http_pipe_ptr = NULL;
    http_resource_destroy( &p_drm_file->_http_res_ptr );
    p_drm_file->_http_res_ptr = NULL;
    return SUCCESS;
}

_int32 drm_get_fille_ptr( _u32 drm_id, DRM_FILE **pp_drm_file, BOOL is_remove )
{
    DRM_FILE* p_drm_file;
    LIST_ITERATOR list_it;
	
    *pp_drm_file = NULL;
    for(list_it = LIST_BEGIN(g_drm_manager.drm_file_list); list_it != LIST_END(g_drm_manager.drm_file_list);)
    {
		p_drm_file = LIST_VALUE(list_it);
		if(p_drm_file->_drm_id == drm_id)
		{
		      *pp_drm_file = p_drm_file;
			if(is_remove){
		              list_erase(&g_drm_manager.drm_file_list, (list_it));
			}
                    LOG_DEBUG( "drm_get_fille_ptr, drm_id:%d, p_drm_file:0x%x", drm_id, *pp_drm_file);
		      return SUCCESS;;
		}
		list_it = LIST_NEXT(list_it);
     }
     return DRM_INVALID_ID;
}

_int32 drm_get_fille_ptr_by_pipe( DATA_PIPE *p_data_pipe, DRM_FILE **pp_drm_file )
{
    DRM_FILE* p_drm_file;
    LIST_ITERATOR list_it;
	
    *pp_drm_file = NULL;
    for(list_it = LIST_BEGIN(g_drm_manager.drm_file_list); list_it != LIST_END(g_drm_manager.drm_file_list);)
    {
		p_drm_file = LIST_VALUE(list_it);
		if(p_drm_file->_http_pipe_ptr == p_data_pipe)
		{
		      *pp_drm_file = p_drm_file;
                    LOG_DEBUG( "drm_get_fille_ptr_by_pipe, data_pipe:0x%x, p_drm_file:0x%x", p_data_pipe, *pp_drm_file);
		      return SUCCESS;
		}
		list_it = LIST_NEXT(list_it);
     }
     return DRM_INVALID_ID;
}


_int32 drm_rsa_decode_xlv_head( _u8 *p_input, _u32 input_len, _u8 *p_output, _u32 *p_output_len )
{
    _int32 ret_val = SUCCESS;
    _u32 rsa_size = 0;
	const _u8 pubKey[141] = {
		0x30, 0x81, 0x89, 0x02, 0x81, 0x81, 0x00, 0xed, 0xfc, 0x3a, 0x50, 0xa4, 0x73, 0xee, 0x9b, 0xbb, 
			0xb8, 0x79, 0x01, 0x16, 0x22, 0x97, 0xfa, 0xb8, 0xd0, 0x38, 0xd4, 0xcb, 0xa8, 0x7d, 0xed, 0xed, 
			0x42, 0x53, 0x67, 0x15, 0xf7, 0x7b, 0xd8, 0x12, 0x06, 0x27, 0x38, 0x90, 0xfe, 0xda, 0x1c, 0xce, 
			0x7f, 0x51, 0xd3, 0xc0, 0x98, 0xbd, 0x94, 0xb4, 0x27, 0x9a, 0x1a, 0x1b, 0x7e, 0xc1, 0x32, 0x9c, 
			0x94, 0xa6, 0xc8, 0x2c, 0x2f, 0xa6, 0x45, 0xf6, 0xb4, 0xfa, 0x8e, 0x27, 0xab, 0x77, 0xa4, 0x9d, 
			0xeb, 0x4b, 0x78, 0xcc, 0x0c, 0xa4, 0xa7, 0x9f, 0x8a, 0xe2, 0xfb, 0xc4, 0x78, 0x04, 0xb0, 0xfd, 
			0xdd, 0x40, 0x24, 0x29, 0x94, 0xd7, 0x98, 0x67, 0xa9, 0x3c, 0x62, 0x17, 0x6f, 0x42, 0xb3, 0x6e, 
			0x40, 0xd5, 0x82, 0x32, 0xe2, 0x19, 0x2d, 0x26, 0x25, 0x9a, 0x75, 0xd9, 0xd5, 0xc7, 0x1c, 0x20, 
			0x6f, 0x15, 0x67, 0x74, 0x4f, 0xd7, 0xe7, 0x02, 0x03, 0x01, 0x00, 0x01, 0xd5
	};
    const _u8 *key = pubKey;
	RSA_MY *rsa = NULL;


	if (NULL == p_input || input_len == 0
        || NULL == p_output || *p_output_len == 0)
	{
        sd_assert( FALSE );
        return -1;
	}

	rsa = openssli_d2i_RSAPublicKey(NULL, (_u8**)&key, sizeof(pubKey) / sizeof(_u8));
	if (NULL == rsa)
		return -2;

	rsa_size = openssli_RSA_size(rsa);
	if (rsa_size <= 0)
	{
        openssli_RSA_free(rsa);
        rsa = NULL;
		return -3;
	}

	if(input_len > rsa_size)
    {
        openssli_RSA_free(rsa);
        rsa = NULL;
		return -4;
    }
    
	if(*p_output_len < rsa_size)
    {
        sd_assert( FALSE );
        openssli_RSA_free(rsa);
        rsa = NULL;
		return -5;
    }    
    
	ret_val = openssli_RSA_public_decrypt(input_len, p_input, p_output, rsa, 1);
	if (ret_val == -1)
	{
        openssli_RSA_free(rsa);
        rsa = NULL;
		return -6;
	}
	*p_output_len = ret_val;
	openssli_RSA_free(rsa);
	rsa = NULL;
    
    LOG_DEBUG( "drm_rsa_decode_xlv_head, input_len:%u, output_len:%u", input_len, *p_output_len );
    return SUCCESS;
}


_int32 drm_rsa_decode_certificate( _u8 *p_input, _u32 input_len, _u8 *p_output, _u32 *p_output_len )
{
	RSA_MY *rsa = NULL;
    BIO_MY *bio = NULL;
    _u32 rsasize = 0;
    const _u8 pubKey[294] = {
			0x30, 0x82, 0x01, 0x22, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 
			0x01, 0x05, 0x00, 0x03, 0x82, 0x01, 0x0f, 0x00, 0x30, 0x82, 0x01, 0x0a, 0x02, 0x82, 0x01, 0x01, 
			0x00, 0x8a, 0xb6, 0xfb, 0x77, 0x6b, 0xf5, 0x53, 0x4d, 0xb2, 0xdb, 0x9a, 0x3e, 0x03, 0x7b, 0x32, 
			0x52, 0xae, 0x8d, 0x98, 0x18, 0xf6, 0x06, 0xff, 0x40, 0x6c, 0x84, 0x67, 0x0a, 0xac, 0xab, 0x90, 
			0xca, 0x06, 0xbf, 0x4a, 0xd3, 0x85, 0x2a, 0x50, 0xd3, 0x7b, 0x0c, 0x8a, 0xfa, 0xa0, 0xc3, 0xc7, 
			0xe7, 0xb5, 0xaa, 0x9f, 0x4d, 0xe8, 0xf5, 0x9b, 0xd1, 0xa9, 0x52, 0x54, 0x63, 0x9f, 0x9c, 0xf2, 
			0x52, 0x41, 0xcf, 0x06, 0x25, 0x77, 0x71, 0x06, 0x93, 0xec, 0x38, 0x20, 0xf5, 0xbb, 0xf0, 0x10, 
			0x23, 0xcd, 0x63, 0x2f, 0xcd, 0x71, 0xd4, 0xc2, 0x6b, 0x83, 0x0e, 0xb4, 0xea, 0xeb, 0xfc, 0x54, 
			0x2e, 0x30, 0xb2, 0x56, 0xa1, 0x83, 0xf1, 0x5c, 0xc8, 0xdd, 0x1b, 0x44, 0x65, 0xa8, 0xde, 0x24, 
			0x3b, 0x7b, 0xcf, 0xe6, 0x10, 0x8a, 0xf7, 0x23, 0x45, 0x9d, 0x8e, 0x95, 0x4d, 0xc7, 0xd2, 0x34, 
			0xa5, 0xc3, 0xb7, 0xe3, 0x7a, 0x71, 0xb7, 0xfe, 0x92, 0x3d, 0x8d, 0xd5, 0x66, 0x27, 0x89, 0xf8, 
			0xac, 0x72, 0x7e, 0x5c, 0x15, 0xbb, 0x13, 0x9e, 0xf5, 0xd0, 0x2f, 0x48, 0xcb, 0x2c, 0x1c, 0x70, 
			0x9f, 0x63, 0xec, 0xc0, 0xe9, 0xfd, 0x93, 0x91, 0x8f, 0xee, 0x2c, 0x8f, 0x90, 0x91, 0xe5, 0xd2, 
			0xa4, 0x93, 0xdd, 0x2e, 0x89, 0xc0, 0x07, 0xcd, 0x29, 0x40, 0x17, 0xc4, 0x76, 0xcb, 0xb5, 0x06, 
			0xbe, 0x42, 0xab, 0xfc, 0xcb, 0x23, 0xdf, 0x5a, 0x0b, 0xcc, 0xc8, 0xdc, 0xe4, 0xca, 0x40, 0x06, 
			0x8d, 0xc2, 0xc5, 0xf3, 0xef, 0xfd, 0x49, 0x08, 0x31, 0x92, 0x31, 0xdd, 0x79, 0xc0, 0xf4, 0x83, 
			0x46, 0x48, 0xe2, 0x05, 0xe3, 0xf3, 0xf7, 0xe0, 0x36, 0xad, 0xe7, 0x3e, 0x62, 0x51, 0x5f, 0xfb, 
			0x21, 0x6f, 0x94, 0x0e, 0xed, 0x5d, 0xf4, 0x1d, 0xce, 0xe7, 0x95, 0x74, 0x35, 0xae, 0xad, 0x67, 
			0xc1, 0x02, 0x03, 0x01, 0x00, 0x01
	};
	
	if (NULL == p_input || input_len == 0
        || NULL == p_output || *p_output_len == 0)
		return -1;

	bio = openssli_BIO_new_mem_buf((void*)pubKey, 294);	
	openssli_d2i_RSA_PUBKEY_bio(bio, &rsa);

	if (rsa == NULL)
	{
      	openssli_BIO_free(bio);
	    return -1;
	}

	rsasize = (_u32)openssli_RSA_size(rsa);
	if(*p_output_len < rsasize)
    {
        sd_assert( FALSE );
        openssli_BIO_free(bio);
        openssli_RSA_free(rsa);
        rsa = NULL;
		return -2;
    }    

	sd_memset( p_output, 0, *p_output_len);

	*p_output_len = openssli_RSA_public_decrypt( input_len, p_input, p_output, rsa, 1 );
	openssli_BIO_free(bio);
	openssli_RSA_free(rsa);
    
    LOG_DEBUG( "drm_rsa_decode_certificate, input_len:%u, output_len:%u", input_len, *p_output_len );
	return SUCCESS;
}

_int32 drm_aes_decode_xlmv_append_data( DRM_FILE *p_drm_file, char *p_append_data, _u32 data_len )
{
    _int32 ret_val = SUCCESS;
    _u32 key_data_len = data_len;
    ctx_md5 md5;
    _u8 szKey[16] = {0};
    _u8 key[32] = {
        0x2f, 0x1c, 0x22, 0x78, 0x00, 0xab, 0x02, 0xdd, 0xa2, 0x1f, 0x8c, 0x14, 0xec, 0x4b, 0x88, 0xbd,
        0x3e, 0x68, 0x08, 0x2c, 0xda, 0xcc, 0xd3, 0xe2, 0x5f, 0x6b, 0xfa, 0xb7, 0xf5, 0x81, 0x5e, 0x89
        };
   LOG_ERROR( "drm_aes_decode_xlmv_append_data" );

   ret_val = drm_aes_decode( (_u8*)p_append_data, &key_data_len, key, 32 );
   if( ret_val != SUCCESS ) return ret_val;

   if( key_data_len != DRM_KEY_LEN )
   {
       sd_assert( FALSE );
       LOG_ERROR( "drm_aes_decode_xlmv_append_data, key_data_len err:%d", key_data_len );
       return -1;
   }
   sd_memcpy( p_drm_file->_key, p_append_data, DRM_KEY_LEN );
   
   md5_initialize(&md5);
   md5_update(&md5, (unsigned char*)p_drm_file->_key,DRM_KEY_LEN);
   md5_finish(&md5, szKey);
   
   if( sd_memcmp( szKey, p_drm_file->_key_md5, KEY_MD5_LEN ) != 0 )
   {
       sd_assert( FALSE );
       LOG_ERROR( "drm_aes_decode_xlmv_append_data, _key_md5 err:0x%x", p_drm_file );
       return -1;
   }
   p_drm_file->_is_key_valid = TRUE;

   return SUCCESS;
}

_int32 drm_aes_decode_certificate( _u8 *pDataBuff, _u32 *nBuffLen )
{
    _int32 ret_val = SUCCESS;
    char peerid[PEER_ID_SIZE] = {0};
    ctx_md5 md5;
    _u8 szKey[16];

    LOG_DEBUG("drm_aes_decode_certificate.");

    ret_val = get_peerid( peerid, PEER_ID_SIZE );
    CHECK_VALUE( ret_val );
    
    md5_initialize(&md5);
    md5_update(&md5, (unsigned char*)peerid, PEER_ID_SIZE);
    md5_finish(&md5, szKey);
    
    return drm_aes_decode( pDataBuff, nBuffLen, szKey, 16 );

}


_int32 drm_aes_decode( _u8 *pDataBuff, _u32 *nBuffLen, _u8 *key, _u32 key_len )
{
    _int32 ret_val = SUCCESS;
    _int32  nOutLen;
    ctx_aes aes;
    _int32 nInOffset;
    _int32 nOutOffset;
	char *pOutBuff;
    _u8 inBuff[ENCRYPT_BLOCK_SIZE],ouBuff[ENCRYPT_BLOCK_SIZE];
    char * out_ptr;

    if (pDataBuff == NULL
        || nBuffLen == NULL)
    {
        return FALSE;
    }
    if (*nBuffLen%ENCRYPT_BLOCK_SIZE != 0)
    {
        return -1;
    }

	ret_val = sd_malloc( *nBuffLen + 16, (void**)&pOutBuff);
	if(ret_val != SUCCESS)
	{
		LOG_DEBUG("drm_aes_decode, malloc failed.");
		CHECK_VALUE(ret_val);
	}

    nOutLen = 0;

    aes_init(&aes,key_len,(unsigned char*)key);
    nInOffset = 0;
    nOutOffset = 0;
    sd_memset(inBuff,0,ENCRYPT_BLOCK_SIZE);
    sd_memset(ouBuff,0,ENCRYPT_BLOCK_SIZE);
    while(*nBuffLen - nInOffset > 0)
    {
        sd_memcpy(inBuff,pDataBuff+nInOffset,ENCRYPT_BLOCK_SIZE);
        aes_invcipher(&aes, inBuff,ouBuff);
        sd_memcpy(pOutBuff+nOutOffset,ouBuff,ENCRYPT_BLOCK_SIZE);
        nInOffset += ENCRYPT_BLOCK_SIZE;
        nOutOffset += ENCRYPT_BLOCK_SIZE;
    }
    nOutLen = nOutOffset;
	sd_memcpy(pDataBuff,pOutBuff,nOutLen);
	out_ptr = pOutBuff + nOutLen - 1;
    if (*out_ptr <= 0 || *out_ptr > ENCRYPT_BLOCK_SIZE)
    {
        ret_val = -1;
    }
    else
    {
        if(nOutLen - *out_ptr < *nBuffLen)
        {
            *nBuffLen = nOutLen - *out_ptr;
            ret_val = SUCCESS;
        }
        else
        {
            ret_val = -1;
        }
    }
    sd_free(pOutBuff);
	pOutBuff = NULL;
    LOG_DEBUG( "drm_aes_decode, ret_val:%d", ret_val );
#ifdef REPORTER_DEBUG
    {
        char test_buffer[1024];
        sd_memset(test_buffer,0,1024);
        str2hex( (char*)(pDataBuff), *nBuffLen, test_buffer, 1024);
        LOG_DEBUG( "drm_aes_decode:*buffer[%u]=%s",
            (_u32)*nBuffLen, test_buffer);
    }
#endif /* _DEBUG */

    
    return ret_val;

}


/*
_int32 drm_rsa_decode_xlv_head( _u8 *p_input, _u32 input_len, _u8 *p_output, _u32 *p_output_len )
{
    _int32 ret_val = 0;
    R_RSA_PUBLIC_KEY key;
    _u32 *exponent = (_u32*)&key.exponent[124];
    _u32 index = 0;
    _u8 modues[MAX_RSA_MODULUS_LEN] =
    {
               0xed,  0xfc,  0x3a,  0x50,  0xa4,  0x73,  0xee,  0x9b,  0xbb,  0xb8,  0x79,  0x01,  0x16,  0x22,  
        0x97,  0xfa,  0xb8,  0xd0,  0x38,  0xd4,  0xcb,  0xa8,  0x7d,  0xed,  0xed,  0x42,  0x53,  0x67,  0x15, 
        0xf7,  0x7b,  0xd8,  0x12,  0x06,  0x27,  0x38,  0x90,  0xfe,  0xda,  0x1c,  0xce,  0x7f,  0x51,  0xd3,  
        0xc0,  0x98,  0xbd,  0x94,  0xb4,  0x27,  0x9a,  0x1a,  0x1b,  0x7e,  0xc1,  0x32,  0x9c,  0x94,  0xa6,  
        0xc8,  0x2c,  0x2f,  0xa6,  0x45,  0xf6,  0xb4,  0xfa,  0x8e,  0x27,  0xab,  0x77,  0xa4,  0x9d,  0xeb,  
        0x4b,  0x78,  0xcc,  0x0c,  0xa4,  0xa7,  0x9f,  0x8a,  0xe2,  0xfb,  0xc4,  0x78,  0x04,  0xb0,  0xfd,  
        0xdd,  0x40,  0x24,  0x29,  0x94,  0xd7,  0x98,  0x67,  0xa9,  0x3c,  0x62,  0x17,  0x6f,  0x42,  0xb3,  
        0x6e,  0x40,  0xd5,  0x82,  0x32,  0xe2,  0x19,  0x2d,  0x26,  0x25,  0x9a,  0x75,  0xd9,  0xd5,  0xc7,  
        0x1c,  0x20,  0x6f,  0x15,  0x67,  0x74,  0x4f,  0xd7,  0xe7
    };

    sd_memset( &key, 0, sizeof(key) );

    key.bits = 1024;
    //sd_memcpy( &key.modulus, modues, MAX_RSA_MODULUS_LEN );
    for( ; index < MAX_RSA_MODULUS_LEN; index++ )
    {
        key.modulus[index] = modues[MAX_RSA_MODULUS_LEN-1-index];
    }
    
    *exponent = 65537;
    
    ret_val = RSAPublicDecrypt( p_output, p_output_len, p_input, input_len, &key );
    return SUCCESS;
}
*/



_int32 drm_pipe_change_range(struct tagDATA_PIPE *p_data_pipe, const struct _tag_range *range, BOOL cancel_flag )
{
	return http_pipe_change_ranges(p_data_pipe, range);
}

_int32 drm_pipe_get_dispatcher_range(struct tagDATA_PIPE *p_data_pipe, const struct _tag_range *p_dispatcher_range, void *p_pipe_range )
{
	RANGE* range = (RANGE*)p_pipe_range;
	range->_index = p_dispatcher_range->_index;
	range->_num = p_dispatcher_range->_num;
	return SUCCESS;
}

_int32 drm_pipe_set_dispatcher_range(struct tagDATA_PIPE *p_data_pipe, const void *p_pipe_range, struct _tag_range *p_dispatcher_range )
{
	RANGE* range = (RANGE*)p_pipe_range;
	p_dispatcher_range->_index = range->_index;
	p_dispatcher_range->_num = range->_num;
	return SUCCESS;
}

_u64 drm_pipe_get_file_size( struct tagDATA_PIPE *p_data_pipe)
{
	return 0;
}

_int32 drm_pipe_set_file_size( struct tagDATA_PIPE *p_data_pipe, _u64 filesize)
{
	return SUCCESS;
}

_int32 drm_pipe_get_data_buffer( struct tagDATA_PIPE *p_data_pipe, char **pp_data_buffer, _u32 *p_data_len)
{
	//return dm_get_data_buffer(NULL, pp_data_buffer, p_data_len);
	_int32 ret_val = SUCCESS;
	ret_val = sd_malloc(DRM_DATA_BUFFRE_LEN, (void**)pp_data_buffer);
	*p_data_len = DRM_DATA_BUFFRE_LEN;
	
	LOG_DEBUG("drm_pipe_get_data_buffer.ret:%d", ret_val );
	return ret_val;
}

_int32 drm_pipe_free_data_buffer( struct tagDATA_PIPE *p_data_pipe, char **pp_data_buffer, _u32 data_len)
{
	LOG_DEBUG("drm_pipe_free_data_buffer.");
	//dm_free_data_buffer(NULL, pp_data_buffer, data_len);
	//return SUCCESS;

	sd_free( *pp_data_buffer );
	*pp_data_buffer = NULL;
	return SUCCESS;
}

_int32 drm_pipe_put_data_buffer( struct tagDATA_PIPE *p_data_pipe, const struct _tag_range *p_range, char **pp_data_buffer, _u32 data_len,_u32 buffer_len, struct tagRESOURCE *p_resource )
{
    _int32 ret_val = SUCCESS;
    DRM_FILE *p_drm_file = NULL;
    char sign = '\n';
    char *p_tmp = NULL, *p_data = NULL;
    _u32 key_len = 0;
    const char *p_Certificate = "Certificate";
    const char *p_UserNotFoundError = "UserNotFoundError";
    const char *p_NotBoughtError = "NotBoughtError";
    const char *p_NotOnSaleError = "NotOnSaleError";
    const char *p_PeerIdBindError = "PeerIdBindError";
    const char *p_ServerBusyError = "ServerBusyError";
    const char *p_KeyNotFoundError = "KeyNotFoundError";
    const char *p_BerylFreeProductError = "BerylFreeProductError";
    const char *p_BerylTrialNotFreeError = "BerylTrialNotFreeError";

    
    LOG_DEBUG( "drm_pipe_put_data_buffer, data_pipe:0x%x, buffer:%s, len:%d", 
        p_data_pipe, *pp_data_buffer, data_len );
    
    ret_val = drm_get_fille_ptr_by_pipe( p_data_pipe, &p_drm_file );
    if( ret_val != SUCCESS )
    {
        sd_assert( FALSE );
        drm_enter_err( p_drm_file, ret_val );
        return SUCCESS;
    }

    p_tmp = sd_strchr( *pp_data_buffer, sign, 0 );
    if( p_tmp == NULL )
    {
        drm_enter_err( p_drm_file, DRM_CERTIFICATE_DOWNLOAD_ERR );
        return SUCCESS;
    }

    p_data = *pp_data_buffer;
    key_len = p_tmp - *pp_data_buffer;

    if( sd_strncmp( p_data, p_Certificate,  key_len ) == 0 )
    {
        p_data = p_tmp+1;
        if( key_len + 1 >= data_len )
        {
            drm_enter_err( p_drm_file, DRM_CERTIFICATE_DOWNLOAD_ERR );
            return SUCCESS;
        }
        ret_val = drm_write_certificate_data( p_drm_file, p_data, data_len - key_len - 1 );
        if( ret_val != SUCCESS )
        {
            drm_enter_err( p_drm_file, ret_val );
            return SUCCESS;
        }
    }
    else if( sd_strncmp( p_data, p_UserNotFoundError,  key_len ) == 0 )
    {
        drm_enter_err( p_drm_file, DRM_USER_NOT_FOUND_ERR );
    }
    else if( sd_strncmp( p_data, p_NotBoughtError,  key_len ) == 0 )
    {
        drm_enter_err( p_drm_file, DRM_NOT_BOUGHT_ERR );
    }
    else if( sd_strncmp( p_data, p_NotOnSaleError,  key_len ) == 0 )
    {
        drm_enter_err( p_drm_file, DRM_NOT_ON_SALE_ERR );
    }
    else if( sd_strncmp( p_data, p_PeerIdBindError,  key_len ) == 0 )
    {
        drm_enter_err( p_drm_file, DRM_PEERID_BIND_ERR );
    }
    else if( sd_strncmp( p_data, p_ServerBusyError,  key_len ) == 0 )
    {
        drm_enter_err( p_drm_file, DRM_SERVER_BUSY_ERR );
    }
    else if( sd_strncmp( p_data, p_KeyNotFoundError,  key_len ) == 0 )
    {
        drm_enter_err( p_drm_file, DRM_KEY_NOT_FOUND_ERR );
    }
    else if( sd_strncmp( p_data, p_BerylFreeProductError,  key_len ) == 0 )
    {
        drm_enter_err( p_drm_file, DRM_BERYL_FREE_PRODUCT_ERR );
    }	
	else if( sd_strncmp( p_data, p_BerylTrialNotFreeError,  key_len ) == 0 )
    {
        drm_enter_err( p_drm_file, DRM_BERY_TRIAL_NOT_FREE_ERROR );
    }	
	else 
	{
        drm_enter_err( p_drm_file, DRM_UNKNOWN_SERVER_ERR );
	}

    //dm_free_data_buffer(NULL, pp_data_buffer, buffer_len);
    
    return SUCCESS;
}

_int32 drm_write_certificate_data( DRM_FILE *p_drm_file, char *p_buffer, _u32 data_len )
{
    _int32 ret_val = SUCCESS;
    _u32 file_id = INVALID_FILE_ID;
    char cert_file_full_name[MAX_FULL_PATH_LEN] = {0};
    _u32 write_size = 0;
    
    LOG_DEBUG( "drm_write_certificate_data, data_len:%d", data_len );

    ret_val = drm_get_certificate_file_full_path( p_drm_file, cert_file_full_name );
    if( ret_val != SUCCESS )
    {
        sd_assert( FALSE );
        return SUCCESS;
    }

    ret_val = sd_open_ex( cert_file_full_name, O_FS_CREATE, &file_id );
    if( ret_val != SUCCESS )
    {
        return ret_val;
    }

    ret_val = sd_write( file_id, p_buffer, data_len, &write_size );
    if( ret_val != SUCCESS
        || write_size != data_len )
    {
        sd_assert( FALSE );
        sd_close_ex( file_id );
        return SUCCESS;
    }
    
    sd_close_ex( file_id );
    p_drm_file->_is_write_file_ok = TRUE;
    
    LOG_DEBUG( "drm_write_certificate_data SUCCESS, data_len:%d, path:%s", data_len, cert_file_full_name );
	return SUCCESS;
}

_int32  drm_pipe_read_data( struct tagDATA_PIPE *p_data_pipe, void *p_user, struct _tag_range_data_buffer* p_data_buffer, notify_read_result p_call_back )
{
	return SUCCESS;
}


_int32 drm_pipe_notify_dispatch_data_finish( struct tagDATA_PIPE *p_data_pipe)
{
	return SUCCESS;
}

_int32 drm_pipe_get_file_type( struct tagDATA_PIPE* p_data_pipe)
{
	return 0;
}

#else
/* 仅仅是为了在MACOS下能顺利编译 */
#if defined(MACOS)&&(!defined(MOBILE_PHONE))
void drm_test(void)
{
	return ;
}
#endif /* defined(MACOS)&&(!defined(MOBILE_PHONE)) */
#endif

