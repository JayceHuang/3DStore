
#include "utility/define.h"
#ifdef ENABLE_BT


#include "bencode.h"
#include "utility/string.h"
#include "utility/utility.h"
#include "utility/mempool.h"

#include "utility/logid.h"
#define LOGID LOGID_BENCODE
#include "utility/logger.h"

#include "platform/sd_fs.h"
#include "torrent_parser/torrent_parser_interface.h"


static SLAB* g_bc_int_slab = NULL;
static SLAB* g_bc_str_slab = NULL;
static SLAB* g_bc_list_slab = NULL;
static SLAB* g_bc_dict_slab = NULL;

#define MIN_BC_INT     50
#define MIN_BC_STR     50
#define MIN_BC_LIST    50
#define MIN_BC_DICT    50

_int32 init_bc_slabs(void)
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "init_bc_slabs." );
	
	ret_val = init_bc_int_slabs();
	CHECK_VALUE( ret_val );

	ret_val = init_bc_str_slabs();
	if( ret_val!=SUCCESS ) goto ErrorHanle;

	ret_val = init_bc_list_slabs();
	if( ret_val!=SUCCESS ) goto ErrorHanle;

	ret_val = init_bc_dict_slabs();
	if( ret_val!=SUCCESS ) goto ErrorHanle;

	return SUCCESS;
ErrorHanle:
	uninit_bc_slabs();
	return ret_val;
}

_int32 uninit_bc_slabs(void)
{
	LOG_DEBUG( "uninit_bc_slabs." );

	uninit_bc_dict_slabs();

	uninit_bc_list_slabs();

	uninit_bc_str_slabs();

	uninit_bc_int_slabs();
	
	return SUCCESS;
}

_int32 bc_parser_create( char *p_buffer, _u32 buffer_len, _u32 read_size, BC_PARSER **pp_bc_parser )
{
	_int32 ret_val = SUCCESS;
	BC_PARSER *p_reader = NULL;
	
	LOG_DEBUG( "tp_create_bc_parser." );

	if( buffer_len <= MIN_BC_READER_BUFFER )
	{
		return BT_SEED_READ_BUFFER_TOO_SMALL;
	}

	ret_val = sd_malloc( sizeof( BC_PARSER ), (void **)&p_reader );
	LOG_DEBUG( "sd_malloc p_reader ret_val:%d.", ret_val );
	CHECK_VALUE( ret_val );
	
	p_reader->_bc_buffer = p_buffer;
	p_reader->_max_read_len = buffer_len;
	p_reader->_read_size = read_size;
	p_reader->_used_str_len = 0;
	p_reader->_used_buffer_str_len = 0;

	p_reader->_file_id = INVALID_FILE_ID;
	p_reader->_file_size = 0;
	p_reader->_is_calc_info_hash = FALSE;
	
	p_reader->_sha1_ptr = NULL;
	p_reader->_is_start_sha1 = FALSE;
	
	p_reader->_seed_file_count = 0;
	p_reader->_is_pre_parser_seed = FALSE;
	
	p_reader->_is_parse_piece_hash = TRUE;
	p_reader->_file_dict_handler = NULL;
	p_reader->_user_ptr = NULL;
	p_reader->_torrent_parser_ptr = NULL;
	p_reader->_is_stop_fill_str = FALSE;

	LOG_DEBUG( "seed_read_max_len:%d", buffer_len );

	*pp_bc_parser = p_reader;
	
	return SUCCESS;
}

_int32 bc_parser_init_file_para( BC_PARSER *p_bc_parser, char *seed_path )
{
	_int32 ret_val = SUCCESS;
	_u64 file_size = 0;
	_u32 file_id;

	if( !sd_file_exist( seed_path) )
	{
		LOG_ERROR( "seed_file not exist. seed_path:%s", seed_path  );
		return BT_SEED_FILE_NOT_EXIST;
	}
	LOG_DEBUG( "tp_init_bc_parser.seed_path:%s", seed_path );
	ret_val = sd_open_ex( seed_path, O_FS_RDONLY, &file_id );
	LOG_DEBUG( "sd_open_ex ret_val:%d.", ret_val );	
	if(ret_val != SUCCESS) return ret_val;		 
    //CHECK_VALUE( ret_val );
	
	ret_val = sd_filesize( file_id, &file_size );
	if(ret_val != SUCCESS)
	{
		return ret_val;		 
	}
	p_bc_parser->_file_id = file_id;

	if(file_size == 0)	
	{
		return BT_SEED_PARSE_FAILED;
	}

	if( file_size > MAX_U32 ) 
	{
		return BT_SEED_TOO_BIG;
	}
	
	p_bc_parser->_file_size = file_size;

	return SUCCESS;
}

_int32 bc_parser_init_sha1_para( BC_PARSER *p_bc_parser )
{
	_int32 ret_val = SUCCESS;
	p_bc_parser->_is_calc_info_hash = TRUE;
	p_bc_parser->_is_start_sha1 = FALSE;
	sd_assert( p_bc_parser->_sha1_ptr == NULL );
	ret_val = sd_malloc( sizeof(ctx_sha1), (void **)&p_bc_parser->_sha1_ptr );
	CHECK_VALUE( ret_val );

	sha1_initialize( p_bc_parser->_sha1_ptr );
	
	return SUCCESS;
}

void bc_parser_set_pre_parser( BC_PARSER *p_bc_parser, BOOL is_only_get_file_count )
{
	p_bc_parser->_is_pre_parser_seed = is_only_get_file_count;
}

void bc_parser_piece_hash_set( BC_PARSER *p_bc_parser, BOOL is_parse_piece_hash )
{
	p_bc_parser->_is_parse_piece_hash = is_parse_piece_hash;
}

void bc_parser_set_file_dict_handler( BC_PARSER *p_bc_parser, bc_dict_handler dict_handler, struct tagTORRENT_PARSER *p_torrent_parser, void *p_user )
{
	p_bc_parser->_file_dict_handler = dict_handler;
	p_bc_parser->_torrent_parser_ptr = p_torrent_parser;
	p_bc_parser->_user_ptr = p_user;
}


_int32 bc_parser_destory( BC_PARSER *p_bc_parser )
{
	_int32 ret_val = SUCCESS;
	sd_assert( p_bc_parser != NULL );
	if( p_bc_parser == NULL ) return SUCCESS;
	
	if( p_bc_parser->_file_id != INVALID_FILE_ID )
	{
		ret_val = sd_close_ex( p_bc_parser->_file_id );
		LOG_DEBUG( "sd_close_ex ret_val:%d.", ret_val );		
	}
	if( p_bc_parser->_sha1_ptr != NULL )
	{
		sd_assert( p_bc_parser->_is_calc_info_hash );
		SAFE_DELETE( p_bc_parser->_sha1_ptr );
	}

	SAFE_DELETE( p_bc_parser );
	
	return SUCCESS;
}

