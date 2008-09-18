CC     = $(CROSS_COMPILE)gcc
CPP    = $(CROSS_COMPILE)g++
LD     = $(CROSS_COMPILE)gcc
AR     = $(CROSS_COMPILE)ar
RANLIB = $(CROSS_COMPILE)ranlib

LIBS = -lpthread `sdl-config --libs`
CFLAGS = -O2 -pipe `sdl-config --cflags` -D_REENTRANT -D_GNU_SOURCE

ifndef CONFIGDEFINE
CONFIGDEFINE   =
#CONFIGDEFINE  += -DJPG
CONFIGDEFINE  += -DPNG
CONFIGDEFINE  += -DBMP
#CONFIGDEFINE  += -DGIF
#CONFIGDEFINE  += -DLBM
#CONFIGDEFINE  += -DTGA
#CONFIGDEFINE  += -DXCF
#CONFIGDEFINE  += -DXXX
#CONFIGDEFINE  += -DPCX
#CONFIGDEFINE  += -DPNM
#CONFIGDEFINE  += -DTIF
#CONFIGDEFINE  += -DXPM
endif

LIB=../libSDL_image.a
SRC = IMG.c

ifeq ($(findstring -DBMP, $(CONFIGDEFINE)), -DBMP)
	SRC += IMG_bmp.c
	CFLAGS +=-DBMP
endif

ifeq ($(findstring -DGIF, $(CONFIGDEFINE)), -DGIF)
	SRC += IMG_gif.c
	CFLAGS +=-DGIF
endif

ifeq ($(findstring -DLBM, $(CONFIGDEFINE)), -DLBM)
	SRC += IMG_lbm.c
	CFLAGS +=-DLBM
endif
ifeq ($(findstring -DPNG, $(CONFIGDEFINE)), -DPNG)
	SRC += IMG_png.c
	CFLAGS +=-DPNG `libpng-config --cflags`
	LIBS += `libpng-config --ldflags`
endif
ifeq ($(findstring -DTGA, $(CONFIGDEFINE)), -DTGA)
	SRC += IMG_tga.c
	CFLAGS +=-DTGA
endif
ifeq ($(findstring -DXCF, $(CONFIGDEFINE)), -DXCF)
	SRC += IMG_xcf.c
	CFLAGS +=-DXCF
endif
ifeq ($(findstring -DXXX, $(CONFIGDEFINE)), -DXXX)
	SRC += IMG_xxx.c
	CFLAGS +=-DXXX
endif
ifeq ($(findstring -DJPG, $(CONFIGDEFINE)), -DJPG)
	SRC += IMG_jpg.c
	CFLAGS +=-DJPG
	LIBS += -ljpeg
endif
ifeq ($(findstring -DPCX, $(CONFIGDEFINE)), -DPCX)
	SRC += IMG_pcx.c
	CFLAGS +=-DPCX
endif
ifeq ($(findstring -DPNM, $(CONFIGDEFINE)), -DPNM)
	SRC += IMG_pnm.c
	CFLAGS +=-DPNM
endif
ifeq ($(findstring -DTIF, $(CONFIGDEFINE)), -DTIF)
	SRC += IMG_tif.c
	CFLAGS +=-DTIF
endif
ifeq ($(findstring -DXPM, $(CONFIGDEFINE)), -DXPM)
	SRC += IMG_xpm.c
	CFLAGS +=-DXPM
endif

OBJS=$(addprefix objects/, $(addsuffix .o, $(basename $(notdir $(SRC)))))

all: deps lib
# automatic generation of all the rules written by vincent by hand.
deps: $(SRC) Makefile
	@echo "Generating new dependency file...";
	@-rm -f deps;
	@for f in $(SRC); do \
		OBJ=objects/`basename $$f|sed -e 's/\.cpp/\.o/' -e 's/\.cxx/\.o/' -e 's/\.c/\.o/'`; \
		echo $$OBJ: $$f>> deps; \
		echo '	$(CC) $$(CFLAGS) -c -o $$@ $$^'>> deps; \
	done


-include ./deps

objects:
	mkdir objects

.PHONY: madlib


all: lib
lib: objects $(OBJS)
	@rm -f $(LIB) 
	$(AR) r $(LIB) $(OBJS)
	$(RANLIB) $(LIB) 
	
test:
	$(CPP) -static showimage.cpp -lSDL_image $(CFLAGS) $(LIBS) -o showimage -lz -L../ -lm
clean:
	rm -rf objects *.bin deps *~ *.a *.o showimage $(LIB)

