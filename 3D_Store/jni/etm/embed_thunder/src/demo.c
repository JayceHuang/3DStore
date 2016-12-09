/*--------------------------------------------------------------------------*/
/*                               IDENTIFICATION                             */
/*--------------------------------------------------------------------------*/
/*     Filename  : Demo.c                                         */
/*     Author     : ZengYuqing                                              */
/*     Project    : EmbedThunder                                        */
/*     Version    : 1.3                                                     */
/*--------------------------------------------------------------------------*/
/*                  Shenzhen XunLei Networks                                */
/*--------------------------------------------------------------------------*/
/*                  - C (copyright) - www.xunlei.com -                      */
/*                                                                          */
/*      This document is the proprietary of XunLei                          */
/*                                                                          */
/*      All rights reserved. Integral or partial reproduction               */
/*      prohibited without a written authorization from the                 */
/*      permanent of the author rights.                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
/*                         FUNCTIONAL DESCRIPTION                           */
/*--------------------------------------------------------------------------*/
/* This file contains the main function for EmbedThunder Demo Program                        */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*                              HISTORY                                     */
/*--------------------------------------------------------------------------*/
/*   Date     |    Author   | Modification                                  */
/*--------------------------------------------------------------------------*/
/* 2008.09.06 | ZengYuqing  | Creation                                      */
/*--------------------------------------------------------------------------*/
/* 2009.01.19 | ZengYuqing  | Update to version 1.2                                     */
/*--------------------------------------------------------------------------*/
/* 2009.04.13 | ZengYuqing  | Update to version 1.3

   1.增加用于启动（et_start_http_server）和关闭（et_stop_http_server）VOD点播用的http服务器的接口调用。
   2.增加用.torrent文件直接启动BT任务的启动方法：./EmbedThunder ./my_bt.torrent
   3.在显示任务信息时增加显示参数vv_speed用于显示点播时实际的数据下载速度。
   4.增加用于调用et_create_task_by_tcid_file_size_gcid接口的代码。  [2009.05.14]
   5.增加用于调用et_set_download_record_file_path 接口的代码。  [2009.07.01]
   6.增加用于调用et_get_upload_pipe_info 接口的代码。   [2009.07.23]

   --------------------------------------------------------------------------*/
#if  !defined(MOBILE_PHONE)
#include <stdio.h>

#ifdef LINUX
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <memory.h>
#include <signal.h>
#ifndef MACOS
#include <malloc.h>
#endif
#include <dirent.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#elif defined(WINCE)
#include <winsock2.h>
#include <windows.h>
#elif defined(SUNPLUS)
#include <sys/param.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#endif
#include "embed_thunder.h"

#ifdef ENABLE_DEMO_RSA
#include <openssl/rsa.h>
#include <openssl/bio.h>
#include <openssl/x509.h>
#endif

//#define  LONG_RUN_TESTING
#define  TIME_OUT_TESTING

#define URL_FILE "./url.txt"
#define RECORD_FILE_PATH "./"

#define LOCAL_PATH "tddownload/"
#define DRM_FILE_FULL_PATH "./480p.xv"

#define LICENSE "13032020010000014000000ck3ej3fk5400s6v3cv8"    //Super license
//#define LICENSE "10051800010000000000001foh2j15w25ih9k3pc30"  //Super license

//#define LICENSE "0810100001099a951a5fcd4ad593a129815438ef39"  //Super license
//#define LICENSE "09122200010000000000001o66pgv265sg406p50zk"  //Super license

#define MIN_BT_FILE_SIZE 50*1024
#define INT_TO_HEX(ch)  ((ch) < 10 ? (ch) + '0' : (ch) - 10 + 'A')
#define MAX_BT_RUN_FILE_NUM 50

typedef struct
{
    int task_type;      /* 0: p2sp task,1: bt task, 2: emule task */

    char url[2048];
    char refer_url[2048];
    char dir[256];
    char filename[256];
    unsigned char cid[20];
    unsigned char gcid[20];
    unsigned long long filesize;
    int is_created;
    int is_cid;
    int is_gcid;
    unsigned int task_id;

    /* Just for bt task */
    char seed_file_path[256];
    unsigned int seed_file_path_len;
    unsigned int download_file_index_array[MAX_BT_RUN_FILE_NUM];
    unsigned int file_num;
    int is_default_mode;
    int is_lixian_on;
} TASK_PARA;

typedef struct task_list
{
    TASK_PARA task;
    struct task_list *_p_next;
} TASK_LIST;


int string_to_cid (char *str, unsigned char *cid);
unsigned long long str_to_u64 (char *str);
void str_to_hex (char *in_buf, int in_bufsize, char *out_buf,
                 int out_bufsize);
int get_path_from_argv (char *_argv_path, char *_path, char *_file_name);
TASK_LIST *load_task_from_url_file (char *_file_name);
int parse_seed_info (char *seed_path, TASK_PARA * p_task_para);
int notify_license_result (unsigned int result, unsigned int expire_time, char *sessionid, unsigned int sessionid_len);
int parse_main_argvs (int argc, char *argv[], TASK_LIST ** pp_task_list,
                      unsigned int *p_task_timeout,
                      BOOL * p_parse_torrent_seed_file, BOOL * p_from_file);
int create_single_task (TASK_PARA * p_task);
int create_tasks (TASK_LIST * _p_task_list,
                  TASK_LIST ** pp_task_list_next_wait_create);
int start_tasks (TASK_LIST * _p_task_list, int *p_running_task_num);
void display_task_informations (TASK_PARA * p_task, ET_TASK * info);
#ifdef _CONNECT_DETAIL
void display_upoad_pipe_informations (ET_PEER_PIPE_INFO_ARRAY *
                                      p_upload_pipe_info_array);
#endif

#ifdef _SUPPORT_ZLIB
int zlib_uncompress (unsigned char *p_out_buffer, int *p_out_len,
                     const unsigned char *p_in_buffer, int in_len);
#endif

static TASK_LIST **g_task_list_pp = NULL;
#ifdef LINUX
void sigint_handler(int sig)
{
    TASK_LIST *p_task_list = *g_task_list_pp;
    TASK_LIST *p_tmp_node = NULL;
    /* Warning:
       Generally, DO NOT call printf in a signal handler,
       it's not safe! But in this case, this signal handler
       makes the precess going down, and just output the
       information to notify what happened! It dosen't matter
       if some unexpected happened.
    */
    printf("Break, clear tasks...");
    while(p_task_list && (p_task_list->task.task_id != 0))
    {
        p_tmp_node = p_task_list->_p_next;
        et_stop_task(p_task_list->task.task_id);
        et_delete_task(p_task_list->task.task_id);
        printf("%d.", p_task_list->task.task_id);
        fflush(stdout);
        usleep(1000*500);// 500ms
        free(p_task_list);
        p_task_list = p_tmp_node;
    }
    printf("Done!\nFinalizing ET...");
    et_uninit();
    printf("Done!\n");
    exit(0);
}
#endif /* LINUX */

extern void prn_alloc_info(void);

