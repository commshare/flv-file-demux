#include <string.h>
#include "../demux.h"
#include "../avformat.h"
#include "../mp_msg.h"
#include "../commonplaytype.h"
#include "flv_demux.h"
#include "flv_parse.h"
#include "amf_parse.h"

static DemuxContext * create_demux_context();
static void destroy_demux_context(DemuxContext * ctx);
static int  flv_demux_get_aac_sub_type (unsigned char* data);
extern void flv_demux_force_close(void);
static int  read_file_data (FLVDemuxInfo* dmx, long long pos, int size);
static int  add_a_preread_packet (DemuxContext* ctx, AVPacket* pack, long long pos);
static AVPacket* check_preread_tags (FLVDemuxInfo* dmx, long long pos);

static int  need_force_close = 0;
static int  flv_demux_get_aac_sub_type (unsigned char* data)
{
    char  codec_tag   = 0;
    long  codec_data  = 0;
    unsigned char *outbufData = NULL;
    unsigned char flags = data[0];
    unsigned char aac_packet_type;

    codec_tag = flags >> 4;
    if (codec_tag == 0x0A)
    {
        codec_data = 2;
    }
    else
    {
        return DACF_NONE;
    }
    outbufData= data + codec_data;

    aac_packet_type = data[1];
    switch(aac_packet_type)
    {
    case 0:
        return DACF_ADIF;
    case 1:
        return DACF_NONE;
    default:
        return DACF_NONE;
    }
}
extern void flv_demux_force_close(void)
{
    need_force_close = 1;
}
static int  read_file_data (FLVDemuxInfo* dmx, long long pos, int size)
{
#ifdef _FLV_DEMUX_TEST_
    FILE *fileptr;
#endif
    int rlen = 0;
    if (size > dmx->m_InputInfor.buflength)
    {
        if (dmx->m_InputInfor.data != NULL)
        {
            free (dmx->m_InputInfor.data);
        }
        dmx->m_InputInfor.data = (unsigned char*)malloc(size);
        if (dmx->m_InputInfor.data == NULL)
        {
            return -1;
        }
        dmx->m_InputInfor.buflength = size;
    }
    dmx->m_InputInfor.size = size;
#   ifdef  _FLV_DEMUX_TEST_
    fileptr = fopen("D:\\Private\\WorkArea\\FLVDemux\\flvdemux_src\\test.flv", "rb+");
    fseek(fileptr, (long)pos, SEEK_SET);
    rlen = fread(dmx->m_InputInfor.data + rlen, 1, size, fileptr);
    fclose(fileptr);
#   endif 

#   ifndef _FLV_DEMUX_TEST_
    rlen = dmx->m_URLProtocol->url_read(dmx->m_URLProtocol, dmx->m_InputInfor.data + rlen\
        , size - rlen);
#   endif

    if (rlen == 0)
    {
        return 0;
    }
    if (rlen == size)
    {
        return rlen;
    }
    return -1;
}
static int  add_a_preread_packet (DemuxContext* ctx, AVPacket* pack, long long pos)
{
    FLVDemuxInfo* dmx = (FLVDemuxInfo*)ctx->priv_data;
    PrereadTagNode* tmp = NULL;

    if (dmx->m_PrereadList == NULL)
    {
        dmx->m_PrereadList = (PrereadTagNode*)malloc(sizeof(PrereadTagNode));
        if (dmx->m_PrereadList == NULL)
        {
            mp_msg(0, MSGL_V, "Allocate Preread Node Failed\n");
            return -1;
        }
        mp_msg(0, MSGL_V, "Allocate Preread List Header = %p And All Members are set as NULL\n"\
            , dmx->m_PrereadList);
        dmx->m_PrereadList->next = NULL;
        dmx->m_PrereadList->pack = NULL;
        dmx->m_PrereadList->pos  = 0LL;
    }

    tmp = dmx->m_PrereadList;
    while (tmp->next != NULL)
    {
        tmp = tmp->next;
    }

    tmp->next = (PrereadTagNode*)malloc(sizeof(PrereadTagNode));
    if (tmp->next == NULL)
    {
        mp_msg(0, MSGL_V, "Allocate a new preread tag node failed\n");
        return -1;
    }
    mp_msg(0, MSGL_V, "Allocate Preread List Node %p Add AVPacket = %p Packet Data = %p\n"\
        , tmp->next, pack, pack->data);
    tmp = tmp->next;
    tmp->next = NULL;
    tmp->pack = pack;
    tmp->pos  = pos;
    return 0;
}
static AVPacket* check_preread_tags (FLVDemuxInfo* dmx, long long pos)
{
    PrereadTagNode* tmp = dmx->m_PrereadList;
    PrereadTagNode* cur;
    if (tmp == NULL)
    {
        return NULL;
    }

    cur = tmp->next;
    if (cur == NULL)
    {
        if (tmp->pack)
        {
            if (tmp->pack->data)
            {
                mp_msg(0, MSGL_V, "Free Packet data %p\n", tmp->pack->data);
                free(tmp->pack->data);
                tmp->pack->data = NULL;
            }
            mp_msg(0, MSGL_V, "Free Packet %p\n", tmp->pack);
            free(tmp->pack);
            tmp->pack = NULL;
        }
        mp_msg(0, MSGL_V, "Free Preread List Header %p\n", tmp);
        free(tmp);
        tmp = NULL;
        dmx->m_PrereadList = NULL;
        return NULL;
    }

    while(cur != NULL)
    {
        if (pos == cur->pos)
        {
            return cur->pack;
        }
        else if (pos > cur->pos)
        {
            tmp->next = cur->next;
            if (cur->pack != NULL)
            {
                if (cur->pack->data != NULL)
                {
                    mp_msg(0, MSGL_V, "Free Packet data %p\n", cur->pack->data);
                    free(cur->pack->data);
                    cur->pack->data = NULL;
                }
                mp_msg(0, MSGL_V, "Free Packet %p\n", cur->pack);
                free(cur->pack);
                cur->pack = NULL;
            }
            mp_msg(0, MSGL_V, "Free Preread List Node %p\n", cur);
            free (cur);
            cur = tmp->next;
        }
        else
        {
            return NULL;
        }
    }

    return NULL;
}


