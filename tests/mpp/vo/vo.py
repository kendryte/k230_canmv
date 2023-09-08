from mpp.vo import *
from mpp import *
from time import *

def display_hardware_init():
    kd_display_set_backlight()
    kd_display_reset()

hx8399 = k_vo_display_resolution(
    pclk = 74250,
    phyclk = 445500,
    htotal = 1240,
    hdisplay = 1080,
    hsync_len = 20,
    hback_porch = 20,
    hfront_porch = 120,
    vtotal = 1988,
    vdisplay = 1920,
    vsync_len = 5,
    vback_porch = 8,
    vfront_porch = 55,
)

def hx8399_v2_init(test_mode_en):
    param1 = bytes((0xB9, 0xFF, 0x83, 0x99))
    param21 = bytes((0xD2, 0xAA))
    param2 = bytes((0xB1, 0x02, 0x04, 0x71, 0x91, 0x01, 0x32, 0x33, 0x11, 0x11, 0xab, 0x4d, 0x56, 0x73, 0x02, 0x02))
    param3 = bytes((0xB2, 0x00, 0x80, 0x80, 0xae, 0x05, 0x07, 0x5a, 0x11, 0x00, 0x00, 0x10, 0x1e, 0x70, 0x03, 0xd4))
    param4 = bytes((0xB4, 0x00, 0xFF, 0x02, 0xC0, 0x02, 0xc0, 0x00, 0x00, 0x08, 0x00, 0x04, 0x06, 0x00, 0x32, 0x04, 0x0a, 0x08, 0x21, 0x03, 0x01, 0x00, 0x0f, 0xb8, 0x8b, 0x02, 0xc0, 0x02, 0xc0, 0x00, 0x00, 0x08, 0x00, 0x04, 0x06, 0x00, 0x32, 0x04, 0x0a, 0x08, 0x01, 0x00, 0x0f, 0xb8, 0x01))
    param5 = bytes((0xD3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x10, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x05, 0x05, 0x07, 0x00, 0x00, 0x00, 0x05, 0x40))
    param6 = bytes((0xD5, 0x18, 0x18, 0x19, 0x19, 0x18, 0x18, 0x21, 0x20, 0x01, 0x00, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x2f, 0x2f, 0x30, 0x30, 0x31, 0x31, 0x18, 0x18, 0x18, 0x18))
    param7 = bytes((0xD6, 0x18, 0x18, 0x19, 0x19, 0x40, 0x40, 0x20, 0x21, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x00, 0x01, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x2f, 0x2f, 0x30, 0x30, 0x31, 0x31, 0x40, 0x40, 0x40, 0x40))
    param8 = bytes((0xD8, 0xa2, 0xaa, 0x02, 0xa0, 0xa2, 0xa8, 0x02, 0xa0, 0xb0, 0x00, 0x00, 0x00, 0xb0, 0x00, 0x00, 0x00))
    param9 = bytes((0xBD, 0x01))
    param10 = bytes((0xD8, 0xB0, 0x00, 0x00, 0x00, 0xB0, 0x00, 0x00, 0x00, 0xE2, 0xAA, 0x03, 0xF0, 0xE2, 0xAA, 0x03, 0xF0))
    param11 = bytes((0xBD, 0x02))
    param12 = bytes((0xD8, 0xE2, 0xAA, 0x03, 0xF0, 0xE2, 0xAA, 0x03, 0xF0))
    param13 = bytes((0xBD, 0x00))
    param14 = bytes((0xB6, 0x8D, 0x8D))
    param15 = bytes((0xCC, 0x09))
    param16 = bytes((0xC6, 0xFF, 0xF9))
    param22 = bytes((0xE0, 0x00, 0x12, 0x1f, 0x1a, 0x40, 0x4a, 0x59, 0x55, 0x5e, 0x67, 0x6f, 0x75, 0x7a, 0x82, 0x8b, 0x90, 0x95, 0x9f, 0xa3, 0xad, 0xa2, 0xb2, 0xB6, 0x5e, 0x5a, 0x65, 0x77, 0x00, 0x12, 0x1f, 0x1a, 0x40, 0x4a, 0x59, 0x55, 0x5e, 0x67, 0x6f, 0x75, 0x7a, 0x82, 0x8b, 0x90, 0x95, 0x9f, 0xa3, 0xad, 0xa2, 0xb2, 0xB6, 0x5e, 0x5a, 0x65, 0x77))
    param23 = bytes((0x11,))
    param24 = bytes((0x29,))
    pag20 = bytes((0xB2, 0x0b, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77))

    kd_mpi_dsi_send_cmd(param1, len(param1))
    kd_mpi_dsi_send_cmd(param21, len(param21))
    kd_mpi_dsi_send_cmd(param2, len(param2))
    kd_mpi_dsi_send_cmd(param3, len(param3))
    kd_mpi_dsi_send_cmd(param4, len(param4))
    kd_mpi_dsi_send_cmd(param5, len(param5))
    kd_mpi_dsi_send_cmd(param6, len(param6))
    kd_mpi_dsi_send_cmd(param7, len(param7))
    kd_mpi_dsi_send_cmd(param8, len(param8))
    kd_mpi_dsi_send_cmd(param9, len(param9))

    if (test_mode_en):
        kd_mpi_dsi_send_cmd(pag20, 10)

    kd_mpi_dsi_send_cmd(param10, len(param10))
    kd_mpi_dsi_send_cmd(param11, len(param11))
    kd_mpi_dsi_send_cmd(param12, len(param12))
    kd_mpi_dsi_send_cmd(param13, len(param13))
    kd_mpi_dsi_send_cmd(param14, len(param14))
    kd_mpi_dsi_send_cmd(param15, len(param15))
    kd_mpi_dsi_send_cmd(param16, len(param16))
    kd_mpi_dsi_send_cmd(param22, len(param22))
    kd_mpi_dsi_send_cmd(param23, 1)
    sleep_ms(300)
    kd_mpi_dsi_send_cmd(param24, 1)
    sleep_ms(100)

