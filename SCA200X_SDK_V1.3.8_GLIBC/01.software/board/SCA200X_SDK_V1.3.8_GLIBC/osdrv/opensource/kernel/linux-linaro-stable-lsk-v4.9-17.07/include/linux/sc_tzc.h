#ifndef __SMARTX_TRUSTZONE_H__
#define __SMARTX_TRUSTZONE_H__

#define SC_TZC_400_2F 0
#define SC_TZC_400_4F 1

/*reaction value*/
#define    TZCINT_LOW_AND_ISSUE_OK  0x0
#define    TZCINT_LOW_AND_DEC_ERR   0x1
#define    TZCINT_HIGH_AND_ISSUE_OK 0x2
#define    TZCINT_HIGH_AND_DEC_ERR  0x3

/*gate keeper status*/
#define    GATE_KEEPER_CLOSE  0x0
#define    GATE_KEEPER_OPEN   0x1

/*spectulation access*/
#define WRITE_SPEC_DISABLE   0x2
#define READ_SPEC_DISABLE    0x1

enum {
	TZC_1_FILTER  = 0x0,
	TZC_2_FILTER  = 0x1,
	TZC_UNDEFINED = 0x2,
	TZC_4_FILTER  = 0x4,
};

/*register offset */
#define   TZ_BUILD_CONFIG     0x000
#define   TZ_ACTION           0x004
#define   TZ_GATE_KEEPER      0x008
#define   TZ_SPECULATION_CTRL 0x00C
#define   TZ_INT_STATUS       0x010
#define   TZ_INT_CLEAR        0x014

/*Filter0~3: 0x10*x x=0~3 */
#define    FAIL_ADDR_LOW    0x020 + (0x10*0)
#define    FAIL_ADDR_HIGH   0x024 + (0x10*0)
#define    FAIL_CONTROL     0x028 + (0x10*0)
#define    FAIL_ID          0x02C + (0x10*0)

/*Region0~8: 0x20*n n=1~8 */
#define    RGN_BASE_LOW     0x100 + (0x20*0)
#define    RGN_BASE_HIGH    0x104 + (0x20*0)
#define    RGN_TOP_LOW      0x108 + (0x20*0)
#define    RGN_TOP_HIGH     0x10C + (0x20*0)
#define    RGN_ATTRIBUTE    0x110 + (0x20*0)
#define    RGN_ID_ACCESS    0x114 + (0x20*0)

#define    GMAC_FILTER_IDX  0x02

static void tzc400_init(u32 tzc_idx,  /*0: trust_2f, 1:trust_4f */
    u32 action,       //Control the interrupt and bus response signal hehavior
    u32 gate_keeper,  //Control gate keeper closed or open
    u32 spec_ctrl     //Spectulation access control
);

void tzc400_rgn_config(
    u32 tzc_idx,  /*0: trust_2f, 1:trust_4f */
    u32 region_num,
    u32 region_base_low,
    u32 region_base_high,
    u32 region_top_low,
    u32 region_top_high,
    u32 region_attribute,
    u32 region_id_access
);

void tzc400_disable_filter_region(u32 tzc_idx, int filter, int region_num);

#endif

