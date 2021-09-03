CXX=g++
CXXFLAGS=-O3 -Wall -Wextra -std=c++17 -fdata-sections -ffunction-sections
LDFLAGS=-Wl,--gc-sections
LDLIBS=-I ../lib
RM=rm -rf
TARGETS=test_config test_path test_fs

all:$(TARGETS)

%: %.cc libmerkle.a
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS) $(LDFLAGS)

%.o: ../lib/%.cc
	$(CXX) $(CXXFLAGS) -c $^ $(LDLIBS) $(LDFLAGS)

libmerkle.a: fs.o path.o config.o
	ar rcs $@ $^

libs: libmerkle.a

.PHONY: clean

clean:
	$(RM) $(TARGETS) *.o *.a
