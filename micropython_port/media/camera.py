from mpp.vicap import *
from mpp import *
from time import *
from media.media import *
import image

CAM_CHN0_OUT_WIDTH_MAX = 3072
CAM_CHN0_OUT_HEIGHT_MAX = 2160

CAM_CHN1_OUT_WIDTH_MAX = 1920
CAM_CHN1_OUT_HEIGHT_MAX = 1080

CAM_CHN2_OUT_WIDTH_MAX = 1920
CAM_CHN2_OUT_HEIGHT_MAX = 1080

CAM_OUT_WIDTH_MIN = 64
CAM_OUT_HEIGHT_MIN = 64

# currently supported sensor type definition
CAM_OV9732_1280X720_30FPS_10BIT_LINEAR = OV_OV9732_MIPI_1280X720_30FPS_10BIT_LINEAR

CAM_OV9286_1280X720_30FPS_10BIT_LINEAR_IR = OV_OV9286_MIPI_1280X720_30FPS_10BIT_LINEAR_IR
CAM_OV9286_1280X720_30FPS_10BIT_LINEAR_SPECKLE = OV_OV9286_MIPI_1280X720_30FPS_10BIT_LINEAR_SPECKLE

CAM_OV9286_1280X720_60FPS_10BIT_LINEAR_IR = OV_OV9286_MIPI_1280X720_60FPS_10BIT_LINEAR_IR
CAM_OV9286_1280X720_60FPS_10BIT_LINEAR_SPECKLE = OV_OV9286_MIPI_1280X720_60FPS_10BIT_LINEAR_SPECKLE

CAM_OV9286_1280X720_30FPS_10BIT_LINEAR_IR_SPECKLE = OV_OV9286_MIPI_1280X720_30FPS_10BIT_LINEAR_IR_SPECKLE
CAM_OV9286_1280X720_60FPS_10BIT_LINEAR_IR_SPECKLE = OV_OV9286_MIPI_1280X720_60FPS_10BIT_LINEAR_IR_SPECKLE


CAM_IMX335_2LANE_1920X1080_30FPS_12BIT_LINEAR = IMX335_MIPI_2LANE_RAW12_1920X1080_30FPS_LINEAR
CAM_IMX335_2LANE_2592X1944_30FPS_12BIT_LINEAR = IMX335_MIPI_2LANE_RAW12_2592X1944_30FPS_LINEAR
CAM_IMX335_4LANE_2592X1944_30FPS_12BIT_LINEAR = IMX335_MIPI_4LANE_RAW12_2592X1944_30FPS_LINEAR
CAM_IMX335_2LANE_1920X1080_30FPS_12BIT_USEMCLK_LINEAR = IMX335_MIPI_2LANE_RAW12_1920X1080_30FPS_MCLK_7425_LINEAR
CAM_IMX335_2LANE_2592X1944_30FPS_12BIT_USEMCLK_LINEAR = IMX335_MIPI_2LANE_RAW12_2592X1944_30FPS_MCLK_7425_LINEAR
CAM_IMX335_4LANE_2592X1944_30FPS_12BIT_USEMCLK_LINEAR = IMX335_MIPI_4LANE_RAW12_2592X1944_30FPS_MCLK_7425_LINEAR

CAM_IMX335_2LANE_2592X1944_30FPS_10BIT_2HDR = IMX335_MIPI_4LANE_RAW10_2XDOL
CAM_IMX335_2LANE_2592X1944_30FPS_10BIT_3HDR = IMX335_MIPI_4LANE_RAW10_3XDOL


CAM_OV5647_1920X1080_30FPS_10BIT_LINEAR = OV_OV5647_MIPI_1920X1080_30FPS_10BIT_LINEAR
CAM_OV5647_2592x1944_10FPS_10BIT_LINEAR = OV_OV5647_MIPI_2592x1944_10FPS_10BIT_LINEAR
CAM_OV5647_2592x1944_10FPS_10BIT_LINEAR = OV_OV5647_MIPI_640x480_60FPS_10BIT_LINEAR
CAM_OV5647_1920X1080_30FPS_10BIT_USEMCLK_LINEAR = OV_OV5647_MIPI_CSI0_1920X1080_30FPS_10BIT_LINEAR

CAM_SENSOR_TYPE_MAX = SENSOR_TYPE_MAX

