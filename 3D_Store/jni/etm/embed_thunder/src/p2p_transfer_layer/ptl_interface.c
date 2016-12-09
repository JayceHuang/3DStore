#include "utility/local_ip.h"
#include "utility/settings.h"
#include "utility/mempool.h"
#include "utility/peer_capability.h"
#include "utility/logid.h"
#define  LOGID  LOGID_P2P_TRANSFER_LAYER
#include "utility/logger.h"
#include "platform/sd_network.h"

#include "ptl_interface.h"

#include "ptl_cmd_define.h"
#include "ptl_passive_connect.h"
#include "ptl_cmd_sender.h"
#include "ptl_udp_device.h"
#include "ptl_get_peersn.h"
#include "ptl_memory_slab.h"
#include "ptl_intranet_manager.h"
#include "ptl_active_punch_hole.h"
#include "ptl_passive_punch_hole.h"
#include "ptl_active_udp_broker.h"
#include "ptl_passive_udp_broker.h"
#include "ptl_active_tcp_broker.h"
#include "ptl_passive_tcp_broker.h"
#include "udt/udt_device_manager.h"
#include "udt/udt_interface.h"
#include "udt/udt_utility.h"
#include "tcp/tcp_interface.h"
#include "tcp/tcp_device.h"

static _u32 g_ping_timeout_id = INVALID_MSG_ID;

_int32 init_ptl_modular(void)
{
#if 1
	return 0;
#else
    LOG_DEBUG("init_ptl_modular");
    
    ptl_init_memory_slab();
    ptl_init_cmd_sender();
    
    _int32 ret = SUCCESS;
#ifdef MOBILE_PHONE
    if (NT_GPRS_WAP & sd_get_net_type())
    {
        ret = init_tcp_modular();
        CHECK_VALUE(ret);
        return ret;
    }
#endif /* MOBILE_PHONE */

    /*create udp device and try to receive data*/
    ret = ptl_create_udp_device();
    if (ret != SUCCESS)
    {
        return ret;
    }
    ptl_udp_recvfrom();
    ptl_init_get_peersn();

    ret = init_tcp_modular();
    CHECK_VALUE(ret);
    ret = init_udt_modular();
    CHECK_VALUE(ret);

    ret = ptl_start_intranet_manager();
    CHECK_VALUE(ret);

    ret = ptl_init_active_punch_hole();
    CHECK_VALUE(ret);
    ret = ptl_init_passive_punch_hole();
    CHECK_VALUE(ret);
    ret = ptl_init_passive_tcp_broker();
    CHECK_VALUE(ret);
    ret = ptl_init_passive_udp_broker();
    CHECK_VALUE(ret);
    ret = ptl_init_active_tcp_broker();
    CHECK_VALUE(ret);
    ret = ptl_init_active_udp_broker();
    CHECK_VALUE(ret);
    ret = ptl_create_passive_connect();
    CHECK_VALUE(ret);
    return ret;
#endif
}

_int32 uninit_ptl_modular(void)
{
#if 1
		return 0;
#else
#ifdef MOBILE_PHONE
    if(NT_GPRS_WAP & sd_get_net_type())
    {
        uninit_tcp_modular();
        return ptl_uninit_memory_slab();
    }
#endif /* MOBILE_PHONE */

    ptl_close_passive_connect();
    ptl_uninit_passive_udp_broker();
    ptl_uninit_passive_tcp_broker();
    ptl_uninit_passive_punch_hole();
    ptl_uninit_active_punch_hole();
    ptl_uninit_active_tcp_broker();
    ptl_uninit_active_udp_broker();

    ptl_exit();

    uninit_udt_modular();
    uninit_tcp_modular();
    ptl_uninit_get_peersn();
    if (g_ping_timeout_id != INVALID_MSG_ID)
    {
        cancel_timer(g_ping_timeout_id);
        g_ping_timeout_id = INVALID_MSG_ID;
    }
    ptl_close_udp_device();

    return ptl_uninit_memory_slab();
#endif
}

_int32 ptl_exit(void)
{
#if 1
		return 0;
#else
    _int32 ret = ptl_send_logout_cmd();
    CHECK_VALUE(ret);
    ret =  ptl_stop_intranet_manager();
    return ret;
#endif
}

