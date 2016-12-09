#include "utility/define.h"
#ifdef ENABLE_EMULE
#include "emule_impl.h"
#include "emule_interface.h"
#include "emule_task_manager.h"
#include "emule_data_manager/emule_data_manager.h"
#include "emule_utility/emule_utility.h"
#include "emule_utility/emule_setting.h"
#include "emule_utility/emule_define.h"
#include "emule_server/emule_server_interface.h"
#include "emule_pipe/emule_pipe_device.h"
#include "emule_query/emule_query_emule_tracker.h"
#include "res_query/res_query_interface.h"
#include "utility/string.h"
#include "utility/peerid.h"
#include "utility/utility.h"
#include "utility/settings.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"
#include "task_manager/task_manager.h"
#include "data_manager/file_info.h"
#include "reporter/reporter_interface.h"
#include "utility/time.h"
#include "data_manager/file_info.h"

static	LIST		    g_emule_download_queue;
static	EMULE_PEER		g_emule_local_peer;

_int32 emule_init_local_peer(void)
{
	_u32 bitfeild = 0;
	char peerid[PEER_ID_SIZE + 1] = {0};
	char nick_name[64] = {0};
	EMULE_TAG* tag = NULL;
	LOG_DEBUG("emule_init_local_peer.");
	g_emule_local_peer._client_id = 0;
	//userid不能更改，否则很可能被verycd封杀
	emule_get_user_id(g_emule_local_peer._user_id);
	g_emule_local_peer._server_ip = 0;
	g_emule_local_peer._server_port = 0;
	//昵称
	emule_tag_list_init(&g_emule_local_peer._tag_list);
	sd_memcpy(nick_name, "[CHN]shaohan", sizeof("[CHN]shaohan"));
	settings_get_str_item("emule.nickname", nick_name);
	emule_create_str_tag(&tag, CT_NAME, nick_name);
	emule_tag_list_add(&g_emule_local_peer._tag_list, tag);
	emule_create_u32_tag(&tag, CT_VERSION, EDONKEYVERSION);
	emule_tag_list_add(&g_emule_local_peer._tag_list, tag);
	//这个标志重要，没有的话从迅雷客户端下载会被限速(限制为10 K)
	//但是加上这个标志，会被easyMule客户端禁用掉，所以目前默认不加
	if(emule_enable_thunder_tag())
	{
		get_peerid(peerid, PEER_ID_SIZE + 1);
		emule_create_str_tag(&tag, FT_THUNDERPEERID, peerid);
		emule_tag_list_add(&g_emule_local_peer._tag_list, tag);
	}
	//这个标志重要， 没有的话将被verycd客户端封杀
	emule_create_str_tag(&tag, CT_MOD_VERSION, "VeryCD 090304");
	emule_tag_list_add(&g_emule_local_peer._tag_list, tag);
	//这个标志用提高verycd的上传积分，没有的话会被verycd客户端封杀
	emule_create_u32_tag(&tag, CT_EMULE_VERSION, (0<<17) |(48<<10)|(0<<7));
	emule_tag_list_add(&g_emule_local_peer._tag_list, tag);
	//这个标志用来告诉对方kad和本地udp端口，Xtreme会根据该字段来屏蔽客户端，
	//必须有这个字段，否则Xtreme握手失败
	emule_create_u32_tag(&tag, CT_EMULE_UDPPORTS, /*kad_port << 16 |*/g_emule_local_peer._udp_port);
	emule_tag_list_add(&g_emule_local_peer._tag_list, tag);
	//这个标志用来告诉对方支持的功能
	SET_MISC_OPTION(bitfeild,UDPVER,4);
	SET_MISC_OPTION(bitfeild,COMPRESS, 0);			//是否支持压缩
//	SET_MISC_OPTION(bitfeild,SECUREIDENT,0x01 | 0x02); 	//支持两个版本的身份验证
	SET_MISC_OPTION(bitfeild,SOURCEEXCHANGE,4);         // 是否支持来源交换
	SET_MISC_OPTION(bitfeild,EXTENDREQUEST,2);
//	SET_MISC_OPTION(bitfeild,COMMENT,1);                // 
//	SET_MISC_OPTION(bitfeild,NOVIEWSHAREDFILE,1);       // 
//	SET_MISC_OPTION(bitfeild,MULTIPACKET,0);
//	SET_MISC_OPTION(bitfeild,PREVIEW,0);                // 是否支持预览
//	SET_MISC_OPTION(bitfeild,PEERCACHE,0);              // 是否支持peercache
//	SET_MISC_OPTION(bitfeild,UNICODE,1);                // 是否支持unicode
	SET_MISC_OPTION(bitfeild,AICH,1);            // 是否支持AICH纠错
	emule_create_u32_tag(&tag, CT_EMULE_MISCOPTIONS1, bitfeild);
	emule_tag_list_add(&g_emule_local_peer._tag_list, tag);
	//CT_EMULE_MISCOPTIONS2混合选项,这个标志用来告诉对方扩展的功能
	bitfeild = 0;
	SET_MISC_OPTION(bitfeild, LARGE_FILES, 1);		//是否支持大于4G 的文件
	emule_create_u32_tag(&tag, CT_EMULE_MISCOPTIONS2, bitfeild);
	emule_tag_list_add(&g_emule_local_peer._tag_list, tag);
	return SUCCESS;
}

_int32 emule_uninit_local_peer(void)
{
	emule_tag_list_uninit(&g_emule_local_peer._tag_list, TRUE);
	return SUCCESS;
}

EMULE_PEER* emule_get_local_peer(void)
{
	return &g_emule_local_peer;
}

_u32 emule_get_local_client_id(void)
{
    return g_emule_local_peer._client_id;
}

