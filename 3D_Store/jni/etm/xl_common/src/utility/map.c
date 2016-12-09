#include "utility/map.h"
#include "utility/mempool.h"
#include "utility/errcode.h"
#include "utility/string.h"

#include "platform/sd_task.h"

TASK_LOCK  g_global_map_lock ;

static SLAB *gp_mapslab = NULL;

static SLAB *gp_setslab = NULL;

#define SENTINEL(pset)  (&(pset)->_set_nil)

#define ROOT(pset)	((pset)->_set_nil._parent)

#define SET_MIN(pset)((pset)->_set_nil._left)
#define SET_MAX(pset)((pset)->_set_nil._right)

static comparator g_current_user_comparator = NULL;
_int32 map_comparator(void *E1, void *E2)
{
	return g_current_user_comparator(((PAIR*)E1)->_key, ((PAIR*)E2)->_key);
}

SET_NODE* successor(SET *set, SET_NODE *x)
{
	SET_NODE *node = x;

	if(node->_right != SENTINEL(set))
	{ /* MININUM */
		node = node->_right;
		while(node->_left != SENTINEL(set))
			node = node->_left;

		return node;
	}

	node = x->_parent;
	while(node != SENTINEL(set) && x == node->_right)
	{
		x = node;
		node = node->_parent;
	}

	return node;

}

SET_NODE* predecessor(SET *set, SET_NODE *x)
{
	SET_NODE *node = x;

	if(node->_left != SENTINEL(set))
	{ /* MAXINUM */
		node = node->_left;
		while(node->_right != SENTINEL(set))
			node = node->_right;

		return node;
	}

	node = x->_parent;
	while(node != SENTINEL(set) && x == node->_left)
	{
		x = node;
		node = node->_parent;
	}

	return node;
}

static void rotate_left(SET *set, SET_NODE *x)
{
	SET_NODE *root = ROOT(set);
	SET_NODE *y = x->_right;

	x->_right = y->_left;
	y->_left->_parent = x;
	y->_parent = x->_parent;

	if(x->_parent == SENTINEL(set))
		root = y;
	else
	{
		if(x == x->_parent->_left)
			x->_parent->_left = y;
		else
			x->_parent->_right = y;
	}

	y->_left = x;
	x->_parent = y;

	ROOT(set) = root;
}

static void rotate_right(SET *set, SET_NODE *x)
{
	SET_NODE *root = ROOT(set);
	SET_NODE *y = x->_left;

	x->_left = y->_right;
	y->_right->_parent = x;
	y->_parent = x->_parent;

	if(x->_parent == SENTINEL(set))
		root = y;
	else
	{
		if(x == x->_parent->_left)
			x->_parent->_left = y;
		else
			x->_parent->_right = y;
	}

	y->_right = x;
	x->_parent = y;

	ROOT(set) = root;
}

static void insert_fixup(SET *set, SET_NODE *x)
{
	SET_NODE *y = NULL;

	while(x->_parent->_color == RED)
	{
		if(x->_parent == x->_parent->_parent->_left)
		{
			y = x->_parent->_parent->_right;
			if(y->_color == RED)
			{/* uncle is RED */
				x->_parent->_color = BLACK;
				y->_color = BLACK;
				x->_parent->_parent->_color = RED;

				x = x->_parent->_parent;
			}
			else
			{/* uncle is BLACK */
				if(x == x->_parent->_right)
				{
					x = x->_parent;
					rotate_left(set, x);
				}

				x->_parent->_color = BLACK;
				x->_parent->_parent->_color = RED;
				rotate_right(set, x->_parent->_parent);
			}
		}
		else
		{
			y = x->_parent->_parent->_left;
			if(y->_color == RED)
			{/* uncle is RED */
				x->_parent->_color = BLACK;
				y->_color = BLACK;
				x->_parent->_parent->_color = RED;

				x = x->_parent->_parent;
			}
			else
			{/* uncle is BLACK */
				if(x == x->_parent->_left)
				{
					x = x->_parent;
					rotate_right(set, x);
				}

				x->_parent->_color = BLACK;
				x->_parent->_parent->_color = RED;
				rotate_left(set, x->_parent->_parent);
			}
		}
	}

	ROOT(set)->_color = BLACK;
}

