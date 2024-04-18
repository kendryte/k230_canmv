include $(MPP_SRC_DIR)/middleware/mpp.mk
include $(MPP_SRC_DIR)/middleware/rt-smart.mk

AR = riscv64-unknown-linux-musl-ar
ARFLAGS = -rc

CURRECT_DIR_NAME=$(shell basename `pwd`)
LOCAL_SRC_DIR = $(shell pwd)

LIVE555= $(LOCAL_SRC_DIR)/live555
RTSP_CLIENT= $(LOCAL_SRC_DIR)/rtsp_client
RTSP_SERVER= $(LOCAL_SRC_DIR)/rtsp_server
MP4_FORMAT = $(LOCAL_SRC_DIR)/mp4_format
MP4_PLAYER = $(LOCAL_SRC_DIR)/mp4_player
KDMEDIA = $(LOCAL_SRC_DIR)/kdmedia

all:
	cd $(MP4_FORMAT) ; make


clean:
	echo "${PWD}/Makefile clean"
	@cd $(MP4_FORMAT); make clean

.PHONY: all clean
