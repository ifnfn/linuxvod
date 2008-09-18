TOPDIR=../../

LIBS += -static-libgcc -lktv -L../../lib -lsqlite -L.
CPPFLAGS += -I../ui -I../../lib -O2 -pipe -Werror -Wall

BIN=song

SRC += \
	createindex.cpp \
	index.cxx

-include $(TOPDIR)/inc.Makefile

LD=$(CROSS_COMPILD)g++