static void delete_fixup(SET *set, SET_NODE *x)
{
	SET_NODE *y = NULL;

	while(x != ROOT(set) && x->_color == BLACK)
	{
		if(x == x->_parent->_left)
		{
			y = x->_parent->_right;
			if(y->_color == RED)
			{
				y->_color = BLACK;
				x->_parent->_color = RED;
				rotate_left(set, x->_parent);
				y = x->_parent->_right;
			}
			if(y->_left->_color == BLACK && y->_right->_color == BLACK)
			{
				y->_color = RED;
				x = x->_parent;
			}
			else
			{
				if(y->_right->_color == BLACK)
				{
					y->_left->_color = BLACK;
					y->_color = RED;
					rotate_right(set, y);
					y = x->_parent->_right;
				}

				y->_color = x->_parent->_color;
				x->_parent->_color = BLACK;
				y->_right->_color = BLACK;
				rotate_left(set, x->_parent);

				x = ROOT(set);
			}
		}
		else
		{
			y = x->_parent->_left;
			if(y->_color == RED)
			{
				y->_color = BLACK;
				x->_parent->_color = RED;
				rotate_right(set, x->_parent);
				y = x->_parent->_left;
			}
			if(y->_left->_color == BLACK && y->_right->_color == BLACK)
			{
				y->_color = RED;
				x = x->_parent;
			}
			else
			{
				if(y->_left->_color == BLACK)
				{
					y->_right->_color = BLACK;
					y->_color = RED;
					rotate_left(set, y);
					y = x->_parent->_left;
				}

				y->_color = x->_parent->_color;
				x->_parent->_color = BLACK;
				y->_left->_color = BLACK;
				rotate_right(set, x->_parent);

				x = ROOT(set);
			}
		}
	}

	x->_color = BLACK;
}

/************************************************************************/

_int32 set_alloctor_init(void)
{
	_int32 ret_val = SUCCESS;

	if(!gp_setslab)
	{
		ret_val = mpool_create_slab(sizeof(SET_NODE), MIN_SET_MEMORY, 0, &gp_setslab);
		CHECK_VALUE(ret_val);
	}

	return ret_val;
}

_int32 set_alloctor_uninit(void)
{
	_int32 ret_val = SUCCESS;

	if(gp_setslab)
	{
		ret_val = mpool_destory_slab(gp_setslab);
		CHECK_VALUE(ret_val);
		gp_setslab = NULL;
	}

	return ret_val;
}

void set_init(SET *set, comparator compare_fun)
{
	set->_size = 0;
	set->_comp = compare_fun;

	set->_set_nil._data = NULL;
	set->_set_nil._color = BLACK;
	set->_set_nil._left = set->_set_nil._right = set->_set_nil._parent = &set->_set_nil;
}

_u32 set_size(SET *set)
{
	return set->_size;
}

_int32 set_insert_node(SET *set, void *data)
{
	_int32 ret_val = SUCCESS;

	SET_NODE *cur = ROOT(set), *parent = SENTINEL(set), *new_node = NULL;
	comparator comp = set->_comp;

	while(cur != SENTINEL(set))
	{
		ret_val = comp(data, cur->_data);
		if(ret_val == 0)
			return MAP_DUPLICATE_KEY;

		parent = cur;
		cur = ret_val < 0 ? cur->_left : cur->_right;
	}

	ret_val = mpool_get_slip(gp_setslab, (void**)&new_node);
	CHECK_VALUE(ret_val);

	new_node->_parent = parent;
	new_node->_data = data;
	new_node->_left = new_node->_right = SENTINEL(set);
	new_node->_color = RED;

	if(parent == SENTINEL(set))
	{
		SET_MAX(set) = SET_MIN(set) = ROOT(set) = new_node;
	}
	else
	{
		if(comp(data, SET_DATA(parent)) < 0)
		{
			parent->_left = new_node;
			if(parent == SET_MIN(set))
				SET_MIN(set) = new_node;
		}
		else
		{
			parent->_right = new_node;
			if(parent == SET_MAX(set))
				SET_MAX(set) = new_node;
		}
	}

	insert_fixup(set, new_node);

	set->_size ++;

	return SUCCESS;
}

