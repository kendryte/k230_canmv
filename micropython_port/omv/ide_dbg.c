/*
 * This file is part of the OpenMV project.
 * Copyright (c) 2013/2014 Ibrahim Abdelkader <i.abdalkader@gmail.com>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * USB debug support.
 *
 */
#include "builtin.h"
#include "mpconfig.h"
#include "boards/k230_evb/mpconfigboard.h"
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

#define CONFIG_CANMV_IDE_SUPPORT 1

#include "ide_dbg.h"
#include <stdio.h>

#if CONFIG_CANMV_IDE_SUPPORT

int usb_cdc_fd;

void mp_hal_stdout_tx_strn(const char *str, size_t len);

#define pr(fmt,...) fprintf(stderr,fmt "\n",##__VA_ARGS__)

static pthread_t ide_dbg_task_p;
static unsigned char usb_cdc_read_buf[1024];
static struct ide_dbg_svfil_t ide_dbg_sv_file;
static mp_obj_exception_t ide_exception; //IDE interrupt
static mp_obj_str_t ide_exception_str;
static mp_obj_tuple_t* ide_exception_str_tuple = NULL;
static mp_obj_t mp_const_ide_interrupt = (mp_obj_t)(&ide_exception);
static sem_t stdin_sem;
static char stdin_ring_buffer[256];
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
    fprintf(stderr, "raw: \"");
    for (size_t i = 0; i < size; i++) {
        fprintf(stderr, "\\x%02X", ((unsigned char*)data)[i]);
    }
    fprintf(stderr, "\"\n");
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
    fprintf(stderr, "SHA256: ");
    for (unsigned i = 0; i < 32; i++) {
        fprintf(stderr, "%02x", sha256[i]);
    }
    fprintf(stderr, "\n");
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
    stdin_write_ptr %= 256;
    sem_post(&stdin_sem);
}

static volatile bool enable_pic = true;

#define TEST_PIC 0

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
                        #if TEST_PIC
                        ide_script_running = 1;
                        #else
                        // recv script string
                        if (script_string != NULL) {
                            free(script_string);
                        }
                        script_string = malloc(state->data_length + 1);
                        read_until(usb_cdc_fd, script_string, state->data_length);
                        script_string[state->data_length] = '\0';
                        // into script mode, interrupt REPL, send CTRL-D
                        ide_script_running = 1;
                        sem_post(&script_sem);
                        #endif
                        break;
                    }
                    case USBDBG_SCRIPT_STOP: {
                        // TODO
                        pr("cmd: USBDBG_SCRIPT_STOP");
                        #if TEST_PIC
                        ide_script_running = 0;
                        #else
                        // raise IDE interrupt
                        if (ide_script_running) {
                            // FIXME
                            mp_obj_exception_clear_traceback(mp_const_ide_interrupt);
                            mp_sched_exception(mp_const_ide_interrupt);
                        }
                        ide_script_running = 0;
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
                        ide_dbg_sv_file.chunk_buffer = malloc(state->data_length);
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
                        // TODO
                        // DO NOT PRINT
                        #if PRINT_ALL
                        pr("cmd: USBDBG_FRAME_SIZE");
                        #endif
                        #if TEST_PIC
                        uint32_t resp[3] = {
                            640, // width
                            680, // height
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
                        #endif
                        usb_tx(&resp, sizeof(resp));
                        break;
                    }
                    case USBDBG_FRAME_DUMP: {
                        // TODO
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
                        #endif
                        break;
                    }
                    case USBDBG_SYS_RESET: {
                        // TODO: reset serialport to REPL mode
                        pr("cmd: USBDBG_SYS_RESET");
                        ide_attached = false;
                        // exit script mode
                        if (script_string) {
                            free(script_string);
                            script_string = NULL;
                        }
                        sem_post(&script_sem);
                        break;
                    }
                    case USBDBG_FB_ENABLE: {
                        // FIXME: stream parse
                        if (i + 1 < length) {
                            enable_pic = data[i++];
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
        ssize_t size = read(usb_cdc_fd, usb_cdc_read_buf, sizeof(usb_cdc_read_buf));
        if (size == 0) {
            // reset request
            fprintf(stderr, "[usb] RTS\n");
            // ignore the first RTS
            static bool first_rts = true;
            if (first_rts) {
                first_rts = false;
            } else {
                interrupt_repl();
            }
        } else if (size <= 3) {
            fprintf(stderr, "[usb] read %lu bytes ", size);
            print_raw(usb_cdc_read_buf, size);
            if ((size == 1) && (usb_cdc_read_buf[0] == CHAR_CTRL_C) && repl_script_running) {
                // terminate script running, FIXME: multithread raise
                #if MICROPY_KBD_EXCEPTION
                mp_obj_exception_clear_traceback(MP_OBJ_FROM_PTR(&MP_STATE_VM(mp_kbd_exception)));
                MP_STATE_THREAD(mp_pending_exception) = &MP_STATE_VM(mp_kbd_exception);
                #if MICROPY_ENABLE_SCHEDULER
                if (MP_STATE_VM(sched_state) == MP_SCHED_IDLE) {
                    MP_STATE_VM(sched_state) = MP_SCHED_PENDING;
                }
                #endif
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
        } else if (size > 0) {
            // FIXME: handshake
            if (!ide_dbg_attach()) {
                interrupt_repl();
            }
            ide_attached = true;
            ide_dbg_update(&state, usb_cdc_read_buf, size);
            if (usb_cdc_read_buf[0] == 0x30) {
                
            }
        }
    }
    return NULL;
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

