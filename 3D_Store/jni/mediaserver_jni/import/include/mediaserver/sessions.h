#ifndef SESSIONS_H_
#define SESSIONS_H_

#include <stddef.h>

/* session_cb return value */
#define SESSIONS_EACH_NONE    0
#define SESSIONS_EACH_REMOVE  1
#define SESSIONS_EACH_BREAK   2

struct session;
struct session_node;

typedef int (*session_cb)(struct session *s, void *data);

struct session_node *sessions_add(struct session_node *sessions, struct session *s);
struct session_node *sessions_remove(struct session_node *sessions, struct session *s);
struct session_node *sessions_remove_all(struct session_node *sessions);

struct session_node *sessions_each(struct session_node *sessions, session_cb cb, void *data);
int sessions_size(struct session_node *sessions);

#endif /* SESSIONS_H_ */
