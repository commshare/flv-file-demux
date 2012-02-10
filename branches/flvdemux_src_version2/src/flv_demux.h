#ifndef _FLV_DEMUX_H_
#define _FLV_DEMUX_H_

#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include <stdlib.h>
#include "byte_parse.h"
#include "../urlprotocol.h"
#include "../avformat.h"
#include "../demux.h"


#define AUDIO_STREAM_ID 8
#define VIDEO_STREAM_ID 9
#define OTHER_STREAM_ID 0

#define FLV_FILE_HEADER_SIZE 13U
#define FLV_TAGS_HEADER_SIZE 11U
#define FLV_TAGS_TAILER_SIZE  4U

/** AAC format identifiers. */
#define STBAACDETECT_SYNC_ADTS_1    0xFF
#define STBAACDETECT_SYNC_ADTS_2    0xF0
#define STBAACDETECT_START_ADIF_1   0x41
#define STBAACDETECT_START_ADIF_2   0x44
#define STBAACDETECT_START_ADIF_3   0x49
#define STBAACDETECT_START_ADIF_4   0x46

typedef struct FLVTagPacket
{
    FLVTagType  m_TagType;
    UI32        m_TagDataSize;
    UI32        m_TagTimestamp;
    UI32        m_TagBufferLen;
    UI8*        m_TagData;
}FLVTagPacket;
typedef struct FLVTSInfo
{
    UI64        m_Pos;
    UI32        m_TimeDelta;
}FLVTSInfo;
typedef struct TimestampInd
{
    FLVTSInfo*  m_Index;
    UI32        m_Count;
}TimestampInd;
typedef struct FLVDemuxInf
{
    UI64         m_CurrentPosition;
    FLVTagPacket m_CurrentPacket;
    TimestampInd m_TimestampIndex;
}FLVDemuxInf;



BOOL flv_demux_get_packet (FLVDemuxInf* dmx);

typedef enum
{
    FLV_DEMUX_NONE   = 0,
    FLV_DEMUX_HEADER = 1,
    FLV_DEMUX_MDATA  = 2,
    FLV_DEMUX_AUDIO  = 3,
    FLV_DEMUX_VIDEO  = 4
}FLVDemuxState;
typedef struct 
{
    long long ts;   ///< second
    long long pos;
}FLVIndex;
typedef struct 
{
    unsigned long count;
    FLVIndex*     elems;
}FLVIndexList;
typedef struct
{
    unsigned char* data;
    int            size;
    int            buflength;
}Input;
typedef struct PrereadTagNode
{
    AVPacket* pack;
    long long pos;
    struct PrereadTagNode* next;
}PrereadTagNode;
typedef struct  
{
    URLProtocol*  m_URLProtocol;
    Input         m_InputInfor;
    AVPacket*     m_AVPacket;
    Metadata*     m_Metadata;
    FLVIndexList  m_IndexList;

    long long     m_FileSize;
    long long     m_AVDataOffset;
    int           m_Duration;
    long long     m_CurrPos;
    long          m_TagDataSize;
    long          m_TagTimestamp;
    FLVDemuxState m_DemuxState;

    PrereadTagNode*   m_PrereadList;
}FLVDemuxInfo;
typedef struct
{
    unsigned char* data;
    long  numBit;
    long  size;
    long  currentBit;
    long  numByte;
}stbDemux_BitBuffer_t;


int can_handle (int fileformat);
int flv_demux_open (DemuxContext* ctx, URLProtocol* h);
int flv_demux_probe (DemuxContext* ctx);
int flv_demux_close (DemuxContext* ctx);
int flv_demux_parse_metadata (DemuxContext* ctx, Metadata* meta);
int flv_demux_read_packet (DemuxContext* ctx, AVPacket* pkt);
long long flv_demux_seek (DemuxContext* ctx, long long ts);
/*
 * meta - fill the field can get, others should be set to zero
 *
 * return 0 on success; -1 on error; >0 input data was not enough, the length required(from file begin)
 */
int flv_demux_parse_codec_from_raw_data(unsigned char * data, int size, Metadata* meta);
#endif/*_FLV_DEMUX_H_*/
