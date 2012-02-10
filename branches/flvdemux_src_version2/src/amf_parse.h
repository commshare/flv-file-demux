#ifndef AMF_PARSER_H
#define AMF_PARSER_H

#include "flv_demux.h"
#include "byte_parse.h"

BOOL amf_parse_elem_name    (FLVDemuxInf* dmx);
BOOL amf_parse_number       (FLVDemuxInf* dmx);
BOOL amf_parse_boolean      (FLVDemuxInf* dmx);
BOOL amf_parse_string       (FLVDemuxInf* dmx);
BOOL amf_parse_long_string  (FLVDemuxInf* dmx);
BOOL amf_parse_object       (FLVDemuxInf* dmx);
BOOL amf_parse_ecma_array   (FLVDemuxInf* dmx);
BOOL amf_parse_strict_array (FLVDemuxInf* dmx);

#endif/*AMF_PARSER_H*/
