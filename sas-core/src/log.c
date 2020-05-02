#include <stdio.h>
#include <stdarg.h>

#include <sas/log.h>

#define LOG_STREAM stdout

void sas_log(const char *restrict msg, ...)
{
    va_list arg;
    va_start(arg, msg);
    vfprintf(LOG_STREAM, msg, arg);
    va_end(arg);
}
