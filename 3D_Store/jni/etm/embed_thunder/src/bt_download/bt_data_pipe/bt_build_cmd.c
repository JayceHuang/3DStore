#include "utility/define.h"
#ifdef ENABLE_BT 
#ifdef ENABLE_BT_PROTOCOL

#include "bt_build_cmd.h"
#include "bt_cmd_define.h"
#include "bt_data_pipe.h"
#include "bt_download/bt_utility/bt_utility.h"
#include "utility/bitmap.h"
#include "utility/errcode.h"
#include "utility/bytebuffer.h"
#include "utility/sd_assert.h"
#include "utility/mempool.h"
#include "../torrent_parser/bencode.h"
#include "p2p_transfer_layer/ptl_utility.h"

#include "utility/logid.h"
#define	LOGID	LOGID_BT_DOWNLOAD
#include "utility/logger.h"

_int32 bt_build_handshake_cmd(char** buffer, _u32* len, 
    _u8 info_hash[BT_INFO_HASH_LEN], BOOL is_magnet)
{
	_int32 ret = SUCCESS, tmp_len = 0;
	char resevered[8] = {0};
	char local_peerid[BT_PEERID_LEN + 1] = {0}; 
	char* tmp_buf = NULL;
	*len = 68;
	ret = sd_malloc(*len, (void**)buffer);
	if(ret != SUCCESS)
		return ret;
	bt_get_local_peerid(local_peerid, BT_PEERID_LEN);
	tmp_buf = *buffer;
	tmp_len = *len;
	sd_set_int8(&tmp_buf, &tmp_len, 19);
	sd_set_bytes(&tmp_buf, &tmp_len, BT_PROTOCOL_STRING, BT_PROTOCOL_STRING_LEN);
	resevered[0] = 'e';
	resevered[1] = 'x';
	resevered[7] = '\x01';
    if(is_magnet)
    {
       resevered[5] = '\x10';
    }
	sd_set_bytes(&tmp_buf, &tmp_len, resevered, 8);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)info_hash, BT_INFO_HASH_LEN);
	ret = sd_set_bytes(&tmp_buf, &tmp_len, local_peerid, BT_PEERID_LEN);
	sd_assert(tmp_len == 0);
	if(ret != SUCCESS)
	{
		LOG_ERROR("bt_build_handshake_cmd failed, errcode = %d.", ret);
		sd_free(*buffer);
		*buffer = NULL;
	}
	return ret;
}

