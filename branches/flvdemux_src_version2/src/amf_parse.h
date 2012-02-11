#ifndef AMF_PARSER_H
#define AMF_PARSER_H

#include "flv_demux.h"
#include "byte_parse.h"

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
    DATA_MARKER         = 0x0B, ///< This program will skip this AMF data
    LONG_STRING_MARKER  = 0x0C, ///< This program will skip this AMF data
    UNSUPPORTED_MARKER  = 0x0D, ///< This program does not supported this AMF data type
    RECORDSET_MARKER    = 0x0E, ///< reserved, AMF_0 protocol does not supported
    XML_DOCUMENT_MARKER = 0x0F, ///< This program does not supported this AMF data type
    TYPED_OBJECT_MARKER = 0x10  ///< This program does not supported this AMF data type
}AMFType;

BOOL amf_parse_elem_name    (UI8** buf, UI32 *size, UI8** data, UI16* lens);
BOOL amf_parse_skip_bytes   (UI8** buf, UI32* size, UI8   type);
BOOL amf_parse_number       (UI8** buf, UI32* size, UI64* data);
BOOL amf_parse_object       (UI8** buf, UI32* size, Metadata* data);
BOOL amf_parse_ecma_array   (UI8** buf, UI32* size, FLVDemuxer* dmx);
BOOL amf_parse_strict_array (UI8** buf, UI32* size, FLVDemuxer* dmx);

#endif/*AMF_PARSER_H*/
