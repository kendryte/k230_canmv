#include <ft2build.h>
#include FT_FREETYPE_H

#include <stdio.h>

#include "py/obj.h"
#include "py/runtime.h"

#include "imlib.h"
#include "ff_wrapper.h"

#define FREETYPE_DEFAULT_FONT_PATH "/sdcard/res/font/SourceHanSansSC-Normal-Min.ttf"

static FT_Face s_ft_face;
static FT_Library s_ft_library;

static int s_ft_init_flag = 0;
static char s_ft_font_path[128];
static const char *s_ft_dft_font_path = FREETYPE_DEFAULT_FONT_PATH;

void freetype_deinit(void)
{
    if(0x01 == s_ft_init_flag)
    {
        // Clean up
        FT_Done_Face(s_ft_face);
        FT_Done_FreeType(s_ft_library);
    }

    s_ft_init_flag = 0;
    memset(s_ft_font_path, 0, sizeof(s_ft_font_path));
}

static int imlib_freetype_init(const char *font_path)
{
    FIL fp;
    FRESULT res;

    FT_Error error;
    const char *_font_path = font_path ? font_path : s_ft_dft_font_path;

    if((0x01 == s_ft_init_flag) && (0x00 == strncmp(_font_path, s_ft_font_path, sizeof(s_ft_font_path)))) {
        return 0;
    }

    if(s_ft_init_flag) {
        // Clean up
        FT_Done_Face(s_ft_face);
        FT_Done_FreeType(s_ft_library);
    }
    s_ft_init_flag = 0;
    memset(s_ft_font_path, 0, sizeof(s_ft_font_path));

    if ((res = f_open_helper(&fp, _font_path, "rb")) != FR_OK) {
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("Open font %s failed."), _font_path);
    }
    f_close(&fp);

    // Initialize the FreeType library
    error = FT_Init_FreeType(&s_ft_library);
    if (error) {
        printf("Could not initialize FreeType library\n");
        return 1;
    }

    // Load a font face from a file
    error = FT_New_Face(s_ft_library, _font_path, 0, &s_ft_face);
    if (error) {
        printf("Could not load font face\n");
        FT_Done_FreeType(s_ft_library);
        return 2;
    }

    s_ft_init_flag = 1;
    strncpy(s_ft_font_path, _font_path, sizeof(s_ft_font_path));

    return 0;
}

// static void print_glyph(FT_GlyphSlot slot) {
//     FT_Bitmap *bitmap = &slot->bitmap;

//     printf("%dx%d, left %d, top %d, x %ld\n", bitmap->width, bitmap->rows, slot->bitmap_left, slot->bitmap_top, slot->advance.x >> 6);

//     for(int i = 0; i < bitmap->rows; i++) {
//         for (int j = 0; j < bitmap->width; j++) {
//             if(bitmap->buffer[i * bitmap->width + j]) {
//                 printf("*");
//             } else {
//                 printf(" ");
//             }
//         }
//         printf("\n");
//     }
// }

// Function to decode a UTF-8 character to a Unicode code point
static FT_ULong utf8_to_unicode(const char **ptr) {
    const unsigned char *p = (const unsigned char *)*ptr;
    FT_ULong unicode;

    if (p[0] < 0x80) {
        unicode = p[0];
        *ptr += 1;
    } else if ((p[0] & 0xE0) == 0xC0) {
        unicode = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F);
        *ptr += 2;
    } else if ((p[0] & 0xF0) == 0xE0) {
        unicode = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
        *ptr += 3;
    } else if ((p[0] & 0xF8) == 0xF0) {
        unicode = ((p[0] & 0x07) << 18) | ((p[1] & 0x3F) << 12) | ((p[2] & 0x3F) << 6) | (p[3] & 0x3F);
        *ptr += 4;
    } else {
        unicode = 0; // Invalid UTF-8 character
        *ptr += 1;
    }

    return unicode;
}

static void inline draw_bitmap(image_t *img, int color, int x, int y, FT_Bitmap *bitmap) {
    FT_Int i, j, p, q;
    FT_Int x_max = x + bitmap->width;
    FT_Int y_max = y + bitmap->rows;

    for (i = x, p = 0; i < x_max; i++, p++) {
        for (j = y, q = 0; j < y_max; j++, q++) {
            if (i < 0 || j < 0 || i >= img->w || j >= img->h)
                continue;

            if(bitmap->buffer[q * bitmap->width + p]) {
                imlib_set_pixel(img, i, j, color);
            }
        }
    }
}

void imlib_draw_string_advance(image_t *img,
                               int x_off,
                               int y_off,
                               int char_size,
                               const char *str,
                               int color,
                               const char *font_path)
{
    int error = 0;

    // printf("x %d, y %d, size %d, str \'%s\', color %x, font %s\n", x_off, y_off, char_size, str, color, font_path);

    if(0x00 != (error = imlib_freetype_init(font_path))) {
        mp_raise_msg_varg(&mp_type_RuntimeError, MP_ERROR_TEXT("Init font %s failed %d."), font_path, error);
    }

    // Set the font size
    FT_Set_Pixel_Sizes(s_ft_face, 0, char_size);  // Width set to 0 to dynamically adjust based on height

    int point_x = x_off;
    int point_y = y_off + char_size;
    const char *p = str;

    while(*p) {
        FT_ULong charcode = utf8_to_unicode(&p);

        if(0x00 == charcode) {
            continue;
        }

        // Load the glyph for the character with the transformation
        if (0x00 != (error = FT_Load_Char(s_ft_face, charcode, FT_LOAD_RENDER))) {
            mp_raise_msg_varg(&mp_type_RuntimeError, MP_ERROR_TEXT("Load char(%x) failed, %d."), charcode, error);
        }

        // Access the bitmap
        FT_Bitmap *bitmap = &s_ft_face->glyph->bitmap;

        // Draw the bitmap
        draw_bitmap(img, color, point_x + s_ft_face->glyph->bitmap_left, point_y - s_ft_face->glyph->bitmap_top, bitmap);

        // Advance the cursor to the start of the next character
        point_x += s_ft_face->glyph->advance.x >> 6;
    }
}
