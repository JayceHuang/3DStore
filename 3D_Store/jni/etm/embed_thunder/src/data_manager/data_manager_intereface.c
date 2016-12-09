
#include "data_manager_interface.h"
#include "data_buffer.h"
#include "et_utility.h"

#include "p2sp_download/p2sp_task.h"

#include "utility/string.h"
#include "utility/utility.h"
#include "utility/url.h"
#include "platform/sd_fs.h"
#include "utility/sd_iconv.h"
#include "download_task/download_task.h"
#ifdef UPLOAD_ENABLE
#include "upload_manager/upload_manager.h"
#endif
#include "utility/logid.h"
#define LOGID LOGID_DATA_MANAGER
#include "utility/logger.h"

#include "p2p_data_pipe/p2p_pipe_interface.h"
#include "p2p_data_pipe/p2p_data_pipe.h"
#include "high_speed_channel/hsc_info.h"
#include "utility/settings.h"
#include "http_data_pipe/http_resource.h"
#include "ftp_data_pipe/ftp_data_pipe_interface.h"
#include "data_manager/data_manager_interface.h"

const _u32  MIN_NOT_BELIEVE_FILESIZE =  RANGE_DATA_UNIT_SIZE;

const _u32  MIN_BELIEVE_FILESIZE =  (1024*1024);

const _u32  VOD_ONCE_RANGE_NUM =  32*(64*1024/RANGE_DATA_UNIT_SIZE);

#ifdef VVVV_VOD_DEBUG
extern _u64 gloabal_recv_data = 0;

extern _u64 gloabal_invalid_data  = 0;

#endif

static BOOL g_is_section_download = FALSE;
#define MANAGER_DOWNLOAD_ONCE_NUM (1024*1024/16)
static BOOL g_data_manager_download_once_num = 0;
static ET_FILENAME_POLICY g_decision_filename_policy = ET_FILENAME_POLICY_DEFAULT_SMART;

static DM_NOTIFY_FILE_NAME_CHANGED g_notify_filename_changed_callback = NULL;
static DM_NOTIFY_EVENT g_notify_event_callback = NULL;
static EM_SCHEDULER g_notify_etm_scheduler = NULL;

static _int32 dm_get_origin_res_filename(DATA_MANAGER* p_data_manager_interface
    , char* p_str_name_buffer
    , int buffer_len);

static _int32 dm_create_empty_file(DATA_MANAGER* p_data_manager_interface);

_int32  init_data_manager_module()
{
    _int32 ret_val = SUCCESS;
    ret_val = get_data_manager_cfg_parameter();
    CHECK_VALUE(ret_val);

    ret_val = dm_create_slabs_and_init_data_buffer();
    CHECK_VALUE(ret_val);

    file_info_adjust_max_read_length();
    g_data_manager_download_once_num = MANAGER_DOWNLOAD_ONCE_NUM;
    settings_get_int_item( "data_manager.download_once_num", (_int32*)&(g_data_manager_download_once_num) );

    return ret_val;
}

_int32  uninit_data_manager_module()
{
    _int32 ret_val = SUCCESS;

    ret_val = dm_destroy_slabs_and_unit_data_buffer();
    CHECK_VALUE(ret_val);

    return ret_val;
}

_int32 get_data_manager_cfg_parameter()
{
    _int32 ret_val = SUCCESS;

    ret_val = get_data_buffer_cfg_parameter();
    CHECK_VALUE(ret_val);

    ret_val = get_correct_manager_cfg_parameter();
    CHECK_VALUE(ret_val);

    ret_val = get_data_receiver_cfg_parameter();
    CHECK_VALUE(ret_val);

    return ret_val;
}

_int32  dm_create_slabs_and_init_data_buffer()
{
    _int32 ret_val = SUCCESS;

    ret_val = init_range_data_buffer_slab();
    CHECK_VALUE(ret_val);

    ret_val = init_error_block_slab();
    CHECK_VALUE(ret_val);

    ret_val = dm_data_buffer_init();

    return (ret_val);

}

_int32  dm_destroy_slabs_and_unit_data_buffer()
{
    _int32 ret_val = SUCCESS;

    ret_val = destroy_range_data_buffer_slab();
    CHECK_VALUE(ret_val);

    ret_val = destroy_error_block_slab();
    CHECK_VALUE(ret_val);

    ret_val = dm_data_buffer_unint();

    return (ret_val);

}

_int32  dm_init_data_manager(DATA_MANAGER* p_data_manager_interface, struct _tag_task* p_task )
{

    FILE_INFO_CALLBACK  file_info_callback;

    LOG_DEBUG("dm_init_data_manager");

    file_info_callback._p_notify_create_event =  dm_notify_file_create_result ;
    file_info_callback._p_notify_close_event =  dm_notify_file_close_result ;
    file_info_callback._p_notify_check_event =  dm_notify_file_block_check_result ;
    file_info_callback._p_notify_file_result_event=  dm_notify_file_result ;
    file_info_callback._p_get_buffer_list_from_cache =  dm_get_buffer_list_from_cache ;


    init_file_info(&p_data_manager_interface->_file_info, p_data_manager_interface);

    file_info_register_callbacks(&p_data_manager_interface->_file_info, &file_info_callback);

    init_correct_manager(&p_data_manager_interface->_correct_manage);

    //init_data_checker(&p_data_manager_interface->_data_checker, p_data_manager_interface);
    init_range_manager(&p_data_manager_interface->_range_record);
    //data_receiver_init(&p_data_manager_interface->_data_receiver);

    //list_init(&p_data_manager_interface->_flush_buffer_list._data_list);

    p_data_manager_interface->_origin_res = NULL;
    p_data_manager_interface->_data_status_code = DATA_DOWNING;

    range_list_init(&p_data_manager_interface->part_3_cid_range);

    p_data_manager_interface->_only_use_orgin = FALSE;

    p_data_manager_interface->_has_bub_info = FALSE;
    p_data_manager_interface->_task_ptr = p_task;

    p_data_manager_interface->_waited_success_close_file= FALSE;
    p_data_manager_interface->_need_call_back= FALSE;

    sd_memset(p_data_manager_interface->_file_suffix, 0, MAX_SUFFIX_LEN);

    p_data_manager_interface->_IS_VOD_MOD= FALSE;
    p_data_manager_interface->_start_pos_index= 0;
    p_data_manager_interface->_conrinue_times = 0;
    p_data_manager_interface->_server_dl_bytes = 0;
    p_data_manager_interface->_peer_dl_bytes = 0;
    p_data_manager_interface->_cdn_dl_bytes = 0;
    p_data_manager_interface->_lixian_dl_bytes = 0;

    p_data_manager_interface->_server_recv_bytes = 0;
    p_data_manager_interface->_peer_recv_bytes = 0;
    p_data_manager_interface->_cdn_recv_bytes = 0;
    p_data_manager_interface->_lixian_recv_bytes = 0;

#ifdef _VOD_NO_DISK
    p_data_manager_interface->_is_no_disk = FALSE;
#endif

    p_data_manager_interface->_is_check_data = FALSE;

    p_data_manager_interface->_cur_range_step = 0;

    return SUCCESS;
}

_int32  dm_unit_data_manager(DATA_MANAGER* p_data_manager_interface)
{
    LOG_DEBUG("dm_unit_data_manager");

    unit_correct_manager(&p_data_manager_interface->_correct_manage);
    //unit_file_info(&p_data_manager_interface->_file_info);

    //unit_data_checker(&p_data_manager_interface->_data_checker);
    unit_range_manager(&p_data_manager_interface->_range_record);
    //data_receiver_unit(&p_data_manager_interface->_data_receiver);

    //drop_buffer_list(&p_data_manager_interface->_flush_buffer_list);


    range_list_clear(&p_data_manager_interface->part_3_cid_range);

    p_data_manager_interface->_only_use_orgin = TRUE;
    p_data_manager_interface->_has_bub_info = FALSE;

    sd_memset(p_data_manager_interface->_file_suffix, 0, MAX_SUFFIX_LEN);


    p_data_manager_interface->_start_pos_index= 0;
    p_data_manager_interface->_conrinue_times = 0;

    if(p_data_manager_interface->_need_call_back == FALSE 
		&& p_data_manager_interface->_waited_success_close_file == FALSE)
    {
        file_info_close_tmp_file(&p_data_manager_interface->_file_info);
        p_data_manager_interface->_need_call_back= TRUE;
    }
    else
    {
        p_data_manager_interface->_need_call_back= TRUE;
    }
    if(p_data_manager_interface->_data_status_code == VOD_DATA_FINISHED)
    {
        p_data_manager_interface->_waited_success_close_file= TRUE;
    }
#if defined(KANKAN_PROJ)
    /* KEEP VOD MODE */
#else
    p_data_manager_interface->_IS_VOD_MOD= FALSE;
#endif
    p_data_manager_interface->_data_status_code = DATA_UNINITED;
    //p_data_manager_interface->_task_ptr = NULL;

#ifdef _VOD_NO_DISK
    p_data_manager_interface->_is_no_disk = FALSE;
#endif

    p_data_manager_interface->_is_check_data = FALSE;

    return SUCCESS;
}


_int32  data_manager_notify_failure(DATA_MANAGER* p_data_manager_interface,  _int32 error_code)
{
    LOG_DEBUG("data_manager_notify_failure, errorcode: %d", error_code);
    dm_set_status_code( p_data_manager_interface,error_code);
    return SUCCESS;
}

_int32  data_manager_notify_success(DATA_MANAGER* p_data_manager_interface)
{
    BOOL ret_val = TRUE;
    _u8 cid[CID_SIZE];
    _u8 gcid[CID_SIZE];
    _int32 errcode =0;

//   char* final_name = NULL;

    LOG_DEBUG("data_manager_notify_success");

    if(file_info_cid_is_valid(&p_data_manager_interface->_file_info) == FALSE)
    {
        LOG_ERROR("data_manager_notify_success, cid is invalid");

        ret_val = get_file_3_part_cid(&p_data_manager_interface->_file_info, cid , &errcode);
        if(ret_val == FALSE)
        {
            LOG_ERROR("data_manager_notify_success, get cid failure");
            if(errcode == DM_CAL_CID_NO_BUFFER)
            {
                data_manager_notify_failure( p_data_manager_interface,DATA_CANOT_CHECK_CID_NOBUFFER);
            }
            else if(errcode == DM_CAL_CID_READ_ERROR)
            {
                data_manager_notify_failure( p_data_manager_interface,DATA_CANOT_CHECK_CID_READ_ERROR);
            }
            else
            {
                data_manager_notify_failure( p_data_manager_interface,DATA_CAN_NOT_GET_CID);
            }
            return SUCCESS;
        }

        LOG_DEBUG("data_manager_notify_success, set cid");
        // dm_set_cid(p_data_manager_interface, cid) ;
        file_info_set_cid(&p_data_manager_interface->_file_info,cid);

    }
    else
    {
     	 BOOL read_file_error = FALSE;
        LOG_DEBUG("data_manager_notify_success, cid is valid, but will  check cid");
        char file_calc_cid[CID_SIZE];
		sd_memset(file_calc_cid,0,sizeof(file_calc_cid));
        ret_val = file_info_check_cid(&p_data_manager_interface->_file_info, &errcode, &read_file_error, file_calc_cid);
	 LOG_DEBUG("data_manager_notify_success, check result:%d, but will not set taskfailed...read_file_error:%d", ret_val, read_file_error);

	 if(!ret_val && !read_file_error)
	 {
	 	TASK* task_ptr = p_data_manager_interface->_task_ptr;
	 	if(NULL != task_ptr)
	 	{
	 		sd_memcpy(p_data_manager_interface->_file_info._cid, file_calc_cid, CID_SIZE);
	 		task_ptr->_is_shub_cid_error = TRUE;
	 		LOG_DEBUG("data_manager_notify_success, check cid failed... need report.. _is_shub_cid_error = TRUE");
	 	}
	 }
	
	 // 必须读 cid失败 才 设置 任务失败...
        if(!ret_val && read_file_error)
        {
            LOG_ERROR("data_manager_notify_success, cid is valid, but check cid fail!");
            if(errcode == DM_CAL_CID_NO_BUFFER)
            {
                data_manager_notify_failure( p_data_manager_interface,DATA_CANOT_CHECK_CID_NOBUFFER);
            }
            else if(errcode == DM_CAL_CID_READ_ERROR)
            {
                data_manager_notify_failure( p_data_manager_interface,DATA_CANOT_CHECK_CID_READ_ERROR);
            }
            else
            {
                data_manager_notify_failure( p_data_manager_interface,DATA_CHECK_CID_ERROR);
            }
            return SUCCESS;
        }
    }

    if(file_info_gcid_is_valid(&p_data_manager_interface->_file_info)== FALSE)
    {
        LOG_ERROR("data_manager_notify_success, gcid is invalid");
        ret_val = get_file_gcid(&p_data_manager_interface->_file_info, gcid);
        if(ret_val == FALSE)
        {
            LOG_ERROR("data_manager_notify_success, gcid is invalid ,but get gcid fail.");
            data_manager_notify_failure( p_data_manager_interface,DATA_CAN_NOT_GET_GCID);
            return SUCCESS;
        }
        LOG_ERROR("data_manager_notify_success, gcid is invalid ,get gcid succ, so set gcid.");
        file_info_set_gcid(&p_data_manager_interface->_file_info, gcid) ;

    }
    else
    {
        LOG_DEBUG("data_manager_notify_success, gcid is valid, so check gcid");
        ret_val = file_info_check_gcid( &p_data_manager_interface->_file_info);
        if(ret_val == FALSE)
        {
            LOG_ERROR("data_manager_notify_success, gcid is valid, but check gcid fail!");
            data_manager_notify_failure( p_data_manager_interface,DATA_CHECK_GCID_ERROR);
            return SUCCESS;
        }
    }

    if(p_data_manager_interface->_IS_VOD_MOD == FALSE)
    {
        if(p_data_manager_interface->_need_call_back == FALSE 
            && p_data_manager_interface->_waited_success_close_file == FALSE)
        {
            //if(p_data_manager_interface->_file_info._write_mode == FWM_BLOCK_NEED_ORDER)
            //{
            /* 将文件内容从乱序调整为顺序 */

            //}
            //else
            //{
            p_data_manager_interface->_waited_success_close_file= TRUE;

            file_info_close_tmp_file(&p_data_manager_interface->_file_info);
            //}
        }
        else
        {
            p_data_manager_interface->_waited_success_close_file= TRUE;
        }
    }
    else
    {
        dm_set_status_code( p_data_manager_interface,VOD_DATA_FINISHED);
    }



    /*    file_info_delete_cfg_file(&p_data_manager_interface->_file_info);

        LOG_DEBUG("data_manager_notify_success, delette cfg file.");

      final_name =  file_info_get_finish_filename(&p_data_manager_interface->_file_info);

        LOG_DEBUG("data_manager_notify_success, get final filename:%s ", final_name);

        data_check_change_finish_filename(&p_data_manager_interface->_data_checker, final_name);

    LOG_DEBUG("data_manager_notify_success, change final name:%s ", final_name);

        dm_set_status_code( p_data_manager_interface,DATA_SUCCESS); */
    return SUCCESS;
}


void  data_manager_notify_file_create_info(DATA_MANAGER* p_data_manager_interface, BOOL creat_success)
{
    LOG_DEBUG("data_manager_notify_file_create_info, data_manager:0x%x, success: %u", p_data_manager_interface, (_u32)creat_success);

    // if(creat_success == TRUE)
    //   {
    //          dm_save_to_cfg_file(p_data_manager_interface);
    //    }

    //  dt_notify_file_creating_result_event(p_data_manager_interface->_task_ptr, creat_success);
    pt_notify_file_creating_result_event(p_data_manager_interface->_task_ptr, creat_success);
}


