#ifndef EM_MAP_H_00138F8F2E70_200806121303
#define EM_MAP_H_00138F8F2E70_200806121303

#include "em_common/em_define.h"

#include "utility/map.h"

#define em_set_init set_init
#define em_set_size set_size
#define em_successor successor
#define em_predecessor predecessor
#define em_set_insert_node set_insert_node
#define em_set_erase_node set_erase_node
#define em_set_erase_iterator set_erase_iterator
#define em_set_find_node set_find_node
#define em_set_find_iterator set_find_iterator
#define em_set_clear set_clear

#define EM_SET_BEGIN(set) ((set)._set_nil._left)
#define EM_SET_END(set) (&(set)._set_nil)

#define EM_SET_NEXT(set, cur_it) (em_successor((&set), (cur_it)))

#define EM_SET_DATA(set_it)	((set_it)->_data)


/****************   customed allocator   *******************************/

#define em_set_insert_setnode set_insert_setnode
#define em_set_erase_it_without_free set_erase_it_without_free



/************************************************************************/
/*                       MAP                                            */
/************************************************************************/

#define em_map_init map_init
#define em_map_size map_size
#define em_map_insert_node map_insert_node
#define em_map_erase_node map_erase_node
#define em_map_find_node map_find_node
#define em_map_erase_iterator map_erase_iterator
#define em_map_find_iterator map_find_iterator
#define em_map_clear map_clear

#define EM_MAP_BEGIN(map) (EM_SET_BEGIN((map)._inner_set))
#define EM_MAP_END(map) (EM_SET_END((map)._inner_set))

#define EM_MAP_NEXT(map, cur_it) (EM_SET_NEXT(((map)._inner_set), (cur_it)))

#define EM_MAP_KEY(map_it)		(((PAIR*)(EM_SET_DATA(map_it)))->_key)
#define EM_MAP_VALUE(map_it)	(((PAIR*)(EM_SET_DATA(map_it)))->_value)

#endif
