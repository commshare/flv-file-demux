DEFINES += _FLV_DEMUX_TEST_
DEFINES += _CRT_SECURE_NO_WARNINGS
HEADERS += \
    SRC/DEMUX/flv_parse.h \
    SRC/DEMUX/flv_demux.h \
    SRC/DEMUX/amf_parse.h \
    SRC/DATIO/file.h \
    SRC/logger.h \
    SRC/format.h \
    SRC/demux.h \
    SRC/datIO.h 

SOURCES += \
    SRC/DEMUX/flv_parse.c \
    SRC/DEMUX/flv_demux.c \
    SRC/DEMUX/amf_parse.c \
    SRC/DATIO/file.c \
    SRC/logger.c \
    SRC/testRoutine.c

