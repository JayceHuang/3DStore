#include "ptl_active_punch_hole.h"
#include "ptl_cmd_define.h"
#include "ptl_get_peersn.h"
#include "ptl_callback_func.h"
#include "ptl_cmd_sender.h"
#include "udt/udt_utility.h"
#include "udt/udt_device.h"
#include "udt/udt_device_manager.h"
#include "udt/udt_cmd_sender.h"
#include "udt/udt_interface.h"
#include "utility/mempool.h"
#include "utility/map.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_P2P_TRANSFER_LAYER
#include "utility/logger.h"

typedef enum tagPTL_CONNECT_STATE 
{
	GET_PEERSN_STATE = 0,
	ICALLSOMEONE_STATE
}PTL_CONNECT_STATE;

typedef struct tagACTIVE_PUNCH_HOLE_DATA
{
	CONN_ID _id;
	char _remote_peerid[PEER_ID_SIZE + 1];
	_u32 _retry_times;
	_u32 _timeout_id;
	_u32 _remote_ip;
	_u16 _remote_port;
	_u16 _latest_ex_port;
	_u16 _guess_ex_port;
	PTL_CONNECT_STATE _state;
}ACTIVE_PUNCH_HOLE_DATA;

#define	PTL_ICALLSOMEONE_TIMEOUT		(10000)			/*10��û����Ӧ��ʱ*/
#define	PTL_ICALLSOMEONE_MAX_RETRY	(3)
#define	PTL_SYN_TIMEOUT				(4000)
#define	PTL_SYN_MAX_RETRY				(10)

static SET g_active_punch_hole_data_set;

_int32 ptl_init_active_punch_hole()
{
	LOG_DEBUG("ptl_init_active_punch_hole.");
	set_init(&g_active_punch_hole_data_set, conn_id_comparator);
	return SUCCESS;
}

_int32 ptl_uninit_active_punch_hole()
{
	ACTIVE_PUNCH_HOLE_DATA* data = NULL;
	SET_ITERATOR cur_iter, next_iter; 
	LOG_DEBUG("ptl_uninit_active_punch_hole.");
	cur_iter = SET_BEGIN(g_active_punch_hole_data_set);
	while (cur_iter != SET_END(g_active_punch_hole_data_set))
	{
		next_iter = SET_NEXT(g_active_punch_hole_data_set, cur_iter);
		data = (ACTIVE_PUNCH_HOLE_DATA*)SET_DATA(cur_iter);
		ptl_erase_active_punch_hole_data(data);
		cur_iter = next_iter;
		data = NULL;
	}
	return SUCCESS;
}

_int32 ptl_active_punch_hole(CONN_ID* id, const char* peerid)
{
	_int32 ret = SUCCESS;
	ACTIVE_PUNCH_HOLE_DATA* data = NULL;
	ret = sd_malloc(sizeof(ACTIVE_PUNCH_HOLE_DATA), (void**)&data);
	CHECK_VALUE(ret);
	sd_memset(data, 0, sizeof(ACTIVE_PUNCH_HOLE_DATA));
	data->_id = *id;
	data->_timeout_id = INVALID_MSG_ID;
	sd_memcpy(data->_remote_peerid, peerid, PEER_ID_SIZE);
	ret = set_insert_node(&g_active_punch_hole_data_set, data);
	CHECK_VALUE(ret);
	data->_state = GET_PEERSN_STATE;
	return ptl_get_peersn(peerid, ptl_active_punch_hole_get_peersn_callback, data);
}

_int32 ptl_active_punch_hole_get_peersn_callback(_int32 errcode, _u32 sn_ip, _u16 sn_port, void* user_data)
{
	_int32 ret = SUCCESS;
	ACTIVE_PUNCH_HOLE_DATA* data = (ACTIVE_PUNCH_HOLE_DATA*)user_data;
	UDT_DEVICE*	udt = NULL;
	if(errcode == MSG_CANCELLED)
		return SUCCESS;
	udt = udt_find_device(&data->_id);
	if(udt == NULL)
	{	//�豸�Ѿ������ڣ�ֱ��ɾ��punch_hole����
		LOG_DEBUG("ptl_active_punch_hole_get_peersn_callback, can't find udt_device[%u, %u, %u].",
					(_u32)data->_id._virtual_source_port, (_u32)data->_id._virtual_target_port, data->_id._peerid_hashcode);
		return ptl_erase_active_punch_hole_data(data);
	}
	if(errcode != SUCCESS)
	{
		ptl_connect_callback(errcode, udt->_device);
		return ptl_erase_active_punch_hole_data(data);
	}
	data->_remote_ip = sn_ip;
	data->_remote_port = sn_port;
	data->_state = ICALLSOMEONE_STATE;
	ret = ptl_send_icallsomeone_cmd(data->_remote_peerid, data->_id._virtual_source_port, sn_ip, sn_port);
	CHECK_VALUE(ret);
	return start_timer(ptl_handle_icallsomeone_timeout, NOTICE_INFINITE, PTL_ICALLSOMEONE_TIMEOUT, 0, data, &data->_timeout_id);
}

