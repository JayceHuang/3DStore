/*
 ============================================================================
 Name		 : sd_gz.c
 Author	  	 : xueren
 Copyright   : copyright xunlei 2010
 Description : Static lib source file
 ============================================================================
 */
#include "platform/sd_gz.h"
#ifdef ENABLE_ZIP
//  Include Files  
#include "platform/sd_fs.h"
#include "utility/errcode.h"
#include "utility/string.h"
#include "utility/mempool.h"


#include "zlib.h"

/* gzip flag byte */
#define ASCII_FLAG   0x01 /* bit 0 set: file probably ascii text */
#define HEAD_CRC     0x02 /* bit 1 set: header CRC present */
#define EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define COMMENT      0x10 /* bit 4 set: file comment present */
#define RESERVED     0xE0 /* bits 5..7: reserved */

#define ALLOC_SIZE_OF_OUT_BUFFER           (2048)

static _u32 check_header(char * s, _u32 length);

_u32 sd_gz_uncompress_file(char * filename, char * newname)
{
	z_stream stream;
	char *buffer_in;
	char buffer_inn[4096];
	char buffer_out[4096];
	_int32 ret;
	_u32 input, output;
	_u32 size_in, size_out, this_out;
	_u32 headlen, total_read_size;
	_u64  file_size;

	//Open input file.
	sd_open_ex(filename, O_FS_RDONLY, &input);
	if (input == 0)
		return -1;
	sd_filesize(input, &file_size);

	//Open output file.
	sd_open_ex(newname, O_FS_WRONLY | O_FS_CREATE, &output);
	if (output == (_u32)NULL)
		return -1;
	
	//Read the first part of input file.
	sd_memset(buffer_inn, 0, sizeof(buffer_inn));
	total_read_size = 0;
	sd_read(input, buffer_inn, sizeof(buffer_inn), &size_in);
	total_read_size += size_in;

	//Set input buffer to stream.
	sd_memset(&stream, 0, sizeof(stream));
	buffer_in = buffer_inn;
	stream.next_in = ((unsigned char *) buffer_in);
	stream.avail_in = size_in;

	//Get the data stream
	inflateInit2(&stream, -MAX_WBITS);
	headlen = check_header(buffer_in, size_in);
	if (headlen == -1)
		return -1;
	buffer_in = buffer_in + headlen;
	stream.next_in = ((unsigned char *) buffer_in);
	stream.avail_in = size_in - headlen;
	
	this_out = 0;
	while(1)
	{
		if(total_read_size >= file_size -8)						//if we come to the end, we have kick the CRC bits out of input buffer
			stream.avail_in -= total_read_size + 8 - file_size;
		
		do
		{	
			//Set output buffer to stream.
			sd_memset(buffer_out, 0, sizeof(buffer_out));
			stream.next_out = ((unsigned char *) buffer_out);
			stream.avail_out = sizeof(buffer_out);
			
			//Uncompress as much data as output buffer.
			ret = inflate(&stream, Z_SYNC_FLUSH);
			this_out = stream.total_out - this_out;		
			sd_write(output, buffer_out, this_out, &size_out);
			this_out = stream.total_out;
						
			if(ret < 0)
				break;
			
		} while( ret != 1 && stream.avail_out == 0);
	
		if(ret < 0)
			break;
		
		if(total_read_size >= file_size -8)		//when come to the end of the data.
			break;
		
		//Read more input data.
		sd_memset(buffer_inn, 0, sizeof(buffer_inn));
		sd_read(input, buffer_inn, sizeof(buffer_inn), &size_in);
		total_read_size += size_in;
		stream.next_in = ((unsigned char *) buffer_inn);
		stream.avail_in = size_in;
	}
	
	inflateEnd(&stream);
	sd_close_ex(input);
	sd_close_ex(output);

	return 0;
}

