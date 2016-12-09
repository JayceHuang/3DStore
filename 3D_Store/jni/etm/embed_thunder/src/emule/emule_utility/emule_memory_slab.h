
/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/10/14
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_MEMORY_SLAB_H_
#define _EMULE_MEMORY_SLAB_H_

#include "utility/define.h"
#ifdef ENABLE_EMULE

struct tagEMULE_TAG;
#ifdef ENABLE_EMULE_PROTOCOL


struct tagEMULE_SERVER;
struct tagEMULE_UPLOAD_REQ;
struct tagEMULE_SOCKET_DEVICE;
struct tagEMULE_TCP_DEVICE;
struct tagEMULE_UDT_DEVICE;
struct tagEMULE_UDT_SEND_BUFFER;
struct tagEMULE_UDT_RECV_BUFFER;
struct tagEMULE_UDT_RECV_QUEUE;
struct tagEMULE_UDT_SEND_QUEUE;
struct tagEMULE_UDT_SEND_REQ;
#endif

_int32 emule_init_memory_slab(void);

_int32 emule_uninit_memory_slab(void);

_int32 emule_get_emule_tag_slip(struct tagEMULE_TAG** pointer);

_int32 emule_free_emule_tag_slip(struct tagEMULE_TAG* pointer);
#ifdef ENABLE_EMULE_PROTOCOL


_int32 emule_get_emule_server_slip(struct tagEMULE_SERVER** pointer);

_int32 emule_free_emule_server_slip(struct tagEMULE_SERVER* pointer);

_int32 emule_get_udp_buffer_slip(char** pointer);

_int32 emule_free_udp_buffer_slip(char* pointer);

_int32 emule_get_emule_upload_req_slip(struct tagEMULE_UPLOAD_REQ** pointer);

_int32 emule_free_emule_upload_req_slip(struct tagEMULE_UPLOAD_REQ* pointer);

_int32 emule_get_emule_socket_device_slip(struct tagEMULE_SOCKET_DEVICE** pointer);

_int32 emule_free_emule_socket_device_slip(struct tagEMULE_SOCKET_DEVICE* pointer);

_int32 emule_get_emule_tcp_device_slip(struct tagEMULE_TCP_DEVICE** pointer);

_int32 emule_free_emule_tcp_device_slip(struct tagEMULE_TCP_DEVICE* pointer);

_int32 emule_get_emule_udt_device_slip(struct tagEMULE_UDT_DEVICE** pointer);

_int32 emule_free_emule_udt_device_slip(struct tagEMULE_UDT_DEVICE* pointer);

_int32 emule_get_emule_udt_send_buffer_slip(struct tagEMULE_UDT_SEND_BUFFER** pointer);

_int32 emule_free_emule_udt_send_buffer_slip(struct tagEMULE_UDT_SEND_BUFFER* pointer);

_int32 emule_get_emule_udt_recv_buffer_slip(struct tagEMULE_UDT_RECV_BUFFER** pointer);

_int32 emule_free_emule_udt_recv_buffer_slip(struct tagEMULE_UDT_RECV_BUFFER* pointer);

_int32 emule_get_emule_udt_recv_queue_slip(struct tagEMULE_UDT_RECV_QUEUE** pointer);

_int32 emule_free_emule_udt_recv_queue_slip(struct tagEMULE_UDT_RECV_QUEUE* pointer);

_int32 emule_get_emule_udt_send_queue_slip(struct tagEMULE_UDT_SEND_QUEUE** pointer);

_int32 emule_free_emule_udt_send_queue_slip(struct tagEMULE_UDT_SEND_QUEUE* pointer);

_int32 emule_get_emule_udt_send_req_slip(struct tagEMULE_UDT_SEND_REQ** pointer);

_int32 emule_free_emule_udt_send_req_slip(struct tagEMULE_UDT_SEND_REQ* pointer);

#endif
#endif /* ENABLE_EMULE */
#endif


