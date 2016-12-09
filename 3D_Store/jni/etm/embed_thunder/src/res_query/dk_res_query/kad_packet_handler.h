/*****************************************************************************
 *
 * Filename: 			kad_packet_handler.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2009/10/15
 *	
 * Purpose:				handle the kad_recv_packet.
 *
 *****************************************************************************/
	 
#if !defined(__kad_packet_handler_20091015)
#define __kad_packet_handler_20091015

#ifdef __cplusplus
extern "C" 
{
#endif


#include "utility/define.h"
#ifdef _DK_QUERY
#ifdef ENABLE_EMULE

#include "res_query/dk_res_query/k_node.h"

struct tagEMULE_TAG_LIST;

_int32 kad_fill_query_ping_buffer( char **pp_buffer, _u32 *p_data_len, _u8 version, BOOL is_query_boot );
_int32 kad_fill_query_find_node_buffer( char **pp_buffer, _u32 *p_data_len, 
    _u8 version, _u8 find_type, _u8 *p_key, _u32 key_len, _u8 *p_id );
_int32 kad_fill_query_find_source_buffer( char **pp_buffer, _u32 *p_data_len, 
    _u8 version, _u64 file_size, _u8 *p_key, _u32 key_len );
_int32 kad_fill_query_announce_souce_buffer( char **pp_buffer, _u32 *p_data_len, 
    _u8 version, _u8 *p_key, _u32 key_len, struct tagEMULE_TAG_LIST *p_tag_list );

_int32 kad_fill_query_call_back_buffer( char **pp_buffer, _u32 *p_data_len,
    _u8 buddy_hash[KAD_ID_LEN], _u8 file_id[KAD_ID_LEN], _u16 port );

_int32 kad_recv_handle_recv_packet( _u32 ip, _u16 port, char *p_buffer, _u32 data_len );

_int32 kad_on_query( _u32 ip, _u16 port, char *p_buffer, _u32 data_len, _u8 opcode );
_int32 kad_on_query_boot( _u32 ip, _u16 port, char *p_buffer, _u32 data_len );
_int32 kad_on_query_boot2( _u32 ip, _u16 port, char *p_buffer, _u32 data_len );
_int32 kad_on_query_hello( _u32 ip, _u16 port, char *p_buffer, _u32 data_len );
_int32 kad_on_query_hello2( _u32 ip, _u16 port, char *p_buffer, _u32 data_len );
_int32 kad_on_query_hello_common( _u32 ip, _u16 port, _u8 protocol, _u8 opcode, BOOL is_kad2 );
_int32 kad_on_query_find_node( _u32 ip, _u16 port, char *p_buffer, _u32 data_len, BOOL is_kad2 );
_int32 kad_on_query_find_source( _u32 ip, _u16 port, char *p_buffer, _u32 data_len, BOOL is_kad2 );


_int32 kad_on_response( _u32 ip, _u16 port, char *p_buffer, _u32 data_len, _u8 opcode );
_int32 kad_on_response_boot( _u32 ip, _u16 port, char *p_buffer, _u32 data_len );
_int32 kad_on_response_boot2( _u32 ip, _u16 port, char *p_buffer, _u32 data_len );
_int32 kad_parser_contact_node( char **pp_buffer, _u32 *p_data_len, KAD_NODE **pp_kad_node );
_int32 kad_on_response_hello( _u32 ip, _u16 port, char *p_buffer, _u32 data_len );
_int32 kad_on_response_hello2( _u32 ip, _u16 port, char *p_buffer, _u32 data_len );
_int32 kad_on_response_find_node( _u32 ip, _u16 port, char *p_buffer, _u32 data_len, BOOL is_kad2 );
_int32 kad_on_response_find_source( _u32 ip, _u16 port, char *p_buffer, _u32 data_len, BOOL is_kad2 );
_int32 kad_on_response_publish( _u32 ip, _u16 port, char *p_buffer, _u32 data_len );

_int32 kad_fill_my_detail( char **pp_buffer, _u32 *p_buffer_len, BOOL is_kad2 );
_int32 kad_send_data( _u32 ip, _u16 port, const char *p_data, _u32 data_len );

_int32 kad_get_bytes(char **buffer, _int32 *cur_buflen, char *dest_buf, _int32 dest_len);
_int32 kad_set_bytes(char **buffer, _int32 *cur_buflen, char *dest_buf, _int32 dest_len);
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif // !defined(__kad_packet_handler_20091015)





