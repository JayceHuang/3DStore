
#include "utility/string.h"
#include "utility/utility.h"
#include "utility/url.h"
#include "utility/map.h"
#include "utility/mempool.h"

#include "utility/logid.h"
#define	 LOGID	LOGID_INDEX_PARSER
#include "utility/logger.h"


#include "index_util.h"
#include "index_parser.h"
#include "wmv_parser.h"

const __GUID asf_header = {
    0x30, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11, 0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C
};

const __GUID file_header = {
    0xA1, 0xDC, 0xAB, 0x8C, 0x47, 0xA9, 0xCF, 0x11, 0x8E, 0xE4, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65
};

const __GUID stream_header = {
    0x91, 0x07, 0xDC, 0xB7, 0xB7, 0xA9, 0xCF, 0x11, 0x8E, 0xE6, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65
};

const __GUID data_header = {
    0x36, 0x26, 0xb2, 0x75, 0x8E, 0x66, 0xCF, 0x11, 0xa6, 0xd9, 0x00, 0xaa, 0x00, 0x62, 0xce, 0x6c
};

const __GUID simple_index_header = {
	0x90, 0x08, 0x00, 0x33, 0xB1, 0xE5, 0xCF, 0x11, 0x89, 0xF4, 0x00, 0xA0, 0xC9, 0x03, 0x49, 0xCB
};


#define read_uint8(io, addr) buffer_read_uint8(io, addr)
#define read_uint16_le(io, addr) buffer_read_uint16_le(io, addr)
#define read_uint32_le(io, addr) buffer_read_uint32_le(io, addr)
#define read_uint64_le(io, addr) buffer_read_uint64_le(io, addr)


_int32 ip_memcmp(void *dest, const void *src, _int32 n)
{
	_int32 i;
	i=0;
	while(i<n)
	{
	    if( *((char*)dest+i) != *((char*)src+i))
	    {
	         return -1;
	    }
           i++;
	}

	return SUCCESS;
}

