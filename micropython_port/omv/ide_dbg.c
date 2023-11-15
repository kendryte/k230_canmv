/*
 * This file is part of the OpenMV project.
 * Copyright (c) 2013/2014 Ibrahim Abdelkader <i.abdalkader@gmail.com>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * USB debug support.
 *
 */
#include "builtin.h"
#include "k_connector_comm.h"
#include "k_vb_comm.h"
#include "k_video_comm.h"
#include "mpconfig.h"
#include "boards/k230_evb/mpconfigboard.h"
#include "mpi_connector_api.h"
#include "mpi_vb_api.h"
#include "mpi_vo_api.h"
#include "mpstate.h"
#include "obj.h"
#include "objstr.h"
#include "readline.h"
#include "shared/runtime/pyexec.h"
#include "stream.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <sha256.h>
#include "py/runtime.h"
#include <signal.h>

#define CONFIG_CANMV_IDE_SUPPORT 1

#include "ide_dbg.h"
#include <stdio.h>

#if CONFIG_CANMV_IDE_SUPPORT

#define TEST_PIC 0

int usb_cdc_fd;

void mp_hal_stdout_tx_strn(const char *str, size_t len);

#if IDE_DEBUG_PRINT
#define pr(fmt,...) fprintf(stderr,fmt "\n",##__VA_ARGS__)
#else
#define pr(fmt,...)
#endif

static pthread_t ide_dbg_task_p;
static unsigned char usb_cdc_read_buf[4096];
static struct ide_dbg_svfil_t ide_dbg_sv_file;
static mp_obj_exception_t ide_exception; //IDE interrupt
static mp_obj_str_t ide_exception_str;
static mp_obj_tuple_t* ide_exception_str_tuple = NULL;
#if !TEST_PIC
static mp_obj_t mp_const_ide_interrupt = (mp_obj_t)(&ide_exception);
#endif
static sem_t stdin_sem;
static char stdin_ring_buffer[4096];
static unsigned stdin_write_ptr = 0;
static unsigned stdin_read_ptr = 0;

int usb_rx(void) {
    char c;
    sem_wait(&stdin_sem);
    c = stdin_ring_buffer[stdin_read_ptr++];
    stdin_read_ptr %= sizeof(stdin_ring_buffer);
    return c;
}

int usb_tx(const void* buffer, size_t size) {
    return write(usb_cdc_fd, buffer, size);
}

void print_raw(const uint8_t* data, size_t size) {
    #if IDE_DEBUG_PRINT
    fprintf(stderr, "raw: \"");
    for (size_t i = 0; i < size; i++) {
        fprintf(stderr, "\\x%02X", ((unsigned char*)data)[i]);
    }
    fprintf(stderr, "\"\n");
    #endif
}

static uint32_t ide_script_running = 0;
static char tx_buf[256];
static volatile uint32_t tx_buf_len = 0;
static pthread_mutex_t tx_buf_mutex = PTHREAD_MUTEX_INITIALIZER;

void mpy_stdout_tx(const char* data, size_t size) {
    // FIXME
    pthread_mutex_lock(&tx_buf_mutex);
    if (sizeof(tx_buf) - tx_buf_len < size) {
        pthread_mutex_unlock(&tx_buf_mutex);
        // wait tx_buf_len to zero
        while (tx_buf_len) {
            usleep(2000);
        }
        pthread_mutex_lock(&tx_buf_mutex);
    }
    memcpy(tx_buf + tx_buf_len, data, size);
    tx_buf_len += size;
    pthread_mutex_unlock(&tx_buf_mutex);
}

static void read_until(int fd, void* buffer, size_t size) {
    size_t idx = 0;
    do {
        size_t recv = read(fd, (char*)buffer + idx, size - idx);
        idx += recv;
    } while (idx < size);
}

static void print_sha256(const uint8_t sha256[32]) {
    #if IDE_DEBUG_PRINT
    fprintf(stderr, "SHA256: ");
    for (unsigned i = 0; i < 32; i++) {
        fprintf(stderr, "%02x", sha256[i]);
    }
    fprintf(stderr, "\n");
    #endif
}

