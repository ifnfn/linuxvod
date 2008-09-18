CC     = $(CROSS_COMPILE)gcc

CFLAGS = -Wall -Os
LDFLAGS =

hwscan: main.o pci.o modules.o 
	$(CC) $(LDFLAGS) -o $@ $+

clean:
	rm -vf hwscan *~ *.o

