CPP  = g++
CC   = gcc
WINDRES = windres.exe
RES  = 
OBJ  = main.o fmpovlctrl.o headerdetection.o $(RES)
LINKOBJ  = main.o fmpovlctrl.o headerdetection.o $(RES)
#LIBS =  -L"C:/Dev-Cpp/lib"  
#INCS =  -I"C:/Dev-Cpp/include" 
#CXXINCS =  -I"C:/Dev-Cpp/include/c++"  -I"C:/Dev-Cpp/include" 
BIN  = fmp
CXXFLAGS = $(CXXINCS)  
CFLAGS = $(INCS)  
RM = rm -f

.PHONY: all all-before all-after clean clean-custom

all: all-before fmp all-after


clean: clean-custom
	${RM} $(OBJ) $(BIN)

$(BIN): $(OBJ)
	$(CC) $(LINKOBJ) -o $(BIN) $(LIBS)

main.o: main.c
	$(CC) -c main.c -o main.o $(CFLAGS)

fmpovlctrl.o: fmpovlctrl.c
	$(CC) -c fmpovlctrl.c -o fmpovlctrl.o $(CFLAGS)

headerdetection.o: headerdetection.c
	$(CC) -c headerdetection.c -o headerdetection.o $(CFLAGS)
