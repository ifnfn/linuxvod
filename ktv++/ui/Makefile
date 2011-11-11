TOPDIR=../../

LIBS += -ltinyxml -lktv -lnetio -static-libgcc -L../../lib -L.
CPPFLAGS += -I../../lib -g
BIN = mtv
SRC =

#CLEANFILE+=libstdc++.a

ifndef CONFIGDEFINE
CONFIGDEFINE  =
#CONFIGDEFINE += TSLIB
#CONFIGDEFINE += SDLGUI
#CONFIGDEFINE += NEWMOUSE
CONFIGDEFINE += ENABLE_HANDWRITE
#CONFIGDEFINE += CONSOLEINPUT
CONFIGDEFINE += DIRECTFB
#CONFIGDEFINE += SPHEGUI
#CONFIGDEFINE += MEMTEST
CONFIGDEFINE  += ENABLE_MULTLANGIAGE
endif

ifeq ($(findstring ENABLE_MULTLANGIAGE, $(CONFIGDEFINE)), ENABLE_MULTLANGIAGE)
	CPPFLAGS += -DENABLE_MULTLANGIAGE
endif

ifeq ($(findstring ENABLE_FOODMENU, $(CONFIGDEFINE)), ENABLE_FOODMENU)
	CPPFLAGS += -DENABLE_FOODMENU
endif

ifeq ($(findstring MEMTEST, $(CONFIGDEFINE)), MEMTEST)
	SRC+= memwatch.c
	CPPFLAGS += -DMEMWATCH -DMW_STDIO -DMW_NOCPP
endif

ifeq ($(findstring CONSOLEINPUT, $(CONFIGDEFINE)), CONSOLEINPUT)
	CPPFLAGS += -DCONSOLEINPUT
endif

ifeq ($(findstring DIRECTFB, $(CONFIGDEFINE)), DIRECTFB)
	CPPFLAGS +=-DDIRECTFB `directfb-config --cflags`
	LIBS += `directfb-config --libs` 
endif

ifeq ($(findstring SPHEGUI, $(CONFIGDEFINE)), SPHEGUI)
	LIBS +=/home/silicon/sunplus/sdk-1.09/lib/lib1001_IPSTB.a
	LIBS += crc32.o	inflate.o zutil.o adler32.o inftrees.o inffast.o
	CPPFLAGS +=-DSPHEGUI -I/home/silicon/sunplus/sdk-1.09/include
	LIBS += -lSDL_image `sdl-config --libs`
	LIBS += `libpng-config --ldflags`
	CPPFLAGS += `sdl-config --cflags`
endif

ifeq ($(findstring SDLGUI, $(CONFIGDEFINE)), SDLGUI)
	CPPFLAGS += -DSDLGUI
	LIBS += -ljpeg 
	LIBS += `sdl-config --static-libs` `freetype-config --libs` `libpng-config --libs`
	CPPFLAGS += `sdl-config --cflags`
	CPPFLAGS += `freetype-config --cflags`
endif

ifeq ($(findstring WIN32, $(CONFIGDEFINE)), WIN32)
	LIBS +=-lwsock32 -lgdi32
endif

ifeq ($(findstring NEWMOUSE, $(CONFIGDEFINE)), NEWMOUSE)
	SRC += mousecursor.cpp
	CPPFLAGS += -DNEWMOUSE
endif

ifeq ($(findstring ENABLE_HANDWRITE, $(CONFIGDEFINE)), ENABLE_HANDWRITE)
	SRC += hw/hwim.cpp
	CPPFLAGS += -DENABLE_HANDWRITE
	LIBS += hw/hw-i386.a
endif

ifeq ($(findstring TSLIB, $(CONFIGDEFINE)), TSLIB)
	CPPFLAGS += -DTSLIB
	LIBS += -lts
endif

SRC += \
	color.cxx         \
	keybuf.cxx        \
	font.cxx          \
	stack.cxx         \
	application.cxx   \
	xmltheme.cxx	  \
	key.cxx	          \
	data.cxx          \
	ktvstdclass.cxx   \
	songstream.cxx    \
	player.cxx        \
	main.cxx          \
	mtvconfig.cxx     \
	windowmanage.cxx  \
	win.cxx           \
	gui.cxx	          \
	timer.cxx         \
	diskcache.cxx     \
	list.cxx          \

-include $(TOPDIR)/inc.Makefile

#CC=$(CROSS_COMPILE)g++
LD=$(CROSS_COMPILE)g++

md5: 
	@md5sum -b mtv | tr a-z A-Z
