#ifndef __GIC_H_
#define __GIC_H_

#include <asm/arch/cpu.h>

#define MAX_INTR_NUM        200

// GIC BASE address
//#define GIC_BASE          (0x01000000)//move to asm/arch-sca200v100/cpu.h
#define GIC_DIS_BASE        (GIC_BASE + 0x1000)
#define GIC_CPU_BASE        (GIC_BASE + 0x2000)
#define GIC_VINTC_BASE      (GIC_BASE + 0x4000)
#define GIC_VINTP_BASE      (GIC_BASE + 0x5000)
#define GIC_VCPU_BASE       (GIC_BASE + 0x6000)

// GIC DISTRIBUTER regs address
#define GICD_CTLR           *(volatile unsigned int *)(unsigned long)(GIC_DIS_BASE)
#define GICD_TYPER          *(volatile unsigned int *)(unsigned long)(GIC_DIS_BASE + 0x4)
#define GICD_IIDR           *(volatile unsigned int *)(unsigned long)(GIC_DIS_BASE + 0x8)
#define GICD_IGROUPR(_n)    *(volatile unsigned int *)(unsigned long)(GIC_DIS_BASE + 0x80 + 4 * (_n))
#define GICD_ISENABLER(_n)  *(volatile unsigned int *)(unsigned long)(GIC_DIS_BASE + 0x100 + 4 * (_n))
#define GICD_ICENABLER(_n)  *(volatile unsigned int *)(unsigned long)(GIC_DIS_BASE + 0x180 + 4 * (_n))
#define GICD_ISPENDR(_n)    *(volatile unsigned int *)(unsigned long)(GIC_DIS_BASE + 0x200 + 4 * (_n))
#define GICD_ICPENDR(_n)    *(volatile unsigned int *)(unsigned long)(GIC_DIS_BASE + 0x280 + 4 * (_n))
#define GICD_ISACTIVER(_n)  *(volatile unsigned int *)(unsigned long)(GIC_DIS_BASE + 0x300 + 4 * (_n))
#define GICD_ICACTIVER(_n)  *(volatile unsigned int *)(unsigned long)(GIC_DIS_BASE + 0x380 + 4 * (_n))
#define GICD_IPRIORITYR(_n) *(volatile unsigned int *)(unsigned long)(GIC_DIS_BASE + 0x400 + 4 * (_n))
#define GICD_ITARGETSR(_n)  *(volatile unsigned int *)(unsigned long)(GIC_DIS_BASE + 0x800 + 4 * (_n))
#define GICD_ICFGR(_n)      *(volatile unsigned int *)(unsigned long)(GIC_DIS_BASE + 0xC00 + 4 * (_n))
#define GICD_PPISR          *(volatile unsigned int *)(unsigned long)(GIC_DIS_BASE + 0xD00 )
#define GICD_SPISR(_n)      *(volatile unsigned int *)(unsigned long)(GIC_DIS_BASE + 0xD04 )
#define GICD_SGIR           *(volatile unsigned int *)(unsigned long)(GIC_DIS_BASE + 0xF00 )
#define GICD_CPENDSGIR(_n)  *(volatile unsigned int *)(unsigned long)(GIC_DIS_BASE + 0xF10 + 4 * (_n))
#define GICD_SPENDSGIR(_n)  *(volatile unsigned int *)(unsigned long)(GIC_DIS_BASE + 0xF20 + 4 * (_n))
#define GICD_PIDR4          *(volatile unsigned int *)(unsigned long)(GIC_DIS_BASE + 0xFD0 )
#define GICD_PIDR5          *(volatile unsigned int *)(unsigned long)(GIC_DIS_BASE + 0xFD4 )
#define GICD_PIDR6          *(volatile unsigned int *)(unsigned long)(GIC_DIS_BASE + 0xFD8 )
#define GICD_PIDR7          *(volatile unsigned int *)(unsigned long)(GIC_DIS_BASE + 0xFDC )
#define GICD_PIDR0          *(volatile unsigned int *)(unsigned long)(GIC_DIS_BASE + 0xFE0 )
#define GICD_PIDR1          *(volatile unsigned int *)(unsigned long)(GIC_DIS_BASE + 0xFE4 )
#define GICD_PIDR2          *(volatile unsigned int *)(unsigned long)(GIC_DIS_BASE + 0xFE8 )
#define GICD_PIDR3          *(volatile unsigned int *)(unsigned long)(GIC_DIS_BASE + 0xFEC )
#define GICD_CIDR0          *(volatile unsigned int *)(unsigned long)(GIC_DIS_BASE + 0xFF0 )
#define GICD_CIDR1          *(volatile unsigned int *)(unsigned long)(GIC_DIS_BASE + 0xFF4 )
#define GICD_CIDR2          *(volatile unsigned int *)(unsigned long)(GIC_DIS_BASE + 0xFF8 )
#define GICD_CIDR3          *(volatile unsigned int *)(unsigned long)(GIC_DIS_BASE + 0xFFC )

