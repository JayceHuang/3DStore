#include "xml_service.h"
#include "expat.h"

#include "utility/list.h"
#include "utility/errcode.h"
#include "utility/sd_assert.h"
#include "utility/mempool.h"
#include "utility/string.h"

#include "utility/logid.h"
#ifdef LOGID
#undef LOGID
#endif

#define LOGID LOGID_XML

#include "utility/logger.h"
//#include "eui/eui_module.h"
#include "platform/sd_fs.h"

#define T XML_SERVICE

#define MAX_VALUE_LENGTH 256
#define BUFFER_SIZE 1024

//typedef struct XML_NODE
//{
//	void *node_data;
//	_int32 node_type;
//};

#ifdef __cplusplus
#define _T XML_SERVICE_T
struct _T
#else
struct T
#endif
{
	_int32 depth;
	char value_buffer[MAX_VALUE_LENGTH];
	char *extend_buffer;
	_int32 value_length;
	_int32 buffer_length;
	_int32 state;
	_int32 cdata_value;
	_int32 need_space;
	LIST node_list;
	LIST type_list;
	void *user_data;
	xml_node_proc xml_node_processer;
	xml_node_end_proc xml_node_end_processer;
	xml_node_ex_proc xml_node_ex_processer;
	xml_attr_proc xml_attr_processer;
	xml_value_proc xml_value_processer;
};

#define XS_NONE		0
#define XS_ELEMENT	1
#define XS_VALUE	2
#define XS_CDATA	3

static void XMLCALL xml_element_start(void *data, const char *element, const char **attr);
static void XMLCALL xml_element_end(void *data, const char *el);
static void XMLCALL xml_character_data(void *data, char const *str, int len);
static void XMLCALL xml_cdata_start(void *data);
static void XMLCALL xml_cdata_end(void *data);

T create_xml_service()
{
	T xml_service = NULL;
	_int32 ret =  sd_malloc(sizeof(*xml_service), (void **)&xml_service);
	if (SUCCESS == ret)
	{
		 sd_memset(xml_service, 0, sizeof(*xml_service));
		 list_init(&xml_service->node_list);
		 list_init(&xml_service->type_list);
		return xml_service;
	}
	return NULL;
}

_int32 destroy_xml_service(T xml_service)
{
	sd_assert(xml_service);
	 list_clear(&xml_service->node_list);
	 list_clear(&xml_service->type_list);
	if (xml_service->extend_buffer)
	{
		 sd_free(xml_service->extend_buffer);
	}
	 sd_free(xml_service);
	return 0;
}

_int32 read_xml_file(T xml_service, char const *file_name, _int32 name_length, void *user_data, xml_node_proc node_proc, xml_node_end_proc node_end_proc, xml_attr_proc attr_proc, xml_value_proc value_proc)
{
	XML_Parser parser = NULL;
	_int32 ret_val = 0;
	_u32 file = 0;
	_u32 read_size = 0;
	_int32 done = 0;
	char buffer[BUFFER_SIZE] = {0};
	sd_assert(xml_service);
	parser = XML_ParserCreate(NULL);
	if (!parser)
	{
		LOG_ERROR("xml parser create failed.");
		return 1;
	}
	
	xml_service->depth = -1;
	xml_service->state = XS_NONE;
	xml_service->cdata_value = 0;
	 list_clear(&xml_service->node_list);
	 list_clear(&xml_service->type_list);
	
	xml_service->user_data = user_data;
	xml_service->xml_node_processer = node_proc;
	xml_service->xml_node_end_processer = node_end_proc;
	xml_service->xml_attr_processer = attr_proc;
	xml_service->xml_value_processer = value_proc;
	
	XML_SetUserData(parser, xml_service);
	XML_SetElementHandler(parser, xml_element_start, xml_element_end);
	XML_SetCharacterDataHandler(parser, xml_character_data);
	XML_SetCdataSectionHandler(parser, xml_cdata_start, xml_cdata_end);
    ret_val =  sd_open_ex((char *)file_name, O_FS_RDONLY, &file);
	for (;;)
	{
		ret_val =  sd_read(file, buffer, BUFFER_SIZE, &read_size);
		if (SUCCESS != ret_val)
		{
			break;
		}
		done = (read_size != BUFFER_SIZE);

		if (XML_Parse(parser, buffer, read_size, done) == XML_STATUS_ERROR)
		{
			LOG_ERROR("Parse error at line %u, reason: %s",
					XML_GetCurrentLineNumber(parser),
					XML_ErrorString(XML_GetErrorCode(parser)));
            ret_val = XML_GetErrorCode(parser);
			break;
		}

		if (done)
			break;
		 sd_memset(buffer, 0, BUFFER_SIZE);
	}
	 sd_close_ex(file);
	XML_ParserFree(parser);
	return ret_val;
}

