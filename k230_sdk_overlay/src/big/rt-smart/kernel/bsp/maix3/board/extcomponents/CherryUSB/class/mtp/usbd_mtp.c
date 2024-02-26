/*
 * Copyright (c) 2022, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "usbd_core.h"
#include "usbd_mtp.h"
#include "mtp.h"
#include "mtp_helpers.h"
#include "rtthread.h"
#include "usb_osal.h"

/* Max USB packet size */
#ifndef CONFIG_USB_HS
#define MTP_BULK_EP_MPS 64
#else
#define MTP_BULK_EP_MPS 512
#endif

#define MTP_OUT_EP_IDX 0
#define MTP_IN_EP_IDX  1
#define MTP_INT_EP_IDX 2

/* Describe EndPoints configuration */
static struct usbd_endpoint mtp_ep_data[3];
static mtp_ctx *mtp_context;
static usb_osal_thread_t mtp_tid;
static rt_event_t mtp_event;
static volatile uint32_t read_size;
static volatile uint32_t write_size;
#define EV_CONFIGURED 0x01
#define EV_DISCONNECT 0x02
#define EV_BULK_READ_FINISH 0x04
#define EV_BULK_WRITE_FINISH 0x08
#define EV_INT_WRITE_FINISH 0x10

int read_usb(void * ctx, unsigned char * buffer, int maxsize)
{
    rt_uint32_t re;

    read_size = 0;
    usbd_ep_start_read(mtp_ep_data[MTP_OUT_EP_IDX].ep_addr, buffer, maxsize);
    rt_event_recv(mtp_event, EV_BULK_READ_FINISH | EV_DISCONNECT, RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, &re);
    if (re & EV_DISCONNECT)
        return -1;

    return read_size;
}

int write_usb(void * ctx, int channel, unsigned char * buffer, int size)
{
    rt_uint32_t re;

    if (channel == MTP_IN_EP_IDX) {
        write_size = 0;
        usbd_ep_start_write(mtp_ep_data[channel].ep_addr, buffer, size);
        rt_event_recv(mtp_event, EV_BULK_WRITE_FINISH | EV_DISCONNECT, RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, &re);
    } else {
        usbd_ep_start_write(mtp_ep_data[channel].ep_addr, buffer, size);
        rt_event_recv(mtp_event, EV_INT_WRITE_FINISH | EV_DISCONNECT, RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, &re);
    }
    if (re & EV_DISCONNECT)
        return -1;

    return write_size;
}

int mtp_fs_db_valid(void)
{
    return mtp_context->fs_db ? 1 : 0;
}

static void mtp_thread(void *arg)
{
    while (1) {
        rt_event_control(mtp_event, RT_IPC_CMD_RESET, NULL);
        rt_event_recv(mtp_event, EV_CONFIGURED, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, NULL);
        rt_event_control(mtp_event, RT_IPC_CMD_RESET, NULL);
        while (1) {
            int ret = mtp_incoming_packet(mtp_context);
            if (ret) {
                pthread_mutex_lock(&mtp_context->inotify_mutex);
                if (mtp_context->fs_db) {
                    deinit_fs_db(mtp_context->fs_db);
                    mtp_context->fs_db = 0;
                }
                pthread_mutex_unlock(&mtp_context->inotify_mutex);
                break;
            }
        }
    }
}

static int mtp_class_interface_request_handler(struct usb_setup_packet *setup, uint8_t **data, uint32_t *len)
{
    USB_LOG_DBG("MTP Class request: "
                "bRequest 0x%02x\r\n",
                setup->bRequest);

    switch (setup->bRequest) {
        case MTP_REQUEST_CANCEL:

            break;
        case MTP_REQUEST_GET_EXT_EVENT_DATA:

            break;
        case MTP_REQUEST_RESET:

            break;
        case MTP_REQUEST_GET_DEVICE_STATUS:

            break;

        default:
            USB_LOG_WRN("Unhandled MTP Class bRequest 0x%02x\r\n", setup->bRequest);
            return -1;
    }

    return 0;
}

static void usbd_mtp_bulk_out(uint8_t ep, uint32_t nbytes)
{
    read_size = nbytes;
    rt_event_send(mtp_event, EV_BULK_READ_FINISH);
}

