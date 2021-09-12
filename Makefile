CC=gcc
CXX=g++
RM=rm -f
MAKE=make
CXXFLAGS=-O3 -Wall -Wextra -std=c++17 -fdata-sections -ffunction-sections $(shell pkg-config fuse3 --cflags)
LIBPATH=lib
LIBNAME=merkle
LDFLAGS=-L$(LIBPATH) -l$(LIBNAME)\
		$(shell pkg-config fuse3 --libs)\
		$(shell pkg-config --libs protobuf grpc++)\
		-pthread\
		-Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed\
		-Wl,--gc-sections\
		-ldl

all: libs merklefs

merklefs: merklefs.o
	$(CXX) -o $@ $^ $(LDLIBS) $(LDFLAGS)

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $<

.PHONY: clean libs

libs:
	$(MAKE) -C $(LIBPATH)

clean:
	$(RM) $(TARGETS) *.o