void mpy_start_script(char* filepath);
void mpy_stop_script();
static char *script_string = NULL;
static sem_t script_sem;
static volatile bool ide_attached = false;

char* ide_dbg_get_script() {
    sem_wait(&script_sem);
    return script_string;
}

bool ide_dbg_attach(void) {
    return ide_attached;
}

void ide_dbg_on_script_start(void) {
    ide_script_running = 1;
}

void ide_dbg_on_script_end(void) {
    ide_script_running = 0;
    // wait print done
    while (tx_buf_len) {
        usleep(2000);
    }
}

static void interrupt_repl(void) {
    stdin_ring_buffer[stdin_write_ptr++] = CHAR_CTRL_D;
    stdin_write_ptr %= sizeof(stdin_ring_buffer);
    sem_post(&stdin_sem);
}

static void interrupt_ide(void) {
    pr("[usb] exit IDE mode");
    ide_attached = false;
    // exit script mode
    if (script_string) {
        free(script_string);
        script_string = NULL;
    }
    sem_post(&script_sem);
}

static volatile bool enable_pic = true;
static bool flag_wait_exit = false;

static enum {
    FB_FROM_NONE,
    FB_FROM_USER_SET,
    FB_FROM_VO_WRITEBACK
} fb_from = FB_FROM_NONE;
static volatile void* fb_data = NULL;
static uint32_t fb_size = 0, fb_width = 0, fb_height = 0;
static pthread_mutex_t fb_mutex;
// FIXME: reuse buf
void ide_set_fb(const void* data, uint32_t size, uint32_t width, uint32_t height) {
    pthread_mutex_lock(&fb_mutex);
    if (fb_data) {
        // not sended
        pthread_mutex_unlock(&fb_mutex);
        return;
    }
    fb_data = malloc(size);
    memcpy((void*)fb_data, data, size);
    fb_size = size;
    fb_width = width;
    fb_height = height;
    pthread_mutex_unlock(&fb_mutex);
}
#define ENABLE_VO_WRITEBACK 0
// for VO writeback
#if ENABLE_VO_WRITEBACK
static volatile char vo_wbc_enabled = 0;
static void* wbc_jpeg_buffer = NULL;
static size_t wbc_jpeg_buffer_size = 0;
static uint32_t wbc_jpeg_size = 0;
static volatile k_connector_type connector_type = 0;
#endif
int ide_dbg_vo_init(k_connector_type _connector_type) {
    #if ENABLE_VO_WRITEBACK
    connector_type = _connector_type;
    #endif
    return 0;
}

int ide_dbg_vo_wbc_init(void) {
    #if ENABLE_VO_WRITEBACK
    k_connector_info vo_info;
    kd_mpi_get_connector_info(connector_type, &vo_info);
    pr("[omv] %s(%d), %ux%u", __func__, connector_type, vo_info.resolution.hdisplay, vo_info.resolution.vdisplay);
    k_vo_wbc_attr attr = {
        .target_size = {
            .width = vo_info.resolution.hdisplay,
            .height = vo_info.resolution.vdisplay
        }
    };
    if (kd_mpi_vo_set_wbc_attr(&attr)) {
        pr("[omv] kd_mpi_vo_set_wbc_attr error");
        vo_wbc_enabled = -1;
        return -1;
    }
    if (kd_mpi_vo_enable_wbc()) {
        pr("[omv] kd_mpi_vo_enable_wbc error");
        vo_wbc_enabled = -1;
        return -1;
    }
    vo_wbc_enabled = 1;
    pr("[omv] VO writeback enabled");
    #endif
    return 0;
}

int ide_dbg_vo_deinit(void) {
    pr("[omv] %s", __func__);
    #if ENABLE_VO_WRITEBACK
    if (vo_wbc_enabled > 0) {
        kd_mpi_vo_disable_wbc();
        vo_wbc_enabled = 0;
    }
    #endif
    return 0;
}