_int32 emule_notify_get_cid_info(EMULE_TASK* task, _u8 cid[CID_SIZE], _u8 gcid[CID_SIZE])
{
	char  cid_str[41] = {0}, gcid_str[41] = {0};
	str2hex((char*)cid, CID_SIZE, cid_str, 40);
	str2hex((char*)gcid, CID_SIZE, gcid_str, 40);
	LOG_DEBUG("[emule_task = 0x%x]emule_notify_get_cid_info. cid = %s, gcid = %s", task, cid_str, gcid_str);
	task->_data_manager->_is_cid_and_gcid_valid = TRUE;
	sd_memcpy(task->_data_manager->_cid, cid, CID_SIZE);
	sd_memcpy(task->_data_manager->_gcid, gcid, CID_SIZE);
	//task->_query_param._file_index = 0;		//要填0
	//task->_query_param._task_ptr = (TASK*)task;
    file_info_set_is_calc_bcid(&task->_data_manager->_file_info, FALSE);
    emule_task_shub_res_query(task);
    emule_task_peer_res_query(task);

    return SUCCESS;
}

_int32 emule_notify_get_part_hash(EMULE_TASK* task, const _u8* part_hash, _u32 part_hash_len)
{
	EMULE_DATA_MANAGER* data_manager = task->_data_manager;
	LOG_DEBUG("[emule_task = 0x%x]emule_notify_get_part_hash, part_hash_len = %u.", task, part_hash_len);
	return emule_set_part_hash(data_manager, part_hash, part_hash_len);
}

_int32 emule_notify_get_aich_hash(EMULE_TASK* task, const _u8* aich_hash, _u32 aich_hash_len)
{
	EMULE_DATA_MANAGER* data_manager = task->_data_manager;
	LOG_DEBUG("[emule_task = 0x%x]emule_notify_get_aich_hash, aich_hash_len = %u.", task, aich_hash_len);
	return emule_set_aich_hash(data_manager, aich_hash, aich_hash_len);
}

_int32 emule_notify_query_failed(EMULE_TASK* task)
{
	EMULE_DATA_MANAGER* data_manager = task->_data_manager;
	LOG_DEBUG("[emule_task = 0x%x]emule_notify_query_failed.", task);
	return file_info_reset_bcid_info(&data_manager->_file_info);
}

_int32 emule_notify_task_failed(EMULE_TASK* task, _int32 errcode)
{	
#ifdef EMBED_REPORT
     VOD_REPORT_PARA report_para;
#endif

	LOG_DEBUG("[emule_task = 0x%x]emule_notify_task_failed, errcode = %d.", task, errcode);

   if(task->_task.task_status != TASK_S_STOPPED) 
   {
	   task->_task.task_status = TASK_S_FAILED;
   }
   task->_task.failed_code = errcode;

#ifdef EMBED_REPORT
    sd_memset(&report_para, 0, sizeof(VOD_REPORT_PARA));
    emb_reporter_emule_stop_task((TASK*)task, &report_para, TASK_S_FAILED);
#endif    
	return SUCCESS;	
}

_int32 emule_notify_task_success(struct tagEMULE_TASK* task)
{

	LOG_DEBUG("[emule_task = 0x%x]emule_notify_task_success.", task);
	task->_task.task_status = TASK_S_SUCCESS;
	task->_task._downloaded_data_size = task->_task.file_size;
    return SUCCESS;
}

void emule_notify_task_delete(struct tagEMULE_TASK* task)
{
	LOG_DEBUG("[emule_task = 0x%x]emule_notify_task_delete.", task);
    emule_close_data_manager(task->_data_manager);
    delete_task(&task->_task);
    sd_free(task);
	task = NULL;
    tm_signal_sevent_handle();
}

_int32 emule_notify_client_id_change(_u32 client_id)
{

	const LIST* tasks_list = NULL;
#ifdef ENABLE_EMULE_PROTOCOL

	LIST_ITERATOR iter = NULL;
	EMULE_TASK* emule_task = NULL;
#endif
    LOG_DEBUG("emule_notify_client_id_change, client_id = %u.", client_id);
	g_emule_local_peer._client_id = client_id;
	//登录服务器成功后，可以查看resource
	emule_get_task_list(&tasks_list);
#ifdef ENABLE_EMULE_PROTOCOL
	for(iter = LIST_BEGIN(*tasks_list); iter != LIST_END(*tasks_list); iter = LIST_NEXT(iter))
	{
		emule_task = (EMULE_TASK*)LIST_VALUE(iter);

        if(cm_is_need_more_peer_res(&emule_task->_task._connect_manager, INVALID_FILE_INDEX) == TRUE)
        {
            emule_server_query_source(emule_task->_data_manager->_file_id, 
                                      emule_task->_data_manager->_file_info._file_size);
        }
	}
#endif    
	return SUCCESS;
}

_int32 emule_notify_find_source(_u8 file_id [ FILE_ID_SIZE ], _u32 client_id, _u16 client_port, _u32 server_ip, _u16 server_port)
{
	_int32 ret = SUCCESS;
	EMULE_TASK* task = NULL;
	task = emule_find_task(file_id);
	if(task == NULL)
	{
		LOG_ERROR("emule_notify_find_source, but task not found.");
		return ret;
	}
	ret = cm_add_emule_resource(&task->_task._connect_manager, client_id, client_port, server_ip, server_port);
	return ret;	
}

#ifdef ENABLE_EMULE_PROTOCOL
_int32 emule_notify_passive_connect(struct tagEMULE_PIPE_DEVICE* pipe_device)
{
    LOG_DEBUG("emule_notify_passive_connect, pipe_device(0x%x).", pipe_device);
	return emule_pipe_device_recv_cmd(pipe_device, EMULE_HEADER_SIZE);
}
#endif

//emule download_queue 
_int32 emule_init_download_queue(void)
{
	list_init(&g_emule_download_queue);
	return SUCCESS;
}
#ifdef ENABLE_EMULE_PROTOCOL

