#ifdef _FLV_DEMUX_TEST_

#include "demux.h"
#include "logger.h"
#include "datIO.h"
#include "DEMUX\flv_demux.h"
#include "DATIO\file.h"


int main()
{
    URLProtocol*  h = CreateURLProtocol();
    DemuxContext* c = (DemuxContext*)malloc(sizeof(DemuxContext));
    Metadata*    meta = (Metadata*)malloc(sizeof(Metadata));
    AVPacket*    pack = (AVPacket*)malloc(sizeof(AVPacket));
    memset (meta, 0, sizeof(*meta));
    memset (pack, 0, sizeof(*pack));

    h->url_open(h, "D:\\Private\\WorkArea\\FLVDemux\\flvdemux_src_version2\\test.flv"\
        , 0, NULL);

    do
    {
        /// Test Open
        if (flv_demux_open (c, h))
        {
            break;
        }
        /// Test Parse Metadata
        if (flv_demux_parse_metadata(c, meta))
        {
            break;
        }
//        /// Test Packet Reading
//        while ((ret = flv_demux_read_packet(c, pack)) > 0);
//        if (ret < 0)
//        {
//            break;
//        }

        /// Test Seek (Contain Index & Exclusive Index)
        /// Seek To 24s
        if (flv_demux_seek(c, 24000) < 0)
        {
            break;
        }
        if (flv_demux_read_packet(c, pack) < 0)
        {
            break;
        }
        /// Seek To 50s
        if (flv_demux_seek(c, 50000) < 0)
        {
            break;
        }
        if (flv_demux_read_packet(c, pack) < 0)
        {
            break;
        }
        /// Seek To 100s
        if (flv_demux_seek(c, 100000) < 0)
        {
            break;
        }
        if (flv_demux_read_packet(c, pack) < 0)
        {
            break;
        }
        /// Seek To 0s
        if (flv_demux_seek(c, 0) < 0)
        {
            break;
        }
        if (flv_demux_read_packet(c, pack) < 0)
        {
            break;
        }
        /// Seek To 10000s
        if (flv_demux_seek(c, 10000000) < 0)
        {
            break;
        }
        if (flv_demux_read_packet(c, pack) < 0)
        {
            break;
        }


//        /// Test Parse Codec From Raw Data
//        h->url_seek(h, 0, SEEK_SET);
//        h->url_read(h, rawdata, datasize);
//        meta->audiocodec = -1;
//        meta->videocodec = -1;
//        if (flv_demux_parse_codec_from_raw_data(rawdata, datasize, meta) < 0)
//        {
//            break;
//        }
//        if (flv_demux_close(c) < 0)
//        {
//            break;
//        }
    }while (0);


    printf ("DEMUX ################ test_routine Test Completely\n");
    //fgetc(stdin);
    return 0;
}
#endif
