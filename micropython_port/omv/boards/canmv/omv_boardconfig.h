/*
 * This file is part of the OpenMV project.
 *
 * Copyright (c) 2013-2021 Ibrahim Abdelkader <iabdalkader@openmv.io>
 * Copyright (c) 2013-2021 Kwabena W. Agyeman <kwagyeman@openmv.io>
 *
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * Board configuration and pin definitions.
 */
#ifndef __OMV_BOARDCONFIG_H__
#define __OMV_BOARDCONFIG_H__

// Enable hardware JPEG
#define OMV_HARDWARE_JPEG                     (0)

// If buffer size is bigger than this threshold, the quality is reduced.
// This is only used for JPEG images sent to the IDE not normal compression.
#define JPEG_QUALITY_THRESH                   (1920 * 1080 * 2)

// Low and high JPEG QS.
#define JPEG_QUALITY_LOW                      50
#define JPEG_QUALITY_HIGH                     90

// FB Heap Block Size
#define OMV_UMM_BLOCK_SIZE                    256

#define OMV_FB_ALLOC_SIZE                     (8 * 1024 * 1024) // minimum fb alloc size
#define OMV_FB_ALLOC_BUFFER_COUNT             (256)

#define OMV_JPEG_BUF_SIZE                     (1024 * 1024) // IDE JPEG buffer (header + data).

#endif //__OMV_BOARDCONFIG_H__