int main (int argc, char *argv[])
{

    /*
       Some test url:

       "http://down.sandai.net/Thunder5.8.4.572.exe";
       "http://cachefile23.fs2you.com/zh-cn/download/9e5bc3516fd6162627a64ff80e241387/setup_2_7039build_NOD.rar";
       "http://bbs.jiashan.gov.cn/music/name/bjhyn.mp3";
       "http://www1.cnxp.com/WoW134.exe";
       "ftp://20080827:20080827@down.gx73.com:25/20080827/[DVD][高清晰][中英字幕][高速下载]木乃伊3：龙帝之墓.rmvb";
       "ftp://lado:ffdy@ftp10.ffdy.cn:11235/功夫熊猫真正DVD中字/[放放电影www.ffdy.cn]功夫熊猫真正DVD中字.rmvb";
       "http://download101.wanmei.com/chibi/client/chibi_ob.zip";
     */
    ET_TASK info;
    int i = 0, j = 0;
    int ret_val = 0;
    TASK_LIST *_p_task_list_head = NULL, *_p_task_list = NULL, *_p_task_list_pre = NULL, *_p_task_list_next_wait_create = NULL;
    char file_name_buffer[256];
    int buffer_size = 256, force_end_task = 0, running_task_num = 0;
    unsigned int task_timeout = 0;  // just for timeout testing
    unsigned int download_time = 0;
    BOOL is_hsc_enabled = FALSE;
    ET_HIGH_SPEED_CHANNEL_INFO hsc_info;

    BOOL hsc_auto_flag = FALSE;
    u32 round_cnt = 0;
    g_task_list_pp = &_p_task_list_head;
#ifdef ENABLE_DEMO_RSA
    void *openssl_rsa_func_table[ET_OPENSSL_IDX_COUNT] =
    {
        (void *) d2i_RSAPublicKey,
        (void *) RSA_size,
        (void *) BIO_new_mem_buf,
        (void *) d2i_RSA_PUBKEY_bio,
        (void *) RSA_public_decrypt,
        (void *) RSA_free,
        (void *) BIO_free,
        (void *) RSA_public_encrypt
    };
#endif

#ifdef LINUX
    struct stat stat_buf;
#endif
    unsigned int up_time = 0;
#ifdef _CONNECT_DETAIL
    ET_PEER_PIPE_INFO_ARRAY upload_pipe_info_array;
#endif

#ifdef VOD_READ_TEST
    unsigned long long read_pos;
    int last_time = 0;
    int now_time = 0;
    int first_time = 0;
    char *read_buffer;
    int read_len = 16 * 1024;
    int vod_ret;
    int default_bitrate = 100 * 1024;
#endif

    BOOL has_task = FALSE, from_file = FALSE, _parse_torrent_seed_file = FALSE;
#ifdef  LONG_RUN_TESTING
    int count = 0;
#endif
    printf ("\n");

    if ((argc == 2) && (argv[1] != 0) && (strcmp (argv[1], "-v") == 0))
    {
        /* Show the version of Embed Thunder download library */
        printf (et_get_version ());
        printf ("\n");
        return 0;
    }
    else
    {
        /* load task(s) ... */
        ret_val = parse_main_argvs (argc, argv, &_p_task_list_head, &task_timeout, &_parse_torrent_seed_file, &from_file);
        if (ret_val != 0)
        {
            return ret_val;
        }

        _p_task_list = _p_task_list_head;
    }

#ifdef WINCE
    {
        WSADATA infowsa;
        if (WSAStartup (MAKEWORD (2, 0), &infowsa))
        {

        }
    }
#endif
    /* Initiate the Embed Thunder download library */

#ifdef _SUPPORT_ZLIB
    et_set_customed_interface (ET_ZLIB_UNCOMPRESS, zlib_uncompress);
#endif

#ifdef ENABLE_DEMO_RSA
    ret_val = et_set_openssl_rsa_interface (ET_OPENSSL_IDX_COUNT, openssl_rsa_func_table);
    if (ret_val != 0)
    {
        printf (" init error, ret = %d \n", ret_val);
        goto ErrHandle;
    }
#endif
    ret_val = et_init (NULL);
    if (ret_val != 0)
    {
        printf (" init error, ret = %d \n", ret_val);
        goto ErrHandle;
    }

#ifdef LINUX
    /* Delay 1 second */
    usleep (1000 * 1000);
#else
    Sleep (1000);
#endif

#ifdef LINUX
    /* Delay 1 second */
    usleep (1000 * 1000);
#else
    Sleep (1000);
#endif
#ifdef LINUX
    /* Delay 1 second */
    usleep (1000 * 1000);
#else
    Sleep (1000);
#endif
    /* Check if need to parse the torrent seed file */
    if (_parse_torrent_seed_file == TRUE)
    {
        parse_seed_info (argv[2], NULL);
        et_uninit ();
        return 0;
    }

#ifdef LINUX
    /* Chek if the download directory is exist */
    memset (&stat_buf, 0, sizeof (stat_buf));
    if ((lstat (LOCAL_PATH, &stat_buf) == 0) && (S_ISDIR (stat_buf.st_mode)))
    {
        printf ("Directory :%s is found!\n", LOCAL_PATH);
    }
    else
    {
        printf ("Directory :%s is not found,need be created!\n", LOCAL_PATH);
        /* Create the download directory */
        mkdir (LOCAL_PATH, 0777);
    }
#endif

    ret_val = et_set_download_record_file_path (RECORD_FILE_PATH, strlen (RECORD_FILE_PATH));
    if ((ret_val != 0) && (ret_val != 2058))
    {
        printf (" et_set_download_record_file_path error, ret = %d \n", ret_val);
        goto ErrHandle2;
    }

#ifdef  LONG_RUN_TESTING
create_task:
#endif
    ret_val = create_tasks (_p_task_list_head, &_p_task_list_next_wait_create);
    if (ret_val != 0)
    {
        goto ErrHandle3;
    }

    et_set_product_sub_id (_p_task_list_head->task.task_id, 0, 0);

    ret_val = start_tasks (_p_task_list_head, &running_task_num);
    if (ret_val != 0)
    {
        goto ErrHandle4;
    }
    /* Get informations of all the running tasks and display in the screen */

#ifdef _SUPPORT_ZLIB
    et_set_customed_interface(ET_ZLIB_UNCOMPRESS,zlib_uncompress );
#endif

#ifdef _CONNECT_DETAIL
    printf
    ("  task info:  status,  speed(server_speed,peer_speed), progress,  filesize. cm_level \n");
    printf
    ("  pipes info: pipe num(server_dowing,server_connect, peer_downing,peer_connecting), tcp_pipe, udp_pipe, tcp_peer_speed, udp_peer_speed.\n");
    printf
    ("  resource info: server resources num(idle_server_res_num,using_server_res_num,candidate_server_res_num,retry_server_res_num,discard_server_res_num,destroy_server_res_num). peer resources num(idle_peer_res_num,using_peer_res_num,candidate_peer_res_num,retry_peer_res_num,discard_peer_res_num,destroy_peer_res_num).\n");
#else
    printf
    ("  task info  status,  speed(server_speed,peer_speed), progress, pipes(server_dowing,server_connect, peer_dowing,peer_connecting), filesize.\n");

#endif
#ifdef VOD_READ_TEST
    read_pos = 0;
    read_buffer = malloc (read_len);
    now_time = time (NULL);
    last_time = now_time;
    first_time = now_time;
    srand (now_time);
#endif

    /* Loop for getting task information untill all tasks finished */
    et_auto_hsc_sw (1);
    char *key = "287B449E73DA05384B098D74367791F35B6294D63F4A746002FB100E2AA1E2F6AC5221F8A22A56C278E88F1AC3CBDBF604983DE1AC93E5E29617A86A01A8F7E7E98F0C5DD4309DADD1B1DABCBC90AE2A";
    while (TRUE)
    {

        round_cnt++;
        printf ("\n");
        has_task = FALSE;
        _p_task_list = _p_task_list_head;
        _p_task_list_pre = _p_task_list;

        /* Get the information of running tasks one by one */
        while ((_p_task_list) && (_p_task_list->task.task_id != 0))
        {
            et_get_hsc_info (_p_task_list->task.task_id, &hsc_info);
            et_get_hsc_is_auto (&hsc_auto_flag);
            if (hsc_auto_flag || is_hsc_enabled)
            {
                printf("\n>>>>>>>>>>>>>>>>>>>>>>HIGH SPEED CHANNEL INFOMATION<<<<<<<<<<<<<<<<<<<<<<\n");
                printf("remainning bytes: %llu, cost bytes this time: %llu\nresources: %u, speed: %u, downloaded bytes: %llu, \n",
                       hsc_info._remaining_flow, hsc_info._cur_cost_flow, hsc_info._res_num, hsc_info._speed, hsc_info._dl_bytes);
                printf("AUTO ENABLE OPTION: %s, avaliable:%d\n", hsc_auto_flag ? "ON" : "OFF", hsc_info._can_use);
                printf("HIGH SPEED CHANNEL STATUS: %d\tError Code:%d\n", hsc_info._stat, hsc_info._errcode);
                printf("channel type: %d\n", hsc_info._channel_type);
                printf(">>>>>>>>>>>>>>>>>>>>>>HIGH SPEED CHANNEL INFOMATION<<<<<<<<<<<<<<<<<<<<<<\n");
            }
            if (_p_task_list->task.is_lixian_on == FALSE)
            {
            
            }
            if (_p_task_list->task.is_lixian_on)
            {
                 printf("\n>>>>>>>>>>>>>>>>>>>>>> LIXIAN <<<<<<<<<<<<<<<<<<<<<<\n");
            }
            has_task = TRUE;
            memset (&info, 0, sizeof (ET_TASK));
            ret_val = et_get_task_info (_p_task_list->task.task_id, &info);
            if (ret_val != 0)
            {
                printf(" get_task_info %u, error, ret = %d \n", _p_task_list->task.task_id, ret_val);
                goto ErrHandle4;
            }

            /* Display the task information in the screen */
            if (info._task_status != ET_TASK_IDLE)
            {
                display_task_informations (&(_p_task_list->task), &info);
            }

#ifdef VOD_READ_TEST
            now_time = time (NULL);
            printf ("\nVOD lasttime=%d ,now_time=%d ", last_time, now_time);
            if ((now_time - last_time) > 1)
            {
                last_time = now_time;
                while (1)
                {
                    vod_ret = et_vod_read_file (_p_task_list->task.task_id, read_pos, read_len, read_buffer, 400);
                    printf("\nVOD et_vod_read_file read_pos=%llu ret=%d  time_len = %d , speed=%.2f B/S", read_pos, vod_ret, (now_time - first_time),
                           (float) ((long double) read_pos / (long double) (now_time - first_time)));
                    if (vod_ret == 0)
                    {
                        read_pos = (read_pos + read_len) % info._file_size;
                        if (read_pos > (default_bitrate * (now_time - first_time)))
                        {
                            //read enough data
                            break;
                        }
                    }
                    else
                    {
                        //read fail
                        break;
                    }
                }
            }
#endif
            /* Check if timeout */
            /* 注意：原则上不建议用任务开始时间（_start_time）和完成时间（_finished_time）作时间比较的运算，因为在获取这两个时间之间系统时间有可能会被恶意修改。
               此处只是在确保系统时间没有被修改的情况下作测试和演示用！ */
#ifdef LINUX
            if ((task_timeout != 0) && (((unsigned int) time (NULL)) - info._start_time >= task_timeout))
            {
                /* timeout,should stop the task! */
                force_end_task = 1;
            }
#else
#endif

            /* Check if the task has finished */
            if (info._task_status == ET_TASK_SUCCESS || info._task_status == ET_TASK_FAILED || force_end_task == 1)
            {
                if (info._task_status == ET_TASK_SUCCESS)
                {
                    ret_val = et_get_task_file_name (_p_task_list->task.task_id, file_name_buffer, &buffer_size);
                    if (ret_val == 0)
                    {
                        printf ("task id:%u, file name[%d] =%s\n", _p_task_list->task.task_id, strlen (file_name_buffer), file_name_buffer);
                    }
                    printf ("task id:%u, Down load task success!\n", _p_task_list->task.task_id);
                    /* 注意：原则上不建议用任务开始时间（_start_time）和完成时间（_finished_time）作时间比较的运算，因为在获取这两个时间之间系统时间有可能会被恶意修改。
                       此处只是在确保系统时间没有被修改的情况下作测试和演示用！ */
                    download_time = info._finished_time - info._start_time;
                }
                else if (info._task_status == ET_TASK_FAILED)
                {
                    printf("task_id:%u, Down load task failed! _failed_code=%u \n", _p_task_list->task.task_id, info._failed_code);
                    /* 注意：原则上不建议用任务开始时间（_start_time）和完成时间（_finished_time）作时间比较的运算，因为在获取这两个时间之间系统时间有可能会被恶意修改。
                       此处只是在确保系统时间没有被修改的情况下作测试和演示用！ */
                    download_time = info._finished_time - info._start_time;
                }
                else
                {
                    printf("\n\ntask_id:%u, Down load task force end by timeout! timeout=%u \n", _p_task_list->task.task_id, task_timeout);
                    download_time = task_timeout;
                    force_end_task = 0;
                }

                /* Stop the task */
                et_stop_task (_p_task_list->task.task_id);

                if (download_time == 0)
                    download_time = 1;  //The task running time is less than 1 second

                /* 注意：这里用任务开始时间（_start_time）和完成时间（_finished_time）作时间比较的运算出来的平均速度只是在确保系统时间没有被修改的情况下作测试和演示用！ */
                printf("\ntask id: %u, filesize=%llu, downloaded_data_size=%llu,_progress=%d, download_time=%u s, average_speed=%llu B/s [%7u,%7u]\n\n",
                       _p_task_list->task.task_id, info._file_size, info._downloaded_data_size, info._progress, download_time, info._downloaded_data_size/download_time,
                       info._server_speed, info._peer_speed);
#if 0
                extern unsigned int g_read_statistics_count, g_write_statistics_count, g_block_check_in_mem_count, g_block_check_by_read_count;
                extern unsigned int g_data_manager_download_once_num, MAX_FLUSH_UNIT_NUM, DATA_UNIT_SIZE;
                extern unsigned long long int g_read_statistics_bytes, g_write_statistics_bytes;
                extern unsigned int g_fm_max_merge_range_num;
                extern unsigned int MAX_CACHE_BUFFER_SIZE, MAX_ALLOC_BUFFER_SIZE;
                extern unsigned int g_sequential_write_count;
                printf("==================================================\n"
                       "read count=%u, read bytes=%llu\n"
                       "write count=%u, sequential_write_count=%u, write bytes=%llu\n"
                       "check block in mem count: %u, by read: %u\n"
                       "section size=%u, flush size=%d, max merge num=%u\n"
                       "MAX_CACHE_BUFFER_SIZE=%u, MAX_ALLOC_BUFFER_SIZE=%u\n"
                       "==================================================\n",
                       g_read_statistics_count, g_read_statistics_bytes,
                       g_write_statistics_count, g_sequential_write_count, g_write_statistics_bytes,
                       g_block_check_in_mem_count, g_block_check_by_read_count,
                       g_data_manager_download_once_num*DATA_UNIT_SIZE,
                       MAX_FLUSH_UNIT_NUM*DATA_UNIT_SIZE, g_fm_max_merge_range_num,
                       MAX_CACHE_BUFFER_SIZE, MAX_ALLOC_BUFFER_SIZE);
                fflush (stdout);
#endif
                /* Remove the temp file(s) of task */
                if (info._task_status == ET_TASK_FAILED)
                {
                    //et_remove_task_tmp_file( _p_task_list->task.task_id);
                }

                /* Delete the task */
                et_delete_task (_p_task_list->task.task_id);

                /* Free the memory */
                if (_p_task_list == _p_task_list_head)
                {
                    _p_task_list_head = _p_task_list->_p_next;
                    _p_task_list_pre = _p_task_list_head;
                    free (_p_task_list);
                    _p_task_list = _p_task_list_pre;
                }
                else
                {
                    _p_task_list_pre->_p_next = _p_task_list->_p_next;
                    free (_p_task_list);
                    _p_task_list = _p_task_list_pre->_p_next;
                }

                /* Create the next task  */
                if (_p_task_list_next_wait_create != NULL)
                {
                    printf ("\n OK,create the next task \n");
                    ret_val = create_single_task (&(_p_task_list_next_wait_create->task));
                    /* Check the task creating result */
                    if (ret_val != 0)
                    {
                        printf(" create_task %u, error, ret = %d \n", _p_task_list_next_wait_create->task.task_id, ret_val);
                        goto ErrHandle4;
                    }

#ifdef VOD_READ_TEST
                    et_set_task_no_disk(_p_task_list_next_wait_create->task.task_id);
#else
                    //et_set_task_no_disk(_p_task_list_next_wait_create->task.task_id);
#endif
                    /* Start the next task  */
                    printf("\n OK,start the next task:task_id= %u \n", _p_task_list_next_wait_create->task.task_id);
                    ret_val = et_start_task (_p_task_list_next_wait_create->task.task_id);
                    /* Check the task starting result */
                    if (ret_val != 0)
                    {
                        printf("start_task %u, error, ret = %d \n", _p_task_list_next_wait_create->task.task_id, ret_val);
                        goto ErrHandle4;
                    }

                    _p_task_list_next_wait_create = _p_task_list_next_wait_create->_p_next;

                }
            }
            else
            {
                /* The task is still running... */
                _p_task_list_pre = _p_task_list;
                _p_task_list = _p_task_list_pre->_p_next;
            }

            /* Continue to get the next task information */

        }

        /* All tasks's informations have displayed to the screen !  */

#ifdef _CONNECT_DETAIL
        if (0 == et_get_upload_pipe_info (&upload_pipe_info_array))
        {
            display_upoad_pipe_informations (&upload_pipe_info_array);
        }
#endif
        /* Check is there any task still running ? */
        if (has_task == FALSE)
        {
            /* All tasks have finished!  */
#ifdef LONG_RUN_TESTING
            if (from_file == FALSE)
            {
                /* All done,quit! */
                break;
            }
            else
            {
                /* Reload all the tasks from the url.txt */

                while (_p_task_list_head != NULL)
                {
                    _p_task_list = _p_task_list_head;

                    _p_task_list_head = _p_task_list_head->_p_next;

                    free (_p_task_list);

                }

                /*clear the last downed files */
#if defined(_ANDROID_ARM)
                system ("rm ./tddownload/* -r");
#else
                system ("rm ./tddownload/* -fr");
#endif

                sleep (60);

                count++;
                printf("Start to load task from url file:%s , count:%d \n", URL_FILE, count);
                _p_task_list_head = load_task_from_url_file (URL_FILE);
                if (_p_task_list_head != NULL)
                {
                    printf("load task from url file:%s ok, count :%d !\n", URL_FILE, count);
                    _p_task_list = _p_task_list_head;
                }
                else
                {
                    printf("load task from url file:%s failure, count :%d !\n", URL_FILE, count);
                    break;
                }

                /* Run again... */
                goto create_task;
            }
#else
            /* All done,quit! */
            break;
#endif /* LONG_RUN_TESTING */

        }
#ifdef LINUX
        /* Delay 1 second */
        usleep (1000 * 1000);
#else
        Sleep (1000);
#endif
    }

    up_time = (unsigned int) time (NULL);
    while (((unsigned int) time (NULL)) - up_time < task_timeout)
    {
        printf("\n download_speed=%u,upload_speed=%u \n", et_get_current_download_speed (), et_get_current_upload_speed ());
#ifdef _CONNECT_DETAIL
        if (0 == et_get_upload_pipe_info (&upload_pipe_info_array))
        {
            display_upoad_pipe_informations (&upload_pipe_info_array);
        }
#endif
#ifdef LINUX
        /* Delay 1 second */
        usleep (1000 * 1000);
#elif defined(SUNPLUS)
        sleep (1);
#else
        Sleep (1000);
#endif
    }

    printf ("wait for free mem when task finish task");
    prn_alloc_info ();

    printf (" \n");

    /* Uninitiate the Embed Thunder download library */
    et_uninit ();

    printf ("wait for free mem when program finished\n");

    return 0;

ErrHandle4:

    printf (" ErrHandle4: stop all the running tasks \n  ");
    _p_task_list = _p_task_list_head;
    while ((_p_task_list) && (_p_task_list->task.task_id != 0))
    {
        et_stop_task (_p_task_list->task.task_id);
        _p_task_list = _p_task_list->_p_next;
    }
    _p_task_list = _p_task_list_head;


ErrHandle3:

    printf (" ErrHandle3: detele all the tasks \n  ");
    _p_task_list = _p_task_list_head;
    while ((_p_task_list) && (_p_task_list->task.task_id != 0))
    {
        et_delete_task (_p_task_list->task.task_id);
        _p_task_list = _p_task_list->_p_next;
    }
    _p_task_list = _p_task_list_head;

    //et_stop_http_server();

ErrHandle2:

    printf
    (" ErrHandle2: uninitiate the Embed Thunder download library \n  ");
    et_uninit ();

ErrHandle:

    printf (" ErrHandle: release memory,ret_val=%d \n  ", ret_val);
    while (_p_task_list)
    {
        _p_task_list_head = _p_task_list->_p_next;
        free (_p_task_list);
        _p_task_list = _p_task_list_head;
    }

    return -1;
}

