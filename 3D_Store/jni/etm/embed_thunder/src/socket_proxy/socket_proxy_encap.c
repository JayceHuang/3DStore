#include "utility/map.h"
#include "utility/mempool.h"

#include "socket_proxy_encap.h"
/*
 * 用MAP保存封装属性。
 */

static MAP g_socket_encap_map;

static _int32 sock_comparator(void *E1, void *E2)
{
	if ((_u32)E1 > (_u32)E2)
	{
		return 1;
	}
	else if ((_u32)E1 == (_u32)E2)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

_int32 init_socket_encap()
{
	map_init(&g_socket_encap_map, sock_comparator);
	return SUCCESS;
}

_int32 uninit_socket_encap()
{
	//_int32 ret = 0;
	MAP_ITERATOR iter;
	
	iter = MAP_BEGIN(g_socket_encap_map);
	while (iter != MAP_END(g_socket_encap_map))
	{
		if (iter->_data != NULL)
		{
			sd_free(iter->_data);
			iter->_data = NULL;
		}
		map_erase_iterator(&g_socket_encap_map, iter);
		iter = MAP_BEGIN(g_socket_encap_map);
	}

	return SUCCESS;
}

_int32 insert_socket_encap_prop(_u32 sock_num, SOCKET_ENCAP_PROP_S* p_encap_prop)
{
	_int32 ret = 0;
	PAIR pair;

	pair._key = (void *)sock_num;
	pair._value = (void *)p_encap_prop;
	ret = map_insert_node(&g_socket_encap_map, &pair);
	CHECK_VALUE(ret);

	return SUCCESS;
}

_int32 remove_socket_encap_prop(_u32 sock_num)
{
	_int32 ret = 0;
	MAP_ITERATOR iter;
	SOCKET_ENCAP_PROP_S* p_encap_prop = NULL;

	ret = map_find_iterator(&g_socket_encap_map, (void *)sock_num, &iter);
	CHECK_VALUE(ret);

	if (iter == MAP_END(g_socket_encap_map))
	{
		// 没找到。
	}
	else
	{
		p_encap_prop = (SOCKET_ENCAP_PROP_S*)MAP_VALUE(iter);
		if (p_encap_prop != NULL)
		{
			sd_free(p_encap_prop);
			p_encap_prop = NULL;
		}

		ret = map_erase_iterator(&g_socket_encap_map, iter);
		CHECK_VALUE(ret);
	}

	return SUCCESS;
}

_int32 get_socket_encap_prop_by_sock(_u32 sock_num, SOCKET_ENCAP_PROP_S** pp_encap_prop)
{
	_int32 ret = 0;
	MAP_ITERATOR iter;

	ret = map_find_iterator(&g_socket_encap_map, (void *)sock_num, &iter);
	CHECK_VALUE(ret);
	
	if (iter == MAP_END(g_socket_encap_map))
	{
		// 没找到。
		*pp_encap_prop = NULL;
	}
	else
	{
		*pp_encap_prop = (SOCKET_ENCAP_PROP_S*)MAP_VALUE(iter);
	}

	return SUCCESS;
}