void bc_parser_get_info_hash( BC_PARSER *p_bc_parser, _u8 info_hash[20] )
{
	sd_assert( p_bc_parser->_is_calc_info_hash );
	sha1_finish( p_bc_parser->_sha1_ptr, info_hash );
}

_u32 bc_parser_get_seed_file_count( BC_PARSER *p_bc_parser )
{
	LOG_DEBUG( "bc_parser_get_seed_file_count:%u.", p_bc_parser->_seed_file_count );		
	return p_bc_parser->_seed_file_count;
}


_int32 bc_parser_str( BC_PARSER *p_bc_parser, BC_OBJ **pp_bc_obj )
{
	_int32 ret_val = SUCCESS;
	BC_OBJ *p_bc_obj = NULL;
	const char *p_bc_str = NULL;
	LOG_DEBUG( "bc_parser_str. p_bc_parser:0x%x", p_bc_parser );

	ret_val = bc_parser_try_to_update_buffer( p_bc_parser, MIN_BC_READER_BUFFER );
	CHECK_VALUE( ret_val );
	
	*pp_bc_obj = NULL;
	if( p_bc_parser->_bc_buffer == NULL ) 
	{
		sd_assert( FALSE );
		return BT_SEED_PARSE_FAILED;
	}

	if( p_bc_parser->_used_buffer_str_len >= p_bc_parser->_max_read_len )
	{
		//sd_assert( FALSE );
		return BT_SEED_PARSE_FAILED;
	}
	p_bc_str = (const char *)( p_bc_parser->_bc_buffer + p_bc_parser->_used_buffer_str_len );
	
	if( p_bc_str[0] == 'i' )
	{
		ret_val = bc_int_malloc_wrap( (BC_INT **)&p_bc_obj );
		if( ret_val == SUCCESS )
		{
			bc_int_init( p_bc_obj, p_bc_parser );
		}
	}
	else if( p_bc_str[0] == 'l' )
	{
		ret_val = bc_list_malloc_wrap( (BC_LIST **)&p_bc_obj );
		if( ret_val == SUCCESS )
		{
			bc_list_init( p_bc_obj, p_bc_parser );
		}
	}
	else if( p_bc_str[0] == 'd' )
	{
		ret_val = bc_dict_malloc_wrap( (BC_DICT **)&p_bc_obj );
		if( ret_val == SUCCESS )
		{
			bc_dict_init( p_bc_obj, p_bc_parser );
		}
	}
	else if( p_bc_str[0] >= '0' && p_bc_str[0] <= '9' )
	{
		ret_val = bc_str_malloc_wrap( (BC_STR **)&p_bc_obj );
		if( ret_val == SUCCESS )
		{
			bc_str_init( p_bc_obj, p_bc_parser );
		}
	}
	else
	{
		//sd_assert( FALSE );
		return BT_SEED_PARSE_FAILED;
	}
	CHECK_VALUE( ret_val );
	
	*pp_bc_obj = p_bc_obj;
	ret_val = p_bc_obj->p_from_str_call_back( p_bc_obj );

	if(ret_val != SUCCESS)
	{
		p_bc_obj->p_uninit_call_back( p_bc_obj );	
		*pp_bc_obj = NULL;
		return ret_val;	
	}

	return SUCCESS;
}

BC_TYPE bc_get_type( BC_OBJ *p_bc_obj )
{
	return p_bc_obj->_bc_type;
}

_int32 bc_int_create_with_value( _u64 value, BC_INT **pp_bc_int )
{
	_int32 ret_val = SUCCESS;
	BC_INT *p_bc_int = NULL;
	
	LOG_DEBUG( "bc_int_create_with_value. value:%llu", value );

	*pp_bc_int = NULL;
	
	ret_val = bc_int_malloc_wrap( &p_bc_int );
	CHECK_VALUE( ret_val );

	ret_val = bc_int_init( (BC_OBJ *)p_bc_int, NULL );
	if( ret_val != SUCCESS )
	{
		bc_int_free_wrap( p_bc_int );
		return ret_val;
	}

	p_bc_int->_value = value;
	p_bc_int->_bc_str_len = sd_digit_bit_count(value) + 2;
	*pp_bc_int = p_bc_int;
	return SUCCESS;
}

_int32 bc_str_create_with_value( const char *p_str, _u32 str_len, BC_STR **pp_bc_str )
{
	_int32 ret_val = SUCCESS;
	BC_STR *p_bc_str = NULL;
	
	LOG_DEBUG( "bc_str_create_with_value. str_len:%u", str_len );

	*pp_bc_str = NULL;

	ret_val = bc_str_malloc_wrap( &p_bc_str );
	CHECK_VALUE( ret_val );

	ret_val = bc_str_init( (BC_OBJ *)p_bc_str, NULL );
	if( ret_val != SUCCESS )
	{
		bc_str_free_wrap( p_bc_str );
		return ret_val;
	}

	ret_val = sd_malloc( str_len + 1, (void **)&p_bc_str->_str_ptr );
	if( ret_val != SUCCESS )
	{
		bc_str_uninit( (BC_OBJ *)p_bc_str );
		return ret_val;
	}

	sd_memcpy( p_bc_str->_str_ptr, p_str, str_len );
	p_bc_str->_str_ptr[ str_len ] = '\0';
	p_bc_str->_str_len = str_len;
	
	p_bc_str->_bc_str_len = str_len + 1 + sd_digit_bit_count(str_len);
	*pp_bc_str = p_bc_str;

	return SUCCESS;	
}

_int32 bc_dict_create( BC_DICT **pp_bc_dict )
{
	_int32 ret_val = SUCCESS;
	BC_DICT *p_dict = NULL;
	
	LOG_DEBUG( "bc_dict_create" );

	*pp_bc_dict = NULL;
	ret_val =  bc_dict_malloc_wrap( &p_dict );
	CHECK_VALUE( ret_val );

	ret_val = bc_dict_init( (BC_OBJ *)p_dict, NULL );
	if( ret_val != SUCCESS )
	{
		bc_dict_free_wrap( p_dict );
		return ret_val;
	}
	
	*pp_bc_dict = p_dict;
	return SUCCESS;	
}

