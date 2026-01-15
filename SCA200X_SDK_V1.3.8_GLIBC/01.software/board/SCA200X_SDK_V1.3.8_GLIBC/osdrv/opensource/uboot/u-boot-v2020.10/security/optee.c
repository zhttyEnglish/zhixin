#include <common.h>

#define OPTEE_MAGIC        0x4554504f
#define OPTEE_VERSION      1
#define OPTEE_ARCH_ARM32   0
#define OPTEE_ARCH_ARM64   1

struct optee_header {
	uint32_t magic;
	uint8_t  version;
	uint8_t  arch_id;
	uint16_t flags;
	uint32_t init_size;
	uint32_t init_load_addr_hi;
	uint32_t init_load_addr_lo;
	uint32_t init_mem_usage;
	uint32_t paged_size;
};

extern void *memmove(void *dest, const void *src, size_t count);

int optee_parse_header(unsigned int src, unsigned len, unsigned int *ep)
{
	struct optee_header *hdr = (struct optee_header *)(ulong)src;
	unsigned int hdr_len = sizeof(struct optee_header);

	printf("Optee verify image\n");
	if(hdr-> magic != OPTEE_MAGIC ||
	    hdr->version != OPTEE_VERSION ||
	    hdr->arch_id != OPTEE_ARCH_ARM32) {
		printf("Bad optee image\n");
		return -1;
	}

	printf("Optee verify image success\n");
	printf("Move optee to address 0x%x...\n", hdr->init_load_addr_lo);

	/* move image to optee entry point */
	memmove((void*)(ulong)hdr->init_load_addr_lo, (void*)(ulong)(src + hdr_len), len - hdr_len);

	printf("Move optee to address 0x%x done\n", hdr->init_load_addr_lo);

	/* set secondary entry point register */
	//todo fix
	//*(int *)0x6061002c = hdr->init_load_addr_lo;

	/* return optee entry point */
	*ep = hdr->init_load_addr_lo;

	return 0;
}
