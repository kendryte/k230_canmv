FAST_DL ?= 1
NATIVE_BUILD ?= 1
CONF ?= k230_evb_defconfig
SAVECONF ?= ${CONF}

export K230_CANMV_ROOT := $(shell pwd)
export K230_CANMV_BUILD_DIR := $(K230_CANMV_ROOT)/output/$(CONF)
$(shell mkdir -p ${K230_CANMV_BUILD_DIR})

CONFIG_PATH = ${K230_CANMV_ROOT}/configs
KCONFIG_PATH = ${K230_CANMV_ROOT}/scripts/kconfig
KCONFIG_CFG = ${K230_CANMV_ROOT}/Kconfig

.PHONY: all
all: build-image

${K230_CANMV_BUILD_DIR}/.config: ${CONFIG_PATH}/${CONF}
	@if [ ! -f $(KCONFIG_PATH)/mconf -o ! -f $(KCONFIG_PATH)/conf ];then \
	cd $(KCONFIG_PATH); make; fi
	@cd ${K230_CANMV_BUILD_DIR}; \
	$(KCONFIG_PATH)/conf --defconfig ${CONFIG_PATH}/${CONF} $(KCONFIG_CFG);

${K230_CANMV_BUILD_DIR}/.config.old: ${K230_CANMV_BUILD_DIR}/.config
	@cd ${K230_CANMV_BUILD_DIR}; \
	$(KCONFIG_PATH)/conf --syncconfig $(KCONFIG_CFG); \
	touch .config.old;

.PHONY: menuconfig
menuconfig: ${K230_CANMV_BUILD_DIR}/.config
	@cd ${K230_CANMV_BUILD_DIR}; \
	$(KCONFIG_PATH)/mconf $(KCONFIG_CFG);

.PHONY: savedefconfig
savedefconfig: ${K230_CANMV_BUILD_DIR}/.config
	@cd ${K230_CANMV_BUILD_DIR}; \
	$(KCONFIG_PATH)/conf --savedefconfig ${CONFIG_PATH}/${SAVECONF} $(KCONFIG_CFG);

.PHONY: .autoconf
.autoconf: ${K230_CANMV_BUILD_DIR}/.config.old

.PHONY: .fast_dl
.fast_dl:
	@make -f scripts/helper.mk fast_dl FAST_DL=${FAST_DL} NATIVE_BUILD=${NATIVE_BUILD}

.PHONY: sync_submodule
sync_submodule: .fast_dl
	@echo "sync_submodule"
	@make -f scripts/helper.mk sync_submodule

.PHONY: prepare_sourcecode
prepare_sourcecode: sync_submodule
	@echo "prepare_sourcecode"
	@make -C k230_sdk prepare_sourcecode

.PHONY: .sync_overlay
.sync_overlay: .autoconf
	@make -f scripts/helper.mk sync_overlay

.PHONY: .rt-smart-kernel
.rt-smart-kernel: .sync_overlay
	@make -C k230_sdk rt-smart-kernel
	@make -C k230_sdk mpp-kernel
	@make -C k230_sdk mpp-apps

.PHONY: micropython
micropython: .rt-smart-kernel
	@make -C micropython_port
	@mkdir -p ${K230_CANMV_BUILD_DIR}/images/app
	@cd ${K230_CANMV_BUILD_DIR}; cp micropython/micropython images/app

.PHONY: build-image
build-image: micropython
	@make -C k230_sdk rt-smart-apps
	@make -C k230_sdk big-core-opensbi
	@make -C k230_sdk uboot
	@make -C k230_sdk build-image
