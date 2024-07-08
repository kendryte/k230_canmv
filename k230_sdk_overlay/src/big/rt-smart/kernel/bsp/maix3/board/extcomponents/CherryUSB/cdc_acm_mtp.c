#include "ipc/waitqueue.h"
#include "usbd_core.h"
#include "usbd_cdc.h"
#include "usbd_mtp.h"
#include "dfs_poll.h"
#include "dfs_file.h"
#include "rtthread.h"

/*!< endpoint address */
#define CDC_IN_EP  0x81
#define CDC_OUT_EP 0x01
#define CDC_INT_EP 0x82

/*!< endpoint address */
#define MTP_IN_EP  0x83
#define MTP_OUT_EP 0x03
#define MTP_INT_EP 0x84

// OpenMV Cam
#define USBD_VID           0x1209
#define USBD_PID           0xABD1
#define USBD_MAX_POWER     500
#define USBD_LANGID_STRING 1033

#ifdef CONFIG_USB_HS
#define CDC_MAX_MPS 512
#else
#define CDC_MAX_MPS 64
#endif

/*!< config descriptor size */
#define ENABLE_USB_MTP 1
#if ENABLE_USB_MTP
#define USB_CONFIG_SIZE (9 + CDC_ACM_DESCRIPTOR_LEN + MTP_DESCRIPTOR_LEN)
#else
#define USB_CONFIG_SIZE (9 + CDC_ACM_DESCRIPTOR_LEN)
#endif

/*!< global descriptor */
static const uint8_t cdc_msc_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0xEF, 0x02, 0x01, USBD_VID, USBD_PID, 0x0100, 0x01),
    #if ENABLE_USB_MTP
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x03, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    #else
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x02, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    #endif
    CDC_ACM_DESCRIPTOR_INIT(0x00, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, CDC_MAX_MPS, 0x02),
    #if ENABLE_USB_MTP
    MTP_DESCRIPTOR_INIT(0x02, MTP_OUT_EP, MTP_IN_EP, MTP_INT_EP, 0x00),
    #endif
    ///////////////////////////////////////
    /// string0 descriptor
    ///////////////////////////////////////
    USB_LANGID_INIT(USBD_LANGID_STRING),
    ///////////////////////////////////////
    /// string1 descriptor
    ///////////////////////////////////////
    0x12,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'K', 0x00,                  /* wcChar0 */
    'e', 0x00,                  /* wcChar1 */
    'n', 0x00,                  /* wcChar2 */
    'd', 0x00,                  /* wcChar3 */
    'r', 0x00,                  /* wcChar4 */
    'y', 0x00,                  /* wcChar5 */
    't', 0x00,                  /* wcChar6 */
    'e', 0x00,                  /* wcChar7 */
    ///////////////////////////////////////
    /// string2 descriptor
    ///////////////////////////////////////
    0x0C,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'C', 0x00,                  /* wcChar0 */
    'a', 0x00,                  /* wcChar1 */
    'n', 0x00,                  /* wcChar2 */
    'M', 0x00,                  /* wcChar3 */
    'V', 0x00,                  /* wcChar4 */
    ///////////////////////////////////////
    /// string3 descriptor
    ///////////////////////////////////////
    0x14,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    '0', 0x00,                  /* wcChar0 */
    '0', 0x00,                  /* wcChar1 */
    '1', 0x00,                  /* wcChar2 */
    '0', 0x00,                  /* wcChar3 */
    '0', 0x00,                  /* wcChar4 */
    '0', 0x00,                  /* wcChar5 */
    '0', 0x00,                  /* wcChar6 */
    '0', 0x00,                  /* wcChar7 */
    '0', 0x00,                  /* wcChar8 */
#ifdef CONFIG_USB_HS
    ///////////////////////////////////////
    /// device qualifier descriptor
    ///////////////////////////////////////
    0x0a,
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0xEF,
    0x02,
    0x01,
    0x40,
    0x00,
    0x00,
#endif
    0x00
};

static void usbd_cdc_acm_bulk_out(uint8_t busid, uint8_t ep, uint32_t nbytes);
static void usbd_cdc_acm_bulk_in(uint8_t busid, uint8_t ep, uint32_t nbytes);

static struct usbd_endpoint cdc_out_ep = {
    .ep_addr = CDC_OUT_EP,
    .ep_cb = usbd_cdc_acm_bulk_out
};
static struct usbd_endpoint cdc_in_ep = {
    .ep_addr = CDC_IN_EP,
    .ep_cb = usbd_cdc_acm_bulk_in
};

static struct usbd_interface intf0;
static struct usbd_interface intf1;
static struct usbd_interface intf2;

static struct rt_device cdc_device;
static int cdc_poll_flag, cdc_device_connect;
static USB_MEM_ALIGNX uint8_t usb_read_buffer[4096];
static uint32_t actual_read;
static struct rt_semaphore cdc_read_sem, cdc_write_sem;

static void cdc_device_init(void);

