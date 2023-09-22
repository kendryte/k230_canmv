from media.media import *


def media_buf_test():
    print("media_buf_test start")
    config = k_vb_config()
    config.max_pool_cnt = 10

    config.comm_pool[0].blk_size = 1024*1024
    config.comm_pool[0].blk_cnt = 10
    config.comm_pool[0].mode = VB_REMAP_MODE_NONE

    config.comm_pool[1].blk_size = 2*1024*1024
    config.comm_pool[1].blk_cnt = 10
    config.comm_pool[1].mode = VB_REMAP_MODE_NOCACHE

    config.comm_pool[2].blk_size = 3*1024*1024
    config.comm_pool[2].blk_cnt = 10
    config.comm_pool[3].mode = VB_REMAP_MODE_CACHED

    print("media_buf_test buffer_config 111")
    ret = media.buffer_config(config)
    if ret:
        print("media_buf_test, buffer_config failed")
        return ret

    config.max_pool_cnt = 10

    config.comm_pool[0].blk_size = 1024*1024
    config.comm_pool[0].blk_cnt = 10
    config.comm_pool[0].mode = VB_REMAP_MODE_NONE

    config.comm_pool[1].blk_size = 2*1024*1024
    config.comm_pool[1].blk_cnt = 10
    config.comm_pool[1].mode = VB_REMAP_MODE_NOCACHE

    config.comm_pool[2].blk_size = 3*1024*1024
    config.comm_pool[2].blk_cnt = 10
    config.comm_pool[3].mode = VB_REMAP_MODE_CACHED

    print("media_buf_test buffer_config 222")
    ret = media.buffer_config(config)
    if ret:
        print("media_buf_test, buffer_config failed")
        return ret

    config.max_pool_cnt = 20

    config.comm_pool[0].blk_size = 4*1024*1024
    config.comm_pool[0].blk_cnt = 3
    config.comm_pool[0].mode = VB_REMAP_MODE_NONE

    config.comm_pool[1].blk_size = 5*1024*1024
    config.comm_pool[1].blk_cnt = 3
    config.comm_pool[1].mode = VB_REMAP_MODE_NOCACHE

    config.comm_pool[2].blk_size = 6*1024*1024
    config.comm_pool[2].blk_cnt = 3
    config.comm_pool[3].mode = VB_REMAP_MODE_CACHED

    print("media_buf_test buffer_config 333")
    ret = media.buffer_config(config)
    if ret:
        print("media_buf_test, buffer_config failed")
        return ret

    config.max_pool_cnt = 30

    config.comm_pool[0].blk_size = 4*1024*1024
    config.comm_pool[0].blk_cnt = 5
    config.comm_pool[0].mode = VB_REMAP_MODE_NONE

    config.comm_pool[1].blk_size = 4*1024*1024
    config.comm_pool[1].blk_cnt = 5
    config.comm_pool[1].mode = VB_REMAP_MODE_NOCACHE

    config.comm_pool[2].blk_size = 4*1024*1024
    config.comm_pool[2].blk_cnt = 5
    config.comm_pool[3].mode = VB_REMAP_MODE_CACHED

    print("media_buf_test buffer_config 444")
    ret = media.buffer_config(config)
    if ret:
        print("media_buf_test, buffer_config failed")
        return ret

    print("media_buf_test buffer_init")
    ret = media.buffer_init()
    if ret:
        print("media_buf_test, buffer_init failed")

    ret = media.buffer_deinit()
    if ret:
        print("media_buf_test, buffer_deinit failed")
        return ret

    print("media_buf_test end")
    return 0


media_buf_test()
