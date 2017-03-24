CXX = g++
CXXFLAGS = -g -Wall -std=gnu++11 -lcurl

all: main

main: main1
	/usr/bin/g++ -o site-tester setMem.o main.o -lcurl -Wall

main1:
	/usr/bin/gcc -c -o setMem.o setMem.c -lcurl -Wall
	/usr/bin/g++ -c -o main.o main.cpp -std=c++0x -lcurl -Wall

#code.c:
#	gcc -c -o setMem.o setMem.c
clean:
	rm -f site-tester *.o
