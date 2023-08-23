# helper.mk

ifeq (${MAKECMDGOALS}, fast_dl)
.PHONY: fast_dl
ifeq ($(FAST_DL),0)
fast_dl:
else
ifeq ($(NATIVE_BUILD),1)
server_url = https://ai.b-bug.org/k230
k230_sdk_download_url = ${server_url}/release/sdk/github/k230_sdk.tar.gz
else
server_url = https://kendryte-download.canaan-creative.com/k230
k230_sdk_download_url = ${server_url}/release/sdk/k230_sdk.tar.gz
endif # ifeq ($(NATIVE_BUILD),1)
micropython_download_url = ${server_url}/downloads/canmv/micropython.tar.gz

fast_dl:
	@set -e; \
	if [ ! -f k230_sdk_overlay/.ready_dl_src ]; then \
		echo "download k230_sdk"; \
		wget -c ${k230_sdk_download_url} -O - | tar -xz ; \
		touch k230_sdk_overlay/.ready_dl_src; \
	fi; \
	if [ ! -f micropython_port/.ready_dl_src ]; then \
		echo "download micropython"; \
		wget -c ${micropython_download_url} -O - | tar -xz ; \
		touch micropython_port/.ready_dl_src; \
	fi;

endif # end ifeq ($(FAST_DL),0)
endif # end ifeq (${MAKECMDGOALS}, fast_dl)

k230_sdk_clean_exclude_file = \
	-e toolchain \
	-e output \
	-e src/.src_fetched \
	-e src/big/rt-smart/kernel/bsp/maix3/.sconsign.dblite \
	-e src/big/rt-smart/userapps/.sconsign.dblite

k230_sdk_overlay_rsync_exclude_file = \
	--exclude=/.ready*

.PHONY: sync_submodule
sync_submodule:
	@git submodule update --init -f k230_sdk
	@git -C k230_sdk clean -fdq ${k230_sdk_clean_exclude_file}
	@rsync -a -q k230_sdk_overlay/ k230_sdk/ ${k230_sdk_overlay_rsync_exclude_file}
	@rm k230_sdk/src/big/rt-smart/kernel/bsp/maix3/.sconsign.dblite
	@rm k230_sdk/src/big/rt-smart/userapps/.sconsign.dblite
	@git submodule update --init -f micropython
	@git -C micropython clean -fdq
	@rsync -a -q micropython_port/micropython_overlay/ micropython/
	@touch micropython_port/.ready_sync_file
	@touch micropython_port/.ready_sync_dir
	@touch k230_sdk_overlay/.ready_sync_file
	@touch k230_sdk_overlay/.ready_sync_dir

ifeq (${MAKECMDGOALS}, sync_overlay)
k230_sdk_overlay_dir = $(shell find k230_sdk_overlay -type d)
k230_sdk_overlay_file = $(shell find k230_sdk_overlay -type f -a -not -name ".ready*")
micropython_overlay_dir = $(shell find micropython_port/micropython_overlay -type d)
micropython_overlay_file = $(shell find micropython_port/micropython_overlay -type f)
endif

k230_sdk_overlay/.ready_sync_dir: ${k230_sdk_overlay_dir}
	@echo "sync_k230_sdk_overlay_dir"
	@git -C k230_sdk clean -fdq ${k230_sdk_clean_exclude_file}
	@rsync -a -q k230_sdk_overlay/ k230_sdk/ ${k230_sdk_overlay_rsync_exclude_file}
	@touch k230_sdk_overlay/.ready_sync_file
	@touch k230_sdk_overlay/.ready_sync_dir

k230_sdk_overlay/.ready_sync_file: ${k230_sdk_overlay_file}
	@echo "sync_k230_sdk_overlay_file"
	@rsync -a -q k230_sdk_overlay/ k230_sdk/ ${k230_sdk_overlay_rsync_exclude_file}
	@touch k230_sdk_overlay/.ready_sync_file

micropython_port/.ready_sync_dir: ${micropython_overlay_dir}
	@echo "sync_micropython_overlay_dir"
	@git -C micropython clean -fdq
	@rsync -a -q micropython_port/micropython_overlay/ micropython/
	@touch micropython_port/.ready_sync_file
	@touch micropython_port/.ready_sync_dir

micropython_port/.ready_sync_file: ${micropython_overlay_file}
	@echo "sync_micropython_overlay_file"
	@rsync -a -q micropython_port/micropython_overlay/ micropython/
	@touch micropython_port/.ready_sync_file

.PHONY: sync_overlay
sync_overlay: k230_sdk_overlay/.ready_sync_dir k230_sdk_overlay/.ready_sync_file micropython_port/.ready_sync_dir micropython_port/.ready_sync_file