static void usbd_mtp_bulk_in(uint8_t ep, uint32_t nbytes)
{
    write_size = nbytes;
    rt_event_send(mtp_event, EV_BULK_WRITE_FINISH);
}

static void usbd_mtp_int_in(uint8_t ep, uint32_t nbytes)
{
    rt_event_send(mtp_event, EV_INT_WRITE_FINISH);
}

static void mtp_notify_handler(uint8_t event, void *arg)
{
    switch (event) {
        case USBD_EVENT_RESET:
            rt_event_send(mtp_event, EV_DISCONNECT);
            break;
        case USBD_EVENT_CONFIGURED:
            rt_event_send(mtp_event, EV_CONFIGURED);
            break;
        default:
            break;
    }
}

static int init_usb_mtp_buffer(mtp_ctx * ctx)
{
    if(ctx->wrbuffer)
        free(ctx->wrbuffer);

    ctx->wrbuffer = malloc( ctx->usb_wr_buffer_max_size );
    if(!ctx->wrbuffer)
        goto init_error;

    memset(ctx->wrbuffer,0,ctx->usb_wr_buffer_max_size);

    if(ctx->rdbuffer)
        free(ctx->rdbuffer);

    ctx->rdbuffer = malloc( ctx->usb_rd_buffer_max_size );
    if(!ctx->rdbuffer)
        goto init_error;

    memset(ctx->rdbuffer,0,ctx->usb_rd_buffer_max_size);

    if(ctx->rdbuffer2)
        free(ctx->rdbuffer2);

    ctx->rdbuffer2 = malloc( ctx->usb_rd_buffer_max_size );
    if(!ctx->rdbuffer2)
        goto init_error;

    memset(ctx->rdbuffer2,0,ctx->usb_rd_buffer_max_size);

    return 0;
init_error:
    USB_LOG_ERR("init_usb_mtp_gadget init error !");

    if(ctx->wrbuffer)
    {
        free(ctx->wrbuffer);
        ctx->wrbuffer = NULL;
    }

    if(ctx->rdbuffer)
    {
        free(ctx->rdbuffer);
        ctx->rdbuffer = NULL;
    }

    if(ctx->rdbuffer2)
    {
        free(ctx->rdbuffer2);
        ctx->rdbuffer2 = NULL;
    }

    return -1;
}

struct usbd_interface *usbd_mtp_init_intf(struct usbd_interface *intf,
                                          const uint8_t out_ep,
                                          const uint8_t in_ep,
                                          const uint8_t int_ep)
{
    mtp_context = mtp_init_responder();
    mtp_load_config_file(mtp_context, "/sdcard/conf/mtp.conf");
    init_usb_mtp_buffer(mtp_context);
    mtp_set_usb_handle(mtp_context, NULL, MTP_BULK_EP_MPS);
    mtp_add_storage(mtp_context, "/sdcard", "sdcard", 0, 0, UMTP_STORAGE_READWRITE);
    mtp_event = rt_event_create("mtp", RT_IPC_FLAG_FIFO);
    mtp_tid = usb_osal_thread_create("mtp", CONFIG_USBDEV_MTP_STACKSIZE, CONFIG_USBDEV_MTP_PRIO, mtp_thread, mtp_context);

    intf->class_interface_handler = mtp_class_interface_request_handler;
    intf->class_endpoint_handler = NULL;
    intf->vendor_handler = NULL;
    intf->notify_handler = mtp_notify_handler;

    mtp_ep_data[MTP_OUT_EP_IDX].ep_addr = out_ep;
    mtp_ep_data[MTP_OUT_EP_IDX].ep_cb = usbd_mtp_bulk_out;
    mtp_ep_data[MTP_IN_EP_IDX].ep_addr = in_ep;
    mtp_ep_data[MTP_IN_EP_IDX].ep_cb = usbd_mtp_bulk_in;
    mtp_ep_data[MTP_INT_EP_IDX].ep_addr = int_ep;
    mtp_ep_data[MTP_INT_EP_IDX].ep_cb = usbd_mtp_int_in;

    usbd_add_endpoint(&mtp_ep_data[MTP_OUT_EP_IDX]);
    usbd_add_endpoint(&mtp_ep_data[MTP_IN_EP_IDX]);
    usbd_add_endpoint(&mtp_ep_data[MTP_INT_EP_IDX]);

    return intf;
}