void  data_manager_notify_file_close_info(DATA_MANAGER* p_data_manager_interface, _int32 close_result)
{

    _int32 ret = SUCCESS;
    LOG_ERROR("data_manager_notify_file_close_info, data_manager:0x%x, (waite_success, need_callback) (%u,%u), close_result: %d",
              p_data_manager_interface, (_u32)p_data_manager_interface->_waited_success_close_file,(_u32)p_data_manager_interface->_need_call_back,  close_result);

    if(p_data_manager_interface->_waited_success_close_file == TRUE)
    {
        LOG_ERROR("data_manager_notify_file_close_info, data_manager:0x%x, close_result: %d", p_data_manager_interface, close_result);

        if(p_data_manager_interface->_file_info._write_mode == FWM_RANGE)
        {
            sd_assert(p_data_manager_interface->_file_info._write_mode == FWM_RANGE);
            file_info_change_td_name(&p_data_manager_interface->_file_info);

            LOG_ERROR("data_manager_notify_file_close_info, delette cfg file.");

            ret = file_info_delete_cfg_file(&p_data_manager_interface->_file_info);

            LOG_ERROR("data_manager_notify_file_close_info, delete cfg file. ret = %d", ret);
            file_info_decide_finish_filename(&p_data_manager_interface->_file_info);

        }
        else
        {
            file_info_close_cfg_file(&p_data_manager_interface->_file_info);
        }

        //LOG_DEBUG("data_manager_notify_success, get final filename:%s ", final_name);

        //          LOG_DEBUG("data_manager_notify_success, change final name:%s ", final_name);

        p_data_manager_interface->_waited_success_close_file = FALSE;

        if(p_data_manager_interface->_data_status_code != DATA_UNINITED)
        {
            if(p_data_manager_interface->_file_info._write_mode==FWM_BLOCK)
            {
                p_data_manager_interface->_file_info._finished = TRUE;
            }
            else
            {
                dm_set_status_code( p_data_manager_interface,DATA_SUCCESS);
            }
        }



    }

    if(p_data_manager_interface->_need_call_back == TRUE)
    {
        unit_file_info(&p_data_manager_interface->_file_info);
        p_data_manager_interface->_need_call_back = FALSE;
//            dt_notify_file_closing_result_event(p_data_manager_interface->_task_ptr, close_result);
        pt_notify_file_closing_result_event(p_data_manager_interface->_task_ptr, close_result);

    }


    /*
          if( p_data_manager_interface->_need_call_back == TRUE && p_data_manager_interface->_waited_success_close_file == TRUE)
             {
                   p_data_manager_interface->_waited_success_close_file = FALSE;
             }
          else if( p_data_manager_interface->_need_call_back == TRUE && p_data_manager_interface->_waited_success_close_file == FALSE)
          {
                  p_data_manager_interface->_need_call_back = FALSE;
    //            dt_notify_file_closing_result_event(p_data_manager_interface->_task_ptr, close_result);
                  pt_notify_file_closing_result_event(p_data_manager_interface->_task_ptr, close_result);
          }
          else if( p_data_manager_interface->_need_call_back == FALSE && p_data_manager_interface->_waited_success_close_file == TRUE)
          {
                      LOG_DEBUG("data_manager_notify_file_close_info, data_manager:0x%x, close_result: %d", p_data_manager_interface, close_result);

                      file_info_change_td_name(&p_data_manager_interface->_file_info);

                      LOG_DEBUG("data_manager_notify_success, delette cfg file.");

                file_info_delete_cfg_file(&p_data_manager_interface->_file_info);

                 file_info_decide_finish_filename(&p_data_manager_interface->_file_info);

                   //LOG_DEBUG("data_manager_notify_success, get final filename:%s ", final_name);



                     //          LOG_DEBUG("data_manager_notify_success, change final name:%s ", final_name);

                       dm_set_status_code( p_data_manager_interface,DATA_SUCCESS);

                 p_data_manager_interface->_waited_success_close_file = FALSE;
              }
          else
          {
                sd_assert(FALSE);
          }

             unit_file_info(&p_data_manager_interface->_file_info);
    */
}




_int32 dm_set_file_info( DATA_MANAGER* p_data_manager_interface,char* filename, char*filedir, char* originurl, char* originrefurl)
{
    _int32 ret_val = SUCCESS;

    LOG_DEBUG("dm_set_file_info, filename:%s, filepath:%s, originurl:%s, refurl:%s", filename==NULL?"":filename, filedir==NULL?"":filedir, originurl==NULL?"":originurl, originrefurl==NULL?"":originrefurl);
    ret_val  = file_info_set_user_name(&p_data_manager_interface->_file_info, filename, filedir);
    CHECK_VALUE(ret_val);

    LOG_DEBUG("dm_set_file_info,  originurl:%s, refurl:%s",  originurl==NULL?"":originurl, originrefurl==NULL?"":originrefurl);

    ret_val  = file_info_set_origin_url(&p_data_manager_interface->_file_info, originurl, originrefurl);
    return ret_val;
}

_int32 dm_set_origin_url_info( DATA_MANAGER* p_data_manager_interface, char* originurl, char* originrefurl)
{
    _int32 ret_val = SUCCESS;

    LOG_DEBUG("dm_set_origin_url_info,  originurl:%s, refurl:%s",  originurl==NULL?"":originurl, originrefurl==NULL?"":originrefurl);

    ret_val  = file_info_set_origin_url(&p_data_manager_interface->_file_info, originurl, originrefurl);
    return ret_val;
}

_int32 dm_set_cookie_info( DATA_MANAGER* p_data_manager_interface, char* cookie)
{
    _int32 ret_val = SUCCESS;

    LOG_DEBUG("dm_set_cookie_info,  cookie:%s", cookie==NULL?"":cookie);

    ret_val  = file_info_set_cookie(&p_data_manager_interface->_file_info, cookie);
    return ret_val;
}

BOOL dm_get_origin_url( DATA_MANAGER* p_data_manager_interface,char** origin_url)
{
    return file_info_get_origin_url(&p_data_manager_interface->_file_info, origin_url);
}

BOOL dm_get_origin_ref_url( DATA_MANAGER* p_data_manager_interface,char** origin_ref_url)
{
    return file_info_get_origin_ref_url(&p_data_manager_interface->_file_info,origin_ref_url);
}

BOOL dm_get_file_path( DATA_MANAGER* p_data_manager_interface,char** file_path)
{
    return file_info_get_file_path(&p_data_manager_interface->_file_info,file_path);
}

BOOL dm_decide_and_get_file_name( DATA_MANAGER* p_data_manager_interface,char** filename)
{
    _int32 ret_val = SUCCESS;
    char* ret_filename = NULL;

    LOG_DEBUG("dm_decide_and_get_file_name");

    /*if(sd_strlen(p_data_manager_interface->_file_info._file_name) == 0)
    {
           ret_val = dm_decide_filename(p_data_manager_interface);

               CHECK_VALUE(ret_val);
    }*/

    if(file_info_get_file_name(&p_data_manager_interface->_file_info, &ret_filename) == FALSE)
    {
        ret_val = dm_decide_filename(p_data_manager_interface);

        if(ret_val != SUCCESS)
        {
            return FALSE;
        }

        return   file_info_get_file_name(&p_data_manager_interface->_file_info, filename);
    }
    else
    {
        *filename = ret_filename;
        return TRUE;
    }


    /* if(p_data_manager_interface->_file_info._file_name!= NULL)
     {
           LOG_DEBUG("dm_get_file_name, get file name:%s .", p_data_manager_interface->_file_info._file_name);
          *filename = p_data_manager_interface->_file_info._file_name;
        return TRUE;
     }
    else
    {
           LOG_ERROR("dm_get_file_name, get file name fail.");
          *filename = NULL;
       return FALSE;
    }*/

}

_int32 dm_set_file_suffix(DATA_MANAGER* p_data_manager_interface, char *file_suffix)
{
    return sd_memcpy(p_data_manager_interface->_file_suffix,file_suffix, MAX_SUFFIX_LEN);
}

char* dm_get_file_suffix(DATA_MANAGER* p_data_manager_interface)
{
    return p_data_manager_interface->_file_suffix;
}

PIPE_FILE_TYPE  dm_get_file_type( DATA_MANAGER* p_data_manager_interface )
{
    char tmp_suffix[MAX_SUFFIX_LEN];

    if(sd_strlen(p_data_manager_interface->_file_suffix) == 0)
    {

        return PIPE_FILE_UNKNOWN_TYPE;
    }

    tmp_suffix[0]='.';

    tmp_suffix[1]='\0';

    sd_memcpy(tmp_suffix +1 , p_data_manager_interface->_file_suffix, MAX_SUFFIX_LEN -1);

    if(sd_is_binary_file(tmp_suffix, NULL) == TRUE)
    {
        return PIPE_FILE_BINARY_TYPE;
    }
    else
    {
        return PIPE_FILE_HTML_TYPE;
    }

}

BOOL dm_get_filnal_file_name( DATA_MANAGER* p_data_manager_interface,char** filename)
{
    return  file_info_get_file_name(&p_data_manager_interface->_file_info, filename);
}

static _int32 dm_get_origin_res_filename(DATA_MANAGER* p_data_manager_interface
    , char* p_str_name_buffer
    , int buffer_len)
{
    RESOURCE* p_res = p_data_manager_interface->_origin_res;
    if(p_res == NULL )
	{
		LOG_DEBUG( " No origin resource,don't know what to do!  ");
		return FALSE;
	}

	if( p_res->_resource_type == HTTP_RESOURCE )
	{
		return http_resource_get_file_name(p_res,p_str_name_buffer,buffer_len,FALSE);
	}
	else if( p_res->_resource_type == FTP_RESOURCE )
	{
		return ftp_resource_get_file_name(p_res,p_str_name_buffer,buffer_len,FALSE);			
	}
	return FALSE;
}

BOOL file_extension_is_same(char *file_name1, char *file_name2)
{
	if (!file_name1 || !file_name2) return FALSE;
	char *ext1 = sd_strrchr(file_name1, '.');
	char *ext2 = sd_strrchr(file_name2, '.');
	if (!ext1 || !ext2) return FALSE;
	return sd_stricmp(ext1, ext2)==0;
}

_int32 dm_set_decision_filename_policy(ET_FILENAME_POLICY namepolicy)
{
    g_decision_filename_policy = namepolicy;

    return SUCCESS;
}

static ET_FILENAME_POLICY dm_get_decision_filename_policy(void)
{
    return g_decision_filename_policy ;
}


