/*
 * This file is part of the OpenMV project.
 * Copyright (c) 2013-2016 Kwabena W. Agyeman <kwagyeman@openmv.io>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * Interface for using extra frame buffer RAM as a stack.
 *
 */
#include <stdlib.h>
#include <string.h>
#include "py/obj.h"
#include "py/runtime.h"
#include "fb_alloc.h"
#include "omv_boardconfig.h"

static struct {
    struct {
        uint32_t size;
        void *ptr;
    } buf[OMV_FB_ALLOC_BUFFER_COUNT];
    uint32_t alloc_buffer;
    uint32_t alloc_buffer_peak;
    uint32_t alloc_bytes;
    uint32_t alloc_bytes_peak;
} fb_alloc_mgt;

#define FB_MARK_FLAG        0x1
#define FB_PERMANENT_FLAG   0x2

#define FB_ALLOC_ALIGNMENT  32

void fb_alloc_fail() {
    mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("Out of fast frame buffer stack memory"));
}

static void fb_alloc_buffer_fail() {
    mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("Out of fast frame buffer stack index"));
}

void fb_alloc_init0() {
    memset(&fb_alloc_mgt, 0, sizeof(fb_alloc_mgt));
}

uint32_t fb_avail() {
    return OMV_FB_ALLOC_SIZE - fb_alloc_mgt.alloc_bytes;
}

void fb_alloc_mark() {
    if (fb_alloc_mgt.alloc_buffer >= OMV_FB_ALLOC_BUFFER_COUNT)
        fb_alloc_buffer_fail();

    fb_alloc_mgt.buf[fb_alloc_mgt.alloc_buffer].size = FB_MARK_FLAG;
    fb_alloc_mgt.buf[fb_alloc_mgt.alloc_buffer].ptr = NULL;

    fb_alloc_mgt.alloc_buffer++;
    if (fb_alloc_mgt.alloc_buffer > fb_alloc_mgt.alloc_buffer_peak)
        fb_alloc_mgt.alloc_buffer_peak = fb_alloc_mgt.alloc_buffer;
}

static void int_fb_alloc_free_till_mark(bool free_permanent) {
    while (fb_alloc_mgt.alloc_buffer) {
        uint32_t size;
        size = fb_alloc_mgt.buf[fb_alloc_mgt.alloc_buffer - 1].size;
        if ((!free_permanent) && (size & FB_PERMANENT_FLAG))
            return;
        fb_alloc_mgt.alloc_buffer--;
        fb_alloc_mgt.buf[fb_alloc_mgt.alloc_buffer].size = 0;
        if (size & (~7UL)) {
            free(fb_alloc_mgt.buf[fb_alloc_mgt.alloc_buffer].ptr);
            fb_alloc_mgt.buf[fb_alloc_mgt.alloc_buffer].ptr = NULL;
            fb_alloc_mgt.alloc_bytes -= (size & (~7UL));
        }
        if (size & FB_MARK_FLAG)
            break;
    }
}

void fb_alloc_free_till_mark() {
    int_fb_alloc_free_till_mark(false);
}

void fb_alloc_mark_permanent() {
    fb_alloc_mgt.buf[fb_alloc_mgt.alloc_buffer - 1].size |= FB_PERMANENT_FLAG;
}

void fb_alloc_free_till_mark_past_mark_permanent() {
    int_fb_alloc_free_till_mark(true);
}

// returns null pointer without error if size==0
void *fb_alloc(uint32_t size, int hints) {
    uint32_t align;
    void *ptr;

    if (!size)
        return NULL;

    align = hints & FB_ALLOC_CACHE_ALIGN ? FB_ALLOC_ALIGNMENT : 8;
    size = size < 8 ? 8 : size;

    if (fb_alloc_mgt.alloc_buffer >= OMV_FB_ALLOC_BUFFER_COUNT)
        fb_alloc_buffer_fail();

    if (size > fb_avail())
        fb_alloc_fail();

    ptr = aligned_alloc(align, size);
    if (ptr == NULL)
        fb_alloc_fail();

    fb_alloc_mgt.buf[fb_alloc_mgt.alloc_buffer].size = size;
    fb_alloc_mgt.buf[fb_alloc_mgt.alloc_buffer].ptr = ptr;

    fb_alloc_mgt.alloc_buffer++;
    if (fb_alloc_mgt.alloc_buffer > fb_alloc_mgt.alloc_buffer_peak)
        fb_alloc_mgt.alloc_buffer_peak = fb_alloc_mgt.alloc_buffer;

    fb_alloc_mgt.alloc_bytes += size;
    if (fb_alloc_mgt.alloc_bytes > fb_alloc_mgt.alloc_bytes_peak)
        fb_alloc_mgt.alloc_bytes_peak = fb_alloc_mgt.alloc_bytes;

    return ptr;
}

// returns null pointer without error if passed size==0
void *fb_alloc0(uint32_t size, int hints) {
    void *mem = fb_alloc(size, hints);
    memset(mem, 0, size); // does nothing if size is zero.
    return mem;
}

void *fb_alloc_all(uint32_t *size, int hints) {
    uint32_t align, avail;
    void *ptr;

    align = hints & FB_ALLOC_CACHE_ALIGN ? FB_ALLOC_ALIGNMENT : 8;
    avail = fb_avail();

    if (avail < 8 || fb_alloc_mgt.alloc_buffer >= OMV_FB_ALLOC_BUFFER_COUNT)
        return NULL;

    ptr = aligned_alloc(align, avail);
    if (ptr == NULL)
        return NULL;

    fb_alloc_mgt.buf[fb_alloc_mgt.alloc_buffer].size = avail;
    fb_alloc_mgt.buf[fb_alloc_mgt.alloc_buffer].ptr = ptr;

    fb_alloc_mgt.alloc_buffer++;
    if (fb_alloc_mgt.alloc_buffer > fb_alloc_mgt.alloc_buffer_peak)
        fb_alloc_mgt.alloc_buffer_peak = fb_alloc_mgt.alloc_buffer;

    fb_alloc_mgt.alloc_bytes += avail;
    if (fb_alloc_mgt.alloc_bytes > fb_alloc_mgt.alloc_bytes_peak)
        fb_alloc_mgt.alloc_bytes_peak = fb_alloc_mgt.alloc_bytes;

    *size = avail;

    return ptr;
}

// returns null pointer without error if returned size==0
void *fb_alloc0_all(uint32_t *size, int hints) {
    void *mem = fb_alloc_all(size, hints);
    memset(mem, 0, *size); // does nothing if size is zero.
    return mem;
}

void fb_free() {
    if (fb_alloc_mgt.alloc_buffer) {
        uint32_t size;
        fb_alloc_mgt.alloc_buffer--;
        size = fb_alloc_mgt.buf[fb_alloc_mgt.alloc_buffer].size & (~7UL);
        fb_alloc_mgt.buf[fb_alloc_mgt.alloc_buffer].size = 0;
        if (size) {
            free(fb_alloc_mgt.buf[fb_alloc_mgt.alloc_buffer].ptr);
            fb_alloc_mgt.buf[fb_alloc_mgt.alloc_buffer].ptr = NULL;
            fb_alloc_mgt.alloc_bytes -= size;
        }
    }
}

void fb_free_all() {
    while (fb_alloc_mgt.alloc_buffer) {
        fb_free();
    }
}