_int32 emule_download_queue_add(EMULE_DATA_PIPE* emule_pipe)
{
    EMULE_DATA_PIPE* list_pipe = NULL;
    LIST_ITERATOR cur_node = LIST_BEGIN(g_emule_download_queue);
	LOG_DEBUG("[emule_pipe = 0x%x]emule_download_queue_add.", emule_pipe);

    while( cur_node != LIST_END(g_emule_download_queue) )
    {
        list_pipe = (EMULE_DATA_PIPE*)LIST_VALUE( cur_node );
        if( list_pipe == emule_pipe )
        {
            return SUCCESS;
        }
        cur_node = LIST_NEXT( cur_node );
    }
	return list_push(&g_emule_download_queue, emule_pipe);
}

_int32 emule_download_queue_remove(EMULE_DATA_PIPE* emule_pipe)
{
	_int32 ret = -1;
	LIST_ITERATOR iter = NULL;
	for(iter = LIST_BEGIN(g_emule_download_queue); iter != LIST_END(g_emule_download_queue); iter = LIST_NEXT(iter))
	{
		if(LIST_VALUE(iter) == emule_pipe)
		{
			list_erase(&g_emule_download_queue, iter);
			ret = SUCCESS;
			break;
		}
	}
	LOG_DEBUG("[emule_pipe = 0x%x]emule_download_queue_remove, result = %d.", emule_pipe, ret);
	return ret;
}

EMULE_DATA_PIPE* emule_download_queue_find(_u32 ip, _u16 udp_port)
{
	EMULE_DATA_PIPE* emule_pipe = NULL;
	LIST_ITERATOR iter = NULL;
	for(iter = LIST_BEGIN(g_emule_download_queue); iter != LIST_END(g_emule_download_queue); iter = LIST_NEXT(iter))
	{
		emule_pipe = (EMULE_DATA_PIPE*)LIST_VALUE(iter);
		if(emule_pipe->_remote_peer._client_id == ip && emule_pipe->_remote_peer._udp_port == udp_port)
			return emule_pipe;
	}
	return NULL;
}
#endif

_int32 emule_uninit_download_queue(void)
{
	sd_assert(list_size(&g_emule_download_queue) == 0);
	return SUCCESS;
}

_int32 emule_kad_notify_find_sources(void* user_data, _u8 user_hash[USER_ID_SIZE], struct tagEMULE_TAG_LIST* tag_list )
{
	_int32 ret = SUCCESS;
	_u32 ip = 0;
	_u16 port = 0;
	const EMULE_TAG* tag = NULL;
	EMULE_TASK* task = (EMULE_TASK*)user_data;
	tag = emule_tag_list_get(tag_list, TAG_SOURCEIP);
	//sd_assert(tag != NULL && emule_tag_is_u32(tag));
    
	if(tag == NULL || !emule_tag_is_u32(tag)) return SUCCESS;
    
	ip = tag->_value._val_u32;
	tag = emule_tag_list_get(tag_list, TAG_SOURCEPORT);
	sd_assert(tag != NULL);
	port = (_u16)tag->_value._val_u32;
	if(IS_HIGHID(ip))
		ip = sd_ntohl(ip);
	LOG_DEBUG("[emule_task = 0x%x]emule_kad_notify_find_sources, ip = %u, port = %u, is_high_id = %d.", task, ip, (_u32)port, IS_HIGHID(ip));
	cm_add_emule_resource(&task->_task._connect_manager, ip, port, 0, 0);
	return ret;
}

void emule_task_shub_res_query(struct tagEMULE_TASK* task)
{
    _int32 ret_val = SUCCESS;
    if(task->_res_query_shub_retry_count > MAX_QUERY_SHUB_RETRY_TIMES ) return;
    if( task->_res_query_shub_state != RES_QUERY_REQING && sd_is_cid_valid(task->_data_manager->_cid)  )
    {
        if( sd_is_cid_valid(task->_data_manager->_gcid) )
        {
            LOG_DEBUG( "MMMM emule_task_shub_res_query:res_query_shub_by_cid 1" );
            ret_val = res_query_shub_by_resinfo_newcmd(&task->_query_param, emule_notify_query_only_res_shub_callback, task->_data_manager->_cid, task->_data_manager->_file_size, TRUE, 
                (char *)task->_data_manager->_gcid,TRUE, MAX_SERVER, DEFAULT_BONUS_RES_NUM, task->_query_shub_newres_times_sn++, 4);
        }
        else
        {
            LOG_DEBUG( "MMMM emule_task_shub_res_query:res_query_shub_by_cid 2" );
            ret_val = res_query_shub_by_cid_newcmd(&task->_query_param, emule_notify_query_shub_callback, task->_data_manager->_cid, task->_data_manager->_file_size, FALSE, 
                DEFAULT_LOCAL_URL,FALSE, MAX_SERVER, DEFAULT_BONUS_RES_NUM, task->_query_shub_times_sn++, 4);
        }

        if( ret_val != SUCCESS ) 
        {
            task->_res_query_shub_state = RES_QUERY_FAILED;
        }
        else
        {
            task->_res_query_shub_retry_count++;
            task->_res_query_shub_state = RES_QUERY_REQING;
#ifdef EMBED_REPORT
            sd_time_ms( &(((TASK*)task)->_res_query_report_para._shub_query_time) );
#endif	
        }     
    }    
}

void emule_task_query_emule_tracker(struct tagEMULE_TASK *task)
{
    _int32 ret_val = SUCCESS;
    LOG_DEBUG("emule_task_query_emule_tracker, task(0x%x), query_state(%d)", task, 
        task->_res_query_emule_tracker_state);

    if (task->_res_query_emule_tracker_state != RES_QUERY_REQING)
    {
        ret_val = res_query_emule_tracker(&task->_query_param, emule_notify_query_emule_tracker_callback, 
            task->_last_query_emule_tracker_stamp, (const char*)task->_data_manager->_file_id, 0, 0);
        if( ret_val != SUCCESS ) 
        {
            task->_res_query_emule_tracker_state = RES_QUERY_FAILED;
        }
        else
        {
            task->_res_query_emule_tracker_state = RES_QUERY_REQING;
        } 
    }
}

