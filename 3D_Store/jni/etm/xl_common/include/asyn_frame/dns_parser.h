#ifndef _SD_DNS_PARSER_H_200905221723
#define _SD_DNS_PARSER_H_200905221723

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"

/************************************************************************
 *
 * DNS server ip.
 *
 ************************************************************************/

/* 超过(n)个域名无法被解析，则此nameserver视为无效。 */
#define DNS_SERVER_IP_INVALID_SCORE (3)

/* 定期检查nameserver配置文件的时间。 */
#define DNS_SERVER_IP_CHECK_SERVER_IP_INTERVAL (2000)

typedef struct
{
    _int32 _host_name_key[DNS_SERVER_IP_INVALID_SCORE];
    _u32 _count;
} DNS_SERVER_IP_VALIDITY;

typedef struct
{
    _u32 _ip_list[DNS_SERVER_IP_COUNT_MAX];
    DNS_SERVER_IP_VALIDITY _validity[DNS_SERVER_IP_COUNT_MAX];
    _u32 _ip_count;
    _u64 _file_size;
    _u32 _file_last_modified_time;
    _u32 _last_check_file_attrib_time;
} DNS_SERVER_IP;

/************************************************************************
 *
 * DNS content.
 *
 ************************************************************************/

typedef struct
{
    char _host_name[MAX_HOST_NAME_LEN];
    _u32 _host_name_buffer_len;
    _u32 _ip_list[DNS_CONTENT_IP_COUNT_MAX];
    _u32 _ttl[DNS_CONTENT_IP_COUNT_MAX];    /* TTL，降序排列。 */
    _u32 _start_time;   /* 获取到TTL时的时间戳。 */
    _u32 _ip_count;
    _u32 _result;
} DNS_CONTENT_PACKAGE;

/************************************************************************
 *
 * DNS code package.
 *
 ************************************************************************/

#define DNS_CODE_SIZE_MAX (1024)

typedef struct
{
    char _dns_code[DNS_CODE_SIZE_MAX];
    _u32 _len;
}DNS_CODE_PACKAGE;

/************************************************************************
 *
 * DNS cache.
 *
 ************************************************************************/

/* cache size. */
#define DNS_CHCHE_SIZE_MAX (29)

/* 解析错误的DNS在CACHE中的TTL. */
#define DNS_CHCHE_INVAIL_DNS_TTL (5000)

typedef struct
{
    _u32 _index[DNS_CHCHE_SIZE_MAX];
    _u32 _conflict_next_index[DNS_CHCHE_SIZE_MAX];
}DNS_CACHE_HASH_TABLE;

typedef struct
{
    _u32 _prior_index[DNS_CHCHE_SIZE_MAX];
    _u32 _next_index[DNS_CHCHE_SIZE_MAX];
    _u32 _min_index;
    _u32 _max_index;
}DNS_CACHE_PRIORITY_LIST;

typedef struct
{
    DNS_CONTENT_PACKAGE _dns_content[DNS_CHCHE_SIZE_MAX];
    DNS_CACHE_PRIORITY_LIST _pri_lru;
    DNS_CACHE_PRIORITY_LIST _pri_ttl;
    DNS_CACHE_HASH_TABLE _hash_table;
    _u32 _size;
}DNS_CACHE;

/* 初始化
 * @return:
 *	   SUCCESS - 成功。
 */
_int32 dns_cache_init(DNS_CACHE *dns_cache);

/* 反初始化
 * @return:
 *	   SUCCESS - 成功。
 */
_int32 dns_cache_uninit(DNS_CACHE *dns_cache);

/* 清除cache
 * @return:
 *	  SUCCESS - 成功。
 */
_int32 dns_cache_clear(DNS_CACHE *dns_cache);

/* 查询cache
 * @return:
 *	  SUCCESS: 成功。
 *	  -1: 没找到。
 */
_int32 dns_cache_query(DNS_CACHE *dns_cache, char *host_name, DNS_CONTENT_PACKAGE *dns_content);

/* 新增查询结果到cache
 * @return:
 *	  SUCCESS - 成功。
 */
_int32 dns_cache_append(DNS_CACHE *dns_cache, DNS_CONTENT_PACKAGE *dns_content);


/************************************************************************
 *
 * DNS request queue.
 * 用于保存已发出request，但未收到answer的DNS msg。
 *
 ************************************************************************/

/* request queue size. */
#define DNS_REQUEST_QUEUE_SIZE_MAX (29)

/* retry times after request fail or expired. */
#define DNS_REQUEST_RETRY_TIMES (1)

/* request ttl. */
#define DNS_REQUEST_TTL (5000)

typedef struct
{
    _u32 _dns_id;
    char _host_name[MAX_HOST_NAME_LEN];
    _u32 _host_name_len;
    _u32 _request_time;
    _u32 _retry_times;
    _u32 _ttl;
    _u32 _dns_server_ip_index;  /* 当index值为 DNS_SERVER_IP_COUNT_MAX 时，表示此请求的nameserver已失效。 */
    void *_data;
}DNS_REQUEST_RECORD;

