TARGET_NAME := femto8
TARGET := build/$(TARGET_NAME)
INCFLAGS += -ISDL-1.2/include -Isrc -Isrc/data -Isrc/lua -Isrc/lodepng -Isrc/lexaloffle
LDFLAGS += -LSDL-1.2/build/.libs
LIBS += -Wl,-Bstatic -lSDL -lSDLmain -Wl,-Bdynamic -lpthread
fpic := -fPIC
DEFINES += -DLUA_USE_POSIX -DLODEPNG_NO_COMPILE_ENCODER -DLODEPNG_NO_COMPILE_DISK -DLODEPNG_NO_COMPILE_ANCILLARY_CHUNKS -DLODEPNG_NO_COLOR_CONVERT

CFLAGS   += -Wall $(DEFINES) $(fpic) $(INCFLAGS) -g
CXXFLAGS += -Wall $(DEFINES) $(fpic) $(INCFLAGS) -g -fno-threadsafe-statics

# Source directories
SRC_DIRS := src src/lua src/data src/lodepng src/lexaloffle

# Create build directories
$(shell mkdir -p build/lua build/lexaloffle build/lodepng)

# Collect all source files
SOURCES_C := $(wildcard $(addsuffix /*.c, $(SRC_DIRS)))
SOURCES_CXX := $(wildcard $(addsuffix /*.cpp, $(SRC_DIRS)))

# Generate object file paths
OBJECTS := $(patsubst src/%.c,build/%.o,$(SOURCES_C)) $(patsubst src/%.cpp,build/%.o,$(SOURCES_CXX))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CFLAGS) $(LDFLAGS) $(OBJECTS) -o $@ $(LIBS)

build/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: clean