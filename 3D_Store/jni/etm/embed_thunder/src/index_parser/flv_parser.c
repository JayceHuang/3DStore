
#include "utility/string.h"
#include "utility/mempool.h"
#include "flv_parser.h"
#include "utility/bytebuffer.h"

#include "utility/logid.h"
#define	 LOGID	LOGID_INDEX_PARSER
#include "utility/logger.h"

_u32 read_8( _u8 const* buffer )
{
	return buffer[0];
}

_u8* write_8( _u8* buffer, _u32 v )
{
	buffer[0] = (_u8)v;

	return buffer + 1;
}

_u16 read_16( _u8 const* buffer )
{
	return (buffer[0] << 8) |
		(buffer[1] << 0);
}

_u8* write_16( _u8* buffer, _u32 v )
{
	buffer[0] = (_u8)(v >> 8);
	buffer[1] = (_u8)(v >> 0);

	return buffer + 2;
}

_u32 read_24( _u8 const* buffer )
{
	return (buffer[0] << 16) |
		(buffer[1] << 8) |
		(buffer[2] << 0);
}

_u8* write_24( _u8* buffer, _u32 v )
{
	buffer[0] = (_u8)(v >> 16);
	buffer[1] = (_u8)(v >> 8);
	buffer[2] = (_u8)(v >> 0);

	return buffer + 3;
}

_u32 read_32( _u8 const* buffer )
{
	return (buffer[0] << 24) |
		(buffer[1] << 16) |
		(buffer[2] << 8) |
		(buffer[3] << 0);
}

_u8* write_32( _u8* buffer, _u32 v )
{
	buffer[0] = (_u8)(v >> 24);
	buffer[1] = (_u8)(v >> 16);
	buffer[2] = (_u8)(v >> 8);
	buffer[3] = (_u8)(v >> 0);

	return buffer + 4;
}

/*unsigned _double read_64(unsigned char const* buffer)
{
	return ((unsigned __int64)(read_32(buffer)) << 32) + read_32(buffer + 4);
}

unsigned char* write_64(unsigned char* buffer, unsigned __int64 v)
{
	write_32(buffer + 0, (unsigned long)(v >> 32));
	write_32(buffer + 4, (unsigned long)(v >> 0));

	return buffer + 8;
}

unsigned long read_n(unsigned char const* buffer, unsigned int n)
{
	switch(n)
	{
	case 8:
		return read_8(buffer);
	case 16:
		return read_16(buffer);
	case 24:
		return read_24(buffer);
	case 32:
		return read_32(buffer);
	default:
		// program error
		return 0;
	}
}
*/
_u8* flv_read_file_header( _u8* buffer, FLVFileHeader_t * flv_header )
{
	write_24( flv_header->signature, read_24( buffer ) );
	flv_header->version = read_8( buffer + 3 );
	//		unsigned char type_flags = read_8( buffer + 4 );
	//		memcpy( &flv_header->type_flags, &type_flags, 1 );
	flv_header->type_flags_u.flags = read_8( buffer + 4 );
	flv_header->data_offset = read_32( buffer + 5 );
	return buffer + flv_header->data_offset;
}

_u8* flv_read_pre_tag_size( _u8* buffer, _u32* pre_tag_size )
{
	*pre_tag_size = read_32( buffer );
	return buffer + 4;
}

_u8* read_flv_tags( _u8* buffer, struct FLVTag_t* flv_tags )
{
	flv_tags->tag_type = read_8( buffer );
	write_24( flv_tags->data_size, read_24( buffer + 1 ) );
	write_24( flv_tags->timestamp.val248.val24, read_24( buffer + 4 ) );
	flv_tags->timestamp.val248.val8 = read_8( buffer + 7 );
	write_24( flv_tags->stream_id, read_24( buffer + 8 ) );
	//		flv_tags->data = buffer + 11;
	return buffer + 11;//sizeof(FLVTag_t);
}

_u8* read_flvscript_value_string( _u8* buffer, _int8 * value, _u16* len )
{
	_u16 i = 0;
	if(!buffer || !value || !len) 
	{
		sd_assert(FALSE);
		return NULL;
	}
	*len = read_16( buffer );
	for ( i=0; i < *len; i++ )
	{
		*(value + i) = *( (_int8*)buffer + sizeof(_u16) + i );
	}
	return buffer + sizeof( _u16 ) + *len;
}

