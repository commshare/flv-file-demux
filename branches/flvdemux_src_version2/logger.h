#ifndef MP_MSG_H
#define MP_MSG_H

#define MSGL_FATAL   0  // will exit/abort
#define MSGL_ERR     1  // continues
#define MSGL_WARN    2  // only warning
#define MSGL_HINT    3  // short help message
#define MSGL_INFO    4  // -quiet
#define MSGL_STATUS  5  // v=0
#define MSGL_V       6  // v=1
#define MSGL_DBG2    7  // v=2
#define MSGL_DBG3    8  // v=3
#define MSGL_DBG4    9  // v=4
#define MSGL_DBG5    10 // v=5

#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>

void mp_msg(int mod, int lev, const char* format, ...);
#endif
