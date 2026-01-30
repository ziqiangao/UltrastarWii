#-------------------------------------------------------------------
# Master Makefile at workspace root
# Cross-platform build system
#-------------------------------------------------------------------

# Default platform
PLATFORM ?= windows

# Include platform-specific slave Makefile
ifeq ($(PLATFORM),wii)
include src/backends/wii/Makefile
endif

ifeq ($(PLATFORM),windows)
include src/backends/windows/Makefile
endif

#-------------------------------------------------------------------
# Common build directories
#-------------------------------------------------------------------
BUILD := build/$(PLATFORM)
SRC := src
TARGET := $(TARGET)  # Taken from slave Makefile

#-------------------------------------------------------------------
# Collect source files
#-------------------------------------------------------------------
# Shared sources
CPPFILES := $(wildcard $(SRC)/*.cpp)
CFILES   := $(wildcard $(SRC)/*.c)

# Platform-specific sources from slave folder
CPPFILES += $(wildcard $(SRC)/backends/$(PLATFORM)/*.cpp)
CFILES   += $(wildcard $(SRC)/backends/$(PLATFORM)/*.c)

#-------------------------------------------------------------------
# Compute object file paths
#-------------------------------------------------------------------
OFILES_CPP := $(patsubst $(SRC)/%.cpp,$(BUILD)/%.o,$(CPPFILES))
OFILES_C   := $(patsubst $(SRC)/%.c,$(BUILD)/%.o,$(CFILES))
OFILES     := $(OFILES_CPP) $(OFILES_C)

#-------------------------------------------------------------------
# Build rules
#-------------------------------------------------------------------
all: $(BUILD)/$(TARGET)

# Link executable
$(BUILD)/$(TARGET): $(OFILES)
	@mkdir -p $(dir $@)
	$(CXX) $(OFILES) $(LDFLAGS) $(LIBS) -o $@

# Compile C++ files
$(BUILD)/%.o: $(SRC)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

# Compile C files
$(BUILD)/%.o: $(SRC)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

#-------------------------------------------------------------------
# Clean build
#-------------------------------------------------------------------
clean:
	rm -rf build/*

.PHONY: all clean
