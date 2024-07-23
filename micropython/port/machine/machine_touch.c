/* Copyright (c) 2023, Canaan Bright Sight Co., Ltd
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "sys/ioctl.h"

#include "machine_touch.h"
#include "mpprint.h"
#include "py/runtime.h"
#include "py/obj.h"

#define TOUCH_POINT_NUMBER_MAX 10

/* Touch event */
#define RT_TOUCH_EVENT_NONE              (0)   /* Touch none */
#define RT_TOUCH_EVENT_UP                (1)   /* Touch up event */
#define RT_TOUCH_EVENT_DOWN              (2)   /* Touch down event */
#define RT_TOUCH_EVENT_MOVE              (3)   /* Touch move event */

#define TOUCH_ROTATE_DEGREE_0           (0)
#define TOUCH_ROTATE_DEGREE_90          (1)
#define TOUCH_ROTATE_DEGREE_180         (2)
#define TOUCH_ROTATE_DEGREE_270         (3)

struct rt_touch_data {
    uint8_t event; /* The touch event of the data */
    uint8_t track_id; /* Track id of point */
    uint8_t width; /* Point of width */
    uint16_t x_coordinate; /* Point of x coordinate */
    uint16_t y_coordinate; /* Point of y coordinate */
    uint32_t timestamp; /* The timestamp when the data was received */
};

struct rt_touch_info
{
    uint8_t      type;                       /* The touch type */
    uint8_t      vendor;                     /* Vendor of touchs */
    uint8_t      point_num;                  /* Support point num */
    uint32_t     range_x;                    /* X coordinate range */
    uint32_t     range_y;                    /* Y coordinate range */
};

typedef struct {
    mp_obj_base_t base;
    int fd;
    uint8_t index;
    uint8_t rotate;
    uint32_t range_x, range_y;
    bool valid;
} machine_touch_obj_t;

typedef struct {
    mp_obj_base_t base;
    struct rt_touch_data info;
} machine_touch_info_obj_t;

static inline void rotate_touch_point(machine_touch_obj_t *self, struct rt_touch_data *tdata)
{
    uint8_t rotate = self->rotate;
    uint32_t range_x = self->range_x, range_y = self->range_y;
    uint16_t point_x = tdata->x_coordinate, point_y = tdata->y_coordinate;

    switch (rotate) {
        case TOUCH_ROTATE_DEGREE_0:
        default:
            // do nothing
        break;
        case TOUCH_ROTATE_DEGREE_90:
            tdata->x_coordinate = range_y - point_y - 1;
            tdata->y_coordinate = point_x;
        break;
        case TOUCH_ROTATE_DEGREE_180:
            tdata->x_coordinate = range_x - point_x - 1;
            tdata->y_coordinate = range_y - point_y - 1;
        break;
        case TOUCH_ROTATE_DEGREE_270:
            tdata->x_coordinate = point_y;
            tdata->y_coordinate = range_x - point_x - 1;
        break;
    }
}

STATIC void machine_touch_info_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_touch_info_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "track_id: %d, event: %d, width: %d, x: %d, y: %d, timestamp: %d",
        self->info.event, self->info.track_id, self->info.width, self->info.x_coordinate,
        self->info.y_coordinate, self->info.timestamp);
}

STATIC void machine_touch_info_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    machine_touch_info_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (dest[0] == MP_OBJ_NULL) {
        switch (attr) {
            case MP_QSTR_event:
                dest[0] = mp_obj_new_int(self->info.event);
                break;
            case MP_QSTR_track_id:
                dest[0] = mp_obj_new_int(self->info.track_id);
                break;
            case MP_QSTR_width:
                dest[0] = mp_obj_new_int(self->info.width);
                break;
            case MP_QSTR_x:
                dest[0] = mp_obj_new_int(self->info.x_coordinate);
                break;
            case MP_QSTR_y:
                dest[0] = mp_obj_new_int(self->info.y_coordinate);
                break;
            case MP_QSTR_timestamp:
                dest[0] = mp_obj_new_int(self->info.timestamp);
                break;
        }
    }
}

MP_DEFINE_CONST_OBJ_TYPE(
    machine_touch_info_type,
    MP_QSTR_TOUCH_INFO,
    MP_TYPE_FLAG_NONE,
    print, machine_touch_info_print,
    attr, machine_touch_info_attr
    );

STATIC void machine_touch_obj_check(machine_touch_obj_t *self) {
    if (self->valid == 0) {
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("The TOUCH object has been deleted"));
    }
}