_int32 dm_decide_filename(DATA_MANAGER* p_data_manager_interface)
{
    char finalname[MAX_FILE_NAME_LEN] = {0};
    char new_name[MAX_FILE_NAME_LEN] = {0};
    char filename_from_url[MAX_FILE_NAME_LEN] = {0};
    char full_path_filename[MAX_FILE_NAME_LEN + MAX_FILE_PATH_LEN] = {0};
    char td_full_path_filename[MAX_FILE_NAME_LEN + MAX_FILE_PATH_LEN + MAX_TD_CFG_SUFFIX_LEN] = {0};
    char num_str[MAX_CFG_VALUE_LEN]= {0};

    char* user_file_name = NULL;
    char* user_file_path = NULL;

    char* poffix_pos =  NULL;
    char* new_poffix =  NULL;
    BOOL bool_ret =  FALSE;
    _u32 name_len = 0;
    _u32 file_path_len = 0;
    _u32 num = 0;

    char* p_origin_url = NULL;
    _u32 origin_len = 0;

    char* pos2 =NULL;
    _u32 num_len = 0;
    _u32 pos_len = 0;

    LOG_DEBUG("dm_decide_filename");

    num_str[0] = '_';

    file_info_get_user_file_name(&p_data_manager_interface->_file_info, &user_file_name);

    if (user_file_name != NULL)
    {
        name_len = sd_strlen(user_file_name);
    }
    else
    {
        name_len = 0;
    }

    if (name_len>0)
    {
		ET_FILENAME_POLICY namepolicy;
        LOG_DEBUG("dm_decide_filename, user name :%s .", user_file_name);

        /* 修改MIUI添加后缀名问题 */
        namepolicy = dm_get_decision_filename_policy();
        if (ET_FILENAME_POLICY_SIMPLE_USER == namepolicy)
        {
            sd_strncpy(finalname, user_file_name, MIN(sd_strlen(user_file_name), MAX_FILE_NAME_LEN));
    		finalname[MIN(sd_strlen(user_file_name), MAX_FILE_NAME_LEN)] = '\0';
        }
        else if (ET_FILENAME_POLICY_DEFAULT_SMART == namepolicy)
        {
            char *url = p_data_manager_interface->_file_info._origin_url;
            char str_temp[MAX_FILE_NAME_BUFFER_LEN];
    	 
            if (et_get_file_name_from_url(url, sd_strlen(url), str_temp, MAX_FILE_NAME_BUFFER_LEN))
            {
                sd_memcpy(filename_from_url, str_temp, sd_strlen(str_temp));
            }
            
            //先判断用户指定的文件名是否是地址中的文件，如果是则可能是我们程序算出来的
            // 如果发生重定向，看重定向后的url是否一个好的url        
            BOOL new_name_is_ok = dm_get_origin_res_filename(p_data_manager_interface, new_name, MAX_FILE_NAME_LEN);
            if (new_name_is_ok) 
                new_name_is_ok = is_excellent_filename(new_name);
                
            if( (sd_stricmp(filename_from_url, user_file_name)==0 && new_name_is_ok) ||
            	(new_name_is_ok && !file_extension_is_same(user_file_name, new_name)))
            {        
                LOG_DEBUG("dm_get_origin_res_filename, new_name : %s is good name", new_name);
                name_len = sd_strlen(new_name);
                if(name_len >= MAX_FILE_NAME_LEN)
                {
                    name_len = MAX_FIELD_NAME_LEN - 4;
                }


	        char* new_name_suffix = get_file_suffix(new_name);
            _u32  suffix_len = sd_strlen(new_name_suffix);
            
	        if(NULL == new_name_suffix || suffix_len == 0 || suffix_len >= (MAX_FIELD_NAME_LEN - 4))
	        {
            		sd_strncpy(finalname, new_name, MAX_FILE_NAME_LEN);
            		finalname[name_len] = '\0';
	        }
            else
            {
			sd_strncpy(finalname, user_file_name, MAX_FILE_NAME_LEN - suffix_len - 1); 
			if(sd_stristr(finalname, new_name_suffix, 0) == NULL)
			{
				sd_strcat(finalname, new_name_suffix, suffix_len);
			}
            }
        }
        else if(TRUE == is_excellent_filename(user_file_name)) //优先用用户指定文件名
        {
            LOG_DEBUG("dm_decide_filename, user name :%s  is good name.", user_file_name);
            if(name_len >= MAX_FILE_NAME_LEN)
            {
                name_len = MAX_FILE_NAME_LEN-4;
            }

            sd_strncpy(finalname, user_file_name, MAX_FILE_NAME_LEN);
            finalname[name_len] = '\0';
            char* new_name_suffix = get_file_suffix(new_name);
            char *user_name_suffix = get_file_suffix(finalname);
            //user file name has not extension
            if (!user_name_suffix && new_name_suffix)
            {
                //ensure buffer is enough
                if (sizeof(finalname)>sd_strlen(finalname)+sd_strlen(new_name_suffix))
                {
                    sd_strcat(finalname, new_name_suffix, sd_strlen(new_name_suffix));
                }
            }
            else if (user_name_suffix && new_name_suffix && sd_strcmp(new_name_suffix, user_name_suffix)!=0 )
            {
                //ensure buffer is enough
                if (sizeof(finalname)>sd_strlen(finalname)+sd_strlen(new_name_suffix)-sd_strlen(user_name_suffix))
                {
                    sd_strncpy(user_name_suffix, new_name_suffix, sd_strlen(new_name_suffix));
                }
            }
        }		
	else 
        {
            LOG_DEBUG("dm_decide_filename, user name :%s  is bad name.", user_file_name);
            // bool_ret= cm_get_excellent_filename(&p_data_manager_interface->_task_ptr->_connect_manager, new_name,MAX_FILE_NAME_LEN);
            bool_ret= pt_get_excellent_filename(p_data_manager_interface->_task_ptr, new_name,MAX_FILE_NAME_LEN);
            if(bool_ret == TRUE)
            {
                LOG_DEBUG("dm_decide_filename, get good name :%s .", new_name);

                if((TRUE == is_excellent_filename(new_name)))
                {
                    poffix_pos =  sd_strrchr(user_file_name, '.');
                    if(poffix_pos != NULL)
                    {

                        name_len = poffix_pos - user_file_name;
                    }

                    if(name_len >= MAX_FILE_NAME_LEN)
                    {
                        name_len = MAX_FILE_NAME_LEN-4;
                    }

                    sd_strncpy(finalname, user_file_name, MAX_FILE_NAME_LEN);

                    new_poffix =  get_file_suffix(new_name);
                    if(new_poffix != NULL)
                    {
                        LOG_DEBUG("dm_decide_filename, get good posi :%s .", new_poffix);

                        sd_strncpy(finalname+name_len, new_poffix, MAX_FILE_NAME_LEN-name_len);

                        name_len += sd_strlen(new_poffix);
                    }
                    /*else
                    {

                    }*/


                    finalname[name_len] = '\0';
                }
                else
                {
                    LOG_DEBUG("dm_decide_filename,  get good name is no good , so use user name:%s .", user_file_name);

                    sd_strncpy(finalname, user_file_name, MAX_FILE_NAME_LEN);
                    finalname[name_len] = '\0';

                    if((FALSE == is_excellent_filename(finalname)))
                    {
                        new_poffix =  get_file_suffix(finalname);
                        //poffix_pos = dm_get_file_suffix(p_data_manager_interface->_task_ptr);
                        poffix_pos = p_data_manager_interface->_file_suffix;

                        if((poffix_pos!=NULL)&&(poffix_pos[0]!='\0'))
                        {
                            if(new_poffix != NULL)
                            {
                                new_poffix[0]='\0';
                            }

                            sd_strcat(finalname, ".", sd_strlen("."));
                            sd_strcat(finalname, poffix_pos, sd_strlen(poffix_pos));
                        }
                    }
                }
            }
            else
            {
                LOG_DEBUG("dm_decide_filename, can not get good name , so use user name:%s .", user_file_name);

                sd_strncpy(finalname, user_file_name, MAX_FILE_NAME_LEN);
                finalname[name_len] = '\0';

                if((FALSE == is_excellent_filename(finalname)))
                {
                    new_poffix =  get_file_suffix(finalname);
                    //poffix_pos = dm_get_file_suffix(p_data_manager_interface);
                    poffix_pos = p_data_manager_interface->_file_suffix;

                    if((poffix_pos!=NULL)&&(poffix_pos[0]!='\0'))
                    {
                        if(new_poffix != NULL)
                        {
                            new_poffix[0]='\0';
                        }

                            sd_strcat(finalname, ".", sd_strlen("."));
                            sd_strcat(finalname, poffix_pos, sd_strlen(poffix_pos));
                        }
                    }
                }
            }
        }
        else
        {
            //todo other name policy
        }
    }
    else
    {
        LOG_DEBUG("dm_decide_filename, no user name .");

        BOOL new_name_is_ok =  dm_get_origin_res_filename(p_data_manager_interface, new_name, MAX_FILE_NAME_LEN);

	 LOG_DEBUG("dm_decide_filename, no user name .new_name_is_ok=%d, new_name=%s", new_name_is_ok, new_name);
	 if(new_name_is_ok)
	 {
         	new_name_is_ok =  is_excellent_filename(new_name) ;
        }

	 LOG_DEBUG("dm_decide_filename, no user name .new_name_is_ok=%d, new_name=%s", new_name_is_ok, new_name);  
	 
 	 bool_ret= new_name_is_ok;
 	 if(!bool_ret)
 	 {
 	 	bool_ret = pt_get_excellent_filename(p_data_manager_interface->_task_ptr, new_name,MAX_FILE_NAME_LEN);;
 	 }
      	 
        if(bool_ret == TRUE)
        {
            LOG_DEBUG("dm_decide_filename, no user name , get good name:%s.", new_name);

            name_len = sd_strlen(new_name);
            sd_strncpy(finalname, new_name, MAX_FILE_NAME_LEN);
            finalname[name_len] = '\0';
        }
        else
        {
            LOG_ERROR("dm_decide_filename, no user name , but can not get good name.");
            dm_get_origin_url(p_data_manager_interface, &p_origin_url);

            if(p_origin_url == NULL)
            {
                LOG_ERROR("dm_decide_filename, no user name , but can not get origin url , so can not decide filename");
                return CANNOT_GET_FILE_NAME;
            }
            origin_len = sd_strlen(p_origin_url);
            if(origin_len == 0)
            {
                LOG_ERROR("dm_decide_filename, no user name , the origin url len is 0 , so can not decide filename");
                return CANNOT_GET_FILE_NAME;
            }


            if(sd_get_file_name_from_url(p_origin_url, origin_len, finalname, MAX_FILE_NAME_LEN)<0)
            {
                LOG_ERROR("dm_decide_filename, no user name , but can not get filename from origin_url :%s", p_origin_url);
                return CANNOT_GET_FILE_NAME;
            }

            if((FALSE == is_excellent_filename(finalname)))
            {
                new_poffix =  get_file_suffix(finalname);
                //poffix_pos = dm_get_file_suffix(p_data_manager_interface);
                poffix_pos = p_data_manager_interface->_file_suffix;

                if(poffix_pos[0]!='\0')
                {
                    if(new_poffix != NULL)
                    {
                        new_poffix[0]='\0';
                    }

                    sd_strcat(finalname, ".", sd_strlen("."));
                    sd_strcat(finalname, poffix_pos, sd_strlen(poffix_pos));
                }

            }

        }
#ifdef UTF8_NAME
        {
            char utf8_name[MAX_FILE_NAME_LEN];
            _u32 utf8_len = MAX_FILE_NAME_LEN;
            _int32 ret_val = 0;
            enum CODE_PAGE code_page = sd_conjecture_code_page( finalname);
            name_len = sd_strlen(finalname);

            if(_CP_GBK ==code_page)
            {
                ret_val =  sd_gbk_2_utf8(finalname, name_len, utf8_name, &utf8_len);
                if(ret_val == SUCCESS && utf8_len != 0 && utf8_len<MAX_FILE_NAME_LEN)
                {
                    utf8_name[utf8_len] = '\0';
                    LOG_DEBUG("dm_decide_filename, 1 get ut8 name:%s.", utf8_name);
                    sd_strncpy(finalname, utf8_name,  utf8_len);
                }
                else
                {
                    utf8_len = MAX_FILE_NAME_LEN;
                    ret_val =  sd_big5_2_utf8(finalname, name_len, utf8_name, &utf8_len);
                    if(ret_val == SUCCESS && utf8_len != 0 && utf8_len<MAX_FILE_NAME_LEN)
                    {
                        utf8_name[utf8_len] = '\0';
                        LOG_DEBUG("dm_decide_filename, 2 get ut8 name:%s.", utf8_name);
                        sd_strncpy(finalname, utf8_name,  utf8_len);
                    }
                }

                /* remove all the '%' */
                sd_decode_file_name(finalname,NULL, MAX_FILE_NAME_LEN);

            }
            else if(_CP_BIG5 ==code_page)
            {
                ret_val =  sd_big5_2_utf8(finalname, name_len, utf8_name, &utf8_len);
                if(ret_val == SUCCESS && utf8_len != 0 && utf8_len<MAX_FILE_NAME_LEN)
                {
                    utf8_name[utf8_len] = '\0';
                    LOG_DEBUG("dm_decide_filename, 3 get ut8 name:%s.", utf8_name);
                    sd_strncpy(finalname, utf8_name,  utf8_len);
                }
                else
                {
                    utf8_len = MAX_FILE_NAME_LEN;
                    ret_val =  sd_gbk_2_utf8(finalname, name_len, utf8_name, &utf8_len);
                    if(ret_val == SUCCESS && utf8_len != 0 && utf8_len<MAX_FILE_NAME_LEN)
                    {
                        utf8_name[utf8_len] = '\0';
                        LOG_DEBUG("dm_decide_filename, 4 get ut8 name:%s.", utf8_name);
                        sd_strncpy(finalname, utf8_name,  utf8_len);
                    }
                }

                /* remove all the '%' */
                sd_decode_file_name(finalname,NULL);

            }
        }
#endif


    }

    LOG_DEBUG("dm_decide_filename, set final name:%s.", finalname);

    file_info_get_file_path(&p_data_manager_interface->_file_info, &user_file_path);

    file_path_len = sd_strlen(user_file_path);

    name_len = sd_strlen(finalname);

    sd_strncpy(full_path_filename, user_file_path, MAX_FILE_NAME_LEN + MAX_FILE_PATH_LEN);

    sd_strncpy(full_path_filename+file_path_len, finalname, MAX_FILE_NAME_LEN + MAX_FILE_PATH_LEN -file_path_len );

    full_path_filename[name_len + file_path_len] = '\0';

    sd_strncpy(td_full_path_filename, full_path_filename, MAX_FILE_NAME_LEN + MAX_FILE_PATH_LEN);

    sd_strncpy(td_full_path_filename+name_len + file_path_len,".rf", 3);

    td_full_path_filename[name_len + file_path_len+3] = '\0';

    if(sd_file_exist(full_path_filename) == TRUE  || sd_file_exist(td_full_path_filename) == TRUE)
    {
        LOG_DEBUG("dm_decide_filename, full_path_filename:%s or td file :%s exist, so change name..", full_path_filename, td_full_path_filename);

        pos2 = sd_strrchr(finalname, '.');
        if(pos2 == NULL)
        {
            pos_len = name_len;
        }
        else
        {
            pos_len = pos2 - finalname;
        }

        while(1)
        {
            sd_snprintf(num_str+1,MAX_CFG_VALUE_LEN-1,"%u",num);
            num_len = sd_strlen(num_str);
            if(file_path_len+pos_len + num_len >=  MAX_FILE_NAME_LEN + MAX_FILE_PATH_LEN )
            {
                LOG_ERROR("dm_decide_filename, file name is too long,no supprt .");
                return CANNOT_GET_FILE_NAME;
            }
            sd_strncpy(full_path_filename+file_path_len+pos_len,  num_str, num_len);
            if(pos2 != NULL)
            {
                sd_strncpy(full_path_filename+file_path_len+pos_len+num_len,  pos2, name_len-pos_len);
            }

            full_path_filename[file_path_len+name_len+num_len] = '\0';

            sd_strncpy(td_full_path_filename, full_path_filename, name_len + file_path_len + num_len);

            sd_strncpy(td_full_path_filename + name_len + file_path_len + num_len,".rf", 3);

            td_full_path_filename[name_len + file_path_len + num_len +3] = '\0';

            if(sd_file_exist(full_path_filename) == TRUE  || sd_file_exist(td_full_path_filename) == TRUE)
            {
                LOG_DEBUG("dm_decide_filename, num:%u, full_path_filename:%s  or td file:%s exist, so continue change name..", num,full_path_filename, td_full_path_filename);
                num++;
            }
            else
            {
                LOG_DEBUG("dm_decide_filename, num:%u, full_path_filename:%s  is not exist, so is final name..", num,full_path_filename);
                break;
            }
        }

        sd_strncpy(finalname, full_path_filename+file_path_len,name_len+num_len);
        finalname[name_len+num_len] = '\0';

        LOG_DEBUG("dm_decide_filename, num:%u, after change name,the final file name is %s .", num,finalname);

    }

    LOG_DEBUG("dm_decide_filename, num:%u, before set final name :%s  .", num,finalname);

    name_len = sd_strlen(finalname);
    sd_strncpy(finalname+name_len,".rf", 3);
    finalname[name_len+3] = '\0';

    LOG_DEBUG("dm_decide_filename, add td,  before set final name :%s  .", finalname);

    file_info_set_final_file_name(&p_data_manager_interface->_file_info, finalname);

    file_info_set_td_flag(&p_data_manager_interface->_file_info);

    return SUCCESS;

}

_int32 dm_create_empty_file(DATA_MANAGER* p_data_manager_interface)
{
    _int32 ret_val = SUCCESS;
    char *file_path = NULL;
    char *file_name = NULL;
    char full_file_path[MAX_FILE_NAME_LEN + MAX_FILE_PATH_LEN] = {0};
    char szOriName[MAX_FILE_NAME_LEN + MAX_FILE_PATH_LEN] = {0};
	
    _int32 flag = 0;
    flag |= O_FS_CREATE;
    _u32 file_id = INVALID_FILE_ID;
    int nDotOffset = 0;
    int j = 0;
    
    dm_decide_filename(p_data_manager_interface);
    if (file_info_get_file_path(&p_data_manager_interface->_file_info, &file_path))
    {
        sd_strncpy(full_file_path, file_path, sd_strlen(file_path) );
        file_info_decide_finish_filename(&p_data_manager_interface->_file_info);
        if (file_info_get_file_name(&p_data_manager_interface->_file_info, &file_name))
        {
            sd_strcat(full_file_path, file_name, sd_strlen(file_name) );
        }
    }
    else
    {
        
        return SD_ERROR;
    }
    
    LOG_DEBUG("dm_create_empty_file, full_file_path = %s", full_file_path);
	
    if (sd_file_exist(full_file_path))
	{
		strcpy(szOriName, full_file_path);
		int i = strlen(szOriName);
		while (i > 0)
		{
			if (szOriName[i] == '.')
			{
				nDotOffset = i;
				break;
			}
			i--;
		}

		while (sd_file_exist(full_file_path))
	    {
			sd_memset(full_file_path, 0, MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN);
			strcpy(full_file_path, szOriName);
			if (j < 10)
			{
				strcpy(full_file_path + nDotOffset + 2, szOriName + nDotOffset);
				full_file_path[nDotOffset] = '_';
				full_file_path[nDotOffset + 1] = j + '0';
			}
			else if(j >= 10 && j < 100)
			{
				strcpy(full_file_path + nDotOffset + 3, szOriName + nDotOffset);
				full_file_path[nDotOffset] = '_';
				full_file_path[nDotOffset + 1] = j / 10 + '0';
				full_file_path[nDotOffset + 2] = j % 10 + '0';
			}
			else if(j >= 100 && j < 1000)
			{
				int nTemp = j;
				strcpy(full_file_path + nDotOffset + 4, szOriName + nDotOffset);
				full_file_path[nDotOffset] = '_';
				full_file_path[nDotOffset + 1] = nTemp / 100 + '0';
				nTemp = nTemp / 10;
				full_file_path[nDotOffset + 2] = nTemp / 10 + '0';
				full_file_path[nDotOffset + 3] = nTemp % 10 + '0';
			}

			j++;

	    }
	}
    
    ret_val = sd_open_ex(full_file_path, flag, &file_id);
    if (ret_val == SUCCESS)
    {
        sd_close_ex(file_id);
    }
    LOG_DEBUG("create empty file ret_val = %d", ret_val);
    return ret_val;
}

_int32 dm_set_file_size(DATA_MANAGER* p_data_manager_interface, _u64 filesize, BOOL force)
{
    _int32 ret_val = SUCCESS;
    _u64 rel_filesize = 0;

    LOG_DEBUG("dm_set_file_size. filesize :%llu . force=%d", filesize, force);

    if (filesize == 0)
    {
        ret_val = dm_create_empty_file(p_data_manager_interface);
        if (ret_val == SUCCESS)
        {
            dm_set_status_code(p_data_manager_interface, DATA_SUCCESS);
            return SUCCESS;
        }
        else
        {
            ret_val = SD_INVALID_FILE_SIZE;
            CHECK_VALUE(ret_val);
        }
    }

    if ((p_data_manager_interface->_data_status_code != DATA_DOWNING)
       || (p_data_manager_interface->_need_call_back == TRUE) 
       || (p_data_manager_interface ->_waited_success_close_file == TRUE))
    {
        LOG_DEBUG("dm_set_file_size. filesize :%llu , wrong status: %d , need call back %u,  wait success.", 
                    filesize, p_data_manager_interface->_data_status_code,
                    (_u32) p_data_manager_interface->_need_call_back, 
                    (_u32)p_data_manager_interface ->_waited_success_close_file);
        return SUCCESS;
    }

    if (file_info_filesize_is_valid(&p_data_manager_interface->_file_info) == TRUE)
    {
        rel_filesize = file_info_get_filesize(&p_data_manager_interface->_file_info);

        LOG_DEBUG("dm_set_file_size. filesize :%llu , rel filesize: %llu .", filesize, rel_filesize);

        if (rel_filesize == filesize)
        {
            return SUCCESS;
        }
        else
        {
            if ((rel_filesize >= MIN_BELIEVE_FILESIZE) && (filesize < MIN_NOT_BELIEVE_FILESIZE))
            {
                LOG_DEBUG("dm_set_file_size. filesize :%llu , rel filesize: %llu , because filesize is too small so drop origin res.", filesize, rel_filesize);
                ret_val = FILE_SIZE_TOO_SMALL;
                return ret_val;
            }

            ret_val = file_info_set_filesize(&p_data_manager_interface->_file_info, filesize);

            CHECK_VALUE(ret_val);

            p_data_manager_interface->_is_origion_res_changed = TRUE;
            file_info_invalid_bcid(&p_data_manager_interface->_file_info);
            file_info_invalid_cid(&p_data_manager_interface->_file_info);
            file_info_invalid_gcid(&p_data_manager_interface->_file_info);

            file_info_clear_all_recv_data(&p_data_manager_interface->_file_info);

            p_data_manager_interface->_only_use_orgin = TRUE;

            clear_error_block_list(&p_data_manager_interface->_correct_manage._error_ranages);

            //cm_set_origin_download_mode(&p_data_manager_interface->_task_ptr->_connect_manager, p_data_manager_interface->_origin_res);
            pt_set_origin_download_mode(p_data_manager_interface->_task_ptr, p_data_manager_interface->_origin_res);

            ret_val  = file_info_filesize_change(&p_data_manager_interface->_file_info, filesize);
            CHECK_VALUE(ret_val);

            compute_3part_range_list(filesize, &p_data_manager_interface->part_3_cid_range);
            correct_manager_add_prority_range_list(&p_data_manager_interface->_correct_manage, &p_data_manager_interface->part_3_cid_range);

            return ret_val;

        }

    }
    else
    {
        if( filesize < MIN_NOT_BELIEVE_FILESIZE 
            && p_data_manager_interface->_has_bub_info == FALSE 
            && !force )
        {
            LOG_DEBUG("dm_set_file_size. first set filesize :%llu  is not believe able.", filesize);
            ret_val = FILE_SIZE_NOT_BELIEVE;
            return ret_val;
        }

        LOG_DEBUG("dm_set_file_size. first set filesize :%llu .", filesize);

        compute_3part_range_list(filesize, &p_data_manager_interface->part_3_cid_range);

        if(file_info_cid_is_valid(&p_data_manager_interface->_file_info) == FALSE)
        {
            correct_manager_add_prority_range_list(&p_data_manager_interface->_correct_manage, &p_data_manager_interface->part_3_cid_range);
        }
    }

    ret_val = file_info_set_filesize(&p_data_manager_interface->_file_info, filesize);

    return ret_val;

}

_u64  dm_get_file_size(DATA_MANAGER* p_data_manager_interface)
{
    if(file_info_filesize_is_valid(&p_data_manager_interface->_file_info) == FALSE)
    {
        LOG_ERROR("dm_get_file_size. can not get filesize. " );
        return 0;
    }
    return file_info_get_filesize(&p_data_manager_interface->_file_info);
}

_int32 dm_get_block_size(DATA_MANAGER* p_data_manager_interface)
{
    if(file_info_filesize_is_valid(&p_data_manager_interface->_file_info) == FALSE)
    {
        LOG_ERROR("dm_get_block_size. can not get filesize, so return 0. " );
        return 0;
    }

    LOG_DEBUG("dm_get_block_size. get block size: %u.", p_data_manager_interface->_file_info._block_size);

    return file_info_get_blocksize(&p_data_manager_interface->_file_info);
}

