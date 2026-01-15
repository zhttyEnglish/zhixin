#ifndef _SMARTCHIP_LINUX_ION_H_
#define _SMARTCHIP_LINUX_ION_H_

#include <linux/ioctl.h>
#include <linux/types.h>

#define SMARTCHIP_ION_ID_SYSTEM            0

#define SMARTCHIP_ION_ID_CMA_BASEBAND      10
#define SMARTCHIP_ION_ID_CMA_DSP           11
#define SMARTCHIP_ION_ID_CMA_ICC           12

#define SMARTCHIP_ION_ID_CARVEOUT_BASEBAND 20
#define SMARTCHIP_ION_ID_CARVEOUT_DSP      21
#define SMARTCHIP_ION_ID_CARVEOUT_ICC      22

int smartchip_client_create(unsigned char *client_name);
int smartchip_alloc_buffer(unsigned char *client_name, size_t len, size_t align,
    int heap_id_mask, unsigned int flags, void **va, void **pa);
int smartchip_free_buffer(unsigned char *client_name, int handle_id);
int smartchip_destroy_client(unsigned char *client_name);

#endif