# the default sensor type
CAM_DEFAULT_SENSOR = CAM_OV5647_1920X1080_30FPS_10BIT_USEMCLK_LINEAR

#
CAM_DEFAULT_OUTPUT_BUF_NUM = 6
CAM_DEFAULT_INPUT_BUF_NUM = 4


# NOTE:This is a private class for internal use only!!!
#      Don't edit it arbitrarily!!!

class __camera_device:
    def __init__(self):
        self.dev_attr = k_vicap_dev_attr()
        self.chn_attr = [k_vicap_chn_attr() for i in range(0, VICAP_CHN_ID_MAX)]
        self.buf_init = [False for i in range(0, VICAP_CHN_ID_MAX)]
        # set the default value
        self.dev_attr.buffer_num = CAM_DEFAULT_INPUT_BUF_NUM
        self.dev_attr.mode = VICAP_WORK_ONLINE_MODE
        self.dev_attr.input_type = VICAP_INPUT_TYPE_SENSOR

        for i in range(0, VICAP_CHN_ID_MAX):
            self.chn_attr[i].buffer_num = CAM_DEFAULT_OUTPUT_BUF_NUM


class camera:
    # gloable cam_dev obj define
    cam_dev = [__camera_device() for i in range(0, VICAP_DEV_ID_MAX)]


    # sensor_init
    @classmethod
    def sensor_init(cls, dev_num, type):
        print("sensor_init enter")

        if (dev_num > CAM_DEV_ID_MAX - 1) or (type > CAM_SENSOR_TYPE_MAX - 1):
            printf(f"sensor_init, invalid param, dev_num({dev_num}, sensor type({type}))");
            return -1;

        ret = kd_mpi_vicap_get_sensor_info(type, cls.cam_dev[dev_num].dev_attr.sensor_info)
        if ret:
            print("get sensor info err")
            return -1

        if type == CAM_OV5647_1920X1080_30FPS_10BIT_USEMCLK_LINEAR:
            kd_mpi_vicap_set_mclk(VICAP_MCLK0, VICAP_PLL0_CLK_DIV4, 16, 1)

        cls.cam_dev[dev_num].dev_attr.acq_win.h_start = 0
        cls.cam_dev[dev_num].dev_attr.acq_win.v_start = 0
        cls.cam_dev[dev_num].dev_attr.acq_win.width = cls.cam_dev[dev_num].dev_attr.sensor_info.width
        cls.cam_dev[dev_num].dev_attr.acq_win.height = cls.cam_dev[dev_num].dev_attr.sensor_info.height

        cls.cam_dev[dev_num].dev_attr.mode = VICAP_WORK_ONLINE_MODE
        cls.cam_dev[dev_num].dev_attr.input_type = VICAP_INPUT_TYPE_SENSOR
        cls.cam_dev[dev_num].dev_attr.dev_enable = True

        print("sensor_init exit")
        return 0


    # set_inbufs
    @classmethod
    def set_inbufs(cls, dev_num, num):
        print("set_inbufs enter")

        if (dev_num > CAM_DEV_ID_MAX - 1):
            printf(f"set_inbufs, invalid param, dev_num({dev_num}");
            return -1;

        cls.cam_dev[dev_num].dev_attr.mode = VICAP_WORK_OFFLINE_MODE
        cls.cam_dev[dev_num].dev_attr.buffer_num = num
        cls.cam_dev[dev_num].dev_attr.buffer_size = \
            ALIGN_UP((cls.cam_dev[dev_num].dev_attr.acq_win.width * \
              cls.cam_dev[dev_num].dev_attr.acq_win.height * 2), VICAP_ALIGN_4K)

        # config vb for offline mode
        if cls.cam_dev[dev_num].dev_attr.mode == VICAP_WORK_OFFLINE_MODE:
            config = k_vb_config()
            config.max_pool_cnt = 64
            config.comm_pool[0].blk_cnt = cls.cam_dev[dev_num].dev_attr.buffer_num
            config.comm_pool[0].mode = VB_REMAP_MODE_NOCACHE
            config.comm_pool[0].blk_size = cls.cam_dev[dev_num].dev_attr.buffer_size
            ret = media.buffer_config(config)
            if ret:
                print("set_inbufs({dev_num}), buffer_config failed({ret})")
                return ret

        print("set_inbufs exit")
        return 0


    # set_outbufs
    @classmethod
    def set_outbufs(cls, dev_num, chn_num, num):
        print("set_outbufs enter")

        if (dev_num > CAM_DEV_ID_MAX - 1) or (chn_num > CAM_CHN_ID_MAX - 1):
            printf(f"set_outbufs, invalid param, dev_num({dev_num}, chn_num({chn_num}))");
            return -1;

        cls.cam_dev[dev_num].chn_attr[chn_num].chn_enable = True
        cls.cam_dev[dev_num].chn_attr[chn_num].buffer_num = num

        # config vb for out channel
        if cls.cam_dev[dev_num].buf_init[chn_num] == False \
           and cls.cam_dev[dev_num].chn_attr[chn_num].buffer_num \
           and cls.cam_dev[dev_num].chn_attr[chn_num].buffer_size:
            cls.cam_dev[dev_num].buf_init[chn_num] = True
            config = k_vb_config()
            config.max_pool_cnt = 64
            config.comm_pool[0].blk_cnt = cls.cam_dev[dev_num].chn_attr[chn_num].buffer_num
            config.comm_pool[0].mode = VB_REMAP_MODE_NOCACHE
            config.comm_pool[0].blk_size = cls.cam_dev[dev_num].chn_attr[chn_num].buffer_size
            ret = media.buffer_config(config)
            if ret:
                print(f"set_outbufs({dev_num}), buffer_config failed({ret})")
                return ret

        print("set_outbufs exit")
        return 0


    # set_outsize
    @classmethod
    def set_outsize(cls, dev_num, chn_num, width, height):
        print("set_outsize enter")

        if (dev_num > CAM_DEV_ID_MAX - 1) or (chn_num > CAM_CHN_ID_MAX - 1):
            printf(f"invalid param, dev_num({dev_num}, chn_num({chn_num}))");
            return -1;

        if (width > cls.cam_dev[dev_num].dev_attr.acq_win.width) \
           or (height > cls.cam_dev[dev_num].dev_attr.acq_win.height) \
           or (width < CAM_OUT_WIDTH_MIN) \
           or (height < CAM_OUT_HEIGHT_MIN):
            printf(f"invalid out size, width({width}, height({height}))");
            return -1;

        cls.cam_dev[dev_num].chn_attr[chn_num].chn_enable = True
        cls.cam_dev[dev_num].chn_attr[chn_num].out_win.h_start = 0
        cls.cam_dev[dev_num].chn_attr[chn_num].out_win.v_start = 0
        cls.cam_dev[dev_num].chn_attr[chn_num].out_win.width = ALIGN_UP(width, 16)
        cls.cam_dev[dev_num].chn_attr[chn_num].out_win.height = height

        if cls.cam_dev[dev_num].chn_attr[chn_num].pix_format:
            buf_size = 0
            pix_format = cls.cam_dev[dev_num].chn_attr[chn_num].pix_format
            out_width = cls.cam_dev[dev_num].chn_attr[chn_num].out_win.width
            out_height = cls.cam_dev[dev_num].chn_attr[chn_num].out_win.height
            in_width = cls.cam_dev[dev_num].dev_attr.acq_win.width
            in_height = cls.cam_dev[dev_num].dev_attr.acq_win.height

            if pix_format == PIXEL_FORMAT_YUV_SEMIPLANAR_420:
                buf_size = ALIGN_UP((out_width * out_height * 3 // 2), VICAP_ALIGN_4K)
            elif pix_format in [PIXEL_FORMAT_RGB_888, PIXEL_FORMAT_BGR_888_PLANAR]:
                buf_size = ALIGN_UP((out_width * out_height * 3), VICAP_ALIGN_4K)
            elif pix_format in [PIXEL_FORMAT_RGB_BAYER_10BPP, PIXEL_FORMAT_RGB_BAYER_12BPP, \
                                PIXEL_FORMAT_RGB_BAYER_14BPP, PIXEL_FORMAT_RGB_BAYER_16BPP]:
                cls.cam_dev[dev_num].chn_attr[chn_num].out_win.width = in_width
                cls.cam_dev[dev_num].chn_attr[chn_num].out_win.height = in_height
                buf_size = ALIGN_UP((in_width * in_height * 2), VICAP_ALIGN_4K)
            else:
                print(f"set_outsize({dev_num},{chn_num}),unspported format!")
                return -1

            cls.cam_dev[dev_num].chn_attr[chn_num].buffer_size = buf_size

        # config vb for out channel
        if (cls.cam_dev[dev_num].buf_init[chn_num] == False) \
           and cls.cam_dev[dev_num].chn_attr[chn_num].buffer_num \
           and cls.cam_dev[dev_num].chn_attr[chn_num].buffer_size:
            cls.cam_dev[dev_num].buf_init[chn_num] = True
            config = k_vb_config()
            config.max_pool_cnt = 64
            config.comm_pool[0].blk_cnt = cls.cam_dev[dev_num].chn_attr[chn_num].buffer_num
            config.comm_pool[0].mode = VB_REMAP_MODE_NOCACHE
            config.comm_pool[0].blk_size = cls.cam_dev[dev_num].chn_attr[chn_num].buffer_size
            ret = media.buffer_config(config)
            if ret:
                print(f"set_outsize({dev_num}), buffer_config failed({ret})")
                return ret

        print("set_outsize exit")
        return 0


    # set_outfmt
    @classmethod
    def set_outfmt(cls, dev_num, chn_num, pix_format):
        print("set_outfmt enter")

        if (dev_num > CAM_DEV_ID_MAX - 1) or (chn_num > CAM_CHN_ID_MAX - 1):
            printf(f"invalid param, dev_num({dev_num}, chn_num({chn_num}))");
            return -1;

        cls.cam_dev[dev_num].chn_attr[chn_num].pix_format = pix_format
        cls.cam_dev[dev_num].chn_attr[chn_num].chn_enable = True

        if cls.cam_dev[dev_num].chn_attr[chn_num].out_win.width \
           and cls.cam_dev[dev_num].chn_attr[chn_num].out_win.height:
            buf_size = 0
            out_width = cls.cam_dev[dev_num].chn_attr[chn_num].out_win.width
            out_height = cls.cam_dev[dev_num].chn_attr[chn_num].out_win.height
            in_width = cls.cam_dev[dev_num].dev_attr.acq_win.width
            in_height = cls.cam_dev[dev_num].dev_attr.acq_win.height

            if pix_format == PIXEL_FORMAT_YUV_SEMIPLANAR_420:
                buf_size = ALIGN_UP((out_width * out_height * 3 // 2), VICAP_ALIGN_4K)
            elif pix_format in [PIXEL_FORMAT_RGB_888, PIXEL_FORMAT_BGR_888_PLANAR]:
                buf_size = ALIGN_UP((out_width * out_height * 3), VICAP_ALIGN_4K)
            elif pix_format in [PIXEL_FORMAT_RGB_BAYER_10BPP, PIXEL_FORMAT_RGB_BAYER_12BPP, \
                                PIXEL_FORMAT_RGB_BAYER_14BPP, PIXEL_FORMAT_RGB_BAYER_16BPP]:
                cls.cam_dev[dev_num].chn_attr[chn_num].out_win.width = in_width
                cls.cam_dev[dev_num].chn_attr[chn_num].out_win.height = in_height
                buf_size = ALIGN_UP((in_width * in_height * 2), VICAP_ALIGN_4K)
            else:
                print(f"set_outfmt({dev_num},{chn_num}),unspported format!")
                return -1

            cls.cam_dev[dev_num].chn_attr[chn_num].buffer_size = buf_size

        # config vb for out channel
        if cls.cam_dev[dev_num].buf_init[chn_num] == False \
           and cls.cam_dev[dev_num].chn_attr[chn_num].buffer_num \
           and cls.cam_dev[dev_num].chn_attr[chn_num].buffer_size:
            cls.cam_dev[dev_num].buf_init[chn_num] = True
            config = k_vb_config()
            config.max_pool_cnt = 64
            config.comm_pool[0].blk_cnt = cls.cam_dev[dev_num].chn_attr[chn_num].buffer_num
            config.comm_pool[0].mode = VB_REMAP_MODE_NOCACHE
            config.comm_pool[0].blk_size = cls.cam_dev[dev_num].chn_attr[chn_num].buffer_size
            ret = media.buffer_config(config)
            if ret:
                print(f"set_outfmt({dev_num}), buffer_config failed({ret})")
                return ret

        print("set_outfmt exit")
        return 0


    # start_stream
    @classmethod
    def start_stream(cls, dev_num):
        print("start_stream enter")

        if (dev_num > CAM_DEV_ID_MAX - 1):
            printf(f"invalid param, dev_num({dev_num}");
            return -1;

        ret = kd_mpi_vicap_set_dev_attr(dev_num, cls.cam_dev[dev_num].dev_attr)
        if ret:
            print(f"start_stream({dev_num}), set dev attr failed({ret})")
            return ret

        # vicap channel attr set
        for chn_num in range(0, VICAP_CHN_ID_MAX):
            if not cls.cam_dev[dev_num].chn_attr[chn_num].chn_enable:
                continue

            ret = kd_mpi_vicap_set_chn_attr(dev_num, chn_num, cls.cam_dev[dev_num].chn_attr[chn_num])
            if ret:
                print("start_stream({dev_num}), set chn attr failed({ret})")
                return -1

        ret = kd_mpi_vicap_init(dev_num)
        if ret:
            print(f"start_stream({dev_num}), vicap init failed({ret})")
            return ret

        ret = kd_mpi_vicap_start_stream(dev_num)
        if ret:
            print(f"start_stream({dev_num}), start stream failed({ret})")
            return ret

        print("start_stream exit")
        return 0


    # stop_stream
    @classmethod
    def stop_stream(cls, dev_num):
        print("stop_stream enter")
        if (dev_num > CAM_DEV_ID_MAX - 1):
            printf(f"stop_stream, invalid param, dev_num({dev_num}");
            return -1;

        ret = kd_mpi_vicap_stop_stream(dev_num)
        if ret:
            print(f"stop_stream({dev_num}), stop stream failed({ret})")
            return ret

        ret = kd_mpi_vicap_deinit(dev_num)
        if ret:
            print(f"stop_stream({dev_num}), deinit failed({ret})")
            return ret

        print("stop_stream exit")
        return 0


    # capture_image
    @classmethod
    def capture_image(cls, dev_num, chn_num):
        #print("capture_image enter")

        frame_info = k_video_frame_info()

        if (dev_num > CAM_DEV_ID_MAX - 1) or (chn_num > CAM_CHN_ID_MAX - 1):
            printf(f"capture_image, invalid param, dev_num({dev_num}, chn_num({chn_num}))")
            return -1;

        ret = kd_mpi_vicap_dump_frame(dev_num, chn_num, VICAP_DUMP_YUV, frame_info, 1000)
        if ret:
            print(f"capture_image, dev({dev_num}) chn({chn_num}) request frame failed.")
            return -1

        phys_addr = frame_info.v_frame.phys_addr[0]
        img_width = frame_info.v_frame.width
        img_height = frame_info.v_frame.height
        fmt = frame_info.v_frame.pixel_format

        if fmt == PIXEL_FORMAT_YUV_SEMIPLANAR_420:
            img_size = img_width * img_height * 3 // 2
            img_fmt = image.YUV420
        elif fmt == PIXEL_FORMAT_RGB_888:
            img_size = img_width * img_height * 3
            img_fmt = image.RGB888
        elif fmt == PIXEL_FORMAT_BGR_888_PLANAR:
            img_size = img_width * img_height * 3
            img_fmt = image.RGBP888
        else:
            print("capture_image: unsupported format.")
            return -1;

        virt_addr = kd_mpi_sys_mmap(phys_addr, img_size)
        if virt_addr:
            # Create image object
            img = image.Image(img_width, img_height, img_fmt, alloc=image.ALLOC_VB, phyaddr=phys_addr, virtaddr=virt_addr)
        else:
            print("capture_image, mmap failed")
            return -1;

        #print("capture_image exit")
        return img


    # release_image
    @classmethod
    def release_image(cls, dev_num, chn_num, img):
        #print("release_image enter")

        frame_info = k_video_frame_info()

        phy_addr = img.phyaddr()
        virt_addr = img.virtaddr()
        img_size = img.size()

        ret = kd_mpi_sys_munmap(virt_addr, img_size)
        if ret:
            print(f"release_image, dev({dev_num}) chn({chn_num}) munmap failed")

        frame_info.v_frame.phys_addr[0] = phy_addr;

        ret = kd_mpi_vicap_dump_release(dev_num, chn_num, frame_info)
        if ret:
            print(f"release_image, dev({dev_num}) chn({chn_num}) release frame failed")

        #print("release_image exit")
        return 0