static ide_dbg_status_t ide_dbg_update(ide_dbg_state_t* state, const uint8_t* data, size_t length) {
    #if TEST_PIC
    static unsigned pic_idx = 0;
    extern unsigned long num_jpeg;
    extern unsigned long index_jpeg[];
    extern unsigned char data_jpeg[];
    #endif
    for (size_t i = 0; i < length;) {
        switch (state->state) {
            case FRAME_HEAD:
                if (data[i] == 0x30) {
                    state->state = FRAME_CMD;
                }
                i += 1;
                break;
            case FRAME_CMD:
                state->cmd = data[i];
                state->state = FRAME_DATA_LENGTH;
                i += 1;
                break;
            case FRAME_DATA_LENGTH:
                // recv 4 bytes
                state->recv_lack = 4;
                state->state = FRAME_RECV;
                state->recv_next = FRAME_DISPATCH;
                state->recv_data = &state->data_length;
                break;
            case FRAME_DISPATCH:
                #define PRINT_ALL 0
                #if !PRINT_ALL
                if ((state->cmd != USBDBG_SCRIPT_RUNNING) &&
                    (state->cmd != USBDBG_FRAME_SIZE) &&
                    (state->cmd != USBDBG_TX_BUF_LEN) &&
                    (state->cmd != USBDBG_FRAME_DUMP))
                #endif
                {
                    print_raw(data, length);
                    pr("cmd: %x", state->cmd);
                }
                switch (state->cmd) {
                    case USBDBG_NONE:
                        break;
                    case USBDBG_QUERY_STATUS: {
                        pr("cmd: USBDBG_QUERY_STATUS");
                        uint32_t resp = 0xFFEEBBAA;
                        usb_tx(&resp, sizeof(resp));
                        break;
                    }
                    case USBDBG_FW_VERSION: {
                        pr("cmd: USBDBG_FW_VERSION");
                        uint32_t resp[3] = {
                            MICROPY_VERSION_MAJOR,
                            MICROPY_VERSION_MINOR,
                            MICROPY_VERSION_MICRO
                        };
                        usb_tx(&resp, sizeof(resp));
                        break;
                    }
                    case USBDBG_ARCH_STR: {
                        char buffer[0x40];
                        if (state->data_length != sizeof(buffer)) {
                            pr("Warning: USBDBG_ARCH_STR data length %u, expected %lu", state->data_length, sizeof(buffer));
                        }
                        snprintf(buffer, sizeof(buffer), "%s [%s:%08X%08X%08X]",
                            OMV_ARCH_STR, OMV_BOARD_TYPE, 0, 0, 0); // TODO: UID
                        pr("cmd: USBDBG_ARCH_STR %s", buffer);
                        usb_tx(buffer, sizeof(buffer));
                        break;
                    }
                    case USBDBG_SCRIPT_EXEC: {
                        // TODO
                        pr("cmd: USBDBG_SCRIPT_EXEC size %u", state->data_length);
                        // recv script string
                        if (script_string != NULL) {
                            free(script_string);
                        }
                        script_string = malloc(state->data_length + 1);
                        read_until(usb_cdc_fd, script_string, state->data_length);
                        script_string[state->data_length] = '\0';
                        // into script mode, interrupt REPL, send CTRL-D
                        ide_script_running = 1;
                        #if !TEST_PIC
                        sem_post(&script_sem);
                        #endif
                        break;
                    }
                    case USBDBG_SCRIPT_STOP: {
                        // TODO
                        pr("cmd: USBDBG_SCRIPT_STOP");
                        flag_wait_exit = true;
                        #if TEST_PIC
                        ide_script_running = 0;
                        #else
                        // raise IDE interrupt
                        if (ide_script_running) {
                            // FIXME
                            mp_obj_exception_clear_traceback(mp_const_ide_interrupt);
                            mp_sched_exception(mp_const_ide_interrupt);
                        }
                        #endif
                        break;
                    }
                    case USBDBG_SCRIPT_SAVE: {
                        // TODO
                        pr("cmd: USBDBG_SCRIPT_SAVE");
                        break;
                    }
                    case USBDBG_QUERY_FILE_STAT: {
                        pr("cmd: USBDBG_QUERY_FILE_STAT");
                        usb_tx(&ide_dbg_sv_file.errcode, sizeof(ide_dbg_sv_file.errcode));
                        break;
                    }
                    case USBDBG_WRITEFILE: {
                        pr("cmd: USBDBG_WRITEFILE %u bytes", state->data_length);
                        if ((ide_dbg_sv_file.file == NULL) ||
                            (ide_dbg_sv_file.chunk_buffer == NULL) ||
                            (ide_dbg_sv_file.info.chunk_size < state->data_length)) {
                            ide_dbg_sv_file.errcode = USBDBG_SVFILE_ERR_WRITE_ERR;
                            break;
                        }
                        read_until(usb_cdc_fd, ide_dbg_sv_file.chunk_buffer, state->data_length);
                        if (ide_dbg_sv_file.file == NULL) {
                            ide_dbg_sv_file.errcode = USBDBG_SVFILE_ERR_WRITE_ERR;
                        } else {
                            if (fwrite(ide_dbg_sv_file.chunk_buffer, 1, state->data_length, ide_dbg_sv_file.file) == state->data_length) {
                                ide_dbg_sv_file.errcode = USBDBG_SVFILE_ERR_NONE;
                            } else {
                                fclose(ide_dbg_sv_file.file);
                                ide_dbg_sv_file.file = NULL;
                                ide_dbg_sv_file.errcode = USBDBG_SVFILE_ERR_WRITE_ERR;
                            }
                        }
                        break;
                    }
                    case USBDBG_VERIFYFILE: {
                        pr("cmd: USBDBG_VERIFYFILE");
                        // TODO: use hardware sha256
                        uint32_t resp = USBDBG_SVFILE_VERIFY_ERR_NONE;
                        if (ide_dbg_sv_file.file == NULL) {
                            resp = USBDBG_SVFILE_VERIFY_NOT_OPEN;
                        }
                        fclose(ide_dbg_sv_file.file);
                        ide_dbg_sv_file.file = NULL;
                        char filepath[120] = "/sdcard/";
                        memcpy(filepath + strlen(filepath), ide_dbg_sv_file.info.name, strlen(ide_dbg_sv_file.info.name));
                        FILE* f = fopen(filepath, "r");
                        if (f == NULL) {
                            resp = USBDBG_SVFILE_VERIFY_NOT_OPEN;
                            goto verify_end;
                        }
                        unsigned char buffer[256];
                        CRYAL_SHA256_CTX sha256;
                        sha256_init(&sha256);
                        size_t nbytes;
                        do {
                            nbytes = fread(buffer, 1, sizeof(buffer), f);
                            sha256_update(&sha256, buffer, nbytes);
                        } while (nbytes == sizeof(buffer));
                        fclose(f);
                        uint8_t sha256_result[32];
                        sha256_final(&sha256, sha256_result);
                        if (strncmp((const char *)sha256_result, (const char *)ide_dbg_sv_file.info.sha256, sizeof(sha256_result)) != 0) {
                            resp = USBDBG_SVFILE_VERIFY_SHA2_ERR;
                            pr("file sha256 unmatched");
                            print_sha256(sha256_result);
                        }
                        verify_end:
                        usb_tx(&resp, sizeof(resp));
                        ide_dbg_sv_file.file = NULL;
                        break;
                    }
                    case USBDBG_CREATEFILE: {
                        pr("cmd: USBDBG_CREATEFILE");
                        memset(&ide_dbg_sv_file.info, 0, sizeof(ide_dbg_sv_file.info));
                        if (sizeof(ide_dbg_sv_file.info) != state->data_length) {
                            ide_dbg_sv_file.errcode = USBDBG_SVFILE_ERR_PATH_ERR;
                            pr("Warning: CREATEFILE expect data length %lu, got %u", sizeof(ide_dbg_sv_file.info), state->data_length);
                            break;
                        }
                        // continue receiving
                        read_until(usb_cdc_fd, &ide_dbg_sv_file.info, sizeof(ide_dbg_sv_file.info));
                        pr("create file: chunk_size(%d), name(%s)",
                            ide_dbg_sv_file.info.chunk_size, ide_dbg_sv_file.info.name
                        );
                        print_sha256(ide_dbg_sv_file.info.sha256);
                        // FIXME: use micropython API
                        // if file is opened, we close it.
                        if (ide_dbg_sv_file.file != NULL) {
                            fclose(ide_dbg_sv_file.file);
                        }
                        if (ide_dbg_sv_file.chunk_buffer) {
                            free(ide_dbg_sv_file.chunk_buffer);
                            ide_dbg_sv_file.chunk_buffer = NULL;
                        }
                        char filepath[120] = "/sdcard/";
                        memcpy(filepath + strlen(filepath), ide_dbg_sv_file.info.name, strlen(ide_dbg_sv_file.info.name));
                        ide_dbg_sv_file.file = fopen(filepath, "w");
                        if (ide_dbg_sv_file.file == NULL) {
                            ide_dbg_sv_file.errcode = USBDBG_SVFILE_ERR_OPEN_ERR;
                            break;
                        }
                        ide_dbg_sv_file.chunk_buffer = malloc(ide_dbg_sv_file.info.chunk_size);
                        ide_dbg_sv_file.errcode = USBDBG_SVFILE_ERR_NONE;
                        break;
                    }
                    case USBDBG_SCRIPT_RUNNING: {
                        // DO NOT PRINT
                        #if PRINT_ALL
                        pr("cmd: USBDBG_SCRIPT_RUNNING");
                        #endif
                        usb_tx(&ide_script_running, sizeof(ide_script_running));
                        break;
                    }
                    case USBDBG_TX_BUF_LEN: {
                        // DO NOT PRINT
                        pthread_mutex_lock(&tx_buf_mutex);
                        #if !PRINT_ALL
                        if (tx_buf_len)
                        #endif
                        {
                            pr("cmd: USBDBG_TX_BUF_LEN %u", tx_buf_len);
                        }
                        uint32_t tmp = tx_buf_len;
                        pthread_mutex_unlock(&tx_buf_mutex);
                        usb_tx(&tmp, sizeof(tx_buf_len));
                        break;
                    }
                    case USBDBG_TX_BUF: {
                        pr("cmd: USBDBG_TX_BUF");
                        pthread_mutex_lock(&tx_buf_mutex);
                        usb_tx(tx_buf, tx_buf_len);
                        tx_buf_len = 0;
                        pthread_mutex_unlock(&tx_buf_mutex);
                        break;
                    }
                    case USBDBG_FRAME_SIZE: {
                        // DO NOT PRINT
                        #if PRINT_ALL
                        pr("cmd: USBDBG_FRAME_SIZE");
                        #endif
                        #if TEST_PIC
                        uint32_t resp[3] = {
                            1920, // width
                            1080, // height
                            index_jpeg[pic_idx + 1] - index_jpeg[pic_idx], // size
                        };
                        if (!ide_script_running) {
                            resp[0] = 0;
                            resp[1] = 0;
                            resp[2] = 0;
                        }
                        #else
                        uint32_t resp[3] = {
                            0, // width
                            0, // height
                            0, // size
                        };

                        pthread_mutex_lock(&fb_mutex);
                        if (fb_data) {
                            pr("[omv] use user set fb");
                            resp[0] = fb_width;
                            resp[1] = fb_height;
                            resp[2] = fb_size;
                            fb_from = FB_FROM_USER_SET;
                        }
                        pthread_mutex_unlock(&fb_mutex);
                        if (resp[2] != 0) {
                            goto skip;
                        }
                        #if ENABLE_VO_WRITEBACK
                        // try vo writeback
                        if (vo_wbc_enabled > 0) {
                            pr("[omv] try using writeback");
                            k_video_frame_info frame_info;
                            unsigned error = kd_mpi_wbc_dump_frame(&frame_info, 100);
                            if (error) {
                                pr("[omv] kd_mpi_wbc_dump_frame error: %u", error);
                                goto skip;
                            }
                            frame_info.v_frame.width = 1920;
                            frame_info.v_frame.height = 1080;
                            frame_info.v_frame.pixel_format = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
                            pr("[omv] kd_mpi_wbc_dump_frame success w(%u)h(%u)phy(%08lx)",
                            frame_info.v_frame.width, frame_info.v_frame.height, frame_info.v_frame.phys_addr[0]);
                            // JPEG compressing
                            int hd_jpeg_encode(k_video_frame_info* frame, void** buffer, size_t size, int timeout, void*(*realloc)(void*, unsigned long));
                            int ssize = hd_jpeg_encode(&frame_info, &wbc_jpeg_buffer, wbc_jpeg_buffer_size, 1000, realloc);
                            if (ssize <= 0) {
                                // error
                                goto skip;
                            }
                            kd_mpi_wbc_dump_release(&frame_info);
                            resp[0] = frame_info.v_frame.width;
                            resp[1] = frame_info.v_frame.height;
                            resp[2] = ssize;
                            wbc_jpeg_size = ssize;
                            wbc_jpeg_buffer_size = wbc_jpeg_buffer_size > ssize ? wbc_jpeg_buffer_size : ssize;
                            fb_from = FB_FROM_VO_WRITEBACK;
                        }
                        #endif
                        skip:
                        #endif
                        if (resp[2]) {
                            pr("cmd: USBDBG_FRAME_SIZE %u %u %u", resp[0], resp[1], resp[2]);
                        }
                        usb_tx(&resp, sizeof(resp));
                        break;
                    }
                    case USBDBG_FRAME_DUMP: {
                        // pr("cmd: USBDBG_FRAME_DUMP");
                        // DO NOT PRINT
                        #if PRINT_ALL
                        pr("cmd: USBDBG_FRAME_DUMP");
                        #endif
                        #if TEST_PIC
                        if (state->data_length != index_jpeg[pic_idx + 1] - index_jpeg[pic_idx]) {
                            pr("cmd: USBDBG_FRAME_DUMP, expected size %lu, got %u",
                                index_jpeg[pic_idx + 1] - index_jpeg[pic_idx],
                                state->data_length);
                        }
                        // 1024bytes fragment
                        size_t x = 0;
                        for (; x < state->data_length; x += 1024) {
                            usb_tx(data_jpeg + index_jpeg[pic_idx] + x, 1024);
                        }
                        if (x < state->data_length) {
                            usb_tx(data_jpeg + index_jpeg[pic_idx] + x, state->data_length - x);
                        }
                        pic_idx += 1;
                        pic_idx %= num_jpeg;
                        #else
                        if (fb_from == FB_FROM_USER_SET) {
                            pthread_mutex_lock(&fb_mutex);
                            size_t x = 0;
                            for (; x < fb_size; x += 1024) {
                                usb_tx((uint8_t*)fb_data + x, 1024);
                            }
                            if (x < fb_size) {
                                usb_tx((uint8_t*)fb_data + x, fb_size - x);
                            }
                            free((void*)fb_data);
                            fb_data = NULL;
                            pthread_mutex_unlock(&fb_mutex);
                        }
                        #if ENABLE_VO_WRITEBACK
                        else if (fb_from == FB_FROM_VO_WRITEBACK) {
                            size_t x = 0;
                            for (; x < wbc_jpeg_size; x += 1024) {
                                usb_tx(wbc_jpeg_buffer + x, 1024);
                            }
                            if (x < wbc_jpeg_size) {
                                usb_tx(wbc_jpeg_buffer + x, wbc_jpeg_size - x);
                            }
                        }
                        #endif
                        else {
                            // ???
                        }
                        #endif
                        break;
                    }
                    case USBDBG_SYS_RESET: {
                        // TODO: reset serialport to REPL mode
                        pr("cmd: USBDBG_SYS_RESET");
                        interrupt_ide();
                        break;
                    }
                    case USBDBG_FB_ENABLE: {
                        // FIXME: stream parse
                        if (i + 1 < length) {
                            enable_pic = data[i+1];
                        }
                        pr("cmd: USBDBG_FB_ENABLE, enable(%u)", enable_pic);
                        break;
                    }
                    default:
                        // unknown command
                        pr("unknown command %02x", state->cmd);
                        break;
                }
                state->state = FRAME_HEAD;
                i += 1;
                break;
            case FRAME_RECV:
                if (length - i >= state->recv_lack) {
                    memcpy(state->recv_data, data + i, state->recv_lack);
                    state->state = state->recv_next;
                    // FIXME
                    i += state->recv_lack - 1;
                    state->recv_lack = 0;
                } else {
                    memcpy(state->recv_data, data + i, length - i - 1);
                    state->recv_lack -= length - i - 1;
                    i = length;
                }
                break;
            default:
                state->state = FRAME_HEAD;
                break;
        }
    }
    return IDE_DBG_STATUS_OK;
}

