#include <string.h>
#include "../mp_msg.h"
#include "../avformat.h"
#include "amf_parse.h"
#include "flv_parse.h"
#include "byte_parse.h"

BOOL flv_parse_tag_header (FLVTagPacket* pkt, UI8* data, UI32 size)
{
    UI8  amf_tag_type;
    UI8  tag_extend_timestamp;

    if (pkt == NULL)
    {
        return FALSE;
    }

    if (get_Byte (&data, &size, &amf_tag_type) == FALSE)
    {
        return FALSE;
    }

    switch((FLVTagType)amf_tag_type)
    {
    case AUDIO_FLV_STREAM_ID:
    case VIDEO_FLV_STREAM_ID:
    case MDATA_FLV_STREAM_ID:
        pkt->m_TagType = (FLVTagType)amf_tag_type;
        break;
    default:
        pkt->m_TagType = FLV_ERROR_TAG_TYPE;
        return -1;
    }


    if (get_UI24 (&data, &size, &pkt->m_TagDataSize) == FALSE)
    {
        return FALSE;
    }
    if (get_UI24 (&data, &size, &pkt->m_TagTimestamp) == FALSE)
    {
        return FALSE;
    }
    if (get_Byte (&data, &size, &tag_extend_timestamp) == FALSE)
    {
        return FALSE;
    }
    pkt->m_TagTimestamp &= 0x00FFFFFF;
    pkt->m_TagTimestamp |= (tag_extend_timestamp << 24);

    return TRUE;
}
BOOL flv_parse_tag_script (const FLVTagPacket* pkt, TimestampInd* index, Metadata* mdata)
{
    UI8* data = NULL;
    UI32 size = 0UL;
    UI8  flag = 0U;

    if ((pkt == NULL) || ((data = pkt->m_TagData) == NULL) || (size = pkt->m_TagDataSize) < 14UL)
    {
        return FALSE;
    }
    
    flag = data[13];
    mdata->streams = 2;
    mdata->audiostreamindex = 0x08;
    mdata->videostreamindex = 0x09;
    mdata->fileformat = FILEFORMAT_ID_FLV;

    if (data[0] != 0x02 || data[1] != 0x00 || data[2] != 0x0A\
        || memcmp(&data[3], "onMetadata", 10) != 0 || (data[13] != 0x03 && data[13] != 0x08))
    {
        return FALSE;
    }

    

    data += 14;
    size -= 14;

    if (flag == 0x08)
    {
        return amf_parse_ecma_array(&data, &size, index, mdata);
    }
    else if (flag == 0x03)
    {
        return amf_parse_object(&data, &size, index, mdata);
    }
    else
    {
        return FALSE;
    }
    return TRUE;
}