void emule_task_query_phub(struct tagEMULE_TASK* task)
{
    _int32 ret_val = SUCCESS;
    if( task->_res_query_phub_state != RES_QUERY_REQING )
    {
        LOG_DEBUG( "MMMM emule_task_peer_res_query:res_query_phub" );
        ret_val = res_query_phub(&task->_query_param, emule_notify_query_phub_result, task->_data_manager->_cid, 
            task->_data_manager->_gcid, task->_data_manager->_file_size, DEFAULT_BONUS_RES_NUM, 4);
        if( ret_val != SUCCESS ) 
        {
            task->_res_query_phub_state = RES_QUERY_FAILED;
        }
        else
        {
            task->_res_query_phub_state = RES_QUERY_REQING;
#ifdef EMBED_REPORT
            sd_time_ms( &(((TASK*)task)->_res_query_report_para._phub_query_time) );
#endif	
        }
    }
}

_int32 emule_notify_res_query_normal_cdn(void* user_data, _int32 errcode, _u8 result, _u32 retry_interval)
{
	RES_QUERY_PARA *p_para = (RES_QUERY_PARA *)user_data;
	struct tagEMULE_TASK* task = (struct tagEMULE_TASK*)p_para->_task_ptr;

	LOG_DEBUG("MMMM emule_notify_query_normal_cdn_result, errcode = %d, result = %u.", errcode, (_u32)result);

	if( errcode == SUCCESS )
	{
		task->_res_query_normal_cdn_state = RES_QUERY_SUCCESS;
	}
	else
	{
		task->_res_query_normal_cdn_state = RES_QUERY_FAILED;
#ifdef EMBED_REPORT
	    task->_task._res_query_report_para._normal_cdn_fail++;
#endif	
	}
	return SUCCESS;
}

void emule_task_query_normal_cdn(struct tagEMULE_TASK* task)
{
	_int32 ret_val = SUCCESS;
	if( (task->_res_query_normal_cdn_state == RES_QUERY_SUCCESS) ||
		(task->_res_query_normal_cdn_state == RES_QUERY_END)||	
		(task->_res_query_normal_cdn_cnt >= 3) ||
		(task->_res_query_normal_cdn_state == RES_QUERY_REQING)  )
		return;
	LOG_DEBUG( "MMMM emule_task_peer_res_query:res_query_normal_cdn" );
	ret_val = res_query_normal_cdn_manager(&(task->_query_param), 
		emule_notify_res_query_normal_cdn, task->_data_manager->_cid, task->_data_manager->_gcid, task->_data_manager->_file_size, DEFAULT_BONUS_RES_NUM, 4);
	if(ret_val != SUCCESS)
	{
		task->_res_query_normal_cdn_state = RES_QUERY_IDLE;
		LOG_DEBUG( "MMMM emule_task_peer_res_query:res_query_normal_cdn start fail ret_val=%d",ret_val );
	}
	else
	{
		task->_res_query_normal_cdn_state = RES_QUERY_REQING;
		task->_res_query_normal_cdn_cnt++;
	}
}

_u32 emule_task_get_file_idx_content(struct tagEMULE_TASK* task, char **out_file_idx_content)
{
     _int32 ret_val = SUCCESS;
    _u32 file_idx_content_cnt = 0;
    char *file_idx_content = NULL;

    _u32 file_idx_num = 0;

    *out_file_idx_content = NULL;

#if 0

    /*
    _u64 file_size = 0;
    _u32 block_size = 0;
    _u16 b16_idx = 0, e16_idx = 0, l16_idx = 0;
    _u32 b32_idx = 0, e32_idx = 0, l32_idx = 0;
    _u32 i = 0;
    RANGE_LIST_ITEROATOR range_it;
    _u16 *p_2b_idx = NULL;
    _u32 *p_4b_idx = NULL;
    */

    // 先预先申请存放足够file index的内存
    // 最差的情况就是每一个range，占用两个idx（起始和结束）
    file_idx_num = 2 * range_list_size(&task->_data_manager->_uncomplete_range_list);
    LOG_DEBUG("file_idx_num(%d).", file_idx_num);

    if (file_idx_num == 0)
    {
        // 注意：文件较小的话，可能已经下载完成了，所以不应该去查询dphub了
        LOG_DEBUG("There is no uncomplete range, so return.");
        return ERR_RES_QUERY_NO_NEED_QUERY_DPHUB;
    }

    file_size = task->_data_manager->_file_size;
    block_size = task->_data_manager->_file_info._block_size;

    LOG_DEBUG("file_size = %llu, block_size = %u.", file_size, block_size);
    if ((block_size != 0) && (file_size / block_size < 65535))
    {
        LOG_DEBUG("idx-range mode in 16bit per range index.");

        // 这部分内存在填充命令之后释放 build_dphub_rc_query_cmd
        ret_val = sd_malloc(2 * file_idx_num, (void **)&file_idx_content);
        CHECK_VALUE(ret_val);
        sd_memset((void *)file_idx_content, 0, file_idx_num);

        p_2b_idx = (_u16 *)file_idx_content;   // 2B模式
        for (i=0; i<range_list_size(&task->_data_manager->_uncomplete_range_list); ++i)
        {
            range_it = task->_data_manager->_recv_range_list._head_node;
            b16_idx = (_u16)(((_u64)range_it->_range._index) * get_data_unit_size() / block_size + 1);
            e16_idx = (_u16)(range_to_length(&range_it->_range, file_size) / block_size + 1);
            if (b16_idx <= e16_idx)
            {
                if ((file_idx_content == NULL) || (b16_idx > l16_idx + 1))
                {
                    *p_2b_idx = b16_idx;
                    *(p_2b_idx + 1) = e16_idx;
                    p_2b_idx += 2;
                    file_idx_content_cnt += 2;
                }
                else
                {
                    // 两个range相连
                    *p_2b_idx = e16_idx;
                }

                l16_idx = e16_idx;                        
            }
        }

        p_2b_idx = (_u16 *)file_idx_content;
        if ((file_idx_content_cnt == 2) && (*p_2b_idx == 1) && (*(p_2b_idx + 1) >= file_size/block_size))
        {
            // 超出文件范围，不再以部分文件的方式查询
            file_idx_desc_type = 0;
            file_idx_content_cnt = 0;
            if (file_idx_content != 0)
            {
                sd_free(file_idx_content);
                file_idx_content = 0;
            }
        }
        else
        {
            file_idx_desc_type = 2;
        }
    }
    else
    {
        LOG_DEBUG("idx-range mode in 32bit per range index.");

        // 这部分内存在填充命令之后释放 build_dphub_rc_query_cmd
        ret_val = sd_malloc(4 * file_idx_num, (void **)&file_idx_content);
        CHECK_VALUE(ret_val);
        sd_memset((void *)file_idx_content, 0, file_idx_num);

        p_4b_idx = (_u32 *)file_idx_content;   // 4B模式
        for (i=0; i<range_list_size(&task->_data_manager->_uncomplete_range_list); ++i)
        {
            range_it = task->_data_manager->_recv_range_list._head_node;
            b32_idx = (_u32)(((_u64)range_it->_range._index) * get_data_unit_size() / block_size + 1);
            e32_idx = (_u32)(range_to_length(&range_it->_range, file_size) / block_size + 1);
            if (b32_idx <= e32_idx)
            {
                if ((file_idx_content == NULL) || (b32_idx > l32_idx + 1))
                {
                    *p_4b_idx = b32_idx;
                    *(p_4b_idx + 1) = e32_idx;
                    p_4b_idx += 2;
                    file_idx_content_cnt += 2;
                }
                else
                {
                    // 两个range相连
                    *p_4b_idx = e32_idx;
                }
                l32_idx = e32_idx;              
            }
        }

        p_4b_idx = (_u32 *)file_idx_content;
        if ((file_idx_content_cnt == 2) && (*p_2b_idx == 1) && (*(p_2b_idx + 1) >= file_size/block_size))
        {
            // 超出文件范围，不再以部分文件的方式查询
            file_idx_desc_type = 0;
            file_idx_content_cnt = 0;
            if (file_idx_content != 0)
            {
                sd_free(file_idx_content);
                file_idx_content = 0;
            }
        }
        else
        {
            file_idx_desc_type = 4;
        }
    }

#endif

    *out_file_idx_content = file_idx_content;
    return file_idx_content_cnt;
}

