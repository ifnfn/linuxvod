CC     = $(CROSS_COMPILE)gcc

CFLAGS=-g -Os -DSELF_AES
LDFLAGS=$(CFLAGS)
LIBS=
DESTDIR=/usr/local/ghttpd

OBJ = main.o protocol.o util.o 

default: ghttpd

ghttpd: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ) $(LIBS) -d ../lib/libktv.a

clean:
	rm -f *.o ghttpd cscope* tags

