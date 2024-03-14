// #include "autoconf.h"

#include "dfs_file.h"
#include "dfs_poll.h"
#include "rtthread.h"
#include "rthw.h"

#include "cache.h"
#include "ioremap.h"
#include "riscv_io.h"

#include "usb_config.h"
#include "core/usbd_core.h"
#include "usb_dc.h"
#include <string.h>
#include <unistd.h>

uint32_t SystemCoreClock = 32000000 * 2U;   // for cherryusb dwc2 use
uintptr_t g_usb_otg_base = (uintptr_t)0x91500000UL;

static void dcd_int_handler_wrap(int irq, void *param)
{
    (void)irq;
    (void)param;

#if CONFIG_ENABLE_USB_HOST
    extern void USBH_IRQHandler(void);
    USBH_IRQHandler();
#elif CONFIG_ENABLE_USB_DEVICE
    extern void USBD_IRQHandler(void);
    USBD_IRQHandler();
#endif
}

#define CDC_IN_EP  0x81
#define CDC_OUT_EP 0x01
#define CDC_INT_EP 0x82

#define DEV_NAME "ttyUSB1"

struct rt_device g_cdc_rt_device;
volatile int g_cdc_mask = 0;
struct rt_semaphore cdc_read_sem, cdc_write_sem;
struct rt_mutex cdc_write_mutex;
extern volatile char ep_tx_busy_flag;
volatile size_t actual_bytes = 0;
volatile size_t actual_write = 0;
static volatile bool last_dtr = false;
extern uint32_t usb_read_buffer_ptr;
extern uint8_t usb_read_buffer[];

void usbd_cdc_acm_set_dtr(uint8_t intf, bool dtr) {
    // USB_LOG_WRN("set DTR %d\n", dtr);
    if (last_dtr != dtr && dtr == true) {
        //USB_LOG_WRN("RTS reset %d %d\n", last_rts, rts);
        g_cdc_mask |= POLLERR;
        rt_wqueue_wakeup(&g_cdc_rt_device.wait_queue, (void*)POLLERR);
    }
    last_dtr = dtr;
}

void usbd_cdc_acm_set_rts(uint8_t intf, bool rts) {
    // USB_LOG_WRN("set RTS %d\n", rts);
}

static int cdc_open(struct dfs_fd *fd) {
    return 0;
}

static int cdc_close(struct dfs_fd *fd) {
    return 0;
}
// FIXME: read block exit
static int cdc_read(struct dfs_fd *fd, void *buf, size_t count) {
    start:
    if (actual_bytes) {
        memcpy(buf, usb_read_buffer, actual_bytes);
        int tmp = actual_bytes;
        actual_bytes = 0;
        usbd_ep_start_read(CDC_OUT_EP, usb_read_buffer, 2048);
        return tmp;
    }
    rt_err_t error = rt_sem_take(&cdc_read_sem, 1000);
    if (error == RT_EOK) {
        goto start;
    } else if (error == RT_ETIMEOUT) {
        USB_LOG_WRN("read timeout\n");
        return 0;
    } else {
        return -1;
    }
}

static int cdc_write(struct dfs_fd *fd, const void *buf, size_t count) {
    // TODO FIX
    static rt_int32_t ms = 10;
    if (count == 0) {
        return 0;
    }
    rt_mutex_take(&cdc_write_mutex, RT_WAITING_FOREVER);
    rt_err_t error = rt_sem_take(&cdc_write_sem, rt_tick_from_millisecond(ms));
    if (error == RT_EOK) {
        usbd_ep_start_write(CDC_IN_EP, buf, count);
        ms = count / 1000;
        ms = ms < 10 ? 10 : ms;
    }
    rt_mutex_release(&cdc_write_mutex);
    // FIXME: async
    return error == RT_EOK ? count : -error;
}

static int cdc_poll(struct dfs_fd *fd, struct rt_pollreq *req) {
    rt_poll_add(&g_cdc_rt_device.wait_queue, req);
    int tmp = g_cdc_mask;
    g_cdc_mask = 0;
    return tmp;
}

static struct dfs_file_ops cdc_ops = {
    .open = cdc_open,
    .close = cdc_close,
    .read = cdc_read,
    .write = cdc_write,
    .poll = cdc_poll
};

volatile rt_device_t sdcard_device = RT_NULL;

void usb_dc_low_level_init(void)
{
    rt_device_t device = &g_cdc_rt_device;
    uint32_t *hs_reg = (uint32_t *)rt_ioremap((void *)(0x91585000 + 0x9C), 0x1000);
    *hs_reg = 0;
    rt_iounmap(hs_reg);

    rt_hw_interrupt_install(173, dcd_int_handler_wrap, NULL, "usb");
    rt_hw_interrupt_umask(173);

    rt_mutex_init(&cdc_write_mutex, "cdc/write", RT_IPC_FLAG_FIFO);

    // CDC uart
    rt_err_t err = rt_device_register(device, DEV_NAME, RT_DEVICE_OFLAG_RDWR);
    if (err != RT_EOK) {
        rt_kprintf("[usb] register device error\n");
        return;
    }
    device->fops = &cdc_ops;
}

void usb_dc_low_level_deinit(void)
{
    rt_hw_interrupt_mask(173);
    rt_sem_detach(&cdc_read_sem);
    rt_sem_detach(&cdc_write_sem);
    rt_mutex_detach(&cdc_write_mutex);
}

int usb_init(void)
{
#if CONFIG_ENABLE_SUPPORT_NONCACHE
    extern char _cherryusb_no_cache_start;
    extern char _cherryusb_no_cache_end;

    rt_hw_cpu_dcache_invalidate(&_cherryusb_no_cache_start, &_cherryusb_no_cache_end - &_cherryusb_no_cache_start);
    rt_hw_cpu_dcache_clean_flush(&_cherryusb_no_cache_start, &_cherryusb_no_cache_end - &_cherryusb_no_cache_start);

    char *p = &_cherryusb_no_cache_start;
    for (size_t i = 0; i < (&_cherryusb_no_cache_end - &_cherryusb_no_cache_start); i++)
    {
        p[i] = 0x0;
    }

    rt_kprintf("set %p - %p no cache\n", &_cherryusb_no_cache_start, &_cherryusb_no_cache_end);
#endif

    g_usb_otg_base = (uintptr_t)rt_ioremap_nocache((void *)0x91500000UL, 0x10000);
    return 0;
}
