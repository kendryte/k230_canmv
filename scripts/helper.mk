
define k230_sdk_dl
	if [ ! -f .ready_k230_sdk_dl ]; then \
		echo "download k230_sdk"; \
		wget -cq ${K230_SDK_DOWNLOAD_URL} -O - | tar -xz ; \
		touch .ready_k230_sdk_dl; \
	fi
endef

define micropython_dl
	if [ ! -f .ready_micropython_dl ]; then \
		echo "download micropython"; \
		wget -cq ${MICROPYTHON_DOWNLOAD_URL} -O - | tar -xz ; \
		touch .ready_micropython_dl; \
	fi
endef

.PHONY: fast_dl
fast_dl:
	@$(k230_sdk_dl)
	@$(micropython_dl)

.PHONY: sync_submodule
sync_submodule:
	@echo "sync_submodule"
	@cd ${PROJECT_ROOT_DIR}
	@git submodule update --init -f k230_sdk
	@git -C k230_sdk clean -fd -e toolchain
	@rsync -a -v -q k230_sdk_overlay/ k230_sdk/ --exclude=/CMakeLists.txt
	@git submodule update --init -f micropython
	@git -C micropython clean -fd
	@touch .sync_k230_sdk_overlay_dir
	@touch .sync_k230_sdk_overlay_file

K230_SDK_OVERLAY := ${PROJECT_ROOT_DIR}/k230_sdk_overlay
K230_SDK_OVERLAY_DIR := $(shell find ${K230_SDK_OVERLAY} -type d)
K230_SDK_OVERLAY_FILE := $(shell find ${K230_SDK_OVERLAY} -type f)

.sync_k230_sdk_overlay_dir: ${K230_SDK_OVERLAY_DIR}
	@echo "sync_k230_sdk_overlay_dir"
	@cd ${PROJECT_ROOT_DIR}
	@git -C k230_sdk clean -fd -e toolchain -e output
	@rsync -a -v -q k230_sdk_overlay/ k230_sdk/ --exclude=/CMakeLists.txt
	@touch .sync_k230_sdk_overlay_dir
	@touch .sync_k230_sdk_overlay_file

.sync_k230_sdk_overlay_file: ${K230_SDK_OVERLAY_FILE}
	@echo "sync_k230_sdk_overlay_file"
	@cd ${PROJECT_ROOT_DIR}
	@rsync -a -v -q k230_sdk_overlay/ k230_sdk/ --exclude=/CMakeLists.txt
	@touch .sync_k230_sdk_overlay_file

.PHONY: sync_k230_sdk_overlay
sync_k230_sdk_overlay: .sync_k230_sdk_overlay_dir .sync_k230_sdk_overlay_file
