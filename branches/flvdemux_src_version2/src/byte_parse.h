/// @file      byte_parse.h
/// @brief     Analyze data from net data
/// @author    fangj@xinli.com.cn
/// @version   1.0
/// @date      2012-02-10

#ifndef BYTEOPERATION_H
#define BYTEOPERATION_H

#define TRUE     0
#define FALSE   -1

typedef char                I8, BOOL;
typedef short               I16;
typedef long                I32;
typedef long long           I64;
typedef unsigned char       UI8;
typedef unsigned short      UI16;
typedef unsigned long       UI32;
typedef unsigned long long  UI64;

/// @brief Check if local machine is little endian
BOOL is_little_endian ();
/// @brief Get an unsigned integer from buf
BOOL get_UI16 (UI8** buf, int* size, UI16* data);
BOOL get_UI24 (UI8** buf, int* size, UI32* data);
BOOL get_UI32 (UI8** buf, int* size, UI32* data);
BOOL get_UI64 (UI8** buf, int* size, UI64* data);

#endif // BYTEOPERATION_H
