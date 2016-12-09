
#include "asyn_frame/dns_parser.h"
#include "utility/url.h"
#include "utility/string.h"
#include "utility/utility.h"
#include "utility/errcode.h"
#include "utility/time.h"
#include "platform/sd_customed_interface.h"
#include "platform/sd_socket.h"
#include "platform/sd_task.h"
#include "platform/sd_fs.h"

/* log */
#define LOGID LOGID_DNS_PARSER
#include "utility/logger.h"

/************************************************************************
 *
 * DNS server ip.
 *
 ************************************************************************/

#if defined(MACOS)
#define RESOLV_CONF_PATH "/etc/resolv.conf"
#else
#define RESOLV_CONF_PATH "/etc/resolv.conf"
#endif

_int32 dns_server_ip_load(DNS_SERVER_IP *dns_server_ip)
{
    _int32 ret_val = SUCCESS;
    _u32 file_id;
    char buf[256] = {0};
    char line[256] = {0};
    char tmp[16] = {0};
    _u32 read_size = 0;
    _u32 i = 0, j = 0, k = 0;
    _u32 tmp_ip = 0;
    
    if (dns_server_ip == NULL)
    {
        return INVALID_ARGUMENT;
    }
    sd_memset(dns_server_ip, 0, sizeof(DNS_SERVER_IP));

#ifdef LINUX


    #ifdef MACOS
	/* 以下DNS服务器仅作为测试用 */
    ret_val = sd_time_ms(&dns_server_ip->_last_check_file_attrib_time);
    CHECK_VALUE(ret_val);
   dns_server_ip->_ip_count = 0;

   if(is_available_ci(CI_DNS_GET_DNS_SERVER))
   {
	 ((et_dns_get_dns_server)ci_ptr(CI_DNS_GET_DNS_SERVER))(buf, line);
   }
   else
   {
        //API_LAN_GetDnsServer(buf, line);
        sd_strncpy(buf, "202.96.128.86", sizeof(buf));
        sd_strncpy(line, "202.96.128.166", sizeof(line));
		
   }
   // 1st  广东电信
   ret_val = sd_inet_aton(buf, &tmp_ip);
   if (ret_val == SUCCESS)
   {
        dns_server_ip->_ip_list[dns_server_ip->_ip_count] = tmp_ip;
	 dns_server_ip->_ip_count++;
   }
   // 2nd 广东电信
   ret_val = sd_inet_aton(line, &tmp_ip);
   if (ret_val == SUCCESS)
   {
        dns_server_ip->_ip_list[dns_server_ip->_ip_count] = tmp_ip;
	 dns_server_ip->_ip_count++;
   }

   // 3th google
   ret_val = sd_inet_aton("8.8.8.8", &tmp_ip);
   if (ret_val == SUCCESS)
   {
        dns_server_ip->_ip_list[dns_server_ip->_ip_count] = tmp_ip;
	 dns_server_ip->_ip_count++;
   }

    LOG_DEBUG("load server ip, ip_count=%d, ip[0]=0x%x, ip[1]=0x%x, ip[2]=0x%x",
        dns_server_ip->_ip_count,
        dns_server_ip->_ip_list[0],
        dns_server_ip->_ip_list[1],
        dns_server_ip->_ip_list[2]);	
	
    #else

    /* 检测文件属性。 */
    ret_val = sd_get_file_size_and_modified_time(RESOLV_CONF_PATH,
        &dns_server_ip->_file_size, &dns_server_ip->_file_last_modified_time);
    if (SUCCESS != ret_val)
    {
        LOG_DEBUG("can not get file_size and last_modified_time: /etc/resolv.conf, ret_val=%d", ret_val);
        dns_server_ip->_file_size = 0;
        dns_server_ip->_file_last_modified_time = 0;
        return SUCCESS;
    }
    
    ret_val = sd_time_ms(&dns_server_ip->_last_check_file_attrib_time);
    CHECK_VALUE(ret_val);
    
    /* 打开文件，获取server ip. */
    ret_val = sd_open_ex(RESOLV_CONF_PATH, O_FS_RDONLY, &file_id);
    if (SUCCESS != ret_val)
    {
        LOG_DEBUG("can not open file: /etc/resolv.conf, ret_val=%d", ret_val);
        return SUCCESS;
    }

    while (TRUE)
    {
        sd_memset(buf, 0, 256);
        ret_val = sd_read(file_id, buf, 256, &read_size);
        if (ret_val != SUCCESS)
        {
            break;
        }
        
        if (read_size <= 0)
        {
            break;
        }

        for (i = 0; i < read_size; ++i)
        {
            if (buf[i] != '\n')
            {
                line[j] = buf[i];
                j ++;
            }
            else
            {
                line[j] = '\0';
                j = 0;

                /* 去掉行头的空格、'\t'。 */
                sd_trim_prefix_lws(line);

                sd_strncpy(tmp, line, 10);
                tmp[10] = 0;
                if (sd_stricmp(tmp, "nameserver") == 0)
                {
                    sd_trim_prefix_lws(line + 10);
                    sd_trim_postfix_lws(line + 10);
                    ret_val = sd_inet_aton(line + 10, &tmp_ip);
                    if (ret_val == SUCCESS)
                    {
                        /* 去掉重复。 */
                        for (k = 0; k < dns_server_ip->_ip_count; ++k)
                        {
                            if (dns_server_ip->_ip_list[k] == tmp_ip)
                            {
                                break;
                            }
                        }
                        
                        if (k == dns_server_ip->_ip_count)
                        {
                            dns_server_ip->_ip_list[dns_server_ip->_ip_count] = tmp_ip;
                            dns_server_ip->_ip_count++;
                            if (dns_server_ip->_ip_count >= DNS_SERVER_IP_COUNT_MAX)
                            {
                                LOG_DEBUG("load server ip, ip_count=%d, ip[0]=0x%x, ip[1]=0x%x, ip[2]=0x%x",
                                    dns_server_ip->_ip_count,
                                    dns_server_ip->_ip_list[0],
                                    dns_server_ip->_ip_list[1],
                                    dns_server_ip->_ip_list[2]);
                                
                                ret_val = sd_close_ex(file_id);
                                CHECK_VALUE(ret_val);
                                return SUCCESS;
                            }
                        }
                    }
                }
            }
        }
        
        if (read_size < 256)
        {
            line[j] = '\0';
            break;
        }
    }
    
    LOG_DEBUG("load server ip, ip_count=%d, ip[0]=0x%x, ip[1]=0x%x, ip[2]=0x%x",
        dns_server_ip->_ip_count,
        dns_server_ip->_ip_list[0],
        dns_server_ip->_ip_list[1],
        dns_server_ip->_ip_list[2]);

    ret_val = sd_close_ex(file_id);
    CHECK_VALUE(ret_val);
    #endif
#endif

    return SUCCESS;
}

/* 是否需要重读dns server ip的配置? */
BOOL dns_server_ip_need_reload(DNS_SERVER_IP *dns_server_ip)
{
    _int32 ret_val = 0;
    _u32 now_stamp = 0;
    _u64 file_size = 0;
    _u32 file_last_modified_time = 0;

    if (dns_server_ip == NULL)
    {
        sd_assert(FALSE);
        return FALSE;
    }

    /* 无SERVER IP. */
    if (dns_server_ip->_ip_count == 0)
    {
        return TRUE;
    }

    /* 定时检查。 */
    ret_val = sd_time_ms(&now_stamp);
    CHECK_VALUE(ret_val);
    
    if (TIME_GT(dns_server_ip->_last_check_file_attrib_time + DNS_SERVER_IP_CHECK_SERVER_IP_INTERVAL, now_stamp))
    {
        return FALSE;
    }

    dns_server_ip->_last_check_file_attrib_time = now_stamp;

#ifdef LINUX
    ret_val = sd_get_file_size_and_modified_time("/etc/resolv.conf", &file_size, &file_last_modified_time);
    if (SUCCESS != ret_val)
    {
        /* 可能在解析的过程中，删除了"/etc/resolv.conf"文件。 */
        return TRUE;
    }
#endif

    if (file_size != dns_server_ip->_file_size
        || file_last_modified_time != dns_server_ip->_file_last_modified_time)
    {
        return TRUE;
    }

    return FALSE;
}

#define DNS_SERVER_IP_FIRST_GET DNS_SERVER_IP_COUNT_MAX
#define DNS_SERVER_IP_ERROR_NO_SERVER (-1)
#define DNS_SERVER_IP_ERROR_NO_VALID_SERVER (-2)

/* 获取下一个可用的dns server ip index。 */
_int32 dns_server_ip_get_next(DNS_SERVER_IP *dns_server_ip, _u32 cur_index, _u32 *retry_times, _u32 *next_index)
{
    if (dns_server_ip == NULL || next_index == NULL)
    {
        return INVALID_ARGUMENT;
    }

    if (dns_server_ip->_ip_count == 0)
    {
        return DNS_SERVER_IP_ERROR_NO_SERVER;
    }

    if (cur_index >= DNS_SERVER_IP_FIRST_GET)
    {
        *retry_times = 0;
        *next_index = 0;
    }
    else
    {
        *next_index = cur_index + 1;
    }

    for (; (*next_index) < dns_server_ip->_ip_count && (*next_index) < DNS_SERVER_IP_COUNT_MAX; (*next_index)++)
    {
        if (dns_server_ip->_validity[*next_index]._count < DNS_SERVER_IP_INVALID_SCORE)
        {
            return SUCCESS;
        }
    }

    /* no found, retry. */
    if (*retry_times < DNS_REQUEST_RETRY_TIMES)
    {
        (*retry_times) ++;
        *next_index = 0;
        for (; (*next_index) < dns_server_ip->_ip_count; (*next_index)++)
        {
            if (dns_server_ip->_validity[*next_index]._count < DNS_SERVER_IP_INVALID_SCORE)
            {
                return SUCCESS;
            }
        }
    }
    
    return DNS_SERVER_IP_ERROR_NO_VALID_SERVER;
}

/* 投诉 dns server ip index 不可用。 */
_int32 dns_server_ip_complain(DNS_SERVER_IP *dns_server_ip, char *host_name, _u32 index)
{
    _int32 ret_val = SUCCESS;
    _u32 hash_key = 0, i = 0;
    
    if (dns_server_ip == NULL || host_name == NULL || host_name[0] == 0 
        || index >= dns_server_ip->_ip_count || index >= DNS_SERVER_IP_COUNT_MAX)
    {
        return INVALID_ARGUMENT;
    }

    if (dns_server_ip->_validity[index]._count >= DNS_SERVER_IP_INVALID_SCORE)
    {
        return SUCCESS;
    }

    ret_val = sd_get_url_hash_value(host_name, sd_strlen(host_name), &hash_key);
	CHECK_VALUE(ret_val);

    /* 防止重复投诉 */
    for (i = 0; i < dns_server_ip->_validity[index]._count; ++i)
    {
        if (hash_key == dns_server_ip->_validity[index]._host_name_key[i])
        {
            break;
        }
    }

    if (i == dns_server_ip->_validity[index]._count)
    {
        dns_server_ip->_validity[index]._host_name_key[dns_server_ip->_validity[index]._count] = hash_key;
        dns_server_ip->_validity[index]._count ++;
        
        LOG_DEBUG("complain server_ip[%d]=0x%x, by %s, total_complain_count=%d.",
            index, dns_server_ip->_ip_list[index], host_name,
            dns_server_ip->_validity[index]._count);
    }
    
    return SUCCESS;
}

