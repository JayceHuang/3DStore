#include "utility/time.h"
#include "utility/utility.h"
#include "utility/errcode.h"
#include "utility/mempool.h"
#include "utility/local_ip.h"
#include "utility/map.h"
#include "utility/list.h"
#include "utility/logid.h"
#include "utility/url.h"
#define	 LOGID	LOGID_SOCKET_PROXY
#include "utility/logger.h"
#include "asyn_frame/device.h"
#include "platform/sd_network.h"
#include "asyn_frame/device.h"
#include "asyn_frame/asyn_frame.h"

#include "socket_proxy_interface_imp.h"

#include "socket_proxy_speed_limit.h"
#include "socket_proxy_encap.h"

#define SOCKET_CONNECT_TIMEOUT 8000
#define SOCKET_RECV_TIMEOUT 20000	
#define SOCKET_UNCOMPLETE_RECV_TIMEOUT 10000
#define QUERY_DNS_TIMEOUT 30000
#define DNS_COMMON_CACHE_SIZE	 18

typedef struct tagPROXY_DATA
{
	void*	_callback_handler;
	void*	_user_data;
}PROXY_DATA;


typedef struct tagPROXY_SENDTO_DNS
{
	SOCKET		_sock;
	const char*	_buffer;
	_u32		_buffer_len;
	void*		_callback_handler;
	void*		_user_data;
	_u16		_port;			/*network byte order*/
	_u32		_timeout;
}PROXY_SENDTO_DNS;

static SLAB*	g_proxy_data_slab = NULL;
static SLAB*	g_proxy_connect_dns_slab = NULL;
static SLAB*	g_proxy_sendto_dns_slab = NULL;

/*dns query record*/
typedef struct tagFD_MSGID_PAIR
{
	SOCKET		_sock;					/*这个地方顺序不能变，_sock一定得排在第一，用于查询set*/
	LIST		_dns_list;				/*node is _u32(msg_id)*/
}FD_MSGID_PAIR;

static SLAB*	g_fd_msgid_pair_slab = NULL;
static SET		g_tcp_fd_msgid_set;		/*node is FD_DNS_NODE*/
static SET		g_udp_fd_msgid_set;		/*node is FD_DNS_NODE*/

static DNS_COMMON_CACHE g_dns_common_cache[DNS_COMMON_CACHE_SIZE];
static MAP g_dns_domain_map;

static _int32 socket_proxy_msg_notify(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid);
static _int32 socket_proxy_connect_dns_notify(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid);
static _int32 socket_proxy_sendto_dns_notify(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid);
static _int32 dns_common_cache_init(DNS_COMMON_CACHE* dns_cache);
static _int32 dns_common_cache_query(const DNS_COMMON_CACHE* dns_cache, const char* host_name, _u32* ip);
static _int32 dns_common_cache_append(DNS_COMMON_CACHE* dns_cache, const char* host_name, const _u32 ip);
static _int32 dns_hash_comp(void *E1, void *E2);
static _int32 dns_get_domain_ip(const char* host_name, _u32* ip);
static _u32 dns_get_domain_num(void);
static DNS_DOMAIN *dns_get_domain_from_map(_u32 hash);
static _int32 dns_add_domain_to_map(DNS_DOMAIN* p_domain);


_int32 fd_dns_node_comparator(void* E1, void* E2)
{
	if (((FD_MSGID_PAIR*)E1)->_sock == ((FD_MSGID_PAIR*)E2)->_sock)
	{
	    return 0;
	}
	else if (((FD_MSGID_PAIR*)E1)->_sock > ((FD_MSGID_PAIR*)E2)->_sock)
	{
	    return 1;
	}
	else
	{
	    return -1;
	}
}

_int32	init_socket_proxy_module()
{
	_int32 ret = SUCCESS;
	LOG_DEBUG("init_socket_proxy_module...");
	ret = init_socket_encap();
	CHECK_VALUE(ret);
	
	ret = mpool_create_slab(sizeof(PROXY_DATA), PROXY_DATA_SLAB_SIZE, 0, &g_proxy_data_slab);
	CHECK_VALUE(ret);
	ret = mpool_create_slab(sizeof(PROXY_CONNECT_DNS), PROXY_CONNECT_DNS_SLAB_SIZE, 0, &g_proxy_connect_dns_slab);
	CHECK_VALUE(ret);
	ret = mpool_create_slab(sizeof(PROXY_SENDTO_DNS), PROXY_SENDTO_DNS_SLAB_SIZE, 0, &g_proxy_sendto_dns_slab);
	CHECK_VALUE(ret);
	ret = mpool_create_slab(sizeof(FD_MSGID_PAIR), FD_MSGID_PAIR_SLAB_SIZE, 0, &g_fd_msgid_pair_slab);
	CHECK_VALUE(ret);
	set_init(&g_tcp_fd_msgid_set, fd_dns_node_comparator);
	set_init(&g_udp_fd_msgid_set, fd_dns_node_comparator);
	dns_common_cache_init(g_dns_common_cache);
	map_init(&g_dns_domain_map, dns_hash_comp);
	
	return init_socket_proxy_speed_limit();
}

_int32 report_dns_query_result(_int32 errcode, OP_PARA_DNS *op)
{
    _int32 ret = SUCCESS;

#if defined(DNS_ASYNC) && defined(EMBED_REPORT)
    _u32 err_code = 0;
    LIST dns_server_ip_list;
    LIST dns_parse_ip_list;
    _u32 i = 0, count = 0;
    REPORTER_SETTING * rp_set = NULL;

    /* 如果解析的域名是 stat_hub，不上报。 */
    rp_set = get_reporter_setting();
    if ((rp_set != NULL) && (0 == sd_strcmp(rp_set->_stat_hub_addr, op->_host_name)))
    {
        return SUCCESS;
    }
        
    switch (errcode)
    {
        case DNS_NO_SERVER:
            err_code = 1;
            break;
        case DNS_INVALID_ADDR:
            err_code = 2;
            break;
        case DNS_NETWORK_EXCEPTION:
            err_code = 3;
            break;
        case DNS_INVALID_REQUEST:
        default:
			/* 域名解析超时或取消不上报 */
			/*
            err_code = 4;
            break;
            */
            LOG_DEBUG("dns request timeout or canceled, don't report it. errcode=%d.", errcode);
            return SUCCESS;
    }

    /* dns server ip. */
    list_init(&dns_server_ip_list);
    count = MIN(op->_server_ip_count, DNS_SERVER_IP_COUNT_MAX); /* 防止异常时溢出 */
    for (i = 0; i < count; ++i)
    {
        list_push(&dns_server_ip_list, (void *)op->_server_ip[i]);
    }

    /* dns parse ip. */
    list_init(&dns_parse_ip_list);
    count = MIN(op->_ip_count, DNS_CONTENT_IP_COUNT_MAX); /* 防止异常时溢出 */
    for (i = 0; i < count; ++i)
    {
        list_push(&dns_parse_ip_list, (void *)op->_ip_list[i]);
    }

    ret =emb_reporter_dns_abnormal(err_code, &dns_server_ip_list, 
        op->_host_name, &dns_parse_ip_list);
    
    list_clear(&dns_server_ip_list);
    list_clear(&dns_parse_ip_list);
#endif

    return ret;
}

_int32 socket_proxy_clear_fd_msgid_set(SET* p_fd_msgid_set)
{
    SET_ITERATOR fd_it = NULL;
	FD_MSGID_PAIR* cur_fd_node = NULL;
    fd_it = SET_BEGIN(*p_fd_msgid_set);
	while(fd_it != SET_END(*p_fd_msgid_set))
	{
	    cur_fd_node = (FD_MSGID_PAIR*)SET_DATA(fd_it);
		sd_close_socket(cur_fd_node->_sock);
		LOG_DEBUG("socket_proxy_clear_fd_msgid_set success close %u ", cur_fd_node->_sock);
		fd_it= SET_NEXT(*p_fd_msgid_set, fd_it);
	}
	return SUCCESS;
}

_int32	uninit_socket_proxy_module()
{
	_int32 ret;
	LOG_DEBUG("uninit_socket_proxy_module...");
	ret = uninit_socket_proxy_speed_limit();
	CHECK_VALUE(ret);
 
       LOG_DEBUG("uninit_socket_proxy_module... , g_tcp_fd_msgid_set size =  %u ", set_size(&g_tcp_fd_msgid_set));
	//sd_assert(set_size(&g_tcp_fd_msgid_set) == 0);   
	socket_proxy_clear_fd_msgid_set(&g_tcp_fd_msgid_set);
	set_clear(&g_tcp_fd_msgid_set)  ;

	LOG_DEBUG("uninit_socket_proxy_module... , g_udp_fd_msgid_set size =  %u ", set_size(&g_udp_fd_msgid_set));
	//sd_assert(set_size(&g_udp_fd_msgid_set) == 0);   

        /*因为udp只会有一个socket 所以这里在底层就关闭了 这里没有必要关闭了*/
	//socket_proxy_clear_fd_msgid_set(&g_udp_fd_msgid_set);
	set_clear(&g_udp_fd_msgid_set)  ;
	
	ret = mpool_destory_slab(g_proxy_sendto_dns_slab);
	CHECK_VALUE(ret);
	g_proxy_sendto_dns_slab = NULL;
	ret = mpool_destory_slab(g_proxy_connect_dns_slab);
	CHECK_VALUE(ret);
	g_proxy_connect_dns_slab = NULL;
	ret = mpool_destory_slab(g_proxy_data_slab);
	CHECK_VALUE(ret);
	g_proxy_data_slab = NULL;
	ret =  mpool_destory_slab(g_fd_msgid_pair_slab);
	CHECK_VALUE(ret);
	g_fd_msgid_pair_slab = NULL;

	ret = uninit_socket_encap();
	return ret;
}

_int32 socket_proxy_create(SOCKET* sock, _int32 type)
{
    *sock = INVALID_SOCKET;
	_int32 ret = sd_create_socket(SD_AF_INET, type, 0, sock);
	LOG_DEBUG("socket_proxy_create, sock = %u, type = %d.", *sock, type);
	return ret;
}