_int32 read_xml_buffer(T xml_service, char const *buffer, _int32 buffer_length, void *user_data, xml_node_proc node_proc, xml_node_end_proc node_end_proc, xml_attr_proc attr_proc, xml_value_proc value_proc)
{
	XML_Parser parser = NULL;
	_int32 ret_val = 0;
	sd_assert(xml_service);
	parser = XML_ParserCreate(NULL);
	if (!parser)
	{
		LOG_ERROR("xml parser create failed.");
		return 1;
	}
	
	xml_service->depth = -1;
	xml_service->state = XS_NONE;
	xml_service->cdata_value = 0;
	 list_clear(&xml_service->node_list);
	 list_clear(&xml_service->type_list);
	
	xml_service->user_data = user_data;
	xml_service->xml_node_processer = node_proc;
	xml_service->xml_node_end_processer = node_end_proc;
	xml_service->xml_attr_processer = attr_proc;
	xml_service->xml_value_processer = value_proc;
	
	XML_SetUserData(parser, xml_service);
	XML_SetElementHandler(parser, xml_element_start, xml_element_end);
	XML_SetCharacterDataHandler(parser, xml_character_data);
	XML_SetCdataSectionHandler(parser, xml_cdata_start, xml_cdata_end);
	if (XML_Parse(parser, buffer, buffer_length, 1) == XML_STATUS_ERROR)
	{
		LOG_ERROR("Parse error at line %u, reason: %s",
			XML_GetCurrentLineNumber(parser),
			XML_ErrorString(XML_GetErrorCode(parser)));
	}

	XML_ParserFree(parser);
	return 0;
}

_int32 read_xml_file_ex(T xml_service, char const *file_name, _int32 name_length, void *user_data, xml_node_ex_proc node_proc, xml_node_end_proc node_end_proc, xml_value_proc value_proc)
{
	XML_Parser parser = NULL;
	_int32 ret_val = 0;
	_u32 file = 0;
	_u32 read_size = 0;
	_int32 done = 0;
	char buffer[BUFFER_SIZE] = {0};
	sd_assert(xml_service);
	parser = XML_ParserCreate(NULL);
	if (!parser)
	{
		LOG_ERROR("xml parser create failed.");
		return 1;
	}
	
	xml_service->depth = -1;
	xml_service->state = XS_NONE;
	xml_service->cdata_value = 0;
	 list_clear(&xml_service->node_list);
	 list_clear(&xml_service->type_list);
	
	xml_service->user_data = user_data;
	xml_service->xml_node_ex_processer = node_proc;
	xml_service->xml_node_end_processer = node_end_proc;
	xml_service->xml_value_processer = value_proc;
	
	XML_SetUserData(parser, xml_service);
	XML_SetElementHandler(parser, xml_element_start, xml_element_end);
	XML_SetCharacterDataHandler(parser, xml_character_data);
	XML_SetCdataSectionHandler(parser, xml_cdata_start, xml_cdata_end);
    ret_val =  sd_open_ex((char *)file_name, O_FS_RDONLY, &file);
	for (;;)
	{
		ret_val =  sd_read(file, buffer, BUFFER_SIZE, &read_size);
		if (SUCCESS != ret_val)
		{
			break;
		}
		done = (read_size != BUFFER_SIZE);

		if (XML_Parse(parser, buffer, read_size, done) == XML_STATUS_ERROR)
		{
			LOG_ERROR("Parse error at line %u, reason: %s",
					XML_GetCurrentLineNumber(parser),
					XML_ErrorString(XML_GetErrorCode(parser)));
			break;
		}

		if (done)
			break;
		 sd_memset(buffer, 0, BUFFER_SIZE);
	}
	 sd_close_ex(file);
	XML_ParserFree(parser);
	return 0;
}

