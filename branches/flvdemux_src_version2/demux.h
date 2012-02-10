#ifndef _DEMUX_H_
#define _DEMUX_H_
#include "urlprotocol.h"
#include "avformat.h"

typedef struct DemuxContext {
    /*
     * a short name for the demux, such as flv,mp4...
     */
    const char *name;
    /*
     * open this demux
     * return 0 when opened success; -1 on error;
     *
     * MUST implement
     */
    int (*demux_open)(struct DemuxContext *ctx, URLProtocol *h);
    /*
     * check whether this demux can handle the media specified by the URLProtocol
     * return 0 when it can handle the media; else return -1;
     *
     * this function can read the metadata to analyze the fileformat
     *
     * SHOULD implement
     *
     * should be called after demux_open
     */
    int (*demux_probe)(struct DemuxContext *ctx);
    /*
     * close the demux
     * return 0 when closed; -1 (or other error code) on error
     *
     * MUST implement
     */
    int (*demux_close)(struct DemuxContext *ctx);
    /*
     * return 0 on success, -1  (or other error code) on error
     *
     * MUST implement
     */
    int (*demux_parse_metadata)(struct DemuxContext *ctx, Metadata *meta);
    /*
     * read a packet
     *
     * return the bytes written; -1 (or other error code) on error; 0 file is end
     */
    int (*demux_read_packet)(struct DemuxContext *ctx, AVPacket * packet);
    /*
     * seek to the timestamp
     *
     * timestamp milliseconds
     *
     * return >=0 on success(the real timestamp seeked to); -1 on error
     *
     * MUST implement
     */
    long long (*demux_seek)(struct DemuxContext *ctx, long long timestamp);

    struct DemuxContext *next;
    /* for demux to save private data
     * should be initialized when demux_open was called
     * should be released when demux_close was called
     */
    int priv_data_size;
    void *priv_data;
}DemuxContext;

typedef struct DemuxContextHelper {
    /*
     * a short name for the demux, such as flv,mp4...
     */
    const char *name;
    /*
     * check whether this demux can handle the container format
     * return 0 when it can handle ; else return -1;
     *
     * MUST implement
     */
    int (*can_handle)(int fileformat);
    /*
     * create an instance of DemuxContext
     *
     * MUST implement
     */
    DemuxContext * (*create_demux_context)();
    /*
     * destroy an instance of DemuxContext
     *
     * MUST implement
     */
    void (*destroy_demux_context)(DemuxContext * context);
    /*
     * MUST not be changed after initialise, this field will be used by outside controller
     */
    struct DemuxContextHelper * next;
    /* for DemuxContextHelper to save private data, an instance list is recommend
     * for keep track of all instance of DemuxContext
     *
     * when DemuxContext::demux_close() was called, the instance should be freed and
     * remove from the instance list
     */
    void * priv_data;

}DemuxContextHelper;

/*
 * helper's life circle should be same as the app.
 *
 */
void registerDemuxContextHelper(DemuxContextHelper * helper);
DemuxContext * getDemuxByFileFormat(int fileformat);
DemuxContext * getDemuxByProbe(URLProtocol * h);

#endif // _DEMUX_H_