void emule_task_query_dphub(struct tagEMULE_TASK* task)
{
    _int32 ret_val = SUCCESS;
    _int16 file_src_type = 1; // 1: 普通完整文件
    _u8 file_idx_desc_type = 0; // 0：无块描述
    _u32 file_idx_content_cnt = 0;
    char *file_idx_content = NULL;

    if (task->_res_query_dphub_state != RES_QUERY_REQING)
    {
        LOG_DEBUG("MMMM emule_task_peer_res_query:res_query_dphub");

        if (range_list_size(&task->_data_manager->_recv_range_list) != 0)
        {
            file_src_type = 2;  // 2: 普通不完整文件
        }

        if (file_src_type != 1)        
        {
            file_idx_content_cnt = emule_task_get_file_idx_content(task, &file_idx_content);            
        }

        ret_val = res_query_dphub(&task->_query_param, emule_notify_query_dphub_result, 
            file_src_type, 
            file_idx_desc_type,
            file_idx_content_cnt,
            file_idx_content, 
            task->_data_manager->_file_size, 
            task->_data_manager->_file_info._block_size, 
            (const char *)task->_data_manager->_cid, 
            (const char *)task->_data_manager->_gcid, 
            (_u16)EMULE_TASK_TYPE);
    
        task->_res_query_dphub_state = (ret_val != SUCCESS) ? RES_QUERY_FAILED : RES_QUERY_REQING;

        if (ret_val == ERR_RES_QUERY_NO_ROOT)
        {
            task->_res_query_dphub_state = RES_QUERY_IDLE;
            if (task->_dpub_query_context._wait_dphub_root_timer_id == 0)
            {
                ret_val = start_timer(emule_handle_wait_dphub_root_timeout, 1, WAIT_FOR_DPHUB_ROOT_RETURN_TIMEOUT, 0, 
                    (void *)task, &(task->_dpub_query_context._wait_dphub_root_timer_id));
                if(ret_val!=SUCCESS)
                {
                    dt_failure_exit((TASK *)task, ret_val);
                }
            }            
        } 
    }
}

_int32 emule_handle_wait_dphub_root_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid)
{
    TASK * p_task = (TASK *)(msg_info->_user_data);
    EMULE_TASK *p_emule_task = (EMULE_TASK *)(msg_info->_user_data);

    if(errcode == MSG_TIMEOUT)
    {
        if (p_emule_task==NULL) CHECK_VALUE(DT_ERR_INVALID_DOWNLOAD_TASK);

        LOG_DEBUG("emule_handle_wait_dphub_root_timeout:_task_id=%u,_wait_dphub_root_timer_id=%u,task_status=%d,need_more_res=%d",
            p_task->_task_id, p_emule_task->_dpub_query_context._wait_dphub_root_timer_id, p_task->task_status, cm_is_global_need_more_res());

        if(msgid == p_emule_task->_dpub_query_context._wait_dphub_root_timer_id)
        {
            if((p_task->task_status==TASK_S_RUNNING)
               && (emule_enable_p2sp())
               && (cm_is_global_need_more_res())
               && (cm_is_need_more_peer_res(&(p_task->_connect_manager), INVALID_FILE_INDEX)))
            {
                if(p_emule_task->_data_manager->_is_cid_and_gcid_valid == TRUE)
                {
                    emule_task_query_dphub(p_emule_task);
                }
            }
        }

    }

    return SUCCESS;
}

