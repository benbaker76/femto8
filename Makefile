PLATFORM ?= linux
include Makefile.$(PLATFORM)

BUILD_DIR := build-$(PLATFORM)
TARGET_NAME := femto8
TARGET := $(BUILD_DIR)/$(TARGET_NAME)
INCFLAGS += -Isrc -Isrc/data -Isrc/lua -Isrc/lodepng -Isrc/lexaloffle
DEFINES += -DLODEPNG_NO_COMPILE_ENCODER -DLODEPNG_NO_COMPILE_DISK -DLODEPNG_NO_COMPILE_ANCILLARY_CHUNKS -DLODEPNG_NO_COLOR_CONVERT

CFLAGS   += -Wall $(DEFINES) $(fpic) $(INCFLAGS) -g
CXXFLAGS += -Wall $(DEFINES) $(fpic) $(INCFLAGS) -g -fno-threadsafe-statics

# Source directories
SRC_DIRS := src src/lua src/data src/lodepng src/lexaloffle

# Create build directories
$(shell mkdir -p $(BUILD_DIR)/lua $(BUILD_DIR)/lexaloffle $(BUILD_DIR)/lodepng)

# Collect all source files
SOURCES_LUA := $(wildcard src/lua/*.c) src/p8_lua.c
SOURCES_C := $(filter-out $(SOURCES_LUA),$(wildcard $(addsuffix /*.c, $(SRC_DIRS))))
SOURCES_CXX := $(wildcard $(addsuffix /*.cpp, $(SRC_DIRS)))

# Generate object file paths
OBJECTS_C := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SOURCES_C))
OBJECTX_CXX := $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES_CXX))
OBJECTS_LUA := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SOURCES_LUA))
OBJECTS := $(OBJECTS_C) $(OBJECTS_CXX) $(OBJECTS_LUA)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CFLAGS) $(LDFLAGS) $(OBJECTS) -o $@ $(LIBS)

$(OBJECTS_C): $(BUILD_DIR)/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJECTS_CXX): $(BUILD_DIR)/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJECTS_LUA): $(BUILD_DIR)/%.o: src/%.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: clean
