#include "asyn_frame/device.h"
#include "asyn_frame/msg_alloctor.h"
#include "utility/errcode.h"
#include "utility/string.h"
#include "utility/mempool.h"


typedef _int32 (*op_alloc)(void **dest, const void *src);
typedef _int32 (*op_dealloc)(void *dest);

_int32 op_null_alloc(void **dest, const void *src);
_int32 op_null_dealloc(void *dest);

_int32 op_dns_alloc(void **dest, const void *src);
_int32 op_dns_dealloc(void *dest);

_int32 op_accept_alloc(void **dest, const void *src);
_int32 op_accept_dealloc(void *dest);

_int32 op_csrw_alloc(void **dest, const void *src);
_int32 op_csrw_dealloc(void *dest);

_int32 op_ncsrw_alloc(void **dest, const void *src);
_int32 op_ncsrw_dealloc(void *dest);

_int32 op_fso_alloc(void **dest, const void *src);
_int32 op_fso_dealloc(void *dest);

_int32 op_fsrw_alloc(void **dest, const void *src);
_int32 op_fsrw_dealloc(void *dest);

_int32 op_sockaddr_alloc(void **dest, const void *src);
_int32 op_sockaddr_dealloc(void *dest);

_int32 op_calc_hash_alloc(void **dest, const void *src);
_int32 op_calc_hash_dealloc(void *dest);

#if defined(LINUX) 
static void* g_fun_table[][2] = {
	{op_dns_alloc, op_dns_dealloc},
	{op_accept_alloc, op_accept_dealloc},
	{op_csrw_alloc, op_csrw_dealloc},
	{op_csrw_alloc, op_csrw_dealloc},
	{op_ncsrw_alloc, op_ncsrw_dealloc},
	{op_ncsrw_alloc, op_ncsrw_dealloc},
	{op_sockaddr_alloc, op_sockaddr_dealloc},
	{op_fso_alloc, op_fso_dealloc},
	{op_fsrw_alloc, op_fsrw_dealloc},
	{op_fsrw_alloc, op_fsrw_dealloc},
	{op_null_alloc, op_null_dealloc},
    {op_calc_hash_alloc, op_calc_hash_dealloc}
};
#elif defined(WINCE)
static void* g_fun_table[][2] = {
	{(void*)op_dns_alloc, (void*)op_dns_dealloc},
	{(void*)op_accept_alloc, (void*)op_accept_dealloc},
	{(void*)op_csrw_alloc, (void*)op_csrw_dealloc},
	{(void*)op_csrw_alloc, (void*)op_csrw_dealloc},
	{(void*)op_ncsrw_alloc, (void*)op_ncsrw_dealloc},
	{(void*)op_ncsrw_alloc, (void*)op_ncsrw_dealloc},
	{(void*)op_sockaddr_alloc, (void*)op_sockaddr_dealloc},
	{(void*)op_fso_alloc, (void*)op_fso_dealloc},
	{(void*)op_fsrw_alloc, (void*)op_fsrw_dealloc},
	{(void*)op_fsrw_alloc, (void*)op_fsrw_dealloc},
    {(void*)op_null_dealloc, (void*)op_null_dealloc},
    {(void*)op_calc_hash_alloc, (void*)op_calc_hash_dealloc}
	
};
#else
static void* g_fun_table[][2] = {
	{(void*)op_dns_alloc, (void*)op_dns_dealloc},
	{(void*)op_accept_alloc, (void*)op_accept_dealloc},
	{(void*)op_csrw_alloc, (void*)op_csrw_dealloc},
	{(void*)op_csrw_alloc, (void*)op_csrw_dealloc},
	{(void*)op_ncsrw_alloc, (void*)op_ncsrw_dealloc},
	{(void*)op_ncsrw_alloc, (void*)op_ncsrw_dealloc},
	{(void*)op_sockaddr_alloc, (void*)op_sockaddr_dealloc},
	{(void*)op_fso_alloc, (void*)op_fso_dealloc},
	{(void*)op_fsrw_alloc, (void*)op_fsrw_dealloc},
	{(void*)op_fsrw_alloc, (void*)op_fsrw_dealloc},
    {(void*)op_null_dealloc, (void*)op_null_dealloc},
    { (void*)op_calc_hash_alloc, (void*)op_calc_hash_dealloc}
	
};
#endif