BOOL  dm_has_recv_data( DATA_MANAGER* p_data_manager_interface)
{
    BOOL ret = file_info_has_recved_data(&p_data_manager_interface->_file_info);
    LOG_DEBUG("dm_has_recv_data. ret = %d", ret );
    return ret;
}

RANGE  dm_pos_len_to_valid_range( DATA_MANAGER* p_data_manager_interface, _u64 pos, _u64 length)
{
    RANGE ret_r;
    _u64 file_size =0;

    ret_r._index = 0;
    ret_r._num = 0;
    if(file_info_filesize_is_valid(&p_data_manager_interface->_file_info) == FALSE)
    {
        LOG_ERROR("dm_get_file_size. can not change pos_len to valid range. " );
        return ret_r;
    }

    file_size= file_info_get_filesize(&p_data_manager_interface->_file_info);

    return   pos_length_to_range2(pos, length, file_size);
}


_u32  dm_get_file_percent( DATA_MANAGER* p_data_manager_interface)
{
    return file_info_get_file_percent(&p_data_manager_interface->_file_info);
}

_u64  dm_get_download_data_size(DATA_MANAGER* p_data_manager_interface)
{

    return file_info_get_download_data_size(&p_data_manager_interface->_file_info);
}

_u64  dm_get_writed_data_size( DATA_MANAGER* p_data_manager_interface)
{

    return file_info_get_writed_data_size(&p_data_manager_interface->_file_info);
}


_int32 dm_set_status_code( DATA_MANAGER* p_data_manager_interface,_int32 status)
{
    LOG_DEBUG("dm_set_status_code. status:%d.", status);

    p_data_manager_interface->_data_status_code  =  status;
    if(g_notify_etm_scheduler != NULL)
            g_notify_etm_scheduler();
    return SUCCESS;
}

_int32 dm_get_status_code( DATA_MANAGER* p_data_manager_interface)
{
    LOG_DEBUG("dm_get_status_code. 0x%x ,status:%d.", p_data_manager_interface, p_data_manager_interface->_data_status_code);

    if(p_data_manager_interface->_data_status_code == DATA_ALLOC_READ_BUFFER_ERROR)
    {
        LOG_DEBUG("dm_get_status_code. because status is DATA_ALLOC_READ_BUFFER_ERROR. so start check blocks.");

        return DATA_DOWNING;
    }
    else if(p_data_manager_interface->_data_status_code == DATA_CANOT_CHECK_CID_NOBUFFER)
    {
        LOG_DEBUG("dm_get_status_code. because status is DATA_CANOT_CHECK_CID_NOBUFFER. so start check cid.");

        return DATA_DOWNING;
    }
    else if(p_data_manager_interface->_data_status_code == VOD_DATA_FINISHED)
    {
        LOG_DEBUG("dm_get_status_code. because status is VOD_DATA_SUCCESS. so start check cid.");

        return VOD_DATA_FINISHED;
    }
    return p_data_manager_interface->_data_status_code;
}

_int32 dm_handle_extra_things( DATA_MANAGER* p_data_manager_interface)
{
    LOG_DEBUG("dm_handle_extra_things, 0x%x,data_status_code=%d,IS_VOD_MOD=%d. ", p_data_manager_interface,p_data_manager_interface->_data_status_code,p_data_manager_interface->_IS_VOD_MOD);

    if(p_data_manager_interface->_data_status_code == DATA_ALLOC_READ_BUFFER_ERROR)
    {
        LOG_DEBUG("dm_handle_extra_things. because status is DATA_ALLOC_READ_BUFFER_ERROR. so start check blocks.");

        p_data_manager_interface->_data_status_code = DATA_DOWNING;

        start_check_blocks(&p_data_manager_interface->_file_info);
    }
    else if(p_data_manager_interface->_data_status_code == DATA_CANOT_CHECK_CID_NOBUFFER)
    {
        LOG_DEBUG("dm_handle_extra_things. because status is DATA_CANOT_CHECK_CID_NOBUFFER. so start check cid.");

        p_data_manager_interface->_data_status_code = DATA_DOWNING;

        data_manager_notify_success(p_data_manager_interface);
    }
    else if(p_data_manager_interface->_data_status_code == VOD_DATA_FINISHED 
        && p_data_manager_interface->_IS_VOD_MOD == FALSE)
    {
        if(p_data_manager_interface->_need_call_back == FALSE 
            && p_data_manager_interface->_waited_success_close_file == FALSE)
        {
            p_data_manager_interface->_waited_success_close_file = TRUE;

            file_info_close_tmp_file(&p_data_manager_interface->_file_info);
        }
        else
        {
            p_data_manager_interface->_waited_success_close_file = TRUE;
        }

    }


    return SUCCESS;
}


_int32 dm_set_cid( DATA_MANAGER* p_data_manager_interface, _u8 cid[CID_SIZE])
{
    _int32 ret_val = SUCCESS;
	
	TASK* task = NULL;
#ifdef _LOGGER
    char  cid_str[CID_SIZE*2+1];

    str2hex((char*)cid, CID_SIZE, cid_str, CID_SIZE*2);
    cid_str[CID_SIZE*2] = '\0';

    LOG_DEBUG("dm_set_cid: %s", cid_str);
#endif

    ret_val  = file_info_set_cid(&p_data_manager_interface->_file_info,cid);

	if( NULL != g_notify_event_callback)
	{
		task = (TASK*)p_data_manager_interface->_task_ptr;
		LOG_DEBUG("dm_set_cid: task_extern_id = %u, task = 0x%x", task->_extern_id, task);
		if(task != NULL)
		{	
			g_notify_event_callback(task->_extern_id, CID_IS_OK);
			LOG_DEBUG("dm_set_cid: notify_event to ui CID_IS_OK, callback = 0x%x", g_notify_event_callback);
		}
	}
    return ret_val;
}

_int32 dm_set_gcid( DATA_MANAGER* p_data_manager_interface, _u8 gcid[CID_SIZE])
{
    _int32 ret_val = SUCCESS;

#ifdef _LOGGER
    char  gcid_str[CID_SIZE*2+1];

    str2hex((char*)gcid, CID_SIZE, gcid_str, CID_SIZE*2);
    gcid_str[CID_SIZE*2] = '\0';

    LOG_DEBUG("dm_set_gcid: %s", gcid_str);
#endif

    ret_val  = file_info_set_gcid(&p_data_manager_interface->_file_info,gcid);

    return ret_val;
}

_int32 dm_set_user_data(DATA_MANAGER* dm_interface, const _u8 *user_data, _u32 len)
{
    return file_info_set_user_data(&dm_interface->_file_info, user_data, len);
}


//当数据文件发生文件大小变化与hub返回的文件大小不一致时，hub返回的bcid等应该失效。
_int32 dm_set_hub_return_info( DATA_MANAGER* p_data_manager_interface
                            , _int32 gcid_type
                            , unsigned char block_cid[]
                            , _u32 block_count
                            , _u32 block_size
                            , _u64 filesize  )
{
    _int32 ret_val = SUCCESS;
    _u64 rel_filesize  =  0;



#ifdef _LOGGER
    char  bcid_str[CID_SIZE*2+1];
    _u32 i =0;
    for(i=0; i<block_count; i++)
    {
        str2hex((char*)(block_cid + i*CID_SIZE), CID_SIZE,bcid_str, CID_SIZE*2);
        bcid_str[CID_SIZE*2] = '\0';
        LOG_DEBUG("dm_set_hub_return_info: bcidno %u:%s",i, bcid_str);
    }
#endif

    LOG_DEBUG("dm_set_hub_return_info, gcid_type:%d, block_count:%u, block_size:%u, filesize: %llu ."
        , gcid_type,block_count, block_size, filesize);

    p_data_manager_interface->_has_bub_info = TRUE;

    if(filesize == 0)
    {
        ret_val = SD_INVALID_FILE_SIZE;
        return ret_val;
    }

    if(file_info_filesize_is_valid(&p_data_manager_interface->_file_info) == FALSE)
    {
        LOG_DEBUG("dm_set_hub_return_info.  2 first set filesize :%llu .", filesize);

        ret_val  = file_info_set_filesize(&p_data_manager_interface->_file_info, filesize);

        CHECK_VALUE(ret_val);

        compute_3part_range_list(filesize, &p_data_manager_interface->part_3_cid_range);

        //correct_manager_clear_prority_range_list(&p_data_manager_interface->_correct_manage);
        if(p_data_manager_interface->_IS_VOD_MOD == FALSE)
        {
            correct_manager_clear_prority_range_list(&p_data_manager_interface->_correct_manage);
        }

        ret_val  = file_info_set_hub_return_info(&p_data_manager_interface->_file_info, filesize, gcid_type, block_cid, block_count, block_size);

        if(ret_val == OUT_OF_MEMORY)
            data_manager_notify_failure(p_data_manager_interface, DATA_ALLOC_BCID_BUFFER_ERROR);

        return ret_val;
    }
    else
    {
        rel_filesize  =  file_info_get_filesize(&p_data_manager_interface->_file_info);

        LOG_DEBUG("dm_set_hub_return_info.  filesize :%llu, relfilesize:%llu .", filesize, rel_filesize);

        if(rel_filesize != filesize)
        {
            ret_val = SD_INVALID_FILE_SIZE;

            p_data_manager_interface->_only_use_orgin = TRUE;

            file_info_invalid_cid(&p_data_manager_interface->_file_info);
            file_info_invalid_gcid(&p_data_manager_interface->_file_info);

            // cm_set_origin_download_mode(&p_data_manager_interface->_task_ptr->_connect_manager, p_data_manager_interface->_origin_res);
            pt_set_origin_download_mode(p_data_manager_interface->_task_ptr, p_data_manager_interface->_origin_res);
            return ret_val;
        }
        else
        {
            if(p_data_manager_interface->_IS_VOD_MOD == FALSE)
            {
                correct_manager_clear_prority_range_list(&p_data_manager_interface->_correct_manage);
            }
        }

        ret_val  = file_info_set_hub_return_info(&p_data_manager_interface->_file_info, filesize, gcid_type, block_cid, block_count, block_size);
        if(ret_val == OUT_OF_MEMORY)
            data_manager_notify_failure(p_data_manager_interface, DATA_ALLOC_BCID_BUFFER_ERROR);

        return ret_val;
    }

}

_int32 dm_shub_no_result( DATA_MANAGER* p_data_manager_interface)
{
    _u64 rel_filesize = 0;
    TASK* task = NULL;
    LOG_DEBUG("dm_shub_no_result");

    p_data_manager_interface->_has_bub_info = TRUE;
    if(file_info_filesize_is_valid(&p_data_manager_interface->_file_info) == TRUE)
    {
        rel_filesize  =  file_info_get_filesize(&p_data_manager_interface->_file_info);

        LOG_DEBUG("dm_shub_no_result, but has filesize :%llu.", rel_filesize);

        compute_3part_range_list(rel_filesize, &p_data_manager_interface->part_3_cid_range);

        if(file_info_cid_is_valid(&p_data_manager_interface->_file_info) == FALSE)
        {
            correct_manager_add_prority_range_list(&p_data_manager_interface->_correct_manage
                    , &p_data_manager_interface->part_3_cid_range);
        }
    }

#ifndef _ANDROID_LINUX // android手雷不通知hub查询失败。
    if (NULL != g_notify_event_callback)
    {
        task = (TASK*)p_data_manager_interface->_task_ptr;
        LOG_DEBUG("dm_shub_no_result: task_extern_id = %u, task = 0x%x", task->_extern_id, task);
        if (task != NULL)
        {	
            g_notify_event_callback(task->_extern_id, SHUB_NO_RESULT);
            LOG_DEBUG("dm_shub_no_result: notify_event to ui SHUB_NO_RESULT, callback = 0x%x", g_notify_event_callback);
        }
    }
#endif
    return SUCCESS;
}


BOOL dm_get_cid( DATA_MANAGER* p_data_manager_interface, _u8 cid[CID_SIZE])
{
    //LOG_DEBUG("dm_get_cid .");
    BOOL ret_val = TRUE;

#ifdef _LOGGER
    char  cid_str[CID_SIZE*2+1];
#endif

    LOG_DEBUG("dm_get_cid .");

    if(file_info_get_shub_cid(&p_data_manager_interface->_file_info, cid) == TRUE)
//       if(p_data_manager_interface->_file_info._cid_is_valid == TRUE)
    {

        // sd_memcpy(cid, p_data_manager_interface->_file_info._cid,   CID_SIZE);

#ifdef _LOGGER
        str2hex((char*)cid, CID_SIZE, cid_str, CID_SIZE*2);
        cid_str[CID_SIZE*2] = '\0';

        LOG_DEBUG("dm_get_cid, cid is valid, cid:%s.", cid_str);
#endif

        return TRUE;
    }

    if(file_info_filesize_is_valid(&p_data_manager_interface->_file_info) == FALSE || p_data_manager_interface->part_3_cid_range._node_size == 0)
    {
        LOG_ERROR("dm_get_cid, no filesize  so cannot get cid.");
        return FALSE;

    }

    if(dm_3_part_cid_is_ok(p_data_manager_interface)  == FALSE)
    {
        LOG_ERROR("dm_get_cid, cid data is not enough.");
        return FALSE;
    }

    ret_val  = get_file_3_part_cid(&p_data_manager_interface->_file_info, cid, NULL);
    if(ret_val == TRUE)
    {
#ifdef _LOGGER
        str2hex((char*)cid, CID_SIZE, cid_str, CID_SIZE*2);
        cid_str[CID_SIZE*2] = '\0';

        LOG_DEBUG("dm_get_cid, calc cid success, cid:%s.", cid_str);
#endif
        file_info_set_cid(&p_data_manager_interface->_file_info, cid) ;

    }
    else
    {
        LOG_ERROR("dm_get_cid, calc cid failure.");
    }

    return ret_val;
}

BOOL dm_get_shub_gcid( DATA_MANAGER* p_data_manager_interface, _u8 gcid[CID_SIZE])
{

#ifdef _LOGGER
    char  gcid_str[CID_SIZE*2+1];
#endif

    //  if(p_data_manager_interface->_file_info._gcid_is_valid == TRUE)
    if(file_info_get_shub_gcid(&p_data_manager_interface->_file_info, gcid) == TRUE)
    {

        //   sd_memcpy(gcid, p_data_manager_interface->_file_info._gcid, CID_SIZE);

#ifdef _LOGGER
        str2hex((char*)gcid, CID_SIZE, gcid_str, CID_SIZE*2);
        gcid_str[CID_SIZE*2] = '\0';

        LOG_DEBUG("dm_get_shub_gcid success, gcid:%s.", gcid_str);
#endif

        return TRUE;
    }
    else
    {
        LOG_DEBUG("dm_get_shub_gcid  gcid is invalid, so can not get gcid.");
        return FALSE;
    }
}

BOOL dm_get_calc_gcid( DATA_MANAGER* p_data_manager_interface, _u8 gcid[CID_SIZE])
{
    LOG_DEBUG("dm_get_calc_gcid .");
    return  get_file_gcid(&p_data_manager_interface->_file_info, gcid);
}

BOOL dm_bcid_is_valid( DATA_MANAGER* p_data_manager_interface)
{
    return file_info_bcid_valid(&p_data_manager_interface->_file_info);
}

BOOL dm_get_bcids( DATA_MANAGER* p_data_manager_interface, _u32* bcid_num, _u8** bcid_buffer)
{
    BOOL   ret_val =  FALSE;
    _u32  bcid_block_num = 0;
    _u8*  p_bcid_buffer = NULL;

    if(file_info_bcid_valid(&p_data_manager_interface->_file_info) == TRUE)
    {
        bcid_block_num = file_info_get_bcid_num(&p_data_manager_interface->_file_info);
        p_bcid_buffer =    file_info_get_bcid_buffer(&p_data_manager_interface->_file_info);
        ret_val = TRUE;
        LOG_DEBUG("dm_get_bcids, because bcid is valid , so  blocksize : %u , bcid buffer:0x%x .", bcid_block_num, p_bcid_buffer);
    }
    else
    {
        if(file_info_filesize_is_valid(&p_data_manager_interface->_file_info) == FALSE)
        {
            LOG_DEBUG("dm_get_bcids, because filesize is invalid so no bcid .");
            ret_val = FALSE;
        }
        else if(file_info_is_all_checked(&p_data_manager_interface->_file_info) == FALSE)
        {
            LOG_DEBUG("dm_get_bcids, because can not finish download, so no bcid .");
            ret_val = FALSE;

        }
        else
        {
            bcid_block_num =  file_info_get_bcid_num(&p_data_manager_interface->_file_info);
            p_bcid_buffer = file_info_get_bcid_buffer(&p_data_manager_interface->_file_info);
            ret_val = TRUE;
            LOG_DEBUG("dm_get_bcids, because bcid is invalid and has finished download, so  blocksize : %u , bcid buffer:0x%x .", bcid_block_num, p_bcid_buffer);
        }
    }

    *bcid_num = bcid_block_num;
    *bcid_buffer = p_bcid_buffer;

    return ret_val;


}


BOOL dm_only_use_origin( DATA_MANAGER* p_data_manager_interface)
{
    LOG_DEBUG("dm_only_use_origin, only use origin :%u .", p_data_manager_interface->_only_use_orgin);

    return     p_data_manager_interface->_only_use_orgin;
}