_int32 bt_build_ex_handshake_cmd(char** buffer, _u32* len)
{
    _int32 ret_val = SUCCESS;
    BC_DICT *p_m_dict = NULL, *p_ex_dict = NULL;
    BC_INT *p_tmp_int = NULL;
    BC_STR *p_v_str = NULL;
    char bc_buffer[1024];
    _u32 bc_buffer_len = 1024, data_len = 0;
    const char *p_v = "|¨¬Torrent 2.2.16";
    _u32 port = ptl_get_local_tcp_port();
	char* tmp_buf = NULL;
    _int32 tmp_len = 0;

    ret_val = bc_dict_create( &p_m_dict );
    CHECK_VALUE( ret_val );

    ret_val = bc_int_create_with_value( 3, &p_tmp_int );
    if( ret_val != SUCCESS ) goto ErrHandler;

    ret_val = bc_dict_set_value( p_m_dict, "upload_only", (BC_OBJ *)p_tmp_int );
    if( ret_val != SUCCESS ) goto ErrHandler1;

    ret_val = bc_int_create_with_value( 4, &p_tmp_int );
    if( ret_val != SUCCESS ) goto ErrHandler;

    ret_val = bc_dict_set_value( p_m_dict, "ut_holepunch", (BC_OBJ *)p_tmp_int );
    if( ret_val != SUCCESS ) goto ErrHandler1;

    ret_val = bc_int_create_with_value( 3, &p_tmp_int );
    if( ret_val != SUCCESS ) goto ErrHandler;

    ret_val = bc_dict_set_value( p_m_dict, "ut_metadata", (BC_OBJ *)p_tmp_int );
    if( ret_val != SUCCESS ) goto ErrHandler1;

    ret_val = bc_int_create_with_value( 1, &p_tmp_int );
    if( ret_val != SUCCESS ) goto ErrHandler;

    ret_val = bc_dict_set_value( p_m_dict, "ut_pex", (BC_OBJ *)p_tmp_int );
    if( ret_val != SUCCESS ) goto ErrHandler1;

    ret_val = bc_dict_create( &p_ex_dict );
    if( ret_val != SUCCESS ) goto ErrHandler;

    ret_val = bc_dict_set_value( p_ex_dict, "m", (BC_OBJ *)p_m_dict );
    if( ret_val != SUCCESS ) 
    {
       bc_dict_uninit( (BC_OBJ *)p_ex_dict );
       goto ErrHandler; 
    }

    ret_val = bc_int_create_with_value( 0, &p_tmp_int );
    if( ret_val != SUCCESS ) goto ErrHandler2;

    ret_val = bc_dict_set_value( p_ex_dict, "e", (BC_OBJ *)p_tmp_int );
    if( ret_val != SUCCESS ) goto ErrHandler3;  
    
    ret_val = bc_int_create_with_value( port, &p_tmp_int );
    if( ret_val != SUCCESS ) goto ErrHandler2;

    ret_val = bc_dict_set_value( p_ex_dict, "p", (BC_OBJ *)p_tmp_int );
    if( ret_val != SUCCESS ) goto ErrHandler3; 

    ret_val = bc_int_create_with_value( 512, &p_tmp_int );
    if( ret_val != SUCCESS ) goto ErrHandler2;

    ret_val = bc_dict_set_value( p_ex_dict, "reqq", (BC_OBJ *)p_tmp_int );
    if( ret_val != SUCCESS ) goto ErrHandler3; 

    ret_val = bc_str_create_with_value( p_v, sd_strlen(p_v), &p_v_str );
    if( ret_val != SUCCESS ) goto ErrHandler2;

    ret_val = bc_dict_set_value( p_ex_dict, "v", (BC_OBJ *)p_v_str );
    if( ret_val != SUCCESS ) 
    {
        bc_str_uninit( (BC_OBJ *)p_v_str );
        goto ErrHandler2; 
    }

    sd_memset( bc_buffer, 0, bc_buffer_len );

    ret_val = bc_dict_to_str( (BC_OBJ *)p_ex_dict, bc_buffer, bc_buffer_len, &data_len );
    if( ret_val != SUCCESS ) goto ErrHandler2;
    
    bc_dict_uninit( (BC_OBJ *)p_ex_dict );

    *len = data_len + 6;
    tmp_len = *len;
    
    ret_val = sd_malloc( *len, (void **)buffer );
    if( ret_val != SUCCESS ) goto ErrHandler2;    
    
	tmp_buf = *buffer;
	sd_set_int32_to_bg(&tmp_buf, &tmp_len, data_len+2);
	sd_set_int8(&tmp_buf, &tmp_len, '\x14' );
	sd_set_int8(&tmp_buf, &tmp_len, '\x00' );
    
	ret_val = sd_set_bytes(&tmp_buf, &tmp_len, bc_buffer, data_len);
    if( ret_val != SUCCESS ) goto ErrHandler4;

    sd_assert( tmp_len == 0 );
    return SUCCESS;

ErrHandler4:
    SAFE_DELETE( *buffer );
    return ret_val;

ErrHandler3:
    bc_int_uninit( (BC_OBJ *)p_tmp_int );

ErrHandler2:
    bc_dict_uninit( (BC_OBJ *)p_ex_dict );
    return ret_val;


ErrHandler1:
    bc_int_uninit( (BC_OBJ *)p_tmp_int );

ErrHandler:
    bc_dict_uninit( (BC_OBJ *)p_m_dict );
    return ret_val;

}

