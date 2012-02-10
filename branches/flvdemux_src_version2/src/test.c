
#include "flv_demux.h"
#include "../mp_msg.h"


int main()
{
/*
    int ret;
    unsigned char data[2048];
    int size;
    Metadata* meta = (Metadata*)malloc(sizeof(Metadata));
    FILE* fp;

    memset(meta, 0, sizeof(Metadata));

    fp = fopen("./test.flv", "rb+");
    if (fp == NULL)
    {
        printf ("open file failed\n");
        return 0;
    }
    size = fread(data, 1, 1024, fp);

    ret = flv_demux_parse_codec_from_raw_data(data, size, meta);

    printf ("ret = %d\n", ret);

    if (ret == 0)
    {
        printf("Audio_Codec : %d\n", meta->audiocodec);
        printf("Video_Codec : %d\n", meta->videocodec);
    }
*/
    DemuxContext* ctx = (DemuxContext*)malloc(sizeof(DemuxContext));
    URLProtocol* h   = (URLProtocol*)malloc(sizeof(URLProtocol));
    Metadata* meta   = (Metadata*)malloc(sizeof(Metadata));
    AVPacket* pack   = (AVPacket*)malloc(sizeof(AVPacket));
    FLVDemuxInfo* info;
    int ret = 0;
    long ts;

    ret = flv_demux_open(ctx, h);

    info = (FLVDemuxInfo*)ctx->priv_data;

    ret = flv_demux_probe(ctx);

    ret = flv_demux_parse_metadata(ctx, meta);

    printf ("Index Count = %lu\n", info->m_IndexList.count);

    if (info->m_IndexList.count != 0)
    {
        unsigned long i = 0;
        printf ("Index Infors :\n");
        while (i < info->m_IndexList.count)
        {
/*
            printf ("\tIndex_%-5d : Times = %lld  \tPos = %lld\n", i, info->m_IndexList.elems[i].ts\
                , info->m_IndexList.elems[i].pos);
*/
            ++i;
        }
    }

    pack->bufferlength = 0;
    pack->data = NULL;
    pack->size = 0;
    pack->stream_index = 0x00;
    pack->pts = 0LL;

    while (1)
    {
        if (flv_demux_read_packet(ctx, pack) <= 0)
        {
            break;
        }
/*
        printf ("AV = %d PTS = %-10lld DAT = %p SIZE = %-5d BUF = %-5d DATA = 0x%02X 0x%02X 0x%02X\n"\
            , pack->stream_index, pack->pts, pack->data, pack->size, pack->bufferlength\
            , (unsigned char)pack->data[0], (unsigned char)pack->data[1]\
            , (unsigned char)pack->data[2]);
*/
    }

    ts = 3000;
    mp_msg(0, MSGL_V, "testroutine :: require seek to ts = %ld\n", ts);
    flv_demux_seek(ctx, ts);


/*
    free (pack->data);
    free (pack);
    free (meta);
    free (h);
    free (ctx);
*/



    flv_demux_close(ctx);
    return 0;
}
