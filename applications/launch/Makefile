CXX=g++
CXXFLAGS=-std=c++11

TARGETS=main

all: $(TARGETS)

$(TARGETS):
	$(CXX) $(CXXFLAGS) -o $@ main.cpp

clean:
	rm $(TARGETS) stat.log thp.txt pf.txt *.txt *.out
