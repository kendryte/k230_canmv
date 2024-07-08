/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Damien P. George
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

#include "py/runtime.h"
#include "py/mphal.h"
#include "extmod/modnetwork.h"
#include "ipcm_socket_network.h"


#if defined(MICROPY_PY_NETWORK_CANMV)
typedef struct _network_lan_obj_t {
    mp_obj_base_t base;
    char  *eth;
} network_lan_obj_t;

extern const struct _mp_obj_type_t network_lan_type;

STATIC const network_lan_obj_t network_lan_eth0 = { { &network_lan_type }, "eth0" };


STATIC void network_lan_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    //network_lan_obj_t *self = MP_OBJ_TO_PTR(self_in);
    // struct netif *netif = eth_netif(self->eth);
    // int status = eth_link_status(self->eth);
    // mp_printf(print, "<ETH%d status=%u ip=%u.%u.%u.%u>",
    //     self->eth == NULL ? 0 : 1,
    //     status,
    //     netif->ip_addr.addr & 0xff,
    //     netif->ip_addr.addr >> 8 & 0xff,
    //     netif->ip_addr.addr >> 16 & 0xff,
    //     netif->ip_addr.addr >> 24
    //     );
}

STATIC mp_obj_t network_lan_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    // int mac_id = args[ARG_id].u_int;
    const network_lan_obj_t *self;   

    self = &network_lan_eth0;
    return MP_OBJ_FROM_PTR(self);
}
/*
ifconfig eth0 up/down;
ifconfig 
*/
STATIC mp_obj_t network_lan_active(size_t n_args, const mp_obj_t *args) {
    int ret = 0;
    char cmd[128];
    network_lan_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    if (n_args == 1) {//if active 
        snprintf(cmd, sizeof(cmd),"ifconfig %s >/dev/null", self->eth);
    } else {
        //int ret;
        if (mp_obj_is_true(args[1])) { //up 
            snprintf(cmd, sizeof(cmd), "ifconfig %s up >/dev/null", self->eth);
        } else { //down 
            snprintf(cmd, sizeof(cmd),"ifconfig %s down >/dev/null", self->eth);
        }
    }
    ret = ipcm_network_exe_cmd(cmd, NULL, 0);
    return mp_obj_new_bool(ret == 0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(network_lan_active_obj, 1, 2, network_lan_active);

STATIC mp_obj_t network_lan_isconnected(mp_obj_t self_in) {
    int ret = 0;
    char cmd[128];
    network_lan_obj_t *self = MP_OBJ_TO_PTR(self_in);
   
    snprintf(cmd, sizeof(cmd), "ifconfig %s |grep RUNNING >/dev/null", self->eth);
    ret = ipcm_network_exe_cmd(cmd, NULL, 0);
    return mp_obj_new_bool(ret == 0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(network_lan_isconnected_obj, network_lan_isconnected);

STATIC mp_obj_t network_lan_ifconfig(size_t n_args, const mp_obj_t *args) {
    network_lan_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    return ipcm_network_lan_ifconfig(self->eth, n_args - 1, args + 1);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(network_lan_ifconfig_obj, 1, 2, network_lan_ifconfig);

STATIC mp_obj_t network_lan_status(size_t n_args, const mp_obj_t *args) {
    char cmd[128];
    int ret;
    network_lan_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    (void)self;    

    if (n_args == 1) {
        snprintf(cmd, sizeof(cmd),"ifconfig %s >/dev/null", self->eth);
        ret = ipcm_network_exe_cmd(cmd, NULL, 0);
        // No arguments: return link status
        return MP_OBJ_NEW_SMALL_INT(ret == 0);
    }

    mp_raise_ValueError(MP_ERROR_TEXT("unknown status param"));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(network_lan_status_obj, 1, 2, network_lan_status);

STATIC mp_obj_t network_lan_config(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    network_lan_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    char cmd[128];
    char buf[128];
    int ret;

    if (kwargs->used == 0) {
        // // Get config value
        if (n_args != 2) 
            mp_raise_TypeError(MP_ERROR_TEXT("must query one param"));
        
        switch (mp_obj_str_get_qstr(args[1])) {
            case MP_QSTR_mac:{//
                snprintf(cmd, sizeof(cmd),"ifconfig %s | grep HWaddr | awk '{print $5}'", self->eth);
                ret = ipcm_network_exe_cmd(cmd, buf, sizeof(buf));
                if(ret == 0)
                    return mp_obj_new_str(buf, strlen(buf));
            break;
            }
            default:
                mp_raise_ValueError(MP_ERROR_TEXT("unknown config param"));
        }
    } else {
        // Set config value(s)
        if (n_args != 1) {
            mp_raise_TypeError(MP_ERROR_TEXT("can't specify pos and kw args"));
        }

        for (size_t i = 0; i < kwargs->alloc; ++i) {
            if (MP_MAP_SLOT_IS_FILLED(kwargs, i)) {
                mp_map_elem_t *e = &kwargs->table[i];
                switch (mp_obj_str_get_qstr(e->key)) {
                    case MP_QSTR_mac: {
                        mp_buffer_info_t bufinfo;
                        mp_get_buffer_raise(kwargs->table[i].value, &bufinfo, MP_BUFFER_READ);
                        snprintf(cmd, sizeof(cmd),
                                "ifconfig %s down; ifconfig %s hw ether %s; ifconfig %s up;",
                                             self->eth,self->eth, (char *)bufinfo.buf,self->eth);
                        ret = ipcm_network_exe_cmd(cmd, buf, sizeof(buf));
                        if(ret == 0)
                            return mp_const_true;
                    break;
                    }
                    default:
                        mp_raise_ValueError(MP_ERROR_TEXT("unknown config param"));
                }
            }
        }
    }
    return mp_const_false;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(network_lan_config_obj, 1, network_lan_config);

STATIC const mp_rom_map_elem_t network_lan_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_active), MP_ROM_PTR(&network_lan_active_obj) },
    { MP_ROM_QSTR(MP_QSTR_isconnected), MP_ROM_PTR(&network_lan_isconnected_obj) },
    { MP_ROM_QSTR(MP_QSTR_ifconfig), MP_ROM_PTR(&network_lan_ifconfig_obj) },
    { MP_ROM_QSTR(MP_QSTR_status), MP_ROM_PTR(&network_lan_status_obj) },
    { MP_ROM_QSTR(MP_QSTR_config), MP_ROM_PTR(&network_lan_config_obj) },
    //{ MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&machine_timer_deinit_obj) },
};
STATIC MP_DEFINE_CONST_DICT(network_lan_locals_dict, network_lan_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    network_lan_type,
    MP_QSTR_LAN,
    MP_TYPE_FLAG_NONE,
    make_new, network_lan_make_new,
    print, network_lan_print,
    locals_dict, &network_lan_locals_dict
    );


#endif // defined(MICROPY_PY_NETWORK_CANMV)
