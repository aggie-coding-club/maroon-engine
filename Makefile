CC = gcc
CXX = g++

CXXFLAGS = -DUNICODE -Wall
CXXFLAGS += -Ilib/glad/include -Ilib/wgl
CXXFLAGS += -MT $@ -MMD -MP -MF $*.d

LDFLAGS = -municode -fno-exceptions -fno-rtti
LDFLAGS += -lopengl32 -mwindows
LDFLAGS += lib/glad/src/glad.o

SRC = $(wildcard src/*.cpp)
OBJ = $(SRC:.cpp=.o)
BIN = bin

all: libs dirs engine

libs:
	if not exist bin \
		mkdir bin
	if not exist lib/glad/src/glad.o \
		cd lib/glad && $(CC) -o src/glad.o -Iinclude -c src/glad.c

dirs:
	if not exist $(BIN) \
		mkdir $(BIN)

%.o: %.cpp %.d
	$(CXX) $(CXXFLAGS) -o $@ -c $<

DEPFILES := $(SRC:%.cpp=%.d)
include $(wildcard $DEPFILES)

engine: $(OBJ)
	$(CXX) -o $(BIN)/engine.exe $^ $(LDFLAGS)
