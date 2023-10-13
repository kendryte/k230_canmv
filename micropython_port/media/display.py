from mpp.connector import *
from mpp.vo import *
from mpp import *
import image

# define display type
HX8377_1080X1920_30FPS = HX8377_V2_MIPI_4LAN_1080X1920_30FPS
LT9611_1920X1080_30FPS = LT9611_MIPI_4LAN_1920X1080_30FPS

# define VO channel
DISPLAY_CHN_VIDEO1 = K_VO_DISPLAY_CHN_ID1
DISPLAY_CHN_VIDEO2 = K_VO_DISPLAY_CHN_ID2
DISPLAY_CHN_OSD0 = K_VO_DISPLAY_CHN_ID3
DISPLAY_CHN_OSD1 = K_VO_DISPLAY_CHN_ID4
DISPLAY_CHN_OSD2 = K_VO_DISPLAY_CHN_ID5
DISPLAY_CHN_OSD3 = K_VO_DISPLAY_CHN_ID6

# define pixelformat
DISPLAY_OUT_NV12 = PIXEL_FORMAT_YVU_PLANAR_420
DISPLAY_OUT_ARGB8888 = PIXEL_FORMAT_ARGB_8888
DISPLAY_OUT_RGB888 = PIXEL_FORMAT_RGB_888
DISPLAY_OUT_RGB565 = PIXEL_FORMAT_RGB_565

# define VO mirror
DISPLAY_MIRROR_NONE = K_VO_MIRROR_NONE
DISPLAY_MIRROR_HOR = K_VO_MIRROR_HOR
DISPLAY_MIRROR_VER = K_VO_MIRROR_VER
DISPLAY_MIRROR_BOTH = K_VO_MIRROR_BOTH

class display:
    plane_array = [0] * 7

    @classmethod
    def init(cls, type):
        connector_type = type
        connector_info = k_connector_info()
        kd_mpi_get_connector_info(connector_type, connector_info)

        connector_fd = kd_mpi_connector_open(uctypes.string_at(connector_info.connector_name))
        kd_mpi_connector_power_set(connector_fd, 1)
        kd_mpi_connector_init(connector_fd, connector_info)
        cls.plane_array = [0] * 7

    @classmethod
    def set_osd_plane(cls, x, y, width, height, pixelformat, chn):
        offset = k_vo_point()
        offset.x = x
        offset.y = y

        img_size = k_vo_size()
        img_size.width = width
        img_size.height = height

        osd_attr = k_vo_video_osd_attr()
        osd_attr.global_alptha = 0xff
        osd_attr.pixel_format = pixelformat
        if (pixelformat == DISPLAY_OUT_ARGB8888):
            osd_attr.stride = (width * 4) // 8
        elif (pixelformat == DISPLAY_OUT_RGB888):
            osd_attr.stride = (width * 3) // 8
        elif (pixelformat == DISPLAY_OUT_RGB565):
            osd_attr.stride = (width * 2) // 8
        else:
            print('set osd pixelformat failed')
        struct_copy(offset, osd_attr.display_rect)
        struct_copy(img_size, osd_attr.img_size)
        kd_mpi_vo_set_video_osd_attr(chn - 3, osd_attr)
        kd_mpi_vo_osd_enable(chn - 3)
        cls.plane_array[chn] = 1

    @classmethod
    def set_video_plane(cls, x, y, width, height, pixelformat, mirror, chn):
        offset = k_vo_point()
        offset.x = x
        offset.y = y

        img_size = k_vo_size()
        img_size.width = width
        img_size.height = height

        video_attr = k_vo_video_layer_attr()
        video_attr.pixel_format = pixelformat
        video_attr.func = mirror
        video_attr.stride = (width // 8 - 1) + ((height - 1) << 16);
        struct_copy(offset, video_attr.display_rect)
        struct_copy(img_size, video_attr.img_size)
        kd_mpi_vo_set_video_layer_attr(chn, video_attr)
        kd_mpi_vo_enable_video_layer(chn)
        cls.plane_array[chn] = 1

    @classmethod
    def set_plane(cls, x, y, width, height, pixelformat, mirror, chn):
        if (DISPLAY_CHN_VIDEO1 <= chn <= DISPLAY_CHN_VIDEO2):
            cls.set_video_plane(x, y, width, height, pixelformat, mirror, chn)
        elif (DISPLAY_CHN_OSD0 <= chn <= DISPLAY_CHN_OSD3):
            cls.set_osd_plane(x, y, width, height, pixelformat, chn)
        else:
            print('set_plane failed')

    @classmethod
    def disable_plane(cls, chn):
        if (DISPLAY_CHN_VIDEO1 <= chn <= DISPLAY_CHN_VIDEO2):
            kd_mpi_vo_disable_video_layer(chn)
        elif (DISPLAY_CHN_OSD0 <= chn <= DISPLAY_CHN_OSD3):
            kd_mpi_vo_osd_disable(chn - 3)
        else:
            print('disable_plane failed')
        cls.plane_array[chn] = 0

    @classmethod
    def show_image(cls, img, x, y, chn):
        width = img.width()
        height = img.height()
        phys_addr = img.phyaddr()
        pool_id = img.poolid()
        if img.format() == image.ARGB8888:
            pixelformat = DISPLAY_OUT_ARGB8888
        elif img.format() == image.RGB888:
            pixelformat = DISPLAY_OUT_RGB888
        elif img.format() == image.RGB565:
            pixelformat = DISPLAY_OUT_RGB565
        elif img.format() == image.YUV420:
            pixelformat = PIXEL_FORMAT_YUV_SEMIPLANAR_420
        cls.set_plane(x, y, width, height, pixelformat, DISPLAY_MIRROR_NONE, chn)

        frame_info = k_video_frame_info()
        frame_info.mod_id = K_ID_VO;
        frame_info.pool_id = pool_id;
        frame_info.v_frame.width = width
        frame_info.v_frame.height = height
        frame_info.v_frame.pixel_format = pixelformat
        frame_info.v_frame.stride[0] = width
        frame_info.v_frame.phys_addr[0] = phys_addr;
        if pixelformat == PIXEL_FORMAT_YUV_SEMIPLANAR_420:
            frame_info.v_frame.phys_addr[1] = phys_addr + (width * height)
        kd_mpi_vo_chn_insert_frame(chn, frame_info)

    @classmethod
    def deinit(cls):
        for i in range(0, 7):
            if cls.plane_array[i] == 1:
                cls.disable_plane(i)
                print(i)
        kd_display_reset()

