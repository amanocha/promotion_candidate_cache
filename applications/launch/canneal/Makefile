# Makefile for parallel simulated annealer

PREFIX=${PARSECDIR}/pkgs/kernels/canneal/inst/${PARSECPLAT}

TARGET=canneal
LIBS:=$(LIBS) -lm

ifdef version
  ifeq "$(version)" "pthreads"
    CXXFLAGS+=-DENABLE_THREADS -pthread
  endif
endif

all:
	$(CXX) $(CXXFLAGS) annealer_thread.cpp -c -o annealer_thread.o
	$(CXX) $(CXXFLAGS) rng.cpp -c -o rng.o
	$(CXX) $(CXXFLAGS) netlist.cpp -c -o netlist.o
	$(CXX) $(CXXFLAGS) canneal.cpp -c -o canneal.o
	$(CXX) $(CXXFLAGS) netlist_elem.cpp -c -o netlist_elem.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) *.o $(LIBS) -o $(TARGET)
#	$(CXX) -std=c++11 main.cpp -o main

clean:
	rm -f *.o $(TARGET) stat.log thp.txt pf.txt

install:
	mkdir -p $(PREFIX)/bin
	cp -f $(TARGET) $(PREFIX)/bin/$(TARGET)