/*  This is the call back function of license report result
    Please make sure keep it in this type:
    typedef int32 ( *ET_NOTIFY_LICENSE_RESULT)(u32 result, u32 expire_time);
 */
static unsigned int et_license_result = -1;

/* Get task(s)'s parameters from main argvs*/
int
parse_main_argvs (int argc, char *argv[], TASK_LIST ** pp_task_list,
                  unsigned int *p_task_timeout,
                  BOOL * p_parse_torrent_seed_file, BOOL * p_from_file)
{
    int ret_val = 0, _case = 0;
    TASK_LIST *_p_task_list_head = NULL, *_p_task_list = NULL;
    unsigned int task_timeout = 0;
    BOOL _parse_torrent_seed_file = FALSE;
    char *http_head = NULL, *https_head = NULL,
          *ftp_head = NULL, *thunder_head = NULL,
           *torrent_tail = NULL, *emule_head = NULL, *bt_magnet_url = NULL;
    BOOL from_file = FALSE;

    if (argc >= 2)
    {
        /* Get the task parameters(Only one task) */
        _case = (int) str_to_u64 (argv[1]);
        switch (_case)
        {
            case 1:     //create new task by url
                if ((argv[2] != 0) && (strlen (argv[2]) != 0) && (strlen (argv[2]) < 512))  //url
                {
                    _p_task_list = NULL;
                    _p_task_list = (TASK_LIST *) malloc (sizeof (TASK_LIST));
                    if (_p_task_list == NULL)
                    {
                        return -1;
                    }
                    memset (_p_task_list, 0, sizeof (TASK_LIST));
                    strcpy (_p_task_list->task.url, argv[2]);
                    if ((argv[3] != 0) && (strlen (argv[3]) != 0))  //loacal path and name
                    {
                        /* Get file name and path */
                        ret_val =
                            get_path_from_argv (argv[3],
                                                _p_task_list->task.dir,
                                                _p_task_list->task.filename);
                        if (ret_val != 0)
                            goto ErrHandle;
                    }
                    else
                    {
                        /* Can not get file path and name,but they are necessary  */
                        goto ErrHandle;
                    }

                    _p_task_list_head = _p_task_list;


                }
                else
                {
                    /* Invalid URl */
                    goto ErrHandle;
                }
                break;
            case 2:     //create continue task by url
                if ((argv[2] != 0) && (strlen (argv[2]) != 0) && (strlen (argv[2]) < 512))  //url
                {
                    _p_task_list = NULL;
                    _p_task_list = (TASK_LIST *) malloc (sizeof (TASK_LIST));
                    if (_p_task_list == NULL)
                    {
                        return -1;
                    }

                    memset (_p_task_list, 0, sizeof (TASK_LIST));
                    strcpy (_p_task_list->task.url, argv[2]);
                    if ((argv[3] != 0) && (strlen (argv[3]) != 0))  //loacal path and name
                    {
                        /* Get file name and path */

                        ret_val =
                            get_path_from_argv (argv[3],
                                                _p_task_list->task.dir,
                                                _p_task_list->task.filename);
                        if (ret_val != 0)
                            goto ErrHandle;
                    }
                    else
                    {
                        /* Can not get file path and name,but they are necessary  */
                        goto ErrHandle;
                    }
                    _p_task_list->task.is_created = 1;
                    _p_task_list_head = _p_task_list;
                }
                else
                {
                    /* Invalid URl */
                    goto ErrHandle;
                }
                break;
            case 3:     //create new task by cid
                if ((argv[2] != 0) && (strlen (argv[2]) == 40)) //cid
                {
                    _p_task_list = NULL;
                    _p_task_list = (TASK_LIST *) malloc (sizeof (TASK_LIST));
                    if (_p_task_list == NULL)
                    {
                        return -1;
                    }

                    memset (_p_task_list, 0, sizeof (TASK_LIST));

                    /* Get cid */
                    ret_val = string_to_cid (argv[2], _p_task_list->task.cid);
                    if (ret_val != 0)
                    {
                        goto ErrHandle;
                    }

                    /* Get file name and path */
                    if ((argv[3] != 0) && (strlen (argv[3]) != 0))
                    {
                        ret_val =
                            get_path_from_argv (argv[3],
                                                _p_task_list->task.dir,
                                                _p_task_list->task.filename);
                        if (ret_val != 0)
                            goto ErrHandle;
                    }
                    else
                    {
                        /* Can not get file path and name,but they are necessary  */
                        goto ErrHandle;
                    }

                    _p_task_list->task.is_cid = 1;
                    _p_task_list_head = _p_task_list;
                }
                else
                {
                    /* Invalid CID */
                    goto ErrHandle;
                }
                break;
            case 4:     //create continue task by cid
                if ((argv[2] != 0) && (strlen (argv[2]) == 40)) //cid
                {
                    _p_task_list = NULL;
                    _p_task_list = (TASK_LIST *) malloc (sizeof (TASK_LIST));
                    if (_p_task_list == NULL)
                    {
                        return -1;
                    }

                    memset (_p_task_list, 0, sizeof (TASK_LIST));
                    /* Get cid */
                    ret_val = string_to_cid (argv[2], _p_task_list->task.cid);
                    if (ret_val != 0)
                    {
                        goto ErrHandle;
                    }

                    if ((argv[3] != 0) && (strlen (argv[3]) != 0))  //loacal path and name
                    {
                        /* Get file name and path */
                        ret_val =
                            get_path_from_argv (argv[3],
                                                _p_task_list->task.dir,
                                                _p_task_list->task.filename);
                        if (ret_val != 0)
                            goto ErrHandle;

                    }
                    else
                    {
                        /* Can not get file path and name,but they are necessary  */
                        goto ErrHandle;
                    }

                    _p_task_list->task.is_created = 1;
                    _p_task_list->task.is_cid = 1;
                    _p_task_list_head = _p_task_list;
                }
                else
                {
                    /* Invalid CID */
                    goto ErrHandle;
                }
                break;

            case 5:     //bt task
            {
                int argv_index = 4;
                _p_task_list = NULL;
                _p_task_list = (TASK_LIST *) malloc (sizeof (TASK_LIST));
                if (_p_task_list == NULL)
                {
                    return -1;
                }

                memset (_p_task_list, 0, sizeof (TASK_LIST));


                if ((argv[2] != 0) && (strlen (argv[2]) != 0))  //seed full path
                {
                    /* Get seed path */
                    strncpy (_p_task_list->task.seed_file_path, argv[2], 255);
                    _p_task_list->task.seed_file_path_len = strlen (argv[2]);
                }
                else
                {
                    goto ErrHandle;
                }

                if ((argv[3] != 0) && (strlen (argv[3]) != 0))  //data path
                {
                    /* Get seed path */
                    strncpy (_p_task_list->task.dir, argv[3], 255);
                }
                else
                {
                    /* Can not get seed path,but it is necessary  */
                    goto ErrHandle;
                }

                if (argv[4] == 0)
                    goto ErrHandle;

                /* Get the need-download-file index one by one */
                while (argv[argv_index] && argv_index < argc)
                {
#ifdef TIME_OUT_TESTING
                    if (strstr (argv[argv_index], "timeout=") != NULL)
                    {
                        break;
                    }
#endif /* TIME_OUT_TESTING */
                    _p_task_list->task.
                    download_file_index_array[argv_index - 4] =
                        (unsigned int) (str_to_u64 (argv[argv_index]));
                    _p_task_list->task.file_num++;
                    argv_index++;
                }

                _p_task_list->task.task_type = 1;
                _p_task_list->task.is_default_mode = 0;
                _p_task_list_head = _p_task_list;

            }
            break;
            case 6:     //parse the bt seed file and diaplay to screen...
            {
                if ((argv[2] != 0) && (strlen (argv[2]) != 0))  //seed full path
                {
                    _parse_torrent_seed_file = TRUE;

                }
                else
                {
                    /* Can not get seed path,but it is necessary  */
                    goto ErrHandle;
                }


            }
            break;

            /* argv[1] can not convert to numeral */
            default:
            {
                if ((argv[1] != 0) && (strlen (argv[1]) != 0) && (strlen (argv[1]) < 2048)) //url
                {
                    http_head = strstr (argv[1], "http://");
                    ftp_head = strstr (argv[1], "ftp://");
                    https_head = strstr (argv[1], "https://");
                    thunder_head = strstr (argv[1], "thunder://");
                    torrent_tail = strstr (argv[1], ".torrent");
                    emule_head = strstr (argv[1], "ed2k://");
                    bt_magnet_url = strstr (argv[1], "magnet:");

                    //ed2k url里面可能存在"http://"字段,所以必须注意过滤这种情况
                    if (((http_head != NULL) || (ftp_head != NULL)
                         || (https_head != NULL)
                         || (thunder_head != NULL))
                        && emule_head == NULL && bt_magnet_url == NULL)
                    {
                        _p_task_list = NULL;
                        _p_task_list =
                            (TASK_LIST *) malloc (sizeof (TASK_LIST));
                        if (_p_task_list == NULL)
                        {
                            return -1;
                        }
                        memset (_p_task_list, 0, sizeof (TASK_LIST));
                        strcpy (_p_task_list->task.url, argv[1]);
                        if ((argv[2] != 0) && (strlen (argv[2]) != 0))  //task time out or loacal path and name
                        {
                            /* Get file name and path */
                            ret_val =
                                get_path_from_argv (argv[2],
                                                    _p_task_list->
                                                    task.dir,
                                                    _p_task_list->
                                                    task.filename);
                            if (ret_val != 0)
                                goto ErrHandle;

                        }
                        else
                        {
                            sprintf (_p_task_list->task.dir, LOCAL_PATH);
                        }
                        _p_task_list_head = _p_task_list;
                    }
                    else if (torrent_tail != NULL
                             && strlen(torrent_tail) == 8)
                    {
                        /* A  BT seed is found */
                        _p_task_list = NULL;
                        _p_task_list =
                            (TASK_LIST *) malloc (sizeof (TASK_LIST));
                        if (_p_task_list == NULL)
                        {
                            return -1;
                        }
                        memset (_p_task_list, 0, sizeof (TASK_LIST));

                        _p_task_list->task.task_type = 1;
                        _p_task_list->task.is_default_mode = 1;
                        _p_task_list->task.seed_file_path_len =
                            strlen (argv[1]);
                        strncpy (_p_task_list->task.seed_file_path,
                                 argv[1], strlen (argv[1]));
                        sprintf (_p_task_list->task.dir, LOCAL_PATH);
                        _p_task_list_head = _p_task_list;
                    }
                    else if (emule_head != NULL)
                    {
                        // emule
                        _p_task_list =
                            (TASK_LIST *) malloc (sizeof (TASK_LIST));
                        if (_p_task_list == NULL)
                        {
                            return -1;
                        }
                        memset (_p_task_list, 0, sizeof (TASK_LIST));
                        _p_task_list->task.task_type = 2;
                        strcpy (_p_task_list->task.url, argv[1]);
                        sprintf (_p_task_list->task.dir, LOCAL_PATH);
                        _p_task_list_head = _p_task_list;
                    }
                    else if (bt_magnet_url != NULL)
                    {
                        // bt_magnet
                        _p_task_list =
                            (TASK_LIST *) malloc (sizeof (TASK_LIST));
                        if (_p_task_list == NULL)
                        {
                            return -1;
                        }
                        memset (_p_task_list, 0, sizeof (TASK_LIST));
                        _p_task_list->task.task_type = 3;
                        strcpy (_p_task_list->task.url, argv[1]);
                        if (argc == 3 && argv[2] != NULL)
                        {
                            strcpy(_p_task_list->task.dir, argv[2]);
                        }
                        else
                        {
                            sprintf (_p_task_list->task.dir, LOCAL_PATH);
                        }
                        _p_task_list_head = _p_task_list;
                    }
                    else
                    {
#ifdef TIME_OUT_TESTING
                        /* Get timeout */
                        if ((argv[argc - 1] != 0)
                            && (strlen (argv[argc - 1]) != 0)
                            && (strstr (argv[argc - 1], "timeout=") != NULL))
                        {
                            task_timeout =
                                (unsigned int)
                                str_to_u64 (argv[argc - 1] +
                                            strlen ("timeout="));
                            if (task_timeout != -1)
                            {
                                goto LOAD_FILE;
                            }
                        }
#endif /* TIME_OUT_TESTING */
                        printf ("Error when geting task !\n");
                        return -1;
                    }

                }
                else
                {
                    printf ("Error when geting task !\n");
                    return -1;
                }
            }


        }

#ifdef TIME_OUT_TESTING
        /* Get timeout */
        if ((argv[argc - 1] != 0) && (strlen (argv[argc - 1]) != 0)
            && (strstr (argv[argc - 1], "timeout=") != NULL))
        {
            task_timeout =
                (unsigned int) str_to_u64 (argv[argc - 1] +
                                           strlen ("timeout="));
            if (task_timeout == -1)
            {
                task_timeout = 0;
                printf ("Geting timerout error!:%s \n", argv[argc - 1]);
                ret_val = -1;
                goto ErrHandle;
            }
        }
#endif /* TIME_OUT_TESTING */

    }
    else if (argc < 2)
    {
#ifdef TIME_OUT_TESTING
    LOAD_FILE:
#endif
        /* Get the task(s) parameters from task file(Multi-tasks) */
        printf ("Start to load task from url file:%s\n", URL_FILE);
        _p_task_list_head = load_task_from_url_file (URL_FILE);
        if (_p_task_list_head != NULL)
        {
            printf ("load task from url file:%s ok!\n", URL_FILE);
            _p_task_list = _p_task_list_head;
            from_file = TRUE;
        }
        else
        {
#ifdef TIME_OUT_TESTING
            if ((task_timeout == 0) || (task_timeout == -1))
#endif
            {
                printf ("load task from url file:%s failed!\n", URL_FILE);
                return -1;
            }
        }
    }
    else
    {
        printf ("Error when geting task !\n");
        return -1;

    }

    *pp_task_list = _p_task_list_head;
    *p_task_timeout = task_timeout;
    *p_parse_torrent_seed_file = _parse_torrent_seed_file;
    *p_from_file = from_file;

    return 0;

ErrHandle:

    printf (" ErrHandle: ret_val=%d \n  ", ret_val);
    while (_p_task_list)
    {
        _p_task_list_head = _p_task_list->_p_next;
        free (_p_task_list);
        _p_task_list = _p_task_list_head;
    }

    return -1;

}