// GIC CPU INTERFACE regs address
#define GICC_CTLR           *(volatile unsigned int *)(unsigned long)(GIC_CPU_BASE)
#define GICC_PMRn           *(volatile unsigned int *)(unsigned long)(GIC_CPU_BASE + 0x4)
#define GICC_BPR            *(volatile unsigned int *)(unsigned long)(GIC_CPU_BASE + 0x8)
#define GICC_IAR            *(volatile unsigned int *)(unsigned long)(GIC_CPU_BASE + 0xC)
#define GICC_EOIR           *(volatile unsigned int *)(unsigned long)(GIC_CPU_BASE + 0x10)
#define GICC_RPR            *(volatile unsigned int *)(unsigned long)(GIC_CPU_BASE + 0x14)
#define GICC_HPPIR          *(volatile unsigned int *)(unsigned long)(GIC_CPU_BASE + 0x18)
#define GICC_ABPR           *(volatile unsigned int *)(unsigned long)(GIC_CPU_BASE + 0x1C)
#define GICC_AIAR           *(volatile unsigned int *)(unsigned long)(GIC_CPU_BASE + 0x20)
#define GICC_AEOIR          *(volatile unsigned int *)(unsigned long)(GIC_CPU_BASE + 0x24)
#define GICC_AHPPIR         *(volatile unsigned int *)(unsigned long)(GIC_CPU_BASE + 0x28)
#define GICC_APR0           *(volatile unsigned int *)(unsigned long)(GIC_CPU_BASE + 0xD0)
#define GICC_NSAPR0         *(volatile unsigned int *)(unsigned long)(GIC_CPU_BASE + 0xE0)
#define GICC_IIDR           *(volatile unsigned int *)(unsigned long)(GIC_CPU_BASE + 0xFC)
#define GICC_DIR            *(volatile unsigned int *)(unsigned long)(GIC_CPU_BASE + 0x1000)

// GIC VIRTUAL INTERFACE regs address
#define GICH_HCR            *(volatile unsigned int *)(unsigned long)(GIC_VINTC_BASE)
#define GICH_VTR            *(volatile unsigned int *)(unsigned long)(GIC_VINTC_BASE + 0x4)
#define GICH_VMCR           *(volatile unsigned int *)(unsigned long)(GIC_VINTC_BASE + 0x8)
#define GICH_MISR           *(volatile unsigned int *)(unsigned long)(GIC_VINTC_BASE + 0x10)
#define GICH_EISR0          *(volatile unsigned int *)(unsigned long)(GIC_VINTC_BASE + 0x20)
#define GICH_ELSR0          *(volatile unsigned int *)(unsigned long)(GIC_VINTC_BASE + 0x30)
#define GICH_APR0           *(volatile unsigned int *)(unsigned long)(GIC_VINTC_BASE + 0xF0)
#define GICH_LR0            *(volatile unsigned int *)(unsigned long)(GIC_VINTC_BASE + 0x100)
#define GICH_LR1            *(volatile unsigned int *)(unsigned long)(GIC_VINTC_BASE + 0x104)
#define GICH_LR2            *(volatile unsigned int *)(unsigned long)(GIC_VINTC_BASE + 0x108)
#define GICH_LR3            *(volatile unsigned int *)(unsigned long)(GIC_VINTC_BASE + 0x10C)

// GIC VIRTUAL CPU INTERFACE regs address
#define GICV_CTLR           *(volatile unsigned int *)(unsigned long)(GIC_VCPU_BASE)
#define GICV_PMR            *(volatile unsigned int *)(unsigned long)(GIC_VCPU_BASE + 0x4)
#define GICV_BPR            *(volatile unsigned int *)(unsigned long)(GIC_VCPU_BASE + 0x8)
#define GICV_IAR            *(volatile unsigned int *)(unsigned long)(GIC_VCPU_BASE + 0xC)
#define GICV_EOIR           *(volatile unsigned int *)(unsigned long)(GIC_VCPU_BASE + 0x10)
#define GICV_RPR            *(volatile unsigned int *)(unsigned long)(GIC_VCPU_BASE + 0x14)
#define GICV_HPPIR          *(volatile unsigned int *)(unsigned long)(GIC_VCPU_BASE + 0x18)
#define GICV_ABPR           *(volatile unsigned int *)(unsigned long)(GIC_VCPU_BASE + 0x1C)
#define GICV_AIAR           *(volatile unsigned int *)(unsigned long)(GIC_VCPU_BASE + 0x20)
#define GICV_AEOIR          *(volatile unsigned int *)(unsigned long)(GIC_VCPU_BASE + 0x24)
#define GICV_AHPPIR         *(volatile unsigned int *)(unsigned long)(GIC_VCPU_BASE + 0x28)
#define GICV_APR0           *(volatile unsigned int *)(unsigned long)(GIC_VCPU_BASE + 0xD0)
#define GICV_IIDR           *(volatile unsigned int *)(unsigned long)(GIC_VCPU_BASE + 0xFC)
#define GICV_DIR            *(volatile unsigned int *)(unsigned long)(GIC_VCPU_BASE + 0x1000)

void gicv2_init_for_secure_boot(void);
void gicv2_init_for_nonsecure_boot(void);

#endif
