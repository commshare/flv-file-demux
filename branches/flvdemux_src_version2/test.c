#include "mp_msg.h"
#include "urlprotocol.h"
#include "src/flv_demux.h"

typedef struct URLPrivData
{
    FILE* fp;
    UI64  filesize;
    UI64  currpos;
}URLPrivData;

extern void mp_msg(int mod, int lev, const char* format, ...)
{
    va_list va;
    char    tmp[MSGSIZE_MAX];

    va_start(va, format);
    vsnprintf(tmp, MSGSIZE_MAX, format, va);
    va_end(va);
    tmp[MSGSIZE_MAX - 2] = '\n';
    tmp[MSGSIZE_MAX - 1] = 0;

    printf("%s", tmp);
    fflush(stdout);
}

int url_open (URLProtocol *h, const char* path, int flags, void* quit)
{
    URLPrivData* d = NULL;
    h->priv_data = d = (URLPrivData*)malloc(sizeof(URLPrivData));
    d->currpos   = 0ULL;
    d->filesize  = 0ULL;
    d->fp = fopen(path, "rb+");
    fseek(d->fp, 0, SEEK_END);
    d->filesize = ftell(d->fp);
    fseek(d->fp, 0, SEEK_SET);

    h->priv_data_size = sizeof(URLPrivData);
    return 0;
}

int url_close(URLProtocol* h)
{
    URLPrivData* d = (URLPrivData*)h->priv_data;
    fclose(d->fp);
    d->fp = NULL;
    return 0;
}

int url_read (URLProtocol *h, unsigned char *buf, int size)
{
    URLPrivData* d = (URLPrivData*)h->priv_data;
    int ret = fread(buf, 1, size, d->fp);
    d->currpos += ret;
    return ret;
}

long long url_seek (URLProtocol *h, long long pos, int whence)
{
    URLPrivData* d = (URLPrivData*)h->priv_data;
    int ret ;

    if (whence == SEEK_SIZE)
    {
        return (long long)d->filesize;
    }

    ret = fseek(d->fp, (long)pos, whence);
    return (long long)ret;
}

int url_is_live (URLProtocol* h)
{
    h = NULL;
    return 0;
}

URLProtocol* CreateURLProtocol ()
{
    URLProtocol* h = (URLProtocol*)malloc(sizeof(URLProtocol));

    h->url_open         = url_open;
    h->url_close        = url_close;
    h->url_read         = url_read;
    h->url_write        = NULL;
    h->url_seek         = url_seek;
    h->url_is_live      = url_is_live;
    h->url_can_handle   = NULL;
    h->url_time_seek    = NULL;
    h->name             = "file";
    h->next             = NULL;
    h->priv_data        = NULL;
    h->priv_data_size   = 0;

    return h;
}

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
        if (flv_demux_open (c, h))
        {
            break;
        }
        if (flv_demux_parse_metadata(c, meta))
        {
            break;
        }
        while (flv_demux_read_packet(c, pack))
        {
            break;
        }
        flv_demux_close(c);
        break;
    }while (0);

    return 0;
}