STATIC void machine_touch_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_touch_obj_t *self = MP_OBJ_TO_PTR(self_in);
    machine_touch_obj_check(self);
    mp_printf(print, "TOUCH: %d", self->index);
}

STATIC mp_obj_t machine_touch_deinit(mp_obj_t self_in) {
    machine_touch_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->valid == 0) {
        return mp_const_none;
    }
    close(self->fd);
    self->valid = 0;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_touch_deinit_obj, machine_touch_deinit);

STATIC mp_obj_t machine_touch_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_dev, ARG_rotation };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_dev, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_rotation, MP_ARG_INT, {.u_int = TOUCH_ROTATE_DEGREE_270} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    int index = args[ARG_dev].u_int;
    if (index < 0 || index > 9) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid touch device number"));
    }

    char dev_name[16] = "/dev/touch0";
    dev_name[10] = '0' + index;
    int fd = open(dev_name, O_RDWR);
    if (fd < 0) {
        mp_raise_OSError_with_filename(errno, dev_name);
    }

#define RT_TOUCH_CTRL_GET_INFO (21 * 0x100 + 1)
    struct rt_touch_info info;

    if(0x00 != ioctl(fd, RT_TOUCH_CTRL_GET_INFO, &info)) {
        mp_raise_OSError_with_filename(errno, dev_name);
    }

    machine_touch_obj_t *self = m_new_obj_with_finaliser(machine_touch_obj_t);
    self->base.type = &machine_touch_type;
    self->index = index;
    self->fd = fd;
    self->valid = true;
    self->range_x = info.range_x;
    self->range_y = info.range_y;
    self->rotate = (args[ARG_rotation].u_int) & 0x03;

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t machine_touch_read(size_t n_args, const mp_obj_t *args) {
    machine_touch_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int point_number = 0;

    machine_touch_obj_check(self);
    if (n_args > 1) {
        point_number = mp_obj_get_int(args[1]);
        if (point_number < 0 || point_number > TOUCH_POINT_NUMBER_MAX) {
            mp_raise_ValueError(MP_ERROR_TEXT("point_number invalid"));
        }
    }
    if (point_number == 0) {
        point_number = TOUCH_POINT_NUMBER_MAX;
    }
    struct rt_touch_data touch_data[point_number];
    point_number = read(self->fd, touch_data, sizeof(touch_data));
    if (point_number < 0) {
        mp_raise_OSError(errno);
    }
    point_number /= sizeof(struct rt_touch_data);
    machine_touch_info_obj_t *touch_info_obj[point_number];
    for (int i = 0; i < point_number; i++) {
        rotate_touch_point(self, &touch_data[i]);

        touch_info_obj[i] = mp_obj_malloc(machine_touch_info_obj_t, &machine_touch_info_type);
        touch_info_obj[i]->info = touch_data[i];
    }

    return mp_obj_new_tuple(point_number, (mp_obj_t *)touch_info_obj);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_touch_read_obj, 1, 2, machine_touch_read);

STATIC const mp_rom_map_elem_t machine_touch_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&machine_touch_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_touch_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&machine_touch_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_EVENT_NONE), MP_ROM_INT(RT_TOUCH_EVENT_NONE) },
    { MP_ROM_QSTR(MP_QSTR_EVENT_UP), MP_ROM_INT(RT_TOUCH_EVENT_UP) },
    { MP_ROM_QSTR(MP_QSTR_EVENT_DOWN), MP_ROM_INT(RT_TOUCH_EVENT_DOWN) },
    { MP_ROM_QSTR(MP_QSTR_EVENT_MOVE), MP_ROM_INT(RT_TOUCH_EVENT_MOVE) },

    { MP_ROM_QSTR(MP_QSTR_ROTATE_0), MP_ROM_INT(TOUCH_ROTATE_DEGREE_0) },
    { MP_ROM_QSTR(MP_QSTR_ROTATE_90), MP_ROM_INT(TOUCH_ROTATE_DEGREE_90) },
    { MP_ROM_QSTR(MP_QSTR_ROTATE_180), MP_ROM_INT(TOUCH_ROTATE_DEGREE_180) },
    { MP_ROM_QSTR(MP_QSTR_ROTATE_270), MP_ROM_INT(TOUCH_ROTATE_DEGREE_270) },
};
STATIC MP_DEFINE_CONST_DICT(machine_touch_locals_dict, machine_touch_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    machine_touch_type,
    MP_QSTR_TOUCH,
    MP_TYPE_FLAG_NONE,
    make_new, machine_touch_make_new,
    print, machine_touch_print,
    locals_dict, &machine_touch_locals_dict
    );