_int32 set_find_iterator(SET *set, void *find_data, SET_ITERATOR *result_iterator)
{
	_int32 ret_val = SUCCESS;
	SET_NODE *cur = ROOT(set);
	*result_iterator = SENTINEL(set);

	while(set && cur && cur != SENTINEL(set))
	{
		ret_val = set->_comp(find_data, cur->_data);
		if(ret_val == 0)
		{
			*result_iterator = cur;
			break;
		}

		cur = ret_val < 0 ? cur->_left : cur->_right;
	}

	return SUCCESS;
}

_int32 set_find_node(SET *set, void *find_data, void **result_data)
{
	SET_ITERATOR find_it;
	_int32 ret_val = set_find_iterator(set, find_data, &find_it);
	CHECK_VALUE(ret_val);

	if (find_it != SENTINEL(set))
	{
	    *result_data = SET_DATA(find_it);
		ret_val = SUCCESS;
	}
	else
	{
	    *result_data = NULL;
		ret_val = SET_ELEMENT_NOT_FOUND;
	}

	return ret_val;
}

_int32 set_find_iterator_by_custom_compare_function(comparator compare_fun, SET *set, void *find_data, SET_ITERATOR *result_iterator)
{
	_int32 ret_val = SUCCESS;
	SET_NODE *cur = ROOT(set);
	*result_iterator = SENTINEL(set);

	while(cur != SENTINEL(set))
	{
		ret_val = compare_fun(find_data, cur->_data);
		if(ret_val == 0)
		{
			*result_iterator = cur;
			break;
		}

		cur = ret_val < 0 ? cur->_left : cur->_right;
	}

	return SUCCESS;
}

_int32 set_find_node_by_custom_compare_function(comparator compare_fun, SET *set, void *find_data, void **find_result)
{
	_int32 ret_val = SUCCESS;
	SET_ITERATOR find_it;

	ret_val = set_find_iterator_by_custom_compare_function(compare_fun, set, find_data, &find_it);
	CHECK_VALUE(ret_val);

	if(find_it != SENTINEL(set))
		*find_result = SET_DATA(find_it);
	else
		*find_result = NULL;

	return ret_val;
}

_int32 set_erase_node(SET *set, void *data)
{
	_int32 ret_val = SUCCESS;
	SET_ITERATOR it = SENTINEL(set);

	ret_val = set_find_iterator(set, data, &it);
	CHECK_VALUE(ret_val);

	if (it == SENTINEL(set))
	{
	    return MAP_KEY_NOT_FOUND;
	}
	else
	{
		return set_erase_iterator(set, it);
	}
}

_int32 set_erase_iterator(SET *set, SET_ITERATOR it)
{
	_int32 ret_val = SUCCESS;

	if(it == SENTINEL(set))
		return INVALID_ITERATOR;

	set_erase_it_without_free(set, it);

	ret_val = mpool_free_slip(gp_setslab, it);
	CHECK_VALUE(ret_val);

	return SUCCESS;
}

/******************  customed allocator  *********************************/

