# helper.mk

ifeq ($(MAKECMDGOALS), fast_dl)
.PHONY: fast_dl
ifeq ($(FAST_DL),0)
fast_dl:
else
ifeq ($(NATIVE_BUILD),1)
server_url = https://ai.b-bug.org/k230
k230_sdk_download_url = $(server_url)/release/sdk/github/k230_sdk.tar.gz
else
server_url = https://kendryte-download.canaan-creative.com/k230
k230_sdk_download_url = $(server_url)/release/sdk/k230_sdk.tar.gz
endif # ifeq ($(NATIVE_BUILD),1)
micropython_download_url = $(server_url)/downloads/canmv/micropython.tar.gz
lvgl_download_url = $(server_url)/downloads/canmv/lv_binding_micropython.tar.gz

fast_dl:
	@set -e; \
	if [ ! -f k230_sdk_overlay/.ready_dl_src ]; then \
		echo "download k230_sdk"; \
		if [ $(NATIVE_BUILD) -ne 1 ]; then \
		wget -c --show-progress $(k230_sdk_download_url) -O - | tar -xz ; fi; \
		touch k230_sdk_overlay/.ready_dl_src; \
	fi; \
	if [ ! -f micropython_port/.ready_dl_src ]; then \
		echo "download micropython"; \
		wget -c --show-progress $(micropython_download_url) -O - | tar -xz ; \
		touch micropython_port/.ready_dl_src; \
	fi; \
	if [ ! -f micropython_port/lvgl/.ready_dl_src ]; then \
		echo "download lvgl"; \
		wget -c --show-progress $(lvgl_download_url) -O - | tar -xz -C micropython_port/lvgl ; \
		touch micropython_port/lvgl/.ready_dl_src; \
	fi;

endif # end ifeq ($(FAST_DL),0)
endif # end ifeq ($(MAKECMDGOALS), fast_dl)

k230_sdk_clean_exclude_file = \
	-e toolchain -e output \
	-e .config -e .config.old -e .last_conf \
	-e src/big/kmodel -e src/big/nncase -e src/big/utils \
	-e src/.src_fetched \
	-e src/little/buildroot-ext/dl \
	-e src/little/buildroot-ext/buildroot-9d1d4818c39d97ad7a1cdf6e075b9acae6dfff71 \
	-e src/little/buildroot-ext/package/tuning-server \
	-e src/big/rt-smart/kernel/bsp/maix3/.sconsign.dblite \
	-e src/big/rt-smart/userapps/.sconsign.dblite

k230_sdk_overlay_rsync_exclude_file = \
	--exclude=/.ready*

.PHONY: sync_submodule
sync_submodule:
	@git submodule update --init -f k230_sdk
	@git -C k230_sdk clean -fdq $(k230_sdk_clean_exclude_file)
	@rsync -a -q k230_sdk_overlay/ k230_sdk/ $(k230_sdk_overlay_rsync_exclude_file)
	@rm -f k230_sdk/src/big/rt-smart/kernel/bsp/maix3/.sconsign.dblite
	@rm -f k230_sdk/src/big/rt-smart/userapps/.sconsign.dblite
	@touch k230_sdk_overlay/.ready_sync_file
	@touch k230_sdk_overlay/.ready_sync_dir
	@git submodule update --init -f micropython
	@git -C micropython clean -fdq
	@rsync -a -q micropython_port/micropython_overlay/ micropython/
	@touch micropython_port/.ready_sync_file
	@touch micropython_port/.ready_sync_dir
	@git submodule update --init -f micropython_port/lvgl/lv_binding_micropython
	@git -C micropython_port/lvgl/lv_binding_micropython clean -fdq
	@rsync -a -q micropython_port/lvgl/overlay/ micropython_port/lvgl/lv_binding_micropython/
	@touch micropython_port/lvgl/.ready_sync_file
	@touch micropython_port/lvgl/.ready_sync_dir

ifeq ($(MAKECMDGOALS), sync_overlay)
k230_sdk_overlay_dir = $(shell find k230_sdk_overlay -type d)
k230_sdk_overlay_file = $(shell find k230_sdk_overlay -type f -a -not -name ".ready*")
micropython_overlay_dir = $(shell find micropython_port/micropython_overlay -type d)
micropython_overlay_file = $(shell find micropython_port/micropython_overlay -type f)
lvgl_overlay_dir = $(shell find micropython_port/lvgl/overlay -type d)
lvgl_overlay_file = $(shell find micropython_port/lvgl/overlay -type f)
endif

k230_sdk_overlay/.ready_sync_dir: $(k230_sdk_overlay_dir)
	@echo "sync_k230_sdk_overlay_dir"
	@git -C k230_sdk clean -fdq $(k230_sdk_clean_exclude_file)
	@git -C k230_sdk reset -q --hard
	@rm -f k230_sdk/src/big/rt-smart/kernel/bsp/maix3/.sconsign.dblite
	@rm -f k230_sdk/src/big/rt-smart/userapps/.sconsign.dblite
	@touch k230_sdk_overlay/.ready_sync_dir

k230_sdk_overlay/.ready_sync_file: k230_sdk_overlay/.ready_sync_dir $(k230_sdk_overlay_file)
	@echo "sync_k230_sdk_overlay_file"
	@rsync -a -q k230_sdk_overlay/ k230_sdk/ $(k230_sdk_overlay_rsync_exclude_file)
	@touch k230_sdk_overlay/.ready_sync_file

micropython_port/.ready_sync_dir: $(micropython_overlay_dir)
	@echo "sync_micropython_overlay_dir"
	@git -C micropython clean -fdq
	@git -C micropython reset -q --hard
	@touch micropython_port/.ready_sync_dir

micropython_port/.ready_sync_file: micropython_port/.ready_sync_dir $(micropython_overlay_file)
	@echo "sync_micropython_overlay_file"
	@rsync -a -q micropython_port/micropython_overlay/ micropython/
	@touch micropython_port/.ready_sync_file

micropython_port/lvgl/.ready_sync_dir: $(lvgl_overlay_dir)
	@echo "sync_lvgl_overlay_dir"
	@git -C micropython_port/lvgl/lv_binding_micropython clean -fdq
	@git -C micropython_port/lvgl/lv_binding_micropython reset -q --hard
	@touch micropython_port/lvgl/.ready_sync_dir

micropython_port/lvgl/.ready_sync_file: micropython_port/lvgl/.ready_sync_dir $(lvgl_overlay_file)
	@echo "sync_lvgl_overlay_file"
	@rsync -a -q micropython_port/lvgl/overlay/ micropython_port/lvgl/lv_binding_micropython/
	@touch micropython_port/lvgl/.ready_sync_file

.PHONY: k230_sdk_sync_overlay
k230_sdk_sync_overlay: k230_sdk_overlay/.ready_sync_dir k230_sdk_overlay/.ready_sync_file

.PHONY: micropython_sync_overlay
micropython_sync_overlay: micropython_port/.ready_sync_dir micropython_port/.ready_sync_file

.PHONY: lvgl_sync_overlay
lvgl_sync_overlay: micropython_port/lvgl/.ready_sync_dir micropython_port/lvgl/.ready_sync_file

.PHONY: sync_overlay
sync_overlay: k230_sdk_sync_overlay micropython_sync_overlay lvgl_sync_overlay

$(K230_CANMV_BUILD_DIR)/.k230_sdk_all: k230_sdk_overlay/.ready_sync_dir k230_sdk_overlay/.ready_sync_file
	@make -C k230_sdk all CONF=$(CONF) && touch $@
