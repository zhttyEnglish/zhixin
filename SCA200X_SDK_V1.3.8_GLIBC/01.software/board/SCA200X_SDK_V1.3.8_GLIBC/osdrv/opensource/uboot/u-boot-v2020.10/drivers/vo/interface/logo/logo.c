#include <stdint.h>
#define STR2(x) #x
#define STR(x) STR2(x)

#define INCBIN(name, file) \
    __asm__(".pushsection .rodata\n" \
            ".global incbin_" STR(name) "_start\n" \
            ".type incbin_" STR(name) "_start, %object\n" \
            ".balign 16\n" \
            "incbin_" STR(name) "_start:\n" \
            ".incbin \"" file "\"\n" \
            \
            ".global incbin_" STR(name) "_end\n" \
            ".type incbin_" STR(name) "_end, %object\n" \
            ".balign 1\n" \
            "incbin_" STR(name) "_end:\n" \
            ".byte 0\n" \
            ".popsection" \
    ); \
extern const __attribute__((aligned(128))) void* incbin_ ## name ## _start; \
extern const void* incbin_ ## name ## _end; \

#define LOGO_X 760
#define LOGO_Y 480
#define LOGO_W 400
#define LOGO_H 120
INCBIN(logo, "drivers/sc_display/interface/logo/logo.bin");
static uint32_t logo_addr;

void *get_logo_bin_addr(uint32_t *start_addr, uint32_t *size)
{
	if((!start_addr) || (!size))
		return -1;
	*start_addr = (uint32_t)&incbin_logo_start;
	*size = ((uint32_t)&incbin_logo_end - (uint32_t)&incbin_logo_start);
	return (void *)(*start_addr);
}

void get_logo_bin_pos_wh(int *x, int *y, int *w, int *h)
{
	*w = LOGO_W;
	*h = LOGO_H;
	*x = LOGO_X;
	*y = LOGO_Y;
	return;
}