/* 清空投诉记录。 */
_int32 dns_server_ip_complain_clear(DNS_SERVER_IP *dns_server_ip)
{
    if (dns_server_ip == NULL)
    {
        return INVALID_ARGUMENT;
    }

    sd_memset(dns_server_ip->_validity, 0, DNS_SERVER_IP_COUNT_MAX * sizeof(DNS_SERVER_IP_VALIDITY));
    LOG_DEBUG("all complain clear.");
    return SUCCESS;
}

/************************************************************************
 *
 * DNS cache.
 *
 ************************************************************************/

#define DNS_CHCHE_INDEX_NULL DNS_CHCHE_SIZE_MAX

_int32 dns_cache_query_lru(DNS_CACHE *dns_cache, char *host_name, DNS_CONTENT_PACKAGE *dns_content)
{
    _int32 ret_val = SUCCESS;
    _u32 hash_key = 0;
    _u32 found_index = 0;
    _u32 now_stamp = 0;
    _u32 exp_time_stamp = 0;

    if (dns_cache == NULL || host_name == NULL || dns_content == NULL)
    {
        return INVALID_ARGUMENT;
    }

    ret_val = sd_time_ms(&now_stamp);
    CHECK_VALUE(ret_val);

    ret_val = sd_get_url_hash_value(host_name, sd_strlen(host_name), &hash_key);
	CHECK_VALUE(ret_val);
    hash_key %= DNS_CHCHE_SIZE_MAX;
    
    found_index = dns_cache->_hash_table._index[hash_key];
    while (found_index != DNS_CHCHE_INDEX_NULL)
    {
        exp_time_stamp = dns_cache->_dns_content[found_index]._start_time
            + dns_cache->_dns_content[found_index]._ttl[0];

        /* 如果TTL 超时，将优先级降到最低。 */
        if (TIME_LT(exp_time_stamp, now_stamp))    
        {
            if (dns_cache->_pri_lru._min_index != found_index)
            {
                dns_cache->_pri_lru._next_index[dns_cache->_pri_lru._prior_index[found_index]]
                    = dns_cache->_pri_lru._next_index[found_index];

                if (dns_cache->_pri_lru._max_index == found_index)
                {
                    dns_cache->_pri_lru._max_index = dns_cache->_pri_lru._prior_index[found_index];
                }
                else
                {
                    dns_cache->_pri_lru._prior_index[dns_cache->_pri_lru._next_index[found_index]]
                        = dns_cache->_pri_lru._prior_index[found_index];
                }
                dns_cache->_pri_lru._prior_index[found_index] = DNS_CHCHE_INDEX_NULL;
                dns_cache->_pri_lru._next_index[found_index] = dns_cache->_pri_lru._min_index;
                dns_cache->_pri_lru._min_index = found_index;

                dns_cache->_pri_lru._prior_index[dns_cache->_pri_lru._min_index] = DNS_CHCHE_INDEX_NULL;
                dns_cache->_pri_lru._next_index[dns_cache->_pri_lru._max_index] = DNS_CHCHE_INDEX_NULL;
            }
        }
        
        if (0 == sd_strncmp(dns_cache->_dns_content[found_index]._host_name, host_name,
            dns_cache->_dns_content[found_index]._host_name_buffer_len))
        {
            if (TIME_LT(exp_time_stamp, now_stamp))
            {
                /* not found. */
                return -1;
            }
            
            sd_memcpy(dns_content, &dns_cache->_dns_content[found_index], sizeof(DNS_CONTENT_PACKAGE));

            /* 将优先级升到最高。 */
            if (dns_cache->_pri_lru._max_index != found_index)
            {
                dns_cache->_pri_lru._prior_index[dns_cache->_pri_lru._next_index[found_index]]
                    = dns_cache->_pri_lru._prior_index[found_index];

                if (dns_cache->_pri_lru._min_index == found_index)
                {
                    dns_cache->_pri_lru._min_index = dns_cache->_pri_lru._next_index[found_index];
                }
                else
                {
                    dns_cache->_pri_lru._next_index[dns_cache->_pri_lru._prior_index[found_index]]
                        = dns_cache->_pri_lru._next_index[found_index];
                }
                dns_cache->_pri_lru._prior_index[found_index] = dns_cache->_pri_lru._max_index;
                dns_cache->_pri_lru._next_index[dns_cache->_pri_lru._max_index] = found_index;
                dns_cache->_pri_lru._max_index = found_index;

                dns_cache->_pri_lru._prior_index[dns_cache->_pri_lru._min_index] = DNS_CHCHE_INDEX_NULL;
                dns_cache->_pri_lru._next_index[dns_cache->_pri_lru._max_index] = DNS_CHCHE_INDEX_NULL;
            }

            return SUCCESS;
        }

        found_index = dns_cache->_hash_table._conflict_next_index[found_index];
            
    }

    /* not found. */
    return -1;
}

_int32 dns_cache_append_lru(DNS_CACHE *dns_cache, DNS_CONTENT_PACKAGE *dns_content)
{
	_int32 ret_val = SUCCESS;
    _u32 updated_index = DNS_CHCHE_INDEX_NULL;
    _u32 new_hash_key = 0;
    _u32 old_hash_key = 0;
    _u32 hash_index = 0;
    _u32 now_stamp = 0;
    _u32 exp_time_stamp = 0;
    BOOL need_replaced = FALSE;

    if (dns_cache == NULL || dns_content == NULL)
    {
        return INVALID_ARGUMENT;
    }

    ret_val = sd_time_ms(&now_stamp);
    CHECK_VALUE(ret_val);

    /* 查重复. */
    ret_val = sd_get_url_hash_value(dns_content->_host_name, sd_strlen(dns_content->_host_name), &new_hash_key);
	CHECK_VALUE(ret_val);
    new_hash_key %= DNS_CHCHE_SIZE_MAX;

    hash_index = dns_cache->_hash_table._index[new_hash_key];
    while (hash_index != DNS_CHCHE_INDEX_NULL)
    {
        if (0 == sd_strncmp(dns_cache->_dns_content[hash_index]._host_name, dns_content->_host_name,
            dns_cache->_dns_content[hash_index]._host_name_buffer_len))
        {
            exp_time_stamp = dns_cache->_dns_content[hash_index]._start_time
                + dns_cache->_dns_content[hash_index]._ttl[0];
            
            /* 重复但超时，准备替代. */
            if (TIME_LT(exp_time_stamp, now_stamp))
            {
                need_replaced = TRUE;
                updated_index = hash_index;
                break;
            }

            /* 重复. */
            return SUCCESS;
        }
        hash_index = dns_cache->_hash_table._conflict_next_index[hash_index];
    }

    /* 如果没有存在重复且超时的域名. */
    if (DNS_CHCHE_INDEX_NULL == updated_index)
    {
        if (dns_cache->_size < DNS_CHCHE_SIZE_MAX)
        {
            /* cache is not full. */
            updated_index = dns_cache->_size;
            dns_cache->_size ++;
            need_replaced = FALSE;
        }
        else
        {
            /* cache is full. */
            updated_index = dns_cache->_pri_lru._min_index;
            need_replaced = TRUE;
        }
    }

    if (need_replaced)
    {
        /* remove from priority_lru. */
        if (dns_cache->_pri_lru._min_index == updated_index)
        {
            dns_cache->_pri_lru._min_index = dns_cache->_pri_lru._next_index[updated_index];
        }
        else
        {
            dns_cache->_pri_lru._next_index[dns_cache->_pri_lru._prior_index[updated_index]]
                = dns_cache->_pri_lru._next_index[updated_index];
        }

        if (dns_cache->_pri_lru._max_index == updated_index)
        {
            dns_cache->_pri_lru._max_index = dns_cache->_pri_lru._prior_index[updated_index];
        }
        else
        {
            dns_cache->_pri_lru._prior_index[dns_cache->_pri_lru._next_index[updated_index]]
                = dns_cache->_pri_lru._prior_index[updated_index];
        }

        dns_cache->_pri_lru._prior_index[updated_index] = DNS_CHCHE_INDEX_NULL;
        dns_cache->_pri_lru._next_index[updated_index] = DNS_CHCHE_INDEX_NULL;

        /* remove updated_index from hash table. */
        ret_val = sd_get_url_hash_value(dns_cache->_dns_content[updated_index]._host_name,
            sd_strlen(dns_cache->_dns_content[updated_index]._host_name), &old_hash_key);
        
        CHECK_VALUE(ret_val);
        old_hash_key %= DNS_CHCHE_SIZE_MAX;

        hash_index = dns_cache->_hash_table._index[old_hash_key];
        if (hash_index == updated_index)
        {
            dns_cache->_hash_table._index[old_hash_key] = dns_cache->_hash_table._conflict_next_index[updated_index];
            dns_cache->_hash_table._conflict_next_index[updated_index] = DNS_CHCHE_INDEX_NULL;
        }
        else if (hash_index != DNS_CHCHE_INDEX_NULL)
        {
            while (dns_cache->_hash_table._conflict_next_index[hash_index] != updated_index)
            {
                hash_index = dns_cache->_hash_table._conflict_next_index[hash_index];
            }
            
            dns_cache->_hash_table._conflict_next_index[hash_index] = dns_cache->_hash_table._conflict_next_index[updated_index];
            dns_cache->_hash_table._conflict_next_index[updated_index] = DNS_CHCHE_INDEX_NULL;
        }
    }

    /* update new content. */
    sd_memcpy(&dns_cache->_dns_content[updated_index], dns_content, sizeof(DNS_CONTENT_PACKAGE));

    /* update priority_lru. */
    if (dns_cache->_pri_lru._min_index == DNS_CHCHE_INDEX_NULL)
    {
        dns_cache->_pri_lru._min_index = updated_index;
    }
    else
    {
        dns_cache->_pri_lru._prior_index[updated_index] = dns_cache->_pri_lru._max_index;
        dns_cache->_pri_lru._next_index[dns_cache->_pri_lru._max_index] = updated_index;
    }
    dns_cache->_pri_lru._max_index = updated_index;
    
    /* update hash table. */

    /* new_hash_key 之前已得到。 */
    hash_index = dns_cache->_hash_table._index[new_hash_key];
    if (hash_index == DNS_CHCHE_INDEX_NULL)
    {
        dns_cache->_hash_table._index[new_hash_key] = updated_index;
    }
    else
    {
        while (dns_cache->_hash_table._conflict_next_index[hash_index] != DNS_CHCHE_INDEX_NULL)
        {
            hash_index = dns_cache->_hash_table._conflict_next_index[hash_index];
        }
        dns_cache->_hash_table._conflict_next_index[hash_index] = updated_index;
    }
    
    return SUCCESS;
}