_int32 ptl_recv_icallsomeone_resp_cmd(ICALLSOMEONE_RESP_CMD* cmd)
{
	_int32 ret = SUCCESS;
	UDT_DEVICE* udt = NULL;
	ACTIVE_PUNCH_HOLE_DATA* data = NULL;
	SET_ITERATOR cur_iter = NULL, next_iter = NULL;
	CONN_ID id;
	LOG_DEBUG("ptl_recv_icallsomeone_resp_cmd, remote_peerid = %s, is_on_line = %d, virtual_port = %d.", cmd->_remote_peerid, cmd->_is_on_line, cmd->_virtual_port);	
	//����ICALLSOMEONE_RESP����������Է�������ʱ��ֻ��remote_peerid�ֶ���Ч��
	//virtual_port�ֶ�Ϊ0�����Բ��ܸ���virtual_port�ֶ���ɵ�conn_id������udt�豸��
	//�������remote_peerid�����Ҷ�Ӧ������п�����������ȥ����ͬ
	//һ��remote_peer�����ܴ���remote_peer��ͬ����������PTL_CONNECT_STATE
	//״̬�������֡�
	if(cmd->_is_on_line == 0)
	{	
		cur_iter = SET_BEGIN(g_active_punch_hole_data_set);
		while(cur_iter != SET_END(g_active_punch_hole_data_set))
		{
			next_iter = SET_NEXT(g_active_punch_hole_data_set, cur_iter);
			data = (ACTIVE_PUNCH_HOLE_DATA*)cur_iter->_data;
			//remote_peeridֻ�ܱȽ�ǰ��12λ�������λ���ܲ�ͬ��ԭ���������λ���ܱ���Ϊȫ0
			if(sd_strncmp(cmd->_remote_peerid, data->_remote_peerid, 12) == 0 && data->_state == ICALLSOMEONE_STATE)
			{
				udt = udt_find_device(&data->_id);
				if(udt != NULL)
				{
					LOG_DEBUG("[udt = 0x%x, device = 0x%x]icallsomeone response, but remote peer not on_line, notify connect failed and erase active_punch_hole_data, conn_id[%u, %u, %u].",
							udt, udt->_device, (_u32)udt->_id._virtual_source_port, (_u32)udt->_id._virtual_target_port, udt->_id._peerid_hashcode);
					ptl_connect_callback(ERR_PTL_PEER_OFFLINE, udt->_device);
				}
				ptl_erase_active_punch_hole_data(data);
			}
			cur_iter = next_iter;
		}
		return SUCCESS;
	}
	id._virtual_source_port = cmd->_virtual_port;
	id._virtual_target_port = 0;
	id._peerid_hashcode = udt_hash_peerid(cmd->_remote_peerid);
	set_find_node(&g_active_punch_hole_data_set, &id, (void**)&data);
	if(data == NULL)
	{
		LOG_DEBUG("ptl_recv_icallsomeone_resp_cmd, but ACTIVE_PUNCH_HOLE_DATA can't find, conn_id[%u, %u, %u].",
					(_u32)id._virtual_source_port, (_u32)id._virtual_target_port, id._peerid_hashcode);
		return SUCCESS;			//�Ѿ���������
	}
	udt = udt_find_device(&id);
	if(udt == NULL)
	{
		return ptl_erase_active_punch_hole_data(data);	//�豸�Ѿ�������
	}
	/*icallsomeone�ɹ�����ʼ��*/
	//ȡ��icallsomeone��ʱ������ʼ����syn��	
	ret = cancel_timer(data->_timeout_id);
	CHECK_VALUE(ret);
	data->_timeout_id = INVALID_MSG_ID;
	data->_retry_times = 0;
	data->_remote_ip = cmd->_remote_ip;
	data->_remote_port = cmd->_remote_port;
	data->_latest_ex_port = cmd->_latest_ex_port;
	data->_guess_ex_port = cmd->_guess_ex_port;
	LOG_DEBUG("[udt = 0x%x, device = 0x%x]ptl_recv_icallsomeone_resp_cmd, udt_send_syn_cmd, remote_ip = %u, reomte_port = %u.", udt, udt->_device, data->_remote_ip, data->_remote_port);
	ret = udt_send_syn_cmd(FALSE, udt->_next_send_seq, udt->_next_recv_seq, udt->_recv_window, udt->_id._virtual_source_port,
							udt->_id._virtual_target_port, data->_remote_ip, data->_remote_port);
	CHECK_VALUE(ret);
	if(data->_latest_ex_port != 0 && data->_latest_ex_port != data->_remote_port)
	{		
		LOG_DEBUG("[udt = 0x%x, device = 0x%x]ptl_recv_icallsomeone_resp_cmd, udt_send_syn_cmd, remote_ip = %u, reomte_port = %u.", udt, udt->_device, data->_remote_ip, data->_latest_ex_port);
		ret = udt_send_syn_cmd(FALSE, udt->_next_send_seq, udt->_next_recv_seq, udt->_recv_window, udt->_id._virtual_source_port,
								udt->_id._virtual_target_port, data->_remote_ip, data->_latest_ex_port);
		CHECK_VALUE(ret);
	}
	if(data->_guess_ex_port != 0 && data->_guess_ex_port != data->_remote_port && data->_guess_ex_port != data->_latest_ex_port)
	{		
		LOG_DEBUG("[udt = 0x%x, device = 0x%x]ptl_recv_icallsomeone_resp_cmd, udt_send_syn_cmd, remote_ip = %u, reomte_port = %u.", udt, udt->_device, data->_remote_ip, data->_guess_ex_port);
		ret = udt_send_syn_cmd(FALSE, udt->_next_send_seq, udt->_next_recv_seq, udt->_recv_window, udt->_id._virtual_source_port,
								udt->_id._virtual_target_port, data->_remote_ip, data->_guess_ex_port);
		CHECK_VALUE(ret);
	}
	return start_timer(ptl_handle_syn_timeout, NOTICE_INFINITE, PTL_SYN_TIMEOUT, 0, data, &data->_timeout_id);
}