_u8* read_flvscript_value_longstring( _u8* buffer, _int8 * value, _u32* len )
{
	_u32 i = 0;
	*len = read_32( buffer );
	for ( i=0; i < *len; i++ )
	{
		*(value + i) = *( (_int8*)buffer + sizeof( _u32 ) + i);
	}
	return buffer + sizeof( _u32 ) + *len;
}

_u8* read_flvscript_value_bool( _u8* buffer, _u8* value )
{
	*value = *buffer;
	return buffer + sizeof( _u8 );
}

_u8* read_flvscript_value_double( _u8* buffer, double* value )
{
	_u32 i = 0;
	_u8 temp[8] = {0};
	_int32 ret_val = SUCCESS,buflen = 8;
	for ( i=0; i < 8; i++ )
		temp[ i ] = *(buffer + 7 - i);
	#ifdef MACOS
	*value = *((double*)temp);
	#else
	//sd_assert(sizeof(double)==sizeof(_int64));
	//ret_val = sd_get_int64_from_lt((char**)&temp, &buflen, (_int64 *)value);
	//sd_assert(ret_val==SUCCESS);
	*value = 0;
	#endif
	return buffer + sizeof( double );
}

_u8* read_flvscript_variables( _u8* buffer, FLVMetaData_t* metadata )
{
	//		char szBuf[FLV_MSG_BUF_SIZE] = {0};
	_u8* pBuf = buffer;
	while ( read_24( pBuf ) != FLVSCRIPTDATAOBJECTEND )
	{
		_u32 type = read_8( pBuf );
		pBuf ++;

		switch ( type )
		{
		case 0:	// Double/Number
			{
				double dbValue = 0;
				pBuf = read_flvscript_value_double( pBuf, &dbValue );
			}
			break;
		case 1:	// Boolean
			{
				_u8 bValue = 0;
				pBuf = read_flvscript_value_bool( pBuf, &bValue );
			}
			break;
		case 2:	// String
		case 4:	// Movieclip
			{
				_u16 string_size = 0;
				_int8 string_name[256] = {0};
				pBuf =  read_flvscript_value_string( pBuf, string_name, &string_size );
			}
			break;
		case 3:	// Object
			break;
		case 5:	// Null
		case 6:	// Undefined
			break;
		case 7:	// Reference
			break;
		case 8:	// EMCAArray
			{
				_u32 nEMCAArrayLength =  read_32( pBuf );
				_u32 j;
				pBuf += sizeof( _u32 );

				for ( j = 0; j < nEMCAArrayLength; j++ )
				{
					_u16 nVariableName = 0;
					_int8 szVariableName[256] = {0}; //szVariableValue[65535] = {0};
					_u8 nValueType;
					pBuf =  read_flvscript_value_string( pBuf, szVariableName, &nVariableName );
					nValueType =  read_8( pBuf );
					pBuf += sizeof( _u8 );
					
					switch ( nValueType )
					{
					case 0:
						{
							double dbValue = 0;
							pBuf =  read_flvscript_value_double( pBuf, &dbValue );
							if ( sd_stricmp( szVariableName, "audiocodecid" ) == 0 )
								metadata->audiocodecid = dbValue;
							else if ( sd_stricmp( szVariableName, "audiosamplerate" ) == 0 )
								metadata->audiosamplerate = dbValue;
							else if ( sd_stricmp( szVariableName, "audiodatarate" ) == 0 )
								metadata->audiodatarate = dbValue;
							else if ( sd_stricmp( szVariableName, "audiosamplesize" ) == 0 )
								metadata->audiosamplesize = dbValue;
							else if ( sd_stricmp( szVariableName, "audiodelay" ) == 0 )
								metadata->audiodelay = dbValue;
							else if ( sd_stricmp( szVariableName, "videocodecid" ) == 0 )
								metadata->videocodecid = dbValue;
							else if ( sd_stricmp( szVariableName, "framerate" ) == 0 )
								metadata->framerate = dbValue;
							else if ( sd_stricmp( szVariableName, "videodatarate" ) == 0 )
								metadata->videodatarate = dbValue;
							else if ( sd_stricmp( szVariableName, "height" ) == 0 )
								metadata->height = dbValue;
							else if ( sd_stricmp( szVariableName, "width" ) == 0 )
								metadata->width = dbValue;
							else if ( sd_stricmp( szVariableName, "datasize" ) == 0 )
								metadata->datasize = dbValue;
							else if ( sd_stricmp( szVariableName, "audiosize" ) == 0 )
								metadata->audiosize = dbValue;
							else if ( sd_stricmp( szVariableName, "videosize" ) == 0 )
								metadata->videosize = dbValue;
							else if ( sd_stricmp( szVariableName, "filesize" ) == 0 )
								metadata->filesize = dbValue;
							else if ( sd_stricmp( szVariableName, "lasttimestamp" ) == 0 )
								metadata->lasttimestamp = dbValue;
							else if ( sd_stricmp( szVariableName, "lastvideoframetimestamp" ) == 0 )
								metadata->lastvideoframetimestamp = dbValue;
							else if ( sd_stricmp( szVariableName, "lastkeyframetimestamp" ) == 0 )
								metadata->lastkeyframetimestamp = dbValue;
							else if ( sd_stricmp( szVariableName, "lastkeyframelocation" ) == 0 )
								metadata->lastkeyframelocation = dbValue;
							else if ( sd_stricmp( szVariableName, "duration" ) == 0 )
								metadata->duration = dbValue;
						}
						break;
					case 1:
						{
							_u8 bValue = 0;
							pBuf =  read_flvscript_value_bool( pBuf, &bValue );

							if ( sd_stricmp( szVariableName, "hasKeyframes" ) == 0 )
								metadata->hasKeyframes = bValue;
							else if ( sd_stricmp( szVariableName, "hasVideo" ) == 0 )
								metadata->hasVideo = bValue;
							else if ( sd_stricmp( szVariableName, "hasAudio" ) == 0 )
								metadata->hasAudio = bValue;
							else if ( sd_stricmp( szVariableName, "hasMetadata" ) == 0 )
								metadata->hasMetadata = bValue;
							else if ( sd_stricmp( szVariableName, "hasCuePoints" ) == 0 )
								metadata->hasCuePoints = bValue;
							else if ( sd_stricmp( szVariableName, "canSeekToEnd" ) == 0 )
								metadata->canSeekToEnd = bValue;
							else if ( sd_stricmp( szVariableName, "stereo" ) == 0 )
								metadata->stereo = bValue;

						}
						break;
					case 2:
					case 4:
						{
							_u16 len = read_16( pBuf );
							pBuf += sizeof( _u16 ) + len;    //just pass it
/*							if ( sd_stricmp( szVariableName, "creator" ) == 0 )
							{
								unsigned short nValueLen = 0;
								pBuf =  read_flvscript_value_string( pBuf, szVariableValue, &nValueLen );
								memcpy( metadata->creator, szVariableValue, nValueLen );
								metadata->creator[ nValueLen ] = 0;
							}
							else if ( sd_stricmp( szVariableName, "metadatacreator" ) == 0 )
							{
								unsigned short nValueLen = 0;
								pBuf =  read_flvscript_value_string( pBuf, szVariableValue, &nValueLen );
								memcpy( metadata->metadatacreator, szVariableValue, nValueLen );
								metadata->metadatacreator[ nValueLen ] = 0;
							}
							else
							{
								unsigned short nValueLen = 0;
								pBuf =  read_flvscript_value_string( pBuf, szVariableValue, &nValueLen );
							}
*/
						}
						break;
					case 3:
						{
							while (  read_24( pBuf ) != FLVSCRIPTDATAOBJECTEND )
							{
								_u16 nMemberNameLen = 0;
								_int8 szMemberName[256] = {0};
								_u32 nMemberType;
								pBuf =  read_flvscript_value_string( pBuf, szMemberName, &nMemberNameLen );

								nMemberType =  read_8( pBuf );
								pBuf ++;

								if ( nMemberType == 10 )
								{
									_u32 nCount =  read_32( pBuf );
									pBuf += sizeof( _u32 );

									if ( sd_stricmp( szMemberName, "filepositions" ) == 0 )
									{
										_u32 i;
										_u32 ret;
										if ( metadata->keyframes == 0 )
											metadata->keyframes = nCount;
										ret = sd_malloc( sizeof(double) * nCount, (void**)&metadata->filepositions );
										for ( i = 0; i < nCount; i++ )
										{
											//_u8 nArrValueType =  read_8( pBuf );
											pBuf ++;
											pBuf =  read_flvscript_value_double( pBuf, (double*)&metadata->filepositions[ i ] );
										}
										ret = sd_free(metadata->filepositions);
										metadata->filepositions = NULL;
									}
									else if ( sd_stricmp( szMemberName, "times" ) == 0 )
									{
										_u32 i;
										_u32 ret;
										ret = sd_malloc( sizeof(double) * nCount, (void**)&metadata->times );
										for ( i = 0; i < nCount; i++ )
										{
											//_u8 nArrValueType =  read_8( pBuf );
											pBuf ++;
											pBuf =  read_flvscript_value_double( pBuf,(double*) &metadata->times[ i ] );
										}
										ret = sd_free(metadata->times);
										metadata->times = NULL;
									}
									else
									{
										_u32 i;
										for ( i = 0; i < nCount; i++ )
										{
											//_u8 nArrValueType =  read_8( pBuf );
											double dbValue = 0;
											pBuf ++;
											pBuf =  read_flvscript_value_double( pBuf, &dbValue );
										}
									}
								}
							}
							pBuf += 3;	// pass FLVSCRIPTDATAOBJECTEND
						}
						break;
					case 10:
						{
							_u32 nCount = read_32( pBuf );
							_u32 i;
							pBuf += sizeof( _u32 );

							for ( i = 0; i < nCount; i++ )
							{
								//_u8 nArrValueType = read_8( pBuf );
								double dbValue = 0;
								pBuf ++;
								pBuf = read_flvscript_value_double( pBuf, &dbValue );
							}

						}
						break;
					case 11:
						{
							double dbDatetime = 0;
							_u16 nLocalOffset = 0;
							pBuf = read_flvscript_value_double( pBuf, &dbDatetime );
							nLocalOffset = read_16( pBuf );
							pBuf += sizeof( _u16 );
						}
						break;
					case 12:
						{
							_u32 len = read_32( pBuf );
							pBuf += sizeof( _u8 ) + len;    //just pass it
//							unsigned long nValueLen = 0;
//							pBuf = read_flvscript_value_longstring( pBuf, szVariableValue, &nValueLen );
						}
						break;
					}
				}
			}
			break;
		case 10:// Strict Array
			{
				_u32 nCount = read_32( pBuf );
				_u32 i;
				pBuf += sizeof( _u32 );
				for ( i = 0; i < nCount; i++ )
				{
					//_u8 nArrValueType = read_8( pBuf );
					double dbValue = 0;
					pBuf ++;
					pBuf = read_flvscript_value_double( pBuf, &dbValue );
				}
			}
			break;
		case 11:// Date
			{
				double dbDatetime = 0;
				_u16 nLocalOffset = 0;
				pBuf = read_flvscript_value_double( pBuf, &dbDatetime );
				nLocalOffset = read_16( pBuf );
				pBuf += sizeof( _u16 );
			}
			break;
		case 12:// Long String
			{
				_u32 len = read_32( pBuf );
				pBuf += sizeof( _u32 ) + len;    //just pass it
//				char szVariableValue[65535] = {0};
//				unsigned long nValueLen = 0;
//				pBuf = read_flvscript_value_longstring( pBuf, szVariableValue, &nValueLen );
			}
			break;
		default:
			return pBuf;
			break;
		}
	}

	pBuf += 3;	// pass FLVSCRIPTDATAOBJECTEND*/
	return pBuf;
}

