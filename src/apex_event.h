#ifndef APEX_EVENT_H
#define APEX_EVENT_H

#include <sys/epoll.h>

struct apex_event_s;

typedef int (*apex_callback_fn_t)(struct epoll_event* event,void* data);

struct apex_client_s {
	apex_callback_fn_t pfn;
	void *data;
	int fd;
}; 

typedef struct apex_client_s apex_client_t;
#endif