_int32 read_xml_buffer_ex(T xml_service, char const *buffer, _int32 buffer_length, void *user_data, xml_node_ex_proc node_proc, xml_node_end_proc node_end_proc, xml_value_proc value_proc)
{
	XML_Parser parser = NULL;
	_int32 ret_val = 0;
	sd_assert(xml_service);
	parser = XML_ParserCreate(NULL);
	if (!parser)
	{
		LOG_ERROR("xml parser create failed.");
		return 1;
	}
	
	xml_service->depth = -1;
	xml_service->state = XS_NONE;
	xml_service->cdata_value = 0;
	 list_clear(&xml_service->node_list);
	 list_clear(&xml_service->type_list);
	
	xml_service->user_data = user_data;
	xml_service->xml_node_ex_processer = node_proc;
	xml_service->xml_node_end_processer = node_end_proc;
	xml_service->xml_value_processer = value_proc;
	
	XML_SetUserData(parser, xml_service);
	XML_SetElementHandler(parser, xml_element_start, xml_element_end);
	XML_SetCharacterDataHandler(parser, xml_character_data);
	XML_SetCdataSectionHandler(parser, xml_cdata_start, xml_cdata_end);
	if (XML_Parse(parser, buffer, buffer_length, 1) == XML_STATUS_ERROR)
	{
		LOG_ERROR("Parse error at line %u, reason: %s",
			XML_GetCurrentLineNumber(parser),
			XML_ErrorString(XML_GetErrorCode(parser)));
	}

	XML_ParserFree(parser);
	return 0;
}

_int32 begin_write_xml_file(T xml_service, char const *file_name, _int32 name_length)
{
	return 0;
}

_int32 end_write_xml_file(T xml_service)
{
	return 0;
}

_int32 begin_write_xml_node(T xml_service, char const *node_name)
{
	return 0;
}

_int32 end_write_xml_node(T xml_service, char const *value, _int32 is_cdata)
{
	return 0;
}

_int32 add_xml_attr(T xml_service, char const *attr_name, char const *attr_value)
{
	return 0;
}

void XMLCALL xml_element_start(void *data, const char *element, const char **attr)
{
	T xml_service = (T) data;
	_int32 i = 0;
	_int32 attr_count = 0;
	void *parent_node = NULL;
	void *node = NULL;
	_int32 parent_type = 0;
	_int32 node_type = 0;
	if (xml_service)
	{
		++xml_service->depth;
		if ( list_size(&xml_service->node_list) > 0)
		{
			parent_node = LIST_VALUE(LIST_RBEGIN(xml_service->node_list));
			parent_type = (_int32)LIST_VALUE(LIST_RBEGIN(xml_service->type_list));
		}
		if (xml_service->xml_node_processer)
		{
			xml_service->xml_node_processer(xml_service->user_data, parent_node, parent_type, xml_service->depth, element, &node, &node_type);
			 list_push(&xml_service->node_list, node);
			 list_push(&xml_service->type_list, (void *)node_type);
			if (xml_service->xml_attr_processer)
			{
				for (i = 0; attr[i]; i += 2)
				{
					xml_service->xml_attr_processer(xml_service->user_data, node, node_type, xml_service->depth, attr[i], attr[i + 1]);
				}
			}
		}
		else if (xml_service->xml_node_ex_processer)
		{
			for (i = 0; attr[i]; i += 2)
			{
				++attr_count;
			}
			xml_service->xml_node_ex_processer(xml_service->user_data, parent_node, parent_type, xml_service->depth, element, attr, attr_count, &node, &node_type);
			 list_push(&xml_service->node_list, node);
			 list_push(&xml_service->type_list, (void *)node_type);
		}
		xml_service->value_length = 0;
		xml_service->state = XS_VALUE;
	}
}

void XMLCALL xml_element_end(void *data, const char *el)
{
	T xml_service = (T) data;
	_int32 i = 0;
	void *node = NULL;
	_int32 node_type = 0;
	char *buffer = NULL;
	if (xml_service)
	{
		if ( list_size(&xml_service->node_list) > 0)
		{
			node = LIST_VALUE(LIST_RBEGIN(xml_service->node_list));
			node_type = (_int32)LIST_VALUE(LIST_RBEGIN(xml_service->type_list));
			 list_erase(&xml_service->node_list, LIST_RBEGIN(xml_service->node_list));
			 list_erase(&xml_service->type_list, LIST_RBEGIN(xml_service->type_list));
		}
		if (xml_service->value_length > 0)
		{
			if (xml_service->xml_value_processer)
			{
				if (xml_service->extend_buffer)
				{
					buffer = xml_service->extend_buffer;
				}
				else
				{
					buffer = xml_service->value_buffer;
				}
				
				if (0 == xml_service->cdata_value)
				{
					for (i = xml_service->value_length - 1; i >= 0; --i)
					{
						if (buffer[i] != ' ' && buffer[i] != '\t' && buffer[i] != '\r' && buffer[i] != '\n')
						{
							break;
						}
					}
					xml_service->value_length = i + 1;
				}
				
				if (xml_service->value_length > 0)
				{
					buffer[xml_service->value_length] = 0;
					xml_service->xml_value_processer(xml_service->user_data, node, node_type, xml_service->depth, buffer);
				}
			}
			xml_service->value_length = 0;
		}
		if (xml_service->xml_node_end_processer)
		{
			xml_service->xml_node_end_processer(xml_service->user_data, node, node_type, xml_service->depth, el);
		}
		--xml_service->depth;
		xml_service->state = XS_ELEMENT;
		xml_service->cdata_value = 0;
		xml_service->need_space = 0;
	}
}

