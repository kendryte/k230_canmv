/*
 * This file is part of the OpenMV project.
 *
 * Copyright (c) 2013-2021 Ibrahim Abdelkader <iabdalkader@openmv.io>
 * Copyright (c) 2013-2021 Kwabena W. Agyeman <kwagyeman@openmv.io>
 *
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * QR-code recognition library.
 */
#include "imlib.h"
#ifdef IMLIB_ENABLE_QRCODES
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"

// *INDENT-OFF*
////////////////////////////////////////////////////////////////////////////////////////////////////
//////// "quirc.h"
////////////////////////////////////////////////////////////////////////////////////////////////////

/* quirc -- QR-code recognition library
 * Copyright (C) 2010-2012 Daniel Beer <dlbeer@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

struct quirc;

/* Obtain the library version string. */
const char *quirc_version(void);

/* Construct a new QR-code recognizer. This function will return NULL
 * if sufficient memory could not be allocated.
 */
struct quirc *quirc_new(void);

/* Destroy a QR-code recognizer. */
void quirc_destroy(struct quirc *q);

/* Resize the QR-code recognizer. The size of an image must be
 * specified before codes can be analyzed.
 *
 * This function returns 0 on success, or -1 if sufficient memory could
 * not be allocated.
 */
int quirc_resize(struct quirc *q, int w, int h);

/* These functions are used to process images for QR-code recognition.
 * quirc_begin() must first be called to obtain access to a buffer into
 * which the input image should be placed. Optionally, the current
 * width and height may be returned.
 *
 * After filling the buffer, quirc_end() should be called to process
 * the image for QR-code recognition. The locations and content of each
 * code may be obtained using accessor functions described below.
 */
uint8_t *quirc_begin(struct quirc *q, int *w, int *h);
void quirc_end(struct quirc *q);

/* This structure describes a location in the input image buffer. */
struct quirc_point {
    int x;
    int y;
};

/* This enum describes the various decoder errors which may occur. */
typedef enum {
	QUIRC_SUCCESS = 0,
	QUIRC_ERROR_INVALID_GRID_SIZE,
	QUIRC_ERROR_INVALID_VERSION,
	QUIRC_ERROR_FORMAT_ECC,
	QUIRC_ERROR_DATA_ECC,
	QUIRC_ERROR_UNKNOWN_DATA_TYPE,
	QUIRC_ERROR_DATA_OVERFLOW,
	QUIRC_ERROR_DATA_UNDERFLOW
} quirc_decode_error_t;

/* Return a string error message for an error code. */
const char *quirc_strerror(quirc_decode_error_t err);

/* Limits on the maximum size of QR-codes and their content. */
#define QUIRC_MAX_VERSION	40
#define QUIRC_MAX_GRID_SIZE	(QUIRC_MAX_VERSION * 4 + 17)
#define QUIRC_MAX_BITMAP    (((QUIRC_MAX_GRID_SIZE * QUIRC_MAX_GRID_SIZE) + 7) / 8)
#define QUIRC_MAX_PAYLOAD   8896

/* QR-code ECC types. */
#define QUIRC_ECC_LEVEL_M   0
#define QUIRC_ECC_LEVEL_L   1
#define QUIRC_ECC_LEVEL_H   2
#define QUIRC_ECC_LEVEL_Q   3

/* QR-code data types. */
#define QUIRC_DATA_TYPE_NUMERIC 1
#define QUIRC_DATA_TYPE_ALPHA   2
#define QUIRC_DATA_TYPE_BYTE    4
#define QUIRC_DATA_TYPE_KANJI   8

/* Common character encodings */
#define QUIRC_ECI_ISO_8859_1    1
#define QUIRC_ECI_IBM437        2
#define QUIRC_ECI_ISO_8859_2    4
#define QUIRC_ECI_ISO_8859_3    5
#define QUIRC_ECI_ISO_8859_4    6
#define QUIRC_ECI_ISO_8859_5    7
#define QUIRC_ECI_ISO_8859_6    8
#define QUIRC_ECI_ISO_8859_7    9
#define QUIRC_ECI_ISO_8859_8    10
#define QUIRC_ECI_ISO_8859_9    11
#define QUIRC_ECI_WINDOWS_874   13
#define QUIRC_ECI_ISO_8859_13   15
#define QUIRC_ECI_ISO_8859_15   17
#define QUIRC_ECI_SHIFT_JIS     20
#define QUIRC_ECI_UTF_8         26

/* This structure is used to return information about detected QR codes
 * in the input image.
 */
struct quirc_code {
	/* The four corners of the QR-code, from top left, clockwise */
	struct quirc_point  corners[4];

	/* The number of cells across in the QR-code. The cell bitmap
	 * is a bitmask giving the actual values of cells. If the cell
	 * at (x, y) is black, then the following bit is set:
	 *
	 *      cell_bitmap[i >> 3] & (1 << (i & 7))
	 *
	 * where i = (y * size) + x.
	 */
	int                 size;
	uint8_t             cell_bitmap[QUIRC_MAX_BITMAP];
};

/* This structure holds the decoded QR-code data */
struct quirc_data {
	/* Various parameters of the QR-code. These can mostly be
	 * ignored if you only care about the data.
	 */
	int         version;
	int         ecc_level;
	int         mask;

	/* This field is the highest-valued data type found in the QR
	 * code.
	 */
	int         data_type;

	/* Data payload. For the Kanji datatype, payload is encoded as
	 * Shift-JIS. For all other datatypes, payload is ASCII text.
	 */
	uint8_t     payload[QUIRC_MAX_PAYLOAD];
	int         payload_len;

	/* ECI assignment number */
	uint32_t    eci;
};

/* Return the number of QR-codes identified in the last processed
 * image.
 */
int quirc_count(const struct quirc *q);

/* Extract the QR-code specified by the given index. */
void quirc_extract(const struct quirc *q, int index,
		   struct quirc_code *code);

/* Decode a QR-code, returning the payload data. */
quirc_decode_error_t quirc_decode(const struct quirc_code *code,
				  struct quirc_data *data);

/* Flip a QR-code according to optional mirror feature of ISO 18004:2015 */
void quirc_flip(struct quirc_code *code);

////////////////////////////////////////////////////////////////////////////////////////////////////
//////// "quirc_internal.h"
////////////////////////////////////////////////////////////////////////////////////////////////////

/* quirc -- QR-code recognition library
 * Copyright (C) 2010-2012 Daniel Beer <dlbeer@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#define QUIRC_ASSERT(a)	assert(a)

#define QUIRC_PIXEL_WHITE   0
#define QUIRC_PIXEL_BLACK   1
#define QUIRC_PIXEL_REGION  2

#ifndef QUIRC_MAX_REGIONS
#define QUIRC_MAX_REGIONS   254
#endif

#define QUIRC_MAX_CAPSTONES 32
#define QUIRC_MAX_GRIDS     (QUIRC_MAX_CAPSTONES * 2)

#define QUIRC_PERSPECTIVE_PARAMS    8

#if QUIRC_MAX_REGIONS < UINT8_MAX
#define QUIRC_PIXEL_ALIAS_IMAGE 1
typedef uint8_t quirc_pixel_t;
#elif QUIRC_MAX_REGIONS < UINT16_MAX
#define QUIRC_PIXEL_ALIAS_IMAGE 0
typedef uint16_t quirc_pixel_t;
#else
#error "QUIRC_MAX_REGIONS > 65534 is not supported"
#endif

#ifdef QUIRC_FLOAT_TYPE
/* Quirc uses double precision floating point internally by default.
 * On platforms with a single precision FPU but no double precision FPU,
 * this can be changed to float by defining QUIRC_FLOAT_TYPE.
 *
 * When setting QUIRC_FLOAT_TYPE to 'float', consider also defining QUIRC_USE_TGMATH.
 * This will use the type-generic math functions (tgmath.h, C99 or later) instead of the normal ones,
 * which will allow the compiler to use the correct overloaded functions for the type.
 */
typedef QUIRC_FLOAT_TYPE quirc_float_t;
#else
typedef double quirc_float_t;
#endif

struct quirc_region {
	struct quirc_point  seed;
	int                 count;
	int                 capstone;
};

struct quirc_capstone {
	int                 ring;
	int                 stone;

	struct quirc_point  corners[4];
	struct quirc_point  center;
	quirc_float_t		c[QUIRC_PERSPECTIVE_PARAMS];

	int                 qr_grid;
};

struct quirc_grid {
	/* Capstone indices */
	int                 caps[3];

	/* Alignment pattern region and corner */
	int                 align_region;
	struct quirc_point  align;

	/* Timing pattern endpoints */
	struct quirc_point  tpep[3];

	/* Grid size and perspective transform */
	int                 grid_size;
	quirc_float_t		c[QUIRC_PERSPECTIVE_PARAMS];
};

struct quirc_flood_fill_vars {
	int y;
	int right;
	int left_up;
	int left_down;
};

struct quirc {
	uint8_t                 *image;
	quirc_pixel_t           *pixels;
	int                     w;
	int                     h;

	int                     num_regions;
	struct quirc_region     regions[QUIRC_MAX_REGIONS];

	int                     num_capstones;
	struct quirc_capstone   capstones[QUIRC_MAX_CAPSTONES];

	int                     num_grids;
	struct quirc_grid       grids[QUIRC_MAX_GRIDS];

	size_t      		num_flood_fill_vars;
	struct quirc_flood_fill_vars *flood_fill_vars;
};

/************************************************************************
 * QR-code version information database
 */

#define QUIRC_MAX_VERSION   40
#define QUIRC_MAX_ALIGNMENT 7

struct quirc_rs_params {
	int             bs; /* Small block size */
	int             dw; /* Small data words */
	int		ns; /* Number of small blocks */
};

struct quirc_version_info {
	int                 data_bytes;
	int                 apat[QUIRC_MAX_ALIGNMENT];
	struct quirc_rs_params  ecc[4];
};

extern const struct quirc_version_info quirc_version_db[QUIRC_MAX_VERSION + 1];

////////////////////////////////////////////////////////////////////////////////////////////////////
//////// "version_db.c"
////////////////////////////////////////////////////////////////////////////////////////////////////

