from mpp.connector import *
from mpp.vo import *
from mpp import *
from media.media import *
import image
import machine

# define VO channel
DISPLAY_CHN_VIDEO1 = K_VO_DISPLAY_CHN_ID1
DISPLAY_CHN_VIDEO2 = K_VO_DISPLAY_CHN_ID2
DISPLAY_CHN_OSD0 = K_VO_DISPLAY_CHN_ID3
DISPLAY_CHN_OSD1 = K_VO_DISPLAY_CHN_ID4
DISPLAY_CHN_OSD2 = K_VO_DISPLAY_CHN_ID5
DISPLAY_CHN_OSD3 = K_VO_DISPLAY_CHN_ID6


class lcd:
    _init = False
    _ide_show = False
    _width = 0
    _height = 0
    _plane_enable = [False] * K_VO_MAX_CHN_NUMS
    _plane_mirror = [K_VO_MIRROR_NONE] * K_VO_MAX_CHN_NUMS
    _plane_alpha = [255] * K_VO_MAX_CHN_NUMS
    _plane_format = [(0,0,0,0,-1)] * K_VO_MAX_CHN_NUMS
    _plane_buffer = [None] * K_VO_MAX_CHN_NUMS
    _plane_mirror_buffer = [None] * K_VO_MAX_CHN_NUMS
    _video_link = False
    _video_link_source = None
    _video_link_sink = None

    @classmethod
    def init(cls, type, video_enable=True, osd_buf_cnt=1, ide_show=True):
        if cls._init:
            return
        connector_type = type
        connector_info = k_connector_info()
        kd_mpi_get_connector_info(connector_type, connector_info)
        w = connector_info.resolution.hdisplay
        h = connector_info.resolution.vdisplay

        connector_fd = kd_mpi_connector_open(uctypes.string_at(connector_info.connector_name))
        kd_mpi_connector_power_set(connector_fd, 1)
        kd_mpi_connector_init(connector_fd, connector_info)
        kd_mpi_connector_close(connector_fd)
        ide_dbg_vo_init(type)
        ide_dbg_set_vo_wbc(ide_show)

        if type == ST7701_V1_MIPI_2LAN_480X800_30FPS:
            cls._plane_mirror = [K_ROTATION_90] * K_VO_MAX_CHN_NUMS
            osd_buf_cnt = osd_buf_cnt + 2
        else:
            cls._plane_mirror = [K_VO_MIRROR_NONE] * K_VO_MAX_CHN_NUMS

        cls._video_link = False
        if video_enable:
            cls._video_link_source = media_device(CAMERA_MOD_ID, CAM_DEV_ID_0, CAM_CHN_ID_0)
            cls._video_link_sink = media_device(DISPLAY_MOD_ID, DISPLAY_DEV_ID, DISPLAY_CHN_VIDEO1)
            media.create_link(cls._video_link_source, cls._video_link_sink)
            cls._video_link = True
            cls.set_video_plane(0, 0, w, h, PIXEL_FORMAT_YVU_PLANAR_420, DISPLAY_CHN_VIDEO1, cls._plane_mirror[DISPLAY_CHN_VIDEO1])

        if osd_buf_cnt > 0:
            config = k_vb_config()
            config.max_pool_cnt = 1
            config.comm_pool[0].blk_size = 4 * w * h
            config.comm_pool[0].blk_cnt = osd_buf_cnt
            config.comm_pool[0].mode = VB_REMAP_MODE_NOCACHE
            media.buffer_config(config)

        if ide_show:
            config = k_vb_config()
            config.max_pool_cnt = 2
            config.comm_pool[0].blk_size = w * h * 2
            config.comm_pool[0].blk_cnt = 4
            config.comm_pool[0].mode = VB_REMAP_MODE_NOCACHE
            config.comm_pool[1].blk_size = (w * h + 0xfff) & ~0xfff
            config.comm_pool[1].blk_cnt = 2
            config.comm_pool[1].mode = VB_REMAP_MODE_NOCACHE
            media.buffer_config(config)

        cls._plane_alpha = [255] * K_VO_MAX_CHN_NUMS
        cls._plane_format = [(0,0,0,0,-1)] * K_VO_MAX_CHN_NUMS
        cls._plane_buffer = [None] * K_VO_MAX_CHN_NUMS
        cls._width = w
        cls._height = h
        cls._ide_show = ide_show
        cls._init = True

    @classmethod
    def set_osd_plane(cls, x, y, width, height, pixelformat, chn, mirror=None, alpha=None):
        if DISPLAY_CHN_OSD0 > chn or chn > DISPLAY_CHN_OSD3:
            raise ValueError("plane invalid")

        if width & 7:
            raise ValueError("width must be an integral multiple of 8 pixels")

        if mirror != None:
            cls._plane_mirror[chn] = mirror

        if (x,y,width,height,pixelformat) == cls._plane_format[chn]:
            if alpha == None or alpha == cls._plane_alpha[chn]:
                return

        cls._plane_format[chn] = (x,y,width,height,pixelformat)
        if alpha != None:
            cls._plane_alpha[chn] = alpha

        cls.disable_plane(chn)

        offset = k_vo_point()
        offset.x = x
        offset.y = y

        img_size = k_vo_size()
        img_size.width = width
        img_size.height = height

        osd_attr = k_vo_video_osd_attr()
        osd_attr.global_alptha = cls._plane_alpha[chn]
        osd_attr.pixel_format = pixelformat
        if (pixelformat == PIXEL_FORMAT_ARGB_8888 or pixelformat == PIXEL_FORMAT_ABGR_8888):
            osd_attr.stride = (width * 4) // 8
        elif (pixelformat == PIXEL_FORMAT_RGB_888 or pixelformat == PIXEL_FORMAT_BGR_888):
            osd_attr.stride = (width * 3) // 8
        elif (pixelformat == PIXEL_FORMAT_RGB_565_LE or pixelformat == PIXEL_FORMAT_BGR_565_LE):
            osd_attr.stride = (width * 2) // 8
        elif (pixelformat == PIXEL_FORMAT_RGB_MONOCHROME_8BPP):
            osd_attr.stride = (width) // 8
        else:
            raise ValueError("osd plane pixelformat not support")
        struct_copy(offset, osd_attr.display_rect)
        struct_copy(img_size, osd_attr.img_size)
        kd_mpi_vo_set_video_osd_attr(chn - 3, osd_attr)

    @classmethod
    def set_video_plane(cls, x, y, width, height, pixelformat, chn, mirror=None):
        if DISPLAY_CHN_VIDEO1 > chn or chn > DISPLAY_CHN_VIDEO2:
            raise ValueError("plane invalid")

        if width & 7:
            raise ValueError("width must be an integral multiple of 8 pixels")

        if (x,y,width,height,pixelformat) == cls._plane_format[chn]:
            if mirror == None or mirror == cls._plane_mirror[chn]:
                return

        cls._plane_format[chn] = (x,y,width,height,pixelformat)
        if mirror != None:
            cls._plane_mirror[chn] = mirror

        cls.disable_plane(chn)

        offset = k_vo_point()
        offset.x = x
        offset.y = y

        img_size = k_vo_size()
        img_size.width = width
        img_size.height = height

        video_attr = k_vo_video_layer_attr()
        video_attr.pixel_format = pixelformat
        video_attr.func = cls._plane_mirror[chn]
        if pixelformat == PIXEL_FORMAT_YVU_PLANAR_420:
            video_attr.stride = (width // 8 - 1) + ((height - 1) << 16)
        else:
            raise ValueError("video plane pixelformat not support")
        struct_copy(offset, video_attr.display_rect)
        struct_copy(img_size, video_attr.img_size)
        kd_mpi_vo_set_video_layer_attr(chn, video_attr)
        kd_mpi_vo_enable_video_layer(chn)
        cls._plane_enable[chn] = True

    @classmethod
    def set_plane(cls, x, y, width, height, pixelformat, chn, mirror=None, alpha=None):
        if (DISPLAY_CHN_VIDEO1 <= chn <= DISPLAY_CHN_VIDEO2):
            cls.set_video_plane(x, y, width, height, pixelformat, chn, mirror=mirror)
        elif (DISPLAY_CHN_OSD0 <= chn <= DISPLAY_CHN_OSD3):
            cls.set_osd_plane(x, y, width, height, pixelformat, chn, mirror=mirror, alpha=alpha)
        else:
            raise ValueError("plane invalid")

    @classmethod
    def disable_plane(cls, chn):
        if cls._plane_enable[chn] == False:
            return
        if (DISPLAY_CHN_VIDEO1 <= chn <= DISPLAY_CHN_VIDEO2):
            kd_mpi_vo_disable_video_layer(chn)
        elif (DISPLAY_CHN_OSD0 <= chn <= DISPLAY_CHN_OSD3):
            kd_mpi_vo_osd_disable(chn - DISPLAY_CHN_OSD0)
        else:
            raise ValueError("disable_plane failed")
        cls._plane_enable[chn] = False

    @classmethod
    def show_image(cls, img, x=0, y=0, chn=DISPLAY_CHN_OSD0):
        format = img.format()
        width = img.width()
        height = img.height()

        if width & 7:
            raise ValueError("Image width must be an integral multiple of 8 pixels")
        if width * height > cls._width * cls._height:
            raise ValueError("Image size is too large")

        if format == image.ARGB8888:
            pixelformat = PIXEL_FORMAT_ARGB_8888
        elif format == image.RGB888:
            pixelformat = PIXEL_FORMAT_RGB_888
        elif format == image.RGB565:
            pixelformat = PIXEL_FORMAT_RGB_565_LE
        elif format == image.GRAYSCALE:
            pixelformat = PIXEL_FORMAT_RGB_MONOCHROME_8BPP
        else:
            raise ValueError("Image format not support")

        if cls._plane_buffer[chn] == None:
            try:
                cls._plane_buffer[chn] = media.request_buffer(4 * cls._width * cls._height)
            except Exception:
                print("please check osd_buf_cnt paramter")
                raise
        if cls._plane_mirror[chn] != K_VO_MIRROR_NONE and cls._plane_mirror_buffer[chn] == None:
            try:
                cls._plane_mirror_buffer[chn] = media.request_buffer(4 * cls._width * cls._height)
            except Exception:
                print("please check osd_buf_cnt paramter")
                raise

        if (pixelformat == PIXEL_FORMAT_ARGB_8888 or pixelformat == PIXEL_FORMAT_ABGR_8888):
            stride = 4
        elif (pixelformat == PIXEL_FORMAT_RGB_888 or pixelformat == PIXEL_FORMAT_BGR_888):
            stride = 3
        elif (pixelformat == PIXEL_FORMAT_RGB_565_LE or pixelformat == PIXEL_FORMAT_BGR_565_LE):
            stride = 2
        elif (pixelformat == PIXEL_FORMAT_RGB_MONOCHROME_8BPP):
            stride = 1
        else:
            stride = 1

        if cls._plane_mirror[chn] == K_ROTATION_90:
            _x, _y, _w, _h = x, y, width, height
            x = cls._width - _h - _y
            y = _x
            width = _h
            height = _w
            input_frame = k_video_frame_info()
            input_frame.pool_id = cls._plane_mirror_buffer[chn].pool_id
            input_frame.v_frame.width = _w
            input_frame.v_frame.height = _h
            input_frame.v_frame.pixel_format = pixelformat
            input_frame.v_frame.stride[0] = _w * stride
            input_frame.v_frame.phys_addr[0] = cls._plane_mirror_buffer[chn].phys_addr
            input_frame.v_frame.virt_addr[0] = cls._plane_mirror_buffer[chn].virt_addr
            machine.mem_copy(cls._plane_mirror_buffer[chn].virt_addr, img.virtaddr(), img.size())
            output_frame = k_video_frame_info()
            output_frame.v_frame.width = width
            output_frame.v_frame.height = height
            output_frame.v_frame.stride[0] = width * stride
            output_frame.v_frame.virt_addr[0] = cls._plane_buffer[chn].virt_addr
            kd_mpi_vo_osd_rotation(K_ROTATION_90, input_frame, output_frame)
        else:
            machine.mem_copy(cls._plane_buffer[chn].virt_addr, img.virtaddr(), img.size())

        cls.set_osd_plane(x, y, width, height, pixelformat, chn)

        if cls._plane_enable[chn]:
            return

        frame_info = k_video_frame_info()
        frame_info.mod_id = K_ID_VO
        frame_info.pool_id = cls._plane_buffer[chn].pool_id
        frame_info.v_frame.width = width
        frame_info.v_frame.height = height
        frame_info.v_frame.pixel_format = pixelformat
        frame_info.v_frame.stride[0] = width * stride
        frame_info.v_frame.phys_addr[0] = cls._plane_buffer[chn].phys_addr
        kd_mpi_vo_chn_insert_frame(chn, frame_info)
        kd_mpi_vo_osd_enable(chn - 3)
        cls._plane_enable[chn] = True

    @classmethod
    def deinit(cls):
        for i in range(0, K_VO_MAX_CHN_NUMS):
            if cls._plane_enable[i]:
                cls.disable_plane(i)
        ide_dbg_vo_deinit()
        kd_display_reset()

        for i in range(0, K_VO_MAX_CHN_NUMS):
            if cls._plane_buffer[i] != None:
                media.release_buffer(cls._plane_buffer[i])
            if cls._plane_mirror_buffer[i] != None:
                media.release_buffer(cls._plane_mirror_buffer[i])

        if cls._video_link:
            media.destroy_link(cls._video_link_source, cls._video_link_sink)

        cls._init = False

    @classmethod
    def enable_ide_compress(cls, ide_show=True):
        if cls._ide_show != True:
            raise OSError("ide show is disabled, please call init")
        ide_dbg_set_vo_wbc(ide_show)
