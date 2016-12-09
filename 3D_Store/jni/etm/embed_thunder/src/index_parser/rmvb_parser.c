
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
#include "rmvb_parser.h"

#define read_uint8(io, addr) buffer_read_uint8(io, addr)
#define read_uint16_be(io, addr) buffer_read_uint16_be(io, addr)
#define read_uint32_be(io, addr) buffer_read_uint32_be(io, addr)
#define read_uint8_to(io, addr) read_uint8(io, addr)
#define read_uint16_be_to(io, addr) read_uint16_be(io, addr)
#define read_uint32_be_to(io, addr) read_uint32_be(io, addr)
#define get_fourcc(b) rmff_get_uint32_be(b)

_int32 rmff_read_headers(char* head_buffer, _int32 length, RMFF_FILE_T *file) 
{
  _u32 object_id, object_size, size;
  _u16 object_version;
  rmff_rmf_t *rmf;
  rmff_prop_t *prop;
  rmff_cont_t *cont;
  rmff_mdpr_t *mdpr;
  rmff_track_t *track;
//  real_video_props_t *rvp;
//  real_audio_v4_props_t *ra4p;
  rmff_track_t my_track;
  BUFFER_IO io;
  _int32 prop_header_found;
  _u32 ret;
  _u16  size_short;
  _u8    size_byte;


  LOG_DEBUG("rmff_read_headers... ");

  io._buffer = head_buffer;
  io._size = length;
  io._pos = 0;
  io.read = ip_std_buffer_read;

  rmf = &file->rmf_header;
  ret = read_uint32_be_to(&io, &rmf->obj.id);
  ret |= read_uint32_be_to(&io, &rmf->obj.size);
  ret |= read_uint16_be_to(&io, &rmf->obj.version);
  ret |= read_uint32_be_to(&io, &rmf->format_version);
  ret |= read_uint32_be_to(&io, &rmf->num_headers);
  if (ret != RMFF_ERR_OK)
  {
    LOG_DEBUG("rmff_read_headers  ret = %d", ret);
    return ret;
  }

  LOG_DEBUG("rmff_read_headers 2... ");

  prop = &file->prop_header;
  prop_header_found = 0;
  cont = &file->cont_header;
  while (1) {
    LOG_DEBUG("rmff_read_headers 3... ");
    ret = RMFF_ERR_OK;
    ret |= read_uint32_be(&io, &object_id);
    ret |=  read_uint32_be(&io, &object_size);
    ret |= read_uint16_be(&io, &object_version);
    if (ret != RMFF_ERR_OK)
      break;

    if (object_id == rmffFOURCC('P', 'R', 'O', 'P')) {
       LOG_DEBUG("rmff_read_headers PROP... ");
      rmff_put_uint32_be(&prop->obj.id, object_id);
      rmff_put_uint32_be(&prop->obj.size, object_size);
      rmff_put_uint16_be(&prop->obj.version, object_version);
      ret |= read_uint32_be_to(&io, &prop->max_bit_rate);
      ret |= read_uint32_be_to(&io, &prop->avg_bit_rate);
      ret |= read_uint32_be_to(&io, &prop->max_packet_size);
      ret |= read_uint32_be_to(&io, &prop->avg_packet_size);
      ret |= read_uint32_be_to(&io, &prop->num_packets);
      ret |= read_uint32_be_to(&io, &prop->duration);
      ret |= read_uint32_be_to(&io, &prop->preroll);
      ret |= read_uint32_be_to(&io, &prop->index_offset);
      ret |= read_uint32_be_to(&io, &prop->data_offset);
      ret |= read_uint16_be_to(&io, &prop->num_streams);
      ret |= read_uint16_be_to(&io, &prop->flags);
	  if (ret != RMFF_ERR_OK)
		  break;
      prop_header_found = 1;

    } 
    else if (object_id == rmffFOURCC('C', 'O', 'N', 'T')) {
       LOG_DEBUG("rmff_read_headers CONT... ");
      if (file->cont_header_present) {
      }
      sd_memset(cont, 0, sizeof(rmff_cont_t));
      rmff_put_uint32_be(&cont->obj.id, object_id);
      rmff_put_uint32_be(&cont->obj.size, object_size);
      rmff_put_uint16_be(&cont->obj.version, object_version);

      ret = read_uint16_be(&io, &size_short); /* title_len */
      if (RMFF_ERR_OK == ret && size_short > 0) {
        //ret = sd_malloc(size_short + 1, (void**)&(cont->title));
		//CHECK_VALUE(ret);
        //io.read(&io, cont->title, size_short);
        ret = skip_bytes(&io, (_u64)size_short);
	 CHECK_VALUE(ret);
      }
      ret = read_uint16_be(&io, &size_short); /* author_len */
      if (RMFF_ERR_OK == ret && size_short > 0) {
		//ret = sd_malloc(size_short + 1, (void**)&(cont->author));
		//CHECK_VALUE(ret);
        //io.read(&io, cont->author, size_short);
        ret = skip_bytes(&io, (_u64)size_short);
	 CHECK_VALUE(ret);
      }
      ret =  read_uint16_be(&io, &size_short); /* copyright_len */
      if (RMFF_ERR_OK == ret && size_short > 0) {
	    //ret = sd_malloc(size_short + 1, (void**)&(cont->copyright));
		//CHECK_VALUE(ret);
        //io.read(&io, cont->copyright, size_short);
        ret = skip_bytes(&io, (_u64)size_short);
	CHECK_VALUE(ret);
      }
      ret =  read_uint16_be(&io, &size_short); /* comment_len */
      if (RMFF_ERR_OK == ret && size_short > 0) {
		//ret = sd_malloc(size_short + 1, (void**)&(cont->comment));
		//CHECK_VALUE(ret);
        //io.read(&io, cont->comment, size_short);
        ret = skip_bytes(&io, (_u64)size_short);
	CHECK_VALUE(ret);
      }
      file->cont_header_present = 1;

    } else if (object_id == rmffFOURCC('M', 'D', 'P', 'R')) {
       LOG_DEBUG("rmff_read_headers MDPR... ");
	//  ret = sd_malloc(sizeof(rmff_track_t), (void** )&(track));
       //	CHECK_VALUE(ret);
      track = &my_track;
      sd_memset(&my_track, 0, sizeof(rmff_track_t));

      track->file = (struct rmff_file_t *)file;
      mdpr = &track->mdpr_header;
      rmff_put_uint32_be(&mdpr->obj.id, object_id);
      rmff_put_uint32_be(&mdpr->obj.size, object_size);
      rmff_put_uint16_be(&mdpr->obj.version, object_version);
      ret |= read_uint16_be_to(&io, &mdpr->id);
      track->id = rmff_get_uint16_be(&mdpr->id);
      ret |= read_uint32_be_to(&io, &mdpr->max_bit_rate);
      ret |= read_uint32_be_to(&io, &mdpr->avg_bit_rate);
      ret |= read_uint32_be_to(&io, &mdpr->max_packet_size);
      ret |= read_uint32_be_to(&io, &mdpr->avg_packet_size);
      ret |= read_uint32_be_to(&io, &mdpr->start_time);
      ret |= read_uint32_be_to(&io, &mdpr->preroll);
      ret |= read_uint32_be_to(&io, &mdpr->duration);
	  if (ret != RMFF_ERR_OK)
		  break;
      ret = read_uint8(&io, &size_byte ); /* stream_name_len */
      if (RMFF_ERR_OK == ret && size_byte > 0) {
 		//ret =sd_malloc(size_byte + 1, (void**)&(mdpr->name));
		//CHECK_VALUE(ret);
              //io.read(&io, mdpr->name, size_byte);
              //mdpr->name[size_byte] = 0;
             ret = skip_bytes(&io, (_u64)size_byte);
		CHECK_VALUE(ret);
      }
      ret = read_uint8(&io , &size_byte); /* mime_type_len */
      if (RMFF_ERR_OK == ret &&size_byte > 0) {
		//ret = sd_malloc(size_byte + 1, (void**)&(mdpr->mime_type));
		//CHECK_VALUE(ret);
              // io.read(&io, mdpr->mime_type, size_byte);
              // mdpr->mime_type[size_byte] = 0;
        ret = skip_bytes(&io, (_u64)size_byte);
		CHECK_VALUE(ret);
      }
      ret = read_uint32_be(&io , &size);  /* type_specific_size */
      rmff_put_uint32_be(&mdpr->type_specific_size, size);
      if (RMFF_ERR_OK == ret &&size > 0) {
		//ret = sd_malloc(size + 1, (void**)&(mdpr->type_specific_data));
		//CHECK_VALUE(ret);
              //io.read(&io, mdpr->type_specific_data, size);
              ret = skip_bytes(&io, (_u64)size);
		CHECK_VALUE(ret);
      }

#if 0
      rvp = (real_video_props_t *)mdpr->type_specific_data;
      ra4p = (real_audio_v4_props_t *)mdpr->type_specific_data;
      if ((size >= sizeof(real_video_props_t)) &&
          (get_fourcc(&rvp->fourcc1) == rmffFOURCC('V', 'I', 'D', 'O')))
	  {
        track->type = RMFF_TRACK_TYPE_VIDEO;
      } 
	  else if ((size >= sizeof(real_audio_v4_props_t)) &&
               (get_fourcc(&ra4p->fourcc1) ==
                rmffFOURCC('.', 'r', 'a', 0xfd))) 
	  {
        track->type = RMFF_TRACK_TYPE_AUDIO;
        if ((rmff_get_uint16_be(&ra4p->version1) == 5) &&
            (size < sizeof(real_audio_v5_props_t)))
		{
          /*return set_error(RMFF_ERR_DATA, "RealAudio v5 data indicated but data too small", RMFF_ERR_DATA);*/
		  return RMFF_ERR_DATA;
		}
      }
#endif

	  /*don't save track's property at this time*/
	  /*
      track->internal = safecalloc(sizeof(rmff_track_internal_t));
      file->tracks =
        (rmff_track_t **)saferealloc(file->tracks, (file->num_tracks + 1) *
                                     sizeof(rmff_track_t *));
      file->tracks[file->num_tracks] = track;
      file->num_tracks++;
	  */

    } else if (object_id == rmffFOURCC('D', 'A', 'T', 'A')) {
      /*fint->data_offset = (_u32)io._pos - (4 + 4 + 2);*/
      ret = read_uint32_be(&io, &file->num_packets_in_chunk);
      /*fint->next_data_offset = read_uint32_be(&io);*/
      break;

    } else {
      /* Unknown header type */
      return RMFF_ERR_DATA;
    }
  }

  LOG_DEBUG("rmff_read_headers 4... prop_header_found=%d, file->prop_header.index_offset=%d", prop_header_found , file->prop_header.index_offset);
  if (prop_header_found ) {
    if (rmff_get_uint32_be(&file->prop_header.index_offset) > 0) {
        LOG_DEBUG("rmff_read_headers  got index offset = %u ", file->prop_header.index_offset);
		  /*
	      old_pos = io->tell(fh);
	      read_index(file, rmff_get_uint32_be(&file->prop_header.index_offset));
	      io->seek(fh, old_pos, SEEK_SET);
		  */
    }
    file->headers_read = 1;
    return RMFF_ERR_OK;
  }
  return RMFF_ERR_DATA;
}
