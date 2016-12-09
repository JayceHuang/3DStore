#include "utility/define.h"
#ifdef ENABLE_EMULE
#include "emule_memory_slab.h"
#include "emule_tag.h"
#include "../emule_server/emule_server_interface.h"
#include "../emule_pipe/emule_pipe_upload.h"
#include "../emule_transfer_layer/emule_socket_device.h"
#include "../emule_transfer_layer/emule_tcp_device.h"
#include "../emule_transfer_layer/emule_udt_device.h"
#include "../emule_transfer_layer/emule_udt_recv_queue.h"
#include "../emule_transfer_layer/emule_udt_send_queue.h"
#include "utility/mempool.h"
#include "utility/errcode.h"
#include "utility/string.h"

static	SLAB*	g_emule_tag_slab = NULL;

#ifdef ENABLE_EMULE_PROTOCOL

static	SLAB*	g_emule_server_slab = NULL;
static	SLAB*	g_emule_udp_buffer_slab = NULL;
static	SLAB*	g_emule_upload_req_slab = NULL;
static	SLAB*	g_emule_socket_device_slab = NULL;
static	SLAB*	g_emule_tcp_device_slab = NULL;
static	SLAB*	g_emule_udt_device_slab = NULL;
static	SLAB*	g_emule_udt_recv_buffer_slab = NULL;
static	SLAB*	g_emule_udt_recv_queue_slab = NULL;
static	SLAB*	g_emule_udt_send_buffer_slab = NULL;
static	SLAB*	g_emule_udt_send_queue_slab = NULL;
static	SLAB*	g_emule_udt_send_req_slab = NULL;
#endif


#define	EMULE_TAG_SLAB_SIZE					128
#define	EMULE_SERVER_SLAB_SIZE				20
#define	EMULE_UDP_BUFFER_SLAB_SIZE			8
#define	EMULE_UPLOAD_REQ_SLAB_SIZE			32
#define	EMULE_SOCKET_DEVICE_SLAB_SIZE		128
#define	EMULE_TCP_DEVICE_SLAB_SIZE			64
#define	EMULE_UDT_DEVICE_SLAB_SIZE			64
#define	EMULE_UDT_SEND_BUFFER_SIZE			64
#define	EMULE_UDT_RECV_BUFFER_SIZE			128
#define	EMULE_UDT_RECV_QUEUE_SIZE			32
#define	EMULE_UDT_SEND_QUEUE_SIZE			32
#define	EMULE_UDT_SEND_REQ_SIZE				32

_int32 emule_init_memory_slab(void)
{
	_int32 ret = SUCCESS;
	ret = mpool_create_slab(sizeof(EMULE_TAG), EMULE_TAG_SLAB_SIZE, 0, &g_emule_tag_slab);
	CHECK_VALUE(ret);
#ifdef ENABLE_EMULE_PROTOCOL

	ret = mpool_create_slab(sizeof(EMULE_SERVER), EMULE_SERVER_SLAB_SIZE, 0, &g_emule_server_slab);
	CHECK_VALUE(ret);
	ret = mpool_create_slab(UDP_BUFFER_SIZE, EMULE_UDP_BUFFER_SLAB_SIZE, 0, &g_emule_udp_buffer_slab);
	CHECK_VALUE(ret);
	ret = mpool_create_slab(sizeof(EMULE_UPLOAD_REQ), EMULE_UPLOAD_REQ_SLAB_SIZE, 0, &g_emule_upload_req_slab);
	CHECK_VALUE(ret);
	ret = mpool_create_slab(sizeof(EMULE_SOCKET_DEVICE), EMULE_SOCKET_DEVICE_SLAB_SIZE, 0, &g_emule_socket_device_slab);
	CHECK_VALUE(ret);
	ret = mpool_create_slab(sizeof(EMULE_TCP_DEVICE), EMULE_TCP_DEVICE_SLAB_SIZE, 0, &g_emule_tcp_device_slab);
	CHECK_VALUE(ret);
	ret = mpool_create_slab(sizeof(EMULE_UDT_DEVICE), EMULE_UDT_DEVICE_SLAB_SIZE, 0, &g_emule_udt_device_slab);
	CHECK_VALUE(ret);
	ret = mpool_create_slab(sizeof(EMULE_UDT_SEND_BUFFER), EMULE_UDT_SEND_BUFFER_SIZE, 0, &g_emule_udt_send_buffer_slab);
	CHECK_VALUE(ret);
	ret = mpool_create_slab(sizeof(EMULE_UDT_RECV_BUFFER), EMULE_UDT_RECV_BUFFER_SIZE, 0, &g_emule_udt_recv_buffer_slab);
	CHECK_VALUE(ret);
	ret = mpool_create_slab(sizeof(EMULE_UDT_RECV_QUEUE), EMULE_UDT_RECV_QUEUE_SIZE, 0, &g_emule_udt_recv_queue_slab);
	CHECK_VALUE(ret);
	ret = mpool_create_slab(sizeof(EMULE_UDT_SEND_QUEUE), EMULE_UDT_SEND_QUEUE_SIZE, 0, &g_emule_udt_send_queue_slab);
	CHECK_VALUE(ret);
	ret = mpool_create_slab(sizeof(EMULE_UDT_SEND_REQ), EMULE_UDT_SEND_REQ_SIZE, 0, &g_emule_udt_send_req_slab);
#endif
	return ret;
}