typedef struct
{
    _u32 _head;
    _u32 _tail;
    _u32 _prior_index[DNS_REQUEST_QUEUE_SIZE_MAX];
    _u32 _next_index[DNS_REQUEST_QUEUE_SIZE_MAX];
}DNS_REQUEST_EXPIRED_LIST;

typedef struct
{
    DNS_REQUEST_RECORD _record[DNS_REQUEST_QUEUE_SIZE_MAX];
    DNS_REQUEST_EXPIRED_LIST _exp_list;
    DNS_SERVER_IP _dns_server_ip;
    _u32 _cur_dns_id;
    _u32 _size;
}DNS_REQUEST_QUEUE;

/************************************************************************
 *
 * DNS PARSER (platform)
 *
 ************************************************************************/

typedef struct
{   
    DNS_REQUEST_QUEUE _dns_request_queue;
    DNS_CACHE _dns_cache;
    _u32 sock;
}DNS_PARSER;

_int32 dns_parser_init(DNS_PARSER *dns_parser);
_int32 dns_parser_uninit(DNS_PARSER *dns_parser); 

/*
 * 当前DNS解析器是否就绪。
 *
 * @arguments:
 *  [out]is_ready:
 *      TRUE - 就绪。
 *      FALSE - 暂时无法处理新的解析请求。
 *
 * @return:
 *      NOT SUCCESS - 异常。
 */
_int32 dns_parser_is_ready(DNS_PARSER * dns_parser, BOOL *is_ready);

/*
 * 查询DNS。
 * 如果本函数未返回成功，需调用 dns_parser_get 函数异步获取结果。
 *
 * @argments:
 *  [out]query_result:
 *      DNS_QUERY_RESULT_OK - 已同步返回解析结果。
 *      DNS_QUERY_RESULT_ASYNC_REQUEST - 未同步得到解析结果，但已发出异步请求。
 *  [in]pdata:
 *      调用异步获取DNS函数时，可获取到此参数值。
 *  [out]content:
 *      同步返回的解析结果。
 *
 * @return:
 *      NOT SUCCESS - 异常。
 */

#define DNS_QUERY_RESULT_OK                 (0)
#define DNS_QUERY_RESULT_ASYNC_REQUEST      (1)

_int32 dns_parser_query(DNS_PARSER *dns_parser, char *host_name, _u32 host_name_len, _int32 *query_result,
    void *pdata, DNS_CONTENT_PACKAGE *content);

/*
 * 异步获取DNS解析结果。
 *
 * @argments:
 *  [out]ack_result:
 *      DNS_ACK_RESULT_OK - 已返回解析结果。
 *      DNS_ACK_RESULT_NOT_GET_ANSWER - 本次调用未获取到解析结果。
 *  [out]ppdata:
 *      查询DNS 函数中传入的值。
 *  [out]content:
 *      同步返回的解析结果。
 *
 * @return:
 *      NOT SUCCESS - 异常。
 */

#define DNS_ACK_RESULT_OK                   (0)
#define DNS_ACK_RESULT_NOT_GET_ANSWER       (1)

_int32 dns_parser_get(DNS_PARSER * dns_parser, _int32 *ack_result, void **ppdata, DNS_CONTENT_PACKAGE *content);

/*
 * 取消DNS请求。
 */
_int32 dns_parser_query_cancel(DNS_PARSER * dns_parser, _u32 index);

/*
 * 是否所有的DNS请求都已解析完成。
 *
 * @arguments:
 *  [out]is_finished:
 *      TRUE - 所有的DNS请求都已解析完成。
 *      FALSE - 还有DNS请求未解析。
 *
 * @return:
 *      NOT SUCCESS - 异常。
 */
_int32 dns_parser_is_finished(DNS_PARSER * dns_parser, BOOL *is_finished);

/*
 * 队列中未处理的DNS请求个数。
 *
 * @arguments:
 *  [out]count: 未处理的请求个数。
 *
 * @return:
 *      NOT SUCCESS - 异常。
 */
_int32 dns_parser_request_count(DNS_PARSER * dns_parser, _u32 *count);

/*
 * 队列中未处理的DNS请求信息。
 *
 * @arguments:
 *  [in]index:   索引。注意：此索引所指信息不作任何排序。
 *  [out]record: 请求信息记录。
 *
 * @return:
 *      NOT SUCCESS - 异常。
 */
_int32 dns_parser_request_record_const(DNS_PARSER * dns_parser, _u32 index, DNS_REQUEST_RECORD **precord);

/*
 * DNS Server 个数。
 *
 * @arguments:
 *  [out]count: 未处理的请求个数。
 *
 * @return:
 *      NOT SUCCESS - 异常。
 */
_int32 dns_parser_dns_server_count(DNS_PARSER * dns_parser, _u32 *count);

/*
 * DNS Server IP 信息。
 *
 * @arguments:
 *  [in]index:   索引。
 *  [out]dns_server_ip: DNS Server IP 信息。
 *
 * @return:
 *      NOT SUCCESS - 异常。
 */
_int32 dns_parser_dns_server_info(DNS_PARSER * dns_parser, _u32 index, _u32 *dns_server_ip);

#ifdef __cplusplus
}
#endif

#endif
