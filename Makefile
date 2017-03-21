CXX = g++
CXXFLAGS = -g -Wall -std=gnu++11 -lcurl

all: main

main: main1 
	g++ -o main setMem.o main.o -lcurl -Wall

main1:
	gcc -c -o setMem.o setMem.c -lcurl -Wall
	g++ -c -o main.o main.cpp -std=gnu++11 -lcurl -Wall

#code.c:
#	gcc -c -o setMem.o setMem.c

clean:
	rm -f main
