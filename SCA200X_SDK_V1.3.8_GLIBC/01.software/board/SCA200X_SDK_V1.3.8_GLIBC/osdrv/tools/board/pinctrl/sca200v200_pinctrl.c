#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "pinctrl.h"

#define SCA200V200_DTS_M "smartchip,smartx-sca200v200"

/* 每个pad 占用 8bit
 * 每个寄存器 配置4个 pad
 */
#define REG_CONFIG_ONEPAD_MASK      0x000000ff
#define REG_CONFIG_ONEPAD_WIDTH     8
#define REG_CONFIG_PAD_PER_REG      4

/* 每个pad 占用 4bit
 * 每个寄存器 配置8个 pad
 */
#define REG_FUNCSEL_ONEPAD_MASK     0x00000007
#define REG_FUNCSEL_ONEPAD_WIDTH    4
#define REG_FUNCSEL_PAD_PER_REG     8

#define PB_MSC_REG0_OFFSET      0x00000000
#define PB_MSC_SW_OFFSET        0x00000004
#define PB_MSC_SEL_OFFSET       0x0000007c
#define PB_MSC_WDT_OFFSET       0x00000138

#define WDT_PAD_0           (10)
#define WDT_PAD_1           (22)
#define WDT_PAD_2           (61)

#define WDT_PAD_0_MODE          (3)
#define WDT_PAD_1_MODE          (4)
#define WDT_PAD_2_MODE          (2)

#define REG_BASE_PIN_CTRL        0x0A10A000
#define REG_SIZE_PIN_CTRL        0x13C

#define REG_OFFSET_PB_SW  0

/* 用户命令行的 config */
#define CONFIG_CONFIG_MASK         0x00ff
#define CONFIG_CONFIG_SHIFT        0x3
#define CONFIG_MODE_MASK           0x0007
#define CONFIG_MODE_SHIFT          0x0

/* 共有多少个引脚 */
#define PIN_CNT 110

struct sca200v200_pin_info {
    unsigned char pin_count;
    unsigned int select_offset;
    unsigned int config_offset;
};

struct sca200v200_pad_info {
    unsigned char pin_index;
    unsigned int select_offset;
    unsigned int config_offset;
};

struct mem_ctl
{

    /* for read, write */
    void *virt_addr;

    /* for free */
    int fd;
    void *map_base;
    uint32_t mapped_size;
};

struct sca200v200_pinctrl_priv
{
    struct mem_ctl pin_ctl;
    struct sca200v200_pad_info pad_info[PIN_CNT];
    void *pb_sw;
};

struct sca200v200_pin_info group_info[] = {
    {23, 0x7c, 0x04},
    {22, 0x88, 0x1c},
    {26, 0x94, 0x34},
    {6,  0xa4, 0x50},
    {6,  0xa8, 0x58},
    {11, 0xac, 0x60},
    {16, 0xb4, 0x6c}
};


static uint32_t readl(const void *addr)
{
    return *(volatile uint32_t *)addr;
}

static void writel(uint32_t val, const void *addr)
{
    *(volatile uint32_t *)addr = val;
}

static void perror_die(const char *errmsg)
{
    printf("err: %s\n", errmsg);
    perror("err");
    exit(1);

}

static int devmem_init(struct mem_ctl *pctl, off_t target, uint32_t msize)
{
    void *map_base;
    unsigned page_size, mapped_size, offset_in_page;
    int fd;

    fd = open("/dev/mem", O_RDWR | O_SYNC);

    page_size = getpagesize();
    offset_in_page = (unsigned)target & (page_size - 1);
    mapped_size = (msize + offset_in_page + page_size - 1) / page_size * page_size;

    map_base = mmap(NULL,
            mapped_size,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            fd,
            target & ~(off_t)(page_size - 1));
    if (map_base == MAP_FAILED)
        perror_die("mmap");

    //  printf("Memory mapped at address %p.\n", map_base);

    pctl->virt_addr = map_base + offset_in_page;
    pctl->fd = fd;
    pctl->map_base = map_base;
    pctl->mapped_size = mapped_size;

    return 0;
}

static int devmem_exit(struct mem_ctl *pctl)
{
    if (munmap(pctl->map_base, pctl->mapped_size) == -1)
        perror_die("munmap");
    close(pctl->fd);
    return 0;
}

