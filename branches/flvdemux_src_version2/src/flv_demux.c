#include <string.h>
#include "../demux.h"
#include "../avformat.h"
#include "../mp_msg.h"
#include "../commonplaytype.h"
#include "flv_demux.h"
#include "flv_parse.h"
#include "amf_parse.h"

/// @brief  Indicate if demux module need force close
static int           need_force_close = 0;
/// @brief  Check if file of this type can handle
static int           can_handle(int fileformat);
/// @brief  Create demux context
static DemuxContext* create_demux_context();
/// @brief  Destroy demux context
static void          destroy_demux_context(DemuxContext * ctx);
/// @brief  Set the force close flag
extern void          flv_demux_force_close ();
/// @brief  Get a tag packet and set it into dmx
static BOOL          flv_demux_get_tag_packet (FLVDemuxer* dmx);
/// @brief  Add a pre-read tag packet
static BOOL          flv_demux_add_a_prepacket (FLVDemuxer* dmx);
/// @brief  Get a pre-read packet
static BOOL          flv_demux_check_prepacket_list (FLVDemuxer* dmx, UI64 pos, AVPacket* pkt);


DemuxContextHelper   flv_demux_helper =
{
    "flv",
    can_handle,
    create_demux_context,
    destroy_demux_context,
    0,
    0
};
static int           can_handle(int fileformat)
{
    if (fileformat == FILEFORMAT_ID_FLV)
    {
        mp_msg(0, MSGL_V, "can_handle : OK\n");
        return 0;
    }
    mp_msg(0, MSGL_V, "can_handle :: Cannot handle\n");
    return -1;
}
static DemuxContext* create_demux_context()
{
    DemuxContext * ctx = (DemuxContext*)calloc(1, sizeof(DemuxContext));
    ctx->demux_open = flv_demux_open;
    ctx->demux_probe = flv_demux_probe;
    ctx->demux_close = flv_demux_close;
    ctx->demux_parse_metadata = flv_demux_parse_metadata;
    ctx->demux_read_packet = flv_demux_read_packet;
    ctx->demux_seek = flv_demux_seek;

    //insert the instance to the global instance list
    if(flv_demux_helper.priv_data == 0)
    {
        flv_demux_helper.priv_data = ctx;
    }
    else
    {
        ctx->next = (DemuxContext*)flv_demux_helper.priv_data;
        flv_demux_helper.priv_data = ctx;
    }

    return ctx;
}
static void          destroy_demux_context(DemuxContext * ctx)
{
    DemuxContext * cur = (DemuxContext*)flv_demux_helper.priv_data;
    DemuxContext * prev = cur;
    while(cur && cur != ctx)
    {
        prev = cur;
        cur = cur->next;
    }
    if(cur == ctx)
    {
        if(cur == flv_demux_helper.priv_data)
        {
            //head will be free
            flv_demux_helper.priv_data = cur->next;
        }
        else
        {
            prev->next = cur->next;
        }
    }

    /// @note destroy DemuxContext only, it's private data should be cleared in flv_demux_close
    free(ctx);
}
extern void          flv_demux_force_close ()
{
    if (need_force_close != 0)
    {
        need_force_close = 1;
    }
}
static BOOL          flv_demux_get_tag_packet (FLVDemuxer* dmx)
{
    FLVTagPacket* pkt = NULL;
    URLProtocol*  pro = NULL;
    UI8 temp[FLV_TAGS_HEADER_SIZE];
    int ret;

    if (dmx == NULL || (pro = dmx->m_URLProtocol) == NULL)
    {
        return FALSE;
    }
    pkt = &dmx->m_CurrentPacket;

    /// Read Tag Header
    ret = pro->url_read(pro, temp, FLV_TAGS_HEADER_SIZE);
    if (0 == ret)
    {
        pkt->m_TagDataSize = 0UL;
        return TRUE;
    }
    if (FLV_TAGS_HEADER_SIZE != ret)
    {
        return FALSE;
    }

    /// Parse Tag Header
    if (flv_parse_tag_header(pkt, temp, FLV_TAGS_HEADER_SIZE) == FALSE)
    {
        int i = 0;
        mp_msg (0, MSGL_ERR\
            , "DEMUX ################ flv_demux_get_tag_packet :: Get Illegal Data : ");
        while (i < FLV_TAGS_HEADER_SIZE)
        {
            mp_msg (0, MSGL_ERR, "0x%02X ", temp[i]);
            ++i;
        }
        mp_msg (0, MSGL_ERR, "\n");
        return FALSE;
    }

    /// Check If Buffer Space Is Enough
    if (pkt->m_TagBufferLen < pkt->m_TagDataSize)
    {
        if (pkt->m_TagData != NULL)
        {
            free (pkt->m_TagData);
            pkt->m_TagData = NULL;
        }
        pkt->m_TagData = (UI8*)malloc(pkt->m_TagDataSize);
        if (pkt->m_TagData == NULL)
        {
            return FALSE;
        }
        pkt->m_TagBufferLen = pkt->m_TagDataSize;
    }

    /// Read Tag Data
    if (pkt->m_TagDataSize != pro->url_read(pro, pkt->m_TagData, pkt->m_TagDataSize))
    {
        return FALSE;
    }

    /// Skip Tag Tailer
    if (FLV_TAGS_TAILER_SIZE != pro->url_read(pro, temp, FLV_TAGS_TAILER_SIZE))
    {
        return FALSE;
    }

    pkt->m_TagPosition = dmx->m_CurrentPosition;
    dmx->m_CurrentPosition += (pkt->m_TagDataSize + FLV_TAGS_HEADER_SIZE + FLV_TAGS_TAILER_SIZE);
    return TRUE;
}
static BOOL          flv_demux_add_a_prepacket (FLVDemuxer* dmx)
{
    PrereadTags* temp = NULL;
    
    if (dmx == NULL)
    {
        return FALSE;
    }

    temp = &dmx->m_PrereadTagList;
    while (temp->m_Next != NULL)
    {
        temp = temp->m_Next;
    }
    temp->m_Next = (PrereadTags*)malloc(sizeof(PrereadTags));
    if (temp->m_Next == NULL)
    {
        return FALSE;
    }
    temp = temp->m_Next;
    temp->m_Next   = NULL;
    temp->m_Packet = (FLVTagPacket*)malloc(sizeof(FLVTagPacket));
    if (temp->m_Packet == NULL)
    {
        return FALSE;
    }
    memcpy (temp->m_Packet, &dmx->m_CurrentPacket, sizeof(FLVTagPacket));
    memset (&dmx->m_CurrentPacket, 0, sizeof(FLVTagPacket));
    return TRUE;
}
static BOOL          flv_demux_check_prepacket_list (FLVDemuxer* dmx, UI64 pos, AVPacket* pkt)
{
    PrereadTags* temp = NULL;

    if (dmx == NULL)
    {
        return FALSE;
    }

    temp = dmx->m_PrereadTagList.m_Next;
    while (temp != NULL)
    {
        FLVTagPacket* pack;
        pack = temp->m_Packet;
        if (pack != NULL && pack->m_TagPosition == pos)
        {
            /// Free Data in AVPacket
            if (pkt->data != NULL)
            {
                free (pkt->data);
                pkt->data = NULL;
            }
            /// Reset AVPacket Members
            memset(pkt, 0, sizeof(AVPacket));
            pkt->pts    = pack->m_TagTimestamp;
            pkt->data   = pack->m_TagData; pack->m_TagData = NULL;
            pkt->size   = pack->m_TagDataSize;
            pkt->bufferlength = pack->m_TagBufferLen;
            pkt->stream_index = pack->m_TagType;
            /// Free Pre-read Packet and PrereadTags Node
            free (pack);
            pack = temp->m_Packet = NULL;
            dmx->m_PrereadTagList.m_Next = temp->m_Next;
            free (temp);
            temp = dmx->m_PrereadTagList.m_Next;

            dmx->m_CurrentPosition += (pkt->size + FLV_TAGS_HEADER_SIZE + FLV_TAGS_TAILER_SIZE);
            return TRUE;
        }
        else if (pack->m_TagPosition > pos)
        {
            if (pack != NULL)
            {
                if (pack->m_TagData != NULL)
                {
                    free (pack->m_TagData);
                    pack->m_TagData = NULL;
                }
                free (pack);
                pack = temp->m_Packet = NULL;
            }
            dmx->m_PrereadTagList.m_Next = temp->m_Next;
            free (temp);
            temp = dmx->m_PrereadTagList.m_Next;
        }
        else
        {
            return FALSE;
        }
    }
    return FALSE;
}

