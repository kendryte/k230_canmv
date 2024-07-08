.PHONY: all sync_submodule sync_overlay submodule_init

ifeq ($(MAKECMDGOALS), sync_overlay)
overlay_dir = $(shell find overlay -type d)
overlay_file = $(shell find overlay -type f)
endif

lv_binding_micropython/.ready_sync_dir: $(overlay_dir)
	@echo "sync lvgl overlay dir"
	@git -C lv_binding_micropython clean -fdq
	@git -C lv_binding_micropython reset -q --hard
	@touch lv_binding_micropython/.ready_sync_dir

lv_binding_micropython/.ready_sync_file: $(overlay_file)
	@echo "sync lvgl overlay file"
	@rsync -a -q overlay/ lv_binding_micropython/
	@touch lv_binding_micropython/.ready_sync_file

sync_overlay: lv_binding_micropython/.ready_sync_dir lv_binding_micropython/.ready_sync_file

sync_submodule:
	@git submodule update --init -f lv_binding_micropython
	@git -C lv_binding_micropython clean -fdq
	@rsync -a -q overlay/ lv_binding_micropython/
	@touch lv_binding_micropython/.ready_sync_dir
	@touch lv_binding_micropython/.ready_sync_file

ifeq ($(MAKECMDGOALS), fast_dl)
.PHONY: fast_dl
ifeq ($(FAST_DL),0)
fast_dl:
else
# need `SERVER`
lvgl_download_url = $(SERVER)/downloads/canmv/lv_binding_micropython.tar.gz
fast_dl:
	@set -e; \
	if [ ! -f .ready_dl_src ]; then \
		echo "download lvgl, url: $(lvgl_download_url)"; \
		wget -c --show-progress $(lvgl_download_url) -O - | tar -xz ; \
		touch .ready_dl_src; \
	fi;
endif # end ifeq ($(FAST_DL),0)
endif # end ifeq ($(MAKECMDGOALS), fast_dl)
