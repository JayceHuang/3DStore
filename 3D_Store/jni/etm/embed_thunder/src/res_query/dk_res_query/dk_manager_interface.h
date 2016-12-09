/*****************************************************************************
 *
 * Filename: 			dk_manager_interface.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2009/10/15
 *	
 * Purpose:				Define the public platform of dk_manager.
 *
 *****************************************************************************/


#if !defined(__dk_manager_interface_20091015)
#define __dk_manager_interface_20091015

#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#ifdef _DK_QUERY

#include "res_query/dk_res_query/dk_manager.h"

struct tagEMULE_TAG_LIST;

typedef _int32 (*dht_res_handler)( void *p_user, _u32 _ip, _u16 _port );
typedef _int32 (*kad_res_handler)( void *p_user, _u8 _peer_id[KAD_ID_LEN], struct tagEMULE_TAG_LIST *p_tag_list );

//public platform
_int32 dk_init_module();
_int32 dk_uninit_module();

_int32 dht_register_res_handler( dht_res_handler res_handler );
_int32 kad_register_res_handler( kad_res_handler res_handler );

_int32 dk_register_qeury( _u8 *p_key, _u32 key_len, void *p_user, _u64 file_size, _u32 dk_type );
_int32 dht_add_routing_nodes( void *p_user, LIST *p_node_list );
_int32 kad_add_routing_node( _u32 ip, _u16 kad_port, _u8 version );

//_int32 dk_start_qeury( _u8 *p_key, _u32 key_len, _u32 dk_type );
//_int32 dk_stoy_qeury( _u8 *p_key, _u32 key_len, _u32 dk_type );
_int32 dk_unregister_qeury( void *p_user, _u32 dk_type );

#endif


#ifdef __cplusplus
}
#endif

#endif // !defined(__dk_manager_interface_20091015)