void emule_task_query_viphub(struct tagEMULE_TASK* task)
{
#ifdef ENABLE_HSC	
    _int32 ret_val = SUCCESS;
    if( task->_res_query_vip_hub_state != RES_QUERY_REQING )
    {
        LOG_DEBUG( "MMMM emule_task_peer_res_query:res_query_vip_hub" );
        ret_val = res_query_vip_hub(&task->_query_param, emule_notify_query_vip_hub_result, task->_data_manager->_cid, 
            task->_data_manager->_gcid, task->_data_manager->_file_size, DEFAULT_BONUS_RES_NUM, 4);
        if( ret_val != SUCCESS ) 
        {
            task->_res_query_vip_hub_state = RES_QUERY_FAILED;
        }
        else
        {
            task->_res_query_vip_hub_state = RES_QUERY_REQING;
        }
    }
#endif
}

void emule_task_query_tracker(struct tagEMULE_TASK* task)
{
    _int32 ret_val = SUCCESS;
    if( task->_res_query_tracker_state != RES_QUERY_REQING )
    {
        LOG_DEBUG( "MMMM emule_task_peer_res_query:res_query_tracker" );
        ret_val = res_query_tracker(&task->_query_param, emule_notify_query_tracker_callback, task->_last_query_stamp, 
            task->_data_manager->_gcid, task->_data_manager->_file_size, MAX_PEER, DEFAULT_BONUS_RES_NUM, 0, 0);
        if( ret_val != SUCCESS ) 
        {
            task->_res_query_tracker_state = RES_QUERY_FAILED;
        }
        else
        {
            task->_res_query_tracker_state = RES_QUERY_REQING;
#ifdef EMBED_REPORT
            sd_time_ms( &(((TASK*)task)->_res_query_report_para._tracker_query_time) );
#endif	
        }  
    }
}

void emule_task_query_partner_cdn(struct tagEMULE_TASK* task)
{
    _int32 ret_val = SUCCESS;
    if( task->_res_query_partner_cdn_state != RES_QUERY_REQING )
    {
        LOG_DEBUG( "MMMM emule_task_peer_res_query:res_query_partner_cdn" );
#if defined(MACOS) && defined(ENABLE_CDN)
        ret_val = res_query_cdn_manager(CDN_VERSION,task->_data_manager->_gcid,task->_data_manager->_file_size,emule_notify_query_cdn_manager_callback,&(task->_query_param));
#else
        ret_val = res_query_partner_cdn(&task->_query_param, emule_notify_query_partner_cdn_callback, task->_data_manager->_cid, task->_data_manager->_gcid, task->_data_manager->_file_size);
#endif /* MACOS */
        if( ret_val != SUCCESS ) 
        {
            task->_res_query_partner_cdn_state = RES_QUERY_FAILED;
        }
        else
        {
            task->_res_query_partner_cdn_state = RES_QUERY_REQING;
        }  
    }
}

void emule_task_peer_res_query(struct tagEMULE_TASK* task)
{
    emule_task_query_phub(task);
    emule_task_query_dphub(task); 
    emule_task_query_viphub(task);
    emule_task_query_tracker(task);
    emule_task_query_partner_cdn(task);
	emule_task_query_normal_cdn(task);
}

_int32 emule_notify_query_shub_callback(void* user_data,_int32 errcode, _u8 result, _u64 file_size, _u8* cid, _u8* gcid, _u32 gcid_level, _u8* bcid, _u32 bcid_size, _u32 block_size,_u32 retry_interval,char * file_suffix,_int32 control_flag)
{
    RES_QUERY_PARA *p_para = (RES_QUERY_PARA *)user_data;
    struct tagEMULE_TASK* task = (struct tagEMULE_TASK*)p_para->_task_ptr;
#ifdef EMBED_REPORT
    _u32 cur_time = 0;
    _u32 shub_query_time = 0;
    RES_QUERY_REPORT_PARA *p_report_para = &p_para->_task_ptr->_res_query_report_para;
#endif

    LOG_DEBUG("MMMM emule_notify_query_shub_callback, errcode = %d, result = %u.", errcode, (_u32)result);

#ifdef EMBED_REPORT
        sd_time_ms( &cur_time );
        shub_query_time = TIME_SUBZ( cur_time, p_report_para->_shub_query_time );
    
        p_report_para->_hub_s_max = MAX(p_report_para->_hub_s_max, shub_query_time);
        if( p_report_para->_hub_s_min == 0 )
        {
            p_report_para->_hub_s_min = shub_query_time;
        }
    
        p_report_para->_hub_s_min = MIN(p_report_para->_hub_s_min, shub_query_time);
        p_report_para->_hub_s_avg = (p_report_para->_hub_s_avg*(p_report_para->_hub_s_succ+p_report_para->_hub_s_fail)+shub_query_time) / (p_report_para->_hub_s_succ+p_report_para->_hub_s_fail+1);
    
#endif	
    if( errcode == SUCCESS )
    {
#ifdef EMBED_REPORT
        p_report_para->_hub_s_succ++;
#endif	
        task->_res_query_shub_state = RES_QUERY_SUCCESS;
    }
    else
    {
#ifdef EMBED_REPORT
        p_report_para->_hub_s_fail++;
#endif
        task->_res_query_shub_state = RES_QUERY_FAILED;
    }

   LOG_DEBUG( "MMMM emule_notify_query_shub_callback:res_query_shub_by_cid 1" );

   if(errcode == SUCCESS && sd_is_cid_valid(cid) && sd_is_cid_valid(gcid) )
   {
  	_int32 ret_val = res_query_shub_by_resinfo_newcmd(&task->_query_param, emule_notify_query_only_res_shub_callback, cid, file_size, TRUE, 
                (const char*)gcid,TRUE, MAX_SERVER, DEFAULT_BONUS_RES_NUM, task->_query_shub_newres_times_sn++, 4);

   	if(ret_val == SUCCESS)
   	{
   		   task->_res_query_shub_state = RES_QUERY_REQING;
   	}
   }

    return SUCCESS;

}



