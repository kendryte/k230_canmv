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
{}

int fb_encode_for_ide_new_size(image_t *img)
{
    return 0;
}

uint32_t framebuffer_get_buffer_size()
{
    return 0;
}

void fb_encode_for_ide(uint8_t *ptr, image_t *img)
{}
