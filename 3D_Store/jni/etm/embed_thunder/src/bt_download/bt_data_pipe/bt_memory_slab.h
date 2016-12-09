/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2009/02/26
-----------------------------------------------------------------------------------------------------------*/
#ifndef _BT_MEMORY_SLAB_H_
#define	_BT_MEMORY_SLAB_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#ifdef ENABLE_BT 

struct tagBT_RESOURCE;
struct tagBT_DATA_PIPE;
struct tagBT_DEVICE;
struct tagBT_PIECE_DATA;
struct tagRANGE_CHANGE_NODE;

_int32 init_bt_memory_slab(void);

_int32 uninit_bt_memory_slab(void);

#ifdef ENABLE_BT_PROTOCOL

_int32 bt_malloc_bt_resource(struct tagBT_RESOURCE** pointer);

_int32 bt_free_bt_resource(struct tagBT_RESOURCE* pointer);

_int32 bt_malloc_bt_data_pipe(struct tagBT_DATA_PIPE** pointer);

_int32 bt_free_bt_data_pipe(struct tagBT_DATA_PIPE* pointer);

_int32 bt_malloc_bt_device(struct tagBT_DEVICE** pointer);

_int32 bt_free_bt_device(struct tagBT_DEVICE* pointer);

_int32 bt_malloc_bt_piece_data(struct tagBT_PIECE_DATA** pointer);

_int32 bt_free_bt_piece_data(struct tagBT_PIECE_DATA* pointer);

#endif

_int32 bt_malloc_range_change_node(struct tagRANGE_CHANGE_NODE** pointer);

_int32 bt_free_range_change_node(struct tagRANGE_CHANGE_NODE* pointer);
#endif

#ifdef __cplusplus
}
#endif
#endif