_int32 dns_cache_query_lru_ttl(DNS_CACHE *dns_cache, char *host_name, DNS_CONTENT_PACKAGE *dns_content)
{
    _int32 ret_val = SUCCESS;
    _u32 hash_key = 0;
    _u32 found_index = 0;
    _u32 now_stamp = 0;
    _u32 exp_time_stamp = 0;

    if (dns_cache == NULL || host_name == NULL || dns_content == NULL)
    {
        return INVALID_ARGUMENT;
    }

    ret_val = sd_time_ms(&now_stamp);
    CHECK_VALUE(ret_val);

    ret_val = sd_get_url_hash_value(host_name, sd_strlen(host_name), &hash_key);
	CHECK_VALUE(ret_val);
    hash_key %= DNS_CHCHE_SIZE_MAX;
    
    found_index = dns_cache->_hash_table._index[hash_key];
    while (found_index != DNS_CHCHE_INDEX_NULL)
    {
        if (0 == sd_strncmp(dns_cache->_dns_content[found_index]._host_name, host_name,
            dns_cache->_dns_content[found_index]._host_name_buffer_len))
        {
            exp_time_stamp = dns_cache->_dns_content[found_index]._start_time
                + dns_cache->_dns_content[found_index]._ttl[0];
            
            if (TIME_LT(exp_time_stamp, now_stamp))
            {
                /* not found. */
                return -1;
            }
            
            sd_memcpy(dns_content, &dns_cache->_dns_content[found_index], sizeof(DNS_CONTENT_PACKAGE));

            /* 将 lru 优先级升到最高。 */
            if (dns_cache->_pri_lru._max_index != found_index)
            {
                dns_cache->_pri_lru._prior_index[dns_cache->_pri_lru._next_index[found_index]]
                    = dns_cache->_pri_lru._prior_index[found_index];

                if (dns_cache->_pri_lru._min_index == found_index)
                {
                    dns_cache->_pri_lru._min_index = dns_cache->_pri_lru._next_index[found_index];
                }
                else
                {
                    dns_cache->_pri_lru._next_index[dns_cache->_pri_lru._prior_index[found_index]]
                        = dns_cache->_pri_lru._next_index[found_index];
                }
                dns_cache->_pri_lru._prior_index[found_index] = dns_cache->_pri_lru._max_index;
                dns_cache->_pri_lru._next_index[dns_cache->_pri_lru._max_index] = found_index;
                dns_cache->_pri_lru._max_index = found_index;

                dns_cache->_pri_lru._prior_index[dns_cache->_pri_lru._min_index] = DNS_CHCHE_INDEX_NULL;
                dns_cache->_pri_lru._next_index[dns_cache->_pri_lru._max_index] = DNS_CHCHE_INDEX_NULL;
            }

            return SUCCESS;
        }

        found_index = dns_cache->_hash_table._conflict_next_index[found_index];
            
    }

    /* not found. */
    return -1;
}

_int32 dns_cache_append_lru_ttl(DNS_CACHE *dns_cache, DNS_CONTENT_PACKAGE *dns_content)
{
	_int32 ret_val = SUCCESS;
    _u32 updated_index = DNS_CHCHE_INDEX_NULL;
    _u32 new_hash_key = 0;
    _u32 old_hash_key = 0;
    _u32 hash_index = 0;
    _u32 now_stamp = 0;
    _u32 exp_time_stamp1 = 0, exp_time_stamp2 = 0;
    _u32 tmp_index = 0;
    BOOL need_replaced = FALSE;

    if (dns_cache == NULL || dns_content == NULL)
    {
        return INVALID_ARGUMENT;
    }

    ret_val = sd_time_ms(&now_stamp);
    CHECK_VALUE(ret_val);

    /* 查重复. */
    ret_val = sd_get_url_hash_value(dns_content->_host_name, sd_strlen(dns_content->_host_name), &new_hash_key);
	CHECK_VALUE(ret_val);
    new_hash_key %= DNS_CHCHE_SIZE_MAX;

    hash_index = dns_cache->_hash_table._index[new_hash_key];
    while (hash_index != DNS_CHCHE_INDEX_NULL)
    {
        if (0 == sd_strncmp(dns_cache->_dns_content[hash_index]._host_name, dns_content->_host_name,
            dns_cache->_dns_content[hash_index]._host_name_buffer_len))
        {
            exp_time_stamp1 = dns_cache->_dns_content[hash_index]._start_time
                + dns_cache->_dns_content[hash_index]._ttl[0];
            
            /* 重复但超时，准备替代. */
            if (TIME_LT(exp_time_stamp1, now_stamp))
            {
                need_replaced = TRUE;
                updated_index = hash_index;
                break;
            }

            /* 重复. */
            return SUCCESS;
        }
        hash_index = dns_cache->_hash_table._conflict_next_index[hash_index];
    }

    /* 如果没有存在重复且超时的域名. */
    if (DNS_CHCHE_INDEX_NULL == updated_index)
    {
        if (dns_cache->_size < DNS_CHCHE_SIZE_MAX)
        {
            /* cache is not full. */
            updated_index = dns_cache->_size;
            dns_cache->_size ++;
            need_replaced = FALSE;
        }
        else
        {
            /* cache is full. */
            exp_time_stamp1 = dns_cache->_dns_content[dns_cache->_pri_ttl._min_index]._start_time
                + dns_cache->_dns_content[dns_cache->_pri_ttl._min_index]._ttl[0];
        
            if (TIME_LT(exp_time_stamp1, now_stamp))
            {
                updated_index = dns_cache->_pri_ttl._min_index;
            }
            else
            {
                updated_index = dns_cache->_pri_lru._min_index;
            }
            need_replaced = TRUE;
        }
    }
    
    if (need_replaced)
    {
        /* remove from pri_lru. */
        if (dns_cache->_pri_lru._min_index == updated_index)
        {
            dns_cache->_pri_lru._min_index = dns_cache->_pri_lru._next_index[updated_index];
        }
        else
        {
            dns_cache->_pri_lru._next_index[dns_cache->_pri_lru._prior_index[updated_index]]
                = dns_cache->_pri_lru._next_index[updated_index];
        }

        if (dns_cache->_pri_lru._max_index == updated_index)
        {
            dns_cache->_pri_lru._max_index = dns_cache->_pri_lru._prior_index[updated_index];
        }
        else
        {
            dns_cache->_pri_lru._prior_index[dns_cache->_pri_lru._next_index[updated_index]]
                = dns_cache->_pri_lru._prior_index[updated_index];
        }

        dns_cache->_pri_lru._prior_index[updated_index] = DNS_CHCHE_INDEX_NULL;
        dns_cache->_pri_lru._next_index[updated_index] = DNS_CHCHE_INDEX_NULL;

        /* remove from pri_ttl. */
        if (dns_cache->_pri_ttl._min_index == updated_index)
        {
            dns_cache->_pri_ttl._min_index = dns_cache->_pri_ttl._next_index[updated_index];
        }
        else
        {
            dns_cache->_pri_ttl._next_index[dns_cache->_pri_ttl._prior_index[updated_index]]
                = dns_cache->_pri_ttl._next_index[updated_index];
        }

        if (dns_cache->_pri_ttl._max_index == updated_index)
        {
            dns_cache->_pri_ttl._max_index = dns_cache->_pri_ttl._prior_index[updated_index];
        }
        else
        {
            dns_cache->_pri_ttl._prior_index[dns_cache->_pri_ttl._next_index[updated_index]]
                = dns_cache->_pri_ttl._prior_index[updated_index];
        }

        dns_cache->_pri_ttl._prior_index[updated_index] = DNS_CHCHE_INDEX_NULL;
        dns_cache->_pri_ttl._next_index[updated_index] = DNS_CHCHE_INDEX_NULL;

        /* remove from hash table. */
        ret_val = sd_get_url_hash_value(dns_cache->_dns_content[updated_index]._host_name,
            sd_strlen(dns_cache->_dns_content[updated_index]._host_name), &old_hash_key);
        CHECK_VALUE(ret_val);
        old_hash_key %= DNS_CHCHE_SIZE_MAX;

        hash_index = dns_cache->_hash_table._index[old_hash_key];
        if (hash_index == updated_index)
        {
            dns_cache->_hash_table._index[old_hash_key] = dns_cache->_hash_table._conflict_next_index[updated_index];
            dns_cache->_hash_table._conflict_next_index[updated_index] = DNS_CHCHE_INDEX_NULL;
        }
        else if (hash_index != DNS_CHCHE_INDEX_NULL)
        {
            while (dns_cache->_hash_table._conflict_next_index[hash_index] != updated_index)
            {
                hash_index = dns_cache->_hash_table._conflict_next_index[hash_index];
            }
            
            dns_cache->_hash_table._conflict_next_index[hash_index] = dns_cache->_hash_table._conflict_next_index[updated_index];
            dns_cache->_hash_table._conflict_next_index[updated_index] = DNS_CHCHE_INDEX_NULL;
        }
    }

    /* update new content. */
    sd_memcpy(&dns_cache->_dns_content[updated_index], dns_content, sizeof(DNS_CONTENT_PACKAGE));

    /* update priority_lru. */
    if (dns_cache->_pri_lru._min_index == DNS_CHCHE_INDEX_NULL)
    {
        dns_cache->_pri_lru._min_index = updated_index;
    }
    else
    {
        dns_cache->_pri_lru._prior_index[updated_index] = dns_cache->_pri_lru._max_index;
        dns_cache->_pri_lru._next_index[dns_cache->_pri_lru._max_index] = updated_index;
    }
    dns_cache->_pri_lru._max_index = updated_index;

    /* update priority_ttl. */
    
    /* 在 tmp_index 之后插入 updated_index。 */
    tmp_index = dns_cache->_pri_ttl._max_index;
    while (tmp_index != DNS_CHCHE_INDEX_NULL)
    {
        exp_time_stamp1 = dns_cache->_dns_content[tmp_index]._start_time
            + dns_cache->_dns_content[tmp_index]._ttl[0];
        
        exp_time_stamp2 = dns_cache->_dns_content[updated_index]._start_time
            + dns_cache->_dns_content[updated_index]._ttl[0];
    
        if (TIME_LE(exp_time_stamp1, exp_time_stamp2) || TIME_LT(exp_time_stamp1, now_stamp))
        {
            break;
        }
        tmp_index = dns_cache->_pri_ttl._prior_index[tmp_index];
    }
    
    if (tmp_index == DNS_CHCHE_INDEX_NULL)
    {
        dns_cache->_pri_ttl._next_index[updated_index] = dns_cache->_pri_ttl._min_index;
        dns_cache->_pri_ttl._min_index = updated_index;
    }
    else
    {
        dns_cache->_pri_ttl._prior_index[updated_index] = tmp_index;
        dns_cache->_pri_ttl._next_index[updated_index] = dns_cache->_pri_ttl._next_index[tmp_index];
        dns_cache->_pri_ttl._next_index[tmp_index] = updated_index;
    }
    
    if (dns_cache->_pri_ttl._next_index[updated_index] == DNS_CHCHE_INDEX_NULL)
    {
        dns_cache->_pri_ttl._max_index = updated_index;
    }
    else
    {
        dns_cache->_pri_ttl._prior_index[dns_cache->_pri_ttl._next_index[updated_index]] = updated_index;
    }

    /* update hash table. */
    
    /* new_hash_key 之前已得到。 */
    hash_index = dns_cache->_hash_table._index[new_hash_key];
    if (hash_index == DNS_CHCHE_INDEX_NULL)
    {
        dns_cache->_hash_table._index[new_hash_key] = updated_index;
    }
    else
    {
        while (dns_cache->_hash_table._conflict_next_index[hash_index] != DNS_CHCHE_INDEX_NULL)
        {
            hash_index = dns_cache->_hash_table._conflict_next_index[hash_index];
        }
        dns_cache->_hash_table._conflict_next_index[hash_index] = updated_index;
    }
    
    return SUCCESS;
}