_int32 bc_dict_set_value( BC_DICT *p_bc_dict, const char *p_key, BC_OBJ *p_value )
{
	_int32 ret_val = SUCCESS;
	BC_STR *p_bc_str = NULL;
	PAIR map_node;
	
	LOG_DEBUG( "bc_dict_set_value, key:%s", p_key );
	
	ret_val =  bc_str_create_with_value( p_key, sd_strlen(p_key), &p_bc_str );
	CHECK_VALUE( ret_val );

	map_node._key = p_bc_str;
	map_node._value = p_value;

	ret_val = map_insert_node( &p_bc_dict->_map, &map_node );
	if( ret_val != SUCCESS )
	{
		bc_str_uninit( (BC_OBJ *)p_bc_str );
		return ret_val;
	}

	return SUCCESS;		
}

void bc_parser_updata_sha1( BC_PARSER *p_bc_parser, _u8 *p_data, _u32 data_len )
{
	if( p_bc_parser->_sha1_ptr != NULL )
	{
		sha1_update( p_bc_parser->_sha1_ptr, p_data, data_len );
	}
}

_int32 bc_parser_try_to_update_buffer( BC_PARSER *p_bc_parser, _u32 low_limit )
{
	_int32 ret_val = SUCCESS;
	_u32 valid_strlen = 0;
	
	if( p_bc_parser->_read_size < p_bc_parser->_used_str_len )
	{
		sd_assert( FALSE );
		return BT_SEED_PARSE_FAILED;
	}
	
	valid_strlen = p_bc_parser->_read_size - p_bc_parser->_used_str_len;
	LOG_DEBUG( "bc_try_to_update_bc_str:_read_size:%u, used_str_len:%u, valid_strlen:%u", 
		p_bc_parser->_read_size, p_bc_parser->_used_str_len, valid_strlen );
	
	if( valid_strlen < low_limit )
	{
		if( p_bc_parser != NULL && p_bc_parser->_file_id != INVALID_FILE_ID )
		{
			ret_val = bc_parser_file_handler( p_bc_parser, valid_strlen );
			CHECK_VALUE( ret_val );
		}
	}	
	
	return SUCCESS;
}


_int32 bc_parser_file_handler( BC_PARSER *p_bc_parser, _u32 valid_strlen )
{
	_int32 ret_val = SUCCESS;
	_u32 need_read_len = 0; 
	_u32 left_len = 0;
	_u32 read_len = 0;
	const char *p_save_str = NULL;

	sd_assert( p_bc_parser->_file_id != INVALID_FILE_ID );
	if( p_bc_parser->_file_size + valid_strlen < p_bc_parser->_read_size )
	{
		sd_assert( FALSE );
		return BT_SEED_READ_ERROR;
	}

	LOG_DEBUG( "tp_bc_read_handler, valid_strlen:%d.",  valid_strlen );	
	ret_val = sd_setfilepos( p_bc_parser->_file_id, p_bc_parser->_read_size );
	LOG_DEBUG( "tp_bc_read_handler, sd_setfilepos file_pos:%u, ret_val:%d.", 
		p_bc_parser->_read_size, ret_val );	
	if(ret_val != SUCCESS)
	{
		return ret_val;  
	}
	p_save_str = (const char *)( p_bc_parser->_bc_buffer + p_bc_parser->_used_buffer_str_len );

	if( valid_strlen != 0 )
	{
		sd_assert( p_bc_parser->_bc_buffer != NULL );
		ret_val = sd_memmove( p_bc_parser->_bc_buffer, p_save_str, valid_strlen );
		CHECK_VALUE( ret_val );	
	}

	left_len = p_bc_parser->_file_size + valid_strlen - p_bc_parser->_read_size;
	sd_assert( valid_strlen <= p_bc_parser->_max_read_len );
	if( valid_strlen >= p_bc_parser->_max_read_len )
	{
		return BT_SEED_READ_ERROR;
	}
	need_read_len = MIN( p_bc_parser->_max_read_len - valid_strlen, left_len );

	sd_assert( p_bc_parser->_file_id != INVALID_FILE_ID );
	if( need_read_len != 0 )
	{
		ret_val = sd_read( p_bc_parser->_file_id, p_bc_parser->_bc_buffer + valid_strlen, need_read_len, &read_len );
		LOG_DEBUG( "tp_bc_read_handler, sd_read ret_val:%d.read_len:%u, last_read_size:%d", 
			ret_val, read_len, p_bc_parser->_read_size );		
		if( ret_val != SUCCESS) 
		{
			sd_assert( FALSE );
			return BT_SEED_READ_ERROR;
		}		
	}

	p_bc_parser->_read_size += read_len;
	p_bc_parser->_used_buffer_str_len = 0;
	sd_assert( p_bc_parser->_used_buffer_str_len < p_bc_parser->_max_read_len );
	return SUCCESS;
}



_int32 bc_int_init( BC_OBJ *p_bc_obj, BC_PARSER *p_bc_parser )
{
	BC_INT *p_bc_int = (BC_INT *)p_bc_obj;
	LOG_DEBUG( "bc_int_init." );
	
	p_bc_int->_bc_obj._bc_type = INT_TYPE;
	p_bc_int->_bc_obj.p_from_str_call_back = bc_int_from_str;
	p_bc_int->_bc_obj.p_to_str_call_back = bc_int_to_str;	
	p_bc_int->_bc_obj.p_uninit_call_back = bc_int_uninit;
	p_bc_int->_value = 0;
	p_bc_int->_bc_str_len = 0;	
	p_bc_int->_bc_obj._bc_parser_ptr = p_bc_parser;
	return SUCCESS;
}

_int32 bc_int_uninit( BC_OBJ *p_bc_obj )
{
	BC_INT *p_bc_int = (BC_INT *)p_bc_obj;
	LOG_DEBUG( "bc_int_uninit." );
	
	p_bc_obj->_bc_parser_ptr = NULL;
	bc_int_free_wrap( p_bc_int );
	return SUCCESS;
}

