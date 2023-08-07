CXX=g++
CXXFLAGS=-std=c++11 -fopenmp

TARGETS=pagerank

all: $(TARGETS)

$(TARGETS):
	$(CXX) $(CXXFLAGS) -o $@ main.cpp

clean:
	rm $(TARGETS)