/* quirc -- QR-code recognition library
 * Copyright (C) 2010-2012 Daniel Beer <dlbeer@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

const struct quirc_version_info quirc_version_db[QUIRC_MAX_VERSION + 1] = {
	    {0},
	    { /* Version 1 */
		    .data_bytes = 26,
		    .apat = {0},
		    .ecc = {
			    {.bs = 26, .dw = 16, .ns = 1},
			    {.bs = 26, .dw = 19, .ns = 1},
			    {.bs = 26, .dw = 9, .ns = 1},
			    {.bs = 26, .dw = 13, .ns = 1}
		    }
	    },
	    { /* Version 2 */
		    .data_bytes = 44,
		    .apat = {6, 18, 0},
		    .ecc = {
			    {.bs = 44, .dw = 28, .ns = 1},
			    {.bs = 44, .dw = 34, .ns = 1},
			    {.bs = 44, .dw = 16, .ns = 1},
			    {.bs = 44, .dw = 22, .ns = 1}
		    }
	    },
	    { /* Version 3 */
		    .data_bytes = 70,
		    .apat = {6, 22, 0},
		    .ecc = {
			    {.bs = 70, .dw = 44, .ns = 1},
			    {.bs = 70, .dw = 55, .ns = 1},
			    {.bs = 35, .dw = 13, .ns = 2},
			    {.bs = 35, .dw = 17, .ns = 2}
		    }
	    },
	    { /* Version 4 */
		    .data_bytes = 100,
		    .apat = {6, 26, 0},
		    .ecc = {
			    {.bs = 50, .dw = 32, .ns = 2},
			    {.bs = 100, .dw = 80, .ns = 1},
			    {.bs = 25, .dw = 9, .ns = 4},
			    {.bs = 50, .dw = 24, .ns = 2}
		    }
	    },
	    { /* Version 5 */
		    .data_bytes = 134,
		    .apat = {6, 30, 0},
		    .ecc = {
			    {.bs = 67, .dw = 43, .ns = 2},
			    {.bs = 134, .dw = 108, .ns = 1},
			    {.bs = 33, .dw = 11, .ns = 2},
			    {.bs = 33, .dw = 15, .ns = 2}
		    }
	    },
	    { /* Version 6 */
		    .data_bytes = 172,
		    .apat = {6, 34, 0},
		    .ecc = {
			    {.bs = 43, .dw = 27, .ns = 4},
			    {.bs = 86, .dw = 68, .ns = 2},
			    {.bs = 43, .dw = 15, .ns = 4},
			    {.bs = 43, .dw = 19, .ns = 4}
		    }
	    },
	    { /* Version 7 */
		    .data_bytes = 196,
		    .apat = {6, 22, 38, 0},
		    .ecc = {
			    {.bs = 49, .dw = 31, .ns = 4},
			    {.bs = 98, .dw = 78, .ns = 2},
			    {.bs = 39, .dw = 13, .ns = 4},
			    {.bs = 32, .dw = 14, .ns = 2}
		    }
	    },
	    { /* Version 8 */
		    .data_bytes = 242,
		    .apat = {6, 24, 42, 0},
		    .ecc = {
			    {.bs = 60, .dw = 38, .ns = 2},
			    {.bs = 121, .dw = 97, .ns = 2},
			    {.bs = 40, .dw = 14, .ns = 4},
			    {.bs = 40, .dw = 18, .ns = 4}
		    }
	    },
	    { /* Version 9 */
		    .data_bytes = 292,
		    .apat = {6, 26, 46, 0},
		    .ecc = {
			    {.bs = 58, .dw = 36, .ns = 3},
			    {.bs = 146, .dw = 116, .ns = 2},
			    {.bs = 36, .dw = 12, .ns = 4},
			    {.bs = 36, .dw = 16, .ns = 4}
		    }
	    },
	    { /* Version 10 */
		    .data_bytes = 346,
		    .apat = {6, 28, 50, 0},
		    .ecc = {
			    {.bs = 69, .dw = 43, .ns = 4},
			    {.bs = 86, .dw = 68, .ns = 2},
			    {.bs = 43, .dw = 15, .ns = 6},
			    {.bs = 43, .dw = 19, .ns = 6}
		    }
	    },
	    { /* Version 11 */
		    .data_bytes = 404,
		    .apat = {6, 30, 54, 0},
		    .ecc = {
			    {.bs = 80, .dw = 50, .ns = 1},
			    {.bs = 101, .dw = 81, .ns = 4},
			    {.bs = 36, .dw = 12, .ns = 3},
			    {.bs = 50, .dw = 22, .ns = 4}
		    }
	    },
	    { /* Version 12 */
		    .data_bytes = 466,
		    .apat = {6, 32, 58, 0},
		    .ecc = {
			    {.bs = 58, .dw = 36, .ns = 6},
			    {.bs = 116, .dw = 92, .ns = 2},
			    {.bs = 42, .dw = 14, .ns = 7},
			    {.bs = 46, .dw = 20, .ns = 4}
		    }
	    },
	    { /* Version 13 */
		    .data_bytes = 532,
		    .apat = {6, 34, 62, 0},
		    .ecc = {
			    {.bs = 59, .dw = 37, .ns = 8},
			    {.bs = 133, .dw = 107, .ns = 4},
			    {.bs = 33, .dw = 11, .ns = 12},
			    {.bs = 44, .dw = 20, .ns = 8}
		    }
	    },
	    { /* Version 14 */
		    .data_bytes = 581,
		    .apat = {6, 26, 46, 66, 0},
		    .ecc = {
			    {.bs = 64, .dw = 40, .ns = 4},
			    {.bs = 145, .dw = 115, .ns = 3},
			    {.bs = 36, .dw = 12, .ns = 11},
			    {.bs = 36, .dw = 16, .ns = 11}
		    }
	    },
	    { /* Version 15 */
		    .data_bytes = 655,
		    .apat = {6, 26, 48, 70, 0},
		    .ecc = {
			    {.bs = 65, .dw = 41, .ns = 5},
			    {.bs = 109, .dw = 87, .ns = 5},
			    {.bs = 36, .dw = 12, .ns = 11},
			    {.bs = 54, .dw = 24, .ns = 5}
		    }
	    },
	    { /* Version 16 */
		    .data_bytes = 733,
		    .apat = {6, 26, 50, 74, 0},
		    .ecc = {
			    {.bs = 73, .dw = 45, .ns = 7},
			    {.bs = 122, .dw = 98, .ns = 5},
			    {.bs = 45, .dw = 15, .ns = 3},
			    {.bs = 43, .dw = 19, .ns = 15}
		    }
	    },
	    { /* Version 17 */
		    .data_bytes = 815,
		    .apat = {6, 30, 54, 78, 0},
		    .ecc = {
			    {.bs = 74, .dw = 46, .ns = 10},
			    {.bs = 135, .dw = 107, .ns = 1},
			    {.bs = 42, .dw = 14, .ns = 2},
			    {.bs = 50, .dw = 22, .ns = 1}
		    }
	    },
	    { /* Version 18 */
		    .data_bytes = 901,
		    .apat = {6, 30, 56, 82, 0},
		    .ecc = {
			    {.bs = 69, .dw = 43, .ns = 9},
			    {.bs = 150, .dw = 120, .ns = 5},
			    {.bs = 42, .dw = 14, .ns = 2},
			    {.bs = 50, .dw = 22, .ns = 17}
		    }
	    },
	    { /* Version 19 */
		    .data_bytes = 991,
		    .apat = {6, 30, 58, 86, 0},
		    .ecc = {
			    {.bs = 70, .dw = 44, .ns = 3},
			    {.bs = 141, .dw = 113, .ns = 3},
			    {.bs = 39, .dw = 13, .ns = 9},
			    {.bs = 47, .dw = 21, .ns = 17}
		    }
	    },
	    { /* Version 20 */
		    .data_bytes = 1085,
		    .apat = {6, 34, 62, 90, 0},
		    .ecc = {
			    {.bs = 67, .dw = 41, .ns = 3},
			    {.bs = 135, .dw = 107, .ns = 3},
			    {.bs = 43, .dw = 15, .ns = 15},
			    {.bs = 54, .dw = 24, .ns = 15}
		    }
	    },
	    { /* Version 21 */
		    .data_bytes = 1156,
		    .apat = {6, 28, 50, 72, 92, 0},
		    .ecc = {
			    {.bs = 68, .dw = 42, .ns = 17},
			    {.bs = 144, .dw = 116, .ns = 4},
			    {.bs = 46, .dw = 16, .ns = 19},
			    {.bs = 50, .dw = 22, .ns = 17}
		    }
	    },
	    { /* Version 22 */
		    .data_bytes = 1258,
		    .apat = {6, 26, 50, 74, 98, 0},
		    .ecc = {
			    {.bs = 74, .dw = 46, .ns = 17},
			    {.bs = 139, .dw = 111, .ns = 2},
			    {.bs = 37, .dw = 13, .ns = 34},
			    {.bs = 54, .dw = 24, .ns = 7}
		    }
	    },
	    { /* Version 23 */
		    .data_bytes = 1364,
		    .apat = {6, 30, 54, 78, 102, 0},
		    .ecc = {
			    {.bs = 75, .dw = 47, .ns = 4},
			    {.bs = 151, .dw = 121, .ns = 4},
			    {.bs = 45, .dw = 15, .ns = 16},
			    {.bs = 54, .dw = 24, .ns = 11}
		    }
	    },
	    { /* Version 24 */
		    .data_bytes = 1474,
		    .apat = {6, 28, 54, 80, 106, 0},
		    .ecc = {
			    {.bs = 73, .dw = 45, .ns = 6},
			    {.bs = 147, .dw = 117, .ns = 6},
			    {.bs = 46, .dw = 16, .ns = 30},
			    {.bs = 54, .dw = 24, .ns = 11}
		    }
	    },
	    { /* Version 25 */
		    .data_bytes = 1588,
		    .apat = {6, 32, 58, 84, 110, 0},
		    .ecc = {
			    {.bs = 75, .dw = 47, .ns = 8},
			    {.bs = 132, .dw = 106, .ns = 8},
			    {.bs = 45, .dw = 15, .ns = 22},
			    {.bs = 54, .dw = 24, .ns = 7}
		    }
	    },
	    { /* Version 26 */
		    .data_bytes = 1706,
		    .apat = {6, 30, 58, 86, 114, 0},
		    .ecc = {
			    {.bs = 74, .dw = 46, .ns = 19},
			    {.bs = 142, .dw = 114, .ns = 10},
			    {.bs = 46, .dw = 16, .ns = 33},
			    {.bs = 50, .dw = 22, .ns = 28}
		    }
	    },
	    { /* Version 27 */
		    .data_bytes = 1828,
		    .apat = {6, 34, 62, 90, 118, 0},
		    .ecc = {
			    {.bs = 73, .dw = 45, .ns = 22},
			    {.bs = 152, .dw = 122, .ns = 8},
			    {.bs = 45, .dw = 15, .ns = 12},
			    {.bs = 53, .dw = 23, .ns = 8}
		    }
	    },
	    { /* Version 28 */
		    .data_bytes = 1921,
		    .apat = {6, 26, 50, 74, 98, 122, 0},
		    .ecc = {
			    {.bs = 73, .dw = 45, .ns = 3},
			    {.bs = 147, .dw = 117, .ns = 3},
			    {.bs = 45, .dw = 15, .ns = 11},
			    {.bs = 54, .dw = 24, .ns = 4}
		    }
	    },
	    { /* Version 29 */
		    .data_bytes = 2051,
		    .apat = {6, 30, 54, 78, 102, 126, 0},
		    .ecc = {
			    {.bs = 73, .dw = 45, .ns = 21},
			    {.bs = 146, .dw = 116, .ns = 7},
			    {.bs = 45, .dw = 15, .ns = 19},
			    {.bs = 53, .dw = 23, .ns = 1}
		    }
	    },
	    { /* Version 30 */
		    .data_bytes = 2185,
		    .apat = {6, 26, 52, 78, 104, 130, 0},
		    .ecc = {
			    {.bs = 75, .dw = 47, .ns = 19},
			    {.bs = 145, .dw = 115, .ns = 5},
			    {.bs = 45, .dw = 15, .ns = 23},
			    {.bs = 54, .dw = 24, .ns = 15}
		    }
	    },
	    { /* Version 31 */
		    .data_bytes = 2323,
		    .apat = {6, 30, 56, 82, 108, 134, 0},
		    .ecc = {
			    {.bs = 74, .dw = 46, .ns = 2},
			    {.bs = 145, .dw = 115, .ns = 13},
			    {.bs = 45, .dw = 15, .ns = 23},
			    {.bs = 54, .dw = 24, .ns = 42}
		    }
	    },
	    { /* Version 32 */
		    .data_bytes = 2465,
		    .apat = {6, 34, 60, 86, 112, 138, 0},
		    .ecc = {
			    {.bs = 74, .dw = 46, .ns = 10},
			    {.bs = 145, .dw = 115, .ns = 17},
			    {.bs = 45, .dw = 15, .ns = 19},
			    {.bs = 54, .dw = 24, .ns = 10}
		    }
	    },
	    { /* Version 33 */
		    .data_bytes = 2611,
		    .apat = {6, 30, 58, 86, 114, 142, 0},
		    .ecc = {
			    {.bs = 74, .dw = 46, .ns = 14},
			    {.bs = 145, .dw = 115, .ns = 17},
			    {.bs = 45, .dw = 15, .ns = 11},
			    {.bs = 54, .dw = 24, .ns = 29}
		    }
	    },
	    { /* Version 34 */
		    .data_bytes = 2761,
		    .apat = {6, 34, 62, 90, 118, 146, 0},
		    .ecc = {
			    {.bs = 74, .dw = 46, .ns = 14},
			    {.bs = 145, .dw = 115, .ns = 13},
			    {.bs = 46, .dw = 16, .ns = 59},
			    {.bs = 54, .dw = 24, .ns = 44}
		    }
	    },
	    { /* Version 35 */
		    .data_bytes = 2876,
		    .apat = {6, 30, 54, 78, 102, 126, 150},
		    .ecc = {
			    {.bs = 75, .dw = 47, .ns = 12},
			    {.bs = 151, .dw = 121, .ns = 12},
			    {.bs = 45, .dw = 15, .ns = 22},
			    {.bs = 54, .dw = 24, .ns = 39}
		    }
	    },
	    { /* Version 36 */
		    .data_bytes = 3034,
		    .apat = {6, 24, 50, 76, 102, 128, 154},
		    .ecc = {
			    {.bs = 75, .dw = 47, .ns = 6},
			    {.bs = 151, .dw = 121, .ns = 6},
			    {.bs = 45, .dw = 15, .ns = 2},
			    {.bs = 54, .dw = 24, .ns = 46}
		    }
	    },
	    { /* Version 37 */
		    .data_bytes = 3196,
		    .apat = {6, 28, 54, 80, 106, 132, 158},
		    .ecc = {
			    {.bs = 74, .dw = 46, .ns = 29},
			    {.bs = 152, .dw = 122, .ns = 17},
			    {.bs = 45, .dw = 15, .ns = 24},
			    {.bs = 54, .dw = 24, .ns = 49}
		    }
	    },
	    { /* Version 38 */
		    .data_bytes = 3362,
		    .apat = {6, 32, 58, 84, 110, 136, 162},
		    .ecc = {
			    {.bs = 74, .dw = 46, .ns = 13},
			    {.bs = 152, .dw = 122, .ns = 4},
			    {.bs = 45, .dw = 15, .ns = 42},
			    {.bs = 54, .dw = 24, .ns = 48}
		    }
	    },
	    { /* Version 39 */
		    .data_bytes = 3532,
		    .apat = {6, 26, 54, 82, 110, 138, 166},
		    .ecc = {
			    {.bs = 75, .dw = 47, .ns = 40},
			    {.bs = 147, .dw = 117, .ns = 20},
			    {.bs = 45, .dw = 15, .ns = 10},
			    {.bs = 54, .dw = 24, .ns = 43}
		    }
	    },
	    { /* Version 40 */
		    .data_bytes = 3706,
		    .apat = {6, 30, 58, 86, 114, 142, 170},
		    .ecc = {
			    {.bs = 75, .dw = 47, .ns = 18},
			    {.bs = 148, .dw = 118, .ns = 19},
			    {.bs = 45, .dw = 15, .ns = 20},
			    {.bs = 54, .dw = 24, .ns = 34}
		    }
	    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//////// "indentify.c"
////////////////////////////////////////////////////////////////////////////////////////////////////

/* quirc - QR-code recognition library
 * Copyright (C) 2010-2012 Daniel Beer <dlbeer@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/************************************************************************
 * Linear algebra routines
 */

static int line_intersect(const struct quirc_point *p0,
			  const struct quirc_point *p1,
			  const struct quirc_point *q0,
			  const struct quirc_point *q1,
			  struct quirc_point *r)
{
	/* (a, b) is perpendicular to line p */
	int a = -(p1->y - p0->y);
	int b = p1->x - p0->x;

	/* (c, d) is perpendicular to line q */
	int c = -(q1->y - q0->y);
	int d = q1->x - q0->x;

	/* e and f are dot products of the respective vectors with p and q */
	int e = a * p1->x + b * p1->y;
	int f = c * q1->x + d * q1->y;

	/* Now we need to solve:
	 *     [a b] [rx]   [e]
	 *     [c d] [ry] = [f]
	 *
	 * We do this by inverting the matrix and applying it to (e, f):
	 *       [ d -b] [e]   [rx]
	 * 1/det [-c  a] [f] = [ry]
	 */
	int det = (a * d) - (b * c);

	if (!det)
		return 0;

	r->x = (d * e - b * f) / det;
	r->y = (-c * e + a * f) / det;

	return 1;
}

static void perspective_setup(quirc_float_t *c,
			      const struct quirc_point *rect,
			      quirc_float_t w, quirc_float_t h)
{
	quirc_float_t x0 = rect[0].x;
	quirc_float_t y0 = rect[0].y;
	quirc_float_t x1 = rect[1].x;
	quirc_float_t y1 = rect[1].y;
	quirc_float_t x2 = rect[2].x;
	quirc_float_t y2 = rect[2].y;
	quirc_float_t x3 = rect[3].x;
	quirc_float_t y3 = rect[3].y;

	quirc_float_t wden = w * (x2*y3 - x3*y2 + (x3-x2)*y1 + x1*(y2-y3));
	quirc_float_t hden = h * (x2*y3 + x1*(y2-y3) - x3*y2 + (x3-x2)*y1);

	c[0] = (x1*(x2*y3-x3*y2) + x0*(-x2*y3+x3*y2+(x2-x3)*y1) +
		x1*(x3-x2)*y0) / wden;
	c[1] = -(x0*(x2*y3+x1*(y2-y3)-x2*y1) - x1*x3*y2 + x2*x3*y1
		 + (x1*x3-x2*x3)*y0) / hden;
	c[2] = x0;
	c[3] = (y0*(x1*(y3-y2)-x2*y3+x3*y2) + y1*(x2*y3-x3*y2) +
		x0*y1*(y2-y3)) / wden;
	c[4] = (x0*(y1*y3-y2*y3) + x1*y2*y3 - x2*y1*y3 +
		y0*(x3*y2-x1*y2+(x2-x3)*y1)) / hden;
	c[5] = y0;
	c[6] = (x1*(y3-y2) + x0*(y2-y3) + (x2-x3)*y1 + (x3-x2)*y0) / wden;
	c[7] = (-x2*y3 + x1*y3 + x3*y2 + x0*(y1-y2) - x3*y1 + (x2-x1)*y0) /
		hden;
}

static void perspective_map(const quirc_float_t *c,
			    quirc_float_t u, quirc_float_t v, struct quirc_point *ret)
{
	quirc_float_t den = c[6]*u + c[7]*v + 1.0;
	quirc_float_t x = (c[0]*u + c[1]*v + c[2]) / den;
	quirc_float_t y = (c[3]*u + c[4]*v + c[5]) / den;

	ret->x = (int) rint(x);
	ret->y = (int) rint(y);
}

static void perspective_unmap(const quirc_float_t *c,
			      const struct quirc_point *in,
			      quirc_float_t *u, quirc_float_t *v)
{
	quirc_float_t x = in->x;
	quirc_float_t y = in->y;
	quirc_float_t den = -c[0]*c[7]*y + c[1]*c[6]*y + (c[3]*c[7]-c[4]*c[6])*x +
		c[0]*c[4] - c[1]*c[3];

	*u = -(c[1]*(y-c[5]) - c[2]*c[7]*y + (c[5]*c[7]-c[4])*x + c[2]*c[4]) /
		den;
	*v = (c[0]*(y-c[5]) - c[2]*c[6]*y + (c[5]*c[6]-c[3])*x + c[2]*c[3]) /
		den;
}

/************************************************************************
 * Span-based floodfill routine
 */

typedef void (*span_func_t)(void *user_data, int y, int left, int right);

static void flood_fill_line(struct quirc *q, int x, int y,
			    int from, int to,
			    span_func_t func, void *user_data,
			    int *leftp, int *rightp)
{
	quirc_pixel_t *row;
	int left;
	int right;
	int i;

	row = q->pixels + y * q->w;
	QUIRC_ASSERT(row[x] == from);

	left = x;
	right = x;

	while (left > 0 && row[left - 1] == from)
		left--;

	while (right < q->w - 1 && row[right + 1] == from)
		right++;

	/* Fill the extent */
	for (i = left; i <= right; i++)
		row[i] = to;

	/* Return the processed range */
	*leftp = left;
	*rightp = right;

	if (func)
		func(user_data, y, left, right);
}

static struct quirc_flood_fill_vars *flood_fill_call_next(
			struct quirc *q,
			quirc_pixel_t *row,
			int from, int to,
			span_func_t func, void *user_data,
			struct quirc_flood_fill_vars *vars,
			int direction)
{
	int *leftp;

	if (direction < 0) {
		leftp = &vars->left_up;
	} else {
		leftp = &vars->left_down;
	}

	while (*leftp <= vars->right) {
		if (row[*leftp] == from) {
			struct quirc_flood_fill_vars *next_vars;
			int next_left;

			/* Set up the next context */
			next_vars = vars + 1;
			next_vars->y = vars->y + direction;

			/* Fill the extent */
			flood_fill_line(q,
					*leftp,
					next_vars->y,
					from, to,
					func, user_data,
					&next_left,
					&next_vars->right);
			next_vars->left_down = next_left;
			next_vars->left_up = next_left;

			return next_vars;
		}
		(*leftp)++;
	}
	return NULL;
}

static void flood_fill_seed(struct quirc *q,
			    int x0, int y0,
			    int from, int to,
			    span_func_t func, void *user_data)
{
	struct quirc_flood_fill_vars *const stack = q->flood_fill_vars;
	const size_t stack_size = q->num_flood_fill_vars;
	const struct quirc_flood_fill_vars *const last_vars =
	    &stack[stack_size - 1];

	QUIRC_ASSERT(from != to);
	QUIRC_ASSERT(q->pixels[y0 * q->w + x0] == from);

	struct quirc_flood_fill_vars *next_vars;
	int next_left;

	/* Set up the first context  */
	next_vars = stack;
	next_vars->y = y0;

	/* Fill the extent */
	flood_fill_line(q, x0, next_vars->y, from, to,
			func, user_data,
			&next_left, &next_vars->right);
	next_vars->left_down = next_left;
	next_vars->left_up = next_left;

	while (true) {
		struct quirc_flood_fill_vars * const vars = next_vars;
		quirc_pixel_t *row;

		if (vars == last_vars) {
			/*
			 * "Stack overflow".
			 * Just stop and return.
			 * This can be caused by very complex shapes in
			 * the image, which is not likely a part of
			 * a valid QR code anyway.
			 */
			break;
		}

		/* Seed new flood-fills */
		if (vars->y > 0) {
			row = q->pixels + (vars->y - 1) * q->w;

			next_vars = flood_fill_call_next(q, row,
							 from, to,
							 func, user_data,
							 vars, -1);
			if (next_vars != NULL) {
				continue;
			}
		}

		if (vars->y < q->h - 1) {
			row = q->pixels + (vars->y + 1) * q->w;

			next_vars = flood_fill_call_next(q, row,
							 from, to,
							 func, user_data,
							 vars, 1);
			if (next_vars != NULL) {
				continue;
			}
		}

		if (vars > stack) {
			/* Restore the previous context */
			next_vars = vars - 1;
			continue;
		}

		/* We've done. */
		break;
	}
}

/************************************************************************
 * Adaptive thresholding
 */

static uint8_t otsu(const struct quirc *q)
{
	unsigned int numPixels = q->w * q->h;

	// Calculate histogram
	unsigned int histogram[UINT8_MAX + 1];
	(void)memset(histogram, 0, sizeof(histogram));
	uint8_t* ptr = q->image;
	unsigned int length = numPixels;
	while (length--) {
		uint8_t value = *ptr++;
		histogram[value]++;
	}

	// Calculate weighted sum of histogram values
	quirc_float_t sum = 0;
	unsigned int i = 0;
	for (i = 0; i <= UINT8_MAX; ++i) {
		sum += i * histogram[i];
	}

	// Compute threshold
	quirc_float_t sumB = 0;
	unsigned int q1 = 0;
	quirc_float_t max = 0;
	uint8_t threshold = 0;
	for (i = 0; i <= UINT8_MAX; ++i) {
		// Weighted background
		q1 += histogram[i];
		if (q1 == 0)
			continue;

		// Weighted foreground
		const unsigned int q2 = numPixels - q1;
		if (q2 == 0)
			break;

		sumB += i * histogram[i];
		const quirc_float_t m1 = sumB / q1;
		const quirc_float_t m2 = (sum - sumB) / q2;
		const quirc_float_t m1m2 = m1 - m2;
		const quirc_float_t variance = m1m2 * m1m2 * q1 * q2;
		if (variance >= max) {
			threshold = i;
			max = variance;
		}
	}

	return threshold;
}

static void area_count(void *user_data, int y, int left, int right)
{
	((struct quirc_region *)user_data)->count += right - left + 1;
}

static int region_code(struct quirc *q, int x, int y)
{
	int pixel;
	struct quirc_region *box;
	int region;

	if (x < 0 || y < 0 || x >= q->w || y >= q->h)
		return -1;

	pixel = q->pixels[y * q->w + x];

	if (pixel >= QUIRC_PIXEL_REGION)
		return pixel;

	if (pixel == QUIRC_PIXEL_WHITE)
		return -1;

	if (q->num_regions >= QUIRC_MAX_REGIONS)
		return -1;

	region = q->num_regions;
	box = &q->regions[q->num_regions++];

	memset(box, 0, sizeof(*box));

	box->seed.x = x;
	box->seed.y = y;
	box->capstone = -1;

	flood_fill_seed(q, x, y, pixel, region, area_count, box);

	return region;
}

struct polygon_score_data {
	struct quirc_point  ref;

	int                 scores[4];
	struct quirc_point  *corners;
};

static void find_one_corner(void *user_data, int y, int left, int right)
{
	struct polygon_score_data *psd =
		(struct polygon_score_data *)user_data;
	int xs[2] = {left, right};
	int dy = y - psd->ref.y;
	int i;

	for (i = 0; i < 2; i++) {
		int dx = xs[i] - psd->ref.x;
		int d = dx * dx + dy * dy;

		if (d > psd->scores[0]) {
			psd->scores[0] = d;
			psd->corners[0].x = xs[i];
			psd->corners[0].y = y;
		}
	}
}

static void find_other_corners(void *user_data, int y, int left, int right)
{
	struct polygon_score_data *psd =
		(struct polygon_score_data *)user_data;
	int xs[2] = {left, right};
	int i;

	for (i = 0; i < 2; i++) {
		int up = xs[i] * psd->ref.x + y * psd->ref.y;
		int right = xs[i] * -psd->ref.y + y * psd->ref.x;
		int scores[4] = {up, right, -up, -right};
		int j;

		for (j = 0; j < 4; j++) {
			if (scores[j] > psd->scores[j]) {
				psd->scores[j] = scores[j];
				psd->corners[j].x = xs[i];
				psd->corners[j].y = y;
			}
		}
	}
}

static void find_region_corners(struct quirc *q,
				int rcode, const struct quirc_point *ref,
				struct quirc_point *corners)
{
	struct quirc_region *region = &q->regions[rcode];
	struct polygon_score_data psd;
	int i;

	memset(&psd, 0, sizeof(psd));
	psd.corners = corners;

	memcpy(&psd.ref, ref, sizeof(psd.ref));
	psd.scores[0] = -1;
	flood_fill_seed(q, region->seed.x, region->seed.y,
			rcode, QUIRC_PIXEL_BLACK,
			find_one_corner, &psd);

	psd.ref.x = psd.corners[0].x - psd.ref.x;
	psd.ref.y = psd.corners[0].y - psd.ref.y;

	for (i = 0; i < 4; i++)
		memcpy(&psd.corners[i], &region->seed,
		       sizeof(psd.corners[i]));

	i = region->seed.x * psd.ref.x + region->seed.y * psd.ref.y;
	psd.scores[0] = i;
	psd.scores[2] = -i;
	i = region->seed.x * -psd.ref.y + region->seed.y * psd.ref.x;
	psd.scores[1] = i;
	psd.scores[3] = -i;

	flood_fill_seed(q, region->seed.x, region->seed.y,
			QUIRC_PIXEL_BLACK, rcode,
			find_other_corners, &psd);
}

static void record_capstone(struct quirc *q, int ring, int stone)
{
	struct quirc_region *stone_reg = &q->regions[stone];
	struct quirc_region *ring_reg = &q->regions[ring];
	struct quirc_capstone *capstone;
	int cs_index;

	if (q->num_capstones >= QUIRC_MAX_CAPSTONES)
		return;

	cs_index = q->num_capstones;
	capstone = &q->capstones[q->num_capstones++];

	memset(capstone, 0, sizeof(*capstone));

	capstone->qr_grid = -1;
	capstone->ring = ring;
	capstone->stone = stone;
	stone_reg->capstone = cs_index;
	ring_reg->capstone = cs_index;

	/* Find the corners of the ring */
	find_region_corners(q, ring, &stone_reg->seed, capstone->corners);

	/* Set up the perspective transform and find the center */
	perspective_setup(capstone->c, capstone->corners, 7.0, 7.0);
	perspective_map(capstone->c, 3.5, 3.5, &capstone->center);
}

static void test_capstone(struct quirc *q, unsigned int x, unsigned int y,
			  unsigned int *pb)
{
	int ring_right = region_code(q, x - pb[4], y);
	int stone = region_code(q, x - pb[4] - pb[3] - pb[2], y);
	int ring_left = region_code(q, x - pb[4] - pb[3] -
				    pb[2] - pb[1] - pb[0],
				    y);
	struct quirc_region *stone_reg;
	struct quirc_region *ring_reg;
	unsigned int ratio;

	if (ring_left < 0 || ring_right < 0 || stone < 0)
		return;

	/* Left and ring of ring should be connected */
	if (ring_left != ring_right)
		return;

	/* Ring should be disconnected from stone */
	if (ring_left == stone)
		return;

	stone_reg = &q->regions[stone];
	ring_reg = &q->regions[ring_left];

	/* Already detected */
	if (stone_reg->capstone >= 0 || ring_reg->capstone >= 0)
		return;

	/* Ratio should ideally be 37.5 */
	ratio = stone_reg->count * 100 / ring_reg->count;
	if (ratio < 10 || ratio > 70)
		return;

	record_capstone(q, ring_left, stone);
}

static void finder_scan(struct quirc *q, unsigned int y)
{
	quirc_pixel_t *row = q->pixels + y * q->w;
	unsigned int x;
	int last_color = 0;
	unsigned int run_length = 0;
	unsigned int run_count = 0;
	unsigned int pb[5];

	memset(pb, 0, sizeof(pb));
	for (x = 0; x < q->w; x++) {
		int color = row[x] ? 1 : 0;

		if (x && color != last_color) {
			memmove(pb, pb + 1, sizeof(pb[0]) * 4);
			pb[4] = run_length;
			run_length = 0;
			run_count++;

			if (!color && run_count >= 5) {
				const int scale = 16;
				static const unsigned int check[5] = {1, 1, 3, 1, 1};
				unsigned int avg, err;
				unsigned int i;
				int ok = 1;

				avg = (pb[0] + pb[1] + pb[3] + pb[4]) * scale / 4;
				err = avg * 3 / 4;

				for (i = 0; i < 5; i++)
					if (pb[i] * scale < check[i] * avg - err ||
					    pb[i] * scale > check[i] * avg + err)
						ok = 0;

				if (ok)
					test_capstone(q, x, y, pb);
			}
		}

		run_length++;
		last_color = color;
	}
}

static void find_alignment_pattern(struct quirc *q, int index)
{
	struct quirc_grid *qr = &q->grids[index];
	struct quirc_capstone *c0 = &q->capstones[qr->caps[0]];
	struct quirc_capstone *c2 = &q->capstones[qr->caps[2]];
	struct quirc_point a;
	struct quirc_point b;
	struct quirc_point c;
	int size_estimate;
	int step_size = 1;
	int dir = 0;
	quirc_float_t u, v;

	/* Grab our previous estimate of the alignment pattern corner */
	memcpy(&b, &qr->align, sizeof(b));

	/* Guess another two corners of the alignment pattern so that we
	 * can estimate its size.
	 */
	perspective_unmap(c0->c, &b, &u, &v);
	perspective_map(c0->c, u, v + 1.0, &a);
	perspective_unmap(c2->c, &b, &u, &v);
	perspective_map(c2->c, u + 1.0, v, &c);

	size_estimate = abs((a.x - b.x) * -(c.y - b.y) +
			    (a.y - b.y) * (c.x - b.x));

	/* Spiral outwards from the estimate point until we find something
	 * roughly the right size. Don't look too far from the estimate
	 * point.
	 */
	while (step_size * step_size < size_estimate * 100) {
		static const int dx_map[] = {1, 0, -1, 0};
		static const int dy_map[] = {0, -1, 0, 1};
		int i;

		for (i = 0; i < step_size; i++) {
			int code = region_code(q, b.x, b.y);

			if (code >= 0) {
				struct quirc_region *reg = &q->regions[code];

				if (reg->count >= size_estimate / 2 &&
				    reg->count <= size_estimate * 2) {
					qr->align_region = code;
					return;
				}
			}

			b.x += dx_map[dir];
			b.y += dy_map[dir];
		}

		dir = (dir + 1) % 4;
		if (!(dir & 1))
			step_size++;
	}
}

static void find_leftmost_to_line(void *user_data, int y, int left, int right)
{
	struct polygon_score_data *psd =
		(struct polygon_score_data *)user_data;
	int xs[2] = {left, right};
	int i;

	for (i = 0; i < 2; i++) {
		int d = -psd->ref.y * xs[i] + psd->ref.x * y;

		if (d < psd->scores[0]) {
			psd->scores[0] = d;
			psd->corners[0].x = xs[i];
			psd->corners[0].y = y;
		}
	}
}

static quirc_float_t length(struct quirc_point a, struct quirc_point b)
{
	quirc_float_t x = abs(a.x - b.x) + 1;
	quirc_float_t y = abs(a.y - b.y) + 1;
	return sqrt(x * x +  y * y);
}

/* Estimate grid size by determing distance between capstones
 */
static void measure_grid_size(struct quirc *q, int index)
{
	struct quirc_grid *qr = &q->grids[index];

	struct quirc_capstone *a = &(q->capstones[qr->caps[0]]);
	struct quirc_capstone *b = &(q->capstones[qr->caps[1]]);
	struct quirc_capstone *c = &(q->capstones[qr->caps[2]]);

	quirc_float_t ab = length(b->corners[0], a->corners[3]);
	quirc_float_t capstone_ab_size = (length(b->corners[0], b->corners[3]) + length(a->corners[0], a->corners[3]))/2.0;
	quirc_float_t ver_grid = 7.0 * ab / capstone_ab_size;

	quirc_float_t bc = length(b->corners[0], c->corners[1]);
	quirc_float_t capstone_bc_size = (length(b->corners[0], b->corners[1]) + length(c->corners[0], c->corners[1]))/2.0;
	quirc_float_t hor_grid = 7.0 * bc / capstone_bc_size;
	
	quirc_float_t grid_size_estimate = (ver_grid + hor_grid) / 2;

	int ver = (int)((grid_size_estimate - 17.0 + 2.0) / 4.0);
	
	qr->grid_size =  4*ver + 17;
}

/* Read a cell from a grid using the currently set perspective
 * transform. Returns +/- 1 for black/white, 0 for cells which are
 * out of image bounds.
 */
static int read_cell(const struct quirc *q, int index, int x, int y)
{
	const struct quirc_grid *qr = &q->grids[index];
	struct quirc_point p;

	perspective_map(qr->c, x + 0.5, y + 0.5, &p);
	if (p.y < 0 || p.y >= q->h || p.x < 0 || p.x >= q->w)
		return 0;

	return q->pixels[p.y * q->w + p.x] ? 1 : -1;
}

static int fitness_cell(const struct quirc *q, int index, int x, int y)
{
	const struct quirc_grid *qr = &q->grids[index];
	int score = 0;
	int u, v;

	for (v = 0; v < 3; v++)
		for (u = 0; u < 3; u++) {
			static const quirc_float_t offsets[] = {0.3, 0.5, 0.7};
			struct quirc_point p;

			perspective_map(qr->c, x + offsets[u],
					       y + offsets[v], &p);
			if (p.y < 0 || p.y >= q->h || p.x < 0 || p.x >= q->w)
				continue;

			if (q->pixels[p.y * q->w + p.x])
				score++;
			else
				score--;
		}

	return score;
}

static int fitness_ring(const struct quirc *q, int index, int cx, int cy,
			int radius)
{
	int i;
	int score = 0;

	for (i = 0; i < radius * 2; i++) {
		score += fitness_cell(q, index, cx - radius + i, cy - radius);
		score += fitness_cell(q, index, cx - radius, cy + radius - i);
		score += fitness_cell(q, index, cx + radius, cy - radius + i);
		score += fitness_cell(q, index, cx + radius - i, cy + radius);
	}

	return score;
}

static int fitness_apat(const struct quirc *q, int index, int cx, int cy)
{
	return fitness_cell(q, index, cx, cy) -
		fitness_ring(q, index, cx, cy, 1) +
		fitness_ring(q, index, cx, cy, 2);
}

static int fitness_capstone(const struct quirc *q, int index, int x, int y)
{
	x += 3;
	y += 3;

	return fitness_cell(q, index, x, y) +
		fitness_ring(q, index, x, y, 1) -
		fitness_ring(q, index, x, y, 2) +
		fitness_ring(q, index, x, y, 3);
}

/* Compute a fitness score for the currently configured perspective
 * transform, using the features we expect to find by scanning the
 * grid.
 */
static int fitness_all(const struct quirc *q, int index)
{
	const struct quirc_grid *qr = &q->grids[index];
	int version = (qr->grid_size - 17) / 4;
	const struct quirc_version_info *info = &quirc_version_db[version];
	int score = 0;
	int i, j;
	int ap_count;

	/* Check the timing pattern */
	for (i = 0; i < qr->grid_size - 14; i++) {
		int expect = (i & 1) ? 1 : -1;

		score += fitness_cell(q, index, i + 7, 6) * expect;
		score += fitness_cell(q, index, 6, i + 7) * expect;
	}

	/* Check capstones */
	score += fitness_capstone(q, index, 0, 0);
	score += fitness_capstone(q, index, qr->grid_size - 7, 0);
	score += fitness_capstone(q, index, 0, qr->grid_size - 7);

	if (version < 0 || version > QUIRC_MAX_VERSION)
		return score;

	/* Check alignment patterns */
	ap_count = 0;
	while ((ap_count < QUIRC_MAX_ALIGNMENT) && info->apat[ap_count])
		ap_count++;

	for (i = 1; i + 1 < ap_count; i++) {
		score += fitness_apat(q, index, 6, info->apat[i]);
		score += fitness_apat(q, index, info->apat[i], 6);
	}

	for (i = 1; i < ap_count; i++)
		for (j = 1; j < ap_count; j++)
			score += fitness_apat(q, index,
					info->apat[i], info->apat[j]);

	return score;
}

static void jiggle_perspective(struct quirc *q, int index)
{
	struct quirc_grid *qr = &q->grids[index];
	int best = fitness_all(q, index);
	int pass;
	quirc_float_t adjustments[8];
	int i;

	for (i = 0; i < 8; i++)
		adjustments[i] = qr->c[i] * 0.02;

	for (pass = 0; pass < 5; pass++) {
		for (i = 0; i < 16; i++) {
			int j = i >> 1;
			int test;
			quirc_float_t old = qr->c[j];
			quirc_float_t step = adjustments[j];
			quirc_float_t new;

			if (i & 1)
				new = old + step;
			else
				new = old - step;

			qr->c[j] = new;
			test = fitness_all(q, index);

			if (test > best)
				best = test;
			else
				qr->c[j] = old;
		}

		for (i = 0; i < 8; i++)
			adjustments[i] *= 0.5;
	}
}

/* Once the capstones are in place and an alignment point has been
 * chosen, we call this function to set up a grid-reading perspective
 * transform.
 */
static void setup_qr_perspective(struct quirc *q, int index)
{
	struct quirc_grid *qr = &q->grids[index];
	struct quirc_point rect[4];

	/* Set up the perspective map for reading the grid */
	memcpy(&rect[0], &q->capstones[qr->caps[1]].corners[0],
	       sizeof(rect[0]));
	memcpy(&rect[1], &q->capstones[qr->caps[2]].corners[0],
	       sizeof(rect[0]));
	memcpy(&rect[2], &qr->align, sizeof(rect[0]));
	memcpy(&rect[3], &q->capstones[qr->caps[0]].corners[0],
	       sizeof(rect[0]));
	perspective_setup(qr->c, rect, qr->grid_size - 7, qr->grid_size - 7);

	jiggle_perspective(q, index);
}

/* Rotate the capstone with so that corner 0 is the leftmost with respect
 * to the given reference line.
 */
static void rotate_capstone(struct quirc_capstone *cap,
			    const struct quirc_point *h0,
			    const struct quirc_point *hd)
{
	struct quirc_point copy[4];
	int j;
	int best = 0;
	int best_score = INT_MAX;

	for (j = 0; j < 4; j++) {
		struct quirc_point *p = &cap->corners[j];
		int score = (p->x - h0->x) * -hd->y +
			(p->y - h0->y) * hd->x;

		if (!j || score < best_score) {
			best = j;
			best_score = score;
		}
	}

	/* Rotate the capstone */
	for (j = 0; j < 4; j++)
		memcpy(&copy[j], &cap->corners[(j + best) % 4],
		       sizeof(copy[j]));
	memcpy(cap->corners, copy, sizeof(cap->corners));
	perspective_setup(cap->c, cap->corners, 7.0, 7.0);
}

static void record_qr_grid(struct quirc *q, int a, int b, int c)
{
	struct quirc_point h0, hd;
	int i;
	int qr_index;
	struct quirc_grid *qr;

	if (q->num_grids >= QUIRC_MAX_GRIDS)
		return;

	/* Construct the hypotenuse line from A to C. B should be to
	 * the left of this line.
	 */
	memcpy(&h0, &q->capstones[a].center, sizeof(h0));
	hd.x = q->capstones[c].center.x - q->capstones[a].center.x;
	hd.y = q->capstones[c].center.y - q->capstones[a].center.y;

	/* Make sure A-B-C is clockwise */
	if ((q->capstones[b].center.x - h0.x) * -hd.y +
	    (q->capstones[b].center.y - h0.y) * hd.x > 0) {
		int swap = a;

		a = c;
		c = swap;
		hd.x = -hd.x;
		hd.y = -hd.y;
	}

	/* Record the grid and its components */
	qr_index = q->num_grids;
	qr = &q->grids[q->num_grids++];

	memset(qr, 0, sizeof(*qr));
	qr->caps[0] = a;
	qr->caps[1] = b;
	qr->caps[2] = c;
	qr->align_region = -1;

	/* Rotate each capstone so that corner 0 is top-left with respect
	 * to the grid.
	 */
	for (i = 0; i < 3; i++) {
		struct quirc_capstone *cap = &q->capstones[qr->caps[i]];

		rotate_capstone(cap, &h0, &hd);
		cap->qr_grid = qr_index;
	}

	/* Check the timing pattern by measuring grid size. This doesn't require a perspective
	 * transform.
	 */
	measure_grid_size(q, qr_index);
	/* Make an estimate based for the alignment pattern based on extending
	 * lines from capstones A and C.
	 */
	if (!line_intersect(&q->capstones[a].corners[0],
			    &q->capstones[a].corners[1],
			    &q->capstones[c].corners[0],
			    &q->capstones[c].corners[3],
			    &qr->align))
		goto fail;

	/* On V2+ grids, we should use the alignment pattern. */
	if (qr->grid_size > 21) {
		/* Try to find the actual location of the alignment pattern. */
		find_alignment_pattern(q, qr_index);

		/* Find the point of the alignment pattern closest to the
		 * top-left of the QR grid.
		 */
		if (qr->align_region >= 0) {
			struct polygon_score_data psd;
			struct quirc_region *reg =
				&q->regions[qr->align_region];

			/* Start from some point inside the alignment pattern */
			memcpy(&qr->align, &reg->seed, sizeof(qr->align));

			memcpy(&psd.ref, &hd, sizeof(psd.ref));
			psd.corners = &qr->align;
			psd.scores[0] = -hd.y * qr->align.x +
				hd.x * qr->align.y;

			flood_fill_seed(q, reg->seed.x, reg->seed.y,
					qr->align_region, QUIRC_PIXEL_BLACK,
					NULL, NULL);
			flood_fill_seed(q, reg->seed.x, reg->seed.y,
					QUIRC_PIXEL_BLACK, qr->align_region,
					find_leftmost_to_line, &psd);
		}
	}

	setup_qr_perspective(q, qr_index);
	return;

fail:
	/* We've been unable to complete setup for this grid. Undo what we've
	 * recorded and pretend it never happened.
	 */
	for (i = 0; i < 3; i++)
		q->capstones[qr->caps[i]].qr_grid = -1;
	q->num_grids--;
}

struct neighbour {
	int     index;
	quirc_float_t		distance;
};

struct neighbour_list {
	struct neighbour    n[QUIRC_MAX_CAPSTONES];
	int                 count;
};

static void test_neighbours(struct quirc *q, int i,
			    const struct neighbour_list *hlist,
			    const struct neighbour_list *vlist)
{
	/* Test each possible grouping */
	for (int j = 0; j < hlist->count; j++) {
		const struct neighbour *hn = &hlist->n[j];
		for (int k = 0; k < vlist->count; k++) {
			const struct neighbour *vn = &vlist->n[k];
			quirc_float_t squareness = fabs(1.0 - hn->distance / vn->distance);
			if (squareness < 0.2)
				record_qr_grid(q, hn->index, i, vn->index);
		}
	}
}

static void test_grouping(struct quirc *q, unsigned int i)
{
	struct quirc_capstone *c1 = &q->capstones[i];
	int j;
	struct neighbour_list hlist;
	struct neighbour_list vlist;

	hlist.count = 0;
	vlist.count = 0;

	/* Look for potential neighbours by examining the relative gradients
	 * from this capstone to others.
	 */
	for (j = 0; j < q->num_capstones; j++) {
		struct quirc_capstone *c2 = &q->capstones[j];
		quirc_float_t u, v;

		if (i == j)
			continue;

		perspective_unmap(c1->c, &c2->center, &u, &v);

		u = fabs(u - 3.5);
		v = fabs(v - 3.5);

		if (u < 0.2 * v) {
			struct neighbour *n = &hlist.n[hlist.count++];

			n->index = j;
			n->distance = v;
		}

		if (v < 0.2 * u) {
			struct neighbour *n = &vlist.n[vlist.count++];

			n->index = j;
			n->distance = u;
		}
	}

	if (!(hlist.count && vlist.count))
		return;

	test_neighbours(q, i, &hlist, &vlist);
}

static void pixels_setup(struct quirc *q, uint8_t threshold)
{
	if (QUIRC_PIXEL_ALIAS_IMAGE) {
		q->pixels = (quirc_pixel_t *)q->image;
	}

	uint8_t* source = q->image;
	quirc_pixel_t* dest = q->pixels;
	int length = q->w * q->h;
	while (length--) {
		uint8_t value = *source++;
		*dest++ = (value < threshold) ? QUIRC_PIXEL_BLACK : QUIRC_PIXEL_WHITE;
	}
}

uint8_t *quirc_begin(struct quirc *q, int *w, int *h)
{
	q->num_regions = QUIRC_PIXEL_REGION;
	q->num_capstones = 0;
	q->num_grids = 0;

	if (w)
		*w = q->w;
	if (h)
		*h = q->h;

	return q->image;
}

void quirc_end(struct quirc *q)
{
	int i;

	uint8_t threshold = otsu(q);
	pixels_setup(q, threshold);

	for (i = 0; i < q->h; i++)
		finder_scan(q, i);

	for (i = 0; i < q->num_capstones; i++)
		test_grouping(q, i);
}

void quirc_extract(const struct quirc *q, int index,
		   struct quirc_code *code)
{
	const struct quirc_grid *qr = &q->grids[index];
	int y;
	int i = 0;

	memset(code, 0, sizeof(*code));

	if (index < 0 || index > q->num_grids)
		return;

	perspective_map(qr->c, 0.0, 0.0, &code->corners[0]);
	perspective_map(qr->c, qr->grid_size, 0.0, &code->corners[1]);
	perspective_map(qr->c, qr->grid_size, qr->grid_size,
			&code->corners[2]);
	perspective_map(qr->c, 0.0, qr->grid_size, &code->corners[3]);

	code->size = qr->grid_size;

	/* Skip out early so as not to overrun the buffer. quirc_decode
	 * will return an error on interpreting the code.
	 */
	if (code->size > QUIRC_MAX_GRID_SIZE)
		return;

	for (y = 0; y < qr->grid_size; y++) {
		int x;
		for (x = 0; x < qr->grid_size; x++) {
			if (read_cell(q, index, x, y) > 0) {
				code->cell_bitmap[i >> 3] |= (1 << (i & 7));
			}
			i++;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//////// "decode.c"
////////////////////////////////////////////////////////////////////////////////////////////////////

/* quirc -- QR-code recognition library
 * Copyright (C) 2010-2012 Daniel Beer <dlbeer@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#define MAX_POLY    64

/************************************************************************
 * Galois fields
 */

struct galois_field {
	int             p;
	const uint8_t   *log;
	const uint8_t   *exp;
};

static const uint8_t gf16_exp[16] = {
	0x01, 0x02, 0x04, 0x08, 0x03, 0x06, 0x0c, 0x0b,
	0x05, 0x0a, 0x07, 0x0e, 0x0f, 0x0d, 0x09, 0x01
};

static const uint8_t gf16_log[16] = {
	0x00, 0x0f, 0x01, 0x04, 0x02, 0x08, 0x05, 0x0a,
	0x03, 0x0e, 0x09, 0x07, 0x06, 0x0d, 0x0b, 0x0c
};

static const struct galois_field gf16 = {
	.p = 15,
	.log = gf16_log,
	.exp = gf16_exp
};

static const uint8_t gf256_exp[256] = {
	0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
	0x1d, 0x3a, 0x74, 0xe8, 0xcd, 0x87, 0x13, 0x26,
	0x4c, 0x98, 0x2d, 0x5a, 0xb4, 0x75, 0xea, 0xc9,
	0x8f, 0x03, 0x06, 0x0c, 0x18, 0x30, 0x60, 0xc0,
	0x9d, 0x27, 0x4e, 0x9c, 0x25, 0x4a, 0x94, 0x35,
	0x6a, 0xd4, 0xb5, 0x77, 0xee, 0xc1, 0x9f, 0x23,
	0x46, 0x8c, 0x05, 0x0a, 0x14, 0x28, 0x50, 0xa0,
	0x5d, 0xba, 0x69, 0xd2, 0xb9, 0x6f, 0xde, 0xa1,
	0x5f, 0xbe, 0x61, 0xc2, 0x99, 0x2f, 0x5e, 0xbc,
	0x65, 0xca, 0x89, 0x0f, 0x1e, 0x3c, 0x78, 0xf0,
	0xfd, 0xe7, 0xd3, 0xbb, 0x6b, 0xd6, 0xb1, 0x7f,
	0xfe, 0xe1, 0xdf, 0xa3, 0x5b, 0xb6, 0x71, 0xe2,
	0xd9, 0xaf, 0x43, 0x86, 0x11, 0x22, 0x44, 0x88,
	0x0d, 0x1a, 0x34, 0x68, 0xd0, 0xbd, 0x67, 0xce,
	0x81, 0x1f, 0x3e, 0x7c, 0xf8, 0xed, 0xc7, 0x93,
	0x3b, 0x76, 0xec, 0xc5, 0x97, 0x33, 0x66, 0xcc,
	0x85, 0x17, 0x2e, 0x5c, 0xb8, 0x6d, 0xda, 0xa9,
	0x4f, 0x9e, 0x21, 0x42, 0x84, 0x15, 0x2a, 0x54,
	0xa8, 0x4d, 0x9a, 0x29, 0x52, 0xa4, 0x55, 0xaa,
	0x49, 0x92, 0x39, 0x72, 0xe4, 0xd5, 0xb7, 0x73,
	0xe6, 0xd1, 0xbf, 0x63, 0xc6, 0x91, 0x3f, 0x7e,
	0xfc, 0xe5, 0xd7, 0xb3, 0x7b, 0xf6, 0xf1, 0xff,
	0xe3, 0xdb, 0xab, 0x4b, 0x96, 0x31, 0x62, 0xc4,
	0x95, 0x37, 0x6e, 0xdc, 0xa5, 0x57, 0xae, 0x41,
	0x82, 0x19, 0x32, 0x64, 0xc8, 0x8d, 0x07, 0x0e,
	0x1c, 0x38, 0x70, 0xe0, 0xdd, 0xa7, 0x53, 0xa6,
	0x51, 0xa2, 0x59, 0xb2, 0x79, 0xf2, 0xf9, 0xef,
	0xc3, 0x9b, 0x2b, 0x56, 0xac, 0x45, 0x8a, 0x09,
	0x12, 0x24, 0x48, 0x90, 0x3d, 0x7a, 0xf4, 0xf5,
	0xf7, 0xf3, 0xfb, 0xeb, 0xcb, 0x8b, 0x0b, 0x16,
	0x2c, 0x58, 0xb0, 0x7d, 0xfa, 0xe9, 0xcf, 0x83,
	0x1b, 0x36, 0x6c, 0xd8, 0xad, 0x47, 0x8e, 0x01
};

static const uint8_t gf256_log[256] = {
	0x00, 0xff, 0x01, 0x19, 0x02, 0x32, 0x1a, 0xc6,
	0x03, 0xdf, 0x33, 0xee, 0x1b, 0x68, 0xc7, 0x4b,
	0x04, 0x64, 0xe0, 0x0e, 0x34, 0x8d, 0xef, 0x81,
	0x1c, 0xc1, 0x69, 0xf8, 0xc8, 0x08, 0x4c, 0x71,
	0x05, 0x8a, 0x65, 0x2f, 0xe1, 0x24, 0x0f, 0x21,
	0x35, 0x93, 0x8e, 0xda, 0xf0, 0x12, 0x82, 0x45,
	0x1d, 0xb5, 0xc2, 0x7d, 0x6a, 0x27, 0xf9, 0xb9,
	0xc9, 0x9a, 0x09, 0x78, 0x4d, 0xe4, 0x72, 0xa6,
	0x06, 0xbf, 0x8b, 0x62, 0x66, 0xdd, 0x30, 0xfd,
	0xe2, 0x98, 0x25, 0xb3, 0x10, 0x91, 0x22, 0x88,
	0x36, 0xd0, 0x94, 0xce, 0x8f, 0x96, 0xdb, 0xbd,
	0xf1, 0xd2, 0x13, 0x5c, 0x83, 0x38, 0x46, 0x40,
	0x1e, 0x42, 0xb6, 0xa3, 0xc3, 0x48, 0x7e, 0x6e,
	0x6b, 0x3a, 0x28, 0x54, 0xfa, 0x85, 0xba, 0x3d,
	0xca, 0x5e, 0x9b, 0x9f, 0x0a, 0x15, 0x79, 0x2b,
	0x4e, 0xd4, 0xe5, 0xac, 0x73, 0xf3, 0xa7, 0x57,
	0x07, 0x70, 0xc0, 0xf7, 0x8c, 0x80, 0x63, 0x0d,
	0x67, 0x4a, 0xde, 0xed, 0x31, 0xc5, 0xfe, 0x18,
	0xe3, 0xa5, 0x99, 0x77, 0x26, 0xb8, 0xb4, 0x7c,
	0x11, 0x44, 0x92, 0xd9, 0x23, 0x20, 0x89, 0x2e,
	0x37, 0x3f, 0xd1, 0x5b, 0x95, 0xbc, 0xcf, 0xcd,
	0x90, 0x87, 0x97, 0xb2, 0xdc, 0xfc, 0xbe, 0x61,
	0xf2, 0x56, 0xd3, 0xab, 0x14, 0x2a, 0x5d, 0x9e,
	0x84, 0x3c, 0x39, 0x53, 0x47, 0x6d, 0x41, 0xa2,
	0x1f, 0x2d, 0x43, 0xd8, 0xb7, 0x7b, 0xa4, 0x76,
	0xc4, 0x17, 0x49, 0xec, 0x7f, 0x0c, 0x6f, 0xf6,
	0x6c, 0xa1, 0x3b, 0x52, 0x29, 0x9d, 0x55, 0xaa,
	0xfb, 0x60, 0x86, 0xb1, 0xbb, 0xcc, 0x3e, 0x5a,
	0xcb, 0x59, 0x5f, 0xb0, 0x9c, 0xa9, 0xa0, 0x51,
	0x0b, 0xf5, 0x16, 0xeb, 0x7a, 0x75, 0x2c, 0xd7,
	0x4f, 0xae, 0xd5, 0xe9, 0xe6, 0xe7, 0xad, 0xe8,
	0x74, 0xd6, 0xf4, 0xea, 0xa8, 0x50, 0x58, 0xaf
};

const static struct galois_field gf256 = {
	.p = 255,
	.log = gf256_log,
	.exp = gf256_exp
};

/************************************************************************
 * Polynomial operations
 */

static void poly_add(uint8_t *dst, const uint8_t *src, uint8_t c,
		     int shift, const struct galois_field *gf)
{
	int i;
	int log_c = gf->log[c];

	if (!c)
		return;

	for (i = 0; i < MAX_POLY; i++) {
		int p = i + shift;
		uint8_t v = src[i];

		if (p < 0 || p >= MAX_POLY)
			continue;
		if (!v)
			continue;

		dst[p] ^= gf->exp[(gf->log[v] + log_c) % gf->p];
	}
}

static uint8_t poly_eval(const uint8_t *s, uint8_t x,
			 const struct galois_field *gf)
{
	int i;
	uint8_t sum = 0;
	uint8_t log_x = gf->log[x];

	if (!x)
		return s[0];

	for (i = 0; i < MAX_POLY; i++) {
		uint8_t c = s[i];

		if (!c)
			continue;

		sum ^= gf->exp[(gf->log[c] + log_x * i) % gf->p];
	}

	return sum;
}

/************************************************************************
 * Berlekamp-Massey algorithm for finding error locator polynomials.
 */

static void berlekamp_massey(const uint8_t *s, int N,
			     const struct galois_field *gf,
			     uint8_t *sigma)
{
	uint8_t C[MAX_POLY];
	uint8_t B[MAX_POLY];
	int L = 0;
	int m = 1;
	uint8_t b = 1;
	int n;

	memset(B, 0, sizeof(B));
	memset(C, 0, sizeof(C));
	B[0] = 1;
	C[0] = 1;

	for (n = 0; n < N; n++) {
		uint8_t d = s[n];
		uint8_t mult;
		int i;

		for (i = 1; i <= L; i++) {
			if (!(C[i] && s[n - i]))
				continue;

			d ^= gf->exp[(gf->log[C[i]] +
				      gf->log[s[n - i]]) %
				     gf->p];
		}

		mult = gf->exp[(gf->p - gf->log[b] + gf->log[d]) % gf->p];

		if (!d) {
			m++;
		} else if (L * 2 <= n) {
			uint8_t T[MAX_POLY];

			memcpy(T, C, sizeof(T));
			poly_add(C, B, mult, m, gf);
			memcpy(B, T, sizeof(B));
			L = n + 1 - L;
			b = d;
			m = 1;
		} else {
			poly_add(C, B, mult, m, gf);
			m++;
		}
	}

	memcpy(sigma, C, MAX_POLY);
}

/************************************************************************
 * Code stream error correction
 *
 * Generator polynomial for GF(2^8) is x^8 + x^4 + x^3 + x^2 + 1
 */

static int block_syndromes(const uint8_t *data, int bs, int npar, uint8_t *s)
{
	int nonzero = 0;
	int i;

	memset(s, 0, MAX_POLY);

	for (i = 0; i < npar; i++) {
		int j;

		for (j = 0; j < bs; j++) {
			uint8_t c = data[bs - j - 1];

			if (!c)
				continue;

			s[i] ^= gf256_exp[((int)gf256_log[c] +
				    i * j) % 255];
		}

		if (s[i])
			nonzero = 1;
	}

	return nonzero;
}

static void eloc_poly(uint8_t *omega,
		      const uint8_t *s, const uint8_t *sigma,
		      int npar)
{
	int i;

	memset(omega, 0, MAX_POLY);

	for (i = 0; i < npar; i++) {
		const uint8_t a = sigma[i];
		const uint8_t log_a = gf256_log[a];
		int j;

		if (!a)
			continue;

		for (j = 0; j + 1 < MAX_POLY; j++) {
			const uint8_t b = s[j + 1];

			if (i + j >= npar)
				break;

			if (!b)
				continue;

			omega[i + j] ^=
			    gf256_exp[(log_a + gf256_log[b]) % 255];
		}
	}
}

static quirc_decode_error_t correct_block(uint8_t *data,
					  const struct quirc_rs_params *ecc)
{
	int npar = ecc->bs - ecc->dw;
	uint8_t s[MAX_POLY];
	uint8_t sigma[MAX_POLY];
	uint8_t sigma_deriv[MAX_POLY];
	uint8_t omega[MAX_POLY];
	int i;

	/* Compute syndrome vector */
	if (!block_syndromes(data, ecc->bs, npar, s))
		return QUIRC_SUCCESS;

	berlekamp_massey(s, npar, &gf256, sigma);

	/* Compute derivative of sigma */
	memset(sigma_deriv, 0, MAX_POLY);
	for (i = 0; i + 1 < MAX_POLY; i += 2)
		sigma_deriv[i] = sigma[i + 1];

	/* Compute error evaluator polynomial */
	eloc_poly(omega, s, sigma, npar - 1);

	/* Find error locations and magnitudes */
	for (i = 0; i < ecc->bs; i++) {
		uint8_t xinv = gf256_exp[255 - i];

		if (!poly_eval(sigma, xinv, &gf256)) {
			uint8_t sd_x = poly_eval(sigma_deriv, xinv, &gf256);
			uint8_t omega_x = poly_eval(omega, xinv, &gf256);
			uint8_t error = gf256_exp[(255 - gf256_log[sd_x] +
						   gf256_log[omega_x]) % 255];

			data[ecc->bs - i - 1] ^= error;
		}
	}

	if (block_syndromes(data, ecc->bs, npar, s))
		return QUIRC_ERROR_DATA_ECC;

	return QUIRC_SUCCESS;
}

/************************************************************************
 * Format value error correction
 *
 * Generator polynomial for GF(2^4) is x^4 + x + 1
 */

#define FORMAT_MAX_ERROR    3
#define FORMAT_SYNDROMES    (FORMAT_MAX_ERROR * 2)
#define FORMAT_BITS         15

static int format_syndromes(uint16_t u, uint8_t *s)
{
	int i;
	int nonzero = 0;

	memset(s, 0, MAX_POLY);

	for (i = 0; i < FORMAT_SYNDROMES; i++) {
		int j;

		s[i] = 0;
		for (j = 0; j < FORMAT_BITS; j++)
			if (u & (1 << j))
				s[i] ^= gf16_exp[((i + 1) * j) % 15];

		if (s[i])
			nonzero = 1;
	}

	return nonzero;
}

static quirc_decode_error_t correct_format(uint16_t *f_ret)
{
	uint16_t u = *f_ret;
	int i;
	uint8_t s[MAX_POLY];
	uint8_t sigma[MAX_POLY];

	/* Evaluate U (received codeword) at each of alpha_1 .. alpha_6
	 * to get S_1 .. S_6 (but we index them from 0).
	 */
	if (!format_syndromes(u, s))
		return QUIRC_SUCCESS;

	berlekamp_massey(s, FORMAT_SYNDROMES, &gf16, sigma);

	/* Now, find the roots of the polynomial */
	for (i = 0; i < 15; i++)
		if (!poly_eval(sigma, gf16_exp[15 - i], &gf16))
			u ^= (1 << i);

	if (format_syndromes(u, s))
		return QUIRC_ERROR_FORMAT_ECC;

	*f_ret = u;
	return QUIRC_SUCCESS;
}

/************************************************************************
 * Decoder algorithm
 */

struct datastream {
	uint8_t		*raw;
	int     data_bits;
	int     ptr;

	uint8_t data[QUIRC_MAX_PAYLOAD];
};

static inline int grid_bit(const struct quirc_code *code, int x, int y)
{
	int p = y * code->size + x;

	return (code->cell_bitmap[p >> 3] >> (p & 7)) & 1;
}

static quirc_decode_error_t read_format(const struct quirc_code *code,
					struct quirc_data *data, int which)
{
	int i;
	uint16_t format = 0;
	uint16_t fdata;
	quirc_decode_error_t err;

	if (which) {
		for (i = 0; i < 7; i++)
			format = (format << 1) |
				grid_bit(code, 8, code->size - 1 - i);
		for (i = 0; i < 8; i++)
			format = (format << 1) |
				grid_bit(code, code->size - 8 + i, 8);
	} else {
		static const int xs[15] = {
			8, 8, 8, 8, 8, 8, 8, 8, 7, 5, 4, 3, 2, 1, 0
		};
		static const int ys[15] = {
			0, 1, 2, 3, 4, 5, 7, 8, 8, 8, 8, 8, 8, 8, 8
		};

		for (i = 14; i >= 0; i--)
			format = (format << 1) | grid_bit(code, xs[i], ys[i]);
	}

	format ^= 0x5412;

	err = correct_format(&format);
	if (err)
		return err;

	fdata = format >> 10;
	data->ecc_level = fdata >> 3;
	data->mask = fdata & 7;

	return QUIRC_SUCCESS;
}

static int mask_bit(int mask, int i, int j)
{
	switch (mask) {
	case 0: return !((i + j) % 2);
	case 1: return !(i % 2);
	case 2: return !(j % 3);
	case 3: return !((i + j) % 3);
	case 4: return !(((i / 2) + (j / 3)) % 2);
	case 5: return !((i * j) % 2 + (i * j) % 3);
	case 6: return !(((i * j) % 2 + (i * j) % 3) % 2);
	case 7: return !(((i * j) % 3 + (i + j) % 2) % 2);
	}

	return 0;
}

static int reserved_cell(int version, int i, int j)
{
	const struct quirc_version_info *ver = &quirc_version_db[version];
	int size = version * 4 + 17;
	int ai = -1, aj = -1, a;

	/* Finder + format: top left */
	if (i < 9 && j < 9)
		return 1;

	/* Finder + format: bottom left */
	if (i + 8 >= size && j < 9)
		return 1;

	/* Finder + format: top right */
	if (i < 9 && j + 8 >= size)
		return 1;

	/* Exclude timing patterns */
	if (i == 6 || j == 6)
		return 1;

	/* Exclude version info, if it exists. Version info sits adjacent to
	 * the top-right and bottom-left finders in three rows, bounded by
	 * the timing pattern.
	 */
	if (version >= 7) {
		if (i < 6 && j + 11 >= size)
			return 1;
		if (i + 11 >= size && j < 6)
			return 1;
	}

	/* Exclude alignment patterns */
	for (a = 0; a < QUIRC_MAX_ALIGNMENT && ver->apat[a]; a++) {
		int p = ver->apat[a];

		if (abs(p - i) < 3)
			ai = a;
		if (abs(p - j) < 3)
			aj = a;
	}

	if (ai >= 0 && aj >= 0) {
		a--;
		if (ai > 0 && ai < a)
			return 1;
		if (aj > 0 && aj < a)
			return 1;
		if (aj == a && ai == a)
			return 1;
	}

	return 0;
}

static void read_bit(const struct quirc_code *code,
		     struct quirc_data *data,
		     struct datastream *ds, int i, int j)
{
	int bitpos = ds->data_bits & 7;
	int bytepos = ds->data_bits >> 3;
	int v = grid_bit(code, j, i);

	if (mask_bit(data->mask, i, j))
		v ^= 1;

	if (v)
		ds->raw[bytepos] |= (0x80 >> bitpos);

	ds->data_bits++;
}

static void _read_data(const struct quirc_code *code,
		      struct quirc_data *data,
		      struct datastream *ds)
{
	int y = code->size - 1;
	int x = code->size - 1;
	int dir = -1;

	while (x > 0) {
		if (x == 6)
			x--;

		if (!reserved_cell(data->version, y, x))
			read_bit(code, data, ds, y, x);

		if (!reserved_cell(data->version, y, x - 1))
			read_bit(code, data, ds, y, x - 1);

		y += dir;
		if (y < 0 || y >= code->size) {
			dir = -dir;
			x -= 2;
			y += dir;
		}
	}
}

static quirc_decode_error_t codestream_ecc(struct quirc_data *data,
					   struct datastream *ds)
{
	const struct quirc_version_info *ver =
		&quirc_version_db[data->version];
	const struct quirc_rs_params *sb_ecc = &ver->ecc[data->ecc_level];
	struct quirc_rs_params lb_ecc;
	const int lb_count =
	    (ver->data_bytes - sb_ecc->bs * sb_ecc->ns) / (sb_ecc->bs + 1);
	const int bc = lb_count + sb_ecc->ns;
	const int ecc_offset = sb_ecc->dw * bc + lb_count;
	int dst_offset = 0;
	int i;

	memcpy(&lb_ecc, sb_ecc, sizeof(lb_ecc));
	lb_ecc.dw++;
	lb_ecc.bs++;

	for (i = 0; i < bc; i++) {
		uint8_t *dst = ds->data + dst_offset;
		const struct quirc_rs_params *ecc =
		    (i < sb_ecc->ns) ? sb_ecc : &lb_ecc;
		const int num_ec = ecc->bs - ecc->dw;
		quirc_decode_error_t err;
		int j;

		for (j = 0; j < ecc->dw; j++)
			dst[j] = ds->raw[j * bc + i];
		for (j = 0; j < num_ec; j++)
			dst[ecc->dw + j] = ds->raw[ecc_offset + j * bc + i];

		err = correct_block(dst, ecc);
		if (err)
			return err;

		dst_offset += ecc->dw;
	}

	ds->data_bits = dst_offset * 8;

	return QUIRC_SUCCESS;
}

static inline int bits_remaining(const struct datastream *ds)
{
	return ds->data_bits - ds->ptr;
}

static int take_bits(struct datastream *ds, int len)
{
	int ret = 0;

	while (len && (ds->ptr < ds->data_bits)) {
		uint8_t b = ds->data[ds->ptr >> 3];
		int bitpos = ds->ptr & 7;

		ret <<= 1;
		if ((b << bitpos) & 0x80)
			ret |= 1;

		ds->ptr++;
		len--;
	}

	return ret;
}

static int numeric_tuple(struct quirc_data *data,
			 struct datastream *ds,
			 int bits, int digits)
{
	int tuple;
	int i;

	if (bits_remaining(ds) < bits)
		return -1;

	tuple = take_bits(ds, bits);

	for (i = digits - 1; i >= 0; i--) {
		data->payload[data->payload_len + i] = tuple % 10 + '0';
		tuple /= 10;
	}

	data->payload_len += digits;
	return 0;
}

static quirc_decode_error_t decode_numeric(struct quirc_data *data,
					   struct datastream *ds)
{
	int bits = 14;
	int count;

	if (data->version < 10)
		bits = 10;
	else if (data->version < 27)
		bits = 12;

	count = take_bits(ds, bits);
	if (data->payload_len + count + 1 > QUIRC_MAX_PAYLOAD)
		return QUIRC_ERROR_DATA_OVERFLOW;

	while (count >= 3) {
		if (numeric_tuple(data, ds, 10, 3) < 0)
			return QUIRC_ERROR_DATA_UNDERFLOW;
		count -= 3;
	}

	if (count >= 2) {
		if (numeric_tuple(data, ds, 7, 2) < 0)
			return QUIRC_ERROR_DATA_UNDERFLOW;
		count -= 2;
	}

	if (count) {
		if (numeric_tuple(data, ds, 4, 1) < 0)
			return QUIRC_ERROR_DATA_UNDERFLOW;
		count--;
	}

	return QUIRC_SUCCESS;
}

static int alpha_tuple(struct quirc_data *data,
		       struct datastream *ds,
		       int bits, int digits)
{
	int tuple;
	int i;

	if (bits_remaining(ds) < bits)
		return -1;

	tuple = take_bits(ds, bits);

	for (i = 0; i < digits; i++) {
		static const char *alpha_map =
			"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ $%*+-./:";

		data->payload[data->payload_len + digits - i - 1] =
			alpha_map[tuple % 45];
		tuple /= 45;
	}

	data->payload_len += digits;
	return 0;
}

static quirc_decode_error_t decode_alpha(struct quirc_data *data,
					 struct datastream *ds)
{
	int bits = 13;
	int count;

	if (data->version < 10)
		bits = 9;
	else if (data->version < 27)
		bits = 11;

	count = take_bits(ds, bits);
	if (data->payload_len + count + 1 > QUIRC_MAX_PAYLOAD)
		return QUIRC_ERROR_DATA_OVERFLOW;

	while (count >= 2) {
		if (alpha_tuple(data, ds, 11, 2) < 0)
			return QUIRC_ERROR_DATA_UNDERFLOW;
		count -= 2;
	}

	if (count) {
		if (alpha_tuple(data, ds, 6, 1) < 0)
			return QUIRC_ERROR_DATA_UNDERFLOW;
		count--;
	}

	return QUIRC_SUCCESS;
}

static quirc_decode_error_t decode_byte(struct quirc_data *data,
					struct datastream *ds)
{
	int bits = 16;
	int count;
	int i;

	if (data->version < 10)
		bits = 8;

	count = take_bits(ds, bits);
	if (data->payload_len + count + 1 > QUIRC_MAX_PAYLOAD)
		return QUIRC_ERROR_DATA_OVERFLOW;
	if (bits_remaining(ds) < count * 8)
		return QUIRC_ERROR_DATA_UNDERFLOW;

	for (i = 0; i < count; i++)
		data->payload[data->payload_len++] = take_bits(ds, 8);

	return QUIRC_SUCCESS;
}

static quirc_decode_error_t decode_kanji(struct quirc_data *data,
					 struct datastream *ds)
{
	int bits = 12;
	int count;
	int i;

	if (data->version < 10)
		bits = 8;
	else if (data->version < 27)
		bits = 10;

	count = take_bits(ds, bits);
	if (data->payload_len + count * 2 + 1 > QUIRC_MAX_PAYLOAD)
		return QUIRC_ERROR_DATA_OVERFLOW;
	if (bits_remaining(ds) < count * 13)
		return QUIRC_ERROR_DATA_UNDERFLOW;

	for (i = 0; i < count; i++) {
		int d = take_bits(ds, 13);
		int msB = d / 0xc0;
		int lsB = d % 0xc0;
		int intermediate = (msB << 8) | lsB;
		uint16_t sjw;

		if (intermediate + 0x8140 <= 0x9ffc) {
			/* bytes are in the range 0x8140 to 0x9FFC */
			sjw = intermediate + 0x8140;
		} else {
			/* bytes are in the range 0xE040 to 0xEBBF */
			sjw = intermediate + 0xc140;
		}

		data->payload[data->payload_len++] = sjw >> 8;
		data->payload[data->payload_len++] = sjw & 0xff;
	}

	return QUIRC_SUCCESS;
}

static quirc_decode_error_t decode_eci(struct quirc_data *data,
				       struct datastream *ds)
{
	if (bits_remaining(ds) < 8)
		return QUIRC_ERROR_DATA_UNDERFLOW;

	data->eci = take_bits(ds, 8);

	if ((data->eci & 0xc0) == 0x80) {
		if (bits_remaining(ds) < 8)
			return QUIRC_ERROR_DATA_UNDERFLOW;

		data->eci = (data->eci << 8) | take_bits(ds, 8);
	} else if ((data->eci & 0xe0) == 0xc0) {
		if (bits_remaining(ds) < 16)
			return QUIRC_ERROR_DATA_UNDERFLOW;

		data->eci = (data->eci << 16) | take_bits(ds, 16);
	}

	return QUIRC_SUCCESS;
}

static quirc_decode_error_t decode_payload(struct quirc_data *data,
					   struct datastream *ds)
{
	while (bits_remaining(ds) >= 4) {
		quirc_decode_error_t err = QUIRC_SUCCESS;
		int type = take_bits(ds, 4);

		switch (type) {
		case QUIRC_DATA_TYPE_NUMERIC:
			err = decode_numeric(data, ds);
			break;

		case QUIRC_DATA_TYPE_ALPHA:
			err = decode_alpha(data, ds);
			break;

		case QUIRC_DATA_TYPE_BYTE:
			err = decode_byte(data, ds);
			break;

		case QUIRC_DATA_TYPE_KANJI:
			err = decode_kanji(data, ds);
			break;

		case 7:
			err = decode_eci(data, ds);
			break;

		default:
			goto done;
		}

		if (err)
			return err;

		if (!(type & (type - 1)) && (type > data->data_type))
			data->data_type = type;
	}

done:

	/* Add nul terminator to all payloads */
	if (data->payload_len >= (int) sizeof(data->payload))
		data->payload_len--;
	data->payload[data->payload_len] = 0;

	return QUIRC_SUCCESS;
}

quirc_decode_error_t quirc_decode(const struct quirc_code *code,
				  struct quirc_data *data)
{
	quirc_decode_error_t err;
	struct datastream ds;

	if (code->size > QUIRC_MAX_GRID_SIZE)
		return QUIRC_ERROR_INVALID_GRID_SIZE;

	if ((code->size - 17) % 4)
		return QUIRC_ERROR_INVALID_GRID_SIZE;

	memset(data, 0, sizeof(*data));
	memset(&ds, 0, sizeof(ds));

	data->version = (code->size - 17) / 4;

	if (data->version < 1 ||
	    data->version > QUIRC_MAX_VERSION)
		return QUIRC_ERROR_INVALID_VERSION;

	/* Read format information -- try both locations */
	err = read_format(code, data, 0);
	if (err)
		err = read_format(code, data, 1);
	if (err)
		return err;

	/*
	 * Borrow data->payload to store the raw bits.
	 * It's only used during read_data + coddestream_ecc below.
	 *
	 * This trick saves the size of struct datastream, which we allocate
	 * on the stack.
	 */

	ds.raw = data->payload;

	_read_data(code, data, &ds);
	err = codestream_ecc(data, &ds);
	if (err)
		return err;

	ds.raw = NULL; /* We've done with this buffer. */

	err = decode_payload(data, &ds);
	if (err)
		return err;

	return QUIRC_SUCCESS;
}

void quirc_flip(struct quirc_code *code)
{
	struct quirc_code flipped = {0};
	unsigned int offset = 0;
	for (int y = 0; y < code->size; y++) {
		for (int x = 0; x < code->size; x++) {
			if (grid_bit(code, y, x)) {
				flipped.cell_bitmap[offset >> 3u] |= (1u << (offset & 7u));
			}
			offset++;
		}
	}
	memcpy(&code->cell_bitmap, &flipped.cell_bitmap, sizeof(flipped.cell_bitmap));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//////// "quirc.c"
////////////////////////////////////////////////////////////////////////////////////////////////////

/* quirc -- QR-code recognition library
 * Copyright (C) 2010-2012 Daniel Beer <dlbeer@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

const char *quirc_version(void)
{
	return "1.0";
}

struct quirc *quirc_new(void)
{
	// struct quirc *q = malloc(sizeof(*q));
    struct quirc *q = fb_alloc(sizeof(*q), FB_ALLOC_NO_HINT);

	if (!q)
		return NULL;

	memset(q, 0, sizeof(*q));
	return q;
}

void quirc_destroy(struct quirc *q)
{
#if 0 // original
	free(q->image);
	/* q->pixels may alias q->image when their type representation is of the
	   same size, so we need to be careful here to avoid a double free */
	if (!QUIRC_PIXEL_ALIAS_IMAGE)
		free(q->pixels);
	free(q->flood_fill_vars);
	free(q);
#else
    if(q->image){
        fb_free();
    }
	/* q->pixels may alias q->image when their type representation is of the
	   same size, so we need to be careful here to avoid a double free */
	if (!QUIRC_PIXEL_ALIAS_IMAGE) {
        if(q->pixels) {
            fb_free();
        }
    }
    if(q->flood_fill_vars) {
        fb_free();
    }
    if(q) {
        fb_free();
    }
#endif
}

int quirc_resize(struct quirc *q, int w, int h)
{
	uint8_t		*image  = NULL;
	quirc_pixel_t	*pixels = NULL;
	size_t num_vars;
	size_t vars_byte_size;
	struct quirc_flood_fill_vars *vars = NULL;

	/*
	 * XXX: w and h should be size_t (or at least unsigned) as negatives
	 * values would not make much sense. The downside is that it would break
	 * both the API and ABI. Thus, at the moment, let's just do a sanity
	 * check.
	 */
	if (w < 0 || h < 0)
		goto fail;

	/*
	 * alloc a new buffer for q->image. We avoid realloc(3) because we want
	 * on failure to be leave `q` in a consistant, unmodified state.
	 */
	// image = calloc(w, h);
    image = fb_alloc(w * h, FB_ALLOC_NO_HINT);
	if (!image)
		goto fail;

	/* compute the "old" (i.e. currently allocated) and the "new"
	   (i.e. requested) image dimensions */
	size_t olddim = q->w * q->h;
	size_t newdim = w * h;
	size_t min = (olddim < newdim ? olddim : newdim);

	/*
	 * copy the data into the new buffer, avoiding (a) to read beyond the
	 * old buffer when the new size is greater and (b) to write beyond the
	 * new buffer when the new size is smaller, hence the min computation.
	 */
	(void)memcpy(image, q->image, min);

	/* alloc a new buffer for q->pixels if needed */
	if (!QUIRC_PIXEL_ALIAS_IMAGE) {
		// pixels = calloc(newdim, sizeof(quirc_pixel_t));
        pixels = fb_alloc(newdim * sizeof(quirc_pixel_t), FB_ALLOC_NO_HINT);
		if (!pixels)
			goto fail;
	}

	/*
	 * alloc the work area for the flood filling logic.
	 *
	 * the size was chosen with the following assumptions and observations:
	 *
	 * - rings are the regions which requires the biggest work area.
	 * - they consumes the most when they are rotated by about 45 degree.
	 *   in that case, the necessary depth is about (2 * height_of_the_ring).
	 * - the maximum height of rings would be about 1/3 of the image height.
	 */

	if ((size_t)h * 2 / 2 != h) {
		goto fail; /* size_t overflow */
	}
	num_vars = (size_t)h * 2 / 3;
	if (num_vars == 0) {
		num_vars = 1;
	}

	vars_byte_size = sizeof(*vars) * num_vars;
	if (vars_byte_size / sizeof(*vars) != num_vars) {
		goto fail; /* size_t overflow */
	}
	// vars = malloc(vars_byte_size);
    vars = fb_alloc(vars_byte_size, FB_ALLOC_NO_HINT);
	if (!vars)
		goto fail;

	/* alloc succeeded, update `q` with the new size and buffers */
	q->w = w;
	q->h = h;
	// free(q->image);
    if(q->image) {
        fb_free();
    }
	q->image = image;
	if (!QUIRC_PIXEL_ALIAS_IMAGE) {
		// free(q->pixels);
        if(q->pixels) {
            fb_free();
        }
		q->pixels = pixels;
	}
	// free(q->flood_fill_vars);
    if(q->flood_fill_vars) {
        fb_free();
    }
	q->flood_fill_vars = vars;
	q->num_flood_fill_vars = num_vars;

	return 0;
	/* NOTREACHED */
fail:
	fb_free(); // free(image);
	fb_free(); // free(pixels);
	fb_free(); // free(vars);

	return -1;
}

int quirc_count(const struct quirc *q)
{
	return q->num_grids;
}

static const char *const error_table[] = {
	[QUIRC_SUCCESS] = "Success",
	[QUIRC_ERROR_INVALID_GRID_SIZE] = "Invalid grid size",
	[QUIRC_ERROR_INVALID_VERSION] = "Invalid version",
	[QUIRC_ERROR_FORMAT_ECC] = "Format data ECC failure",
	[QUIRC_ERROR_DATA_ECC] = "ECC failure",
	[QUIRC_ERROR_UNKNOWN_DATA_TYPE] = "Unknown data type",
	[QUIRC_ERROR_DATA_OVERFLOW] = "Data overflow",
	[QUIRC_ERROR_DATA_UNDERFLOW] = "Data underflow"
};

const char *quirc_strerror(quirc_decode_error_t err)
{
	if (err >= 0 && err < sizeof(error_table) / sizeof(error_table[0]))
		return error_table[err];

	return "Unknown error";
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void imlib_find_qrcodes(list_t *out, image_t *ptr, rectangle_t *roi)
{
    list_init(out, sizeof(find_qrcodes_list_lnk_data_t));

    struct quirc *controller = quirc_new();

    if(0x00 != quirc_resize(controller, roi->w, roi->h)) {
    	quirc_destroy(controller);
		return;
	}

    uint8_t *grayscale_image = quirc_begin(controller, NULL, NULL);

    image_t img;
    img.w = roi->w;
    img.h = roi->h;
    img.pixfmt = PIXFORMAT_GRAYSCALE;
    img.data = grayscale_image;
    imlib_draw_image(&img, ptr, 0, 0, 1.f, 1.f, roi, -1, 256, NULL, NULL, 0, NULL, NULL);

    quirc_end(controller);
    // list_init(out, sizeof(find_qrcodes_list_lnk_data_t));

    for (int i = 0, j = quirc_count(controller); i < j; i++) {
        struct quirc_code *code = fb_alloc(sizeof(struct quirc_code), FB_ALLOC_NO_HINT);
        struct quirc_data *data = fb_alloc(sizeof(struct quirc_data), FB_ALLOC_NO_HINT);
        quirc_extract(controller, i, code);

        if(quirc_decode(code, data) == QUIRC_SUCCESS) {
            find_qrcodes_list_lnk_data_t lnk_data;
            rectangle_init(&(lnk_data.rect), code->corners[0].x + roi->x, code->corners[0].y + roi->y, 0, 0);

            for (size_t k = 1, l = (sizeof(code->corners) / sizeof(code->corners[0])); k < l; k++) {
                rectangle_t temp;
                rectangle_init(&temp, code->corners[k].x + roi->x, code->corners[k].y + roi->y, 0, 0);
                rectangle_united(&(lnk_data.rect), &temp);
            }

            // Add corners...
            lnk_data.corners[0].x = fast_roundf(code->corners[0].x) + roi->x; // top-left
            lnk_data.corners[0].y = fast_roundf(code->corners[0].y) + roi->y; // top-left
            lnk_data.corners[1].x = fast_roundf(code->corners[1].x) + roi->x; // top-right
            lnk_data.corners[1].y = fast_roundf(code->corners[1].y) + roi->y; // top-right
            lnk_data.corners[2].x = fast_roundf(code->corners[2].x) + roi->x; // bottom-right
            lnk_data.corners[2].y = fast_roundf(code->corners[2].y) + roi->y; // bottom-right
            lnk_data.corners[3].x = fast_roundf(code->corners[3].x) + roi->x; // bottom-left
            lnk_data.corners[3].y = fast_roundf(code->corners[3].y) + roi->y; // bottom-left

            // Payload is already null terminated.
            lnk_data.payload_len = data->payload_len;
            lnk_data.payload = xalloc(data->payload_len);
            memcpy(lnk_data.payload, data->payload, data->payload_len);

            lnk_data.version = data->version;
            lnk_data.ecc_level = data->ecc_level;
            lnk_data.mask = data->mask;
            lnk_data.data_type = data->data_type;
            lnk_data.eci = data->eci;

            list_push_back(out, &lnk_data);
        }

        fb_free();
        fb_free();
    }

    quirc_destroy(controller);
}
#endif //IMLIB_ENABLE_QRCODES *INDENT-ON*