_int32 bc_int_from_str( BC_OBJ *p_bc_obj )
{
	_u32 index = 0;
	_int32 ret_val = SUCCESS;
	BC_INT *p_bc_int = (BC_INT *)p_bc_obj;
	BC_PARSER *p_bc_parser = p_bc_obj->_bc_parser_ptr;
	_u32 valid_strlen = 0;
	const char *p_bc_str = NULL;
	
	LOG_DEBUG( "bc_int_from_str. p_bc_obj:0x%x", p_bc_obj );

	if( p_bc_obj->_bc_type != INT_TYPE )
	{
		sd_assert( FALSE );
		return BT_SEED_PARSE_FAILED;
	}

	if( p_bc_parser->_used_buffer_str_len + 2 > p_bc_parser->_max_read_len )
	{
		sd_assert( FALSE );
		return BT_SEED_PARSE_FAILED;
	}	
	
	if( p_bc_parser->_bc_buffer[ p_bc_parser->_used_buffer_str_len ] != 'i' )	
	{
		sd_assert( FALSE );
		return BT_SEED_PARSE_FAILED;
	}
	
	if( p_bc_parser->_read_size <= p_bc_parser->_used_str_len )
	{
		sd_assert( FALSE );
		return BT_SEED_PARSE_FAILED;
	}	
	valid_strlen = p_bc_parser->_read_size - p_bc_parser->_used_str_len;
	if( valid_strlen > p_bc_parser->_max_read_len )
	{
		sd_assert( FALSE );
		return BT_SEED_PARSE_FAILED;
	}	

	p_bc_str = (const char *)( p_bc_parser->_bc_buffer + p_bc_parser->_used_buffer_str_len );
	for( index = 1; p_bc_str[index] != 'e' && index < valid_strlen; index++ );

	if( p_bc_str[index] != 'e' )
	{
		return BT_SEED_PARSE_FAILED;
	}
	
	p_bc_int->_bc_str_len = index + 1;
	ret_val = sd_str_to_u64( (char *)p_bc_str + 1, p_bc_int->_bc_str_len - 2, &p_bc_int->_value );

	p_bc_parser->_used_str_len += p_bc_int->_bc_str_len;
	p_bc_parser->_used_buffer_str_len += p_bc_int->_bc_str_len;

	if( p_bc_parser->_is_start_sha1 )
	{
		bc_parser_updata_sha1( p_bc_parser, (_u8 *)p_bc_str, p_bc_int->_bc_str_len );
	}
	
	LOG_DEBUG( "bc_int_from_str.int value:%llu, len:%u", p_bc_int->_value, p_bc_int->_bc_str_len );
	return SUCCESS;
}

_int32 bc_int_to_str( BC_OBJ *p_bc_obj, char *p_bc_str, _u32 str_len, _u32 *p_used_len )
{
	_int32 ret_val = SUCCESS;
	BC_INT *p_bc_int = (BC_INT *)p_bc_obj;
	sd_assert( p_bc_int->_bc_str_len <= str_len );
	p_bc_str[0] = 'i';
	ret_val = sd_u64_to_str( p_bc_int->_value, p_bc_str + 1, p_bc_int->_bc_str_len - 1 );

	sd_assert( ret_val == p_bc_int->_bc_str_len - 2 );

	p_bc_str[p_bc_int->_bc_str_len - 1 ] = 'e';
	*p_used_len = p_bc_int->_bc_str_len;
	LOG_DEBUG( "bc_int_to_str. str_len:%u, str_value:%llu", p_bc_int->_bc_str_len, p_bc_int->_value );
	return SUCCESS;
}

_int32 bc_str_init( BC_OBJ *p_bc_obj, BC_PARSER *p_bc_parser )
{
	BC_STR *p_bc_str = (BC_STR *)p_bc_obj;
	LOG_DEBUG( "bc_str_init.");
	
	p_bc_str->_bc_obj._bc_type = STR_TYPE;
	p_bc_str->_bc_obj.p_from_str_call_back = bc_str_from_str;
	p_bc_str->_bc_obj.p_to_str_call_back = bc_str_to_str;
	p_bc_str->_bc_obj.p_uninit_call_back = bc_str_uninit;
	p_bc_str->_str_ptr = NULL;
	p_bc_str->_bc_str_len = 0;
	p_bc_str->_str_len = 0;
	p_bc_str->_bc_obj._bc_parser_ptr = p_bc_parser;
	return SUCCESS;
}

_int32 bc_str_uninit( BC_OBJ *p_bc_obj )
{
	BC_STR *p_bc_str = (BC_STR *)p_bc_obj;
	LOG_DEBUG( "bc_str_uninit:str:%s", p_bc_str->_str_ptr );
	SAFE_DELETE( p_bc_str->_str_ptr );
	
	p_bc_obj->_bc_parser_ptr = NULL;
	bc_str_free_wrap( p_bc_str );
	return SUCCESS;
}

