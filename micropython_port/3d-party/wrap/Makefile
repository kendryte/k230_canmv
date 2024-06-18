.PHONY: all sync_submodule sync_overlay submodule_init

all:
	@echo "Don't call make all"

ifeq ($(MAKECMDGOALS), sync_overlay)
overlay_dir = $(shell find overlay -type d)
overlay_file = $(shell find overlay -type f)
endif

sync_overlay: $(overlay_dir) $(overlay_file)
	@echo "sync_wrap_overlay"
	@git -C micropython-wrap clean -fdq
	@git -C micropython-wrap reset -q --hard
	@touch micropython-wrap/.ready_sync_dir
	@rsync -a -q overlay/ micropython-wrap/
	@touch micropython-wrap/.ready_sync_file

sync_submodule:
	@git submodule update --init -f micropython-wrap
	@git -C micropython-wrap clean -fdq
	@rsync -a -q overlay/ micropython-wrap/
	@touch micropython-wrap/.ready_sync_dir
	@touch micropython-wrap/.ready_sync_file
