/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
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

// Options to control how MicroPython is built for this port, overriding
// defaults in py/mpconfig.h. This file is mostly about configuring the
// features to work on Unix-like systems, see mpconfigvariant.h (and
// mpconfigvariant_common.h) for feature enabling.

// For size_t and ssize_t
#include <unistd.h>
#include <stdlib.h>
#include "mpconfigboard.h"

#ifndef MICROPY_CONFIG_ROM_LEVEL
#define MICROPY_CONFIG_ROM_LEVEL (MICROPY_CONFIG_ROM_LEVEL_EVERYTHING)
#endif

#define MICROPY_PY_SYS_PLATFORM  "rt-smart"

#ifndef MICROPY_PY_SYS_PATH_DEFAULT
#define MICROPY_PY_SYS_PATH_DEFAULT ".frozen:/sdcard/app:/sdcard"
#endif

#define MP_STATE_PORT MP_STATE_VM

typedef long mp_int_t; // must be pointer size
typedef unsigned long mp_uint_t; // must be pointer size
typedef long long mp_off_t;

// We need to provide a declaration/definition of alloca()
// unless support for it is disabled.
#if !defined(MICROPY_NO_ALLOCA) || MICROPY_NO_ALLOCA == 0
#if defined(__FreeBSD__) || defined(__NetBSD__)
#include <stdlib.h>
#else
#include <alloca.h>
#endif
#endif

#define MICROPY_USE_INTERNAL_PRINTF (0)
#define MICROPY_USE_READLINE        (1)
// Always enable GC.
#define MICROPY_ENABLE_GC           (1)
#define MICROPY_GC_SPLIT_HEAP       (1)
#define MICROPY_GC_SPLIT_HEAP_AUTO  (1)
#define MICROPY_GCREGS_SETJMP       (1)
// Enable the VFS, and enable the posix "filesystem".
#define MICROPY_ENABLE_FINALISER    (1)
#define MICROPY_VFS                 (1)
#define MICROPY_READER_VFS          (1)
#define MICROPY_HELPER_LEXER_UNIX   (1)
#define MICROPY_VFS_POSIX           (1)
#define MICROPY_PY_OS_STATVFS       (0)
#define MICROPY_READER_POSIX        (1)
#define MICROPY_TRACKED_ALLOC       (0)
// Assume that select() call, interrupted with a signal, and erroring
// with EINTR, updates remaining timeout value.
#define MICROPY_SELECT_REMAINING_TIME (1)
// Disable stackless by default.
#define MICROPY_STACKLESS           (0)
#define MICROPY_STACKLESS_STRICT    (0)

#define MICROPY_ALLOC_PATH_MAX      (PATH_MAX)
#define MICROPY_PY_HASHLIB_MD5      (1)
#define MICROPY_PY_HASHLIB_SHA1     (1)
#define MICROPY_PY_HASHLIB_SHA256   (1)
#define MICROPY_PY_SELECT           (1)
#define MICROPY_PY_THREAD           (1)
#define MICROPY_PY_OS_DUPTERM       (1)
#define MICROPY_SCHEDULER_DEPTH     (8)

#define MICROPY_PY_NETWORK_CANMV (1)
#define MICROPY_PY_NETWORK_HOSTNAME_DEFAULT "canmv"
#define MICROPY_PY_SOCKET_CANMV (1)  //socket
// Ensure builtinimport.c works with -m.
#define MICROPY_MODULE_OVERRIDE_MAIN_IMPORT (1)

// Don't default sys.argv and sys.path because we do that in main.
#define MICROPY_PY_SYS_PATH_ARGV_DEFAULTS (0)

// Enable sys.executable.
#define MICROPY_PY_SYS_EXECUTABLE (1)

#define MICROPY_PY_SOCKET_LISTEN_BACKLOG_DEFAULT (SOMAXCONN < 128 ? SOMAXCONN : 128)

// Bare-metal ports don't have stderr. Printing debug to stderr may give tests
// which check stdout a chance to pass, etc.
extern const struct _mp_print_t mp_stderr_print;
#define MICROPY_DEBUG_PRINTER (&mp_stderr_print)
#define MICROPY_ERROR_PRINTER (&mp_stderr_print)

// From "man readdir": "Under glibc, programs can check for the availability
// of the fields [in struct dirent] not defined in POSIX.1 by testing whether
// the macros [...], _DIRENT_HAVE_D_TYPE are defined."
// Other libc's don't define it, but proactively assume that dirent->d_type
// is available on a modern *nix system.
#ifndef _DIRENT_HAVE_D_TYPE
#define _DIRENT_HAVE_D_TYPE (1)
#endif
// This macro is not provided by glibc but we need it so ports that don't have
// dirent->d_ino can disable the use of this field.
#ifndef _DIRENT_HAVE_D_INO
#define _DIRENT_HAVE_D_INO (1)
#endif

#include <stdio.h>

// If threading is enabled, configure the atomic section.
#if MICROPY_PY_THREAD
#define MICROPY_BEGIN_ATOMIC_SECTION() (mp_thread_unix_begin_atomic_section(), 0xffffffff)
#define MICROPY_END_ATOMIC_SECTION(x) (void)x; mp_thread_unix_end_atomic_section()
#endif

