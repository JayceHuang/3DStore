#ifndef SESSION_RETURN_INFO_H_
#define SESSION_RETURN_INFO_H_



#define HTTP_RET_ERROR_NO_FOUND "{\"ret\": 1, \"msg\": \"task no found\"}"
#define HTTP_RET_ERROR_STATE_ERROR "{\"ret\": 2, \"msg\": \"state error\"}"


#define JUST_RETURN_SUCCESS "{\"ret\": 0, \"msg\": \"OK\"}"


#define GET_VOD_PLAY_INFO_SUCCESS "{\"ret\": 0, \"msg\": \"OK\", \"resp\":" \
	"{ \"filesize\": %lld, \"downloadbytes\": %lld,\"speed\": %u }}"

#define GET_VOD_ERROR_INFO_SUCCESS "{\"ret\": 0, \"msg\": \"OK\", \"resp\":" \
	"{ \"vod_resp_code\": %d}}"
	
#endif /* SESSION_RETURN_INFO_H_ */
