# Comment/uncoment for debug/release build
#CFLAGS = -std=c++11 -O3 -flto -DNDEBUG -DDEBUG=0
CFLAGS := -std=c++11 -Wall -Wextra -g -DDEBUG=1

CXX := clang++

MODULES := blueprint

SRC :=  $(addsuffix .cpp, $(addprefix src/,$(MODULES)))
OBJS := $(addsuffix .o, $(addprefix build/,$(MODULES)))
DEPS := $(addsuffix .d, $(addprefix deps/,$(MODULES)))

.PHONY : all clean

all: nsa

nsa: $(OBJS) | build
	$(CXX) -o nsa $(OBJS)

-include $(DEPS)

build/%.o: src/%.cpp | build deps
	$(CXX) -c $(CFLAGS) src/$*.cpp -o build/$*.o
	$(CXX) -MM -MT build/$*.o $(CFLAGS) src/$*.cpp > deps/$*.d

build:
	mkdir build

deps:
	mkdir deps

clean:
	-rm -rf build deps
