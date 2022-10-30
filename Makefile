CC = gcc
CXX = g++

CXXFLAGS = -DUNICODE -Wall -g 
CXXFLAGS += -Ilib/glad/include -Ilib/stb -Ilib/wgl -Ilib/freetype/include
CXXFLAGS += -MT $@ -MMD -MP -MF $*.d

DEPOBJS = lib/glad/src/glad.o lib/freetype_build/libfreetype.a
DEPOBJS += lib/stb/stb_image.o lib/stb/stb_image_write.o

GEN = -G "MinGW Makefiles"

LDFLAGS = -municode -fno-exceptions -fno-rtti
LDFLAGS += -lopengl32 -lole32 -mwindows -mconsole
LDFLAGS += $(DEPOBJS)

SRC = $(wildcard src/*.cpp)
HEADER = $(wildcard src/*.hpp)

OBJ = $(patsubst src/%.cpp,obj/%.o,$(SRC))
DEP = $(patsubst src/%.cpp,obj/%.d,$(SRC))

all: dirs obj/menu.o engine

lib/freetype_build/libfreetype.a:
	mkdir -p lib\freetype_build
	cd lib/freetype_build && \
	cmake $(GEN) ../freetype && \
	mingw32-make

lib/stb/stb_image.o:
	cd lib/stb && \
	$(CC) -x c -c stb_image.h -DSTB_IMAGE_IMPLEMENTATION

lib/stb/stb_image_write.o:
	cd lib/stb && \
	$(CC) -x c -c stb_image_write.h -DSTB_IMAGE_WRITE_IMPLEMENTATION

lib/glad/src/glad.o:
	cd lib/glad && \
	$(CC) -o src/glad.o -Iinclude -c src/glad.c

dirs:
	mkdir -p ./bin 
	mkdir -p ./obj 

obj/menu.o: src/menu.hpp src/menu.rc
	windres src/menu.rc -o obj/menu.o

include $(wildcard $DEP)

%.o: src/%.cpp %.d
	$(CXX) $(CXXFLAGS) -o $@ -c $<

obj/%.o: src/%.o
	mv $< $@

obj/%.d: src/%.d
	mv $< $@

engine: $(OBJ) $(DEP) obj/menu.o $(DEPOBJS)
	$(CXX) -o bin/engine.exe $(OBJ) obj/menu.o $(LDFLAGS)

clean:
	rm $(OBJ) $(DEP) $(DEPOBJS) obj/menu.o -f
	rm -f -r bin
	rm -f -r obj 
	rm -f -r lib/freetype_build 
