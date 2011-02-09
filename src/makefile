APEX        = apex
INCLUDE  = ./

CFLAGS		    += -g -Wall -I${INCLUDE}
LDFLAGS		    +=
LDLIBS		    +=


all: ${APEX}

apex: apex_epoll.o apex.o apex_log.o apex_http.o
	gcc ${CFLAGS} $^ -o $@

apex.o: apex.h apex_log.h
apex_epoll.o: apex_epoll.h apex_event.h apex_log.h
apex_log.o:apex_log.h
apex_http.o:apex_http.h apex_log.h

clean:
	-rm apex *.o
