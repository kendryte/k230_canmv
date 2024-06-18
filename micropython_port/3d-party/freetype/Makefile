.PHONY: all sync_submodule sync_overlay submodule_init

all:
	@echo "Don't call make all"

ifeq ($(MAKECMDGOALS), sync_overlay)
overlay_dir = $(shell find overlay -type d)
overlay_file = $(shell find overlay -type f)
endif

sync_overlay: $(overlay_dir) $(overlay_file)
	@echo "sync_freetype_overlay"
	@git -C freetype clean -fdq
	@git -C freetype reset -q --hard
	@touch freetype/.ready_sync_dir
	@rsync -a -q overlay/ freetype/
	@touch freetype/.ready_sync_file

submodule_init:
	@git submodule update --init -f freetype

sync_submodule: submodule_init sync_overlay