_int32 is_flv_file( _u8* buffer, _u32 len )
{
	if ( len < 3 )
		return -1;

	if ( buffer[0] == 'F' && buffer[1] == 'L' && buffer[2] == 'V' )
		return 0;
	else
		return 0;
}

_int32 read_flv_first_vidoe_audio_tag( _u8* start_of_tag,_u32 offset_from_header, _u32 length , FLVFileInfo *file_info)
{
	BOOL get_audio_tag = FALSE,get_video_tag = FALSE;
	_u32 size = 0;
	_u8 * buffer = start_of_tag;
	_u8 flv_tag_flag = 0;

	/* 跳过前一个tag的长度(4bytes) */
	buffer+=4;

	while(TRUE)
	{
		/* 获取当前tag的标识位 */
		flv_tag_flag = *buffer;
		buffer++;

		/* 获取当前tag的数据段长度 */
		size=buffer[0]<<16;
		size+=buffer[1]<<8;
		size+=buffer[2];
		
		switch(flv_tag_flag)
		{
			case FLV_AUDIO_FLAG:
				if(!get_audio_tag)
				{
					/* 获取第一个audio tag的结束位置偏移 */
					file_info->end_of_first_audio_tag = offset_from_header+(buffer-start_of_tag+10)+size;
					get_audio_tag = TRUE;
				}
				break;
			case FLV_VIDEO_FLAG:
				if(!get_video_tag)
				{
					/* 获取第一个video tag的结束位置偏移 */
					file_info->end_of_first_video_tag = offset_from_header+(buffer-start_of_tag+10)+size;
					get_video_tag = TRUE;
				}
				break;
		}

		if(get_audio_tag&&get_video_tag)
		{
			/* 第一个音频tag和第一个视频tag的结束偏移均已得到 */
			return SUCCESS;
		}

		/* 跳过3bytes的data size, 4bytes 的timestamp和3bytes的streamID*/
		buffer+=10;

		/* 跳过当前tag的数据段 */
		buffer+=size;

		if(start_of_tag+length<buffer+15)
		{
			return ERR_VOD_INDEX_FILE_HEAD_TOO_SMALL;
		}

		/* 跳过当前tag的长度(4bytes) */
		buffer+=4;
	}

	
	return SUCCESS;
}