int
get_path_from_argv (char *_argv_path, char *_path, char *_file_name)
{
    char *tmp = NULL;
    if ((_argv_path == NULL) || (strlen (_argv_path) < 1) || (_path == NULL)
        || (_file_name == NULL))
        return -1;

    tmp = strrchr (_argv_path, '/');
    if ((tmp != NULL) && (strlen (tmp) != 0))
    {
        strncpy (_file_name, tmp + 1, 255);
        _file_name[strlen (_file_name)] = '\0';
        strncpy (_path, _argv_path, tmp - _argv_path + 1);
        _path[tmp - _argv_path + 1] = '\0';
    }
    else
    {
        strncpy (_file_name, _argv_path, 255);
        sprintf (_path, "./");
    }

    return 0;
}



/*  load url task(s) from url.txt
_file_name: full path name of the task file; e.g. "url.txt"
 */
TASK_LIST *
load_task_from_url_file (char *_file_name)
{
    TASK_LIST *_p_task_list = NULL, *_p_task_list_head =
                                  NULL, *_p_task_list_tail = NULL;
    char buffer[2048];
    char cid_buffer[60];
    char *ptr = NULL;
    FILE *file = NULL;
    int sub_cha = 1;
    char *ptr2 = NULL;

    if (strlen (_file_name) < 1)
    {
        /* File name error */
        printf ("url file name Error!\n");
        return NULL;
    }

#ifdef LINUX
    file = fopen (_file_name, "r");
    if (file == NULL)
    {
        /* Opening File error */
        printf ("Opening url file Error!\n");
        return NULL;
    }
#endif
    memset (buffer, 0, 2048);
    while (fgets (buffer, 2048, file) != NULL)
    {
        if (buffer[0] == '#')
            continue;       /* Comment line should be escaped */
        /* Ok,valid url is found!  */
        _p_task_list = (TASK_LIST *) malloc (sizeof (TASK_LIST));
        if (_p_task_list == NULL)
        {
            printf ("load_task_from_url_file:malloc Error!\n");
            continue;
        }
        memset (_p_task_list, 0, sizeof (TASK_LIST));
        if (strncmp (buffer, "magnet:", strlen ("magnet:")) == 0)
        {
            /*It is a ed2k link */
            _p_task_list->task.task_type = 3;
            strncpy (_p_task_list->task.url, buffer, strlen (buffer));
            sprintf (_p_task_list->task.dir, LOCAL_PATH);
        }
        /*Check if it is a emule seed */
        if (strncmp (buffer, "ed2k://", strlen ("ed2k://")) == 0)
        {
            /*It is a ed2k link */
            _p_task_list->task.task_type = 2;
            strncpy (_p_task_list->task.url, buffer, strlen (buffer));
            sprintf (_p_task_list->task.dir, LOCAL_PATH);
        }
        else if (strncmp (buffer, "bt_seed:", strlen ("bt_seed:")) == 0)
        {
            /*It is a bt link */
            if (buffer[strlen (buffer) - 2] == '\r')
                sub_cha = 2;
            _p_task_list->task.task_type = 1;
            _p_task_list->task.is_default_mode = 1;
            _p_task_list->task.seed_file_path_len =
                strlen (buffer) - strlen ("bt_seed:") - sub_cha;
            strncpy (_p_task_list->task.seed_file_path,
                     buffer + strlen ("bt_seed:"),
                     strlen (buffer) - strlen ("bt_seed:") - sub_cha);
            _p_task_list->task.seed_file_path[_p_task_list->task.
                                              seed_file_path_len] = '\0';
            sprintf (_p_task_list->task.dir, LOCAL_PATH);
        }
        else if (strstr (buffer, "cid=") != NULL)
        {
            /* It is cid task */
            _p_task_list->task.is_cid = 1;
            /*get cid */
            ptr = strstr (buffer, "cid=") + strlen ("cid=");
            strncpy (_p_task_list->task.filename, ptr, 40);
            memset (cid_buffer, 0, sizeof (cid_buffer));
            strncpy (cid_buffer, ptr, 40);
            string_to_cid (cid_buffer, _p_task_list->task.cid);
            /* Get file size */
            memset (cid_buffer, 0, sizeof (cid_buffer));
            ptr = strstr (buffer, "file_size=") + strlen ("file_size=");
            ptr2 = strstr (buffer, "gcid=");
            if (ptr2 != NULL)
            {
                strncpy (cid_buffer, ptr, ptr2 - 1 - ptr);
            }
            else
            {
                strcpy (cid_buffer, ptr);
            }
            _p_task_list->task.filesize = str_to_u64 (cid_buffer);
            /* Get GCID */
            ptr = strstr (buffer, "gcid=");
            if (ptr != NULL)
            {
                //have gcid
                ptr += strlen ("gcid=");
                string_to_cid (ptr, _p_task_list->task.gcid);
                _p_task_list->task.is_gcid = 1;
                _p_task_list->task.is_cid = 0;
            }
            sprintf (_p_task_list->task.dir, LOCAL_PATH);
        }
        else
        {
            /* It is a URL task */
            strncpy (_p_task_list->task.url, buffer, strlen (buffer));
            sprintf (_p_task_list->task.dir, LOCAL_PATH);
        }
        if (_p_task_list_head == NULL)
        {
            _p_task_list_head = _p_task_list;
            _p_task_list_tail = _p_task_list_head;
            _p_task_list_tail->_p_next = NULL;
        }
        else
        {
            _p_task_list_tail->_p_next = _p_task_list;
            _p_task_list_tail = _p_task_list_tail->_p_next;
            _p_task_list_tail->_p_next = NULL;
        }


    }
    fclose (file);
    return _p_task_list_head;
}

