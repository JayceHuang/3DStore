#ifndef SESSION_H_
#define SESSION_H_

#include "ev.h"
#include "utils.h"

struct ev_loop;
struct media_server_local_info;
struct session;


typedef void (*session_accept_cb)(struct session *s);


typedef struct session
{
	ev_io io;
	struct ev_loop *loop;

	char      *root;
	int       active;
	int       released;
	session_accept_cb limit_accept_play_vod_cb;

	struct io_buffer    *buf;
	struct vod_session  *vod_s;
	struct media_server_local_info	*info;	   
} session;


struct session *session_new(struct ev_loop *loop, int fd, const char *root, session_accept_cb cb, struct media_server_local_info *info);

void session_stop(struct session *s);
void session_destroy(struct session *s);
int session_active(struct session *s);
int session_is_play_vod_url(struct session *s);

#endif /* SESSION_H_ */