int flv_demux_open (DemuxContext* ctx, URLProtocol* h)
{
    FLVDemuxInfo* dmx;
    if (ctx == NULL || h == NULL)
    {
        mp_msg(0, MSGL_V, "flv_demux_open :: parmeter error\n");
        return -1;
    }

    dmx = (FLVDemuxInfo*)malloc(sizeof(FLVDemuxInfo));
    if (dmx == NULL)
    {
        mp_msg(0, MSGL_V, "flv_demux_open :: allocate a new demux private data failed\n");
        return -1;
    }

    ctx->priv_data_size = sizeof(FLVDemuxInfo);
    ctx->priv_data = (void *)dmx;
#ifdef _FLV_DEMUX_TEST_
    dmx->m_FileSize = 0LL;
#else
    dmx->m_FileSize = (h ? h->url_seek(h, 0, SEEK_SIZE) : 0);
#endif
    dmx->m_AVDataOffset = 0LL;
    dmx->m_Duration = 0L;
    dmx->m_CurrPos = 0LL;
    dmx->m_TagDataSize = 0L;
    dmx->m_DemuxState = FLV_DEMUX_NONE;
    dmx->m_TagTimestamp = 0L;

    dmx->m_InputInfor.data = NULL;
    dmx->m_InputInfor.size = 0;
    dmx->m_InputInfor.buflength = 0;

    dmx->m_AVPacket = NULL;

    dmx->m_Metadata = NULL;

    dmx->m_URLProtocol = h;
    dmx->m_PrereadList = NULL;

    dmx->m_IndexList.count = 0;
    dmx->m_IndexList.elems = NULL;

    mp_msg(0, MSGL_V, "flv_demux_open : OK\n");
    return 0;
}
int flv_demux_probe (DemuxContext* ctx)
{
    FLVDemuxInfo* dmx;
    int ret = 0;
    if (ctx == NULL || ctx->priv_data == NULL)
    {
        mp_msg(0, MSGL_V, "flv_demux_probe : parameter error\n");
        return -1;
    }
    dmx = (FLVDemuxInfo*)ctx->priv_data;
    dmx->m_CurrPos = 0LL;

    ret = read_file_data(dmx, 0, FLV_FILE_HEADER_SIZE);
    if (ret <= 0)
    {
        mp_msg(0, MSGL_V, "flv_demux_probe::read_file_data : failed\n");
        return ret;
    }

    ret = flv_parse_file_header(dmx);
    if (ret < 0)
    {
        mp_msg(0, MSGL_V, "flv_demux_probe::flv_parse_file_header : failed\n");
        return ret;
    }
    return ret;
}
int flv_demux_close (DemuxContext* ctx)
{
    mp_msg(0, MSGL_INFO, "int flv_demux_close (DemuxContext* ctx)\n");
    FLVDemuxInfo* dmx = NULL;
    if (ctx == NULL || ctx->priv_data == NULL)
    {
        return -1;
    }
    dmx = (FLVDemuxInfo*)ctx->priv_data;

    if (dmx == NULL)
    {
        return 0;
    }
    if (dmx->m_AVPacket != NULL)
    {
        if (dmx->m_AVPacket->data != NULL)
        {
            free (dmx->m_AVPacket->data);
            dmx->m_AVPacket->data = NULL;
        }
    }

    if (dmx->m_InputInfor.data != NULL)
    {
        free (dmx->m_InputInfor.data);
        dmx->m_InputInfor.data = NULL;
    }

    if (dmx->m_IndexList.elems != NULL)
    {
        free (dmx->m_IndexList.elems);
        dmx->m_IndexList.elems = NULL;
        dmx->m_IndexList.count = 0;
    }
    
    free(dmx);
    ctx->priv_data_size = 0;
    ctx->priv_data = NULL;

    mp_msg(0, MSGL_INFO, "flv_demux_close :: OK\n");

    destroy_demux_context(ctx);
    return 0;
}
int flv_demux_parse_metadata (DemuxContext* ctx, Metadata* meta)
{
    FLVDemuxInfo* dmx;
    int retval = 0;

    /// Check parameters and set private data
    if (ctx == NULL || ctx->priv_data == NULL || meta == NULL)
    {
        mp_msg(0, MSGL_V, "flv_demux_parse_metadata :: parameter failed\n");
        return -1;
    }
    dmx = (FLVDemuxInfo*)ctx->priv_data;
    dmx->m_Metadata = meta;
    memset(meta, 0, sizeof(Metadata));

    /// Skip file header
    retval = read_file_data(dmx, 0, FLV_FILE_HEADER_SIZE);
    if (retval <= 0)
    {
        mp_msg(0, MSGL_V, "flv_demux_parse_metadata :: read_file_data :: failed\n");
        return retval;
    }
    dmx->m_CurrPos = FLV_FILE_HEADER_SIZE;

    /// parse tags header
    retval = read_file_data(dmx, dmx->m_CurrPos, FLV_TAGS_HEADER_SIZE);
    if (retval <= 0)
    {
        mp_msg(0, MSGL_V, "flv_demux_parse_metadata :: read_file_data :: failed\n");
        return retval;
    }
    if (flv_parse_tag_header(dmx))
    {
        mp_msg(0, MSGL_V, "flv_demux_parse_metadata :: flv_parse_tag_header :: failed\n");
        return -1;
    }
    dmx->m_CurrPos += FLV_TAGS_HEADER_SIZE;

    /// Get Tag Data
    retval = read_file_data(dmx, dmx->m_CurrPos, dmx->m_TagDataSize);
    if (retval <= 0)
    {
        mp_msg(0, MSGL_V, "flv_demux_parse_metadata :: read_file_data :: failed\n");
        return retval;
    }
    dmx->m_CurrPos += dmx->m_TagDataSize;

    meta->audiocodec = -1;
    meta->subaudiocodec = -1;
    meta->videocodec = -1;
    meta->subaudiocodec = -1;

    /// Parse metadata
    if (dmx->m_DemuxState == FLV_DEMUX_MDATA)
    {
        if (flv_parse_tag_script(dmx))
        {
            mp_msg(0, MSGL_V, "flv_demux_parse_metadata :: flv_parse_tag_script :: failed\n");
            return -1;
        }

        dmx->m_AVDataOffset = dmx->m_CurrPos + FLV_TAGS_TAILER_SIZE;
    }
    /// There's no onMetadata Packet, Add current packet to preread list
    else
    {
        unsigned char flag;

        dmx->m_AVDataOffset = FLV_FILE_HEADER_SIZE;
        dmx->m_AVPacket = (AVPacket*)malloc(sizeof(AVPacket));
        if (dmx->m_AVPacket == NULL)
        {
            mp_msg(0, MSGL_V, "Allocate Packet failed\n");
            return -1;
        }
        mp_msg(0, MSGL_V, "Allocate Packet %p\n", dmx->m_AVPacket);
        memset(dmx->m_AVPacket, 0, sizeof(AVPacket));

        if (dmx->m_DemuxState == FLV_DEMUX_VIDEO)
        {
            if (flv_parse_tag_video(dmx))
            {
                mp_msg(0, MSGL_V, "flv_demux_parse_metadata :: flv_parse_tag_video :: failed\n");
                return -1;
            }
            flag = dmx->m_AVPacket->data[0] & 0x0F;
            switch(flag)
            {
            case 2:///< H.263
                meta->videocodec = CODEC_ID_H263;
                break;
            case 7:///< AVC
                meta->videocodec = CODEC_ID_H264;
                break;
            default:
                meta->videocodec = CODEC_ID_NONE;
                break;
            }
        }
        else if (dmx->m_DemuxState == FLV_DEMUX_AUDIO)
        {
            unsigned char* data = NULL;
            if (flv_parse_tag_audio(dmx))
            {
                mp_msg(0, MSGL_V, "flv_demux_parse_metadata :: flv_parse_tag_audio :: failed\n");
                return -1;
            }
            flag = (dmx->m_AVPacket->data[0] >> 4) & 0x0F;
            switch(flag)
            {
            case 2: ///< mp3
            case 14:///< mp3
                meta->audiocodec = CODEC_ID_MP3;
                break;
            case 10:///< aac
                data = (unsigned char*)malloc(dmx->m_AVPacket->bufferlength);
                memcpy(data, dmx->m_AVPacket->data, dmx->m_AVPacket->bufferlength);
                meta->audiocodec = CODEC_ID_AAC;
                meta->subaudiocodec = flv_demux_get_aac_sub_type(data);
                free(data);
                if (meta->subaudiocodec == -1)
                {
                    mp_msg(0, MSGL_V,\
                        "flv_demux_parse_metadata :: flv_demux_get_aac_sub_type :: failed\n");
                    return -1;
                }
                break;
            default:
                meta->audiocodec = CODEC_ID_NONE;
                break;
            }
        }

        add_a_preread_packet(ctx, dmx->m_AVPacket, dmx->m_AVDataOffset);
        dmx->m_AVPacket = NULL;
    }

    /// Skip tags tailer
    retval = read_file_data(dmx, dmx->m_CurrPos, FLV_TAGS_TAILER_SIZE);
    if (retval <= 0)
    {
        mp_msg(0, MSGL_V, "flv_demux_parse_metadata :: read_file_data :: failed\n");
        return -1;
    }
    dmx->m_CurrPos += FLV_TAGS_TAILER_SIZE;


    while (meta->audiocodec == -1\
        || (meta->audiocodec == CODEC_ID_AAC && meta->subaudiocodec == -1)\
        || meta->videocodec == -1)
    {
        unsigned char flag;
        long long pos = dmx->m_CurrPos;

        /// Parse Tag Header
        retval = read_file_data(dmx, dmx->m_CurrPos, FLV_TAGS_HEADER_SIZE);
        if (retval <= 0)
        {
            mp_msg(0, MSGL_V, "flv_demux_parse_metadata :: read_file_data :: failed\n");
            return -1;
        }
        if (flv_parse_tag_header(dmx))
        {
            mp_msg(0, MSGL_V, "flv_demux_parse_metadata :: flv_parse_tag_header :: failed\n");
            return -1;
        }
        dmx->m_CurrPos += FLV_TAGS_HEADER_SIZE;

        retval = read_file_data(dmx, dmx->m_CurrPos, dmx->m_TagDataSize);
        if (retval <= 0)
        {
            mp_msg(0, MSGL_V, "flv_demux_parse_metadata :: read_file_data :: failed\n");
            return -1;
        }
        dmx->m_CurrPos += dmx->m_TagDataSize;

        /// Add a preread AV packet
        dmx->m_AVPacket = (AVPacket*)malloc(sizeof(AVPacket));
        if (dmx->m_AVPacket == NULL)
        {
            mp_msg(0, MSGL_V, "Allocate Packet failed\n");
            return -1;
        }
        mp_msg(0, MSGL_V, "Allocate Packet %p\n", dmx->m_AVPacket);
        memset(dmx->m_AVPacket, 0, sizeof(AVPacket));

        if (dmx->m_DemuxState == FLV_DEMUX_VIDEO)
        {
            if (flv_parse_tag_video(dmx))
            {
                mp_msg(0, MSGL_V, "flv_demux_parse_metadata :: flv_parse_tag_video :: failed\n");
                return -1;
            }
            flag = dmx->m_AVPacket->data[0] & 0x0F;
            switch(flag)
            {
            case 2:///< H.263
                meta->videocodec = CODEC_ID_H263;
                break;
            case 7:///< AVC
                meta->videocodec = CODEC_ID_H264;
                break;
            default:
                meta->videocodec = CODEC_ID_NONE;
                break;
            }
        }
        else if (dmx->m_DemuxState == FLV_DEMUX_AUDIO)
        {
            unsigned char* data;
            if (flv_parse_tag_audio(dmx))
            {
                mp_msg(0, MSGL_V, "flv_demux_parse_metadata :: flv_parse_tag_audio :: failed\n");
                return -1;
            }
            flag = (dmx->m_AVPacket->data[0] >> 4) & 0x0F;
            switch(flag)
            {
            case 2: ///< mp3
            case 14:///< mp3
                meta->audiocodec = CODEC_ID_MP3;
                break;
            case 10:///< aac
                data = (unsigned char*)malloc(dmx->m_AVPacket->bufferlength);
                memcpy(data, dmx->m_AVPacket->data, dmx->m_AVPacket->bufferlength);
                meta->audiocodec = CODEC_ID_AAC;
                meta->subaudiocodec = flv_demux_get_aac_sub_type(data);
                free(data);
                if (meta->subaudiocodec == -1)
                {
                    mp_msg(0, MSGL_V,\
                        "flv_demux_parse_metadata :: flv_demux_get_aac_sub_type :: failed\n");
                    return -1;
                }
                break;
            default:
                meta->audiocodec = CODEC_ID_NONE;
                break;
            }
        }
        else
        {
            mp_msg(0, MSGL_V, "flv_demux_parse_metadata :: unexpected tag\n");
            return -1;
        }
        add_a_preread_packet(ctx, dmx->m_AVPacket, pos);
        dmx->m_AVPacket = NULL;

        /// Skip tag tailer
        retval = read_file_data(dmx, dmx->m_CurrPos, FLV_TAGS_TAILER_SIZE);
        if (retval <= 0)
        {
            mp_msg(0, MSGL_V, "flv_demux_parse_metadata :: read_file_data :: failed\n");
            return -1;
        }
        dmx->m_CurrPos += FLV_TAGS_TAILER_SIZE;
    }


    mp_msg(0, MSGL_V, "MetaData : \n");
    mp_msg(0, MSGL_V, "\tmeta->audiocodec = %d\n", meta->audiocodec);
    mp_msg(0, MSGL_V, "\tmeta->subaudioID = %d\n", meta->subaudiocodec);
    mp_msg(0, MSGL_V, "\tmeta->videocodec = %d\n", meta->videocodec);
    mp_msg(0, MSGL_V, "\tmeta->subvideoID = %d\n", meta->subvideocodec);
    mp_msg(0, MSGL_V, "\tmeta->duation    = %d\n", meta->duation);
    mp_msg(0, MSGL_V, "flv_demux_parse_metadata :: OK\n");

    /// Clear private packet and mark AV data position
    dmx->m_CurrPos = dmx->m_AVDataOffset;
    dmx->m_Metadata = NULL;
    return 0;
}
int flv_demux_read_packet (DemuxContext* ctx, AVPacket* pack)
{
    //mp_msg(0, MSGL_V, "flv_demux_read_packet :: MetaData : \n");
    FLVDemuxInfo* dmx;
    AVPacket* prepack;
    int ret;

    /// Check parameters and set private data
    if (ctx == NULL || ctx->priv_data == NULL || pack == NULL)
    {
        mp_msg(0, MSGL_V, "flv_demux_read_packet :: parameters error\n");
        return -1;
    }
    dmx = (FLVDemuxInfo*)ctx->priv_data;
    prepack = NULL;
    
    /// if read a preread packet, no need to skip
    if (dmx->m_PrereadList != NULL)
    {
        prepack = check_preread_tags(dmx, dmx->m_CurrPos);
    }
    if (prepack != NULL)
    {
        pack->bufferlength = prepack->bufferlength;
        pack->size = prepack->size;
        pack->pts  = prepack->pts;
        pack->stream_index = prepack->stream_index;
        if (pack->data != NULL)
        {
            free (pack->data);
            mp_msg(0, MSGL_V, "Free Packet Data %p LENS %d\n", pack->data, pack->bufferlength);
        }
        pack->data = prepack->data; ///< pure data is given packet parameter
        prepack->data = NULL;       ///< and then i set pre-read pakcet data as null and i left it

        dmx->m_CurrPos += (FLV_TAGS_HEADER_SIZE + pack->size + FLV_TAGS_TAILER_SIZE);

        mp_msg(0, MSGL_DBG2\
            , "flv_demux_read_packet :: Type = %-2d Pts = %-6lld\t Size = %-6d\t Pos = %-8lld\n"\
            , pack->stream_index, pack->pts, pack->size, dmx->m_CurrPos);

        return pack->size;
    }

    dmx->m_AVPacket = pack;
    /// parse tags header
    ret = read_file_data(dmx, dmx->m_CurrPos, FLV_TAGS_HEADER_SIZE);
    if (ret <= 0)
    {
        mp_msg(0, MSGL_V, "flv_demux_read_packet :: read_file_data :: failed\n");
        return ret;
    }

    if (flv_parse_tag_header(dmx) < 0)
    {
        int i = 0;
        mp_msg (0, MSGL_V\
            , "flv_demux_read_packet :: flv_parse_tag_header :: Not Support Tag, POS = %lld\n"\
            , dmx->m_CurrPos - FLV_TAGS_HEADER_SIZE);
        mp_msg (0, MSGL_V, "flv_demux_read_packet :: Error Data : \n");
        while (i < 11)
        {
            mp_msg(0, MSGL_V, "0x%02X ", dmx->m_InputInfor.data[i]);
            ++i;
        }
        mp_msg (0, MSGL_V, "\n");
        return -1;
    }
    dmx->m_CurrPos += FLV_TAGS_HEADER_SIZE;

    while (dmx->m_DemuxState == FLV_DEMUX_MDATA)
    {
        ret = read_file_data (dmx, dmx->m_CurrPos, dmx->m_TagDataSize + FLV_TAGS_TAILER_SIZE);
        if (ret <= 0)
        {
            mp_msg (0, MSGL_V, "flv_demux_read_packet :: read_file_data :: fail");
            return -1;
        }
        dmx->m_CurrPos += (dmx->m_TagDataSize + FLV_TAGS_TAILER_SIZE);

        ret = read_file_data (dmx, dmx->m_CurrPos, FLV_TAGS_HEADER_SIZE);
        if (ret <= 0)
        {
            mp_msg (0, MSGL_V, "flv_demux_read_packet :: read_file_data :: fail");
            return -1;
        }
        dmx->m_CurrPos += (FLV_TAGS_HEADER_SIZE);
        if (flv_parse_tag_header(dmx) < 0)
        {
            int i = 0;
            mp_msg (0, MSGL_V\
                , "flv_demux_read_packet :: flv_parse_tag_header :: Not Support Tag, POS = %lld\n"\
                , dmx->m_CurrPos - FLV_TAGS_HEADER_SIZE);
            mp_msg (0, MSGL_V, "flv_demux_read_packet :: Error Data : ");
            while (i < 11)
            {
                mp_msg(0, MSGL_V, "0x%02X ", dmx->m_InputInfor.data[i]);
                ++i;
            }
            mp_msg (0, MSGL_V, "\n");
        }
    }

    /// Get and parse AV tag data
    ret = read_file_data(dmx, dmx->m_CurrPos, dmx->m_TagDataSize);
    if (ret <= 0)
    {
        mp_msg(0, MSGL_V, "flv_demux_read_packet :: read_file_data :: failed\n");
        return ret;
    }
    if (dmx->m_DemuxState == FLV_DEMUX_AUDIO)
    {
        if (flv_parse_tag_audio(dmx))
        {
            mp_msg(0, MSGL_V, "flv_demux_read_packet :: flv_parse_tag_audio :: failed\n");
            return -1;
        }
    }
    else if (dmx->m_DemuxState == FLV_DEMUX_VIDEO)
    {
        if (flv_parse_tag_video(dmx))
        {
            mp_msg(0, MSGL_V, "flv_demux_read_packet :: flv_parse_tag_video :: failed\n");
            return -1;
        }
    }
    else
    {
        mp_msg(0, MSGL_V, "flv_demux_read_packet :: Unanalyzable tag type");
        return -1;
    }
    dmx->m_CurrPos += dmx->m_TagDataSize;

    /// Skip tag tailer
    ret = read_file_data(dmx, dmx->m_CurrPos, FLV_TAGS_TAILER_SIZE);
    if (ret <= 0)
    {
        mp_msg(0, MSGL_V, "flv_demux_read_packet :: read_file_data :: failed\n");
        return -1;
    }
    dmx->m_CurrPos += FLV_TAGS_TAILER_SIZE;

    mp_msg(0, MSGL_DBG2\
        , "flv_demux_read_packet :: Type = %d Pts = %-8lld\t Size = %-6ld\t Pos = %-8lld\n"\
        , pack->stream_index, pack->pts, pack->size, dmx->m_CurrPos);

    /// Clear private data
    dmx->m_AVPacket = NULL;
    return pack->size;
}
long long flv_demux_seek (DemuxContext* ctx, long long ts)
{
    FLVDemuxInfo* dmx   = NULL;
    FLVIndex* index = NULL;
    int count = 0;

    /// Check parameters and check if this is a live stream
    if (ctx == NULL || ctx->priv_data == NULL)
    {
        mp_msg(0, MSGL_V, "flv_demux_seek :: parameters error\n");
        return -1;
    }
    dmx = (FLVDemuxInfo*)ctx->priv_data;
#   ifndef _FLV_DEMUX_TEST_
    if (dmx->m_URLProtocol->url_is_live != NULL\
        && (1 == dmx->m_URLProtocol->url_is_live(dmx->m_URLProtocol)))
    {
        mp_msg(0, MSGL_V, "flv_demux_seek :: live stream cannot seek\n");
        return -1;
    }
#   endif

    mp_msg(0, MSGL_V, "flv_demux_seek :: current pos = %lld\n", dmx->m_CurrPos);

    index = dmx->m_IndexList.elems;
    count = dmx->m_IndexList.count;
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
                if(validtags >= 5 && havevideo == 1)
                {
                    if (haveavcdat == 1)
                    {
                        if (keyfrmpos != -1)
                        {
                            dmx->m_CurrPos = keyfrmpos;
                            mp_msg(0, MSGL_V, "flv_demux_seek :: seek to pos = %lld\n", keyfrmpos);
#                           ifdef _FLV_DEMUX_TEST_
                            read_file_data(dmx, dmx->m_CurrPos, FLV_TAGS_HEADER_SIZE);
                            flv_parse_tag_header(dmx);
                            mp_msg (0, MSGL_V, "flv_demux_seek :: seek to tms = %l\n", dmx->m_TagTimestamp);
#                           endif
                            return keyfrmpos;
                        }
                    }
                    else
                    {
                        mp_msg(0, MSGL_V, "flv_demux_seek :: seek to pos = %lld\n", dmx->m_CurrPos);
                        return dmx->m_CurrPos;
                    }
                }

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

DemuxContextHelper flv_demux_helper = {
    "flv",
    can_handle,
    create_demux_context,
    destroy_demux_context,
    0,
    0
};

int can_handle(int fileformat)
{
    if (fileformat == FILEFORMAT_ID_FLV)
    {
        mp_msg(0, MSGL_V, "flv_demux_can_handle : OK\n");
        return 0;
    }
    mp_msg(0, MSGL_V, "flv_demux_can_handle : This format cannot handle\n");
    return -1;
}

static DemuxContext * create_demux_context()
{
    DemuxContext * ctx = calloc(1, sizeof(DemuxContext));
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
        ctx->next = flv_demux_helper.priv_data;
        flv_demux_helper.priv_data = ctx;
    }

    return ctx;
}

static void destroy_demux_context(DemuxContext * ctx)
{
    //remove the instance from the global instance list
    DemuxContext * cur = flv_demux_helper.priv_data;
    DemuxContext * prev = cur;
    while(cur && cur != ctx)
    {
        prev = cur;
        cur = cur->next;
    }
    if(cur == ctx)
    {
        prev->next = cur->next;
    }

    //destroy DemuxContext only, it's private data should be cleared in flv_demux_close
    free(ctx);
}