void set_erase_it_without_free(SET *set, SET_ITERATOR it)
{
	SET_NODE *x = it, *y = it;
	SET_NODE tmp_node;

	if(it->_left != SENTINEL(set) && it->_right != SENTINEL(set))
	{
		y = successor(set, it);

		/* swap content of it && y, for case of keep the iterator be not deleted */
		sd_memcpy(&tmp_node, it, sizeof(SET_NODE));
		sd_memcpy(it, y, sizeof(SET_NODE));
		sd_memcpy(y, &tmp_node, sizeof(SET_NODE));

		x = it;	it = y;	y = x;

		/* change pointer related */
		it->_left->_parent = it;
		if(it->_right == it) /* it adjacent to y */
			it->_right = y;
		it->_right->_parent = it;
		if(it->_parent == SENTINEL(set))
			ROOT(set) = it;
		else
		{
			if(it->_parent->_left == y)
				it->_parent->_left = it;
			else
				it->_parent->_right = it;
		}

		/* y have no left-child and y can not be root */
		if(y->_right != SENTINEL(set))
			y->_right->_parent = y;
		if(y->_parent != it) /* the case of (y->_parent == it) has been completed */
		{
			if(y->_parent->_left == it)
				y->_parent->_left = y;
			else
				y->_parent->_right = y;
		}

		/* check root and max */
		if(ROOT(set) == y)
			ROOT(set) = it;

		/* if(SET_MAX(set) == it) then SET_MAX(set) = y. but y will be deleted later, so SET_MAX(set) = it exactly */
	}
	else
	{
		/* min && max node */
		if(SET_MIN(set) == it)
			SET_MIN(set) = successor(set, it);
		if(SET_MAX(set) == it)
			SET_MAX(set) = predecessor(set, it);
	}

	if(y->_left != SENTINEL(set))
		x = y->_left;
	else
		x = y->_right;

	/* duplicate x to protect SENTINEL,
	 we can do it becase call delete_fixup() only concern about x->parent */
	if(x == SENTINEL(set))
	{
		sd_memcpy(&tmp_node, x, sizeof(SET_NODE));
		x = &tmp_node;
	}

	x->_parent = y->_parent;
	if(y->_parent == SENTINEL(set))
	{
		ROOT(set) = x;
	}
	else
	{
		if(y == y->_parent->_left)
			y->_parent->_left = x;
		else
			y->_parent->_right = x;
	}

	if(y != it)
	{
		SET_DATA(it) = y->_data;
	}

	if(y->_color == BLACK)
	{
		/* this call only concern about x->parent */
		delete_fixup(set, x);
	}

	/* recovery SENTINEL */
	if(&tmp_node == x)
	{
		if(x->_parent == SENTINEL(set))
			ROOT(set) = SENTINEL(set);
		else
		{
			if(x == x->_parent->_left)
				x->_parent->_left = SENTINEL(set);
			else
				x->_parent->_right = SENTINEL(set);
		}
	}

/*
	ret_val = mpool_free_slip(gp_setslab, y);
	CHECK_VALUE(ret_val);
*/

	set->_size --;
}

_int32 set_insert_setnode(SET *set, SET_NODE *new_node)
{
	_int32 ret_val = SUCCESS;
	SET_NODE *cur = ROOT(set), *parent = SENTINEL(set);
	comparator comp = set->_comp;
	void *data = new_node->_data;

	while(cur != SENTINEL(set))
	{
		ret_val = comp(data, cur->_data);
		if(ret_val == 0)
			return MAP_DUPLICATE_KEY;

		parent = cur;
		cur = ret_val < 0 ? cur->_left : cur->_right;
	}

	new_node->_parent = parent;
	new_node->_left = new_node->_right = SENTINEL(set);
	new_node->_color = RED;

	if(parent == SENTINEL(set))
	{
		SET_MAX(set) = SET_MIN(set) = ROOT(set) = new_node;
	}
	else
	{
		if(comp(data, SET_DATA(parent)) < 0)
		{
			parent->_left = new_node;
			if(parent == SET_MIN(set))
				SET_MIN(set) = new_node;
		}
		else
		{
			parent->_right = new_node;
			if(parent == SET_MAX(set))
				SET_MAX(set) = new_node;
		}
	}

	insert_fixup(set, new_node);

	set->_size ++;

	return SUCCESS;
}

/************************ end of customed allocator  **************************/


