#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <errno.h>
#include <unistd.h>
#include <signal.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "apex_epoll.h"
#include "apex_event.h"
#include "apex_log.h"
#include "apex_http.h"

#define TEST_BUF_SIZE	1024
#define BYTES_EXPECTED	65536

int fd;
int g_bytes_read=0;
int verbose = 1;

static volatile sig_atomic_t apex_shutdown = 0;
static volatile sig_atomic_t graceful_shutdown = 0;
apex_epoll_t apex_epoll;

int apex_callback(struct epoll_event *event, void* data)
{
	char buf[TEST_BUF_SIZE];
	memset(buf,0,TEST_BUF_SIZE);
	int n,fd;
	fd = event->data.fd;
	apex_log_info("fd %d Event: %d\n", fd, event->events);
	if (event->events & EPOLLIN) {

		//apex_log_info("fd %d Event:EPOLLIN %d\n", fd, event->events);
		int n_read = 0;
		do {
			n = read(fd, buf, TEST_BUF_SIZE);
			if (n > 0)
				n_read += n;
		} while (n == TEST_BUF_SIZE);
		g_bytes_read += n_read;
		apex_log_info("%d bytes read in this event\n", n_read);

		if (n == -1) {
			apex_log_info(", last read failed errno %d %s \n", errno,strerror(errno));
			if (errno == EWOULDBLOCK) {
				apex_log_info("= EWOULDBLOCK");
				if (n_read == 0)
					apex_log_info(", spurious wakeup (ok)\n");
			}	
		}
		else
		{
			apex_http_parser(buf, &apex_epoll, fd);
		}
	} else {
		switch (event->events) {
			case EPOLLOUT: 
				{
					//apex_log_info("fd %d Event:EPOLLOUT %d\n", fd, event->events);
					if(data)
					{						
						//send( fd,data,strlen(data),0);
						//apex_log_info("Event:EPOLLOUT %s\n", data);
						apex_http_response((char*)data, &apex_epoll, fd);
					}
					apex_epoll_set( &apex_epoll, fd, EPOLLIN, NULL);
				}
				break;

			case EPOLLERR:
				apex_log_info("fd %d Event:EPOLLERR %d\n", fd, event->events);
				break;
			case EPOLLHUP:
				apex_log_info("fd %d Event:EPOLLHUP %d\n", fd, event->events);
				apex_epoll_del( &apex_epoll, fd);
				break;
			default:
				apex_log_info("fd %d Event: %d\n", fd, event->events);
				break;
		}
	}
	
	return 0;
}

int apex_accept(struct epoll_event *event,void* data)
{
	apex_log_info("new accept\n");
	
	int new_fd =-1;
	struct sockaddr_in caddr;
	socklen_t len = sizeof(struct sockaddr_in);
	
	new_fd = accept(event->data.fd, (struct sockaddr *) &caddr,&len);
	if (new_fd < 0) 
	{
		apex_log_error(" accept fd %d new_fd %d\n",event->data.fd, new_fd);
	}
	else
	{
		apex_log_info("connect from: %s:%d, assign socket is:%d\n", inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port), new_fd);
                apex_epoll_prepare_fd( &apex_epoll, new_fd );		 
                apex_epoll_add( &apex_epoll, new_fd, apex_callback, NULL);
	}
	return 0;
}

int apex_server_init(apex_epoll_t* epoll, short port)
{
    int fd,opt=1;
    struct sockaddr_in sin;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) <= 0)
    {
          apex_log_error("socket failed\n");
          return -1;
    }
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void*)&opt, sizeof(opt));
    
    apex_epoll_prepare_fd(epoll, fd);
    
    apex_epoll_add( epoll, fd, apex_accept, NULL);
    epoll->listener_fd=fd;
    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons((short)(port));
    sin.sin_addr.s_addr = INADDR_ANY;
    if (bind(fd, (struct sockaddr *)&sin, sizeof(sin)) != 0)
    {
          apex_log_error( "bind port %d failed error code = %d: %s \n", port, errno, strerror(errno));
          return -1;
    }
    if (listen(fd, 32) != 0)
    {
        apex_log_error( "listen failed %d %s \n",errno, strerror(errno));
          return -1;
    }
    
    return 0;
}

void signal_handler(const int sig)
{
	
	switch (sig) {
	case SIGTERM:
		apex_shutdown = 1;
		apex_log_info("signal_handler %d shutdown =%d \n", sig,apex_shutdown);
		break;
	case SIGINT:
		if (graceful_shutdown) {
			apex_shutdown = 1;
		} else {
			graceful_shutdown = 1;
		}
		apex_shutdown = 1;
		apex_log_info("signal_handler %d shutdown =%d \n", sig,apex_shutdown);
		break;
	}
}


int main()
{
	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);
	
	memset(&apex_epoll,0,sizeof(apex_epoll_t));
	if(apex_epoll_init(&apex_epoll,10000))
	{
		apex_log_error("apex_epoll_init failed %d %s\n",errno, strerror(errno));
		return 1;
	}
	
	if(apex_server_init(&apex_epoll,8080))
	{
		apex_log_error("apex_server_init  port =  %d \n",8080);
		return 2;
	}
	double dif=0.0;
	time_t start,end;
	start =time(NULL);
	end = time(NULL);
	dif = difftime (end,start);
	
	while(!apex_shutdown)
	{
		//time(now);
		apex_epoll_wait_dispatch_events(&apex_epoll,1000);

		end = time(NULL);
		dif = difftime(end,start);

		if(dif>=1.0)
		{	
			apex_http_timer(  &apex_epoll );
			start = time(NULL);
			dif= 0.0;
		}
	}
	
	apex_epoll_shutdown(&apex_epoll);
	
	return 0;
}
