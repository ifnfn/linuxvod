PLUGINS =  linear.so dejitter.so variance.so
PLUGIN_DIR = /usr/local/share/ts/plugins

LDFLAGS := -s -I../src --share $(LDFLAGS) -lts #-Wl,-rpath=$(PLUGIN_DIR) -lts 

all: $(PLUGINS)
	
variance.so:
	$(CC) $(LDFLAGS) variance.c -o $@
dejitter.so:
	$(CC) $(LDFLAGS) dejitter.c -o $@
linear.so:
	$(CC) $(LDFLAGS) linear.c -o  $@

install:
	cp $(PLUGINS) $(PLUGIN_DIR)
clean: 
	rm *.so *.o -f
