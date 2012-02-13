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


#define FLV_FILE_HEADER_SIZE 13U
#define FLV_TAGS_HEADER_SIZE 11U
#define FLV_TAGS_TAILER_SIZE  4U

/// @brief FLV tag packet
typedef struct FLVTagPacket
{
    FLVTagType          m_TagType;          ///< Tag type
    UI64                m_TagPosition;      ///< Tag start position
    UI32                m_TagDataSize;      ///< Tag data size
    UI32                m_TagTimestamp;     ///< Tag timestamp
    UI32                m_TagBufferLen;     ///< Tag buffer length
    UI8*                m_TagData;          ///< Tag data
}FLVTagPacket;
/// @brief FLV Timestamp index info
typedef struct FLVTSInfo
{
    UI64                m_FilePos;          ///< Key frame file position
    UI64                m_TimePos;          ///< Key frame time position
}FLVTSInfo;
/// @brief FLV Timestam indexes
typedef struct TimestampInd
{
    FLVTSInfo*          m_Index;            ///< Index List
    UI32                m_Count;            ///< Index Count
}TimestampInd;
/// @brief Pre-read tags list node
typedef struct PrereadTags
{
    FLVTagPacket*       m_Packet;
    struct PrereadTags* m_Next;
}PrereadTags;
/// @brief FLV demux info
typedef struct FLVDemuxer
{
    UI32                m_AudioBitRate;     ///< Audio bitrate
    UI32                m_VideoBitRate;     ///< Video bitrate
    UI64                m_FileDuration;     ///< File Duration
    UI64                m_CurrentPosition;  ///< Current Demux Position
    FLVTagPacket        m_CurrentPacket;    ///< Current Tag Packet
    PrereadTags         m_PrereadTagList;   ///< Pre-read tag list
    TimestampInd        m_TimestampIndex;   ///< Timestamp Index
    URLProtocol*        m_URLProtocol;      ///< URL Protocol for getting data
}FLVDemuxer;


/// @brief  Open FLV demux
int       flv_demux_open (DemuxContext* ctx, URLProtocol* h);
/// @brief  Probe FLV file
int       flv_demux_probe (DemuxContext* ctx);
/// @brief  Close FLV demux
int       flv_demux_close (DemuxContext* ctx);
/// @brief  Parse metadata
int       flv_demux_parse_metadata (DemuxContext* ctx, Metadata* meta);
/// @brief  Read a audio or video packet
int       flv_demux_read_packet (DemuxContext* ctx, AVPacket* pkt);
/// @brief  Seek according to assigned timestamp
long long flv_demux_seek (DemuxContext* ctx, long long ts);
/// @brief  Meta - fill the field can get, others should be set to zero
/// @return return 0 on success; -1 on error; >0 input data was not enough, the length all required
int       flv_demux_parse_codec_from_raw_data(unsigned char * data, int size, Metadata* meta);


#endif/*_FLV_DEMUX_H_*/