_int32 dns_cache_clear(DNS_CACHE *dns_cache)
{
    _u32 i = 0;
    if (dns_cache == NULL)
    {
        return INVALID_ARGUMENT;
    }
    
    sd_memset(dns_cache, 0, sizeof(DNS_CACHE));
    for (i = 0; i < DNS_CHCHE_SIZE_MAX; ++i)
    {
        dns_cache->_hash_table._index[i] = DNS_CHCHE_INDEX_NULL;
        dns_cache->_hash_table._conflict_next_index[i] = DNS_CHCHE_INDEX_NULL;

        dns_cache->_pri_lru._prior_index[i] = DNS_CHCHE_INDEX_NULL;
        dns_cache->_pri_lru._next_index[i] = DNS_CHCHE_INDEX_NULL;

        dns_cache->_pri_ttl._prior_index[i] = DNS_CHCHE_INDEX_NULL;
        dns_cache->_pri_ttl._next_index[i] = DNS_CHCHE_INDEX_NULL;
    }

    dns_cache->_pri_lru._min_index = DNS_CHCHE_INDEX_NULL;
    dns_cache->_pri_lru._max_index = DNS_CHCHE_INDEX_NULL;

    dns_cache->_pri_ttl._min_index = DNS_CHCHE_INDEX_NULL;
    dns_cache->_pri_ttl._max_index = DNS_CHCHE_INDEX_NULL;

    return SUCCESS;
}

_int32 dns_cache_init(DNS_CACHE *dns_cache)
{
	LOG_DEBUG("dns_cache_init");
    return dns_cache_clear(dns_cache);
}

_int32 dns_cache_uninit(DNS_CACHE *dns_cache)
{
	LOG_DEBUG("dns_cache_uninit");
    return dns_cache_clear(dns_cache);
}

_int32 dns_cache_query(DNS_CACHE *dns_cache, char *host_name, DNS_CONTENT_PACKAGE *dns_content)
{
	_int32 ret = 0;
	
    /*
     * 当执行query操作时，lru_ttl 和 lru 性能相当。
     */
    /* ret = dns_cache_query_lru(dns_cache, host_name, dns_content); */
    ret = dns_cache_query_lru_ttl(dns_cache, host_name, dns_content);
	if (ret == SUCCESS)
	{
		LOG_DEBUG("dns_cache_query: %s (hitted), result=%d(%s), ip_count=%d, ip[0]=0x%x",
			host_name,
			dns_content->_result,
			((dns_content->_result == SUCCESS)? "SUCCESS": 
				(dns_content->_result == DNS_INVALID_ADDR)? "DNS_INVALID_ADDR": "other result"),
			dns_content->_ip_count,
			dns_content->_ip_list[0]);
	}
	else
	{
		LOG_DEBUG("dns_cache_query: %s (not found)", host_name);
	}
	return ret;
}

_int32 dns_cache_append(DNS_CACHE *dns_cache, DNS_CONTENT_PACKAGE *dns_content)
{
	_int32 ret = 0;

    /*
     * 当执行append操作时，因为 lru_ttl 需要维护一个TTL升序链表，所以性能比 lru 差一些。
     * 所以当cache替代很频繁的时候，可考虑用 lru 取代 lru_ttl。
     */
    /* ret = dns_cache_append_lru(dns_cache, dns_content); */
    ret = dns_cache_append_lru_ttl(dns_cache, dns_content);
	
	LOG_DEBUG("dns_cache_append: %s, ip_count=%d, ip[0]=0x%x",
		dns_content->_host_name, dns_content->_ip_count, dns_content->_ip_list[0]);
	return ret;
}

/* test only. */
#ifdef _DEBUG
_int32 dns_cache_debug_list(DNS_CACHE *dns_cache)
{
	_int32 ret_val = SUCCESS;
    _u32 index = 0, i = 0;
    _u32 now_stamp = 0;

    if (dns_cache == NULL)
    {
        return INVALID_ARGUMENT;
    }

    LOG_DEBUG("------list dns cache (begin)-------");
    LOG_DEBUG("dns cache size :%d/%d", dns_cache->_size, DNS_CHCHE_SIZE_MAX);

    ret_val = sd_time_ms(&now_stamp);
    CHECK_VALUE(ret_val);

    /* priority list lru */
    LOG_DEBUG("---dns_cache_priority_lru_[min_max]: %d < (%d, %d) > %d.", 
        dns_cache->_pri_lru._prior_index[dns_cache->_pri_lru._min_index],
        dns_cache->_pri_lru._min_index, dns_cache->_pri_lru._max_index,
        dns_cache->_pri_lru._next_index[dns_cache->_pri_lru._max_index]);
    
    i = 0;
    index = dns_cache->_pri_lru._min_index;
    while (index != DNS_CHCHE_INDEX_NULL)
    {
        LOG_DEBUG("lru[%d]: index[%d]=(%s, ip=0x%x)", i, index,
            dns_cache->_dns_content[index]._host_name, dns_cache->_dns_content[index]._ip_list[0]);

        index = dns_cache->_pri_lru._next_index[index];
        ++i;
    }

    /* priority list ttl */
    LOG_DEBUG("---dns_cache_priority_ttl_[min_max]: %d < (%d, %d) > %d.", 
        dns_cache->_pri_ttl._prior_index[dns_cache->_pri_ttl._min_index],
        dns_cache->_pri_ttl._min_index, dns_cache->_pri_ttl._max_index,
        dns_cache->_pri_ttl._next_index[dns_cache->_pri_ttl._max_index]);
    
    i = 0;
    index = dns_cache->_pri_ttl._min_index;
    while (index != DNS_CHCHE_INDEX_NULL)
    {
        LOG_DEBUG("ttl[%d]: index[%d]=%s, ip=0x%x, ttl:(%d, %d, %d)", i, index,
            dns_cache->_dns_content[index]._host_name, dns_cache->_dns_content[index]._ip_list[0],
            dns_cache->_dns_content[index]._start_time, dns_cache->_dns_content[index]._ttl[0],
            (dns_cache->_dns_content[index]._start_time + dns_cache->_dns_content[index]._ttl[0] - now_stamp));

        index = dns_cache->_pri_ttl._next_index[index];
        ++i;
    }

    /* hash table */
    for (i = 0; i < DNS_CHCHE_SIZE_MAX; ++i)
    {
        index = dns_cache->_hash_table._index[i];
        while (index != DNS_CHCHE_INDEX_NULL)
        {
            LOG_DEBUG("hash[%d]: index[%d]=%s", i, index, dns_cache->_dns_content[index]._host_name);
            index = dns_cache->_hash_table._conflict_next_index[index];
        }
    }

    LOG_DEBUG("------list dns cache (end).-------");
    return SUCCESS;
}
#endif

/************************************************************************
 *
 * DNS request queue.
 * 用于保存已发出request，但未收到answer的DNS msg。
 *
 ************************************************************************/

#define DNS_REQUEST_QUEUE_INDEX_NULL DNS_REQUEST_QUEUE_SIZE_MAX

_int32 dns_request_queue_init(DNS_REQUEST_QUEUE *req_queue)
{
    _u32 i = 0;
    _int32 ret_val = 0;
    
    if (req_queue == NULL)
    {
        return INVALID_ARGUMENT;
    }
    
    sd_memset(req_queue, 0, sizeof(DNS_REQUEST_QUEUE));
    for (i = 0; i < DNS_REQUEST_QUEUE_SIZE_MAX; ++i)
    {
        req_queue->_exp_list._prior_index[i] = DNS_REQUEST_QUEUE_INDEX_NULL;
        req_queue->_exp_list._next_index[i] = DNS_REQUEST_QUEUE_INDEX_NULL;
    }
    req_queue->_exp_list._head = DNS_REQUEST_QUEUE_INDEX_NULL;
    req_queue->_exp_list._tail = DNS_REQUEST_QUEUE_INDEX_NULL;

    ret_val = dns_server_ip_load(&req_queue->_dns_server_ip);
    CHECK_VALUE(ret_val);

    return SUCCESS;
}

_int32 dns_request_queue_is_empty(DNS_REQUEST_QUEUE *req_queue, BOOL *is_empty)
{
    if (req_queue == NULL || is_empty == NULL)
    {
        return INVALID_ARGUMENT;
    }
    *is_empty = (req_queue->_size == 0)? TRUE: FALSE;
    return SUCCESS;
}

_int32 dns_request_queue_is_full(DNS_REQUEST_QUEUE *req_queue, BOOL *is_full)
{
    if (req_queue == NULL || is_full == NULL)
    {
        return INVALID_ARGUMENT;
    }
    *is_full = (req_queue->_size >= DNS_REQUEST_QUEUE_SIZE_MAX)? TRUE: FALSE;
    return SUCCESS;
}

_int32 dns_request_queue_push(DNS_REQUEST_QUEUE *req_queue, DNS_REQUEST_RECORD *record)
{
    _u32 updated_index = 0;
    if (req_queue == NULL || record == NULL)
    {
        return INVALID_ARGUMENT;
    }

    if (req_queue->_size >= DNS_REQUEST_QUEUE_SIZE_MAX)
    {
        return BUFFER_OVERFLOW;
    }

    /*
    LOG_DEBUG("dns_request_queue_push, size(%d), push_index(%d): %s, dns_id=%d, pdata=0x%x, ttl=(%d + %d)",
        req_queue->_size, req_queue->_size,
        record->_host_name, record->_dns_id, record->_data, record->_request_time,
        record->_ttl);
    */        

    updated_index = req_queue->_size;
    sd_memcpy(&req_queue->_record[updated_index], record, sizeof(DNS_REQUEST_RECORD));

    if (req_queue->_exp_list._tail == DNS_REQUEST_QUEUE_INDEX_NULL)
    {
        req_queue->_exp_list._head = updated_index;
    }
    else
    {
        req_queue->_exp_list._prior_index[updated_index] = req_queue->_exp_list._tail;
        req_queue->_exp_list._next_index[req_queue->_exp_list._tail] = updated_index;
    }
    req_queue->_exp_list._tail = updated_index;

    req_queue->_size ++;
    return SUCCESS;
}

/*
 * 将第 pop_index 个请求信息取出来。
 */
