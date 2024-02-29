#include "ipc/waitqueue.h"
#include "usbd_core.h"
#include "usbd_cdc.h"
#include "usbd_mtp.h"
#include "dfs_poll.h"

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
#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 1033

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
    CDC_ACM_DESCRIPTOR_INIT(0x00, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, 0x02),
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

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t usb_read_buffer[2048];
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t write_buffer[2048] = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30 };

volatile char ep_tx_busy_flag = 0;

#ifdef CONFIG_USB_HS
#define CDC_MAX_MPS 512
#else
#define CDC_MAX_MPS 64
#endif

void usbd_configure_done_callback(void)
{
    /* setup first out ep read transfer */
    usbd_ep_start_read(CDC_OUT_EP, usb_read_buffer, 2048);
}

extern size_t actual_bytes;
extern size_t actual_write;
extern struct rt_semaphore cdc_read_sem;
extern struct rt_semaphore cdc_write_sem;
extern struct rt_device g_cdc_rt_device;
extern volatile int g_cdc_mask;

void usbd_cdc_acm_bulk_out(uint8_t ep, uint32_t nbytes)
{
    //USB_LOG_RAW("actual out len:%d\r\n", nbytes);
    actual_bytes = nbytes;
    g_cdc_mask |= POLLIN;
    rt_wqueue_wakeup(&g_cdc_rt_device.wait_queue, (void*)POLLIN);
    rt_sem_release(&cdc_read_sem);
    return;

    rt_kprintf("recv:");
    for (size_t i = 0; i < nbytes; i++) {
        rt_kprintf("\\x%02X", ((unsigned char*)usb_read_buffer)[i]);
    }
    rt_kprintf("\n");
    usbd_ep_start_read(CDC_OUT_EP, usb_read_buffer, 2048);
}

void usbd_cdc_acm_bulk_in(uint8_t ep, uint32_t nbytes)
{
    //USB_LOG_RAW("actual in len:%u\r\n", nbytes);
    actual_write += nbytes;
    if ((nbytes % CDC_MAX_MPS) == 0 && nbytes) {
        /* send zlp */
        usbd_ep_start_write(CDC_IN_EP, NULL, 0);
    } else {
        rt_sem_release(&cdc_write_sem);
        //g_cdc_mask |= POLL_OUT;
        //rt_wqueue_wakeup(&g_cdc_rt_device.wait_queue, (void*)POLL_OUT);
        ep_tx_busy_flag = 0;
    }
}

/*!< endpoint call back */
struct usbd_endpoint cdc_out_ep = {
    .ep_addr = CDC_OUT_EP,
    .ep_cb = usbd_cdc_acm_bulk_out
};

struct usbd_endpoint cdc_in_ep = {
    .ep_addr = CDC_IN_EP,
    .ep_cb = usbd_cdc_acm_bulk_in
};

struct usbd_interface intf0;
struct usbd_interface intf1;
struct usbd_interface intf2;

void cdc_acm_msc_init(void)
{
    rt_sem_init(&cdc_read_sem, "cdc/read", 0, RT_IPC_FLAG_FIFO);
    rt_sem_init(&cdc_write_sem, "cdc/write", 1, RT_IPC_FLAG_FIFO);
    usbd_desc_register(cdc_msc_descriptor);
    usbd_add_interface(usbd_cdc_acm_init_intf(&intf0));
    usbd_add_interface(usbd_cdc_acm_init_intf(&intf1));
    usbd_add_endpoint(&cdc_out_ep);
    usbd_add_endpoint(&cdc_in_ep);
#if ENABLE_USB_MTP
    usbd_add_interface(usbd_mtp_init_intf(&intf2, MTP_OUT_EP, MTP_IN_EP, MTP_INT_EP));
#endif

    usbd_initialize();
}
