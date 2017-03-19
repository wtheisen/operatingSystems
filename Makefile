CXX = g++
CXXFLAGS = -g -Wall -std=gnu++11 -lcurl

all: main

main: main.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f main