_int32 emule_uninit_memory_slab(void)
{
	_int32 ret = SUCCESS;
	ret = mpool_destory_slab(g_emule_tag_slab);
	CHECK_VALUE(ret);
	g_emule_tag_slab = NULL;
#ifdef ENABLE_EMULE_PROTOCOL

	ret = mpool_destory_slab(g_emule_server_slab);
	CHECK_VALUE(ret);
	ret = mpool_destory_slab(g_emule_udp_buffer_slab);
	CHECK_VALUE(ret);
	ret = mpool_destory_slab(g_emule_upload_req_slab);
	CHECK_VALUE(ret);
	ret = mpool_destory_slab(g_emule_socket_device_slab);
	CHECK_VALUE(ret);
	ret = mpool_destory_slab(g_emule_tcp_device_slab);
	CHECK_VALUE(ret);
	ret = mpool_destory_slab(g_emule_udt_device_slab);
	CHECK_VALUE(ret);
	ret = mpool_destory_slab(g_emule_udt_send_buffer_slab);
	CHECK_VALUE(ret);
	ret = mpool_destory_slab(g_emule_udt_recv_buffer_slab);
	CHECK_VALUE(ret);
	ret = mpool_destory_slab(g_emule_udt_recv_queue_slab);
	CHECK_VALUE(ret);
	ret = mpool_destory_slab(g_emule_udt_send_queue_slab);
	CHECK_VALUE(ret);
	ret =  mpool_destory_slab(g_emule_udt_send_req_slab);
#endif

    return ret;
}

_int32 emule_get_emule_tag_slip(EMULE_TAG** pointer)
{
	_int32 ret = mpool_get_slip(g_emule_tag_slab, (void**)pointer);
	if(ret == SUCCESS)
		sd_memset(*pointer, 0, sizeof(EMULE_TAG));
	return ret;
}

_int32 emule_free_emule_tag_slip(EMULE_TAG* pointer)
{
	return mpool_free_slip(g_emule_tag_slab, pointer);
}
#ifdef ENABLE_EMULE_PROTOCOL

_int32 emule_get_emule_server_slip(EMULE_SERVER** pointer)
{
	return mpool_get_slip(g_emule_server_slab, (void**)pointer);
}

_int32 emule_free_emule_server_slip(EMULE_SERVER* pointer)
{
	return mpool_free_slip(g_emule_server_slab, pointer);
}

_int32 emule_get_udp_buffer_slip(char** pointer)
{
	return mpool_get_slip(g_emule_udp_buffer_slab, (void**)pointer);
}

_int32 emule_free_udp_buffer_slip(char* pointer)
{
	return mpool_free_slip(g_emule_udp_buffer_slab, pointer);
}

