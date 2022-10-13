CC = gcc
CXX = g++

CXXFLAGS = -DUNICODE -Wall -g 
CXXFLAGS += -Ilib/glad/include -Ilib/stb -Ilib/wgl
CXXFLAGS += -MT $@ -MMD -MP -MF $*.d

LDFLAGS = -municode -fno-exceptions -fno-rtti
LDFLAGS += -lopengl32 -lole32 -mwindows -mconsole
LDFLAGS += lib/glad/src/glad.o lib/stb/stb_image.o

SRC = $(wildcard src/*.cpp)
OBJ = $(SRC:%.cpp=%.o)
DEP = $(SRC:%.cpp=%.d)

all: libs dirs src/menu.o engine

libs:
	if not exist lib/stb/stb_image.o \
		cd lib/stb && \
		$(CC) -x c -c stb_image.h -DSTB_IMAGE_IMPLEMENTATION 
	if not exist lib/glad/src/glad.o \
		cd lib/glad && \
		$(CC) -o src/glad.o -Iinclude -c src/glad.c

dirs:
	if not exist bin \
		mkdir bin 

src/menu.o: src/menu.hpp src/menu.rc
	windres src/menu.rc -o src/menu.o

%.o: %.cpp %.d
	$(CXX) $(CXXFLAGS) -o $@ -c $<

include $(wildcard $DEP)

engine: $(OBJ) src/menu.o
	$(CXX) -o bin/engine.exe $^ $(LDFLAGS)

clean:
	rm $(OBJ) $(DEP) -f
	rm -r bin