int       flv_demux_open (DemuxContext* ctx, URLProtocol* h)
{
    FLVDemuxer* dmx;
    I8  errstr[64];
    int errtyp = MSGL_V;

    do{
        if (ctx == NULL || h == NULL)
        {
            errtyp = MSGL_ERR;
            strcpy(errstr, "Parameters Error");
            break;
        }

        dmx = (FLVDemuxer*)malloc(sizeof(FLVDemuxer));
        if (dmx == NULL)
        {
            errtyp = MSGL_ERR;
            strcpy(errstr, "Allocate Private Data Falied");
            break;
        }
        memset (dmx, 0, sizeof(FLVDemuxer));

        ctx->priv_data_size     = sizeof(FLVDemuxer);
        ctx->priv_data          = (void *)dmx;

        dmx->m_FileSize         = (h ? h->url_seek(h, 0, SEEK_SIZE) : 0);
        dmx->m_AudioBitRate     = 0UL;
        dmx->m_VideoBitRate     = 0UL;
        dmx->m_FileDuration     = 0ULL;
        dmx->m_CurrentPosition  = 0ULL;
        dmx->m_URLProtocol      = h;
        memset (&dmx->m_CurrentPacket,  0, sizeof(FLVTagPacket));
        memset (&dmx->m_PrereadTagList, 0, sizeof(PrereadTags));

        errtyp = MSGL_V;
        strcpy(errstr, "Open Demux Success");
    }while(0);

    mp_msg(0, errtyp, "DEMUX ################ flv_demux_open %s\n", errstr);
    return (errtyp == MSGL_ERR ? -1 : 0);
}
int       flv_demux_probe (DemuxContext* ctx)
{
    FLVDemuxer*  dmx = NULL;
    URLProtocol* pro = NULL;
    UI8 temp[FLV_FILE_HEADER_SIZE];
    I8  errstr[64];
    int errtyp = MSGL_V;

    do{
        if ((ctx == NULL) || ((dmx = (FLVDemuxer*)ctx->priv_data) == NULL)\
            || ((pro = dmx->m_URLProtocol) == NULL))
        {
            errtyp = MSGL_ERR;
            strcpy(errstr, "Parameter Error");
            break;
        }

        if (FLV_FILE_HEADER_SIZE != pro->url_read(pro, temp, FLV_FILE_HEADER_SIZE))
        {
            errtyp = MSGL_ERR;
            strcpy(errstr, "Read Data Failed");
            break;
        }

        if (temp[0] != 'F' || temp[1] != 'L' || temp[2] != 'V')
        {
            errtyp = MSGL_ERR;
            strcpy(errstr, "File Is Not a FLV File");
            break;
        }
        errtyp = MSGL_V;
        strcpy(errstr, "Probe File is available");
    }while(0);

    mp_msg(0, errtyp, "DEMUX ################ flv_demux_probe %s\n", errstr);
    return (errtyp == MSGL_ERR ? -1 : 0);
}
int       flv_demux_close (DemuxContext* ctx)
{
    FLVDemuxer*  dmx  = NULL;
    PrereadTags* tags = NULL;
    PrereadTags* temp = NULL;
    I8  errstr[64];
    int errtyp = MSGL_V;

    if ((ctx == NULL) || ((dmx = (FLVDemuxer*)ctx->priv_data) == NULL))
    {
        errtyp = MSGL_ERR;
        strcpy(errstr, "Parameter Error");
        goto FLV_DEMUX_CLOSE_ERROR;
    }

    /// XXX Free Pre-read List
    tags = &dmx->m_PrereadTagList;
    if (tags->m_Packet != NULL)
    {
        if (tags->m_Packet->m_TagData != NULL)
        {
            free (tags->m_Packet->m_TagData);
            tags->m_Packet->m_TagData = NULL;
        }
        free (tags->m_Packet);
        tags->m_Packet = NULL;
    }
    temp = tags->m_Next;
    while (temp != NULL)
    {
        tags->m_Next = temp->m_Next;
        if (temp->m_Packet != NULL)
        {
            if (temp->m_Packet->m_TagData != NULL)
            {
                free (temp->m_Packet->m_TagData);
                temp->m_Packet->m_TagData = NULL;
            }
            free (temp->m_Packet);
            temp->m_Packet = NULL;
        }
        free (temp);
        temp = tags->m_Next;
    }
    /// XXX Free Timestamps Index
    if (NULL != dmx->m_TimestampIndex.m_Index)
    {
        free (dmx->m_TimestampIndex.m_Index);
        dmx->m_TimestampIndex.m_Index = NULL;
    }
    /// XXX Free Current Packet in #FLVDemuxer
    if (NULL != dmx->m_CurrentPacket.m_TagData)
    {
        free (dmx->m_CurrentPacket.m_TagData);
        dmx->m_CurrentPacket.m_TagData = NULL;
    }

    errtyp = MSGL_V;
    strcpy(errstr, "Close Demux Success");

FLV_DEMUX_CLOSE_ERROR:
    mp_msg(0, errtyp, "DEMUX ################ flv_demux_close %s\n", errstr);
    return (errtyp == MSGL_ERR ? -1 : 0);
}
int       flv_demux_parse_metadata (DemuxContext* ctx, Metadata* meta)
{
    FLVDemuxer*   dmx = NULL;
    URLProtocol*  pro = NULL;
    UI8  header[FLV_FILE_HEADER_SIZE];
    I8   errstr[64];
    UI8  errtyp = MSGL_V;
    UI64 avoffset = 0ULL;
    BOOL flag;

    if ((ctx == NULL) || ((dmx = (FLVDemuxer*)ctx->priv_data) == NULL)\
        || ((pro = dmx->m_URLProtocol) == NULL))
    {
        errtyp = MSGL_ERR;
        strcpy(errstr, "Parameter Error");
        goto FLV_DEMUX_PARSE_METADATA_ERROR;
    }
    /// XXX Skip FLV File Header
    if (FLV_FILE_HEADER_SIZE != pro->url_read(pro, header, FLV_FILE_HEADER_SIZE))
    {
        errtyp = MSGL_ERR;
        strcpy(errstr, "Skip File Header Failed");
        goto FLV_DEMUX_PARSE_METADATA_ERROR;
    }
    dmx->m_CurrentPosition = FLV_FILE_HEADER_SIZE;

    /// Read A Tag Packet : #FLV_TAGS_HEADER_SIZE + Tag Data Size + #FLV_TAGS_TAILER_SIZE;
    flag = flv_demux_get_tag_packet (dmx);
    if (FALSE == flag)
    {
        errtyp = MSGL_ERR;
        strcpy(errstr, "Get Tag Packet Failed");
        goto FLV_DEMUX_PARSE_METADATA_ERROR;
    }
    else if (TRUE == flag && dmx->m_CurrentPacket.m_TagDataSize == 0UL)
    {
        errtyp = MSGL_ERR;
        strcpy(errstr, "File Termination Unexpected");
        goto FLV_DEMUX_PARSE_METADATA_ERROR;
    }

    meta->audiocodec    = -1;
    meta->subaudiocodec = -1;
    meta->videocodec    = -1;
    meta->subaudiocodec = -1;

    if (dmx->m_CurrentPacket.m_TagType == MDATA_FLV_STREAM_ID)
    {
        avoffset = dmx->m_CurrentPosition;
        if (flv_parse_tag_script(&dmx->m_CurrentPacket, &dmx->m_TimestampIndex, meta) == FALSE)
        {
            errtyp = MSGL_ERR;
            strcpy(errstr, "Parse Tag Script Failed");
            goto FLV_DEMUX_PARSE_METADATA_ERROR;
        }
    }
    else
    {
        UI8  flag = dmx->m_CurrentPacket.m_TagData[0];
        avoffset = (dmx->m_CurrentPosition - dmx->m_CurrentPacket.m_TagDataSize\
            - FLV_TAGS_HEADER_SIZE + FLV_TAGS_TAILER_SIZE);
        if (dmx->m_CurrentPacket.m_TagType == AUDIO_FLV_STREAM_ID)
        {
            flag = (flag >> 4) & 0x0F;
            switch(flag)
            {
            case 2: ///< mp3
            case 14:///< mp3
                meta->audiocodec = CODEC_ID_MP3;
                break;
            case 10:///< aac
                meta->audiocodec = CODEC_ID_AAC;
                meta->subaudiocodec = DACF_ADIF;
                break;
            default:
                meta->audiocodec = CODEC_ID_NONE;
                break;
            }
        }
        else
        {
            flag = flag & 0x0F;
            switch(flag)
            {
            case 2:
                meta->videocodec = CODEC_ID_H263;
                break;
            case 7:
                meta->videocodec = CODEC_ID_H264;
                break;
            default:
                meta->videocodec = CODEC_ID_NONE;
                break;
            }
        }
        if (FALSE == flv_demux_add_a_prepacket(dmx))
        {
            errtyp = MSGL_ERR;
            strcpy(errstr, "Add A Pre-read Packet Failed");
            goto FLV_DEMUX_PARSE_METADATA_ERROR;
        }
    }

    while ((meta->audiocodec == -1)\
        || (meta->audiocodec == CODEC_ID_AAC && meta->subaudiocodec == -1)\
        || (meta->videocodec == -1))
    {
        UI8  flag = 0U;

        do 
        {
            BOOL readflag = flv_demux_get_tag_packet (dmx);
            if (FALSE == readflag)
            {
                errtyp = MSGL_ERR;
                strcpy(errstr, "Get Tag Packet Failed");
                goto FLV_DEMUX_PARSE_METADATA_ERROR;
            }
            else if (TRUE == readflag && dmx->m_CurrentPacket.m_TagDataSize == 0UL)
            {
                errtyp = MSGL_V;
                strcpy(errstr, "File End");
                goto FLV_DEMUX_PARSE_METADATA_ERROR;
            }
        }while(dmx->m_CurrentPacket.m_TagType == MDATA_FLV_STREAM_ID);

        flag = dmx->m_CurrentPacket.m_TagData[0];

        if (dmx->m_CurrentPacket.m_TagType == AUDIO_FLV_STREAM_ID)
        {
            flag = (flag >> 4) & 0x0F;
            switch(flag)
            {
            case 2: ///< mp3
            case 14:///< mp3
                meta->audiocodec = CODEC_ID_MP3;
                break;
            case 10:///< aac
                meta->audiocodec = CODEC_ID_AAC;
                meta->subaudiocodec = DACF_ADIF;
                break;
            default:
                meta->audiocodec = CODEC_ID_NONE;
                break;
            }
        }
        else
        {
            flag = flag & 0x0F;
            switch(flag)
            {
            case 2:
                meta->videocodec = CODEC_ID_H263;
                break;
            case 7:
                meta->videocodec = CODEC_ID_H264;
                break;
            default:
                meta->videocodec = CODEC_ID_NONE;
                break;
            }
        }
        if (FALSE == flv_demux_add_a_prepacket(dmx))
        {
            errtyp = MSGL_ERR;
            strcpy(errstr, "Add A Pre-read Packet Failed");
            goto FLV_DEMUX_PARSE_METADATA_ERROR;
        }
    }
    errtyp = MSGL_V;
    strcpy(errstr, "Parsing Metadata Successfully");
FLV_DEMUX_PARSE_METADATA_ERROR:
    dmx->m_CurrentPosition = avoffset;
    mp_msg(0, errtyp, "DEMUX ################ flv_demux_parse_metadata %s\n", errstr);
    if (errtyp == MSGL_V)
    {
        mp_msg(0, errtyp, "DEMUX ################ flv_demux_parse_metadata VIDEO_ID = %d\n"\
            , meta->videocodec);
        mp_msg(0, errtyp, "DEMUX ################ flv_demux_parse_metadata AUDIO_ID = %d\n"\
            , meta->audiocodec);
        mp_msg(0, errtyp, "DEMUX ################ flv_demux_parse_metadata V_SUB_ID = %d\n"\
            , meta->subvideocodec);
        mp_msg(0, errtyp, "DEMUX ################ flv_demux_parse_metadata A_SUB_ID = %d\n"\
            , meta->subaudiocodec);
        mp_msg(0, errtyp, "DEMUX ################ flv_demux_parse_metadata V_BITRAT = %d\n"\
            , meta->vbitrate);
        mp_msg(0, errtyp, "DEMUX ################ flv_demux_parse_metadata A_BITRAT = %d\n"\
            , meta->abitrate);
        mp_msg(0, errtyp, "DEMUX ################ flv_demux_parse_metadata DURATION = %d\n"\
            , meta->duation);
        dmx->m_FileDuration = meta->duation;
        dmx->m_AudioBitRate = meta->abitrate;
        dmx->m_VideoBitRate = meta->vbitrate;
    }
    return (errtyp == MSGL_ERR ? -1 : 0);
}
int       flv_demux_read_packet (DemuxContext* ctx, AVPacket* pkt)
{
    FLVDemuxer* dmx;
    I8   errstr[64];
    UI8  errtyp = MSGL_V;
    UI64 pos;

    if ((ctx == NULL) || (dmx = (FLVDemuxer*)ctx->priv_data) == NULL || (pkt == NULL)\
        || (dmx->m_URLProtocol == NULL))
    {
        errtyp = MSGL_ERR;
        strcpy(errstr, "Parameter Error");
        goto FLV_DEMUX_READ_PACKET_ERROR;
    }

    pos = dmx->m_CurrentPosition;

    if (TRUE == flv_demux_check_prepacket_list(dmx, dmx->m_CurrentPosition, pkt))
    {
        goto FLV_DEMUX_READ_PACKET_OK;
    }

    do
    {
        BOOL readflag = flv_demux_get_tag_packet (dmx);
        if (FALSE == readflag)
        {
            errtyp = MSGL_ERR;
            strcpy(errstr, "Get Tag Packet Failed");
            goto FLV_DEMUX_READ_PACKET_ERROR;
        }
        else if (TRUE == readflag && dmx->m_CurrentPacket.m_TagDataSize == 0UL)
        {
            errtyp = MSGL_V;
            strcpy(errstr, "File End");
            goto FLV_DEMUX_READ_PACKET_FILE_END;
        }
    }while(dmx->m_CurrentPacket.m_TagType == MDATA_FLV_STREAM_ID);

    if (pkt->bufferlength < (int)dmx->m_CurrentPacket.m_TagDataSize)
    {
        if (pkt->data != NULL)
        {
            free (pkt->data);
            pkt->data = NULL;
        }
        pkt->data = (UI8*)malloc(dmx->m_CurrentPacket.m_TagDataSize);
        if (pkt->data == NULL)
        {
            errtyp = MSGL_ERR;
            strcpy(errstr, "Re-Allocate AVPacket Data Space Failed");
            goto FLV_DEMUX_READ_PACKET_ERROR;
        }
        pkt->bufferlength = dmx->m_CurrentPacket.m_TagDataSize;
    }
    pkt->size         = dmx->m_CurrentPacket.m_TagDataSize;
    pkt->pts          = dmx->m_CurrentPacket.m_TagTimestamp;
    pkt->stream_index = dmx->m_CurrentPacket.m_TagType;
    memcpy(pkt->data, dmx->m_CurrentPacket.m_TagData, pkt->size);
    goto FLV_DEMUX_READ_PACKET_OK;

FLV_DEMUX_READ_PACKET_ERROR:    ///< Operation is Failed
    mp_msg(0, errtyp, "DEMUX ################ flv_demux_read_packet %s\n", errstr);
    return -1;
FLV_DEMUX_READ_PACKET_OK:       ///< All Process is OK
    mp_msg(0, errtyp\
        , "DEMUX ################ flv_demux_read_packet -%s- PTS = %-8lld SIZE = %-8d POS = %lld\n"\
        , (pkt->stream_index == 0x08 ? "AUDIO" : "VIDEO"), pkt->pts, pkt->size, pos);
    return pkt->size;
FLV_DEMUX_READ_PACKET_FILE_END: ///< File End
    mp_msg(0, errtyp, "DEMUX ################ flv_demux_read_packet %s\n", errstr);
    return 0;
}
long long flv_demux_seek (DemuxContext* ctx, long long ts)
{
    FLVDemuxer*   dmx = NULL;
    URLProtocol*  pro = NULL;
    I8   errstr[64];
    UI8  errtyp = MSGL_V;

    /// Check Parameters
    if (ctx == NULL || ((dmx = (FLVDemuxer*)(ctx->priv_data)) == NULL)\
        || ((pro = (URLProtocol*)dmx->m_URLProtocol) == NULL))
    {
        errtyp = MSGL_ERR;
        strcpy(errstr, "Parameters Error");
        goto FLV_DEMUX_SEEK_ERROR;
    }

    /// Check URLProtocol Status
    if (1 == pro->url_is_live(pro))
    {
        errtyp = MSGL_ERR;
        strcpy(errstr, "This is a live stream, Cannot seek");
        goto FLV_DEMUX_SEEK_ERROR;
    }

    /// Check File Size And File Duration
    if (dmx->m_FileSize == 0x7FFFFFFFFFFFFFFFULL || dmx->m_FileSize == 0ULL\
        || dmx->m_FileDuration == 0ULL)
    {
        errtyp = MSGL_ERR;
        strcpy(errstr, "File Size or Duration Is Not Indicated, Cannot Seek");
        goto FLV_DEMUX_SEEK_ERROR;
    }


    mp_msg(0, MSGL_V, "DEMUX ################ flv_demux_seek CURR_POS = %lld\n"\
        , dmx->m_CurrentPosition);

    /// Included Timestamp Index in FLV File
    if (dmx->m_TimestampIndex.m_Count != 0UL && dmx->m_TimestampIndex.m_Index != NULL)
    {
        UI32 i = 0;
        ts /= 1000;
        while (i < dmx->m_TimestampIndex.m_Count)
        {
            if ((I64)dmx->m_TimestampIndex.m_Index[i].m_TimePos >= ts)
            {
                break;
            }
            ++i;
        }

        if (i == dmx->m_TimestampIndex.m_Count)
        {
            dmx->m_CurrentPosition = dmx->m_TimestampIndex.m_Index[i - 1].m_FilePos;
        }
        else
        {
            dmx->m_CurrentPosition = dmx->m_TimestampIndex.m_Index[i].m_FilePos;
        }

        errtyp = MSGL_V;
        strcpy(errstr, "Seek Successful");
        goto FLV_DEMUX_SEEK_OK;
    }
    else
    {
        I64 testpos = dmx->m_FileSize / dmx->m_FileDuration * ts / 1000;
        I64 currpos;

        while (1)
        {
            UI8 flag = 0;
            int ret  = 0;
            UI8 temp[FLV_TAGS_HEADER_SIZE];
            FLVTagPacket* pkt = &dmx->m_CurrentPacket;

            /// Seek To Test Position
            currpos = testpos;
            if (pro->url_seek(pro, testpos, SEEK_SET) < 0)
            {
                errtyp = MSGL_ERR;
                strcpy(errstr, "Calling url_seek Failed");
                goto FLV_DEMUX_SEEK_ERROR;
            }

            /// Find 0x08 or 0x09 Flag from Test Position on
            while (flag != 0x08 && flag != 0x09)
            {
                ret = pro->url_read(pro, &flag, 1);
                if (ret == 0)
                {
                    errtyp = MSGL_ERR;
                    strcpy(errstr, "File End");
                    goto FLV_DEMUX_SEEK_ERROR;
                }
                if (ret != 1)
                {
                    errtyp = MSGL_ERR;
                    strcpy(errstr, "Calling url_read Failed");
                    goto FLV_DEMUX_SEEK_ERROR;
                }
                ++testpos;
                ++currpos;
            }

            /// Test following data
            while (1)
            {
                int  validtag = 0;          ///< Valid Tag Count(MUST >= 5)
                BOOL getvideo = FALSE;      ///< If have got video tag
                BOOL avcvideo = FALSE;      ///< If This file is contain AVC
                I64  keyframe = -1LL;       ///< Key frame posiion

                /// Circle Exiting And Seek Return Condition
                if ((getvideo== TRUE) && (validtag >= 5)\
                    && ((avcvideo == TRUE && keyframe != -1) || (avcvideo == FALSE)))
                {
                    dmx->m_CurrentPosition = testpos - 1;
                    goto FLV_DEMUX_SEEK_OK;
                }

                /// Read the left tag header data(FLV_TAGS_HEADER_SIZE - 1)
                temp[0] = flag;
                ret = pro->url_read(pro, &temp[1], FLV_TAGS_HEADER_SIZE - 1);
                if (ret == 0)
                {
                    errtyp = MSGL_ERR;
                    strcpy(errstr, "File End");
                    goto FLV_DEMUX_SEEK_ERROR;
                }
                if (ret != (FLV_TAGS_HEADER_SIZE - 1))
                {
                    errtyp = MSGL_ERR;
                    strcpy(errstr, "Calling url_read Failed");
                    goto FLV_DEMUX_SEEK_ERROR;
                }

                /// Parse Tag Header And Check if the Tag Data Size and PTS are Reasonable
                if (flv_parse_tag_header(pkt, temp, FLV_TAGS_HEADER_SIZE) == FALSE)
                {
                    break;
                }
                if ((pkt->m_TagDataSize + 4 > dmx->m_FileSize)\
                    || (pkt->m_TagTimestamp / 1000 > dmx->m_FileDuration))
                {
                    break;
                }

                /// Now assume found a Tag and then Go to Find Key Frame
                if (temp[0] == AUDIO_FLV_STREAM_ID)
                {
                    getvideo = TRUE;
                    ret = pro->url_read(pro, &flag, 1);
                    if (ret == 0)
                    {
                        errtyp = MSGL_ERR;
                        strcpy(errstr, "File End");
                        goto FLV_DEMUX_SEEK_ERROR;
                    }
                    if (ret != 1)
                    {
                        errtyp = MSGL_ERR;
                        strcpy(errstr, "Calling url_read Failed");
                        goto FLV_DEMUX_SEEK_ERROR;
                    }
                    if ((flag & 0x0F) == 7)
                    {
                        avcvideo = TRUE;
                        if (((flag >> 4) & 0x0F) == 1)
                        {
                            keyframe = currpos;
                        }
                    }
                }
                currpos += (FLV_TAGS_HEADER_SIZE - 1 + pkt->m_TagDataSize + FLV_TAGS_TAILER_SIZE);
                if (pro->url_seek(pro, currpos, SEEK_SET) < 0)
                {
                    errtyp = MSGL_ERR;
                    strcpy(errstr, "Calling url_seek Failed");
                    goto FLV_DEMUX_SEEK_ERROR;
                }
                ++validtag;
            }
            ++testpos;
        }
    }

FLV_DEMUX_SEEK_ERROR :
    mp_msg(0, errtyp, "DEMUX ################ flv_demux_seek %s", errstr);
    return -1;
FLV_DEMUX_SEEK_OK:
    if (pro->url_seek(pro, dmx->m_CurrentPosition, SEEK_SET) < 0)
    {
        errtyp = MSGL_ERR;
        strcpy(errstr, "Calling url_seek Failed");
        goto FLV_DEMUX_SEEK_ERROR;
    }
    mp_msg(0, errtyp, "DEMUX ################ flv_demux_seek %s SEEK_POS = %lld\n"\
        , errstr, dmx->m_CurrentPosition);
    return dmx->m_CurrentPosition;
}
int       flv_demux_parse_codec_from_raw_data(unsigned char * data, int size, Metadata* meta)
{
    return 0;
}

