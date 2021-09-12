MAKE=make
RM=rm -rf
CXX=g++
CXXFLAGS=-O3 -Wall -Wextra -std=c++17 -fdata-sections -ffunction-sections
LIBPATH=../lib
LIBNAME=merkle
LDFLAGS=-L$(LIBPATH) -l$(LIBNAME)\
		$(shell pkg-config --libs protobuf grpc++)\
		-pthread\
		-Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed\
		-Wl,--gc-sections\
		-ldl
SRCS=$(wildcard test_*.cc)
OBJS=$(subst .cc,.o,$(SRCS))
TARGETS=$(subst .cc,,$(SRCS))

all: depend $(TARGETS)

test_%: test_%.o
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cc
	$(CXX) -c $< $(CXXFLAGS)

.PHONY: clean depend

depend:
	$(MAKE) -C $(LIBPATH)

clean:
	$(RM) $(TARGETS) *.o
