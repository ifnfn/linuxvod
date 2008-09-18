TARGET_TYPE := EXECUTABLE
MAIN_SRC = pushplayer.c \

ENABLE_NETPLAYER=no
ENABLE_OSDMENU=no

COMPILKIND  +=release

CC = gcc

LIBSRC = \
	config.c \
	showjpeg.c \
	songdata.c \
	callbacktable.c \
	playcontrol.c

SRC:= \

ifeq ($(ENABLE_NETPLAYER), yes)
	RMCFLAGS += -DNETPLAYER
	SRC += $(LIBSRC)
else
	LDFLAGS += -lplayctrl
#	SRC += $(LIBSRC)
endif

export COMPILKIND += withthreads withdl
USE_STD_LIB=1
LOCAL_MAKEFILE := Makefile
RMCFLAGS += -I../ -I../../lib
RMCFLAGS += -I/RMF/RMF_src/externalapi/rmf -I/RMF/RMF_src/externalapi/rmproperties/include -DDATAPATH=\"/ktvdata\"
RMCFLAGS += -I/RMF/RUA_src/realmagichwl/include -g
LDFLAGS  += -L/RMF/lib -lrmcaribbeanapi -ljpeg -lm -L. -L../../lib -lktv
LDFLAGS  += -g

ifeq ($(ENABLE_OSDMENU), yes)
        RMCFLAGS += -DOSDMENU
	SRC +=osd.c
	RMCFLAGS += `sdl-config --cflags` `freetype-config --cflags`
	LDFLAGS  += `sdl-config --libs` `freetype-config --libs`
endif

include /RMF/RMF_src/scripts/inc.Makefile


LIBOBJS=$(addprefix objects/, $(addsuffix .o, $(basename $(notdir $(LIBSRC)))))

LIBSO=libplayctrl.so

DEPEND_FILE += -r \
	deps \
	objects \
	$(LIBSO)

so: deps $(LIBSO)

LIBCFLAGS=$(RMCFLAGS) -I. -D_REENTRANT -DWITH_THREADS=1 -U_DEBUG  -Wundef -Wall -Wchar-subscripts -Wmissing-prototypes -Wsign-compare -Wuninitialized -O -Werror  -Wnested-externs -Wmissing-declarations

deps: $(LIBSRC) Makefile
	@for f in $(LIBSRC); do \
		OBJ=objects/`basename $$f|sed -e 's/\.cpp/\.o/' -e 's/\.cxx/\.o/' -e 's/\.c/\.o/'`; \
		echo $$OBJ: $$f>> deps; \
		echo '	$(CC) $$(LIBCFLAGS) -c -o $$@ $$^'>> deps; \
	done

-include ./deps

objects:
	mkdir objects

$(LIBSO): objects $(LIBOBJS)
	$(CC) --share $(LIBOBJS) -L../../lib -lktv -o $@ -s

md5:
	@md5sum -b pushplayer | tr a-z A-Z
