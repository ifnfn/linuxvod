################################################################
# Copyright (c) 2004 . All rights reserved.                    #
# Author : Silicon                                             #
# Date : 06-19-2001                                            #
#                                                              #
# This is the base makefile for all realmagic projects         #
#                                                              #
################################################################
INCLUDES =
CC =gcc
LD =gcc
SRC = \
	main.c \
	vstpserver.c \

DEFS =
DEFAULT_INCLUDES=-I. -I../lib
LDFLAGS =
LIBS = -lm -lsqlite -lpthread -lktv -L../lib
CFLAGS = $(DEFS) $(DEFAULT_INCLUDES)

OBJS=$(addprefix objects/, $(addsuffix .o, $(basename $(notdir $(SRC)))))
all: deps mtv 

# automatic generation of all the rules written by vincent by hand.
deps: $(SRC) Makefile
	@echo "Generating new dependency file...";
	@-rm -f deps;
	@for f in $(SRC); do \
          OBJ=objects/`basename $$f|sed -e 's/\.cxx/\.o/' -e 's/\.c/\.o/'`; \
          echo $$OBJ: $$f>> deps; \
          echo '	$(CC) $$(CFLAGS) -c -o $$@ $$^'>> deps; \
        done
-include ./deps
objects:
	mkdir objects
.PHONY: madlib

mtv: objects $(OBJS) 
	$(LD) $(OBJS) $(LIBS) -o $@

clean:
	rm -rf mtv objects *.bin deps *~ index *.o server
