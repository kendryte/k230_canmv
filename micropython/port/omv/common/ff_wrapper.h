/*
 * This file is part of the OpenMV project.
 * Copyright (c) 2013-2016 Kwabena W. Agyeman <kwagyeman@openmv.io>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * File System Helper Functions
 *
 */
#ifndef __FF_WRAPPER_H__
#define __FF_WRAPPER_H__
#include <stdint.h>
#include <stdio.h>

#define FIL FILE*
#define FRESULT int
#define UINT size_t
#define FR_OK 0

extern const char *ffs_strerror(FRESULT res);

size_t f_tell(FIL *fp);
void f_close(FIL *fp);
size_t f_size(FIL *fp);
int f_eof(FIL *fp);
FRESULT f_read(FIL* fp, void* buff, UINT btr, UINT* br);
FRESULT f_write(FIL* fp, const void* buff, UINT btw, UINT* bw);
FRESULT f_open_helper(FIL *fp, const char *path, char *mode);

void ff_unsupported_format(FIL *fp);
void ff_file_corrupted(FIL *fp);
void ff_not_equal(FIL *fp);
void ff_no_intersection(FIL *fp);
void file_read_open(FIL *fp, const char *path);
void file_write_open(FIL *fp, const char *path);
void file_read_write_open_existing(FIL *fp, const char *path);
void file_read_write_open_always(FIL *fp, const char *path);
void file_close(FIL *fp);
void file_seek(FIL *fp, UINT offset);
void file_truncate(FIL *fp);
void file_sync(FIL *fp);

// File buffer functions.
void file_buffer_init0();
void file_buffer_on(FIL *fp); // does fb_alloc_all
uint32_t file_tell_w_buf(FIL *fp); // valid between on and off
uint32_t file_size_w_buf(FIL *fp); // valid between on and off
void file_buffer_off(FIL *fp); // does fb_free
void read_byte(FIL *fp, uint8_t *value);
void read_byte_expect(FIL *fp, uint8_t value);
void read_byte_ignore(FIL *fp);
void read_word(FIL *fp, uint16_t *value);
void read_word_expect(FIL *fp, uint16_t value);
void read_word_ignore(FIL *fp);
void read_long(FIL *fp, uint32_t *value);
void read_long_expect(FIL *fp, uint32_t value);
void read_long_ignore(FIL *fp);
void read_data(FIL *fp, void *data, UINT size);
void write_byte(FIL *fp, uint8_t value);
void write_word(FIL *fp, uint16_t value);
void write_long(FIL *fp, uint32_t value);
void write_data(FIL *fp, const void *data, UINT size);
#endif /* __FF_WRAPPER_H__ */