int
create_single_task (TASK_PARA * p_task)
{
    if (p_task->task_type == 1)
    {
        /* BT task */
        if (p_task->is_default_mode == 1)
        {
            parse_seed_info (p_task->seed_file_path, p_task);
        }

        //以默认格式输出
        return et_create_bt_task (p_task->seed_file_path,
                                  p_task->seed_file_path_len, p_task->dir,
                                  strlen (p_task->dir),
                                  p_task->download_file_index_array,
                                  p_task->file_num,
                                  ET_ENCODING_DEFAULT_SWITCH,
                                  &p_task->task_id);
    }
    else if (p_task->task_type == 2)
    {
        //emule task
        return et_create_emule_task (p_task->url, strlen (p_task->url),
                                     p_task->dir, strlen (p_task->dir),
                                     NULL, 0, &p_task->task_id);
    }
    else if (p_task->task_type == 3)
    {
        //bt magnet task
        return et_create_bt_magnet_task (p_task->url, strlen (p_task->url),
                                         p_task->dir, strlen (p_task->dir),
										 NULL, 0, 0,
                                         ET_ENCODING_DEFAULT_SWITCH,
                                         &p_task->task_id);
    }
    else if (p_task->is_cid == 0)
    {
        if (p_task->is_created == 0)
        {
            if (p_task->is_gcid == 1)
            {
                return et_create_task_by_tcid_file_size_gcid (p_task->
                        cid,
                        p_task->
                        filesize,
                        p_task->
                        gcid,
                        p_task->
                        filename,
                        strlen
                        (p_task->
                         filename),
                        p_task->
                        dir,
                        strlen
                        (p_task->
                         dir),
                        &(p_task->
                          task_id));

            }
            else
            {
                return et_create_new_task_by_url (p_task->url, strlen (p_task->url),    /* url */
                                                  p_task->refer_url, strlen (p_task->refer_url),  /* ref_url */
                                                  NULL, 0,    /* description */
                                                  p_task->dir, strlen (p_task->dir),  /* file path */
                                                  p_task->filename, strlen (p_task->filename),    /* user file name */
                                                  &(p_task->task_id));
            }
        }
        else
        {
            if (p_task->is_gcid == 1)
            {
                return et_create_task_by_tcid_file_size_gcid (p_task->
                        cid,
                        p_task->
                        filesize,
                        p_task->
                        gcid,
                        p_task->
                        filename,
                        strlen
                        (p_task->
                         filename),
                        p_task->
                        dir,
                        strlen
                        (p_task->
                         dir),
                        &(p_task->
                          task_id));
            }
            else
            {
                return et_create_continue_task_by_url (p_task->url, strlen (p_task->url),   /* url */
                                                       p_task->refer_url, strlen (p_task->refer_url),  /* ref_url */
                                                       NULL, 0,    /* description */
                                                       p_task->dir, strlen (p_task->dir),  /* file path */
                                                       p_task->filename, strlen (p_task->filename),    /* user file name */
                                                       &(p_task->task_id));
            }
        }
    }
    else
    {
        if (p_task->is_created == 0)
            return et_create_new_task_by_tcid (p_task->cid,
                                               p_task->filesize,
                                               p_task->filename,
                                               strlen (p_task->filename),
                                               p_task->dir,
                                               strlen (p_task->dir),
                                               &(p_task->task_id));
        else
            return et_create_continue_task_by_tcid (p_task->cid,
                                                    p_task->filename,
                                                    strlen (p_task->
                                                            filename),
                                                    p_task->dir,
                                                    strlen (p_task->dir),
                                                    &(p_task->task_id));
    }

}
int
create_tasks (TASK_LIST * _p_task_list,
              TASK_LIST ** pp_task_list_next_wait_create)
{
    int ret_val = 0;

    while (_p_task_list)
    {
        ret_val = create_single_task (&(_p_task_list->task));
        /* Check the result of creating task */
        if ((ret_val != 0) && (ret_val != 4103))
        {
            printf (" create_new_task  error, ret = %d \n", ret_val);
            return -1;
        }
        else if (ret_val == 4103)   //TM_ERR_TASK_FULL
        {
            printf
            (" Warning: TM_ERR_TASK_FULL,Too many tasks,will be created later! \n");
            *pp_task_list_next_wait_create = _p_task_list;
            break;
        }
        _p_task_list = _p_task_list->_p_next;
    }

    return 0;
}

