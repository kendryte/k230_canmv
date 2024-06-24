.PHONY: all sync_submodule sync_overlay submodule_init

ifeq ($(MAKECMDGOALS), sync_overlay)
overlay_dir = $(shell find overlay -type d)
overlay_file = $(shell find overlay -type f)
endif

freetype/.ready_sync_dir: $(overlay_dir)
	@echo "sync freetype overlay dir"
	@git -C freetype clean -fdq
	@git -C freetype reset -q --hard
	@touch freetype/.ready_sync_dir

freetype/.ready_sync_file: $(overlay_file)
	@echo "sync freetype overlay file"
	@rsync -a -q overlay/ freetype/
	@touch freetype/.ready_sync_file

sync_overlay: freetype/.ready_sync_dir freetype/.ready_sync_file

sync_submodule:
	@git submodule update --init -f freetype
	@git -C freetype clean -fdq
	@rsync -a -q overlay/ freetype/
	@touch freetype/.ready_sync_dir
	@touch freetype/.ready_sync_file

ifeq ($(MAKECMDGOALS), fast_dl)
.PHONY: fast_dl
ifeq ($(FAST_DL),0)
fast_dl:
else
# need `SERVER`
freetype_download_url = $(SERVER)/downloads/canmv/freetype.tar.gz
fast_dl:
	@set -e; \
	if [ ! -f .ready_dl_src ]; then \
		echo "download freetype, url: $(freetype_download_url)"; \
		wget -c --show-progress $(freetype_download_url) -O - | tar -xz ; \
		touch .ready_dl_src; \
	fi;
endif # end ifeq ($(FAST_DL),0)
endif # end ifeq ($(MAKECMDGOALS), fast_dl)