static void event_handler(uint8_t busid, uint8_t event)
{
    if (event == USBD_EVENT_RESET) {
        cdc_device_connect = 0;
    } else if (event == USBD_EVENT_CONFIGURED) {
        rt_sem_control(&cdc_write_sem, RT_IPC_CMD_RESET, 1);
        cdc_device_connect = 1;
        usbd_ep_start_read(CDC_DEV_BUSID, CDC_OUT_EP, usb_read_buffer, sizeof(usb_read_buffer));
    }
}

void cdc_acm_mtp_init(void)
{
    cdc_device_init();
    usbd_desc_register(CDC_DEV_BUSID, cdc_msc_descriptor);
    usbd_cdc_acm_init_intf(CDC_DEV_BUSID, &intf0);
    usbd_cdc_acm_init_intf(CDC_DEV_BUSID, &intf1);
    usbd_add_interface(CDC_DEV_BUSID, &intf0);
    usbd_add_interface(CDC_DEV_BUSID, &intf1);
    usbd_add_endpoint(CDC_DEV_BUSID, &cdc_out_ep);
    usbd_add_endpoint(CDC_DEV_BUSID, &cdc_in_ep);
#if ENABLE_USB_MTP
    usbd_mtp_init_intf(&intf2, MTP_OUT_EP, MTP_IN_EP, MTP_INT_EP);
    usbd_add_interface(CDC_DEV_BUSID, &intf2);
#endif
    void *usb_base;
#ifdef CHERRYUSB_DEVICE_USING_USB0
    usb_base = rt_ioremap((void *)0x91500000UL, 0x10000);
#else
    usb_base = rt_ioremap((void *)0x91540000UL, 0x10000);
#endif

    usbd_initialize(CDC_DEV_BUSID, (uint32_t)usb_base, event_handler);
}

static void usbd_cdc_acm_bulk_out(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    actual_read = nbytes;
    cdc_poll_flag |= POLLIN;
    rt_wqueue_wakeup(&cdc_device.wait_queue, (void*)POLLIN);
    rt_sem_release(&cdc_read_sem);
}

static void usbd_cdc_acm_bulk_in(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    /* send zlp */
    if (nbytes && (nbytes % CDC_MAX_MPS) == 0)
        usbd_ep_start_write(CDC_DEV_BUSID, CDC_IN_EP, NULL, 0);
    else
        rt_sem_release(&cdc_write_sem);
}

void usbd_cdc_acm_set_dtr(uint8_t busid, uint8_t intf, bool dtr)
{
    cdc_poll_flag |= POLLERR;
    rt_wqueue_wakeup(&cdc_device.wait_queue, (void*)POLLERR);
}

void usbd_cdc_acm_set_rts(uint8_t busid, uint8_t intf, bool rts)
{
}

static int cdc_open(struct dfs_fd *fd)
{
    return 0;
}

static int cdc_close(struct dfs_fd *fd)
{
    return 0;
}

static int cdc_read(struct dfs_fd *fd, void *buf, size_t count) {
    rt_err_t error = rt_sem_take(&cdc_read_sem, rt_tick_from_millisecond(100));
    if (error == RT_EOK) {
        int tmp = actual_read;
        memcpy(buf, usb_read_buffer, actual_read);
        actual_read = 0;
        usbd_ep_start_read(CDC_DEV_BUSID, CDC_OUT_EP, usb_read_buffer, sizeof(usb_read_buffer));
        return tmp;
    } else if (error == -RT_ETIMEOUT) {
        USB_LOG_WRN("read timeout\n");
        return 0;
    } else {
        return -1;
    }
}

static int cdc_write(struct dfs_fd *fd, const void *buf, size_t count) {
    if (count == 0 || cdc_device_connect == 0)
        return 0;
    rt_err_t error = rt_sem_take(&cdc_write_sem, rt_tick_from_millisecond(10));
    if (error == RT_EOK)
        usbd_ep_start_write(CDC_DEV_BUSID, CDC_IN_EP, buf, count);
    return error == RT_EOK ? count : -error;
}

static int cdc_poll(struct dfs_fd *fd, struct rt_pollreq *req)
{
    rt_poll_add(&cdc_device.wait_queue, req);
    int tmp = cdc_poll_flag;
    cdc_poll_flag = 0;
    return tmp;
}

static struct dfs_file_ops cdc_ops = {
    .open = cdc_open,
    .close = cdc_close,
    .read = cdc_read,
    .write = cdc_write,
    .poll = cdc_poll
};

static void cdc_device_init(void)
{
    rt_err_t err = rt_device_register(&cdc_device, "ttyUSB1", RT_DEVICE_OFLAG_RDWR);
    if (err != RT_EOK) {
        rt_kprintf("[usb] register device error\n");
        return;
    }
    cdc_device.fops = &cdc_ops;
    rt_sem_init(&cdc_read_sem, "cdc_read", 0, RT_IPC_FLAG_FIFO);
    rt_sem_init(&cdc_write_sem, "cdc_write", 1, RT_IPC_FLAG_FIFO);
}
