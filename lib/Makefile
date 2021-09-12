MAKE=make
RM=rm -rf
CXX=g++
CPPFLAGS=$(pkg-config --cflags protobuf grpc++)
CXXFLAGS=-O3 -Wall -Wextra -std=c++17 -fdata-sections -ffunction-sections
OBJS := $(subst .cc,.o,$(wildcard *.cc))
TARGET=libmerkle.a

PROTOS=object
PROTOC=protoc
GRPC_CPP_PLUGIN_PATH=$(shell which grpc_cpp_plugin)
PROTOS_PATH=../protos
vpath %.proto $(PROTOS_PATH)

all: depend

%.a: $(OBJS)
	ar rcs $@ $^

%.o: %.cc %.hpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $<

.PRECIOUS: %.grpc.pb.cc
%.grpc.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $<

.PRECIOUS: %.pb.cc
%.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=. $<

.PHONY: proto clean all

depend: $(PROTOS).pb.cc $(PROTOS).grpc.pb.cc
	$(MAKE) $(TARGET)

clean:
	$(RM) *.a *.o *.pb.*