// In lieu of a WFI(), slow down polling from being a tight loop.
#ifndef MICROPY_EVENT_POLL_HOOK
#define MICROPY_EVENT_POLL_HOOK \
    do { \
        extern void mp_handle_pending(bool); \
        mp_thread_exitpoint(EXITPOINT_ENABLE_SLEEP); \
        mp_handle_pending(true); \
        MP_THREAD_GIL_EXIT(); \
        usleep(500); /* equivalent to mp_hal_delay_us(500) */ \
        MP_THREAD_GIL_ENTER(); \
    } while (0);
#endif

// Configure the implementation of machine.idle().
#include <sched.h>
#define MICROPY_UNIX_MACHINE_IDLE sched_yield();

// Send raise KeyboardInterrupt directly from the signal handler rather than
// scheduling it into the VM.
#define MICROPY_ASYNC_KBD_INTR         (0)

// Enable helpers for printing debugging information.
#ifndef MICROPY_DEBUG_PRINTERS
#define MICROPY_DEBUG_PRINTERS         (1)
#endif

// Enable floating point by default.
#ifndef MICROPY_FLOAT_IMPL
#define MICROPY_FLOAT_IMPL             (MICROPY_FLOAT_IMPL_FLOAT)
#endif

// Enable arbitrary precision long-int by default.
#ifndef MICROPY_LONGINT_IMPL
#define MICROPY_LONGINT_IMPL           (MICROPY_LONGINT_IMPL_MPZ)
#endif

// Enable use of C libraries that need read/write/lseek/fsync, e.g. axtls.
#define MICROPY_STREAMS_POSIX_API      (1)

// REPL conveniences.
#define MICROPY_REPL_EMACS_WORDS_MOVE  (1)
#define MICROPY_REPL_EMACS_EXTRA_WORDS_MOVE (1)
#define MICROPY_USE_READLINE_HISTORY   (1)
#ifndef MICROPY_READLINE_HISTORY_SIZE
#define MICROPY_READLINE_HISTORY_SIZE  (50)
#endif

// Seed random on import.
// #define MICROPY_PY_RANDOM_SEED_INIT_FUNC (mp_random_seed_init())

// Allow exception details in low-memory conditions.
#define MICROPY_ENABLE_EMERGENCY_EXCEPTION_BUF (1)
#define MICROPY_EMERGENCY_EXCEPTION_BUF_SIZE (256)

// Allow loading of .mpy files.
#define MICROPY_PERSISTENT_CODE_LOAD   (1)

// Extra memory debugging.
#define MICROPY_MALLOC_USES_ALLOCATED_SIZE (0)
#define MICROPY_MEM_STATS              (0)

// Enable a small performance boost for the VM.
#define MICROPY_OPT_COMPUTED_GOTO      (1)

// Return number of collected objects from gc.collect().
#define MICROPY_PY_GC_COLLECT_RETVAL   (1)

// Enable detailed error messages and warnings.
#define MICROPY_ERROR_REPORTING     (MICROPY_ERROR_REPORTING_DETAILED)
#define MICROPY_WARNINGS               (1)
#define MICROPY_PY_STR_BYTES_CMP_WARN  (1)

// Configure the "sys" module with features not usually enabled on bare-metal.
#define MICROPY_PY_SYS_ATEXIT          (1)
#define MICROPY_PY_SYS_EXC_INFO        (1)

// Configure the "os" module with extra unix features.
#define MICROPY_PY_OS_INCLUDEFILE      "core/modos.c"
#define MICROPY_PY_OS_ERRNO            (1)
#define MICROPY_PY_OS_GETENV_PUTENV_UNSETENV (1)
#define MICROPY_PY_OS_SEP              (1)
#define MICROPY_PY_OS_SYSTEM           (1)
#define MICROPY_PY_OS_URANDOM          (0)

// Enable the unix-specific "time" module.
#define MICROPY_EPOCH_IS_1970          (1)
#define MICROPY_PY_TIME                (1)
#define MICROPY_PY_TIME_TIME_TIME_NS   (1)
#define MICROPY_PY_TIME_INCLUDEFILE    "core/modtime.c"

#if MICROPY_PY_SSL
#define MICROPY_PY_HASHLIB_MD5         (1)
#define MICROPY_PY_HASHLIB_SHA1        (1)
#define MICROPY_PY_CRYPTOLIB           (1)
#define MICROPY_PY_CRYPTOLIB_CONSTS    (1)
#define MICROPY_PY_CRYPTOLIB_CTR       (1)
#endif

// Enable the "websocket" module.
#define MICROPY_PY_WEBSOCKET           (0)

// Enable the "machine" module, mostly for machine.mem*.
#define MICROPY_PY_MACHINE             (1)

#define MICROPY_PY_NNCASE_RUNTIME      (1)

// uctypes
#define MICROPY_PY_UCTYPES_STRING_AT_MAX_SIZE (1024 * 1024)

// ulab
#define ULAB_MAX_DIMS   (4)
