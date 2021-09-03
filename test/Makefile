CXX=g++
CXXFLAGS=-O3 -Wall -Wextra -std=c++17
LDLIBS=-I ../lib
RM=rm -rf
TARGETS=test_config test_path test_fs

all:$(TARGETS)

%: %.cc libmerkle.a
	$(CXX) -o $@ $^ $(LDLIBS) $(CXXFLAGS)

%.o: ../lib/%.cc
	$(CXX) -c $^ $(LDLIBS) $(CXXFLAGS)

libmerkle.a: fs.o path.o config.o
	ar rcs $@ $^

libs: libmerkle.a

.PHONY: clean

clean:
	$(RM) $(TARGETS) *.o *.a
