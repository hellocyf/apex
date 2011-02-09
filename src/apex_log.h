#ifndef APEX_LOG_H
#define APEX_LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void apex_log_print(int level, int flags, int line, const char *file, const char *function, const char *format, ...);
#define apex_log_info(...)      do { apex_log_print(0, 1, __LINE__, __FILE__,__FUNCTION__, __VA_ARGS__); } while (0)
#define apex_log_trace(...)      do { apex_log_print(1, 1, __LINE__, __FILE__,__FUNCTION__, __VA_ARGS__); } while (0)
#define apex_log_warning(...)      do { apex_log_print(2, 1,__LINE__,__FILE__,__FUNCTION__, __VA_ARGS__); } while (0)
#define apex_log_error(...)      do { apex_log_print(3, 1, __LINE__, __FILE__,__FUNCTION__, __VA_ARGS__); } while (0)


#endif //APEX_LOG_H