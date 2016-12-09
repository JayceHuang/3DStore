#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <utility/queue.h>
#include <platform/sd_fs.h>
#include "utility/logid.h"
#define LOGID EM_LOGID_DOWNLOAD_TASK
#include "utility/logger.h"

#include "TaskAsyncSave.h"

#ifdef TASK_STORE_ASYNC_PERSISTENT

typedef struct _ASYNC_SAVE_DATA
{
    QUEUE queue;
    pthread_t thread_id;    //ˢ���߳�
    _u32 flush_interval;
    pthread_mutex_t mutex;
    pthread_mutex_t cond_mutex;
    pthread_cond_t cond;
    int exit_flush_thread;
}ASYNC_SAVE_DATA;

typedef struct _SAVE_NODE
{
    _u32 task_id;
    ASYNC_OP op;
    char szTaskFileName[256];
    unsigned char *data;
    _u32 data_len;
}SAVE_NODE;

#define MAX_SAVE_QUEUE_SIZE 20
static ASYNC_SAVE_DATA g_save_data;
static BOOL g_save_data_has_inited = FALSE;


static void notify_flush();
static void* flush_thread(void* user_data);
static _int32 create_flush_thread();
static void flush_buffer_to_file();
#endif

static _int32 flush_delete_file(_u32 task_id, const char *cszFileName);
static _int32 flush_write_file(_u32 task_id, const char *cszTaskFileName, const unsigned char *data, _int32 data_len);

_int32 init_task_async_save()
{
    LOG_DEBUG("init_task_async_save");
#ifdef TASK_STORE_ASYNC_PERSISTENT
    g_save_data.exit_flush_thread        = 0;
    g_save_data.flush_interval = 10;
    pthread_mutex_init(&g_save_data.mutex, NULL);
    pthread_mutex_init(&g_save_data.cond_mutex, NULL);
    pthread_cond_init(&g_save_data.cond, NULL);

    _int32 ret_val = queue_init( &g_save_data.queue, MAX_SAVE_QUEUE_SIZE);
    create_flush_thread();
    
    g_save_data_has_inited = TRUE;
#endif;
    return SUCCESS;
}

_int32 uninit_task_async_save()
{
    LOG_DEBUG("uninit_task_async_save");
#ifdef TASK_STORE_ASYNC_PERSISTENT
    
    if (!g_save_data_has_inited) return SUCCESS;
    g_save_data.exit_flush_thread = 1;
    notify_flush();
    pthread_join(g_save_data.thread_id, NULL);
    //printf("slog flush thread exit.\n");

    pthread_mutex_destroy(&g_save_data.mutex);
    pthread_mutex_destroy(&g_save_data.cond_mutex);
    pthread_cond_destroy(&g_save_data.cond);

    queue_uninit( &g_save_data.queue );

    g_save_data_has_inited = FALSE;
#endif
    return SUCCESS;
}