_int32 flv_read_headers( _u8* head_buffer, _u32 length, FLVFileInfo *file_info )
{
	FLVMetaData_t* metadata; 

	_u8* buffer = (_u8*)head_buffer;
	_u32 pre_tag_size = 0;
	_int32 ret = 0;

	ret = sd_malloc(sizeof(FLVMetaData_t), (void**)&metadata);
	sd_memset(metadata, 0, sizeof(FLVMetaData_t));

	buffer = flv_read_file_header( buffer, &file_info->flv_file_header );
	buffer = flv_read_pre_tag_size( buffer, &pre_tag_size );
	buffer = read_flv_tags( buffer, &file_info->flv_scripttag_header );
	
	file_info->metadata_offset = 24;
	file_info->metadata_size = read_24( file_info->flv_scripttag_header.data_size );
	
	if( length < file_info->metadata_offset + file_info->metadata_size )
	{	
		file_info->audiodatarate = 0;
		file_info->videodatarate = 0;		
		LOG_DEBUG("head buffer is not large enough");
		sd_free(metadata);
		metadata = NULL;
		return ERR_VOD_INDEX_FILE_HEAD_TOO_SMALL;
	}
	else
	{
		buffer = read_flvscript_variables( buffer, metadata );
		file_info->audiodatarate = metadata->audiodatarate * 1024;
		file_info->videodatarate = metadata->videodatarate * 1024;

		#ifdef ENABLE_FLASH_PLAYER		
		if(length+head_buffer<buffer+15) 
		{
			sd_free(metadata);
			metadata = NULL;
			return ERR_VOD_INDEX_FILE_HEAD_TOO_SMALL;
		}
		ret = read_flv_first_vidoe_audio_tag( buffer,buffer-head_buffer, length-(buffer-head_buffer),file_info );
		if(ret!=SUCCESS)
		{
			sd_assert(FALSE);
		}
		#endif /* ENABLE_FLASH_PLAYER */
	}

	sd_free(metadata);
	metadata = NULL;
	return ret;
}

