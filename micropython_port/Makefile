DEBUG = 1
include ${K230_CANMV_BUILD_DIR}/include/config/auto.conf

CROSS_COMPILE = ${K230_CANMV_ROOT}/k230_sdk/toolchain/riscv64-linux-musleabi_for_x86_64-pc-linux-gnu/bin/riscv64-unknown-linux-musl-

BOARD_DIR = boards/${CONFIG_BOARD}
BUILD = ${K230_CANMV_BUILD_DIR}/micropython

include ../micropython/py/mkenv.mk

# Use the default frozen manifest, variants may override this.
FROZEN_MANIFEST ?= ${BOARD_DIR}/manifest.py
include boards/mpconfigport.mk
-include ${BOARD_DIR}/mpconfigport.mk

# This should be configured by the mpconfigvariant.mk
PROG ?= micropython

USER_C_MODULES = ulab

# include py core make definitions
include $(TOP)/py/py.mk
include $(TOP)/extmod/extmod.mk

INC += -I.
INC += -I$(TOP)
INC += -I$(TOP)/py
INC += -I$(TOP)/shared/readline
INC += -I$(TOP)/lib/crypto-algorithms
INC += -I$(BUILD)
INC += -I${BOARD_DIR}
INC += -Iinclude/core
INC += -Iinclude/machine
INC += -Iinclude/maix
INC += -Iinclude/mpp
INC += -Iinclude/kpu
INC += -Iinclude/omv
INC += -I$(K230_CANMV_ROOT)/k230_sdk/src/big/mpp/include
INC += -I$(K230_CANMV_ROOT)/k230_sdk/src/big/mpp/include/comm
INC += -I$(K230_CANMV_ROOT)/k230_sdk/src/big/mpp/include/ioctl
INC += -I$(K230_CANMV_ROOT)/k230_sdk/src/big/mpp/userapps/api
INC += -I$(K230_CANMV_ROOT)/micropython_port/ulab
INC += -I$(K230_CANMV_ROOT)/k230_sdk/src/big/rt-smart/userapps
INC += -I$(K230_CANMV_ROOT)/k230_sdk/src/big/rt-smart/userapps/sdk/rt-thread/include
INC += -I$(K230_CANMV_ROOT)/k230_sdk/src/big/rt-smart/userapps/sdk/rt-thread/components/drivers
INC += -I$(K230_CANMV_ROOT)/k230_sdk/src/big/middleware/mp4_format/include

# compiler settings
CWARN = -Wall -Werror
CWARN += -Wno-int-conversion
CFLAGS += -fopenmp -march=rv64imafdcv -mabi=lp64d -mcmodel=medany
CFLAGS += $(INC) -std=gnu99 $(COPT) $(CFLAGS_EXTRA)

NNCASEINC += -I$(K230_CANMV_ROOT)/k230_sdk/src/big/nncase/riscv64
NNCASEINC += -I$(K230_CANMV_ROOT)/k230_sdk/src/big/nncase/riscv64/nncase/include
CXXFLAGS += -fopenmp -march=rv64imafdcv -mabi=lp64d -mcmodel=medany
CXXFLAGS += $(INC) $(NNCASEINC) -std=c++20

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
CFLAGS += -g -gdwarf-2

# Use gcc syntax for map file
LDFLAGS_ARCH = -Wl,-Map=$@.map,--cref -Wl,--gc-sections
LDFLAGS += -T core/link.lds --static
LDFLAGS += $(LDFLAGS_MOD) $(LDFLAGS_ARCH) -lm -lpthread $(LDFLAGS_EXTRA)

MPP_LIB_PATH = $(K230_CANMV_ROOT)/k230_sdk/src/big/mpp/userapps/lib
MPP_LIBS = $(addprefix -l, $(subst lib, ,$(basename $(notdir $(foreach dir, $(MPP_LIB_PATH), $(wildcard $(dir)/*))))))

LIBPATH = $(K230_CANMV_ROOT)/k230_sdk/src/big/rt-smart/userapps/sdk/rt-thread/lib/risc-v/rv64
MPP_LIBS += -lrtthread

LDFLAGS_MPP = -L$(MPP_LIB_PATH) -L$(LIBPATH) -Wl,--start-group $(MPP_LIBS) -Wl,--end-group

LDFLAGS += $(LDFLAGS_MPP)
LDFLAGS += -L${K230_CANMV_ROOT}/k230_sdk/src/big/nncase/riscv64/nncase/lib -lNncase.Runtime.Native -lnncase.rt_modules.k230 -lfunctional_k230 -lstdc++
MP4_LIB_PATH = $(K230_CANMV_ROOT)/k230_sdk/src/big/middleware/mp4_format/lib
LDFLAGS_MP4 += -L$(MP4_LIB_PATH) -lmp4

LDFLAGS += $(LDFLAGS_MPP) $(LDFLAGS_MP4)

ifeq ($(MICROPY_PY_TERMIOS),1)
CFLAGS += -DMICROPY_PY_TERMIOS=1
endif
ifeq ($(MICROPY_PY_SOCKET),1)
CFLAGS += -DMICROPY_PY_SOCKET=1 
endif
CFLAGS += -DHAVE_CCONFIG_H 
# source files
SRC_C += \
	core/main.c \
	core/gccollect.c \
	core/mphalport.c \
	core/input.c \
	core/mpthreadport.c \
	$(wildcard $(BOARD_DIR)/*.c)

MACHINE_SRC_C = $(wildcard machine/*.c)
MAIX_SRC_C = $(wildcard maix/*.c)
MPP_SRC_C = $(wildcard mpp/*.c)
KPU_SRC_C = $(wildcard kpu/*.c)
OMV_SRC_C = $(wildcard omv/*.c)

OMV_SRC_C += $(wildcard omv/*/*.c)
INC += -Iomv/alloc -Iomv/boards/canmv -Iomv/common -Iomv/imlib -Iomv/modules

SRC_C += \
	${MACHINE_SRC_C} \
	${MAIX_SRC_C} \
	${MPP_SRC_C} \
	${KPU_SRC_C} \
	${OMV_SRC_C}

SHARED_SRC_C += $(addprefix shared/,\
	runtime/gchelper_generic.c \
	timeutils/timeutils.c \
	readline/readline.c \
	runtime/pyexec.c \
	)

KPU_SRC_CXX = $(wildcard kpu/*.cpp)
SRC_CXX += ${KPU_SRC_CXX}

OBJ = $(PY_O)
OBJ += $(addprefix $(BUILD)/, $(SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_CXX:.cpp=.o))
OBJ += $(addprefix $(BUILD)/, $(SHARED_SRC_C:.c=.o))

# List of sources for qstr extraction
SRC_QSTR += $(SRC_C) $(SRC_CXX) $(SHARED_SRC_C)
# Append any auto-generated sources that are needed by sources listed in
# SRC_QSTR
SRC_QSTR_AUTO_DEPS +=

CXXFLAGS += $(filter-out -Wmissing-prototypes -Wold-style-definition -std=gnu99, $(CXXFLAGS_MOD))
CFLAGS += $(CWARN)
include $(TOP)/py/mkrules.mk