_int32 ptl_recv_punch_hole_cmd(PUNCH_HOLE_CMD* cmd, _u32 remote_ip, _u16 remote_port)
{
	UDT_DEVICE* udt = NULL;
	ACTIVE_PUNCH_HOLE_DATA* data = NULL;
	CONN_ID id;
	id._virtual_source_port = cmd->_target_port;
	id._virtual_target_port = cmd->_source_port;
	id._peerid_hashcode = udt_hash_peerid(cmd->_peerid);
	set_find_node(&g_active_punch_hole_data_set, &id, (void**)&data);
	if(data == NULL)
	{
		LOG_DEBUG("ptl_recv_punch_hole_cmd, but ACTIVE_PUNCH_HOLE_DATA can't find, conn_id[%u, %u, %u].",
					(_u32)id._virtual_source_port, (_u32)id._virtual_target_port, id._peerid_hashcode);
		return SUCCESS;			//�Ѿ���������
	}
	//�յ�punch_hole_cmd��˵����ͨ��
	ptl_erase_active_punch_hole_data(data);
	udt = udt_find_device(&id);
	if(udt == NULL)
		return SUCCESS;		//û�ж�Ӧ���豸��������
	return udt_device_notify_punch_hole_success(udt, remote_ip, remote_port);
}

/*��ĳ������£������յ��Ĳ���punch_hole�������syn_ack����
����:�����򶴣�����������syn�������������punch_hole���
��syn���͸�ˣ����������ܵ�syn��ᷢ��syn_ack�����������յ�syn_ack;
��punch_hole����û�д�͸�ɹ�������һֱ�ղ���punch_hole����*/
_int32 ptl_remove_active_punch_hole_data(SYN_CMD* cmd, _u32 remote_ip, _u16 remote_port)
{
	UDT_DEVICE* udt = NULL;
	ACTIVE_PUNCH_HOLE_DATA* data = NULL;
	CONN_ID id;
	sd_assert(cmd->_flags == 1);
	id._virtual_source_port = cmd->_target_port;
	id._virtual_target_port = cmd->_source_port;
	id._peerid_hashcode = cmd->_peerid_hashcode;
	set_find_node(&g_active_punch_hole_data_set, &id, (void**)&data);
	if(data == NULL)
	{
		LOG_DEBUG("ptl_recv_syn_ack_cmd, but ACTIVE_PUNCH_HOLE_DATA can't find, conn_id[%u, %u, %u].",
					(_u32)id._virtual_source_port, (_u32)id._virtual_target_port, id._peerid_hashcode);
		return SUCCESS;			//�Ѿ���������
	}
	//�յ�syn_ack��˵����ͨ��
	ptl_erase_active_punch_hole_data(data);
	udt = udt_find_device(&id);
	if(udt == NULL)
		return SUCCESS;		//û�ж�Ӧ���豸��������
	return udt_device_notify_punch_hole_success(udt, remote_ip, remote_port);
}