_int32 bt_build_metadata_request_cmd(char** buffer, _u32* len, _u32 metadata_type, _u32 piece_index )
{
    _int32 ret_val = SUCCESS;
    BC_DICT *p_dict = NULL;
    BC_INT *p_tmp_int = NULL;
    char bc_buffer[1024];
    _u32 bc_buffer_len = 1024, data_len = 0;
    _int32 tmp_len = 0;
	char* tmp_buf = NULL;
    
    ret_val = bc_dict_create( &p_dict );
    CHECK_VALUE( ret_val );

    ret_val = bc_int_create_with_value( 0, &p_tmp_int );
    if ( ret_val ) goto ErrHandler; 

    ret_val = bc_dict_set_value( p_dict, "msg_type", (BC_OBJ *)p_tmp_int );
    if ( ret_val ) goto ErrHandler1; 
    
    ret_val = bc_int_create_with_value( piece_index, &p_tmp_int );
    if ( ret_val ) goto ErrHandler; 

    ret_val = bc_dict_set_value( p_dict, "piece", (BC_OBJ *)p_tmp_int );
    if ( ret_val ) goto ErrHandler1; 
    
    sd_memset( bc_buffer, 0, bc_buffer_len );

    ret_val = bc_dict_to_str( (BC_OBJ *)p_dict, bc_buffer, bc_buffer_len, &data_len );
    if( ret_val != SUCCESS ) goto ErrHandler1;
    
    bc_dict_uninit( (BC_OBJ *)p_dict );

    *len = data_len + 6;
    tmp_len = *len;

    ret_val = sd_malloc( *len, (void **)buffer );
    CHECK_VALUE( ret_val );
    
	tmp_buf = *buffer;
	sd_set_int32_to_bg(&tmp_buf, &tmp_len, data_len+2);
	sd_set_int8(&tmp_buf, &tmp_len, '\x14' );
	sd_set_int8(&tmp_buf, &tmp_len, metadata_type );
    
	ret_val = sd_set_bytes(&tmp_buf, &tmp_len, bc_buffer, data_len);
    if( ret_val != SUCCESS ) goto ErrHandler2;

    sd_assert( tmp_len == 0 );
    return SUCCESS;

ErrHandler2:
    SAFE_DELETE( *buffer );
    return ret_val;


ErrHandler1:
    bc_int_uninit( (BC_OBJ *)p_tmp_int );
    
ErrHandler:
    bc_dict_uninit( (BC_OBJ *)p_dict );
    return ret_val;
}

_int32 bt_build_choke_cmd(char** buffer, _u32* len, BOOL is_choke)
{
	_int32 ret = SUCCESS, tmp_len = 0;
	char* tmp_buf = NULL;
	*len = BT_PRO_HEADER_LEN + 1;
	ret = sd_malloc(*len, (void**)buffer);
	if(ret != SUCCESS)
		return ret;
	tmp_buf = *buffer;
	tmp_len = *len;
	sd_set_int32_to_bg(&tmp_buf, &tmp_len, 1);
	if(is_choke == TRUE)
		ret = sd_set_int8(&tmp_buf, &tmp_len, BT_CHOKE);
	else
		ret = sd_set_int8(&tmp_buf, &tmp_len, BT_UNCHOKE);
	sd_assert(tmp_len == 0);
	if(ret != SUCCESS)
	{
		LOG_ERROR("bt_build_choke_cmd failed, errcode = %d.", ret);
		sd_free(*buffer);
		*buffer = NULL;
	}
	return ret;
}

_int32 bt_build_bitfield_cmd(char** buffer, _u32* len, const BITMAP* bitmap_i_have)
{
	_int32 ret = SUCCESS, tmp_len = 0;
	char* tmp_buf = NULL;
	*len =BT_PRO_HEADER_LEN + 1 + (bitmap_i_have->_bit_count + 7) / 8;
	ret = sd_malloc(*len, (void**)buffer);
	if(ret != SUCCESS)
		return ret;
	tmp_buf = *buffer;
	tmp_len = *len;
	sd_set_int32_to_bg(&tmp_buf, &tmp_len, (_int32)(*len - BT_PRO_HEADER_LEN));
	ret = sd_set_int8(&tmp_buf, &tmp_len, BT_BITFIELD);
	if(bitmap_i_have->_bit_count != 0)
		ret = sd_set_bytes(&tmp_buf, &tmp_len, (char*)bitmap_i_have->_bit, (bitmap_i_have->_bit_count + 7) / 8);
	sd_assert(tmp_len == 0);
	if(ret != SUCCESS)
	{
		LOG_ERROR("bt_build_bitfield_cmd failed, errcode = %d.", ret);
		sd_free(*buffer);
		*buffer = NULL;
	}
	return ret;
}

_int32 bt_build_interested_cmd(char** buffer, _u32* len, BOOL is_interest)
{
	_int32 ret = SUCCESS, tmp_len = 0;
	char* tmp_buf = NULL;
	*len = 5;
	ret = sd_malloc(*len, (void**)buffer);
	if(ret != SUCCESS)
		return ret;
	tmp_buf = *buffer;
	tmp_len = *len;
	sd_set_int32_to_bg(&tmp_buf, &tmp_len, 1);
	if(is_interest == TRUE)
		ret = sd_set_int8(&tmp_buf, &tmp_len, BT_INTERESTED);
	else
		ret = sd_set_int8(&tmp_buf, &tmp_len, BT_NOT_INTERESTED);
	sd_assert(tmp_len == 0);
	if(ret != SUCCESS)
	{
		LOG_ERROR("bt_build_interst_cmd failed, errcode = %d.", ret);
		sd_free(*buffer);
		*buffer = NULL;
	}
	return ret;
}

