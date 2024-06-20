/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 * and Mnemote Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016, 2017 Nick Moore @mnemote
 * Copyright (c) 2017 "Eric Poulsen" <eric@zyxod.com>
 *
 * Based on esp8266/modnetwork.c which is Copyright (c) 2015 Paul Sokolovsky
 * And the ESP IDF example code which is Public Domain / CC0
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

#include <string.h>

#include "py/objlist.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "extmod/modnetwork.h"
#include "ipcm_socket_network.h"

#ifndef MICROPY_PY_NETWORK_CANMV
#define MICROPY_PY_NETWORK_CANMV 1
#endif 

#if MICROPY_PY_NETWORK_CANMV


typedef struct _base_if_obj_t {
    mp_obj_base_t base;
    char *name;
} wlan_if_obj_t;

extern const struct _mp_obj_type_t network_wlan_type;
#define WPA_SUPPLICANT        "ps | grep -v grep | grep  wpa_supplicant >/dev/null " \
                                "|| (wpa_supplicant -D nl80211 -i wlan0 -c /etc/wpa_supplicant.conf -B  >/dev/null \\&  ; sleep 5;); "

#define CHECK_STA_MODE       if(strcmp(self->name , "wlan0")) \
                                mp_raise_ValueError(MP_ERROR_TEXT("current interface is  ap mode!"));            
#define WPA_SUPPLICANT_ERROR "wpa_supplicant failed!\n"

#define CHECK_WPA_SUPPLICANT  do { \
    snprintf(cmd, sizeof(cmd), WPA_SUPPLICANT);    \
    if(ipcm_network_exe_cmd(cmd, NULL, 0)) \
        return mp_obj_new_str(WPA_SUPPLICANT_ERROR, strlen(WPA_SUPPLICANT_ERROR));} while(0)

