LIBSO=libts-0.0.so.0.1.0

SUBDIRS = 

prefix = /usr/local
PLUGIN_DIR = $(prefix)/share/ts/plugins
TS_CONF = $(prefix)/etc/ts.conf

CFLAGS +=-DPLUGIN_DIR=\"$(PLUGIN_DIR)\" -DTS_CONF=\"$(TS_CONF)\"

SRC = \
	ts_attach.c      \
	ts_config.c      \
	ts_fd.c          \
	ts_open.c        \
	ts_read.c        \
	ts_close.c       \
	ts_error.c       \
	ts_load_module.c \
	ts_parse_vars.c  \
	ts_read_raw.c    \

CLEANFILE+=

-include ../../inc.Makefile

install:
	strop $(LIBSO)
	cp $(LIBSO) /usr/local/lib/$(LIBS) -arf

