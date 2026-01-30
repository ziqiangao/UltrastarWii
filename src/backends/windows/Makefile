#-------------------------------------------------------------------
# backends/windows/Makefile - Windows platform (MinGW GCC) slave Makefile
# OpenGL + SDL2
#-------------------------------------------------------------------

# Target and platform
TARGET      := UltrastarWindows
PLATFORM    := windows

#-------------------------------------------------------------------
# Source directories
#-------------------------------------------------------------------
SOURCES     := $(CURDIR)/../../src $(CURDIR)
DATA        := $(CURDIR)/../../data
INCLUDES    := $(CURDIR)/../../src $(CURDIR)/bridge

#-------------------------------------------------------------------
# Compiler options
#-------------------------------------------------------------------
CC          := gcc
CXX         := g++
CFLAGS      := -g -O2 -Wall $(INCLUDE)
CXXFLAGS    := $(CFLAGS) -std=c++11
LDFLAGS     := -g

#-------------------------------------------------------------------
# Libraries
#-------------------------------------------------------------------
LIBS        := -lSDL2 -lSDL2_ttf -lSDL2_image -lopengl32 -lglu32 -lm -lwinmm

#-------------------------------------------------------------------
# Include and library paths
#-------------------------------------------------------------------
# Update these paths if SDL2 or other libraries are installed elsewhere
SDL2_INC    := C:/SDL2/include
SDL2_LIB    := C:/SDL2/lib
INCLUDE     := $(foreach dir,$(INCLUDES),-iquote $(dir)) -I$(SDL2_INC)
LIBPATHS    := -L$(SDL2_LIB)

#-------------------------------------------------------------------
# Collect source files
#-------------------------------------------------------------------
CFILES      := $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.c))
CPPFILES    := $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.cpp))
SFILES      := $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.s))
SFILES_UP   := $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.S))

# Binary assets (optional)
BINFILES    := $(foreach dir,$(DATA),$(wildcard $(dir)/*.*))

#-------------------------------------------------------------------
# Optional: custom rules for converting binary data
#-------------------------------------------------------------------
%.jpg.o: %.jpg
	@echo Converting $(notdir $<)
	$(bin2o)

%.png.o: %.png
	@echo Converting $(notdir $<)
	$(bin2o)

%.bmp.o: %.bmp
	@echo Converting $(notdir $<)
	$(bin2o)

%.ttf.o: %.ttf
	@echo Converting $(notdir $<)
	$(bin2o)
