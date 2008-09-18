TOPDIR=../
SUBDIRS= ui index player 

-include $(TOPDIR)/inc.Makefile

md5:
	@md5sum -b ui/mtv   | tr a-f A-F | awk '{print $$1, " mtv"}'
	@md5sum -b player/pushplayer   | tr a-f A-F | awk '{print $$1, " player"}'
