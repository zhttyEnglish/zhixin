/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Header providing constants for SmartChip pmu bindings.
 *
 * Copyright (c) 2021 SmartChip
 */

#ifndef __DT_BINDINGS_PMU_SCA200V100_H__
#define __DT_BINDINGS_PMU_SCA200V100_H__

#define SCA200V100_PWR_CTRL_SOC_VENC         0
#define SCA200V100_PWR_CTRL_SOC_ISP          1
#define SCA200V100_PWR_CTRL_SOC_DLA          2
#define SCA200V100_PWR_CTRL_A53_CORE1        3
#define SCA200V100_PWR_CTRL_A53_CORE2        4
#define SCA200V100_PWR_CTRL_A53_CORE3        5
#define SCA200V100_PWR_CTRL_CEVA_CORE0       6

#define SCA200V100_PWR_CTRL_MAX              7

#define SCA200V100_PWR_CTRL_GATING_SHIFT     16
#define SCA200V100_PWR_CTRL_OFF              (0 << SCA200V100_PWR_CTRL_GATING_SHIFT)

#endif