_int32 ptl_start_ping()
{
#if 1
		return 0;
#else
    LOG_DEBUG("ptl_start_upload enter");
    _int32 ret = ptl_send_ping_cmd();
    CHECK_VALUE(ret);
    if (g_ping_timeout_id == INVALID_MSG_ID)
    {
        _u32 ping_timeout = 300 * 1000;
        settings_get_int_item("ptl_setting.ping_timeout", (_int32*)&ping_timeout);
        ret = start_timer(ptl_handle_ping_timeout,
            NOTICE_INFINITE, 
            ping_timeout, 
            0, 
            NULL,
            &g_ping_timeout_id);
        CHECK_VALUE(ret);
    }
    return ret;
#endif
}

_int32 ptl_stop_ping()
{
#if 1
		return 0;
#else
    _int32 ret = ptl_send_logout_cmd();
    CHECK_VALUE(ret);  
    if (g_ping_timeout_id != INVALID_MSG_ID)
    {
        cancel_timer(g_ping_timeout_id);
        g_ping_timeout_id = INVALID_MSG_ID;
    }
    return ret;
#endif
}

_int32 ptl_create_device(PTL_DEVICE** device, void* user_data, void** table)
{
#if 1
		return 0;
#else
    *device = NULL;
    _int32 ret = sd_malloc(sizeof(PTL_DEVICE), (void**)device);
    if (ret != SUCCESS)
    {
        return ret;
    }
    
    (*device)->_type = UNKNOWN_TYPE;
    (*device)->_device = NULL;
    (*device)->_user_data = user_data;
    (*device)->_table = table;
    return ret;
#endif
}

//目前调用这个如果是SUCCESS，那么可以直接destroy device，否则需要等待notify_close，再destroy
_int32 ptl_close_device(PTL_DEVICE* device)
{
#if 1
		return 0;
#else
    _int32 ret = SUCCESS;
    TCP_DEVICE* tcp = NULL;
    UDT_DEVICE* udt = NULL;
    switch (device->_type)
    {
        case UNKNOWN_TYPE:
        {
            //说明设备还在连接中
            if(device->_connect_type == ACTIVE_TCP_BROKER_CONN)
                return ptl_cancel_active_tcp_broker_req(device);
            if(device->_connect_type == ACTIVE_UDP_BROKER_CONN)
                return ptl_cancel_active_udp_broker_req(device);
            break;
        }
        case TCP_TYPE:
        {
            tcp = (TCP_DEVICE*)device->_device;
            ret = tcp_device_close(tcp);
            break;
        }
        case UDT_TYPE:
        {
            udt = (UDT_DEVICE*)device->_device;
            ret = udt_device_close(udt);
            sd_assert(ret == SUCCESS);
            break;
        }
        default:
        {
            sd_assert(FALSE);
        }
    }
    return ret;
#endif
}

_int32 ptl_destroy_device(PTL_DEVICE* device)
{
#if 1
		return 0;
#else
    _int32 ret_val = sd_free(device);
    device = NULL;
    return ret_val;
#endif
}

_int32 force_close_ptl_modular_res(void)
{
#if 1
		return 0;
#else
    force_close_p2p_acceptor_socket();
    return ptl_force_close_udp_device_socket();
#endif
}

_int32 ptl_malloc_cmd_buffer(char** cmd_buffer, _u32* len, _u32* offset, PTL_DEVICE* device)
{
#if 1
		return 0;
#else
    *cmd_buffer = NULL;
    if (device->_type == TCP_TYPE)
    {
        *offset = 0;
    }
    else if (device->_type == UDT_TYPE)
    {
        *offset = UDT_NEW_HEADER_SIZE;
        *len = *len + UDT_NEW_HEADER_SIZE;
    }
    else
    {
        sd_assert(FALSE);
    }
    return sd_malloc(*len, (void**)cmd_buffer);
#endif
}

_int32 ptl_free_cmd_buffer(char* cmd_buffer)
{
#if 1
		return 0;
#else
    _int32 ret_val = sd_free(cmd_buffer);
    cmd_buffer = NULL;
    return ret_val;
#endif
}

