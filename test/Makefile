CXX=g++
CPPFLAGS=-O3 -Wall -Wextra -std=c++17
LDLIBS=-I ../lib
RM=rm -rf
TARGETS=test_config test_path test_fs

all:$(TARGETS)

test_config: test_config.cc config.o
	$(CXX) -o $@ $^ $(LDLIBS) $(CPPFLAGS)

config.o: ../lib/config.cc
	$(CXX) -c $^ $(LDLIBS) $(CPPFLAGS)

test_path: test_path.cc path.o
	$(CXX) -o $@ $^ $(LDLIBS) $(CPPFLAGS)

path.o: ../lib/path.cc
	$(CXX) -c $^ $(LDLIBS) $(CPPFLAGS)

test_fs: test_fs.cc fs.o path.o
	$(CXX) -o $@ $^ $(LDLIBS) $(CPPFLAGS)

fs.o: ../lib/fs.cc
	$(CXX) -c $^ $(LDLIBS) $(CPPFLAGS)

.PHONY: clean

clean:
	$(RM) $(TARGETS) *.o
