/*
 * This file is part of the OpenMV project.
 * Copyright (c) 2013-2016 Kwabena W. Agyeman <kwagyeman@openmv.io>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * File System Helper Functions
 *
 */
#include "imlib_config.h"
#if defined(IMLIB_ENABLE_IMAGE_FILE_IO)

#include <string.h>
#include <errno.h>
#include <unistd.h>
// #include <sys/types.h>
#include "py/runtime.h"
#include "omv_common.h"
#include "fb_alloc.h"
#include "ff_wrapper.h"

#define FF_MIN(x, y)    (((x) < (y))?(x):(y))

size_t f_tell(FIL *fp)
{
    return ftell(*fp);
}

void f_close(FIL *fp)
{
    fclose(*fp);
}

size_t f_size(FIL *fp)
{
    size_t cur = ftell(*fp);
    fseek(*fp, 0, SEEK_END);
    size_t size = ftell(*fp);
    fseek(*fp, cur, SEEK_SET);
    return size;
}

int f_eof(FIL *fp)
{
    return feof(*fp);
}

const char *ffs_strerror(FRESULT res)
{
    return strerror(res);
}

NORETURN static void ff_fail(FIL *fp, FRESULT res) {
    if (fp && *fp) {
        fclose(*fp);
    }
    mp_raise_msg(&mp_type_OSError, (mp_rom_error_text_t) ffs_strerror(res));
}

NORETURN static void ff_expect_fail(FIL *fp) {
    if (fp && *fp) {
        fclose(*fp);
    }
    mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("Unexpected value read!"));
}

NORETURN void ff_unsupported_format(FIL *fp) {
    if (fp && *fp) {
        fclose(*fp);
    }
    mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("Unsupported format!"));
}

NORETURN void ff_file_corrupted(FIL *fp) {
    if (fp && *fp) {
        fclose(*fp);
    }
    mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("File corrupted!"));
}

NORETURN void ff_not_equal(FIL *fp) {
    if (fp && *fp) {
        fclose(*fp);
    }
    mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("Images not equal!"));
}

NORETURN void ff_no_intersection(FIL *fp) {
    if (fp && *fp) {
        fclose(*fp);
    }
    mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("No intersection!"));
}

void file_read_open(FIL *fp, const char *path) {
    FRESULT res = f_open_helper(fp, path, "rb");
    if (res != FR_OK) {
        ff_fail(fp, res);
    }
}

void file_write_open(FIL *fp, const char *path) {
    FRESULT res = f_open_helper(fp, path, "wb");
    if (res != FR_OK) {
        ff_fail(fp, res);
    }
}

void file_read_write_open_existing(FIL *fp, const char *path) {
    FRESULT res = f_open_helper(fp, path, "rb+");
    if (res != FR_OK) {
        ff_fail(fp, res);
    }
}

void file_read_write_open_always(FIL *fp, const char *path) {
    FRESULT res = f_open_helper(fp, path, "wb+");
    if (res != FR_OK) {
        ff_fail(fp, res);
    }
}

void file_close(FIL *fp) {
    FRESULT res = fclose(*fp);
    if (res != FR_OK) {
        ff_fail(fp, res);
    }
}

void file_seek(FIL *fp, UINT offset) {
    FRESULT res = fseek(*fp, offset, SEEK_SET);
    if (res != FR_OK) {
        ff_fail(fp, res);
    }
}

void file_truncate(FIL *fp) {
    ftruncate(fileno(*fp), ftell(*fp));
}

void file_sync(FIL *fp) {
    FRESULT res = fflush(*fp);
    if (res != FR_OK) {
        ff_fail(fp, res);
    }
}

FRESULT f_read(FIL* fp, void* buff, UINT btr, UINT* br)
{
    if (1 != fread(buff, btr, 1, *fp))
        return errno;
    *br = btr;
    return 0;
}

FRESULT f_write(FIL* fp, const void* buff, UINT btw, UINT* bw)
{
    if (1 != fwrite(buff, btw, 1, *fp))
        return errno;
    *bw = btw;
    return 0;
}

FRESULT f_open_helper(FIL *fp, const char *path, char *mode) {
    *fp = fopen(path, mode);
    return *fp == NULL ? errno : 0;
}

uint32_t file_tell_w_buf(FIL *fp) {
    return ftell(*fp);
}

uint32_t file_size_w_buf(FIL *fp) {
    size_t cur = ftell(*fp);
    fseek(*fp, 0, SEEK_END);
    size_t size = ftell(*fp);
    fseek(*fp, cur, SEEK_SET);
    return size;
}

void file_buffer_on(FIL *fp) {
}

void file_buffer_off(FIL *fp) {
}

void read_byte(FIL *fp, uint8_t *value) {
    read_data(fp, value, 1);
}

void read_byte_expect(FIL *fp, uint8_t value) {
    uint8_t compare;
    read_byte(fp, &compare);
    if (value != compare) {
        ff_expect_fail(fp);
    }
}

void read_byte_ignore(FIL *fp) {
    uint8_t trash;
    read_byte(fp, &trash);
}

void read_word(FIL *fp, uint16_t *value) {
    read_data(fp, value, 2);
}

void read_word_expect(FIL *fp, uint16_t value) {
    uint16_t compare;
    read_word(fp, &compare);
    if (value != compare) {
        ff_expect_fail(fp);
    }
}

void read_word_ignore(FIL *fp) {
    uint16_t trash;
    read_word(fp, &trash);
}

void read_long(FIL *fp, uint32_t *value) {
    read_data(fp, value, 4);
}

void read_long_expect(FIL *fp, uint32_t value) {
    uint32_t compare;
    read_long(fp, &compare);
    if (value != compare) {
        ff_expect_fail(fp);
    }
}

void read_long_ignore(FIL *fp) {
    uint32_t trash;
    read_long(fp, &trash);
}

void read_data(FIL *fp, void *data, UINT size) {
    if (1 != fread(data, size, 1, *fp))
        ff_fail(fp, errno);
}

void write_byte(FIL *fp, uint8_t value) {
    write_data(fp, &value, sizeof(value));
}

void write_word(FIL *fp, uint16_t value) {
    write_data(fp, &value, sizeof(value));
}

void write_long(FIL *fp, uint32_t value) {
    write_data(fp, &value, sizeof(value));
}

void write_data(FIL *fp, const void *data, UINT size) {
    if (1 != fwrite(data, size, 1, *fp))
        ff_fail(fp, errno);
}
#endif //IMLIB_ENABLE_IMAGE_FILE_IO
