CC     = $(CROSS_COMPILE)gcc 
LD     = $(CROSS_COMPILE)gcc
AR     = $(CROSS_COMPILE)ar
RANLIB = $(CROSS_COMPILE)ranlib

BIN=addsong checkkey usbpnpd saveoff reg send download gtalk

all: $(BIN)
	make -C hwscan
saveoff:
	@echo "main(){char ar[2]={10, 0};ioctl(0,\"/dev/vcs\",ar);}">a.c
	$(CC) a.c -o $@ -s
	@rm a.c -f
addsong:
	$(CC) addsong.c ../lib/osnet.c ../lib/ini.c -o addsong -ltinyxml \
		-lsqlite -lktv -L../lib -I../lib -s
usbpnpd:
	$(CC) usbpnpd.c -o usbpnpd -s
checkkey:
	$(CC) checkey.c -o checkkey -s
reg:
	$(CC) reg.c -lktv -L../lib -I../lib -o $@ -DREGCODE -s
download:
	$(CC) $(CXXFLAGS) download.c ../lib/osnet.c -o download -s -lktv -I../lib -lktv -L../lib -s
send:
	$(CC) send.c ../lib/osnet.c -o send -I../lib -lktv -L../lib -s
gtalk:
	$(CC) gtalk.c -I../lib `pkg-config loudmouth-1.0 --cflags --libs` \
	`pkg-config sqlite --libs` -lktv -L../lib -s -o $@
clean:
	rm $(BIN) -rf
	make -C hwscan clean