_int32 socket_proxy_create_http_client(SOCKET* sock, _int32 type)
{
	_int32 ret = SUCCESS;
	_u32 sock_num = 0;
	SOCKET_ENCAP_PROP_S* p_encap_prop = NULL;

	ret = sd_malloc(sizeof(SOCKET_ENCAP_PROP_S), (void **)&p_encap_prop);
	CHECK_VALUE(ret);
	sd_memset(p_encap_prop, 0, sizeof(SOCKET_ENCAP_PROP_S));
	p_encap_prop->_encap_type = SOCKET_ENCAP_TYPE_HTTP_CLIENT;

	ret = socket_proxy_create(sock, type);
	CHECK_VALUE(ret);

	sock_num = *sock;
	LOG_DEBUG("socket_proxy_create_http_client, sock = %u, type = %d.", sock_num, type);

	ret = insert_socket_encap_prop(sock_num, p_encap_prop);
	return ret;
}

_int32 socket_proxy_create_browser(SOCKET* sock, _int32 type)
{

	_int32 ret = socket_proxy_create(sock,type);
	if(ret != SUCCESS)
		*sock = INVALID_SOCKET;
	LOG_DEBUG("socket_proxy_create_browser, sock = %u, type = %d.", *sock, type);
	return ret;
}

/*
端口绑定失败的话，会把端口号加1后再次尝试，注意addr->_sin_port会变化，最后绑定成功的端口号不一定是当初传进去的端口号
*/
_int32 socket_proxy_bind(SOCKET sock, SD_SOCKADDR* addr)
{
	_int32 ret = SUCCESS, i;
	for(i = 0; i < 3; ++i)
	{
		ret = sd_socket_bind(sock, addr);
		if(ret == SUCCESS)
		{
		    LOG_ERROR("socket_proxy_bind port success, port = %u", sd_ntohs(addr->_sin_port) );
			return ret;
		}
		else
		{
			LOG_DEBUG("socket_proxy_bind port(%u) failed, try again.", sd_ntohs(addr->_sin_port));
			++addr->_sin_port;
		}
	}
	LOG_DEBUG("socket_proxy_bind port failed.");
	return ret;
}

_int32 socket_proxy_listen(SOCKET sock, _int32 backlog)
{
	return sd_socket_listen(sock, backlog);
}

_int32 socket_proxy_peek_op_count(SOCKET sock, _int32 type, _u32* count)
{
	_u32 speed_limit_count = 0;
	FD_MSGID_PAIR* result = NULL;
	_int32 ret = peek_operation_count_by_device_id(sock, type, count);
	CHECK_VALUE(ret);
	if(type == DEVICE_SOCKET_TCP)
	{
		set_find_node(&g_tcp_fd_msgid_set, (void**)&sock, (void**)&result);
	}
	else	/*type == DEVICE_SOCKET_UDP*/
	{
		set_find_node(&g_udp_fd_msgid_set, &sock, (void**)&result);
	}
	if(result != NULL)
	{	/*found dns query, pending operation must add 1*/
		(*count) = (*count) + list_size(&result->_dns_list);
	}
	/*found if any recv request in speed_limit list*/
	speed_limit_peek_op_count(sock, type, &speed_limit_count);
	(*count) = (*count) + speed_limit_count;
	LOG_DEBUG("socket_proxy_peek_op_count,_socket=%u,type=%d,*count=%u",sock,type,*count);
	return SUCCESS;
}

_int32 socket_proxy_cancel(SOCKET sock, _int32 type)
{
	FD_MSGID_PAIR* result;
	LIST_ITERATOR iter;
	_int32 ret;
	LOG_DEBUG("socket_proxy_cancel,_socket=%u,type=%d",sock,type);
	speed_limit_cancel_request(sock, type);		/*cancel all pending recv request in speed limit*/
	ret = cancel_message_by_device_id(sock, type);
	CHECK_VALUE(ret);
	if(type == DEVICE_SOCKET_TCP)
	{
		set_find_node(&g_tcp_fd_msgid_set, &sock, (void**)&result);
	}
	else		/*DEVICE_SOCKET_UDP*/
	{
		set_find_node(&g_udp_fd_msgid_set, &sock, (void**)&result);
	}
	if(result == NULL)
		return SUCCESS;		/*no dns query*/
	for(iter = LIST_BEGIN(result->_dns_list); iter != LIST_END(result->_dns_list); iter = LIST_NEXT(iter))
	{
		ret = cancel_message_by_msgid((_u32)(iter->_data));
		CHECK_VALUE(ret);
	}
	return ret;
}

_int32 socket_proxy_close(SOCKET sock)
{
	_int32 ret = SUCCESS;
	_u32 sock_num = sock;
	LOG_DEBUG("socket_proxy_close, sock = %u.", sock);
	
	ret = remove_socket_encap_prop(sock_num);
	CHECK_VALUE(ret);
	
	ret = sd_close_socket(sock_num);
	return ret;
}

#ifdef _ENABLE_SSL
_int32 socket_proxy_connect_ssl(SOCKET sock, BIO* pbio, socket_proxy_connect_handler callback_handler, void* user_data)
{
	PROXY_DATA* data;
	MSG_INFO msg_info = {};

	if (callback_handler == NULL) return SP_INVALID_ARGUMENT;

	msg_info._device_id = sock;
	msg_info._device_type = DEVICE_SOCKET_TCP;
	msg_info._msg_priority = MSG_PRIORITY_NORMAL;
	msg_info._operation_type = OP_CONNECT;
	msg_info._operation_parameter = NULL;
	msg_info._ssl_magicnum = SSL_MAGICNUM;
	msg_info._pbio = (void*)pbio;
	if (mpool_get_slip(g_proxy_data_slab, (void**)&data) != SUCCESS)
		return SP_OUT_OF_MEMORY;
	data->_callback_handler = (void*)callback_handler;
	data->_user_data = user_data;
	msg_info._user_data = data;
	return post_message(&msg_info, socket_proxy_msg_notify, NOTICE_ONCE, SOCKET_CONNECT_TIMEOUT, NULL);
}

_int32 socket_proxy_send_ssl(SOCKET sock, BIO* pbio, char* buffer, _u32 len, 
	socket_proxy_send_handler callback_handler, void* user_data)
{
	OP_PARA_CONN_SOCKET_RW para;
	MSG_INFO msg_info = {};
	PROXY_DATA* data;
	para._buffer = buffer;
	para._expect_size = len;
	para._operated_size = 0;
	para._oneshot = FALSE;		/*complete read or write*/
	msg_info._device_id = sock;
	msg_info._device_type = DEVICE_SOCKET_TCP;
	msg_info._msg_priority = MSG_PRIORITY_NORMAL;
	msg_info._operation_type = OP_SEND;
	msg_info._operation_parameter = &para;
	msg_info._ssl_magicnum = SSL_MAGICNUM;
	msg_info._pbio = pbio;
	if (mpool_get_slip(g_proxy_data_slab, (void**)&data) != SUCCESS)
		return SP_OUT_OF_MEMORY;
	data->_callback_handler = callback_handler;
	data->_user_data = user_data;
	msg_info._user_data = data;
	return post_message(&msg_info, socket_proxy_msg_notify, NOTICE_ONCE, WAIT_INFINITE, NULL);
}
_int32 socket_proxy_recv_ssl(SOCKET sock, BIO* pbio, char* buffer, _u32 len, 
	socket_proxy_recv_handler callback_handler, void* user_data)
{
	OP_PARA_CONN_SOCKET_RW para;
	MSG_INFO msg_info = {};
	PROXY_DATA* data;
	para._buffer = buffer;
	para._expect_size = len;
	para._operated_size = 0;
	para._oneshot = 1;		/*complete read or write*/
	msg_info._device_id = sock;
	msg_info._device_type = DEVICE_SOCKET_TCP;
	msg_info._msg_priority = MSG_PRIORITY_NORMAL;
	msg_info._operation_type = OP_RECV;
	msg_info._operation_parameter = &para;
	msg_info._ssl_magicnum = SSL_MAGICNUM;
	msg_info._pbio = pbio;
	if (mpool_get_slip(g_proxy_data_slab, (void**)&data) != SUCCESS)
		return SP_OUT_OF_MEMORY;
	data->_callback_handler = callback_handler;
	data->_user_data = user_data;
	msg_info._user_data = data;
	return post_message(&msg_info, socket_proxy_msg_notify, NOTICE_ONCE, SOCKET_RECV_TIMEOUT, NULL);
}
#endif

_int32 socket_proxy_connect(SOCKET sock, SD_SOCKADDR* addr, socket_proxy_connect_handler callback_handler, void* user_data)
{
	PROXY_DATA* data;
	MSG_INFO msg_info = {};
#if defined(MOBILE_PHONE)
	char addr_buf[16];
	_int32 ret_val = SUCCESS;
#endif	

	if(callback_handler == NULL || addr == NULL)
		return SP_INVALID_ARGUMENT;

	LOG_DEBUG("socket_proxy_connect, sock = %u,%u:%u.", sock,addr->_sin_addr,addr->_sin_port);
#if defined(MOBILE_PHONE)
	if(NT_GPRS_WAP & sd_get_net_type())
	{
		ret_val=sd_inet_ntoa(addr->_sin_addr, addr_buf, 16);
		CHECK_VALUE(ret_val);
		return socket_proxy_connect_by_proxy(sock, addr_buf, sd_ntohs(addr->_sin_port),callback_handler, user_data);
	}
#endif	
	
	msg_info._device_id = sock;
	msg_info._device_type = DEVICE_SOCKET_TCP;
	msg_info._msg_priority = MSG_PRIORITY_NORMAL;
	msg_info._operation_type = OP_CONNECT;
	msg_info._operation_parameter = addr;
	if(mpool_get_slip(g_proxy_data_slab, (void**)&data) != SUCCESS)
		return SP_OUT_OF_MEMORY;
	data->_callback_handler = (void*)callback_handler;
	data->_user_data = user_data;
	msg_info._user_data = data;
	return post_message(&msg_info, socket_proxy_msg_notify, NOTICE_ONCE, SOCKET_CONNECT_TIMEOUT, NULL);
}