BOOL dm_check_gcid(DATA_MANAGER* p_data_manager_interface)
{
    _u8 s_gcid[CID_SIZE];
    _u8 c_gcid[CID_SIZE];
    BOOL ret_val = TRUE;
    _int32 i=0;

#ifdef _LOGGER
    char  cid_str[CID_SIZE*2+1];
#endif

    LOG_DEBUG("dm_check_gcid .");

    ret_val = file_info_get_shub_gcid( &p_data_manager_interface->_file_info,  s_gcid);
    if(ret_val  == FALSE)
    {
        LOG_ERROR("dm_check_gcid , cannot get valid gcid, so  failure.");
        return ret_val;
    }

#ifdef _LOGGER
    str2hex((char*)s_gcid, CID_SIZE, cid_str, CID_SIZE*2);
    cid_str[CID_SIZE*2] = '\0';

    LOG_DEBUG("dm_check_gcid, get shub gcid:%s.", cid_str);
#endif

    ret_val = get_file_gcid( &p_data_manager_interface->_file_info,  c_gcid);
    if(ret_val  == FALSE)
    {
        LOG_ERROR("dm_check_gcid, calc gcid  failure.");
        return ret_val;
    }

#ifdef _LOGGER
    str2hex((char*)c_gcid, CID_SIZE, cid_str, CID_SIZE*2);
    cid_str[CID_SIZE*2] = '\0';

    LOG_DEBUG("dm_check_gcid, calc gcid:%s.", cid_str);
#endif

    ret_val = TRUE;
    for(i=0; i<CID_SIZE; i++)
    {
        if(s_gcid[i] != c_gcid[i])
        {
            ret_val = FALSE;
            LOG_DEBUG("dm_check_gcid, check gcid  failure.");
            break;
        }
    }

    return ret_val;
}

BOOL dm_check_cid(DATA_MANAGER* p_data_manager_interface, _int32* err_code)
{
    return file_info_check_cid(&p_data_manager_interface->_file_info, err_code, NULL, NULL);
}

BOOL dm_3_part_cid_is_ok( DATA_MANAGER* p_data_manager_interface)
{
    LOG_DEBUG("dm_3_part_cid_is_ok .");
    //  return  range_list_is_contained(&p_data_manager_interface->_file_info._writed_range_list, &p_data_manager_interface->part_3_cid_range);
    return file_info_rangelist_is_write(&p_data_manager_interface->_file_info, &p_data_manager_interface->part_3_cid_range);
}

_int32 dm_set_origin_resource( DATA_MANAGER* p_data_manager_interface, RESOURCE*  origin_res)
{
    LOG_DEBUG("dm_set_origin_resource, origin res:0x%x .", origin_res);
    p_data_manager_interface->_origin_res = origin_res;

    correct_manager_set_origin_res(&p_data_manager_interface->_correct_manage, origin_res);
    return SUCCESS;
}

BOOL dm_is_origin_resource( DATA_MANAGER* p_data_manager_interface, RESOURCE*  res)
{
    LOG_DEBUG("dm_is_origin_resource, res:0x%x .", res);

    if(p_data_manager_interface->_origin_res  == NULL)
    {
        LOG_DEBUG("dm_is_origin_resource, origin res is NULL.");
        return FALSE;
    }

    if(p_data_manager_interface->_origin_res == res)
    {
        LOG_DEBUG("dm_is_origin_resource,res:0x%x  is origin.", res);
        return TRUE;
    }
    else
    {
        LOG_DEBUG("dm_is_origin_resource,res:0x%x  ,but origin is:0x%x .", res, p_data_manager_interface->_origin_res);
        return FALSE;
    }
}


BOOL dm_origin_resource_is_useable( DATA_MANAGER* p_data_manager_interface)
{
    DISPATCH_STATE res_state;
    LOG_DEBUG("dm_origin_resource_is_useable .");

    if(p_data_manager_interface->_origin_res  == NULL)
    {
        LOG_ERROR("dm_origin_resource_is_useable, origin res is NULL.");
        return FALSE;
    }

    res_state =  get_resource_state(p_data_manager_interface->_origin_res);

    if(res_state == ABANDON)
    {

        LOG_ERROR("dm_origin_resource_is_useable, because origin res 0x%x is abdon, so not use.", p_data_manager_interface->_origin_res);
        return FALSE;
    }
    else
    {
        LOG_DEBUG("dm_origin_resource_is_useable, because origin res 0x%x is %u, so can use.", p_data_manager_interface->_origin_res,res_state);
        return TRUE;
    }
}

/*BOOL dm_abandon_resource( DATA_MANAGER* p_data_manager_interface, RESOURCE*  abdon_res)
{
       LOG_DEBUG("dm_abandon_resource,DATA_MANAGER:0x%x  ,RESOURCE is:0x%x .", p_data_manager_interface, abdon_res);

        if(abdon_res == NULL)
        {
        return TRUE;
        }

     correct_manager_erase_abandon_res(&p_data_manager_interface->_correct_manage,  abdon_res);

     if(range_manager_has_resource(&p_data_manager_interface->_range_record, abdon_res) == TRUE)
     {
            LOG_DEBUG("dm_abandon_resource,DATA_MANAGER:0x%x  , abandon RESOURCE is:0x%x  failure.", p_data_manager_interface, abdon_res);
            return FALSE;
     }
     else
     {
            LOG_DEBUG("dm_abandon_resource,DATA_MANAGER:0x%x  , abandon RESOURCE is:0x%x success.", p_data_manager_interface, abdon_res);
            return TRUE;
     }

}*/

BOOL dm_is_recv_data_valid(DATA_MANAGER* p_data_manager_interface , const RANGE *r, _u32 data_len,
                           RESOURCE *resource_ptr) 
{
    if(p_data_manager_interface->_data_status_code == DATA_UNINITED)
    {
        LOG_DEBUG("dm_is_valid_recv_data data_length: because data_manager is unint so free bufer, data_manager :0x%x",p_data_manager_interface);
        return FALSE;
    }

    _u64 file_size = file_info_get_filesize(&p_data_manager_interface->_file_info);

    RANGE data_r  = *r;
    _u32  data_length =  data_len;
    char* filename = NULL;
    if(data_r._num*get_data_unit_size()   !=  data_length)
    {

        LOG_DEBUG("data_length:%u is not full range.", data_length);

        if(file_size != 0)
        {
            if(((_u64)(data_r._index))*get_data_unit_size() + data_length  < file_size)
            {
                data_r._num = data_length/get_data_unit_size();
                data_length =  data_r._num*get_data_unit_size();
            }
            LOG_DEBUG("data_length:, after process, the range is (%u,%u), data_len is %u.", data_r._index, data_r._num, data_length);

            if(data_length == 0 || data_r._index == MAX_U32 || (_u64)data_r._index*(_u64)get_data_unit_size() + data_length > file_size)
            {
                LOG_ERROR("data_length:, after process, the range is (%u,%u), data_len is %u, because data_len=0, so drop buffer.", data_r._index, data_r._num, data_length);
                return FALSE;
            }
        }
        else
        {
            LOG_DEBUG("dm_put_data data_length:%u is not full range, filesize is invalid.", data_length);
        }
    }


    if(p_data_manager_interface->_only_use_orgin == TRUE
        && resource_ptr != p_data_manager_interface->_origin_res)
    {
        LOG_ERROR("data_length:, the range is (%u,%u), data_len is %u, because resource:0x%x is not the origin resource :0x%x, so drop the data.",
            data_r._index, data_r._num, data_length, resource_ptr, p_data_manager_interface->_origin_res);
        return FALSE;
    }


    if(file_info_range_is_recv(&p_data_manager_interface->_file_info, &data_r) == TRUE)
    {
        LOG_ERROR("data_length:, the range is (%u,%u), data_len is %u  is recved, so drop it.",
            data_r._index, data_r._num, data_length);
        return FALSE;
    }


    return TRUE;
}

void dm_stat_recv_data_size(DATA_MANAGER* p_data_manager_interface, RESOURCE *resource_ptr, _u32 data_len)
{
#ifdef EMBED_REPORT
    if( resource_ptr == NULL ) return;

    LOG_DEBUG("dm_stat_recv_data_size , the resource type = %d , is_cdn_resource = %d",resource_ptr->_resource_type, p2p_is_cdn_resource(resource_ptr));

    if( resource_ptr->_resource_type == HTTP_RESOURCE || resource_ptr->_resource_type == FTP_RESOURCE)
    {
        if( resource_ptr->_resource_type == HTTP_RESOURCE && http_resource_is_lixian((HTTP_SERVER_RESOURCE *)resource_ptr) )
        {
            p_data_manager_interface->_lixian_recv_bytes += data_len;
        } 
        else
        {
            p_data_manager_interface->_server_recv_bytes += data_len;
        }
    }
    else if( resource_ptr->_resource_type == PEER_RESOURCE )
    {

        //if( p2p_is_cdn_resource(resource_ptr) )
        if(p2p_get_from(resource_ptr) == P2P_FROM_VIP_HUB \
            || p2p_get_from(resource_ptr) == P2P_FROM_CDN \
            || p2p_get_from(resource_ptr) == P2P_FROM_PARTNER_CDN)
        {
            p_data_manager_interface->_cdn_recv_bytes += data_len;
        }
        else if (p2p_get_from(resource_ptr) == P2P_FROM_LIXIAN)
        {
            p_data_manager_interface->_lixian_recv_bytes+= data_len;
        }
        else
        {
            p_data_manager_interface->_peer_recv_bytes += data_len;
        }
    }
#endif
}

void dm_stat_valid_data_size(DATA_MANAGER* p_data_manager_interface, RESOURCE *resource_ptr, const RANGE *r, _u32 data_len)
{
#ifdef EMBED_REPORT
    if( resource_ptr == NULL ) return;

    LOG_DEBUG("dm_stat_valid_data_size , the resource type = %d , is_cdn_resource = %d",resource_ptr->_resource_type, p2p_is_cdn_resource(resource_ptr));

    if( resource_ptr->_resource_type == HTTP_RESOURCE )
    {

        if( http_resource_is_lixian((HTTP_SERVER_RESOURCE *)resource_ptr) )
        {
            p_data_manager_interface->_lixian_dl_bytes+= data_len;
        } 
        else
        {
            p_data_manager_interface->_server_dl_bytes += data_len;
        }
        if((resource_ptr->_resource_type == HTTP_RESOURCE)&&http_resource_is_origin((HTTP_SERVER_RESOURCE *)resource_ptr) )
        {
            LOG_DEBUG("dm_put_data, file_info_range_is_write: %d", file_info_range_is_write(&p_data_manager_interface->_file_info, r));
            // 新收到的原始数据已经在写入文件的RANGE列表中，则该数据不需要记录在原始资源字段
            if( FALSE == file_info_range_is_write(&p_data_manager_interface->_file_info, r) )
                p_data_manager_interface->_origin_dl_bytes += data_len;
        }

        HTTP_SERVER_RESOURCE* p_http_res = (HTTP_SERVER_RESOURCE*)resource_ptr;
        if(p_http_res->_relation_res)	
        {
            p_data_manager_interface->_appacc_dl_bytes += data_len;
        }	
    }
    else if(resource_ptr->_resource_type == FTP_RESOURCE )
    {
        p_data_manager_interface->_server_dl_bytes += data_len;
        FTP_SERVER_RESOURCE  *ftp_resource = NULL;
        ftp_resource = (FTP_SERVER_RESOURCE *)resource_ptr;
        if((resource_ptr->_resource_type == FTP_RESOURCE) && ftp_resource->_b_is_origin )
        {
            LOG_DEBUG("dm_put_data, file_info_range_is_write: %d", file_info_range_is_write(&p_data_manager_interface->_file_info, r));
            if( FALSE == file_info_range_is_write(&p_data_manager_interface->_file_info, r) )
                p_data_manager_interface->_origin_dl_bytes += data_len;
        }
    }
    else if( resource_ptr->_resource_type == PEER_RESOURCE )
    {
#ifdef ENABLE_HSC
        HIGH_SPEED_CHANNEL_INFO *p_hsc_info = NULL;
        p_hsc_info = &(p_data_manager_interface->_task_ptr->_hsc_info);
        if (P2P_FROM_VIP_HUB == p2p_get_from(resource_ptr))
        {
            p_hsc_info->_dl_bytes += data_len;
            LOG_DEBUG("dm_put_data, resource from vip hub, downloaded bytes: %llu", p_hsc_info->_dl_bytes);
        }
#endif
        
		LOG_DEBUG("dm_put_data , the p2p get from = %d ",p2p_get_from(resource_ptr));
        if(p2p_get_from(resource_ptr) == P2P_FROM_VIP_HUB || 
		   p2p_get_from(resource_ptr) == P2P_FROM_CDN || 
		   p2p_get_from(resource_ptr) == P2P_FROM_PARTNER_CDN)
        {
            p_data_manager_interface->_cdn_dl_bytes += data_len;
        }
        else if (p2p_get_from(resource_ptr) == P2P_FROM_LIXIAN)
        {
            p_data_manager_interface->_lixian_dl_bytes+= data_len;
            p_data_manager_interface->_task_ptr->_main_task_lixian_info._dl_bytes += data_len;
        }
		else if (p2p_get_from(resource_ptr) == P2P_FROM_NORMAL_CDN)
		{
			p_data_manager_interface->_normal_cdn_dl_bytes += data_len;
		}
        else
        {
            p_data_manager_interface->_peer_dl_bytes += data_len;
        }
    }
#endif
}

_int32 dm_put_data(DATA_MANAGER* p_data_manager_interface, const RANGE *r, char **data_ptr, _u32 data_len,_u32 buffer_len,
                   DATA_PIPE *p_data_pipe, RESOURCE *resource_ptr)
{
    _int32 ret_val = SUCCESS;

    RANGE data_r  = *r;
    _u32  data_length =  data_len;

    LOG_DEBUG("dm_put_data , the range is (%u,%u), data_len is %u, buffer_length:%u, buffer:0x%x , resource :0x%x.",
        data_r._index, data_r._num, data_length, buffer_len, *data_ptr, resource_ptr);

#ifdef EMBED_REPORT
    dm_stat_recv_data_size(p_data_manager_interface, resource_ptr, data_len);
#endif
#ifdef VVVV_VOD_DEBUG
    gloabal_recv_data += data_length;
#endif


    BOOL is_recv_data_valid = dm_is_recv_data_valid(p_data_manager_interface, r, data_len, resource_ptr);
    LOG_DEBUG("[dispatch_stat] range %u_%u check 0x%x %d.", data_r._index, data_r._num, p_data_pipe, is_recv_data_valid);
    if (FALSE == is_recv_data_valid)
    {
        LOG_DEBUG("dm_put_data is_recv_data_valid==FALSE. data_length:, the range is (%u,%u), data_len is %u, buffer_length:%u, buffer:0x%x",
            data_r._index, data_r._num, data_length, buffer_len, *data_ptr);
        dm_free_data_buffer(p_data_manager_interface, data_ptr, buffer_len);
#ifdef EMBED_REPORT
        file_info_add_overloap_date(&p_data_manager_interface->_file_info, data_length);
#endif
        return SUCCESS;
    }


    if( file_info_file_is_create(&p_data_manager_interface->_file_info) == FALSE  ) 
    {
        char* filename = NULL;

        static char callback_filename[MAX_FILE_NAME_BUFFER_LEN] = {0};
        _u32 callback_filename_len = 0;
        if(TRUE == dm_decide_and_get_file_name(p_data_manager_interface, &filename))
        {         
            if( p_data_manager_interface->_task_ptr->_task_type == P2SP_TASK_TYPE
                && p_data_manager_interface->_task_ptr->_extern_id != -1
                && g_notify_filename_changed_callback!=NULL )
            {                    
                sd_memset(callback_filename,0,MAX_FILE_NAME_BUFFER_LEN);
                callback_filename_len = sd_strlen(p_data_manager_interface->_file_info._file_name)- 3 ;

                if(sd_stricmp(p_data_manager_interface->_file_info._file_name + callback_filename_len - 3
                    ,".rf")== 0)
                {
                    callback_filename_len -=3;
                }

                sd_memcpy(callback_filename,p_data_manager_interface->_file_info._file_name,callback_filename_len);
                g_notify_filename_changed_callback(p_data_manager_interface->_task_ptr->_extern_id,
                    callback_filename,
                    callback_filename_len);

                LOG_DEBUG("g_notify_filename_changed_callback id %d , name %s",
                    p_data_manager_interface->_task_ptr->_extern_id,
                    p_data_manager_interface->_file_info._file_name);
            }
        }
    }

    put_range_record(&p_data_manager_interface->_range_record, resource_ptr, &data_r);

    ret_val =  file_info_put_safe_data(&p_data_manager_interface->_file_info , &data_r, data_ptr,  data_len, buffer_len);

    if(ret_val == SUCCESS)
    {
        *data_ptr =  NULL;
    }
    else
    {
        LOG_DEBUG("dm_put_data , data_receiver_add_buffer fail, ret = %d.", ret_val);
        dm_free_data_buffer(p_data_manager_interface ,  data_ptr, buffer_len);

#ifdef EMBED_REPORT
        file_info_add_overloap_date(&p_data_manager_interface->_file_info, data_length);
#endif
        //  range_manager_erase_range(&p_data_manager_interface->_range_record, &data_r, NULL);
        return ret_val;
    }



#ifdef EMBED_REPORT
    file_info_handle_valid_data_report( &p_data_manager_interface->_file_info, resource_ptr, data_len );
    dm_stat_valid_data_size(p_data_manager_interface, resource_ptr, r, data_len);
#endif

    LOG_DEBUG("dm_put_data ,server_dl_bytes = %llu, origin_dl_bytes = %llu, download_dl_bytes = %llu",p_data_manager_interface->_server_dl_bytes,
        p_data_manager_interface->_origin_dl_bytes, dm_get_download_data_size(p_data_manager_interface) );

    LOG_DEBUG("dm_put_data ,lixian_dl_bytes = %llu, cdn_dl_bytes = %llu, peer_dl_bytes = %llu\n",
        p_data_manager_interface->_lixian_dl_bytes, p_data_manager_interface->_cdn_dl_bytes, p_data_manager_interface->_peer_dl_bytes);

    return SUCCESS;

}

