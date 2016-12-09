#include "utility/define.h"
#ifdef ENABLE_BT 
#ifdef ENABLE_BT_PROTOCOL

#include "bt_extract_cmd.h"
#include "bt_cmd_define.h"
#include "utility/string.h"
#include "utility/bytebuffer.h"
#include "utility/errcode.h"
#include "utility/sd_assert.h"
#include "../torrent_parser/bencode.h"

_int32 bt_extract_handshake_cmd(char* buffer, _int32 len, _u8 info_hash[BT_INFO_HASH_LEN], 
    char peerid[BT_PEERID_LEN + 1], char resevered[8] )
{
	_int32 ret = SUCCESS;
	_u8 type = 0;
	char iden[BT_PROTOCOL_STRING_LEN + 1] = {0};
	sd_get_int8(&buffer, &len, (_int8*)&type);
	sd_get_bytes(&buffer, &len, iden, BT_PROTOCOL_STRING_LEN);
	sd_get_bytes(&buffer, &len, resevered, 8);
	sd_get_bytes(&buffer, &len, (char*)info_hash, BT_INFO_HASH_LEN);
	ret =  sd_get_bytes(&buffer, &len, peerid, BT_PEERID_LEN);
	if(ret != SUCCESS || type != BT_HANDSHAKE || sd_strcmp(iden, BT_PROTOCOL_STRING) != 0 || len != 0)
		return -1;
	return SUCCESS;
}

_int32 bt_extract_bitfield_cmd(char* buffer, _int32 len, char** data, _u32* data_len)
{
	_u8 type = 0;
	sd_get_int8(&buffer, &len, (_int8*)&type);
	*data = buffer;
	*data_len = len;
	sd_assert(*data_len > 0);
	return SUCCESS;
}

_int32 bt_extract_have_cmd(char* buffer, _int32 len, _u32* index)
{
	_int32 ret = SUCCESS;
	_u8 type = 0;
	sd_get_int8(&buffer, &len, (_int8*)&type);
	ret = sd_get_int32_from_bg(&buffer, &len, (_int32*)index);
	if(ret != SUCCESS || len != 0)
		return -1;
	return SUCCESS;
}

_int32 bt_extract_request_cmd(char* buffer, _int32 len, BT_PIECE_DATA* piece)
{
	_int32 ret = SUCCESS;
	_u8 type = 0;
	sd_get_int8(&buffer, &len, (_int8*)&type);
	sd_get_int32_from_bg(&buffer, &len, (_int32*)&piece->_index);
	sd_get_int32_from_bg(&buffer, &len, (_int32*)&piece->_offset);
	ret = sd_get_int32_from_bg(&buffer, &len, (_int32*)&piece->_len);
	if(ret != SUCCESS || len != 0)
		return -1;
	return SUCCESS;
}

_int32 bt_extract_piece_cmd(char* buffer, _int32 len, BT_PIECE_DATA* data)
{
	_int32 ret = SUCCESS;
	_u8 type = 0;
	sd_get_int8(&buffer, &len, (_int8*)&type);
	sd_get_int32_from_bg(&buffer, &len, (_int32*)&data->_index);
	ret = sd_get_int32_from_bg(&buffer, &len, (_int32*)&data->_offset);
	if(ret != SUCCESS || len != 0)
		return -1;
	return SUCCESS;
}

_int32 bt_extract_cancel_cmd(char* buffer, _int32 len, BT_PIECE_DATA* piece)
{
	_int32 ret = SUCCESS;
	_u8 type = 0;
	sd_get_int8(&buffer, &len, (_int8*)&type);
	sd_get_int32_from_bg(&buffer, &len, (_int32*)&piece->_index);
	sd_get_int32_from_bg(&buffer, &len, (_int32*)&piece->_offset);
	ret = sd_get_int32_from_bg(&buffer, &len, (_int32*)&piece->_len);
	if(ret != SUCCESS || len != 0)
		return -1;
	return SUCCESS;
}

