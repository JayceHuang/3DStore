

#include"upload_read_file.h"

#include "platform/sd_fs.h"
#include "utility/logid.h"
#define LOGID LOGID_FILE_MANAGER
#include "utility/logger.h"

_int32 up_create_file_struct( const char *p_file_name, const char *p_file_path, _u64 file_size, void *p_user, notify_file_create p_call_back, struct tagTMP_FILE **pp_file_struct )
{
         char full_file_path[MAX_FILE_PATH_LEN];

	 _u32 len = 0;
	 _int32 ret_val = 0;

	 _u32 cur_len = 0;

	 if(p_file_name == NULL || p_file_path == NULL || pp_file_struct == NULL)
	 {
	       LOG_DEBUG( "up_create_file_struct  invalid parameter." );
	       return FILE_INVALID_PARA;
	 }

     LOG_DEBUG( "up_create_file_struct  , filename:%s, file_path: %s.  p_user:0x%x .", p_file_name, p_file_path, p_user);
	 
	 len = sd_strlen(p_file_path);
        if(cur_len+len > MAX_FILE_PATH_LEN)
        {
               return FILE_PATH_TOO_LONG;
        }
	 sd_strncpy(full_file_path+cur_len, p_file_path, len);

	 cur_len += len; 

	 if(full_file_path[cur_len-1] != '/' ||  full_file_path[cur_len-1] != '\\' )
	 {
	       full_file_path[cur_len] = '/';
	       cur_len++;	 
	 }

	 len = sd_strlen(p_file_name);
        if(cur_len+len > MAX_FILE_PATH_LEN)
        {
               return FILE_PATH_TOO_LONG;
        }
	 sd_strncpy(full_file_path+cur_len, p_file_name, len);

        cur_len += len; 

        full_file_path[cur_len] = '\0';
		
	 if(sd_file_exist(full_file_path) ==  FALSE)
	 {
	        LOG_DEBUG( "up_create_file_struct  , full path file : %s not exsist.", full_file_path);
	 
	        return FILE_NOT_EXIST;
	 }

	 ret_val =  fm_create_file_struct( p_file_name, p_file_path, file_size, p_user, p_call_back, pp_file_struct,0 /* FWM_RANGE */);

        return ret_val;	 

}


_int32  up_file_asyn_read_buffer( struct tagTMP_FILE *p_file_struct, RANGE_DATA_BUFFER *p_data_buffer, notify_read_result p_call_back, void *p_user )
{
         _int32 ret_val = 0;
 
        if(p_file_struct == NULL || p_data_buffer == NULL || p_user == NULL)
        {
               LOG_DEBUG( "up_file_asyn_read_buffer  invalid parameter." );
	        return FILE_INVALID_PARA;
        }

	  LOG_DEBUG( "up_file_asyn_read_buffer  , p_file_struct:0x%x, p_data_buffer: 0x%x,  p_user:0x%x .", p_file_struct, p_data_buffer, p_user);

	ret_val = fm_file_asyn_read_buffer( p_file_struct, p_data_buffer, p_call_back, p_user);
  
	return ret_val; 	
}


_int32 up_file_close( struct tagTMP_FILE *p_file_struct, notify_file_close p_call_back, void *p_user )
{
         _int32 ret_val = 0;
 
        if(p_file_struct == NULL ||  p_user == NULL)
        {
               LOG_DEBUG( "up_file_close  invalid parameter." );
	        return FILE_INVALID_PARA;
        }

	  LOG_DEBUG( "up_file_close  , p_file_struct:0x%x,   p_user:0x%x .", p_file_struct,  p_user);

	ret_val = fm_close( p_file_struct, p_call_back, p_user );
  
	return ret_val; 	
}