static _int32 erase_node(SET *set, SET_NODE *node)
{
	_int32 ret_val = SUCCESS;
	SET_NODE *right, *left;

	if(node != SENTINEL(set))
	{
		left = node->_left;
		right = node->_right;

		ret_val = mpool_free_slip(gp_setslab, node);
		CHECK_VALUE(ret_val);

		ret_val = erase_node(set, left);
		CHECK_VALUE(ret_val);

		ret_val = erase_node(set, right);
		CHECK_VALUE(ret_val);
	}

	return ret_val;
}

_int32 set_clear(SET *set)
{
	_int32 ret_val = SUCCESS;

    if(set->_size == 0)
    {
        return ret_val;
    }

	ret_val = erase_node(set, ROOT(set));
	CHECK_VALUE(ret_val);

	set->_size = 0;
	set->_set_nil._left = set->_set_nil._right = set->_set_nil._parent = &set->_set_nil;

	return ret_val;
}

/* only used for map clear */
static _int32 erase_node_and_data(SET *set, SET_NODE *node)
{
	_int32 ret_val = SUCCESS;
	SET_NODE *right, *left;

	if(node != SENTINEL(set))
	{
		left = node->_left;
		right = node->_right;

		ret_val = mpool_free_slip(gp_mapslab, node->_data);
		CHECK_VALUE(ret_val);

		ret_val = mpool_free_slip(gp_setslab, node);
		CHECK_VALUE(ret_val);

		ret_val = erase_node_and_data(set, left);
		CHECK_VALUE(ret_val);

		ret_val = erase_node_and_data(set, right);
		CHECK_VALUE(ret_val);
	}

	return ret_val;
}


/************************************************************************/
/************************************************************************/

_int32 map_clear(MAP *map)
{
	_int32 ret_val = SUCCESS;

	ret_val = erase_node_and_data(&map->_inner_set, ROOT(&map->_inner_set));
	CHECK_VALUE(ret_val);

	map->_inner_set._size = 0;
	map->_inner_set._set_nil._left = map->_inner_set._set_nil._right = map->_inner_set._set_nil._parent = &map->_inner_set._set_nil;

	return ret_val;
}


_int32 map_alloctor_init(void)
{
	_int32 ret_val = SUCCESS;

	if(!gp_mapslab)
	{
		ret_val = mpool_create_slab(sizeof(PAIR), MIN_MAP_MEMORY, 0, &gp_mapslab);
		CHECK_VALUE(ret_val);

       ret_val = sd_init_task_lock(&g_global_map_lock);	   
	CHECK_VALUE(ret_val);

	}

	return ret_val;
}

_int32 map_alloctor_uninit(void)
{
	_int32 ret_val = SUCCESS;

	if(gp_mapslab)
	{

       ret_val = sd_uninit_task_lock(&g_global_map_lock);	   
	CHECK_VALUE(ret_val);
		ret_val = mpool_destory_slab(gp_mapslab);
		CHECK_VALUE(ret_val);
		gp_mapslab = NULL;
	}

	return ret_val;
}

void map_init(MAP *map, comparator compare_fun)
{
	set_init(&map->_inner_set, map_comparator);
	map->_user_comp = compare_fun;
}

_u32 map_size(MAP *map)
{
	return map->_inner_set._size;
}

_int32 map_insert_node(MAP *map, const PAIR *node)
{
	_int32 ret_val = SUCCESS;
	PAIR *new_node = NULL;

	ret_val = mpool_get_slip(gp_mapslab, (void**)&new_node);
	CHECK_VALUE(ret_val);

	new_node->_key = node->_key;
	new_node->_value = node->_value;

     ret_val = sd_task_lock ( &g_global_map_lock );
     CHECK_VALUE(ret_val);

	g_current_user_comparator = map->_user_comp;
	ret_val = set_insert_node(&map->_inner_set, new_node);

       sd_task_unlock ( &g_global_map_lock );
	if(ret_val != SUCCESS)
	{
		mpool_free_slip(gp_mapslab, new_node);
		if(MAP_DUPLICATE_KEY!=ret_val)
		{
			CHECK_VALUE(ret_val);
		}
	}

	return ret_val;
}

