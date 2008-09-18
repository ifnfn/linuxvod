CC     = $(CROSS_COMPILE)gcc
CPP    = $(CROSS_COMPILE)g++
LD     = $(CROSS_COMPILE)gcc
AR     = $(CROSS_COMPILE)ar
RANLIB = $(CROSS_COMPILE)ranlib

INCS +=-Werror -Wundef -Wall -pipe -Os
CFLAGS += $(INCS) $(CPPFLAGS)

OBJS=$(addprefix objects/, $(addsuffix .o, $(basename $(notdir $(SRC)))))

ifneq "$(LIB)" ""
	TARGET=$(LIB)
endif

ifneq "$(LIBSO)" ""
	TARGETSO=$(LIBSO)
endif
all: objects deps subdirs $(OBJS) $(TARGET) $(BIN) $(TARGETSO)

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

$(TARGET): $(OBJS)
	$(AR) r $@ $(OBJS)
	$(RANLIB) $@

$(TARGETSO): $(OBJS)
	$(CC) --share -s $(OBJS) -o $@

$(BIN): $(OBJS)
	@ln -s `g++ -print-file-name=libstdc++.a` -f
	$(LD) $(OBJS) $(LIBS) -o $@ -s
	@unlink libstdc++.a 
	
subdirs:
	@list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		echo "Making $$target in $$subdir"; \
		cd $$subdir && $(MAKE); \
		cd ..; \
	done;

subdirsclean:
	@list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		echo "Making $$target in $$subdir"; \
		cd $$subdir && $(MAKE) clean; \
		cd ..; \
	done;

clean: subdirsclean
	rm -rf $(OBJS) *.o *~ .*swp objects deps $(CLEANFILE) $(BIN) $(TARGET) $(TARGETSO)