void XMLCALL xml_character_data(void *data, char const *str, int len)
{
	T xml_service = (T) data;
	char *buffer = NULL;
	_int32 old_len = len;
	_int32 need_length;
	_int32 old_need_space = 0;
	_int32 ret = 0;
	if (xml_service)
	{
		if (xml_service->state != XS_VALUE && xml_service->state != XS_CDATA)
		{
			return;
		}
		if (xml_service->state == XS_VALUE && xml_service->cdata_value)
		{
			return;
		}
		
		old_need_space = xml_service->need_space;
		if (0 == xml_service->cdata_value)
		{
			while (len > 0 && (*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n'))
			{
				++str;
				--len;
			}
			if (old_len > len)
			{
				old_need_space = 1;
			}
			old_len = len;
			while (len > 0 && (str[len - 1] == ' ' || str[len - 1] == '\t' || str[len - 1] == '\r' || str[len - 1] == '\n'))
			{
				--len;
			}
			if (old_len > len)
			{
				xml_service->need_space = 1;
			}
		}
		if (len > 0)
		{
			need_length = xml_service->value_length;
			if (need_length > 0 && old_need_space && 0 == xml_service->cdata_value)
			{
				++need_length;
			}
			need_length += len;
			++need_length;
			if (NULL == xml_service->extend_buffer)
			{
				if (need_length <= MAX_VALUE_LENGTH)
				{
					if (xml_service->value_length > 0 && old_need_space && 0 == xml_service->cdata_value)
					{
						xml_service->value_buffer[xml_service->value_length++] = ' ';
					}
					buffer = xml_service->value_buffer;
					buffer += xml_service->value_length;
					 sd_strncpy(buffer, str, len);
					xml_service->value_length += len;
				}
				else
				{
					ret =  sd_malloc(need_length, (void **)&xml_service->extend_buffer);
					if (SUCCESS == ret)
					{
						xml_service->buffer_length = need_length;
						 sd_memset(xml_service->extend_buffer, 0, xml_service->buffer_length);
						if (xml_service->value_length > 0)
						{
							 sd_strncpy(xml_service->extend_buffer, xml_service->value_buffer, xml_service->value_length);
							if (0 == xml_service->cdata_value && old_need_space)
							{
								xml_service->extend_buffer[xml_service->value_length++] = ' ';
							}
						}
						buffer = xml_service->extend_buffer + xml_service->value_length;
						 sd_strncpy(buffer, str, len);
						xml_service->value_length += len;
					}
				}
			}
			else
			{
				if (need_length <= xml_service->buffer_length)
				{
					if (xml_service->value_length > 0 && old_need_space && 0 == xml_service->cdata_value)
					{
						xml_service->extend_buffer[xml_service->value_length++] = ' ';
					}
					buffer = xml_service->extend_buffer + xml_service->value_length;
					 sd_strncpy(buffer, str, len);
					xml_service->value_length += len;
				}
				else
				{
					ret =  sd_malloc(need_length, (void **)&buffer);
					if (SUCCESS == ret)
					{
						 sd_memset(buffer, 0, need_length);
						if (xml_service->value_length > 0)
						{
							 sd_strncpy(buffer, xml_service->extend_buffer, xml_service->value_length);
							if (0 == xml_service->cdata_value && old_need_space)
							{
								buffer[xml_service->value_length++] = ' ';
							}
						}
						 sd_strncpy(buffer + xml_service->value_length, str, len);
						 sd_free(xml_service->extend_buffer);
						xml_service->extend_buffer = buffer;
						xml_service->value_length += len;
						xml_service->buffer_length = need_length;
					}
				}
			}
		}
	}
}

void XMLCALL xml_cdata_start(void *data)
{
	T xml_service = (T) data;
	if (xml_service)
	{
		xml_service->state = XS_CDATA;
		xml_service->cdata_value = 1;
		xml_service->value_length = 0;
	}
}

void XMLCALL xml_cdata_end(void *data)
{
	T xml_service = (T) data;
	if (xml_service)
	{
		xml_service->state = XS_VALUE;
	}
}

#undef T
