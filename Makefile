#(C)2004-2005 AMX Mod X Development Team
# Makefile written by David "BAILOPAN" Anderson

HLSDK = ../../../hlsdk
MM_ROOT = ../../metamod/metamod

### EDIT BELOW FOR OTHER PROJECTS ###


OPT_FLAGS = -O3 -funroll-loops -s -pipe -fomit-frame-pointer -fno-strict-aliasing -DNDEBUG

DEBUG_FLAGS = -g -ggdb3
CPP = gcc-4.1
#CPP = gcc-2.95
NAME = weaponmod

BIN_SUFFIX = amxx_i386.so

OBJECTS = sdk/amxxmodule.cpp  BSP_parse_ents.cpp \
CVirtHook.cpp  dllFunc.cpp  meta_api.cpp  natives.cpp \
srvcmd.cpp  utils.cpp  weaponmod.cpp


LINK = 

INCLUDE = -I. -I$(HLSDK) -I$(HLSDK)/dlls -I$(HLSDK)/engine -I$(HLSDK)/game_shared -I$(HLSDK)/game_shared \
	-I$(MM_ROOT) -I$(HLSDK)/common -I$(HLSDK)/pm_shared -I./tableentries -Isdk

GCC_VERSION := $(shell $(CPP) -dumpversion >&1 | cut -b1)

ifeq "$(DEBUG)" "true"
	BIN_DIR = Debug
	CFLAGS = $(DEBUG_FLAGS)
else
	BIN_DIR = Release

	ifeq "$(GCC_VERSION)" "4"
		OPT_FLAGS += -fvisibility=hidden -fvisibility-inlines-hidden
	endif
	CFLAGS = $(OPT_FLAGS)
endif

CFLAGS += -Wall -Wno-non-virtual-dtor -fno-exceptions -DHAVE_STDINT_H -fno-rtti -m32  -D_stricmp=strcasecmp -D_strnicmp=strncasecmp

BINARY = $(NAME)_$(BIN_SUFFIX)
CFLAGS += -DPAWN_CELL_SIZE=32 -DJIT -DASM32
OPT_FLAGS += -march=i586

OBJ_LINUX := $(OBJECTS:%.cpp=$(BIN_DIR)/%.o)

$(BIN_DIR)/%.o: %.cpp
	$(CPP) $(INCLUDE) $(CFLAGS) -o $@ -c $<

all:
	mkdir -p $(BIN_DIR)
	mkdir -p $(BIN_DIR)/sdk
	$(MAKE) weaponmod

weaponmod: $(OBJ_LINUX)
	$(CPP) $(INCLUDE) $(CFLAGS) $(OBJ_LINUX) $(LINK) -shared -ldl -lm -o$(BIN_DIR)/$(BINARY)

debug:	
	$(MAKE) all DEBUG=true

default: all

clean:
	rm -rf Release/*.o
	rm -rf Release/sdk/*.o
	rm -rf Release/$(NAME)_$(BIN_SUFFIX)
	rm -rf Debug/*.o
	rm -rf Debug/sdk/*.o
	rm -rf Debug/$(NAME)_$(BIN_SUFFIX)
	