extern volatile bool repl_script_running;

static void* ide_dbg_task(void* args) {
    ide_dbg_state_t state;
    state.state = FRAME_HEAD;
    while (1) {
        struct timeval tv = {
            .tv_sec = 1,
            .tv_usec = 0
        };
        fd_set rfds, efds;
        FD_ZERO(&rfds);
        FD_SET(usb_cdc_fd, &rfds);
        FD_SET(STDIN_FILENO, &rfds);
        FD_ZERO(&efds);
        FD_SET(usb_cdc_fd, &efds);
        int result = select(usb_cdc_fd + 1, &rfds, NULL, &efds, &tv);
        if (result == 0) {
            continue;
        } else if (result < 0) {
            perror("select() error");
            kill(getpid(), SIGINT);
            return NULL;
        }
        if (FD_ISSET(STDIN_FILENO, &rfds)) {
            pr("exit, press enter");
            kill(getpid(), SIGINT);
            return NULL;
        }
        if (FD_ISSET(usb_cdc_fd, &efds)) {
            // RTS
            pr("[usb] RTS");
            if (ide_dbg_attach()) {
                if (flag_wait_exit) {
                    interrupt_ide();
                    flag_wait_exit = false;
                }
            } else {
                interrupt_repl();
            }
        }
        if (!FD_ISSET(usb_cdc_fd, &rfds)) {
            continue;
        }
        ssize_t size = read(usb_cdc_fd, usb_cdc_read_buf, sizeof(usb_cdc_read_buf));
        if (size == 0) {
            pr("[usb] read timeout");
            continue;
        } else if (size < 0) {
            // TODO: error, but ???
            perror("[usb] read ttyUSB1");
        } else if (ide_dbg_attach()) {
            ide_dbg_update(&state, usb_cdc_read_buf, size);
        } else {
            // FIXME: IDE connect
            // FIXME: IDE special token
            const char* IDE_TOKEN = "\x30\x8D\x04\x00\x00\x00"; // CanMV IDE
            const char* IDE_TOKEN2 = "\x30\x80\x0C\x00\x00\x00"; // OpenMV IDE
            const char* IDE_TOKEN3 = "\x30\x87\x04\x00\x00\x00";
            if ((size == 6) && (
                (strncmp((const char*)usb_cdc_read_buf, IDE_TOKEN, size) == 0) ||
                (strncmp((const char*)usb_cdc_read_buf, IDE_TOKEN2, size) == 0) ||
                (strncmp((const char*)usb_cdc_read_buf, IDE_TOKEN3, size) == 0)
                )) {
                // switch to ide mode
                pr("[usb] switch to IDE mode");
                if (!ide_dbg_attach()) {
                    interrupt_repl();
                }
                ide_attached = true;
                ide_dbg_update(&state, usb_cdc_read_buf, size);
            } else {
                // FIXME: mock machine.UART, restore this when UART library finish
                const char* MOCK_FOR_IDE[] = {
                    "from machine import UART\r",
                    "repl = UART.repl_uart()\r",
                    "repl.init(1500000, 8, None, 1, read_buf_len=2048, ide=True)\r"
                };
                if ((size >= 23) && (
                    (strncmp((const char*)usb_cdc_read_buf, MOCK_FOR_IDE[0], 23) == 0) ||
                    (strncmp((const char*)usb_cdc_read_buf, MOCK_FOR_IDE[1], 23) == 0) ||
                    (strncmp((const char*)usb_cdc_read_buf, MOCK_FOR_IDE[2], 23) == 0)
                    )) {
                    // ignore
                    continue;
                }
                // normal REPL
                pr("[usb] read %lu bytes ", size);
                print_raw(usb_cdc_read_buf, size);
                if ((size == 1) && (usb_cdc_read_buf[0] == CHAR_CTRL_C) && repl_script_running) {
                    // terminate script running, FIXME: multithread raise
                    #if MICROPY_KBD_EXCEPTION
                    mp_sched_keyboard_interrupt();
                    //nlr_raise(MP_OBJ_FROM_PTR(&MP_STATE_VM(mp_kbd_exception)));
                    #endif
                } else {
                    if (stdin_write_ptr + size <= sizeof(stdin_ring_buffer)) {
                        memcpy(stdin_ring_buffer + stdin_write_ptr, usb_cdc_read_buf, size);
                        stdin_write_ptr += size;
                    } else {
                        // rotate
                        memcpy(stdin_ring_buffer + stdin_write_ptr, usb_cdc_read_buf, sizeof(stdin_ring_buffer) - stdin_write_ptr);
                        memcpy(stdin_ring_buffer, usb_cdc_read_buf + (sizeof(stdin_ring_buffer) - stdin_write_ptr),
                            stdin_write_ptr + size - sizeof(stdin_ring_buffer));
                        stdin_write_ptr = stdin_write_ptr + size - sizeof(stdin_ring_buffer);
                    }
                    while (size--)
                        sem_post(&stdin_sem);
                }
            }
        }
    }
    return NULL;
}