_int32 bc_str_from_str( BC_OBJ *p_bc_obj )
{
	_u64 seed_str_len = 0;
	_u64 left_str_len = 0;
	_int32 ret_val = SUCCESS;
	_u32 index = 0;
	BC_STR *p_str = (BC_STR *)p_bc_obj;
	BC_PARSER *p_bc_parser = p_bc_obj->_bc_parser_ptr;
	_u32 read_pos = 0;
	const char *p_bc_str = NULL;
	_u32 valid_strlen = 0;
	_u32 used_strlen = 0;
	_u32 copy_len = 0;

	LOG_DEBUG( "bc_str_from_str. p_bc_obj:0x%x", p_bc_obj );

	if( p_bc_parser->_read_size < p_bc_parser->_used_str_len )
	{
		sd_assert( FALSE );
		return BT_SEED_PARSE_FAILED;
	}	
	valid_strlen = p_bc_parser->_read_size - p_bc_parser->_used_str_len;
	if( valid_strlen > p_bc_parser->_max_read_len )
	{
		sd_assert( FALSE );
		return BT_SEED_PARSE_FAILED;
	}	

	if( p_bc_obj->_bc_type != STR_TYPE )
	{
		sd_assert( FALSE );
		return BT_SEED_PARSE_FAILED;
	}

	p_bc_str = (const char *)( p_bc_parser->_bc_buffer + p_bc_parser->_used_buffer_str_len );
	
	for( index = 0; index < valid_strlen && p_bc_str[index] != ':'; index++ );

	if( p_bc_str[index] != ':' )
	{ 
		return BT_SEED_PARSE_FAILED;
	}

	ret_val = sd_str_to_u64( p_bc_str, index, &seed_str_len );
	if( ret_val != SUCCESS ) return ret_val;
	//CHECK_VALUE( ret_val );

	LOG_DEBUG( "bc_str_from_str.str_ptr:0x%x, index:%u, seed_str_len:%llu", 
		p_str, index, seed_str_len );
	if( seed_str_len > MAX_U32 )
	{
		return BT_SEED_PARSE_FAILED;
	}
	if( !p_bc_parser->_is_stop_fill_str )
	{
		ret_val = sd_malloc( (_u32)seed_str_len + 1, (void **)&p_str->_str_ptr );
		if(ret_val != SUCCESS)
		{
		    p_str->_str_ptr = NULL;
			return ret_val;   
		}		
	}
	else
	{
		p_str->_str_ptr = NULL;
	}

	p_bc_parser->_used_str_len += index + 1;
	p_bc_parser->_used_buffer_str_len += index + 1;
	
	if( p_bc_parser->_is_start_sha1 )
	{
		bc_parser_updata_sha1( p_bc_parser, (_u8 *)p_bc_str, index + 1 );
	}
	
	p_bc_str = (const char *)( p_bc_parser->_bc_buffer + p_bc_parser->_used_buffer_str_len );
	if( p_bc_parser->_read_size < p_bc_parser->_used_str_len )
	{
		sd_assert( FALSE );
		return BT_SEED_PARSE_FAILED;
	}	
	valid_strlen = p_bc_parser->_read_size - p_bc_parser->_used_str_len;
	if( valid_strlen > p_bc_parser->_max_read_len )
	{
		sd_assert( FALSE );
		return BT_SEED_PARSE_FAILED;
	}	
	read_pos = 0;
	left_str_len = seed_str_len;
	copy_len = MIN( valid_strlen, left_str_len );
	
	while( left_str_len > 0 )
	{
		if( p_bc_parser->_used_str_len >= p_bc_parser->_read_size ) return BT_SEED_PARSE_FAILED;

		if( !p_bc_parser->_is_stop_fill_str )
		{
			ret_val = sd_memcpy( p_str->_str_ptr + read_pos, p_bc_str, copy_len );
			CHECK_VALUE( ret_val );
		}	
		
		if( left_str_len < copy_len )
		{
			sd_assert( FALSE );
			return BT_SEED_PARSE_FAILED;
		}

		left_str_len -= copy_len;
		read_pos += copy_len;
		
		p_bc_parser->_used_str_len += copy_len;
		p_bc_parser->_used_buffer_str_len += copy_len;

		if( p_bc_parser->_is_start_sha1 )
		{
			bc_parser_updata_sha1( p_bc_parser, (_u8 *)p_bc_str, copy_len );
		}

		ret_val = bc_parser_try_to_update_buffer( p_bc_parser, p_bc_parser->_max_read_len );
		CHECK_VALUE( ret_val );

		p_bc_str = (const char *)( p_bc_parser->_bc_buffer + p_bc_parser->_used_buffer_str_len );
		if( p_bc_parser->_read_size < p_bc_parser->_used_str_len )
		{
			sd_assert( FALSE );
			return BT_SEED_PARSE_FAILED;
		}	
		valid_strlen = p_bc_parser->_read_size - p_bc_parser->_used_str_len;
		if( valid_strlen > p_bc_parser->_max_read_len )
		{
			sd_assert( FALSE );
			return BT_SEED_PARSE_FAILED;
		}	
		copy_len = MIN( valid_strlen, left_str_len );

	}
	if( !p_bc_parser->_is_stop_fill_str )
	{
		p_str->_str_ptr[ seed_str_len ] = '\0';
	}
	used_strlen = index + (_u32)seed_str_len + 1;
	p_str->_bc_str_len = used_strlen;
	p_str->_str_len = (_u32)seed_str_len;	

	LOG_DEBUG( "bc_str_from_str:%s. used_len:%u", p_str->_str_ptr, used_strlen );
	return SUCCESS;
}

_int32 bc_str_to_str( BC_OBJ *p_bc_obj, char *p_bc_str, _u32 str_len, _u32 *p_used_len )
{
	_int32 ret_val = SUCCESS;
	BC_STR *p_str = (BC_STR *)p_bc_obj;
	_u32 info_len = p_str->_str_len;

	LOG_DEBUG( "bc_str_to_str.str_ptr:0x%x.str:%s, bc_str_len:%u, str_len:%u.",
		p_str, p_str->_str_ptr, p_str->_bc_str_len, p_str->_str_len );	
	sd_assert( p_str->_bc_str_len <= str_len );
	ret_val = sd_u32_to_str( info_len, p_bc_str, p_str->_bc_str_len - p_str->_str_len );
	sd_assert( ret_val == p_str->_bc_str_len - p_str->_str_len - 1 );

	p_bc_str[ p_str->_bc_str_len - info_len - 1 ] = ':';
	ret_val = sd_memcpy( p_bc_str + p_str->_bc_str_len - info_len, p_str->_str_ptr, info_len );
	*p_used_len = p_str->_bc_str_len;

	return SUCCESS;
}

_int32 bc_list_init( BC_OBJ *p_bc_obj, BC_PARSER *p_bc_parser )
{
	BC_LIST *p_bc_list = (BC_LIST *)p_bc_obj;
	LOG_DEBUG( "bc_list_init." );
	
	p_bc_list->_bc_obj._bc_type = LIST_TYPE;
	p_bc_list->_bc_obj.p_from_str_call_back = bc_list_from_str;
	p_bc_list->_bc_obj.p_to_str_call_back = bc_list_to_str;
	p_bc_list->_bc_obj.p_uninit_call_back = bc_list_uninit;	
	p_bc_list->_bc_obj._bc_parser_ptr = p_bc_parser;
	
	list_init( &p_bc_list->_list );
	return SUCCESS;
}

_int32 bc_list_uninit( BC_OBJ *p_bc_obj )
{
	
	BC_LIST *p_bc_list = (BC_LIST *)p_bc_obj;
	LIST_ITERATOR cur_node = LIST_BEGIN( p_bc_list->_list );
	LIST_ITERATOR next_node;

	LOG_DEBUG( "bc_list_uninit." );
	
	while( cur_node != LIST_END( p_bc_list->_list ) )
	{
		BC_OBJ *p_value = LIST_VALUE( cur_node );
		p_value->p_uninit_call_back( p_value );
		next_node = LIST_NEXT( cur_node );
		list_erase( &p_bc_list->_list, cur_node );
		cur_node = next_node;
	}
	
	p_bc_obj->_bc_parser_ptr = NULL;
	bc_list_free_wrap( p_bc_list );
	return SUCCESS;
}

