include ${K230_CANMV_BUILD_DIR}/include/config/auto.conf

CROSS_COMPILE = ${K230_CANMV_ROOT}/k230_sdk/toolchain/riscv64-linux-musleabi_for_x86_64-pc-linux-gnu/bin/riscv64-unknown-linux-musl-

BOARD_DIR = boards/${CONFIG_BOARD}
BUILD = ${K230_CANMV_BUILD_DIR}/micropython

include ../micropython/py/mkenv.mk

# Use the default frozen manifest, variants may override this.
FROZEN_MANIFEST ?= ${BOARD_DIR}/manifest.py

# This should be configured by the mpconfigvariant.mk
PROG ?= micropython

# include py core make definitions
include $(TOP)/py/py.mk
include $(TOP)/extmod/extmod.mk

INC += -I.
INC += -I$(TOP)
INC += -I$(BUILD)
INC += -I${BOARD_DIR}
INC += -Iinclude/core
INC += -Iinclude/machine
INC += -Iinclude/maix
INC += -Iinclude/mpp
INC += -Iinclude/kpu
INC += -I$(K230_CANMV_ROOT)/k230_sdk/src/big/mpp/include
INC += -I$(K230_CANMV_ROOT)/k230_sdk/src/big/mpp/include/comm
INC += -I$(K230_CANMV_ROOT)/k230_sdk/src/big/mpp/include/ioctl
INC += -I$(K230_CANMV_ROOT)/k230_sdk/src/big/mpp/userapps/api

# compiler settings
CWARN = -Wall -Werror
CWARN += -Wno-unused-parameter -Wpointer-arith -Wdouble-promotion -Wfloat-conversion
CFLAGS += -fopenmp -march=rv64imafdcv -mabi=lp64d -mcmodel=medany
CFLAGS += $(INC) $(CWARN) -std=gnu99 -DUNIX $(COPT) $(CFLAGS_EXTRA)

# Debugging/Optimization
ifdef DEBUG
COPT ?= -Og
else
COPT ?= -Os
COPT += -DNDEBUG
endif

# Remove unused sections.
COPT += -fdata-sections -ffunction-sections

# Always enable symbols -- They're occasionally useful, and don't make it into the
# final .bin/.hex/.dfu so the extra size doesn't matter.
CFLAGS += -g

# Use gcc syntax for map file
LDFLAGS_ARCH = -Wl,-Map=$@.map,--cref -Wl,--gc-sections
LDFLAGS += -T core/link.lds --static
LDFLAGS += $(LDFLAGS_MOD) $(LDFLAGS_ARCH) -lm $(LDFLAGS_EXTRA)

LDFLAGS_MPP = -L$(K230_CANMV_ROOT)/k230_sdk/src/big/mpp/userapps/lib \
	-lvo -lsys

LDFLAGS += $(LDFLAGS_MPP)

# Flags to link with pthread library
LIBPTHREAD = -lpthread

ifeq ($(MICROPY_PY_TERMIOS),1)
CFLAGS += -DMICROPY_PY_TERMIOS=1
endif
ifeq ($(MICROPY_PY_SOCKET),1)
CFLAGS += -DMICROPY_PY_SOCKET=1
endif
ifeq ($(MICROPY_PY_THREAD),1)
CFLAGS += -DMICROPY_PY_THREAD=1 -DMICROPY_PY_THREAD_GIL=0
LDFLAGS += $(LIBPTHREAD)
endif

# source files
SRC_C += \
	core/main.c \
	core/gccollect.c \
	core/mphalport.c \
	core/input.c \
	$(wildcard $(BOARD_DIR)/*.c)

MACHINE_SRC_C = $(wildcard machine/*.c)
MAIX_SRC_C = $(wildcard maix/*.c)
MPP_SRC_C = $(wildcard mpp/*.c)
KPU_SRC_C = $(wildcard kpu/*.c)

SRC_C += \
	${MACHINE_SRC_C} \
	${MAIX_SRC_C} \
	${MPP_SRC_C} \
	${KPU_SRC_C}

SHARED_SRC_C += $(addprefix shared/,\
	runtime/gchelper_generic.c \
	timeutils/timeutils.c \
	readline/readline.c \
	runtime/pyexec.c \
	)

SRC_CXX += \

OBJ = $(PY_O)
OBJ += $(addprefix $(BUILD)/, $(SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_CXX:.cpp=.o))
OBJ += $(addprefix $(BUILD)/, $(SHARED_SRC_C:.c=.o))

# List of sources for qstr extraction
SRC_QSTR += $(SRC_C) $(SRC_CXX) $(SHARED_SRC_C)
# Append any auto-generated sources that are needed by sources listed in
# SRC_QSTR
SRC_QSTR_AUTO_DEPS +=

CXXFLAGS += $(filter-out -Wmissing-prototypes -Wold-style-definition -std=gnu99,$(CFLAGS) $(CXXFLAGS_MOD))

include $(TOP)/py/mkrules.mk