_int32 emule_notify_query_only_res_shub_callback(void* user_data,_int32 errcode, _u8 result, _u64 file_size, _u8* cid, _u8* gcid, _u32 gcid_level, _u8* bcid, _u32 bcid_size, _u32 block_size,_u32 retry_interval,char * file_suffix,_int32 control_flag)
{
    RES_QUERY_PARA *p_para = (RES_QUERY_PARA *)user_data;
    struct tagEMULE_TASK* task = (struct tagEMULE_TASK*)p_para->_task_ptr;
#ifdef EMBED_REPORT
    _u32 cur_time = 0;
    _u32 shub_query_time = 0;
    RES_QUERY_REPORT_PARA *p_report_para = &p_para->_task_ptr->_res_query_report_para;
#endif

    LOG_DEBUG("MMMM emule_notify_query_only_res_shub_callback, errcode = %d, result = %u.", errcode, (_u32)result);

#ifdef EMBED_REPORT
        sd_time_ms( &cur_time );
        shub_query_time = TIME_SUBZ( cur_time, p_report_para->_shub_query_time );
    
        p_report_para->_hub_s_max = MAX(p_report_para->_hub_s_max, shub_query_time);
        if( p_report_para->_hub_s_min == 0 )
        {
            p_report_para->_hub_s_min = shub_query_time;
        }
    
        p_report_para->_hub_s_min = MIN(p_report_para->_hub_s_min, shub_query_time);
        p_report_para->_hub_s_avg = (p_report_para->_hub_s_avg*(p_report_para->_hub_s_succ+p_report_para->_hub_s_fail)+shub_query_time) / (p_report_para->_hub_s_succ+p_report_para->_hub_s_fail+1);
    
#endif	
    if( errcode == SUCCESS )
    {
#ifdef EMBED_REPORT
        p_report_para->_hub_s_succ++;
#endif	
        task->_res_query_shub_state = RES_QUERY_SUCCESS;
    }
    else
    {
#ifdef EMBED_REPORT
        p_report_para->_hub_s_fail++;
#endif
        task->_res_query_shub_state = RES_QUERY_FAILED;
    }

    return SUCCESS;

}


_int32 emule_notify_query_phub_result(void* user_data,_int32 errcode, _u8 result, _u32 retry_interval)
{
    RES_QUERY_PARA *p_para = (RES_QUERY_PARA *)user_data;
    struct tagEMULE_TASK* task = (struct tagEMULE_TASK*)p_para->_task_ptr;

#ifdef EMBED_REPORT
    _u32 cur_time = 0;
    _u32 phub_query_time = 0;
    RES_QUERY_REPORT_PARA *p_report_para = &p_para->_task_ptr->_res_query_report_para;
#endif

    LOG_DEBUG("MMMM emule_notify_query_phub_result, errcode = %d, result = %u.", errcode, (_u32)result);

#ifdef EMBED_REPORT
    sd_time_ms( &cur_time );
    phub_query_time = TIME_SUBZ( cur_time, p_report_para->_phub_query_time );

    p_report_para->_hub_p_max = MAX(p_report_para->_hub_p_max, phub_query_time);
    if( p_report_para->_hub_p_min == 0 )
    {
        p_report_para->_hub_p_min = phub_query_time;
    }
    p_report_para->_hub_p_min = MIN(p_report_para->_hub_p_min, phub_query_time);
    p_report_para->_hub_p_avg = (p_report_para->_hub_p_avg*(p_report_para->_hub_p_succ+p_report_para->_hub_p_fail)+phub_query_time) / (p_report_para->_hub_p_succ+p_report_para->_hub_p_fail+1);

#endif	
    if( errcode == SUCCESS )
    {
        task->_res_query_phub_state = RES_QUERY_SUCCESS;
#ifdef EMBED_REPORT
        p_report_para->_hub_p_succ++;
#endif	

    }
    else
    {
        task->_res_query_phub_state = RES_QUERY_FAILED;
#ifdef EMBED_REPORT
        p_report_para->_hub_p_fail++;
#endif		

    }
    return SUCCESS;
}

#ifdef ENABLE_HSC
_int32 emule_notify_query_vip_hub_result(void* user_data,_int32 errcode, _u8 result, _u32 retry_interval)
{
    RES_QUERY_PARA *p_para = (RES_QUERY_PARA *)user_data;
    struct tagEMULE_TASK* task = (struct tagEMULE_TASK*)p_para->_task_ptr;

    LOG_DEBUG("MMMM emule_notify_query_vip_hub_result, errcode = %d, result = %u.", errcode, (_u32)result);

    if( errcode == SUCCESS )
    {
        task->_res_query_vip_hub_state = RES_QUERY_SUCCESS;
    }
    else
    {
        task->_res_query_vip_hub_state = RES_QUERY_FAILED;
    }
    return SUCCESS;
}

#endif /* ENABLE_HSC */

