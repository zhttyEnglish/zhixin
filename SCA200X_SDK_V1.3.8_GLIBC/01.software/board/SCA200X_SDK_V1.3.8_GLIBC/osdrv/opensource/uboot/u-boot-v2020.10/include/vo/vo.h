/**
 * @file vo.h
 * @brief  vo include file
 * @author SmartChip Software Team
 * @version 0.0.1
 * @date 2021/07/19
 * @license 2021-2025, SmartChip. Co., Ltd.
**/
#ifndef __SC_VO_H__
#define __SC_VO_H__

#ifdef __cplusplus
extern "C" {
#endif

int sc_vo_dev_enable(uint32_t dev_id, uint32_t intf_type, uint32_t sub_intf_type, uint32_t sync_type);
int sc_vo_dev_disable(uint32_t dev_id);
int sc_vo_layer_enable(uint32_t layer_id, uint32_t addr, uint32_t stride,
    uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t csc);
int sc_vo_layer_disable(uint32_t layer_id);
int sc_vo_init(void);

#endif

