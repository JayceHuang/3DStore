#ifndef __XML_SERVICE_H__
#define __XML_SERVICE_H__

#include "utility/define.h"

#define T XML_SERVICE

#ifdef __cplusplus
extern "C" 
{
#define _T XML_SERVICE_T
struct _T;
typedef struct _T *T;
#else
typedef struct T *T;
#endif

typedef _int32 (*xml_node_proc)(void *user_data, void *parent_node, _int32 parent_type, _int32 depth, char const *node_name, void **node, _int32 *node_type);
typedef _int32 (*xml_node_ex_proc)(void *user_data, void *parent_node, _int32 parent_type, _int32 depth, char const *node_name, char const **attr, _int32 attr_count, void **node, _int32 *node_type);
typedef _int32 (*xml_node_end_proc)(void *user_data, void *node, _int32 node_type, _int32 depth, char const *node_name);
typedef _int32 (*xml_attr_proc)(void *user_data, void *node, _int32 node_type, _int32 depth, char const *attr_name, char const *attr_value);
typedef _int32 (*xml_value_proc)(void *user_data, void *node, _int32 node_type, _int32 depth, char const *value);

T create_xml_service();
_int32 destroy_xml_service(T xml_service);
_int32 read_xml_file(T xml_service, char const *file_name, _int32 name_length, void *user_data, xml_node_proc node_proc, xml_node_end_proc node_end_proc, xml_attr_proc attr_proc, xml_value_proc value_proc);
_int32 read_xml_buffer(T xml_service, char const *buffer, _int32 buffer_length, void *user_data, xml_node_proc node_proc, xml_node_end_proc node_end_proc, xml_attr_proc attr_proc, xml_value_proc value_proc);
_int32 read_xml_file_ex(T xml_service, char const *file_name, _int32 name_length, void *user_data, xml_node_ex_proc node_proc, xml_node_end_proc node_end_proc, xml_value_proc value_proc);
_int32 read_xml_buffer_ex(T xml_service, char const *buffer, _int32 buffer_length, void *user_data, xml_node_ex_proc node_proc, xml_node_end_proc node_end_proc, xml_value_proc value_proc);
_int32 begin_write_xml_file(T xml_service, char const *file_name, _int32 name_length);
_int32 end_write_xml_file(T xml_service);
_int32 begin_write_xml_node(T xml_service, char const *node_name);
_int32 end_write_xml_node(T xml_service, char const *value, _int32 is_cdata);
_int32 add_xml_attr(T xml_service, char const *attr_name, char const *attr_value);

#ifdef __cplusplus
#undef _T
}
#endif

#undef T

#endif //__XML_SERVICE_H__