STATIC mp_obj_t network_wlan_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    int idx = MOD_NETWORK_STA_IF;
    wlan_if_obj_t *self = NULL;
    mp_arg_check_num(n_args, n_kw, 0, 1, false);
    if (n_args > 0) {
        idx = mp_obj_get_int(args[0]);
        if ( (idx != MOD_NETWORK_STA_IF)  &&  (idx != MOD_NETWORK_AP_IF) ) {
            mp_raise_ValueError(NULL);
        }
    }    
    self = mp_obj_malloc(wlan_if_obj_t, &network_wlan_type); //会自己释放；
    (idx == MOD_NETWORK_STA_IF ) ? (self->name = "wlan0") : (self->name = "wlan1" );
    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t network_wlan_active(size_t n_args, const mp_obj_t *args) {   
    int ret = 0;
    char cmd[128];
    char cmd_recv[128];
    wlan_if_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    if (n_args == 1) {//if active 
        snprintf(cmd, sizeof(cmd),"ifconfig | grep %s 2>&1 ", self->name);
    } else {

        if (mp_obj_is_true(args[1])) { //up 
            snprintf(cmd, sizeof(cmd), "ifconfig %s up 2>&1; ifconfig %s up 2>&1;", self->name, self->name);
        } else { //down 
            snprintf(cmd, sizeof(cmd),"ifconfig %s down 2>&1", self->name);
        }
    }
    ret = ipcm_network_exe_cmd(cmd, cmd_recv, sizeof(cmd_recv));
    if((ret == 0) || !strlen(cmd_recv) )
        return mp_obj_new_bool(ret == 0);
    else 
        return mp_obj_new_str(cmd_recv, strlen(cmd_recv));         
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(network_wlan_active_obj, 1, 2, network_wlan_active);

STATIC mp_obj_t network_wlan_connect(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    char cmd[256];  
    char cmd_recv[128];
    size_t len;  
    wlan_if_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    int net_id = 1;

    enum { ARG_ssid, ARG_key, ARG_bssid };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_, MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_, MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_bssid, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    
    CHECK_STA_MODE;
    CHECK_WPA_SUPPLICANT;

    snprintf(cmd, sizeof(cmd), "wpa_cli -i wlan0 add_network;");    
    if(ipcm_network_exe_cmd(cmd, cmd_recv, sizeof(cmd_recv)))
        return mp_obj_new_bool(false);

    net_id = atoi(cmd_recv);

    snprintf(cmd, sizeof(cmd),"wpa_cli -i wlan0 set_network %d ssid '\"%s\"';"
                              "wpa_cli -i wlan0 set_network %d psk '\"%s\"';"
                              ,net_id,mp_obj_str_get_data(args[ARG_ssid].u_obj, &len) 
                              ,net_id,mp_obj_str_get_data(args[ARG_key].u_obj, &len) );
    //printf("f=%s l=%d cmd=%s\n", __func__, __LINE__, cmd);
    ipcm_network_exe_cmd(cmd, NULL, 0);

    if (args[ARG_bssid].u_obj != mp_const_none){
        snprintf(cmd, sizeof(cmd),"wpa_cli -i wlan0 set_network %d bssid '\"%s\"';"
                            ,net_id,mp_obj_str_get_data(args[ARG_bssid].u_obj, &len));
        printf("f=%s l=%d cmd=%s\n", __func__, __LINE__, cmd);
        ipcm_network_exe_cmd(cmd, NULL, 0);
    }

    snprintf(cmd, sizeof(cmd),  "wpa_cli -i wlan0 select_network %d && "
                                " udhcpc -i wlan0 -q -n ;ifmetric wlan0 100;", net_id);
    //printf("f=%s l=%d cmd=%s\n", __func__, __LINE__, cmd);
    ipcm_network_exe_cmd(cmd, NULL, 0);
    return mp_obj_new_bool(true);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(network_wlan_connect_obj, 1, network_wlan_connect);

STATIC mp_obj_t network_wlan_disconnect(mp_obj_t self_in) {    
    char cmd[256];  
    
    wlan_if_obj_t *self = MP_OBJ_TO_PTR(self_in);
    CHECK_STA_MODE;
    CHECK_WPA_SUPPLICANT;

    snprintf(cmd, sizeof(cmd), "wpa_cli  disconnect -i wlan0;");
    //printf("f=%s l=%d cmd=%s\n", __func__, __LINE__, cmd);
    ipcm_network_exe_cmd(cmd, NULL, 0);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(network_wlan_disconnect_obj, network_wlan_disconnect);

STATIC mp_obj_t network_wlan_status(size_t n_args, const mp_obj_t *args) {
    char cmd[256]; 
    char cmd_recv[1024];

    wlan_if_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if(strcmp(self->name, "wlan0") == 0){
        CHECK_WPA_SUPPLICANT;
        snprintf(cmd, sizeof(cmd), "wpa_cli -i wlan0 status;");  
    }else{
        snprintf(cmd, sizeof(cmd), "hostapd_cli status;");  
    }
      
    ipcm_network_exe_cmd(cmd, cmd_recv, sizeof(cmd_recv));
    //printf("f=%s l=%d cmd=%s , recv=%s \n", __func__, __LINE__, cmd,cmd_recv);
    return mp_obj_new_str(cmd_recv, strlen(cmd_recv));        
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(network_wlan_status_obj, 1, 2, network_wlan_status);
STATIC mp_obj_t network_wlan_scan(mp_obj_t self_in) {
    char cmd[256]; 
    char cmd_recv[1024];

    wlan_if_obj_t *self = MP_OBJ_TO_PTR(self_in);
    CHECK_STA_MODE;
    CHECK_WPA_SUPPLICANT;
    
    snprintf(cmd, sizeof(cmd), " wpa_cli -i wlan0 scan; wpa_cli -i wlan0 scan_result;");
    ipcm_network_exe_cmd(cmd, cmd_recv, sizeof(cmd_recv));
    return mp_obj_new_str(cmd_recv, strlen(cmd_recv));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(network_wlan_scan_obj, network_wlan_scan);
STATIC mp_obj_t network_wlan_isconnected(mp_obj_t self_in) {
    char cmd[256]; 
    char cmd_recv[256];
    int ret = 0;

    wlan_if_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if(strcmp(self->name , "wlan0") == 0 ){
        CHECK_WPA_SUPPLICANT;
        snprintf(cmd, sizeof(cmd), "wpa_cli -i wlan0 status | grep wpa_state=COMPLETED && wpa_cli -i wlan0 status | grep ip_address");        
    }else {
        snprintf(cmd, sizeof(cmd), "hostapd_cli status | grep 'num_sta\\[[0-9]\\]=[1-9][0-9]*'");
    }
    ret = ipcm_network_exe_cmd(cmd, cmd_recv, sizeof(cmd_recv));
    return mp_obj_new_bool(ret == 0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(network_wlan_isconnected_obj, network_wlan_isconnected);
STATIC mp_obj_t network_wlan_ifconfig(size_t n_args, const mp_obj_t *args) {
    wlan_if_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    return ipcm_network_lan_ifconfig(self->name, n_args - 1, args + 1);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(network_wlan_ifconfig_obj, 1, 2, network_wlan_ifconfig);
#define AP_CONF "/etc/hostapd.conf"
#define AP_BUILD_CMD(K)  do{ \
    snprintf(cmd, sizeof(cmd),"cat /etc/hostapd.conf | grep  ^%s= | cut -d= -f2",(K)); \
    }while(0)
#define SET_BUF_VALUE(V) do{\
    snprintf(buf, sizeof(buf), "%s", (V));\
    }while(0)

STATIC mp_obj_t network_wlan_config_get(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs){
    wlan_if_obj_t *self = MP_OBJ_TO_PTR(args[0]);
     // Get config
    char cmd[256];
    char buf[512];

    buf[0]=0;
    cmd[0]=0;
    
    if(n_args == 1){
        snprintf(cmd, sizeof(cmd),"cat %s", AP_CONF );
        ipcm_network_exe_cmd(cmd, buf, sizeof(buf));
        return mp_obj_new_str(buf, strlen(buf));
    }

    switch (mp_obj_str_get_qstr(args[1])) {
    case MP_QSTR_mac: 
        snprintf(cmd, sizeof(cmd),"ifconfig %s | grep HWaddr | awk '{print $5}'", self->name);break;
    case MP_QSTR_ssid://
    case MP_QSTR_essid:
        if(strcmp(self->name, "wlan0") == 0 )
            snprintf(cmd, sizeof(cmd),"wpa_cli -i wlan0 status | grep  ^ssid= | cut -d= -f2");
        else
            AP_BUILD_CMD("ssid");             
        break;
    case MP_QSTR_hidden:AP_BUILD_CMD("ignore_broadcast_ssid");break;
    case MP_QSTR_security:
    case MP_QSTR_authmode:SET_BUF_VALUE("WPA2");break;//AP_BUILD_CMD("auth_algs");break;
    case MP_QSTR_channel:AP_BUILD_CMD("channel");break;
    case MP_QSTR_ifname: SET_BUF_VALUE(self->name);break;
    case MP_QSTR_hostname:
    case MP_QSTR_dhcp_hostname: snprintf(cmd, sizeof(cmd),"hostname"); break;
    case MP_QSTR_max_clients: AP_BUILD_CMD("channel");break;
    case MP_QSTR_reconnects:SET_BUF_VALUE("unknown");break;
    case MP_QSTR_txpower: SET_BUF_VALUE("unknown");break;
    case MP_QSTR_protocol: SET_BUF_VALUE("unknown");break;
    case MP_QSTR_pm: SET_BUF_VALUE("unknown");break;
    default:SET_BUF_VALUE("unknown");break;
    }

    if(strlen(cmd) > 0){
        ipcm_network_exe_cmd(cmd, buf, sizeof(buf));
    }
    return mp_obj_new_str(buf, strlen(buf));
}

#define AP_CONF_SET_STR(K) do{ \
            snprintf(cmd, sizeof(cmd), \
                    "sed -i -e '/^%s=/d' %s; echo '%s=%s' >> %s;",  \
                        K, AP_CONF,K, mp_obj_str_get_data(kwargs->table[i].value, &tmp), AP_CONF); \
            ret = ipcm_network_exe_cmd(cmd, buf, sizeof(buf)); \
            need_reset_hostap_flage++;\
            }while(0)
            
#define AP_CONF_SET_INT(K) do{ \
            snprintf(cmd, sizeof(cmd), \
                    "sed -i -e '/^%s=/d' %s; echo '%s=%d' >> %s;",  \
                        K, AP_CONF,K, (int)mp_obj_get_int(kwargs->table[i].value), AP_CONF);\
            ret = ipcm_network_exe_cmd(cmd, buf, sizeof(buf)); \
            need_reset_hostap_flage++;\
        }while(0)

#define AP_CONF_SET_UNSUPPORT(K) do{ \
            snprintf(buf, sizeof(buf),"unsurpoort %s", K);\
            return mp_obj_new_str(buf, strlen(buf)); \
        }while(0)

STATIC mp_obj_t network_wlan_config_set(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs){
    char cmd[256];
    char buf[256];
    int ret = -1;
    size_t tmp;
    char need_reset_hostap_flage = 0;
    wlan_if_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    for (size_t i = 0; i < kwargs->alloc; i++) {
        if (!mp_map_slot_is_filled(kwargs, i))
            continue;

        switch (mp_obj_str_get_qstr(kwargs->table[i].key)) {
            case MP_QSTR_mac: 
                if(strcmp(self->name, "wlan0") == 0){
                    snprintf(cmd, sizeof(cmd),"ifconfig wlan0 down; "                        
                            "ifconfig wlan0 hw ether %s; ifconfig wlan0 up;",
                                mp_obj_str_get_data(kwargs->table[i].value, &tmp));
                    ret = ipcm_network_exe_cmd(cmd, buf, sizeof(buf));
                }else {
                    AP_CONF_SET_STR("bssid");
                }
                break;
            case MP_QSTR_ssid: 
            case MP_QSTR_essid: AP_CONF_SET_STR("ssid");break;
            case MP_QSTR_hidden: AP_CONF_SET_INT("ignore_broadcast_ssid");break;
            case MP_QSTR_security:
            case MP_QSTR_authmode: AP_CONF_SET_UNSUPPORT("authmode");break; //AP_CONF_SET_INT("auth_algs");break;
            case MP_QSTR_key:
            case MP_QSTR_password: AP_CONF_SET_STR("wpa_passphrase");break;
            case MP_QSTR_channel: AP_CONF_SET_INT("channel");break;
            case MP_QSTR_hostname:
            case MP_QSTR_dhcp_hostname: 
                snprintf(cmd, sizeof(cmd),"hostname %s",mp_obj_str_get_data(kwargs->table[i].value, &tmp));
                ret = ipcm_network_exe_cmd(cmd, buf, sizeof(buf));
                break;
          
            case MP_QSTR_max_clients: AP_CONF_SET_INT("max_num_sta");break;
            case MP_QSTR_reconnects: AP_CONF_SET_UNSUPPORT("reconnects");break;
            case MP_QSTR_txpower: AP_CONF_SET_UNSUPPORT("txpower");break;
            case MP_QSTR_protocol: AP_CONF_SET_UNSUPPORT("protocol");break;
            case MP_QSTR_pm: AP_CONF_SET_UNSUPPORT("pm");break; 
            default:break;
        }
    }
    
    if(need_reset_hostap_flage > 0){
        snprintf(cmd, sizeof(cmd), "killall hostapd;killall udhcpd ; sleep 1;"
                    "ifconfig wlan1 192.168.1.1 up; hostapd /etc/hostapd.conf -B; udhcpd /etc/udhcpd.conf \\& ;");
        ret = ipcm_network_exe_cmd(cmd, buf, sizeof(buf));
    }
    return mp_obj_new_bool(ret == 0);
}


STATIC mp_obj_t network_wlan_config(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    if (n_args != 1 && n_args != 2  && kwargs->used != 0) {
        mp_raise_TypeError(MP_ERROR_TEXT("either pos or kw args are allowed"));
    }

    if (kwargs->used != 0) { //set param        
        return network_wlan_config_set(n_args,args,kwargs);
    }else {
        return network_wlan_config_get(n_args,args,kwargs);
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(network_wlan_config_obj, 1, network_wlan_config);

STATIC const mp_rom_map_elem_t wlan_if_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_active), MP_ROM_PTR(&network_wlan_active_obj) },
    { MP_ROM_QSTR(MP_QSTR_connect), MP_ROM_PTR(&network_wlan_connect_obj) },// sta 连接wifi；
    { MP_ROM_QSTR(MP_QSTR_disconnect), MP_ROM_PTR(&network_wlan_disconnect_obj) },
    { MP_ROM_QSTR(MP_QSTR_status), MP_ROM_PTR(&network_wlan_status_obj) },
    { MP_ROM_QSTR(MP_QSTR_scan), MP_ROM_PTR(&network_wlan_scan_obj) },// only sta;
    { MP_ROM_QSTR(MP_QSTR_isconnected), MP_ROM_PTR(&network_wlan_isconnected_obj) },
    { MP_ROM_QSTR(MP_QSTR_config), MP_ROM_PTR(&network_wlan_config_obj) },// 配置ap
    { MP_ROM_QSTR(MP_QSTR_ifconfig), MP_ROM_PTR(&network_wlan_ifconfig_obj) },

    // Constants
    // { MP_ROM_QSTR(MP_QSTR_PM_NONE), MP_ROM_INT(WIFI_PS_NONE) },
    // { MP_ROM_QSTR(MP_QSTR_PM_PERFORMANCE), MP_ROM_INT(WIFI_PS_MIN_MODEM) },
    // { MP_ROM_QSTR(MP_QSTR_PM_POWERSAVE), MP_ROM_INT(WIFI_PS_MAX_MODEM) },
};
STATIC MP_DEFINE_CONST_DICT(wlan_if_locals_dict, wlan_if_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    network_wlan_type,
    MP_QSTR_WLAN,
    MP_TYPE_FLAG_NONE,
    make_new, network_wlan_make_new,
    locals_dict, &wlan_if_locals_dict
    );

#endif // MICROPY_PY_NETWORK_CANMV
