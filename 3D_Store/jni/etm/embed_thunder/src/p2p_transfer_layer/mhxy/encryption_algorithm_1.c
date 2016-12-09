#include "encryption_algorithm_1.h"

#ifdef _SUPPORT_MHXY

#include "encryption_algorithm.h"
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <stdio.h>

BOOL encryption_algorithm_create_key_1(ENCRYPTION_ALGORITHM *p_algorithm, const _u8 *data, _u32 len, void *key, _u32 *p_key_len )
{
    // 创建首部
    if (len == 0)
    {
        p_algorithm->_key_len = 0;

        // 加密方法
        _u32 head = ((8192*1 + rand() % 8192) << 16) + rand() % 65536; // [0x20000000, 0x3FFFFFFF]
        memcpy(p_algorithm->_key, &head, 4);
        p_algorithm->_key_len += 4;

        // 随机长度
        _u8 rand_len = rand();
        memcpy(p_algorithm->_key+p_algorithm->_key_len, &rand_len, 1);
        p_algorithm->_key_len += 1;
        rand_len = rand_len %4 + 2;

        // 填充字符
        _u8 padding;
        _u8 i=0;
        for (i=0; i<rand_len; i++)
        {
            padding = rand();
            memcpy(p_algorithm->_key+p_algorithm->_key_len, &padding, 1);
            p_algorithm->_key_len += 1;
        }

        // 校验字符
        padding = (p_algorithm->_key[p_algorithm->_key_len-1]*13 ^ ((p_algorithm->_key_len+2)*7));
        memcpy(p_algorithm->_key+p_algorithm->_key_len, &padding, 1);
        p_algorithm->_key_len += 1;
        padding = (p_algorithm->_key[p_algorithm->_key_len-1]*13 ^ ((p_algorithm->_key_len+2)*7));
        memcpy(p_algorithm->_key+p_algorithm->_key_len, &padding, 1);
        p_algorithm->_key_len += 1;

        // 拷贝首部
        assert(key != NULL);
        memcpy(key, p_algorithm->_key, p_algorithm->_key_len);
        *p_key_len = p_algorithm->_key_len;

        p_algorithm->_key_pos = 0;
        return TRUE;
    }
    // 解析首部
    else
    {
        if (len < 5)
        {
            return FALSE;
        }

        // 首部长度
        p_algorithm->_key_len = data[4] % 4 + 9;
        if (len < p_algorithm->_key_len)
        {
            return FALSE;
        }

        *p_key_len = p_algorithm->_key_len;
        memcpy(p_algorithm->_key, data, p_algorithm->_key_len);

        // 检查校验字符
        _u8 padding;
        padding = (p_algorithm->_key[p_algorithm->_key_len-2]*13 ^ ((p_algorithm->_key_len+1)*7));
        if (p_algorithm->_key[p_algorithm->_key_len-1] != padding)
        {
            return FALSE;
        }
        padding = (p_algorithm->_key[p_algorithm->_key_len-3]*13 ^ ((p_algorithm->_key_len+0)*7));
        if (p_algorithm->_key[p_algorithm->_key_len-2] != padding)
        {
            return FALSE;
        }

        p_algorithm->_key_pos = 0;
        return TRUE;
    }

}

BOOL encryption_algorithm_encrypt_1(ENCRYPTION_ALGORITHM *p_algorithm, _u8 *in_buffer, _u32 len )
{
    _u32 next;
    _u32 i = 0;
    for (i=0; i<len; i++)
    {
        next = p_algorithm->_key_pos + 1;
        if (next == p_algorithm->_key_len)
        {
            next = 0;
        }
        p_algorithm->_key[p_algorithm->_key_pos] = p_algorithm->_key[p_algorithm->_key_pos] ^ (p_algorithm->_key[next]+7*13);
        in_buffer[i] = in_buffer[i] ^ p_algorithm->_key[p_algorithm->_key_pos];
        p_algorithm->_key_pos = next;
    }
    return TRUE;
}

BOOL encryption_algorithm_decrypt_1(ENCRYPTION_ALGORITHM *p_algorithm, _u8 *in_buffer, _u32 len )
{
    _u32 next;
    _u32 i=0;
    for (i=0; i<len; i++)
    {
        next = p_algorithm->_key_pos + 1;
        if (next == p_algorithm->_key_len)
        {
            next = 0;
        }
        p_algorithm->_key[p_algorithm->_key_pos] = p_algorithm->_key[p_algorithm->_key_pos] ^ (p_algorithm->_key[next]+7*13);
        in_buffer[i] = in_buffer[i] ^ p_algorithm->_key[p_algorithm->_key_pos];
        p_algorithm->_key_pos = next;
    }
    return TRUE;
}

BOOL encryption_algorithm_1_new(struct tagENCRYPTION_ALGORITHM ** pp_algorithm)
{
    *pp_algorithm = (ENCRYPTION_ALGORITHM *)malloc(sizeof( ENCRYPTION_ALGORITHM ));
    memset(*pp_algorithm, 0, sizeof( ENCRYPTION_ALGORITHM ));
    (*pp_algorithm)->create_key = encryption_algorithm_create_key_1;
    (*pp_algorithm)->encrypt = encryption_algorithm_encrypt_1;
    (*pp_algorithm)->decrypt = encryption_algorithm_decrypt_1;
}

#endif //#ifdef _SUPPORT_MHXY

