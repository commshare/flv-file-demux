#include <string.h>
#include "../mp_msg.h"
#include "../avformat.h"
#include "amf_parse.h"
#include "flv_parse.h"
#include "byte_parse.h"

int flv_parse_file_header (FLVDemuxInfo* dmx)
{
    unsigned char* data = dmx->m_InputInfor.data;

    if (data[0] == 'F' && data[1] == 'L' && data[2] == 'V')
    {
        return 0;
    }
    return -1;
}
int flv_parse_tag_header (FLVDemuxInfo* dmx)
{
    unsigned char* data = dmx->m_InputInfor.data;
    int size = dmx->m_InputInfor.size;

    int tag_type = (int)data[0];
    switch((FLVTagType)tag_type)
    {
    case FLV_AUDIO_TAG:
        dmx->m_DemuxState = FLV_DEMUX_AUDIO;
        break;
    case FLV_VIDEO_TAG:
        dmx->m_DemuxState = FLV_DEMUX_VIDEO;
        break;
    case FLV_SCIRPT_TAG:
        dmx->m_DemuxState = FLV_DEMUX_MDATA;
        break;
    default:
        dmx->m_TagDataSize = 0;
        dmx->m_DemuxState = FLV_DEMUX_NONE;
        return -1;
    }

    data += 1;
    size -= 1;
    dmx->m_TagDataSize = is_little_endian() ? (long)get_le24_net(data, size)\
        : (long)get_be24_net(data, size);
    data += 3;
    size -= 3;
    dmx->m_TagTimestamp = is_little_endian() ? (long)get_le24_net(data, size)\
        : (long)get_be24_net(data, size);
    data += 3;
    size -= 3;
    dmx->m_TagTimestamp |= (get_byte_net(data, size)) << 24;
    return 0;
}
int flv_parse_tag_script (FLVDemuxInfo* dmx)
{
    unsigned char* data = dmx->m_InputInfor.data;
    int size = dmx->m_InputInfor.size;
    Metadata* meta = dmx->m_Metadata;

    meta->streams = 2;
    meta->audiostreamindex = 0x08;
    meta->videostreamindex = 0x09;
    meta->fileformat = FILEFORMAT_ID_FLV;

    /* the 1st element type must be 0x02, and string is "onMetaData" */
    if (data[0] != 0x02)
    {
        return -1;
    }
    if (10 != get_ui16(&data[1], size - 1))
    {
        return -1;
    }
    if (memcmp(&data[3], "onMetaData", 10) != 0)
    {
        return -1;
    }
    data += 13;
    size -= 13;

    if (data[0] == 0x08)
    {
        return amf_parse_ecma_array(dmx, &data, &size);
    }
    if (data[0] == 0x03)
    {
        return amf_parse_object(dmx, &data, &size);
    }

    return -1;
}
int flv_parse_tag_audio (FLVDemuxInfo* dmx)
{
    AVPacket* pkt = dmx->m_AVPacket;
    
    pkt->pts  = dmx->m_TagTimestamp;
    pkt->size = dmx->m_TagDataSize;
    pkt->stream_index = 0x08;
    if (pkt->bufferlength < pkt->size)
    {
        if (pkt->data != NULL)
        {
            free(pkt->data);
            mp_msg(0, MSGL_V, "Free Packet Data %p LENS = %d\n", pkt->data, pkt->bufferlength);
            pkt->data = NULL;
        }
        pkt->data = (unsigned char *)malloc(pkt->size);
        mp_msg(0, MSGL_V, "Allocate Packet Data %p LENS = %d\n", pkt->data, pkt->size);
        if (pkt->data == NULL)
        {
            return -1;
        }
        pkt->bufferlength = pkt->size;
    }
    memcpy(pkt->data, dmx->m_InputInfor.data, pkt->size);
    return 0;
}
int flv_parse_tag_video (FLVDemuxInfo* dmx)
{
    AVPacket* pkt = dmx->m_AVPacket;

    pkt->pts  = dmx->m_TagTimestamp;
    pkt->size = dmx->m_TagDataSize;
    pkt->stream_index = 0x09;
    if (pkt->bufferlength < pkt->size)
    {
        if (pkt->data != NULL)
        {
            free(pkt->data);
            mp_msg(0, MSGL_V, "Free Packet Data %p LENS = %d\n", pkt->data, pkt->bufferlength);
            pkt->data = NULL;
        }
        pkt->data = (unsigned char *)malloc(pkt->size);
        mp_msg(0, MSGL_V, "Allocate Packet Data %p LENS = %d\n", pkt->data, pkt->size);
        if (pkt->data == NULL)
        {
            return -1;
        }
        pkt->bufferlength = pkt->size;
    }
    memcpy(pkt->data, dmx->m_InputInfor.data, pkt->size);
    return 0;
}
