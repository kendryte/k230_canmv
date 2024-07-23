include ${K230_CANMV_BUILD_DIR}/include/config/auto.conf

CROSS_COMPILE = ${K230_CANMV_ROOT}/k230_sdk/toolchain/riscv64-linux-musleabi_for_x86_64-pc-linux-gnu/bin/riscv64-unknown-linux-musl-

FREETYPE_DIR = ${K230_CANMV_ROOT}/micropython/port/3d-party/freetype/freetype/
BUILD = ${K230_CANMV_BUILD_DIR}/micropython/freetype/

RM = rm
ECHO = @echo
CP = cp
MKDIR = mkdir
SED = sed
CAT = cat
TOUCH = touch
PYTHON = python3
ZIP = zip

AS = $(CROSS_COMPILE)as
CC = $(CROSS_COMPILE)gcc
CPP = $(CC) -E
CXX = $(CROSS_COMPILE)g++
GDB = $(CROSS_COMPILE)gdb
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
SIZE = $(CROSS_COMPILE)size
STRIP = $(CROSS_COMPILE)strip
AR = $(CROSS_COMPILE)ar

SRC_DIRS :=

SFILES :=
SFILES_EXCLUDE :=
SFLAGS :=

CFILES :=
CFILES_EXCLUDE :=
CFLAGS :=

CFILES += \
      src/base/ftsystem.c \
      src/base/ftinit.c \
      src/base/ftdebug.c \
      src/base/ftbase.c \
      src/base/ftbbox.c \
      src/base/ftglyph.c \
      src/base/ftbitmap.c \
      src/base/ftmm.c \
      # src/base/ftbdf.c \
      src/base/ftcid.c \
      src/base/ftfstype.c \
      src/base/ftgasp.c \
      src/base/ftgxval.c \
      src/base/ftotval.c \
      src/base/ftpatent.c \
      src/base/ftpfr.c \
      src/base/ftstroke.c \
      src/base/ftsynth.c \
      src/base/fttype1.c \
      src/base/ftwinfnt.c \

CFILES += \
      src/sfnt/sfnt.c \
      src/truetype/truetype.c

CFILES += \
      src/smooth/smooth.c \
      # src/autofit/autofit.c \
      src/raster/raster.c \
      src/sdf/sdf.c \

CFILES += \
      src/cache/ftcache.c

CFLAGS += -Iinclude -DFT2_BUILD_LIBRARY

CWARN := 
# CWARN += -Wall -Werror
# CWARN += -Wno-int-conversion

CFLAGS += -std=gnu99 -fopenmp -march=rv64imafdcv -mabi=lp64d -mcmodel=medany $(CWARN)

SFILES_ALL = $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.S))
CFILES_ALL = $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.c))

SFILES += $(filter-out $(SFILES_EXCLUDE), $(SFILES_ALL))
CFILES += $(filter-out $(CFILES_EXCLUDE), $(CFILES_ALL))

SDEPS	:= $(patsubst %, $(BUILD)/%, $(SFILES:.S=.o.d))
CDEPS	:= $(patsubst %, $(BUILD)/%, $(CFILES:.c=.o.d))
DEPS	:= $(SDEPS) $(CDEPS)

SOBJS	:= $(patsubst %, $(BUILD)/%, $(SFILES:.S=.o))
COBJS	:= $(patsubst %, $(BUILD)/%, $(CFILES:.c=.o))
OBJS	:= $(SOBJS) $(COBJS)

LIB_FREETYPE = libfreetype.a

.PHONY: all lib prepare

all: prepare $(BUILD)/$(LIB_FREETYPE)

$(BUILD)/$(LIB_FREETYPE): $(OBJS) 
	@$(ECHO) "Link $(LIB_FREETYPE)"
	@$(LD) -r -o $@ $^

prepare:
	@set -e; \
	if [ ! -f $(BUILD)/include/ft2build.h ]; then \
		$(MKDIR) -p $(BUILD); \
		ln -sf $(FREETYPE_DIR)/include $(BUILD); \
      fi;

clean: $(OBJS) $(DEPS)
	@$(RM) $^

$(SOBJS) : $(BUILD)/%.o : %.S
	@echo [AS] $<
	@$(AS) $(SFLAGS) -MD -MP -MF $@.d -c $< -o $@

$(COBJS) : $(BUILD)/%.o : %.c
	@echo [CC] $<
	@$(CC) $(CFLAGS) -MD -MP -MF $@.d -c $< -o $@

# $(sort $(var)) removes duplicates
#
# The net effect of this, is it causes the objects to depend on the
# object directories (but only for existence), and the object directories
# will be created if they don't exist.
OBJ_DIRS = $(sort $(dir $(OBJS)))
$(OBJS): | $(OBJ_DIRS)
$(OBJ_DIRS):
	@$(MKDIR) -p $@

sinclude $(DEPS)
