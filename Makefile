CXX = g++
CXXFLAGS = -g -Wall -std=gnu++11 -lcurl

all: main

main: main.cpp code.c
	g++ -o main setMem.o main.o

main.cpp:
	g++ -c -o main.o main.cpp

code.c:
	gcc -c -o setMem.o setMem.c

clean:
	rm -f main
