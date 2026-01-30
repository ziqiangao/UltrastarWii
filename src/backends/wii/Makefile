#-------------------------------------------------------------------
# backends/wii/Makefile - Wii platform slave Makefile
# Exports variables only, no recursive make calls
#-------------------------------------------------------------------

ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC")
endif

include $(DEVKITPPC)/wii_rules

#-------------------------------------------------------------------
# Target and platform
#-------------------------------------------------------------------
TARGET      := UltrastarWii
PLATFORM    := wii

#-------------------------------------------------------------------
# Source directories
#-------------------------------------------------------------------
SOURCES     := $(CURDIR)/../../src $(CURDIR)
DATA        := $(CURDIR)/../../data $(CURDIR)/../../data/mii
INCLUDES    := $(CURDIR)/../../src $(CURDIR)/bridge

#-------------------------------------------------------------------
# Compiler options
#-------------------------------------------------------------------
CFLAGS      := -g -O2 -Wall $(MACHDEP) $(INCLUDE)
CXXFLAGS    := $(CFLAGS) -std=c++11
LDFLAGS     := -g $(MACHDEP) -Wl,-Map,$(TARGET).map

#-------------------------------------------------------------------
# Libraries and library paths
#-------------------------------------------------------------------
LIBS        := -ldb -lgrrlib -lpngu `$(PREFIX)pkg-config freetype2 libpng libjpeg --libs` \
               -lfat -lwiiuse -lbte -logc -lmii -lisfs -lmad -lasnd

LIBDIRS     := $(PORTLIBS)
INCLUDE     := $(foreach dir,$(INCLUDES),-iquote $(dir)) \
               $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
               -I$(LIBOGC_INC)
LIBPATHS    := -L$(LIBOGC_LIB) $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

#-------------------------------------------------------------------
# Collect source files for this platform
#-------------------------------------------------------------------
CFILES      := $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.c))
CPPFILES    := $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.cpp))
SFILES      := $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.s))
SFILES_UP   := $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.S))

# Object files will be computed by master Makefile
#-------------------------------------------------------------------
# Optional: Binary data files (images/fonts)
BINFILES    := $(foreach dir,$(DATA),$(wildcard $(dir)/*.*))

#-------------------------------------------------------------------
# Optional: custom rules for converting binary data (handled by master)
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
