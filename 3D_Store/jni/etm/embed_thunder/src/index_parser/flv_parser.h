
#if !defined(__FLV_PARSER_20101029)
#define __FLV_PARSER_20101029

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#include "utility/range.h"
#include "utility/errcode.h"

#define FLVSCRIPTDATAOBJECTEND 0x000009
#define FLV_AUDIODATA	8
#define FLV_VIDEODATA	9
#define FLV_SCRIPTDATAOBJECT	18
#define FLV_HIGH_VEDIORATE  128

typedef struct FLVFileHeader_t
{
	_u8	signature[3];	// Signature byte always "FLV"
	_u8	version;		// File version (for example, 0x01 for FLV version 1)

	union
	{
		struct type_flags_t 
		{
			_u8	video		: 1;	// Video tags are present
			_u8	reserved2	: 1;	// Must be 0
			_u8	audio		: 1;	// Audio tags are present
			_u8	reserved1	: 5;	// Must be 0
		} type_flags;
		_u8 flags;
	}type_flags_u;
	_u32	data_offset;		// Offset in bytes from start of file to start of body (that is, size of header)
}FLVFileHeader_t;

typedef struct FLVTag_t
{
	_u8	tag_type;			// 8:audio; 9:video; 18:script data
	_u8	data_size[3];		// Length of the data in the Data field

	union
	{
		_u32	val32;
		struct 
		{
			_u8	val24[3];	// Time in milliseconds at which the data in this tag applies. 
			// This value is relative to the first tag in the FLV file, 
			// which always has a timestamp of 0.
			_u8	val8;		// Extension of the Timestamp field to form a UI32 value. 
			// This field represents the upper 8 bits, while the previous Timestamp field
			// represents the lower 24 bits of the time in milliseconds.
		} val248;
	} timestamp;

	_u8	stream_id[3];		// Always 0
//	_u8   zero;
	//		void*	data;						// Body of the tag
}FLVTag_t;

typedef struct FLVMetaData_t{
	_u32 hasKeyframes;
	_u32 hasVideo;
	_u32 hasAudio;
	_u32 hasMetadata;
	_u32 hasCuePoints;
	_u32 canSeekToEnd;

	_int64 audiocodecid;
	_int64 audiosamplerate;
	_int64 audiodatarate;
	_int64 audiosamplesize;
	_int64 audiodelay;
	_u32 stereo;

	_int64 videocodecid;
	_int64 framerate;
	_int64 videodatarate;
	_int64 height;
	_int64 width;

	_int64 datasize;
	_int64 audiosize;
	_int64 videosize;
	_int64 filesize;

	_int64 lasttimestamp;
	_int64 lastvideoframetimestamp;
	_int64 lastkeyframetimestamp;
	_int64 lastkeyframelocation;

	_u32 keyframes;
	_int64 *filepositions;
	_int64 *times;
	_int64 duration;

	_u8 metadatacreator[256];
	_u8 creator[256];

	_u32 onmetadatalength;
	_u32 metadatasize;
//	size_t onlastsecondlength;
//	size_t lastsecondsize;
	_u32 hasLastSecond;
	_u32 lastsecondTagCount;
//	size_t onlastkeyframelength;
//	size_t lastkeyframesize;
	_u32 hasLastKeyframe;
}FLVMetaData_t;

typedef struct FLVFileInfo_t
{
	FLVFileHeader_t flv_file_header;
	FLVTag_t flv_scripttag_header;
	_u32 audiodatarate;
	_u32 videodatarate;
	_u32 metadata_offset;
	_u32 metadata_size;
	_u32 end_of_first_audio_tag;
	_u32 end_of_first_video_tag;
}FLVFileInfo;



_u32 read_8(_u8 const* buffer);
_u8* write_8(_u8* buffer, _u32 v);
_u16 read_16(_u8 const* buffer);
_u8* write_16(_u8* buffer, _u32 v);
_u32 read_24(_u8 const* buffer);
_u8* write_24(_u8* buffer, _u32 v);
_u32 read_32(_u8 const* buffer);
_u8* write_32(_u8* buffer, _u32 v);
//unsigned _int64 read_64(_u8 const* buffer);
//_u8* write_64(_u8* buffer, unsigned _int64 v);
//_u32 read_n(_u8 const* buffer, _u32 n);
_u8* write_n(_u8* buffer, _u32 n, _u32 v);

_u8* flv_read_file_header( _u8* buffer, FLVFileHeader_t * flv_header );
_u8* flv_read_pre_tag_size( _u8* buffer, _u32* pre_tag_size );
_u8* read_flv_tags( _u8* buffer, FLVTag_t* flv_tags );

_u8* read_flvscript_value_string( _u8* buffer, _int8* value, _u16* len );
_u8* read_flvscript_value_longstring( _u8* buffer, _int8 * value, _u32* len );
_u8* read_flvscript_value_bool( _u8* buffer, _u8* value );
_u8* read_flvscript_value_double( _u8* buffer, double* value );
_u8* read_flvscript_variables( _u8* buffer, FLVMetaData_t* metadata );
//_u8* read_flvscript_value_variablearray( _u8* buffer, void** value );
//_u8* read_flvscript_value_valuearray( _u8* buffer, void** value, _u32* len );
_int32 is_flv_file( _u8* buffer, _u32 len );

_int32 flv_read_headers( _u8* head_buffer, _u32 length, FLVFileInfo *file_info);

#define AVPROBE_SCORE_MAX 100               ///< maximum score, half of that is used for file-extension-based detection
#define AVPROBE_PADDING_SIZE 32             ///< extra allocated bytes at the end of the probe buffer
#define FLV_AUDIO_FLAG (8)
#define FLV_VIDEO_FLAG (9)

typedef struct {
	double* tav_flv_keyframe;
	double* tav_flv_filepos;
	unsigned int  tav_flvmeta_count;
	unsigned int tav_flv_fi;
	unsigned int tav_flv_ki;
	int tav_flv_seek_mode;
	
} FLVFormatContext;

_int32 flv_analyze_metadata(FLVFormatContext *s,unsigned char*  buf,_u64 slen);
_int32 flv_get_next_tag_start_pos(FLVFormatContext *s,_u64 in_pos,_u64 * start_pos);

#ifdef __cplusplus
}
#endif

#endif /*__FLV_PARSER_20101029*/
