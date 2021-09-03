CXX=g++
CXXFLAGS=-O3 -Wall -Wextra -std=c++17 -fdata-sections -ffunction-sections
LDFLAGS=-Wl,--gc-sections
LDLIBS=-I ../lib
RM=rm -rf
NAMES=config path metadata
TARGETS=$(addprefix test_, $(NAMES))
OBJECTS=$(addsuffix .o, $(NAMES))

all:$(TARGETS)

%: %.cc libmerkle.a
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS) $(LDFLAGS)

%.o: ../lib/%.cc
	$(CXX) $(CXXFLAGS) -c $^ $(LDLIBS) $(LDFLAGS)

libmerkle.a: $(OBJECTS)
	ar rcs $@ $^

libs: libmerkle.a

.PHONY: clean

clean:
	$(RM) $(TARGETS) *.o *.a