_int32 dm_notify_dispatch_data_finish(DATA_MANAGER* p_data_manager_interface ,DATA_PIPE *ptr_data_pipe)
{
    //return ds_dispatch_at_pipe(&p_data_manager_interface->_task_ptr->_dispatcher, ptr_data_pipe);
    return dt_dispatch_at_pipe(p_data_manager_interface->_task_ptr, ptr_data_pipe);
}


_int32 dm_flush_cached_buffer(DATA_MANAGER* p_data_manager_interface)
{
    _u32 ret_val = SUCCESS;

    LOG_DEBUG("dm_flush_data_to_file .");

    ret_val = file_info_flush_cached_buffer(&p_data_manager_interface->_file_info);
    LOG_DEBUG("dm_flush_cached_buffer, ret_val :%u .", ret_val);

    return (_int32)ret_val;
}




_int32 dm_check_block_success(DATA_MANAGER* p_data_manager_interface, RANGE* p_block_range)
{
#ifdef UPLOAD_ENABLE
    _u8 gcid[CID_SIZE]= {0};
#endif
    //  RANGE block_range =  to_range(blockno, p_data_manager_interface->_data_checker._block_size, p_data_manager_interface->_file_info._file_size);

    LOG_DEBUG("dm_check_block_success, block_range :(%u,%u) check success .", p_block_range->_index, p_block_range->_num);

    correct_manager_erase_error_block(&p_data_manager_interface->_correct_manage,p_block_range);

    range_manager_erase_range(&p_data_manager_interface->_range_record, p_block_range, p_data_manager_interface->_origin_res);

    /* if(dm_ia_all_received( p_data_manager_interface) == TRUE)
     {
            LOG_DEBUG("dm_check_block_success, blockno :%u check success, all data has recved .", blockno);
            data_manager_notify_success(p_data_manager_interface);
     }*/

#ifdef UPLOAD_ENABLE
    if(dm_get_shub_gcid( p_data_manager_interface, gcid))
    {
        ulm_notify_have_block( gcid);
    }
    //dt_notify_have_block( p_data_manager_interface->_task_ptr, MAX_U32 );
#endif
    return SUCCESS;

}

_int32 dm_check_block_failure(DATA_MANAGER* p_data_manager_interface, RANGE* p_block_range)
{
    // RANGE block_range =  to_range(blockno, p_data_manager_interface->_data_checker._block_size, p_data_manager_interface->_file_info._file_size);
    LIST res_list;
    _int32 check_ret = 0;

    RESOURCE * r_res = NULL;
    LIST_ITERATOR it_res = NULL;

    LOG_DEBUG("dm_check_block_failure, block range :(%u, %u) check failure .", p_block_range->_index, p_block_range->_num);

    list_init(&res_list);

    get_res_from_range(&p_data_manager_interface->_range_record,  p_block_range, &res_list);

    /*
          0   no error
        -1  peer/server  correct
        -2  one resource  correct
        -3  origin correct
        -4  can not correct
       */

#ifdef _VOD_NO_DISK
    if(p_data_manager_interface->_is_no_disk == TRUE)
    {
        pt_dm_notify_check_error_by_range(p_data_manager_interface->_task_ptr, p_block_range);
    }
#endif

    out_put_res_list(&res_list);

    if(list_size(&res_list) == 1)
    {
        it_res = LIST_BEGIN(res_list);
        r_res = (RESOURCE*) LIST_VALUE(it_res);

#ifdef EMBED_REPORT
        file_info_handle_err_data_report( &p_data_manager_interface->_file_info, r_res, p_block_range->_num * get_data_unit_size());
#endif

        if(range_is_all_from_res(&p_data_manager_interface->_range_record, r_res, p_block_range) == FALSE)
        {
            LOG_DEBUG("dm_check_block_failure range_is_all_from_res,  res:0x%x , range (%u,%u) ret FALSE.", r_res, p_block_range->_index, p_block_range->_num);

            correct_manager_clear_res_list(&res_list );
        }
    }

    check_ret = correct_manager_add_error_block(&p_data_manager_interface->_correct_manage,  p_block_range,&res_list);

    if(check_ret == -4)
    {
        if(file_info_add_no_need_check_range(&p_data_manager_interface->_file_info, p_block_range) == FALSE)
        {

            /* can not correct error, so task failure*/
            LOG_DEBUG("dm_check_block_failure, block range :(%u,%u) check failure, correct manager ret -4 .", p_block_range->_index, p_block_range->_num);

            // dm_cannot_correct_block_data(p_data_manager_interface, blockno);

            data_manager_notify_failure(p_data_manager_interface, DATA_CANNOT_CORRECT_ERROR);

            correct_manager_clear_res_list(&res_list );
            return SUCCESS;
        }
        else
        {
            LOG_DEBUG("dm_check_block_failure, blockrange :(%u.%u) check failure, reach max correct times  so no need check .", p_block_range->_index, p_block_range->_num);

            correct_manager_clear_res_list(&res_list );

            range_manager_erase_range(&p_data_manager_interface->_range_record, p_block_range, NULL);

            return SUCCESS;
        }

    }
    else if(check_ret == -3)
    {
        /*  shub bcid is valid, use origin to download */
        LOG_DEBUG("dm_check_block_failure, block range :(%u, %u) check failure, correct manager ret -3 .", p_block_range->_index, p_block_range->_num);

	 p_data_manager_interface->_is_origion_res_changed = TRUE;

        file_info_invalid_bcid(&p_data_manager_interface->_file_info);

        file_info_invalid_cid(&p_data_manager_interface->_file_info);
        file_info_invalid_gcid(&p_data_manager_interface->_file_info);

        if(file_info_range_is_write(&p_data_manager_interface->_file_info, p_block_range) != TRUE)
        {
            range_manager_erase_range(&p_data_manager_interface->_range_record, p_block_range, NULL);
        }

        dm_erase_data_except_origin(p_data_manager_interface);

        p_data_manager_interface->_only_use_orgin = TRUE;

        clear_error_block_list(&p_data_manager_interface->_correct_manage._error_ranages);

        //cm_set_origin_download_mode(&p_data_manager_interface->_task_ptr->_connect_manager, p_data_manager_interface->_origin_res);
        pt_set_origin_download_mode(p_data_manager_interface->_task_ptr, p_data_manager_interface->_origin_res);

        correct_manager_clear_res_list(&res_list );

        //correct_manager_add_prority_range_list(&p_data_manager_interface->_correct_manage, &p_data_manager_interface->part_3_cid_range);
        correct_manager_clear_prority_range_list(&p_data_manager_interface->_correct_manage);
        return SUCCESS;

    }
    else
    {
        LOG_DEBUG("dm_check_block_failure, blockrange :(%u.%u) check failure, correct manager ret %d .", p_block_range->_index, p_block_range->_num, check_ret);

        correct_manager_clear_res_list(&res_list );

//       data_receiver_erase_range(&p_data_manager_interface->_data_receiver, p_block_range);

        range_manager_erase_range(&p_data_manager_interface->_range_record, p_block_range, NULL);

        //      range_list_delete_range(&p_data_manager_interface->_data_checker._writed_range_list, p_block_range, NULL, NULL);

        return SUCCESS;
    }
}



BOOL dm_ia_all_received(DATA_MANAGER* p_data_manager_interface)
{

    return  file_info_ia_all_received(&p_data_manager_interface->_file_info);
}

_int32 dm_erase_data_except_origin(DATA_MANAGER* p_data_manager_interface)
{
    RANGE_LIST* recv_list = NULL;

    LOG_DEBUG("dm_erase_data_except_origin .");

    recv_list = file_info_get_recved_range_list(&p_data_manager_interface->_file_info);
    if(recv_list == NULL)
    {
        sd_assert(FALSE);
    }

    out_put_range_list(recv_list);

    range_list_clear(recv_list);

    get_range_from_res(&p_data_manager_interface->_range_record
        ,  p_data_manager_interface->_origin_res, recv_list);

    LOG_DEBUG("dm_erase_data_except_origin, after clear, recv range_list .");
    out_put_range_list(recv_list);

    /*  range_list_cp_range_list(GET_TOTAL_RECV_RANGE_LIST(p_data_manager_interface->_file_info._data_receiver), &p_data_manager_interface->_file_info._writed_range_list);

    range_list_clear(&p_data_manager_interface->_file_info._done_ranges);

    data_receiver_syn_data_buffer(&p_data_manager_interface->_file_info._data_receiver);

    clear_check_blocks(&p_data_manager_interface->_file_info);   */

    file_info_asyn_all_recv_data(&p_data_manager_interface->_file_info);

    return SUCCESS;
}

_int32 dm_get_data_buffer(DATA_MANAGER* p_data_manager_interface ,  char **data_ptr, _u32* data_len)
{
    _int32 ret_val = SUCCESS;

    LOG_DEBUG("dm_get_data_buffer .");

    ret_val = dm_get_buffer_from_data_buffer(  data_len, data_ptr);

    if(ret_val == SUCCESS)
    {
        LOG_DEBUG("dm_get_data_buffer,  data_buffer:0x%x, data_len:%u.", *data_ptr, *data_len);
    }
    else if(ret_val== DATA_BUFFER_IS_FULL || ret_val== OUT_OF_MEMORY)
    {
        LOG_DEBUG("DATA_MANAGER : 0x%x  , dm_get_data_buffer failue, so flush data,  data_buffer:0x%x, data_len:%u.", p_data_manager_interface, *data_ptr, *data_len);

#ifndef _VOD_NO_DISK
        if(p_data_manager_interface != NULL)
        {
            file_info_flush_data_to_file(&p_data_manager_interface->_file_info, FALSE);
        }
#else
        if(p_data_manager_interface != NULL && p_data_manager_interface->_is_no_disk == TRUE)
        {
            dm_flush_data_to_vod_cache( p_data_manager_interface);

            ret_val = dm_get_buffer_from_data_buffer(  data_len, data_ptr);
            if(ret_val == SUCCESS)
            {
                LOG_DEBUG("dm_get_data_buffer 2 ,  data_buffer:0x%x, data_len:%u.", *data_ptr, *data_len);
            }
            else if(ret_val== DATA_BUFFER_IS_FULL || ret_val== OUT_OF_MEMORY)
            {
                LOG_DEBUG("DATA_MANAGER 2 : 0x%x  , dm_get_data_buffer failue, so flush data,  data_buffer:0x%x, data_len:%u.", p_data_manager_interface, *data_ptr, *data_len);
            }

        }
        else
        {
            if(p_data_manager_interface != NULL)
            {
                file_info_flush_data_to_file(&p_data_manager_interface->_file_info, TRUE);
            }
        }
#endif
    }



    return ret_val;


}

_int32 dm_free_data_buffer(DATA_MANAGER* p_data_manager_interface ,  char **data_ptr, _u32 data_len)
{
    _int32 ret_val = SUCCESS;

    LOG_DEBUG("dm_free_data_buffer,  data_buffer:0x%x, data_len:%u.", *data_ptr, data_len);

    ret_val = dm_free_buffer_to_data_buffer(data_len, data_ptr);

    return (ret_val);

}

BOOL dm_range_is_recv(DATA_MANAGER* p_data_manager_interface, RANGE* r)
{
    return file_info_range_is_recv(&p_data_manager_interface->_file_info, r);
}

BOOL dm_range_is_write(DATA_MANAGER* p_data_manager_interface, RANGE* r)
{
    LOG_DEBUG("dm_range_is_write .");

    return file_info_range_is_write(&p_data_manager_interface->_file_info, r);
}

BOOL dm_range_is_check(DATA_MANAGER* p_data_manager_interface, RANGE* r)
{
    LOG_DEBUG("dm_range_is_check .");

    return file_info_range_is_check(&p_data_manager_interface->_file_info, r);
}

BOOL dm_range_is_cached(DATA_MANAGER* p_data_manager_interface, RANGE* r)
{
    LOG_DEBUG("dm_range_is_cached .");

    return file_info_range_is_cached(&p_data_manager_interface->_file_info, r);
}

RANGE_LIST* dm_get_recved_range_list(DATA_MANAGER* p_data_manager_interface)
{
    return file_info_get_recved_range_list(&p_data_manager_interface->_file_info);
}

RANGE_LIST* dm_get_writed_range_list(DATA_MANAGER* p_data_manager_interface)
{
    return file_info_get_writed_range_list(&p_data_manager_interface->_file_info);
}

RANGE_LIST* dm_get_checked_range_list(DATA_MANAGER* p_data_manager_interface)
{
    return file_info_get_checked_range_list(&p_data_manager_interface->_file_info);
}

BOOL dm_is_only_using_origin_res(DATA_MANAGER* p_data_manager_interface)
{
    LOG_DEBUG("dm_is_only_using_origin_res .");
    return (p_data_manager_interface->_only_use_orgin);
}

_int32 dm_get_uncomplete_range(DATA_MANAGER* p_data_manager_interface
        , RANGE_LIST*  un_complete_range_list)
{
    _int32 ret_val = SUCCESS;
    _u64 file_size = 0;
    RANGE full_r ;
    RANGE start_r;
    RANGE_LIST* recv_list = NULL;
    RANGE section_r;
    _u32 tmp_end = 0;

    LOG_DEBUG("dm_get_uncomplete_range .");
    file_size = file_info_get_filesize(&p_data_manager_interface->_file_info);
    if(file_size == 0)  return SUCCESS;

    recv_list = file_info_get_recved_range_list(&p_data_manager_interface->_file_info);
    if(recv_list == NULL)
    {
        sd_assert(FALSE);
    }

    full_r = pos_length_to_range(0, file_size, file_size);

    LOG_DEBUG("dm_get_uncomplete_range , before clear, un_complete_range_list:.");
    out_put_range_list(un_complete_range_list);
    range_list_clear(un_complete_range_list);

    if(p_data_manager_interface->_IS_VOD_MOD == TRUE && p_data_manager_interface->_start_pos_index != MAX_U32)
    {
        do
        {
            //p_data_manager_interface->_conrinue_times++;
            start_r._index = p_data_manager_interface->_start_pos_index;
#ifdef _VOD_NO_DISK
            if(p_data_manager_interface->_is_no_disk == TRUE)
            {
                start_r._num =  VOD_ONCE_RANGE_NUM;
            }
            else
            {
                start_r._num = (p_data_manager_interface->_conrinue_times+1) * VOD_ONCE_RANGE_NUM;
            }
#else
            start_r._num = (p_data_manager_interface->_conrinue_times+1) * VOD_ONCE_RANGE_NUM;
#endif
            if(start_r._index + start_r._num > full_r._index + full_r._num)
            {
                start_r._num =  full_r._index + full_r._num -start_r._index;
            }
            LOG_DEBUG("dm_get_uncomplete_range , before calc, continue times :%u.", p_data_manager_interface->_conrinue_times);

            range_list_add_range(un_complete_range_list, &start_r, NULL,NULL);

            LOG_DEBUG("dm_get_uncomplete_range , before calc, un_complete_range_list:.");
            out_put_range_list(un_complete_range_list);
            LOG_DEBUG("dm_get_uncomplete_range , before calc, _total_receive_range:.");
            out_put_range_list(recv_list);

            range_list_delete_range_list(un_complete_range_list, recv_list);

            LOG_DEBUG("dm_get_uncomplete_range , after calc, un_complete_range_list:.");
            out_put_range_list(un_complete_range_list);
            LOG_DEBUG("dm_get_uncomplete_range , after calc, _total_receive_range:.");
            out_put_range_list(recv_list);

            if(un_complete_range_list->_node_size != 0)
            {
                break;
            }
            else
            {
                if(start_r._index + start_r._num >= full_r._index + full_r._num)
                {
#ifdef _VOD_NO_DISK
                    if(p_data_manager_interface->_is_no_disk == TRUE)
                    {
                        return ret_val;
                    }

#endif
                    p_data_manager_interface->_start_pos_index = MAX_U32;

                    range_list_add_range(un_complete_range_list, &full_r, NULL,NULL);

                    LOG_DEBUG("dm_get_uncomplete_range , before calc, un_complete_range_list:.");
                    out_put_range_list(un_complete_range_list);
                    LOG_DEBUG("dm_get_uncomplete_range , before calc, _total_receive_range:.");
                    out_put_range_list(recv_list);

                    ret_val =  range_list_delete_range_list(un_complete_range_list, recv_list);

                    LOG_DEBUG("dm_get_uncomplete_range , after calc, un_complete_range_list:.");
                    out_put_range_list(un_complete_range_list);
                    LOG_DEBUG("dm_get_uncomplete_range , after calc, _total_receive_range:.");
                    out_put_range_list(recv_list);
                    return  ret_val;

                }
                else
                {
                    p_data_manager_interface->_start_pos_index =  start_r._index + start_r._num;
                    p_data_manager_interface->_conrinue_times++;
                    ret_val = 1;
                    continue;
                }
            }
        }
        while(TRUE);

    }
    else
    {
        if( g_is_section_download )
        {
            section_r._index = p_data_manager_interface->_cur_range_step*g_data_manager_download_once_num;
            section_r._num = g_data_manager_download_once_num;
            LOG_DEBUG("dm_get_uncomplete_range , section_r._index:%u, section_r._num:%u, full_r._index:%u, full_r._num:%u",
                      section_r._index, section_r._num, full_r._index, full_r._num );

            if( section_r._index >= full_r._index + full_r._num )
            {
                return SUCCESS;
            }

            start_r._index = MAX( section_r._index, full_r._index );
            tmp_end = MIN( section_r._index + section_r._num, full_r._index + full_r._num );
            if( tmp_end <= start_r._index )
            {
                return SUCCESS;
            }
            start_r._num = tmp_end - start_r._index;
        }
        else
        {
            start_r._index = full_r._index;
            start_r._num = full_r._num;
        }
        LOG_DEBUG("dm_get_uncomplete_range , start_idex:%u, start_num:%u", start_r._index, start_r._num );

        range_list_add_range(un_complete_range_list, &start_r, NULL,NULL);

        LOG_DEBUG("dm_get_uncomplete_range , before calc, un_complete_range_list:.");
        out_put_range_list(un_complete_range_list);
        LOG_DEBUG("dm_get_uncomplete_range , before calc, _total_receive_range:.");
        out_put_range_list(recv_list);

        ret_val =  range_list_delete_range_list(un_complete_range_list, recv_list);

        LOG_DEBUG("dm_get_uncomplete_range , after calc, un_complete_range_list:.");
        out_put_range_list(un_complete_range_list);
        LOG_DEBUG("dm_get_uncomplete_range , after calc, _total_receive_range:.");
        out_put_range_list(recv_list);

        if( range_list_size(un_complete_range_list) == 0 && p_data_manager_interface->_cur_range_step != MAX_U32 )
        {
            p_data_manager_interface->_cur_range_step++;
        }

        LOG_DEBUG("dm_get_uncomplete_range , un_complete_range_list size:%d.range_step:%d",
                  range_list_size(un_complete_range_list), p_data_manager_interface->_cur_range_step );
    }

    return ret_val;
}