_int32 output_task_to_file(_u32 task_id, ASYNC_OP op, const char *cszTaskFileName, unsigned char *data, _u32 data_len)
{
    _int32 ret_val = SUCCESS;
#ifdef TASK_STORE_ASYNC_PERSISTENT

    SAVE_NODE *pTempNode = NULL;
    QUEUE_NODE *queue_node;
    LOG_DEBUG("output_task_to_file task_id:%u, action:%d, file_name:%s, data_len:%u", task_id, op, cszTaskFileName==NULL?"":cszTaskFileName, data_len);
        
    pthread_mutex_lock(&g_save_data.mutex);

    BOOL bMergeToOne = FALSE;
    queue_node = g_save_data.queue._queue_head->_nxt_node;
    while (queue_node != g_save_data.queue._queue_tail)
    {
        pTempNode = (SAVE_NODE *)queue_node->_data;
        if (pTempNode && pTempNode->task_id == task_id)
        {
            if ( op==ASYNC_OP_DELETE || op==pTempNode->op)
            {
                pTempNode->op = op;
                free(pTempNode->data);
                pTempNode->data = NULL;
                pTempNode->data_len = 0;
                if (op!=ASYNC_OP_DELETE)
                {
                    pTempNode->data = data;
                    pTempNode->data_len = data_len;
                }
                bMergeToOne = TRUE;
                LOG_DEBUG("output_task_to_file: replace a node in queue,  task_id:%u, action:%d, file_name:%s, data_len:%u", task_id, op, cszTaskFileName==NULL?"":cszTaskFileName, data_len);
                break;
            }
        }
        queue_node = queue_node->_nxt_node;
    }
    if (!bMergeToOne)
    {
        SAVE_NODE *pNode = NULL;
        ret_val = sd_malloc(sizeof(SAVE_NODE), (void **)&pNode);
        pNode->task_id = task_id;
        pNode->op = op;
        sd_strncpy(pNode->szTaskFileName, cszTaskFileName, sd_strlen(cszTaskFileName)+1);
        pNode->data = data;
        pNode->data_len = data_len;
    
        ret_val = queue_push(&g_save_data.queue, (void *)pNode);
        LOG_DEBUG("output_task_to_file: push into queue,  task_id:%u, action:%d, file_name:%s, data_len:%u", task_id, op, cszTaskFileName==NULL?"":cszTaskFileName, data_len);
    }
    
    pthread_mutex_unlock(&g_save_data.mutex);
    notify_flush();
#else
    char file_full_name[MAX_FULL_PATH_LEN];
    char * p_backup_file_full_name;
    if(0xFFFFFFFF==task_id)
	{
		p_backup_file_full_name=dt_get_task_store_backup_file_path(FALSE);
	}
	else
	{
		file_full_name[sizeof(file_full_name)-1]=0;
		dt_get_task_alone_store_backup_file_path_by_task_id(task_id,file_full_name,sizeof(file_full_name));
		p_backup_file_full_name=file_full_name;
	}
    if (op==ASYNC_OP_DELETE)
    {
        ret_val = flush_delete_file(task_id, cszTaskFileName);
		flush_delete_file(task_id,p_backup_file_full_name);
    }
    else if (op==ASYNC_OP_WRITE)
    {
        ret_val = flush_write_file(task_id, cszTaskFileName, data, data_len);
		flush_write_file(task_id,p_backup_file_full_name,data,data_len);
    }
#endif
    return ret_val;
}

#ifdef TASK_STORE_ASYNC_PERSISTENT
static void notify_flush()
{
	//printf("notify_flush\n");
	pthread_mutex_lock(&g_save_data.cond_mutex);
	pthread_cond_signal(&g_save_data.cond);
	pthread_mutex_unlock(&g_save_data.cond_mutex);
	return ;
}

static void* flush_thread(void* user_data)
{
    struct timeval start, last_update;
    struct timespec timeout;
    _u32 pending_count = 0;

    last_update.tv_sec = 0;
    BOOL need_backup = FALSE;
	while (1)
	{
		pthread_mutex_lock(&g_save_data.mutex);
		pending_count = queue_size(&g_save_data.queue);
		pthread_mutex_unlock(&g_save_data.mutex);
		if (pending_count==0)
		{
			if(g_save_data.exit_flush_thread)
				break;
#ifndef DISABLE_TASK_STORE_FILE_BACKUP_TO_SDCARD //����ֹ���ݵ�sdcard
            if (need_backup)
            {
                dt_backup_newest_task_store_file();
                need_backup = FALSE;
            }
#endif //DISABLE_TASK_STORE_FILE_BACKUP_TO_SDCARD //����ֹ���ݵ�sdcard
        
            gettimeofday(&start, NULL);
            if(last_update.tv_sec == 0)
            	last_update.tv_sec = start.tv_sec;

            timeout.tv_sec = start.tv_sec + g_save_data.flush_interval;
            timeout.tv_nsec = start.tv_usec*1000;
            //�ȴ�ʱ����ˢ��֪ͨ
            pthread_mutex_lock(&g_save_data.cond_mutex);
            pthread_cond_timedwait(&g_save_data.cond, &g_save_data.cond_mutex, &timeout);
            pthread_mutex_unlock(&g_save_data.cond_mutex);
            continue;
        }
        //�����������ˢ�µ�task�ļ�
        flush_buffer_to_file();
        need_backup = TRUE;
    }

#ifndef DISABLE_TASK_STORE_FILE_BACKUP_TO_SDCARD //����ֹ���ݵ�sdcard
    if (need_backup)
    {
        dt_backup_newest_task_store_file();
    }
#endif //DISABLE_TASK_STORE_FILE_BACKUP_TO_SDCARD //����ֹ���ݵ�sdcard
    
    return NULL;
}

