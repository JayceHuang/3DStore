
#include "utility/string.h"
#include "utility/utility.h"
#include "utility/settings.h"
#include "utility/local_ip.h"
#include "utility/peer_capability.h"
#include "utility/bytebuffer.h"

#include "platform/sd_fs.h"
#include "file_info.h"
#include "file_manager_interface.h"
#include "data_manager_interface.h"
#include "http_data_pipe/http_resource.h"
#include "calc_hash_helper.h"

#ifdef EMBED_REPORT
#include "p2p_data_pipe/p2p_pipe_interface.h"
#endif

#include "utility/logid.h"
#define LOGID LOGID_FILE_INFO
#include "utility/logger.h"


#define POSTFIXS_SIZE 56

/*static _u8 postfixs_len[POSTFIXS_SIZE] =
{
    3, 3 , 3, 4, 4,
        2 , 4, 3, 3,  3,
        2 , 2, 3, 3,  3,
        3,  3 , 3, 3,  3,
        3, 3 , 3, 3,  3,
        3, 3 , 3, 3,  3,
        3, 3 , 3, 3,  7
};*/

static _u32 MAX_READ_LENGTH =  DATA_CHECK_MAX_READ_LENGTH;

static _u8 bitmap_mask[8]= {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
static _u8 bitmap_mask2[8]= {0x01,0x03,0x07,0x0F,0x1F,0x3F,0x7F,0xFF};


static SLAB* g_file_info_node_slab = NULL;
static _u32 MIN_FILE_INFO_NODE = 10;

static _int32 g_fast_stop = 1;

static  char postfixs[POSTFIXS_SIZE][10] =
{
    ".jpg",".png",".gif",".bmp",".tiff",".raw",             /* Õº∆¨ */
    ".chm",".doc",".docx",".pdf",".txt",    ".xls",".xlsx",".ppt",  ".pptx",                /* ÔøΩƒ±ÔøΩ */
    ".aac",".ape",".amr",".mp3",".ogg",".wav",".wma",               /* ÔøΩÔøΩ∆µ */
    ".xv",".avi",".asf",".mpeg",".mpga", ".mpg",".mp4",".m3u",".mov",".wmv",".rmvb",".rm",".3gp", ".dat",".flv",".mkv",".xlmv",".swf", /* ÔøΩÔøΩ∆µ */
    ".exe", ".msi", ".jar" , ".ipa", ".apk",".img",                 /* ”¶ÔøΩ√≥ÔøΩÔøΩÔøΩ */
    ".rar",  ".zip",   ".iso",  ".bz2", ".tar",".ra",  ".gz" ,".7z",        /* —πÔøΩÔøΩÔøΩƒµÔøΩ */
    ".bin",".torrent"                                  /* ÔøΩÔøΩÔøΩÔøΩ */
};

#define MAX_MEDIA_CHECK_BLOCK_NUM 3

#define MEDIA_POSTFIXS_SIZE 18
static  char media_postfixs[MEDIA_POSTFIXS_SIZE][10] =
{
    ".xv",".avi",".asf",".mpeg",".mpga", ".mpg",".mp4",".m3u",".mov",".wmv",".rmvb",".rm",".3gp", ".dat",".flv",".mkv",".xlmv",".swf"  /* ÔøΩÔøΩ∆µ */
};

static _int32 file_info_calc_hash_notify_result(CHH_HASH_INFO* hash_result, void* user_data);



static _u32 set_bitmap(_u8* bitmap, _u32 blockno)
{
    _u32  index  = get_bitmap_index(blockno);
    _u32  off = get_bitmap_off(blockno);

    LOG_DEBUG("set_bitmap, check blockno:%u, index:%u, off:%u .", blockno, index, off);

    bitmap[index] |= bitmap_mask[off];
    return SUCCESS;
}

static _u32 unset_bitmap(_u8* bitmap, _u32 blockno)
{
    _u32  index  = get_bitmap_index(blockno);
    _u32  off = get_bitmap_off(blockno);

    LOG_DEBUG("unset_bitmap, check blockno:%u, index:%u, off:%u .", blockno, index, off);

    bitmap[index] &= ~bitmap_mask[off];
    return SUCCESS;
}

static BOOL bitmap_is_set(_u8*  bitmap, _u32 blockno)
{
    _u32  index  = get_bitmap_index(blockno);
    _u32  off = get_bitmap_off(blockno);

    if((bitmap[index] & bitmap_mask[off]) == bitmap_mask[off] )
    {
        LOG_DEBUG("bitmap_is_set, check blockno:%u, index:%u, off:%u is set.", blockno, index, off);

        return TRUE;
    }
    else
    {
        LOG_DEBUG("bitmap_is_set, check blockno:%u, index:%u, off:%u is not set.", blockno, index, off);
        return FALSE;
    }
}


char*  get_file_suffix( const char* filename )
{
    char* pos = sd_strrchr((char*)filename,'.');
    if( pos == NULL )
        return NULL;
    else
    {
        return pos;
    }
}

BOOL is_excellent_filename( char* filename )
{
    int i= 0;
    char* postfix = get_file_suffix( filename );

    if(postfix == NULL)
        return FALSE;

    for(i=0; i < POSTFIXS_SIZE; i++)
    {
        if(sd_stricmp(postfix, postfixs[i]) == 0)
            return TRUE;
    }
    return FALSE;
}

BOOL file_is_media_file(char* filename)
{
    int i= 0;
    char* suffix_postfix = NULL;
    char* td_postfix = get_file_suffix( filename );

    if(td_postfix == NULL)
        return FALSE;

    if(sd_stricmp(td_postfix+1, "rf") == 0)
    {
        *td_postfix = '\0';
        suffix_postfix = get_file_suffix( filename );
        if(suffix_postfix== NULL)
        {
            *td_postfix = '.';
            return FALSE;

        }
    }
    else
    {
        suffix_postfix = td_postfix;
        td_postfix = NULL;
    }



    for(i=0; i < MEDIA_POSTFIXS_SIZE; i++)
    {
        if(sd_stricmp(suffix_postfix+1, media_postfixs[i]) == 0)
        {
            if(td_postfix != NULL)
            {
                *td_postfix = '.';
            }
            return TRUE;
        }
    }

    if(td_postfix != NULL)
    {
        *td_postfix = '.';
    }
    return FALSE;
}

_int32 file_info_adjust_max_read_length(void)
{
    if(MAX_READ_LENGTH < get_data_unit_size())
    {
        MAX_READ_LENGTH = get_data_unit_size();
        LOG_DEBUG("file_info_adjust_max_read_length:%u." ,MAX_READ_LENGTH);
    }

    return SUCCESS;
}

_int32 init_file_info_slabs(void)
{
    _int32 ret_val = SUCCESS;

    if(g_file_info_node_slab == NULL)
    {
        ret_val = mpool_create_slab(sizeof(FILEINFO), MIN_FILE_INFO_NODE, 0, &g_file_info_node_slab);
        CHECK_VALUE(ret_val);
    }

    return (ret_val);
}

_int32 uninit_file_info_slabs(void)
{
    _int32 ret_val = SUCCESS;
    if(g_file_info_node_slab !=  NULL)
    {
        mpool_destory_slab(g_file_info_node_slab);
        g_file_info_node_slab = NULL;
    }

    return (ret_val);
}

_int32 file_info_alloc_node(FILEINFO** pp_node)
{
    return  mpool_get_slip(g_file_info_node_slab, (void**)pp_node);
}

_int32 file_info_free_node(FILEINFO* p_node)
{
    return  mpool_free_slip(g_file_info_node_slab, (void*)p_node);
}

_int32  init_file_info(FILEINFO* p_file_info, void* owner_ptr)
{
    LOG_DEBUG("init_file_info:0x%x, owner ptr:0x%x." ,p_file_info, owner_ptr);

    sd_memset(p_file_info->_file_name, 0, MAX_FILE_NAME_BUFFER_LEN);
    sd_memset(p_file_info->_path, 0, MAX_FILE_PATH_LEN);
    sd_memset(p_file_info->_user_file_name , 0, MAX_FILE_NAME_BUFFER_LEN);
    p_file_info->_has_postfix = FALSE;
    p_file_info->_cfg_file = 0;

    p_file_info->_file_size = 0;
    p_file_info->_block_size = 0;
    p_file_info->_unit_num_per_block = 0;

    p_file_info->_cid_is_valid = FALSE;
    sd_memset(p_file_info->_cid, 0, CID_SIZE);

    p_file_info->_user_data = NULL;
    p_file_info->_user_data_len = 0;

    p_file_info->_gcid_is_valid = FALSE;
    sd_memset(p_file_info->_gcid, 0, CID_SIZE);

    p_file_info->_bcid_is_valid = FALSE;
	p_file_info->_is_backup_bcid_valid = FALSE;
    p_file_info->_is_calc_bcid = TRUE;
    sd_memset(&p_file_info->_bcid_info, 0, sizeof(bcid_info));
	sd_memset(&p_file_info->_bcid_info_backup, 0, sizeof(bcid_info));
    p_file_info->_bcid_map_from_cfg = FALSE;
    p_file_info->_is_reset_bcid = FALSE;
    p_file_info->_has_history_check_info = FALSE;

    sd_memset(p_file_info->_origin_url , 0, MAX_URL_LEN);
    sd_memset(p_file_info->_origin_ref_url , 0, MAX_URL_LEN);

    range_list_init( &p_file_info->_done_ranges);
    range_list_init( &p_file_info->_writed_range_list);

    data_receiver_init(&p_file_info->_data_receiver);

    list_init(&p_file_info->_flush_buffer_list._data_list);

    sd_memset(&p_file_info->_block_caculator, 0, sizeof(BLOCK_CACULATOR));

    list_init(&p_file_info->_block_caculator._need_calc_blocks);
    p_file_info->_block_caculator._cal_blockno = MAX_U32;
    p_file_info->_block_caculator._cal_length = 0;
    p_file_info->_block_caculator._data_buffer = NULL;
    p_file_info->_block_caculator._expect_length = 0;


    p_file_info->_tmp_file_struct = NULL;
    p_file_info->_wait_for_close = FALSE;

    sd_memset(&p_file_info->_call_back_func_table, 0, sizeof(FILE_INFO_CALLBACK));

    p_file_info->_p_user_ptr = owner_ptr;

    range_list_init( &p_file_info->_no_need_check_range);
    p_file_info->_check_file_type = UNKOWN_FILE_TYPE;

#ifdef _VOD_NO_DISK
    p_file_info->_is_no_disk = FALSE;
#endif

    init_speed_calculator(&(p_file_info->_cur_data_speed), 50, 100);

    p_file_info->_bcid_enable = TRUE;

#ifdef EMBED_REPORT
    sd_memset(&p_file_info->_report_para, 0, sizeof(FILE_INFO_REPORT_PARA) );
#endif
    p_file_info->_write_mode = FWM_RANGE;
    p_file_info->_finished = FALSE;

    p_file_info->_is_need_calc_bcid = TRUE;

    sd_malloc(sizeof(HASH_CALCULATOR), (void**)&p_file_info->_hash_calculator);
    chh_init(p_file_info->_hash_calculator, (void*)p_file_info, file_info_calc_hash_notify_result);

    return SUCCESS;
}


/*  file_info_close_tmp_file must be reference before and the close callback must return */

_int32  unit_file_info(FILEINFO* p_file_info)
{
    LOG_DEBUG("unit_file_info .");
   
    if(p_file_info->_cfg_file != 0)
    {
        sd_close_ex(p_file_info->_cfg_file);
        p_file_info->_cfg_file = 0;
    }

    if (p_file_info->_user_data_len)
    {
        sd_free(p_file_info->_user_data);
        p_file_info->_user_data = NULL;
        p_file_info->_user_data_len = 0;
    }
    range_list_clear( &p_file_info->_done_ranges);
    range_list_clear( &p_file_info->_writed_range_list);

    data_receiver_unit(&p_file_info->_data_receiver);

    drop_buffer_list(&p_file_info->_flush_buffer_list);

    clear_check_blocks(p_file_info);

    if(p_file_info->_bcid_info._bcid_buffer !=  NULL)
    {
        sd_free(p_file_info->_bcid_info._bcid_buffer);
        p_file_info->_bcid_info._bcid_buffer = NULL;
    }

    if(p_file_info->_bcid_info._bcid_bitmap != NULL)
    {
        sd_free(p_file_info->_bcid_info._bcid_bitmap);
        p_file_info->_bcid_info._bcid_bitmap = NULL;
    }

    if(p_file_info->_bcid_info._bcid_checking_bitmap != NULL)
    {
        sd_free(p_file_info->_bcid_info._bcid_checking_bitmap);
        p_file_info->_bcid_info._bcid_checking_bitmap = NULL;
    }

	if(p_file_info->_is_backup_bcid_valid 
		&& p_file_info->_bcid_info_backup._bcid_buffer !=  NULL)
	{
		sd_free(p_file_info->_bcid_info_backup._bcid_buffer);
		p_file_info->_bcid_info_backup._bcid_buffer = NULL;
	}

	if(p_file_info->_is_backup_bcid_valid 
		&& p_file_info->_bcid_info_backup._bcid_bitmap != NULL)
	{
		sd_free(p_file_info->_bcid_info_backup._bcid_bitmap);
		p_file_info->_bcid_info_backup._bcid_bitmap = NULL;
	}

    p_file_info->_bcid_map_from_cfg = FALSE;
    p_file_info->_is_reset_bcid = FALSE;
    
    sd_assert(p_file_info->_tmp_file_struct == NULL);

    p_file_info->_tmp_file_struct = NULL;
    p_file_info->_wait_for_close = FALSE;

    if(p_file_info->_block_caculator._cal_blockno == MAX_U32)
    {
        if(p_file_info->_block_caculator._p_buffer_data != NULL)
        {
            if(p_file_info->_block_caculator._p_buffer_data->_data_ptr != NULL)
            {
                sd_free(p_file_info->_block_caculator._p_buffer_data->_data_ptr);
                p_file_info->_block_caculator._p_buffer_data->_data_ptr = NULL;
            }

            //  sd_free(p_data_checker->_p_buffer_data);
            free_range_data_buffer_node(p_file_info->_block_caculator._p_buffer_data);
        }
    }

    p_file_info->_block_caculator._p_buffer_data = NULL;
    p_file_info->_block_caculator._cal_length = 0;
    p_file_info->_block_caculator._cal_blockno = MAX_U32;
    SAFE_DELETE(p_file_info->_block_caculator._data_buffer);
    p_file_info->_block_caculator._expect_length = 0;

    range_list_clear( &p_file_info->_no_need_check_range);
    p_file_info->_check_file_type = UNKOWN_FILE_TYPE;

#ifdef _VOD_NO_DISK
    p_file_info->_is_no_disk = FALSE;
#endif

    uninit_speed_calculator(&(p_file_info->_cur_data_speed));

    // p_file_info->_p_user_ptr = NULL;
    chh_uninit(p_file_info->_hash_calculator);
    sd_free(p_file_info->_hash_calculator);

    return SUCCESS;
}


BOOL  file_info_has_tmp_file(FILEINFO* p_file_info)
{
    return p_file_info->_tmp_file_struct != NULL;
}

_int32  file_info_register_callbacks(FILEINFO* p_file_info, FILE_INFO_CALLBACK* _callback_funcs)
{
    p_file_info->_call_back_func_table._p_notify_create_event = _callback_funcs->_p_notify_create_event;
    p_file_info->_call_back_func_table._p_notify_close_event = _callback_funcs->_p_notify_close_event;
    p_file_info->_call_back_func_table._p_notify_check_event = _callback_funcs->_p_notify_check_event;
    p_file_info->_call_back_func_table._p_notify_file_result_event = _callback_funcs->_p_notify_file_result_event;
    p_file_info->_call_back_func_table._p_get_buffer_list_from_cache = _callback_funcs->_p_get_buffer_list_from_cache;
    return SUCCESS;
}

_int32  file_info_notify_create(FILEINFO* p_file_info, _int32  result)
{
    p_file_info->_call_back_func_table._p_notify_create_event(p_file_info->_p_user_ptr, p_file_info, result);
    return SUCCESS;
}

_int32  file_info_notify_close(FILEINFO* p_file_info, _int32  result)
{
    p_file_info->_call_back_func_table._p_notify_close_event(p_file_info->_p_user_ptr, p_file_info, result);//dm_notify_file_close_result
    return SUCCESS;
}

_int32  file_info_notify_check(FILEINFO* p_file_info, _u32 blockno, BOOL result)
{
    RANGE check_r;
    if(p_file_info->_bcid_enable == TRUE)
    {
        check_r = to_range(blockno, p_file_info->_block_size, p_file_info->_file_size);
        if(result == TRUE)
        {
            file_info_add_done_range(p_file_info, &check_r);
        }
        else
        {
            data_receiver_erase_range(&p_file_info->_data_receiver, &check_r);

            range_list_delete_range(&p_file_info->_writed_range_list, &check_r, NULL, NULL);

        }
        p_file_info->_call_back_func_table._p_notify_check_event(p_file_info->_p_user_ptr, p_file_info, &check_r,result); //dm_notify_file_block_check_result
    }
    return SUCCESS;
}

_int32  file_info_notify_file_result(FILEINFO* p_file_info, _int32  result)
{
    p_file_info->_call_back_func_table._p_notify_file_result_event(p_file_info->_p_user_ptr, p_file_info, result);//dm_notify_file_result
    return SUCCESS;
}

_int32  file_info_set_td_flag(FILEINFO* p_file_info)
{
    LOG_DEBUG("file_info_set_td_flag .");
    p_file_info->_has_postfix = TRUE;
    return SUCCESS;
}


_int32  file_info_add_done_range(FILEINFO* p_file_info, RANGE* r)
{
    _int32 ret_val = SUCCESS;

    LOG_DEBUG("file_info_add_done_range add range (%u,%u).",r->_index, r->_num);

    ret_val  =  range_list_add_range(&p_file_info->_done_ranges,r, NULL, NULL);
    return ret_val;
}

_int32  file_info_add_check_done_range(FILEINFO* p_file_info, RANGE* r)
{
    _int32 ret_val = SUCCESS;

    RANGE_LIST_NODE *_p_ret_node = NULL;
    LOG_DEBUG("file_info_add_check_done_range add range (%u,%u).",r->_index, r->_num);

    ret_val  =  range_list_add_range(&p_file_info->_done_ranges,r, NULL, &_p_ret_node);

    if(NULL != _p_ret_node)
    {
		file_info_sync_bcid_bitmap(p_file_info, _p_ret_node->_range);/* always return SUCCESS */
    }

    return ret_val;
}

/*
 * file_info_sync_bcid_bitmap
 * created by xietao
 */
_int32 file_info_sync_bcid_bitmap(FILEINFO *p_file_info, const RANGE r)
{
    bcid_info *p_bcid_info = &(p_file_info->_bcid_info);
    _u64 index = r._index, num = r._num;
    _u32 block_size = p_file_info->_block_size;
	_u32 unit_size = get_data_unit_size();
    _u64 begin_pos, end_pos;
    _u64 begin_bno, end_bno;
    _u64 *last_bg_bno = &(p_file_info->_bcid_info._last_bg_bno),
          *last_end_bno = &(p_file_info->_bcid_info._last_end_bno);
    int bg_offset, end_offset;
    _u64 bg_index, end_index;

#ifdef _DEBUG
    char buff[1024];
    LOG_DEBUG("syncing range(%u, %u)", r._index, r._num);
    sd_memset(buff,0,1024);
    str2hex( (char*)(p_bcid_info->_bcid_bitmap),( (p_bcid_info->_bcid_num + 7)/8), buff, 1024);
    LOG_DEBUG("before sync bitmap:%s(block_size:%u)", buff, block_size);
#endif /* _DEBUG */

    if( num * unit_size < block_size)
        return SUCCESS;

    begin_pos = (_u64)index * unit_size;
    end_pos = (index + num) * unit_size;

    begin_bno = begin_pos / block_size;
    if(begin_pos % block_size)
        begin_bno++;

    end_bno = end_pos / block_size;
    end_bno--;

    if(begin_bno > end_bno)
        return SUCCESS;

    if(begin_bno == *last_bg_bno)
    {
        begin_bno = *last_end_bno;
        *last_end_bno = end_bno;
    }
    else
    {
        *last_bg_bno = begin_bno;
        *last_end_bno = end_bno;
    }
    /*
        for(; begin_bno <= end_bno; begin_bno++)
            set_blockmap(p_bcid_info, begin_bno);
    */

    bg_offset = begin_bno % 8,
    end_offset = end_bno % 8;
    bg_index = begin_bno / 8,
    end_index = end_bno / 8;

    if (bg_offset)
    {
        for(; bg_offset < 8 && begin_bno < end_bno; bg_offset++)
            set_blockmap(p_bcid_info, begin_bno++);
        if(bg_index < end_index)
            bg_index++;
    }

    if(bg_index < end_index)
        sd_memset(p_bcid_info->_bcid_bitmap + bg_index, ~0, end_index - bg_index);

    if(end_offset)
    {
        for(; end_offset >= 0 && end_bno >= begin_bno; end_offset--)
            set_blockmap(p_bcid_info, end_bno--);
    }
    else
    {
        set_blockmap(p_bcid_info, end_bno);
    }

#ifdef _DEBUG
    sd_memset(buff, 0, 1024);
    str2hex( (char*)(p_bcid_info->_bcid_bitmap),( (p_bcid_info->_bcid_num + 7)/8), buff, 1024);
    LOG_DEBUG("synced bitmap:%s(block_size:%u)", buff, block_size);
#endif /* _DEBUG */

    return SUCCESS;
}

_int32  file_info_add_erase_range(FILEINFO* p_file_info, RANGE* r)
{
    _int32 ret_val = SUCCESS;

    LOG_DEBUG("file_info_add_erase_range erase range (%u,%u).",r->_index, r->_num);

    ret_val  =  range_list_delete_range(&p_file_info->_done_ranges,r, NULL, NULL);
    return ret_val;
}


_int32  file_info_set_user_name(FILEINFO* p_file_info, char* userfilename, char* filepath)
{
    _u32 len = 0;

    LOG_DEBUG("file_info_set_user_name");

    if(userfilename  != NULL)
    {
        LOG_DEBUG("file_info_set_user_name set user name:%s",userfilename);

        len = sd_strlen(userfilename);

        if(len >= MAX_FILE_NAME_BUFFER_LEN)
        {
            return FILE_PATH_TOO_LONG;
        }

        sd_strncpy(p_file_info->_user_file_name, userfilename, len);
        p_file_info->_user_file_name[len] = '\0';
    }

    if(filepath  != NULL)
    {
        LOG_DEBUG("file_info_set_user_name set filepath:%s",filepath);

        len = sd_strlen(filepath);

        if(len >= MAX_FILE_PATH_LEN)
        {
            return FILE_PATH_TOO_LONG;
        }

        sd_strncpy(p_file_info->_path, filepath, len);
        p_file_info->_path[len] = '\0';
    }

    return SUCCESS;
}



_int32  file_info_set_final_file_name(FILEINFO* p_file_info, char*  finalname)
{
    _int32 ret_val = SUCCESS;
    _u32 len = 0;
    LOG_DEBUG("file_info_set_user_name");

    if(finalname  != NULL)
    {
        LOG_DEBUG("file_info_set_user_name set finalname:%s",finalname);

        len = sd_strlen(finalname);

        if(len >=  MAX_FILE_NAME_BUFFER_LEN-4)
        {
            return FILE_PATH_TOO_LONG;
        }

        sd_strncpy(p_file_info->_file_name, finalname, len);
        p_file_info->_file_name[len] = '\0';
    }


    return  ret_val;


}

_int32  file_info_set_origin_url(FILEINFO* p_file_info, char* url, char* origin_ref_url)
{
    _u32 len = 0;

    LOG_DEBUG("file_info_set_origin_url");

    if(url  != NULL)
    {
        LOG_DEBUG("file_info_set_user_name set url:%s",url);

        len = sd_strlen(url);

        if(len >= MAX_URL_LEN)
        {
            return FILE_PATH_TOO_LONG;
        }

        sd_strncpy(p_file_info->_origin_url, url, len);
        p_file_info->_origin_url[len] = '\0';
    }

    if(sd_strcmp(p_file_info->_origin_url, "http://127.0.0.1/") == 0)
    {
        //  file_info_set_down_mode( p_file_info, FALSE);
    }

    if(origin_ref_url  != NULL)
    {
        LOG_DEBUG("file_info_set_user_name set origin_ref_url:%s",origin_ref_url);

        len = sd_strlen(origin_ref_url);

        if(len >= MAX_URL_LEN)
        {
            return FILE_PATH_TOO_LONG;
        }

        sd_strncpy(p_file_info->_origin_ref_url, origin_ref_url, len);
        p_file_info->_origin_ref_url[len] = '\0';
    }

    return SUCCESS;
}

_int32  file_info_set_cookie(FILEINFO* p_file_info, char* cookie)
{
    _u32 len = 0;

    LOG_DEBUG("file_info_set_cookie");

    if(cookie != NULL)
    {
        LOG_DEBUG("file_info_set_cookie set cookie:%s",cookie);

        len = sd_strlen(cookie);

        if(len >= MAX_COOKIE_LEN)
        {
            return FILE_PATH_TOO_LONG;
        }

        sd_strncpy(p_file_info->_cookie, cookie, len);
        p_file_info->_cookie[len] = '\0';
    }
    return SUCCESS;
}

BOOL file_info_get_origin_url(FILEINFO* p_file_info, char** origin_url)
{
    if(sd_strlen(p_file_info->_origin_url) != 0)
    {
        LOG_DEBUG("file_info_get_origin_url, get origin url :%s .", p_file_info->_origin_url);
        *origin_url = p_file_info->_origin_url;
        return TRUE;
    }
    else
    {
        LOG_ERROR("file_info_get_origin_url, get file origin url fail.");
        *origin_url = NULL;
        return FALSE;
    }
}

BOOL file_info_get_origin_ref_url( FILEINFO* p_file_info,char** origin_ref_url)
{
    if(sd_strlen(p_file_info->_origin_ref_url) != 0)
    {
        LOG_DEBUG("file_info_get_origin_ref_url, get origin  ref url :%s .", p_file_info->_origin_ref_url);
        *origin_ref_url = p_file_info->_origin_ref_url;
        return TRUE;
    }
    else
    {
        LOG_ERROR("file_info_get_origin_ref_url, get rigin ref url fail.");
        *origin_ref_url = NULL;
        return FALSE;
    }
}

BOOL file_info_get_file_path( FILEINFO* p_file_info,char** file_path)
{
    if(sd_strlen(p_file_info->_path) != 0)
    {
        LOG_DEBUG("file_info_get_file_path, get file path:%s .", p_file_info->_path);
        *file_path = p_file_info->_path;
        return TRUE;
    }
    else
    {
        LOG_ERROR("file_info_get_file_path, get file path fail.");
        *file_path = NULL;
        return FALSE;
    }
}

void  file_info_decide_finish_filename(FILEINFO* p_file_info)
{
    _u32 name_len =  0;

    LOG_DEBUG("file_info_decide_finish_filename");

    if(p_file_info->_has_postfix == TRUE)
    {
        name_len =  sd_strlen(p_file_info->_file_name);
        p_file_info->_file_name[name_len -3] = '\0';
        p_file_info->_has_postfix = FALSE;
    }

    LOG_DEBUG("file_info_decide_finish_filename, finish file name :%s", p_file_info->_file_name);

    return;

}


BOOL   file_info_get_file_name(FILEINFO* p_file_info, char** file_name)
{
    if(sd_strlen(p_file_info->_file_name) != 0)
    {
        LOG_DEBUG("file_info_get_file_name, get file name:%s .", p_file_info->_file_name);
        *file_name = p_file_info->_file_name;
        return TRUE;
    }
    else
    {
        LOG_ERROR("file_info_get_file_name, get file name fail.");
        *file_name = NULL;
        return FALSE;
    }
}

BOOL   file_info_get_user_file_name(FILEINFO* p_file_info, char** file_name)
{
    if(sd_strlen(p_file_info->_user_file_name) != 0)
    {
        LOG_DEBUG("file_info_get_user_file_name, get file name:%s .", p_file_info->_user_file_name);
        *file_name = p_file_info->_user_file_name;
        return TRUE;
    }
    else
    {
        LOG_ERROR("file_info_get_user_file_name, get file name fail.");
        *file_name = NULL;
        return FALSE;
    }
}

_u64  file_info_get_filesize(FILEINFO* p_file_info)
{
    return  p_file_info->_file_size;
}

_u32  file_info_get_blocksize(FILEINFO* p_file_info)
{
    return p_file_info->_block_size;
}

BOOL  file_info_filesize_is_valid(FILEINFO* p_file_info)
{
    if (p_file_info->_file_size == 0)
    {
        LOG_DEBUG("file_info_filesize_is_valid, filesize is invalid.");
        return FALSE;
    }
    else
    {

        LOG_DEBUG("file_info_filesize_is_valid, filesize:%llu is valid.", p_file_info->_file_size);
        return TRUE;
    }
}

_int32  file_info_set_cid(FILEINFO* p_file_info, _u8 cid[CID_SIZE])
{
    LOG_DEBUG("file_info_set_cid");

    sd_memcpy(p_file_info->_cid, cid, CID_SIZE);
    p_file_info->_cid_is_valid = TRUE;

    return SUCCESS;
}

_int32  file_info_set_gcid(FILEINFO* p_file_info, _u8 gcid[CID_SIZE])
{
    LOG_DEBUG("file_info_set_gcid");

    sd_memcpy(p_file_info->_gcid, gcid, CID_SIZE);
    p_file_info->_gcid_is_valid = TRUE;

    return SUCCESS;
}

_int32 file_info_set_user_data(FILEINFO* fi, const _u8 *user_data, _int32 len)
{
    LOG_DEBUG("file_info_set_user_data, len: %d, %s", len, user_data);
    if (fi->_user_data_len)
    {
        sd_free(fi->_user_data);
        fi->_user_data = NULL;
    }
    if (len)
    {
        sd_malloc(len, (void**)&(fi->_user_data));
        sd_memcpy(fi->_user_data, user_data, len);
    }
    fi->_user_data_len = len;
    return SUCCESS;
}

_int32  file_info_invalid_cid(FILEINFO* p_file_info)
{
    LOG_DEBUG("file_info_invalid_cid");

    p_file_info->_cid_is_valid = FALSE;
    sd_memset(p_file_info->_cid, 0, CID_SIZE);
    return SUCCESS;
}

_int32  file_info_invalid_gcid(FILEINFO* p_file_info)
{
    LOG_DEBUG("file_info_invalid_gcid");

    p_file_info->_gcid_is_valid = FALSE;
    sd_memset(p_file_info->_gcid, 0, CID_SIZE);
    return SUCCESS;
}

BOOL  file_info_get_shub_cid(FILEINFO* p_file_info, _u8 cid[CID_SIZE])
{
    LOG_DEBUG("file_info_get_shub_cid");

    if(p_file_info->_cid_is_valid == FALSE)
        return FALSE;
    sd_memcpy(cid, p_file_info->_cid,  CID_SIZE);

    return TRUE;
}
BOOL  file_info_get_shub_gcid(FILEINFO* p_file_info, _u8 gcid[CID_SIZE])
{
	LOG_DEBUG("file_info_get_shub_gcid, valid=%d", p_file_info->_gcid_is_valid);

    if(p_file_info->_gcid_is_valid == FALSE)
        return FALSE;
    sd_memcpy(gcid, p_file_info->_gcid,  CID_SIZE);

    return TRUE;
}

BOOL file_info_cid_is_valid(FILEINFO* p_file_info)
{
    return p_file_info->_cid_is_valid;
}

BOOL file_info_gcid_is_valid(FILEINFO* p_file_info)
{
    if(p_file_info->_is_need_calc_bcid == FALSE)
    {
        LOG_DEBUG("file_info_gcid_is_valid, but is_need_calc_bcid = FALSE");
        return TRUE;
    }

    return p_file_info->_gcid_is_valid;
}


BOOL file_info_check_cid(FILEINFO* p_file_info, _int32* err_code, BOOL* read_file_error, char* file_calc_cid)
{
    _u8 c_cid[CID_SIZE];
    BOOL ret_val  = TRUE;

    if(NULL != read_file_error)
    {
    	  *read_file_error = FALSE;
    }

#ifdef _LOGGER
    char  cid_str[CID_SIZE*2+1];
#endif

    LOG_DEBUG("file_info_check_cid .");

    if(p_file_info->_cid_is_valid == FALSE)
    {
        LOG_ERROR("file_info_check_cid, but cid in invalid, so failure .");
        return FALSE;
    }

#ifdef _LOGGER
    str2hex((char*)(p_file_info->_cid), CID_SIZE, cid_str, CID_SIZE*2);
    cid_str[CID_SIZE*2] = '\0';

    LOG_DEBUG("file_info_check_cid,get file cid:%s.", cid_str);
#endif

    ret_val  = get_file_3_part_cid(p_file_info, c_cid, err_code);
    if(ret_val == FALSE)
    {
        LOG_ERROR("file_info_check_cid, get  cid in invalid, so failure .");

	if(read_file_error != NULL)
	{
		*read_file_error = TRUE;
	}	
        
        return FALSE;
    }

    if(NULL != file_calc_cid)
    {
          sd_memcpy(file_calc_cid, c_cid, CID_SIZE);
    }
    

#ifdef _LOGGER
    str2hex((char*)c_cid, CID_SIZE, cid_str, CID_SIZE*2);
    cid_str[CID_SIZE*2] = '\0';

    LOG_DEBUG("file_info_check_cid, calc file cid:%s.", cid_str);
#endif
    return sd_is_cid_equal(p_file_info->_cid, c_cid);
}

BOOL file_info_check_gcid(FILEINFO* p_file_info)
{
    //_u8 s_gcid[CID_SIZE];
    _u8 c_gcid[CID_SIZE];
	_u8 c_gcid2[CID_SIZE] = {0};
    BOOL ret_val = TRUE;
	BOOL is_get_bakup_gcid = FALSE;

#ifdef _LOGGER
    char  cid_str[CID_SIZE*2+1];
#endif

    LOG_DEBUG("file_info_check_gcid .");

    if(p_file_info->_is_need_calc_bcid == FALSE)
    {
        LOG_DEBUG("file_info_check_gcid, but is_need_calc_bcid = FALSE");
        return TRUE;
    }

    if(p_file_info->_gcid_is_valid == FALSE)
    {
        LOG_ERROR("file_info_check_gcid, gcid is invalid.");
        return FALSE;
    }

#ifdef _LOGGER
    str2hex((char*)p_file_info->_gcid, CID_SIZE, cid_str, CID_SIZE*2);
    cid_str[CID_SIZE*2] = '\0';

    LOG_DEBUG("file_info_check_gcid, get shub gcid:%s.", cid_str);
#endif

    ret_val = get_file_gcid(p_file_info,  c_gcid);
    if(ret_val  == FALSE)
    {
        LOG_ERROR("file_info_check_gcid, calc gcid  failure.");
        return ret_val;
    }

	is_get_bakup_gcid = get_file_gcid_backup(p_file_info, c_gcid2);

	LOG_DEBUG("get_file_gcid_backup, ret_val = %d.", is_get_bakup_gcid);

#ifdef _LOGGER
    str2hex((char*)c_gcid, CID_SIZE, cid_str, CID_SIZE*2);
    cid_str[CID_SIZE*2] = '\0';

    LOG_DEBUG("file_info_check_gcid, calc gcid1:%s.", cid_str);

	str2hex((char*)c_gcid2, CID_SIZE, cid_str, CID_SIZE*2);
	cid_str[CID_SIZE*2] = '\0';

	LOG_DEBUG("file_info_check_gcid, calc gcid_for_backup:%s.", cid_str);
#endif

	if (is_get_bakup_gcid)
	{
		LOG_DEBUG("has backup gcid, so check such gcid using two saving gcid..");
		return sd_is_cid_equal(p_file_info->_gcid, c_gcid) || sd_is_cid_equal(p_file_info->_gcid, c_gcid2);
	}
	else
	{
		return sd_is_cid_equal(p_file_info->_gcid, c_gcid);
	}
}


/*_int32  file_info_add_range(FILEINFO* p_file_info, RANGE* r)
{
       _int32 ret_val = SUCCESS;

     LOG_DEBUG("file_info_add_range add range (%u,%u) .", r->_index, r->_num);

    ret_val = range_list_add_range(&p_file_info->_done_ranges, r, NULL,NULL);
    return ret_val;
}
_int32  file_info_add_rangelist(FILEINFO* p_file_info, RANGE_LIST* r_list)
{
       _int32 ret_val = SUCCESS;

       LOG_DEBUG("file_info_add_rangelist");

    ret_val = range_list_add_range_list(&p_file_info->_done_ranges, r_list);
    return ret_val;
}*/
_int32  file_info_delete_range(FILEINFO* p_file_info, RANGE* r)
{
    _int32 ret_val = SUCCESS;
    _int32 start_block = 0;
    _int32 end_block = 0;

    LOG_DEBUG("file_info_delete_range delete range (%u,%u) .", r->_index, r->_num);

    ret_val = range_list_delete_range(&p_file_info->_done_ranges, r, NULL,NULL);

    ret_val = range_list_delete_range(&p_file_info->_writed_range_list, r, NULL,NULL);

    data_receiver_del_buffer(&p_file_info->_data_receiver, r);

    data_receiver_erase_range(&p_file_info->_data_receiver, r);

    start_block = r->_index/p_file_info->_unit_num_per_block ;
    end_block = (r->_index + r->_num -1 )/p_file_info->_unit_num_per_block ;

    for(; start_block<=end_block ; start_block++)
    {
        clear_blockmap(&p_file_info->_bcid_info, start_block);
    }

    //Â∑≤ÁªèÊäïÈÄíÂà∞calc_hash_helperÁöÑÂú® [start_block,end_block]Âå∫Èó¥ÁöÑÂùóÈÉΩÈúÄË¶ÅcancelÊéâ„ÄÇ

    if(p_file_info->_block_caculator._cal_blockno >= start_block
       && p_file_info->_block_caculator._cal_blockno <= end_block)
    {
        LOG_DEBUG("file_info_delete_range delete range (%u,%u), because %u is calc bcid so stop calc .", r->_index, r->_num, p_file_info->_block_caculator._cal_blockno);
        p_file_info->_block_caculator._cal_blockno  =  MAX_U32;
    }

    clear_check_blocks(p_file_info);

    chh_cancel_all_items(p_file_info->_hash_calculator);

    return ret_val;
}

//÷ª÷ßÔøΩÔøΩÔøΩÔøΩÔøΩ¬ºÔøΩÔøΩÔøΩbcid“ªÔøΩÔøΩ
_int32 file_info_reset_bcid_info(FILEINFO* p_file_info)
{
    _u32 block_count = 0;
    _u32 bitmap_num = 0;
    LOG_DEBUG("file_info_reset_bcid_info.");

    if(p_file_info->_is_reset_bcid) return SUCCESS;
    p_file_info->_is_reset_bcid = TRUE;
    p_file_info->_block_caculator._cal_blockno = MAX_U32;
    clear_check_blocks(p_file_info);
    chh_cancel_all_items(p_file_info->_hash_calculator);
    block_count = (p_file_info->_file_size+ p_file_info->_block_size-1)/(p_file_info->_block_size );

    bitmap_num = (block_count+8-1)/(8);

    // Â∑≤ÁªèÊäïÈÄíÂà∞calc_hash_helperÁöÑÊâÄÊúâÂùóÈÉΩÈúÄË¶ÅcancelÊéâ„ÄÇ

    sd_assert( NULL != p_file_info->_bcid_info._bcid_bitmap );
    sd_assert( block_count == p_file_info->_bcid_info._bcid_num );
    sd_memset(p_file_info->_bcid_info._bcid_bitmap, 0, bitmap_num);
    start_check_blocks(p_file_info);
    return SUCCESS;
}

_int32  file_info_delete_buffer_by_range(FILEINFO* p_file_info, RANGE* r)
{
    int ret_val = SUCCESS;

    RANGE_LIST erase_range_list;

    range_list_init(&erase_range_list);

    LOG_DEBUG("file_info_delete_buffer_by_range, FILEINFO:0x%x , delete range(%u,%u) .", p_file_info, r->_index, r->_num);

    ret_val = buffer_list_delete(&(p_file_info->_data_receiver._buffer_list), r, &erase_range_list);

    CHECK_VALUE(ret_val);

    out_put_range_list(&erase_range_list);

    ret_val = range_list_delete_range_list(&(p_file_info->_data_receiver._cur_cache_range),&erase_range_list);

    range_list_clear(&erase_range_list);
    return ret_val;
}

BOOL  file_info_add_no_need_check_range(FILEINFO* p_file_info, RANGE* r)
{
    LOG_DEBUG("file_info_add_no_need_check_range , file_info :0x%x , add  range (%u,%u) .", p_file_info, r->_index, r->_num);

    if(p_file_info->_check_file_type == UNKOWN_FILE_TYPE)
    {
        if(file_is_media_file(p_file_info->_file_name) == TRUE)
        {
            p_file_info->_check_file_type = MEDIA_FILE_TYPE;

        }

        else
        {
            p_file_info->_check_file_type = OTHER_FILE_TYPE;

        }
    }

    if( p_file_info->_check_file_type != MEDIA_FILE_TYPE)
    {
        LOG_DEBUG("file_info_add_no_need_check_range , file_info :0x%x , add  range (%u,%u)  is not media file  so return false.", p_file_info, r->_index, r->_num);
        return FALSE;
    }

    range_list_add_range(&p_file_info->_no_need_check_range, r, NULL, NULL);

    out_put_range_list(&p_file_info->_no_need_check_range);

    if(range_list_get_total_num(&p_file_info->_no_need_check_range) > p_file_info->_unit_num_per_block*MAX_MEDIA_CHECK_BLOCK_NUM)
    {
        LOG_DEBUG("file_info_add_no_need_check_range , file_info :0x%x , add  range (%u,%u)  is  media file ,  but to many error block so return false.", p_file_info, r->_index, r->_num);

        return FALSE;
    }
    else
    {
        LOG_DEBUG("file_info_add_no_need_check_range , file_info :0x%x , add  range (%u,%u)  is  media file ,  return true.", p_file_info, r->_index, r->_num);
        return TRUE;
    }

}

/*_int32  file_info_delete_rangelist(FILEINFO* p_file_info, RANGE_LIST* r_list)
{

_int32 ret_val = SUCCESS;

    LOG_DEBUG("file_info_delete_rangelist");

    ret_val = range_list_delete_range_list(&p_file_info->_done_ranges, r_list);
    return ret_val;
}*/


_int32  file_info_open_cfg_file(FILEINFO* p_file_info,char * cfg_file_path)
{
    _int32 ret_val = SUCCESS;
    _u32 len = 0;
    _u32 cur_len = 0;
    char* cfg_postix = ".cfg";
    _int32 flag =O_FS_CREATE;


    LOG_DEBUG("file_info_open_cfg_file");

    if(p_file_info->_cfg_file != 0)
    {
        LOG_DEBUG("file_info_open_cfg_file has open!");

        return SUCCESS;
    }

    len = sd_strlen(p_file_info->_path);
    if(cur_len+len > MAX_FILE_NAME_LEN + MAX_FILE_PATH_LEN)
    {
        return FILE_PATH_TOO_LONG;
    }
    sd_strncpy(cfg_file_path+cur_len, p_file_info->_path, len);

    cur_len += len;

    len = sd_strlen(p_file_info->_file_name);
    if(cur_len+len > MAX_FILE_NAME_LEN + MAX_FILE_PATH_LEN)
    {
        return FILE_PATH_TOO_LONG;
    }
    sd_strncpy(cfg_file_path+cur_len, p_file_info->_file_name, len);

    cur_len += len;

    len = sd_strlen(cfg_postix);
    if(cur_len+len > MAX_FILE_NAME_LEN + MAX_FILE_PATH_LEN)
    {
        return FILE_PATH_TOO_LONG;
    }
    sd_strncpy(cfg_file_path+cur_len, cfg_postix, len);

    cur_len += len;

    cfg_file_path[cur_len]='\0';

    LOG_DEBUG("file_info_open_cfg_file, open cfg file:%s", cfg_file_path);

    ret_val =  sd_open_ex(cfg_file_path, flag, &p_file_info->_cfg_file);

    return ret_val;
}

_int32  file_info_close_cfg_file(FILEINFO* p_file_info)
{
    _int32 ret_val = SUCCESS;

    LOG_DEBUG("file_info_close_cfg_file");

    if(p_file_info->_cfg_file != 0)
    {
        LOG_DEBUG("file_info_close_cfg_file, close cfg file:%u", p_file_info->_cfg_file);

        sd_close_ex(p_file_info->_cfg_file);
        p_file_info->_cfg_file = 0;
    }

    return ret_val;
}

_int32  file_info_delete_cfg_file(FILEINFO* p_file_info)
{
    _int32 ret_val = SUCCESS;
    _u32 len = 0;
    _u32 cur_len = 0;
    char* cfg_postix = ".cfg";

    char cfg_file_path[MAX_FILE_NAME_LEN + MAX_FILE_PATH_LEN];

    LOG_ERROR("file_info_delete_cfg_file");

    file_info_close_cfg_file(p_file_info);

    len = sd_strlen(p_file_info->_path);
    if(cur_len+len > MAX_FILE_NAME_LEN + MAX_FILE_PATH_LEN)
    {
        return FILE_PATH_TOO_LONG;
    }
    sd_strncpy(cfg_file_path+cur_len, p_file_info->_path, len);

    cur_len += len;

    len = sd_strlen(p_file_info->_file_name);
    if(cur_len+len > MAX_FILE_NAME_LEN + MAX_FILE_PATH_LEN)
    {
        return FILE_PATH_TOO_LONG;
    }
    sd_strncpy(cfg_file_path+cur_len, p_file_info->_file_name, len);

    cur_len += len;

    len = sd_strlen(cfg_postix);
    if(cur_len+len > MAX_FILE_NAME_LEN + MAX_FILE_PATH_LEN)
    {
        return FILE_PATH_TOO_LONG;
    }
    sd_strncpy(cfg_file_path+cur_len, cfg_postix, len);

    cur_len += len;

    cfg_file_path[cur_len]='\0';

    LOG_ERROR("file_info_delete_cfg_file,  cfg file:%s", cfg_file_path);

    ret_val =  sd_delete_file(cfg_file_path);

    return ret_val;
}

_int32  file_info_delete_tmp_file(FILEINFO* p_file_info)
{
    _int32 ret_val = SUCCESS;
    _u32 len = 0;
    _u32 cur_len = 0;
    char tmp_file_path[MAX_FILE_NAME_LEN + MAX_FILE_PATH_LEN];

    LOG_DEBUG("file_info_delete_tmp_file");

    len = sd_strlen(p_file_info->_path);
    if(cur_len+len > MAX_FILE_NAME_LEN + MAX_FILE_PATH_LEN)
    {
        return FILE_PATH_TOO_LONG;
    }
    sd_strncpy(tmp_file_path+cur_len, p_file_info->_path, len);

    cur_len += len;

    len = sd_strlen(p_file_info->_file_name);
    if(cur_len+len > MAX_FILE_NAME_LEN + MAX_FILE_PATH_LEN)
    {
        return FILE_PATH_TOO_LONG;
    }
    sd_strncpy(tmp_file_path+cur_len, p_file_info->_file_name, len);

    cur_len += len;

    tmp_file_path[cur_len]='\0';

    LOG_DEBUG("file_info_delete_tmp_file,  tmp file:%s", tmp_file_path);

    ret_val =  sd_delete_file(tmp_file_path);

    return ret_val;

}


void  file_info_clear_all_recv_data(FILEINFO* p_file_info)
{
    range_list_clear(GET_TOTAL_RECV_RANGE_LIST(p_file_info->_data_receiver));

    /* range_list_cp_range_list(GET_TOTAL_RECV_RANGE_LIST(p_file_info->_data_receiver), &p_file_info->_writed_range_list);

    range_list_clear(&p_file_info->_done_ranges);

    data_receiver_syn_data_buffer(&p_file_info->_data_receiver);

    clear_check_blocks(p_file_info);   */


    range_list_clear(&p_file_info->_writed_range_list);


    file_info_asyn_all_recv_data(p_file_info);

    return;

}

void  file_info_asyn_all_recv_data(FILEINFO* p_file_info)
{

    //range_list_cp_range_list(GET_TOTAL_RECV_RANGE_LIST(p_file_info->_data_receiver)
    //    , &p_file_info->_writed_range_list);

    range_list_clear(&p_file_info->_done_ranges);

    data_receiver_syn_data_buffer(&p_file_info->_data_receiver);

    clear_check_blocks(p_file_info);

    chh_cancel_all_items(p_file_info->_hash_calculator);
   //Â∑≤ÁªèÊäïÈÄíÂà∞calc_hash_helperÁöÑÊâÄÊúâÂùóÈÉΩÈúÄË¶ÅcancelÊéâ„ÄÇ

    return;

}


BOOL  file_info_has_recved_data(FILEINFO* p_file_info)
{
    LOG_ERROR("file_info_has_recved_data. " );

    if(p_file_info->_data_receiver._total_receive_range._node_size >0)
    {
        LOG_ERROR("file_info_has_recved_data, recv num %u. ", range_list_get_total_num(&p_file_info->_data_receiver._total_receive_range) );
        return TRUE;
    }
    else
    {
        LOG_ERROR("file_info_has_recved_data, return false. " );
        return FALSE;
    }
}


_u32  file_info_get_file_percent(FILEINFO* p_file_info)
{
    _u32 percent = 0;
    _u32 precessed_data_unit_num = 0;
    _u64 filesize = 0;

    if(p_file_info->_file_size == 0)
    {
        LOG_DEBUG("file_info_get_file_percent. because filesize is invalid, so percent of file is 0. " );
        percent = 0;
        return percent;
    }

    if(file_info_ia_all_received(p_file_info) == TRUE)
    {
        LOG_DEBUG("file_info_get_file_percent. all data has recved, so percent of file is 100. " );
        percent = 100;
        return percent;
    }

    precessed_data_unit_num = range_list_get_total_num(&p_file_info->_writed_range_list);
    filesize = p_file_info->_file_size;

    LOG_DEBUG("file_info_get_file_percent. get recv data unit num:%u , filesize :%llu. ",  precessed_data_unit_num, filesize);

    if(filesize == 0)
    {
        LOG_DEBUG("file_info_get_file_percent. because filesize 0, so percent of file is 0. " );
        percent = 0;
    }
    else
    {
        percent =  (((_u64)precessed_data_unit_num*get_data_unit_size()*100)/filesize);
        if(percent > 100)
        {
            percent = 99;
        }
    }

    LOG_DEBUG("file_info_get_file_percent. unit_num:%u, filesize%llu, percent:%u. ",  precessed_data_unit_num, filesize, percent);

    return percent;

}

_u64  file_info_get_download_data_size(FILEINFO* p_file_info)
{

    _u64 download_size = 0;
    _u32 download_unit_num = 0;
    _u64 filesize = 0;

    download_unit_num = range_list_get_total_num(&p_file_info->_data_receiver._total_receive_range);

    if(p_file_info->_file_size != 0)
    {
        filesize = p_file_info->_file_size;

        download_size = (_u64)download_unit_num * (_u64) get_data_unit_size();

        LOG_DEBUG("file_info_get_download_data_size. down range num :%u , filesize: %llu , downsize :%llu .", download_unit_num,filesize, download_size);

        if(download_size > filesize)
        {
            download_size = filesize;
        }
    }
    else
    {
        download_size = (_u64)download_unit_num * (_u64) get_data_unit_size();
        LOG_DEBUG("file_info_get_download_data_size. filesize is invalid down range num :%u ,  downsize :%llu .", download_unit_num, download_size);

    }

    return download_size;

}

_u64  file_info_get_writed_data_size(FILEINFO* p_file_info)
{

    _u64 writed_size = 0;
    _u32 writed_unit_num = 0;
    _u64 filesize = 0;

    writed_unit_num = range_list_get_total_num(&p_file_info->_writed_range_list);

    if(p_file_info->_file_size != 0)
    {
        filesize = p_file_info->_file_size;

        writed_size = (_u64)writed_unit_num * (_u64) get_data_unit_size();

        LOG_DEBUG("file_info_get_writed_data_size. write range num :%u , filesize: %llu , writesize :%llu .", writed_unit_num,filesize, writed_size);

        if(writed_size > filesize)
        {
            writed_size = filesize;
        }
    }
    else
    {
        writed_size = (_u64)writed_unit_num * (_u64) get_data_unit_size();
        LOG_DEBUG("file_info_get_writed_data_size. filesize is invalid, write range num :%u ,  writesize :%llu .",writed_unit_num, writed_size);

    }

    return writed_size;

}

_int32  file_info_set_write_mode(FILEINFO* p_file_info,FILE_WRITE_MODE write_mode)
{

#ifdef _VOD_NO_DISK
    if(p_file_info->_is_no_disk == TRUE)
    {
        return SUCCESS;
    }
#endif
    sd_assert(write_mode==FWM_RANGE||write_mode==FWM_BLOCK);  /* ÔøΩÔøΩ ±÷ª÷ßÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩ */
    if(write_mode!=FWM_RANGE&&write_mode!=FWM_BLOCK)
    {
        return -1;
    }

    if(p_file_info->_tmp_file_struct ==  NULL)
    {
        /* ÔøΩƒºÔøΩÔøΩÔøΩŒ¥ÔøΩÔøΩÔøΩÔøΩ,ÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩ“ªÔøΩÔøΩ–¥ÔøΩÔøΩ Ω */
        p_file_info->_write_mode = write_mode;
    }
    else
    {
        if((p_file_info->_write_mode == FWM_BLOCK)&&(write_mode == FWM_BLOCK_NEED_ORDER))
        {
            /* ÔøΩƒºÔøΩÔøΩÔøΩÔøΩ—¥ÔøΩÔøΩÔøΩ,÷ªÔøΩ‹¥ÔøΩ FWM_BLOCKÔøΩÔøΩŒ™FWM_BLOCK_NEED_ORDER */
            sd_assert(p_file_info->_write_mode == p_file_info->_tmp_file_struct->_write_mode);
            p_file_info->_write_mode = write_mode;
            p_file_info->_tmp_file_struct->_write_mode = write_mode;
        }
    }
    return SUCCESS;
}
_int32  file_info_get_write_mode(FILEINFO* p_file_info,FILE_WRITE_MODE * write_mode)
{

    *write_mode = p_file_info->_write_mode;
    return SUCCESS;
}

_u32 compute_block_size(_u64 file_size)
{
    _u32 block_size = BCID_DEF_BLOCK_SIZE;
    if( 0 != file_size )
    {
        while(file_size > (((_u64)block_size) * BCID_DEF_MIN_BLOCK_COUNT))
        {
            block_size *= 2;
            if( BCID_DEF_MAX_BLOCK_SIZE <= block_size ) break;
        }
    }

    LOG_DEBUG("compute_block_size, get block_size:%u from filesize %llu .", block_size, file_size);

    return block_size;
}

_int32 compute_3part_range_list(_u64 file_size, RANGE_LIST*t_list)
{
    RANGE f;

    range_list_clear(t_list);

    LOG_DEBUG("compute_3part_range_list .");

    if( TCID_SAMPLE_SIZE >= file_size )
    {
        f =  pos_length_to_range(0, file_size,  file_size);
        range_list_add_range(t_list, &f, NULL,NULL);

        LOG_DEBUG("compute_3part_range_list, filesize:%llu, get cid range(%u,%u) .", file_size, f._index, f._num);
    }
    else
    {
        f =  pos_length_to_range(0, TCID_SAMPLE_UNITSIZE,  file_size);
        range_list_add_range(t_list, &f, NULL,NULL);

        LOG_DEBUG("compute_3part_range_list, filesize:%llu, get cid0 range(%u,%u) .", file_size, f._index, f._num);

        f =  pos_length_to_range(file_size / 3, TCID_SAMPLE_UNITSIZE,  file_size);
        range_list_add_range(t_list, &f, NULL,NULL);

        LOG_DEBUG("compute_3part_range_list, filesize:%llu, get cid1 range(%u,%u) .", file_size, f._index, f._num);

        f =  pos_length_to_range(file_size - TCID_SAMPLE_UNITSIZE, TCID_SAMPLE_UNITSIZE,  file_size);
        range_list_add_range(t_list, &f, NULL,NULL);

        LOG_DEBUG("compute_3part_range_list, filesize:%llu, get cid2 range(%u,%u) .", file_size, f._index, f._num);
    }

    return SUCCESS;
}


_u32 get_bitmap_index(_u32 blockno)
{
    return blockno/(8);
}

_u32 get_bitmap_off(_u32 blockno)
{
    return blockno%(8);
}

_u32 get_read_length_for_bcid_check(void)
{
    return 2*1024*1024;
}

_u32 get_read_length(void)
{
    return MAX_READ_LENGTH;;
}

_u32 get_data_unit_index(_u32 block_no, _u32 unit_num_per_block)
{
    return block_no*unit_num_per_block;
}

RANGE to_range(_u32 block_no, _u32 block_size, _u64 file_size)
{
    _u64 pos = block_size;
    _u64 len = block_size;
    RANGE tmp;
    _u32 unit_num  = 0;

    pos *= block_no;
    if( 0 != file_size && file_size < pos + len )
    {
        len = file_size - pos;
    }

    unit_num = compute_unit_num(block_size);
    tmp._index = unit_num* block_no;
    tmp._num = compute_unit_num(len);

    LOG_DEBUG("to_range, block_no:%u, block_size%u, filesize:%llu get range (%u,%u).", block_no, block_size, file_size, tmp._index, tmp._num);
    return tmp;
}

/*return the range of the whole blocks which include r, */
RANGE range_to_block_range(RANGE* r, _u32 block_size, _u64 file_size)
{
    _u32 first_block = 0;
    _u32 last_block =0;
    _u64 pos = 0;
    _u64 length = 0;
    RANGE null_r ;
    null_r._index = 0;
    null_r._num = 0;

    if(r->_index + r->_num <=0)
    {
        return null_r;
    }

    first_block =  r->_index/((block_size)/get_data_unit_size());
    last_block =  (r->_index+r->_num -1)/((block_size)/get_data_unit_size());

    pos  = (_u64)first_block*(_u64)block_size;
    length= (last_block - first_block +1)*(_u64)block_size;
    return pos_length_to_range(pos, length, file_size);
}


BOOL range_list_length_is_enough(RANGE_LIST* range_list, _u64 filesize)
{
    _u64 total_num = range_list_get_total_num(range_list);

    LOG_DEBUG("range_list_length_is_enough, range list num:%llu is enough, filesize:%llu .", total_num, filesize);

    if(total_num*get_data_unit_size() >= filesize )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


_u32 to_blockno(_u64 pos, _u32 block_size)
{
    return (_u32)(pos / block_size);
}

void clear_list(LIST* l)
{
    void* data = NULL;

    LOG_DEBUG("clear_list .");

    if(list_size(l) == 0)
    {
        return ;
    }

    do
    {
        list_pop(l, &data);
        if(data == NULL)
        {
            break;
        }

    }
    while(1);

    return ;
}


_u32 prepare_for_bcid_info(bcid_info* p_bcid_info, _u32 bcid_num)
{
    _int32 ret_val = SUCCESS;
    _u32 bitmap_num = 0;

    LOG_DEBUG("prepare_for_bcid_info :0x%x , 1 bcid_num:%u , rel num:%u, bcid_buffer:0x%x, bitmap:0x%x.",
              p_bcid_info,bcid_num, p_bcid_info->_bcid_num, p_bcid_info->_bcid_buffer, p_bcid_info->_bcid_bitmap);

    if(bcid_num == 0)
    {
        return ret_val;
    }

    if(p_bcid_info->_bcid_num != bcid_num)
    {
        if(p_bcid_info->_bcid_buffer != NULL)
        {
            sd_free(p_bcid_info->_bcid_buffer);
            p_bcid_info->_bcid_buffer = NULL;
        }

        if(p_bcid_info->_bcid_bitmap!= NULL)
        {
            sd_free(p_bcid_info->_bcid_bitmap);
            p_bcid_info->_bcid_bitmap = NULL;
        }

        if(p_bcid_info->_bcid_checking_bitmap != NULL)
        {
            sd_free(p_bcid_info->_bcid_checking_bitmap);
            p_bcid_info->_bcid_checking_bitmap = NULL;
        }

        ret_val = sd_malloc(bcid_num*CID_SIZE*sizeof(_u8), (void**)&(p_bcid_info->_bcid_buffer));
        CHECK_VALUE(ret_val);

        bitmap_num = (bcid_num+8-1)/(8);
        ret_val = sd_malloc(bitmap_num, (void**)&(p_bcid_info->_bcid_bitmap));
        CHECK_VALUE(ret_val);
        sd_memset(p_bcid_info->_bcid_bitmap, 0, bitmap_num);

        ret_val = sd_malloc(bitmap_num, (void**)&(p_bcid_info->_bcid_checking_bitmap));
        sd_memset(p_bcid_info->_bcid_checking_bitmap, 0, bitmap_num);

        p_bcid_info->_bcid_num = bcid_num;

        LOG_DEBUG("prepare_for_bcid_info :0x%x , 2 bcid_num:%u , rel num:%u, bcid_buffer:0x%x, bitmap:0x%x.",
                  p_bcid_info,bcid_num, p_bcid_info->_bcid_num, p_bcid_info->_bcid_buffer, p_bcid_info->_bcid_bitmap);

        return ret_val;
    }
    else
    {
        bitmap_num = (bcid_num+8-1)/(8);
        if(p_bcid_info->_bcid_buffer == NULL)
        {
            ret_val = sd_malloc(bcid_num*CID_SIZE*sizeof(_u8), (void**)&(p_bcid_info->_bcid_buffer));
            CHECK_VALUE(ret_val);
        }
        if(p_bcid_info->_bcid_bitmap == NULL)
        {
            ret_val = sd_malloc(bitmap_num, (void**)&(p_bcid_info->_bcid_bitmap));
            CHECK_VALUE(ret_val);
            if(ret_val != SUCCESS)
            {
                sd_free(p_bcid_info->_bcid_buffer);
                p_bcid_info->_bcid_buffer = NULL;
                return ret_val;
            }
            sd_memset(p_bcid_info->_bcid_bitmap, 0, bitmap_num);
        }

        if(p_bcid_info->_bcid_checking_bitmap == NULL)
        {
            ret_val = sd_malloc(bitmap_num, (void**)&(p_bcid_info->_bcid_checking_bitmap));
            CHECK_VALUE(ret_val);
            if(ret_val != SUCCESS)
            {
                sd_free(p_bcid_info->_bcid_buffer);
                p_bcid_info->_bcid_buffer = NULL;
                return ret_val;
            }
            sd_memset(p_bcid_info->_bcid_checking_bitmap, 0, bitmap_num);
        }


        LOG_DEBUG("prepare_for_bcid_info :0x%x , 3 bcid_num:%u , rel num:%u, bcid_buffer:0x%x, bitmap:0x%x.",
                  p_bcid_info,bcid_num, p_bcid_info->_bcid_num, p_bcid_info->_bcid_buffer, p_bcid_info->_bcid_bitmap);
        return ret_val;
    }

    LOG_DEBUG("prepare_for_bcid_info :0x%x , 4 bcid_num:%u , rel num:%u, bcid_buffer:0x%x, bitmap:0x%x.",
              p_bcid_info,bcid_num, p_bcid_info->_bcid_num, p_bcid_info->_bcid_buffer, p_bcid_info->_bcid_bitmap);

    return ret_val;
}



void file_info_close_tmp_file(FILEINFO* p_file_info)
{
	MSG_INFO msg = {};
    _u32 msg_id = 0;
    _int32 ret;

    if(p_file_info->_tmp_file_struct !=  NULL)
    {
        if( fm_file_is_created( p_file_info->_tmp_file_struct) == TRUE) 
		{
			if(!(dm_get_enable_fast_stop()&& p_file_info->_file_size >= MAX_FILE_SIZE_NEED_FLUSH_DATA_B4_CLOSE))
	        {
	            file_info_flush_data_to_file(p_file_info, TRUE);
            
			}
        }
        file_info_save_to_cfg_file(p_file_info);

        p_file_info->_wait_for_close = TRUE;

        LOG_DEBUG("file_info_close_tmp_file, fileinfo :0x%x , tmp_file :0x%x is closing."
            , p_file_info, p_file_info->_tmp_file_struct);

        if(p_file_info->_block_caculator._cal_blockno == MAX_U32)
        {
            if(p_file_info->_block_caculator._p_buffer_data != NULL)
            {
                if(p_file_info->_block_caculator._p_buffer_data->_data_ptr != NULL)
                {
                    sd_free(p_file_info->_block_caculator._p_buffer_data->_data_ptr);
                    p_file_info->_block_caculator._p_buffer_data->_data_ptr = NULL;
                }

                //  sd_free(p_data_checker->_p_buffer_data);
                free_range_data_buffer_node(p_file_info->_block_caculator._p_buffer_data);
            }
        }

        clear_check_blocks(p_file_info);
        // Â∑≤ÁªèÊäïÈÄíÂà∞calc_hash_helperÁöÑÊâÄÊúâÂùóÈÉΩÈúÄË¶ÅcancelÊéâ„ÄÇ
        chh_cancel_all_items(p_file_info->_hash_calculator);

        p_file_info->_block_caculator._p_buffer_data = NULL;

        fm_close(p_file_info->_tmp_file_struct , (notify_file_close) file_info_notify_file_close
                , (void*) p_file_info );


        LOG_DEBUG("file_info_close_tmp_file :0x%x, close tmp file :0x%x .", p_file_info, p_file_info->_tmp_file_struct);

        p_file_info->_tmp_file_struct = NULL;
    }
    else
    {
        // sd_assert(FALSE);

        msg._device_id = (_u32)p_file_info;
        msg._user_data = 0;
        msg._device_type = DEVICE_NONE;
        msg._operation_type = OP_NONE;

        ret = post_message(&msg, file_info_notify_fm_can_close, NOTICE_ONCE, 0, &msg_id);

        LOG_DEBUG("file_info_close_tmp_file :0x%x, because has not creat tmp file so post close msg , msgid :%u .", p_file_info, msg_id);

    }
}


BOOL file_info_file_is_create(FILEINFO* p_file_info)
{

#ifdef _VOD_NO_DISK

    /*  if(sd_strlen(p_file_info->_file_name) != 0)
          return TRUE;
    else
      return FALSE;*/
    if(p_file_info->_is_no_disk == TRUE)
    {

        if(p_file_info->_file_name[0] != '\0')
            return TRUE;
        else
            return FALSE;
    }

#endif

    if(p_file_info->_tmp_file_struct !=  NULL)
        return TRUE;
    else
        return FALSE;
}

_int32 file_info_try_create_file(FILEINFO* p_file_info)
{

    char* filepath = NULL;
    char* filename = NULL;
    // BOOL bool_val = TRUE;
    _int32 ret_val = SUCCESS;

    LOG_DEBUG("file_info_try_create_file .");

#ifdef _VOD_NO_DISK
    if(p_file_info->_is_no_disk == TRUE)
        return SUCCESS;
#endif


    if(p_file_info->_tmp_file_struct !=  NULL)
    {
        LOG_ERROR("data_checker_try_create_file, but file has created .");
        return SUCCESS;
    }

    /* bool_val = dm_get_file_path( p_file_info->_p_data_manager,&filepath);
        if(bool_val == FALSE)
        {
              LOG_ERROR("data_checker_try_create_file, but can not get file path .");
              return NO_FILE_PATH;
        }

     bool_val = dm_get_file_name( p_file_info->_p_data_manager,&filename);
        if(bool_val == FALSE)
        {
              LOG_ERROR("data_checker_try_create_file, but can not get file name .");
              return NO_FILE_NAME;
        }*/

    if(sd_strlen(p_file_info->_path) == 0)
    {
        LOG_ERROR("file_info_try_create_file, but can no file path .");
        return NO_FILE_PATH;
    }

    filepath = p_file_info->_path;

    BOOL pathExist = sd_dir_exist(filepath);
    if (!pathExist)
    {
        ret_val = sd_mkdir(filepath);
        if (SUCCESS!=ret_val)
        {
            LOG_DEBUG("file_info_try_create_file: create task download path fail");
            return NO_FILE_PATH;
        }
    }

    if(sd_strlen(p_file_info->_file_name) == 0)
    {
        LOG_ERROR("file_info_try_create_file, but can no  _file_name .");
        return NO_FILE_NAME;
    }

    filename = p_file_info->_file_name;

    ret_val = fm_create_file_struct( filename, filepath, p_file_info->_file_size,(void* )p_file_info, (notify_file_create) file_info_notify_file_create, &p_file_info->_tmp_file_struct,p_file_info->_write_mode);
    if(ret_val != SUCCESS)
    {
        LOG_ERROR("file_info_try_create_file,fm_create_file_struct fail .");

        return CREATE_FILE_FAIL;
    }

    if(p_file_info->_file_size != 0 && p_file_info->_block_size != 0)
    {
        LOG_DEBUG("file_info_try_create_file success, file info :0x%x, set filesize:%llu , set block_size:%u,  .", p_file_info, p_file_info->_file_size, p_file_info->_block_size);
        //fm_set_filesize( file_struct, ptr_data_checker->_file_size);
        fm_init_file_info( p_file_info->_tmp_file_struct, p_file_info->_file_size, p_file_info->_block_size);
    }
    else
    {
        LOG_ERROR("file_info_try_create_file, file info :0x%x, but filesize or blocksize is invalid,  filesize:%llu ,  block_size:%u,  .", p_file_info, p_file_info->_file_size, p_file_info->_block_size);
    }


    return SUCCESS;
}

_int32 file_info_set_filesize(FILEINFO* p_file_info, _u64 filesize)
{
    _int32 ret_val = SUCCESS;
    _int32 block_count = 0;

    LOG_DEBUG("file_info_set_filesize, datat check:0x%x, filesize:%llu .", p_file_info, filesize);

    p_file_info->_file_size = filesize;
    p_file_info->_block_size = compute_block_size(filesize);
    p_file_info->_unit_num_per_block = compute_unit_num(p_file_info->_block_size);
    //p_file_info->_bcid_is_valid =  FALSE;

    block_count = (filesize + p_file_info->_block_size-1)/(p_file_info->_block_size );

    ret_val = prepare_for_bcid_info(&(p_file_info->_bcid_info), block_count);
    if(ret_val == OUT_OF_MEMORY)
    {
        //data_manager_notify_failure(p_file_info->_p_data_manager, DATA_ALLOC_BCID_BUFFER_ERROR);

        //p_file_info->_call_back_func_table.notify_file_result(p_file_info->_p_user_ptr, p_file_info, DATA_ALLOC_BCID_BUFFER_ERROR);
        file_info_notify_file_result(p_file_info,DATA_ALLOC_BCID_BUFFER_ERROR);
    }
    CHECK_VALUE(ret_val);

    if(p_file_info->_tmp_file_struct != NULL)
    {
        LOG_DEBUG("file_info_set_filesize,set tmp file info,  :0x%x, set filesize:%llu , set block_size:%u,  .", p_file_info, p_file_info->_file_size, p_file_info->_block_size);
        fm_init_file_info( p_file_info->_tmp_file_struct, p_file_info->_file_size, p_file_info->_block_size);

        if(p_file_info->_writed_range_list._node_size != 0 && p_file_info->_bcid_enable == TRUE)
        {
            start_check_blocks(p_file_info);
        }
    }

    LOG_DEBUG("file_info_set_filesize,set tmp file info  :0x%x, set filesize:%llu , set block_size:%u,  .", p_file_info, p_file_info->_file_size, p_file_info->_block_size);

    return ret_val;

}

_u32 file_info_set_hub_return_info(FILEINFO* p_file_info
    , _u64 filesize
    , _u32 gcid_type
    , _u8 block_cid[]
    , _u32 block_count
    , _u32 block_size )
{
    _int32 ret_val = SUCCESS;

#ifdef _LOGGER
    char  bcid_str[CID_SIZE*2+1];
    _u32 i =0;
    for(i=0; i<block_count; i++)
    {
        str2hex((char*)(block_cid + i*CID_SIZE), CID_SIZE,bcid_str, CID_SIZE*2);
        bcid_str[CID_SIZE*2] = '\0';
        LOG_DEBUG("file_info_set_hub_return_info: bcidno %u:%s",i, bcid_str);
    }
#endif

    LOG_DEBUG("set_hub_return_info, filesize:%llu, gcid_type:%u, block_count:%u, block_size:%u..", filesize, gcid_type, block_count,block_size);

    p_file_info->_file_size = filesize;
    p_file_info->_block_size = block_size;


    p_file_info->_unit_num_per_block = compute_unit_num(block_size);

    ret_val = prepare_for_bcid_info(&(p_file_info->_bcid_info), block_count);

    CHECK_VALUE(ret_val);

    p_file_info->_bcid_info._check_level= gcid_type;
    p_file_info->_bcid_info._bcid_num = block_count;

    sd_memcpy(p_file_info->_bcid_info._bcid_buffer, block_cid, block_count*CID_SIZE);

    if(p_file_info->_bcid_is_valid == FALSE && p_file_info->_bcid_map_from_cfg == FALSE)
    {
        file_info_invalid_bcid(p_file_info);
    }
    p_file_info->_bcid_is_valid = TRUE;

    return SUCCESS;
}


_u32 file_info_set_hub_return_info2(FILEINFO* p_file_info
	, _u32 gcid_type
	, _u8 block_cid[]
	, _u32 block_count
	, _u32 block_size )
{
	_int32 ret_val = SUCCESS;

#ifdef _LOGGER
	char  bcid_str[CID_SIZE*2+1];
	_u32 i =0;
	for(i=0; i<block_count; i++)
	{
		str2hex((char*)(block_cid + i*CID_SIZE), CID_SIZE,bcid_str, CID_SIZE*2);
		bcid_str[CID_SIZE*2] = '\0';
		LOG_DEBUG("file_info_set_hub_return_info2: bcidno %u:%s",i, bcid_str);
	}
#endif

	LOG_DEBUG("set_hub_return_info2, gcid_type:%u, block_count:%u, block_size:%u..", gcid_type, block_count,block_size);
	ret_val = prepare_for_bcid_info(&(p_file_info->_bcid_info_backup), block_count);

	CHECK_VALUE(ret_val);

	p_file_info->_bcid_info_backup._check_level= gcid_type;
	p_file_info->_bcid_info_backup._bcid_num = block_count;

	sd_memcpy(p_file_info->_bcid_info_backup._bcid_buffer, block_cid, block_count*CID_SIZE);

	p_file_info->_is_backup_bcid_valid = TRUE;

	return SUCCESS;
}

_int32 file_info_filesize_change(FILEINFO* p_file_info, _u64 filesize)
{

    LOG_DEBUG("file_info_filesize_change .");

#ifdef _VOD_NO_DISK
    if(p_file_info->_is_no_disk == TRUE)
        return SUCCESS;
#endif


    if(p_file_info->_tmp_file_struct == NULL)
    {
        return SUCCESS;
    }
    // Â∑≤ÁªèÊäïÈÄíÂà∞calc_hash_helperÁöÑÊâÄÊúâÂùóÈÉΩÈúÄË¶ÅcancelÊéâ„ÄÇ

    //p_file_info->_file_size_changed = TRUE;

    /*fm_close(p_data_checker->_tmp_file_struct , (notify_file_close) data_check_notify_file_close, (void*) p_data_checker );

    p_data_checker->_tmp_file_struct = NULL;

       data_checker_try_create_file(p_data_checker);*/

    //LOG_DEBUG("fm_change_file_size .");

    fm_change_file_size(p_file_info->_tmp_file_struct, filesize, (void*)p_file_info, (notify_file_create)file_info_notify_filesize_change);

    return SUCCESS;

}

BOOL  file_info_bcid_valid(FILEINFO* p_file_info)
{
    LOG_DEBUG("file_info_bcid_valid  %u.", (_u32)p_file_info->_bcid_is_valid);
    return p_file_info->_bcid_is_valid;
}

_int32  file_info_invalid_bcid(FILEINFO* p_file_info)
{
    _u32 bitmap_num = 0;

    LOG_DEBUG("file_info_invalid_bcid .");

    //  if(p_data_checker->_bcid_is_valid == TRUE)
    //{
    p_file_info->_bcid_is_valid = FALSE;
    bitmap_num = (p_file_info->_bcid_info._bcid_num + 8-1)/(8);
    sd_memset(p_file_info->_bcid_info._bcid_bitmap, 0, bitmap_num);
    //}

    return SUCCESS;
}

_u32  file_info_get_bcid_num(FILEINFO* p_file_info)
{
    // if(p_file_info->_bcid_is_valid == FALSE)
    //      return 0;
    return p_file_info->_bcid_info._bcid_num;
}

_u8*  file_info_get_bcid_buffer(FILEINFO* p_file_info)
{
    //  if(p_file_info->_bcid_is_valid == FALSE)
    //      return NULL;
    return p_file_info->_bcid_info._bcid_buffer;
}


_u32 prepare_for_shahash(FILEINFO* p_file_info)
{
    _int32 ret_val = SUCCESS;

    LOG_DEBUG("prepare_for_shahash, file_info = 0x%x", p_file_info);
    //sha1_initialize(&p_file_info->_block_caculator._hash_ptr);
    SAFE_DELETE(p_file_info->_block_caculator._data_buffer);
    ret_val = sd_malloc(p_file_info->_block_caculator._expect_length
            , (void**)&p_file_info->_block_caculator._data_buffer);

    return ret_val;
}

_u32 check_block(FILEINFO* p_file_info ,_u32 blockno, RANGE_DATA_BUFFER_LIST* buffer,  BOOL drop_over_data)
{
    _u8 block_cid[CID_SIZE];

    _int32 ret_val = SUCCESS;

    LOG_DEBUG("check_block, p_file_info:0x%x ,check blockno:%u .", p_file_info, blockno);
    if(p_file_info->_is_calc_bcid == FALSE || p_file_info->_is_need_calc_bcid == FALSE)
    {
        LOG_DEBUG("check_block but is_calc_bcid = FALSE");
        return SUCCESS;
    }

    ret_val = calc_block_cid(p_file_info, blockno, buffer, block_cid, drop_over_data);
    //if(ret_val != SUCCESS)
    //    return ret_val;


  //  if(p_file_info->_bcid_is_valid == TRUE )
  //  {
  //      ret_val = verify_block_cid(&p_file_info->_bcid_info, blockno, block_cid);
		//LOG_DEBUG("ptr_file_info->_is_backup_bcid_valid = %d, ret_val = %d", p_file_info->_is_backup_bcid_valid, ret_val);
		//if(p_file_info->_is_backup_bcid_valid == TRUE && ret_val != SUCCESS)
		//{
		//	ret_val = verify_block_cid(&p_file_info->_bcid_info_backup, blockno, block_cid);
		//}

  //      if(ret_val == SUCCESS)
  //      {
  //          LOG_DEBUG("check_block, p_file_info:0x%x ,check blockno:%u  success.", p_file_info,blockno);
  //          set_blockmap(&p_file_info->_bcid_info, blockno);
  //      }
  //      else
  //      {
  //          LOG_DEBUG("check_block, p_file_info:0x%x ,check blockno:%u  failure.", p_file_info,blockno);
  //      }
  //  }
  //  else
  //  {
  //      ret_val = set_block_cid(&p_file_info->_bcid_info, blockno, block_cid);

  //      if(ret_val == SUCCESS)
  //      {
  //          LOG_DEBUG("check_block, p_file_info:0x%x ,set blockno:%u  success.", p_file_info,blockno);
  //          set_blockmap(&p_file_info->_bcid_info, blockno);
  //      }
  //      else
  //      {
  //          LOG_DEBUG("check_block, p_file_info:0x%x ,set blockno:%u  failure, ret_val:%u.", p_file_info, blockno, ret_val);
  //      }
  //  }

    return   ret_val;
}

_u32 set_block_cid(bcid_info*  p_bcids ,_u32 blockno, _u8 cal_bcid[CID_SIZE] )
{
    _int32 ret_val = SUCCESS;
    _u32 buffer_pos = 0;

#ifdef _LOGGER
    char  cid_str[CID_SIZE*2+1];
#endif

    LOG_DEBUG("set_block_cid, check blockno:%u, rel bcid_num:%u .", blockno, blockno >p_bcids->_bcid_num);

#ifdef _LOGGER
    str2hex((char*)cal_bcid, CID_SIZE, cid_str, CID_SIZE*2);
    cid_str[CID_SIZE*2] = '\0';

    LOG_DEBUG("set_block_cid, cal block %u cid: %s", blockno, cid_str);
#endif

    if(blockno >p_bcids->_bcid_num)
    {
        ret_val = BLOCK_NO_INVALID;
        CHECK_VALUE(ret_val);
    }

    buffer_pos = blockno*CID_SIZE;

    sd_memcpy(&p_bcids->_bcid_buffer[buffer_pos], cal_bcid, CID_SIZE);

    return ret_val;
}

_u32 verify_block_cid(bcid_info*  p_bcids ,_u32 blockno, _u8 cal_bcid[CID_SIZE])
{
    _int32 ret_val = SUCCESS;
    _u32 buffer_pos = 0;
    _u32 i = 0;

#ifdef _LOGGER
    char  cid_str[CID_SIZE*2+1];
#endif


    LOG_DEBUG("verify_block_cid, check blockno:%u, rel bcid_num:%u .", blockno, p_bcids->_bcid_num);

#ifdef _LOGGER
    str2hex((char*)cal_bcid, CID_SIZE, cid_str, CID_SIZE*2);
    cid_str[CID_SIZE*2] = '\0';

    LOG_DEBUG("verify_block_cid, cal block %u cid: %s", blockno, cid_str);
#endif


    if(blockno >p_bcids->_bcid_num)
    {
        ret_val = BLOCK_NO_INVALID;
        CHECK_VALUE(ret_val);
    }

    buffer_pos = blockno*CID_SIZE;

#ifdef _LOGGER
    str2hex((char*)(p_bcids->_bcid_buffer + buffer_pos), CID_SIZE, cid_str, CID_SIZE*2);
    cid_str[CID_SIZE*2] = '\0';

    LOG_DEBUG("verify_block_cid, exist block %u cid: %s", blockno, cid_str);
#endif


    for(i=0; i<CID_SIZE; i++)
    {
        if(p_bcids->_bcid_buffer[buffer_pos+i] != cal_bcid[i])
        {
            ret_val = BCID_CHECK_FAIL;
            break;
        }
    }

    return ret_val;
}

_u32 set_blockmap(bcid_info*  p_bcids, _u32 blockno)
{
    _u32  index  = get_bitmap_index(blockno);
    _u32  off = get_bitmap_off(blockno);

    LOG_DEBUG("set_blockmap, check blockno:%u, index:%u, off:%u .", blockno, index, off);

    p_bcids->_bcid_bitmap[index] |= bitmap_mask[off];
    return SUCCESS;
}

_u32 clear_blockmap(bcid_info*  p_bcids, _u32 blockno)
{
    _u32  index  = get_bitmap_index(blockno);
    _u32  off = get_bitmap_off(blockno);

    LOG_DEBUG("clear_blockmap, check blockno:%u, index:%u, off:%u .", blockno, index, off);

    p_bcids->_bcid_bitmap[index] &= ~bitmap_mask[off];
    return SUCCESS;
}

BOOL blockno_is_set(bcid_info*  p_bcids, _u32 blockno) // Áî®Êù•ËÆ°ÁÆóÁé∞Âú®ÈúÄË¶ÅÊ†°È™åÁöÑÂùó
{
    return ( blockno_map_is_set(p_bcids->_bcid_bitmap, blockno) 
        || bitmap_is_set(p_bcids->_bcid_checking_bitmap, blockno) )? TRUE : FALSE;
}


BOOL blockno_map_is_set(_u8*  p_bcid_map, _u32 blockno)
{
    _u32  index  = get_bitmap_index(blockno);
    _u32  off = get_bitmap_off(blockno);

    if((p_bcid_map[index] & bitmap_mask[off]) == bitmap_mask[off] )
    {
        LOG_DEBUG("blockno_map_is_set, check blockno:%u, index:%u, off:%u is set.", blockno, index, off);

        return TRUE;
    }
    else
    {
        LOG_DEBUG("blockno_map_is_set, check blockno:%u, index:%u, off:%u is not set.", blockno, index, off);

        return FALSE;
    }
}

BOOL file_info_is_all_checked(FILEINFO* p_file_info)
{
    return blockno_is_all_checked(&p_file_info->_bcid_info);
}

BOOL blockno_is_all_checked(bcid_info*  p_bcids)
{
    _u32 i =0;
    _u32 index_num =  get_bitmap_index(p_bcids->_bcid_num);
    _u32  off = get_bitmap_off(p_bcids->_bcid_num);

    _u8* cur_index = p_bcids->_bcid_bitmap;
    for( i =0; i<index_num; i++)
    {
        if(*(cur_index+i) != bitmap_mask2[7])
        {
            LOG_DEBUG("blockno_is_all_checked, bit block index:%u is not check", i);

            return FALSE;
        }
    }

    if(off != 0)
    {
        if(*(cur_index+i) != bitmap_mask2[off-1])
        {
            LOG_DEBUG("blockno_is_all_checked, last bit block index:%u is not check", i);
            return FALSE;
        }
    }

    LOG_DEBUG("blockno_is_all_checked, all checked.");

    return TRUE;

}


_u32 calc_block_cid(FILEINFO* p_file_info ,_u32 blockno, RANGE_DATA_BUFFER_LIST* buffer, _u8 ret_bcid[20],BOOL drop_over_data)
{
    _u32 ret_val = SUCCESS;
    _u32 start_index = 0;
    _u64 start_pos = 0;
    _u32 data_len =  0;
    _u32 buffer_pos = 0;
    _u32  buffer_len = 0;
    _u32 unit_num = 0;

    ctx_sha1 cid_hash_ptr;

    LIST_ITERATOR  cur_node = NULL;
    LIST_ITERATOR  erase_node = NULL;
    RANGE_DATA_BUFFER* cur_buffer = NULL;

    LOG_DEBUG("calc_block_cid, block no :%u, rel_num:%u.", blockno, p_file_info->_bcid_info._bcid_num);

    if(blockno > p_file_info->_bcid_info._bcid_num)
    {
        ret_val = BLOCK_NO_INVALID;
        CHECK_VALUE(ret_val);
    }
    //sha1_initialize(&cid_hash_ptr);

    start_index =  get_data_unit_index(blockno, p_file_info->_unit_num_per_block);
    start_pos =  get_data_pos_from_index(start_index);

    data_len =  p_file_info->_block_size;

    LOG_DEBUG("calc_block_cid, 1 block no :%u, rel_num:%u, start_index:%u, start_pos:%llu, data_len:%u.",
              blockno, p_file_info->_bcid_info._bcid_num, start_index, start_pos, data_len);

    if(start_pos + data_len > p_file_info->_file_size)
    {
        data_len =  p_file_info->_file_size - start_pos;
    }

    /*  calc hash */
    CHH_HASH_INFO* tmp = NULL;
    sd_malloc(sizeof(CHH_HASH_INFO), (void**)&tmp);
    tmp->_block_no = blockno;
    tmp->_data_len = data_len;
    tmp->_hash_result_len = 20;
    sd_malloc(20, (void**)&tmp->_hash_result);
    sd_malloc(data_len, (void**)&tmp->_data_buffer);

    LOG_DEBUG("calc_block_cid, 2 block no :%u, rel_num:%u, start_index:%u, start_pos:%llu, data_len:%u.",
              blockno, p_file_info->_bcid_info._bcid_num, start_index, start_pos, data_len);

    cur_node = LIST_BEGIN(buffer->_data_list);
    while(cur_node != LIST_END(buffer->_data_list))
    {
        cur_buffer = (RANGE_DATA_BUFFER*)LIST_VALUE(cur_node);

        LOG_DEBUG("calc_block_cid, 3 block no :%u, rel_num:%u, start_index:%u, start_pos:%llu, need data_len:%u, range(%u,%u), cur date_len:%u.",
                  blockno, p_file_info->_bcid_info._bcid_num, start_index, start_pos, data_len,
                  cur_buffer->_range._index,cur_buffer->_range._num,cur_buffer->_data_length);


        if(cur_buffer->_range._index <= start_index && cur_buffer->_range._index +  cur_buffer->_range._num > start_index)
        {
            if(cur_buffer->_range._num*get_data_unit_size() != cur_buffer->_data_length)
            {
                buffer_pos = (start_index - cur_buffer->_range._index)*get_data_unit_size();
                buffer_len = cur_buffer->_data_length - buffer_pos;
                unit_num = (buffer_len+get_data_unit_size()-1)/(get_data_unit_size());
                if(buffer_len > data_len)
                {
                    buffer_len = data_len;
                }
            }
            else
            {
                buffer_pos = (start_index - cur_buffer->_range._index)*get_data_unit_size();
                unit_num = cur_buffer->_range._index +  cur_buffer->_range._num - start_index;
                buffer_len = unit_num*get_data_unit_size();
                if(buffer_len > data_len)
                {
                    buffer_len = data_len;
                }
            }

            LOG_DEBUG("calc_block_cid, 4 block no :%u, rel_num:%u, start_index:%u, start_pos:%llu, need data_len:%u, range(%u,%u), cur data_len:%u , valid data_len:%u, unit_num:%u.",
                      blockno, p_file_info->_bcid_info._bcid_num, start_index, start_pos, data_len,
                      cur_buffer->_range._index,cur_buffer->_range._num,cur_buffer->_data_length, buffer_len, unit_num);


            //sha1_update(&cid_hash_ptr, (unsigned char*)(cur_buffer->_data_ptr + buffer_pos), buffer_len);
            sd_memcpy(tmp->_data_buffer + tmp->_data_len - data_len, cur_buffer->_data_ptr + buffer_pos, buffer_len );

            data_len -= buffer_len;
            start_index += unit_num;

            if(data_len == 0)
            {
                cur_node = LIST_NEXT(cur_node);

                while(cur_node != LIST_END(buffer->_data_list))
                {
                    cur_buffer = (RANGE_DATA_BUFFER*)LIST_VALUE(cur_node);

                    LOG_DEBUG("calc_block_cid, 5 block no :%u, rel_num:%u, start_index:%u, start_pos:%llu, need data_len:%u, range(%u,%u), cur data_len:%u.",
                              blockno, p_file_info->_bcid_info._bcid_num, start_index, start_pos, data_len,
                              cur_buffer->_range._index,cur_buffer->_range._num,cur_buffer->_data_length);

                    if(cur_buffer->_range._index + cur_buffer->_range._num <= start_index)
                    {
                        LOG_DEBUG("calc_block_cid, 6 block no :%u, rel_num:%u, start_index:%u, start_pos:%llu, need data_len:%u, range(%u,%u), cur data_len:%u  is overlad data so drop it.",
                                  blockno, p_file_info->_bcid_info._bcid_num, start_index, start_pos, data_len,
                                  cur_buffer->_range._index,cur_buffer->_range._num,cur_buffer->_data_length);
                        if(drop_over_data ==TRUE)
                        {

                            dm_free_buffer_to_data_buffer(cur_buffer->_buffer_length, &(cur_buffer->_data_ptr));

                            free_range_data_buffer_node(cur_buffer);

                            cur_buffer = NULL;

                            erase_node =  cur_node;
                            cur_node = LIST_NEXT(cur_node);

                            list_erase(&buffer->_data_list, erase_node);
                        }
                        else
                        {
                            cur_node = LIST_NEXT(cur_node);
                        }

                    }
                    else
                    {
                        break;
                    }

                }


                break;
            }
        }


        cur_node = LIST_NEXT(cur_node);
    }

    if(data_len != 0)
    {
        LOG_ERROR("calc_block_cid, block no :%u  data is no enough to calc bcid, data_len:%u.", blockno, data_len);
        sd_free(tmp->_data_buffer);
        sd_free(tmp->_hash_result);
        sd_free(tmp);
        ret_val = CHECK_DATA_BUFFER_NOT_ENOUGH;
        CHECK_VALUE(ret_val);
    }

    chh_insert_item(p_file_info->_hash_calculator, tmp);
    set_bitmap(p_file_info->_bcid_info._bcid_checking_bitmap, blockno);
    //sha1_finish(&cid_hash_ptr,ret_bcid);

    return ret_val;
}



_u32 add_check_block(FILEINFO* p_file_info ,_u32 blockno)
{
    _int32 ret_val = SUCCESS;
    _u32 cur_no  = 0;

    //LOG_DEBUG("add_check_block, block no :%u .", blockno);



    LIST_ITERATOR  cur_node = NULL;

#ifdef _VOD_NO_DISK
    if(p_file_info->_is_no_disk == TRUE)
        return SUCCESS;
#endif

    cur_node = LIST_BEGIN(p_file_info->_block_caculator._need_calc_blocks);
    LOG_DEBUG("add_check_block, block no :%u .", blockno);
    while(cur_node !=  LIST_END(p_file_info->_block_caculator._need_calc_blocks))
    {
        cur_no  = (_u32)LIST_VALUE(cur_node);

        if(cur_no < blockno)
        {
            cur_node = LIST_NEXT(cur_node);
        }
        else if(cur_no== blockno)
        {
            return ret_val;
        }
        else
        {
            break;
        }
    }

    if(cur_node !=  LIST_END(p_file_info->_block_caculator._need_calc_blocks))
    {
        list_insert(&p_file_info->_block_caculator._need_calc_blocks, (void*)blockno, cur_node);
    }
    else
    {
        list_push(&p_file_info->_block_caculator._need_calc_blocks, (void*)blockno);
    }

    return ret_val;

}

_u32 prepare_for_readbuffer(FILEINFO* p_file_info, _u32 blockno )
{
    _int32 ret_val = SUCCESS;
    _u32 data_len = 0;
    _u64 pos = 0;

    LOG_DEBUG("prepare_for_readbuffer, block no :%u .", blockno);

    if(p_file_info->_block_caculator._p_buffer_data == NULL)
    {
        //ret_val = sd_malloc(sizeof(RANGE_DATA_BUFFER), (void**)&(p_data_checker->_p_buffer_data));
        ret_val = alloc_range_data_buffer_node(&(p_file_info->_block_caculator._p_buffer_data));
        CHECK_VALUE(ret_val);

        ret_val = sd_malloc(get_read_length_for_bcid_check(), (void**)&(p_file_info->_block_caculator._p_buffer_data->_data_ptr));
        if(ret_val != SUCCESS)
        {
            // sd_free(p_data_checker->_p_buffer_data);
            free_range_data_buffer_node(p_file_info->_block_caculator._p_buffer_data);
            p_file_info->_block_caculator._p_buffer_data = NULL;
            return ret_val;
        }
        p_file_info->_block_caculator._p_buffer_data->_buffer_length = get_read_length_for_bcid_check();
    }

    data_len = p_file_info->_block_size;
    pos = data_len;

    pos *= blockno;
    if(pos + data_len > p_file_info->_file_size)
    {
        data_len =  p_file_info->_file_size - pos;
    }

    p_file_info->_block_caculator._expect_length = data_len;
    p_file_info->_block_caculator._cal_length = data_len;
    p_file_info->_block_caculator._cal_blockno = blockno;


    if(data_len > get_read_length_for_bcid_check())
    {
        data_len = get_read_length_for_bcid_check();
    }

    p_file_info->_block_caculator._p_buffer_data->_range._index = get_data_unit_index(blockno, p_file_info->_unit_num_per_block);
    p_file_info->_block_caculator._p_buffer_data->_range._num = (data_len+get_data_unit_size()-1)/get_data_unit_size();
    p_file_info->_block_caculator._p_buffer_data->_data_length = data_len;

    LOG_DEBUG("prepare_for_readbuffer, block no :%u, range (%u,%u), data_length:%u .", blockno, p_file_info->_block_caculator._p_buffer_data->_range._index,p_file_info->_block_caculator._p_buffer_data->_range._num, p_file_info->_block_caculator._p_buffer_data->_data_length);

    return ret_val;
}

BOOL file_info_has_block_need_check(FILEINFO* p_file_info)
{
    BOOL  has_block_need_check = FALSE;
    _u32 i = 0;
    RANGE block_range;

    LOG_DEBUG("file_info_has_block_need_check .");

#ifdef _VOD_NO_DISK
    if(p_file_info->_is_no_disk == TRUE)
        return SUCCESS;
#endif

    for( i = 0; i<p_file_info->_bcid_info._bcid_num; i++ )
    {
        block_range =  to_range(i, p_file_info->_block_size, p_file_info->_file_size);
        if( FALSE == blockno_is_set(&p_file_info->_bcid_info, i)
            && file_info_range_is_write(p_file_info,  &block_range) == TRUE
            && file_info_range_is_recv(p_file_info,  &block_range) == TRUE 
            && p_file_info->_block_caculator._cal_blockno != i )
        {
            add_check_block(p_file_info,  i);
            has_block_need_check =  TRUE;
            LOG_DEBUG("file_info_has_block_need_check,  block %u need to check .", i);
        }
    }

    if(p_file_info->_is_calc_bcid == FALSE)
    {
        LOG_DEBUG("file_info_has_block_need_check but is_calc_bcid = FALSE");
        has_block_need_check = FALSE;
    }

    if(has_block_need_check == FALSE)
    {
        LOG_DEBUG("file_info_has_block_need_check, no block to check .");
        if(file_info_ia_all_received(p_file_info) == TRUE)
        {
            file_info_notify_file_result(p_file_info,SUCCESS);
        }
    }
    else
    {
        LOG_DEBUG("file_info_has_block_need_check, there blocks need to check .");
    }

    return  has_block_need_check;
}


_u32 start_check_blocks(FILEINFO* p_file_info)
{

    _int32 ret_val = SUCCESS;
    BOOL  need_check = TRUE;
    _u32  need_calc_block =0;

    _u32 calc_nums = list_size(&p_file_info->_block_caculator._need_calc_blocks) ;

    LOG_DEBUG("start_check_blocks, calc nums is :%u . is_need_calc bcid = %d"
        , calc_nums, p_file_info->_is_need_calc_bcid );

#ifdef _VOD_NO_DISK
    if(p_file_info->_is_no_disk == TRUE)
        return SUCCESS;
#endif
    if(p_file_info->_is_calc_bcid == FALSE )
    {
        LOG_DEBUG("start_check_blocks but is_calc_bcid = FALSE");
        return SUCCESS;
    }

    if(p_file_info->_is_need_calc_bcid == FALSE)
    {
        LOG_DEBUG("start_check_blocks but _is_need_calc_bcid = FALSE");
        if(file_info_ia_all_received(p_file_info) == TRUE)
        {
            file_info_notify_file_result(p_file_info,SUCCESS);
        }
        return SUCCESS;
    }

    if( NULL == p_file_info->_tmp_file_struct )
    {
        return SUCCESS;
    }

    if(calc_nums == 0)
    {
        LOG_DEBUG("start_check_blocks, no block check .");

        need_check = file_info_has_block_need_check(p_file_info);

        if(need_check == TRUE)
        {
            LOG_DEBUG("start_check_blocks, there are block need to check .");

            ret_val = start_check_blocks(p_file_info);
        }
        else
        {
            LOG_DEBUG("start_check_blocks, no  block need to check, so write cfg .");

        }
        return ret_val;
    }

    if(p_file_info->_block_caculator._cal_blockno != MAX_U32)
    {
        LOG_DEBUG("start_check_blocks, because there is one block is reading ,so can not read now!! .");

        return SUCCESS;
    }

    list_pop(&p_file_info->_block_caculator._need_calc_blocks, (void**)&need_calc_block);

    if(need_calc_block > p_file_info->_bcid_info._bcid_num)
    {
        LOG_DEBUG("start_check_blocks, need_calc_block :%u is invalid, rel num is %u.", need_calc_block, p_file_info->_bcid_info._bcid_num);

        ret_val = BLOCK_NO_INVALID;
        CHECK_VALUE(ret_val);
    }

    ret_val = prepare_for_readbuffer(p_file_info, need_calc_block);
    if(ret_val == OUT_OF_MEMORY)
    {
        LOG_DEBUG("start_check_blocks, need_calc_bloc=%u, but alloc read buffer fail, so cannot check now!! ", need_calc_block);
        add_check_block(p_file_info, need_calc_block);
        file_info_notify_file_result(p_file_info, DATA_ALLOC_READ_BUFFER_ERROR);
        return ret_val;
    }

    ret_val = prepare_for_shahash(p_file_info);
    if(ret_val == OUT_OF_MEMORY)
    {
        LOG_DEBUG("start_check_blocks, need_calc_bloc=%u, but alloc ssha buffer fail, so cannot check now!! ", need_calc_block);
        add_check_block(p_file_info, need_calc_block);
        file_info_notify_file_result(p_file_info, DATA_ALLOC_READ_BUFFER_ERROR);
        return ret_val;

    }

    p_file_info->_block_caculator._cal_blockno = need_calc_block;

    ret_val = fm_file_asyn_read_buffer(p_file_info->_tmp_file_struct,p_file_info->_block_caculator._p_buffer_data
        , (notify_read_result)notify_read_data_result, (void*)p_file_info);

    //set_bitmap(p_file_info->_bcid_info._bcid_checking_bitmap, p_file_info->_block_caculator._cal_blockno);

    return ret_val;

}

_u32 clear_check_blocks(FILEINFO* p_file_info)
{
    LOG_DEBUG("clear_check_blocks .");

    clear_list(&p_file_info->_block_caculator._need_calc_blocks);
    return SUCCESS;
}

_u32 handle_read_failure(FILEINFO* p_file_info, _int32 error)
{
    LOG_DEBUG("handle_read_failure, error :%d .", error);

    // data_manager_notify_failure(p_data_checker->_p_data_manager, DATA_READ_FILE_ERROR);

    // p_file_info->_call_back_func_table.notify_file_result(p_file_info->_p_user_ptr,p_file_info,  DATA_READ_FILE_ERROR);

    file_info_notify_file_result(p_file_info, DATA_READ_FILE_ERROR);
    return SUCCESS;
}

_u32 handle_write_failure(FILEINFO* p_file_info, _int32 error)
{
    LOG_DEBUG("handle_write_failure, error :%d .", error);
    if(error == 22 && p_file_info->_file_size > 4294967296L)
        file_info_notify_file_result(p_file_info, DATA_FILE_BIGGER_THAN_4G);
    else
        file_info_notify_file_result(p_file_info, DATA_WRITE_FILE_ERROR);
    return SUCCESS;
}

_u32 handle_create_failure(FILEINFO* p_file_info, _int32 error)
{
    LOG_DEBUG("handle_create_failure, error :%d .", error);

    if( error == INSUFFICIENT_DISK_SPACE )
    {
        file_info_notify_create(p_file_info, DATA_NO_SPACE_TO_CREAT_FILE);
    }
    else if( error == FILE_TOO_BIG)
    {
        file_info_notify_create(p_file_info, DATA_FILE_BIGGER_THAN_4G);
    }
    else
    {
        file_info_notify_create(p_file_info, DATA_CREAT_FILE_ERROR);
    }
    return SUCCESS;
}

_u32 notify_read_data_result(struct tagTMP_FILE*  file_struct, void* user_ptr,RANGE_DATA_BUFFER* data_buffer, _int32 read_result, _int32 read_len)
{
    _int32 ret_val = SUCCESS;
    _u8 block_cid[CID_SIZE];
    _u32 valid_num = 0;
    RANGE check_r;

    FILEINFO*  ptr_file_info = (FILEINFO*) user_ptr;

    if(read_result == FM_READ_CLOSE_EVENT)
    {
        LOG_DEBUG("notify_read_data_result, file info :0x%x, has close, clear buffer list:0x%x .", ptr_file_info, data_buffer);

        if(data_buffer != NULL)
        {
            if(data_buffer->_data_ptr != NULL)
            {
                sd_free(data_buffer->_data_ptr);
                data_buffer->_data_ptr = NULL;
            }
            free_range_data_buffer_node(data_buffer);
        }

        return ret_val;
    }

    if(read_result != SUCCESS)
    {
        ptr_file_info->_block_caculator._cal_blockno = MAX_U32;
        
        SAFE_DELETE(ptr_file_info->_block_caculator._data_buffer);
        ptr_file_info->_block_caculator._expect_length = 0;

        file_info_notify_file_result(ptr_file_info, read_result);

        return SUCCESS;
    }

    if(ptr_file_info->_block_caculator._p_buffer_data != data_buffer)
    {
        LOG_ERROR("notify_read_data_result, ptr_file_info :0x%x, ret buffer:0x%x is not equal origin buffer .", ptr_file_info, data_buffer, ptr_file_info->_block_caculator._p_buffer_data);

        read_result = READ_FILE_ERR;
        SAFE_DELETE(ptr_file_info->_block_caculator._data_buffer);
        ptr_file_info->_block_caculator._expect_length = 0;
        file_info_notify_file_result(ptr_file_info, read_result);
        return SUCCESS;
    }

    if(ptr_file_info->_block_caculator._cal_blockno == MAX_U32)
    {
        LOG_DEBUG("notify_read_data_result, ptr_file_info :0x%x, read buffer list:0x%x, ret_len:%u, cal_length:%u, because cal bcid no is MAXU32, so no need read ,start check .",
                  ptr_file_info, data_buffer, read_len, ptr_file_info->_block_caculator._cal_length, ptr_file_info->_block_caculator._cal_blockno);
        ret_val = start_check_blocks(ptr_file_info);
        return SUCCESS;
    }

    LOG_DEBUG("notify_read_data_result, ptr_file_info :0x%x, read buffer list:0x%x, ret_len:%u, cal_length:%u .", ptr_file_info, data_buffer, read_len, ptr_file_info->_block_caculator._cal_length);

    if(read_len >= ptr_file_info->_block_caculator._cal_length)
    {
        if(read_len >  ptr_file_info->_block_caculator._cal_length)
        {
            LOG_ERROR("notify_read_data_result, ptr_file_info :0x%x, read buffer list:0x%x, ret_len:%u is large than cal_length:%u .", ptr_file_info, data_buffer, read_len, ptr_file_info->_block_caculator._cal_length);
            read_len =  ptr_file_info->_block_caculator._cal_length;
        }

        sd_memcpy(ptr_file_info->_block_caculator._data_buffer 
            + ptr_file_info->_block_caculator._expect_length - ptr_file_info->_block_caculator._cal_length
            , (unsigned char*)data_buffer->_data_ptr, read_len );
        //sha1_update(&ptr_file_info->_block_caculator._hash_ptr, (unsigned char*)data_buffer->_data_ptr, read_len);
        ptr_file_info->_block_caculator._cal_length = 0;
        
        CHH_HASH_INFO* tmp = NULL;
        sd_malloc(sizeof(CHH_HASH_INFO), (void**)&tmp);
        tmp->_block_no = ptr_file_info->_block_caculator._cal_blockno;
        tmp->_data_len = ptr_file_info->_block_caculator._expect_length;
        tmp->_hash_result_len = 20;
        sd_malloc(20, (void**)&tmp->_hash_result);
        tmp->_data_buffer = ptr_file_info->_block_caculator._data_buffer;
        ptr_file_info->_block_caculator._data_buffer = NULL;
        ptr_file_info->_block_caculator._expect_length = 0;

        chh_insert_item(ptr_file_info->_hash_calculator, tmp);
        set_bitmap(ptr_file_info->_bcid_info._bcid_checking_bitmap, ptr_file_info->_block_caculator._cal_blockno);
        //sha1_finish(&ptr_file_info->_block_caculator._hash_ptr,block_cid);

   //     check_r = to_range(ptr_file_info->_block_caculator._cal_blockno, ptr_file_info->_block_size, ptr_file_info->_file_size);

   //     if(ptr_file_info->_bcid_is_valid == TRUE
   //        && (ptr_file_info->_no_need_check_range._node_size == 0
   //            || (range_list_is_include(&ptr_file_info->_no_need_check_range, &check_r) == FALSE) ))
   //     {
   //         LOG_DEBUG("notify_read_data_result, file info :0x%x, ret buffer:0x%x, read blockno:%u ,finish calc bcid .",ptr_file_info, data_buffer, ptr_file_info->_block_caculator._cal_blockno);

   //         ret_val = verify_block_cid(&ptr_file_info->_bcid_info, ptr_file_info->_block_caculator._cal_blockno, block_cid);

			//LOG_DEBUG("ptr_file_info->_is_backup_bcid_valid = %d, ret_val = %d", ptr_file_info->_is_backup_bcid_valid, ret_val);
			//if(ptr_file_info->_is_backup_bcid_valid == TRUE && ret_val != SUCCESS)
			//{
			//	ret_val = verify_block_cid(&ptr_file_info->_bcid_info_backup, ptr_file_info->_block_caculator._cal_blockno, block_cid);
			//}

   //         if(ret_val == SUCCESS)
   //         {
   //             LOG_DEBUG("notify_read_data_result, file info :0x%x, ret buffer:0x%x, read blockno:%u ,check success .",ptr_file_info, data_buffer, ptr_file_info->_block_caculator._cal_blockno);

   //             set_blockmap(&ptr_file_info->_bcid_info, ptr_file_info->_block_caculator._cal_blockno);

   //             //             dm_check_block_success(ptr_file_info->_p_data_manager, ptr_file_info->_block_caculator._cal_blockno);
   //             //      ptr_file_info->_call_back_func_table.notify_file_block_check_result(ptr_file_info->_p_user_ptr,ptr_file_info, )
   //             file_info_notify_check(ptr_file_info,ptr_file_info->_block_caculator._cal_blockno, TRUE);
   //         }
   //         else if(ret_val == BCID_CHECK_FAIL)
   //         {
   //             LOG_DEBUG("notify_read_data_result, file info :0x%x, ret buffer:0x%x, read blockno:%u ,check failure .",ptr_file_info, data_buffer, ptr_file_info->_block_caculator._cal_blockno);

   //             //dm_check_block_failure(ptr_data_checker->_p_data_manager, ptr_data_checker->_cal_blockno);

   //             file_info_notify_check(ptr_file_info,ptr_file_info->_block_caculator._cal_blockno, FALSE);
   //         }
   //     }
   //     else
   //     {
   //         LOG_DEBUG("notify_read_data_result, file info :0x%x, ret buffer:0x%x, read blockno:%u ,finish 2 calc bcid .",ptr_file_info, data_buffer, ptr_file_info->_block_caculator._cal_blockno);
   //         out_put_range_list(&ptr_file_info->_no_need_check_range);

   //         if(ptr_file_info->_bcid_is_valid == FALSE)
   //         {
   //             ret_val = set_block_cid(&ptr_file_info->_bcid_info, ptr_file_info->_block_caculator._cal_blockno, block_cid);
   //         }

   //         if(ret_val == SUCCESS)
   //         {
   //             LOG_DEBUG("notify_read_data_result, file info :0x%x, ret buffer:0x%x, read blockno:%u ,set success .",ptr_file_info, data_buffer, ptr_file_info->_block_caculator._cal_blockno);

   //             set_blockmap(&ptr_file_info->_bcid_info, ptr_file_info->_block_caculator._cal_blockno);
   //             //  dm_check_block_success(ptr_data_checker->_p_data_manager, ptr_data_checker->_cal_blockno);
   //             file_info_notify_check(ptr_file_info,ptr_file_info->_block_caculator._cal_blockno, TRUE);
   //         }
   //         else
   //         {
   //             LOG_DEBUG("notify_read_data_result, file info :0x%x, ret buffer:0x%x, read blockno:%u ,set failure .",ptr_file_info, data_buffer, ptr_file_info->_block_caculator._cal_blockno);
   //         }
   //     }

        ptr_file_info->_block_caculator._cal_blockno = MAX_U32;

        ret_val = start_check_blocks(ptr_file_info);

        return (ret_val);
    }

    valid_num = read_len/get_data_unit_size();

    LOG_DEBUG("notify_read_data_result, file info :0x%x, read buffer list:0x%x, ret_len:%u, cal_length:%u, valid_num:%u .", ptr_file_info, data_buffer, read_len, ptr_file_info->_block_caculator._cal_length, valid_num);

    if(valid_num == 0)
    {
        ret_val = fm_file_asyn_read_buffer(ptr_file_info->_tmp_file_struct,  ptr_file_info->_block_caculator._p_buffer_data,(notify_read_result)notify_read_data_result, (void*)ptr_file_info);
        return ret_val;
    }

    read_len = valid_num*get_data_unit_size();

    //sha1_update(&ptr_file_info->_block_caculator._hash_ptr, (unsigned char*)data_buffer->_data_ptr, read_len);

    sd_memcpy(ptr_file_info->_block_caculator._data_buffer 
        + ptr_file_info->_block_caculator._expect_length - ptr_file_info->_block_caculator._cal_length
        , (unsigned char*)data_buffer->_data_ptr, read_len );

    ptr_file_info->_block_caculator._cal_length -= read_len;

    if(read_len <=  ptr_file_info->_block_caculator._p_buffer_data->_data_length)
    {
        ptr_file_info->_block_caculator._p_buffer_data->_data_length -= read_len;
    }
    else
    {
        LOG_ERROR("notify_read_data_result, file info :0x%x, read buffer list:0x%x, read len:%u is large than need len: %u .", ptr_file_info, data_buffer, read_len, ptr_file_info->_block_caculator._p_buffer_data->_data_length);
        ptr_file_info->_block_caculator._p_buffer_data->_data_length = 0;
    }

    ptr_file_info->_block_caculator._p_buffer_data->_range._index += valid_num;
    ptr_file_info->_block_caculator._p_buffer_data->_range._num -= valid_num;

    if(ptr_file_info->_block_caculator._p_buffer_data->_data_length == 0)
    {
        if(ptr_file_info->_block_caculator._cal_length > get_read_length_for_bcid_check())
        {
            ptr_file_info->_block_caculator._p_buffer_data->_data_length = get_read_length_for_bcid_check();
        }
        else
        {
            ptr_file_info->_block_caculator._p_buffer_data->_data_length = ptr_file_info->_block_caculator._cal_length;
        }

        ptr_file_info->_block_caculator._p_buffer_data->_range._num = ( ptr_file_info->_block_caculator._p_buffer_data->_data_length+get_data_unit_size()-1)/get_data_unit_size();
    }

    LOG_DEBUG("notify_read_data_result, file info :0x%x, read buffer list:0x%x, ret_len:%u, cal_length:%u, valid_num:%u, index:%u,num:%u,data_len:%u .",
              ptr_file_info, data_buffer, read_len, ptr_file_info->_block_caculator._cal_length, valid_num, ptr_file_info->_block_caculator._p_buffer_data->_range._index,
              ptr_file_info->_block_caculator._p_buffer_data->_range._num, ptr_file_info->_block_caculator._p_buffer_data->_data_length);

    ret_val = fm_file_asyn_read_buffer(ptr_file_info->_tmp_file_struct,  ptr_file_info->_block_caculator._p_buffer_data,(notify_read_result)notify_read_data_result, (void*)ptr_file_info);

    return ret_val;
}


_u32 file_info_flush_data(FILEINFO* p_file_info, RANGE_DATA_BUFFER_LIST* buffer)
{

    _int32 ret_val = SUCCESS;

    LOG_DEBUG("file_info_flush_data .");

#ifdef _VOD_NO_DISK
    if(p_file_info->_is_no_disk == TRUE)
        return SUCCESS;
#endif

    if(p_file_info->_tmp_file_struct == NULL)
    {
        LOG_DEBUG("file_info_flush_data , try create file.");
        file_info_try_create_file(p_file_info);
        return  FILE_CREATING;
    }

    if( fm_file_is_created( p_file_info->_tmp_file_struct) == FALSE)
    {
        LOG_DEBUG("file_info_flush_data ,  file file is creating.");
        file_info_try_create_file(p_file_info);
        return  FILE_CREATING;
    }

    LOG_DEBUG("file_info_flush_data, flush write buffer.");


    ret_val = fm_file_write_buffer_list(p_file_info->_tmp_file_struct
            ,  buffer
            , (notify_write_result)notify_write_data_result
            , (void*)p_file_info);

    return ret_val;

}


_u32 notify_write_data_result(struct tagTMP_FILE*  file_struct
    , void* user_ptr
    , RANGE_DATA_BUFFER_LIST* buffer_list
    , _int32 write_result)
{
    _int32 ret_val = SUCCESS;
    FILEINFO*  ptr_file_info = (FILEINFO*) user_ptr;

    LOG_DEBUG("notify_write_data_result .");

    if(write_result == FM_WRITE_CLOSE_EVENT)
    {
        LOG_DEBUG("notify_write_data_result, file info  :0x%x, has close, clear buffer_list:0x%x .", ptr_file_info, buffer_list);

        ret_val = drop_buffer_list_without_buffer(buffer_list);
        return ret_val;
    }

    if(write_result != SUCCESS)
    {
        drop_buffer_list_without_buffer(buffer_list);

        handle_write_failure(ptr_file_info, write_result);
        return SUCCESS;
    }

    range_list_add_buffer_range_list(&ptr_file_info->_writed_range_list, buffer_list);

    file_info_save_to_cfg_file(ptr_file_info);

    if( ptr_file_info->_wait_for_close == TRUE)
    {
        //file_info_save_to_cfg_file(ptr_file_info);
        drop_buffer_list_without_buffer(buffer_list);
        return ret_val;
    }

    if(ptr_file_info->_file_size != 0)
    {
        LOG_DEBUG("notify_write_data_result, file info :0x%x,  buffer_list:0x%x , filesize is valid, so check bcid .", ptr_file_info, buffer_list);

        file_info_cal_need_check_block(ptr_file_info,  buffer_list) ;
        drop_buffer_list_without_buffer(buffer_list);
        ret_val = start_check_blocks(ptr_file_info);
    }
    else
    {
        LOG_DEBUG("notify_write_data_result, file info  :0x%x,  buffer_list:0x%x , filesize is invalid, so drop buffer list .", ptr_file_info, buffer_list);

        drop_buffer_list_without_buffer(buffer_list);
    }

    return ret_val;

}

_u32 file_info_cal_need_check_block(FILEINFO* p_file_info, RANGE_DATA_BUFFER_LIST* buffer)
{
    RANGE_DATA_BUFFER* cur_buffer = NULL;
    _u32 cur_index = 0;
    _u32 block_no  = 0;
    RANGE block_range;

    LIST_ITERATOR  cur_node = LIST_BEGIN(buffer->_data_list);

    if(p_file_info->_is_calc_bcid == FALSE )
    {
        LOG_DEBUG("file_info_cal_need_check_block but is_calc_bcid = FALSE");
        return SUCCESS;
    }

    if(p_file_info->_is_need_calc_bcid == FALSE)
    {
        LOG_DEBUG("file_info_has_block_need_check but _is_need_calc_bcid = FALSE");
        return SUCCESS;
    }


    LOG_DEBUG("data_check_cal_need_check_block .");

    while(cur_node != LIST_END(buffer->_data_list))
    {
        cur_buffer = (RANGE_DATA_BUFFER*)LIST_VALUE(cur_node);
        cur_index = cur_buffer->_range._index;
        block_no  = cur_index/((p_file_info->_block_size)/get_data_unit_size());
        block_range =  to_range(block_no, p_file_info->_block_size, p_file_info->_file_size);

        if( file_info_range_is_write(p_file_info, &block_range)== TRUE
            && FALSE == blockno_is_set(&p_file_info->_bcid_info, block_no) 
            && p_file_info->_block_caculator._cal_blockno != block_no )
        {
            LOG_DEBUG("data_check_cal_need_check_block, add block no:%u .", block_no);
            add_check_block(p_file_info,  block_no);
        }

        cur_node= LIST_NEXT(cur_node);
    }

    return SUCCESS;
}

BOOL get_file_3_part_cid(FILEINFO* p_file_info, _u8 cid[CID_SIZE] , _int32* err_code)
{
    _int32 ret_val = SUCCESS;
    /*_u8* data_buffer = NULL;*/
    RANGE f ;
    RANGE_DATA_BUFFER read_buf;
    ctx_sha1 cid_hash_ptr;
    _u32 cur_buffer_len= 0;
    _u64 offset = 0;

    LOG_DEBUG("calling get_file_3_part_cid.");

    if(err_code != NULL)
        *err_code = 0;

    if(p_file_info->_tmp_file_struct == NULL
       || !fm_file_is_created(p_file_info->_tmp_file_struct) )
    {
        LOG_ERROR("get_file_3_part_cid failue because tmp file is not create.");
        return FALSE;
    }
    
    if( TCID_SAMPLE_SIZE >= p_file_info->_file_size)
    {
        f =  pos_length_to_range(0, p_file_info->_file_size,  p_file_info->_file_size);


        read_buf._range = f;
        read_buf._data_length = p_file_info->_file_size;

        LOG_DEBUG("get_file_3_part_cid, calc cid because filsize is small, filesize:%llu , range(%u,%u) .", p_file_info->_file_size, f._index, f._num);

        ret_val = sd_malloc(read_buf._data_length, (void**)&read_buf._data_ptr);
        if(ret_val  != SUCCESS)
        {
            LOG_ERROR("get_file_3_part_cid, calc cid because filsize is small, filesize:%llu , range(%u,%u) , alloc buffer fail, buffer_length:%u.", p_file_info->_file_size, f._index, f._num, read_buf._data_length);

            if(err_code != NULL)
                *err_code = DM_CAL_CID_NO_BUFFER;
            return FALSE;
        }

        ret_val =  fm_file_syn_read_buffer( p_file_info->_tmp_file_struct, &read_buf);
        if(ret_val != SUCCESS)
        {
            LOG_ERROR("get_file_3_part_cid, calc cid because filsize is small, filesize:%llu , range(%u,%u) ,  buffer_length:%u, syn read failure.", p_file_info->_file_size, f._index, f._num, read_buf._data_length);

            sd_free(read_buf._data_ptr);
            read_buf._data_ptr = NULL;
            if(err_code != NULL)
                *err_code = DM_CAL_CID_READ_ERROR;
            return FALSE;
        }

        sha1_initialize(&cid_hash_ptr);
        sha1_update(&cid_hash_ptr, (unsigned char*)read_buf._data_ptr, read_buf._data_length);
        sha1_finish(&cid_hash_ptr,cid);

        sd_free(read_buf._data_ptr);
        read_buf._data_ptr = NULL;
        return TRUE;

    }
    else
    {
        /*first part*/

        sha1_initialize(&cid_hash_ptr);
        f =  pos_length_to_range(0, TCID_SAMPLE_UNITSIZE,  p_file_info->_file_size);

        read_buf._range = f;
        read_buf._data_length = f._num*get_data_unit_size();
        offset = ((_u64)f._index)*((_u64)get_data_unit_size());

        LOG_DEBUG("get_file_3_part_cid, calc cid0 , filesize:%llu , range(%u,%u), data_length:%u, offset:%llu .", p_file_info->_file_size, f._index, f._num, read_buf._data_length, offset);

        if(offset+  read_buf._data_length> p_file_info->_file_size)
        {
            read_buf._data_length = p_file_info->_file_size - offset;
        }

        ret_val = sd_malloc(read_buf._data_length, (void**)&read_buf._data_ptr);
        if(ret_val  != SUCCESS)
        {
            LOG_ERROR("get_file_3_part_cid, calc cid0 , filesize:%llu , range(%u,%u), data_length:%u, offset:%llu , buffer alloc failure.", p_file_info->_file_size, f._index, f._num, read_buf._data_length, offset);
            if(err_code != NULL)
                *err_code = DM_CAL_CID_NO_BUFFER;
            return FALSE;
        }

        cur_buffer_len = read_buf._data_length;

        ret_val =  fm_file_syn_read_buffer( p_file_info->_tmp_file_struct,  &read_buf);
        if(ret_val != SUCCESS)
        {
            LOG_ERROR("get_file_3_part_cid, calc cid0 , filesize:%llu , range(%u,%u), data_length:%u, offset:%llu ,  syn raed failure.", p_file_info->_file_size, f._index, f._num, read_buf._data_length, offset);

            sd_free(read_buf._data_ptr);
            read_buf._data_ptr = NULL;

            if(err_code != NULL)
                *err_code = DM_CAL_CID_READ_ERROR;
            return FALSE;
        }

        sha1_update(&cid_hash_ptr, (unsigned char*)read_buf._data_ptr , TCID_SAMPLE_UNITSIZE);

        /*second part*/
        f =  pos_length_to_range(p_file_info->_file_size / 3, TCID_SAMPLE_UNITSIZE,  p_file_info->_file_size);

        read_buf._range = f;
        read_buf._data_length = f._num*get_data_unit_size();
        offset =((_u64) f._index)*((_u64)get_data_unit_size());

        if(offset+  read_buf._data_length> p_file_info->_file_size)
        {
            read_buf._data_length = p_file_info->_file_size - offset;
        }

        LOG_DEBUG("get_file_3_part_cid, calc cid1 , filesize:%llu , range(%u,%u), data_length:%u, offset:%llu .", p_file_info->_file_size, f._index, f._num, read_buf._data_length, offset);

        if(read_buf._data_length > cur_buffer_len)
        {
            sd_free(read_buf._data_ptr);
            read_buf._data_ptr = NULL;
            ret_val = sd_malloc(read_buf._data_length, (void**)&read_buf._data_ptr);
            if(ret_val  != SUCCESS)
            {
                LOG_ERROR("get_file_3_part_cid, calc cid1 , filesize:%llu , range(%u,%u), data_length:%u, offset:%llu , buffer alloc failure.", p_file_info->_file_size, f._index, f._num, read_buf._data_length, offset);
                if(err_code != NULL)
                    *err_code = DM_CAL_CID_NO_BUFFER;
                return FALSE;
            }

            cur_buffer_len = read_buf._data_length;
        }

        ret_val =  fm_file_syn_read_buffer( p_file_info->_tmp_file_struct ,  &read_buf);
        if(ret_val != SUCCESS)
        {
            LOG_ERROR("get_file_3_part_cid, calc cid1 , filesize:%llu , range(%u,%u), data_length:%u, offset:%llu ,  syn raed failure.", p_file_info->_file_size, f._index, f._num, read_buf._data_length, offset);

            sd_free(read_buf._data_ptr);
            read_buf._data_ptr = NULL;
            if(err_code != NULL)
                *err_code = DM_CAL_CID_READ_ERROR;
            return FALSE;
        }

        sha1_update(&cid_hash_ptr, (unsigned char*)(read_buf._data_ptr+p_file_info->_file_size/3 -  offset) , TCID_SAMPLE_UNITSIZE);

        /*third part*/
        f =  pos_length_to_range(p_file_info->_file_size - TCID_SAMPLE_UNITSIZE, TCID_SAMPLE_UNITSIZE,  p_file_info->_file_size);

        read_buf._range = f;
        read_buf._data_length = f._num*get_data_unit_size();
        offset =((_u64) f._index)*((_u64)get_data_unit_size());

        if(offset + read_buf._data_length> p_file_info->_file_size)
        {
            read_buf._data_length = p_file_info->_file_size - offset;
        }

        LOG_DEBUG("get_file_3_part_cid, calc cid2 , filesize:%llu , range(%u,%u), data_length:%u, offset:%llu .", p_file_info->_file_size, f._index, f._num, read_buf._data_length, offset);

        if(read_buf._data_length > cur_buffer_len)
        {
            sd_free(read_buf._data_ptr);
            read_buf._data_ptr = NULL;
            ret_val = sd_malloc(read_buf._data_length, (void**)&read_buf._data_ptr);
            if(ret_val  != SUCCESS)
            {
                LOG_ERROR("get_file_3_part_cid, calc cid2 , filesize:%llu , range(%u,%u), data_length:%u, offset:%llu , buffer alloc failure.", p_file_info->_file_size, f._index, f._num, read_buf._data_length, offset);
                if(err_code != NULL)
                    *err_code = DM_CAL_CID_NO_BUFFER;
                return ret_val;
            }

            cur_buffer_len = read_buf._data_length;
        }

        ret_val =  fm_file_syn_read_buffer( p_file_info->_tmp_file_struct ,  &read_buf);
        if(ret_val != SUCCESS)
        {
            LOG_ERROR("get_file_3_part_cid, calc cid2 , filesize:%llu , range(%u,%u), data_length:%u, offset:%llu ,  syn raed failure.", p_file_info->_file_size, f._index, f._num, read_buf._data_length, offset);

            sd_free(read_buf._data_ptr);
            read_buf._data_ptr = NULL;

            if(err_code != NULL)
                *err_code = DM_CAL_CID_READ_ERROR;

            return FALSE;
        }

        sha1_update(&cid_hash_ptr, (unsigned char*)(read_buf._data_ptr+p_file_info->_file_size - TCID_SAMPLE_UNITSIZE -  offset) , TCID_SAMPLE_UNITSIZE);

        sha1_finish(&cid_hash_ptr,cid);
        sd_free(read_buf._data_ptr);
        read_buf._data_ptr = NULL;
        return TRUE;
    }

}

BOOL get_file_gcid_helper(bcid_info *_bcid_info, _u8 gcid[CID_SIZE], BOOL is_bakup)
{
	ctx_sha1 cid_hash_ptr;

	LOG_DEBUG("get_file_gcid_helper, _bcid_info=0x%x.", _bcid_info);

	if (!_bcid_info) 
	{
		LOG_URGENT("get_file_gcid_helper parameter error.");
		return FALSE;
	}

	// Â¶ÇÊûúÊòØÊ†°È™åÂ§á‰ªΩÁöÑbcidÔºåÂ∞±‰∏çÂÜçÈúÄË¶ÅËøô‰∏™Ê£ÄÊü•ÊòØ‰∏çÊòØÊâÄÊúâÂùóÈÉΩÊ£ÄÈ™åËøá‰∫ÜÔºåÂè™ÊòØ‰∏∫‰∫ÜÂæóÂà∞Ëøô‰∏™gcid
	if (!is_bakup) 
	{
		if(FALSE == blockno_is_all_checked(_bcid_info))
		{
			LOG_ERROR("get_file_gcid_helper , failure because blockno_is_all_checked FALSE.");
			return FALSE;
		}
	}
#ifdef _DEBUG
	{
		char hexstr[64] = { 0 };
		int i = 0;
		while (i < _bcid_info->_bcid_num)
		{
			str2hex(_bcid_info->_bcid_buffer + i*CID_SIZE, CID_SIZE, hexstr, sizeof hexstr);
			LOG_DEBUG("%04d: %s", i, hexstr);
			i++;
		}
	}
#endif
	sha1_initialize(&cid_hash_ptr);
	sha1_update(&cid_hash_ptr, (unsigned char*)(_bcid_info->_bcid_buffer), _bcid_info->_bcid_num*CID_SIZE);
	sha1_finish(&cid_hash_ptr, gcid);
	return TRUE;
}

BOOL get_file_gcid_backup(FILEINFO* p_file_info, _u8 gcid[CID_SIZE])
{
	LOG_DEBUG("get_file_gcid_backup.");

	if (!p_file_info)
	{
		LOG_URGENT("get_file_gcid_backup p_file_info is NULL.");
		return FALSE;
	}

	if(p_file_info->_file_size == 0 || p_file_info->_bcid_info_backup._bcid_num == 0)
	{
		LOG_ERROR("get_file_gcid_backup , failure because filesize is 0 or bcid_num is 0.");
		return FALSE;
	}

	return get_file_gcid_helper(&p_file_info->_bcid_info_backup, gcid, TRUE);
}

BOOL get_file_gcid(FILEINFO* p_file_info, _u8 gcid[CID_SIZE])
{
    LOG_DEBUG("get_file_gcid.");

	if (!p_file_info)
	{
		LOG_URGENT("get_file_gcid p_file_info is NULL.");
		return FALSE;
	}

    if(p_file_info->_file_size == 0 || p_file_info->_bcid_info._bcid_num == 0)
    {
        LOG_ERROR("get_file_gcid , failure because filesize is 0 or bcid_num is 0.");
        return FALSE;
    }

    return get_file_gcid_helper(&p_file_info->_bcid_info, gcid, FALSE);
}

_int32 file_info_flush_cached_buffer(FILEINFO* p_file_info)
{
    _u32 ret_val = SUCCESS;

    LOG_DEBUG("file_info_flush_cached_buffer .");

    if(list_size(&p_file_info->_flush_buffer_list._data_list) == 0)
    {
        LOG_DEBUG("dm_flush_data_to_file no data to flush, so start check data.");
        start_check_blocks(p_file_info);
        return ret_val;
    }

    LOG_DEBUG("dm_flush_cached_buffer, try flush cache flush data size is %u.", list_size(&p_file_info->_flush_buffer_list._data_list));

    ret_val =  file_info_flush_data(p_file_info, &p_file_info->_flush_buffer_list);

    LOG_DEBUG("dm_flush_cached_buffer, ret_val :%u .", ret_val);

    return (_int32)ret_val;
}

_int32 file_info_notify_file_create(struct tagTMP_FILE* file_struct, void* user_ptr, _int32 create_result)
{
    _int32 ret_val = SUCCESS;
    FILEINFO*  ptr_file_info = (FILEINFO*) user_ptr;


    if(create_result == FM_CREAT_CLOSE_EVENT)
    {
        LOG_ERROR("file_info_notify_file_create, file_info :0x%x, because file is closed .", ptr_file_info);
        return SUCCESS;
    }

    if(create_result != SUCCESS)
    {
        LOG_ERROR("file_info_notify_file_create, file_info :0x%x, failure .", ptr_file_info);

        handle_create_failure(ptr_file_info, create_result);

        //data_manager_notify_file_create_info(ptr_data_checker->_p_data_manager, FALSE);
        //file_info_notify_create(ptr_file_info, create_result);
        return SUCCESS;
    }

    LOG_DEBUG("file_info_notify_file_create, file_info :0x%x, success .", ptr_file_info);

    /*if(ptr_data_checker->_file_size != 0)
    {
           LOG_DEBUG("data_check_notify_file_create, DATA_CHECKER :0x%x, success, set filesize:%llu .", ptr_data_checker, ptr_data_checker->_file_size);
        fm_set_filesize( file_struct, ptr_data_checker->_file_size);
    }
    else
    {
          LOG_ERROR("data_check_notify_file_create, DATA_CHECKER :0x%x, success, but filesize is 0 .", ptr_data_checker);
    }

    if(ptr_data_checker->_block_size != 0)
    {
            LOG_DEBUG("data_check_notify_file_create, DATA_CHECKER :0x%x, success, set blocksize:%u .", ptr_data_checker, ptr_data_checker->_block_size);
            fm_set_blocksize(file_struct, ptr_data_checker->_block_size);
    }
    else
    {
           LOG_ERROR("data_check_notify_file_create, DATA_CHECKER :0x%x, success, but blocksize is 0 .", ptr_data_checker);
    }*/

//  data_manager_notify_file_create_info(ptr_data_checker->_p_data_manager, TRUE);
    file_info_notify_create(ptr_file_info, SUCCESS);
    file_info_save_to_cfg_file(ptr_file_info);

    if(ptr_file_info->_file_size != 0 && ptr_file_info->_block_size != 0)
    {
        LOG_DEBUG("file_info_notify_file_create, file info :0x%x, success, set filesize:%llu , set block_size:%u,  .", ptr_file_info, ptr_file_info->_file_size, ptr_file_info->_block_size);
        fm_init_file_info( file_struct, ptr_file_info->_file_size, ptr_file_info->_block_size);
    }
    else
    {
        LOG_ERROR("file_info_notify_file_create, file info :0x%x, success, but filesize or blocksize is invalid,  filesize:%llu ,  block_size:%u,  .", ptr_file_info, ptr_file_info->_file_size, ptr_file_info->_block_size);
    }


    if(ptr_file_info->_finished)
    {
        LOG_DEBUG("The file is finished!");
        ret_val = SUCCESS;
    }
    else
        ret_val = file_info_flush_cached_buffer(ptr_file_info);
    return ret_val;
}


_int32 file_info_notify_file_close(struct tagTMP_FILE* file_struct, void* user_ptr, _int32 close_result)
{
    //_int32 ret_val = SUCCESS;
    FILEINFO*  ptr_file_info = (FILEINFO*) user_ptr;

    LOG_DEBUG("file_info_notify_file_close , file info :0x%x , file_struct : 0x%x , close result: %d.",  ptr_file_info, file_struct, close_result);

    ptr_file_info->_wait_for_close = FALSE;
    //    data_manager_notify_file_close_info(ptr_data_checker->_p_data_manager,  file_struct, close_result);
    file_info_notify_close(ptr_file_info, close_result);

    return SUCCESS;
}

_int32  file_info_change_finish_filename(FILEINFO* p_file_info, char* p_final_name)
{
    _int32 ret_val = SUCCESS;

    LOG_DEBUG("file_info_change_finish_filename .");

    if(p_final_name == NULL || p_file_info == NULL || p_file_info->_tmp_file_struct  == NULL)
    {
        LOG_ERROR("file_info_change_finish_filename failue because invalid parameter.");
        return ret_val;
    }

    LOG_DEBUG("file_info_change_finish_filename, final name:%s .", p_final_name);

    ret_val = fm_file_change_name(p_file_info->_tmp_file_struct,  p_final_name);

    return ret_val;

}


_int32 file_info_notify_fm_can_close(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid)
{
    FILEINFO* p_file_info = (FILEINFO*)msg_info->_device_id;

    LOG_DEBUG("file_info_notify_fm_can_close, file info : 0x%x  , msg id: %u  , errcode:%d.", p_file_info,  msgid, errcode);

    //if(errcode == MSG_CANCELLED)
    //  return SUCCESS;

    //     data_manager_notify_file_close_info(p_data_checker->_p_data_manager,NULL,  0);
    file_info_notify_close(p_file_info, SUCCESS);

    return SUCCESS;
}

_int32 file_info_notify_filesize_change(struct tagTMP_FILE* file_struct, void* user_ptr, _int32 create_result)
{
    _int32 ret_val = SUCCESS;
    FILEINFO*  ptr_file_info = (FILEINFO*) user_ptr;


    if(create_result == FM_CREAT_CLOSE_EVENT)
    {
        LOG_ERROR("file_info_notify_filesize_change, file info :0x%x, because file is closed .", ptr_file_info);
        return SUCCESS;
    }

    if(create_result != SUCCESS)
    {
        LOG_ERROR("file_info_notify_filesize_change, file info :0x%x, failure .", ptr_file_info);

        handle_create_failure(ptr_file_info, create_result);

//      data_manager_notify_file_create_info(ptr_data_checker->_p_data_manager, FALSE);
        //file_info_notify_create(ptr_file_info, create_result);
        return SUCCESS;
    }

    LOG_DEBUG("file_info_notify_filesize_change, file info :0x%x, success .", ptr_file_info);

    /*if(ptr_data_checker->_file_size != 0)
    {
           LOG_DEBUG("data_check_notify_file_create, DATA_CHECKER :0x%x, success, set filesize:%llu .", ptr_data_checker, ptr_data_checker->_file_size);
        fm_set_filesize( file_struct, ptr_data_checker->_file_size);
    }
    else
    {
          LOG_ERROR("data_check_notify_file_create, DATA_CHECKER :0x%x, success, but filesize is 0 .", ptr_data_checker);
    }

    if(ptr_data_checker->_block_size != 0)
    {
            LOG_DEBUG("data_check_notify_file_create, DATA_CHECKER :0x%x, success, set blocksize:%u .", ptr_data_checker, ptr_data_checker->_block_size);
            fm_set_blocksize(file_struct, ptr_data_checker->_block_size);
    }
    else
    {
           LOG_ERROR("data_check_notify_file_create, DATA_CHECKER :0x%x, success, but blocksize is 0 .", ptr_data_checker);
    }*/

    //    data_manager_notify_file_create_info(ptr_file_info->_p_data_manager, TRUE);

    file_info_notify_create(ptr_file_info, SUCCESS);
    file_info_save_to_cfg_file(ptr_file_info);

    if(ptr_file_info->_file_size != 0 && ptr_file_info->_block_size != 0)
    {
        LOG_DEBUG("file_info_notify_filesize_change, file info :0x%x, success, set filesize:%llu , set block_size:%u,  .", ptr_file_info, ptr_file_info->_file_size, ptr_file_info->_block_size);
        fm_init_file_info( file_struct, ptr_file_info->_file_size, ptr_file_info->_block_size);
    }
    else
    {
        LOG_ERROR("file_info_notify_filesize_change, file info :0x%x, success, but filesize or blocksize is invalid,  filesize:%llu ,  block_size:%u,  .", ptr_file_info, ptr_file_info->_file_size, ptr_file_info->_block_size);
    }

    ret_val = file_info_flush_cached_buffer(ptr_file_info);
    return ret_val;
}


BOOL file_info_ia_all_received(FILEINFO* p_file_info)
{
    if(file_info_filesize_is_valid(p_file_info) == TRUE)
    {
        LOG_DEBUG("file_info_ia_all_received, but filesize is valid .");
        if(p_file_info->_is_need_calc_bcid == FALSE)
        {
            return  (range_list_length_is_enough(&p_file_info->_data_receiver._total_receive_range,file_info_get_filesize(p_file_info))
                 && range_list_length_is_enough(&p_file_info->_writed_range_list,file_info_get_filesize(p_file_info))
                );        
        }
        else
        {
            return  (range_list_length_is_enough(&p_file_info->_done_ranges,file_info_get_filesize(p_file_info))
                 && range_list_length_is_enough(&p_file_info->_data_receiver._total_receive_range,file_info_get_filesize(p_file_info))
                 && range_list_length_is_enough(&p_file_info->_writed_range_list,file_info_get_filesize(p_file_info))
                );
         }
    }
    else
    {
        LOG_DEBUG("file_info_ia_all_received, but filesize is invalid .");
        return FALSE;
    }
}


BOOL file_info_range_is_recv(FILEINFO* p_file_info, const RANGE* r)
{
    LOG_DEBUG("file_info_range_is_recv .");

    return  data_receiver_range_is_recv(&p_file_info->_data_receiver,r);
}

BOOL file_info_range_is_write(FILEINFO* p_file_info, const RANGE* r)
{
    LOG_DEBUG("data_checker_range_is_write,  range (%u, %u) .",r->_index, r->_num);

    return range_list_is_include(&p_file_info->_writed_range_list, r);
}

BOOL file_info_range_is_check(FILEINFO* p_file_info, const RANGE* r)
{
    LOG_DEBUG("file_info_range_is_check .");
    return  range_list_is_include(&p_file_info->_done_ranges,r);
}

BOOL file_info_range_is_cached(FILEINFO* p_file_info, const RANGE* r)
{
    LOG_DEBUG("file_info_range_is_cached .");
    return  range_list_is_include(&p_file_info->_data_receiver._cur_cache_range, r);
}

BOOL file_info_rangelist_is_write(FILEINFO* p_file_info, RANGE_LIST* r_list)
{
    return range_list_is_contained(&p_file_info->_writed_range_list, r_list);
}



RANGE_LIST* file_info_get_recved_range_list(FILEINFO* p_file_info)
{

    if(p_file_info == NULL)
        return NULL;

    return  &p_file_info->_data_receiver._total_receive_range;
}

RANGE_LIST* file_info_get_writed_range_list(FILEINFO* p_file_info)
{

    if(p_file_info == NULL)
        return NULL;

    return &p_file_info->_writed_range_list;
}

RANGE_LIST* file_info_get_checked_range_list(FILEINFO* p_file_info)
{

    if(p_file_info == NULL)
        return NULL;

    return &p_file_info->_done_ranges;
}


_int32 file_info_get_range_data_buffer( FILEINFO* p_file_info, RANGE need_r,  RANGE_DATA_BUFFER_LIST* ret_range_buffer)
{
    LOG_DEBUG("file_info_get_range_data_buffer ... need_range(%d,%d)", need_r._index, need_r._num);

    if(p_file_info == NULL || ret_range_buffer == NULL)
        return -1;

    if(need_r._num == 0)
    {
        return -1;
    }

    LOG_DEBUG("file_info_get_range_data_buffer _data_receiver._cur_cache_range is");
    out_put_range_list(&(p_file_info->_data_receiver._cur_cache_range));
    if( range_list_is_relevant(&p_file_info->_data_receiver._cur_cache_range,  &need_r) == TRUE)
    {
        data_receiver_get_releate_data_buffer(&p_file_info->_data_receiver, &need_r, ret_range_buffer);
        if(list_size(&ret_range_buffer ->_data_list) == 0)
        {
            return -1;
        }
        else
        {
            return SUCCESS;
        }

    }
    else
    {
        LOG_DEBUG("file_info_get_range_data_buffer ... need_range(%d,%d)  range_list_is_relevant not in _cur_cache_range", need_r._index, need_r._num);
        return -1;
    }

}


#ifdef _VOD_NO_DISK

_int32 file_info_vod_set_no_disk_mode( FILEINFO* p_file_info)
{
    p_file_info->_is_no_disk = TRUE;
    return SUCCESS;
}

_int32 file_info_vod_no_disk_notify_free_data_buffer( FILEINFO* p_file_info,   RANGE_DATA_BUFFER_LIST* del_range_buffer)
{

    RANGE_LIST  del_range_list;
    RANGE_LIST_ITEROATOR  del_it = NULL;

    sd_assert(p_file_info->_is_no_disk == TRUE);

    range_list_init(&del_range_list);

    if(del_range_buffer == NULL)
        return SUCCESS;

    get_range_list_from_buffer_list(del_range_buffer, &del_range_list);

    if(del_range_list._node_size == 0)
        return SUCCESS;

    range_list_get_head_node(&del_range_list, &del_it);

    while(del_it != NULL)
    {
        data_receiver_del_buffer(&p_file_info->_data_receiver, &del_it->_range);

        range_list_get_next_node(&del_range_list, del_it, &del_it);
    }

    range_list_clear(&del_range_list);

    return SUCCESS;


}
#endif


_int32  file_info_asyn_read_data_buffer( FILEINFO* p_file_info, RANGE_DATA_BUFFER *p_data_buffer, notify_read_result p_call_back, void *p_user )
{

    _int32 ret_val = 0;

#ifdef _VOD_NO_DISK
    if(p_file_info->_is_no_disk == TRUE)
        return -1;
#endif

    if(p_file_info == NULL || p_data_buffer == NULL || p_user == NULL ||p_file_info->_tmp_file_struct == NULL)
    {
        LOG_DEBUG( "file_info_asyn_read_data_buffer  invalid parameter." );
        return -1;
    }

    LOG_DEBUG( "file_info_asyn_read_data_buffer  , p_file_struct:0x%x, p_data_buffer: 0x%x,  p_user:0x%x .", p_file_info->_tmp_file_struct, p_data_buffer, p_user);

    if(file_info_range_is_write(p_file_info, &p_data_buffer->_range) ==  FALSE)
    {
        LOG_DEBUG( "file_info_asyn_read_data_buffer  read range has not writed, read range:(%u,%u) .", p_data_buffer->_range._index, p_data_buffer->_range._num);
        return -1;
    }

    ret_val = fm_file_asyn_read_buffer( p_file_info->_tmp_file_struct, p_data_buffer, p_call_back, p_user);

    return ret_val;

}

_int32 file_info_read_data(FILEINFO* file_info, RANGE_DATA_BUFFER* data_buffer, notify_read_result callback, void* user_data)
{
    _int32 ret = SUCCESS;
    RANGE_DATA_BUFFER_LIST data_buffer_list;
    LIST_ITERATOR  cur_node = NULL;
    RANGE_DATA_BUFFER* cur_data_buffer = NULL;
    if(file_info == NULL || data_buffer == NULL || callback == NULL
       || file_info->_tmp_file_struct == NULL || user_data == NULL|| file_info->_tmp_file_struct->_device_id== 0 || file_info->_tmp_file_struct->_device_id== INVALID_FILE_ID)
        return -1;
    if(file_info_range_is_recv(file_info, &data_buffer->_range) == FALSE)
        return -1;
    if(file_info_range_is_cached(file_info, &data_buffer->_range) == TRUE)
    {
        buffer_list_init(&data_buffer_list);
        file_info_get_range_data_buffer(file_info, data_buffer->_range, &data_buffer_list);
       
         cur_node = LIST_BEGIN(data_buffer_list._data_list);
    	  while(cur_node != LIST_END(data_buffer_list._data_list))
         {
         	 cur_data_buffer = (RANGE_DATA_BUFFER*)LIST_VALUE(cur_node);

		 sd_assert(cur_data_buffer->_range._num > 0);

      		_u32 cur_range_index = cur_data_buffer->_range._index;
      		_u32 end_range_index = cur_data_buffer->_range._index + cur_data_buffer->_range._num -  1;
		for(; cur_range_index <= end_range_index; cur_range_index++)
		{
			if(cur_range_index < data_buffer->_range._index)
			{
				continue;
			}
			if(cur_range_index >= (data_buffer->_range._index + data_buffer->_range._num))
			{
				continue;
			}

			_u32 read_size = get_data_unit_size();
			if(cur_range_index == end_range_index && (data_buffer->_data_length % read_size) != 0 )
			{
				read_size = (data_buffer->_data_length % read_size);
			}
			
        		_u32 outoffset = (cur_range_index - data_buffer->_range._index) * get_data_unit_size();
        		_u32 inoffset =  (cur_range_index - cur_data_buffer->_range._index) * get_data_unit_size();
            		sd_memcpy(data_buffer->_data_ptr + outoffset, cur_data_buffer->_data_ptr + inoffset, read_size);

            		LOG_DEBUG("file_info_read_data outoffset:%u, inoffset:%u, read_size:%u, cur_range_index:%u, end_range_index:%u", outoffset, inoffset, read_size,
            		cur_range_index, end_range_index);
        	}

        	cur_node = LIST_NEXT(cur_node);
    	 }

        list_clear( &(data_buffer_list._data_list));

        return SUCCESS;
    }
    else if(file_info_range_is_write(file_info, &data_buffer->_range) == FALSE)
    {
        return ERROR_DATA_IS_WRITTING;
    }

    ret = file_info_asyn_read_data_buffer(file_info, data_buffer, callback, user_data);
    CHECK_VALUE(ret);
    return ERROR_WAIT_NOTIFY;
}

_int32 file_info_put_data(FILEINFO* p_file_info 
    , const RANGE *r
    , char **data_ptr
    , _u32 data_len
    ,_u32 buffer_len)
{
    //_int32 ret_val = SUCCESS;

    // char* data_buffer = *data_ptr;
    RANGE data_r  = *r;
    _u32  data_length =  data_len;
    //_u32 cur_index = 0;
    //     _u32 block_no  = 0;
//  RANGE block_range;

    LOG_DEBUG("file_info_put_data , the range is (%u,%u), data_len is %u, buffer_length:%u, buffer:0x%x .",
              data_r._index, data_r._num, data_length, buffer_len, *data_ptr);

    if(p_file_info->_file_size != 0 )
    {
        if(((_u64)(data_r._index))*(_u64)get_data_unit_size() + data_length  > p_file_info->_file_size)
        {
            data_length =  p_file_info->_file_size - ((_u64)(data_r._index))*(_u64)get_data_unit_size();
            data_r._num = (data_length + (get_data_unit_size() - 1))/get_data_unit_size();
        }
    }
	sd_assert( ( (_int32)data_length) > 0);
    if(data_r._num*get_data_unit_size()  !=  data_length)
    {

        LOG_DEBUG("file_info_put_data data_length:%u is not full range.", data_length);

        if(p_file_info->_file_size != 0)
        {
            if(((_u64)(data_r._index))*get_data_unit_size() + data_length  < p_file_info->_file_size)
            {
                data_r._num = data_length/get_data_unit_size();
                data_length =  data_r._num*get_data_unit_size();
            }
            LOG_DEBUG("file_info_put_data data_length:, after process, the range is (%u,%u), data_len is %u.", data_r._index, data_r._num, data_length);

            if(data_length == 0 || data_r._index == MAX_U32 || data_r._index*get_data_unit_size() + data_length > p_file_info->_file_size)
            {
                LOG_ERROR("file_info_put_data data_length:, after process, the range is (%u,%u), data_len is %u, because data_len=0, so drop buffer.", data_r._index, data_r._num, data_length);
                //dm_free_data_buffer(p_data_manager_interface ,  data_ptr, buffer_len);
                return FIL_INFO_INVALID_DATA;
            }
        }
        else
        {
            LOG_DEBUG("file_info_put_data data_length:%u is not full range, filesize is invalid.", data_length);
        }
    }

	sd_assert( ( (_int32)data_length) > 0);

    if(file_info_range_is_recv(p_file_info, &data_r) == TRUE)
    {
        LOG_ERROR("file_info_put_data data_length:, the range is (%u,%u), data_len is %u, buffer_length:%u, buffer:0x%x  is recved, so drop it.",
                  data_r._index, data_r._num, data_length, buffer_len, *data_ptr);
        // dm_free_buffer_to_data_buffer(buffer_len ,  data_ptr);
        return FIL_INFO_RECVED_DATA;
    }


    /*    is it the time to flush the data to file*/


    //   ret_val = file_info_flush_data_to_file(p_file_info, FALSE);

    return file_info_put_safe_data(p_file_info, &data_r, data_ptr, data_length, buffer_len);

    //return SUCCESS;

}


_int32 file_info_put_safe_data(FILEINFO* p_file_info 
    , const RANGE *r
    , char **data_ptr
    , _u32 data_len
    , _u32 buffer_len)
{
    _int32 ret_val = SUCCESS;

    char* data_buffer = *data_ptr;
    RANGE data_r  = *r;
    _u32  data_length =  data_len;
    _u32 cur_index = 0;
    _u32 block_no  = 0;
    RANGE block_range;
#ifdef CHECK_FAIL_TEST
    static _int32 first_check = 1;
    _int32 i;
#endif

#ifdef _VOD_NO_DISK
    RANGE_DATA_BUFFER_LIST cur_data_list;
    RANGE_DATA_BUFFER_LIST cache_data_list;
#endif

    LOG_DEBUG("file_info_put_safe_data , the range is (%u,%u), data_len is %u, buffer_length:%u, buffer:0x%x .",
              data_r._index, data_r._num, data_length, buffer_len, data_buffer);

    ret_val = data_receiver_add_buffer(&p_file_info->_data_receiver
            , &data_r
            , data_buffer
            , data_length
            , buffer_len);

    if(ret_val == SUCCESS)
    {
        *data_ptr =  NULL;
    }
    else
    {
        LOG_DEBUG("file_info_put_safe_data , data_receiver_add_buffer fail, ret = %d.", ret_val);
        dm_free_buffer_to_data_buffer(buffer_len, (_int8**)data_ptr);
        *data_ptr =  NULL;
        return ret_val;
    }
    
    add_speed_record(&(p_file_info->_cur_data_speed), data_length);

    if(file_info_file_is_create(p_file_info) == FALSE)
    {
        LOG_DEBUG("file_info_put_safe_data , try to create file.");


        file_info_try_create_file(p_file_info);
        //dm_save_to_cfg_file(p_data_manager_interface);
    }

    /*if the whole block is now in cache buffer, check the data*/
    if(file_info_filesize_is_valid(p_file_info) == TRUE)
    {
        cur_index = data_r._index;

        block_no  = cur_index/((p_file_info->_block_size)/get_data_unit_size());

        block_range =  to_range(block_no, p_file_info->_block_size, p_file_info->_file_size);

#ifndef _VOD_NO_DISK
        if( range_list_is_include(&p_file_info->_data_receiver._cur_cache_range,  &block_range) == TRUE
            && !bitmap_is_set(p_file_info->_bcid_info._bcid_checking_bitmap, block_no) 
            && chh_can_put_item(p_file_info->_hash_calculator) )
        {

            LOG_DEBUG("file_info_put_safe_data , block no: %u, range (%u,%u) is success received.", block_no, block_range._index,block_range._num);


            if(p_file_info->_no_need_check_range._node_size == 0
               || (range_list_is_include(&p_file_info->_no_need_check_range, &block_range) == FALSE))
            {
                ret_val = check_block(p_file_info, block_no, &p_file_info->_data_receiver._buffer_list, TRUE);
            }
            else
            {
                LOG_DEBUG("file_info_put_safe_data , block no: %u, range (%u,%u) is success received, but no need check .", block_no, block_range._index,block_range._num);
                out_put_range_list(&p_file_info->_no_need_check_range);
            }
        }
#else
        if(p_file_info->_is_no_disk == FALSE)
        {
            if( range_list_is_include(&p_file_info->_data_receiver._cur_cache_range,  &block_range) == TRUE)
            {

                LOG_DEBUG("file_info_put_safe_data , block no: %u, range (%u,%u) is success received.", block_no, block_range._index,block_range._num);

                if(p_file_info->_no_need_check_range._node_size == 0
                   || (range_list_is_include(&p_file_info->_no_need_check_range, &block_range) == FALSE))
                {
                    ret_val =  check_block(p_file_info, block_no, &p_file_info->_data_receiver._buffer_list, TRUE);
                }
                else
                {
                    LOG_DEBUG("file_info_put_safe_data , block no: %u, range (%u,%u) is success received, but no need check .", block_no, block_range._index,block_range._num);
                    out_put_range_list(&p_file_info->_no_need_check_range);
                }
            }
        }
        else
        {
            if( range_list_is_include(&p_file_info->_data_receiver._total_receive_range,  &block_range) == TRUE)
            {
                if( range_list_is_include(&p_file_info->_data_receiver._cur_cache_range,  &block_range) == TRUE)
                {

                    LOG_DEBUG("file_info_put_safe_data , block no: %u, range (%u,%u) is success received.", block_no, block_range._index,block_range._num);

                    if(p_file_info->_no_need_check_range._node_size == 0
                       || (range_list_is_include(&p_file_info->_no_need_check_range, &block_range) == FALSE))
                    {
                        ret_val =  check_block(p_file_info, block_no, &p_file_info->_data_receiver._buffer_list,TRUE);

                        
                    }
                    else
                    {
                        LOG_DEBUG("file_info_put_safe_data , block no: %u, range (%u,%u) is success received, but no need check .", block_no, block_range._index,block_range._num);
                        out_put_range_list(&p_file_info->_no_need_check_range);
                    }
                }
                else
                {
                    LOG_DEBUG("file_info_put_safe_data ,2 ,  block no: %u, range (%u,%u) is success received .", block_no, block_range._index,block_range._num);

                    if(p_file_info->_no_need_check_range._node_size == 0
                       || (range_list_is_include(&p_file_info->_no_need_check_range, &block_range) == FALSE))
                    {
                        LOG_DEBUG("file_info_put_safe_data ,4-1");
                        list_init(&cur_data_list._data_list);
                        list_init(&cache_data_list._data_list);

                        if(p_file_info->_call_back_func_table._p_get_buffer_list_from_cache != NULL)
                        {
                            get_releate_data_buffer_from_range_data_buffer_by_range(&p_file_info->_data_receiver._buffer_list, &block_range,&cur_data_list);
                            LOG_DEBUG("file_info_put_safe_data ,4-2");

                            p_file_info->_call_back_func_table._p_get_buffer_list_from_cache(p_file_info->_p_user_ptr,  &block_range,&cache_data_list);

                            buffer_list_splice(&cur_data_list,&cache_data_list);

                            ret_val =  check_block(p_file_info, block_no, &cur_data_list, FALSE);

                            
                            //need free cache_data_list
                            drop_buffer_list_without_buffer(&cache_data_list);
                        }

                        list_clear(&cur_data_list._data_list);
                        list_clear(&cache_data_list._data_list);
                    }
                    else
                    {
                        LOG_DEBUG("file_info_put_safe_data ,2 , block no: %u, range (%u,%u) is success received, but no need check .", block_no, block_range._index,block_range._num);
                        out_put_range_list(&p_file_info->_no_need_check_range);
                    }

                }
            }

        }
#endif

    }
    /*    is it the time to flush the data to file*/



#ifndef _VOD_NO_DISK

    ret_val = file_info_flush_data_to_file(p_file_info, FALSE);
#else

    if(p_file_info->_is_no_disk == TRUE)
        return SUCCESS;
    ret_val = file_info_flush_data_to_file(p_file_info, FALSE);
#endif

    return SUCCESS;

}

RANGE_DATA_BUFFER_LIST* file_info_get_cache_range_buffer_list(FILEINFO* p_file_info)
{
    if(p_file_info == NULL)
        return NULL;

    return     &(p_file_info->_data_receiver._buffer_list);
}

_int32  file_info_flush_data_to_file(FILEINFO* p_file_info, BOOL force)
{
    _u32 ret_val = SUCCESS;
    _u32 buffer_size = 0;

    RANGE_DATA_BUFFER_LIST tmp_buffer_list;

    DATA_RECEIVER* data_receive =   &p_file_info->_data_receiver;

    _u64 total_nuit_num = range_list_get_total_num(&data_receive->_total_receive_range);

    list_init(&tmp_buffer_list._data_list);

    LOG_DEBUG("file_info_flush_data_to_file .");

#ifdef _VOD_NO_DISK
    if(p_file_info->_is_no_disk == TRUE)
        return SUCCESS;
#endif

    if(list_size(&p_file_info->_flush_buffer_list._data_list) > 0)
    {
        LOG_DEBUG("file_info_flush_data_to_file, because has data before so ttmp store,  store size:%u .", list_size(&p_file_info->_flush_buffer_list._data_list));
        list_swap(&tmp_buffer_list._data_list, &p_file_info->_flush_buffer_list._data_list);
    }

    if( force == TRUE)
    {
        /*force flush data*/
        LOG_DEBUG("file_info_flush_data_to_file, force to flush data to file .");

        file_info_get_flush_data(p_file_info, &p_file_info->_flush_buffer_list, TRUE);
    }
    else if(p_file_info->_file_size != 0
            &&(total_nuit_num* get_data_unit_size() >= p_file_info->_file_size ))
    {

        {
            /*force flush data*/
            LOG_DEBUG("file_info_flush_data_to_file,all data is recved, so  force to flush data to file .");
            file_info_get_flush_data(p_file_info, &p_file_info->_flush_buffer_list, TRUE);
        }
    }
    else
    {
        /*  flush data*/
        LOG_DEBUG("file_info_flush_data_to_file,try flush data , downed num %llu." , total_nuit_num);
        file_info_get_flush_data(p_file_info, &p_file_info->_flush_buffer_list, FALSE);
    }

    if(list_size(&tmp_buffer_list._data_list) > 0)
    {
        LOG_DEBUG("file_info_flush_data_to_file, because has  tmp store, so return data,  store size:%u .", list_size(&tmp_buffer_list._data_list));

        buffer_list_splice(&p_file_info->_flush_buffer_list, &tmp_buffer_list);
    }

    list_clear(&tmp_buffer_list._data_list);

    buffer_size = list_size(&(p_file_info->_flush_buffer_list._data_list));

    if(buffer_size  == 0)
    {
        LOG_DEBUG("file_info_flush_data_to_file,no data to  flush.");

        return SUCCESS;
    }

    ret_val =  file_info_flush_data(p_file_info, &p_file_info->_flush_buffer_list);

    if(FILE_CREATING == ret_val)
    {
        LOG_DEBUG("file_info_flush_data_to_file,file is creating, can not flush data, cache flush data size is %u.", list_size(&p_file_info->_flush_buffer_list._data_list));
        return SUCCESS;
    }

    return ret_val;

}

_int32 file_info_get_flush_data(FILEINFO* p_file_info, RANGE_DATA_BUFFER_LIST* need_flush_data, BOOL force_flush)
{
    _u32 block_no  = 0;
    _u32 unit_num = 0;
    RANGE block_range;

    _u32 block_index = 0;
    BOOL  is_full_down = FALSE;
    LIST_ITERATOR  cur_node = NULL;
    RANGE_DATA_BUFFER* cur_buffer = NULL;
    _u32 cur_index = 0;
    LIST_ITERATOR erase_node  = NULL;
    _u32 unit_num2 = 0;

    RANGE_DATA_BUFFER_LIST tmp_buffer_list;

    DATA_RECEIVER* data_receive =  &p_file_info->_data_receiver;

    unit_num = range_list_get_total_num(&data_receive->_cur_cache_range);

    LOG_DEBUG("file_info_get_flush_data, now unit_num = %d .", unit_num);

    if(force_flush == TRUE)
    {
        LOG_DEBUG("file_info_get_flush_data,force to flush data .");

        data_receiver_cp_buffer(data_receive, need_flush_data);
        return SUCCESS;
    }

    if(file_info_filesize_is_valid(p_file_info) == FALSE)
    {
        LOG_DEBUG("file_info_get_flush_data, unit_num:%u is enough, but filesize is invalid so all flush to file.", unit_num);
        data_receiver_cp_buffer(data_receive, need_flush_data);
        return SUCCESS;
    }

    if(unit_num < get_max_flush_unit_num())
    {
        LOG_DEBUG("file_info_get_flush_data, unit_num:%u is too small %d, so not flush. ", unit_num, get_max_flush_unit_num());
        return SUCCESS;
    }


    /*1  cached full block need to flush  and check bcid
       2  received full block need flush
       3  after 1 and 2 flush, if  data unit block num  is large half of MAX_FLUSH_UNIT_NUM, the buffer mush flash*/

    block_range._index = 0;
    block_range._num = 0;

    cur_node = LIST_BEGIN(data_receive->_buffer_list._data_list);
    while(cur_node != LIST_END(data_receive->_buffer_list._data_list))
    {
        cur_buffer = (RANGE_DATA_BUFFER*)LIST_VALUE(cur_node);

        cur_index =  cur_buffer->_range._index;

        if(block_range._num == 0 || cur_index<block_range._index || (cur_index)>= (block_range._index+block_range._num))
        {
            block_no  = cur_index/(p_file_info->_block_size/get_data_unit_size());

            block_range =  to_range(block_no, p_file_info->_block_size, p_file_info->_file_size);

            block_index = block_range._index;

            if( range_list_is_include(&data_receive->_total_receive_range,  &block_range) == TRUE)
            {
                LOG_DEBUG("file_info_get_flush_data, block_no:%u , range(%u,%u) is recved..", block_no, block_range._index, block_range._num);
                is_full_down = TRUE;
            }
            else
            {
                is_full_down = FALSE;
            }
        }

        if(is_full_down == TRUE)
        {
            list_push(&need_flush_data->_data_list, cur_buffer);

            erase_node  =  cur_node;
            cur_node= LIST_NEXT(cur_node);

            list_erase(&data_receive->_buffer_list._data_list, erase_node);
        }
        else
        {
            cur_node= LIST_NEXT(cur_node);
        }
    }

    range_list_erase_buffer_range_list(&data_receive->_cur_cache_range, need_flush_data);

    unit_num2 = range_list_get_total_num(&data_receive->_cur_cache_range);

    LOG_DEBUG("file_info_get_flush_data, after cp data first, unit_num:%u ,  unit_num2:%u .", unit_num, unit_num2);

    if(unit_num2*2 > unit_num)
    {
        LOG_DEBUG("file_info_get_flush_data, unit_num:%u ,  unit_num2:%u, because unit_num_2 is too large so flush all data .", unit_num, unit_num2);

        list_init(&tmp_buffer_list._data_list);

        LOG_DEBUG("file_info_get_flush_data, before swap tmp_buffer_list size: %u, need_flush_data size :%u .", list_size(&tmp_buffer_list._data_list), list_size(&need_flush_data->_data_list));

        list_swap(&tmp_buffer_list._data_list, &need_flush_data->_data_list);

        LOG_DEBUG("file_info_get_flush_data, after swap tmp_buffer_list size: %u, need_flush_data size :%u .", list_size(&tmp_buffer_list._data_list), list_size(&need_flush_data->_data_list));

        data_receiver_cp_buffer(data_receive, need_flush_data);

        LOG_DEBUG("file_info_get_flush_data, after cp buffer tmp_buffer_list size: %u, need_flush_data size :%u .", list_size(&tmp_buffer_list._data_list), list_size(&need_flush_data->_data_list));

        buffer_list_splice(need_flush_data, &tmp_buffer_list);

        LOG_DEBUG("file_info_get_flush_data, after splice buffer tmp_buffer_list size: %u, need_flush_data size :%u .", list_size(&tmp_buffer_list._data_list), list_size(&need_flush_data->_data_list));

        list_clear(&tmp_buffer_list._data_list);

        LOG_DEBUG("file_info_get_flush_data, after clear tmp list buffer tmp_buffer_list size: %u, need_flush_data size :%u .", list_size(&tmp_buffer_list._data_list), list_size(&need_flush_data->_data_list));

        return SUCCESS;
    }

    return SUCCESS;

}


_int32 file_info_change_td_name( FILEINFO* p_file_info )
{
    _int32 ret_val = SUCCESS;
    _int32 path_len = 0;
    _int32 file_name_len = 0;

    char* td_str = ".rf";

    char new_full_name[MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN];
    char old_full_name[MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN];
	int j = 0;
	char szOriName[MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN] = {0};
	int nLenOri = 0;
	int nDotOffset = 0;

    path_len = sd_strlen( p_file_info->_path);

    file_name_len = sd_strlen( p_file_info->_file_name);

    ret_val = sd_memcpy( new_full_name, p_file_info->_path, path_len);
    CHECK_VALUE( ret_val );

    ret_val = sd_memcpy( old_full_name, p_file_info->_path, path_len);
    CHECK_VALUE( ret_val );

    ret_val = sd_memcpy( new_full_name + path_len , p_file_info->_file_name, file_name_len );
    CHECK_VALUE( ret_val );

    ret_val = sd_memcpy( old_full_name + path_len , p_file_info->_file_name, file_name_len );
    CHECK_VALUE( ret_val );


    if(p_file_info->_has_postfix == TRUE)
    {
        new_full_name[file_name_len+path_len -3 ] = '\0';
        old_full_name[file_name_len+path_len] = '\0';
    }
    else
    {
        ret_val = sd_memcpy( new_full_name + path_len + file_name_len , td_str, 3 );
        CHECK_VALUE( ret_val );

        new_full_name[file_name_len+path_len +3 ] = '\0';
        old_full_name[file_name_len+path_len] = '\0';
    }

    LOG_ERROR( "file_info_change_td_name. old_full_name:%s ,new_full_file name:%s.", old_full_name,  new_full_name);

	if(sd_file_exist(new_full_name))
	{
		int i = strlen(szOriName);
		strcpy(szOriName, new_full_name);
		while(i > 0)
		{
			if(szOriName[i] == '.')
			{
				nDotOffset = i;
				break;
			}
			i--;
		}

		while (sd_file_exist(new_full_name))
	    {
			sd_memset(new_full_name, 0, MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN);
			strcpy(new_full_name, szOriName);
			if(j < 10)
			{
				strcpy(new_full_name + nDotOffset + 2, szOriName + nDotOffset);
				new_full_name[nDotOffset] = '_';
				new_full_name[nDotOffset + 1] = j + '0';
			}
			else if(j >= 10 && j < 100)
			{
				strcpy(new_full_name + nDotOffset + 3, szOriName + nDotOffset);
				new_full_name[nDotOffset] = '_';
				new_full_name[nDotOffset + 1] = j / 10 + '0';
				new_full_name[nDotOffset + 2] = j % 10 + '0';
			}
			else if(j >= 100 && j < 1000)
			{
				int nTemp = j;
				strcpy(new_full_name + nDotOffset + 4, szOriName + nDotOffset);
				new_full_name[nDotOffset] = '_';
				new_full_name[nDotOffset + 1] = nTemp / 100 + '0';
				nTemp = nTemp / 10;
				new_full_name[nDotOffset + 2] = nTemp / 10 + '0';
				new_full_name[nDotOffset + 3] = nTemp % 10 + '0';
			}

			j++;

	    }
	}

	 
    


    ret_val = sd_rename_file(old_full_name, new_full_name);
    LOG_ERROR( "file_info_change_td_name. ret_val=%d.", ret_val);
    /* ÔøΩÔøΩÔøΩÔøΩetmÔøΩÔøΩdestroy taskÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩ…æÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩƒºÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩ–µÔøΩœµÕ≥(ÔøΩÔøΩios)ÔøΩÔøΩcloseÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩ…æÔøΩÔøΩÔøΩÔøΩÔøΩ÷ÆÔøΩÛ∑µªÿ£ÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩ“≤ÔøΩÔøΩÔøΩold_full_nameÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔ≤ªÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩ?-- by zyq 20120416 */
    //CHECK_VALUE( ret_val );

    return SUCCESS;
}

BOOL file_info_save_to_cfg_file(FILEINFO* p_file_info)
{
    _u32 cfg_file_id = 0;
    _u32 count = 0;
    _u32 len = 0;
    _u32 verno = 0;
    _u32 write_len = 0;
    _u8 has_postfix = 0;
    _u32 block_size = 0;
    RANGE_LIST_ITEROATOR cur_node = NULL;
    _u32 cid_size = CID_SIZE;
    BOOL cid_is_valid = FALSE;
    _int32 ret_val = SUCCESS;
    RANGE_LIST* write_range_list = NULL;
    char  buffer[1024] = {0};
    _u32 buffer_pos = 0, buffer_len = 1024;
    _u32 data_unit_size = get_data_unit_size();
    _u32 bcid_num = 0;
    _u32 bcid_map_num = 0;
    BOOL bcid_is_valid = FALSE;
#ifdef _LOGGER
    char test_buffer[1024] = {0};
#endif
    char cfg_file_path[MAX_FILE_NAME_LEN + MAX_FILE_PATH_LEN]= {0};
    char buf_int[8] = {0};
    char *p_int = NULL;
    _int32 len_int = 0;

    if( p_file_info->_bcid_is_valid || !p_file_info->_bcid_enable
        || p_file_info->_has_history_check_info )
    {
        bcid_is_valid = TRUE;
    }

    LOG_DEBUG("file_info_save_to_cfg_file, bcid_is_valid:%d.", bcid_is_valid);

    ret_val =  file_info_open_cfg_file(p_file_info,cfg_file_path);
    if(ret_val != SUCCESS)
    {
        LOG_DEBUG("file_info_save_to_cfg_file, open cfg file failure, ret_val=%d .", ret_val);
        return FALSE;
    }

    cfg_file_id =p_file_info->_cfg_file;

    sd_setfilepos(cfg_file_id, 0);

    /*version: 4Bytes*/
    verno = 7;
    write_len = 0;

    p_int = buf_int;
    len_int = sizeof(buf_int);
    sd_set_int32_to_lt(&p_int, &len_int, (_int32)verno);
    ret_val = sd_write_save_to_buffer(cfg_file_id, buffer, buffer_len,&buffer_pos, (char*)buf_int, sizeof(verno));
    if(ret_val != SUCCESS) goto ErrHandler;

    p_int = buf_int;
    len_int = sizeof(buf_int);
    sd_set_int32_to_lt(&p_int, &len_int, (_int32)p_file_info->_user_data_len);
    ret_val = sd_write_save_to_buffer(cfg_file_id, buffer, buffer_len,&buffer_pos, (char*)buf_int, sizeof(p_file_info->_user_data_len));
    if(ret_val != SUCCESS) goto ErrHandler;
    if (p_file_info->_user_data_len)
    {
        ret_val = sd_write_save_to_buffer(cfg_file_id, buffer, buffer_len,&buffer_pos, (char*)p_file_info->_user_data, p_file_info->_user_data_len);
        if(ret_val != SUCCESS) goto ErrHandler;
    }

    /*data_unit_size: 4Bytes*/
    p_int = buf_int;
    len_int = sizeof(buf_int);
    sd_set_int32_to_lt(&p_int, &len_int, (_int32)data_unit_size);
    ret_val = sd_write_save_to_buffer(cfg_file_id, buffer, buffer_len,&buffer_pos, (char*)buf_int, sizeof(data_unit_size));
    if(ret_val != SUCCESS) goto ErrHandler;

    /*data  file length: 8Bytes*/
    p_int = buf_int;
    len_int = sizeof(buf_int);
    sd_set_int64_to_lt(&p_int, &len_int, (_int64)p_file_info->_file_size);
    ret_val = sd_write_save_to_buffer(cfg_file_id, buffer, buffer_len,&buffer_pos, (char*)buf_int, sizeof(p_file_info->_file_size));
    if(ret_val != SUCCESS) goto ErrHandler;

    /*has postfix: 1Byte*/
    has_postfix = p_file_info->_has_postfix;
    ret_val = sd_write_save_to_buffer(cfg_file_id, buffer, buffer_len,&buffer_pos, (char*)&has_postfix, sizeof(has_postfix));
    if(ret_val != SUCCESS) goto ErrHandler;

	/*user file name: ≥§∂»£´ƒ⁄»›, ?Bytes*/
    len = sd_strlen(p_file_info->_file_name);
    p_int = buf_int;
    len_int = sizeof(buf_int);
    sd_set_int32_to_lt(&p_int, &len_int, (_int32)len);
    ret_val = sd_write_save_to_buffer(cfg_file_id, buffer, buffer_len,&buffer_pos, (char*)buf_int, sizeof(len));
    if(ret_val != SUCCESS) goto ErrHandler;
    ret_val = sd_write_save_to_buffer(cfg_file_id, buffer, buffer_len,&buffer_pos, p_file_info->_file_name, len);
    if(ret_val != SUCCESS) goto ErrHandler;

	/*origin url: ≥§∂»£´ƒ⁄»›, ?Bytes*/
    len = sd_strlen(p_file_info->_origin_url);
    p_int = buf_int;
    len_int = sizeof(buf_int);
    sd_set_int32_to_lt(&p_int, &len_int, (_int32)len);
    ret_val = sd_write_save_to_buffer(cfg_file_id, buffer, buffer_len,&buffer_pos, (char*)buf_int, sizeof(len));
    if(ret_val != SUCCESS) goto ErrHandler;
    ret_val = sd_write_save_to_buffer(cfg_file_id, buffer, buffer_len,&buffer_pos, p_file_info->_origin_url, len);
    if(ret_val != SUCCESS) goto ErrHandler;

   	/*origin ref url: ≥§∂»£´ƒ⁄»›, ?Bytes */
    len = sd_strlen(p_file_info->_origin_ref_url);
    p_int = buf_int;
    len_int = sizeof(buf_int);
    sd_set_int32_to_lt(&p_int, &len_int, (_int32)len);
    ret_val = sd_write_save_to_buffer(cfg_file_id, buffer, buffer_len,&buffer_pos, (char*)buf_int, sizeof(len));
    if(ret_val != SUCCESS) goto ErrHandler;
    ret_val = sd_write_save_to_buffer(cfg_file_id, buffer, buffer_len,&buffer_pos, p_file_info->_origin_ref_url, len);
    if(ret_val != SUCCESS) goto ErrHandler;

	/*cookie: ≥§∂»£´ƒ⁄»›, ?Bytes */
    len = sd_strlen(p_file_info->_cookie);
	p_int = buf_int;
    len_int = sizeof(buf_int);
	sd_set_int32_to_lt(&p_int, &len_int, (_int32)len);
    ret_val = sd_write_save_to_buffer(cfg_file_id, buffer, buffer_len,&buffer_pos, (char*)buf_int, sizeof(len));
    if(ret_val != SUCCESS) goto ErrHandler;
    ret_val = sd_write_save_to_buffer(cfg_file_id, buffer, buffer_len,&buffer_pos, p_file_info->_cookie, len);
    if(ret_val != SUCCESS) goto ErrHandler;
	
    /*block size: 4Bytes*/
    block_size = p_file_info->_block_size;

    p_int = buf_int;
    len_int = sizeof(buf_int);
    sd_set_int32_to_lt(&p_int, &len_int, (_int32)block_size);
    ret_val = sd_write_save_to_buffer(cfg_file_id, buffer, buffer_len,&buffer_pos, (char*)buf_int, sizeof(block_size));
    if(ret_val != SUCCESS) goto ErrHandler;

    /*block cid length: 4Bytes, skip*/
    cid_size = CID_SIZE;

    p_int = buf_int;
    len_int = sizeof(buf_int);
    sd_set_int32_to_lt(&p_int, &len_int, (_int32)cid_size);
    ret_val = sd_write_save_to_buffer(cfg_file_id, buffer, buffer_len,&buffer_pos, (char*)buf_int, sizeof(cid_size));
    if(ret_val != SUCCESS) goto ErrHandler;

    /*cid info,4Bytes+?*/
    cid_is_valid = p_file_info->_cid_is_valid;

    p_int = buf_int;
    len_int = sizeof(buf_int);
    sd_set_int32_to_lt(&p_int, &len_int, (_int32)cid_is_valid);
    ret_val = sd_write_save_to_buffer(cfg_file_id, buffer, buffer_len,&buffer_pos, (char*)buf_int, sizeof(cid_is_valid));
    if(ret_val != SUCCESS) goto ErrHandler;

    if(cid_is_valid == TRUE)
    {
        ret_val = sd_write_save_to_buffer(cfg_file_id, buffer, buffer_len,&buffer_pos, (char*)p_file_info->_cid,CID_SIZE);
        if(ret_val != SUCCESS) goto ErrHandler;
    }

    write_range_list = &p_file_info->_writed_range_list;
    count = write_range_list->_node_size;

    p_int = buf_int;
    len_int = sizeof(buf_int);
    sd_set_int32_to_lt(&p_int, &len_int, (_int32)count);
    ret_val = sd_write_save_to_buffer(cfg_file_id, buffer, buffer_len,&buffer_pos, (char*)buf_int, sizeof(count));
    if(ret_val != SUCCESS) goto ErrHandler;

    LOG_DEBUG("file_info_save_to_cfg_file,  write range count: %u .", count);

    cur_node = NULL;
    range_list_get_head_node(write_range_list, &cur_node);
    while(cur_node != NULL)
    {
        p_int = buf_int;
        len_int = sizeof(buf_int);
        sd_set_int32_to_lt(&p_int, &len_int, (_int32)cur_node->_range._index);
        ret_val = sd_write_save_to_buffer(cfg_file_id, buffer, buffer_len,&buffer_pos, (char*)buf_int, sizeof(cur_node->_range._index));
        if(ret_val != SUCCESS) goto ErrHandler;
        p_int = buf_int;
        len_int = sizeof(buf_int);
        sd_set_int32_to_lt(&p_int, &len_int, (_int32)cur_node->_range._num);
        ret_val = sd_write_save_to_buffer(cfg_file_id, buffer, buffer_len,&buffer_pos, (char*)buf_int, sizeof(cur_node->_range._num));
        if(ret_val != SUCCESS) goto ErrHandler;

        LOG_DEBUG("file_info_save_to_cfg_file,	write range:[%u, %u].",
            cur_node->_range._index, cur_node->_range._num);
        range_list_get_next_node(write_range_list, cur_node, &cur_node);
    }

    p_int = buf_int;
    len_int = sizeof(buf_int);
    sd_set_int32_to_lt(&p_int, &len_int, (_int32)bcid_is_valid);
    ret_val = sd_write_save_to_buffer(cfg_file_id, buffer, buffer_len,&buffer_pos, (char*)buf_int, sizeof(bcid_is_valid));
    if(ret_val != SUCCESS) goto ErrHandler;

    if(bcid_is_valid == TRUE)
    {
        bcid_num = p_file_info->_bcid_info._bcid_num;
        bcid_map_num = (bcid_num+8-1)/(8);

        p_int = buf_int;
        len_int = sizeof(buf_int);
        sd_set_int32_to_lt(&p_int, &len_int, (_int32)bcid_map_num);
        ret_val = sd_write_save_to_buffer(cfg_file_id, buffer, buffer_len,&buffer_pos, (char*)buf_int, sizeof(bcid_map_num));
        if(ret_val != SUCCESS) goto ErrHandler;

        ret_val = sd_write_save_to_buffer(cfg_file_id, buffer, buffer_len,&buffer_pos, (char*)p_file_info->_bcid_info._bcid_bitmap, bcid_map_num);
        if(ret_val != SUCCESS) goto ErrHandler;
#ifdef _LOGGER
        sd_memset(test_buffer,0,1024);
        str2hex( (char *)p_file_info->_bcid_info._bcid_bitmap, bcid_map_num, test_buffer, 1024);
        LOG_DEBUG( "file_info_save_to_cfg_file:*bcid_map_buffer[%u]=%s" ,(_u32)bcid_map_num, test_buffer);
#endif

    }

    if (verno >= 6)
    {
        p_int = buf_int;
        len_int = sizeof(buf_int);
        sd_set_int32_to_lt(&p_int, &len_int, (_int32)p_file_info->_write_mode);
        ret_val = sd_write_save_to_buffer(cfg_file_id, buffer, buffer_len,&buffer_pos, (char*)buf_int, sizeof(FILE_WRITE_MODE));
        if(ret_val != SUCCESS) goto ErrHandler;
    }

    if(buffer_pos!=0)
    {
        ret_val =sd_write(cfg_file_id,buffer, buffer_pos, &write_len);
        if(ret_val != SUCCESS) goto ErrHandler;
        sd_assert(buffer_pos==write_len);
        if(buffer_pos!=write_len)
        {
            ret_val = FM_RW_ERROR;
            goto ErrHandler;
        }
    }

    /*file manager info */
    if(p_file_info->_write_mode!=FWM_RANGE)
    {
        ret_val = fm_flush_cfg_block_index_array( p_file_info->_tmp_file_struct, cfg_file_id );
        if(ret_val != SUCCESS) goto ErrHandler;
    }
    return TRUE;

ErrHandler:

    LOG_DEBUG("file_info_save_to_cfg_file ERROR: ret_val=%d",ret_val);


    if(cfg_file_id != 0)
    {
        ret_val = sd_close_ex(cfg_file_id);
        sd_assert(ret_val == SUCCESS);
        if( ret_val == SUCCESS )
        {
            p_file_info->_cfg_file = 0;
        }
        ret_val = sd_delete_file(cfg_file_path);
        sd_assert(ret_val == SUCCESS);
    }

    return FALSE;
}

BOOL file_info_load_from_cfg_file(FILEINFO* p_file_info, char* cfg_file_path)
{

    _u32 len = 0, count = 0, m = 0, verno = 0;
    _u32 read_len = 0;
    _u8 has_postfix = 0;
    _u32 block_size = 0;
    RANGE tmp_range = {0};
    _u32 cid_size = 0;
    _int32 ret_val = SUCCESS;
    BOOL cid_is_valid = FALSE;
    _u32 cfg_file_id = 0;
    _int32 flag = O_FS_CREATE;
    _u64 fileszie = 0, writed_data_size = 0;
    RANGE full_r = {0};

    _u32 bcid_num = 0;
    _u32 bcid_map_num = 0;
    BOOL bcid_is_valid = FALSE;
    _u8* bcid_map_buffer = NULL;

    RANGE block_range = {0};
    _u32 i = 0;

    _u32 cfg_data_unit_size = 0;
    _u64 range_pos = 0;
    _u32 range_index = 0;
    _u64 next_range_pos = 0;
    _u32 next_range_index = 0;

#ifdef _LOGGER
    char  cid_str[CID_SIZE*2+1];
    char test_buffer[1024];
#endif

    char buf_int[8] = {0};
    char *p_int = NULL;
    _int32 len_int = 0;

    LOG_DEBUG("file_info_load_from_cfg_file .");

    full_r._index = 0;
    full_r._num = 0;

    ret_val =  sd_open_ex(cfg_file_path, flag, &cfg_file_id);

    if(ret_val != SUCCESS)
    {
        LOG_DEBUG("file_info_load_from_cfg_file, open cfg file %s failure, ret = %d .", cfg_file_path, ret_val);
        return FALSE;
    }

    sd_setfilepos(cfg_file_id, 0);

	/*version: 4Bytes*/
    ret_val = sd_read(cfg_file_id, (char*)buf_int, sizeof(verno), &read_len);
    p_int = buf_int;
    len_int = sizeof(buf_int);
    sd_get_int32_from_lt(&p_int, &len_int, (_int32 *)&verno);
    if( ret_val != SUCCESS || sizeof(verno) != read_len )
    {
        sd_close_ex(cfg_file_id);
		LOG_DEBUG("file_info_load_from_cfg_file .get version failure, ret = %d", ret_val);
        return FALSE;
    }

	//∞Ê±æ7 “˝»Îuserdata◊÷∂Œ°£
    if (verno == 7)
    {
        _int32 data_len;
        _u8 user_data[MAX_USER_DATA_LEN];
        ret_val = sd_read(cfg_file_id, (char*)buf_int, sizeof(data_len), &read_len);
        p_int = buf_int;
        len_int = sizeof(buf_int);
        sd_get_int32_from_lt(&p_int, &len_int, (_int32 *)&data_len);
        if( ret_val != SUCCESS || sizeof(data_len) != read_len || data_len > MAX_USER_DATA_LEN)
        {
            sd_close_ex(cfg_file_id);
			LOG_DEBUG("file_info_load_from_cfg_file .get user_data_len failure, ret = %d, data_len = %d, read_len = %d", ret_val, data_len, read_len);
            return FALSE;
        }

        if (data_len) 
        {
            ret_val = sd_read(cfg_file_id, (char*)user_data, data_len, &read_len);
            if( ret_val != SUCCESS || data_len != read_len )
            {
                sd_close_ex(cfg_file_id);
				LOG_DEBUG("file_info_load_from_cfg_file .get user_data failure, ret = %d", ret_val);
                return FALSE;
            }
        }
    }
	/* cfg_data_unit_size: 4Bytes */
    if (verno >= 5)
    {
        ret_val = sd_read(cfg_file_id, (char*)buf_int, sizeof(cfg_data_unit_size), &read_len);
        p_int = buf_int;
        len_int = sizeof(buf_int);
        sd_get_int32_from_lt(&p_int, &len_int, (_int32 *)&cfg_data_unit_size);
        if( ret_val != SUCCESS || sizeof(cfg_data_unit_size) != read_len )
        {
            sd_close_ex(cfg_file_id);
            return FALSE;
        }

        LOG_DEBUG("file_info_load_from_cfg_file, read cfg_data_unit_size = %u  .", cfg_data_unit_size);
        if( cfg_data_unit_size == 0 )
        {
            sd_close_ex(cfg_file_id);
            return FALSE;
        }
    }
    else if((3 == verno) || (4 == verno))
    {
        cfg_data_unit_size = 64 * 1024;
    }
    else
    {
        LOG_ERROR("file_info_load_from_cfg_file, cfg file verno %u failure .", verno);
        sd_close_ex(cfg_file_id);
        return FALSE;
    }

    /* fileszie: 8Bytes */
    ret_val = sd_read(cfg_file_id, (char*)buf_int, sizeof(fileszie), &read_len);
    p_int = buf_int;
    len_int = sizeof(buf_int);
    sd_get_int64_from_lt(&p_int, &len_int, (_int64 *)&fileszie);
    if( ret_val != SUCCESS || sizeof(fileszie) != read_len )
    {
        sd_close_ex(cfg_file_id);
		LOG_DEBUG("file_info_load_from_cfg_file .get file_size failure, ret = %d", ret_val);
        return FALSE;
    }

    LOG_DEBUG("file_info_load_from_cfg_file, verno:%u, read filesize = %llu  .", verno, fileszie);

    if(fileszie != 0)
    {
        file_info_set_filesize( p_file_info,fileszie);

    }

    /*has postfix: 1Byte*/
    ret_val = sd_read(cfg_file_id, (char*)&has_postfix, sizeof(has_postfix), &read_len);
    if( ret_val != SUCCESS || sizeof(has_postfix) != read_len )
    {
        sd_close_ex(cfg_file_id);
        return FALSE;
    }

    LOG_DEBUG("file_info_load_from_cfg_file, read has_postfix = %u  .", (_u32)has_postfix);

    if(has_postfix == 0)
    {
        p_file_info->_has_postfix =  FALSE;
    }
    else
    {
        p_file_info->_has_postfix =  TRUE;
    }

	/*user file name: ≥§∂»£´ƒ⁄»›, ?Bytes*/
    ret_val = sd_read(cfg_file_id, (char*)buf_int, sizeof(len), &read_len);
    p_int = buf_int;
    len_int = sizeof(buf_int);
    sd_get_int32_from_lt(&p_int, &len_int, (_int32 *)&len);
    if( ret_val != SUCCESS || sizeof(len) != read_len )
    {
        sd_close_ex(cfg_file_id);
        return FALSE;
    }

    LOG_DEBUG("file_info_load_from_cfg_file, read file name length = %u  .", len);

    if(len != 0)
    {
        if(len > MAX_FILE_NAME_LEN)
        {
            sd_close_ex(cfg_file_id);
            return FALSE;
        }
        ret_val = sd_read(cfg_file_id, (char*)(p_file_info->_file_name), len, &read_len);
        if(ret_val != SUCCESS || len != read_len)
        {
            sd_close_ex(cfg_file_id);
            return FALSE;
        }
        p_file_info->_file_name[len] = '\0';

        LOG_DEBUG("file_info_load_from_cfg_file, read file name  = %s  .", p_file_info->_file_name);
    }
	
    /*origin url: ≥§∂»£´ƒ⁄»›, ?Bytes*/
    ret_val = sd_read(cfg_file_id, (char*)buf_int, sizeof(len), &read_len);
    p_int = buf_int;
    len_int = sizeof(buf_int);
    sd_get_int32_from_lt(&p_int, &len_int, (_int32 *)&len);
    if( ret_val != SUCCESS || sizeof(len) != read_len )
    {
        sd_close_ex(cfg_file_id);
        return FALSE;
    }

    LOG_DEBUG("file_info_load_from_cfg_file, read url  length = %u  .", len);

    if(len != 0)
    {
        if(len > MAX_URL_LEN)
        {
            sd_close_ex(cfg_file_id);
            return FALSE;
        }
        ret_val = sd_read(cfg_file_id, (char*)(p_file_info->_origin_url), len, &read_len);
        if(ret_val != SUCCESS || len != read_len)
        {
            sd_close_ex(cfg_file_id);
            return FALSE;

        }

        p_file_info->_origin_url[len] = '\0';

        LOG_DEBUG("file_info_load_from_cfg_file, read url  = %s  .", p_file_info->_origin_url);
    }
	
    /*origin ref url: ≥§∂»£´ƒ⁄»›, ?Bytes*/
    ret_val = sd_read(cfg_file_id, (char*)&buf_int, sizeof(len), &read_len);
    p_int = buf_int;
    len_int = sizeof(buf_int);
    sd_get_int32_from_lt(&p_int, &len_int, (_int32 *)&len);
    if( ret_val != SUCCESS || sizeof(len) != read_len )
    {
        sd_close_ex(cfg_file_id);
        return FALSE;
    }

    LOG_DEBUG("file_info_load_from_cfg_file, read ref url  length = %u  .", len);

    if(len != 0)
    {
        if(len > MAX_URL_LEN)
        {
            sd_close_ex(cfg_file_id);
            return FALSE;
        }
        ret_val = sd_read(cfg_file_id, (char*)(p_file_info->_origin_ref_url), len, &read_len);
        if(ret_val != SUCCESS || len != read_len)
        {
            sd_close_ex(cfg_file_id);
            return FALSE;
        }

        p_file_info->_origin_ref_url[len] = '\0';
        LOG_DEBUG("file_info_load_from_cfg_file, read ref url  = %s  .", p_file_info->_origin_ref_url);
    }
	
   	/*cookie: ≥§∂»£´ƒ⁄»›, ?Bytes*/
	if(verno == 7)
	{
		ret_val = sd_read(cfg_file_id, (char*)&buf_int, sizeof(len), &read_len);
	    p_int = buf_int;
	    len_int = sizeof(buf_int);
	    sd_get_int32_from_lt(&p_int, &len_int, (_int32 *)&len);
	    if( ret_val != SUCCESS || sizeof(len) != read_len )
	    {
	        sd_close_ex(cfg_file_id);
			LOG_DEBUG("file_info_load_from_cfg_file, read cookie failed: ret = %d, read_len = %d  .", ret_val, read_len);
	        return FALSE;
	    }

	    LOG_DEBUG("file_info_load_from_cfg_file, read cookie  length = %u  .", len);

	    if(len != 0)
	    {
	        if(len > MAX_COOKIE_LEN)
	        {
	            sd_close_ex(cfg_file_id);
				LOG_DEBUG("file_info_load_from_cfg_file, read cookie failed: cookie_len > MAX_COOKIE_LEN  ." );
	            return FALSE;
	        }
	        ret_val = sd_read(cfg_file_id, (char*)(p_file_info->_cookie), len, &read_len);
	        if(ret_val != SUCCESS || len != read_len)
	        {
	            sd_close_ex(cfg_file_id);
	            return FALSE;
	        }

	        p_file_info->_cookie[len] = '\0';
	        LOG_DEBUG("file_info_load_from_cfg_file, read cookie  = %s  .", p_file_info->_cookie);
	    }
	}
	
    /*block size: 4Bytes*/
    ret_val = sd_read(cfg_file_id, (char*)buf_int, sizeof(block_size), &read_len);
    p_int = buf_int;
    len_int = sizeof(buf_int);
    sd_get_int32_from_lt(&p_int, &len_int, (_int32 *)&block_size);
    if( ret_val != SUCCESS || sizeof(block_size) != read_len )
    {
        sd_close_ex(cfg_file_id);
        return FALSE;
    }

    LOG_DEBUG("file_info_load_from_cfg_file, read block_size = %u  .", block_size);

    p_file_info->_block_size = block_size;

    /* CID : 4+4+20Bytes*/
    ret_val = sd_read(cfg_file_id, (char*)buf_int, sizeof(cid_size), &read_len);
    p_int = buf_int;
    len_int = sizeof(buf_int);
    sd_get_int32_from_lt(&p_int, &len_int, (_int32 *)&cid_size);
    if( ret_val != SUCCESS || sizeof(cid_size) != read_len )
    {
        sd_close_ex(cfg_file_id);
        return FALSE;
    }

    LOG_DEBUG("file_info_load_from_cfg_file, read cid_size = %u  .", cid_size);
    if(cid_size != CID_SIZE)
    {
        sd_close_ex(cfg_file_id);
        return FALSE;
    }
    ret_val = sd_read(cfg_file_id, (char*)buf_int, sizeof(cid_is_valid), &read_len);
    p_int = buf_int;
    len_int = sizeof(buf_int);
    sd_get_int32_from_lt(&p_int, &len_int, (_int32 *)&cid_is_valid);
    if( ret_val != SUCCESS || sizeof(cid_is_valid) != read_len )
    {
        sd_close_ex(cfg_file_id);
        return FALSE;
    }

    LOG_DEBUG("file_info_load_from_cfg_file, read cid_is_valid = %u  .", (_u32)cid_is_valid);

    p_file_info->_cid_is_valid = cid_is_valid;

    if(cid_is_valid == TRUE)
    {
        ret_val = sd_read(cfg_file_id, (char*)p_file_info->_cid, CID_SIZE, &read_len);
        if(ret_val != SUCCESS || CID_SIZE != read_len)
        {
            sd_close_ex(cfg_file_id);

            return FALSE;
        }

#ifdef _LOGGER
        str2hex((char*)p_file_info->_cid, CID_SIZE, cid_str, CID_SIZE*2);
        cid_str[CID_SIZE*2] = '\0';

        LOG_DEBUG("file_info_load_from_cfg_file,  read cid: %s .", cid_str);
#endif
    }

    /*  done ranges*/
    ret_val = sd_read(cfg_file_id, (char*)buf_int, sizeof(count), &read_len);
    p_int = buf_int;
    len_int = sizeof(buf_int);
    sd_get_int32_from_lt(&p_int, &len_int, (_int32 *)&count);
    if( ret_val != SUCCESS || sizeof(count) != read_len )
    {
        sd_close_ex(cfg_file_id);
        return FALSE;
    }

    LOG_DEBUG("file_info_load_from_cfg_file,  read write range count: %u .", count);

    if(fileszie != 0)
    {
        full_r = pos_length_to_range(0, fileszie, fileszie);
    }

    for(m = 0; m < count; m++)
    {
        tmp_range._index = 0;
        tmp_range._num = 0;

        ret_val = sd_read(cfg_file_id, (char*)buf_int, sizeof(tmp_range._index), &read_len);
        p_int = buf_int;
        len_int = sizeof(buf_int);
        sd_get_int32_from_lt(&p_int, &len_int, (_int32 *)&tmp_range._index);
        if( ret_val != SUCCESS || sizeof(tmp_range._index) != read_len )
        {
            sd_close_ex(cfg_file_id);
            return FALSE;
        }

        ret_val = sd_read(cfg_file_id, (char*)&buf_int, sizeof(tmp_range._num), &read_len);
        p_int = buf_int;
        len_int = sizeof(buf_int);
        sd_get_int32_from_lt(&p_int, &len_int, (_int32 *)&tmp_range._num);
        if( ret_val != SUCCESS || sizeof(tmp_range._num) != read_len )
        {
            sd_close_ex(cfg_file_id);
            return FALSE;
        }

        range_pos = (_u64)tmp_range._index * cfg_data_unit_size;
        if( range_pos % get_data_unit_size() == 0 )
        {
            range_index = range_pos / get_data_unit_size();
        }
        else
        {
            range_index = range_pos / get_data_unit_size() + 1;
        }
        next_range_pos = range_pos + (_u64)tmp_range._num * cfg_data_unit_size;
        next_range_index = next_range_pos / get_data_unit_size();

        if( next_range_index > range_index )
        {
            tmp_range._num = next_range_index - range_index;
        }
        else
        {
            tmp_range._num = 0;
        }
        tmp_range._index = range_index;


        LOG_DEBUG("file_info_load_from_cfg_file,  read write range node:%u:(%u,%u) .", m, tmp_range._index,tmp_range._num);
        if(full_r._num != 0)
        {
            if(tmp_range._index >= full_r._num)
            {
                tmp_range._num = 0;
                LOG_DEBUG("file_info_load_from_cfg_file,  read write range node:%u:(%u,%u)  but index is large then full_r._num:%u . so drop.", m, tmp_range._index,tmp_range._num, full_r._num);
            }
            else  if(tmp_range._index + tmp_range._num >  full_r._num)
            {
                tmp_range._num =   full_r._num - tmp_range._index ;
                LOG_DEBUG("file_info_load_from_cfg_file,  read write range node:%u: is force to (%u,%u) .", m, tmp_range._index,tmp_range._num);
            }
        }

        if( 0 != tmp_range._num)
        {
            range_list_add_range(&p_file_info->_writed_range_list, &tmp_range,NULL, NULL);
        }
    }

    range_list_cp_range_list(&p_file_info->_writed_range_list, &p_file_info->_data_receiver._total_receive_range);

    out_put_range_list(&p_file_info->_writed_range_list);

    out_put_range_list(&p_file_info->_data_receiver._total_receive_range);
    block_size = compute_block_size(fileszie);
    bcid_num =   (fileszie + block_size-1)/(block_size );

    if (verno >= 4)
    {
        ret_val = sd_read(cfg_file_id, (char*)buf_int, sizeof(bcid_is_valid), &read_len);
        p_int = buf_int;
        len_int = sizeof(buf_int);
        sd_get_int32_from_lt(&p_int, &len_int, (_int32 *)&bcid_is_valid);
        if( ret_val != SUCCESS || sizeof(bcid_is_valid) != read_len )
        {
            sd_close_ex(cfg_file_id);
            return FALSE;
        }

        LOG_DEBUG("file_info_load_from_cfg_file,  bcid_is_valid:%d .", bcid_is_valid);

        if(bcid_is_valid == TRUE)
        {
            ret_val = sd_read(cfg_file_id, (char*)&buf_int, sizeof(bcid_map_num), &read_len);
            p_int = buf_int;
            len_int = sizeof(buf_int);
            sd_get_int32_from_lt(&p_int, &len_int, (_int32 *)&bcid_map_num);
            if( ret_val != SUCCESS || sizeof(bcid_map_num) != read_len )
            {
                sd_close_ex(cfg_file_id);
                return FALSE;
            }
            sd_assert(bcid_map_num == ((bcid_num+8-1)/(8)) );

            sd_malloc(bcid_map_num, (void**)&bcid_map_buffer);
            if(bcid_map_buffer  != NULL)
            {
                ret_val = sd_read(cfg_file_id, (char*)bcid_map_buffer, bcid_map_num, &read_len);
                if( ret_val != SUCCESS || bcid_map_num != read_len )
                {
                    sd_close_ex(cfg_file_id);
                    return FALSE;
                }
            }
#ifdef _LOGGER
            sd_memset(test_buffer,0,1024);
            str2hex( (char *)bcid_map_buffer, bcid_map_num, test_buffer, 1024);
            LOG_DEBUG( "file_info_load_from_cfg_file:*bcid_map_buffer[%u]=%s" ,(_u32)bcid_map_num, test_buffer);
#endif
            p_file_info->_has_history_check_info = TRUE;
        }
    }

    if(fileszie != 0)
    {
        file_info_set_filesize( p_file_info , fileszie);

        if (verno == 3)
        {
            for(i = 0; i<bcid_num; i++)
            {
                block_range =  to_range(i, p_file_info->_block_size, fileszie);
                if( file_info_range_is_write(p_file_info,  &block_range) == TRUE )
                {
                    set_blockmap(&p_file_info->_bcid_info, i);
                    file_info_add_done_range(p_file_info, &block_range);
                }
            }
            p_file_info->_bcid_map_from_cfg = TRUE;
        }
        else if(bcid_map_buffer == NULL)
        {
            p_file_info->_bcid_map_from_cfg = FALSE;
        }
        else if (verno >= 4)
        {
            for(i = 0; i<bcid_num; i++)
            {
                block_range =  to_range(i, p_file_info->_block_size, fileszie);
                if( file_info_range_is_write(p_file_info,  &block_range) == TRUE
                    && TRUE == blockno_map_is_set(bcid_map_buffer, i))
                {
                    set_blockmap(&p_file_info->_bcid_info, i);
                    file_info_add_done_range(p_file_info, &block_range);
                }
            }
            p_file_info->_bcid_map_from_cfg = TRUE;
        }
    }

    if(bcid_map_buffer != NULL)
    {
        sd_free(bcid_map_buffer);
        bcid_map_buffer = NULL;
    }

    if (verno >= 6)
    {
        ret_val = sd_read(cfg_file_id, (char*)buf_int, sizeof(FILE_WRITE_MODE), &read_len);
        p_int = buf_int;
        len_int = sizeof(buf_int);
        sd_get_int32_from_lt(&p_int, &len_int, (_int32 *)&p_file_info->_write_mode);
        if( ret_val != SUCCESS || sizeof(FILE_WRITE_MODE) != read_len )
        {
            sd_close_ex(cfg_file_id);
            return FALSE;
        }
    }
    /*file manager info */
    ret_val = file_info_try_create_file( p_file_info);
    if( ret_val != SUCCESS )
    {
        sd_close_ex(cfg_file_id);
        return FALSE;
    }
    ret_val = fm_load_cfg_block_index_array( p_file_info->_tmp_file_struct, cfg_file_id );
    if( ret_val != SUCCESS )
    {
        sd_close_ex(cfg_file_id);
        /* Broken file,need deleted! */
        LOG_ERROR("file_info_load_from_cfg_file:the td.cfg file is broken,need deleted!:%s",cfg_file_path);
        sd_delete_file(cfg_file_path);
        return FALSE;
    }

    sd_close_ex(cfg_file_id);

    writed_data_size = file_info_get_writed_data_size( p_file_info );

#ifdef EMBED_REPORT
    p_file_info->_report_para._down_exist = writed_data_size;
#endif

    if(writed_data_size!=0&&writed_data_size>=fileszie)
    {
        if(p_file_info->_tmp_file_struct && p_file_info->_tmp_file_struct->_block_index_array._total_index_num!=0 &&
            p_file_info->_tmp_file_struct->_block_index_array._total_index_num == p_file_info->_tmp_file_struct->_block_index_array._valid_index_num)
        {
            p_file_info->_finished = TRUE;
        }
    }


    return TRUE;
}

_int32 file_info_get_info_from_cfg_file(char* cfg_file_path, char *origin_url, BOOL *cid_is_valid, _u8 *cid, _u64* file_size, char* file_name, char *lixian_url, char *cookie, char *user_data)
{
	_u32 len, verno; 
	_u32 read_len;
	_u8 has_postfix;
	_int32 ret_val = SUCCESS;
	_u32 cfg_file_id = 0;
	_int32 flag =O_FS_CREATE;
	_u64 filesize = 0;
	char filename[MAX_FILE_NAME_LEN] = {0};
	char url [MAX_URL_LEN];
	RANGE full_r ;
	_u32 cfg_data_unit_size = 0;
	_u32 block_size;
	BOOL file_cid_is_valid = FALSE;	
	_u8 file_cid[CID_SIZE];
	_u32 cid_size;
	char  cid_str[CID_SIZE*2+1];

	char buf_int[8] = {0};
    char *p_int = NULL;
    _int32 len_int = 0;
	LOG_DEBUG("file_info_get_info_from_cfg_file .");	  
	
	full_r._index = 0;
	full_r._num = 0;
	ret_val =  sd_open_ex(cfg_file_path, flag, &cfg_file_id);

	if(ret_val != SUCCESS)
	{
		LOG_DEBUG("file_info_get_info_from_cfg_file, open cfg file %s failure, ret = %d .", cfg_file_path, ret_val);
		return ret_val;	  
	}

	sd_setfilepos(cfg_file_id, 0);	  

	ret_val = sd_read(cfg_file_id, (char*)&verno, sizeof(verno), &read_len);
	if( ret_val != SUCCESS || sizeof(verno) != read_len )
	{
		sd_close_ex(cfg_file_id);
		return FALSE;
	}
	
 	if (verno == 7)
    {
        _int32 data_len;
        _u8 tmp_user_data[MAX_USER_DATA_LEN];
        ret_val = sd_read(cfg_file_id, (char*)buf_int, sizeof(data_len), &read_len);
        p_int = buf_int;
        len_int = sizeof(buf_int);
        sd_get_int32_from_lt(&p_int, &len_int, (_int32 *)&data_len);
        if( ret_val != SUCCESS || sizeof(data_len) != read_len || data_len > MAX_USER_DATA_LEN)
        {
            sd_close_ex(cfg_file_id);
			LOG_DEBUG("file_info_load_from_cfg_file .get user_data_len failure, ret = %d, data_len = %d, read_len = %d", ret_val, data_len, read_len);
            return FALSE;
        }

        if (data_len) 
        {
            ret_val = sd_read(cfg_file_id, (char*)tmp_user_data, data_len, &read_len);
            if( ret_val != SUCCESS || data_len != read_len )
            {
                sd_close_ex(cfg_file_id);
				LOG_DEBUG("file_info_load_from_cfg_file .get user_data failure, ret = %d", ret_val);
                return FALSE;
            }
        }
		//sd_strncpy(user_data, tmp_user_data, data_len);
    }

	if( verno >= 5 )
	{    
		ret_val = sd_read(cfg_file_id, (char*)&cfg_data_unit_size, sizeof(cfg_data_unit_size), &read_len);
		if( ret_val != SUCCESS || sizeof(cfg_data_unit_size) != read_len )
		{
			sd_close_ex(cfg_file_id);
			return FALSE;
		}
		LOG_DEBUG("file_info_get_info_from_cfg_file, read cfg_data_unit_size = %u  .", cfg_data_unit_size);	
		if( cfg_data_unit_size == 0 )
		{
			sd_close_ex(cfg_file_id);
			return FALSE;
		}
	}
	else if( 3 == verno || 4 == verno )
	{
		cfg_data_unit_size = 64 * 1024;
	}
	else
	{
		LOG_ERROR("file_info_get_info_from_cfg_file, cfg file verno %u failure .", verno);
		sd_close_ex(cfg_file_id); 
		return FALSE;
	}

	/*data file length: 8Bytes */
	ret_val = sd_read(cfg_file_id, (char*)&filesize, sizeof(filesize), &read_len);
	if( ret_val != SUCCESS || sizeof(filesize) != read_len )
	{
		sd_close_ex(cfg_file_id);
		return FALSE;
	}	 
	LOG_DEBUG("file_info_get_info_from_cfg_file, verno:%u, read filesize = %llu  .", verno, filesize);
	*file_size = filesize;
	
	/*has postfix: 1Byte*/
	ret_val = sd_read(cfg_file_id, (char*)&has_postfix, sizeof(has_postfix), &read_len);  
	if( ret_val != SUCCESS || sizeof(has_postfix) != read_len )
	{
		sd_close_ex(cfg_file_id);
		return FALSE;
	}

	LOG_DEBUG("file_info_get_info_from_cfg_file, read has_postfix = %u  .", (_u32)has_postfix);	 

	/*user file name: ≥§∂»£´ƒ⁄»›, ?Bytes*/
	ret_val = sd_read(cfg_file_id, (char*)&len, sizeof(len), &read_len);   
	if( ret_val != SUCCESS || sizeof(len) != read_len )
	{
		sd_close_ex(cfg_file_id);
		return FALSE;
	}

	LOG_DEBUG("file_info_get_info_from_cfg_file, read file name length = %u  .", len);	    

	if(len != 0)
	{
		if(len > MAX_FILE_NAME_LEN)
		{
			sd_close_ex(cfg_file_id); 
			return FALSE;
		}
		ret_val = sd_read(cfg_file_id, (char*)(filename), len, &read_len);
		if( ret_val != SUCCESS || len != read_len )
		{
			sd_close_ex(cfg_file_id);
			return FALSE;
		}
		filename[len] = '\0';    
		LOG_DEBUG("file_info_get_info_from_cfg_file, read file name  = %s  .", filename);	  
		
		sd_strncpy(file_name, filename, len);
	}
	
	/*origin url: ≥§∂»£´ƒ⁄»›, ?Bytes*/
	ret_val = sd_read(cfg_file_id, (char*)&len, sizeof(len), &read_len);   
	if( ret_val != SUCCESS || sizeof(len) != read_len )
	{
		sd_close_ex(cfg_file_id);
		return 0;
	}

	LOG_DEBUG("file_info_get_info_from_cfg_file, read url  length = %u  .", len);	

	if(len != 0)
	{	
		if(len > MAX_URL_LEN)
		{
			sd_close_ex(cfg_file_id); 
			return FALSE;   
		}
		ret_val = sd_read(cfg_file_id, (char*)(url), len, &read_len);
		if( ret_val != SUCCESS || len != read_len )
		{
			sd_close_ex(cfg_file_id);
			return FALSE;
		}
		url[len] = '\0';    
		LOG_DEBUG("file_info_get_info_from_cfg_file, read url  = %s  .", url);	  
	}

	sd_strncpy(origin_url, url, len);

	/*origin ref url: ≥§∂»£´ƒ⁄»›, ?Bytes*/
	ret_val = sd_read(cfg_file_id, (char*)&len, sizeof(len), &read_len);   
	if( ret_val != SUCCESS || sizeof(len) != read_len )
	{
		sd_close_ex(cfg_file_id);
		return FALSE;
	}

	LOG_DEBUG("file_info_get_info_from_cfg_file, read ref url  length = %u  .", len);	
	 
	if(len != 0)
	{
		if(len > MAX_URL_LEN)
		{
			sd_close_ex(cfg_file_id); 
			return FALSE;   	       
		}
		ret_val = sd_read(cfg_file_id, (char*)(url), len, &read_len);
		if( ret_val != SUCCESS || len != read_len )
		{
			sd_close_ex(cfg_file_id);
			return FALSE;
		}
		
		url[len] = '\0';   
		LOG_DEBUG("file_info_get_info_from_cfg_file, read ref url  = %s  .", url);	  
	}
	
	sd_strncpy(lixian_url, url, len);

	/*cookie: ≥§∂»£´ƒ⁄»›, ?Bytes*/
	if( verno == 7)
	{
		ret_val = sd_read(cfg_file_id, (char*)&len, sizeof(len), &read_len);   
		if( ret_val != SUCCESS || sizeof(len) != read_len )
		{
			sd_close_ex(cfg_file_id);
			return FALSE;
		}

		LOG_DEBUG("file_info_get_info_from_cfg_file, read cookie  length = %u  .", len);	
		 
		if(len != 0)
		{
			if(len > MAX_COOKIE_LEN)
			{
				sd_close_ex(cfg_file_id); 
				return FALSE;   	       
			}
			ret_val = sd_read(cfg_file_id, (char*)(url), len, &read_len);
			if( ret_val != SUCCESS || len != read_len )
			{
				sd_close_ex(cfg_file_id);
				return FALSE;
			}
			
			url[len] = '\0';   
			LOG_DEBUG("file_info_get_info_from_cfg_file, read cookie  = %s  .", url);	  
		}
		sd_strncpy(cookie, url, len);
	}
	/*block size: 4Bytes*/
	ret_val = sd_read(cfg_file_id, (char*)&block_size, sizeof(block_size), &read_len);
	if( ret_val != SUCCESS || sizeof(block_size) != read_len )
	{
		sd_close_ex(cfg_file_id);
		return 0;
	}

	LOG_DEBUG("file_info_get_info_from_cfg_file, read block_size = %u  .", block_size);	
	ret_val = sd_read(cfg_file_id, (char*)&cid_size, sizeof(cid_size), &read_len);
	if( ret_val != SUCCESS || sizeof(cid_size) != read_len )
	{
		sd_close_ex(cfg_file_id);
		return 0;
	}
       
	LOG_DEBUG("file_info_get_info_from_cfg_file, read cid_size = %u  .", cid_size);	
	if(cid_size != CID_SIZE)
	{
		sd_close_ex(cfg_file_id); 
		return FALSE;
	}
	ret_val = sd_read(cfg_file_id, (char*)&file_cid_is_valid, sizeof(file_cid_is_valid), &read_len);
	if( ret_val != SUCCESS || sizeof(file_cid_is_valid) != read_len )
	{
		sd_close_ex(cfg_file_id);
		return 0;
	}
		
	LOG_DEBUG("file_info_get_info_from_cfg_file, read cid_is_valid = %u  .", (_u32)file_cid_is_valid);	

	*cid_is_valid = file_cid_is_valid;
	
	if(file_cid_is_valid == TRUE)
	{
		ret_val = sd_read(cfg_file_id, (char*)file_cid, CID_SIZE, &read_len);
		if( ret_val != SUCCESS || CID_SIZE != read_len )
		{
			sd_close_ex(cfg_file_id);
			return 0;
		}
 
		str2hex((char*)file_cid, CID_SIZE, cid_str, CID_SIZE*2);  
		cid_str[CID_SIZE*2] = '\0'; 
		LOG_DEBUG("file_info_get_info_from_cfg_file,  read cid: %s .", cid_str);  

		sd_memcpy(cid, cid_str, CID_SIZE*2);
	}
	
	sd_close_ex(cfg_file_id);
	return TRUE;
}

_u64 file_info_get_downsize_from_cfg_file(char* cfg_file_path)
{
    _u32 len, count, m, verno;
    _u32 read_len;
    _u8 has_postfix;
    _u32 block_size;
    RANGE tmp_range;
    _u32 cid_size;
    _int32 ret_val = SUCCESS;
    BOOL cid_is_valid = FALSE;
    _u32 cfg_file_id = 0;
    _int32 flag =O_FS_CREATE;
    _u64 fileszie = 0;
    _u64 down_size = 0;
    _u32 down_num = 0;
    char file_name [MAX_FILE_NAME_LEN];
    char url [MAX_URL_LEN];
    _u8 cid[CID_SIZE];
    RANGE full_r ;
    _u32 cfg_data_unit_size = 0;

#ifdef _LOGGER
    char  cid_str[CID_SIZE*2+1];
#endif

    char buf_int[8] = {0};
    char *p_int = NULL;
    _int32 len_int = 0;

    LOG_DEBUG("file_info_get_downsize_from_cfg_file .");

    full_r._index = 0;
    full_r._num = 0;

    ret_val =  sd_open_ex(cfg_file_path, flag, &cfg_file_id);

    if(ret_val != SUCCESS)
    {
        LOG_DEBUG("file_info_get_downsize_from_cfg_file, open cfg file %s failure, ret = %d .", cfg_file_path, ret_val);
        return down_size;
    }

    sd_setfilepos(cfg_file_id, 0);

    ret_val = sd_read(cfg_file_id, (char*)&buf_int, sizeof(verno), &read_len);
    p_int = buf_int;
    len_int = sizeof(buf_int);
    sd_get_int32_from_lt(&p_int, &len_int, (_int32 *)&verno);
    if( ret_val != SUCCESS || sizeof(verno) != read_len )
    {
        sd_close_ex(cfg_file_id);
        return 0;
    }

    if( 5 <= verno)
    {
        ret_val = sd_read(cfg_file_id, (char*)buf_int, sizeof(cfg_data_unit_size), &read_len);
        p_int = buf_int;
        len_int = sizeof(buf_int);
        sd_get_int32_from_lt(&p_int, &len_int, (_int32 *)&cfg_data_unit_size);
        if( ret_val != SUCCESS || sizeof(cfg_data_unit_size) != read_len )
        {
            sd_close_ex(cfg_file_id);
            return 0;
        }

        LOG_DEBUG("file_info_get_downsize_from_cfg_file, read cfg_data_unit_size = %u  .", cfg_data_unit_size);
        if( cfg_data_unit_size == 0 )
        {
            sd_close_ex(cfg_file_id);
            return 0;
        }
    }
    else if( 3 == verno || 4 == verno )
    {
        cfg_data_unit_size = 64 * 1024;
    }
    else
    {
        LOG_ERROR("file_info_get_downsize_from_cfg_file, cfg file verno %u failure .", verno);
        sd_close_ex(cfg_file_id);
        return 0;
    }

    /*data file length: 8Bytes */
    ret_val = sd_read(cfg_file_id, (char*)buf_int, sizeof(fileszie), &read_len);
    p_int = buf_int;
    len_int = sizeof(buf_int);
    sd_get_int64_from_lt(&p_int, &len_int, (_int64 *)&fileszie);
    if( ret_val != SUCCESS || sizeof(fileszie) != read_len )
    {
        sd_close_ex(cfg_file_id);
        return 0;
    }

    LOG_DEBUG("file_info_get_downsize_from_cfg_file, read filesize = %llu  .", fileszie);

    if(fileszie != 0)
    {

    }

    /*has postfix: 1Byte*/
    ret_val = sd_read(cfg_file_id, (char*)&has_postfix, sizeof(has_postfix), &read_len);
    if( ret_val != SUCCESS || sizeof(has_postfix) != read_len )
    {
        sd_close_ex(cfg_file_id);
        return 0;
    }

    LOG_DEBUG("file_info_get_downsize_from_cfg_file, read has_postfix = %u  .", (_u32)has_postfix);

    /*user file name: ÈîüÊñ§Êã∑ÈîüÈ•∫ÔΩèÊã∑ÈîüÊñ§Êã∑ÈîüÊñ§?, ?Bytes*/
    ret_val = sd_read(cfg_file_id, (char*)buf_int, sizeof(len), &read_len);
    p_int = buf_int;
    len_int = sizeof(buf_int);
    sd_get_int32_from_lt(&p_int, &len_int, (_int32 *)&len);
    if( ret_val != SUCCESS || sizeof(len) != read_len )
    {
        sd_close_ex(cfg_file_id);
        return 0;
    }

    LOG_DEBUG("file_info_get_downsize_from_cfg_file, read file name length = %u  .", len);

    if(len != 0)
    {
        if(len > MAX_FILE_NAME_LEN)
        {
            sd_close_ex(cfg_file_id);
            return down_size;
        }
        ret_val = sd_read(cfg_file_id, (char*)(file_name), len, &read_len);
        if( ret_val != SUCCESS || len != read_len )
        {
            sd_close_ex(cfg_file_id);
            return 0;
        }

        file_name[len] = '\0';
        LOG_DEBUG("file_info_get_downsize_from_cfg_file, read file name  = %s  .", file_name);
    }

    ret_val = sd_read(cfg_file_id, (char*)buf_int, sizeof(len), &read_len);
    p_int = buf_int;
    len_int = sizeof(buf_int);
    sd_get_int32_from_lt(&p_int, &len_int, (_int32 *)&len);
    if( ret_val != SUCCESS || sizeof(len) != read_len )
    {
        sd_close_ex(cfg_file_id);
        return 0;
    }

    LOG_DEBUG("file_info_get_downsize_from_cfg_file, read url  length = %u  .", len);

    if(len != 0)
    {
        if(len > MAX_URL_LEN)
        {
            sd_close_ex(cfg_file_id);
            return down_size;
        }
        ret_val = sd_read(cfg_file_id, (char*)(url), len, &read_len);
        if( ret_val != SUCCESS || len != read_len )
        {
            sd_close_ex(cfg_file_id);
            return 0;
        }
        url[len] = '\0';
        LOG_DEBUG("file_info_get_downsize_from_cfg_file, read url  = %s  .", url);
    }

    ret_val = sd_read(cfg_file_id, (char*)buf_int, sizeof(len), &read_len);
    p_int = buf_int;
    len_int = sizeof(buf_int);
    sd_get_int32_from_lt(&p_int, &len_int, (_int32 *)&len);
    if( ret_val != SUCCESS || sizeof(len) != read_len )
    {
        sd_close_ex(cfg_file_id);
        return 0;
    }

    LOG_DEBUG("file_info_get_downsize_from_cfg_file, read ref url  length = %u  .", len);

    if(len != 0)
    {
        if(len > MAX_URL_LEN)
        {
            sd_close_ex(cfg_file_id);
            return down_size;
        }
        ret_val = sd_read(cfg_file_id, (char*)(url), len, &read_len);
        if( ret_val != SUCCESS || len != read_len )
        {
            sd_close_ex(cfg_file_id);
            return 0;
        }

        url[len] = '\0';
        LOG_DEBUG("file_info_get_downsize_from_cfg_file, read ref url  = %s  .", url);
    }

    /*block size: 4Bytes*/
    ret_val = sd_read(cfg_file_id, (char*)buf_int, sizeof(block_size), &read_len);
    p_int = buf_int;
    len_int = sizeof(buf_int);
    sd_get_int32_from_lt(&p_int, &len_int, (_int32 *)&block_size);
    if( ret_val != SUCCESS || sizeof(block_size) != read_len )
    {
        sd_close_ex(cfg_file_id);
        return 0;
    }

    LOG_DEBUG("file_info_get_downsize_from_cfg_file, read block_size = %u  .", block_size);
    ret_val = sd_read(cfg_file_id, (char*)buf_int, sizeof(cid_size), &read_len);
    p_int = buf_int;
    len_int = sizeof(buf_int);
    sd_get_int32_from_lt(&p_int, &len_int, (_int32 *)&cid_size);
    if( ret_val != SUCCESS || sizeof(cid_size) != read_len )
    {
        sd_close_ex(cfg_file_id);
        return 0;
    }

    LOG_DEBUG("file_info_get_downsize_from_cfg_file, read cid_size = %u  .", cid_size);
    if(cid_size != CID_SIZE)
    {
        sd_close_ex(cfg_file_id);
        return down_size;
    }
    ret_val = sd_read(cfg_file_id, (char*)buf_int, sizeof(cid_is_valid), &read_len);
    p_int = buf_int;
    len_int = sizeof(buf_int);
    sd_get_int32_from_lt(&p_int, &len_int, (_int32 *)&cid_is_valid);
    if( ret_val != SUCCESS || sizeof(cid_is_valid) != read_len )
    {
        sd_close_ex(cfg_file_id);
        return 0;
    }

    LOG_DEBUG("file_info_get_downsize_from_cfg_file, read cid_is_valid = %u  .", (_u32)cid_is_valid);

    if(cid_is_valid == TRUE)
    {
        ret_val = sd_read(cfg_file_id, (char*)cid, CID_SIZE, &read_len);
        if( ret_val != SUCCESS || CID_SIZE != read_len )
        {
            sd_close_ex(cfg_file_id);
            return 0;
        }

#ifdef _LOGGER
        str2hex((char*)cid, CID_SIZE, cid_str, CID_SIZE*2);
        cid_str[CID_SIZE*2] = '\0';

        LOG_DEBUG("file_info_get_downsize_from_cfg_file,  read cid: %s .", cid_str);
#endif
    }

    /*  done ranges*/
    ret_val = sd_read(cfg_file_id, (char*)&buf_int, sizeof(count), &read_len);
    p_int = buf_int;
    len_int = sizeof(buf_int);
    sd_get_int32_from_lt(&p_int, &len_int, (_int32 *)&count);
    if( ret_val != SUCCESS || sizeof(count) != read_len )
    {
        sd_close_ex(cfg_file_id);
        return 0;
    }

    LOG_DEBUG("file_info_get_downsize_from_cfg_file,  read write range count: %u .", count);

    if(fileszie != 0)
    {
        full_r = pos_length_to_range(0, fileszie, fileszie);
    }

    for(m = 0; m < count; m++)
    {
        tmp_range._index = 0;
        tmp_range._num = 0;

        ret_val = sd_read(cfg_file_id, (char*)buf_int, sizeof(tmp_range._index), &read_len);
        p_int = buf_int;
        len_int = sizeof(buf_int);
        sd_get_int32_from_lt(&p_int, &len_int, (_int32 *)&tmp_range._index);
        if( ret_val != SUCCESS || sizeof(tmp_range._index) != read_len )
        {
            sd_close_ex(cfg_file_id);
            return 0;
        }

        ret_val = sd_read(cfg_file_id, (char*)&buf_int, sizeof(tmp_range._num), &read_len);
        p_int = buf_int;
        len_int = sizeof(buf_int);
        sd_get_int32_from_lt(&p_int, &len_int, (_int32 *)&tmp_range._num);
        if( ret_val != SUCCESS || sizeof(tmp_range._num) != read_len )
        {
            sd_close_ex(cfg_file_id);
            return 0;
        }

        LOG_DEBUG("file_info_get_downsize_from_cfg_file,  read write range node:%u:(%u,%u) .", m, tmp_range._index,tmp_range._num);

        if(full_r._num != 0)
        {
            if(tmp_range._index >= full_r._num)
            {
                tmp_range._num = 0;
                LOG_DEBUG("file_info_get_downsize_from_cfg_file,  read write range node:%u:(%u,%u)  but index is large then full_r._num:%u . so drop.", m, tmp_range._index,tmp_range._num, full_r._num);
            }
            else  if(tmp_range._index + tmp_range._num >  full_r._num)
            {
                tmp_range._num =   full_r._num - tmp_range._index ;
                LOG_DEBUG("file_info_get_downsize_from_cfg_file,  read write range node:%u: is force to (%u,%u) .", m, tmp_range._index,tmp_range._num);
            }
        }

        if(tmp_range._num != 0)
        {
            down_num += tmp_range._num;
        }

    }

    down_size = (_u64)down_num *(_u64)cfg_data_unit_size;
    if(fileszie != 0 )
    {
        if(down_size > fileszie)
        {
            down_size = fileszie;
        }
    }
    sd_close_ex(cfg_file_id);
    return down_size;
}
_u32 file_info_get_valid_data_speed(FILEINFO* p_file_info )
{
    _u32 speed = 0;
    calculate_speed(&p_file_info->_cur_data_speed, &speed);
    LOG_DEBUG("file_info_get_valid_data_speed, %u  .", speed);

    return speed;
}

_int32 file_info_set_bcid_enable(FILEINFO* p_file_info, BOOL enable)
{
    p_file_info->_bcid_enable = enable;
    return SUCCESS;
}

_int32 file_info_set_is_calc_bcid(FILEINFO* p_file_info, BOOL is_calc)
{
    LOG_DEBUG("file_info_set_is_calc_bcid,%d .", is_calc);
    p_file_info->_is_calc_bcid = is_calc;
    return SUCCESS;
}

_int32 file_info_set_check_mode(FILEINFO* p_file_info, BOOL _is_need_calc_bcid)
{
    
    BOOL ret = SUCCESS;
    BOOL tmp_flag = p_file_info->_is_need_calc_bcid;
    p_file_info->_is_need_calc_bcid = _is_need_calc_bcid;
    LOG_DEBUG("file_info set check mode : %d, prev_flag = %d, input_flag = %d"
        , _is_need_calc_bcid, tmp_flag, _is_need_calc_bcid );
    if (tmp_flag == FALSE && _is_need_calc_bcid == TRUE)
    {
        ret = start_check_blocks(p_file_info);
    }
    return ret;
}

_int32 file_info_calc_hash_notify_result(CHH_HASH_INFO* hash_result, void* user_data)
{
    _int32 ret_val = SUCCESS;
    FILEINFO* p_file_info = (FILEINFO*)user_data;
    _u32 blockno = hash_result->_block_no;
    _u8* block_cid = hash_result->_hash_result;
    RANGE block_range;
    
    unset_bitmap(p_file_info->_bcid_info._bcid_checking_bitmap, blockno);

    if(p_file_info->_bcid_is_valid == TRUE )
    {
        ret_val = verify_block_cid(&p_file_info->_bcid_info, blockno, block_cid);
        LOG_DEBUG("ptr_file_info->_is_backup_bcid_valid = %d, ret_val = %d", p_file_info->_is_backup_bcid_valid, ret_val);
        if(p_file_info->_is_backup_bcid_valid == TRUE && ret_val != SUCCESS)
        {
            ret_val = verify_block_cid(&p_file_info->_bcid_info_backup, blockno, block_cid);
        }

        if(ret_val == SUCCESS)
        {
            LOG_DEBUG("file_info_calc_hash_notify_result, verify_block_cid, p_file_info:0x%x ,check blockno:%u  success.", p_file_info,blockno);
            set_blockmap(&p_file_info->_bcid_info, blockno);
        }
        else
        {
            LOG_DEBUG("file_info_calc_hash_notify_result, verify_block_cid, p_file_info:0x%x ,check blockno:%u  failure.", p_file_info,blockno);
        }
    }
    else
    {
        ret_val = set_block_cid(&p_file_info->_bcid_info, blockno, block_cid);

        if(ret_val == SUCCESS)
        {
            LOG_DEBUG("file_info_calc_hash_notify_result, verify_block_cid, p_file_info:0x%x ,set blockno:%u  success.", p_file_info,blockno);
            set_blockmap(&p_file_info->_bcid_info, blockno);
        }
        else
        {
            LOG_DEBUG("file_info_calc_hash_notify_result, verify_block_cid, p_file_info:0x%x ,set blockno:%u  failure, ret_val:%u.", p_file_info, blockno, ret_val);
        }
    }

    block_range = to_range(blockno, p_file_info->_block_size, p_file_info->_file_size);

    sd_free(hash_result->_hash_result);
    sd_free(hash_result->_data_buffer);
    sd_free(hash_result);
    if(ret_val == BCID_CHECK_FAIL)
    {
        LOG_ERROR("file_info_calc_hash_notify_result , block no: %u, range (%u,%u) check failure."
            , blockno, block_range._index,block_range._num);

        data_receiver_del_buffer(&p_file_info->_data_receiver, &block_range);

        file_info_notify_check(p_file_info, blockno, FALSE);
    }
    else if(ret_val == SUCCESS)
    {
        LOG_DEBUG("file_info_calc_hash_notify_result , block no: %u, range (%u,%u) check success."
            , blockno, block_range._index,block_range._num);
        file_info_notify_check(p_file_info, blockno, TRUE);
    }



    start_check_blocks(p_file_info);

    return ret_val;
}

#ifdef EMBED_REPORT
FILE_INFO_REPORT_PARA *file_info_get_report_para( FILEINFO* p_file_info )
{
    return &p_file_info->_report_para;
}


void file_info_handle_valid_data_report(FILEINFO* p_file_info, RESOURCE *resource_ptr, _u32 data_len )
{
    BOOL is_loacl_nate = sd_is_in_nat(sd_get_local_ip());
    _u32 peer_capability = 0;
    if( resource_ptr == NULL || resource_ptr->_resource_type != PEER_RESOURCE ) return;

    peer_capability = p2p_get_capability(resource_ptr);
    if( p2p_get_from( resource_ptr ) == P2P_FROM_PARTNER_CDN )
    {
        p_file_info->_report_para._down_by_partner_cdn += data_len;
    }

    if( is_loacl_nate )
    {
        if( !is_nated(peer_capability) )
        {
            if( p2p_get_from( resource_ptr ) == P2P_FROM_HUB  )
            {
                p_file_info->_report_para._down_n2i_no += data_len;
            }
            else if( p2p_get_from( resource_ptr ) == P2P_FROM_TRACKER )
            {
                p_file_info->_report_para._down_n2i_yes += data_len;
            }
        }
        else
        {
            if( p2p_get_from( resource_ptr ) == P2P_FROM_HUB )
            {
                p_file_info->_report_para._down_n2n_no += data_len;
            }
            else if( p2p_get_from( resource_ptr ) == P2P_FROM_TRACKER )
            {
                p_file_info->_report_para._down_n2n_yes += data_len;
            }
            if( is_same_nat(peer_capability) )
            {
                if( p2p_get_from( resource_ptr ) == P2P_FROM_HUB )
                {
                    p_file_info->_report_para._down_n2s_no += data_len;
                }
                else if( p2p_get_from( resource_ptr ) == P2P_FROM_TRACKER )
                {
                    p_file_info->_report_para._down_n2s_yes += data_len;
                }
            }
        }
    }
    else
    {
        if( !is_nated(peer_capability) )
        {
            if( p2p_get_from( resource_ptr ) == P2P_FROM_HUB )
            {
                p_file_info->_report_para._down_i2i_no += data_len;
            }
            else if( p2p_get_from( resource_ptr ) == P2P_FROM_TRACKER )
            {
                p_file_info->_report_para._down_i2i_yes += data_len;
            }
        }
        else
        {
            if( p2p_get_from( resource_ptr ) == P2P_FROM_HUB )
            {
                p_file_info->_report_para._down_i2n_no += data_len;
            }
            else if( p2p_get_from( resource_ptr ) == P2P_FROM_TRACKER )
            {
                p_file_info->_report_para._down_i2n_yes += data_len;
            }
        }

    }

}

void file_info_handle_err_data_report(FILEINFO* p_file_info, RESOURCE *resource_ptr, _u32 data_len )
{
    if( resource_ptr->_resource_type == HTTP_RESOURCE
        || resource_ptr->_resource_type == FTP_RESOURCE )
    {
        p_file_info->_report_para._down_err_by_svr += data_len;

        if( resource_ptr->_resource_type == HTTP_RESOURCE)
        {
        	HTTP_SERVER_RESOURCE* p_http_res = (HTTP_SERVER_RESOURCE* )resource_ptr;
        	if(p_http_res->_relation_res)
        	{
        		 p_file_info->_report_para._down_err_by_appacc += data_len;
        	}
        }
    }
    else if( resource_ptr->_resource_type == PEER_RESOURCE )
    {
        if(  p2p_get_from(resource_ptr) == P2P_FROM_VIP_HUB)
        {
            p_file_info->_report_para._down_err_by_cdn += data_len;
        }
        else if( p2p_get_from(resource_ptr) == P2P_FROM_PARTNER_CDN )
        {
            p_file_info->_report_para._down_err_by_partner_cdn += data_len;
        }
        else
        {
            p_file_info->_report_para._down_err_by_peer += data_len;
        }
    }
}

void file_info_add_overloap_date(FILEINFO* p_file_info, _u32 date_len)
{
    if( p_file_info == NULL )
    {
        sd_assert( FALSE );
        return;
    }
    p_file_info->_report_para._overlap_bytes += date_len;
}

#endif

_int32 dm_donot_fast_stop_hook(const _int32 down_task)
{
	g_fast_stop = 0;
	return SUCCESS;
}

_int32 dm_get_enable_fast_stop()
{
	return g_fast_stop;
}
