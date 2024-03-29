/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stddef.h>
#include <stdio.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

#include "py/mphal.h"
#include "py/mpthread.h"
#include "py/runtime.h"
#include "extmod/misc.h"

#if defined(__GLIBC__) && defined(__GLIBC_PREREQ)
#if __GLIBC_PREREQ(2, 25)
#include <sys/random.h>
#define _HAVE_GETRANDOM
#endif
#endif

extern bool process_exit;
extern bool ide_dbg_attach(void);
extern void interrupt_repl(void);
extern void interrupt_ide(void);

STATIC void sigint_handler_other(int signum) {
    if (signum == SIGINT) {
        process_exit = true;
        if (ide_dbg_attach())
            interrupt_ide();
        else
            interrupt_repl();
    }
}

STATIC void sigint_handler_ctrl_c(int signum) {
    if (signum == SIGINT) {
        #if MICROPY_ASYNC_KBD_INTR
        #if MICROPY_PY_THREAD_GIL
        // Since signals can occur at any time, we may not be holding the GIL when
        // this callback is called, so it is not safe to raise an exception here
        #error "MICROPY_ASYNC_KBD_INTR and MICROPY_PY_THREAD_GIL are not compatible"
        #endif
        mp_obj_exception_clear_traceback(MP_OBJ_FROM_PTR(&MP_STATE_VM(mp_kbd_exception)));
        sigset_t mask;
        sigemptyset(&mask);
        // On entry to handler, its signal is blocked, and unblocked on
        // normal exit. As we instead perform longjmp, unblock it manually.
        sigprocmask(SIG_SETMASK, &mask, NULL);
        nlr_raise(MP_OBJ_FROM_PTR(&MP_STATE_VM(mp_kbd_exception)));
        #else
        process_exit = true;
        mp_thread_set_exception_main(MP_OBJ_FROM_PTR(&MP_STATE_VM(mp_kbd_exception)));
        if (!ide_dbg_attach())
            interrupt_repl();
        #endif
    }
}

void mp_hal_set_interrupt_char(int c) {
    if (c == CHAR_CTRL_C) {
        struct sigaction sa;
        sa.sa_flags = 0;
        sa.sa_handler = sigint_handler_ctrl_c;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGINT, &sa, NULL);
    } else {
        struct sigaction sa;
        sa.sa_flags = 0;
        sa.sa_handler = sigint_handler_other;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGINT, &sa, NULL);
    }
}

#if MICROPY_USE_READLINE == 1

#include <termios.h>

static struct termios orig_termios;

void mp_hal_stdio_mode_raw(void) {
    // save and set terminal settings
    tcgetattr(0, &orig_termios);
    static struct termios termios;
    termios = orig_termios;
    termios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    termios.c_cflag = (termios.c_cflag & ~(CSIZE | PARENB)) | CS8;
    termios.c_lflag = 0;
    termios.c_cc[VMIN] = 1;
    termios.c_cc[VTIME] = 0;
    tcsetattr(0, TCSAFLUSH, &termios);
}

void mp_hal_stdio_mode_orig(void) {
    // restore terminal settings
    tcsetattr(0, TCSAFLUSH, &orig_termios);
}

#endif

int mp_hal_stdin_rx_chr(void) {
    int c;
    extern int usb_rx(void);
    while (1) {
        MP_THREAD_GIL_EXIT();
        c = usb_rx();
        MP_THREAD_GIL_ENTER();
        if (c != -1)
            break;
        mp_handle_pending(true);
    }
    return c;
}

void mp_hal_stdout_tx_strn(const char *str, size_t len) {
    extern bool ide_dbg_attach(void);
    extern bool command_line_mode;
    if (command_line_mode) {
        fwrite(str, 1, len, stdout);
        return;
    }
    if (ide_dbg_attach()) {
        extern void mpy_stdout_tx(const char* data, size_t size);
        mpy_stdout_tx(str, len);
    } else {
        extern int usb_tx(const void* buffer, size_t size);
        usb_tx(str, len);
    }
    mp_os_dupterm_tx_strn(str, len);
}

// replace \n to \r\n
void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len) {
    size_t last_seg = 0;

    for (size_t i = 0; i < len; i++) {
        if (str[i] == '\n') {
            mp_hal_stdout_tx_strn(str + last_seg, i - last_seg);
            mp_hal_stdout_tx_strn("\r\n", 2);
            last_seg = i + 1;
        }
    }
    if (last_seg < len)
        mp_hal_stdout_tx_strn(str + last_seg, len - last_seg);
}

void mp_hal_stdout_tx_str(const char *str) {
    mp_hal_stdout_tx_strn(str, strlen(str));
}

void mp_hal_stdout_tx_str_cooked(const char* str) {
    mp_hal_stdout_tx_strn_cooked(str, strlen(str));
}
