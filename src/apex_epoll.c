#include <assert.h>
#include <stdlib.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include "apex_epoll.h"
#include "apex_log.h"

int apex_epoll_init(struct apex_epoll *this, int maxfds)
{
	int i;
	
	apex_log_info("init \n");
	
	this->maxfds = maxfds;
	this->fds_used = 0;
	
	this->apex_clients = (apex_client_t *)malloc(sizeof(apex_client_t) * maxfds);
	if (!this->apex_clients)
		return ENOMEM;	
	for (i=0; i<maxfds; i++) {
		this->apex_clients[i].pfn = NULL;
		this->apex_clients[i].data = NULL;
	}

	this->epoll_fd = epoll_create(maxfds);

	if (this->epoll_fd == -1)
		return errno;

	this->epoll_events = malloc(maxfds * sizeof(*this->epoll_events));
	if (!this->epoll_events)
		return ENOMEM;

	return 0;
}

void apex_epoll_shutdown( struct apex_epoll *this )
{
	apex_log_info("maxfds: %d fd used: %d \n", this->maxfds, this->fds_used);	
	free(this->apex_clients);
	close(this->epoll_fd);
	this->epoll_fd = -1;
	free(this->epoll_events);
}

int apex_epoll_prepare_fd(struct apex_epoll *this, int fd)
{
	apex_log_info("prepare fd %d \n",fd);
	int flags = O_RDWR | O_NONBLOCK;
	
	if (fcntl(fd, F_SETFL, flags) < 0) {
		apex_log_error("fcntl(fd %d, F_SETFL, 0x%x) returns err %d\n",fd, flags, errno);
		return errno;
	}
	return 0;
}

int apex_epoll_add(struct apex_epoll *this,int fd, apex_callback_fn_t pfn, void *data)
{
	struct epoll_event event;

	if (fd >= this->maxfds) {
		apex_log_error("add: epoll_ctl(fd %d...) returns err %d\n",fd, -1);
		return -1;
	}

	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP ;
	if (epoll_ctl(this->epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1 ) {
		apex_log_error("add: epoll_ctl(fd %d...) returns err %d\n", fd, errno);
		return errno;
	}

	this->fds_used++;
	this->apex_clients[fd].pfn = pfn;
	this->apex_clients[fd].data = data;
	this->apex_clients[fd].fd = fd;
	
	apex_log_info("add(%d, %p, %p) this->fds_used %d\n", fd, pfn, data, this->fds_used);

	return 0;
}

int apex_epoll_set(struct apex_epoll *this, int fd, int eventcode, void* data)
{
	struct epoll_event event;

	if (fd >= this->maxfds) {
		apex_log_error("add: epoll_ctl(fd %d...) returns err %d\n",fd, -1);
		return -1;
	}
	//event = &this->epoll_events[fd];
	event.data.fd = fd;
	event.events = 0;
	event.events = eventcode | EPOLLERR | EPOLLHUP;
	if (epoll_ctl(this->epoll_fd, EPOLL_CTL_MOD, fd, &event) == -1 ) {
		apex_log_error("add: epoll_ctl(fd %d...) returns err %d\n", fd, errno);
		return errno;
	}

	this->apex_clients[fd].data = data;
	
	if(data)
		apex_log_info("set(%d, %d, %d,%s) this->fds_used %d\n", fd, eventcode,event.events, (char*)data, this->fds_used);
	else
		apex_log_info("set(%d, %d, %d, %p) this->fds_used %d\n", fd, eventcode, event.events,data, this->fds_used);


	return 0;
}

int apex_epoll_del(struct apex_epoll *this, int fd)
{
	struct epoll_event efd;
	
	/* Sanity checks */
	if ((fd < 0) || (fd >= this->maxfds) || (this->fds_used == 0)) {
		apex_log_warning("del(fd %d): fd out of range\n", fd);
		return EINVAL;
	}

	efd.data.fd = fd;
	efd.events = -1;
	if (epoll_ctl(this->epoll_fd, EPOLL_CTL_DEL, fd, &efd) == -1 ) {
		apex_log_error("del: epoll_ctl(fd %d, ...) returns err %d\n", fd, errno);
		return errno;
	}
	this->fds_used--;
	this->apex_clients[fd].pfn = NULL;
	this->apex_clients[fd].data = NULL;
	this->apex_clients[fd].fd = 0;
	apex_log_info("del(fd %d) fduser %d\n", fd, this->fds_used);
	return 0;
}

int apex_epoll_broadcast_message(struct apex_epoll* this, void* data)
{
	int i;
	for(i =0; i<this->fds_used; i++)
	{
		apex_epoll_send_message(this,this->epoll_events[i].data.fd, data);
	}
	return 0;
}

int apex_epoll_send_message(struct apex_epoll* this, int fd, void* data)
{
	apex_epoll_set(this, fd, EPOLLOUT, data);
	return 0 ;
}

int apex_epoll_wait_dispatch_events(struct apex_epoll *this, int timeout_millisec)
{
	struct epoll_event* epoll_event=NULL;
	apex_client_t* apex_client= NULL;
	int nfds;

	nfds = epoll_wait(this->epoll_fd, this->epoll_events, this->maxfds, timeout_millisec);
	//apex_log_info("epoll returns %d fds. errno: %d\n", nfds,errno);
	
	for (; nfds > 0; nfds--, this->epoll_events++) {
		epoll_event = this->epoll_events;		
		int fd = epoll_event->data.fd;

		if ((fd < 0) || (this->fds_used == 0)) {
			apex_log_warning("nfds %d  ignoring event on fd %d. used %d\n", 
				nfds ,fd,  this->fds_used);
			/* silently ignore.  Might be stale (aren't time skews fun?) */
			continue;
		}
		apex_client = &this->apex_clients[fd];
		//apex_log_info("fd %d, events %x\n", epoll_event->data.fd, epoll_event->events);
		apex_client->pfn(epoll_event, apex_client->data);
	}
	return 0;
}