_int32 op_null_alloc(void **dest, const void *src)
{
	_int32 ret_val = SUCCESS;
	*dest = NULL;
	return ret_val;
}

_int32 op_null_dealloc(void *dest)
{
	_int32 ret_val = SUCCESS;
	dest = NULL;
	return ret_val;
}

_int32 op_dns_alloc(void **dest, const void *src)
{
	_int32 ret_val = SUCCESS;
	OP_PARA_DNS *pdns = NULL;
	char *host = NULL;

	ret_val = para_dns_alloc(dest);
	CHECK_VALUE(ret_val);

	pdns = (OP_PARA_DNS*)(*dest);

	sd_memcpy(*dest, src, sizeof(OP_PARA_DNS));

	host = pdns->_host_name;

	/* alloc new mem to save path */
	ret_val = sd_malloc(pdns->_host_name_buffer_len + 1, (void**)&pdns->_host_name);
	CHECK_VALUE(ret_val);

	sd_memcpy(pdns->_host_name, host, pdns->_host_name_buffer_len);
	pdns->_host_name[pdns->_host_name_buffer_len] = 0;

	return ret_val;
}

_int32 op_dns_dealloc(void *dest)
{
	_int32 ret_val = SUCCESS;
	OP_PARA_DNS *pdns = (OP_PARA_DNS*)dest;

	ret_val = sd_free(pdns->_host_name);
	pdns->_host_name = NULL;
	CHECK_VALUE(ret_val);

	ret_val = para_dns_dealloc(dest);
	CHECK_VALUE(ret_val);

	return ret_val;
}

_int32 op_accept_alloc(void **dest, const void *src)
{
	_int32 ret_val = SUCCESS;

	ret_val = para_accept_alloc(dest);
	CHECK_VALUE(ret_val);

	sd_memset(*dest, 0, sizeof(OP_PARA_ACCEPT));

	return ret_val;
}

_int32 op_accept_dealloc(void *dest)
{
	_int32 ret_val = SUCCESS;

	ret_val = para_accept_dealloc(dest);
	CHECK_VALUE(ret_val);

	return ret_val;
}

_int32 op_csrw_alloc(void **dest, const void *src)
{
	_int32 ret_val = SUCCESS;

	ret_val = para_connsock_rw_alloc(dest);
	CHECK_VALUE(ret_val);

	sd_memcpy(*dest, src, sizeof(OP_PARA_CONN_SOCKET_RW));

	return ret_val;
}

_int32 op_csrw_dealloc(void *dest)
{
	_int32 ret_val = SUCCESS;

	ret_val = para_connsock_rw_dealloc(dest);
	CHECK_VALUE(ret_val);

	return ret_val;
}

_int32 op_ncsrw_alloc(void **dest, const void *src)
{
	_int32 ret_val = SUCCESS;

	ret_val = para_noconnsock_rw_alloc(dest);
	CHECK_VALUE(ret_val);

	sd_memcpy(*dest, src, sizeof(OP_PARA_NOCONN_SOCKET_RW));

	return ret_val;
}

_int32 op_ncsrw_dealloc(void *dest)
{
	_int32 ret_val = SUCCESS;

	ret_val = para_noconnsock_rw_dealloc(dest);
	CHECK_VALUE(ret_val);

	return ret_val;
}

_int32 op_fso_alloc(void **dest, const void *src)
{
	_int32 ret_val = SUCCESS;
	OP_PARA_FS_OPEN *pfso = NULL;
	char *path = NULL;

	ret_val = para_fsopen_alloc(dest);
	CHECK_VALUE(ret_val);

	pfso = (OP_PARA_FS_OPEN*)(*dest);

	sd_memcpy(*dest, src, sizeof(OP_PARA_FS_OPEN));

	path = pfso->_filepath;

	/* alloc new mem to save path */
	ret_val = sd_malloc(pfso->_filepath_buffer_len + 1, (void**)&pfso->_filepath);
	CHECK_VALUE(ret_val);

	sd_memcpy(pfso->_filepath, path, pfso->_filepath_buffer_len);
	pfso->_filepath[pfso->_filepath_buffer_len] = 0;

	return ret_val;
}

