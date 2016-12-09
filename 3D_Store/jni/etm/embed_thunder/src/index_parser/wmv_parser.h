#if !defined(__WMV_PARSER_20090201)
#define __WMV_PARSER_20090201

#ifdef __cplusplus
extern "C" 
{
#endif
	
#include "utility/define.h"
#include "utility/range.h"

typedef _u8 __GUID[16];

extern const __GUID asf_header;
extern const __GUID file_header;
extern const __GUID stream_header;
extern const __GUID data_header;
extern const __GUID simple_index_header;



typedef struct asf_object_header_ptr
{
	__GUID object_id;
	_u64 object_size;
	
}AsfObjectHeader;

typedef struct header_object_header_ptr
{
	__GUID object_id;
	_u64 object_size;
	_u32 header_object_num;
	_u8 reserved1;
	_u8 reserved2;
}HeaderObjectHeader;

typedef struct file_prop_object_ptr
{
	__GUID object_id;		//Oject ID (16 bytes)
	_u64	object_size;	//Oject Size (8 bytes)
	
	__GUID file_id;
	_u64 file_size;
	_u64 creation_date;
	_u64 data_packets_count;
	_u64 play_duration;
	_u64 send_duration;
	_u64 preroll;
	_u32 flags;
	_u32 min_data_packet_size;
	_u32 max_data_packet_size;
	_u32 max_bitrate;
}FilePropObject;

typedef struct stream_prop_object_ptr
{
	__GUID object_id;
	_u64 object_size;
	
	__GUID stream_type;
	__GUID error_correction_type;
	_u64 time_offset;
	//_u32 type_specific_data_length;
	//_u32 error_correction_data_length;
	//_u16 flags;
	//_u32 reserved;
	
	//type specific data	BYTE varies
	//error correction data	BYTE varies
}StreamPropObject;

typedef struct data_object_header_ptr
{
	__GUID object_id;
	_u64 object_size;
	__GUID file_id;
	_u64 total_data_packets;
	_u16 reserved;
	
	//Data Packets	varies
}DataObjectheader;

typedef struct index_entry_ptr
{
	_u32 packet_number;
	_u16 packet_count;
}IndexEntry;

typedef struct simple_index_object_ptr
{
	__GUID object_id;
	_u64 object_size;
	__GUID file_id;
	_u64 index_entry_time_interval;
	_u32 max_packet_count;
	_u32 index_entries_count;
	
	IndexEntry* index_entries;
}SimpleIndexObject;

typedef struct asf_format_info_ptr
{
	HeaderObjectHeader header_object_header;
	_u64	offset_of_header_object;
	
	FilePropObject	file_prop_object;
	_u64	offset_of_file_prop;
	
	int	stream_num;
	_u64	offset_of_stream_prop;
	
	DataObjectheader data_object_header;
	_u64	offset_of_data_object;
	
	SimpleIndexObject simple_index_object;
	_u64 offset_of_simple_index_object;
	
}AsfFormatInfo;

int asf_read_header(char* head_buffer, _int32 length,  AsfFormatInfo* asf_info);
#ifdef __cplusplus
}
#endif

#endif /*__WMV_PARSER_20090201*/