_u32 check_header(char * s, _u32 length)
{
	_u32 flags; /* flags byte */
	_u32 len;
	char * origin = s;

	/* Check the gzip magic header */
	//    for (len = 0; len < 2; len++) {
	//	c = get_byte(s);
	//	if (c != gz_magic[len]) {
	//	    if (len != 0) s->stream.avail_in++, s->stream.next_in--;
	//	    if (c != EOF) {
	//		s->stream.avail_in++, s->stream.next_in--;
	//		s->transparent = 1;
	//	    }
	//	    s->z_err = s->stream.avail_in != 0 ? Z_OK : Z_STREAM_END;
	//	    return;
	//	}
	//    }

	if (length < 10)
		return -1;

	s += 2;
	//method = get_byte(s);
	s += 1;
	flags = *s;
	s += 1;
	//    if (method != Z_DEFLATED || (flags & RESERVED) != 0) {
	//	s->z_err = Z_DATA_ERROR;
	//	return;
	//    }

	/* Discard time, xflags and OS code: */
	//    for (len = 0; len < 6; len++) (void)get_byte(s);
	s += 6;

	length -= 10;

	if ((flags & EXTRA_FIELD) != 0)
	{ /* skip the extra field */
		if (length < 2)
			return -1;
		length -= 2;

		len = (_u32) * s;
		s += 1; //len = (uInt) get_byte(s);
		len += (_u32) * s << 8;
		s += 1; //len += ((uInt) get_byte(s)) << 8;
		/* len is garbage if EOF but the loop below will quit anyway */
		//		while (len-- != 0 && get_byte(s) != EOF)
		//			;
		//�����Ȳ���
		if (length < len)
			return -1;
		length -= len;
		s += len;
	}

	if ((flags & ORIG_NAME) != 0)
	{ /* skip the original file name */
		while (((*s++)) != 0)// && c != EOF)
			if ((length--) == 0)
				return -1;
	}

	if ((flags & COMMENT) != 0)
	{ /* skip the .gz file comment */
		while (((*s++)) != 0)// && c != EOF)
			if ((length--) == 0)
				return -1;
	}

	if ((flags & HEAD_CRC) != 0)
	{ /* skip the header crc */
		//for (len = 0; len < 2; len++)
		if (length < 2)
			return -1;
		s += 2;
	}
	//s->z_err = s->z_eof ? Z_DATA_ERROR : Z_OK;
	return (_u32)(s - origin);
}

_int32 sd_gz_decode_buffer(_u8* buffer_in, _u32 buffer_in_len, _u8** buffer_out, _u32* buffer_out_len,  _u32* decode_len)
{
	_int32 ret = SUCCESS;
	_u32 gzip_header_len = 0;
	_u8* temp_buffer = NULL;
	z_stream stream;
	*decode_len = 0;
	sd_memset(&stream, 0, sizeof(stream));
	//Get the data stream
	inflateInit2(&stream, -MAX_WBITS);
	gzip_header_len = check_header((char*)buffer_in, buffer_in_len);
	if(gzip_header_len == -1)	
		return -1;
	stream.next_in = buffer_in + gzip_header_len;
	stream.avail_in = buffer_in_len - gzip_header_len;
	while(TRUE)
	{
		stream.next_out = (*buffer_out) + (*decode_len);
		stream.avail_out = (*buffer_out_len) - (*decode_len);
		ret = inflate(&stream, Z_SYNC_FLUSH);
		if(ret < 0 || ret > 1)
			return ret;		//������
		(*decode_len) = stream.total_out;
		if(ret == Z_STREAM_END)
			break;			//�ɹ���
		sd_assert(ret == Z_OK);
//		if(ret ==  Z_OK)
//		{	//�ڴ治��
		if(sd_malloc((*buffer_out_len) * 2, (void**)&temp_buffer) != SUCCESS)
			return -1;
		sd_memcpy(temp_buffer, *buffer_out, *decode_len);
		sd_free(*buffer_out);
		*buffer_out = temp_buffer;
		*buffer_out_len = (*buffer_out_len) * 2;
//		}		
	}

	if (inflateEnd(&stream) != Z_OK)
		return -1;

	return SUCCESS;	
}

_int32 sd_gz_encode_buffer(_u8* buffer_in, _u32 buffer_in_len, _u8* buffer_out, _u32 buffer_out_len, _u32* encode_len)
{	
	_int32 ret = SUCCESS; 
	_u32 crc_value = 0;
	z_stream stream;
	if(buffer_out_len < buffer_in_len + 18)		
		return -1;			//���ܳ��Ȳ���
	*encode_len = 0;
	sd_memset(&stream, 0, sizeof(z_stream));
	//Get the data stream
	if (deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY) != Z_OK)
		return -1;
	//���ͷ��
	buffer_out[0] = 0x1f;
	buffer_out[1] = 0x8b;
	buffer_out[2] = 8;
	buffer_out[3] = 0;
	buffer_out[4] = 0;
	buffer_out[5] = 0;
	buffer_out[6] = 0;
	buffer_out[7] = 0;
	buffer_out[8] = 0;
	buffer_out[9] = 0x03;		/* Window 95 & Windows NT */
	*encode_len += 10;
	//��ʼѹ��
	stream.next_in = buffer_in;
	stream.avail_in = buffer_in_len;
	stream.next_out = buffer_out + (*encode_len);
	stream.avail_out = buffer_out_len - (*encode_len);
	//compress as much data as output buffer.
	ret = deflate(&stream, Z_FINISH);
	if(ret != Z_STREAM_END)
		return -1;	//������
	(*encode_len) += stream.total_out;
	//����һ��crc32β��
	sd_assert(*encode_len + 8 <= buffer_out_len);
	crc_value = crc32(crc_value, buffer_in, buffer_in_len);
	sd_memcpy(buffer_out + (*encode_len), &crc_value, sizeof(_u32));
	(*encode_len) += sizeof(_u32);
	sd_memcpy(buffer_out + (*encode_len), &stream.total_in, sizeof(_u32));
	(*encode_len) += sizeof(_u32);

	if (deflateEnd(&stream) != Z_OK)
		return -1;

	return SUCCESS;
}

