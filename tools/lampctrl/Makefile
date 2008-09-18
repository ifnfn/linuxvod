CC=gcc
all:
	bison -y -d calc.y
	flex calc.l
	$(CC) -c y.tab.c lex.yy.c -DLINUX=1
	$(CC) y.tab.o lex.yy.o calc.c serial.c dsk31.c -DLINUX=1 -o lamp -lpthread -s
	$(CC) test.c serial.c -DLINUX=1 -o automic -s
dsk:
	$(CC) dsk.c dsk31.c serial.c -DLINUX=1 -o dsk -s

clean:
	rm y.tab.* lex.yy.* *.o lamp dsk automic -f

