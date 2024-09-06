INCLUDE_DIR = $(INSTALL_ROOT)/include
LIB_DIR = $(INSTALL_ROOT)/lib

all: main

main.o: main.cpp Makefile
	gcc -c -std=c++20 -I $(INCLUDE_DIR) main.cpp

main: main.o Makefile
	gcc -std=c++20 -o main main.o -L $(LIB_DIR) $(LIBS)

clean: 
	rm *.o main || true
