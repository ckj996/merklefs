CXX=g++
CXXFLAGS=-O3 -Wall -Wextra -std=c++17 -fdata-sections -ffunction-sections
LDFLAGS=-Wl,--gc-sections
RM=rm -rf
MAKE=make
LIBPATH=../lib
LIBNAME=merkle
LDLIBS=-L$(LIBPATH) -l$(LIBNAME)
SRCS=$(wildcard *.cc)
OBJS=$(subst .cc,.o,$(SRCS))
TARGETS=$(subst .cc,,$(SRCS))

all: libs $(TARGETS)

%: %.o
	$(CXX) -o $@ $^ $(LDLIBS) $(LDFLAGS)

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $<

.PHONY: clean libs

libs:
	$(MAKE) -C $(LIBPATH)

clean:
	$(RM) $(TARGETS) *.o
