/*
 * This file is part of the OpenMV project.
 * Copyright (c) 2013/2014 Ibrahim Abdelkader <i.abdalkader@gmail.com>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * USB debug support.
 *
 */
#ifndef __IDE_DBG_H__
#define __IDE_DBG_H__


#include "stdint.h"
#include "stdbool.h"
#include "obj.h"
//#include "global_config.h"

#define CONFIG_CANMV_IDE_SUPPORT 1

#if CONFIG_CANMV_IDE_SUPPORT

#define NEW_SAVE_FILE_IMPL

/**
  * To add a new debugging command, increment the last command value used.
  * Set the MSB of the value if the request has a device-to-host data phase.
  * Add the command to usr/openmv.py using the same value.
  * Handle the command control and data in/out (if any) phases in usbdbg.c.
  *
  * See usbdbg.c for examples.
  */
enum usbdbg_cmd {
    USBDBG_NONE             =0x00,
    USBDBG_FW_VERSION       =0x80,
    USBDBG_FRAME_SIZE       =0x81,
    USBDBG_FRAME_DUMP       =0x82,
    USBDBG_ARCH_STR         =0x83,
    USBDBG_SCRIPT_EXEC      =0x05,
    USBDBG_SCRIPT_STOP      =0x06,
    USBDBG_SCRIPT_SAVE      =0x07,
    // USBDBG_FILE_SAVE        =0x07,
    // USBDBG_FILE_SAVE_STATUS =0x88,
    USBDBG_SCRIPT_RUNNING   =0x87,
    USBDBG_TEMPLATE_SAVE    =0x08,
    USBDBG_DESCRIPTOR_SAVE  =0x09,
    USBDBG_ATTR_READ        =0x8A,
    USBDBG_ATTR_WRITE       =0x0B,
    USBDBG_SYS_RESET        =0x0C,
    USBDBG_FB_ENABLE        =0x0D,
    USBDBG_QUERY_STATUS     =0x8D,//0xFFEEBBAA
    USBDBG_TX_BUF_LEN       =0x8E,
    USBDBG_TX_BUF           =0x8F,
    USBDBG_SENSOR_ID        =0x90,
    USBDBG_TX_INPUT         =0x11,
    USBDBG_SET_TIME         =0x12,
    USBDBG_CREATEFILE       =0x20,
    USBDBG_WRITEFILE        =0x21,
    USBDBG_QUERY_FILE_STAT  =0xA0,
    USBDBG_VERIFYFILE       =0xA1,
};

#ifdef NEW_SAVE_FILE_IMPL

#define USBDBG_SVFILE_NAME_LEN            (64) // whole path length.

#define USBDBG_SVFILE_ERR_NONE            (1024 + 0)
#define USBDBG_SVFILE_ERR_PATH_ERR        (1024 + 1)
#define USBDBG_SVFILE_ERR_CHKSUM_ERR      (1024 + 2)
#define USBDBG_SVFILE_ERR_WRITE_ERR       (1024 + 3)
#define USBDBG_SVFILE_ERR_CHUNK_ERR       (1024 + 4)

#define USBDBG_SVFILE_VERIFY_NOT_OPEN     (1024 + 5)
#define USBDBG_SVFILE_VERIFY_SHA2_ERR     (1024 + 6)
#define USBDBG_SVFILE_VERIFY_ERR_NONE     (1024 + 7)

#define USBDBG_SVFILE_ERR_OPEN_ERR        (1024 + 8)

struct ide_dbg_svfil_info_t {
    int chunk_size; // must be 4096
    char name[USBDBG_SVFILE_NAME_LEN + 4];
    uint8_t sha256[32];
};

struct ide_dbg_svfil_t {
    int errcode;

    FILE* file;

    uint8_t *chunk_buffer;
    struct ide_dbg_svfil_info_t info;
};
#else
bool      ide_dbg_need_save_file();
void      ide_save_file(); 
#endif

typedef enum{
    IDE_DBG_STATUS_OK = 0,
    IDE_DBG_DISPATCH_STATUS_ERR = 1,
    IDE_DBG_DISPATCH_STATUS_WAIT,
    IDE_DBG_DISPATCH_STATUS_DATA,
    IDE_DBG_DISPATCH_STATUS_BUSY,
} ide_dbg_status_t;

enum frame_state {
    FRAME_HEAD,
    FRAME_CMD,
    FRAME_RECV,
    FRAME_DATA_LENGTH,
    FRAME_DISPATCH
};

typedef struct {
    enum frame_state state;
    enum usbdbg_cmd cmd;
    uint32_t data_length;
    void* data;
    // private
    uint32_t recv_lack;
    enum frame_state recv_next;
    void* recv_data;
} ide_dbg_state_t;

#else // CONFIG_CANMV_IDE_SUPPORT
#endif

#endif /* __USBDBG_H__ */
