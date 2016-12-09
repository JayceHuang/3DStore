#include "utility/sd_assert.h"

#include "pipe_function_table.h"


static void *g_common_pipe_function_table[MAX_FUNC_NUM];

#ifdef ENABLE_BT 
static void* g_speedup_pipe_function_table[MAX_FUNC_NUM];
static void* g_bt_pipe_function_table[MAX_FUNC_NUM];
#endif

void **pft_get_common_pipe_function_table(void)
{
	return g_common_pipe_function_table;
}

#ifdef ENABLE_BT 
void **pft_get_speedup_pipe_function_table(void)
{
	return g_speedup_pipe_function_table;
}

void **pft_get_bt_pipe_function_table(void)
{
	return g_bt_pipe_function_table;
}
#endif

void pft_register_change_range_handler(void **p_func_table, change_range_handler p_change_range_handler)
{
	sd_assert(CHANGE_PIPE_RANGE_FUNC < MAX_FUNC_NUM);
	p_func_table[CHANGE_PIPE_RANGE_FUNC] = (void *)p_change_range_handler;
}

void pft_register_get_dispatcher_range_handler(void **p_func_table, get_dispatcher_range_handler p_get_dispatcher_range_handler)
{
	sd_assert(GET_DISPATCHER_RANGE_FUNC < MAX_FUNC_NUM);
	p_func_table[GET_DISPATCHER_RANGE_FUNC] = (void *)p_get_dispatcher_range_handler;
}

void pft_register_set_dispatcher_range_handler(void **p_func_table, set_dispatcher_range_handler p_set_dispatcher_range_handler)
{
	sd_assert(SET_DISPATCHER_RANGE_FUNC < MAX_FUNC_NUM);
	p_func_table[SET_DISPATCHER_RANGE_FUNC] = (void *)p_set_dispatcher_range_handler;
}

void pft_register_get_file_size_handler(void **p_func_table, get_file_size_handler p_get_file_size_handler)
{
	sd_assert(GET_FILE_SIZE_FUNC < MAX_FUNC_NUM);
	p_func_table[GET_FILE_SIZE_FUNC] = (void *)p_get_file_size_handler;
}

void pft_register_set_file_size_handler(void **p_func_table, set_file_size_handler p_set_file_size_handler)
{
	sd_assert(SET_FILE_SIZE_FUNC < MAX_FUNC_NUM);
	p_func_table[SET_FILE_SIZE_FUNC] = (void *)p_set_file_size_handler;
}


void pft_register_get_data_buffer_handler(void **p_func_table, get_data_buffer_handler p_get_data_buffer_handler)
{
	sd_assert(GET_DATA_BUFFER_FUNC < MAX_FUNC_NUM);
	p_func_table[GET_DATA_BUFFER_FUNC] = (void *)p_get_data_buffer_handler;
}

void pft_register_free_data_buffer_handler(void **p_func_table, free_data_buffer_handler p_free_data_buffer_handler)
{
	sd_assert(FREE_DATA_BUFFER_FUNC < MAX_FUNC_NUM);
	p_func_table[FREE_DATA_BUFFER_FUNC] = (void *)p_free_data_buffer_handler;
}

void pft_register_put_data_handler(void **p_func_table, put_data_handler p_put_data_handler)
{
	sd_assert(PUT_DATA_FUNC < MAX_FUNC_NUM);
	p_func_table[PUT_DATA_FUNC] = (void *)p_put_data_handler;
}

void pft_register_read_data_handler(void **p_func_table, read_data_handler p_put_data_handler)
{
	sd_assert( READ_DATA_FUNC < MAX_FUNC_NUM );
	p_func_table[READ_DATA_FUNC] = (void *)p_put_data_handler;
}

void pft_register_notify_dispatch_data_finish_handler(void **p_func_table, notify_dispatch_data_finish_handler p_notify_dispatch_data_finish_handler)
{
	sd_assert(NOTIFY_DISPATCH_DATA_FINISHED_FUNC < MAX_FUNC_NUM);
	p_func_table[NOTIFY_DISPATCH_DATA_FINISHED_FUNC] = (void *)p_notify_dispatch_data_finish_handler;
}

void pft_register_get_file_type_handler(void **p_func_table, get_file_type_handler p_get_file_type_handler)
{
	sd_assert(GET_FILE_TYPE_FUNC < MAX_FUNC_NUM);
	p_func_table[GET_FILE_TYPE_FUNC ] = (void *)p_get_file_type_handler;
}

void pft_register_get_checked_range_list_handler(void **p_func_table, get_checked_range_list_handler p_get_checked_range_list_handler)
{
	sd_assert(GET_CHECKED_RANGE_LIST_FUNC < MAX_FUNC_NUM);
	p_func_table[GET_CHECKED_RANGE_LIST_FUNC ] = (void *)p_get_checked_range_list_handler;
}

void pft_clear_func_table(void **p_func_table)
{
	sd_memset(p_func_table, 0, MAX_FUNC_NUM);
}

