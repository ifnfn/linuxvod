IMAGEPATH = /home/silicon/work/image
PXEPATH   = $(IMAGEPATH)/rootfs

SUBDIRS= lib ktv++ httpd tools

-include inc.Makefile

CLEANFILE += system.gz
	
md5:
	make -C ktv++ md5
	@md5sum -b tools/download   | tr a-f A-F | awk '{print $$1, " download"}'
		
install: all
	cp ktv++/ui/mtv                $(PXEPATH)/bin/mtv  -f
#	cp ktv++/player/libplayctrl.so $(PXEPATH)/usr/local/lib -f
	cp ktv++/player/pushplayer     $(PXEPATH)/bin/play -f
	cp tools/download              $(PXEPATH)/bin/download -f
	strip $(PXEPATH)/bin/mtv \
		$(PXEPATH)/bin/play \
		$(PXEPATH)/bin/download \
		$(PXEPATH)/usr/local/lib/libplayctrl.so

#	mkfs.cramfs $(PXEPATH)/ system.gz
	mksquashfs $(PXEPATH)/*    system.gz -noappend