static int sca200v200_pin_list(const struct sca200v200_pinctrl_priv *priv)
{
    uint32_t pin_config;
    uint32_t config;
    uint32_t mode;

    int pin_index;

    uint32_t reg_tmp;
    void *reg_addr;
    unsigned int bit_offset;

    const struct sca200v200_pad_info *pad_info;

    pad_info = priv->pad_info;
    printf("pin_index, pin_config(func_mode, config)\n");
    for(pin_index = 0; pin_index < PIN_CNT; pin_index++)
    {
        //1. get sw config
        reg_addr = priv->pb_sw + pad_info[pin_index].config_offset + pad_info[pin_index].pin_index / REG_CONFIG_PAD_PER_REG * 4;
        bit_offset = pad_info[pin_index].pin_index % REG_CONFIG_PAD_PER_REG * REG_CONFIG_ONEPAD_WIDTH;

        reg_tmp = readl(reg_addr);
        config = (reg_tmp >> bit_offset) & REG_CONFIG_ONEPAD_MASK;

        //2. get pad select (pad function mode)
        reg_addr = priv->pb_sw + pad_info[pin_index].select_offset + pad_info[pin_index].pin_index / REG_FUNCSEL_PAD_PER_REG * 4;
        bit_offset = pad_info[pin_index].pin_index % REG_FUNCSEL_PAD_PER_REG * REG_FUNCSEL_ONEPAD_WIDTH;

        reg_tmp = readl(reg_addr);
        mode = (reg_tmp >> bit_offset) & REG_FUNCSEL_ONEPAD_MASK;

        pin_config = (config << CONFIG_CONFIG_SHIFT) | mode;
        printf("PB%03d, %#x (%d, %#x)\n",
                pin_index, pin_config, mode, config);
    }

    return 0;
}

static int sca200v200_pin_set(const struct sca200v200_pinctrl_priv *priv, int pin_index, unsigned int pin_config)
{
    uint32_t config;
    uint32_t mode;

    uint32_t reg_tmp;
    void *reg_addr;
    unsigned int bit_offset;

    const struct sca200v200_pad_info *pad_info;

    pad_info = priv->pad_info;

    config = (pin_config >> CONFIG_CONFIG_SHIFT) & CONFIG_CONFIG_MASK;
    mode = (pin_config >> CONFIG_MODE_SHIFT) & CONFIG_MODE_MASK;

    printf("pin set: pin_index=%d, pin_config=%#x (func_mode=%d, config=%#x)\n",
            pin_index, pin_config, mode, config);

    //1. set sw config
    reg_addr = priv->pb_sw + pad_info[pin_index].config_offset + pad_info[pin_index].pin_index / REG_CONFIG_PAD_PER_REG * 4;
    bit_offset = pad_info[pin_index].pin_index % REG_CONFIG_PAD_PER_REG * REG_CONFIG_ONEPAD_WIDTH;

    reg_tmp = readl(reg_addr);
    reg_tmp &= ~(REG_CONFIG_ONEPAD_MASK << bit_offset);
    reg_tmp |= config << bit_offset;
    writel(reg_tmp, reg_addr);

    //2. set pad select (pad function mode)
    reg_addr = priv->pb_sw + pad_info[pin_index].select_offset + pad_info[pin_index].pin_index / REG_FUNCSEL_PAD_PER_REG * 4;
    bit_offset = pad_info[pin_index].pin_index % REG_FUNCSEL_PAD_PER_REG * REG_FUNCSEL_ONEPAD_WIDTH;

    reg_tmp = readl(reg_addr);
    reg_tmp &= ~(REG_FUNCSEL_ONEPAD_MASK << bit_offset);
    reg_tmp |= mode << bit_offset;
    writel(reg_tmp, reg_addr);

    if(WDT_PAD_0 == pin_index && WDT_PAD_0_MODE == mode) {
        reg_tmp = readl(priv->pb_sw + PB_MSC_WDT_OFFSET);
        reg_tmp |= 0x1;
        writel(reg_tmp, priv->pb_sw + PB_MSC_WDT_OFFSET);
    }

    if(WDT_PAD_1 == pin_index && WDT_PAD_1_MODE == mode) {
        reg_tmp = readl(priv->pb_sw + PB_MSC_WDT_OFFSET);
        reg_tmp |= 0x2;
        writel(reg_tmp, priv->pb_sw + PB_MSC_WDT_OFFSET);
    }

    if(WDT_PAD_2 == pin_index && WDT_PAD_2_MODE == mode) {
        reg_tmp = readl(priv->pb_sw + PB_MSC_WDT_OFFSET);
        reg_tmp |= 0x4;
        writel(reg_tmp, priv->pb_sw + PB_MSC_WDT_OFFSET);
    }

    return 0;
}