int asf_read_header(char* head_buffer, _int32 length,  AsfFormatInfo* asf_info)
{
	__GUID temp_g;
	_u64 offset;
	AsfObjectHeader temp_header;
	_u32 ret;

	BUFFER_IO io;
	
	io._buffer = head_buffer;
	io._size = length;
	io._pos = 0;
    io.read = ip_std_buffer_read;

	LOG_DEBUG("ip_get_index_range_list_wmv_by_file_head...asf_read_header ");

	//check the __GUID	
       io.read(&io, temp_g, sizeof(__GUID));

	if(SUCCESS != ip_memcmp((void*)asf_header,(void*)temp_g,sizeof(__GUID)))
	{
		LOG_DEBUG("Header Object not found!\n");
		return -1;
	}

	LOG_DEBUG("ip_get_index_range_list_wmv_by_file_head...asf_read_header 1");
	//get the header object structure
	asf_info->offset_of_header_object = 0;
	sd_memcpy(asf_info->header_object_header.object_id,temp_g,sizeof(__GUID));
	ret = read_uint64_le(&io, &asf_info->header_object_header.object_size);
	ret |= read_uint32_le(&io, &asf_info->header_object_header.header_object_num);
	ret |= read_uint8(&io, &asf_info->header_object_header.reserved1);
	ret |= read_uint8(&io, &asf_info->header_object_header.reserved2);
	if(INDEX_PARSER_ERR_OK != ret)
	{
	       LOG_DEBUG("ip_get_index_range_list_wmv_by_file_head...asf_read_header 2 ret =%d", ret);
		return ret;
	}

	//try to get other header objects
	while(1)
	{
		offset = io._pos;
		if((_int64)offset < 0)
		{
			LOG_DEBUG("ip_get_index_range_list_wmv_by_file_head Not valid offset in get_byte_offset!\n");
			return -1;
		}
		
		io.read(&io, temp_g, sizeof(__GUID));

		//first check if it is the data object header
		if(0 == ip_memcmp((void*)data_header,temp_g,sizeof(__GUID)))
		{
	               LOG_DEBUG("ip_get_index_range_list_wmv_by_file_head...data_header");
			sd_memcpy(asf_info->data_object_header.object_id,temp_g,sizeof(__GUID));
			ret = read_uint64_le(&io, &asf_info->data_object_header.object_size);
			io.read(&io, asf_info->data_object_header.file_id, sizeof(__GUID));
			ret |= read_uint64_le(&io, &asf_info->data_object_header.total_data_packets);
			ret |= read_uint16_le(&io, &asf_info->data_object_header.reserved);
			asf_info->offset_of_data_object = offset;	

	              LOG_DEBUG("ip_get_index_range_list_wmv_by_file_head...data_header: offset=%llu object_size=%llu, tatal_data_packets=%llu", 
				  	asf_info->offset_of_data_object,
				  	asf_info->data_object_header.object_size,
				  	asf_info->data_object_header.total_data_packets);
			break;
		}
		else if(0 == ip_memcmp((void*)file_header,temp_g,sizeof(__GUID)))
		{
	               LOG_DEBUG("ip_get_index_range_list_wmv_by_file_head...file_header");
			//Object ID
			sd_memcpy(asf_info->file_prop_object.object_id,temp_g,sizeof(__GUID));

			//Object Size
			ret |= read_uint64_le(&io, &asf_info->file_prop_object.object_size);

			//FILE ID
			io.read(&io, asf_info->file_prop_object.file_id, sizeof(__GUID));

			//FILE Size
			ret |= read_uint64_le(&io, &asf_info->file_prop_object.file_size);
			
			//Creation Date
			ret |= read_uint64_le(&io, &asf_info->file_prop_object.creation_date );

			//Data Packets Count
			ret |= read_uint64_le(&io, &asf_info->file_prop_object.data_packets_count);

			//Play Duration
			ret |= read_uint64_le(&io, &asf_info->file_prop_object.play_duration);

			//Send Duration
			ret |= read_uint64_le(&io, &asf_info->file_prop_object.send_duration);

			//Preroll
			ret |= read_uint64_le(&io, &asf_info->file_prop_object.preroll);

			//Flags
			ret |= read_uint32_le(&io, &asf_info->file_prop_object.flags);

			//Minimun Data Packet Size
			ret |= read_uint32_le(&io, &asf_info->file_prop_object.min_data_packet_size);

			//Maximum Data Packet Size
			ret |= read_uint32_le(&io, &asf_info->file_prop_object.max_data_packet_size);

			//Maximum Bitrate
			ret |= read_uint32_le(&io, &asf_info->file_prop_object.max_bitrate );

			asf_info->offset_of_file_prop = offset;

	              LOG_DEBUG("ip_get_index_range_list_wmv_by_file_head...file_header: offset_of_file_prop=%llu, object_size=%llu, file_size=%llu", 
				  	asf_info->offset_of_file_prop,
				  	asf_info->file_prop_object.object_size,
				  	asf_info->file_prop_object.file_size);
			if(INDEX_PARSER_ERR_OK != ret){
	                     LOG_DEBUG("ip_get_index_range_list_wmv_by_file_head...INDEX_PARSER_ERR_OK!=ret");
				return ret;
			}
		}
		else
		{
	               LOG_DEBUG("ip_get_index_range_list_wmv_by_file_head...skip header");
			//just skip them
			sd_memcpy(temp_header.object_id,temp_g,sizeof(__GUID));
			ret = read_uint64_le(&io, &temp_header.object_size);

			ret = skip_bytes(&io, temp_header.object_size-sizeof(__GUID) - sizeof(_u64));
	               LOG_DEBUG("ip_get_index_range_list_wmv_by_file_head...skip_bytes %llu, return=%d ", temp_header.object_size-24, ret);
			if(SUCCESS != ret){
				break;
			}
			
		}
	}

	/*index offset and length can get from asf_info->offset_of_data_object,asf_info->data_object_header.object_size, asf_info->file_prop_object.file_size
	  offset = asf_info->offset_of_data_object+asf_info->data_object_header.object_size  length= asf_info->file_prop_object.file_size - offset
	*/

	return ret;
}
