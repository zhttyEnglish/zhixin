/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2022 SmartChipc, Inc.
 */

#ifndef __DW_SPI_H_
#define __DW_SPI_H_

int dw_spi_reg_read(unsigned int reg, unsigned int *val);
int dw_spi_init(int gpio, int cs);
int dw_spi_remove(void);

#endif