int
start_tasks (TASK_LIST * _p_task_list, int *p_running_task_num)
{
    int ret_val = 0;
    while ((_p_task_list) && (_p_task_list->task.task_id != 0))
    {
#ifdef VOD_READ_TEST
        et_set_task_no_disk (_p_task_list->task.task_id);
#else
        //              et_set_task_no_disk(_p_task_list->task.task_id);
#endif
        //             et_vod_set_task_check_data(_p_task_list->task.task_id);
        ret_val = et_start_task (_p_task_list->task.task_id);
        if (ret_val != 0)
        {
            printf (" start_task %u, error, ret = %d \n",
                    _p_task_list->task.task_id, ret_val);
            return -1;
        }
        _p_task_list = _p_task_list->_p_next;
        (*p_running_task_num)++;
    }
    return 0;
}

void
display_task_informations (TASK_PARA * p_task, ET_TASK * info)
{
    int ret_val = 0, file_count = 0;
    ET_BT_FILE file_info;
#ifdef _CONNECT_DETAIL
    int pipe_info_num = 0;
    ET_PEER_PIPE_INFO_ARRAY *p_peer_pipe_info_array = NULL;

    ET_PEER_PIPE_INFO *p_peer_pipe_info = NULL;
#ifdef WINCE
    printf("\ntask id:%u,status=%d, dl_speed=%d, vv_speed=%u ,task_dl_speed=%d( %d,%d,%d),ul_speed=%d(%d), progress=%d, filesize= %I64d,"
           "dl_size= %I64d,dl_progress=%.2f,  written_size= %I64d, written_progress=%.2f, file_status=%d, cm_level:%d\n",
           info->_task_id, info->_task_status, et_get_current_download_speed(), info->_valid_data_speed, info->_speed, info->_server_speed,
           info->_peer_speed, info->_bt_dl_speed, info->_ul_speed, info->_bt_ul_speed, info->_progress, info->_file_size, info->_downloaded_data_size,
           (float) ((((long double) info->_downloaded_data_size) * 100) / ((long double) info->_file_size + 1)), info->_written_data_size,
           (float) (((long double) info->_written_data_size * 100) / ((long double) info->_file_size + 1)), info->_file_create_status, info->_cm_level);
#if WIN32_DEBUG
    printf
    ("pipe info: pipes=(sw:%d, sc:%d, pw:%d, pc:%d), tcp pipe:%d, udp pipe:%d, tcp_speed:%d,udp_speed:%d\n",
     info->_dowing_server_pipe_num, info->_connecting_server_pipe_num,
     info->_dowing_peer_pipe_num, info->_connecting_peer_pipe_num,
     info->_downloading_tcp_peer_pipe_num,
     info->_downloading_udp_peer_pipe_num,
     info->_downloading_tcp_peer_pipe_speed,
     info->_downloading_udp_peer_pipe_speed);
    printf
    ("resource info: server resources num(  idle:%3u,  using:%3u,  candidate:%3u,  retry:%3u,  discard:%3u,  destroy:%3u)\n",
     info->_idle_server_res_num, info->_using_server_res_num,
     info->_candidate_server_res_num, info->_retry_server_res_num,
     info->_discard_server_res_num, info->_destroy_server_res_num);

    printf
    ("                 peer resources num(  idle:%3u,  using:%3u,  candidate:%3u,  retry:%3u,  discard:%3u,  destroy:%3u)\n",
     info->_idle_peer_res_num, info->_using_peer_res_num,
     info->_candidate_peer_res_num, info->_retry_peer_res_num,
     info->_discard_peer_res_num, info->_destroy_peer_res_num);

    p_peer_pipe_info_array =
        (ET_PEER_PIPE_INFO_ARRAY *) & info->_peer_pipe_info_array;
    for (pipe_info_num = 0;
         pipe_info_num < p_peer_pipe_info_array->_pipe_info_num;
         pipe_info_num++)
    {
        p_peer_pipe_info =
            &p_peer_pipe_info_array->_pipe_info_list[pipe_info_num];
        printf
        ("peer pipe info( ex_ip:%15s, in_ip:%15s, peer_id:%15s, connect_type:%1u, dl_speed:%3u,",
         p_peer_pipe_info->_external_ip, p_peer_pipe_info->_internal_ip,
         p_peer_pipe_info->_peerid, p_peer_pipe_info->_connect_type,
         p_peer_pipe_info->_speed);
        printf ("ul_speed:%3u, ", p_peer_pipe_info->_upload_speed);
        printf ("peer pipe info(score:%3u, state:%1u )\n",
                p_peer_pipe_info->_score, p_peer_pipe_info->_pipe_state);

    }
#endif
#else
    printf
    ("\ntask id:%u,status=%d, dl_speed=%d, vv_speed=%u ,task_dl_speed=%d( %d,%d,%d),ul_speed=%d(%d), progress=%d,\nfilesize=%llu, dl_size=%llu,dl_progress=%.2f,  written_size=%llu, written_progress=%.2f, file_status=%d, cm_level:%d\n",
     info->_task_id, info->_task_status, et_get_current_download_speed (),
     info->_valid_data_speed, info->_speed, info->_server_speed,
     info->_peer_speed, info->_bt_dl_speed, info->_ul_speed,
     info->_bt_ul_speed, info->_progress, info->_file_size,
     info->_downloaded_data_size,
     (float) ((((long double) info->_downloaded_data_size) * 100) /
              ((long double) info->_file_size + 1)),
     info->_written_data_size,
     (float) (((long double) info->_written_data_size * 100) /
              ((long double) info->_file_size + 1)),
     info->_file_create_status, info->_cm_level);
    printf
    ("pipe info: pipes=(sw:%d, sc:%d, pw:%d, pc:%d), tcp pipe:%d, udp pipe:%d, tcp_speed:%d,udp_speed:%d\n",
     info->_dowing_server_pipe_num, info->_connecting_server_pipe_num,
     info->_dowing_peer_pipe_num, info->_connecting_peer_pipe_num,
     info->_downloading_tcp_peer_pipe_num,
     info->_downloading_udp_peer_pipe_num,
     info->_downloading_tcp_peer_pipe_speed,
     info->_downloading_udp_peer_pipe_speed);
    printf
    ("resource info: server resources num(  idle:%3u,  using:%3u,  candidate:%3u,  retry:%3u,  discard:%3u,  destroy:%3u)\n",
     info->_idle_server_res_num, info->_using_server_res_num,
     info->_candidate_server_res_num, info->_retry_server_res_num,
     info->_discard_server_res_num, info->_destroy_server_res_num);

    printf
    ("                 peer resources num(  idle:%3u,  using:%3u,  candidate:%3u,  retry:%3u,  discard:%3u,  destroy:%3u)\n",
     info->_idle_peer_res_num, info->_using_peer_res_num,
     info->_candidate_peer_res_num, info->_retry_peer_res_num,
     info->_discard_peer_res_num, info->_destroy_peer_res_num);

#ifndef VOD_READ_TEST
    p_peer_pipe_info_array =
        (ET_PEER_PIPE_INFO_ARRAY *) & info->_peer_pipe_info_array;
    for (pipe_info_num = 0;
         pipe_info_num < p_peer_pipe_info_array->_pipe_info_num;
         pipe_info_num++)
    {
        p_peer_pipe_info =
            &p_peer_pipe_info_array->_pipe_info_list[pipe_info_num];
        printf
        ("peer pipe info( ex_ip:%15s, in_ip:%15s, peer_id:%15s, connect_type:%1u, speed:%3u, score:%3u, state:%1u )\n",
         p_peer_pipe_info->_external_ip, p_peer_pipe_info->_internal_ip,
         p_peer_pipe_info->_peerid, p_peer_pipe_info->_connect_type,
         p_peer_pipe_info->_speed, p_peer_pipe_info->_score,
         p_peer_pipe_info->_pipe_state);
    }
#endif

#endif
#else
    printf
    ("\ntask id: %u, task_status=%d, download_speed=%7u,upload_speed=%7u,task_dl_speed=%7u (%7u,%7u),ul_speed=%7u,task_progress=%3u,pipes=(%3u,%3u,%3u,%3u), filesize=%llu, dl_size=%llu, file_creating_status=%d\n",
     info->_task_id, info->_task_status, et_get_current_download_speed (),
     et_get_current_upload_speed (), info->_speed, info->_server_speed,
     info->_peer_speed, info->_ul_speed, info->_progress,
     info->_dowing_server_pipe_num, info->_connecting_server_pipe_num,
     info->_dowing_peer_pipe_num, info->_connecting_peer_pipe_num,
     info->_file_size, info->_downloaded_data_size,
     info->_file_create_status);

#endif
    fflush (stdout);
    if (p_task->task_type == 1)
    {
        printf
        ("    file info:(file_index,       size, progress,    dl_size, written_size, file_status, query_result, sub_task_err_code, has_record, accelerate_state)\n");
        for (; file_count < p_task->file_num; file_count++)
        {
            ret_val =
                et_get_bt_file_info (p_task->task_id,
                                     p_task->
                                     download_file_index_array
                                     [file_count], &file_info);
            if (ret_val)
                break;
            printf
            ("    file info: %-10u, %-10llu, %-8u, %-10llu, %-12llu, %-11u, %-12u, %-17u ,%-10u, %u)\n",
             file_info._file_index, file_info._file_size,
             file_info._file_percent, file_info._downloaded_data_size,
             file_info._written_data_size, file_info._file_status,
             file_info._query_result, file_info._sub_task_err_code,
             file_info._has_record, file_info._accelerate_state);
            fflush (stdout);
        }
    }
}

