/**
 * io.h - Cadence USB3 IO Header
 *
 * Copyright (C) 2016 Cadence Design Systems - https://www.cadence.com/
 *
 * Authors: Rafal Ozieblo <rafalo@cadence.com>,
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2  of
 * the License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __DRIVERS_USB_CDNS_IO_H
#define __DRIVERS_USB_CDNS_IO_H

#include <linux/io.h>

static inline u32 cdns_readl(volatile uint32_t __iomem *reg)
{
	u32 value = 0;

	value = readl(reg);
	return value;
}

static inline void cdns_writel(volatile uint32_t __iomem *reg, u32 value)
{
	writel(value, reg);
}

#endif /* __DRIVERS_USB_CDNS_IO_H */