_int32 dns_request_queue_pop_by_index(DNS_REQUEST_QUEUE *req_queue, _u32 pop_index, DNS_REQUEST_RECORD *record)
{
    if (req_queue == NULL || record == NULL)
    {
        return INVALID_ARGUMENT;
    }

    if (pop_index >= req_queue->_size)
    {
        return INVALID_ARGUMENT;
    }

    /*
    LOG_DEBUG("dns_request_queue_pop, size(%d), pop_index(%d): %s, dns_id=%d, pdata=0x%x, ttl=(%d + %d)",
        req_queue->_size, pop_index,
        req_queue->_record[pop_index]._host_name, req_queue->_record[pop_index]._dns_id,
        req_queue->_record[pop_index]._data, req_queue->_record[pop_index]._request_time,
        req_queue->_record[pop_index]._ttl);
    */

    sd_memcpy(record, &req_queue->_record[pop_index], sizeof(DNS_REQUEST_RECORD));

    /* 用 record_queue 中最后一个记录替换第 pop_index 个记录。 */
    if (pop_index != req_queue->_size - 1)
    {
        sd_memcpy(&req_queue->_record[pop_index], &req_queue->_record[req_queue->_size - 1],
            sizeof(DNS_REQUEST_RECORD));
    }

    sd_memset(&req_queue->_record[req_queue->_size - 1], 0, sizeof(DNS_REQUEST_RECORD));

    /* remove exp_list[pop_index] */
    if (req_queue->_exp_list._head == pop_index)
    {
        req_queue->_exp_list._head = req_queue->_exp_list._next_index[pop_index];
    }
    else
    {
        req_queue->_exp_list._next_index[req_queue->_exp_list._prior_index[pop_index]]
            = req_queue->_exp_list._next_index[pop_index];
    }
    
    if (req_queue->_exp_list._tail == pop_index)
    {
        req_queue->_exp_list._tail = req_queue->_exp_list._prior_index[pop_index];
    }
    else
    {
        req_queue->_exp_list._prior_index[req_queue->_exp_list._next_index[pop_index]]
            = req_queue->_exp_list._prior_index[pop_index];
    }
    
    req_queue->_exp_list._prior_index[pop_index] = DNS_REQUEST_QUEUE_INDEX_NULL;
    req_queue->_exp_list._next_index[pop_index] = DNS_REQUEST_QUEUE_INDEX_NULL;
    
    /* exp_list[pop_index] = exp_list[req_queue->_size - 1] */
    if (pop_index != (req_queue->_size - 1))
    {
        if (req_queue->_exp_list._head == (req_queue->_size - 1))
        {
            req_queue->_exp_list._head = pop_index;
        }
        else
        {
            req_queue->_exp_list._next_index[req_queue->_exp_list._prior_index[req_queue->_size - 1]] = pop_index;
        }
        req_queue->_exp_list._prior_index[pop_index] = req_queue->_exp_list._prior_index[req_queue->_size - 1];
        
        if (req_queue->_exp_list._tail == (req_queue->_size - 1))
        {
            req_queue->_exp_list._tail = pop_index;
        }
        else
        {
            req_queue->_exp_list._prior_index[req_queue->_exp_list._next_index[req_queue->_size - 1]] = pop_index;
        }
        req_queue->_exp_list._next_index[pop_index] = req_queue->_exp_list._next_index[req_queue->_size - 1];

        /* exp_list[req_queue->_size - 1] = NULL */
        req_queue->_exp_list._prior_index[req_queue->_size - 1] = DNS_REQUEST_QUEUE_INDEX_NULL;
        req_queue->_exp_list._next_index[req_queue->_size - 1] = DNS_REQUEST_QUEUE_INDEX_NULL;
    }

    req_queue->_size --;
    return SUCCESS;    
}

/*
 * [in]:
 *      type: 0 - pop for answer; 1 - cancel request; 2 - pop the expired one.
 *      record->_host_name, (it is not needed when (type == 2).)
 *      record->_host_name_len, (it is not needed when (type == 2).)
 *      record->_dns_id, (it is not needed when (type == 1) or (type == 2).)
 * [out]:
 *      record
 */

#define DNS_REQUEST_QUEUE_POP_FOR_ANSWER (0)
#define DNS_REQUEST_QUEUE_POP_FOR_CANCEL (1)
#define DNS_REQUEST_QUEUE_POP_FOR_EXPIRED (2)
#define DNS_REQUEST_QUEUE_NOT_FOUND (-1)

_int32 dns_request_queue_pop(DNS_REQUEST_QUEUE *req_queue, DNS_REQUEST_RECORD *record, _u32 type)
{
    _int32 ret_val = SUCCESS;
    _u32 i = 0;
    _u32 pop_index = DNS_REQUEST_QUEUE_INDEX_NULL;
    _u32 expired_index = 0;
    _u32 now_stamp = 0;

    if (req_queue == NULL || record == NULL)
    {
        return INVALID_ARGUMENT;
    }

    switch (type)
    {
        case DNS_REQUEST_QUEUE_POP_FOR_ANSWER:
            for (i = 0; i < req_queue->_size; ++i)
            {
                if ((req_queue->_record[i]._dns_id == record->_dns_id)
                    && (0 == sd_strncmp(req_queue->_record[i]._host_name, record->_host_name,
                    req_queue->_record[i]._host_name_len)))
                {
                    pop_index = i;
                    break;
                }
            }
            break;
        case DNS_REQUEST_QUEUE_POP_FOR_CANCEL:
            for (i = 0; i < req_queue->_size; ++i)
            {
                if (0 == sd_strncmp(req_queue->_record[i]._host_name, record->_host_name,
                    req_queue->_record[i]._host_name_len))
                {
                    pop_index = i;
                    break;
                }
            }
            break;
        case DNS_REQUEST_QUEUE_POP_FOR_EXPIRED:
            ret_val = sd_time_ms(&now_stamp);
            CHECK_VALUE(ret_val);

            expired_index = req_queue->_exp_list._head;
            if (TIME_LT(req_queue->_record[expired_index]._request_time + req_queue->_record[expired_index]._ttl,
                now_stamp))
            {
                pop_index = expired_index;
            }
            break;
        default:
            break;
    }
    
    /* not found. */
    if (pop_index == DNS_REQUEST_QUEUE_INDEX_NULL)
    {
        return DNS_REQUEST_QUEUE_NOT_FOUND;
    }

    return dns_request_queue_pop_by_index(req_queue, pop_index, record);
        
}

_int32 dns_request_queue_mark_invalid_server(DNS_REQUEST_QUEUE *req_queue)
{
    _u32 i = 0;

    if (req_queue == NULL)
    {
        return INVALID_ARGUMENT;
    }
    
    for (i = 0; i < req_queue->_size && i < DNS_REQUEST_QUEUE_SIZE_MAX; ++i)
    {
        req_queue->_record[i]._dns_server_ip_index = DNS_SERVER_IP_COUNT_MAX;
    }

    return SUCCESS;
}

/************************************************************************
 *
 * DNS package and parse.
 *
 ************************************************************************/

#define DNS_FLAG_QR     (0x8000)
#define DNS_FLAG_AA     (0x0400)
#define DNS_FLAG_TC     (0x0200)
#define DNS_FLAG_RD     (0x0100)
#define DNS_FLAG_RA     (0x0080)

#define DNS_ANSWER_TYPE_A     (1)
#define DNS_ANSWER_TYPE_NS    (2)
#define DNS_ANSWER_TYPE_CNAME (5)
#define DNS_ANSWER_TYPE_SOA   (6)
#define DNS_ANSWER_TYPE_WKS   (11)
#define DNS_ANSWER_TYPE_PTR   (12)
#define DNS_ANSWER_TYPE_HINFO (13)
#define DNS_ANSWER_TYPE_MX    (15)
#define TYPE_AAAA             (28)


#define DNS_HEAD_SIZE          (12)
#define DNS_QUESTION_TAIL_SIZE (4)

typedef struct
{
    _u16 id;
    _u16 flag;
    _u16 questions;
    _u16 answer;
    _u16 authority;
    _u16 additional;
}DNS_HEAD;

typedef struct
{
    _u16 type;
    _u16 class;
}DNS_QUESTION_TAIL;

/*
 * "www.xunlei.com\0"   ->   "\3www\6xunlei\3com\0", len = 16.
 */
_int32 host_name_to_dns_package_format(char *host_name, char *dns_package_format, _u32 *dns_package_format_len)
{
    _u32 i = 0;
    _u32 pos_dot = 0;
    _u32 len = 0;

    if (host_name == NULL || dns_package_format == NULL || dns_package_format_len == NULL)
    {
        return INVALID_ARGUMENT;
    }

    if (host_name[0] == 0)
    {
        dns_package_format[0] = 0;
        *dns_package_format_len = 1;
        return SUCCESS;
    }

    len = sd_strlen(host_name);
    dns_package_format[pos_dot] = 0;
    for(i = 0; i < len; ++i)
    {
        if (host_name[i] != '.')
        {
            dns_package_format[i + 1] = host_name[i];
            dns_package_format[pos_dot] += 1;
        }
        else
        {
            pos_dot = i + 1;
            dns_package_format[pos_dot] = 0;
        }
    }
    dns_package_format[len + 1] = 0;
    *dns_package_format_len = len + 2;
    return SUCCESS;
}

/*
 * "\3www\6xunlei\3com\0"   ->   "www.xunlei.com\0", len = 15.
 */
_int32 dns_package_format_to_host_name(char *dns_package_format, char *host_name, _u32 *host_name_len)
{
    _u32 pos1 = 0;
    _u32 pos2 = 0;
    _u32 num = 0;
    _u32 i = 0;

    if (dns_package_format == NULL || host_name == NULL || host_name_len == NULL)
    {
        return INVALID_ARGUMENT;
    }

    num = dns_package_format[pos1++];
    if (num == 0)
    {
        host_name[0] = '\0';
        *host_name_len = 1;
        return SUCCESS;
    }

    for (;;)
    {
        for (i = 0; i < num; ++i)
        {
            host_name[pos2++] = dns_package_format[pos1++];
        }

        num = dns_package_format[pos1++];
        if (num == 0)
        {
            host_name[pos2] = '\0';
            *host_name_len = pos2 + 1;
            break;
        }
        else
        {
            host_name[pos2++] = '.';
        }
    }

    return SUCCESS;
}

/* 
 * package dns request.
 */
_int32 package_dns_request_package(char *host_name, DNS_CODE_PACKAGE *dns_code, _u32 *dns_id)
{
    DNS_HEAD dns_head;
    DNS_QUESTION_TAIL dns_question_tail;
    _u32 pos = 0;
    _u32 host_format_len = 0;

    if (dns_code == NULL || host_name == NULL || host_name[0] == 0 || dns_id == NULL)
    {
        return INVALID_ARGUMENT;
    }

    sd_memset(dns_code, 0, sizeof(DNS_CODE_PACKAGE));

    (*dns_id) += 1;
    (*dns_id) &= 0x0000FFFF;

    /* fill dns head. */
    sd_memset(&dns_head, 0, sizeof(DNS_HEAD));
    dns_head.id = sd_htons(*dns_id);
    dns_head.flag = sd_htons(DNS_FLAG_RD);
    dns_head.questions = sd_htons(0X0001);
    dns_head.answer = 0;
    dns_head.authority = 0;
    dns_head.additional = 0;

    sd_memcpy(dns_code->_dns_code + pos, &dns_head, DNS_HEAD_SIZE);
    pos += DNS_HEAD_SIZE;

    /* fill dns question. */
    host_name_to_dns_package_format(host_name, dns_code->_dns_code + pos, &host_format_len);
    pos += host_format_len;

    dns_question_tail.type = sd_htons(0x01);
    dns_question_tail.class = sd_htons(0x01);
    sd_memcpy(dns_code->_dns_code + pos, &dns_question_tail, DNS_QUESTION_TAIL_SIZE);

    pos += DNS_QUESTION_TAIL_SIZE;

    dns_code->_len = pos;
    return SUCCESS;
}