_int32 dm_get_priority_range(DATA_MANAGER* p_data_manager_interface, RANGE_LIST*  priority_range_list)
{
    _int32 ret_val = SUCCESS;
    RANGE_LIST* recv_list = NULL;

    if(p_data_manager_interface->_correct_manage._prority_ranages._node_size == 0)
    {
        LOG_DEBUG("dm_get_priority_range, but  priority_range is 0.");
        return ret_val;
    }
    //out_put_range_list(&p_data_manager_interface->_correct_manage._prority_ranages);
    range_list_cp_range_list(&p_data_manager_interface->_correct_manage._prority_ranages,priority_range_list);
    //out_put_range_list(priority_range_list);

    recv_list = file_info_get_recved_range_list(&p_data_manager_interface->_file_info);
    if(recv_list == NULL)
    {
        sd_assert(FALSE);
    }

    //out_put_range_list(recv_list);
    ret_val =  range_list_delete_range_list(priority_range_list, recv_list);

    if(priority_range_list->_node_size == 0)
    {
        correct_manager_clear_prority_range_list(&p_data_manager_interface->_correct_manage);
        ret_val = 1;
    }

    LOG_DEBUG( "dm_get_priority_range, uncomplete priority_range is:" );
    out_put_range_list(priority_range_list);

    return ret_val;
}

_int32 dm_get_error_range_block_list(DATA_MANAGER* p_data_manager_interface, ERROR_BLOCKLIST**  pp_error_block_list)
{
    *pp_error_block_list = NULL;

    LOG_DEBUG("dm_get_error_range_block_list .");

    if(list_size(&p_data_manager_interface->_correct_manage._error_ranages._error_block_list) == 0)
    {
        LOG_DEBUG("dm_get_error_range_block_list,  _correct_manage._error_ranages._error_block_list size is 0.");
        return SUCCESS;
    }

    *pp_error_block_list = &p_data_manager_interface->_correct_manage._error_ranages;
    return SUCCESS;
}

/*删除下载任务中的临时文件，包括数据文件和cfg文件，一定要在unit data manager 之后才能调用!*/
_int32 dm_delete_tmp_file(DATA_MANAGER* p_data_manager_interface)
{
    _int32 ret_val = SUCCESS;

    ret_val = file_info_delete_cfg_file(&p_data_manager_interface->_file_info);

    //CHECK_VALUE(ret_val);

    ret_val = file_info_delete_tmp_file(&p_data_manager_interface->_file_info);

//    CHECK_VALUE(ret_val);

    return SUCCESS;
}


_int32 dm_open_old_file(DATA_MANAGER* p_data_manager_interface, char* cfg_file_path)
{
    _int32 ret_val =  SUCCESS;

    char  data_file_full_path [MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN] = {0};
    _u32 cfg_len = 0;

    cfg_len = sd_strlen(cfg_file_path);
    if(cfg_len > MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN)
    {
        LOG_ERROR("dm_open_old_file, cfg file %s is too long :%u .", cfg_file_path, cfg_len);
        ret_val = OPEN_OLD_FILE_FAIL;
        return ret_val;

    }
    sd_strncpy(data_file_full_path, cfg_file_path, cfg_len);
    data_file_full_path[cfg_len-4] = '\0';

    if(sd_file_exist(cfg_file_path) == FALSE )
    {
        LOG_ERROR("dm_open_old_file, cfg file %s not exsist .", cfg_file_path);

        sd_delete_file(data_file_full_path);
        ret_val = OPEN_OLD_FILE_FAIL;
        return ret_val;

    }

    if(sd_file_exist(data_file_full_path) == FALSE )
    {
        LOG_ERROR("dm_open_old_file, data file %s not exsist .", data_file_full_path);
        ret_val = OPEN_OLD_FILE_FAIL;
        return ret_val;

    }

    if(file_info_load_from_cfg_file(&p_data_manager_interface->_file_info,cfg_file_path) == FALSE)
    {
        sd_delete_file(data_file_full_path);
        sd_delete_file(cfg_file_path);
        ret_val = OPEN_OLD_FILE_FAIL;
        LOG_ERROR("dm_open_old_file,file_info_load_from_cfg_file ERROR ret : %d,%s .", ret_val,cfg_file_path);
    }



    LOG_DEBUG("dm_open_old_file, ret : %d .", ret_val);

    return ret_val;
}



_u64 dm_get_downsize_from_cfg_file(char* cfg_file_path)
{
    return  file_info_get_downsize_from_cfg_file(cfg_file_path);
}

void dm_set_vod_mode(DATA_MANAGER* p_data_manager_interface,BOOL  is_vod_mod)
{
    LOG_DEBUG("dm_set_vod_mode : %u", (_u32)is_vod_mod);

    p_data_manager_interface->_IS_VOD_MOD = is_vod_mod;

    return;
}

BOOL dm_is_vod_mode(DATA_MANAGER* p_data_manager_interface)
{
    return p_data_manager_interface->_IS_VOD_MOD;
}

_int32 dm_set_emerge_rangelist( DATA_MANAGER* p_data_manager_interface, RANGE_LIST* em_range_list)
{
    RANGE_LIST_ITEROATOR first_range;

    /*
    #ifdef _VOD_NO_DISK
            RANGE_LIST * p_recv_list = NULL;
            RANGE_LIST  tmp_range_list ;
    #endif
    */
    if(em_range_list == NULL)
        return SUCCESS;

    LOG_ERROR("dm_set_emerge_rangelist ...");

    out_put_range_list(em_range_list);
    correct_manager_clear_prority_range_list(&p_data_manager_interface->_correct_manage);
    correct_manager_add_prority_range_list(&p_data_manager_interface->_correct_manage, em_range_list);
    p_data_manager_interface->_IS_VOD_MOD= TRUE;

    /*
    #ifdef _VOD_NO_DISK
          p_recv_list =  file_info_get_recved_range_list(&p_data_manager_interface->_file_info);

         range_list_init(&tmp_range_list);

         range_list_cp_range_list(p_recv_list, &tmp_range_list);

         out_put_range_list(&tmp_range_list);

         range_list_delete_range_list(&tmp_range_list, em_range_list);

          out_put_range_list(&tmp_range_list);

            if(tmp_range_list._node_size != 0)
            {
              dm_notify_free_data_buffer(p_data_manager_interface, &tmp_range_list);
            }

            range_list_clear(&tmp_range_list);
    #endif
    */
    range_list_get_head_node(em_range_list, &first_range);
    if(first_range != NULL)
    {
        p_data_manager_interface->_start_pos_index = first_range->_range._index + first_range->_range._num;

//         ds_start_dispatcher_immediate(&p_data_manager_interface->_task_ptr->_dispatcher);
        dt_start_dispatcher_immediate(p_data_manager_interface->_task_ptr);
    }

    p_data_manager_interface->_conrinue_times = 0;

    return SUCCESS;
}

_int32 dm_get_range_data_buffer( DATA_MANAGER* p_data_manager_interface, RANGE need_r,  RANGE_DATA_BUFFER_LIST* ret_range_buffer)
{
    LOG_DEBUG("dm_get_range_data_buffer ... need_range(%d,%d)", need_r._index, need_r._num);

    if(p_data_manager_interface == NULL || ret_range_buffer == NULL)
        return -1;

    if(need_r._num == 0)
    {
        return -1;
    }

    return file_info_get_range_data_buffer(&p_data_manager_interface->_file_info,  need_r, ret_range_buffer);

}


#ifdef _VOD_NO_DISK

_int32 dm_vod_set_no_disk_mode( DATA_MANAGER* p_data_manager_interface)
{

    LOG_DEBUG( "dm_vod_set_no_disk_mode." );

    file_info_vod_set_no_disk_mode(&p_data_manager_interface->_file_info);

    p_data_manager_interface->_is_no_disk = TRUE;

    return SUCCESS;
}

BOOL dm_vod_is_no_disk_mode(DATA_MANAGER* p_data_manager_interface)
{
    LOG_DEBUG( "dm_vod_is_no_disk_mode %u." , p_data_manager_interface->_is_no_disk);
    return p_data_manager_interface->_is_no_disk;
}

_int32 dm_vod_no_disk_notify_free_data_buffer( DATA_MANAGER* p_data_manager_interface,   RANGE_DATA_BUFFER_LIST* del_range_buffer)
{




    RANGE_LIST  del_range_list;
    // RANGE_LIST_ITEROATOR  del_it = NULL;

    sd_assert(p_data_manager_interface->_is_no_disk == TRUE);

    range_list_init(&del_range_list);

    if(del_range_buffer == NULL)
        return SUCCESS;

    get_range_list_from_buffer_list(del_range_buffer, &del_range_list);

    /*only free buffer , not free total receive range list*/
    dm_notify_only_free_data_buffer(p_data_manager_interface, &del_range_list);

    correct_manager_del_prority_range_list(&p_data_manager_interface->_correct_manage, &del_range_list);
    /*  if(del_range_list._node_size == 0)
        return SUCCESS;

      range_list_get_head_node(&del_range_list, &del_it);

      while(del_it != NULL)
      {
               file_info_delete_range(&p_data_manager_interface->_file_info, &del_it->_range);

         range_manager_erase_range(&p_data_manager_interface->_range_record, &del_it->_range, NULL);

         range_list_get_next_node(&del_range_list, del_it, &del_it);
      }*/

    range_list_clear(&del_range_list);

    return SUCCESS;

    //file_info_vod_no_disk_notify_free_data_buffer(&p_data_manager_interface->_file_info,del_range_buffer);
}

_int32 dm_notify_free_data_buffer( DATA_MANAGER* p_data_manager_interface,   RANGE_LIST* del_range_list)
{


    RANGE_LIST_ITEROATOR  del_it = NULL;

    if(del_range_list->_node_size == 0)
        return SUCCESS;

    range_list_get_head_node(del_range_list, &del_it);

    while(del_it != NULL)
    {
        file_info_delete_range(&p_data_manager_interface->_file_info, &del_it->_range);

        range_manager_erase_range(&p_data_manager_interface->_range_record, &del_it->_range, NULL);

        range_list_get_next_node(del_range_list, del_it, &del_it);
    }

    return SUCCESS;

    //file_info_vod_no_disk_notify_free_data_buffer(&p_data_manager_interface->_file_info,del_range_buffer);
}

_int32 dm_notify_only_free_data_buffer( DATA_MANAGER* p_data_manager_interface,   RANGE_LIST* del_range_list)
{


    RANGE_LIST_ITEROATOR  del_it = NULL;

    if(del_range_list->_node_size == 0)
        return SUCCESS;

    range_list_get_head_node(del_range_list, &del_it);

    while(del_it != NULL)
    {
        file_info_delete_buffer_by_range(&p_data_manager_interface->_file_info, &del_it->_range);

        range_list_get_next_node(del_range_list, del_it, &del_it);
    }

    return SUCCESS;

    //file_info_vod_no_disk_notify_free_data_buffer(&p_data_manager_interface->_file_info,del_range_buffer);
}

_int32 dm_drop_buffer_not_in_emerge_windows( DATA_MANAGER* p_data_manager_interface, RANGE_LIST* em_range_windows)
{
    RANGE_LIST * p_recv_list = NULL;
    RANGE_LIST  tmp_range_list ;

    RANGE_LIST_ITEROATOR range_it = NULL;
    //RANGE_LIST_ITEROATOR begin_it = NULL;
    RANGE  block_r ;
    //RANGE  del_range ;
    RANGE  full_range;

    sd_assert(p_data_manager_interface->_is_no_disk == TRUE);

    /*  need add  free buffer which is no need */
    p_recv_list =  file_info_get_recved_range_list(&p_data_manager_interface->_file_info);


    /* range_list_cp_range_list(p_recv_list, &tmp_range_list);

     out_put_range_list(&tmp_range_list);

          if(p_data_manager_interface->_is_check_data == FALSE)
          {
                     range_list_delete_range_list(&tmp_range_list, em_range_windows);
          }
       else
       {
                 range_list_get_head_node(em_range_windows, &range_it);

                 while(range_it != NULL)
                 {
                        block_r =  range_to_block_range(&(range_it->_range), p_data_manager_interface->_file_info._block_size, p_data_manager_interface->_file_info._file_size);

                       range_list_delete_range(&tmp_range_list, &block_r, begin_it, &begin_it);

                  range_list_get_next_node(em_range_windows, range_it, &range_it);
                 }
      }*/

    LOG_ERROR( "dm_drop_buffer_not_in_emerge_windows. em_window_list" );
    force_out_put_range_list(em_range_windows);
    range_list_get_head_node(em_range_windows, &range_it);

    if(range_it  == NULL || range_it->_range._num == 0)
    {
        return SUCCESS;
    }


    range_list_init(&tmp_range_list);
    full_range = pos_length_to_range(0, p_data_manager_interface->_file_info._file_size, p_data_manager_interface->_file_info._file_size);
    range_list_add_range(&tmp_range_list, &full_range,NULL, NULL);
    LOG_ERROR( "dm_drop_buffer_not_in_emerge_windows. before DEL" );
    force_out_put_range_list(&tmp_range_list);

    if(p_data_manager_interface->_is_check_data == FALSE)
    {
        range_list_delete_range_list(&tmp_range_list, em_range_windows);
        LOG_ERROR( "dm_drop_buffer_not_in_emerge_windows. after DEL" );
    }
    else
    {
        range_list_get_head_node(em_range_windows, &range_it);
        while(NULL != range_it)
        {
            block_r =  range_to_block_range(&(range_it->_range), p_data_manager_interface->_file_info._block_size, p_data_manager_interface->_file_info._file_size);

            range_list_delete_range(&tmp_range_list, &block_r,NULL, NULL);
            range_it = range_it->_next_node;
        }
    }


    LOG_ERROR( "dm_drop_buffer_not_in_emerge_windows." );

    force_out_put_range_list(&tmp_range_list);

    if(tmp_range_list._node_size != 0)
    {
        dm_notify_free_data_buffer(p_data_manager_interface, &tmp_range_list);

        correct_manager_del_prority_range_list(&p_data_manager_interface->_correct_manage, &tmp_range_list);

        range_list_get_head_node(em_range_windows, &range_it);
        if(range_it->_range._index> p_data_manager_interface->_start_pos_index)
        {
            p_data_manager_interface->_start_pos_index = range_it->_range._index ;
        }
    }

    range_list_clear(&tmp_range_list);
    return SUCCESS;
}


