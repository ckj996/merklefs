CXX=g++
CPPFLAGS=-O3 -Wall -Wextra -std=c++17
LDLIBS=-I ../lib
RM=rm -rf
TARGETS=test_config

all:$(TARGETS)

test_config: config.o
	$(CXX) -o test_config test_config.cc config.o $(LDLIBS) $(CPPFLAGS)

config.o:
	$(CXX) -c ../lib/config.cc $(LDLIBS) $(CPPFLAGS)

.PHONY: clean

clean:
	$(RM) $(TARGETS) *.o
