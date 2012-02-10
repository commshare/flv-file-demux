#include "demux.h"

#ifndef NULL
#define NULL (void*)0
#endif

static DemuxContextHelper * g_demuxHelpers = 0;
///
void registerDemuxContextHelper(DemuxContextHelper *helper)
{
    if(!g_demuxHelpers)
    {
        g_demuxHelpers = helper;
        g_demuxHelpers->next = 0;
    }
    else
    {
        helper->next = g_demuxHelpers;
        g_demuxHelpers = helper;
    }
}

DemuxContext * getDemuxByFileFormat(int fileformat)
{
    DemuxContextHelper * helper = g_demuxHelpers;
    while(helper)
    {
        if( 0 == helper->can_handle(fileformat) )
        {
            return helper->create_demux_context();
        }

        helper = helper->next;
    }
    return NULL;
}

DemuxContext * getDemuxByProbe(URLProtocol *h)
{
    DemuxContextHelper * helper = g_demuxHelpers;
    while(helper)
    {
        DemuxContext * ctx = helper->create_demux_context();
        if( 0 == ctx->demux_open(ctx, h) )
        {
            if( 0 == ctx->demux_probe(ctx) )
            {
                ctx->demux_close(ctx);
                return helper->create_demux_context();
            }
            ctx->demux_close(ctx);
        }
        helper = helper->next;
    }
    return NULL;
}
