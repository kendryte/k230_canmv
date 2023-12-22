# Meida Example
#
# Note: You will need an SD card to run this example.
#
# You can get how to use the meida api form this example.

from media.media import *
import os

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

    # config meida buffer
    media.buffer_config(config)

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

    # config meida buffer
    media.buffer_config(config)

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

    # config meida buffer
    media.buffer_config(config)

    print("media_buf_test buffer_init")
    # init meida buffer
    media.buffer_init()

    print("media_buf_test request_buffer")
    # request meida buffer
    buffer = media.request_buffer(4*1024*1024)
    print(f"buffer handle({buffer.handle})")
    print(f"buffer pool_id({buffer.pool_id})")
    print(f"buffer phys_addr({buffer.phys_addr})")
    print(f"buffer virt_addr({buffer.virt_addr})")
    print(f"buffer size({buffer.size})")
    # release meida buffer
    media.release_buffer(buffer)

    print("media_buf_test buffer_deinit")
    # deinit meida buffer
    media.buffer_deinit()

    print("media_buf_test end")

if __name__ == "__main__":
    os.exitpoint(os.EXITPOINT_ENABLE)
    media_buf_test()