_int32 socket_proxy_connect_with_timeout(SOCKET sock, SD_SOCKADDR* addr, _u32 connect_timeout, socket_proxy_connect_handler callback_handler, void* user_data)
{
	PROXY_DATA* data;
	MSG_INFO msg_info = {};
#if defined(MOBILE_PHONE)
	char addr_buf[16];
	_int32 ret_val = SUCCESS;
#endif	

	if(callback_handler == NULL || addr == NULL)
		return SP_INVALID_ARGUMENT;

	LOG_DEBUG("socket_proxy_connect, sock = %u,%u:%u.", sock,addr->_sin_addr,addr->_sin_port);
#if defined(MOBILE_PHONE)
	if(NT_GPRS_WAP & sd_get_net_type())
	{
		ret_val=sd_inet_ntoa(addr->_sin_addr, addr_buf, 16);
		CHECK_VALUE(ret_val);
		return socket_proxy_connect_by_proxy(sock, addr_buf, sd_ntohs(addr->_sin_port),callback_handler, user_data);
	}
#endif	
	
	msg_info._device_id = sock;
	msg_info._device_type = DEVICE_SOCKET_TCP;
	msg_info._msg_priority = MSG_PRIORITY_NORMAL;
	msg_info._operation_type = OP_CONNECT;
	msg_info._operation_parameter = addr;
	if(mpool_get_slip(g_proxy_data_slab, (void**)&data) != SUCCESS)
		return SP_OUT_OF_MEMORY;
	data->_callback_handler = (void*)callback_handler;
	data->_user_data = user_data;
	msg_info._user_data = data;
	return post_message(&msg_info, socket_proxy_msg_notify, NOTICE_ONCE, connect_timeout, NULL);
}

_int32 socket_proxy_connect_by_proxy(SOCKET sock, const char* host, _u16 port, socket_proxy_connect_handler callback_handler, void* user_data)
{
	//PROXY_DATA* data;
	MSG_INFO msg_info = {};
	PROXY_CONNECT_DNS* connect_dns = NULL;
	OP_PARA_DNS para;
	if(callback_handler == NULL || host == NULL)
		return SP_INVALID_ARGUMENT;
	if(mpool_get_slip(g_proxy_connect_dns_slab, (void**)&connect_dns) != SUCCESS)
		return SP_OUT_OF_MEMORY;

	LOG_DEBUG("socket_proxy_connect_by_proxy, sock = %u,%s:%u.user_data=0x%X,connect_dns=0x%X", sock,host,port,user_data,connect_dns);
	connect_dns->_sock = sock;
	connect_dns->_port = port;
	connect_dns->_callback_handler = (void*)callback_handler;
	connect_dns->_user_data = user_data;
	/*query dns*/
	sd_memset(&para,0,sizeof(OP_PARA_DNS));
	para._host_name = (char*)host;
	para._host_name_buffer_len = sd_strlen(host) + 1;
	para._ip_count = 0;
	
	msg_info._device_id = sock;
	msg_info._device_type = DEVICE_SOCKET_TCP;//DEVICE_COMMON;
	msg_info._msg_priority = MSG_PRIORITY_NORMAL;
	msg_info._operation_type = OP_PROXY_CONNECT;//OP_DNS_RESOLVE;
	msg_info._operation_parameter = &para;
	msg_info._user_data = connect_dns;
	return post_message(&msg_info, socket_proxy_msg_notify, NOTICE_ONCE, SOCKET_CONNECT_TIMEOUT*2, NULL);
}

