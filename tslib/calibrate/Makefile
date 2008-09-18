CC     = $(CROSS_COMPILE)gcc
CPP    = $(CROSS_COMPILE)g++
LD     = $(CROSS_COMPILE)gcc
AR     = $(CROSS_COMPILE)ar
RANLIB = $(CROSS_COMPILE)ranlib

INCS +=-Werror -Wundef -Wall -pipe -Os
CFLAGS += $(INCS) $(CPPFLAGS) -I../src
LIBS+=-lts -ldl

BIN=ts ts_test
OBJS=$(addprefix objects/, $(addsuffix .o, $(basename $(notdir $(SRC)))))

SRC = \
	fbutils.c  \
	font_8x8.c  \
	
#	font_8x16.c  \

all: deps $(BIN)

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
	@mkdir objects
.PHONY: madlib

ts_test: objects $(OBJS)
	$(CC) -s ts_test.c $(CFLAGS) $(OBJS) $(LIBS) -o $@
	
ts: objects $(OBJS)
	$(CC) ts_calibrate.c $(CFLAGS) $(OBJS) $(LIBS) -o $@ -s
	
clean: 
	rm -rf $(OBJS) *.o *~ .*swp objects deps $(BIN) 
