#pragma once

#include "mpi_vb_api.h"
#include "mpi_sys_api.h"

typedef struct 
{
    void *virt_addr; // self.virt_addr = virt_addr
    k_u64 phys_addr; // self.phys_addr = phys_addr

    k_vb_blk_handle handle; // self.handle = handle

    k_s32 pool_id;          // self.pool_id = pool_id
    k_u32 size; // self.size = size, user set
}vb_block_info;

// in ide_dbg.c
extern void dma_dev_deinit(void);

extern k_s32 vb_mgmt_init(void);

extern k_s32 vb_mgmt_deinit(void);

extern k_s32 vb_mgmt_get_block(vb_block_info *info);

extern k_s32 vb_mgmt_put_block(vb_block_info *info);

extern k_s32 vb_mgmt_vicap_dev_inited(k_u32 id);

extern k_s32 vb_mgmt_vicap_dev_deinited(k_u32 id);

extern k_s32 vb_mgmt_push_link_info(k_mpp_chn *src, k_mpp_chn *dst);

extern k_s32 vb_mgmt_pop_link_info(k_mpp_chn *src, k_mpp_chn *dst);

extern void vb_mgmt_py_at_exit(void);