_int32 op_fso_dealloc(void *dest)
{
	_int32 ret_val = SUCCESS;
	OP_PARA_FS_OPEN *pfso = (OP_PARA_FS_OPEN*)dest;

	ret_val = sd_free(pfso->_filepath);
	pfso->_filepath = NULL;
	CHECK_VALUE(ret_val);

	ret_val = para_fsopen_dealloc(dest);
	CHECK_VALUE(ret_val);

	return ret_val;
}

_int32 op_fsrw_alloc(void **dest, const void *src)
{
	_int32 ret_val = SUCCESS;

	ret_val = para_fsrw_alloc(dest);
	CHECK_VALUE(ret_val);

	sd_memcpy(*dest, src, sizeof(OP_PARA_FS_RW));

	return ret_val;
}

_int32 op_fsrw_dealloc(void *dest)
{
	_int32 ret_val = SUCCESS;

	ret_val = para_fsrw_dealloc(dest);
	CHECK_VALUE(ret_val);

	return ret_val;
}


_int32 op_calc_hash_alloc(void** dest, const void* src)
{
    _int32 ret_val = SUCCESS;

    //ret_val = para_calc_hash_alloc(dest);
    ret_val = sd_malloc(sizeof(OP_PARA_CACL_HASH), dest);
    CHECK_VALUE(ret_val);

    sd_memcpy(*dest, src, sizeof(OP_PARA_CACL_HASH));

    return ret_val;
}

_int32 op_calc_hash_dealloc(void* dest)
{
    _int32 ret_val = SUCCESS;
    ret_val = sd_free(dest);
    //ret_val = para_calc_hash_dealloc(dest);
    CHECK_VALUE(ret_val);

    return ret_val;
}

_int32 op_sockaddr_alloc(void **dest, const void *src)
{
	_int32 ret_val = SUCCESS;
	if (src == NULL) return ret_val;
	ret_val = para_sockaddr_alloc(dest);
	CHECK_VALUE(ret_val);

	sd_memcpy(*dest, src, sizeof(SD_SOCKADDR));

	return ret_val;
}

_int32 op_sockaddr_dealloc(void *dest)
{
	_int32 ret_val = SUCCESS;

	ret_val = para_sockaddr_dealloc(dest);
	CHECK_VALUE(ret_val);

	return ret_val;
}

_int32 alloc_and_copy_para(MSG_INFO *dest, const MSG_INFO *src)
{
	_int32 ret_val = SUCCESS;
	_u32 fun_index = 0;

	if(src->_operation_type == OP_COMMON)
		return ret_val;

	fun_index = DEVICE_MASK & (src->_operation_type);
	if(fun_index > OP_MAX)
		return INVALID_OPERATION_TYPE;

	ret_val = ((op_alloc)g_fun_table[fun_index - 1][0])(&dest->_operation_parameter, src->_operation_parameter);
	CHECK_VALUE(ret_val);

	return ret_val;
}

_int32 dealloc_parameter(MSG_INFO *info)
{
	_int32 ret_val = SUCCESS;
	_u32 fun_index = 0;

	if(info->_operation_type == OP_COMMON)
		return ret_val;

	fun_index = DEVICE_MASK & (info->_operation_type);
	if(fun_index > OP_MAX)
		return INVALID_OPERATION_TYPE;

	ret_val = ((op_dealloc)g_fun_table[fun_index - 1][1])(info->_operation_parameter);
	CHECK_VALUE(ret_val);

	return ret_val;
}

BOOL is_a_pending_op(MSG *pmsg)
{
	return ((pmsg->_msg_info._operation_type == OP_OPEN) 
	    && (((OP_PARA_FS_OPEN*)(pmsg->_msg_info._operation_parameter))->_file_size > 0));
}
