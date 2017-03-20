CXX = g++
CXXFLAGS = -g -Wall -std=gnu++11 -lcurl

all: main

main: main1 
	g++ -o main setMem.o main.o -lcurl

main1:
	gcc -c -o setMem.o setMem.c -lcurl
	g++ -c -o main.o main.cpp -std=gnu++11 -lcurl

#code.c:
#	gcc -c -o setMem.o setMem.c

clean:
	rm -f main
