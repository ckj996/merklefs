CC=gcc
CXX=g++
RM=rm -f
MAKE=make
CXXFLAGS=-O3 -Wall -Wextra -std=c++17 -fdata-sections -ffunction-sections $(shell pkg-config fuse3 --cflags)
LDFLAGS=-Wl,--gc-sections
LIBPATH=lib
LIBNAME=merkle
LDLIBS=$(shell pkg-config fuse3 --libs) -L$(LIBPATH) -l$(LIBNAME)

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
