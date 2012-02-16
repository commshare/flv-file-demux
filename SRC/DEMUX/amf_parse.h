/// @file    amf_parse.h
/// @brief   Parse AMF0 raw data
/// @author  fangj@xinli.com.cn
/// @version 1.0
/// @date    2012-02-10

#ifndef AMF_PARSER_H
#define AMF_PARSER_H

#include <stddef.h>
#include "../format.h"
#include "flv_demux.h"

#define TRUE   0
#define FALSE -1

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

typedef enum AMFType
{
    NUMBER_MARKER       = 0x00,
    BOOLEAN_MARKER      = 0x01, ///< This program will skip this AMF data
    STRING_MARKER       = 0x02, ///< This program will skip this AMF data
    OBJECT_MARKER       = 0x03,
    MOVIECLIP_MARKER    = 0x04, ///< reserved, AMF_0 protocol does not supported
    NULL_MARKER         = 0x05, ///< This program will skip this AMF data
    UNDEFINED_MARKER    = 0x06, ///< This program will skip this AMF data
    REFERENCE_MARKER    = 0X07, ///< This program does not supported this AMF data type
    ECMA_ARRAY_MARKER   = 0x08,
    OBJECT_END_MARKER   = 0x09, ///< This program will skip this AMF data
    STRICT_ARRAY_MARKER = 0x0A,
    DATE_MARKER         = 0x0B, ///< This program will skip this AMF data
    LONG_STRING_MARKER  = 0x0C, ///< This program will skip this AMF data
    UNSUPPORTED_MARKER  = 0x0D, ///< This program does not supported this AMF data type
    RECORDSET_MARKER    = 0x0E, ///< reserved, AMF_0 protocol does not supported
    XML_DOCUMENT_MARKER = 0x0F, ///< This program does not supported this AMF data type
    TYPED_OBJECT_MARKER = 0x10  ///< This program does not supported this AMF data type
}AMFType;


/// @brief Check if local machine is little endian
BOOL is_little_endian ();
/// @brief Get an unsigned integer from buf
BOOL get_Byte (UI8** buf, UI32 *size, UI8 * data);
BOOL get_UI16 (UI8** buf, UI32 *size, UI16* data);
BOOL get_UI24 (UI8** buf, UI32 *size, UI32* data);
BOOL get_UI32 (UI8** buf, UI32 *size, UI32* data);
BOOL get_UI64 (UI8** buf, UI32* size, UI64* data);

BOOL amf_parse_elem_name    (UI8** buf, UI32* size, UI8** data, UI16* lens);
BOOL amf_parse_number       (UI8** buf, UI32* size, UI64* data);
BOOL amf_parse_object       (UI8** buf, UI32* size, TimestampInd* index, Metadata* mdata);
BOOL amf_parse_ecma_array   (UI8** buf, UI32* size, TimestampInd* index, Metadata* mdata);
BOOL amf_parse_strict_array (UI8** buf, UI32* size, TimestampInd* index, UI8* flag);

#endif/*AMF_PARSER_H*/
