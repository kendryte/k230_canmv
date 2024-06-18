.PHONY: all sync_submodule sync_overlay submodule_init

all:
	@echo "Don't call make all"

ifeq ($(MAKECMDGOALS), sync_overlay)
overlay_dir = $(shell find overlay -type d)
overlay_file = $(shell find overlay -type f)
endif

sync_overlay: $(overlay_dir) $(overlay_file)
	@echo "sync_lvgl_overlay"
	@git -C lv_binding_micropython clean -fdq
	@git -C lv_binding_micropython reset -q --hard
	@touch lv_binding_micropython/.ready_sync_dir
	@rsync -a -q overlay/ lv_binding_micropython/
	@touch lv_binding_micropython/.ready_sync_file

submodule_init:
	@git submodule update --init -f lv_binding_micropython

sync_submodule: submodule_init sync_overlay