def dwc_dsi_lpmode_test():
    enable = 1
    screen_test_mode = 1

    phy_attr = k_vo_mipi_phy_attr()
    phy_attr.phy_lan_num = K_DSI_4LAN
    phy_attr.m = 295
    phy_attr.n = 15
    phy_attr.voc = 0x17
    phy_attr.hs_freq = 0x96
    kd_mpi_set_mipi_phy_attr(phy_attr)

    attr = k_vo_dsi_attr()
    attr.lan_num = K_DSI_4LAN
    attr.cmd_mode = K_VO_LP_MODE
    attr.lp_div = 8
    struct_copy(hx8399, attr.resolution)
    kd_mpi_dsi_set_attr(attr)

    hx8399_v2_init(screen_test_mode)

    kd_mpi_dsi_enable(enable)

def dwc_dsi_hsmode_test():
    enable = 1
    screen_test_mode = 1

    phy_attr = k_vo_mipi_phy_attr()
    phy_attr.phy_lan_num = K_DSI_4LAN
    phy_attr.m = 295
    phy_attr.n = 15
    phy_attr.voc = 0x17
    phy_attr.hs_freq = 0x96
    kd_mpi_set_mipi_phy_attr(phy_attr)

    attr = k_vo_dsi_attr()
    attr.lan_num = K_DSI_4LAN
    attr.cmd_mode = K_VO_HS_MODE
    attr.lp_div = 8
    struct_copy(hx8399, attr.resolution)
    kd_mpi_dsi_set_attr(attr)

    hx8399_v2_init(screen_test_mode)

    kd_mpi_dsi_enable(enable)

def dwc_dsi_init_with_test_pattern():
    enable = 1
    screen_test_mode = 0

    phy_attr = k_vo_mipi_phy_attr()
    phy_attr.phy_lan_num = K_DSI_4LAN
    phy_attr.m = 295
    phy_attr.n = 15
    phy_attr.voc = 0x17
    phy_attr.hs_freq = 0x96
    kd_mpi_set_mipi_phy_attr(phy_attr)

    attr = k_vo_dsi_attr()
    attr.lan_num = K_DSI_4LAN
    attr.cmd_mode = K_VO_LP_MODE
    attr.lp_div = 8
    struct_copy(hx8399, attr.resolution)
    kd_mpi_dsi_set_attr(attr)

    hx8399_v2_init(screen_test_mode)

    kd_mpi_dsi_enable(enable)

    kd_mpi_dsi_set_test_pattern()

def dwc_dsi_init():
    enable = 1
    screen_test_mode = 0

    phy_attr = k_vo_mipi_phy_attr()
    phy_attr.phy_lan_num = K_DSI_4LAN
    phy_attr.m = 295
    phy_attr.n = 15
    phy_attr.voc = 0x17
    phy_attr.hs_freq = 0x96
    kd_mpi_set_mipi_phy_attr(phy_attr)

    attr = k_vo_dsi_attr()
    attr.lan_num = K_DSI_4LAN
    attr.cmd_mode = K_VO_LP_MODE
    attr.lp_div = 8
    struct_copy(hx8399, attr.resolution)
    kd_mpi_dsi_set_attr(attr)

    hx8399_v2_init(screen_test_mode)

    kd_mpi_dsi_enable(enable)

def vo_background_init():
    attr = k_vo_pub_attr()
    attr.bg_color = 0xffffff
    attr.intf_sync = K_VO_OUT_1080P30
    attr.intf_type = K_VO_INTF_MIPI
    attr.sync_info = struct_ptr(hx8399)

    kd_mpi_vo_init()

    kd_mpi_vo_set_dev_param(attr)

    kd_mpi_vo_enable()

def test_case(index):
    display_hardware_init()
    sleep_ms(200)

    if (index == 0):
        print("DISPLAY_DSI_LP_MODE_TEST ------------------ \n");
        dwc_dsi_lpmode_test()
    elif (index == 1):
        print("DISPLAY_DSI_HS_MODE_TEST ------------------ \n");
        dwc_dsi_hsmode_test()
    elif (index == 2):
        print("dwc_dsi_init_with_test_pattern ------------------ \n");
        dwc_dsi_init_with_test_pattern()
    elif (index == 3):
        print("DISPALY_VO_BACKGROUND_TEST ------------------ \n");
        dwc_dsi_init()
        vo_background_init()
