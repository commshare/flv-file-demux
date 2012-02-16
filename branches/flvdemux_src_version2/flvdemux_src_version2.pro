DEFINES += _FLV_DEMUX_TEST_

HEADERS += \
    src/flv_parse.h \
    src/flv_demux.h \
    src/byte_parse.h \
    src/amf_parse.h \
    urlprotocol.h \
    mp_msg.h \
    demux.h \
    commonplaytype.h \
    avformat.h

SOURCES += \
    src/flv_parse.c \
    src/flv_demux.c \
    src/byte_parse.c \
    src/amf_parse.c \
    test.c
