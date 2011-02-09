#ifndef APEX_HTTP_H
#define APEX_HTTP_H

#include "apex_epoll.h"

typedef struct filedata
{
	char* buffer;
	ssize_t size;
	int error;
}filedata_t;

void apex_http_error(const int error_code, struct apex_epoll *, int fd );
void apex_http_parser(char* input, struct apex_epoll *,int fd );
void apex_http_response(char* name, struct apex_epoll *, int fd);

void apex_http_response_header(int fd,char* type,ssize_t size);
void apex_http_response_data(int fd,void* data,ssize_t size);
void apex_http_timer( struct apex_epoll* this );

int apex_http_readfile(char* name, char* buffer );
#endif //APEX_HTTP_H