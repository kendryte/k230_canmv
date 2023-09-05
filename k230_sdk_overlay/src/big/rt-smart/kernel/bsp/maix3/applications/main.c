/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 */

#include <rtthread.h>
#include <rthw.h>
#include <stdio.h>
#include <string.h>

#if CONFIG_ENABLE_USB_DEVICE
#include "usbd_core.h"
#include "cdc_acm_msc_template.c"
#endif

extern rt_device_t sdcard_device;

int main(void)
{
    printf("RT-SMART Hello RISC-V\n");
#if CONFIG_ENABLE_USB_DEVICE
    printf("CherryUSB device cdc msc example\n");

    extern void cdc_acm_msc_init(void);
    cdc_acm_msc_init();

    // Wait until configured
    while (!usb_device_is_configured())
    {
        rt_thread_delay(10);
    }
#endif
    return 0;
}