#if 0
long long flv_demux_seek (DemuxContext* ctx, long long ts)
{
    if (index == NULL || count == 0)
    {
        int biterate;/* = (int)(datasize / duration); / * bytes per millisecond * /*/
        long long testpos;/* = (int)(rate * ts + dmx->m_AVDataOffset);*/
        unsigned char* data = dmx->m_InputInfor.data;

        if (dmx->m_Duration - ts <= 10000)
        {
            mp_msg(0, MSGL_V, "flv_demux_seek :: close to file end cannot seek\n");
            return -1;
        }
        biterate = (int)((dmx->m_FileSize - dmx->m_AVDataOffset) / dmx->m_Duration);
        testpos  = biterate * ts + dmx->m_AVDataOffset;
        if (testpos >= dmx->m_FileSize)
        {
            mp_msg(0, MSGL_V, "flv_demux_seek :: close to file end cannot seek\n");
            return -1;
        }
        mp_msg(0, MSGL_V, "flv_demux_seek :: test  start = %lld\n", testpos);

        while (1)
        {
            int validtags  = 0;
            int havevideo  = 0;
            int haveavcdat = 0;
            long long keyfrmpos  = -1;

#           ifndef _FLV_DEMUX_TEST_
            if (dmx->m_URLProtocol->url_seek(dmx->m_URLProtocol, testpos, SEEK_SET) < 0)
            {
                mp_msg(0, MSGL_V, "flv_demux_seek :: url_seek :: failed\n");
                return -1;
            }
#           endif
            dmx->m_CurrPos = testpos;

            while (1)
            {
                /// circle exit condition :
                /// Test 5 tags success and tested 1 video tag at least
                /// if the video format is AVC,tested 1 key frame tag at least and mark the position

                /// read and test a tag header
                if (read_file_data(dmx, dmx->m_CurrPos, FLV_TAGS_HEADER_SIZE) <= 0)
                {
                    mp_msg(0, MSGL_V, "flv_demux_seek :: read_file_data :: failed\n");
                    return -1;
                }
                if (flv_parse_tag_header(dmx) < 0 || dmx->m_DemuxState == FLV_DEMUX_MDATA)
                {
                    break;
                }
                if (dmx->m_CurrPos + dmx->m_TagDataSize > dmx->m_FileSize)
                {
                    break;
                }
                if (dmx->m_DemuxState == FLV_DEMUX_VIDEO)
                {
                    havevideo = 1;
                    if (read_file_data(dmx, dmx->m_CurrPos + FLV_TAGS_HEADER_SIZE, 1) <= 0)
                    {
                        mp_msg(0, MSGL_V, "flv_demux_seek :: read_file_data :: failed\n");
                        return -1;
                    }
                    if ((data[0] & 0x0F) == 7)
                    {
                        haveavcdat = 1;
                        if (((data[0] >> 4) & 0x0F) == 1)
                        {
                            keyfrmpos = dmx->m_CurrPos;
                        }
                    }
                }
                dmx->m_CurrPos += (FLV_TAGS_HEADER_SIZE + dmx->m_TagDataSize + FLV_TAGS_TAILER_SIZE);
                /// skip a tag
#               ifndef _FLV_DEMUX_TEST_
                dmx->m_URLProtocol->url_seek(dmx->m_URLProtocol, dmx->m_CurrPos, SEEK_SET);
#               endif
                ++validtags;
            }
            ++testpos;
        }
    }
    else
    {
        int i = 0;
        ts /= 1000;
        while (i < count)
        {
            if (ts <= dmx->m_IndexList.elems[i].ts)
            {
                break;
            }
            ++i;
        }

        if (i == count)
        {
            dmx->m_CurrPos = dmx->m_IndexList.elems[i - 1].pos;
        }
        else
        {
            dmx->m_CurrPos = dmx->m_IndexList.elems[i].pos;
        }
        mp_msg(0, MSGL_V, "flv_demux_seek :: seeked_to_pos = %lld\n", dmx->m_CurrPos);
    }

#   ifndef _FLV_DEMUX_TEST_
    if (dmx->m_URLProtocol->url_seek(dmx->m_URLProtocol, dmx->m_CurrPos, SEEK_SET) < 0)
    {
        mp_msg(0, MSGL_V, "flv_demux_seek :: url_seek :: failed\n");
        return -1;
    }
#   endif
    mp_msg(0, MSGL_V, "flv_demux_seek :: OK\n");
    return dmx->m_CurrPos;
}
int flv_demux_parse_codec_from_raw_data(unsigned char data[], int size, Metadata* meta)
{
    int tag_data_size;
    int got_audio_codec = 0;
    int got_video_codec = 0;

    if (size < FLV_FILE_HEADER_SIZE + FLV_TAGS_HEADER_SIZE)
    {
        return FLV_FILE_HEADER_SIZE + FLV_TAGS_HEADER_SIZE;
    }
    if (data[0] != 'F' || data[1] != 'L' || data[2] != 'V')
    {
        return -1;
    }
    data += FLV_FILE_HEADER_SIZE;
    size -= FLV_FILE_HEADER_SIZE;

    if (data[0] != 0x12)
    {
        return -1;
    }

    tag_data_size = get_ui24(data + 1, size - 1);

    data += FLV_TAGS_HEADER_SIZE;
    size -= FLV_TAGS_HEADER_SIZE;

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

    /* The next element type must be 0x08 */
    if (data[0] != 0x08)
    {
        return -1;
    }

    data += 5;
    size -= 5;

    while (size > 0 && (got_audio_codec == 0 || got_video_codec == 0)\
        && (data[0] != 0x00 || data[1] != 0x00 || data[2] != 0x09))
    {
        int flag = 0;
        int data_type = 0;

        do
        {
            unsigned short namelens;
            unsigned char elemname[128];
            /* 2 bytes of element length */
            if (size < 2)
            {
                flag = -1;
                break;
            }
            namelens = get_ui16(data, size);
            data += 2;
            size -= 2;

            /* namelens bytes of element name */
            if (size < namelens)
            {
                flag = -1;
                break;
            }
            memcpy(elemname, data, namelens);
            elemname[namelens] = 0;
            data += namelens;
            size -= namelens;

            /* 1 byte of data type */
            if (size < 1)
            {
                flag = -1;
                break;
            }
            data_type = data[0];

            /* other type data  */
            if (data_type != 0x00)
            {
                /* boolean */
                if (data_type == 0x01)
                {
                    if (size < 2)
                    {
                        flag = -1;
                    }
                    else
                    {
                        data += 2;
                        size -= 2;
                    }
                }
                /* string */
                else if (data_type == 0x02)
                {
                    if (size < 3)
                    {
                        flag = -1;
                    }
                    else
                    {
                        int len = 0;
                        data += 1;
                        size -= 1;
                        len = get_ui16(data, size);
                        data += 2;
                        size -= 2;
                        if (size < len)
                        {
                            flag = -1;
                        }
                        else
                        {
                            data += len;
                            size -= len;
                        }
                    }
                }
                /* null */
                else if (data_type == 0x05)
                {
                    data += 1;
                    size -= 1;
                }
                /* data */
                else if (data_type == 0x0B)
                {
                    if (size < 11)
                    {
                        flag = -1;
                    }
                    else
                    {
                        data += 11;
                        size -= 11;
                    }
                }
                /* long string */
                else if (data_type == 0x0C)
                {
                    if (size < 5)
                    {
                        flag = -1;
                    }
                    else
                    {
                        int len = 0;
                        data += 1;
                        size -= 1;
                        len = get_ui32(data, size);
                        data += 4;
                        size -= 4;
                        if (size < len)
                        {
                            flag = -1;
                        }
                        else
                        {
                            data += len;
                            size -= len;
                        }
                    }
                }
                /* need all tag data */
                else
                {
                    flag = -1;
                }
                break;
            }

            /* double data */
            if (size < 9)
            {
                flag = -1;
                break;
            }
            data += 1;
            size -= 1;
            if (strcmp("videocodecid", (const char*)elemname) == 0)
            {
                meta->videocodec = (int)get_fl64(data, size);
                switch(meta->videocodec)
                {
                case 2:///< H.263
                    meta->videocodec = CODEC_ID_H263;
                    break;
                case 7:///< AVC
                    meta->videocodec = CODEC_ID_H264;
                    break;
                default:
                    meta->videocodec = CODEC_ID_NONE;
                }
                got_video_codec = 1;
            }
            else if(strcmp("audiocodecid", (const char*)elemname) == 0)
            {
                meta->audiocodec = (int)get_fl64(data, size);
                switch(meta->audiocodec)
                {
                case 2: ///< mp3
                case 14:///< mp3
                    meta->audiocodec = CODEC_ID_MP3;
                    break;
                case 10:///< aac
                    meta->audiocodec = CODEC_ID_AAC;
                    break;
                default:
                    meta->audiocodec = CODEC_ID_NONE;
                }
                got_audio_codec = 1;
            }
            data += 8;
            size -= 8;
        } while (0);
        if (flag != 0)
        {
            break;
        }
    }

    if (got_audio_codec != 1 || got_video_codec != 1)
    {
        return tag_data_size + FLV_FILE_HEADER_SIZE + FLV_TAGS_HEADER_SIZE;
    }

    return 0;
}
#endif
