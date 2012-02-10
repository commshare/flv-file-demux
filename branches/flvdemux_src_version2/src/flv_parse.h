#ifndef _FLV_PARSE_H_
#define _FLV_PARSE_H_

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include "flv_demux.h"

typedef enum
{
    FLV_OTHER_TAG   = -1,
    FLV_AUDIO_TAG   = 0x08,
    FLV_VIDEO_TAG   = 0x09,
    FLV_SCIRPT_TAG  = 0x12
}FLVTagType;

int flv_parse_file_header (FLVDemuxInfo* dmx);
int flv_parse_tag_header (FLVDemuxInfo* dmx);
int flv_parse_tag_script (FLVDemuxInfo* dmx);
int flv_parse_tag_audio (FLVDemuxInfo* dmx);
int flv_parse_tag_video (FLVDemuxInfo* dmx);

#endif/*_FLV_PARSE_H_*/