DEFINES += _FLV_DEMUX_TEST_

HEADERS += \
    DEMUX/flv_parse.h \
    DEMUX/flv_demux.h \
    DEMUX/amf_parse.h \
    logger.h \
    format.h \
    demux.h \
    datIO.h \
    DATIO/file.h

SOURCES += \
    DEMUX/flv_parse.c \
    DEMUX/flv_demux.c \
    DEMUX/amf_parse.c \
    testRoutine.c \
    logger.c \
    DATIO/file.c