#if defined(_DEBUG)
_int32 socket_proxy_connect_replace_domain(SOCKET sock, const char* host, _u16 port, socket_proxy_connect_handler callback_handler, void* user_data)
{
	_int32 ret_val = -1;
	#if 0 //
	_u32 ip;
	if(sd_strcmp(host,"walkbox.vip.xunlei.com")==0||sd_strcmp(host,"dynamic.walkbox.vip.xunlei.com")==0||sd_strcmp(host,"walkbox.xunlei.com wb.xunlei.com")==0
		||sd_strcmp(host,"jump.walkbox.vip.xunlei.com")==0) 
	{
		SD_SOCKADDR addr;
		sd_inet_aton("10.10.2.13", &ip);
		addr._sin_family = AF_INET;
		addr._sin_addr = ip;
		addr._sin_port = sd_htons(port);
		ret_val = socket_proxy_connect(sock, &addr, callback_handler, user_data);
	}
	else
	if(sd_strcmp(host,"up.delta1.walkbox.vip.xunlei.com")==0||sd_strcmp(host,"up1.delta1.walkbox.vip.xunlei.com")==0||sd_strcmp(host,"up2.delta1.walkbox.vip.xunlei.com")==0||sd_strcmp(host,"dl.delta1.walkbox.vip.xunlei.com")==0) 
	{
		SD_SOCKADDR addr;
		sd_inet_aton("221.238.25.221", &ip);
		addr._sin_family = AF_INET;
		addr._sin_addr = ip;
		addr._sin_port = sd_htons(port);
		ret_val = socket_proxy_connect(sock, &addr, callback_handler, user_data);
	}
	else
	if(sd_strcmp(host,"up2.test3.walkbox.vip.xunlei.com")==0||sd_strcmp(host,"dl0.delta1.walkbox.vip.xunlei.com")==0||sd_strcmp(host,"up.test3.walkbox.vip.xunlei.com")==0||sd_strcmp(host,"up1.test3.walkbox.vip.xunlei.com")==0) 
	{
		SD_SOCKADDR addr;
		sd_inet_aton("221.238.25.222", &ip);
		addr._sin_family = AF_INET;
		addr._sin_addr = ip;
		addr._sin_port = sd_htons(port);
		ret_val = socket_proxy_connect(sock, &addr, callback_handler, user_data);
	}
	else
	if(sd_strcmp(host,"dl1.delta1.walkbox.vip.xunlei.com")==0) 
	{
		SD_SOCKADDR addr;
		sd_inet_aton("221.238.25.223", &ip);
		addr._sin_family = AF_INET;
		addr._sin_addr = ip;
		addr._sin_port = sd_htons(port);
		ret_val = socket_proxy_connect(sock, &addr, callback_handler, user_data);
	}
	else
	if(sd_strcmp(host,"up.delta1.walkbox.vip.xunlei.com")==0) 
	{
		SD_SOCKADDR addr;
		sd_inet_aton("221.238.25.224", &ip);
		addr._sin_family = AF_INET;
		addr._sin_addr = ip;
		addr._sin_port = sd_htons(port);
		ret_val = socket_proxy_connect(sock, &addr, callback_handler, user_data);
	}
	else
	if(sd_strcmp(host,"service2.walkbox.vip.xunlei.com")==0)  // #端口 12091 测试环境
	{
		SD_SOCKADDR addr;
		sd_inet_aton("221.238.25.228", &ip);
		addr._sin_family = AF_INET;
		addr._sin_addr = ip;
		addr._sin_port = sd_htons(12091);
		ret_val = socket_proxy_connect(sock, &addr, callback_handler, user_data);
	}
	else
	if(sd_strcmp(host,"up.delta2.walkbox.vip.xunlei.com")==0)  
	{
		SD_SOCKADDR addr;
		sd_inet_aton("121.10.137.168", &ip);
		addr._sin_family = AF_INET;
		addr._sin_addr = ip;
		addr._sin_port = sd_htons(port);
		ret_val = socket_proxy_connect(sock, &addr, callback_handler, user_data);
	}
	else
	if(sd_strcmp(host,"dl.delta2.walkbox.vip.xunlei.com")==0)  
	{
		SD_SOCKADDR addr;
		sd_inet_aton("121.10.137.165", &ip);
		addr._sin_family = AF_INET;
		addr._sin_addr = ip;
		addr._sin_port = sd_htons(port);
		ret_val = socket_proxy_connect(sock, &addr, callback_handler, user_data);
	}
	else
	if(sd_strcmp(host,"dl0.delta2.walkbox.vip.xunlei.com")==0)  
	{
		SD_SOCKADDR addr;
		sd_inet_aton("121.10.137.166", &ip);
		addr._sin_family = AF_INET;
		addr._sin_addr = ip;
		addr._sin_port = sd_htons(port);
		ret_val = socket_proxy_connect(sock, &addr, callback_handler, user_data);
	}
	else
	if(sd_strcmp(host,"com.down.sandai.net")==0)  
	{
		SD_SOCKADDR addr;
		sd_inet_aton("192.168.14.80", &ip);
		addr._sin_family = AF_INET;
		addr._sin_addr = ip;
		addr._sin_port = sd_htons(port);
		ret_val = socket_proxy_connect(sock, &addr, callback_handler, user_data);
	}
	else
	if(sd_strcmp(host,"gdl.lixian.vip.xunlei.com")==0||sd_strcmp(host,"vod0.test2.lixian.vip.xunlei.com")==0) 
	{
		SD_SOCKADDR addr;
		sd_inet_aton("61.188.190.45", &ip);
		addr._sin_family = AF_INET;
		addr._sin_addr = ip;
		addr._sin_port = sd_htons(port);
		ret_val = socket_proxy_connect(sock, &addr, callback_handler, user_data);
	}
	else
	if(sd_strcmp(host,"vod2.test2.lixian.vip.xunlei.com")==0)  
	{
		SD_SOCKADDR addr;
		sd_inet_aton("61.188.190.47", &ip);
		addr._sin_family = AF_INET;
		addr._sin_addr = ip;
		addr._sin_port = sd_htons(port);
		ret_val = socket_proxy_connect(sock, &addr, callback_handler, user_data);
	}
	else
	if(sd_strcmp(host,"vod0.test1.lixian.vip.xunlei.com")==0)  
	{
		SD_SOCKADDR addr;
		sd_inet_aton("61.188.190.49", &ip);
		addr._sin_family = AF_INET;
		addr._sin_addr = ip;
		addr._sin_port = sd_htons(port);
		ret_val = socket_proxy_connect(sock, &addr, callback_handler, user_data);
	}
	else
	if(sd_strcmp(host,"vod1.test1.lixian.vip.xunlei.com")==0)  
	{
		SD_SOCKADDR addr;
		sd_inet_aton("61.188.190.54", &ip);
		addr._sin_family = AF_INET;
		addr._sin_addr = ip;
		addr._sin_port = sd_htons(port);
		ret_val = socket_proxy_connect(sock, &addr, callback_handler, user_data);
	}
	else
	if(sd_strcmp(host,"thumbnail.delta2.walkbox.vip.xunlei.com")==0) 
	{
		SD_SOCKADDR addr;
		sd_inet_aton("121.10.137.166", &ip);
		addr._sin_family = AF_INET;
		addr._sin_addr = ip;
		addr._sin_port = sd_htons(port);
		ret_val = socket_proxy_connect(sock, &addr, callback_handler, user_data);
	}
	else
	if(sd_strcmp(host,"vod.delta2.walkbox.vip.xunlei.com")==0) 
	{
		SD_SOCKADDR addr;
		sd_inet_aton("121.10.137.166", &ip);
		addr._sin_family = AF_INET;
		addr._sin_addr = ip;
		addr._sin_port = sd_htons(port);
		ret_val = socket_proxy_connect(sock, &addr, callback_handler, user_data);
	}
	else
	if(sd_strcmp(host,"wireless.walkbox.vip.xunlei.com")==0) 
	{
		SD_SOCKADDR addr;
		sd_inet_aton("121.10.137.163", &ip);
		addr._sin_family = AF_INET;
		addr._sin_addr = ip;
		addr._sin_port = sd_htons(port);
		ret_val = socket_proxy_connect(sock, &addr, callback_handler, user_data);
	}
	#elif defined(_ENABLE_LIXIAN)
	_u32 ip; /*
	if(sd_strcmp(host,"gdl.lxtest.lixian.vip.xunlei.com")==0) 
	{
		SD_SOCKADDR addr;
		sd_inet_aton("10.10.199.121", &ip);
		addr._sin_family = AF_INET;
		addr._sin_addr = ip;
		addr._sin_port = sd_htons(port);
		ret_val = socket_proxy_connect(sock, &addr, callback_handler, user_data);
	}
	else
	if(sd_strcmp(host,"dl.test2.lixian.vip.xunlei.com")==0 || sd_strcmp(host,"dl.test1.lixian.vip.xunlei.com")==0 || sd_strcmp(host,"dl.test2.lixian.vip.xunlei.com")==0) 
	{
		SD_SOCKADDR addr;
		sd_inet_aton("10.10.199.26", &ip);
		addr._sin_family = AF_INET;
		addr._sin_addr = ip;
		addr._sin_port = sd_htons(port);
		ret_val = socket_proxy_connect(sock, &addr, callback_handler, user_data);
	}
	else
	if(sd_strcmp(host,"vod0.test1.lixian.vip.xunlei.com")==0||sd_strcmp(host,"vod0.test2.lixian.vip.xunlei.com")==0) 
	{
		SD_SOCKADDR addr;
		sd_inet_aton("10.10.199.11", &ip);
		addr._sin_family = AF_INET;
		addr._sin_addr = ip;
		addr._sin_port = sd_htons(port);
		ret_val = socket_proxy_connect(sock, &addr, callback_handler, user_data);
	}
	else
	if(sd_strcmp(host,"vod1.test1.lixian.vip.xunlei.com")==0 || sd_strcmp(host,"vod1.test2.lixian.vip.xunlei.com")==0) 
	{
		SD_SOCKADDR addr;
		sd_inet_aton("10.10.199.12", &ip);
		addr._sin_family = AF_INET;
		addr._sin_addr = ip;
		addr._sin_port = sd_htons(port);
		ret_val = socket_proxy_connect(sock, &addr, callback_handler, user_data);
	}
	else
	if(sd_strcmp(host,"vod2.test1.lixian.vip.xunlei.com")==0||sd_strcmp(host,"vod3.test2.lixian.vip.xunlei.com")==0||sd_strcmp(host,"dl.test1.lixian.vip.xunlei.com")==0) 
	{
		SD_SOCKADDR addr;
		sd_inet_aton("10.10.199.13", &ip);
		addr._sin_family = AF_INET;
		addr._sin_addr = ip;
		addr._sin_port = sd_htons(port);
		ret_val = socket_proxy_connect(sock, &addr, callback_handler, user_data);
	}
	else 
	if(sd_strcmp(host,"gdl.lixian.vip.xunlei.com")==0) 
	{
		SD_SOCKADDR addr;
		sd_inet_aton("121.10.137.161", &ip);
		addr._sin_family = AF_INET;
		addr._sin_addr = ip;
		addr._sin_port = sd_htons(port);
		ret_val = socket_proxy_connect(sock, &addr, callback_handler, user_data);
	}
	else */
        if(sd_strcmp(host,"pad.i.vod.xunlei.com")==0) 
        {
            SD_SOCKADDR addr;
            sd_inet_aton("114.112.165.196", &ip);
            //sd_inet_aton("123.150.206.196", &ip);
            //sd_inet_aton("221.238.25.193", &ip); /* 正式服务器 */
            addr._sin_family = AF_INET;
            addr._sin_addr = ip;
            addr._sin_port = sd_htons(port);
            ret_val = socket_proxy_connect(sock, &addr, callback_handler, user_data);
        }
	#endif
	return ret_val;
}
#endif
_int32 socket_proxy_connect_by_domain(SOCKET sock, const char* host, _u16 port, socket_proxy_connect_handler callback_handler, void* user_data)
{
	_int32 ret;
	_u32  msgid;
	FD_MSGID_PAIR* fd_msgid = NULL;
	PROXY_CONNECT_DNS* connect_dns = NULL;
	OP_PARA_DNS para;
	MSG_INFO msg_info = {};
	_u32 ip;

	LOG_DEBUG("socket_proxy_connect_by_domain, sock = %u,%s:%u.", sock,host,port);

	if(sd_strlen(host)<3) return SP_INVALID_ARGUMENT;
	
#if defined(MOBILE_PHONE)
	if(NT_GPRS_WAP & sd_get_net_type())
	{
		return socket_proxy_connect_by_proxy(sock, host, port,callback_handler, user_data);
	}
#endif	
#if defined(_DEBUG)
	/* 某种情况下需要修改host配置进行调试 */
	ret = socket_proxy_connect_replace_domain( sock, host,  port,  callback_handler, user_data);
	if(ret==SUCCESS) return ret;
#endif
	if(sd_inet_aton(host, &ip) == SUCCESS 
	||dns_get_domain_ip(host, &ip) == SUCCESS 	
	|| dns_common_cache_query(g_dns_common_cache, host, &ip) == SUCCESS)
	{	/*no need to query dns*/
		SD_SOCKADDR addr;
		addr._sin_family = AF_INET;
		addr._sin_addr = ip;
		addr._sin_port = sd_htons(port);
		return socket_proxy_connect(sock, &addr, callback_handler, user_data);
	}
	LOG_DEBUG("socket_proxy_connect_by_domain, host = %s, sock = %u.", host, sock);
	if(host == NULL || callback_handler == NULL)
		return SP_INVALID_ARGUMENT;
	if(mpool_get_slip(g_proxy_connect_dns_slab, (void**)&connect_dns) != SUCCESS)
		return SP_OUT_OF_MEMORY;
	if(mpool_get_slip(g_fd_msgid_pair_slab, (void**)&fd_msgid) != SUCCESS)
		return SP_OUT_OF_MEMORY;
	connect_dns->_sock = sock;
	connect_dns->_port = sd_htons(port);
	connect_dns->_callback_handler = (void*)callback_handler;
	connect_dns->_user_data = user_data;
	/*query dns*/
	para._host_name = (char*)host;
	para._host_name_buffer_len = sd_strlen(host) + 1;
	para._ip_count = 1;
#ifdef DNS_ASYNC
    para._server_ip[0] = 0;
    para._server_ip_count = 0;
#endif
	ret = sd_malloc(sizeof(_u32) * para._ip_count, (void**)&para._ip_list);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("socket_proxy_connect_by_domain, malloc failed.");
		CHECK_VALUE(ret);
	}
	msg_info._device_id = 0;
	msg_info._device_type = DEVICE_COMMON;
	msg_info._msg_priority = MSG_PRIORITY_NORMAL;
	msg_info._operation_type = OP_DNS_RESOLVE;
	msg_info._operation_parameter = &para;
	msg_info._user_data = connect_dns;
	ret = post_message(&msg_info, socket_proxy_connect_dns_notify, NOTICE_ONCE, QUERY_DNS_TIMEOUT, &msgid);
	CHECK_VALUE(ret);
	list_init(&fd_msgid->_dns_list);
	list_push(&fd_msgid->_dns_list, (void*)msgid);
	fd_msgid->_sock = sock;
	ret = set_insert_node(&g_tcp_fd_msgid_set, fd_msgid);
	sd_assert(ret != MAP_DUPLICATE_KEY);
	return ret;
}

static _int32 socket_proxy_connect_dns_notify(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid)
{
	_u32 pending_op_count = msg_info->_pending_op_count;
	FD_MSGID_PAIR* fd_msgid = NULL;
	PROXY_CONNECT_DNS* dns = (PROXY_CONNECT_DNS*)msg_info->_user_data;
	OP_PARA_DNS* op = (OP_PARA_DNS*)msg_info->_operation_parameter;
	socket_proxy_connect_handler callback;
	LOG_DEBUG("socket_proxy_connect_dns_notify, errcode = %d, msgid = %u.", errcode, msgid);
	/*dns operation finish, update g_tcp_fd_msgid_set*/
	set_find_node(&g_tcp_fd_msgid_set, (void*)&dns->_sock, (void**)&fd_msgid);
	sd_assert(fd_msgid != NULL);
	if(fd_msgid != NULL)
	{
		LIST_ITERATOR iter;
		for(iter = LIST_BEGIN(fd_msgid->_dns_list); iter != LIST_END(fd_msgid->_dns_list); iter = LIST_NEXT(iter))
		{
			if( (_u32)(LIST_VALUE(iter)) == msgid )
			{
				list_erase(&fd_msgid->_dns_list, iter);
				break;
			}
		}
		if(list_size(&fd_msgid->_dns_list) == 0)
		{
			_int32 ret = set_erase_node(&g_tcp_fd_msgid_set, fd_msgid);
			CHECK_VALUE(ret);
			mpool_free_slip(g_fd_msgid_pair_slab, fd_msgid);
		}
	}
	if(errcode == SUCCESS && op->_ip_count > 0)
	{ 
		SD_SOCKADDR addr;
		addr._sin_family = SD_AF_INET;
		addr._sin_addr = ((OP_PARA_DNS*)msg_info->_operation_parameter)->_ip_list[0];
		addr._sin_port = dns->_port;
		dns_common_cache_append(g_dns_common_cache, op->_host_name, addr._sin_addr);
		socket_proxy_connect(dns->_sock, &addr, (socket_proxy_connect_handler)dns->_callback_handler, dns->_user_data);
	}
	else
	{	/*query dns failed*/
        report_dns_query_result(errcode, op);

		callback = (socket_proxy_connect_handler)dns->_callback_handler;
		if(errcode == MSG_CANCELLED)
			peek_operation_count_by_device_id(dns->_sock, DEVICE_SOCKET_TCP, &pending_op_count);
		callback((errcode == SUCCESS? -1 : errcode), pending_op_count, dns->_user_data);
	}
	sd_free(op->_ip_list);
	op->_ip_list = NULL;
	return mpool_free_slip(g_proxy_connect_dns_slab, dns);
}