_int32 bc_list_from_str( BC_OBJ *p_bc_obj )
{
	_int32 ret_val = SUCCESS;
	BC_LIST *p_bc_list = (BC_LIST *)p_bc_obj;
	
	BC_DICT *p_bc_dict = NULL;
	BC_PARSER *p_bc_parser = p_bc_obj->_bc_parser_ptr;
	BC_OBJ *p_bc_item = NULL;

	if( p_bc_obj->_bc_type != LIST_TYPE )
	{
		sd_assert( FALSE );
		return BT_SEED_PARSE_FAILED;
	}

	if( p_bc_parser->_used_buffer_str_len + 2 > p_bc_parser->_max_read_len )
	{
		sd_assert( FALSE );
		return BT_SEED_PARSE_FAILED;
	}	

	if( p_bc_parser->_bc_buffer[ p_bc_parser->_used_buffer_str_len ] != 'l' )	
	{
		sd_assert( FALSE );
		return BT_SEED_PARSE_FAILED;
	}
	if( p_bc_parser->_is_start_sha1 )
	{
		bc_parser_updata_sha1( p_bc_parser, (_u8 *)( p_bc_parser->_bc_buffer + p_bc_parser->_used_buffer_str_len ), 1 );
	}

	p_bc_parser->_used_str_len += 1;
	p_bc_parser->_used_buffer_str_len += 1;
	

	LOG_DEBUG( "bc_list_from_str." );
	
	while( p_bc_parser->_bc_buffer[p_bc_parser->_used_buffer_str_len] != 'e' )
	{
		ret_val = bc_parser_str( p_bc_parser, &p_bc_item );
		if(ret_val != SUCCESS)
		{
		    return ret_val;   
		}

		if( p_bc_parser->_used_buffer_str_len >= p_bc_parser->_max_read_len )
		{
			sd_assert( FALSE );
			return BT_SEED_PARSE_FAILED;
		}	

		if( p_bc_item->_bc_type == DICT_TYPE  )
		{
			sd_assert( p_bc_item != NULL );
			p_bc_dict = (BC_DICT *)p_bc_item;
			if( ( p_bc_parser->_file_dict_handler != NULL 
				|| p_bc_parser->_is_pre_parser_seed )
				&& p_bc_dict->_is_file_dict )
			{
				if( p_bc_parser->_file_dict_handler != NULL )
				{
					ret_val = p_bc_parser->_file_dict_handler( p_bc_parser->_torrent_parser_ptr, p_bc_parser->_user_ptr, p_bc_dict );
					//sd_assert( ret_val == SUCCESS );	
					if( ret_val != SUCCESS ) 
					{
						p_bc_item->p_uninit_call_back( p_bc_item ); 
						return ret_val;
					}
					//if( ret_val != SUCCESS ) return ret_val;
					//CHECK_VALUE( ret_val );
				}
				p_bc_item->p_uninit_call_back( p_bc_item ); 
				continue;
			}
		}

		ret_val = list_push( &p_bc_list->_list, (void *)p_bc_item );
		if( ret_val != SUCCESS )
		{
			if( p_bc_item != NULL )
			{
				p_bc_item->p_uninit_call_back( p_bc_item );	
			}
			return ret_val;
		}			

	}


	if( p_bc_parser->_used_buffer_str_len + 1 > p_bc_parser->_max_read_len )
	{
		sd_assert( FALSE );
		return BT_SEED_PARSE_FAILED;
	}	
	
	if( p_bc_parser->_bc_buffer[p_bc_parser->_used_buffer_str_len ] != 'e' )	
	{
		return BT_SEED_PARSE_FAILED;
	}
	if( p_bc_parser->_is_start_sha1 )
	{
		bc_parser_updata_sha1( p_bc_parser, (_u8 *)( p_bc_parser->_bc_buffer + p_bc_parser->_used_buffer_str_len ), 1 );
	}	

	p_bc_parser->_used_str_len += 1;
	p_bc_parser->_used_buffer_str_len += 1;
	
	return SUCCESS;
}

_int32 bc_list_to_str( BC_OBJ *p_bc_obj, char *p_bc_str, _u32 str_len, _u32 *p_used_len )
{
	_int32 ret_val = SUCCESS;
	_u32 used_len = 1;

       BC_LIST *p_list = (BC_LIST *)p_bc_obj;

       LIST_ITERATOR cur_node = NULL;

	sd_assert( str_len > 2);
	p_bc_str[0] = 'l';
	*p_used_len = 1;
	
	

	LOG_DEBUG( "bc_list_to_str." );
	cur_node = LIST_BEGIN( p_list->_list );
	while( cur_node != LIST_END( p_list->_list ) )
	{
		BC_OBJ *p_bc_obj_node = (BC_OBJ *)LIST_VALUE( cur_node );
		ret_val = p_bc_obj_node->p_to_str_call_back( p_bc_obj_node, p_bc_str + *p_used_len, str_len - *p_used_len, &used_len );
		CHECK_VALUE( ret_val );
		*p_used_len += used_len;
		cur_node = LIST_NEXT( cur_node );
	}
	p_bc_str[*p_used_len] = 'e';
	*p_used_len += 1;
	return SUCCESS;
}

_int32 bc_dict_init( BC_OBJ *p_bc_obj, BC_PARSER *p_bc_parser  )
{
	BC_DICT *p_bc_dict = (BC_DICT *)p_bc_obj;
	LOG_DEBUG( "bc_dict_init." );
	
	p_bc_dict->_bc_obj._bc_type = DICT_TYPE;
	p_bc_dict->_bc_obj.p_from_str_call_back = bc_dict_from_str;
	p_bc_dict->_bc_obj.p_to_str_call_back = bc_dict_to_str;	
	p_bc_dict->_bc_obj.p_uninit_call_back = bc_dict_uninit;		
	p_bc_dict->_bc_obj._bc_parser_ptr = p_bc_parser;		
	p_bc_dict->_is_file_dict = FALSE;		
	
	map_init( &p_bc_dict->_map, bc_map_compare_fun );
	return SUCCESS;
}

_int32 bc_dict_uninit( BC_OBJ *p_bc_obj )
{
	BC_DICT *p_bc_dict = (BC_DICT *)p_bc_obj;
	MAP_ITERATOR cur_node = MAP_BEGIN( p_bc_dict->_map );
	MAP_ITERATOR next_node;

	LOG_DEBUG( "bc_dict_uninit." );

	while( cur_node != MAP_END( p_bc_dict->_map ) )
	{
		BC_OBJ *p_key = (BC_OBJ *)MAP_KEY( cur_node );
		BC_OBJ *p_value = (BC_OBJ *)MAP_VALUE( cur_node );
		next_node = MAP_NEXT( p_bc_dict->_map, cur_node );
		map_erase_iterator( &p_bc_dict->_map, cur_node );
		p_key->p_uninit_call_back( p_key );
		p_value->p_uninit_call_back( p_value );	
		cur_node = next_node;
	}
	
	p_bc_obj->_bc_parser_ptr = NULL;
	bc_dict_free_wrap( p_bc_dict );
	return SUCCESS;
}

