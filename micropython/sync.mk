.PHONY: all sync_submodule sync_overlay submodule_init

ifeq ($(MAKECMDGOALS), sync_overlay)
overlay_dir = $(shell find overlay -type d)
overlay_file = $(shell find overlay -type f)
endif

micropython/.ready_sync_dir: $(overlay_dir)
	@echo "sync micropython overlay dir"
	@git -C micropython clean -fdq
	@git -C micropython reset -q --hard
	@touch micropython/.ready_sync_dir

micropython/.ready_sync_file: $(overlay_file)
	@echo "sync micropython overlay file"
	@rsync -a -q overlay/ micropython/
	@touch micropython/.ready_sync_file

micropython_sync_overlay: micropython/.ready_sync_dir micropython/.ready_sync_file

sync_overlay: micropython_sync_overlay
	@make -C port/3d-party/freetype -f sync.mk sync_overlay
	@make -C port/3d-party/lvgl -f sync.mk sync_overlay
	@make -C port/3d-party/wrap -f sync.mk sync_overlay

micropython_sync_submodule:
	@git submodule update --init -f micropython
	@git -C micropython clean -fdq
	@rsync -a -q overlay/ micropython/
	@touch micropython/.ready_sync_dir
	@touch micropython/.ready_sync_file

sync_submodule: micropython_sync_submodule
	@make -C port/3d-party/freetype -f sync.mk sync_submodule
	@make -C port/3d-party/lvgl -f sync.mk sync_submodule
	@make -C port/3d-party/wrap -f sync.mk sync_submodule

ifeq ($(MAKECMDGOALS), fast_dl)
.PHONY: fast_dl
ifeq ($(FAST_DL),0)
fast_dl:
else
# need `SERVER`
micropython_download_url = $(SERVER)/downloads/canmv/micropython.tar.gz

micropython_fast_dl:
	@set -e; \
	if [ ! -f micropython/.ready_dl_src ]; then \
		echo "download micropython, url: $(micropython_download_url)"; \
		wget -c --show-progress $(micropython_download_url) -O - | tar -xz ; \
		touch micropython/.ready_dl_src; \
	fi;

fast_dl: micropython_fast_dl
	@make -C port/3d-party/freetype -f sync.mk SERVER=$(SERVER) FAST_DL=$(FAST_DL) fast_dl
	@make -C port/3d-party/lvgl -f sync.mk SERVER=$(SERVER) FAST_DL=$(FAST_DL) fast_dl
	@make -C port/3d-party/wrap -f sync.mk SERVER=$(SERVER) FAST_DL=$(FAST_DL) fast_dl
	@ls -alht $(pwd)

endif # end ifeq ($(FAST_DL),0)
endif # end ifeq ($(MAKECMDGOALS), fast_dl)