_int32 socket_proxy_accept(SOCKET sock, socket_proxy_accept_handler callback_handler, void* user_data)
{
	OP_PARA_ACCEPT op;
	MSG_INFO msg_info = {};
	PROXY_DATA* data;
	if(callback_handler == NULL)
		return SP_INVALID_ARGUMENT;
	msg_info._device_id = sock;
	msg_info._device_type = DEVICE_SOCKET_TCP;
	msg_info._msg_priority = MSG_PRIORITY_NORMAL;
	msg_info._operation_type = OP_ACCEPT;
	msg_info._operation_parameter = &op;
	if(mpool_get_slip(g_proxy_data_slab, (void**)&data) != SUCCESS)
		return SP_OUT_OF_MEMORY;
	data->_callback_handler = (void*)callback_handler;
	data->_user_data = user_data;
	msg_info._user_data = data;
	LOG_DEBUG("socket_proxy_accept, sock = %u, callback = 0x%x, user_data = 0x%x.", sock, callback_handler, user_data);
	return post_message(&msg_info, socket_proxy_msg_notify, NOTICE_ONCE, WAIT_INFINITE, NULL);
}


_int32 socket_proxy_send_in_speed_limit(SOCKET sock
    , const char* buffer
    , _u32 len
    , socket_proxy_send_handler callback_handler
    , void* user_data)
{
	_int32 ret;
	if(callback_handler == NULL || buffer == NULL || len == 0)
		return SP_INVALID_ARGUMENT;
	ret = speed_limit_add_send_request_data_item(sock
	    , DEVICE_SOCKET_TCP
	    , buffer
	    , len
	    , NULL
	    , (void*)callback_handler
	    , user_data);
	CHECK_VALUE(ret);
	speed_limit_update_send_request();
	return SUCCESS;   
}

_int32 socket_proxy_send(SOCKET sock, const char* buffer, _u32 len, socket_proxy_send_handler callback_handler, void* user_data)
{
	_int32 ret = SUCCESS;
	_u32 sock_num = sock;
	SOCKET_ENCAP_PROP_S* p_encap_prop = NULL;
	
	ret = get_socket_encap_prop_by_sock(sock_num, &p_encap_prop);
	CHECK_VALUE(ret);
	if (p_encap_prop == NULL || p_encap_prop->_encap_type == SOCKET_ENCAP_TYPE_NONE)
	{
		ret = socket_proxy_send_function(sock, buffer, len, callback_handler, user_data);
	}
	else if (p_encap_prop->_encap_type == SOCKET_ENCAP_TYPE_HTTP_CLIENT)
	{
		ret = socket_encap_http_client_send(sock, buffer, len, callback_handler, user_data);		
	}
	else
	{
		CHECK_VALUE(-1);
	}
	return ret;
}

_int32 socket_proxy_send_function(SOCKET sock, const char* buffer, _u32 len, socket_proxy_send_handler callback_handler, void* user_data)
{
    /*
	_int32 ret;
	if(callback_handler == NULL || buffer == NULL || len == 0)
		return SP_INVALID_ARGUMENT;
	ret = speed_limit_add_send_request(sock, DEVICE_SOCKET_TCP, buffer, len, NULL, (void*)callback_handler, user_data);
	CHECK_VALUE(ret);
	speed_limit_update_send_request();
	return SUCCESS;
	*/
    OP_PARA_CONN_SOCKET_RW para;
	MSG_INFO msg_info = {};
	PROXY_DATA* data;
	para._buffer = (char*)buffer;
	para._expect_size = len;
	para._operated_size = 0;
	para._oneshot = FALSE;		/*complete read or write*/
	msg_info._device_id = sock;
	msg_info._device_type = DEVICE_SOCKET_TCP;
	msg_info._msg_priority = MSG_PRIORITY_NORMAL;
	msg_info._operation_type = OP_SEND;
	msg_info._operation_parameter = &para; 
	if(mpool_get_slip(g_proxy_data_slab, (void**)&data) != SUCCESS)
		return SP_OUT_OF_MEMORY;
	data->_callback_handler = callback_handler;
	data->_user_data = user_data;
	msg_info._user_data = data;
	return post_message(&msg_info, socket_proxy_msg_notify, NOTICE_ONCE, WAIT_INFINITE, NULL);  	
}

_int32 socket_proxy_send_impl(SOCKET_SEND_REQUEST* request)
{
	OP_PARA_CONN_SOCKET_RW para;
	MSG_INFO msg_info = {};
	PROXY_DATA* data;
	para._buffer = (char*)request->_buffer;
	para._expect_size = request->_len;
	para._operated_size = 0;
	para._oneshot = FALSE;		/*complete read or write*/
	msg_info._device_id = request->_sock;
	msg_info._device_type = DEVICE_SOCKET_TCP;
	msg_info._msg_priority = MSG_PRIORITY_NORMAL;
	msg_info._operation_type = OP_SEND;
	msg_info._operation_parameter = &para; 
	if(mpool_get_slip(g_proxy_data_slab, (void**)&data) != SUCCESS)
		return SP_OUT_OF_MEMORY;
	data->_callback_handler = (void*)request->_callback;
	data->_user_data = request->_user_data;
	msg_info._user_data = data;
	return post_message(&msg_info, socket_proxy_msg_notify, NOTICE_ONCE, WAIT_INFINITE, NULL);
}

_int32 socket_proxy_recv(SOCKET sock, char* buffer, _u32 len, socket_proxy_recv_handler callback_handler, void* user_data)
{
	_int32 ret = SUCCESS;
	_u32 sock_num = sock;
	SOCKET_ENCAP_PROP_S* p_encap_prop = NULL;
	ret = get_socket_encap_prop_by_sock(sock_num, &p_encap_prop);
	CHECK_VALUE(ret);
	if (p_encap_prop == NULL || p_encap_prop->_encap_type == SOCKET_ENCAP_TYPE_NONE)
	{
		ret = socket_proxy_recv_function(sock, buffer, len, callback_handler, user_data, FALSE);
	}
	else if (p_encap_prop->_encap_type == SOCKET_ENCAP_TYPE_HTTP_CLIENT)
	{
		ret = socket_encap_http_client_recv(sock, buffer, len, callback_handler, user_data);		
	}
	else
	{
		CHECK_VALUE(-1);
	}
	return ret;
}

_int32 socket_proxy_recv_function(SOCKET sock, char* buffer, _u32 len, socket_proxy_recv_handler callback_handler, void* user_data, BOOL is_one_shot)
{
	_int32 ret;

	/* len==0: 上层仅期望获得一个 recv callback.(比如上层自身的recv buf中还有可用的数据)。 */
	if(callback_handler == NULL || buffer == NULL)
		return SP_INVALID_ARGUMENT;
	ret = speed_limit_add_recv_request(sock, DEVICE_SOCKET_TCP, buffer, len, (void*)callback_handler, user_data, is_one_shot);
	CHECK_VALUE(ret);
	speed_limit_update_request();
	return SUCCESS;
}

_int32 socket_proxy_recv_impl(SOCKET_RECV_REQUEST* request)
{
	_int32 ret;
	OP_PARA_CONN_SOCKET_RW	para;
	MSG_INFO msg_info = {};
	PROXY_DATA* data;
	socket_proxy_recv_handler callback = (socket_proxy_recv_handler)request->_callback;
	para._buffer = request->_buffer;
	para._expect_size = request->_len;
	para._operated_size = 0;
	para._oneshot = request->_oneshot;
	msg_info._device_id = request->_sock;
	msg_info._device_type = request->_type;
	msg_info._msg_priority = MSG_PRIORITY_NORMAL;
	msg_info._operation_type = OP_RECV;
	msg_info._operation_parameter = &para;
	ret = mpool_get_slip(g_proxy_data_slab, (void**)&data);
	if(ret != SUCCESS)
	{
		LOG_ERROR("socket_proxy_recv_impl, mpool_get_slip failed, errcode = %d", ret);
		return callback(ret, 0, request->_buffer, 0, request->_user_data);
	}
	data->_callback_handler = (void*)callback;
	data->_user_data = request->_user_data;
	msg_info._user_data = data;
	if(request->_oneshot == FALSE)
		ret = post_message(&msg_info, socket_proxy_msg_notify, NOTICE_ONCE, SOCKET_RECV_TIMEOUT, NULL);
	else 
		ret = post_message(&msg_info, socket_proxy_msg_notify, NOTICE_ONCE, SOCKET_UNCOMPLETE_RECV_TIMEOUT, NULL);
	sd_assert(ret == SUCCESS);
	return ret;
}

_int32 socket_proxy_sendto_in_speed_limit(SOCKET sock
    , const char* buffer
    , _u32 len
    , SD_SOCKADDR* addr
    , socket_proxy_sendto_handler callback_handler
    , void* user_data)
{
    LOG_DEBUG("socket_proxy_sendto_in_speed_limit called, sock = %d, len = %d", sock, len);
   	_int32 ret;
	if(callback_handler == NULL || buffer == NULL || len == 0)
		return SP_INVALID_ARGUMENT;
	ret = speed_limit_add_send_request_data_item(sock, DEVICE_SOCKET_UDP, buffer
	    , len, addr, (void*)callback_handler, user_data);
	CHECK_VALUE(ret);
	speed_limit_update_send_request();
	return SUCCESS; 
}

