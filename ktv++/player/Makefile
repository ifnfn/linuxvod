TARGET_TYPE := EXECUTABLE
MAIN_SRC = pushplayer.c \
	   847xjpeg.c \

ifndef CONFIGDEFINE
CONFIGDEFINE  =
#CONFIGDEFINE  += OSDMENU
#CONFIGDEFINE  += NETPLAYER 
endif

COMPILKIND  +=release

CC = g++
SRC:= \
	config.c \
	showjpeg.c \
	songdata.c \
	playcontrol.c \
	

ifeq ($(findstring NETPLAYER, $(CONFIGDEFINE)), NETPLAYER)
        RMCFLAGS += -DNETPLAYER
endif  

export COMPILKIND += withthreads withdl
USE_STD_LIB=1
LOCAL_MAKEFILE := Makefile
RMCFLAGS += -I../ -I../../lib
RMCFLAGS += -I/RMF/RMF_src/externalapi/rmf -I/RMF/RMF_src/externalapi/rmproperties/include -DDATAPATH=\"/ktvdata\"
RMCFLAGS += -I/RMF/RUA_src/realmagichwl/include
LDFLAGS  += -L/RMF/lib -lrmcaribbeanapi -ljpeg -lm -L../../lib -lktv
LDFLAGS  += -s

ifeq ($(findstring OSDMENU, $(CONFIGDEFINE)), OSDMENU)
        RMCFLAGS += -DOSDMENU
	SRC +=osd.c
	RMCFLAGS += `sdl-config --cflags` `freetype-config --cflags`
	LDFLAGS  += `sdl-config --libs` `freetype-config --libs`
endif

include /RMF/RMF_src/scripts/inc.Makefile

md5:
	@md5sum -b pushplayer | tr a-z A-Z
