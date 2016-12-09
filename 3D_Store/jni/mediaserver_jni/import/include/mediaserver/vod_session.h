#ifndef VOD_SESSION_H_
#define VOD_SESSION_H_

#include "ev.h"

#include <stddef.h>
#include "utils.h"
#include "media_server_command.h"

typedef struct vod_session
{
  ev_io socket_r;
  ev_io socket_w;
  struct ev_loop *loop;

  ev_timer get_file_size_timeout_timer;
  
  unsigned int task_id;
  bool task_have_data;
  uint64 downloaded_data_pos;
  int local;
  struct io_buffer *buf;
  uint64 offset;
  uint64 deliver_to_player_size;
  
  int active;
} vod_session;


struct ev_loop;
struct vod_session;

struct vod_session *create_vod_session(struct ev_loop *loop, int socket_fd, uint64 offset, uint64 size, struct command_parameter *comand_para);

struct vod_session *vod_session_new(struct ev_loop *loop, int socket_fd, int task_id, uint64 offset, uint64 size, int local);

void vod_session_stop(struct ev_loop *loop, struct vod_session *s, bool set_socket_linger);
void vod_session_destroy(struct vod_session *s);
int vod_session_active(struct vod_session *s);

//保存当前任务task_id
typedef struct VOD_TASK_INFO_CACHE {
	unsigned int current_task_id;
	command_parameter comand_para;
}VOD_TASK_INFO_CACHE;


#endif /* VOD_SESSION_H_ */