_int32 parse_dns_answer_package(DNS_CODE_PACKAGE *dns_code, DNS_CONTENT_PACKAGE *dns_content, _u32 *dns_id)
{
    _int32 ret_val = SUCCESS;
    DNS_HEAD dns_head;
    DNS_QUESTION_TAIL dns_question_tail;
    _u32 pos = 0;
    _u16 answer_domain_name_redirect_ptr = 0;
    _u16 answer_type = 0;
    _u16 answer_class = 0;
    _u32 answer_ttl = 0;
    _u16 answer_data_length = 0;
    _u32 answer_ip = 0;
    _u32 now_stamp = 0;
    _u32 tmp_count = 0;

    if (dns_code == NULL || dns_content == NULL || dns_id == NULL)
    {
        return INVALID_ARGUMENT;
    }

    if (dns_code->_len < DNS_HEAD_SIZE + 4)
    {
        /* DNS包长度不足。 */
        return -1;
    }

    ret_val = sd_time_ms(&now_stamp);
    CHECK_VALUE(ret_val);

    sd_memset(dns_content, 0, sizeof(DNS_CONTENT_PACKAGE));
    dns_content->_start_time = now_stamp;
    
    /* get dns head. */
    sd_memcpy(&dns_head, dns_code->_dns_code + pos, DNS_HEAD_SIZE);
    pos += DNS_HEAD_SIZE;

    dns_head.id = sd_ntohs(dns_head.id);
    dns_head.flag = sd_ntohs(dns_head.flag);
    dns_head.questions = sd_ntohs(dns_head.questions);
    dns_head.answer = sd_ntohs(dns_head.answer);
    dns_head.authority = sd_ntohs(dns_head.authority);
    dns_head.additional = sd_ntohs(dns_head.additional);

    if (dns_head.answer == 0 && dns_head.authority == 0)
    {
        return -1;
    }
    
    *dns_id = dns_head.id;

    /* question. */
    tmp_count = dns_head.questions;
    while (tmp_count > 0)
    {
        if ((*(dns_code->_dns_code + pos) & 0xC0) == 0xC0)
        {
            /* host_name redir. do not get it here. */
            pos += 2;
        }
        else if((*(dns_code->_dns_code + pos) & 0xC0) == 0x00)
        {
            /* host_name. */
            dns_package_format_to_host_name(dns_code->_dns_code + pos,
                dns_content->_host_name, &dns_content->_host_name_buffer_len);
            
            while (pos < dns_code->_len && 0X00 != *(dns_code->_dns_code + pos))
            {
                ++pos;
            }
            ++pos;
        }
        else
        {
            /* reserved */
            return -1;
        }

        /* question tail. */
        sd_memcpy(&dns_question_tail, dns_code->_dns_code + pos, DNS_QUESTION_TAIL_SIZE);
        pos += DNS_QUESTION_TAIL_SIZE;

        tmp_count --;
    }

    /* answer(s). */
    tmp_count = dns_head.answer;
    while (tmp_count > 0)
    {   
        if ((*(dns_code->_dns_code + pos) & 0xC0) == 0xC0)
        {
            /*
             * answer_domain_name_redirect_ptr (2)
             * C0 0C
             * 最首2Bit为11, 剩下的Bit构成一个指针,指向answer_domain_name_redirect_ptr.
             */
            sd_memcpy(&answer_domain_name_redirect_ptr, dns_code->_dns_code + pos, 2);
            pos += 2;
            answer_domain_name_redirect_ptr = sd_ntohs(answer_domain_name_redirect_ptr);
            if (0X00 == answer_domain_name_redirect_ptr)
            {
                /* 域名未得到IP，可能是非法域名. */
                return -1;
            }
        }
        else if((*(dns_code->_dns_code + pos) & 0xC0) == 0x00)
        {
            /* host_name. maybe no "question". */
            if (dns_content->_host_name[0] == 0)
            {
                dns_package_format_to_host_name(dns_code->_dns_code + pos,
                    dns_content->_host_name, &dns_content->_host_name_buffer_len);
            }
            
            while (pos < dns_code->_len && 0X00 != *(dns_code->_dns_code + pos))
            {
                ++pos;
            }
            ++pos;
        }
        else
        {
            /* reserved */
            return -1;
        }
        
        /* answer_type(2) */
        sd_memcpy(&answer_type, dns_code->_dns_code + pos, 2);
        pos += 2;
        answer_type = sd_ntohs(answer_type);
        
        /* answer_class(2) */
        sd_memcpy(&answer_class, dns_code->_dns_code + pos, 2);
        pos += 2;
        answer_class = sd_ntohs(answer_class);

        /* TTL (4) */
        sd_memcpy(&answer_ttl, dns_code->_dns_code + pos, 4);
        pos += 4;
        answer_ttl = sd_ntohl(answer_ttl);
        answer_ttl *= 1000; /* 单位：ms */

        /* answer_data_length (2) */
        sd_memcpy(&answer_data_length, dns_code->_dns_code + pos, 2);
        pos += 2;
        answer_data_length = sd_ntohs(answer_data_length);

        /* answer_ip */
        sd_memcpy(&answer_ip, dns_code->_dns_code + pos, 4);
        pos += answer_data_length;

        if (DNS_ANSWER_TYPE_A == answer_type)
        {
            /* 一般来说TTL都是相同的，所以此处未作降序处理。 */
            if (dns_content->_ip_count >= DNS_CONTENT_IP_COUNT_MAX)
            {
                break;
            }
            dns_content->_ip_list[dns_content->_ip_count] = answer_ip;
            dns_content->_ttl[dns_content->_ip_count] = answer_ttl;
            dns_content->_ip_count ++;
        }

        tmp_count --;
    }

    if (pos > dns_code->_len)
    {
        return -1;
    }

    /* skip "authority" and "additional". */
    return SUCCESS;
}

/************************************************************************
 *
 * DNS send and read.
 *
 ************************************************************************/

_int32 send_dns_request_package(_u32 *sock, DNS_CODE_PACKAGE *dns_code, _u32 dns_server_ip)
{
    _int32 ret_val = SUCCESS;
    _int32 send_len = 0;
    _u32 send_total_len = 0;
    SD_SOCKADDR dns_server_addr;

    if (dns_code == NULL)
    {
        return INVALID_ARGUMENT;
    }

    sd_memset(&dns_server_addr, 0, sizeof(dns_server_addr));
    dns_server_addr._sin_family = AF_INET;
    dns_server_addr._sin_port = sd_htons(0x35);
    dns_server_addr._sin_addr = dns_server_ip;
    while (send_total_len < dns_code->_len)
    {
        ret_val = sd_sendto(*sock, dns_code->_dns_code, dns_code->_len, &dns_server_addr, &send_len);
    	if(ret_val==32)
    	{
		LOG_ERROR("send_dns_request_package, 32: Broken pipe!");
		ret_val = sd_close_socket(*sock);
		sd_assert(ret_val==SUCCESS);
		ret_val = sd_create_socket(SD_AF_INET, SD_SOCK_DGRAM, 0, sock);
  	}

        if (SUCCESS != ret_val)
        {
            LOG_DEBUG("send_dns_request_package, sd_sendto, ERROR: ret_val=%d, sock=%d, dns_code_len=%d",
                ret_val, *sock, dns_code->_len);
            
            return ret_val;
        }
        send_total_len += send_len;
    }

    return SUCCESS;
}

_int32 read_dns_answer_package(_u32 sock, DNS_CODE_PACKAGE *dns_code, _u32 *recv_dns_server_ip)
{
    _int32 ret_val = SUCCESS;
    SD_SOCKADDR dns_server_addr;

    if (dns_code == NULL || recv_dns_server_ip == NULL)
    {
        return INVALID_ARGUMENT;
    }

    sd_memset(dns_code, 0, sizeof(DNS_CODE_PACKAGE));
    ret_val = sd_recvfrom(sock, dns_code->_dns_code, DNS_CODE_SIZE_MAX, &dns_server_addr, (_int32 *)&dns_code->_len);
    if (SUCCESS != ret_val)
    {
        return ret_val;
    }

    if (dns_server_addr._sin_port != sd_htons(0x0035))
    {
        /* 不是DNS包，返回. */
        return -1;
    }

    *recv_dns_server_ip = dns_server_addr._sin_addr;
    return SUCCESS;
}

/************************************************************************
 *
 * DNS Parser.
 *
 ************************************************************************/

_int32 dns_parser_init(DNS_PARSER *dns_parser)
{
    _int32 ret_val = SUCCESS;
    if (dns_parser == NULL)
    {
        return INVALID_ARGUMENT;
    }

    ret_val = dns_cache_init(&dns_parser->_dns_cache);
    CHECK_VALUE(ret_val);
    
    ret_val = dns_request_queue_init(&dns_parser->_dns_request_queue);
    CHECK_VALUE(ret_val);

    ret_val = sd_create_socket(SD_AF_INET, SD_SOCK_DGRAM, 0, &dns_parser->sock);
    CHECK_VALUE(ret_val);

    return SUCCESS;
}

_int32 dns_parser_uninit(DNS_PARSER *dns_parser)
{
    _int32 ret_val = SUCCESS;
    
    if (dns_parser == NULL)
    {
        return INVALID_ARGUMENT;
    }
    
    ret_val = sd_close_socket(dns_parser->sock);
    CHECK_VALUE(ret_val);

    return SUCCESS;
}

/*
 * 当前DNS解析器是否就绪。
 */
_int32 dns_parser_is_ready(DNS_PARSER * dns_parser, BOOL *is_ready)
{
    _int32 ret_val = SUCCESS;
    BOOL is_full = FALSE;

    if (dns_parser == NULL || is_ready == NULL)
    {
        return INVALID_ARGUMENT;
    }

    ret_val = dns_request_queue_is_full(&dns_parser->_dns_request_queue, &is_full);
    if (SUCCESS != ret_val)
    {
        return ret_val;
    }
    
    *is_ready = (is_full)? FALSE: TRUE;
    return SUCCESS;
}

/*
 * 是否为有效域名
 */
BOOL is_valid_host_name(char *host_name, _u32 host_name_len)
{
    _u32 i = 0;

    if (host_name == NULL || host_name[0] == 0)
    {
        return FALSE;
    }

    /* 超长或者未以'\0'结尾。 */
    if (host_name_len > MAX_HOST_NAME_LEN
        || host_name_len == 0
        || host_name[host_name_len - 1] != '\0')
    {
        return FALSE;
    }
    
    /* 是否存在非法字符 */
    for (i = 0; i < host_name_len - 1; ++i)
    {
        if (host_name[i] == '\0'
            || host_name[i] == '?'
            || host_name[i] == '/'
            || host_name[i] == '\\'
            || host_name[i] == '*'
            || host_name[i] == '"'
            || host_name[i] == '|')
        {
            return FALSE;
        }
    }

    return TRUE;
}

/*
 * 查询DNS。
 */
