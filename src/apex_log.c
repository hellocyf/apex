#include <time.h>
#include <string.h>
#include <sys/time.h>

#include "apex_log.h"

void apex_log_print(int level, int flags, int line, const char *file, const char *function, const char *format, ...)
{
	va_list ap;

	if(level<1)
		return;
	
	if (flags)
		printf("%s:%d:", file, line);
	
	switch( level )
	{
		case 0:
			printf("info:");
			break;
		case 1:
			printf("trace:");
			break;
		case 2:
			printf("warning:");
			break;
		case 3:
			printf("error:");
			break;
		
	}
	
	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);
	
	time_t rawtime;
	struct tm * timeinfo;
	char* timestr;
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	timestr= asctime (timeinfo) ;
	
	timestr= strtok(timestr,"\n");
	printf ( "  @%s:%s\n", function,timestr);
	

}
