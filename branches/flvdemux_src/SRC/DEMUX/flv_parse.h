#ifndef _FLV_PARSE_H_
#define _FLV_PARSE_H_

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include "flv_parse.h"

BOOL flv_parse_tag_header (FLVTagPacket* pkt, UI8* data, UI32 size);
BOOL flv_parse_tag_script (const FLVTagPacket* pkt, TimestampInd* index, Metadata* mdata);

#endif/*_FLV_PARSE_H_*/
