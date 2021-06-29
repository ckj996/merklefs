CC=gcc
CXX=g++
RM=rm -f
CPPFLAGS=-O2 -Wall -Wextra $(shell pkg-config fuse3 --cflags)
LDLIBS=$(shell pkg-config fuse3 --libs)

SRCS=merklefs.cc
OBJS=$(subst .cc,.o,$(SRCS))

all: merklefs

merklefs: $(OBJS)
	$(CXX) -o merklefs $(OBJS) $(LDLIBS)

depend: .depend

.depend: $(SRCS)
	$(RM) ./.depend
	$(CXX) $(CPPFLAGS) -MM $^>>./.depend;

.PHONY: clean
clean:
	$(RM) $(OBJS)

distclean: clean
	$(RM) *~ .depend

include .depend