static int sca200v200_pin_get(const struct sca200v200_pinctrl_priv *priv, int pin_index)
{
    uint32_t pin_config;
    uint32_t config;
    uint32_t mode;

    uint32_t reg_tmp;
    void *reg_addr;
    unsigned int bit_offset;

    const struct sca200v200_pad_info *pad_info;

    pad_info = priv->pad_info;

    //1. get sw config
    reg_addr = priv->pb_sw + pad_info[pin_index].config_offset + pad_info[pin_index].pin_index / REG_CONFIG_PAD_PER_REG * 4;
    bit_offset = pad_info[pin_index].pin_index % REG_CONFIG_PAD_PER_REG * REG_CONFIG_ONEPAD_WIDTH;

    reg_tmp = readl(reg_addr);
    config = (reg_tmp >> bit_offset) & REG_CONFIG_ONEPAD_MASK;

    //2. get pad select (pad function mode)
    reg_addr = priv->pb_sw + pad_info[pin_index].select_offset + pad_info[pin_index].pin_index / REG_FUNCSEL_PAD_PER_REG * 4;
    bit_offset = pad_info[pin_index].pin_index % REG_FUNCSEL_PAD_PER_REG * REG_FUNCSEL_ONEPAD_WIDTH;

    reg_tmp = readl(reg_addr);
    mode = (reg_tmp >> bit_offset) & REG_FUNCSEL_ONEPAD_MASK;

    pin_config = (config << CONFIG_CONFIG_SHIFT) | mode;
    printf("pin get: pin_index=%d, pin_config=%#x (func_mode=%d, config=%#x)\n",
            pin_index, pin_config, mode, config);

    return 0;
}

static int pad_info_init(struct sca200v200_pad_info *pad_info)
{
    int idx;
    int i, j;

    idx = 0;
    for(i = 0; i < (sizeof(group_info) / sizeof(group_info[0])); ++i) {
        for(j = 0; j < group_info[i].pin_count; ++j) {
            //printf("%d %d %x %x\n", i, j, group_info[i].select_offset, group_info[i].config_offset);
            if(idx >= PIN_CNT)
            {
                printf("err: pin count %d\n", idx);
                return -1;
            }
            pad_info[idx].pin_index = j;
            pad_info[idx].select_offset = group_info[i].select_offset;
            pad_info[idx].config_offset = group_info[i].config_offset;
            idx++;

        }
    }

    return 0;
}

static int sca200v200_pinctrl_init(struct sca200v200_pinctrl_priv *priv)
{

    devmem_init(&priv->pin_ctl, REG_BASE_PIN_CTRL, REG_SIZE_PIN_CTRL);
    pad_info_init(priv->pad_info);

    priv->pb_sw = priv->pin_ctl.virt_addr + REG_OFFSET_PB_SW;

    return 0;
}

static int sca200v200_pinctrl_exit(struct sca200v200_pinctrl_priv *priv)
{
    devmem_exit(&priv->pin_ctl);
    return 0;
}

static int sca200v200_do_set(int pin_index, unsigned int pin_config)
{
    struct sca200v200_pinctrl_priv priv;

    sca200v200_pinctrl_init(&priv);
    sca200v200_pin_set(&priv, pin_index, pin_config);
    sca200v200_pinctrl_exit(&priv);
    return 0;
}

static int sca200v200_do_get(int pin_index)
{
    struct sca200v200_pinctrl_priv priv;

    sca200v200_pinctrl_init(&priv);
    sca200v200_pin_get(&priv, pin_index);
    sca200v200_pinctrl_exit(&priv);
    return 0;
}

static int sca200v200_do_list(void)
{
    struct sca200v200_pinctrl_priv priv;

    sca200v200_pinctrl_init(&priv);
    sca200v200_pin_list(&priv);
    sca200v200_pinctrl_exit(&priv);
    return 0;
}

static void usage(const char *name)
{
    printf("args:\n");
    printf("  pin_index: <<SCA200V200_PINOUT.xlsx>>\n");
    printf("    PAD name=PB109, pin_index is 109\n");
    printf("  pin_config: <<SCA200V200_PINOUT.xlsx>> sheet: 04 register decription\n");
    printf("    total 11bit\n");
    printf("    bit[2:0] function select\n");
    printf("    bit[3]: pull up\n");
    printf("    bit[4]: pull down\n");
    printf("    bit[5]: slew rate enable\n");
    printf("    bit[6]: schmitt trigger\n");
    printf("    bit[7:10]: driving selector\n");
}

int sca200v200_pinctrl_chip(struct pinctrl_chip *pchip, const char *comp)
{
    if(strcmp(SCA200V200_DTS_M, comp))
        return -1;

    memset(pchip, 0, sizeof(*pchip));

    pchip->list = sca200v200_do_list;
    pchip->get = sca200v200_do_get;
    pchip->set = sca200v200_do_set;
    pchip->usage = usage;

    return 0;
}
