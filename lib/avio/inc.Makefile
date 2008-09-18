ifndef CONFIGDEFINE
CONFIGDEFINE   =
CONFIGDEFINE  += -DFILE_PROTO
CONFIGDEFINE  += -DUDP_PROTO
CONFIGDEFINE  += -DTCP_PROTO
CONFIGDEFINE  += -DHTTP_PROTO
CONFIGDEFINE  += -DRTP_PROTO
endif

SRC += utils.c avio.c fifo.c aviobuf.c

ifeq ($(findstring -DRTP_PROTO, $(CONFIGDEFINE)), -DRTP_PROTO)
	CFLAGS += -DRTP_PROTO
	SRC += rtpproto.c
endif
ifeq ($(findstring -DFILE_PROTO, $(CONFIGDEFINE)), -DFILE_PROTO)
	CFLAGS += -DFILE_PROTO
	SRC += file.c
endif

ifeq ($(findstring -DUDP_PROTO, $(CONFIGDEFINE)), -DUDP_PROTO)
	CFLAGS += -DUDP_PROTO
	SRC += udp.c
endif

ifeq ($(findstring -DTCP_PROTO, $(CONFIGDEFINE)), -DTCP_PROTO)
	CFLAGS += -DTCP_PROTO
	SRC += tcp.c
endif

ifeq ($(findstring -DHTTP_PROTO, $(CONFIGDEFINE)), -DHTTP_PROTO)
	CFLAGS +=-DHTTP_PROTO
	SRC += http.c
endif

-include ../../inc.Makefile
