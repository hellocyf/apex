#ifndef APEX_EPOLL_H
#define APEX_EPOLL_H

#include "apex_event.h"

typedef struct apex_epoll {
	int maxfds;
	int epoll_fd;
	int listener_fd;
	apex_client_t* apex_clients;	
	struct epoll_event *epoll_events;
	int fds_used;
}apex_epoll_t;

int apex_epoll_init(struct apex_epoll *, int maxfds);
void apex_epoll_shutdown(struct apex_epoll *);
int apex_epoll_prepare_fd(struct apex_epoll *,int fd);
int apex_epoll_add(struct apex_epoll *, int fd, apex_callback_fn_t pfn, void *data);
int apex_epoll_set(struct apex_epoll *this, int fd, int eventcode, void* data);
int apex_epoll_del(struct apex_epoll *, int fd);

int apex_epoll_broadcast_message(struct apex_epoll *, void* data);
int apex_epoll_send_message(struct apex_epoll *, int fd, void* data);

int apex_epoll_wait_dispatch_events(struct apex_epoll *, int timeout_millisec);

#endif