_int32 bc_dict_from_str( BC_OBJ *p_bc_obj )
{
	BC_DICT *p_bc_dict = (BC_DICT *)p_bc_obj;
	_int32 ret_val = SUCCESS;
	BC_PARSER *p_bc_parser = p_bc_obj->_bc_parser_ptr;
	PAIR map_node;
	char *p_info_key = "info";
	char *p_length_key = "length";	
	
	char *p_pieces_key = "pieces";	
	BC_OBJ *p_bc_str_obj = NULL;
	BC_STR *p_bc_key = NULL;
	BC_OBJ *p_bc_value_obj = NULL;
	MAP_ITERATOR cur_node = NULL;

	if( p_bc_obj->_bc_type != DICT_TYPE )
	{
		sd_assert( FALSE );
		return BT_SEED_PARSE_FAILED;
	}

	if( p_bc_parser->_used_buffer_str_len + 2 > p_bc_parser->_max_read_len )
	{
		sd_assert( FALSE );
		return BT_SEED_PARSE_FAILED;
	}	
	
	if( p_bc_parser->_bc_buffer[ p_bc_parser->_used_buffer_str_len ] != 'd' )	
	{
		sd_assert( FALSE );
		return BT_SEED_PARSE_FAILED;
	}
	if( p_bc_parser->_is_start_sha1 )
	{
		bc_parser_updata_sha1( p_bc_parser, (_u8 *)( p_bc_parser->_bc_buffer + p_bc_parser->_used_buffer_str_len ), 1 );
	}	

	p_bc_parser->_used_str_len += 1;
	p_bc_parser->_used_buffer_str_len += 1;
	
	LOG_DEBUG( "bc_dict_from_str." );
	while( p_bc_parser->_bc_buffer[ p_bc_parser->_used_buffer_str_len ] != 'e' )
	{
		ret_val = bc_parser_str( p_bc_parser, &p_bc_str_obj );
		if(ret_val != SUCCESS)
		{
			return ret_val;		  
		}
		
		sd_assert( p_bc_str_obj->_bc_type == STR_TYPE );
		if( p_bc_str_obj->_bc_type != STR_TYPE )
		{
			bc_str_uninit(p_bc_str_obj);
			return ret_val;			
		}	

		p_bc_key = (BC_STR *)p_bc_str_obj;

		if( sd_strncmp( p_info_key, p_bc_key->_str_ptr, p_bc_key->_str_len ) == 0 )
		{
			p_bc_parser->_is_start_sha1 = TRUE;
		}
		if( sd_strncmp( p_pieces_key, p_bc_key->_str_ptr, p_bc_key->_str_len ) == 0
			&& !p_bc_parser->_is_parse_piece_hash )
		{
			p_bc_parser->_is_stop_fill_str = TRUE;
		}	

		ret_val = bc_parser_str( p_bc_parser, &p_bc_value_obj );
		if(ret_val != SUCCESS)
		{
		    bc_str_uninit(p_bc_str_obj);
			return ret_val;
		}
		
		if( sd_strncmp( p_pieces_key, p_bc_key->_str_ptr, p_bc_key->_str_len ) == 0
			&& !p_bc_parser->_is_parse_piece_hash )
		{
			p_bc_parser->_is_stop_fill_str = FALSE;
		}	
		
		LOG_DEBUG( "bc_dict_from_str: dict_ptr:0x%x.key:%s,value:0x%x", 
			p_bc_dict, ((BC_STR *) p_bc_str_obj)->_str_ptr, p_bc_value_obj );

		if( sd_strncmp( p_info_key, p_bc_key->_str_ptr, p_bc_key->_str_len ) == 0 )
		{
			p_bc_parser->_is_start_sha1 = FALSE;
		}
		
		if( sd_strncmp( p_length_key, p_bc_key->_str_ptr, p_bc_key->_str_len ) == 0 )
		{
			p_bc_parser->_seed_file_count++;
			p_bc_dict->_is_file_dict = TRUE;		
		}		

		if( p_bc_parser->_used_buffer_str_len >= p_bc_parser->_max_read_len )
		{
			//sd_assert( FALSE );
		    bc_str_uninit(p_bc_str_obj);
			p_bc_value_obj->p_uninit_call_back( p_bc_value_obj );	
			return BT_SEED_PARSE_FAILED;
		}	

		map_node._key = p_bc_str_obj;
		map_node._value = p_bc_value_obj;
		
		cur_node = NULL;
		map_find_iterator( &p_bc_dict->_map, p_bc_str_obj, &cur_node );
		if( cur_node != MAP_END(p_bc_dict->_map) )
		{
		    bc_str_uninit(p_bc_str_obj);
			if(p_bc_value_obj != NULL)
			{
				p_bc_value_obj->p_uninit_call_back( p_bc_value_obj );	
			}
			continue;
		}
		ret_val = map_insert_node( &p_bc_dict->_map, &map_node );
		if(ret_val != SUCCESS)
		{
		    bc_str_uninit(p_bc_str_obj);
			if(p_bc_value_obj != NULL)
			{
				p_bc_value_obj->p_uninit_call_back( p_bc_value_obj );	
			}
			return ret_val;		
		}	

	}

	if( p_bc_parser->_used_buffer_str_len + 1 > p_bc_parser->_max_read_len )
	{
		sd_assert( FALSE );
		return BT_SEED_PARSE_FAILED;
	}	
	
	if( p_bc_parser->_bc_buffer[p_bc_parser->_used_buffer_str_len ] != 'e' )	
	{
		return BT_SEED_PARSE_FAILED;
	}
	if( p_bc_parser->_is_start_sha1 )
	{
		bc_parser_updata_sha1( p_bc_parser, (_u8 *)( p_bc_parser->_bc_buffer + p_bc_parser->_used_buffer_str_len ), 1 );
	}		
	p_bc_parser->_used_str_len += 1;
	p_bc_parser->_used_buffer_str_len += 1;	
	

	return SUCCESS;
}

_int32 bc_dict_to_str( BC_OBJ *p_bc_obj, char *p_bc_str, _u32 str_len, _u32 *p_used_len )
{
	_int32 ret_val = SUCCESS;
	_u32 used_len = 1;

	BC_DICT *p_dict = (BC_DICT *)p_bc_obj;
    MAP_ITERATOR cur_node = NULL;
	BC_OBJ *p_key = NULL;
	BC_OBJ *p_bc_obj_node = NULL;
	
	sd_assert( str_len > 2);
	p_bc_str[0] = 'd';
	*p_used_len = 1;
	
	LOG_DEBUG( "bc_dict_to_str." );
	cur_node = MAP_BEGIN( p_dict->_map );
	while( cur_node != MAP_END( p_dict->_map ) )
	{
		p_key = NULL;
		p_bc_obj_node = NULL;
		
		p_key = (BC_OBJ *)MAP_KEY( cur_node );
		LOG_DEBUG( "bc_dict_to_str: dict_ptr:0x%x.key:%s", 
			p_dict, ((BC_STR *) p_key)->_str_ptr );
	
		ret_val = p_key->p_to_str_call_back( p_key, p_bc_str + *p_used_len, str_len - *p_used_len, &used_len );
		CHECK_VALUE( ret_val );
		*p_used_len += used_len;

		p_bc_obj_node = (BC_OBJ *)MAP_VALUE( cur_node );
		ret_val = p_bc_obj_node->p_to_str_call_back( p_bc_obj_node, p_bc_str + *p_used_len, str_len - *p_used_len, &used_len );
		CHECK_VALUE( ret_val );
		*p_used_len += used_len;

		LOG_DEBUG( "bc_dict_to_str: dict_ptr:0x%x, key:%s, value:0x%x", 
			p_dict, ((BC_STR *) p_key)->_str_ptr, p_bc_obj_node );
		
		cur_node = MAP_NEXT( p_dict->_map, cur_node );
	}
	
	p_bc_str[*p_used_len] = 'e';
	*p_used_len += 1;
	return SUCCESS;
}

