# Software modules to be built
MODULES := main blueprint

# Dependencies configurable with pkg-config
PKG_CONFIG_DEPS := OGRE OIS

# Comment/uncoment for debug/release build
#CFLAGS = -std=c++11 -O3 -flto -DNDEBUG -DDEBUG=0
CFLAGS := -std=c++11 -Wall -Wextra -g -DDEBUG=1

# Run pkg-config and get flags
CFLAGS += $(shell pkg-config --cflags $(PKG_CONFIG_DEPS))
LIBS = $(shell pkg-config --libs $(PKG_CONFIG_DEPS)) -lboost_system

CXX = clang++

SRC := $(addsuffix .cpp, $(addprefix src/,$(MODULES)))
OBJS := $(addsuffix .o, $(addprefix build/,$(MODULES)))
DEPS := $(addsuffix .d, $(addprefix deps/,$(MODULES)))

.PHONY : all clean

all: nsa

nsa: $(OBJS) | build
	$(CXX) -o nsa $(OBJS) $(LIBS)

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
