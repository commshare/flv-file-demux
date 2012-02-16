#ifndef FILE_H
#define FILE_H

#include <stdio.h>
#include <stddef.h>
#include "../datIO.h"

#ifndef _TYPE_DEFINED_
#define _TYPE_DEFINED_
typedef char                I8, BOOL;
typedef short               I16;
typedef long                I32;
typedef long long           I64;
typedef unsigned char       UI8;
typedef unsigned short      UI16;
typedef unsigned long       UI32;
typedef unsigned long long  UI64;
#endif

typedef struct FilePrivData
{
    FILE* fp;
    UI64  filesize;
    UI64  currpos;
}FilePrivData;


URLProtocol* CreateURLProtocol ();

int url_open (URLProtocol *h, const char* path, int flags, void* quit);
int url_close(URLProtocol* h);
int url_read (URLProtocol *h, unsigned char *buf, int size);
I64 url_seek (URLProtocol *h, long long pos, int whence);
int url_is_live (URLProtocol* h);
#endif // FILE_H