_int32 emule_get_emule_upload_req_slip(EMULE_UPLOAD_REQ** pointer)
{
	_int32 ret = mpool_get_slip(g_emule_upload_req_slab, (void**)pointer);
	if(ret == SUCCESS)
		sd_memset(*pointer, 0, sizeof(EMULE_UPLOAD_REQ));
	return ret;
}

_int32 emule_free_emule_upload_req_slip(EMULE_UPLOAD_REQ* pointer)
{
	return mpool_free_slip(g_emule_upload_req_slab, pointer);
}

_int32 emule_get_emule_socket_device_slip(struct tagEMULE_SOCKET_DEVICE** pointer)
{
	return mpool_get_slip(g_emule_socket_device_slab, (void**)pointer);
}

_int32 emule_free_emule_socket_device_slip(struct tagEMULE_SOCKET_DEVICE* pointer)
{
	return mpool_free_slip(g_emule_socket_device_slab, pointer);
}

_int32 emule_get_emule_tcp_device_slip(struct tagEMULE_TCP_DEVICE** pointer)
{
	return mpool_get_slip(g_emule_tcp_device_slab, (void**)pointer);
}

_int32 emule_free_emule_tcp_device_slip(struct tagEMULE_TCP_DEVICE* pointer)
{
	return mpool_free_slip(g_emule_tcp_device_slab, pointer);
}

_int32 emule_get_emule_udt_device_slip(struct tagEMULE_UDT_DEVICE** pointer)
{
	return mpool_get_slip(g_emule_udt_device_slab, (void**)pointer);
}

_int32 emule_free_emule_udt_device_slip(struct tagEMULE_UDT_DEVICE* pointer)
{
	return mpool_free_slip(g_emule_udt_device_slab, pointer);
}

_int32 emule_get_emule_udt_send_buffer_slip(struct tagEMULE_UDT_SEND_BUFFER** pointer)
{
	return mpool_get_slip(g_emule_udt_send_buffer_slab, (void**)pointer);
}

_int32 emule_free_emule_udt_send_buffer_slip(struct tagEMULE_UDT_SEND_BUFFER* pointer)
{
	return mpool_free_slip(g_emule_udt_send_buffer_slab, pointer);
}

_int32 emule_get_emule_udt_recv_buffer_slip(struct tagEMULE_UDT_RECV_BUFFER** pointer)
{
	return mpool_get_slip(g_emule_udt_recv_buffer_slab, (void**)pointer);
}

_int32 emule_free_emule_udt_recv_buffer_slip(struct tagEMULE_UDT_RECV_BUFFER* pointer)
{
	return mpool_free_slip(g_emule_udt_recv_buffer_slab, pointer);
}

_int32 emule_get_emule_udt_recv_queue_slip(struct tagEMULE_UDT_RECV_QUEUE** pointer)
{
	return mpool_get_slip(g_emule_udt_recv_queue_slab, (void**)pointer);
}

_int32 emule_free_emule_udt_recv_queue_slip(struct tagEMULE_UDT_RECV_QUEUE* pointer)
{
	return mpool_free_slip(g_emule_udt_recv_queue_slab, pointer);
}

_int32 emule_get_emule_udt_send_queue_slip(struct tagEMULE_UDT_SEND_QUEUE** pointer)
{
	return mpool_get_slip(g_emule_udt_send_queue_slab, (void**)pointer);
}

_int32 emule_free_emule_udt_send_queue_slip(struct tagEMULE_UDT_SEND_QUEUE* pointer)
{
	return mpool_free_slip(g_emule_udt_send_queue_slab, pointer);
}

_int32 emule_get_emule_udt_send_req_slip(struct tagEMULE_UDT_SEND_REQ** pointer)
{
	return mpool_get_slip(g_emule_udt_send_req_slab, (void**)pointer);
}

_int32 emule_free_emule_udt_send_req_slip(struct tagEMULE_UDT_SEND_REQ* pointer)
{
	return mpool_free_slip(g_emule_udt_send_req_slab, pointer);
}

#endif /* ENABLE_EMULE */
#endif /* ENABLE_EMULE */

