CXX=g++
CPPFLAGS=-O3 -Wall -Wextra -std=c++17
LDLIBS=-I ../lib
RM=rm -rf
TARGETS=test_config test_path

all:$(TARGETS)

test_config: test_config.cc config.o
	$(CXX) -o test_config test_config.cc config.o $(LDLIBS) $(CPPFLAGS)

config.o: ../lib/config.cc
	$(CXX) -c ../lib/config.cc $(LDLIBS) $(CPPFLAGS)

test_path: test_path.cc path.o
	$(CXX) -o test_path test_path.cc path.o $(LDLIBS) $(CPPFLAGS)

path.o: ../lib/path.cc
	$(CXX) -c ../lib/path.cc $(LDLIBS) $(CPPFLAGS)

.PHONY: clean

clean:
	$(RM) $(TARGETS) *.o
