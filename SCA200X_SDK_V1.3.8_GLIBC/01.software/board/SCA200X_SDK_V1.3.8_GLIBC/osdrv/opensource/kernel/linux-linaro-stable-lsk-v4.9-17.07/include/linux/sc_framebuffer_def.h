#ifndef __SC_FRAMEBUFFER_DEF_H
#define __SC_FRAMEBUFFER_DEF_H

#include <linux/fb.h>

typedef struct {
	unsigned int x;
	unsigned int y;
} sc_fb_cursor_pos_t;

#define SC_FRAMEBUFFER_IO_CURSOR_SET_ICON           _IOW('F', 0x81, __u32)
#define SC_FRAMEBUFFER_IO_CURSOR_DISPLAY_ENABLE     _IOW('F', 0x82, __u32)
#define SC_FRAMEBUFFER_IO_CMD_CURSOR_SET_POS        _IOW('F', 0x83, sc_fb_cursor_pos_t)

#endif
