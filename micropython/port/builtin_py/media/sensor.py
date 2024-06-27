from mpp import *
from mpp.vicap import *
from media.media import *
import image
import os

CAM_CHN0_OUT_WIDTH_MAX = const(3072)
CAM_CHN0_OUT_HEIGHT_MAX = const(2160)

CAM_CHN1_OUT_WIDTH_MAX = const(1920)
CAM_CHN1_OUT_HEIGHT_MAX = const(1080)

CAM_CHN2_OUT_WIDTH_MAX = const(1920)
CAM_CHN2_OUT_HEIGHT_MAX = const(1080)

CAM_OUT_WIDTH_MIN = const(64)
CAM_OUT_HEIGHT_MIN = const(64)

# NOTE:This is a private class for internal use only!!!
#      Don't edit it arbitrarily!!!

class Sensor:
    RGB565   = PIXEL_FORMAT_RGB_565
    RGB888   = PIXEL_FORMAT_RGB_888
    RGBP888  = PIXEL_FORMAT_RGB_888_PLANAR
    YUV420SP = PIXEL_FORMAT_YUV_SEMIPLANAR_420
    GRAYSCALE = PIXEL_FORMAT_GRAYSCALE

    # QQSIF           #: 88x60
    # QQQQVGA         #: 40x30
    # QQQVGA          #: 80x60
    # HQQQQVGA        #: 30x20
    # HQQQVGA         #: 60x40
    # B64X32          #: 64x32 (for use with Image.find_displacement())
    QQCIF       = const(0)    #: 88x72
    QCIF        = const(1)    #: 176x144
    CIF         = const(2)    #: 352x288
    QSIF        = const(3)    #: 176x120
    SIF         = const(4)    #: 352x240
    QQVGA       = const(5)    #: 160x120
    QVGA        = const(6)    #: 320x240
    VGA         = const(7)    #: 640x480
    HQQVGA      = const(8)    #: 120x80
    HQVGA       = const(9)    #: 240x160
    HVGA        = const(10)   #: 480x320
    B64X64      = const(11)   #: 64x64
    B128X64     = const(12)   #: 128x64
    B128X128    = const(13)   #: 128x128
    B160X160    = const(14)   #: 160x160
    B320X320    = const(15)   #: 320x320
    QQVGA2      = const(16)   #: 128x160
    WVGA        = const(17)   #: 720x480
    WVGA2       = const(18)   #: 752x480
    SVGA        = const(19)   #: 800x600
    XGA         = const(20)   #: 1024x768
    WXGA        = const(21)   #: 1280x768
    SXGA        = const(22)   #: 1280x1024
    SXGAM       = const(23)   #: 1280x960
    UXGA        = const(24)   #: 1600x1200
    HD          = const(25)   #: 1280x720
    FHD         = const(26)   #: 1920x1080
    QHD         = const(27)   #: 2560x1440
    QXGA        = const(28)   #: 2048x1536
    WQXGA       = const(29)   #: 2560x1600
    WQXGA2      = const(30)   #: 2592x1944

    FRAME_SIZE_INVAILD = const(31)

    _devs = [None for i in range(0, CAM_DEV_ID_MAX)]

    @classmethod
    def deinit(cls):
        for i in range(0, CAM_DEV_ID_MAX):
            if isinstance(cls._devs[i], Sensor):
                cls._devs[i].stop(is_del = True)

    @classmethod
    def _is_mcm_device(cls) -> bool:
        cnt = 0
        for i in range(0, CAM_DEV_ID_MAX):
            if isinstance(cls._devs[i], Sensor):
                cnt = cnt + 1
        return True if cnt > 1 else False

    @classmethod
    def _handle_mcm_device(cls):
        if not cls._is_mcm_device():
            return
        for i in range(0, CAM_DEV_ID_MAX):
            if isinstance(cls._devs[i], Sensor):
                cls._devs[i]._set_inbufs()

    @classmethod
    def _run_mcm_device(cls):
        if not cls._is_mcm_device():
            return

        for i in range(0, CAM_DEV_ID_MAX):
            if isinstance(cls._devs[i], Sensor):
                sensor = cls._devs[i]
                if not sensor._dev_attr.dev_enable or sensor._is_started:
                    continue
                ret = kd_mpi_vicap_set_dev_attr(i, sensor._dev_attr)
                if ret:
                    raise RuntimeError(f"sensor({i}) run error, set dev attr failed({ret})")

                # vicap channel attr set
                for chn_num in range(0, VICAP_CHN_ID_MAX):
                    if not sensor._chn_attr[chn_num].chn_enable:
                        continue
                    ret = kd_mpi_vicap_set_chn_attr(i, chn_num, sensor._chn_attr[chn_num])
                    if ret:
                        raise RuntimeError(f"sensor({i}) run error, set chn({chn_num}) attr failed({ret})")

        for i in range(0, CAM_DEV_ID_MAX):
            if isinstance(cls._devs[i], Sensor):
                sensor = cls._devs[i]
                if not sensor._dev_attr.dev_enable or sensor._is_started:
                    continue
                ret = kd_mpi_vicap_init(i)
                if ret:
                    raise RuntimeError(f"sensor({i}) run error, vicap init failed({ret})")

        for i in range(0, CAM_DEV_ID_MAX):
            if isinstance(cls._devs[i], Sensor):
                sensor = cls._devs[i]
                if not sensor._dev_attr.dev_enable or sensor._is_started:
                    continue
                print(f"sensor({i}), mode {sensor._dev_attr.mode}, buffer_num {sensor._dev_attr.buffer_num}, buffer_size {sensor._dev_attr.buffer_size}")
                ret = kd_mpi_vicap_start_stream(i)
                if ret:
                    raise RuntimeError(f"sensor({i}) run error, vicap start stream failed({ret})")

                sensor._is_started = True

    # id
    # type
    # force
    def __init__(self, **kwargs):
        self._dft_input_buff_num = 4
        self._dft_output_buff_num = 6

        def_mirror = 0
        dft_sensor_id = CAM_DEV_ID_0

        brd = os.uname()[-1]
        if brd == "k230d_canmv":
            dft_sensor_id = CAM_DEV_ID_2
            self._dft_output_buff_num = 4
        elif brd == "k230_canmv_01studio":
            dft_sensor_id = CAM_DEV_ID_2

        self._dev_id = kwargs.get('id', dft_sensor_id)
        if (self._dev_id > CAM_DEV_ID_MAX - 1):
            raise AssertionError(f"invaild sensor id {self._dev_id}, should < {CAM_DEV_ID_MAX - 1}")

        force = kwargs.get('force', False)
        if not force and Sensor._devs[self._dev_id] is not None:
            raise OSError(f"sensor({self._dev_id}) is already inited.")

        arg_type = kwargs.get('type', None)
        if arg_type is not None:
            self._type = arg_type
        else:
            info = k_vicap_sensor_info()
            cfg = k_vicap_probe_config()
            cfg.csi = self._dev_id + 1 # convert to csi num
            cfg.fps = kwargs.get('fps', 30)
            cfg.width = kwargs.get('width', 1920)
            cfg.height = kwargs.get('height', 1080)

            ret = kd_mpi_sensor_adapt_get(cfg, info)
            if 0 != ret:
                raise RuntimeError(f"Can not found sensor on {self._dev_id}")

            # def_mirror = cfg.def_mirror
            self._type = info.type
            print(f"use sensor {info.type}, output {info.width}x{info.height}@{info.fps}")

        if (self._type > SENSOR_TYPE_MAX - 1):
            raise AssertionError(f"invaild sensor type {self._type}, should < {SENSOR_TYPE_MAX - 1}")

        self._dev_attr = k_vicap_dev_attr()
        self._chn_attr = [k_vicap_chn_attr() for i in range(0, VICAP_CHN_ID_MAX)]
        self._buf_init = [False for i in range(0, VICAP_CHN_ID_MAX)]
        self._buf_in_init = False
        # set the default value
        self._dev_attr.buffer_num = self._dft_input_buff_num
        self._dev_attr.mode = VICAP_WORK_ONLINE_MODE
        # self._dev_attr.mode = VICAP_WORK_OFFLINE_MODE
        self._dev_attr.input_type = VICAP_INPUT_TYPE_SENSOR
        self._dev_attr.mirror = def_mirror

        self._is_started = False

        for i in range(0, VICAP_CHN_ID_MAX):
            self._chn_attr[i].buffer_num = self._dft_output_buff_num

        self._imgs = [None for i in range(0, VICAP_CHN_ID_MAX)]
        self._is_rgb565 = [False for i in range(0, VICAP_CHN_ID_MAX)]
        self._is_grayscale = [False for i in range(0, VICAP_CHN_ID_MAX)]

        self._framesize = [Sensor.FRAME_SIZE_INVAILD for i in range(0, VICAP_CHN_ID_MAX)]

        Sensor._devs[self._dev_id] = self

    def __del__(self):
        self.stop(is_del = True)

    def __str__(self):
        pass

    def _set_inbufs(self):
        if not self._dev_attr.dev_enable:
            raise AssertionError("should call reset() first")

        # if (self._dev_id > CAM_DEV_ID_MAX - 1):
        #     raise AssertionError(f"invaild sensor id {self._dev_id}, should < {CAM_DEV_ID_MAX - 1}")

        if self._buf_in_init:
            return

        self._dev_attr.mode = VICAP_WORK_OFFLINE_MODE
        self._dev_attr.buffer_num = self._dft_input_buff_num
        self._dev_attr.buffer_size = ALIGN_UP((self._dev_attr.acq_win.width * self._dev_attr.acq_win.height * 2), VICAP_ALIGN_4K)

        # config vb for offline mode
        if self._dev_attr.mode == VICAP_WORK_OFFLINE_MODE:
            config = k_vb_config()
            config.max_pool_cnt = 1
            config.comm_pool[0].blk_size = self._dev_attr.buffer_size
            config.comm_pool[0].blk_cnt = self._dev_attr.buffer_num
            config.comm_pool[0].mode = VB_REMAP_MODE_NOCACHE
            ret = MediaManager._config(config)
            if not ret:
                raise RuntimeError("sensor({self._dev_id}) configure buffer failed({ret})")

        self._buf_in_init = True

    def wrap(func):
        def wrapper(*args, **kwargs):
            print(f"not support {func.__name__} now...")

            raise NotImplementedError(f"{func.__name__}")

            return func(*args, **kwargs)

        return wrapper

    def reset(self):
        # if (self._type > SENSOR_TYPE_MAX - 1):
        #     raise AssertionError(f"invaild sensor type {self._type}, should < {SENSOR_TYPE_MAX - 1}")

        # if (self._dev_id > CAM_DEV_ID_MAX - 1):
        #     raise AssertionError(f"invaild sensor id {self._dev_id}, should < {CAM_DEV_ID_MAX - 1}")

        ret = kd_mpi_vicap_get_sensor_info(self._type, self._dev_attr.sensor_info)
        if ret:
            raise RuntimeError("sensor({self._dev_id}) get info failed({ret})")

        self._dev_attr.acq_win.h_start = 0
        self._dev_attr.acq_win.v_start = 0
        self._dev_attr.acq_win.width = self._dev_attr.sensor_info.width
        self._dev_attr.acq_win.height = self._dev_attr.sensor_info.height

        self._dev_attr.mode = VICAP_WORK_ONLINE_MODE
        # self.dev_attr.mode = VICAP_WORK_OFFLINE_MODE
        # self.dev_attr.buffer_num = self._dft_input_buff_num
        # cls.set_inbufs(self._dev_id, self._dft_input_buff_num)
        self._dev_attr.input_type = VICAP_INPUT_TYPE_SENSOR
        self._dev_attr.dev_enable = True
        self._dev_attr.pipe_ctrl.data = 0xffffffff
        self._dev_attr.pipe_ctrl.bits.af_enable = 0
        self._dev_attr.pipe_ctrl.bits.ahdr_enable = 0
        self._dev_attr.pipe_ctrl.bits.dnr3_enable = 0
        self._dev_attr.dw_enable = 0
        self._dev_attr.cpature_frame = 0

        Sensor._handle_mcm_device()

    @wrap
    def sleep(self, enable):
        pass

    @wrap
    def shutdown(self, enable):
        pass

    @wrap
    def flush(self, enable):
        pass
    
    # for snapshot
    def _release_image(self, img, chn = CAM_CHN_ID_0):
        virt_addr = img.virtaddr()
        if virt_addr == 0:
            return
        img_size = img.size()
        ret1 = kd_mpi_sys_munmap(virt_addr, img_size)
        phy_addr = img.phyaddr()
        frame_info = k_video_frame_info()
        frame_info.v_frame.phys_addr[0] = phy_addr
        ret2 = kd_mpi_vicap_dump_release(self._dev_id, chn, frame_info)
        img.__del__()
        self._imgs[chn] = None
        assert ret1 == 0, "kd_mpi_sys_munmap"
        assert ret2 == 0, "kd_mpi_vicap_dump_release"

    # for snapshot
    def _release_all_chn_image(self):
        for chn in range(0, VICAP_CHN_ID_MAX):
            img = self._imgs[chn]
            if img:
                self._release_image(img, chn)

    def snapshot(self, chn = CAM_CHN_ID_0):
        if not self._dev_attr.dev_enable:
            raise AssertionError("should call reset() first")

        # if (self._dev_id > CAM_DEV_ID_MAX - 1):
        #     raise AssertionError(f"invaild sensor id {self._dev_id}, should < {CAM_DEV_ID_MAX - 1}")

        if (chn > CAM_CHN_ID_MAX - 1):
            raise AssertionError(f"invaild chn id {chn}, should < {CAM_CHN_ID_MAX - 1}")

        if self._imgs[chn]:
            self._release_image(self._imgs[chn], chn)

        status = 0
        frame_info = k_video_frame_info()
        try:
            virt_addr = 0
            ret = kd_mpi_vicap_dump_frame(self._dev_id, chn, VICAP_DUMP_YUV, frame_info, 1000)
            if ret:
                raise RuntimeError(f"sensor({self._dev_id}) snapshot chn({chn}) failed({ret})")
            status = 1
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
            elif fmt == PIXEL_FORMAT_RGB_888_PLANAR:
                img_size = img_width * img_height * 3
                img_fmt = image.RGBP888
            else:
                raise RuntimeError(f"sensor({self._dev_id}) snapshot chn({chn}) not support pixelformat({fmt})")
            virt_addr = kd_mpi_sys_mmap_cached(phys_addr, img_size)
            if virt_addr:
                status = 2

                if self._is_rgb565[chn] and (img_fmt == image.RGB888):
                    img = image.Image(img_width, img_height, img_fmt, cvt_565 = True, alloc=image.ALLOC_VB, phyaddr=phys_addr, virtaddr=virt_addr, poolid=frame_info.pool_id)
                elif self._is_grayscale[chn] and (img_fmt == image.YUV420):
                    img = image.Image(img_width, img_height, image.GRAYSCALE, cvt_565 = False, alloc=image.ALLOC_VB, phyaddr=phys_addr, virtaddr=virt_addr, poolid=frame_info.pool_id)
                else:
                    img = image.Image(img_width, img_height, img_fmt, alloc=image.ALLOC_VB, phyaddr=phys_addr, virtaddr=virt_addr, poolid=frame_info.pool_id)
            else:
                raise RuntimeError(f"sensor({self._dev_id}) snapshot chn({chn}) mmap failed")

            self._imgs[chn] = img
            return img
        except Exception as e:
            raise e

    @wrap
    def skip_frames(self, **kwargs):
        pass

    def width(self, chn = CAM_CHN_ID_0):
        if not self._dev_attr.dev_enable:
            raise AssertionError("should call reset() first")

        # if (self._dev_id > CAM_DEV_ID_MAX - 1):
        #     raise AssertionError(f"invaild sensor id {self._dev_id}, should < {CAM_DEV_ID_MAX - 1}")

        if (chn > CAM_CHN_ID_MAX - 1):
            raise AssertionError(f"invaild chn id {chn}, should < {CAM_CHN_ID_MAX - 1}")

        return self._chn_attr[chn].out_win.width

    def height(self, chn = CAM_CHN_ID_0):
        if not self._dev_attr.dev_enable:
            raise AssertionError("should call reset() first")

        # if (self._dev_id > CAM_DEV_ID_MAX - 1):
        #     raise AssertionError(f"invaild sensor id {self._dev_id}, should < {CAM_DEV_ID_MAX - 1}")

        if (chn > CAM_CHN_ID_MAX - 1):
            raise AssertionError(f"invaild chn id {chn}, should < {CAM_CHN_ID_MAX - 1}")

        return self._chn_attr[chn].out_win.height

    @wrap
    def get_fb(self):
        pass

    @wrap
    def get_id(self):
        # if not self._dev_attr.dev_enable or self._dev_id > CAM_DEV_ID_MAX - 1:
        #     raise ValueError(f"invalid param, dev({self._dev_id})")
        # return self._dev_id
        pass

    def get_type(self):
        # if not self._dev_attr.dev_enable or self._dev_id > CAM_DEV_ID_MAX - 1:
        #     raise ValueError(f"invalid param, dev({self._dev_id})")
        return self._type

    @wrap
    def alloc_extra_fb(self, width, height, pixformat):
        pass

    @wrap
    def dealloc_extra_fb(self):
        pass

    def set_pixformat(self, pix_format, chn = CAM_CHN_ID_0):
        if not self._dev_attr.dev_enable:
            raise AssertionError("should call reset() first")

        # if (self._dev_id > CAM_DEV_ID_MAX - 1):
        #     raise AssertionError(f"invaild sensor id {self._dev_id}, should < {CAM_DEV_ID_MAX - 1}")

        if (chn > CAM_CHN_ID_MAX - 1):
            raise AssertionError(f"invaild chn id {chn}, should < {CAM_CHN_ID_MAX - 1}")

        self._is_rgb565[chn] = False
        if pix_format == Sensor.RGB565:
            self._is_rgb565[chn] = True
            pix_format = PIXEL_FORMAT_RGB_888
        elif pix_format == Sensor.GRAYSCALE:
            self._is_grayscale[chn] = True
            pix_format = PIXEL_FORMAT_YUV_SEMIPLANAR_420

        self._chn_attr[chn].pix_format = pix_format
        self._chn_attr[chn].chn_enable = True

        if self._chn_attr[chn].out_win.width \
           and self._chn_attr[chn].out_win.height:
            buf_size = 0
            out_width = self._chn_attr[chn].out_win.width
            out_height = self._chn_attr[chn].out_win.height
            in_width = self._dev_attr.acq_win.width
            in_height = self._dev_attr.acq_win.height

            if pix_format == PIXEL_FORMAT_YUV_SEMIPLANAR_420:
                buf_size = ALIGN_UP((out_width * out_height * 3 // 2), VICAP_ALIGN_4K)
            elif pix_format in [PIXEL_FORMAT_RGB_888, PIXEL_FORMAT_RGB_888_PLANAR]:
                buf_size = ALIGN_UP((out_width * out_height * 3), VICAP_ALIGN_4K)
            elif pix_format in [PIXEL_FORMAT_RGB_BAYER_10BPP, PIXEL_FORMAT_RGB_BAYER_12BPP, \
                                PIXEL_FORMAT_RGB_BAYER_14BPP, PIXEL_FORMAT_RGB_BAYER_16BPP]:
                self._chn_attr[chn].out_win.width = in_width
                self._chn_attr[chn].out_win.height = in_height
                buf_size = ALIGN_UP((in_width * in_height * 2), VICAP_ALIGN_4K)
            else:
                raise RuntimeError(f"sensor({self._dev_id}) chn({chn}) set_pixformat not support pix_format({pix_format})")

            self._chn_attr[chn].buffer_size = buf_size

        # config vb for out channel
        if self._buf_init[chn] == False \
           and self._chn_attr[chn].buffer_num \
           and self._chn_attr[chn].buffer_size:
            config = k_vb_config()
            config.max_pool_cnt = 1
            config.comm_pool[0].blk_size = self._chn_attr[chn].buffer_size
            config.comm_pool[0].blk_cnt = self._chn_attr[chn].buffer_num
            config.comm_pool[0].mode = VB_REMAP_MODE_NOCACHE
            ret = MediaManager._config(config)
            if not ret:
                raise RuntimeError(f"sensor({self._dev_id}) chn({chn}) set_pixformat configure buffer failed({ret})")
            self._buf_init[chn] = True

    def get_pixformat(self, chn = CAM_CHN_ID_0):
        if not self._dev_attr.dev_enable:
            raise AssertionError("should call reset() first")

        # if (self._dev_id > CAM_DEV_ID_MAX - 1):
        #     raise AssertionError(f"invaild sensor id {self._dev_id}, should < {CAM_DEV_ID_MAX - 1}")

        if (chn > CAM_CHN_ID_MAX - 1):
            raise AssertionError(f"invaild chn id {chn}, should < {CAM_CHN_ID_MAX - 1}")

        return self._chn_attr[chn].pix_format

    @staticmethod
    def _parse_framesize(framesize):
        sizes = [
            (88, 72),           #QQCIF
            (176, 144),         #QCIF
            (352, 288),         #CIF
            (176, 120),         #QSIF
            (352, 240),         #SIF
            (160, 120),         #QQVGA
            (320, 240),         #QVGA
            (640, 480),         #VGA
            (120, 80),          #HQQVGA
            (240, 160),         #HQVGA
            (480, 320),         #HVGA
            (64, 64),           #B64X64
            (128, 64),          #B128X64
            (128, 128),         #B128X128
            (160, 160),         #B160X160
            (320, 320),         #B320X320
            (128, 160),         #QQVGA2
            (720, 480),         #WVGA
            (752, 480),         #WVGA2
            (800, 600),         #SVGA
            (1024, 768),        #XGA
            (1280, 768),        #WXGA
            (1280, 1024),       #SXGA
            (1280, 960),        #SXGAM
            (1600, 1200),       #UXGA
            (1280, 720),        #HD
            (1920, 1080),       #FHD
            (2560, 1440),       #QHD
            (2048, 1536),       #QXGA
            (2560, 1600),       #WQXGA
            (2592, 1944),       #WQXGA2
        ]

        if 0 <= framesize < len(sizes):
            return sizes[framesize]
        else:
            return 0, 0

    def set_framesize(self, framesize = FRAME_SIZE_INVAILD, chn = CAM_CHN_ID_0, alignment=0, **kwargs):
        if not self._dev_attr.dev_enable:
            raise AssertionError("should call reset() first")

        # if (self._dev_id > CAM_DEV_ID_MAX - 1):
        #     raise AssertionError(f"invaild sensor id {self._dev_id}, should < {CAM_DEV_ID_MAX - 1}")

        if (chn > CAM_CHN_ID_MAX - 1):
            raise AssertionError(f"invaild chn id {chn}, should < {CAM_CHN_ID_MAX - 1}")

        self._framesize[chn] = framesize

        if 'w' in kwargs and 'h' in kwargs:
            width = kwargs.get('w', 0)
            height = kwargs.get('h', 0)
        elif 'width' in kwargs and 'height' in kwargs:
            width = kwargs.get('width', 0)
            height = kwargs.get('height', 0)
        else:
            width, height = Sensor._parse_framesize(framesize)

        if width % 16 != 0:
            width = ALIGN_UP(width, 16)
            print(f"Warning: sensor({self._dev_id}) chn({chn}) set_framesize align up width to {width}")

        if width > self._dev_attr.acq_win.width or width < CAM_OUT_WIDTH_MIN:
            raise AssertionError(f"sensor({self._dev_id}) chn({chn}) set_framesize invaild width({width}), should be {CAM_OUT_WIDTH_MIN} - {self._dev_attr.acq_win.width}")

        if height > self._dev_attr.acq_win.height or height < CAM_OUT_HEIGHT_MIN:
            raise AssertionError(f"sensor({self._dev_id}) chn({chn}) set_framesize invaild height({height}), should be {CAM_OUT_HEIGHT_MIN} - {self._dev_attr.acq_win.height}")

        self._chn_attr[chn].chn_enable = True
        self._chn_attr[chn].out_win.h_start = 0
        self._chn_attr[chn].out_win.v_start = 0
        self._chn_attr[chn].out_win.width = width
        self._chn_attr[chn].out_win.height = height
        self._chn_attr[chn].alignment = alignment

        if self._chn_attr[chn].pix_format:
            buf_size = 0
            pix_format = self._chn_attr[chn].pix_format
            out_width = self._chn_attr[chn].out_win.width
            out_height = self._chn_attr[chn].out_win.height
            in_width = self._dev_attr.acq_win.width
            in_height = self._dev_attr.acq_win.height

            if pix_format == PIXEL_FORMAT_YUV_SEMIPLANAR_420:
                buf_size = ALIGN_UP((out_width * out_height * 3 // 2), VICAP_ALIGN_4K)
            elif pix_format in [PIXEL_FORMAT_RGB_888, PIXEL_FORMAT_RGB_888_PLANAR]:
                buf_size = ALIGN_UP((out_width * out_height * 3), VICAP_ALIGN_4K)
            elif pix_format in [PIXEL_FORMAT_RGB_BAYER_10BPP, PIXEL_FORMAT_RGB_BAYER_12BPP, \
                                PIXEL_FORMAT_RGB_BAYER_14BPP, PIXEL_FORMAT_RGB_BAYER_16BPP]:
                self._chn_attr[chn].out_win.width = in_width
                self._chn_attr[chn].out_win.height = in_height
                buf_size = ALIGN_UP((in_width * in_height * 2), VICAP_ALIGN_4K)
            else:
                raise RuntimeError(f"sensor({self._dev_id}) chn({chn}) set_framesize not support pix_format({pix_format})")

            self._chn_attr[chn].buffer_size = buf_size

        # config vb for out channel
        if (self._buf_init[chn] == False) \
           and self._chn_attr[chn].buffer_num \
           and self._chn_attr[chn].buffer_size:
            self.buf_init[chn] = True
            config = k_vb_config()
            config.max_pool_cnt = 1
            config.comm_pool[0].blk_size = self.chn_attr[chn].buffer_size
            config.comm_pool[0].blk_cnt = self.chn_attr[chn].buffer_num
            config.comm_pool[0].mode = VB_REMAP_MODE_NOCACHE
            ret = MediaManager._config(config)
            if not ret:
                raise RuntimeError(f"sensor({self._dev_id}) chn({chn}) set_framesize configure buffer failed({ret})")

    def get_framesize(self, chn = CAM_CHN_ID_0):
        if not self._dev_attr.dev_enable:
            raise AssertionError("should call reset() first")

        # if (self._dev_id > CAM_DEV_ID_MAX - 1):
        #     raise AssertionError(f"invaild sensor id {self._dev_id}, should < {CAM_DEV_ID_MAX - 1}")

        if (chn > CAM_CHN_ID_MAX - 1):
            raise AssertionError(f"invaild chn id {chn}, should < {CAM_CHN_ID_MAX - 1}")

        print("please use width() and height()")

        return self._framesize[chn]
        # return (self._chn_attr[chn].out_win.width, self._chn_attr[chn].out_win.height)

    @wrap
    def set_framerate(self, rate):
        pass

    @wrap
    def get_framerate(self):
        pass

    @wrap
    def set_windowing(self, roi):
        pass

    @wrap
    def get_windowing(self):
        pass
    
    @wrap
    def set_contrast(self, constrast):
        pass

    @wrap
    def set_brightness(self, brightness):
        pass

    @wrap
    def set_saturation(self, saturation):
        pass

    @wrap
    def set_quality(self, quality):
        pass

    @wrap
    def set_colorbar(self, enable):
        pass

    @wrap
    def set_auto_gain(self, enable, **kwargs):
        pass

    @wrap
    def get_gain_db(self):
        pass

    @wrap
    def set_auto_exposure(self, enable, **kwargs):
        pass

    @wrap
    def get_exposure_us(self):
        pass

    @wrap
    def set_auto_whitebal(self, enable, **kwargs):
        pass

    @wrap
    def get_rgb_gain_db(self):
        pass

    @wrap
    def set_auto_blc(self, enable, **kwargs):
        pass

    @wrap
    def get_blc_regs(self):
        pass

    def set_hmirror(self, enable):
        if not self._dev_attr.dev_enable:
            raise AssertionError("should call reset() first")

        # if (self._dev_id > CAM_DEV_ID_MAX - 1):
        #     raise AssertionError(f"invaild sensor id {self._dev_id}, should < {CAM_DEV_ID_MAX - 1}")

        if enable:
            self._dev_attr.mirror = self._dev_attr.mirror | 1
        else:
            self._dev_attr.mirror = self._dev_attr.mirror & (~(1))

    def get_hmirror(self) -> bool:
        if not self._dev_attr.dev_enable:
            raise AssertionError("should call reset() first")

        # if (self._dev_id > CAM_DEV_ID_MAX - 1):
        #     raise AssertionError(f"invaild sensor id {self._dev_id}, should < {CAM_DEV_ID_MAX - 1}")

        if self._dev_attr.mirror & 1:
            return True
        else:
            return False

    def set_vflip(self, enable):
        if not self._dev_attr.dev_enable:
            raise AssertionError("should call reset() first")

        # if (self._dev_id > CAM_DEV_ID_MAX - 1):
        #     raise AssertionError(f"invaild sensor id {self._dev_id}, should < {CAM_DEV_ID_MAX - 1}")

        if enable:
            self._dev_attr.mirror = self._dev_attr.mirror | 2
        else:
            self._dev_attr.mirror = self._dev_attr.mirror & (~(2))

    def get_vflip(self) -> bool:
        if not self._dev_attr.dev_enable:
            raise AssertionError("should call reset() first")

        # if (self._dev_id > CAM_DEV_ID_MAX - 1):
        #     raise AssertionError(f"invaild sensor id {self._dev_id}, should < {CAM_DEV_ID_MAX - 1}")

        if self._dev_attr.mirror & 2:
            return True
        else:
            return False

    @wrap
    def set_transpose(self, enable):
        pass

    @wrap
    def get_transpose(self):
        pass

    @wrap
    def set_auto_rotation(self, enable):
        pass

    @wrap
    def get_auto_rotation(self):
        pass

    @wrap
    def set_framebuffers(self, count):
        pass

    @wrap
    def get_framebuffers(self):
        pass

    @wrap
    def disable_delays(self, **kwargs):
        pass

    @wrap
    def disable_full_flush(self, **kwargs):
        pass

    @wrap
    def set_lens_correction(self, enabld, radi, coef):
        pass
    
    @wrap
    def set_vsync_callback(self, cb):
        pass

    @wrap
    def set_frame_callback(self, cb):
        pass

    @wrap
    def ioctl(self, **kwargs):
        pass

    @wrap
    def set_color_palette(self, palette):
        pass
    
    @wrap
    def get_color_palette(self):
        pass

    @wrap
    def __write_reg(self, address, value):
        pass

    @wrap
    def __read_reg(self, address):
        pass

    # custom method
    def run(self):
        if not self._dev_attr.dev_enable:
            raise AssertionError("should call reset() first")

        # if (self._dev_id > CAM_DEV_ID_MAX - 1):
        #     raise AssertionError(f"invaild sensor id {self._dev_id}, should < {CAM_DEV_ID_MAX - 1}")

        if Sensor._is_mcm_device():
            return Sensor._run_mcm_device()

        if self._is_started:
            return

        ret = kd_mpi_vicap_set_dev_attr(self._dev_id, self._dev_attr)
        if ret:
            raise RuntimeError(f"sensor({self._dev_id}) run error, set dev attr failed({ret})")

        # vicap channel attr set
        for chn_num in range(0, VICAP_CHN_ID_MAX):
            if not self._chn_attr[chn_num].chn_enable:
                continue
            ret = kd_mpi_vicap_set_chn_attr(self._dev_id, chn_num, self._chn_attr[chn_num])
            if ret:
                raise RuntimeError(f"sensor({self._dev_id}) run error, set chn({chn_num}) attr failed({ret})")

        ret = kd_mpi_vicap_init(self._dev_id)
        if ret:
            raise RuntimeError(f"sensor({self._dev_id}) run error, vicap init failed({ret})")

        print(f"sensor({self._dev_id}), mode {self._dev_attr.mode}, buffer_num {self._dev_attr.buffer_num}, buffer_size {self._dev_attr.buffer_size}")
        ret = kd_mpi_vicap_start_stream(self._dev_id)
        if ret:
            kd_mpi_vicap_deinit(self._dev_id)
            raise RuntimeError(f"sensor({self._dev_id}) run error, vicap start stream failed({ret})")

        vb_mgmt_vicap_dev_inited(self._dev_id)

        self._is_started = True

    def stop(self, is_del = False):
        # if (self._dev_id > CAM_DEV_ID_MAX - 1):
        #     raise AssertionError(f"invaild sensor id {self._dev_id}, should < {CAM_DEV_ID_MAX - 1}")

        # is_del = kwargs.get('is_del', False)

        if not is_del and not self._dev_attr.dev_enable:
            raise AssertionError("should call reset() first")

        if not is_del and not self._is_started:
            raise AssertionError("should call run() first")

        ret = kd_mpi_vicap_stop_stream(self._dev_id)
        if not is_del and ret:
            raise RuntimeError(f"sensor({self._dev_id}) stop error, stop stream failed({ret})")

        ret = kd_mpi_vicap_deinit(self._dev_id)
        if not is_del and ret:
            raise RuntimeError(f"sensor({self._dev_id}) stop error, vicap deinit failed({ret})")

        vb_mgmt_vicap_dev_deinited(self._dev_id)

        self._dev_attr.dev_enable = False
        self._release_all_chn_image()
        self._is_started = False

        Sensor._devs[self._dev_id] = None

    def bind_info(self, x = 0, y = 0, chn = CAM_CHN_ID_0):
        if not self._dev_attr.dev_enable:
            raise AssertionError("should call reset() first")

        # if (self._dev_id > CAM_DEV_ID_MAX - 1):
        #     raise AssertionError(f"invaild sensor id {self._dev_id}, should < {CAM_DEV_ID_MAX - 1}")

        if (chn > CAM_CHN_ID_MAX - 1):
            raise AssertionError(f"invaild chn id {chn}, should < {CAM_CHN_ID_MAX - 1}")

        width = self.width(chn)
        height = self.height(chn)
        pix_format = self.get_pixformat(chn)

        kwargs = {
            'src': (CAMERA_MOD_ID, self._dev_id, chn),
            'rect': (x, y, width, height),
            'pix_format': pix_format,
        }

        return kwargs
