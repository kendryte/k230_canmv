/*
 * This file is part of the OpenMV project.
 *
 * Copyright (c) 2013-2021 Ibrahim Abdelkader <iabdalkader@openmv.io>
 * Copyright (c) 2013-2021 Kwabena W. Agyeman <kwagyeman@openmv.io>
 *
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * Framebuffer functions.
 */
#include <stdio.h>
#include "mpprint.h"
#include "framebuffer.h"
#include "omv_boardconfig.h"

void framebuffer_update_jpeg_buffer()
{
    fprintf(stderr, "[omv] %s\n", __func__);
}

int fb_encode_for_ide_new_size(image_t *img)
{
    // fprintf(stderr, "[omv] %s %lu\n", __func__, img->phy_addr);
    return 0;
}

uint32_t framebuffer_get_buffer_size()
{
    // fprintf(stderr, "[omv] %s\n", __func__);
    return 0;
}

void ide_set_fb(const void* data, uint32_t size, uint32_t width, uint32_t height);

void fb_encode_for_ide(uint8_t *ptr, image_t *img)
{
    // fprintf(stderr, "[omv] %s %p %u %p\n", __func__, ptr, img->size, img->data);
    ide_set_fb(img->data, img->size, img->w, img->h);
    #if 0
    FILE* f = fopen("/sdcard/img0.bin", "w");
    fwrite(ptr, 1, img->size, f);
    fclose(f);
    fopen("/sdcard/img1.bin", "w");
    fwrite(img->data, 1, img->size, f);
    fclose(f);
    #endif
}