_int32 dns_parser_query(DNS_PARSER *dns_parser, char *host_name, _u32 host_name_len, _int32 *query_result,
    void *pdata, DNS_CONTENT_PACKAGE *content)
{
    _int32 ret_val = SUCCESS;
    _u32 now_time = 0;
    DNS_CODE_PACKAGE dns_code_package;
    DNS_REQUEST_RECORD dns_request_rec;
    _u32 dns_server_ip_index = 0;
    _u32 ip = 0;
    
    if (dns_parser == NULL || host_name == NULL || query_result == NULL || pdata == NULL || content == NULL)
    {
        return INVALID_ARGUMENT;
    }

    LOG_DEBUG("dns_parser_query: %s, len=%d.", host_name, host_name_len);

    /* 是否无效域名. */
    if (!is_valid_host_name(host_name, host_name_len))
    {
        /* Maybe the URL parsed error, assert it for debugging. */
        sd_assert(FALSE);

        sd_memset(content, 0, sizeof(DNS_CONTENT_PACKAGE));
        sd_strncpy(content->_host_name, host_name, host_name_len);
        content->_host_name_buffer_len = host_name_len;
        content->_result = DNS_INVALID_ADDR;
        
        *query_result = DNS_QUERY_RESULT_OK;
        return SUCCESS;
    }

    /* if host_name is ip, return this ip. */
	if(sd_inet_aton(host_name, &ip) == SUCCESS)    /* WARNNING: sd_inet_aton() is not thread safe. */
	{
        sd_memset(content, 0, sizeof(DNS_CONTENT_PACKAGE));
        sd_strncpy(content->_host_name, host_name, host_name_len);
        content->_host_name_buffer_len = host_name_len;
        content->_ip_list[0] = ip;
        content->_ttl[0] = 0xFFFFFFFF;
        content->_ip_count = 1;
        content->_result = SUCCESS;
        
        *query_result = DNS_QUERY_RESULT_OK;
        return SUCCESS;
	}

    /* nameserver 是否需要重读? */
    if (dns_server_ip_need_reload(&dns_parser->_dns_request_queue._dns_server_ip))
    {
        ret_val = dns_server_ip_load(&dns_parser->_dns_request_queue._dns_server_ip);
        CHECK_VALUE(ret_val);

        /* 标记 request queue 中的server_ip为无效。 */
        ret_val = dns_request_queue_mark_invalid_server(&dns_parser->_dns_request_queue);
        CHECK_VALUE(ret_val);

        /* 清空 cache 。 */
        dns_cache_clear(&dns_parser->_dns_cache);
        CHECK_VALUE(ret_val);
    }

    /* find in cache. */
    ret_val = dns_cache_query(&dns_parser->_dns_cache, host_name, content);
    if (SUCCESS == ret_val)
    {
        *query_result = DNS_QUERY_RESULT_OK;
        return SUCCESS;
    }

    sd_memset(content, 0, sizeof(DNS_CONTENT_PACKAGE));
    sd_strncpy(content->_host_name, host_name, host_name_len);
    content->_host_name_buffer_len = host_name_len;
    
    /* get nameserver ip. */
    sd_memset(&dns_request_rec, 0, sizeof(DNS_REQUEST_RECORD));
    ret_val = dns_server_ip_get_next(&dns_parser->_dns_request_queue._dns_server_ip,
        DNS_SERVER_IP_FIRST_GET, &dns_request_rec._retry_times, &dns_server_ip_index);

    if (DNS_SERVER_IP_ERROR_NO_SERVER == ret_val)
    {
        /* 配置文件中没有 nameserver ip. */
        LOG_DEBUG("no dns server ip.");

        content->_result = DNS_NO_SERVER;
        *query_result = DNS_QUERY_RESULT_OK;
        return SUCCESS;
    }
    else if (DNS_SERVER_IP_ERROR_NO_VALID_SERVER == ret_val)
    {
        /* 
         * 没有获取到有效的 nameserver ip （所有的nameserver都被投诉为无效）。
         * 处理：清空投诉记录之后重新获取 server ip，适用于 nameserver 在失效一段时间后又恢复正常的情况。 
         */
        dns_server_ip_complain_clear(&dns_parser->_dns_request_queue._dns_server_ip);
        ret_val = dns_server_ip_get_next(&dns_parser->_dns_request_queue._dns_server_ip,
            DNS_SERVER_IP_FIRST_GET, &dns_request_rec._retry_times, &dns_server_ip_index);
        CHECK_VALUE(ret_val);
    }
    
    /* package dns request. */
    ret_val = package_dns_request_package(host_name, &dns_code_package, &dns_parser->_dns_request_queue._cur_dns_id);
    CHECK_VALUE(ret_val);
    
    /* send dns request. */
    ret_val = send_dns_request_package(&dns_parser->sock, &dns_code_package, 
        dns_parser->_dns_request_queue._dns_server_ip._ip_list[dns_server_ip_index]);
    
    if (SUCCESS != ret_val)
    {
        content->_result = DNS_NETWORK_EXCEPTION;
        *query_result = DNS_QUERY_RESULT_OK;
        return SUCCESS;
    }
    
    /* push dns request queue. */
    ret_val = sd_time_ms(&now_time);
    CHECK_VALUE(ret_val);
    
    dns_request_rec._dns_id = dns_parser->_dns_request_queue._cur_dns_id;
    sd_strncpy(dns_request_rec._host_name, host_name, host_name_len);
    dns_request_rec._host_name_len = host_name_len;
    dns_request_rec._request_time = now_time;
    dns_request_rec._retry_times = 0;
    dns_request_rec._ttl = (DNS_REQUEST_TTL << dns_request_rec._retry_times);
    dns_request_rec._dns_server_ip_index = dns_server_ip_index;
    dns_request_rec._data = pdata;
    
    ret_val = dns_request_queue_push(&dns_parser->_dns_request_queue, &dns_request_rec);
    CHECK_VALUE(ret_val);
    
    LOG_DEBUG("dns send & push request: %s, dns_server_count=%d, retry=%d, dns_server[%d]=0x%x, dns_id=%d, ttl=(%u + %u)",
        dns_request_rec._host_name,
        dns_parser->_dns_request_queue._dns_server_ip._ip_count,
        dns_request_rec._retry_times,
        dns_request_rec._dns_server_ip_index,
        dns_parser->_dns_request_queue._dns_server_ip._ip_list[dns_request_rec._dns_server_ip_index],
        dns_request_rec._dns_id, dns_request_rec._request_time, dns_request_rec._ttl);

    *query_result = DNS_QUERY_RESULT_ASYNC_REQUEST;
    return SUCCESS;
}

/*
 * 获取DNS解析结果。
 */
