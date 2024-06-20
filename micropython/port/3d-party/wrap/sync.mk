.PHONY: all sync_submodule sync_overlay submodule_init

ifeq ($(MAKECMDGOALS), sync_overlay)
overlay_dir = $(shell find overlay -type d)
overlay_file = $(shell find overlay -type f)
endif

micropython-wrap/.ready_sync_dir: $(overlay_dir)
	@echo "sync micropython-wrap overlay dir"
	@git -C micropython-wrap clean -fdq
	@git -C micropython-wrap reset -q --hard
	@touch micropython-wrap/.ready_sync_dir

micropython-wrap/.ready_sync_file: $(overlay_file)
	@echo "sync micropython-wrap overlay file"
	@rsync -a -q overlay/ micropython-wrap/
	@touch micropython-wrap/.ready_sync_file

sync_overlay: micropython-wrap/.ready_sync_dir micropython-wrap/.ready_sync_file

sync_submodule:
	@git submodule update --init -f micropython-wrap
	@git -C micropython-wrap clean -fdq
	@rsync -a -q overlay/ micropython-wrap/
	@touch micropython-wrap/.ready_sync_dir
	@touch micropython-wrap/.ready_sync_file

ifeq ($(MAKECMDGOALS), fast_dl)
.PHONY: fast_dl
ifeq ($(FAST_DL),0)
fast_dl:
else
# need `SERVER`
wrap_download_url = $(SERVER)/downloads/canmv/micropython_wrap.tar.gz
fast_dl:
	@set -e; \
	if [ ! -f micropython-wrap/.ready_dl_src ]; then \
		echo "download wrap, url: $(wrap_download_url)"; \
		wget -c --show-progress $(wrap_download_url) -O - | tar -xz; \
		touch micropython-wrap/.ready_dl_src; \
	fi;
endif # end ifeq ($(FAST_DL),0)
endif # end ifeq ($(MAKECMDGOALS), fast_dl)
