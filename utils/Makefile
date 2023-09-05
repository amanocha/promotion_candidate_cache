C=gcc
CFLAGS=-lm
CXX=g++
CXXFLAGS=-std=c++11

C_TARGETS= fragm free
CXX_TARGETS= dbg

all: $(C_TARGETS) $(CXX_TARGETS)

$(C_TARGETS):
	$(C) -o $@ $@.c $(CFLAGS)

$(CXX_TARGETS):
	$(CXX) $(CXXFLAGS) -o $@ $@.cpp

clean:
	rm $(C_TARGETS)
	rm $(CXX_TARGETS)
