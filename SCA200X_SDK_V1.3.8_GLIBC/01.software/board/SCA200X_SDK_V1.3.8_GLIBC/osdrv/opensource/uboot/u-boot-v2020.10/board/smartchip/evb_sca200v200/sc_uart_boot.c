// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2020 SmartChip Electronics Co., Ltd
 */

#include <common.h>
#include <debug_uart.h>
#include <dm.h>
#include <hang.h>
#include <image.h>
#include <asm/arch/image.h>
#include <init.h>
#include <ram.h>
#include <spl.h>
#include <asm/io.h>
#include <linux/bitops.h>
#include <asm/arch/boot.h>
#include <configs/smartchip-common.h>
#include <serial.h>

//#define DEBUG
#ifdef DEBUG
	#define sc_dbg(fmt, args...) printf(fmt, ##args)
#else
	#define sc_dbg(fmt, args...)
#endif

#define SC_MAGIC_HEADER_1           'S'
#define SC_MAGIC_HEADER_2           'C'
#define SC_UART_ACK_CHAR            'Y'
#define SC_UART_NAK_CHAR            'N'

#define SC_UART_CMD_SEND_DATA 0
#define SC_UART_CMD_BURN_DDR  1
#define SC_UART_CMD_BURN_EMMC 2
#define SC_UART_CMD_CH_BAUDRATE 3

/* 20 byte fbl header */
struct sc_uart_cmd_header {
	uint8_t     magic_header[2];
	uint16_t    cmd; /* SC_UART_CMD_SEND_DATA... */
	uint32_t    len;

	uint32_t    dst_addr_l; /* addr */
	uint32_t    dst_addr_h;

	uint8_t data_checksum;
	uint8_t data_type;          //1: compress. 0:not compress.
	uint8_t need_decompress;    //1: need to decompress.
	uint8_t header_checksum;
};

static int sc_uart_readn(void *p, int size)
{
	int key;
	char *dst = p;

	while (size > 0) {
		if (serial_tstc()) {
			key = serial_getc();
			*dst = key;
			dst++;
			size --;
		}
	}
	return 0;
}

static uint64_t __maybe_unused sc_uart_get_dst_addr(const struct sc_uart_cmd_header *header)
{
	uint64_t dst_addr;

	dst_addr = ((uint64_t)header->dst_addr_h << 32 ) | header->dst_addr_l;

	return dst_addr;
}

static int sc_uart_get_header(char *buf)
{
	int id = 0;

	while(2 != id) {
		sc_uart_readn(buf + id, 1);
		switch(buf[id]) {
		case SC_MAGIC_HEADER_1:
			if(0 != id) {
				buf[0] = buf[id];
			}
			id = 1;
			break;
		case SC_MAGIC_HEADER_2:
			if(1 == id) {
				id = 2;
			} else {
				id = 0;
			}
			break;
		default:
			id = 0;
			break;
		}
	}
	return 0;

}

static unsigned char __maybe_unused sc_checksum(unsigned char * tmp_buf, unsigned int buf_len)
{
	unsigned char tmp_checksum = 0;
	unsigned int i = 0;
	for (i = 0; i < buf_len; ++i)
		tmp_checksum += tmp_buf[i];

	return tmp_checksum;
}

