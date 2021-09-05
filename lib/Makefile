CXX=g++
CXXFLAGS=-O3 -Wall -Wextra -std=c++17 -fdata-sections -ffunction-sections
RM=rm -rf
OBJS=$(subst .cc,.o,$(wildcard *.cc))
TARGETS=libmerkle.a

all: $(TARGETS)

%.a: $(OBJS)
	ar rcs $@ $^

%.o: %.cc %.hpp
	$(CXX) $(CXXFLAGS) -c $<

.PHONY: clean

clean:
	$(RM) $(TARGETS) *.o