_int32 bc_dict_get_value( BC_DICT *p_bc_obj, const char *p_key, BC_OBJ **pp_value )
{
	BC_STR *p_bc_str = NULL;
	_int32 ret_val = SUCCESS;
    
    if( p_bc_obj == NULL ) return INVALID_ARGUMENT;
    
	ret_val = bc_str_malloc_wrap( &p_bc_str );
	CHECK_VALUE( ret_val );
	bc_str_init( (BC_OBJ *)p_bc_str, p_bc_obj->_bc_obj._bc_parser_ptr );
	p_bc_str->_str_ptr = (char *)p_key;
	ret_val = map_find_node( &p_bc_obj->_map, p_bc_str, (void **)pp_value );
	//CHECK_VALUE( ret_val );
	if(ret_val != SUCCESS)
	{
	      bc_str_free_wrap( p_bc_str );
	      return ret_val;	  
	}
	LOG_DEBUG( "bc_dict_get_value.key:%s, value:0x%x.", p_key, *pp_value );

	ret_val = bc_str_free_wrap( p_bc_str );
	CHECK_VALUE( ret_val );
	return SUCCESS;
}

_int32 bc_map_compare_fun( void *E1, void *E2 )
{
	BC_STR *p_bc_str1 = (BC_STR *)E1;
	BC_STR *p_bc_str2 = (BC_STR *)E2;
	LOG_DEBUG( "bc_map_compare_fun." );
	return sd_strcmp( p_bc_str1->_str_ptr, p_bc_str2->_str_ptr );
}


//int malloc/free
_int32 init_bc_int_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_bc_int_slab == NULL )
	{              
		ret_val = mpool_create_slab( sizeof( BC_INT ), MIN_BC_INT, 0, &g_bc_int_slab );
		CHECK_VALUE( ret_val );
	}
	return ret_val;
}

_int32 uninit_bc_int_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_bc_int_slab != NULL )
	{              
		ret_val = mpool_destory_slab( g_bc_int_slab );
		CHECK_VALUE( ret_val );
		g_bc_int_slab = NULL;
	}
	return ret_val;
}

_int32 bc_int_malloc_wrap(BC_INT **pp_node)
{
	return mpool_get_slip( g_bc_int_slab, (void**)pp_node );
}

_int32 bc_int_free_wrap(BC_INT *p_node)
{
	sd_assert( p_node != NULL );
	if( p_node == NULL ) return SUCCESS;
	return mpool_free_slip( g_bc_int_slab, (void*)p_node );
}

//str malloc/free
_int32 init_bc_str_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_bc_str_slab == NULL )
	{              
		ret_val = mpool_create_slab( sizeof( BC_STR ), MIN_BC_STR, 0, &g_bc_str_slab );
		CHECK_VALUE( ret_val );
	}
	return ret_val;
}

_int32 uninit_bc_str_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_bc_str_slab != NULL )
	{              
		ret_val = mpool_destory_slab( g_bc_str_slab );
		CHECK_VALUE( ret_val );
		g_bc_str_slab = NULL;
	}
	return ret_val;
}

_int32 bc_str_malloc_wrap(BC_STR **pp_node)
{
	return mpool_get_slip( g_bc_str_slab, (void**)pp_node );
}

_int32 bc_str_free_wrap(BC_STR *p_node)
{
	sd_assert( p_node != NULL );
	if( p_node == NULL ) return SUCCESS;
	return mpool_free_slip( g_bc_str_slab, (void*)p_node );
}


//list malloc/free
_int32 init_bc_list_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_bc_list_slab == NULL )
	{              
		ret_val = mpool_create_slab( sizeof( BC_LIST ), MIN_BC_LIST, 0, &g_bc_list_slab );
		CHECK_VALUE( ret_val );
	}
	return ret_val;
}

_int32 uninit_bc_list_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_bc_list_slab != NULL )
	{              
		ret_val = mpool_destory_slab( g_bc_list_slab );
		CHECK_VALUE( ret_val );
		g_bc_list_slab = NULL;
	}
	return ret_val;
}

_int32 bc_list_malloc_wrap(BC_LIST **pp_node)
{
	return mpool_get_slip( g_bc_list_slab, (void**)pp_node );
}

_int32 bc_list_free_wrap(BC_LIST *p_node)
{
	sd_assert( p_node != NULL );
	if( p_node == NULL ) return SUCCESS;
	return mpool_free_slip( g_bc_list_slab, (void*)p_node );
}


//dict malloc/free
_int32 init_bc_dict_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_bc_dict_slab == NULL )
	{              
		ret_val = mpool_create_slab( sizeof( BC_DICT ), MIN_BC_DICT, 0, &g_bc_dict_slab );
		CHECK_VALUE( ret_val );
	}
	return ret_val;
}

_int32 uninit_bc_dict_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_bc_dict_slab != NULL )
	{              
		ret_val = mpool_destory_slab( g_bc_dict_slab );
		CHECK_VALUE( ret_val );
		g_bc_dict_slab = NULL;
	}
	return ret_val;
}

_int32 bc_dict_malloc_wrap(BC_DICT **pp_node)
{
	return mpool_get_slip( g_bc_dict_slab, (void**)pp_node );
}

_int32 bc_dict_free_wrap(BC_DICT *p_node)
{
	sd_assert( p_node != NULL );
	if( p_node == NULL ) return SUCCESS;
	return mpool_free_slip( g_bc_dict_slab, (void*)p_node );
}

_int32 bc_parser_get_used_str_len(BC_PARSER *p_bc_parser, _u32 *p_len)
{
	if(p_len && p_bc_parser)
	{
		*p_len = p_bc_parser->_used_str_len;
	}

	return SUCCESS;
}

#endif /* #ifdef ENABLE_BT */

