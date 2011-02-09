#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "apex_http.h"
#include "apex_log.h"


#define INPUT_BUFFER 8180
#define PARSE_BUFFER (INPUT_BUFFER + 12)
#define HEADER_BUFFER 1024

void apex_http_error(const int error_code ,struct apex_epoll* this,int fd )
{
	const char error_header[] =
	    "Server: apex\nContent-type: text/plain\n"
	    "Connection: keep-alive\n\n\n";
	const char not_found[] = "HTTP/1.1 404 Not found\n";
	const char internal_server_error[] =
	    "HTTP/1.1 500 Internal Server Error\n";
	const char not_implemented[] = "HTTP/1.1 501 Not implemented\n";
	size_t size=0;
	char* output=NULL;
	switch (error_code) {
	case 404:
		output = not_found;
		size= strlen(not_found);

		break;
	case 500:
		output = internal_server_error;
		size= strlen(internal_server_error);
		
		break;
	case 501:
		output = not_implemented;
		size= strlen(not_implemented);
	
		break;
	default:
		apex_log_error("unknown error_code: %d", error_code);
		return;
	}

	apex_log_trace(output);
	apex_http_response_data(fd, output, strlen(output));
	apex_http_response_header(fd, "text/plain",size);
	apex_http_response_data(fd, output, strlen(output));
	//apex_epoll_set( this, fd,  EPOLLOUT , output);

}

void apex_http_parser(char* input ,struct apex_epoll* this,int fd )
{
	int first_char, i;
	char file[PARSE_BUFFER] = { 0 };
	//apex_log_info( "input: \"%s\".\n", input);

	
	first_char = 4;
	if (input[0] == 'G' && input[1] == 'E' && input[2] == 'T') {
		while (input[first_char] == '/' || input[first_char] == '.'
		       || input[first_char] == ':') {
			first_char++;
			if (first_char == INPUT_BUFFER)
				return;
		}
		for (i = first_char; i < INPUT_BUFFER; i++) {
			if (input[i] == '.' && input[i + 1] == '.' &&
			    ((input[i + 2] == '/' || input[i + 2] == ' ')
			     || (input[i] == ':')))
				continue;
			if (input[i] != ' ' && input[i] != '\n'
			    && input[i] != '\r')
				file[i - first_char] = input[i];
			else
				break;
		}
	} else {
		/*
		int length = strlen(input);
		if( length ==0 )
		{
			apex_epoll_set( this, fd,  EPOLLOUT , "");
			return ;
		}
		*/
		
		apex_log_error("input %s(illegal request)\" \n",input);
		apex_epoll_del(this, fd);
		
		//apex_http_error(501, this, fd);
		return ;
	}
	
	if ( (strlen(file) == 0))
		strcpy(file, "index.html");
	
	apex_epoll_set( this, fd,  EPOLLOUT , file);
	apex_log_trace("request %s fd %d fdused %d", file, fd, this->fds_used);
	//apex_http_response( file, this, fd );
}


void apex_http_response_header(int fd,char* type,ssize_t size)
{
	char header_buffer[HEADER_BUFFER]={0};	
	sprintf(header_buffer,"HTTP/1.1 200 OK\nServer: apex\n"
					"Content-type:%sContent-length: %d\n"
					"Cache-Control: no-cache\n"
					"Connection: keep-alive\n\n", 
					type,size);
	send(fd, header_buffer, strlen(header_buffer), 0);	
}

void apex_http_response_data(int fd,void* data,ssize_t size)
{
	//apex_log_info("fd %d size %d %s\n",fd, size, (char*)data );
	send(fd, data, size, 0);
}

void apex_http_response_file (char* name, char* type, struct apex_epoll* this,int fd )
{
	FILE *fp = NULL;
	ssize_t size;
	char* buffer=NULL;
	
	fp = fopen(name, "r");
	if (fp == NULL) {
		apex_log_error("not found %s\n",name);
		apex_http_error(404, this, fd);
		return;
	}

	if (fseek(fp, 0, SEEK_END) == -1)
	{
		apex_log_error("fseek\n");
		return ;
	}

	size = ftell(fp);
	if (size == -1)
	{
		apex_log_error("ftell\n");
		return ;
	}
	rewind(fp);
	
	buffer = malloc(size);
	if (buffer == NULL)
	{
		apex_log_error("malloc\n");
		if (fclose(fp) == EOF)
			apex_log_error("fclose\n");
		fp = NULL;
		return apex_http_error(500, this, fd);
	}
	
	fread(buffer, size, 1, fp);
	if (ferror(fp) != 0)
	{
		apex_log_error("fread\n");
		clearerr(fp);
	}
	if (fclose(fp) == EOF)
		apex_log_error("fclose\n");	
	
	apex_http_response_header(fd, type, size);
	apex_http_response_data(fd, buffer, size);
	
	if(buffer)
		free(buffer);	
	
	apex_log_trace("send file fd %d size %d %s",fd, size, name );	
}

void apex_http_response(char* name, struct apex_epoll* this,int fd )
{
	char* type={0};
	ssize_t size = 0;

	if (strstr(name, ".html"))
		type = "text/html\n";
	else if (strstr(name, ".jpeg"))
		type = "image/jpeg\n";
	else if (strstr(name, ".jpg"))
		type = "image/jpg\n";
	else if (strstr(name, ".png"))
		type = "image/png\n";
	else if (strstr(name, ".gif"))
		type = "image/gif\n";	
	else if (strstr(name, ".ico"))
		type = "image/ico\n";
	else if (strstr(name, ".js"))
		type = "text/javascript\n";
	else if (strstr(name, ".css"))
		type = "text/css\n";
	else
	{
		type = "text/plain\n";
		//size = strlen(name);	
		//apex_http_response_header(fd, type, size);
		//apex_http_response_data(fd, name, size);
	}
	return  apex_http_response_file(name, type, this, fd);
}

void apex_http_timer( struct apex_epoll* this)
{
	return;
	int length = 0;
	time_t rawtime;
	struct tm * timeinfo;
	char* timestr;
	char data[128]={0};
	
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	timestr= asctime (timeinfo) ;
	sprintf(data,"<html>\n<head>\n</head>\n<body>\n%s</body>\n</html>\n",timestr);
	int i;
	int fd;
	
	for(i =1; i<50; i++)
	{
		fd = this->apex_clients[i].fd;
		
		//apex_log_info("i %d fd %d \n", i, fd );
		if(fd >0&&fd!=this->listener_fd)
		{
			apex_epoll_set( this, fd,  EPOLLOUT , data);
			//apex_http_response_data( fd , timestr, length);		
		}
	}
	
}
