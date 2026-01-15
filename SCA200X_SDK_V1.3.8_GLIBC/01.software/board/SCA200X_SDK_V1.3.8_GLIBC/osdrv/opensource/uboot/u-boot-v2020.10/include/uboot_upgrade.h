#ifndef _UBOOT_UPGRADE_H
#define _UBOOT_UPGRADE_H

#define ROM_CODE_START_FLASH_OFS    (0x0)

#define FLASH_SHIFT                 (9)
#define FLASH_BLKSZ                 (512)
#define GPT_BUF_SZ                  (34 * FLASH_BLKSZ)

#define SDK_VERSION_SIZE        128
/* FIXME, 64 is enough now */
#define PART_NUM_MAX            64
#define SC_PART_MAX_BOOTARGS    (PART_NUM_MAX + 1)

/* the MSB in gpt entry field reserve means whether this partition is valid */
#define PARTITION_VALID_SHIFT   44
#define PARTITION_VALID_MASK    (1ULL << PARTITION_VALID_SHIFT)

#define MMC_PART_UDA            (0)
#define MMC_PART_BOOT_AERA_1    (1)
#define MMC_PART_BOOT_AERA_2    (2)

#endif /* _UBOOT_UPGRADE_H */