#ifdef _CONNECT_DETAIL
void
display_upoad_pipe_informations (ET_PEER_PIPE_INFO_ARRAY *
                                 p_upload_pipe_info_array)
{
    int pipe_info_num = 0;
    ET_PEER_PIPE_INFO *p_peer_pipe_info = NULL;
    if (p_upload_pipe_info_array->_pipe_info_num > 0)
    {
        printf ("\n");
        for (pipe_info_num = 0;
             pipe_info_num < p_upload_pipe_info_array->_pipe_info_num;
             pipe_info_num++)
        {
            p_peer_pipe_info =
                &p_upload_pipe_info_array->_pipe_info_list[pipe_info_num];
            printf
            (" upload pipe info(ex_ip:%15s:%u,in_ip:%15s:[%u,%u],peer_id:%s,connect_type:%1u,speed:%3u,up speed:%3u,score:%3u,state:%1u)\n",
             p_peer_pipe_info->_external_ip,
             p_peer_pipe_info->_external_port,
             p_peer_pipe_info->_internal_ip,
             p_peer_pipe_info->_internal_tcp_port,
             p_peer_pipe_info->_internal_udp_port,
             p_peer_pipe_info->_peerid,
             p_peer_pipe_info->_connect_type,
             p_peer_pipe_info->_speed,
             p_peer_pipe_info->_upload_speed,
             p_peer_pipe_info->_score, p_peer_pipe_info->_pipe_state);
        }
    }
    return;
}
#endif