//////////////////////////////////
//��src�е����ݽ���gzipѹ����ѹ�������ݱ�����des��
//src:��ѹ������
//src_len:��ѹ�����ݳ���
//des:ѹ���������
//des_len:�����������������ȥ�ĵ���des���ڴ���󳤶�,����������ѹ��������ݳ���
_int32 sd_zip_data( const _u8 * src, _u32 src_len,_u8 * des, _u32 * des_len)
{
	_int32 ret = 0;
	char*	tmp_buf;
	_int32  tmp_len;
	_u8* gzed_buf;
	_int32 gzed_len = 0;
	_u32 total_out_buffer_len = *des_len;

	*des_len = 0;
	
	if (src==NULL||src_len==0||des==NULL||total_out_buffer_len==0)
	{
		return INVALID_ARGUMENT;
	}

	tmp_buf = (char*)src;//�����￪ʼѹ��
	tmp_len = src_len;//��Ҫѹ�����ݵĳ���

	// sd_gz_encode_buffer��Ҫ��������Ϊtmp_len+18��buffer��Ϊ���
	ret = sd_malloc(tmp_len+18, (void**)&gzed_buf);
	CHECK_VALUE(ret);

	ret = sd_gz_encode_buffer((_u8 *)tmp_buf, (_u32)tmp_len, gzed_buf, (_u32)tmp_len+18, (_u32*)&gzed_len);
	//��ѹ���ɹ�������Ч������ʹ��ѹ��
	if((SUCCESS == ret)&&(gzed_len < tmp_len)&&(gzed_len<total_out_buffer_len))
	{
		sd_memcpy(des, gzed_buf, gzed_len);
		*des_len=gzed_len;
	}
	else
	{
		ret = -1;
	}

	SAFE_DELETE(gzed_buf);
	return ret;
}

//////////////////////////////////
//��src�е����ݽ���gzip��ѹ����ѹ�����ݱ�����des��
//src:����ѹ����
//src_len:����ѹ���ݳ���
//des:��ѹ�������
//des_len:�����������������ȥ�ĵ���des���ڴ���󳤶�,���������ǽ�ѹ������ݳ���
_int32 sd_unzip_data( const _u8 * src, _u32 src_len,_u8 * des, _u32 * des_len)
{
	_int32 ret = 0;
	_u8*	tmp_buf;
	_int32  tmp_len;
	char*  de_gzed_buf;
	_u32 new_buffer_len = *des_len,de_gzed_len=0;
	_u32 total_out_buffer_len = *des_len;

	*des_len = 0;
	
	if (src==NULL||src_len==0||des==NULL||total_out_buffer_len<src_len)
	{
		return INVALID_ARGUMENT;
	}


	tmp_buf = (_u8*) src;//�����￪ʼ��ѹ
	tmp_len = src_len;//��Ҫ��ѹ���ݵĳ���

	// sd_gz_decode_buffer�������볤������Ϊlen *2��buffer��Ϊ���
	ret = sd_malloc(new_buffer_len, (void**)&de_gzed_buf);
	CHECK_VALUE(ret);

	ret = sd_gz_decode_buffer(tmp_buf, tmp_len, (_u8**)&de_gzed_buf,&new_buffer_len,&de_gzed_len);

	//����ѹ�ɹ�������Ч������ʹ�ý�ѹ
	if((SUCCESS == ret)&&(de_gzed_len<=total_out_buffer_len))
	{
		sd_memcpy(des, de_gzed_buf, de_gzed_len);
		*des_len=de_gzed_len;
	}
	else
	{
		ret = -1;
	}

	SAFE_DELETE(de_gzed_buf);
	return ret;
}

//////////////////////////////////
//�������ļ�����gzip��ѹ�����ý�ѹ����ļ��滻��ѹǰ���ļ�
//file_name:����ѹ�ļ�ȫ��
_int32 sd_unzip_file(const char * file_name)
{
	char * old_file=NULL;
	_int32 ret=0;
	char tmp_file[MAX_FULL_PATH_BUFFER_LEN] = {0};
	
	old_file =(char *)file_name;

	/*��ʱ�ļ�����*/
	sd_snprintf(tmp_file, MAX_FULL_PATH_BUFFER_LEN-1, "%s.tmp",file_name);

	/*��ѹ�ļ�*/
	ret = sd_gz_uncompress_file(old_file, tmp_file);
	
	/*���ļ�·��ָ���ѹ����ļ�*/
	if(SUCCESS == ret )
	{	/*����ɹ�����ɾ�����ļ�������ʱ�ļ�����Ϊ·����ָ�ļ���*/
		sd_delete_file(old_file);
		ret  = sd_rename_file(tmp_file, file_name);
		CHECK_VALUE(ret);
	}
	else
	{	/*����ɾ����ʱ�ļ�*/
		sd_delete_file(tmp_file);
		ret = -1;
	}

	return ret;
}


#endif /* ENABLE_ZIP */