_int32 dns_parser_get(DNS_PARSER * dns_parser, _int32 *ack_result, void **ppdata, DNS_CONTENT_PACKAGE *content)
{
    _int32 ret_val = SUCCESS;
    _u32 dns_id = 0;
    _u32 now_time = 0;
    DNS_CODE_PACKAGE dns_code_package;
    DNS_REQUEST_RECORD dns_request_rec;
    _u32 dns_server_ip_index = 0;
    _u32 recv_dns_server_ip = 0;
    char dns_dump[512] = {0};

    if (dns_parser == NULL || ack_result == NULL || ppdata == NULL || content == NULL)
    {
        return INVALID_ARGUMENT;
    }

    sd_memset(&dns_code_package, 0, sizeof(DNS_CODE_PACKAGE));
    sd_memset(&dns_request_rec, 0, sizeof(DNS_REQUEST_RECORD));

    /* read dns answer. */
    ret_val = read_dns_answer_package(dns_parser->sock, &dns_code_package, &recv_dns_server_ip);
    if (SUCCESS == ret_val)
    {
        /* parse dns answer. */
        ret_val = parse_dns_answer_package(&dns_code_package, content, &dns_id);
        if (SUCCESS != ret_val)
        {
            /* parse fail, dump it. */
            ret_val = str2hex(dns_code_package._dns_code, dns_code_package._len,
                dns_dump, 512);
            
            LOG_DEBUG("dns parse fail. dump:[len=%d,%s].", dns_code_package._len, dns_dump);
        }
        else
        {
            /* parse ok. */
            LOG_DEBUG("dns read & parse: %s, from 0x%x, ip_count=%d, ip[0]=0x%x, dns_id=%d, ttl=(%u + %u)",
                content->_host_name, recv_dns_server_ip, content->_ip_count,
                content->_ip_list[0], dns_id, 
                content->_start_time, content->_ttl[0]);

            /* pop from queue. */
            dns_request_rec._dns_id = dns_id;
            sd_strncpy(dns_request_rec._host_name, content->_host_name, content->_host_name_buffer_len);
            dns_request_rec._host_name_len = content->_host_name_buffer_len;
            ret_val = dns_request_queue_pop(&dns_parser->_dns_request_queue,
                &dns_request_rec, DNS_REQUEST_QUEUE_POP_FOR_ANSWER);
            if (DNS_REQUEST_QUEUE_NOT_FOUND == ret_val)
            {
                /* maybe the request has been cannelled or expired. */
                LOG_DEBUG("the record is not in request queue, maybe cannel or expired. %s", content->_host_name);
            }
            else if (SUCCESS == ret_val)
            {
                /* nameserver 被重置或 nameserver 返回错误。 */
                if ((DNS_SERVER_IP_COUNT_MAX <= dns_request_rec._dns_server_ip_index)
                    || (content->_ip_count == 0))
                {
                    if (DNS_SERVER_IP_COUNT_MAX <= dns_request_rec._dns_server_ip_index)
                    {
                        /* nameserver 已失效，重发请求。 */
                        LOG_DEBUG("reset server ip (invalid ip). %s", dns_request_rec._host_name);
                        ret_val = dns_server_ip_get_next(&dns_parser->_dns_request_queue._dns_server_ip, 
                            DNS_SERVER_IP_FIRST_GET, &dns_request_rec._retry_times, &dns_server_ip_index);

                        if (SUCCESS != ret_val)
                        {
                            LOG_DEBUG("reset server ip, but no valid server ip. (invalid ip): %s. ip_count=%d, ip[0]=0x%x, "
                                "ip[1]=0x%x, ip[2]=0x%x", dns_request_rec._host_name,
                                dns_parser->_dns_request_queue._dns_server_ip._ip_count,
                                dns_parser->_dns_request_queue._dns_server_ip._ip_list[0],
                                dns_parser->_dns_request_queue._dns_server_ip._ip_list[1],
                                dns_parser->_dns_request_queue._dns_server_ip._ip_list[2]);
                            
                            content->_result = DNS_NO_SERVER;
                            *ppdata = dns_request_rec._data;
                            *ack_result = DNS_ACK_RESULT_OK;
                            return SUCCESS;
                        }
                    }
                    else
                    {
                        /* nameserver返回错误，则获取下一个nameserver重发。 */
                        ret_val = dns_server_ip_get_next(&dns_parser->_dns_request_queue._dns_server_ip, 
                            dns_request_rec._dns_server_ip_index, &dns_request_rec._retry_times, &dns_server_ip_index);
                        
                        /* if all dns servers return error. */
                        if (SUCCESS != ret_val)
                        {
                            LOG_DEBUG("all dns servers return error: %s", content->_host_name);
                        
                            content->_result = DNS_INVALID_ADDR;
                            content->_ttl[0] = DNS_CHCHE_INVAIL_DNS_TTL;
                            ret_val = sd_time_ms(&now_time);
                            CHECK_VALUE(ret_val);
                            content->_start_time = now_time;
                            
                            ret_val = dns_cache_append(&dns_parser->_dns_cache, content);
                            CHECK_VALUE(ret_val);
                        
                            *ppdata = dns_request_rec._data;
                            *ack_result = DNS_ACK_RESULT_OK;
                            return SUCCESS;
                        }
                    }
                    
                    /* resend to another dns server. */
                    ret_val = package_dns_request_package(dns_request_rec._host_name, &dns_code_package,
                        &dns_parser->_dns_request_queue._cur_dns_id);
                    
                    if (SUCCESS == ret_val)
                    {
                        ret_val = send_dns_request_package(&dns_parser->sock, &dns_code_package, 
                            dns_parser->_dns_request_queue._dns_server_ip._ip_list[dns_server_ip_index]);

                        if (SUCCESS != ret_val)
                        {
                            *ppdata = dns_request_rec._data;
                            content->_result = DNS_NETWORK_EXCEPTION;
                            *ack_result = DNS_ACK_RESULT_OK;
                            return SUCCESS;
                        }
                        
                        ret_val = sd_time_ms(&now_time);
                        CHECK_VALUE(ret_val);

                        /* same items: _host_name, _host_name_len, _data. */
                        dns_request_rec._dns_id = dns_parser->_dns_request_queue._cur_dns_id;
                        dns_request_rec._request_time = now_time;
                        dns_request_rec._dns_server_ip_index = dns_server_ip_index;
                        dns_request_rec._ttl = (DNS_REQUEST_TTL << dns_request_rec._retry_times);
                        
                        ret_val = dns_request_queue_push(&dns_parser->_dns_request_queue, &dns_request_rec);
                        CHECK_VALUE(ret_val);
                        
                        LOG_DEBUG("dns resend & push request (invalid ip): %s, dns_server_count=%d, retry=%d, dns_server[%d]=0x%x, dns_id=%d, ttl=(%u + %u)",
                            dns_request_rec._host_name,
                            dns_parser->_dns_request_queue._dns_server_ip._ip_count,
                            dns_request_rec._retry_times,
                            dns_request_rec._dns_server_ip_index,
                            dns_parser->_dns_request_queue._dns_server_ip._ip_list[dns_request_rec._dns_server_ip_index],
                            dns_request_rec._dns_id, dns_request_rec._request_time, dns_request_rec._ttl);
                    }
                }
                else
                {
                    /* dns server 返回解析成功（nameserver 未被重置）。 */
                    *ppdata = dns_request_rec._data;
                    content->_result = SUCCESS;

                    ret_val = dns_cache_append(&dns_parser->_dns_cache, content);
                    CHECK_VALUE(ret_val);
                    
                    *ack_result = DNS_ACK_RESULT_OK;
                    return SUCCESS;
                }
            }
        }
    }

    /* refresh the expired dns request. */
    if (SUCCESS == dns_request_queue_pop(&dns_parser->_dns_request_queue, 
        &dns_request_rec, DNS_REQUEST_QUEUE_POP_FOR_EXPIRED))
    {
        sd_memset(content, 0, sizeof(DNS_CONTENT_PACKAGE));
        sd_strncpy(content->_host_name, dns_request_rec._host_name, dns_request_rec._host_name_len);
        content->_host_name_buffer_len = dns_request_rec._host_name_len;

        /* 判断 nameserver 是否被重置。 */
        if (DNS_SERVER_IP_COUNT_MAX <= dns_request_rec._dns_server_ip_index)
        {
            /* nameserver 已失效，重发请求。 */
            LOG_DEBUG("reset server ip (expired). %s", dns_request_rec._host_name);
            ret_val = dns_server_ip_get_next(&dns_parser->_dns_request_queue._dns_server_ip, 
                DNS_SERVER_IP_FIRST_GET, &dns_request_rec._retry_times, &dns_server_ip_index);

            if (SUCCESS != ret_val)
            {
                LOG_DEBUG("reset server ip, but no valid server ip. (expired): %s. ip_count=%d, ip[0]=0x%x, "
                    "ip[1]=0x%x, ip[2]=0x%x", dns_request_rec._host_name,
                    dns_parser->_dns_request_queue._dns_server_ip._ip_count,
                    dns_parser->_dns_request_queue._dns_server_ip._ip_list[0],
                    dns_parser->_dns_request_queue._dns_server_ip._ip_list[1],
                    dns_parser->_dns_request_queue._dns_server_ip._ip_list[2]);
                            
                content->_result = DNS_NO_SERVER;
                *ppdata = dns_request_rec._data;
                *ack_result = DNS_ACK_RESULT_OK;
                return SUCCESS;
            }
        }
        else
        {
            /* complain the dns server ip is invalid. */
            ret_val = dns_server_ip_complain(&dns_parser->_dns_request_queue._dns_server_ip, dns_request_rec._host_name,
                dns_request_rec._dns_server_ip_index);
            
            /* get next dns server ip. */
            ret_val = dns_server_ip_get_next(&dns_parser->_dns_request_queue._dns_server_ip, 
                dns_request_rec._dns_server_ip_index, &dns_request_rec._retry_times, &dns_server_ip_index);

            /* if all dns servers expired. */
            if (SUCCESS != ret_val)
            {
                LOG_DEBUG("remove the expired request. %s, dns_server_count=%d, dns_server[%d]=0x%x, dns_id=%d, ttl=(%u + %u)",
                    dns_request_rec._host_name,
                    dns_parser->_dns_request_queue._dns_server_ip._ip_count,
                    dns_request_rec._dns_server_ip_index,
                    dns_parser->_dns_request_queue._dns_server_ip._ip_list[dns_request_rec._dns_server_ip_index],
                    dns_request_rec._dns_id, dns_request_rec._request_time, dns_request_rec._ttl);
                    
                content->_result = DNS_INVALID_ADDR;
                content->_ttl[0] = DNS_CHCHE_INVAIL_DNS_TTL;
                ret_val = sd_time_ms(&now_time);
                CHECK_VALUE(ret_val);
                content->_start_time = now_time;
                ret_val = dns_cache_append(&dns_parser->_dns_cache, content);
                CHECK_VALUE(ret_val);
                
                *ppdata = dns_request_rec._data;
                *ack_result = DNS_ACK_RESULT_OK;
                return SUCCESS;
            }
        }
        
        /* resend to another dns server. */
        ret_val = package_dns_request_package(dns_request_rec._host_name, &dns_code_package,
            &dns_parser->_dns_request_queue._cur_dns_id);
        
        if (SUCCESS == ret_val)
        {
            ret_val = send_dns_request_package(&dns_parser->sock, &dns_code_package, 
                dns_parser->_dns_request_queue._dns_server_ip._ip_list[dns_server_ip_index]);

            if (SUCCESS != ret_val)
            {
                *ppdata = dns_request_rec._data;
                content->_result = DNS_NETWORK_EXCEPTION;
                *ack_result = DNS_ACK_RESULT_OK;
                return SUCCESS;
            }
            
            ret_val = sd_time_ms(&now_time);
            CHECK_VALUE(ret_val);
        
            /* same items: _host_name, _host_name_len, _data. */
            dns_request_rec._dns_id = dns_parser->_dns_request_queue._cur_dns_id;
            dns_request_rec._request_time = now_time;
            dns_request_rec._dns_server_ip_index = dns_server_ip_index;
            dns_request_rec._ttl = (DNS_REQUEST_TTL << dns_request_rec._retry_times);
            
            ret_val = dns_request_queue_push(&dns_parser->_dns_request_queue, &dns_request_rec);
            CHECK_VALUE(ret_val);
            
            LOG_DEBUG("dns resend & push request (expired): %s, dns_server_count=%d, retry=%d, dns_server[%d]:0x%x, dns_id=%d, ttl=(%u + %u ~ %u)",
                dns_request_rec._host_name,
                dns_parser->_dns_request_queue._dns_server_ip._ip_count,
                dns_request_rec._retry_times,
                dns_request_rec._dns_server_ip_index,
                dns_parser->_dns_request_queue._dns_server_ip._ip_list[dns_request_rec._dns_server_ip_index],
                dns_request_rec._dns_id, dns_request_rec._request_time, dns_request_rec._ttl, now_time);
        }
    }

    *ack_result = DNS_ACK_RESULT_NOT_GET_ANSWER;
    return SUCCESS;
}

/*
 * 取消DNS请求。
 */
_int32 dns_parser_query_cancel(DNS_PARSER * dns_parser, _u32 index)
{

    DNS_REQUEST_RECORD record;
    _int32 ret_val = SUCCESS;
    if (dns_parser == NULL)
    {
        return INVALID_ARGUMENT;
    }
    //DNS_REQUEST_RECORD record;
    ret_val = dns_request_queue_pop_by_index(&dns_parser->_dns_request_queue, index, &record);
    return ret_val;
}

/*
 * 是否所有的DNS请求都已解析完成。
 */
_int32 dns_parser_is_finished(DNS_PARSER * dns_parser, BOOL *is_finished)
{
    if (dns_parser == NULL || is_finished == NULL)
    {
        return INVALID_ARGUMENT;
    }
    return dns_request_queue_is_empty(&dns_parser->_dns_request_queue, is_finished);
}

/*
 * 队列中未处理的DNS请求信息个数。
 */
_int32 dns_parser_request_count(DNS_PARSER * dns_parser, _u32 *count)
{
    if (dns_parser == NULL || count == NULL)
    {
        return INVALID_ARGUMENT;
    }
    *count = dns_parser->_dns_request_queue._size;
    return SUCCESS;
}

/*
 * 队列中未处理的DNS请求信息。
 */
_int32 dns_parser_request_record_const(DNS_PARSER * dns_parser, _u32 index, DNS_REQUEST_RECORD **precord)
{
    if (dns_parser == NULL || precord == NULL)
    {
        return INVALID_ARGUMENT;
    }

    if (index >= dns_parser->_dns_request_queue._size)
    {
        LOG_DEBUG("dns_parser_query_record_const: (%d >= %d)", index, dns_parser->_dns_request_queue._size);
        return -1;
    }

    *precord = &(dns_parser->_dns_request_queue._record[index]);
    return SUCCESS;
}

/*
 * DNS Server 个数。
 */
_int32 dns_parser_dns_server_count(DNS_PARSER * dns_parser, _u32 *count)
{
    if (dns_parser == NULL || count == NULL)
    {
        return INVALID_ARGUMENT;
    }
    *count = dns_parser->_dns_request_queue._dns_server_ip._ip_count;
    if (*count > DNS_SERVER_IP_COUNT_MAX)
    {
        LOG_DEBUG("dns_parser_dns_server_count: (%d >= %d)", *count, DNS_SERVER_IP_COUNT_MAX);
        *count = DNS_SERVER_IP_COUNT_MAX;
    }
    return SUCCESS;
}

/*
 * DNS Server IP 信息。
 */
_int32 dns_parser_dns_server_info(DNS_PARSER * dns_parser, _u32 index, _u32 *dns_server_ip)
{
    if (dns_parser == NULL || dns_server_ip == NULL)
    {
        return INVALID_ARGUMENT;
    }

    if (index >= dns_parser->_dns_request_queue._dns_server_ip._ip_count)
    {
        return -1;
    }

    *dns_server_ip = dns_parser->_dns_request_queue._dns_server_ip._ip_list[index];
    return SUCCESS;
}

