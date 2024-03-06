FAST_DL ?= 1
$(shell touch .conf)
include .conf

ifeq ("$(origin CONF)", "command line")
  update_conf = 1
else ifeq ("$(origin CONF)", "undefined")
  CONF = k230_canmv_defconfig
  update_conf = 1
else
  update_conf = 0
endif
ifeq ($(shell if [ -f configs/$(CONF) ]; then echo 1; else echo 0; fi;), 0)
  $(error "Please specify a valid CONF")
endif
ifeq ($(update_conf),1)
  $(shell sed -i "/^CONF=/d" .conf)
  $(shell echo "CONF=$(CONF)" >> .conf)
endif

ifeq ("$(origin NATIVE_BUILD)", "command line")
  update_conf = 1
else ifeq ("$(origin NATIVE_BUILD)", "undefined")
  update_conf = 1
  ifeq ($(shell curl --output /dev/null --silent --head --fail https://ai.b-bug.org/k230/ && echo $$?),0)
    NATIVE_BUILD = 1
  else
    NATIVE_BUILD = 0
  endif
else
  update_conf = 0
endif
ifeq ($(update_conf),1)
  $(shell sed -i "/^NATIVE_BUILD=/d" .conf)
  $(shell echo "NATIVE_BUILD=$(NATIVE_BUILD)" >> .conf)
  ifeq ($(NATIVE_BUILD),1)
    $(shell git update-index --assume-unchanged .gitmodules)
    $(shell git submodule set-url k230_sdk git@g.a-bug.org:maix_sw/k230_sdk_release.git >> /dev/null)
  else
    $(shell git update-index --no-assume-unchanged .gitmodules)
    $(shell git submodule set-url k230_sdk https://github.com/kendryte/k230_sdk.git >> /dev/null)
  endif
endif

SAVECONF ?= $(CONF)
export K230_CANMV_ROOT := $(shell pwd)
export K230_CANMV_BUILD_DIR := $(K230_CANMV_ROOT)/output/$(CONF)
$(shell mkdir -p $(K230_CANMV_BUILD_DIR))

CONFIG_PATH = $(K230_CANMV_ROOT)/configs
KCONFIG_PATH = $(K230_CANMV_ROOT)/scripts/kconfig
KCONFIG_CFG = $(K230_CANMV_ROOT)/Kconfig

.PHONY: all
all: build-image

$(K230_CANMV_BUILD_DIR)/.config: $(CONFIG_PATH)/$(CONF)
	@if [ ! -f $(KCONFIG_PATH)/mconf -o ! -f $(KCONFIG_PATH)/conf ];then \
	cd $(KCONFIG_PATH); make; fi
	@cd $(K230_CANMV_BUILD_DIR); \
	$(KCONFIG_PATH)/conf --defconfig $(CONFIG_PATH)/$(CONF) $(KCONFIG_CFG);

$(K230_CANMV_BUILD_DIR)/.config.old: $(K230_CANMV_BUILD_DIR)/.config
	@cd $(K230_CANMV_BUILD_DIR); \
	$(KCONFIG_PATH)/conf --syncconfig $(KCONFIG_CFG); \
	touch .config.old;

.PHONY: menuconfig
menuconfig: $(K230_CANMV_BUILD_DIR)/.config
	@cd $(K230_CANMV_BUILD_DIR); \
	$(KCONFIG_PATH)/mconf $(KCONFIG_CFG);

.PHONY: savedefconfig
savedefconfig: $(K230_CANMV_BUILD_DIR)/.config
	@cd $(K230_CANMV_BUILD_DIR); \
	$(KCONFIG_PATH)/conf --savedefconfig $(CONFIG_PATH)/$(SAVECONF) $(KCONFIG_CFG);

.PHONY: .autoconf
.autoconf: $(K230_CANMV_BUILD_DIR)/.config.old

.PHONY: .fast_dl
.fast_dl:
	@make -f scripts/helper.mk fast_dl FAST_DL=$(FAST_DL) NATIVE_BUILD=$(NATIVE_BUILD)

.PHONY: sync_submodule
sync_submodule: .fast_dl
	@echo "sync_submodule"
	@make -f scripts/helper.mk sync_submodule

.PHONY: prepare_sourcecode
prepare_sourcecode: sync_submodule
	@echo "prepare_sourcecode"
	@make -C k230_sdk prepare_sourcecode CONF=$(CONF) NATIVE_BUILD=$(NATIVE_BUILD)

.PHONY: .sync_overlay
.sync_overlay: .autoconf
	@make -f scripts/helper.mk sync_overlay

.PHONY: k230_sdk_build
k230_sdk_build: .sync_overlay
	@make -f scripts/helper.mk $(K230_CANMV_BUILD_DIR)/.k230_sdk_all CONF=$(CONF)

.PHONY: micropython
micropython: k230_sdk_build
	@make -C micropython_port
	@mkdir -p $(K230_CANMV_BUILD_DIR)/images/app
	@cd $(K230_CANMV_BUILD_DIR); cp micropython/micropython images/app

.PHONY: build-image
build-image: micropython
	@cp -r tests $(K230_CANMV_BUILD_DIR)/images/app
	@mkdir -p $(K230_CANMV_BUILD_DIR)/images/app/tests/utils/features
	@cp -r k230_sdk/src/big/kmodel/ai_poc/kmodel/face_detection_320.kmodel $(K230_CANMV_BUILD_DIR)/images/app/tests/nncase_runtime/face_detection
	@mkdir -p $(K230_CANMV_BUILD_DIR)/images/app/tests/kmodel
	@cp -r k230_sdk/src/big/kmodel/ai_poc/kmodel/face_recognition.kmodel $(K230_CANMV_BUILD_DIR)/images/app/tests/kmodel
	@cp -r k230_sdk/src/big/kmodel/ai_poc/kmodel/face_detection_320.kmodel $(K230_CANMV_BUILD_DIR)/images/app/tests/kmodel
	@cp -r k230_sdk/src/big/kmodel/ai_poc/kmodel/yolov8n_320.kmodel $(K230_CANMV_BUILD_DIR)/images/app/tests/kmodel
	@cp -r k230_sdk/src/big/kmodel/ai_poc/kmodel/yolov8n_seg_320.kmodel $(K230_CANMV_BUILD_DIR)/images/app/tests/kmodel
	@cp -r k230_sdk/src/big/kmodel/ai_poc/kmodel/LPD_640.kmodel $(K230_CANMV_BUILD_DIR)/images/app/tests/kmodel
	@cp -r k230_sdk/src/big/kmodel/ai_poc/kmodel/ocr_det_int16.kmodel $(K230_CANMV_BUILD_DIR)/images/app/tests/kmodel
	@cp -r k230_sdk/src/big/kmodel/ai_poc/kmodel/hand_det.kmodel $(K230_CANMV_BUILD_DIR)/images/app/tests/kmodel
	@cp -r k230_sdk/src/big/kmodel/ai_poc/kmodel/face_landmark.kmodel $(K230_CANMV_BUILD_DIR)/images/app/tests/kmodel
	@cp -r k230_sdk/src/big/kmodel/ai_poc/kmodel/face_pose.kmodel $(K230_CANMV_BUILD_DIR)/images/app/tests/kmodel
	@cp -r k230_sdk/src/big/kmodel/ai_poc/kmodel/face_parse.kmodel $(K230_CANMV_BUILD_DIR)/images/app/tests/kmodel
	@cp -r k230_sdk/src/big/kmodel/ai_poc/kmodel/LPD_640.kmodel $(K230_CANMV_BUILD_DIR)/images/app/tests/kmodel
	@cp -r k230_sdk/src/big/kmodel/ai_poc/kmodel/licence_reco.kmodel $(K230_CANMV_BUILD_DIR)/images/app/tests/kmodel
	@cp -r k230_sdk/src/big/kmodel/ai_poc/kmodel/handkp_det.kmodel $(K230_CANMV_BUILD_DIR)/images/app/tests/kmodel
	@cp -r k230_sdk/src/big/kmodel/ai_poc/kmodel/ocr_rec_int16.kmodel $(K230_CANMV_BUILD_DIR)/images/app/tests/kmodel
	@cp -r k230_sdk/src/big/kmodel/ai_poc/kmodel/hand_reco.kmodel $(K230_CANMV_BUILD_DIR)/images/app/tests/kmodel
	@cp -r k230_sdk/src/big/kmodel/ai_poc/kmodel/person_detect_yolov5n.kmodel $(K230_CANMV_BUILD_DIR)/images/app/tests/kmodel
	@cp -r k230_sdk/src/big/kmodel/ai_poc/kmodel/yolov8n-pose.kmodel $(K230_CANMV_BUILD_DIR)/images/app/tests/kmodel
	@cp -r k230_sdk/src/big/kmodel/ai_poc/kmodel/kws.kmodel $(K230_CANMV_BUILD_DIR)/images/app/tests/kmodel
	@cp -r k230_sdk/src/big/kmodel/ai_poc/kmodel/face_alignment.kmodel $(K230_CANMV_BUILD_DIR)/images/app/tests/kmodel
	@cp -r k230_sdk/src/big/kmodel/ai_poc/kmodel/face_alignment_post.kmodel $(K230_CANMV_BUILD_DIR)/images/app/tests/kmodel
	@cp -r k230_sdk/src/big/kmodel/ai_poc/kmodel/eye_gaze.kmodel $(K230_CANMV_BUILD_DIR)/images/app/tests/kmodel
	@cp -r k230_sdk/src/big/kmodel/ai_poc/kmodel/yolov5n-falldown.kmodel $(K230_CANMV_BUILD_DIR)/images/app/tests/kmodel
	@cp -r k230_sdk/src/big/kmodel/ai_poc/kmodel/cropped_test127.kmodel $(K230_CANMV_BUILD_DIR)/images/app/tests/kmodel
	@cp -r k230_sdk/src/big/kmodel/ai_poc/kmodel/nanotrack_backbone_sim.kmodel $(K230_CANMV_BUILD_DIR)/images/app/tests/kmodel
	@cp -r k230_sdk/src/big/kmodel/ai_poc/kmodel/nanotracker_head_calib_k230.kmodel $(K230_CANMV_BUILD_DIR)/images/app/tests/kmodel
	@cp -r k230_sdk/src/big/kmodel/ai_poc/kmodel/gesture.kmodel $(K230_CANMV_BUILD_DIR)/images/app/tests/kmodel
	@cp -r k230_sdk/src/big/kmodel/ai_poc/kmodel/recognition.kmodel $(K230_CANMV_BUILD_DIR)/images/app/tests/kmodel
	@cp -r k230_sdk/src/big/kmodel/ai_poc/kmodel/hifigan.kmodel $(K230_CANMV_BUILD_DIR)/images/app/tests/kmodel
	@cp -r k230_sdk/src/big/kmodel/ai_poc/kmodel/zh_fastspeech_2.kmodel $(K230_CANMV_BUILD_DIR)/images/app/tests/kmodel
	@cp -r k230_sdk/src/big/kmodel/ai_poc/kmodel/zh_fastspeech_1_f32.kmodel $(K230_CANMV_BUILD_DIR)/images/app/tests/kmodel
	@make -C k230_sdk build-image

.PHONY: micropython-clean
micropython-clean:
	@rm -rf $(K230_CANMV_BUILD_DIR)/micropython

.PHONY: k230_sdk-clean
k230_sdk-clean:
	@make -C k230_sdk clean
	@rm -f $(K230_CANMV_BUILD_DIR)/.k230_sdk_all

.PHONY: clean
clean: micropython-clean k230_sdk-clean