int
parse_seed_info (char *seed_path, TASK_PARA * p_task_para)
{
    int ret_val = 0, i = 0;
    unsigned char info_hash_hex[20 * 2 + 1];
    ET_TORRENT_SEED_INFO *_p_torrent_seed_info = NULL;
    ET_TORRENT_FILE_INFO *file_info_array_ptr = NULL;
    const char *padding_str = "_____padding_file";

    et_set_seed_switch_type (2);
    /* Get the torrent seed file information from seed_path and store the information in the struct: pp_seed_info */
    ret_val = et_get_torrent_seed_info (seed_path, &_p_torrent_seed_info);
    if (ret_val)
    {
        printf
        ("Error when geting torrent seed information ! err_code:%d\n",
         ret_val);
        return ret_val;
    }
    if (p_task_para != NULL)
    {
        p_task_para->file_num = 0;
    }

    str_to_hex ((char *) _p_torrent_seed_info->_info_hash, 20,
                (char *) info_hash_hex, 20 * 2);
    info_hash_hex[20 * 2] = '\0';

    /************************* Display the torrent seed information to the screen *********************************/
    printf
    ("\n******************* Torrent seed information ******************\n");
    printf (" _title_name=%s\n", _p_torrent_seed_info->_title_name);
    printf (" _title_name_len=%u\n", _p_torrent_seed_info->_title_name_len);
    printf (" _file_total_size=%llu\n",
            _p_torrent_seed_info->_file_total_size);
    printf (" _file_num=%u\n", _p_torrent_seed_info->_file_num);
    printf (" _encoding=%u\n", _p_torrent_seed_info->_encoding);
    printf (" _info_hash=%s\n", info_hash_hex);

    for (i = 0; i < _p_torrent_seed_info->_file_num; i++)
    {

		file_info_array_ptr = _p_torrent_seed_info->file_info_array_ptr[i];

        printf ("\n file_info_array_ptr[%d]->_file_index=%u\n", i,
                file_info_array_ptr->_file_index);
        printf (" file_info_array_ptr[%d]->_file_name=%s\n", i,
                file_info_array_ptr->_file_name);
        printf (" file_info_array_ptr[%d]->_file_name_len=%u\n", i,
                file_info_array_ptr->_file_name_len);
        printf (" file_info_array_ptr[%d]->_file_path=%s\n", i,
                file_info_array_ptr->_file_path);
        printf (" file_info_array_ptr[%d]->_file_path_len=%u\n", i,
                file_info_array_ptr->_file_path_len);
        printf (" file_info_array_ptr[%d]->_file_offset=%llu\n", i,
                file_info_array_ptr->_file_offset);
        printf (" file_info_array_ptr[%d]->_file_size=%llu\n", i,
                file_info_array_ptr->_file_size);

        if (file_info_array_ptr->_file_size > MIN_BT_FILE_SIZE
            && p_task_para != NULL
            && p_task_para->file_num < MAX_BT_RUN_FILE_NUM
            && strncmp (file_info_array_ptr->_file_name, padding_str,
                        strlen (padding_str)))
        {
            p_task_para->download_file_index_array[p_task_para->
                                                   file_num] =
                                                           file_info_array_ptr->_file_index;
            p_task_para->file_num++;
        }
        file_info_array_ptr++;
    }

    printf
    ("\n******************* End of torrent seed information ******************\n\n");

    /************************* End of display the torrent seed information to the screen *********************************/
    /* Release the memory in the struct:p_seed_info which malloc by  et_get_torrent_seed_info */
    et_release_torrent_seed_info (_p_torrent_seed_info);
    return 0;
}

/*
   Convert the string(40 chars hex ) to content id(20 bytes in hex)
 */
int
string_to_cid (char *str, unsigned char *cid)
{
    int i = 0, is_cid_ok = 0, cid_byte = 0;

    if ((str == NULL) || (cid == NULL))
        return -1;

    for (i = 0; i < 20; i++)
    {
        if (((str[i * 2]) >= '0') && ((str[i * 2]) <= '9'))
        {
            cid_byte = (str[i * 2] - '0') * 16;
        }
        else if (((str[i * 2]) >= 'A') && ((str[i * 2]) <= 'F'))
        {
            cid_byte = ((str[i * 2] - 'A') + 10) * 16;
        }
        else if (((str[i * 2]) >= 'a') && ((str[i * 2]) <= 'f'))
        {
            cid_byte = ((str[i * 2] - 'a') + 10) * 16;
        }
        else
            return -1;

        if (((str[i * 2 + 1]) >= '0') && ((str[i * 2 + 1]) <= '9'))
        {
            cid_byte += (str[i * 2 + 1] - '0');
        }
        else if (((str[i * 2 + 1]) >= 'A') && ((str[i * 2 + 1]) <= 'F'))
        {
            cid_byte += (str[i * 2 + 1] - 'A') + 10;
        }
        else if (((str[i * 2 + 1]) >= 'a') && ((str[i * 2 + 1]) <= 'f'))
        {
            cid_byte += (str[i * 2 + 1] - 'a') + 10;
        }
        else
            return -1;

        cid[i] = cid_byte;
        if ((is_cid_ok == 0) && (cid_byte != 0))
            is_cid_ok = 1;
    }

    if (is_cid_ok == 1)
        return 0;
    else
        return 1;


}

unsigned long long
str_to_u64 (char *str)
{
    unsigned long long _result = 0;
    int j, len = 0;
    if ((str == NULL) || (strlen (str) < 1))
        return -1;
    len = strlen (str);
    if (str[len - 1] == '\n')
        len--;
    for (j = 0; j < len; j++)
    {
        if ((str[j] < '0') || (str[j] > '9'))
        {
            return -1;
        }
        else
        {
            _result *= 10;
            _result += str[j] - '0';

        }
    }
    return _result;
}

void
str_to_hex (char *in_buf, int in_bufsize, char *out_buf, int out_bufsize)
{
    int idx = 0;
    unsigned char *inp = (unsigned char *) in_buf, ch;

    for (idx = 0; idx < in_bufsize; idx++)
    {
        if (idx * 2 >= out_bufsize)
            break;

        ch = inp[idx] >> 4;
        out_buf[idx * 2] = INT_TO_HEX (ch);
        ch = inp[idx] & 0x0F;
        out_buf[idx * 2 + 1] = INT_TO_HEX (ch);
    }
}



#ifdef _SUPPORT_ZLIB
//这个函数是直接zlib库的解压函数,此时必须动态链接libz.so,目的是为了提高emule下载中kad网络的查源数量,
//若不需要提高emule找源的数量,则不需要动态链接libz.so,也不需要此函数,不需要调用et_set_customed_interface(ET_ZLIB_UNCOMPRESS,zlib_uncompress );
int
zlib_uncompress (unsigned char *p_out_buffer, int *p_out_len,
                 const unsigned char *p_in_buffer, int in_len)
{
    int ret = 0;
    ret =
        uncompress ((unsigned char *) p_out_buffer, p_out_len, p_in_buffer,
                    in_len);
    return ret;
}
#endif

#endif /* MOBILE_PHONE  */