_int32 emule_notify_query_tracker_callback(void* user_data,_int32 errcode, _u8 result, _u32 retry_interval,_u32 query_stamp)
{
    RES_QUERY_PARA *p_para = (RES_QUERY_PARA *)user_data;
    struct tagEMULE_TASK* task = (struct tagEMULE_TASK*)p_para->_task_ptr;
#ifdef EMBED_REPORT
    _u32 cur_time = 0;
    _u32 tracker_query_time = 0;
    RES_QUERY_REPORT_PARA *p_report_para = &p_para->_task_ptr->_res_query_report_para;
#endif

    LOG_DEBUG("MMMM emule_notify_query_tracker_callback, errcode = %d, result = %u.", errcode, (_u32)result);
#ifdef EMBED_REPORT
    sd_time_ms( &cur_time );
    tracker_query_time = TIME_SUBZ( cur_time, p_report_para->_tracker_query_time );

    p_report_para->_hub_t_max = MAX(p_report_para->_hub_t_max, tracker_query_time);
    if( p_report_para->_hub_t_min == 0 )
    {
        p_report_para->_hub_t_min = tracker_query_time;
    }
    p_report_para->_hub_t_min = MIN(p_report_para->_hub_t_min, tracker_query_time);
    p_report_para->_hub_t_avg = (p_report_para->_hub_t_avg*(p_report_para->_hub_t_succ+p_report_para->_hub_t_fail)+tracker_query_time) / (p_report_para->_hub_t_succ+p_report_para->_hub_t_fail+1);

#endif	
    if( errcode == SUCCESS )
    {
        task->_res_query_tracker_state = RES_QUERY_SUCCESS;
		task->_last_query_stamp = query_stamp;
#ifdef EMBED_REPORT
        p_report_para->_hub_t_succ++;
#endif	
    }
    else
    {
        task->_res_query_tracker_state = RES_QUERY_FAILED;
#ifdef EMBED_REPORT
        p_report_para->_hub_t_fail++;
#endif	
    }

    return SUCCESS;
}
#ifdef ENABLE_CDN
_int32 emule_notify_query_cdn_manager_callback(_int32 errcode, void* user_data, _u8 result, BOOL is_end, char* host, _u16 port)
{
	_int32 ret_val = SUCCESS;
	_u32 host_ip=0;
    RES_QUERY_PARA *p_para = (RES_QUERY_PARA *)user_data;
    struct tagEMULE_TASK* task = (struct tagEMULE_TASK*)p_para->_task_ptr;
	
	LOG_DEBUG("emule_notify_query_cdn_manager_callback:user_data=0x%X",user_data);
	#ifdef _DEBUG
	LOG_ERROR("emule_notify_query_cdn_manager_callback: errcode=%d,result=%d,is_end=%d,host=%s,port=%u",errcode,result,is_end,host,port);
	printf("\n emule_notify_query_cdn_manager_callback: errcode=%d,result=%d,is_end=%d,host=%s,port=%u\n",errcode,result,is_end,host,port);
	#endif

	sd_assert(user_data!=NULL);
	if(user_data==NULL) return INVALID_ARGUMENT;
	
	LOG_DEBUG( "emule_notify_query_cdn_manager_callback:errcode=%d,result=%d" ,
		errcode,result);

	if((errcode !=SUCCESS)||(result!=SUCCESS))
	{
		LOG_ERROR("MMMM Query cdn failed!");
        	task->_res_query_partner_cdn_state = RES_QUERY_FAILED;
	}
	else
	{
		if(is_end==TRUE)
		{
			LOG_ERROR("MMMM Query cdn success!");
       		 task->_res_query_partner_cdn_state = RES_QUERY_SUCCESS;
		}
		else
		{
			/* Add the resource to connect_manager */
			ret_val = sd_inet_aton(host, &host_ip);
			CHECK_VALUE(ret_val);

			cm_add_cdn_peer_resource(&(task->_task._connect_manager),0, NULL,   task->_data_manager->_gcid,task->_data_manager->_file_size, 0, host_ip,port,0,0,P2P_FROM_CDN);
			return SUCCESS;
		}
	}

	return SUCCESS;

}
#endif /* ENABLE_CDN */

_int32 emule_notify_query_partner_cdn_callback(void* user_data,_int32 errcode, _u8 result, _u32 retry_interval)
{
    RES_QUERY_PARA *p_para = (RES_QUERY_PARA *)user_data;
    struct tagEMULE_TASK* task = (struct tagEMULE_TASK*)p_para->_task_ptr;
        
    LOG_DEBUG("MMMM emule_notify_query_partner_cdn_callback, errcode = %d, result = %u.", errcode, (_u32)result);

    if( errcode == SUCCESS )
    {
        task->_res_query_partner_cdn_state = RES_QUERY_SUCCESS;
    }
    else
    {
        task->_res_query_partner_cdn_state = RES_QUERY_FAILED;
    }

    return SUCCESS;

}

_int32 emule_notify_query_dphub_result(void* user_data,_int32 errcode, _u16 result, _u32 retry_interval, BOOL is_continued)
{
    _int32 ret_val = 0;
    RES_QUERY_PARA *p_para = (RES_QUERY_PARA *)user_data;
    struct tagEMULE_TASK *p_task = (struct tagEMULE_TASK *)p_para->_task_ptr;

    LOG_DEBUG("MMMM emule_notify_query_dphub_result, errcode = %d, result = %u, retry_interval = %u.", 
        errcode, (_u32)result, retry_interval);

    if (errcode == SUCCESS)
    {
        if ((is_continued == TRUE) && (p_task->_task.task_status == TASK_S_RUNNING))
        {
            _int16 file_src_type = 1;   // 1: 普通完整文件
            _u8 file_idx_desc_type = 0; // 0：无块描述
            _u32 file_idx_content_cnt = 0;
            char *file_idx_content = NULL;

            ret_val = res_query_dphub(&p_task->_query_param, emule_notify_query_dphub_result, 
                file_src_type, 
                file_idx_desc_type,
                file_idx_content_cnt,
                file_idx_content, 
                p_task->_data_manager->_file_size, 
                p_task->_data_manager->_file_info._block_size, 
                (const char*)(p_task->_data_manager->_cid), 
                (const char*)(p_task->_data_manager->_gcid), 
                (_u16)EMULE_TASK_TYPE);

            p_task->_res_query_dphub_state = (ret_val != SUCCESS) ? RES_QUERY_FAILED : RES_QUERY_REQING;

        }
        else
        {
            p_task->_res_query_dphub_state = RES_QUERY_SUCCESS;
        }        
    }
    else
    {
        p_task->_res_query_dphub_state = RES_QUERY_FAILED;
    }

    return SUCCESS;
}

#endif /* ENABLE_EMULE */

