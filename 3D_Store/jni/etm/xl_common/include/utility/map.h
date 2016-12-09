#ifndef SD_MAP_H_00138F8F2E70_200806121303
#define SD_MAP_H_00138F8F2E70_200806121303

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"

/*
 * Return: 
 *    >0: E1 > E2	   ==0: E1 == E2		<0: E1 < E2
 */
typedef _int32 (*comparator)(void *E1, void *E2);


/************************************************************************/
/*                       SET                                            */
/************************************************************************/

typedef enum {BLACK, RED} NODE_COLOR;

typedef struct t_set_node{
	void *_data;
	struct t_set_node *_left;
	struct t_set_node *_parent;
	struct t_set_node *_right;
	NODE_COLOR _color;
} SET_NODE, *pSET_NODE;

typedef struct {
	_int32 _size;
	comparator _comp;
	SET_NODE _set_nil;
} SET;

typedef pSET_NODE SET_ITERATOR;

_int32 set_alloctor_init(void);
_int32 set_alloctor_uninit(void);

/* @Simple Function@
 * Return : void
 */
void set_init(SET *set, comparator compare_fun);

/* @Simple Function@
 * Return : the size of set
 */
_u32 set_size(SET *set);

/* @Simple Function@
 * Return : the successor node of x
 */
SET_NODE* successor(SET *set, SET_NODE *x);

/* @Simple Function@
 * Return : the predecessor node of x
 */
SET_NODE* predecessor(SET *set, SET_NODE *x);


_int32 set_insert_node(SET *set, void *data);

_int32 set_erase_node(SET *set, void *data);
_int32 set_erase_iterator(SET *set, SET_ITERATOR iterator);

/* if not found, the *result_data == NULL */
_int32 set_find_node(SET *set, void *find_data, void **result_data);
/* if not found, the result_iterator == SET_END(set) */
_int32 set_find_iterator(SET *set, void *find_data, SET_ITERATOR *result_iterator);

_int32 set_find_node_by_custom_compare_function(comparator compare_fun, SET *set, void *find_data, void **find_result);
_int32 set_find_iterator_by_custom_compare_function(comparator compare_fun, SET *set, void *find_data, SET_ITERATOR *result_iterator);
	
/* clear all node of set, the caller is responsable for destruction of elements */
_int32 set_clear(SET *set);


#define SET_BEGIN(set) ((set)._set_nil._left)
#define SET_END(set) (&(set)._set_nil)

#define SET_NEXT(set, cur_it) (successor((&set), (cur_it)))

#define SET_DATA(set_it)	((set_it)->_data)


/****************   customed allocator   *******************************/

_int32 set_insert_setnode(SET *set, SET_NODE *pnode);
void set_erase_it_without_free(SET *set, SET_ITERATOR it);



/************************************************************************/
/*                       MAP                                            */
/************************************************************************/

_int32 map_alloctor_init(void);
_int32 map_alloctor_uninit(void);

typedef struct {
	void *_key;
	void *_value;
} PAIR;

typedef struct {
	SET _inner_set;
	comparator _user_comp;
} MAP;

typedef SET_ITERATOR MAP_ITERATOR;

void map_init(MAP *map, comparator compare_fun);


/* @Simple Function@
 * Return : the size of map
 */
_u32 map_size(MAP *map);

_int32 map_insert_node(MAP *map, const PAIR *node);
_int32 map_erase_node(MAP *map, void *key);
_int32 map_find_node(MAP *map, void *key, void **value);

_int32 map_erase_iterator(MAP *map, MAP_ITERATOR iterator);
_int32 map_find_iterator(MAP *map, void *key, MAP_ITERATOR *result_iterator);

_int32 map_find_node_by_custom_compare_function(comparator compare_fun, MAP *map, void *find_data, void **value);
_int32 map_find_iterator_by_custom_compare_function(comparator compare_fun, MAP *map, void *find_data, MAP_ITERATOR *result_iterator);

/* clear all node of map, the caller is responsable for destruction of elements */
_int32 map_clear(MAP *map);


#define MAP_BEGIN(map) (SET_BEGIN((map)._inner_set))
#define MAP_END(map) (SET_END((map)._inner_set))

#define MAP_NEXT(map, cur_it) (SET_NEXT(((map)._inner_set), (cur_it)))

#define MAP_KEY(map_it)		(((PAIR*)(SET_DATA(map_it)))->_key)
#define MAP_VALUE(map_it)	(((PAIR*)(SET_DATA(map_it)))->_value)

#ifdef __cplusplus
}
#endif
#endif
