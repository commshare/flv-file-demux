#include "logger.h"

void mp_msg(int mod, int lev, const char* format, ...)
{
    va_list va;
    char    tmp[256];

    mod = 0;
    lev = 0;

    va_start(va, format);
    vsnprintf(tmp, 256, format, va);
    va_end(va);

    tmp[256 - 1] = 0;

    printf("%s", tmp);
    fflush(stdout);
}

