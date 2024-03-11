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
INC += -Iinclude/ai_cube
INC += -Iinclude/ai_demo
INC += -Iinclude/ai_demo/kws
INC += -Iinclude/ai_demo/tts_zh
INC += -I$(K230_CANMV_ROOT)/k230_sdk/src/big/utils/lib/opencv/include/opencv4
INC += -I$(K230_CANMV_ROOT)/k230_sdk/src/big/mpp/include
INC += -I$(K230_CANMV_ROOT)/k230_sdk/src/big/mpp/include/comm
INC += -I$(K230_CANMV_ROOT)/k230_sdk/src/big/mpp/include/ioctl
INC += -I$(K230_CANMV_ROOT)/k230_sdk/src/big/mpp/userapps/api
INC += -I$(K230_CANMV_ROOT)/k230_sdk/src/common/cdk/user/component/ipcmsg/include/
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
CXXFLAGS += $(INC) $(COPT) $(NNCASEINC) -std=c++20

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
CDK_LIB_PATH = $(K230_CANMV_ROOT)/k230_sdk/src/common/cdk/user/component/ipcmsg/host/lib/

MPP_LIBS = $(addprefix -l, $(subst lib, ,$(basename $(notdir $(foreach dir, $(MPP_LIB_PATH) $(CDK_LIB_PATH), $(wildcard $(dir)/*))))))

LDFLAGS_MPP = -L$(MPP_LIB_PATH) -L$(CDK_LIB_PATH) -Wl,--start-group $(MPP_LIBS) -Wl,--end-group

LDFLAGS += $(LDFLAGS_MPP)
LDFLAGS += -L${K230_CANMV_ROOT}/k230_sdk/src/big/nncase/riscv64/nncase/lib -lNncase.Runtime.Native -lnncase.rt_modules.k230 -lfunctional_k230 -lstdc++
MP4_LIB_PATH = $(K230_CANMV_ROOT)/k230_sdk/src/big/middleware/mp4_format/lib
LDFLAGS_MP4 += -L$(MP4_LIB_PATH) -lmp4

LDFLAGS += $(LDFLAGS_MPP) $(LDFLAGS_MP4)

OPENCV_LIB_PATH = $(K230_CANMV_ROOT)/k230_sdk/src/big/utils/lib/opencv/lib
OPENCV_DEP_LIB_PATH = $(K230_CANMV_ROOT)/k230_sdk/src/big/utils/lib/opencv/lib/opencv4/3rdparty
OPENCV_LIBS = -lstdc++ -lopencv_core -lopencv_imgcodecs -lopencv_imgproc -lopencv_highgui -lopencv_videoio -lzlib -llibjpeg-turbo -llibopenjp2 -llibpng -llibtiff -llibwebp -lcsi_cv -latomic
LDFLAGS += -L$(OPENCV_LIB_PATH) -L$(OPENCV_DEP_LIB_PATH) -Wl,--start-group $(OPENCV_LIBS) -Wl,--end-group

ifeq ($(MICROPY_PY_TERMIOS),1)
CFLAGS += -DMICROPY_PY_TERMIOS=1
endif
ifeq ($(MICROPY_PY_SOCKET_CANMV),1)
CFLAGS += -DMICROPY_PY_SOCKET_CANMV=1 -g
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
AICUBE_SRC_C = $(wildcard ai_cube/*.c)
AIDEMO_SRC_C = $(wildcard ai_demo/*.c)
SOCKET_SRC_C = $(wildcard socket_network/*.c)

OMV_SRC_C += $(wildcard omv/*/*.c)
INC += -Iomv/alloc -Iomv/boards/canmv -Iomv/common -Iomv/imlib -Iomv/modules

# LVGL build rules
LVGL_BINDING_DIR = $(K230_CANMV_ROOT)/micropython_port/lvgl/lv_binding_micropython
LVGL_DIR = $(LVGL_BINDING_DIR)/lvgl
LVGL_GENERIC_DRV_DIR = $(LVGL_BINDING_DIR)/driver/generic
INC += -I$(LVGL_BINDING_DIR)
ALL_LVGL_SRC = $(shell find $(LVGL_DIR) -type f -name '*.h') $(LVGL_BINDING_DIR)/lv_conf.h
LVGL_PP = $(BUILD)/lvgl/lvgl.pp.c
LVGL_MPY = $(BUILD)/lvgl/lv_mpy.c
LVGL_MPY_METADATA = $(BUILD)/lvgl/lv_mpy.json

$(LVGL_MPY): $(ALL_LVGL_SRC) $(LVGL_BINDING_DIR)/gen/gen_mpy.py
	$(ECHO) "LVGL-GEN $@"
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CPP) $(LV_CFLAGS) -DPYCPARSER -x c -I $(LVGL_BINDING_DIR)/pycparser/utils/fake_libc_include $(INC) $(LVGL_DIR)/lvgl.h > $(LVGL_PP)
	$(Q)python3.8 $(LVGL_BINDING_DIR)/gen/gen_mpy.py -M lvgl -MP lv -MD $(LVGL_MPY_METADATA) -E $(LVGL_PP) $(LVGL_DIR)/lvgl.h > $@

LVGL_SRC_C = $(subst $(TOP)/,,$(shell find $(LVGL_DIR)/src -type f -name "*.c") $(LVGL_MPY))

SRC_C += \
	${MACHINE_SRC_C} \
	${MAIX_SRC_C} \
	${MPP_SRC_C} \
	${KPU_SRC_C}  ${SOCKET_SRC_C} \
	${OMV_SRC_C} \
	${AICUBE_SRC_C} \
	${AIDEMO_SRC_C} \
	${LVGL_SRC_C}

SHARED_SRC_C += $(addprefix shared/,\
	runtime/gchelper_generic.c \
	timeutils/timeutils.c \
	readline/readline.c \
	runtime/pyexec.c \
	)

KPU_SRC_CXX = $(wildcard kpu/*.cpp)
SRC_CXX += ${KPU_SRC_CXX}
AICUBE_SRC_CXX = $(wildcard ai_cube/*.cpp)
SRC_CXX += ${AICUBE_SRC_CXX}
AIDEMO_SRC_CXX = $(wildcard ai_demo/*.cpp)
SRC_CXX += ${AIDEMO_SRC_CXX}
AIDEMO_KWS_SRC_CXX = $(wildcard ai_demo/kws/*.cpp)
SRC_CXX += ${AIDEMO_KWS_SRC_CXX}
AIDEMO_TTS_ZH_SRC_CXX = $(wildcard ai_demo/tts_zh/*.cpp)
SRC_CXX += ${AIDEMO_TTS_ZH_SRC_CXX}

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
