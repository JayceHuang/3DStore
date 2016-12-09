/*****************************************************************************
 *
 * Filename: 			dht_recv_packet_handler.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2009/10/15
 *	
 * Purpose:				handle the dht_recv_packet.
 *
 *****************************************************************************/
	 

#if !defined(__dht_recv_packet_handler_20091015)
#define __dht_recv_packet_handler_20091015

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#ifdef _DK_QUERY
#include "bt_download/torrent_parser/bencode.h"

#define DHT_PACKET_ID_BUFFER_LEN 16

_int32 dht_recv_handle_recv_packet( _u32 ip, _u16 port, char *p_buffer, _u32 data_len );

_int32 dht_on_query( _u32 ip, _u16 port, BC_DICT *p_dict_data );
_int32 dht_on_query_ping_or_announce( _u32 ip, _u16 port, BC_DICT *p_dict_data );
_int32 dht_on_query_find_node( _u32 ip, _u16 port, BC_DICT *p_dict_data, BOOL is_get_peers );
_int32 dht_on_query_get_peers( _u32 ip, _u16 port, BC_DICT *p_dict_data );

_int32 dht_on_response( _u32 ip, _u16 port, BC_DICT *p_dict_data );

_int32 dht_on_err( _u32 ip, _u16 port, BC_DICT *p_dict_data );
_u32 dht_get_cur_packet_id( void );
_int32 dht_get_packet_id( char packet_id[DHT_PACKET_ID_BUFFER_LEN] );
#endif


#ifdef __cplusplus
}
#endif

#endif // !defined(__dht_recv_packet_handler_20091015)