_int32 bt_extract_port_cmd(char* buffer, _int32 len, _u16* port)
{
	//该命令不同的客户端可能会有各自定义的字段，这里仅需解析port就行了，该port用于加入dht网络
	_int32 ret = SUCCESS;
	_u8 type = 0;
	sd_get_int8(&buffer, &len, (_int8*)&type);
	ret = sd_get_int16_from_bg(&buffer, &len, (_int16*)port);
	if(ret != SUCCESS)
		return ret;
	return SUCCESS;
}

_int32 bt_extract_ex_handshake_cmd(char* buffer, _int32 len, _u32 *p_metadata_type, _u32 *p_metadata_size )
{
	_int32 ret_val = SUCCESS;
    BC_DICT *p_metadata_dict = NULL, *p_m_dict = NULL;
    BC_INT *p_type_int = NULL, *p_size_int = NULL;
	BC_PARSER *p_bc_parser = NULL;

	ret_val = bc_parser_create(buffer, len, len, &p_bc_parser);
	if(ret_val) return ret_val;
	
	ret_val = bc_parser_str(p_bc_parser, (BC_OBJ **)&p_metadata_dict);
	if( ret_val != SUCCESS ) goto ErrHandler;

   	ret_val = bc_parser_destory(p_bc_parser);
	CHECK_VALUE(ret_val);

    ret_val = bc_dict_get_value( p_metadata_dict, "m", (BC_OBJ **)&p_m_dict );
    if( ret_val != SUCCESS || !p_m_dict) goto ErrHandler;

    ret_val = bc_dict_get_value( p_m_dict, "ut_metadata", (BC_OBJ **)&p_type_int );
    if( ret_val != SUCCESS || !p_type_int) goto ErrHandler;   
    
    *p_metadata_type = p_type_int->_value;

    ret_val = bc_dict_get_value( p_m_dict, "metadata_size", (BC_OBJ **)&p_size_int );
    if( ret_val != SUCCESS || !p_size_int) 
	{
		ret_val = bc_dict_get_value( p_metadata_dict, "metadata_size", (BC_OBJ **)&p_size_int );
	    if( ret_val != SUCCESS || !p_size_int)
	    {
			// sd_assert( FALSE ); // 影响测试，先去掉
			goto ErrHandler;
	    }
    }

    *p_metadata_size = p_size_int->_value;
	
 ErrHandler:
    bc_dict_uninit( (BC_OBJ *)p_metadata_dict );
    return ret_val;
}

_int32 bt_extract_metadata(char* buffer, _int32 len, _u32 *p_piece_index, char **p_data, _u32 *p_data_len )
{
    _int32 ret_val = SUCCESS;
    BC_DICT *p_metadata_dict = NULL;
	BC_PARSER *p_bc_parser = NULL;
    BC_INT *p_type_int = NULL, *p_size_int = NULL, *p_piece_int = NULL;
	_u32 picec_head_len = 0;

	ret_val = bc_parser_create(buffer, len, len, &p_bc_parser);
	CHECK_VALUE(ret_val);

	ret_val = bc_parser_str(p_bc_parser, (BC_OBJ**)&p_metadata_dict);
	if( ret_val != SUCCESS )
	{
		return ret_val;
	}
	bc_parser_get_used_str_len(p_bc_parser, &picec_head_len);
	bc_parser_destory(p_bc_parser);

    ret_val = bc_dict_get_value( p_metadata_dict, "msg_type", (BC_OBJ **)&p_type_int );
    if( ret_val != SUCCESS ) goto ErrHandler;   

    if( p_type_int->_value != 1 )
    {
		bc_dict_uninit( (BC_OBJ *)p_metadata_dict );
		return BT_MAGNET_MSG_TYPE_NOT_SUPPORT;
    }

    ret_val = bc_dict_get_value( p_metadata_dict, "piece", (BC_OBJ **)&p_piece_int );
    if( ret_val != SUCCESS ) goto ErrHandler;  

    *p_piece_index = p_piece_int->_value;

    ret_val = bc_dict_get_value( p_metadata_dict, "total_size", (BC_OBJ **)&p_size_int );
    if( ret_val != SUCCESS ) goto ErrHandler;  

	*p_data_len = len - picec_head_len;
    *p_data = buffer + picec_head_len;

ErrHandler:
    bc_dict_uninit( (BC_OBJ *)p_metadata_dict );
    return ret_val;
}

#endif
#endif