_int32 socket_proxy_sendto(SOCKET sock, const char* buffer, _u32 len, SD_SOCKADDR* addr
    , socket_proxy_sendto_handler callback_handler, void* user_data)
{
    _int32 ret = SUCCESS;
    OP_PARA_NOCONN_SOCKET_RW para;
	MSG_INFO msg_info = {};
    PROXY_DATA* data = NULL;
    para._buffer = (char*)buffer;
    para._expect_size = len;
    para._operated_size = 0;
    para._sockaddr = *addr;
    msg_info._device_id = sock;
    msg_info._device_type = DEVICE_SOCKET_UDP;
    msg_info._msg_priority = MSG_PRIORITY_NORMAL;
    msg_info._operation_type = OP_SENDTO;
    msg_info._operation_parameter = &para; 
    if(mpool_get_slip(g_proxy_data_slab, (void**)&data) != SUCCESS)
        return INVALID_ARGUMENT;
    data->_callback_handler = (void*)callback_handler;
    data->_user_data = user_data;
    msg_info._user_data = data;
    ret = post_message(&msg_info, socket_proxy_msg_notify, NOTICE_ONCE, WAIT_INFINITE, NULL);
    if(ret != SUCCESS)
    {
        LOG_ERROR("socket_proxy_sendto failed, post_message return errcode %d.", ret);
        mpool_free_slip(g_proxy_data_slab, data);
    }
    return ret;

}

_int32 socket_proxy_sendto_impl(SOCKET_SEND_REQUEST* request)
{
	_int32 ret = SUCCESS;
	OP_PARA_NOCONN_SOCKET_RW para;
	MSG_INFO msg_info = {};
	PROXY_DATA* data = NULL;
	para._buffer = (char*)request->_buffer;
	para._expect_size = request->_len;
	para._operated_size = 0;
	para._sockaddr = request->_addr;
	msg_info._device_id = request->_sock;
	msg_info._device_type = DEVICE_SOCKET_UDP;
	msg_info._msg_priority = MSG_PRIORITY_NORMAL;
	msg_info._operation_type = OP_SENDTO;
	msg_info._operation_parameter = &para; 
	if(mpool_get_slip(g_proxy_data_slab, (void**)&data) != SUCCESS)
		return INVALID_ARGUMENT;
	data->_callback_handler = (void*)request->_callback;
	data->_user_data = request->_user_data;
	msg_info._user_data = data;
	ret = post_message(&msg_info, socket_proxy_msg_notify, NOTICE_ONCE, WAIT_INFINITE, NULL);
	if(ret != SUCCESS)
	{
		LOG_ERROR("socket_proxy_sendto failed, post_message return errcode %d.", ret);
		mpool_free_slip(g_proxy_data_slab, data);
	}
	return ret;
}

_int32 socket_proxy_sendto_by_domain(SOCKET sock, const char* buffer, _u32 len, const char* host, _u16 port, socket_proxy_sendto_handler callback_handler, void* user_data)
{
	_u32 msgid, ip;
	_int32 ret;
	FD_MSGID_PAIR* fd_msgid = NULL;
	PROXY_SENDTO_DNS* dns = NULL;
	OP_PARA_DNS para;
	MSG_INFO msg_info = {};
	if(callback_handler == NULL || buffer == NULL || len == 0 || host == NULL)
		return SP_INVALID_ARGUMENT;
	if(sd_inet_aton(host, &ip) == SUCCESS 
	||dns_get_domain_ip(host, &ip) == SUCCESS 
	||dns_common_cache_query(g_dns_common_cache, host, &ip) == SUCCESS)
	{	/*no need to query dns*/
		SD_SOCKADDR addr;
		addr._sin_family = AF_INET;
		addr._sin_addr = ip;
		addr._sin_port = sd_htons(port);
		return socket_proxy_sendto(sock, buffer, len, &addr, callback_handler, user_data);
	}
	if(mpool_get_slip(g_proxy_sendto_dns_slab, (void**)&dns) != SUCCESS)
		return SP_OUT_OF_MEMORY;
	dns->_sock = sock;
	dns->_buffer = buffer;
	dns->_buffer_len = len;
	dns->_port = sd_htons(port);
	dns->_callback_handler = (void*)callback_handler;
	dns->_user_data = user_data;
	/*query dns*/
	para._host_name = (char*)host;
	para._host_name_buffer_len = sd_strlen(host) + 1;
	para._ip_count = 1;
#ifdef DNS_ASYNC
    para._server_ip[0] = 0;
    para._server_ip_count = 0;
#endif
	ret = sd_malloc(sizeof(_u32) * para._ip_count, (void**)&para._ip_list);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("socket_proxy_sendto_by_domain, malloc failed.");
		CHECK_VALUE(ret);
	}
	msg_info._device_id = 0;
	msg_info._device_type = DEVICE_COMMON;
	msg_info._msg_priority = MSG_PRIORITY_NORMAL;
	msg_info._operation_type = OP_DNS_RESOLVE;
	msg_info._operation_parameter = &para;
	msg_info._user_data = dns;
	ret = post_message(&msg_info, socket_proxy_sendto_dns_notify, NOTICE_ONCE, QUERY_DNS_TIMEOUT, &msgid);
	if(ret != SUCCESS)
	{
		LOG_ERROR("socket_proxy_sendto_by_domain failed, post_message return errcode = %d.", ret);
		mpool_free_slip(g_proxy_sendto_dns_slab, dns);
		sd_free(para._ip_list);
		para._ip_list = NULL;
		return ret;
	}
	set_find_node(&g_udp_fd_msgid_set, &sock, (void**)&fd_msgid);
	if(fd_msgid == NULL)
	{
		mpool_get_slip(g_fd_msgid_pair_slab, (void**)&fd_msgid);
		fd_msgid->_sock = sock;
		list_init(&fd_msgid->_dns_list);
		set_insert_node(&g_udp_fd_msgid_set, fd_msgid);
	}
	return list_push(&fd_msgid->_dns_list, (void*)msgid);
}

static _int32 socket_proxy_sendto_dns_notify(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid)
{
	_u32 pending_op_count = msg_info->_pending_op_count;
	FD_MSGID_PAIR*	fd_msgid = NULL;
	PROXY_SENDTO_DNS* dns = (PROXY_SENDTO_DNS*)msg_info->_user_data;
	OP_PARA_DNS* op = (OP_PARA_DNS*)msg_info->_operation_parameter;
	socket_proxy_sendto_handler callback;
	/*dns operation finish, update g_udp_fd_msgid_set*/
	set_find_node(&g_udp_fd_msgid_set, (void*)&dns->_sock, (void**)&fd_msgid);
	sd_assert(fd_msgid != NULL);
	if(fd_msgid != NULL)
	{
		LIST_ITERATOR iter;
		for(iter = LIST_BEGIN(fd_msgid->_dns_list); iter != LIST_END(fd_msgid->_dns_list); iter = LIST_NEXT(iter))
		{
			if( (_u32)(LIST_VALUE(iter)) == msgid )
			{
				list_erase(&fd_msgid->_dns_list, iter);
				break;
			}
		}
		if(list_size(&fd_msgid->_dns_list) == 0)
		{
			_int32 ret = set_erase_node(&g_udp_fd_msgid_set, fd_msgid);
			CHECK_VALUE(ret);
			mpool_free_slip(g_fd_msgid_pair_slab, fd_msgid);
		}
	}
	if(errcode == SUCCESS && op->_ip_count > 0)
	{
		SD_SOCKADDR addr;
		addr._sin_family = SD_AF_INET;
		addr._sin_addr = op->_ip_list[0];
		addr._sin_port = dns->_port;
		dns_common_cache_append(g_dns_common_cache, op->_host_name, op->_ip_list[0]);
		socket_proxy_sendto(dns->_sock, dns->_buffer, dns->_buffer_len, &addr, (socket_proxy_sendto_handler)dns->_callback_handler, dns->_user_data);
	}
	else
	{
        report_dns_query_result(errcode, op);
		callback = (socket_proxy_sendto_handler)dns->_callback_handler;
		if(errcode == MSG_CANCELLED)
			peek_operation_count_by_device_id(dns->_sock, DEVICE_SOCKET_UDP, &pending_op_count);
		callback((errcode == SUCCESS? -1 : errcode), pending_op_count, dns->_buffer, 0, dns->_user_data);
	}
	sd_free(op->_ip_list);
	op->_ip_list = NULL;
	return mpool_free_slip(g_proxy_sendto_dns_slab, dns);
}

_int32 socket_proxy_recvfrom(SOCKET sock, char* buffer, _u32 len, socket_proxy_recvfrom_handler callback_handler, void* user_data)
{
	_int32 ret;
	if(callback_handler == NULL || buffer == NULL || len == 0)
		return SP_INVALID_ARGUMENT;
	ret = speed_limit_add_recv_request(sock, DEVICE_SOCKET_UDP, buffer, len, (void*)callback_handler, user_data, FALSE);
	CHECK_VALUE(ret);
	speed_limit_update_request();
	return SUCCESS;
}

_int32 socket_proxy_recvfrom_impl(SOCKET_RECV_REQUEST* request)
{
	_int32 ret;
	OP_PARA_NOCONN_SOCKET_RW para;
	MSG_INFO msg_info = {};
	PROXY_DATA* data;
	socket_proxy_recvfrom_handler callback = (socket_proxy_recvfrom_handler)request->_callback;
	para._buffer = request->_buffer;
	para._expect_size = request->_len;
	para._operated_size = 0;
	msg_info._device_id = request->_sock;
	msg_info._device_type = request->_type;
	msg_info._msg_priority = MSG_PRIORITY_NORMAL;
	msg_info._operation_type = OP_RECVFROM;
	msg_info._operation_parameter = &para;
	ret = mpool_get_slip(g_proxy_data_slab, (void**)&data);
	if(ret != SUCCESS)
	{
		LOG_ERROR("socket_proxy_recvfrom_impl, mpool_get_slip failed, errcode = %d", ret);
		sd_assert(FALSE);
		return callback(ret, 0, request->_buffer, 0, NULL, request->_user_data);
	}
	data->_callback_handler = (void*)callback;
	data->_user_data = request->_user_data;
	msg_info._user_data = data;
	ret = post_message(&msg_info, socket_proxy_msg_notify, NOTICE_ONCE, WAIT_INFINITE, NULL);
	sd_assert(ret == SUCCESS);
	return ret;
}

