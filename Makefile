CC=gcc
CXX=g++
RM=rm -f
CPPFLAGS=-O3 -Wall -Wextra $(shell pkg-config fuse3 --cflags)
LDLIBS=$(shell pkg-config fuse3 --libs)

SRCS=merklefs.cc
OBJS=$(subst .cc,.o,$(SRCS))
DESTDIR=/usr/local

all: merklefs

merklefs: $(OBJS)
	$(CXX) -o merklefs $(OBJS) $(LDLIBS)

depend: .depend

.depend: $(SRCS)
	$(RM) ./.depend
	$(CXX) $(CPPFLAGS) -MM $^>>./.depend;

.PHONY: clean install uninstall
clean:
	$(RM) $(OBJS)

install:
	install merklefs $(DESTDIR)/bin

uninstall:
	rm -f $(DESTDIR)/bin/merklefs

distclean: clean
	$(RM) *~ .depend

include .depend
