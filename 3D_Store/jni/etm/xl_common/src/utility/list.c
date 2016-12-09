#include "utility/list.h"
#include "utility/errcode.h"
#include "utility/mempool.h"
#include "utility/string.h"

static SLAB *gp_listslab = NULL;


_int32 list_alloctor_init(void)
{
	_int32 ret_val = SUCCESS;

	if(!gp_listslab)
	{
		ret_val = mpool_create_slab(sizeof(LIST_NODE), MIN_LIST_MEMORY, 0, &gp_listslab);
		CHECK_VALUE(ret_val);
	}

	return ret_val;
}

_int32 list_alloctor_uninit(void)
{
	_int32 ret_val = SUCCESS;

	if(gp_listslab)
	{
		ret_val = mpool_destory_slab(gp_listslab);
		CHECK_VALUE(ret_val);
		gp_listslab = NULL;
	}

	return ret_val;
}

void list_init(LIST *list)
{
	sd_memset(list, 0, sizeof(LIST));

	list->_list_nil._nxt_node = list->_list_nil._pre_node = &list->_list_nil;
}

_u32 list_size(const LIST *list)
{
	if(list)
		return list->_list_size;
	else
		return 0;
}

_int32 list_push(LIST *list, void *data)
{
	return list_insert(list, data, &(list->_list_nil));
}

_int32 list_pop(LIST *list, void **data)
{
	_int32 ret_val = SUCCESS;
	LIST_NODE *pnode = NULL;

	*data = NULL;

	if(list->_list_size > 0)
	{
		pnode = (LIST_NODE*)list->_list_nil._nxt_node;
		*data = (void*)pnode->_data;

		ret_val = list_erase(list, pnode);
		CHECK_VALUE(ret_val);
	}

	return ret_val;
}

_int32 list_insert(LIST *list, void *data, LIST_ITERATOR insert_before)
{
	_int32 ret_val = SUCCESS;
	LIST_NODE *new_node = NULL;

	ret_val = mpool_get_slip(gp_listslab, (void**)&new_node);
	CHECK_VALUE(ret_val);

	sd_memset(new_node, 0, sizeof(LIST_NODE));

	new_node->_data = data;

	new_node->_nxt_node = insert_before;
	new_node->_pre_node = insert_before->_pre_node;
	insert_before->_pre_node = new_node;
	new_node->_pre_node->_nxt_node = new_node;

	list->_list_size ++;

	return ret_val;
}

_int32 list_erase(LIST *list, LIST_ITERATOR erase_node)
{
	if (erase_node == &list->_list_nil)
	{
	    return INVALID_ITERATOR;
	}

	erase_node->_nxt_node->_pre_node = erase_node->_pre_node;
	erase_node->_pre_node->_nxt_node = erase_node->_nxt_node;

	_int32 ret_val = mpool_free_slip(gp_listslab, erase_node);
	CHECK_VALUE(ret_val);

	list->_list_size --;

	return ret_val;
}

_int32 list_clear(LIST *list)
{
	_int32 ret_val = SUCCESS;
	if(list == NULL || list->_list_size == 0)
	{
		return SUCCESS;
	}
	LIST_NODE *node = (LIST_NODE *)list->_list_nil._nxt_node;

	for (; node != &list->_list_nil;)
	{
		node = (LIST_NODE*)node->_nxt_node;
		ret_val = mpool_free_slip(gp_listslab, (LIST_NODE *)node->_pre_node);
		CHECK_VALUE(ret_val);
	}

	list->_list_size = 0;
	list->_list_nil._nxt_node = list->_list_nil._pre_node = &list->_list_nil;
	return ret_val;
}

void list_swap(LIST *list1, LIST *list2)
{
	LIST_NODE tmp_node;
	_int32 tmp_size = 0;

	sd_memcpy(&tmp_node, &list1->_list_nil, sizeof(LIST_NODE));
	sd_memcpy(&list1->_list_nil, &list2->_list_nil, sizeof(LIST_NODE));
	sd_memcpy(&list2->_list_nil, &tmp_node, sizeof(LIST_NODE));

	tmp_size = list1->_list_size;
	list1->_list_size = list2->_list_size;
	list2->_list_size = tmp_size;

	if(list1->_list_size > 0)
	{
		list1->_list_nil._nxt_node->_pre_node = &list1->_list_nil;
		list1->_list_nil._pre_node->_nxt_node = &list1->_list_nil;
	}
	else
	{
		list1->_list_nil._nxt_node = list1->_list_nil._pre_node = &list1->_list_nil;
	}

	if(list2->_list_size > 0)
	{
		list2->_list_nil._nxt_node->_pre_node = &list2->_list_nil;
		list2->_list_nil._pre_node->_nxt_node = &list2->_list_nil;
	}
	else
	{
		list2->_list_nil._nxt_node = list2->_list_nil._pre_node = &list2->_list_nil;
	}
}

void list_cat_and_clear(LIST *list1, LIST *list2)
{
	if (list2->_list_size > 0)
	{
		list1->_list_nil._pre_node->_nxt_node = list2->_list_nil._nxt_node;
		list2->_list_nil._nxt_node->_pre_node = list1->_list_nil._pre_node;

		list1->_list_nil._pre_node = list2->_list_nil._pre_node;
		list2->_list_nil._pre_node->_nxt_node = &list1->_list_nil;

		list1->_list_size += list2->_list_size;

		/* clear list2 */
		list2->_list_nil._nxt_node = list2->_list_nil._pre_node = &list2->_list_nil;
		list2->_list_size = 0;
	}
}
