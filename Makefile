
CC ?= gcc
CXX ?= g++
CFLAGS ?= -O2 -g -Wall -pipe
CXXFLAGS ?= -O2 -g -Wall -pipe

all: realloc_benchmark push_back_benchmark


realloc_benchmark: realloc_benchmark.o
	$(CC) -o $@ -lrt $^

realloc_benchmark.o: realloc_benchmark.c
	$(CC) -c -o $@ $(CFLAGS) -std=c99 $<

push_back_benchmark: push_back_benchmark.o
	$(CXX) -o $@ $^

push_back_benchmark.o: push_back_benchmark.cpp
	$(CXX) -c -o $@ $(CXXFLAGS) -std=c++11 $<

clean:
	@rm -f realloc_benchmark.o
	@rm -f push_back_benchmark.o

allclean:
	@git clean -dfx


.PHONY: clean