static _int32 create_flush_thread()
{
    if(pthread_create(&g_save_data.thread_id, NULL, flush_thread, (void*)NULL) != 0)
    {
        LOG_ERROR("ERROR!!! create flush thread failed. exit !!!");
        return -1;
    }
    return SUCCESS;
}
#endif

static _int32 flush_delete_file(_u32 task_id, const char *cszTaskFileName)
{
    LOG_DEBUG("TaskAsyncSave::flush_delete_file delete task file, task_id:%u, file path: %s ", task_id, cszTaskFileName );
    _int32 ret_val = SUCCESS;
    if (ret_val = sd_file_exist(cszTaskFileName))
    {
        ret_val = sd_delete_file(cszTaskFileName);
        sd_assert(ret_val==SUCCESS);
    }
    return ret_val;
}

static _int32 flush_write_file(_u32 task_id, const char *cszTaskFileName, const unsigned char *data, _int32 data_len)
{
    LOG_DEBUG("TaskAsyncSave::flush_write_file write task file, task_id:%u, file path: %s ", task_id, cszTaskFileName);
    _int32 ret_val = SUCCESS;
    const char *pathEnd = strrchr(cszTaskFileName, DIR_SPLIT_CHAR);
    char szPath[MAX_FILE_PATH_LEN] = {0};
    _u32 pathLen = pathEnd - cszTaskFileName;
    sd_strncpy(szPath, cszTaskFileName, pathLen);

    if (sd_is_path_exist(szPath)!=SUCCESS)
    {
        ret_val = sd_mkdir(szPath);
        sd_assert( SUCCESS == ret_val );
        if (SUCCESS != ret_val)
        {
            LOG_DEBUG("TaskAsyncSave::flush_write_file create save path fail : %s", szPath);
            ret_val;
        }
    }

    sprintf(szPath, "%s.tmp", cszTaskFileName); 

    _u32 hFile = 0;
    _u32 writesize = 0;
    ret_val = sd_open_ex(szPath, O_FS_WRONLY|O_FS_CREATE, &hFile);
    sd_assert(hFile);
    if (SUCCESS==ret_val)
    {
        ret_val = sd_write(hFile, (char *)data, data_len, &writesize);

		LOG_DEBUG("liuzongyao hFile:%d data: %s data_len: %d writesize %d ",hFile,data,data_len,writesize);

		
        sd_close_ex(hFile);
        hFile = 0;
        sd_assert(SUCCESS==ret_val && data_len==writesize);
        if (SUCCESS==ret_val && data_len==writesize)
        {
            if (sd_file_exist(cszTaskFileName))
            {
                sd_delete_file(cszTaskFileName);
            }
            ret_val = sd_rename_file(szPath, cszTaskFileName);
        }

    }
    if (SUCCESS != ret_val)
    {
        LOG_DEBUG("TaskAsyncSave::flush_write_file save fail : %s", cszTaskFileName);
    }
    return ret_val;
}

#ifdef TASK_STORE_ASYNC_PERSISTENT
static void flush_buffer_to_file()
{
    //printf("flush to file\n");
    SAVE_NODE *pNode = NULL;
    _int32 ret_val = SUCCESS;
    pthread_mutex_lock(&g_save_data.mutex);
    ret_val = queue_pop(&g_save_data.queue, (void **)&pNode);
    pthread_mutex_unlock(&g_save_data.mutex);

    if (SUCCESS == ret_val && pNode)
    {

        _int32 ret_val = SUCCESS;

        if (pNode->op==ASYNC_OP_DELETE)
        {
            ret_val = flush_delete_file(pNode->task_id, pNode->szTaskFileName);
        }
        else if (pNode->op==ASYNC_OP_WRITE)
        {
            ret_val = flush_write_file(pNode->task_id, pNode->szTaskFileName, pNode->data, pNode->data_len);
        }    
    }

error_handle:
    if (pNode)
    {
        if (pNode->data)
        {
            free(pNode->data);
            pNode->data = NULL;
        }
        sd_free(pNode);
        pNode = NULL;
    }
    
}
#endif