_int32 ptl_erase_active_punch_hole_data(ACTIVE_PUNCH_HOLE_DATA * data)
{
	_int32 ret = SUCCESS;
	LOG_DEBUG("ptl_erase_active_punch_hole_data, conn_id[%u, %u, %u].",(_u32)data->_id._virtual_source_port,
				(_u32)data->_id._virtual_target_port, data->_id._peerid_hashcode);
	ret = set_erase_node(&g_active_punch_hole_data_set, data);
	if(ret!=SUCCESS) return SUCCESS;
	CHECK_VALUE(ret);
	if(data->_timeout_id != INVALID_MSG_ID)
	{
		cancel_timer(data->_timeout_id);
		data->_timeout_id = INVALID_MSG_ID;
	}
	sd_free(data);
	data = NULL;
	return ret;
}

_int32 ptl_handle_icallsomeone_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid)
{
	_int32 ret = SUCCESS;
	UDT_DEVICE* udt = NULL;
	ACTIVE_PUNCH_HOLE_DATA* data = (ACTIVE_PUNCH_HOLE_DATA*)msg_info->_user_data;
	if(errcode == MSG_CANCELLED)
		return SUCCESS;
	udt = udt_find_device(&data->_id);
	if(udt == NULL)
	{	//�豸�Ѿ��������ˣ�ɾ��icallsomeone����
		return ptl_erase_active_punch_hole_data(data);
	}
	++data->_retry_times;
	if(data->_retry_times < PTL_ICALLSOMEONE_MAX_RETRY)
	{
		ret = ptl_send_icallsomeone_cmd(data->_remote_peerid, data->_id._virtual_source_port, data->_remote_ip, data->_remote_port);
	}
	else
	{
		ptl_connect_callback(-1, udt->_device);
		ret = ptl_erase_active_punch_hole_data(data);
	}
	CHECK_VALUE(ret);
	return ret;
}

_int32 ptl_handle_syn_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid)
{
	_int32 ret = SUCCESS;
	UDT_DEVICE* udt = NULL;
	ACTIVE_PUNCH_HOLE_DATA* data = (ACTIVE_PUNCH_HOLE_DATA*)msg_info->_user_data;
	if(errcode == MSG_CANCELLED)
		return SUCCESS;
	udt = udt_find_device(&data->_id);
	if(udt == NULL)
	{
		return ptl_erase_active_punch_hole_data(data);
	}
	
	++data->_retry_times;
	if(data->_retry_times < PTL_SYN_MAX_RETRY)
	{
		ret = udt_send_syn_cmd(FALSE, udt->_next_send_seq, udt->_next_recv_seq, udt->_recv_window, udt->_id._virtual_source_port,
							udt->_id._virtual_target_port, data->_remote_ip, data->_remote_port);
		CHECK_VALUE(ret);
		if(data->_latest_ex_port != 0 && data->_latest_ex_port != data->_remote_port)
		{
			ret = udt_send_syn_cmd(FALSE, udt->_next_send_seq, udt->_next_recv_seq, udt->_recv_window, udt->_id._virtual_source_port,
									udt->_id._virtual_target_port, data->_remote_ip, data->_latest_ex_port);
			CHECK_VALUE(ret);
		}
		if(data->_guess_ex_port != 0 && data->_guess_ex_port != data->_remote_port && data->_guess_ex_port != data->_latest_ex_port)
		{
		
			ret = udt_send_syn_cmd(FALSE, udt->_next_send_seq, udt->_next_recv_seq, udt->_recv_window, udt->_id._virtual_source_port,
									udt->_id._virtual_target_port, data->_remote_ip, data->_guess_ex_port);
			//CHECK_VALUE(ret);
		}
	}
	else
	{
		ptl_connect_callback(-1, udt->_device);
		ret = ptl_erase_active_punch_hole_data(data);
	}
	return ret;
}