_int32 socket_proxy_uncomplete_recv(SOCKET sock, char* buffer, _u32 len, socket_proxy_recv_handler callback_handler, void* user_data)
{
	_int32 ret = SUCCESS;
	_u32 sock_num = sock;
	SOCKET_ENCAP_PROP_S* p_encap_prop = NULL;
	ret = get_socket_encap_prop_by_sock(sock_num, &p_encap_prop);
	CHECK_VALUE(ret);
	if (p_encap_prop == NULL || p_encap_prop->_encap_type == SOCKET_ENCAP_TYPE_NONE)
	{
		ret = socket_proxy_recv_function(sock, buffer, len, callback_handler, user_data, TRUE);
	}
	else if (p_encap_prop->_encap_type == SOCKET_ENCAP_TYPE_HTTP_CLIENT)
	{
		ret = socket_encap_http_client_uncomplete_recv(sock, buffer, len, callback_handler, user_data);		
	}
	else
	{
		CHECK_VALUE(-1);
	}
	return ret;
}

static _int32 socket_proxy_msg_notify(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid)
{
	PROXY_DATA* data = (PROXY_DATA*)msg_info->_user_data;
	_u32 pending_op_count = 0;
	
	LOG_DEBUG("socket_proxy_msg_notify(id:%d), errcode = %d,_socket=%u,type=%d,_operation_type=%u,_user_data=0x%X", 
        msgid,errcode,msg_info->_device_id,msg_info->_device_type,msg_info->_operation_type,msg_info->_user_data);
	
	socket_proxy_peek_op_count(msg_info->_device_id,  msg_info->_device_type,  &pending_op_count);
	//sd_assert(data->_callback_handler != NULL);
	switch(msg_info->_operation_type)
	{
	case OP_CONNECT:
		{
			//LOG_ERROR("socket_proxy_msg_notify OP_CONNECT (id:%d), errcode = %d,_socket=%u,type=%d,_operation_type=%u,_user_data=0x%X", msgid,errcode,msg_info->_device_id,msg_info->_device_type,msg_info->_operation_type,msg_info->_user_data);
			socket_proxy_connect_handler callback = (socket_proxy_connect_handler)data->_callback_handler;
			if(errcode == SUCCESS)
			{	/*set local ip*/
				static SD_SOCKADDR addr;
				sd_getsockname(msg_info->_device_id, &addr);
				sd_set_local_ip(addr._sin_addr);
			}
			callback(errcode, pending_op_count, data->_user_data);
			break;
		}
	case OP_PROXY_CONNECT:
		{
			PROXY_CONNECT_DNS *para =(PROXY_CONNECT_DNS*)msg_info->_user_data;
			socket_proxy_connect_handler callback = (socket_proxy_connect_handler)para->_callback_handler;
			if(errcode == SUCCESS)
			{	/*set local ip*/
				static SD_SOCKADDR addr;
				sd_getsockname(msg_info->_device_id, &addr);
				sd_set_local_ip(addr._sin_addr);
			}
			//TCP_DEVICE* tcp = (TCP_DEVICE*)para->_user_data;
			//LOG_DEBUG("socket_proxy_msg_notify, socket_device = 0x%x,_socket=%u(%u),errcode=%d", tcp->_device,tcp->_socket,para->_sock,errcode);
			LOG_ERROR("socket_proxy_msg_notify,OP_PROXY_CONNECT sock = %u,port=%u.user_data=0x%X,connect_dns=0x%X", para->_sock,para->_port,para->_user_data,para);
			sd_assert(para->_sock==msg_info->_device_id);
			//sd_assert(tcp->_socket==msg_info->_device_id);
			callback(errcode, pending_op_count, para->_user_data);
			return mpool_free_slip(g_proxy_connect_dns_slab, para);
			break;
		}
	case OP_ACCEPT:
		{
			OP_PARA_ACCEPT* op = (OP_PARA_ACCEPT*)msg_info->_operation_parameter;	/*if errcode is not SUCCESS, op is NULL or not?*/
			socket_proxy_accept_handler callback = (socket_proxy_accept_handler)data->_callback_handler;
			sd_assert(callback!=NULL);
			callback(errcode, pending_op_count, op->_socket, data->_user_data); 
			break;
		}
	case OP_SEND:
		{
			OP_PARA_CONN_SOCKET_RW* op = (OP_PARA_CONN_SOCKET_RW*)msg_info->_operation_parameter;
			socket_proxy_send_handler callback = (socket_proxy_send_handler)data->_callback_handler;
			if(!callback)
			{
				/* Fatal Error! */
				sd_assert(FALSE);
				return SUCCESS;
			}
			callback(errcode, pending_op_count, op->_buffer, op->_operated_size, data->_user_data);
			break;
		}
	case OP_RECV:
		{
			OP_PARA_CONN_SOCKET_RW* op = (OP_PARA_CONN_SOCKET_RW*)msg_info->_operation_parameter;
			socket_proxy_recv_handler callback = (socket_proxy_recv_handler)data->_callback_handler;		
			if (errcode == SUCCESS && op->_operated_size == 0 && op->_expect_size > 0)
			{
				errcode = SOCKET_CLOSED;
			}
			callback(errcode, pending_op_count, op->_buffer, op->_operated_size, data->_user_data);
			break;
		}
	case OP_SENDTO:
		{
			OP_PARA_NOCONN_SOCKET_RW* op = (OP_PARA_NOCONN_SOCKET_RW*)msg_info->_operation_parameter;
			socket_proxy_sendto_handler callback = (socket_proxy_sendto_handler)data->_callback_handler;
			callback(errcode, pending_op_count, op->_buffer, op->_operated_size, data->_user_data);
			break;
		}
	case OP_RECVFROM:
		{
			OP_PARA_NOCONN_SOCKET_RW* op = (OP_PARA_NOCONN_SOCKET_RW*)msg_info->_operation_parameter;
			socket_proxy_recvfrom_handler callback = (socket_proxy_recvfrom_handler)data->_callback_handler;
			if (errcode == SUCCESS && op->_operated_size == 0 )
			{
				errcode = SOCKET_CLOSED;
			}
			callback(errcode, pending_op_count, op->_buffer, op->_operated_size, &op->_sockaddr, data->_user_data);
			break;
		}
	default:
		{
			LOG_ERROR("socket_proxy_msg_notify recv a unknown operation type...");
			sd_assert(FALSE);
		}
	}
	return mpool_free_slip(g_proxy_data_slab, data);
}


////////////////////////////////////////////////////////
_int32 socket_proxy_send_browser(SOCKET sock, const char* buffer, _u32 len, socket_proxy_send_handler callback_handler, void* user_data)
{
	OP_PARA_CONN_SOCKET_RW para;
	MSG_INFO msg_info = {};
	PROXY_DATA* data;
	para._buffer = (char*)buffer;
	para._expect_size = len;
	para._operated_size = 0;
	para._oneshot = FALSE;		/*complete read or write*/
	msg_info._device_id = sock;
	msg_info._device_type = DEVICE_SOCKET_TCP;
	msg_info._msg_priority = MSG_PRIORITY_NORMAL;
	msg_info._operation_type = OP_SEND;
	msg_info._operation_parameter = &para; 
	if(mpool_get_slip(g_proxy_data_slab, (void**)&data) != SUCCESS)
		return SP_OUT_OF_MEMORY;
	data->_callback_handler = (void*)callback_handler;
	data->_user_data = user_data;
	msg_info._user_data = data;
	return post_message(&msg_info, socket_proxy_msg_notify, NOTICE_ONCE, WAIT_INFINITE, NULL);
}

_int32 socket_proxy_recv_browser(SOCKET sock, char* buffer, _u32 len, socket_proxy_recv_handler callback_handler, void* user_data)
{
	_int32 ret = SUCCESS;
	OP_PARA_CONN_SOCKET_RW	para;
	MSG_INFO msg_info = {};
	PROXY_DATA* data;
	socket_proxy_recv_handler callback = (socket_proxy_recv_handler)callback_handler;
	para._buffer = buffer;
	para._expect_size = len;
	para._operated_size = 0;
	para._oneshot = FALSE;
	msg_info._device_id = sock;
	msg_info._device_type = DEVICE_SOCKET_TCP;
	msg_info._msg_priority = MSG_PRIORITY_NORMAL;
	msg_info._operation_type = OP_RECV;
	msg_info._operation_parameter = &para;
	ret = mpool_get_slip(g_proxy_data_slab, (void**)&data);
	if(ret != SUCCESS)
	{
		LOG_ERROR("socket_proxy_recv_impl, mpool_get_slip failed, errcode = %d", ret);
		return callback(ret, 0, buffer, 0, user_data);
	}
	data->_callback_handler = (void*)callback;
	data->_user_data = user_data;
	msg_info._user_data = data;
	ret = post_message(&msg_info, socket_proxy_msg_notify, NOTICE_ONCE, SOCKET_RECV_TIMEOUT, NULL);
	sd_assert(ret == SUCCESS);
	return ret;
}

_int32 socket_proxy_uncomplete_recv_browser(SOCKET sock, char* buffer, _u32 len, socket_proxy_recv_handler callback_handler, void* user_data)
{
	_int32 ret = SUCCESS;
	OP_PARA_CONN_SOCKET_RW	para;
	MSG_INFO msg_info = {};
	PROXY_DATA* data;
	socket_proxy_recv_handler callback = (socket_proxy_recv_handler)callback_handler;
	para._buffer = buffer;
	para._expect_size = len;
	para._operated_size = 0;
	para._oneshot = TRUE;
	msg_info._device_id = sock;
	msg_info._device_type = DEVICE_SOCKET_TCP;
	msg_info._msg_priority = MSG_PRIORITY_NORMAL;
	msg_info._operation_type = OP_RECV;
	msg_info._operation_parameter = &para;
	ret = mpool_get_slip(g_proxy_data_slab, (void**)&data);
	if(ret != SUCCESS)
	{
		LOG_ERROR("socket_proxy_recv_impl, mpool_get_slip failed, errcode = %d", ret);
		return callback(ret, 0, buffer, 0, user_data);
	}
	data->_callback_handler = (void*)callback;
	data->_user_data = user_data;
	msg_info._user_data = data;
	ret = post_message(&msg_info, socket_proxy_msg_notify, NOTICE_ONCE, SOCKET_UNCOMPLETE_RECV_TIMEOUT, NULL);
	sd_assert(ret == SUCCESS);
	return ret;
}