_int32 bt_build_have_cmd(char** buffer, _u32* len, _u32 piece_index)
{
	_int32 ret = SUCCESS, tmp_len = 0;
	char* tmp_buf = NULL;
	*len = 9;
	ret = sd_malloc(*len, (void**)buffer);
	if(ret != SUCCESS)
		return ret;
	tmp_buf = *buffer;
	tmp_len = *len;
	sd_set_int32_to_bg(&tmp_buf, &tmp_len, 5);
	sd_set_int8(&tmp_buf, &tmp_len, BT_HAVE);
	ret = sd_set_int32_to_bg(&tmp_buf, &tmp_len, (_int32)piece_index);
	sd_assert(tmp_len == 0);
	if(ret != SUCCESS)
	{
		LOG_ERROR("bt_build_have_cmd failed, errcode = %d.", ret);
		sd_free(*buffer);
		*buffer = NULL;
	}
	return ret;
}

_int32 bt_build_request_cmd(char** buffer, _u32* len, BT_PIECE_DATA* piece)
{
	_int32 ret = SUCCESS, tmp_len = 0;
	char* tmp_buf = NULL;
	*len = 17;
	ret = sd_malloc(*len, (void**)buffer);
	if(ret != SUCCESS)
		return ret;
	tmp_buf = *buffer;
	tmp_len = *len;
	sd_set_int32_to_bg(&tmp_buf, &tmp_len, *len - BT_PRO_HEADER_LEN);
	sd_set_int8(&tmp_buf, &tmp_len, BT_REQUEST);
	sd_set_int32_to_bg(&tmp_buf, &tmp_len, (_int32)piece->_index);
	sd_set_int32_to_bg(&tmp_buf, &tmp_len, (_int32)piece->_offset);
	ret = sd_set_int32_to_bg(&tmp_buf, &tmp_len, (_int32)piece->_len);
	sd_assert(tmp_len == 0);
	if(ret != SUCCESS)
	{
		LOG_ERROR("bt_build_request_cmd failed, errcode = %d.", ret);
		sd_free(*buffer);
		*buffer = NULL;
	}
	return ret;
}

_int32 bt_build_piece_cmd(char** buffer, _u32* len, BT_PIECE_DATA* piece, char* data)
{
	_int32 ret = SUCCESS, tmp_len = 0;
	char* tmp_buf = NULL;
	*len = piece->_len + 13;
	ret = sd_malloc(*len, (void**)buffer);
	if(ret != SUCCESS)
		return ret;
	tmp_buf = *buffer;
	tmp_len = *len;
	sd_set_int32_to_bg(&tmp_buf, &tmp_len, *len - BT_PRO_HEADER_LEN);
	sd_set_int8(&tmp_buf, &tmp_len, BT_PIECE);
	sd_set_int32_to_bg(&tmp_buf, &tmp_len, (_int32)piece->_index);
	sd_set_int32_to_bg(&tmp_buf, &tmp_len, (_int32)piece->_offset);
	ret = sd_set_bytes(&tmp_buf, &tmp_len, data, piece->_len);
	sd_assert(tmp_len == 0);
	if(ret != SUCCESS)
	{
		LOG_ERROR("bt_build_piece_cmd failed, errcode = %d.", ret);
		sd_free(*buffer);
		*buffer = NULL;
	}
	return ret;
}

_int32 bt_build_cancel_cmd(char** buffer, _u32* len, BT_PIECE_DATA* piece)
{
	_int32 ret = SUCCESS, tmp_len = 0;
	char* tmp_buf = NULL;
	*len = 17;
	ret = sd_malloc(*len, (void**)buffer);
	if(ret != SUCCESS)
		return ret;
	tmp_buf = *buffer;
	tmp_len = *len;
	sd_set_int32_to_bg(&tmp_buf, &tmp_len, *len - BT_PRO_HEADER_LEN);
	sd_set_int8(&tmp_buf, &tmp_len, BT_CANCEL);
	sd_set_int32_to_bg(&tmp_buf, &tmp_len, (_int32)piece->_index);
	sd_set_int32_to_bg(&tmp_buf, &tmp_len, (_int32)piece->_offset);
	ret = sd_set_int32_to_bg(&tmp_buf, &tmp_len, (_int32)piece->_len);
	sd_assert(tmp_len == 0);
	if(ret != SUCCESS)
	{
		LOG_ERROR("bt_build_cancel_cmd failed, errcode = %d.", ret);
		sd_free(*buffer);
		*buffer = NULL;
	}
	return ret;
}