PTL_CONNECT_TYPE ptl_get_connect_type(_u32 peer_capability)
{
#if 1
		return 0;
#else
    _u32 local_ip = sd_get_local_ip();
    LOG_DEBUG("peer_capability=%u, local_ip=%u", peer_capability, local_ip);
    
    BOOL local_in_nated, peer_in_nated;
    PTL_CONNECT_TYPE connect_type = INVALID_CONN;
    /*remote peer in nat*/
    if (is_same_nat(peer_capability))
    {
        connect_type = SAME_NAT_CONN;
    }
    else if (is_cdn(peer_capability))
    {
        connect_type = ACTIVE_TCP_CONN;
    }
    else
    {
        local_in_nated = sd_is_in_nat(local_ip);
        peer_in_nated = is_nated(peer_capability);
        if (local_in_nated)
        {
            if (peer_in_nated)
            {
                if (is_support_new_udt(peer_capability))
                {
                    connect_type =  ACTIVE_PUNCH_HOLE_CONN;     //对端内网
                }
                else
                {
                    connect_type =  INVALID_CONN;
                }
            }
            else
            {
                connect_type =  ACTIVE_TCP_CONN;        //对端外网
            }
        }
        else
        {
            if (peer_in_nated)
            {
                if (is_support_new_udt(peer_capability))
                {
                    connect_type =  ACTIVE_UDP_BROKER_CONN; //对端内网
                }
                else
                {
                    connect_type =  ACTIVE_TCP_BROKER_CONN;
                }
            }
            else
            {
                if (is_support_new_udt(peer_capability))
                {
                    connect_type =  ACTIVE_UDT_CONN;        //对端外网
                }
                else
                {
                    connect_type =  ACTIVE_TCP_CONN;
                }
            }
        }
    }

#ifdef MOBILE_PHONE
    if (NT_GPRS_WAP & sd_get_net_type())
    {
        if ((connect_type > ACTIVE_TCP_CONN) && (connect_type < PASSIVE_TCP_BROKER_CONN))
        {
            /* No UDP connecting in WAP! */
            connect_type = INVALID_CONN;
        }
    }
#endif /* MOBILE_PHONE */
    LOG_DEBUG("connect_type=%d", connect_type);

    return connect_type;
#endif
}

_int32 ptl_connect(PTL_DEVICE* device, _u32 peer_capability, const _u8* gcid, _u64 file_size, const char* peerid, _u32 ip, _u16 tcp_port, _u16 udp_port)
{
#if 1
		return 0;
#else
    _int32 ret = -1;
    PTL_CONNECT_TYPE connect_type = ptl_get_connect_type(peer_capability);
    device->_connect_type = connect_type;
    switch (connect_type)
    {
        case INVALID_CONN:
            return -1;
        case SAME_NAT_CONN:
        case ACTIVE_TCP_CONN:
        {
            TCP_DEVICE* tcp = NULL;
            ret = tcp_device_create(&tcp, INVALID_SOCKET, device);
            if (ret != SUCCESS)
            {
                return ret;
            }
            device->_type = TCP_TYPE;
            device->_device = tcp;
            return tcp_device_connect(tcp, ip, tcp_port);
        }
        case ACTIVE_UDT_CONN:
        {
            UDT_DEVICE* udt = NULL;
            ret = udt_device_create(&udt, udt_generate_source_port(), 0, udt_hash_peerid(peerid), device);
            if (ret != SUCCESS)
            {
                return ret;
            }
            udt_add_device(udt);
            device->_type = UDT_TYPE;
            device->_device = udt;
            return udt_device_connect(udt, ip, udp_port);
        }
        case ACTIVE_PUNCH_HOLE_CONN:
        {
            UDT_DEVICE* udt = NULL;
            ret = udt_device_create(&udt, udt_generate_source_port(), 0, udt_hash_peerid(peerid), device);
            if (ret != SUCCESS) 
            {
                return ret;
            }
            udt_add_device(udt);
            device->_type = UDT_TYPE;
            device->_device = udt;
            return ptl_active_punch_hole(&udt->_id, peerid);
        }
        case ACTIVE_UDP_BROKER_CONN:
        {
            device->_connect_type = ACTIVE_UDP_BROKER_CONN;
            return ptl_active_udp_broker(device, peerid);
        }
        case ACTIVE_TCP_BROKER_CONN:
        {
            device->_connect_type = ACTIVE_TCP_BROKER_CONN;
            return ptl_active_tcp_broker(device, peerid);
        }
        default:
        {
            sd_assert(FALSE);
        }
    }
    return -1;
#endif
}