void sighandler(int sig) {
    pr("get signal %d", sig);
    exit(0);
}

void ide_dbg_init(void) {
    pr("IDE debugger built %s %s", __DATE__, __TIME__);
    usb_cdc_fd = open("/dev/ttyUSB1", O_RDWR);
    if (usb_cdc_fd < 0) {
        perror("open /dev/ttyUSB1 error");
        return;
    }
    sem_init(&script_sem, 0, 0);
    sem_init(&stdin_sem, 0, 0);
    pthread_mutex_init(&fb_mutex, NULL);
    ide_exception_str.data = (const byte*)"IDE interrupt";
    ide_exception_str.len  = 13;
    ide_exception_str.base.type = &mp_type_str;
    ide_exception_str.hash = qstr_compute_hash(ide_exception_str.data, ide_exception_str.len);
    ide_exception_str_tuple = (mp_obj_tuple_t*)malloc(sizeof(mp_obj_tuple_t)+sizeof(mp_obj_t)*1);
    if(ide_exception_str_tuple==NULL)
        return;
    ide_exception_str_tuple->base.type = &mp_type_tuple;
    ide_exception_str_tuple->len = 1;
    ide_exception_str_tuple->items[0] = MP_OBJ_FROM_PTR(&ide_exception_str);
    ide_exception.base.type = &mp_type_Exception;
    ide_exception.traceback_alloc = 0;
    ide_exception.traceback_len = 0;
    ide_exception.traceback_data = NULL;
    ide_exception.args = ide_exception_str_tuple;
    pthread_create(&ide_dbg_task_p, NULL, ide_dbg_task, NULL);
}

#else
#endif