_int32 bt_build_a0_cmd(char** buffer, _u32* len)
{
	_int32 ret = SUCCESS;
	char ex[] = {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa9, 0xaa, 0xad, 0xac, 0xc0};
	char* tmp_buf = NULL;
	_int32 tmp_len = 0;
	*len = 13 + BT_PRO_HEADER_LEN;
	ret = sd_malloc(*len, (void**)buffer);
	if(ret != SUCCESS)
		return ret;
	tmp_buf = *buffer;
	tmp_len = *len;
	sd_set_int32_to_bg(&tmp_buf, &tmp_len, (_int32)(*len - BT_PRO_HEADER_LEN));
	sd_set_int8(&tmp_buf, &tmp_len, BT_A0);
	ret = sd_set_bytes(&tmp_buf, &tmp_len, ex, 12);
	sd_assert(tmp_len == 0);
	if(ret != SUCCESS)
	{
		LOG_ERROR("bt_build_a0_cmd failed, errcode = %d.", ret);
		sd_free(*buffer);
		*buffer = NULL;
	}
	return ret;
}

_int32 bt_build_a1_cmd(char** buffer, _u32* len)
{
	char* tmp_buf = NULL;
	_int32 ret = SUCCESS, tmp_len = 0;
	*len = 13 + BT_PRO_HEADER_LEN;
	ret = sd_malloc(*len, (void**)buffer);
	if(ret != SUCCESS)
		return ret;
	tmp_buf = *buffer;
	tmp_len = *len;
	sd_set_int32_to_bg(&tmp_buf, &tmp_len, (_int32)(*len - BT_PRO_HEADER_LEN));
	sd_set_int8(&tmp_buf, &tmp_len, BT_A1);
	sd_set_int32_to_bg(&tmp_buf, &tmp_len, 0);
	sd_set_int32_to_bg(&tmp_buf, &tmp_len, 28);
	ret = sd_set_int32_to_bg(&tmp_buf, &tmp_len, 0);
	sd_assert(tmp_len == 0);
	if(ret != SUCCESS)
	{
		LOG_ERROR("bt_build_a1_cmd failed, errcode = %d.", ret);
		sd_free(*buffer);
		*buffer = NULL;
	}
	return ret;
}

_int32 bt_build_a2_cmd(char** buffer, _u32* len, char* bt_peerid, _u32 ip, _u16 port)
{
	char* tmp_buf = NULL;
	_int32 ret = SUCCESS, tmp_len = 0;
	*len = 37 + BT_PRO_HEADER_LEN;
	ret = sd_malloc(*len, (void**)buffer);
	if(ret != SUCCESS)
		return ret;
	tmp_buf = *buffer;
	tmp_len = *len;
	sd_set_int32_to_bg(&tmp_buf, &tmp_len, (_int32)(*len - BT_PRO_HEADER_LEN));
	sd_set_int8(&tmp_buf, &tmp_len, BT_A2);
	sd_set_int32_to_bg(&tmp_buf, &tmp_len, 0);
	sd_set_int32_to_bg(&tmp_buf, &tmp_len, 28);
	sd_set_bytes(&tmp_buf, &tmp_len, bt_peerid, BT_PEER_ID_LEN);
	sd_memcpy(tmp_buf, &ip, sizeof(_u32));
	tmp_buf += sizeof(_u32);
	tmp_len -= sizeof(_u32);
	sd_set_int16_to_bg(&tmp_buf, &tmp_len, (_int16)port);
	ret = sd_set_int16_to_bg(&tmp_buf, &tmp_len, 0);
	sd_assert(tmp_len == 0);
	if(ret != SUCCESS)
	{
		LOG_ERROR("bt_build_a2_cmd failed, errcode = %d.", ret);
		sd_free(*buffer);
		*buffer = NULL;
	}
	return ret;
}
#endif
#endif