_int32 ptl_recv_cmd(PTL_DEVICE* device, char* buffer, _u32 expect_len, _u32 buffer_len)
{
#if 1
		return 0;
#else
    sd_assert(expect_len <= buffer_len);
    switch (device->_type)
    {
        case TCP_TYPE:
        {
            TCP_DEVICE* tcp = (TCP_DEVICE*)device->_device;
            return tcp_device_recv_cmd(tcp, buffer, expect_len);
        }
        case UDT_TYPE:
        {
            UDT_DEVICE* udt = (UDT_DEVICE*)device->_device;
            return udt_device_recv(udt, RECV_CMD_TYPE, buffer, expect_len, buffer_len);
        }
        default:
        {
            sd_assert(FALSE);
            return -1;
        }
    }
#endif
}

_int32 ptl_recv_data(PTL_DEVICE* device, char* buffer, _u32 expect_len, _u32 buffer_len)
{
#if 1
		return 0;
#else
    switch(device->_type)
    {
        case TCP_TYPE:
        {
            TCP_DEVICE* tcp = (TCP_DEVICE*)device->_device;
            return tcp_device_recv_data(tcp, buffer, expect_len);
        }
        case UDT_TYPE:
        {
            UDT_DEVICE* udt = (UDT_DEVICE*)device->_device;
            return udt_device_recv(udt, RECV_DATA_TYPE, buffer, expect_len, buffer_len);
        }
        default:
        {
            sd_assert(FALSE);
            return -1;
        }
    }
#endif
}

_int32 ptl_recv_discard_data(PTL_DEVICE* device, char* buffer, _u32 expect_len, _u32 buffer_len)
{
#if 1
		return 0;
#else
    TCP_DEVICE* tcp = NULL;
    UDT_DEVICE* udt = NULL;
    switch(device->_type)
    {
        case TCP_TYPE:
        {
            tcp = (TCP_DEVICE*)device->_device;
            return tcp_device_recv_discard_data(tcp, buffer, expect_len);
        }
        case UDT_TYPE:
        {
            udt = (UDT_DEVICE*)device->_device;
            return udt_device_recv(udt, RECV_DISCARD_DATA_TYPE, buffer, expect_len, buffer_len);
        }
        default:
        {
            sd_assert(FALSE);
            return -1;
        }
    }
#endif
}

_int32 ptl_send_in_speed_limit(PTL_DEVICE* device, char* buffer, _u32 len)
{
#if 1
		return 0;
#else
    TCP_DEVICE* tcp = NULL;
    UDT_DEVICE* udt = NULL;
    switch(device->_type)
    {
        case TCP_TYPE:
        {
            tcp = (TCP_DEVICE*)device->_device;
            return tcp_device_send_in_speed_limit(tcp, buffer, len);
        }
        case UDT_TYPE:
        {
            udt = (UDT_DEVICE*)device->_device;
            return udt_device_send(udt, buffer, len, TRUE);
        }
        default:
        {
            sd_assert(FALSE);
            return -1;
        }
    }    
#endif
}

_int32 ptl_send(PTL_DEVICE* device, char* buffer, _u32 len)
{
#if 1
		return 0;
#else
    TCP_DEVICE* tcp = NULL;
    UDT_DEVICE* udt = NULL;
    switch(device->_type)
    {
        case TCP_TYPE:
        {
            tcp = (TCP_DEVICE*)device->_device;
            return tcp_device_send(tcp, buffer, len);
        }
        case UDT_TYPE:
        {
            udt = (UDT_DEVICE*)device->_device;
            return udt_device_send(udt, buffer, len, FALSE);
        }
        default:
        {
            sd_assert(FALSE);
            return -1;
        }
    }
#endif
}

_int32 ptl_handle_ping_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid)
{
#if 1
		return 0;
#else
    if (errcode == MSG_CANCELLED)
    {
        return SUCCESS;
    }
    return ptl_send_ping_cmd(); /*发送ping包*/
#endif
}