_int32 map_erase_node(MAP *map, void *key)
{
	_int32 ret_val = SUCCESS;
	PAIR del_node;
	SET *inner_set = &map->_inner_set;
	SET_ITERATOR find_it = NULL;
	void *tmp_data = NULL;

	find_it = SET_END(map->_inner_set);

	del_node._key = key;

     ret_val = sd_task_lock ( &g_global_map_lock );
     CHECK_VALUE(ret_val);

	g_current_user_comparator = map->_user_comp;
	ret_val = set_find_iterator(inner_set, &del_node, &find_it);
	
       sd_task_unlock ( &g_global_map_lock );
	
	CHECK_VALUE(ret_val);

	if(find_it != SET_END(map->_inner_set))
	{
		tmp_data = SET_DATA(find_it);
		ret_val = set_erase_iterator(inner_set, find_it);
		CHECK_VALUE(ret_val);

		ret_val = mpool_free_slip(gp_mapslab, tmp_data);
		CHECK_VALUE(ret_val);
	}
	else
		return MAP_KEY_NOT_FOUND;

	return ret_val;
}

_int32 map_find_node(MAP *map, void *key, void **value)
{
	_int32 ret_val = SUCCESS;
	PAIR find_pair;
	PAIR *result = NULL;

	find_pair._key = key;

	*value = NULL;

	ret_val = sd_task_lock(&g_global_map_lock);
	CHECK_VALUE(ret_val);

	g_current_user_comparator = map->_user_comp;
	ret_val = set_find_node(&map->_inner_set, &find_pair, (void**)&result);

	sd_task_unlock (&g_global_map_lock);

	if (NULL != result)
	{
	    *value = result->_value;
	}
	ret_val = SUCCESS;
	return ret_val;
}

_int32 map_erase_iterator(MAP *map, MAP_ITERATOR iterator)
{
	_int32 ret_val = SUCCESS;
	void *data = SET_DATA(iterator);

	ret_val = set_erase_iterator(&map->_inner_set, iterator);
	CHECK_VALUE(ret_val);

	ret_val = mpool_free_slip(gp_mapslab, data);
	CHECK_VALUE(ret_val);

	return ret_val;
}

_int32 map_find_iterator(MAP *map, void *key, MAP_ITERATOR *result_iterator)
{
	PAIR find_pair;
	_int32 ret_val = SUCCESS;

	find_pair._key = key;
     ret_val = sd_task_lock ( &g_global_map_lock );
     CHECK_VALUE(ret_val);

	g_current_user_comparator = map->_user_comp;
	ret_val =  set_find_iterator(&map->_inner_set, &find_pair, result_iterator);
	
       sd_task_unlock ( &g_global_map_lock );

       return ret_val;
	
}

_int32 map_find_node_by_custom_compare_function(comparator compare_fun, MAP *map, void *find_data, void **value)
{
	_int32 ret_val = SUCCESS;
	PAIR *result;

     ret_val = sd_task_lock ( &g_global_map_lock );
     CHECK_VALUE(ret_val);

	ret_val = set_find_node_by_custom_compare_function(compare_fun, &map->_inner_set, find_data, (void**)&result);

       sd_task_unlock ( &g_global_map_lock );
	CHECK_VALUE(ret_val);

	*value = NULL;
	if(result)
		*value = result->_value;

	return ret_val;
}


_int32 map_find_iterator_by_custom_compare_function(comparator compare_fun, MAP *map,  void *find_data, MAP_ITERATOR *result_iterator)
{
	_int32 ret_val = SUCCESS;

       ret_val = sd_task_lock ( &g_global_map_lock );
       CHECK_VALUE(ret_val);
	
	ret_val =  set_find_iterator_by_custom_compare_function(compare_fun, &map->_inner_set, find_data, result_iterator);
	
       sd_task_unlock ( &g_global_map_lock );

       return ret_val;
}