_int32 dm_flush_data_to_vod_cache( DATA_MANAGER* p_data_manager_interface)
{
    RANGE_DATA_BUFFER_LIST*  buffer_list = NULL;
    LIST_ITERATOR buffer_list_node = NULL;
    RANGE_DATA_BUFFER* buffer_node = NULL;
    _int32 ret_val = SUCCESS;
    RANGE_LIST  tmp_range_list;
    RANGE_LIST_ITEROATOR begin_node = NULL;

    range_list_init(&tmp_range_list);

    buffer_list = file_info_get_cache_range_buffer_list(&p_data_manager_interface->_file_info);

    if(buffer_list == NULL)
        return  SUCCESS;

    buffer_list_node = LIST_BEGIN(buffer_list->_data_list);

    while(buffer_list_node != LIST_END(buffer_list->_data_list))
    {
        buffer_node = (RANGE_DATA_BUFFER* )LIST_VALUE(buffer_list_node);

        ret_val =  pt_dm_notify_buffer_recved(p_data_manager_interface->_task_ptr, buffer_node );

        if(ret_val == SUCCESS)
        {
            range_list_add_range(&tmp_range_list, &buffer_node->_range, begin_node, &begin_node);
        }

        buffer_list_node = LIST_NEXT(buffer_list_node);
    }

    /*only free buffer , not free total receive range list*/
    dm_notify_only_free_data_buffer(p_data_manager_interface, &tmp_range_list);

    correct_manager_del_prority_range_list(&p_data_manager_interface->_correct_manage, &tmp_range_list);

    range_list_clear(&tmp_range_list);

    return SUCCESS;

}


#endif

_int32 dm_vod_set_check_data_mode( DATA_MANAGER* p_data_manager_interface)
{

    LOG_DEBUG( "dm_vod_set_check_data_mode." );

    p_data_manager_interface->_is_check_data = TRUE;

    return SUCCESS;
}

BOOL dm_vod_is_check_data_mode(DATA_MANAGER* p_data_manager_interface)
{
    return p_data_manager_interface->_is_check_data;
}

_int32  dm_asyn_read_data_buffer( DATA_MANAGER* p_data_manager_interface, RANGE_DATA_BUFFER *p_data_buffer, notify_read_result p_call_back, void *p_user )
{
    // _int32 ret_val = 0;

    if(p_data_manager_interface == NULL || p_data_buffer == NULL || p_user == NULL ||p_data_manager_interface->_file_info._tmp_file_struct == NULL)
    {
        LOG_DEBUG( "dm_asyn_read_data_buffer  invalid parameter." );
        return -1;
    }

    return file_info_asyn_read_data_buffer(&p_data_manager_interface->_file_info,  p_data_buffer, p_call_back,  p_user);
}



void   dm_notify_file_create_result (void* p_user_data, FILEINFO* p_file_infor, _int32 creat_result)
{
    DATA_MANAGER* p_data_manager_interface = (DATA_MANAGER* ) p_user_data;

    LOG_DEBUG( "dm_notify_file_create_result  user data:0x%xm, file_info :0x%x , result:%d .",  p_user_data, p_file_infor, creat_result);

    if(&p_data_manager_interface->_file_info != p_file_infor)
    {
        LOG_ERROR( "dm_notify_file_create_result  user data:0x%xm, file_info :0x%x , result:%d , file info is so equal.",  p_user_data, p_file_infor, creat_result);
        return;
    }

    if(creat_result == SUCCESS)
    {
        data_manager_notify_file_create_info(p_data_manager_interface, TRUE);
    }
    else
    {
        data_manager_notify_file_create_info( p_data_manager_interface, FALSE);

        /* file create failure so data manager failure*/
        data_manager_notify_failure(p_data_manager_interface, creat_result);
    }

    return ;

}
void  dm_notify_file_close_result  (void* p_user_data, FILEINFO* p_file_infor, _int32 close_result)
{
    DATA_MANAGER* p_data_manager_interface = (DATA_MANAGER* ) p_user_data;

    LOG_DEBUG( "dm_notify_file_close_result  user data:0x%xm, file_info :0x%x , result:%d .",  p_user_data, p_file_infor, close_result);

    if(&p_data_manager_interface->_file_info != p_file_infor)
    {
        LOG_ERROR( "dm_notify_file_close_result  user data:0x%xm, file_info :0x%x , result:%d , file info is so equal.",  p_user_data, p_file_infor, close_result);
        return;
    }

    data_manager_notify_file_close_info(p_data_manager_interface,  close_result);

    return ;
}
void  dm_notify_file_block_check_result(void* p_user_data, FILEINFO* p_file_infor, RANGE* p_range, BOOL  check_result)
{
    DATA_MANAGER* p_data_manager_interface = (DATA_MANAGER* ) p_user_data;

    LOG_DEBUG( "dm_notify_file_block_check_result  user data:0x%x, file_info :0x%x , range (%u, %u), result:%d .",  p_user_data, p_file_infor, p_range->_index,p_range->_num, (_int32)(check_result));

    if(&p_data_manager_interface->_file_info != p_file_infor)
    {
        LOG_DEBUG( "dm_notify_file_block_check_result  user data:0x%x, file_info :0x%x , range (%u, %u), result:%d , file info is not equal.",  p_user_data, p_file_infor, p_range->_index,p_range->_num, (_int32)check_result);
        return;
    }
    if( p_data_manager_interface->_file_info._wait_for_close ) return;
    if(check_result == TRUE)
    {
        dm_check_block_success(p_data_manager_interface, p_range);
    }
    else
    {

        dm_check_block_failure(p_data_manager_interface, p_range);
    }

    return ;
}

void  dm_notify_file_result  (void* p_user_data, FILEINFO* p_file_infor, _u32 file_result)
{

    DATA_MANAGER* p_data_manager_interface = (DATA_MANAGER* ) p_user_data;

    LOG_DEBUG( "dm_notify_file_result  user data:0x%xm, file_info :0x%x , result:%d .",  p_user_data, p_file_infor, file_result);

    if(&p_data_manager_interface->_file_info != p_file_infor)
    {
        LOG_ERROR( "dm_notify_file_result  user data:0x%xm, file_info :0x%x , result:%d , file info is so equal.",  p_user_data, p_file_infor, file_result);
        return;
    }

    if(file_result == SUCCESS)
    {
        data_manager_notify_success(p_data_manager_interface);
    }
    else
    {
        data_manager_notify_failure(p_data_manager_interface, file_result);
    }

    return ;
}


void  dm_get_buffer_list_from_cache(void* p_user_data, RANGE* r,  RANGE_DATA_BUFFER_LIST* ret_range_buffer)
{
#ifdef _VOD_NO_DISK
    DATA_MANAGER* p_data_manager_interface = (DATA_MANAGER* ) p_user_data;

    if( p_data_manager_interface->_file_info._wait_for_close ) return;

    LOG_DEBUG( "dm_get_buffer_list_from_cache  user data:0x%xm, range(%u,%u).",  p_user_data, r->_index, r->_num);
    if(p_data_manager_interface->_is_no_disk == TRUE)
    {
        pt_dm_get_data_buffer_by_range(p_data_manager_interface->_task_ptr, r , ret_range_buffer);
        return ;
    }
#endif

    return;
}


_int32 dm_get_dispatcher_data_info(DATA_MANAGER* p_data_manager_interface, DS_DATA_INTEREFACE* p_data_info)
{
    p_data_info->_get_file_size =  dm_ds_intereface_get_file_size;
    p_data_info->_is_only_using_origin_res = dm_ds_intereface_is_only_using_origin_res;
    p_data_info->_is_origin_resource =  dm_ds_intereface_is_origin_resource;
    p_data_info->_get_priority_range = dm_ds_intereface_get_priority_range;
    p_data_info->_get_uncomplete_range =  dm_ds_intereface_get_uncomplete_range ;
    p_data_info->_get_error_range_block_list = dm_ds_intereface_get_error_range_block_list ;
    p_data_info->_get_ds_is_vod_mode = dm_ds_is_vod_mod ;

    p_data_info->_own_ptr = (void*)p_data_manager_interface;

    return SUCCESS;

}

_u64  dm_ds_intereface_get_file_size( void* p_owner_data_manager)
{
    DATA_MANAGER* p_data_manager_interface = (DATA_MANAGER* ) p_owner_data_manager;
    LOG_DEBUG( "dm_ds_intereface_get_file_size  user data:0x%x .",  p_data_manager_interface);

    return dm_get_file_size(p_data_manager_interface);
}

BOOL dm_ds_intereface_is_only_using_origin_res(void* p_owner_data_manager)
{
    DATA_MANAGER* p_data_manager_interface = (DATA_MANAGER* ) p_owner_data_manager;
    LOG_DEBUG( "dm_ds_intereface_is_only_using_origin_res  user data:0x%x .",  p_data_manager_interface);

    return dm_is_only_using_origin_res(p_data_manager_interface);
}
BOOL dm_ds_intereface_is_origin_resource( void* p_owner_data_manager, RESOURCE*  res)
{
    DATA_MANAGER* p_data_manager_interface = (DATA_MANAGER* ) p_owner_data_manager;
    LOG_DEBUG( "dm_ds_intereface_is_origin_resource  user data:0x%x .",  p_data_manager_interface);

    return dm_is_origin_resource(p_data_manager_interface,res);
}

_int32 dm_ds_intereface_get_priority_range(void* p_owner_data_manager, RANGE_LIST*  priority_range_list)
{
    DATA_MANAGER* p_data_manager_interface = (DATA_MANAGER* ) p_owner_data_manager;
    LOG_DEBUG( "dm_ds_intereface_get_priority_range  user data:0x%x .",  p_data_manager_interface);

    return dm_get_priority_range(p_data_manager_interface, priority_range_list);
}

_int32 dm_ds_intereface_get_uncomplete_range(void* p_owner_data_manager, RANGE_LIST*  un_complete_range_list)
{
    DATA_MANAGER* p_data_manager_interface = (DATA_MANAGER* ) p_owner_data_manager;
    LOG_DEBUG( "dm_ds_intereface_get_uncomplete_range  user data:0x%x .",  p_data_manager_interface);

    return dm_get_uncomplete_range(p_data_manager_interface, un_complete_range_list);
}

_int32 dm_ds_intereface_get_error_range_block_list(void* p_owner_data_manager, ERROR_BLOCKLIST**  pp_error_block_list)
{
    DATA_MANAGER* p_data_manager_interface = (DATA_MANAGER* ) p_owner_data_manager;
    LOG_DEBUG( "dm_ds_intereface_get_error_range_block_list  user data:0x%x .",  p_data_manager_interface);

    return dm_get_error_range_block_list(p_data_manager_interface, pp_error_block_list);
}


BOOL dm_ds_is_vod_mod(void* p_owner_data_manager)
{
    DATA_MANAGER* p_data_manager_interface = (DATA_MANAGER* ) p_owner_data_manager;
    LOG_DEBUG( "dm_ds_is_vod_mod  user data:0x%x .",  p_data_manager_interface);

    return dm_is_vod_mode(p_data_manager_interface);
}


BOOL dm_abandon_resource( void* p_data_manager, RESOURCE*  abdon_res)
{
    DATA_MANAGER* p_data_manager_interface = (DATA_MANAGER* ) p_data_manager;

    LOG_DEBUG("dm_abandon_resource,DATA_MANAGER:0x%x  ,RESOURCE is:0x%x .", p_data_manager_interface, abdon_res);

    if(abdon_res == NULL)
    {
        return TRUE;
    }

    correct_manager_erase_abandon_res(&p_data_manager_interface->_correct_manage,  abdon_res);

    /* if(range_manager_has_resource(&p_data_manager_interface->_range_record, abdon_res) == TRUE)
     {
            LOG_DEBUG("dm_abandon_resource,DATA_MANAGER:0x%x  , abandon RESOURCE is:0x%x  failure.", p_data_manager_interface, abdon_res);
            return FALSE;
     }
     else
     {
            LOG_DEBUG("dm_abandon_resource,DATA_MANAGER:0x%x  , abandon RESOURCE is:0x%x success.", p_data_manager_interface, abdon_res);
            return TRUE;
     }*/

    range_manager_erase_resource(&p_data_manager_interface->_range_record, abdon_res);
    return TRUE;

}

_u32 dm_get_valid_data_speed(DATA_MANAGER* p_data_manager_interface)
{
    return file_info_get_valid_data_speed(&p_data_manager_interface->_file_info);
}

void dm_get_dl_bytes(DATA_MANAGER* p_data_manager_interface, _u64 *p_server_dl_bytes, _u64 *p_peer_dl_bytes, _u64 *p_cdn_dl_bytes, _u64 *p_lixian_dl_bytes , _u64* p_appacc_dl_bytes)
{
    if(p_server_dl_bytes)
        *p_server_dl_bytes = p_data_manager_interface->_server_dl_bytes;

    if(p_peer_dl_bytes)
        *p_peer_dl_bytes = p_data_manager_interface->_peer_dl_bytes;

    if(p_cdn_dl_bytes)
        *p_cdn_dl_bytes = p_data_manager_interface->_cdn_dl_bytes;

    if(p_lixian_dl_bytes)
        *p_lixian_dl_bytes = p_data_manager_interface->_lixian_dl_bytes;

    if(p_appacc_dl_bytes)
    	 *p_appacc_dl_bytes = p_data_manager_interface->_appacc_dl_bytes;
}

_u64 dm_get_normal_cdn_down_bytes(DATA_MANAGER *data_manager)
{
	if (!data_manager) return 0;

	return data_manager->_normal_cdn_dl_bytes;
}

#ifdef EMBED_REPORT
struct tagFILE_INFO_REPORT_PARA *dm_get_report_para( DATA_MANAGER* p_data_manager_interface )
{
    return file_info_get_report_para( &p_data_manager_interface->_file_info );
}
#endif

BOOL dm_check_kankan_lan_file_finished(DATA_MANAGER* p_data_manager_interface)
{
    char cfg_file[MAX_FULL_PATH_BUFFER_LEN] = {0};
    char * pos = NULL;
    if(p_data_manager_interface->_file_info._write_mode ==FWM_RANGE)
    {
        sd_snprintf(cfg_file,MAX_FULL_PATH_BUFFER_LEN-1,"%s/%s",p_data_manager_interface->_file_info._path,p_data_manager_interface->_file_info._file_name);
        pos = sd_strstr(cfg_file,".rf",0);
        if(pos)
        {
            pos[0] = '\0';
        }

        sd_strcat(cfg_file, ".rf.cfg",sd_strlen(".rf.cfg"));
        if(sd_file_exist(cfg_file))
        {
            return FALSE;
        }
    }

    return TRUE;
}

void dm_get_origin_resource_dl_bytes(DATA_MANAGER* p_data_manager_interface, _u64 *p_origin_resource_dl_bytes)
{
    if(p_origin_resource_dl_bytes)
        *p_origin_resource_dl_bytes = p_data_manager_interface->_origin_dl_bytes;
}


void dm_get_assist_peer_dl_bytes(DATA_MANAGER* p_data_manager_interface, _u64 *p_assist_peer_dl_bytes)
{
    if(p_assist_peer_dl_bytes)
        *p_assist_peer_dl_bytes = p_data_manager_interface->_assist_peer_dl_bytes;
}

_int32 dm_set_file_name_changed_callback(void * call_ptr)
{
    if (NULL == call_ptr)
    {
        return INVALID_HANDLER;
    }    
    g_notify_filename_changed_callback = (DM_NOTIFY_FILE_NAME_CHANGED)call_ptr;
	LOG_DEBUG("dm_set_file_name_changed_callback:%x", call_ptr);
    return SUCCESS;
}

_int32 dm_get_file_name_changed_callback(DM_NOTIFY_FILE_NAME_CHANGED *call_ptr)
{
    if (NULL == call_ptr)
    {
        return INVALID_HANDLER;
    }    
    *call_ptr = g_notify_filename_changed_callback;
    return SUCCESS;
}

_int32 dm_set_notify_event_callback(void * call_ptr)
{
    if (NULL == call_ptr)
    {
        return INVALID_HANDLER;
    }
    g_notify_event_callback = (DM_NOTIFY_EVENT)call_ptr;
	LOG_DEBUG("dm_set_notify_event_callback:0x%x", call_ptr);
    return SUCCESS;
}

_int32 dm_set_notify_etm_scheduler(void* call_ptr)
{
    if (NULL == call_ptr)
    {
        return INVALID_HANDLER;
    }
    g_notify_etm_scheduler = (EM_SCHEDULER)call_ptr;
	LOG_DEBUG("dm_set_notify_etm_scheduler:0x%x", call_ptr);
    return SUCCESS;
}

_int32 dm_set_check_mode(DATA_MANAGER* p_data_manager_interface, BOOL is_need_calc_bcid)
{
    return file_info_set_check_mode(&p_data_manager_interface->_file_info, is_need_calc_bcid);
}

_int32 dm_get_download_bytes(DATA_MANAGER* p_data_manager_interface, char* host, _u64* download)
{
    _u64 file_size = dm_get_file_size(p_data_manager_interface);
    range_manager_get_download_bytes(&p_data_manager_interface->_range_record, host, download, file_size);
    return SUCCESS;
}

_int32 dm_get_info_from_cfg_file(char* cfg_file_path, char *origin_url, BOOL *cid_is_valid, _u8 *cid, _u64* file_size, char* file_name,char *lixian_url, char *cookie, char *user_data)
{
	if(file_info_get_info_from_cfg_file(cfg_file_path, origin_url, cid_is_valid, cid, file_size, file_name, lixian_url, cookie, user_data) == FALSE)
		return OPEN_OLD_FILE_FAIL;
	else
		return SUCCESS;
}

