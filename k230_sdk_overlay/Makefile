red=\e[1;31m
NC=\e[0m

SHELL=/bin/bash

BUILD_TYPE ?= product
NATIVE_BUILD ?= 1

ifeq ($(NATIVE_BUILD),1)
DOWNLOAD_URL=https://ai.b-bug.org/k230
else
DOWNLOAD_URL?=https://kendryte-download.canaan-creative.com/k230
endif

RTT_TOOLCHAIN_URL = $(DOWNLOAD_URL)/toolchain/riscv64-unknown-linux-musl-rv64imafdcv-lp64d-20230420.tar.bz2
LINUX_TOOLCHAIN_URL = $(DOWNLOAD_URL)/toolchain/Xuantie-900-gcc-linux-5.10.4-glibc-x86_64-V2.6.0.tar.bz2

export K230_SDK_ROOT := $(shell pwd)
ifeq ("$(origin CONF)", "command line")
$(shell echo CONF=$(CONF)>.last_conf;)
else
$(shell [ -f .last_conf ] || echo CONF=k230_evb_defconfig>.last_conf )
endif

ifneq ($(wildcard .last_conf),)
include .last_conf
endif

CONF ?= k230_evb_defconfig
export BUILD_DIR := $(K230_SDK_ROOT)/output/$(CONF)

DEFCONFIG = configs/$(CONF)

include repo.mak
include parse.mak

ADD_PATH := :$(RTT_EXEC_PATH):$(LINUX_EXEC_PATH)
TMP_PATH := $(addsuffix $(ADD_PATH), $(PATH))
export PATH=$(TMP_PATH)
export RTSMART_SRC_DIR=$(K230_SDK_ROOT)/$(RT-SMART_SRC_PATH)
export CDK_SRC_DIR=$(K230_SDK_ROOT)/$(CDK_SRC_PATH)
export MPP_SRC_DIR=$(K230_SDK_ROOT)/$(MPP_SRC_PATH)



KCONFIG_PATH= tools/kconfig/
KCONFIG_MCONF_EXE = tools/kconfig/mconf
KCONFIG_CFG = Kconfig



define CLEAN
	set -e; \
	echo "clean test"

endef

define BUILD_IMAGE
	set -e; \
	echo "build SDK images"

endef

#include .config

.PHONY: all
all .DEFAULT: prepare_memory mpp rt-smart-apps rt-smart-kernel big-core-opensbi uboot build-image

.PHONY: download_toolchain
download_toolchain:
	@set -e; \
	if [ ! -f toolchain/.toolchain_ready ];then \
	echo "download toolchain"; \
	wget -c -P $(K230_SDK_ROOT)/toolchain $(RTT_TOOLCHAIN_URL); \
	wget -c -P $(K230_SDK_ROOT)/toolchain $(LINUX_TOOLCHAIN_URL); \
	fi;

.PHONY: extract_toolchain
extract_toolchain: download_toolchain
	@set -e; \
	if [ ! -f toolchain/.toolchain_ready ];then \
	echo "extract toolchain"; \
	for file in $(shell find ./toolchain -name "*.bz2"); do echo $$file;tar jxf $$file -C $(K230_SDK_ROOT)/toolchain; done; \
	fi;

.PHONY: prepare_toolchain
prepare_toolchain: extract_toolchain
	@if [ ! -f toolchain/.toolchain_ready ];then \
	touch toolchain/.toolchain_ready; \
	fi;
	@echo "toolchain is ready"

.PHONY: prepare_sourcecode check_toolchain
prepare_sourcecode:prepare_toolchain
	@echo "prepare source code"
#ai
	@echo "download nncase sdk"
	@rm -rf src/big/utils/; rm -rf src/big/ai;
	@wget -qc $(DOWNLOAD_URL)/downloads/kmodel/kmodel_v2.1.0.tgz -O - | tar -xzC src/big/
	@wget -qc $(DOWNLOAD_URL)/downloads/nncase/nncase_k230_v2.1.0.tgz -O - | tar -xzC src/big/

#big utils
	@echo "download big utils"
	@wget -qc $(DOWNLOAD_URL)/downloads/big/utils/utils.tar.gz -O - | tar -xzC src/big/
	@cd src/big/utils/lib/;ln -s opencv_thead opencv;cd - >> /dev/null
	@touch src/.src_fetched

check_toolchain:
	@if  [ ! -f  $(CONFIG_TOOLCHAIN_PATH_LINUX)/$(CONFIG_TOOLCHAIN_PREFIX_LINUX)gcc ] || \
		 [ !   -f $(CONFIG_TOOLCHAIN_PATH_RTT)/$(CONFIG_TOOLCHAIN_PREFIX_RTT)gcc  ]; then \
		 echo "please run command: source tools/get_download_url.sh && make prepare_sourcecode"; exit 1;  \
	fi;


#.PHONY: check_src
check_src:check_toolchain
	@if [ ! -f src/.src_fetched ];then \
	echo "Please run command: source tools/get_download_url.sh && make prepare_sourcecode";exit 1; \
	fi;


#.PHONY: defconfig
defconfig:   $(DEFCONFIG)   .last_conf
	@cp $(DEFCONFIG) .config;
	@touch $@

.PHONY: savedefconfig
savedefconfig:
	@cp .config $(DEFCONFIG)

.PHONY: prepare_menuconfig
prepare_menuconfig:
	@if [ ! -f $(KCONFIG_MCONF_EXE) ];then cd $(KCONFIG_PATH);make mconf conf;cd -;fi

.PHONY: menuconfig
menuconfig: prepare_menuconfig
	@$(KCONFIG_MCONF_EXE) $(KCONFIG_CFG)



#.PHONY: prepare_memory
prepare_memory: defconfig  .config  tools/menuconfig_to_code.sh  parse.mak
	@echo "prepare memory"
	@if [ ! -f tools/kconfig/conf ];then cd $(KCONFIG_PATH);make  conf;cd -;fi
	@mkdir -p  include/generated/  include/config/;
	@./tools/kconfig/conf --silentoldconfig  --olddefconfig $(KCONFIG_CFG)
	@cp include/generated/autoconf.h src/little/uboot/board/canaan/k230_evb/sdk_autoconf.h
#	#@cp include/generated/autoconf.h src/little/buildroot-ext/package/feature_opreation/src/sdk_autoconf.h
	@rm -rf include;
	@$(call CONFIG_MEM_RTT)
	@touch $@

.PHONY: mpp-kernel
mpp-kernel: check_src
	@export PATH=$(RTT_EXEC_PATH):$(PATH); \
	export RTSMART_SRC_DIR=$(K230_SDK_ROOT)/$(RT-SMART_SRC_PATH); \
	cd $(MPP_SRC_PATH); \
	make -C kernel || exit $?; \
	cp kernel/lib/* $(RTSMART_SRC_DIR)/kernel/bsp/maix3/board/mpp/lib/; \
	cd -;

.PHONY: mpp-kernel-clean
mpp-kernel-clean:
	@export PATH=$(RTT_EXEC_PATH):$(PATH); \
	export RTSMART_SRC_DIR=$(K230_SDK_ROOT)/$(RT-SMART_SRC_PATH); \
	cd $(MPP_SRC_DIR); \
	make clean -C kernel; \
	rm $(RTSMART_SRC_DIR)/kernel/bsp/maix3/board/mpp/lib/* -rf; \
	cd -;

.PHONY: mpp-apps
mpp-apps:check_src
	@export PATH=$(RTT_EXEC_PATH):$(PATH); \
	export RTSMART_SRC_DIR=$(K230_SDK_ROOT)/$(RT-SMART_SRC_PATH); \
	cd $(MPP_SRC_DIR); \
	make -C userapps/src || exit $?; \
	cd -;

.PHONY: mpp-apps-clean
mpp-apps-clean:
	@export PATH=$(RTT_EXEC_PATH):$(PATH); \
	export RTSMART_SRC_DIR=$(K230_SDK_ROOT)/$(RT-SMART_SRC_PATH); \
	cd $(MPP_SRC_DIR); \
	make clean -C userapps/src; \
	cd -;

.PHONY: mpp
mpp: mpp-kernel mpp-apps

.PHONY: mpp-clean
mpp-clean: mpp-kernel-clean mpp-apps-clean

.PHONY: rt-smart-apps
rt-smart-apps: defconfig prepare_memory  check_src
	@export RTT_CC=$(RTT_CC); \
	export RTT_CC_PREFIX=$(RTT_CC_PREFIX); \
	export RTT_EXEC_PATH=$(RTT_EXEC_PATH); \
	cp -rf $(UNITTEST_SRC_PATH)/testcases $(RT-SMART_SRC_PATH)/userapps; \
	cd $(RT-SMART_SRC_PATH)/userapps; \
	cp configs/def_config_riscv64 .config; \
	scons -j16 || exit $?; \
	cd -;
	mkdir -p $(RT-SMART_SRC_PATH)/userapps/root;
	python3 $(RT-SMART_SRC_PATH)/tools/mkromfs.py $(RT-SMART_SRC_PATH)/userapps/root $(RT-SMART_SRC_PATH)/kernel/bsp/maix3/applications/romfs.c

.PHONY: rt-smart-apps-clean
rt-smart-apps-clean: defconfig
	@cd $(RT-SMART_SRC_PATH)/userapps;scons -c;cd -


.PHONY: rt-smart-kernel
rt-smart-kernel: defconfig  prepare_memory  check_src
	@export RTT_CC=$(RTT_CC); \
	export RTT_CC_PREFIX=$(RTT_CC_PREFIX); \
	export RTT_EXEC_PATH=$(RTT_EXEC_PATH); \
	cd $(RT-SMART_SRC_PATH)/kernel/bsp/maix3; \
	rm -f rtthread.elf; \
	scons -j16 || exit $?; \
	mkdir -p $(RTT_SDK_BUILD_DIR); \
	mkdir -p $(RTSMART_SRC_DIR)/userapps/app; \
	cp rtthread.bin rtthread.elf $(RTT_SDK_BUILD_DIR)/; \
	cd -;


.PHONY: rt-smart-kernel-clean
rt-smart-kernel-clean: defconfig prepare_memory
	@export RTT_CC=$(RTT_CC);export RTT_CC_PREFIX=$(RTT_CC_PREFIX);export RTT_EXEC_PATH=$(RTT_EXEC_PATH); \
	cd $(RT-SMART_SRC_PATH)/kernel/bsp/maix3;scons -c;cd -

.PHONY: big-core-opensbi
big-core-opensbi: rt-smart-kernel
	@mkdir -p $(BIG_OPENSBI_BUILD_DIR); \
	cp $(RT-SMART_SRC_PATH)/kernel/bsp/maix3/rtthread.bin $(OPENSBI_SRC_PATH)/; \
	cd $(OPENSBI_SRC_PATH); \
	export CROSS_COMPILE=$(LINUX_EXEC_PATH)/$(LINUX_CC_PREFIX); \
	export PLATFORM=kendryte/fpgac908; \
	make FW_FDT_PATH=hw.dtb FW_PAYLOAD_PATH=rtthread.bin O=$(BIG_OPENSBI_BUILD_DIR) OPENSBI_QUIET=1 || exit $?; \
	cd -

.PHONY: big-core-opensbi-clean
big-core-opensbi-clean:
	rm -rf $(BIG_OPENSBI_BUILD_DIR)

.PHONY:rt-smart
rt-smart: mpp rt-smart-apps big-core-opensbi

.PHONY:rt-smart-clean
rt-smart-clean: mpp-clean big-core-opensbi-clean rt-smart-kernel-clean rt-smart-apps-clean

.PHONY: uboot
uboot: defconfig prepare_memory check_src
	@export PATH=$(LINUX_EXEC_PATH):$(PATH);export CROSS_COMPILE=$(LINUX_CC_PREFIX);export ARCH=riscv; \
	cd $(UBOOT_SRC_PATH); \
	make $(UBOOT_DEFCONFIG) O=$(UBOOT_BUILD_DIR) || exit $?;make -C $(UBOOT_BUILD_DIR) || exit $?; \
	cd -

.PHONY: uboot-rebuild
uboot-rebuild: defconfig  prepare_memory  check_src
	@export PATH=$(LINUX_EXEC_PATH):$(PATH);export CROSS_COMPILE=$(LINUX_CC_PREFIX);export ARCH=riscv; \
	cd $(UBOOT_SRC_PATH); \
	make -C $(UBOOT_BUILD_DIR) || exit $?; \
	cd -

.PHONY: uboot-menuconfig
uboot-menuconfig: defconfig  prepare_memory check_src
	@export PATH=$(LINUX_EXEC_PATH):$(PATH);export CROSS_COMPILE=$(LINUX_CC_PREFIX);export ARCH=riscv; \
	cd $(UBOOT_SRC_PATH); \
	make -C $(UBOOT_BUILD_DIR) menuconfig; \
	cd -

.PHONY: uboot-savedefconfig
uboot-savedefconfig: defconfig  prepare_memory
	@export PATH=$(LINUX_EXEC_PATH):$(PATH);export CROSS_COMPILE=$(LINUX_CC_PREFIX);export ARCH=riscv; \
	cd $(UBOOT_SRC_PATH); \
	make -C $(UBOOT_BUILD_DIR) savedefconfig; \
	cd -

.PHONY: uboot-clean
uboot-clean: defconfig
	@export PATH=$(LINUX_EXEC_PATH):$(PATH);export CROSS_COMPILE=$(LINUX_CC_PREFIX);export ARCH=riscv; \
	cd $(UBOOT_SRC_PATH); \
	make -C $(UBOOT_BUILD_DIR) clean; \
	cd -

.PHONY: build-image
build-image: defconfig  prepare_memory  check_src
	mkdir -p $(BUILD_DIR)/images/big-core
	cp -rf $(RT-SMART_SRC_PATH)/userapps/root $(BUILD_DIR)/images/big-core/
	cp -f $(RTT_SDK_BUILD_DIR)/rtthread.* $(BUILD_DIR)/images/big-core/
	cp -rf $(BIG_OPENSBI_BUILD_DIR)/ $(BUILD_DIR)/images/big-core/
	$(K230_SDK_ROOT)/tools/gen_image.sh; \
	cd $(K230_SDK_ROOT)

.PHONY: clean
clean:
	@rm -rf defconfig
	@rm -rf prepare_memory
	@$(call CLEAN)



help:
	@echo "Usage: "
	@echo "make CONF=k230_evb_defconfig --$$(ls configs | tr '\n' '/')"
	@echo "make"
	@echo "Supported compilation options"
	@echo "make                          -- Build all for k230";
	@echo "make prepare_sourcecode       -- down source code";
	@echo "make big-core-opensbi         -- Build big core opensbi for k230";
	@echo "make mpp-apps                 -- Build mpp kernel driver user api lib and sample code for k230";
	@echo "make rt-smart                 -- Build mpp rtsmart kernel and userapps and opensbi for k230";
	@echo "make rt-smart-kernel          -- Build rtsmart kernel for k230";
	@echo "make rt-smart-apps            -- Build rtsmart userapps for k230";
	@echo "make uboot                    -- Build k230 uboot code with defconfig";
	@echo "make uboot-menuconfig         -- Menuconfig for k230 uboot, select save will save to output/k230_evb_defconfig/little/uboot/.config";
	@echo "make uboot-savedefconfig      -- Save uboot configuration to output/k230_evb_defconfig/little/uboot/defconfig";
	@echo "make uboot-rebuild            -- Rebuild k230 uboot";
	@echo "make uboot-clean              -- Carry out clean in k230 uboot build directory, run make uboot-rebuild will build all source code";
	@echo "make build-image              -- Build k230 rootfs image";