#define READDOUBLE64(raw,t,d,ptr) {\
								d = *((double*)raw);\
								ptr = (unsigned char*)&d;\
								t = ptr[0];\
								ptr[0] = ptr[7];\
								ptr[7] = t;\
								t = ptr[1];\
								ptr[1] = ptr[6];\
								ptr[6] = t;\
								t = ptr[2];\
								ptr[2] = ptr[5];\
								ptr[5] = t;\
								t = ptr[3];\
								ptr[3] = ptr[4];\
								ptr[4] = t;\
								}


_int32 flv_analyze_metadata(FLVFormatContext *s,unsigned char*  buf,_u64 slen)	
{
	_int32 ret_val = SUCCESS;
		unsigned char* pBuf =NULL;
		unsigned char*  mbuf= NULL;
		unsigned char tmp = 0;
		unsigned char* t_ptr = 0;
		//double d_t = 0;
		double* g_flv_keyframe = s->tav_flv_keyframe;
		double* g_flv_filepos = s->tav_flv_filepos;
		pBuf  = buf;
		mbuf = pBuf;		
		_u64 xlen = 0;
		char szMemberName[256] = {0};
		while(!(mbuf[0] == 0 &&  mbuf[1] == 0 && mbuf[2]==0x09))
		{
			unsigned char  type = *pBuf;
			pBuf++;
			xlen++;
			switch ( type )
			{
			case 0:	// Double/Number
				{					
					pBuf = pBuf + 8;
					xlen +=8;
				}
				break;
			case 1:	// Boolean
				{					
					pBuf++;
					xlen++;
				}
				break;
			case 2:	// String
				{
					unsigned short len = (*pBuf)<<8;
					len +=*(pBuf+1);
					pBuf+= sizeof(unsigned short);
					pBuf+=len;
					xlen+=len;
					xlen+= sizeof(unsigned short);					
				}
				break;
			case 4:	// Movieclip
				{
					unsigned short len = *pBuf <<8 ;
					len +=*(pBuf+1);
					pBuf+= sizeof(unsigned short);
					pBuf+=len;
					xlen+=len;
					xlen+= sizeof(unsigned short);

				}
				break;
			case 3:	// Object
				break;
			case 5:	// Null
			case 6:	// Undefined
				break;
			case 7:	// Reference
				break;
			case 8:	// EMCAArray
				{
					unsigned long nEMCAArrayLength = *pBuf<<24;
					nEMCAArrayLength += *(pBuf + 1)<<16;
					nEMCAArrayLength += +*(pBuf + 2)<<8;
					nEMCAArrayLength +=  *(pBuf+3);
					pBuf += sizeof( unsigned long );
					xlen+= sizeof(unsigned long);
					int j = 0;
					for ( ; j < nEMCAArrayLength; j++ )
					{
						unsigned short len = *pBuf <<8 ;
						len +=*(pBuf+1);
						pBuf+= sizeof(unsigned short);
						xlen+= sizeof(unsigned short);
						//av_log(NULL, AV_LOG_WARNING, "%d\n",len);
						int gi  = 0;
						for (gi =0; gi<len; gi++) {
							*(szMemberName+gi)= *(pBuf);
							pBuf++;
							xlen++;
						}
						*(szMemberName+len) =0;
						//av_log(NULL, AV_LOG_WARNING, "%s\n",szMemberName);
						
						unsigned char nValueType = *pBuf;
						pBuf += sizeof( unsigned char );
						xlen++;
						switch ( nValueType )
						{
						case 0:
							{
								pBuf += sizeof( double);
								xlen+=8;
							}
							break;
						case 1:
							{
								xlen++;
								pBuf++;
							}
							break;
						case 2:
							{
								unsigned short vlen = (*pBuf)<<8;
								vlen +=*(pBuf+1);
								pBuf+= sizeof(unsigned short);
								pBuf+=vlen;
								xlen+=vlen;
								xlen+= sizeof(unsigned short);	
							}
							break;
						case 4:
							{
								unsigned short len = *pBuf <<8 ;
								len +=*(pBuf+1);
								pBuf+= sizeof(unsigned short);
								xlen+= sizeof(unsigned short);
								pBuf+=len;
								xlen+=len; 
							}
							break;
						case 3:
							{	
								unsigned char* mtbuf = NULL; 
								mtbuf = pBuf ;
								while ( !(mtbuf[0]== 0 &&  mtbuf[1] == 0 && mtbuf[2] ==0x09) )
								{								
									if (s->tav_flv_ki >0 && s->tav_flv_fi >0) {										
										return SUCCESS;
									}
									//av_log(NULL, AV_LOG_WARNING, "start  ki ,fi  at the %d\n",xlen);
									unsigned short len = *pBuf <<8;
									len +=*(pBuf+1);
									pBuf+= sizeof(unsigned short);
									xlen +=sizeof(unsigned short);	
									if(xlen >= slen ||len >=254)
									{
										return SUCCESS;
									}
									int k =0;
									for (k =0; k<len; k++) {
										*(szMemberName+k)= *(pBuf);
										pBuf++;
										xlen++;
									}
									*(szMemberName+len) =0;
									//av_log(NULL, AV_LOG_WARNING, "%s\n",szMemberName);
									unsigned char  nMemberType = *pBuf;
									pBuf ++;
									xlen++;

									if ( nMemberType == 10 )
									{
										unsigned long nCount = *pBuf<<24; 
										nCount +=*(pBuf + 1)<<16 ;
										nCount +=*(pBuf + 2)<<8;
										nCount +=*(pBuf+3);										
										
										pBuf += sizeof( unsigned long );
										xlen += sizeof( unsigned long );
										if (nCount == 0) {
											break;
										}
										//av_log(NULL, AV_LOG_WARNING, " filelen  %d\n",xlen);
										if ( sd_strcmp( szMemberName, "filepositions" ) == 0 )
										{
											s->tav_flv_fi = nCount;
											if(g_flv_filepos ==NULL)
											{
												ret_val = sd_malloc(nCount*sizeof(double), (void**)&g_flv_filepos);
												CHECK_VALUE(ret_val);
												s->tav_flv_filepos = g_flv_filepos;
											}
											//av_log(NULL, AV_LOG_WARNING, " file pos %d\n",nCount);
											int i = 0;
											for ( i = 0; i < nCount; i++ )
											{
												//unsigned char nArrValueType = *pBuf;
												pBuf++;
												double xnt = 0;// = readdouble64(pBuf);
												READDOUBLE64(pBuf,tmp,xnt,t_ptr);
												*(g_flv_filepos + i) = xnt;
												//av_log(s, AV_LOG_WARNING, " file pos =%8.8f \n   ",xnt);
												pBuf +=8;
												xlen++;
												xlen  +=sizeof(double);
												
											}
										}
										else if ( sd_strcmp( szMemberName, "times" ) == 0 )
										{
											s->tav_flv_ki = nCount;
											if(g_flv_keyframe ==NULL)
											{
												ret_val = sd_malloc(nCount*sizeof(double), (void**)&g_flv_keyframe);
												CHECK_VALUE(ret_val);
												s->tav_flv_keyframe = g_flv_keyframe;
											}
											int i = 0;
											for ( i= 0; i < nCount; i++ )
											{
												if (xlen >= slen) {
													s->tav_flv_ki = i;
													return SUCCESS;
												}
												
												//unsigned char nArrValueType = *pBuf;
												pBuf++;
												double value = 0;//readdouble64(pBuf);
												READDOUBLE64(pBuf,tmp,value,t_ptr);
												pBuf +=8;
												xlen++;
												xlen  +=8;												

												*(g_flv_keyframe+i) = value;

											}
											 //*/
										}
										else
										{
											 int i = 0;
											for (; i < nCount; i++ )
											{
												//unsigned char nArrValueType = *pBuf;
												pBuf ++;
												xlen++;
												pBuf +=sizeof(double);
												xlen +=sizeof(double);
											}
										}
										
									}
									//av_log(NULL, AV_LOG_WARNING, " filelen  %x, %x,%x  %d\n",*pBuf,*(pBuf+1),*(pBuf+2),xlen);
									
									mtbuf = pBuf;

									
									if (xlen >= slen) {
										return SUCCESS;
									}
								}
								pBuf +=3;
								xlen +=3;
							}
							break;
						case 10:
							{
								unsigned long nCount = *pBuf<<24;
								nCount += *(pBuf + 1)<<16;
								nCount += *(pBuf + 2)<<8;
								nCount +=*(pBuf+3);

								pBuf += sizeof( unsigned long );
								xlen += sizeof( unsigned long );
								if(nCount == 0)
									break;
								if ( sd_strcmp( szMemberName, "filepositions" ) == 0 )
								{
									s->tav_flv_fi = nCount;

									if(g_flv_filepos ==NULL)
									{
										ret_val = sd_malloc(nCount*sizeof(double), (void**)&g_flv_filepos);
										CHECK_VALUE(ret_val);
										s->tav_flv_filepos = g_flv_filepos;
									}
									
									int i = 0;								
									for (; i < nCount; i++ )
									{
										//unsigned char nArrValueType = *pBuf;
										pBuf++;
										double xnt = 0;// = readdouble64(pBuf);
										READDOUBLE64(pBuf,tmp,xnt,t_ptr);
										*(g_flv_filepos + i) = xnt;
										//unsigned long ynt = (unsigned long)xnt;
										pBuf +=8;
										xlen++;
										xlen  +=sizeof(double);									
											
									}
								}
								else if ( sd_strcmp( szMemberName, "times" ) == 0 )
								{
									s->tav_flv_ki = nCount;
									
									if(g_flv_keyframe ==NULL)
									{
										ret_val = sd_malloc(nCount*sizeof(double), (void**)&g_flv_keyframe);
										CHECK_VALUE(ret_val);
										s->tav_flv_keyframe = g_flv_keyframe;
									}

									 int i = 0;
									for (; i < nCount; i++ )
									{
										//unsigned char nArrValueType = *pBuf;
										pBuf++;
										
										double value = 0;//readdouble64(pBuf);
										READDOUBLE64(pBuf,tmp,value,t_ptr);
										
										*(g_flv_keyframe+i) = value;

										pBuf +=8;
										xlen++;
										xlen  +=sizeof(double);
									}
								}
								else
								{ 
									int i = 0;
									for ( ; i < nCount; i++ )
									{
										//unsigned char nArrValueType = *pBuf;
										pBuf ++;
										xlen++;
										pBuf +=sizeof(double);
										xlen +=sizeof(double);
									}
								}								

							}
							break;
						case 11:
							{
									pBuf +=sizeof(double);
									xlen+=sizeof(double);
									pBuf += sizeof( short );
									xlen +=sizeof(short);
							}
							break;
						case 12:
							{
								unsigned long nValueLen = *pBuf<<24;
								nValueLen += *(pBuf + 1)<<16;
								nValueLen +=*(pBuf + 2)<<8;
								nValueLen +=*(pBuf+3);
								pBuf += sizeof( unsigned long );
								xlen +=sizeof( unsigned long );
								pBuf = pBuf + nValueLen;
								xlen +=nValueLen;
							}
							break;
						}
					}
				}
				break;
			case 10:// Strict Array
				{
					 int i = 0;
					unsigned long nCount = *pBuf<<24;
					nCount += *(pBuf + 1)<<16;
					nCount += *(pBuf + 2)<<8;
					nCount +=*(pBuf+3);
					pBuf += sizeof( unsigned long );
					xlen += sizeof( unsigned long );
					for ( ;i < nCount; i++ )
					{
						pBuf ++;
						xlen++;
						pBuf +=sizeof(double);
						xlen+=sizeof(double);
					}
				}
				break;
			case 11:// Date
				{
					
					pBuf +=sizeof(double);
					xlen+=sizeof(double);
					pBuf += sizeof( short );
					xlen +=	sizeof( short );
				}
				break;
			case 12:// Long String
				{
					unsigned long nValueLen = *pBuf<<24;
					nValueLen += *(pBuf + 1)<<16;
					nValueLen +=*(pBuf + 2)<<8;
					nValueLen +=*(pBuf+3);
					pBuf += sizeof( unsigned long );
					xlen +=	sizeof( unsigned long );
					pBuf = pBuf + nValueLen;
					xlen +=nValueLen;
				}
				break;
			default:				
				break;
			}
			mbuf = pBuf;
			if (xlen >= slen) {
				break;
			}
		}
		return SUCCESS;
}
_int32 flv_get_next_tag_start_pos(FLVFormatContext *s,_u64 in_pos,_u64 * start_pos)	
{
	double* g_flv_filepos = s->tav_flv_filepos;
	unsigned int tav_flv_fi = s->tav_flv_fi,i = 0;

	*start_pos = 0;
	
	for(i=0;i<tav_flv_fi;i++)
	{
		if(*(g_flv_filepos+i)>in_pos)
		{
			*start_pos = *(g_flv_filepos+i);
			return SUCCESS;
		}
	}

	return -1;	
}

