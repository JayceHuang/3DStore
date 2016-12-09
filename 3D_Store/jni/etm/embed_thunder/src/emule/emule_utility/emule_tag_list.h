#ifndef _EMULE_TAG_LIST_H_
#define _EMULE_TAG_LIST_H_

#ifdef ENABLE_EMULE

#include "utility/define.h"
#include "utility/map.h"

#include "emule/emule_utility/emule_tag.h"

typedef enum tagEMULE_COUNT_TYPE
{
    TAG_COUNT_U8,
    TAG_COUNT_U32
} EMULE_COUNT_TYPE;

typedef struct tagEMULE_TAG_LIST
{
    EMULE_COUNT_TYPE _count_type;
    SET _emule_tag_set;
}EMULE_TAG_LIST;

void emule_tag_list_init(EMULE_TAG_LIST* tag_list);
void emule_tag_list_set_count_type( EMULE_TAG_LIST* tag_list, EMULE_COUNT_TYPE count_type );

void emule_tag_list_uninit(EMULE_TAG_LIST* tag_list, BOOL free_tag);

_int32 emule_tag_list_add(EMULE_TAG_LIST* tag_list, EMULE_TAG* tag);

const EMULE_TAG* emule_tag_list_get(EMULE_TAG_LIST* tag_list, char* tag_name);

_u32 emule_tag_list_size(EMULE_TAG_LIST* tag_list);

_int32 emule_tag_list_to_buffer(EMULE_TAG_LIST* tag_list, char** buffer, _int32* len);

_int32 emule_tag_list_from_buffer(EMULE_TAG_LIST* tag_list, char** buffer, _int32* len);
#endif /* ENABLE_EMULE */

#endif

