#include "calc_hash_helper.h"
#include "utility/string.h"
#include "asyn_frame/msg.h"
#include "utility/sha1.h"
#include "asyn_frame/device.h"
#include "asyn_frame/calc_hash_reactor.h"
#include "utility/mempool.h"

#include "utility/logid.h"
#define LOGID LOGID_FILE_INFO
#include "utility/logger.h"

#define MAX_PENDING_COUNT 10

static _u32 g_timer_id = INVALID_MSG_ID;
static HASH_CALCULATOR* g_hash_calc = NULL;

static _u32 g_calc_hash_msgid = INVALID_MSG_ID;
static _int32 calc_hash_msg_handler(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid);
static _int32 post_calc_operation();
_int32 chh_init(HASH_CALCULATOR* calculator
                , void* user_data
                , notify_calc_hash_result callback_func)
{
    _int32 ret = SUCCESS;
    sd_memset(calculator, 0, sizeof(HASH_CALCULATOR));
    list_init(&calculator->_need_calc_hash_info_list);
    calculator->_user_data = user_data;
    calculator->_callback = callback_func;
    calculator->_msg_id = INVALID_MSG_ID;
    return ret;
}

_int32 chh_uninit(HASH_CALCULATOR* calculator)
{
    _int32 ret = SUCCESS;
    chh_cancel_all_items(calculator);
    return ret;
}

_int32 post_calc_operation(HASH_CALCULATOR* calculator)
{
    _int32 ret = SUCCESS;
    if(calculator->_cur_pending_item != NULL )
        return ret;
    list_pop(&calculator->_need_calc_hash_info_list, (void**)&calculator->_cur_pending_item);
    if(calculator->_cur_pending_item == NULL)
        return ret;
    MSG_INFO msg_info = {};
    msg_info._device_id = 0;
    msg_info._device_type = DEVICE_NONE;
    msg_info._msg_priority = MSG_PRIORITY_NORMAL;
    msg_info._operation_type = OP_CALC_HASH;
    msg_info._user_data = calculator;
    msg_info._operation_parameter = calculator->_cur_pending_item;
    LOG_DEBUG("post_calc_operation, cur_pending_item.block_no = %d", calculator->_cur_pending_item->_block_no);
    return post_message(&msg_info, calc_hash_msg_handler, NOTICE_ONCE, WAIT_INFINITE, &calculator->_msg_id);
}

_int32 chh_insert_item(HASH_CALCULATOR* calculator, CHH_HASH_INFO* item)
{
    _int32 ret = SUCCESS;
    LOG_DEBUG("chh_insert_item enter, calculator = 0x%x, hash_info.block_no = %d"
        , calculator
        , item->_block_no);
    list_push(&calculator->_need_calc_hash_info_list, (void*)item);
    return post_calc_operation(calculator);
}

_int32 chh_cancel_all_items(HASH_CALCULATOR* calculator)
{
    CHH_HASH_INFO* del_item = NULL;
    while (list_size(&calculator->_need_calc_hash_info_list))
    {
        list_pop(&calculator->_need_calc_hash_info_list, (void**)&del_item);
        sd_free(del_item->_data_buffer);
        sd_free(del_item->_hash_result);
        sd_free(del_item);
        del_item = NULL;
    }
    // 这里有问题， 会把正确的操作msgid和之前cancel的id混乱， id分开， 把calculator丢到calc_hash_helper来释放
    if (calculator->_msg_id != INVALID_MSG_ID)
    {
        LOG_DEBUG("chh_cancel_all_items, calculator->_msg_id = %d", calculator->_msg_id);
        cancel_message_by_msgid(calculator->_msg_id);
        sd_free(calculator->_cur_pending_item);
        calculator->_cur_pending_item = NULL;
        calculator->_msg_id = INVALID_MSG_ID;
    }
    return SUCCESS;
}

BOOL chh_can_put_item(HASH_CALCULATOR* calculator)
{
    return list_size(&calculator->_need_calc_hash_info_list) < MAX_PENDING_COUNT ? TRUE : FALSE;
}

_int32 calc_hash_msg_handler(const MSG_INFO *msg_info
                             , _int32 errcode
                             , _u32 notice_count_left
                             , _u32 elapsed
                             , _u32 msgid)
{
    _int32 ret = SUCCESS;
    CHH_HASH_INFO* hash_info = NULL;
    HASH_CALCULATOR* hash_calc = NULL;
    // cancel掉的item,直接从msg_info的parameter中删除内存, CHH_HASH_INFO内存已经在cancel_msg的时候delete掉了
    if (errcode == MSG_CANCELLED)
    {
        LOG_DEBUG("calc_hash_msg_handler enter, errcode == MSG_CANCELLED");
        hash_info = (CHH_HASH_INFO*)msg_info->_operation_parameter;
        sd_free(hash_info->_data_buffer);
        sd_free(hash_info->_hash_result);
        return ret;
    }
    hash_calc = (HASH_CALCULATOR*)msg_info->_user_data;
    sd_assert(msgid == hash_calc->_msg_id);
    hash_calc->_msg_id = INVALID_MSG_ID;
    hash_calc->_callback(hash_calc->_cur_pending_item, hash_calc->_user_data);
    hash_calc->_cur_pending_item = NULL;

    return post_calc_operation(hash_calc);
}

