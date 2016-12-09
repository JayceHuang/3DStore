/*****************************************************************************
 *
 * Filename: 			dk_protocal_builder.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2009/10/15
 *	
 * Purpose:				protocal_builder.
 *
 *****************************************************************************/
	 
	 
#if !defined(__dk_protocal_builder_20091015)
#define __dk_protocal_builder_20091015

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#ifdef _DK_QUERY
#include "bt_download/torrent_parser/bencode.h"
#include "res_query/dk_res_query/k_node.h"

struct tagEMULE_TAG_LIST;

typedef _int32 (*ping_cmd_builder)( NODE_INFO *p_node_info, char **pp_cmd_buffer, _u32 *p_buffer_len );

typedef _int32 (*find_node_cmd_builder)( NODE_INFO *p_node_info, K_NODE_ID *p_node_id, _u8 para, char **pp_cmd_buffer, _u32 *p_buffer_len, _u32 *p_key );

//dht query cmd
_int32 dht_build_ping_cmd( NODE_INFO *p_node_info, char **pp_cmd_buffer, _u32 *p_buffer_len );
_int32 dht_build_find_node_cmd( NODE_INFO *p_invalid_para, K_NODE_ID *p_node_id, _u8 invalid_para, char **pp_cmd_buffer, _u32 *p_buffer_len, _u32 *p_key );
_int32 dht_build_get_peers_cmd( NODE_INFO *p_invalid_para, K_NODE_ID *p_node_id, _u8 invalid_para, char **pp_cmd_buffer, _u32 *p_buffer_len, _u32 *p_key );
_int32 dht_build_announce_cmd( _u8 infohash[DHT_ID_LEN], _u16 port, char *p_token, _u32 token_len, char **pp_cmd_buffer, _u32 *p_buffer_len );
_int32 dht_get_query_dict( const char *p_q, BC_DICT *p_a_dict, BC_DICT **pp_bc_dict );

//dht resp cmd
_int32 dht_build_ping_or_announce_resp_cmd( char *p_packet_id, _u32 packet_len, char **pp_cmd_buffer, _u32 *p_buffer_len );
_int32 dht_build_find_node_resp_cmd( char *p_packet_id, _u32 packet_len, char *p_nodes, _u32 nodes_str_len, char **pp_cmd_buffer, _u32 *p_buffer_len );
_int32 dht_build_get_peers_resp_cmd( char *p_packet_id, _u32 packet_len, const char* p_info_key, char *p_info_value, _u32 info_len, char **pp_cmd_buffer, _u32 *p_buffer_len );
_int32 dht_get_resp_dict( char *p_packet_id, _u32 packet_len, BC_DICT *p_r_dict, BC_DICT **pp_bc_dict );

//dht common func
_int32 dht_build_set_local_id( BC_DICT *p_dict );
_int32 dht_create_buffer_from_dict( BC_DICT *p_bc_dict, char **pp_cmd_buffer, _u32 *p_buffer_len );
	

#ifdef ENABLE_EMULE
//kad query cmd
_int32 kad_build_ping_cmd( NODE_INFO *p_node_info, char **pp_cmd_buffer, _u32 *p_buffer_len );
_int32 kad_build_find_node_cmd( NODE_INFO *p_node_info, K_NODE_ID *p_node_id, _u8 cmd_type, char **pp_cmd_buffer, _u32 *p_buffer_len, _u32 *p_packet_key );
_int32 kad_build_find_source_cmd( _u32 ip, _u16 port, _u8 verion, K_NODE_ID *p_node_id, _u64 file_size, char **pp_cmd_buffer, _u32 *p_buffer_len, _u32 *p_packet_key );
_int32 kad_build_announce_source_cmd( _u8 verion, K_NODE_ID *p_node_id,
	struct tagEMULE_TAG_LIST *p_tag_list, char **pp_cmd_buffer, _u32 *p_buffer_len );
#endif

_int32 dk_get_cmd_buffer( char *p_cmd, _u32 cmd_len, char **pp_cmd_buffer, _u32 *p_cmd_buffer_len );

#endif

#ifdef __cplusplus
}
#endif

#endif // !defined(__dk_protocal_builder_20091015)






