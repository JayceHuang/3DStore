#ifndef MEDIA_SERVER_COMMAND_H_
#define MEDIA_SERVER_COMMAND_H_

#include <sys/types.h>
#include "utils.h"

enum command_type
{
    PLAY_VOD_URL = 0, 
    GET_VOD_PLAY_INFO,
    GET_VOD_ERROR_INFO,
    STOP_DOWNLOAD, 
    START_DOWNLOAD, TYPE_UNKNOWN
};

typedef struct command_parameter{
	char url[MAX_URL_LEN];
	char gcid[CID_SIZE];
	char cid[CID_SIZE];
	long long file_size;
	int platform;
	char cookie[MAX_COOKIE_LEN];
	int query_hub;
       int quality;
}command_parameter;


typedef struct media_server_command
{
	enum command_type type;
	struct command_parameter param;
}media_server_command;


media_server_command* parse_media_server_command(char *command);

bool is_same_vod_file(command_parameter *para_a, command_parameter *para_b, int check_cookie);

void output_command_parameter_log(char* declare, command_parameter *para);

#endif /* MEDIA_SERVER_COMMAND_H_ */