static _int32 dns_common_cache_init(DNS_COMMON_CACHE* dns_cache)
{
	sd_memset(dns_cache, 0, sizeof(DNS_COMMON_CACHE) * DNS_COMMON_CACHE_SIZE);
	dns_cache[0]._host_name = DEFAULT_LICENSE_HOST_NAME;
	dns_cache[1]._host_name = DEFAULT_SHUB_HOST_NAME;
	dns_cache[2]._host_name = DEFAULT_PHUB_HOST_NAME;
	dns_cache[3]._host_name = DEFAULT_TRCKER_HOST_NAME;
	dns_cache[4]._host_name = DEFAULT_PARTERN_HUB_HOST_NAME;
	dns_cache[5]._host_name = DEFAULT_KANKAN_CDN_MANAGER_HOST_NAME;
	dns_cache[6]._host_name = DEFAULT_BT_HUB_HOST_NAME;
	dns_cache[7]._host_name = DEFAULT_EMULE_HUB_HOST_NAME;
	dns_cache[8]._host_name = DEFAULT_STAT_HUB_HOST_NAME;
	dns_cache[9]._host_name = DEFAULT_EMB_HUB_HOST_NAME;
	dns_cache[10]._host_name = DEFAULT_NET_SERVER_HOST_NAME;
	dns_cache[11]._host_name = DEFAULT_PING_SERVER_HOST_NAME;
	dns_cache[12]._host_name = DEFAULT_MOVIE_SERVER_HOST_NAME;
	dns_cache[13]._host_name = DEFAULT_MAIN_MEMBER_SERVER_HOST_NAME;
	dns_cache[14]._host_name = DEFAULT_PORTAL_MEMBER_SERVER_HOST_NAME;
	dns_cache[15]._host_name = DEFAULT_MAIN_VIP_SERVER_HOST_NAME;
	dns_cache[16]._host_name = DEFAULT_PORTAL_VIP_SERVER_HOST_NAME;
	dns_cache[17]._host_name = DEFAULT_WALKBOX_HOST_NAME;
	return SUCCESS;
}

static _int32 dns_common_cache_query(const DNS_COMMON_CACHE* dns_cache, const char* host_name, _u32* ip)
{
	_u32 index;
	_u32 current_time;
	sd_time(&current_time);
	for(index = 0; index < DNS_COMMON_CACHE_SIZE; index++)
	{
		if(0 == sd_strcmp(dns_cache[index]._host_name, host_name))
		{
			if(0 != dns_cache[index]._ip)
			{
				if(current_time - dns_cache[index]._start_time < MAX_DNS_VALID_TIME)
				{	
					*ip = dns_cache[index]._ip;
					return SUCCESS;
				}
				else
				{
					LOG_DEBUG("dns_common_cache_query, outdated dns record! host: %s, current time: %u, dns record time: %u", 
						host_name, current_time, dns_cache[index]._start_time);
					return -1;
				}
			}
			else
			{
				LOG_DEBUG("dns_common_cache_query, query failed! the ip address of %s is empty", host_name);
				return -1;
			}

		}
	}
	 /* not found. */
	LOG_DEBUG("dns_common_cache_query, query failed! %s is not a common dns host", host_name);
	return -1;
	 
}

// TODO 有些查询回来的域名会每次都解析，可以优化！
static _int32 dns_common_cache_append(DNS_COMMON_CACHE* dns_cache, const char* host_name, const _u32 ip)
{
	_u32 index;
	_u32 current_time;

	for(index = 0; index < DNS_COMMON_CACHE_SIZE; index++)
	{
		if(0 == sd_strcmp(dns_cache[index]._host_name, host_name))
		{
            LOG_DEBUG("dns_common_cache_append, host_name(%s)", host_name);
			sd_time(&current_time); 
			dns_cache[index]._ip = ip;
			dns_cache[index]._start_time = current_time;
		}
	}
	return SUCCESS;	
}

_int32 dns_hash_comp(void *E1, void *E2)
{
	_u32 hash1 = 0,hash2 = 0;
	
	hash1 = (_u32)E1;
	hash2 = (_u32)E2;
	
	if(hash1>hash2) return 1;
	else if(hash1<hash2) return -1;
	else return 0;
}

_int32 dns_add_domain_ip(const char * host_name, const char *ip_str)
{
	_int32 ret_val = SUCCESS;
	_u32 hash_key = 0;
	DNS_DOMAIN* p_domain = NULL;

	if(host_name == NULL || ip_str == NULL) return -1;
	LOG_DEBUG("dns_get_domain_ip:host_name:%s, ip:%s", host_name, ip_str); 
	ret_val = sd_get_url_hash_value((char*)host_name, sd_strlen(host_name), &hash_key);
	CHECK_VALUE(ret_val);
	
	p_domain= dns_get_domain_from_map(hash_key);
	if(p_domain == NULL)
	{
		ret_val = sd_malloc(sizeof(DNS_DOMAIN), (void**)&p_domain);
		CHECK_VALUE(ret_val);
		sd_memset(p_domain, 0x00, sizeof(DNS_DOMAIN));
		p_domain->_hash = hash_key;
		sd_assert(sd_strlen(host_name) <= MAX_HOST_NAME_LEN);
		sd_strncpy(p_domain->_host_name, host_name, sd_strlen(host_name));
		sd_inet_aton(ip_str, &(p_domain->_ip[0]));
		p_domain->_ip_count = 1;
		sd_assert(p_domain->_ip[0] != 0);
		ret_val = dns_add_domain_to_map(p_domain);
		sd_assert(ret_val == 0);
	}
	else
	{
		if(p_domain->_ip_count == DNS_DOMAIN_MAX_IP_NUM)
		{
			return ret_val;
		}
		else
		{
			p_domain->_ip[p_domain->_ip_count] = sd_inet_addr(ip_str);
			sd_assert(p_domain->_ip[p_domain->_ip_count] != 0);
			p_domain->_ip_count++;
		}
		
		/*while((count < DNS_DOMAIN_MAX_IP_NUM))
		{
			if(p_domain->_ip[count] == sd_inet_addr(ip_str))
			{
				break;
			}
			else if(p_domain->_ip[count] == 0)
			{
				p_domain->_ip_count++;
				p_domain->_ip[count] = sd_inet_addr(ip_str);
				sd_assert(p_domain->_ip[count] != 0);
				break;
			}
			count++;
		}*/
	}
	return ret_val;
}

_int32 dns_get_domain_ip(const char* host_name, _u32* ip)
{
	_int32 ret_val = SUCCESS;
	_u32 hash_key = 0;
	DNS_DOMAIN* p_domain = NULL;
	_int32 count = 0;
	_u32 seed = 0;

	if(host_name == NULL || ip == NULL) return -1;
	LOG_DEBUG("dns_get_domain_ip:host_name:%s", host_name); 
	if(dns_get_domain_num() == 0)
	{
		return -1;
	}
	ret_val = sd_get_url_hash_value((char*)host_name, sd_strlen(host_name), &hash_key);
	CHECK_VALUE(ret_val);
	
	p_domain= dns_get_domain_from_map(hash_key);
	if(p_domain == NULL)
	{
		return -1;
	}
	else
	{
		//随机挑选一个IP
		sd_time(&seed);
		sd_srand(seed);
		count = sd_rand()%p_domain->_ip_count;
		sd_assert(p_domain->_ip[count] != 0);
		*ip = p_domain->_ip[count];
	}
	return ret_val;
}

_u32 dns_get_domain_num(void)
{
	return map_size(&g_dns_domain_map);
}

DNS_DOMAIN *dns_get_domain_from_map(_u32 hash)
{
	_int32 ret_val = SUCCESS;
	DNS_DOMAIN * p_domain = NULL;

	LOG_DEBUG("dns_get_domain_from_map:hash=%u",hash); 
	ret_val = map_find_node(&g_dns_domain_map, (void *)hash, (void **)&p_domain);
	//sd_assert(ret_val==SUCCESS);
	return p_domain;
}
_int32 dns_add_domain_to_map(DNS_DOMAIN* p_domain)
{
	_int32 ret_val = SUCCESS;
	PAIR info_map_node;
      	LOG_DEBUG("dns_add_domain_to_map:host hash =%u", p_domain->_hash); 
	info_map_node._key =(void*)p_domain->_hash;
	info_map_node._value = (void*)p_domain;
	ret_val = map_insert_node( &g_dns_domain_map, &info_map_node );
	CHECK_VALUE(ret_val);

	return ret_val;
}

_int32 dns_clear_domain_map(void)
{
	MAP_ITERATOR cur_node = NULL;
	DNS_DOMAIN* p_domain;
	LOG_DEBUG("dns_clear_domain_map:%u",map_size(&g_dns_domain_map)); 
	  
	cur_node = MAP_BEGIN(g_dns_domain_map);
	while(cur_node != MAP_END(g_dns_domain_map))
	{
		p_domain = (DNS_DOMAIN*)MAP_VALUE(cur_node);
		SAFE_DELETE(p_domain);
		map_erase_iterator(&g_dns_domain_map, cur_node);
		cur_node = MAP_BEGIN(g_dns_domain_map);
	}

	return SUCCESS;
}

void socket_proxy_set_speed_limit(_u32 download_limit_speed, _u32 upload_limit_speed)
{
    sl_set_speed_limit(download_limit_speed, upload_limit_speed);
}

void socket_proxy_get_speed_limit(_u32* download_limit_speed, _u32* upload_limit_speed)
{
    sl_get_speed_limit(download_limit_speed, upload_limit_speed);
}

_u32 socket_proxy_get_p2s_recv_block_size(void)
{
    return sl_get_p2s_recv_block_size();
}

_u32 socket_proxy_speed_limit_get_download_speed(void)
{
    return speed_limit_get_download_speed();
}

_u32 socket_proxy_speed_limit_get_max_download_speed(void)
{
    return speed_limit_get_max_download_speed();
}

_u32 socket_proxy_speed_limit_get_upload_speed(void)
{
    return speed_limit_get_upload_speed();
}

