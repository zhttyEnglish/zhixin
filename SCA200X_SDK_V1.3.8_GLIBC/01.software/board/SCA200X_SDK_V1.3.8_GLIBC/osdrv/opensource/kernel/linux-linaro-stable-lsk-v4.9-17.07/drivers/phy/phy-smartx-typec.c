/*
 *
 *
 * The smartchip Type-C PHY has two PLL clocks. The first PLL clock
 * is used for USB3, the second PLL clock is used for DP. This Type-C PHY has
 * 3 working modes: USB3 only mode, DP only mode, and USB3+DP mode.
 * At USB3 only mode, both PLL clocks need to be initialized, this allows the
 * PHY to switch mode between USB3 and USB3+DP, without disconnecting the USB
 * device.
 * In The DP only mode, only the DP PLL needs to be powered on, and the 4 lanes
 * are all used for DP.
 *
 * This driver gets extcon cable state and property, then decides which mode to
 * select:
 *
 * 1. USB3 only mode:
 *    EXTCON_USB or EXTCON_USB_HOST state is true, and
 *    EXTCON_PROP_USB_SS property is true.
 *    EXTCON_DISP_DP state is false.
 *
 * 2. DP only mode:
 *    EXTCON_DISP_DP state is true, and
 *    EXTCON_PROP_USB_SS property is false.
 *    If EXTCON_USB_HOST state is true, it is DP + USB2 mode, since the USB2 phy
 *    is a separate phy, so this case is still DP only mode.
 *
 * 3. USB3+DP mode:
 *    EXTCON_USB_HOST and EXTCON_DISP_DP are both true, and
 *    EXTCON_PROP_USB_SS property is true.
 *
 * This Type-C PHY driver supports normal and flip orientation. The orientation
 * is reported by the EXTCON_PROP_USB_TYPEC_POLARITY property: true is flip
 * orientation, false is normal orientation.
 *
 */

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/delay.h>
#include <linux/extcon.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <linux/interrupt.h>

#include <linux/mfd/syscon.h>
#include <linux/phy/phy.h>
//#include <soc/smartchip/smartchip_phy_typec.h>

#define CMN_SSM_BANDGAP         (0x21 << 2)
#define CMN_SSM_BIAS            (0x22 << 2)
#define CMN_PLLSM0_PLLEN        (0x29 << 2)
#define CMN_PLLSM0_PLLPRE       (0x2a << 2)
#define CMN_PLLSM0_PLLVREF      (0x2b << 2)
#define CMN_PLLSM0_PLLLOCK      (0x2c << 2)
#define CMN_PLLSM1_PLLEN        (0x31 << 2)
#define CMN_PLLSM1_PLLPRE       (0x32 << 2)
#define CMN_PLLSM1_PLLVREF      (0x33 << 2)
#define CMN_PLLSM1_PLLLOCK      (0x34 << 2)
#define CMN_PLLSM1_USER_DEF_CTRL    (0x37 << 2)
#define CMN_ICAL_OVRD           (0xc1 << 2)
#define CMN_PLL0_VCOCAL_OVRD        (0x83 << 2)
#define CMN_PLL0_VCOCAL_INIT        (0x84 << 2)
#define CMN_PLL0_VCOCAL_ITER        (0x85 << 2)
#define CMN_PLL0_LOCK_REFCNT_START  (0x90 << 2)
#define CMN_PLL0_LOCK_PLLCNT_START  (0x92 << 2)
#define CMN_PLL0_LOCK_PLLCNT_THR    (0x93 << 2)
#define CMN_PLL0_INTDIV         (0x94 << 2)
#define CMN_PLL0_FRACDIV        (0x95 << 2)
#define CMN_PLL0_HIGH_THR       (0x96 << 2)
#define CMN_PLL0_DSM_DIAG       (0x97 << 2)
#define CMN_PLL0_SS_CTRL1       (0x98 << 2)
#define CMN_PLL0_SS_CTRL2       (0x99 << 2)
#define CMN_PLL1_VCOCAL_START       (0xa1 << 2)
#define CMN_PLL1_VCOCAL_OVRD        (0xa3 << 2)
#define CMN_PLL1_VCOCAL_INIT        (0xa4 << 2)
#define CMN_PLL1_VCOCAL_ITER        (0xa5 << 2)
#define CMN_PLL1_LOCK_REFCNT_START  (0xb0 << 2)
#define CMN_PLL1_LOCK_PLLCNT_START  (0xb2 << 2)
#define CMN_PLL1_LOCK_PLLCNT_THR    (0xb3 << 2)
#define CMN_PLL1_INTDIV         (0xb4 << 2)
#define CMN_PLL1_FRACDIV        (0xb5 << 2)
#define CMN_PLL1_HIGH_THR       (0xb6 << 2)
#define CMN_PLL1_DSM_DIAG       (0xb7 << 2)
#define CMN_PLL1_SS_CTRL1       (0xb8 << 2)
#define CMN_PLL1_SS_CTRL2       (0xb9 << 2)
#define CMN_RXCAL_OVRD          (0xd1 << 2)

#define CMN_TXPUCAL_CTRL        (0xe0 << 2)
#define CMN_TXPUCAL_OVRD        (0xe1 << 2)
#define CMN_TXPDCAL_CTRL        (0xf0 << 2)
#define CMN_TXPDCAL_OVRD        (0xf1 << 2)

/* For CMN_TXPUCAL_CTRL, CMN_TXPDCAL_CTRL */
#define CMN_TXPXCAL_START       BIT(15)
#define CMN_TXPXCAL_DONE        BIT(14)
#define CMN_TXPXCAL_NO_RESPONSE     BIT(13)
#define CMN_TXPXCAL_CURRENT_RESPONSE    BIT(12)

#define CMN_TXPU_ADJ_CTRL       (0x108 << 2)
#define CMN_TXPD_ADJ_CTRL       (0x10c << 2)

/*
 * For CMN_TXPUCAL_CTRL, CMN_TXPDCAL_CTRL,
 *     CMN_TXPU_ADJ_CTRL, CMN_TXPDCAL_CTRL
 *
 * NOTE: some of these registers are documented to be 2's complement
 * signed numbers, but then documented to be always positive.  Weird.
 * In such a case, using CMN_CALIB_CODE_POS() avoids the unnecessary
 * sign extension.
 */
#define CMN_CALIB_CODE_WIDTH    7
#define CMN_CALIB_CODE_OFFSET   0
#define CMN_CALIB_CODE_MASK GENMASK(CMN_CALIB_CODE_WIDTH, 0)
#define CMN_CALIB_CODE(x)   \
    sign_extend32((x) >> CMN_CALIB_CODE_OFFSET, CMN_CALIB_CODE_WIDTH)

#define CMN_CALIB_CODE_POS_MASK GENMASK(CMN_CALIB_CODE_WIDTH - 1, 0)
#define CMN_CALIB_CODE_POS(x)   \
    (((x) >> CMN_CALIB_CODE_OFFSET) & CMN_CALIB_CODE_POS_MASK)

#define CMN_DIAG_PLL0_FBH_OVRD      (0x1c0 << 2)
#define CMN_DIAG_PLL0_FBL_OVRD      (0x1c1 << 2)
#define CMN_DIAG_PLL0_OVRD      (0x1c2 << 2)
#define CMN_DIAG_PLL0_V2I_TUNE      (0x1c5 << 2)
#define CMN_DIAG_PLL0_CP_TUNE       (0x1c6 << 2)
#define CMN_DIAG_PLL0_LF_PROG       (0x1c7 << 2)
#define CMN_DIAG_PLL1_FBH_OVRD      (0x1d0 << 2)
#define CMN_DIAG_PLL1_FBL_OVRD      (0x1d1 << 2)
#define CMN_DIAG_PLL1_OVRD      (0x1d2 << 2)
#define CMN_DIAG_PLL1_V2I_TUNE      (0x1d5 << 2)
#define CMN_DIAG_PLL1_CP_TUNE       (0x1d6 << 2)
#define CMN_DIAG_PLL1_LF_PROG       (0x1d7 << 2)
#define CMN_DIAG_PLL1_PTATIS_TUNE1  (0x1d8 << 2)
#define CMN_DIAG_PLL1_PTATIS_TUNE2  (0x1d9 << 2)
#define CMN_DIAG_PLL1_INCLK_CTRL    (0x1da << 2)
#define CMN_DIAG_HSCLK_SEL      (0x1e0 << 2)

#define XCVR_PSM_RCTRL(n)       ((0x4001 | ((n) << 9)) << 2)
#define XCVR_PSM_CAL_TMR(n)     ((0x4002 | ((n) << 9)) << 2)
#define XCVR_PSM_A0IN_TMR(n)        ((0x4003 | ((n) << 9)) << 2)
#define TX_TXCC_CAL_SCLR_MULT(n)    ((0x4047 | ((n) << 9)) << 2)
#define TX_TXCC_CPOST_MULT_00(n)    ((0x404c | ((n) << 9)) << 2)
#define TX_TXCC_CPOST_MULT_01(n)    ((0x404d | ((n) << 9)) << 2)
#define TX_TXCC_CPOST_MULT_10(n)    ((0x404e | ((n) << 9)) << 2)
#define TX_TXCC_CPOST_MULT_11(n)    ((0x404f | ((n) << 9)) << 2)
#define TX_TXCC_MGNFS_MULT_000(n)   ((0x4050 | ((n) << 9)) << 2)
#define TX_TXCC_MGNFS_MULT_001(n)   ((0x4051 | ((n) << 9)) << 2)
#define TX_TXCC_MGNFS_MULT_010(n)   ((0x4052 | ((n) << 9)) << 2)
#define TX_TXCC_MGNFS_MULT_011(n)   ((0x4053 | ((n) << 9)) << 2)
#define TX_TXCC_MGNFS_MULT_100(n)   ((0x4054 | ((n) << 9)) << 2)
#define TX_TXCC_MGNFS_MULT_101(n)   ((0x4055 | ((n) << 9)) << 2)
#define TX_TXCC_MGNFS_MULT_110(n)   ((0x4056 | ((n) << 9)) << 2)
#define TX_TXCC_MGNFS_MULT_111(n)   ((0x4057 | ((n) << 9)) << 2)
#define TX_TXCC_MGNLS_MULT_000(n)   ((0x4058 | ((n) << 9)) << 2)
#define TX_TXCC_MGNLS_MULT_001(n)   ((0x4059 | ((n) << 9)) << 2)
#define TX_TXCC_MGNLS_MULT_010(n)   ((0x405a | ((n) << 9)) << 2)
#define TX_TXCC_MGNLS_MULT_011(n)   ((0x405b | ((n) << 9)) << 2)
#define TX_TXCC_MGNLS_MULT_100(n)   ((0x405c | ((n) << 9)) << 2)
#define TX_TXCC_MGNLS_MULT_101(n)   ((0x405d | ((n) << 9)) << 2)
#define TX_TXCC_MGNLS_MULT_110(n)   ((0x405e | ((n) << 9)) << 2)
#define TX_TXCC_MGNLS_MULT_111(n)   ((0x405f | ((n) << 9)) << 2)

#define XCVR_DIAG_PLLDRC_CTRL(n)    ((0x40e0 | ((n) << 9)) << 2)
#define XCVR_DIAG_BIDI_CTRL(n)      ((0x40e8 | ((n) << 9)) << 2)
#define XCVR_DIAG_LANE_FCM_EN_MGN(n)    ((0x40f2 | ((n) << 9)) << 2)
#define TX_PSC_A0(n)            ((0x4100 | ((n) << 9)) << 2)
#define TX_PSC_A1(n)            ((0x4101 | ((n) << 9)) << 2)
#define TX_PSC_A2(n)            ((0x4102 | ((n) << 9)) << 2)
#define TX_PSC_A3(n)            ((0x4103 | ((n) << 9)) << 2)
#define TX_RCVDET_CTRL(n)       ((0x4120 | ((n) << 9)) << 2)
#define TX_RCVDET_EN_TMR(n)     ((0x4122 | ((n) << 9)) << 2)
#define TX_RCVDET_ST_TMR(n)     ((0x4123 | ((n) << 9)) << 2)
#define TX_DIAG_TX_DRV(n)       ((0x41e1 | ((n) << 9)) << 2)
#define TX_DIAG_BGREF_PREDRV_DELAY  (0x41e7 << 2)

/* Use this for "n" in macros like "_MULT_XXX" to target the aux channel */
#define AUX_CH_LANE         8

#define TX_ANA_CTRL_REG_1       (0x5020 << 2)

#define TXDA_DP_AUX_EN          BIT(15)
#define AUXDA_SE_EN         BIT(14)
#define TXDA_CAL_LATCH_EN       BIT(13)
#define AUXDA_POLARITY          BIT(12)
#define TXDA_DRV_POWER_ISOLATION_EN BIT(11)
#define TXDA_DRV_POWER_EN_PH_2_N    BIT(10)
#define TXDA_DRV_POWER_EN_PH_1_N    BIT(9)
#define TXDA_BGREF_EN           BIT(8)
#define TXDA_DRV_LDO_EN         BIT(7)
#define TXDA_DECAP_EN_DEL       BIT(6)
#define TXDA_DECAP_EN           BIT(5)
#define TXDA_UPHY_SUPPLY_EN_DEL     BIT(4)
#define TXDA_UPHY_SUPPLY_EN     BIT(3)
#define TXDA_LOW_LEAKAGE_EN     BIT(2)
#define TXDA_DRV_IDLE_LOWI_EN       BIT(1)
#define TXDA_DRV_CMN_MODE_EN        BIT(0)

#define TX_ANA_CTRL_REG_2       (0x5021 << 2)

#define AUXDA_DEBOUNCING_CLK        BIT(15)
#define TXDA_LPBK_RECOVERED_CLK_EN  BIT(14)
#define TXDA_LPBK_ISI_GEN_EN        BIT(13)
#define TXDA_LPBK_SERIAL_EN     BIT(12)
#define TXDA_LPBK_LINE_EN       BIT(11)
#define TXDA_DRV_LDO_REDC_SINKIQ    BIT(10)
#define XCVR_DECAP_EN_DEL       BIT(9)
#define XCVR_DECAP_EN           BIT(8)
#define TXDA_MPHY_ENABLE_HS_NT      BIT(7)
#define TXDA_MPHY_SA_MODE       BIT(6)
#define TXDA_DRV_LDO_RBYR_FB_EN     BIT(5)
#define TXDA_DRV_RST_PULL_DOWN      BIT(4)
#define TXDA_DRV_LDO_BG_FB_EN       BIT(3)
#define TXDA_DRV_LDO_BG_REF_EN      BIT(2)
#define TXDA_DRV_PREDRV_EN_DEL      BIT(1)
#define TXDA_DRV_PREDRV_EN      BIT(0)

#define TXDA_COEFF_CALC_CTRL        (0x5022 << 2)

#define TX_HIGH_Z           BIT(6)
#define TX_VMARGIN_OFFSET       3
#define TX_VMARGIN_MASK         0x7
#define LOW_POWER_SWING_EN      BIT(2)
#define TX_FCM_DRV_MAIN_EN      BIT(1)
#define TX_FCM_FULL_MARGIN      BIT(0)

#define TX_DIG_CTRL_REG_2       (0x5024 << 2)

#define TX_HIGH_Z_TM_EN         BIT(15)
#define TX_RESCAL_CODE_OFFSET       0
#define TX_RESCAL_CODE_MASK     0x3f

#define TXDA_CYA_AUXDA_CYA      (0x5025 << 2)
#define TX_ANA_CTRL_REG_3       (0x5026 << 2)
#define TX_ANA_CTRL_REG_4       (0x5027 << 2)
#define TX_ANA_CTRL_REG_5       (0x5029 << 2)

#define RX_PSC_A0(n)            ((0x8000 | ((n) << 9)) << 2)
#define RX_PSC_A1(n)            ((0x8001 | ((n) << 9)) << 2)
#define RX_PSC_A2(n)            ((0x8002 | ((n) << 9)) << 2)
#define RX_PSC_A3(n)            ((0x8003 | ((n) << 9)) << 2)
#define RX_PSC_CAL(n)           ((0x8006 | ((n) << 9)) << 2)
#define RX_PSC_RDY(n)           ((0x8007 | ((n) << 9)) << 2)
#define RX_IQPI_ILL_CAL_OVRD        (0x8023 << 2)
#define RX_EPI_ILL_CAL_OVRD     (0x8033 << 2)
#define RX_SDCAL0_OVRD          (0x8041 << 2)
#define RX_SDCAL1_OVRD          (0x8049 << 2)
#define RX_SLC_INIT         (0x806d << 2)
#define RX_SLC_RUN          (0x806e << 2)
#define RX_CDRLF_CNFG2          (0x8081 << 2)
#define RX_SIGDET_HL_FILT_TMR(n)    ((0x8090 | ((n) << 9)) << 2)
#define RX_SLC_IOP0_OVRD        (0x8101 << 2)
#define RX_SLC_IOP1_OVRD        (0x8105 << 2)
#define RX_SLC_QOP0_OVRD        (0x8109 << 2)
#define RX_SLC_QOP1_OVRD        (0x810d << 2)
#define RX_SLC_EOP0_OVRD        (0x8111 << 2)
#define RX_SLC_EOP1_OVRD        (0x8115 << 2)
#define RX_SLC_ION0_OVRD        (0x8119 << 2)
#define RX_SLC_ION1_OVRD        (0x811d << 2)
#define RX_SLC_QON0_OVRD        (0x8121 << 2)
#define RX_SLC_QON1_OVRD        (0x8125 << 2)
#define RX_SLC_EON0_OVRD        (0x8129 << 2)
#define RX_SLC_EON1_OVRD        (0x812d << 2)
#define RX_SLC_IEP0_OVRD        (0x8131 << 2)
#define RX_SLC_IEP1_OVRD        (0x8135 << 2)
#define RX_SLC_QEP0_OVRD        (0x8139 << 2)
#define RX_SLC_QEP1_OVRD        (0x813d << 2)
#define RX_SLC_EEP0_OVRD        (0x8141 << 2)
#define RX_SLC_EEP1_OVRD        (0x8145 << 2)
#define RX_SLC_IEN0_OVRD        (0x8149 << 2)
#define RX_SLC_IEN1_OVRD        (0x814d << 2)
#define RX_SLC_QEN0_OVRD        (0x8151 << 2)
#define RX_SLC_QEN1_OVRD        (0x8155 << 2)
#define RX_SLC_EEN0_OVRD        (0x8159 << 2)
#define RX_SLC_EEN1_OVRD        (0x815d << 2)
#define RX_REE_CTRL_DATA_MASK(n)    ((0x81bb | ((n) << 9)) << 2)
#define RX_DIAG_SIGDET_TUNE(n)      ((0x81dc | ((n) << 9)) << 2)
#define RX_DIAG_SC2C_DELAY      (0x81e1 << 2)

#define PHY_PMA_LANE_CFG        (0xc000 << 2)
#define PMA_LANE3_DP_LANE_SEL(x)    (((x) & 0x3) << 14)
#define PMA_LANE3_INTERFACE_SEL(x)  (((x) & 0x1) << 12)
#define PMA_LANE2_DP_LANE_SEL(x)    (((x) & 0x3) << 10)
#define PMA_LANE2_INTERFACE_SEL(x)  (((x) & 0x1) << 8)
#define PMA_LANE1_DP_LANE_SEL(x)    (((x) & 0x3) << 6)
#define PMA_LANE1_INTERFACE_SEL(x)  (((x) & 0x1) << 4)
#define PMA_LANE0_DP_LANE_SEL(x)    (((x) & 0x3) << 2)
#define PMA_LANE0_INTERFACE_SEL(x)  (((x) & 0x1) << 0)
#define PIPE_CMN_CTRL1          (0xc001 << 2)
#define PIPE_CMN_CTRL2          (0xc002 << 2)
#define PIPE_COM_LOCK_CFG1      (0xc003 << 2)
#define PIPE_COM_LOCK_CFG2      (0xc004 << 2)
#define PIPE_RCV_DET_INH        (0xc005 << 2)
#define PHY_DP_MODE_CTL         (0xc008 << 2)
#define PHY_DP_LANE_DISABLE     GENMASK(15, 12)
#define PHY_DP_LANE_3_DISABLE       BIT(15)
#define PHY_DP_LANE_2_DISABLE       BIT(14)
#define PHY_DP_LANE_1_DISABLE       BIT(13)
#define PHY_DP_LANE_0_DISABLE       BIT(12)
#define PHY_DP_POWER_STATE_ACK_MASK GENMASK(7, 4)
#define PHY_DP_POWER_STATE_ACK_SHIFT    4
#define PHY_DP_POWER_STATE_MASK     GENMASK(3, 0)
#define PHY_DP_CLK_CTL          (0xc009 << 2)
#define DP_PLL_CLOCK_ENABLE_ACK     BIT(3)
#define DP_PLL_CLOCK_ENABLE_MASK    BIT(2)
#define DP_PLL_CLOCK_DISABLE        0
#define DP_PLL_READY            BIT(1)
#define DP_PLL_ENABLE_MASK      BIT(0)
#define DP_PLL_ENABLE           BIT(0)
#define DP_PLL_DISABLE          0
#define DP_CLK_CTL          (0xc009 << 2)
#define STS             (0xc00F << 2)
#define PHY_ISO_CMN_CTRL        (0xc010 << 2)
#define PHY_DP_TX_CTL           (0xc408 << 2)
#define PMA_CMN_CTRL1           (0xc800 << 2)
#define PHY_PMA_ISO_CMN_CTRL        (0xc810 << 2)
#define PHY_ISOLATION_CTRL      (0xc81f << 2)
#define PHY_PMA_ISO_XCVR_CTRL(n)    ((0xcc11 | ((n) << 6)) << 2)
#define PHY_PMA_ISO_LINK_MODE(n)    ((0xcc12 | ((n) << 6)) << 2)
#define PHY_PMA_ISO_PWRST_CTRL(n)   ((0xcc13 | ((n) << 6)) << 2)
#define PHY_PMA_ISO_TX_DATA_LO(n)   ((0xcc14 | ((n) << 6)) << 2)
#define PHY_PMA_ISO_TX_DATA_HI(n)   ((0xcc15 | ((n) << 6)) << 2)
#define PHY_PMA_ISO_RX_DATA_LO(n)   ((0xcc16 | ((n) << 6)) << 2)
#define PHY_PMA_ISO_RX_DATA_HI(n)   ((0xcc17 | ((n) << 6)) << 2)
#define TX_BIST_CTRL(n)         ((0x4140 | ((n) << 9)) << 2)
#define TX_BIST_UDDWR(n)        ((0x4141 | ((n) << 9)) << 2)

/*
 * Selects which PLL clock will be driven on the analog high speed
 * clock 0: PLL 0 div 1
 * clock 1: PLL 1 div 2
 */
#define CLK_PLL1_DIV1           0x20
#define CLK_PLL1_DIV2           0x30
#define CLK_PLL_MASK            0x33

#define CMN_READY           BIT(0)

#define DP_PLL_CLOCK_ENABLE_ACK     BIT(3)
#define DP_PLL_CLOCK_ENABLE     BIT(2)
#define DP_PLL_ENABLE_ACK       BIT(1)
#define DP_PLL_ENABLE           BIT(0)
#define DP_PLL_DATA_RATE_RBR        ((2 << 12) | (4 << 8))
#define DP_PLL_DATA_RATE_HBR        ((2 << 12) | (4 << 8))
#define DP_PLL_DATA_RATE_HBR2       ((1 << 12) | (2 << 8))
#define DP_PLL_DATA_RATE_MASK       0xff00

#define DP_MODE_MASK            0xf
#define DP_MODE_ENTER_A0        BIT(0)
#define DP_MODE_ENTER_A2        BIT(2)
#define DP_MODE_ENTER_A3        BIT(3)
#define DP_MODE_A0_ACK          BIT(4)
#define DP_MODE_A2_ACK          BIT(6)
#define DP_MODE_A3_ACK          BIT(7)
#define DP_LINK_RESET_DEASSERTED    BIT(8)

#define PHY_MODE_SET_TIMEOUT        100000

#define PIN_ASSIGN_C_E          0x51d9
#define PIN_ASSIGN_D_F          0x5100

#define MODE_DISCONNECT         0
#define MODE_UFP_USB            BIT(0)
#define MODE_DFP_USB            BIT(1)
#define MODE_DFP_DP             BIT(2)
#define MODE_DFP_DP_ONLY    BIT(3)

#define POWER_ON_TRIES          5

#define DP_DEFAULT_RATE         540000

struct phy_reg {
	u16 value;
	u32 addr;
};

struct usb3phy_reg {
	u32 offset;
	u32 enable_bit;
	u32 write_enable;
};

struct smartchip_usb3phy_port_cfg {
	struct usb3phy_reg typec_conn_dir;
	struct usb3phy_reg usb3tousb2_en;
	struct usb3phy_reg usb3host_disable;
	struct usb3phy_reg usb3host_port;
	struct usb3phy_reg external_psm;
	struct usb3phy_reg pipe_status;
	struct usb3phy_reg uphy_dp_sel;
};

struct phy_config {
	int swing;
	int pe;
};

struct smartchip_typec_phy {
	struct device *dev;
	void __iomem *base;
	void __iomem        *tcpd_base;
	void __iomem        *comm_phy_base;
	void __iomem        *user_define_base;
	struct extcon_dev *extcon;
	struct regmap *grf_regs;
	struct clk *clk_core;
	struct clk *clk_ref;
	struct reset_control *uphy_rst;
	struct reset_control *pipe_rst;
	struct reset_control *tcphy_rst;
	struct smartchip_usb3phy_port_cfg port_cfgs;
	/* mutex to protect access to individual PHYs */
	struct mutex lock;

	bool flip;
	u8 mode;
	struct phy_config config[3][4];
	struct {
		int link_rate;
		u8 lane_count;
	} dp;
	int (*typec_phy_config)(struct phy *phy, int link_rate,
	    int lanes, u8 swing, u8 pre_emp);
};

static struct platform_device *phy_smartchip_smartx_pdev;

static const struct phy_reg usb3_pll_cfg[] = {
	{ 0xf0,     CMN_PLL0_VCOCAL_INIT },
	{ 0x18,     CMN_PLL0_VCOCAL_ITER },
	{ 0xd0,     CMN_PLL0_INTDIV },
	{ 0x4a4a,   CMN_PLL0_FRACDIV },
	{ 0x34,     CMN_PLL0_HIGH_THR },
	{ 0x1ee,    CMN_PLL0_SS_CTRL1 },
	{ 0x7f03,   CMN_PLL0_SS_CTRL2 },
	{ 0x20,     CMN_PLL0_DSM_DIAG },
	{ 0,        CMN_DIAG_PLL0_OVRD },
	{ 0,        CMN_DIAG_PLL0_FBH_OVRD },
	{ 0,        CMN_DIAG_PLL0_FBL_OVRD },
	{ 0x7,      CMN_DIAG_PLL0_V2I_TUNE },
	{ 0x45,     CMN_DIAG_PLL0_CP_TUNE },
	{ 0x8,      CMN_DIAG_PLL0_LF_PROG },
};

static const struct phy_reg dp_pll_rbr_cfg[] = {
	{ 0xf0,     CMN_PLL1_VCOCAL_INIT },
	{ 0x18,     CMN_PLL1_VCOCAL_ITER },
	{ 0x30b9,   CMN_PLL1_VCOCAL_START },
	{ 0x0087,   CMN_PLL1_INTDIV },//{ 0x0086,   CMN_PLL1_INTDIV },
	{ 0x0000,   CMN_PLL1_FRACDIV },//{ 0xf915,  CMN_PLL1_FRACDIV },
	{ 0x0005,   CMN_PLL1_HIGH_THR },//{ 0x0022, CMN_PLL1_HIGH_THR },
	{ 0x0035,   CMN_PLL1_SS_CTRL1 },//{ 0x0140, CMN_PLL1_SS_CTRL1 },
	{ 0x7f1e,   CMN_PLL1_SS_CTRL2 },//{ 0x7f03, CMN_PLL1_SS_CTRL2 },
	{ 0x20,     CMN_PLL1_DSM_DIAG },
	{ 0,        CMN_PLLSM1_USER_DEF_CTRL },
	{ 0,        CMN_DIAG_PLL1_OVRD },
	{ 0,        CMN_DIAG_PLL1_FBH_OVRD },
	{ 0,        CMN_DIAG_PLL1_FBL_OVRD },
	{ 0x6,      CMN_DIAG_PLL1_V2I_TUNE },
	{ 0x45,     CMN_DIAG_PLL1_CP_TUNE },
	{ 0x8,      CMN_DIAG_PLL1_LF_PROG },
	{ 0x100,    CMN_DIAG_PLL1_PTATIS_TUNE1 },
	{ 0x7,      CMN_DIAG_PLL1_PTATIS_TUNE2 },
	{ 0x1,      CMN_DIAG_PLL1_INCLK_CTRL },
};

static const struct phy_reg dp_pll_hbr_cfg[] = {
	{ 0xf0,     CMN_PLL1_VCOCAL_INIT },
	{ 0x18,     CMN_PLL1_VCOCAL_ITER },
	{ 0x30b4,   CMN_PLL1_VCOCAL_START },
	{ 0x00e0,   CMN_PLL1_INTDIV },
	{ 0xf479,   CMN_PLL1_FRACDIV },
	{ 0x0038,   CMN_PLL1_HIGH_THR },
	{ 0x0204,   CMN_PLL1_SS_CTRL1 },
	{ 0x7f03,   CMN_PLL1_SS_CTRL2 },
	{ 0x20,     CMN_PLL1_DSM_DIAG },
	{ 0x1000,   CMN_PLLSM1_USER_DEF_CTRL },
	{ 0,        CMN_DIAG_PLL1_OVRD },
	{ 0,        CMN_DIAG_PLL1_FBH_OVRD },
	{ 0,        CMN_DIAG_PLL1_FBL_OVRD },
	{ 0x7,      CMN_DIAG_PLL1_V2I_TUNE },
	{ 0x45,     CMN_DIAG_PLL1_CP_TUNE },
	{ 0x8,      CMN_DIAG_PLL1_LF_PROG },
	{ 0x1,      CMN_DIAG_PLL1_PTATIS_TUNE1 },
	{ 0x1,      CMN_DIAG_PLL1_PTATIS_TUNE2 },
	{ 0x1,      CMN_DIAG_PLL1_INCLK_CTRL },
};

static const struct phy_reg dp_pll_hbr2_cfg[] = {
	{ 0xf0,     CMN_PLL1_VCOCAL_INIT },
	{ 0x18,     CMN_PLL1_VCOCAL_ITER },
	{ 0x30b4,   CMN_PLL1_VCOCAL_START },
	{ 0x00e0,   CMN_PLL1_INTDIV },
	{ 0xf479,   CMN_PLL1_FRACDIV },
	{ 0x0038,   CMN_PLL1_HIGH_THR },
	{ 0x0204,   CMN_PLL1_SS_CTRL1 },
	{ 0x7f03,   CMN_PLL1_SS_CTRL2 },
	{ 0x20,     CMN_PLL1_DSM_DIAG },
	{ 0x1000,   CMN_PLLSM1_USER_DEF_CTRL },
	{ 0,        CMN_DIAG_PLL1_OVRD },
	{ 0,        CMN_DIAG_PLL1_FBH_OVRD },
	{ 0,        CMN_DIAG_PLL1_FBL_OVRD },
	{ 0x7,      CMN_DIAG_PLL1_V2I_TUNE },
	{ 0x45,     CMN_DIAG_PLL1_CP_TUNE },
	{ 0x8,      CMN_DIAG_PLL1_LF_PROG },
	{ 0x1,      CMN_DIAG_PLL1_PTATIS_TUNE1 },
	{ 0x1,      CMN_DIAG_PLL1_PTATIS_TUNE2 },
	{ 0x1,      CMN_DIAG_PLL1_INCLK_CTRL },
};

/* default phy config */
static const struct phy_config tcphy_default_config[3][4] = {
	{	{ .swing = 0x2a, .pe = 0x00 },
		{ .swing = 0x1f, .pe = 0x15 },
		{ .swing = 0x14, .pe = 0x22 },
		{ .swing = 0x02, .pe = 0x2b }
	},

	{	{ .swing = 0x21, .pe = 0x00 },
		{ .swing = 0x12, .pe = 0x15 },
		{ .swing = 0x02, .pe = 0x22 },
		{ .swing = 0,    .pe = 0 }
	},

	{	{ .swing = 0x15, .pe = 0x00 },
		{ .swing = 0x00, .pe = 0x15 },
		{ .swing = 0,    .pe = 0 },
		{ .swing = 0,    .pe = 0 }
	},
};

enum phy_dp_power_state {
	PHY_DP_POWER_STATE_DISABLED = -1,
	PHY_DP_POWER_STATE_A0,
	PHY_DP_POWER_STATE_A1,
	PHY_DP_POWER_STATE_A2,
	PHY_DP_POWER_STATE_A3,
};

enum {
	PHY_DP_LANE_0,
	PHY_DP_LANE_1,
	PHY_DP_LANE_2,
	PHY_DP_LANE_3,
};

enum {
	PMA_IF_PIPE_PCS,
	PMA_IF_PHY_DP,
};

static inline u32 smartx_usbphy_readl(void __iomem *base, u32 offset)
{
	u32     tmp;

	tmp = (u32)base;
	tmp |= (offset << 2);

	return readl((void __iomem *)tmp);
}

static inline void smartx_usbphy_writel(void __iomem *base,
    u32 offset, u32 value)
{
	u32     tmp;

	tmp = (u32)base;
	tmp |= (offset << 2);

	writel(value, (void __iomem *)tmp);
}

static int tcphy_dp_set_power_state(struct smartchip_typec_phy *tcphy,
    enum phy_dp_power_state state)
{
	u32 ack, reg, sts = BIT(state);
	int ret;

	/*
	 * Power state changes must not be requested until after the cmn_ready
	 * signal has gone active.
	 */
	reg = readl(tcphy->base + PMA_CMN_CTRL1);
	if (!(reg & CMN_READY)) {
		dev_err(tcphy->dev, "cmn_ready in the inactive state\n");
		return -EINVAL;
	}

	reg = readl(tcphy->base + PHY_DP_MODE_CTL);
	reg &= ~PHY_DP_POWER_STATE_MASK;
	reg |= sts;
	writel(reg, tcphy->base + PHY_DP_MODE_CTL);

	ret = readl_poll_timeout(tcphy->base + PHY_DP_MODE_CTL,
	        ack, (((ack & PHY_DP_POWER_STATE_ACK_MASK) >>
	                PHY_DP_POWER_STATE_ACK_SHIFT) == sts), 10,
	        PHY_MODE_SET_TIMEOUT);
	if (ret < 0) {
		dev_err(tcphy->dev, "failed to enter power state %d\n", state);
		return ret;
	}

	return 0;
}

/*
 * For the TypeC PHY, the 4 lanes are mapping to the USB TypeC receptacle pins
 * as follows:
 *   -------------------------------------------------------------------
 *  PHY Lanes/Module Pins           TypeC Receptacle Pins
 *   -------------------------------------------------------------------
 *  Lane0 (tx_p/m_ln_0)         TX1+/TX1- (pins A2/A3)
 *  Lane1 (tx_rx_p/m_ln_1)          RX1+/RX1- (pins B11/B10)
 *  Lane2 (tx_rx_p/m_ln_2)          RX2+/RX2- (pins A11/A10)
 *  Lane3 (tx_p/m_ln_3)         TX2+/TX2- (pins B2/B3)
 *   -------------------------------------------------------------------
 *
 * USB and DP lanes mapping to TypeC PHY lanes for each of pin assignment
 * options (normal connector orientation) described in the VESA DisplayPort
 * Alt Mode on USB TypeC Standard as follows:
 *
 * ----------------------------------------------------------------------
 *  PHY Lanes   A   B   C   D   E   F
 * ----------------------------------------------------------------------
 *    0        ML1     SSTX    ML2     SSTX    ML2     SSTX
 *    1        ML3     SSRX    ML3     SSRX    ML3     SSRX
 *    2        ML2     ML1     ML0     ML0     ML0     ML0
 *    3        ML0     ML0     ML1     ML1     ML1     ML1
 * ----------------------------------------------------------------------
 */
static void tcphy_set_lane_mapping(struct smartchip_typec_phy *tcphy, u8 mode)
{
	/*
	 * The PHY_PMA_LANE_CFG register is used to select whether a PMA lane
	 * is mapped for USB or PHY DP. The PHY_PMA_LANE_CFG register is
	 * configured based on a normal connector orientation. Logic in the
	 * PHY automatically handles the flipped connector case based on the
	 * setting of orientation of TypeC PHY.
	 */
	if (mode == MODE_DFP_DP) {
		/* This maps to VESA DP Alt Mode pin assignments C and E. */
		writel(PMA_LANE3_DP_LANE_SEL(PHY_DP_LANE_1) |
		    PMA_LANE3_INTERFACE_SEL(PMA_IF_PHY_DP) |
		    PMA_LANE2_DP_LANE_SEL(PHY_DP_LANE_0) |
		    PMA_LANE2_INTERFACE_SEL(PMA_IF_PHY_DP) |
		    PMA_LANE1_DP_LANE_SEL(PHY_DP_LANE_3) |
		    PMA_LANE1_INTERFACE_SEL(PMA_IF_PHY_DP) |
		    PMA_LANE0_DP_LANE_SEL(PHY_DP_LANE_2) |
		    PMA_LANE0_INTERFACE_SEL(PMA_IF_PHY_DP),
		    tcphy->base + PHY_PMA_LANE_CFG);
	} else if(mode == (MODE_DFP_DP | MODE_DFP_DP_ONLY)) {
		/* This maps to VESA Normal DP . */
		writel(PMA_LANE3_DP_LANE_SEL(PHY_DP_LANE_3) |
		    PMA_LANE3_INTERFACE_SEL(PMA_IF_PHY_DP) |
		    PMA_LANE2_DP_LANE_SEL(PHY_DP_LANE_2) |
		    PMA_LANE2_INTERFACE_SEL(PMA_IF_PHY_DP) |
		    PMA_LANE1_DP_LANE_SEL(PHY_DP_LANE_1) |
		    PMA_LANE1_INTERFACE_SEL(PMA_IF_PHY_DP) |
		    PMA_LANE0_DP_LANE_SEL(PHY_DP_LANE_0) |
		    PMA_LANE0_INTERFACE_SEL(PMA_IF_PHY_DP),
		    tcphy->base + PHY_PMA_LANE_CFG);
	} else {
		/* This maps to VESA DP Alt Mode pin assignments D and F. */
		writel(PMA_LANE3_INTERFACE_SEL(PMA_IF_PIPE_PCS) |
		    PMA_LANE2_INTERFACE_SEL(PMA_IF_PIPE_PCS) |
		    PMA_LANE1_INTERFACE_SEL(PMA_IF_PIPE_PCS) |
		    PMA_LANE0_INTERFACE_SEL(PMA_IF_PIPE_PCS),
		    tcphy->base + PHY_PMA_LANE_CFG);
		/*writel(PMA_LANE3_DP_LANE_SEL(PHY_DP_LANE_1) |
		       PMA_LANE3_INTERFACE_SEL(PMA_IF_PHY_DP) |
		       PMA_LANE2_DP_LANE_SEL(PHY_DP_LANE_0) |
		       PMA_LANE2_INTERFACE_SEL(PMA_IF_PHY_DP) |
		       PMA_LANE1_INTERFACE_SEL(PMA_IF_PIPE_PCS) |
		       PMA_LANE0_INTERFACE_SEL(PMA_IF_PIPE_PCS),
		       tcphy->base + PHY_PMA_LANE_CFG);*/
	}
}
#if 0
static int tcphy_dp_set_lane_count(struct smartchip_typec_phy *tcphy,
    u8 lane_count)
{
	u32 reg;

	/*
	 * In cases where fewer than the configured number of DP lanes are
	 * being used. PHY_DP_MODE_CTL[15:12] must be set to disable and
	 * power-down the unused PHY DP lanes (and their mapped PMA lanes).
	 * Set the bit ([15:12]) associated with each DP PHY lane(s) to be
	 * disabled.
	 */
	reg = readl(tcphy->base + PHY_DP_MODE_CTL);
	reg |= PHY_DP_LANE_DISABLE;

	switch (lane_count) {
	case 4:
		reg &= ~(PHY_DP_LANE_3_DISABLE | PHY_DP_LANE_2_DISABLE |
		        PHY_DP_LANE_1_DISABLE | PHY_DP_LANE_0_DISABLE);
		break;
	case 2:
		reg &= ~(PHY_DP_LANE_1_DISABLE | PHY_DP_LANE_0_DISABLE);
		break;
	case 1:
		reg &= ~PHY_DP_LANE_0_DISABLE;
		break;
	default:
		return -EINVAL;
	}

	writel(reg, tcphy->base + PHY_DP_MODE_CTL);

	tcphy->dp.lane_count = lane_count;

	return 0;
}

static int tcphy_dp_set_link_rate(struct smartchip_typec_phy *tcphy,
    int link_rate)
{
	const struct phy_reg *phy_cfg;
	u32 cmn_diag_hsclk_sel, phy_dp_clk_ctl, reg;
	u32 i, cfg_size;
	int ret;

	/* Place the PHY lanes in the A3 power state. */
	ret = tcphy_dp_set_power_state(tcphy, PHY_DP_POWER_STATE_A3);
	if (ret) {
		dev_err(tcphy->dev, "failed to enter A3 state: %d\n", ret);
		return ret;
	}

	/* Gate the PLL clocks from PMA */
	reg = readl(tcphy->base + PHY_DP_CLK_CTL);
	reg &= ~DP_PLL_CLOCK_ENABLE_MASK;
	reg |= DP_PLL_CLOCK_DISABLE;
	writel(reg, tcphy->base + PHY_DP_CLK_CTL);

	ret = readl_poll_timeout(tcphy->base + PHY_DP_CLK_CTL, reg,
	        !(reg & DP_PLL_CLOCK_ENABLE_ACK),
	        10, PHY_MODE_SET_TIMEOUT);
	if (ret) {
		dev_err(tcphy->dev, "wait DP PLL clock disabled timeout\n");
		return ret;
	}

	/* Disable the PLL */
	reg = readl(tcphy->base + PHY_DP_CLK_CTL);
	reg &= ~DP_PLL_ENABLE_MASK;
	reg |= DP_PLL_DISABLE;
	writel(reg, tcphy->base + PHY_DP_CLK_CTL);

	ret = readl_poll_timeout(tcphy->base + PHY_DP_CLK_CTL, reg,
	        !(reg & DP_PLL_READY),
	        10, PHY_MODE_SET_TIMEOUT);
	if (ret) {
		dev_err(tcphy->dev, "wait DP PLL not ready timeout\n");
		return ret;
	}

	/* Re-configure PHY registers for the new data rate */
	cmn_diag_hsclk_sel = readl(tcphy->base + CMN_DIAG_HSCLK_SEL);
	cmn_diag_hsclk_sel &= ~(GENMASK(5, 4) | GENMASK(1, 0));

	phy_dp_clk_ctl = readl(tcphy->base + PHY_DP_CLK_CTL);
	phy_dp_clk_ctl &= ~(GENMASK(15, 12) | GENMASK(11, 8));

	switch (link_rate) {
	case 162000:
		cmn_diag_hsclk_sel |= (3 << 4) | (0 << 0);
		phy_dp_clk_ctl |= (2 << 12) | (4 << 8);

		phy_cfg = dp_pll_rbr_cfg;
		cfg_size = ARRAY_SIZE(dp_pll_rbr_cfg);
		break;
	case 270000:
		cmn_diag_hsclk_sel |= (3 << 4) | (0 << 0);
		phy_dp_clk_ctl |= (2 << 12) | (4 << 8);

		phy_cfg = dp_pll_hbr_cfg;
		cfg_size = ARRAY_SIZE(dp_pll_hbr_cfg);
		break;
	case 540000:
		cmn_diag_hsclk_sel |= (2 << 4) | (0 << 0);
		phy_dp_clk_ctl |= (1 << 12) | (2 << 8);

		phy_cfg = dp_pll_hbr2_cfg;
		cfg_size = ARRAY_SIZE(dp_pll_hbr2_cfg);
		break;
	default:
		return -EINVAL;
	}

	writel(cmn_diag_hsclk_sel, tcphy->base + CMN_DIAG_HSCLK_SEL);
	writel(phy_dp_clk_ctl, tcphy->base + PHY_DP_CLK_CTL);

	/* load the configuration of PLL1 */
	for (i = 0; i < cfg_size; i++)
		writel(phy_cfg[i].value, tcphy->base + phy_cfg[i].addr);

	/* Enable the PLL */
	reg = readl(tcphy->base + PHY_DP_CLK_CTL);
	reg &= ~DP_PLL_ENABLE_MASK;
	reg |= DP_PLL_ENABLE;
	writel(reg, tcphy->base + PHY_DP_CLK_CTL);

	ret = readl_poll_timeout(tcphy->base + PHY_DP_CLK_CTL, reg,
	        reg & DP_PLL_READY,
	        10, PHY_MODE_SET_TIMEOUT);
	if (ret < 0) {
		dev_err(tcphy->dev, "wait DP PLL ready timeout\n");
		return ret;
	}

	/* Enable PMA PLL clocks */
	reg = readl(tcphy->base + PHY_DP_CLK_CTL);
	reg &= ~DP_PLL_CLOCK_ENABLE_MASK;
	reg |= DP_PLL_CLOCK_ENABLE;
	writel(reg, tcphy->base + PHY_DP_CLK_CTL);

	ret = readl_poll_timeout(tcphy->base + PHY_DP_CLK_CTL, reg,
	        reg & DP_PLL_CLOCK_ENABLE_ACK,
	        10, PHY_MODE_SET_TIMEOUT);
	if (ret) {
		dev_err(tcphy->dev, "wait DP PLL clock enabled timeout\n");
		return ret;
	}

	/* The PMA must go through the A2 power state upon a data rate change */
	ret = tcphy_dp_set_power_state(tcphy, PHY_DP_POWER_STATE_A2);
	if (ret) {
		dev_err(tcphy->dev, "failed to enter A2 state: %d\n", ret);
		return ret;
	}

	/* change the PHY power state to A0 */
	ret = tcphy_dp_set_power_state(tcphy, PHY_DP_POWER_STATE_A0);
	if (ret) {
		dev_err(tcphy->dev, "failed to enter A0 state: %d\n", ret);
		return ret;
	}

	tcphy->dp.link_rate = link_rate;

	return 0;
}
#endif
static void tcphy_cfg_24m(struct smartchip_typec_phy *tcphy)
{
	u32 i, rdata;

	/*
	 * cmn_ref_clk_sel = 3, select the 24Mhz for clk parent
	 * cmn_psm_clk_dig_div = 2, set the clk division to 2
	 */
	writel(0x830, tcphy->base + PMA_CMN_CTRL1);
	for (i = 0; i < 4; i++) {
		/*
		 * The following PHY configuration assumes a 24 MHz reference
		 * clock.
		 */
		writel(0x90, tcphy->base + XCVR_DIAG_LANE_FCM_EN_MGN(i));
		writel(0x960, tcphy->base + TX_RCVDET_EN_TMR(i));
		writel(0x30, tcphy->base + TX_RCVDET_ST_TMR(i));
	}

	rdata = readl(tcphy->base + CMN_DIAG_HSCLK_SEL);
	rdata &= ~CLK_PLL_MASK;
	rdata |= CLK_PLL1_DIV2;
	writel(rdata, tcphy->base + CMN_DIAG_HSCLK_SEL);
}

static void tcphy_cfg_usb3_pll(struct smartchip_typec_phy *tcphy)
{
	u32 i;

	/* load the configuration of PLL0 */
	for (i = 0; i < ARRAY_SIZE(usb3_pll_cfg); i++)
		writel(usb3_pll_cfg[i].value,
		    tcphy->base + usb3_pll_cfg[i].addr);
}

static void tcphy_cfg_dp_pll(struct smartchip_typec_phy *tcphy, int link_rate)
{
	const struct phy_reg *phy_cfg;
	u32 clk_ctrl;
	u32 i, cfg_size, hsclk_sel;

	hsclk_sel = readl(tcphy->base + CMN_DIAG_HSCLK_SEL);
	hsclk_sel &= ~CLK_PLL_MASK;

	switch (link_rate) {
	case 540000:
		clk_ctrl = DP_PLL_DATA_RATE_HBR2;
		hsclk_sel |= CLK_PLL1_DIV1;
		phy_cfg = dp_pll_hbr2_cfg;
		cfg_size = ARRAY_SIZE(dp_pll_hbr2_cfg);
		break;
	case 270000:
		clk_ctrl = DP_PLL_DATA_RATE_HBR;
		hsclk_sel |= CLK_PLL1_DIV2;
		phy_cfg = dp_pll_hbr_cfg;
		cfg_size = ARRAY_SIZE(dp_pll_hbr_cfg);
		break;
	case 162000:
	default:
		clk_ctrl = DP_PLL_DATA_RATE_RBR;
		hsclk_sel |= CLK_PLL1_DIV2;
		phy_cfg = dp_pll_rbr_cfg;
		cfg_size = ARRAY_SIZE(dp_pll_rbr_cfg);
		break;
	}

	clk_ctrl |= DP_PLL_CLOCK_ENABLE | DP_PLL_ENABLE;
	writel(clk_ctrl, tcphy->base + PHY_DP_CLK_CTL);
	writel(hsclk_sel, tcphy->base + CMN_DIAG_HSCLK_SEL);

	/* load the configuration of PLL1 */
	for (i = 0; i < cfg_size; i++)
		writel(phy_cfg[i].value, tcphy->base + phy_cfg[i].addr);

	tcphy->dp.link_rate = link_rate;
}

static void tcphy_tx_usb3_cfg_lane(struct smartchip_typec_phy *tcphy, u32 lane)
{
	writel(0x7799, tcphy->base + TX_PSC_A0(lane));
	writel(0x7798, tcphy->base + TX_PSC_A1(lane));
	writel(0x5098, tcphy->base + TX_PSC_A2(lane));
	writel(0x5098, tcphy->base + TX_PSC_A3(lane));
	writel(0, tcphy->base + TX_TXCC_MGNFS_MULT_000(lane));
	writel(0xbf, tcphy->base + XCVR_DIAG_BIDI_CTRL(lane));
}

static void tcphy_rx_usb3_cfg_lane(struct smartchip_typec_phy *tcphy, u32 lane)
{
	writel(0xa6fd, tcphy->base + RX_PSC_A0(lane));
	writel(0xa6fd, tcphy->base + RX_PSC_A1(lane));
	writel(0xa410, tcphy->base + RX_PSC_A2(lane));
	writel(0x2410, tcphy->base + RX_PSC_A3(lane));
	writel(0x23ff, tcphy->base + RX_PSC_CAL(lane));
	writel(0x13, tcphy->base + RX_SIGDET_HL_FILT_TMR(lane));
	writel(0x03e7, tcphy->base + RX_REE_CTRL_DATA_MASK(lane));
	writel(0x1004, tcphy->base + RX_DIAG_SIGDET_TUNE(lane));
	writel(0x2010, tcphy->base + RX_PSC_RDY(lane));
	writel(0xfb, tcphy->base + XCVR_DIAG_BIDI_CTRL(lane));
}

static void tcphy_dp_cfg_lane(struct smartchip_typec_phy *tcphy, int link_rate,
    u8 swing, u8 pre_emp, u32 lane)
{
	u16 val;

	writel(0xbefc, tcphy->base + XCVR_PSM_RCTRL(lane));
	writel(0x6799, tcphy->base + TX_PSC_A0(lane));
	writel(0x6798, tcphy->base + TX_PSC_A1(lane));
	writel(0x98, tcphy->base + TX_PSC_A2(lane));
	writel(0x98, tcphy->base + TX_PSC_A3(lane));

	writel(tcphy->config[swing][pre_emp].swing,
	    tcphy->base + TX_TXCC_MGNFS_MULT_000(lane));
	writel(tcphy->config[swing][pre_emp].pe,
	    tcphy->base + TX_TXCC_CPOST_MULT_00(lane));

	if (swing == 2 && pre_emp == 0 && link_rate != 540000) {
		writel(0x700, tcphy->base + TX_DIAG_TX_DRV(lane));
		writel(0x13c, tcphy->base + TX_TXCC_CAL_SCLR_MULT(lane));
	} else {
		writel(0x128, tcphy->base + TX_TXCC_CAL_SCLR_MULT(lane));
		writel(0x0400, tcphy->base + TX_DIAG_TX_DRV(lane));
	}

	val = readl(tcphy->base + XCVR_DIAG_PLLDRC_CTRL(lane));
	val = val & 0x8fff;
	switch (link_rate) {
	case 540000:
		val |= (5 << 12);
		break;
	case 162000:
	case 270000:
	default:
		val |= (6 << 12);
		break;
	}
	writel(val, tcphy->base + XCVR_DIAG_PLLDRC_CTRL(lane));
}

static void tcphy_dp_aux_set_flip(struct smartchip_typec_phy *tcphy)
{
	u16 tx_ana_ctrl_reg_1;

	/*
	 * Select the polarity of the xcvr:
	 * 1, Reverses the polarity (If TYPEC, Pulls ups aux_p and pull
	 * down aux_m)
	 * 0, Normal polarity (if TYPEC, pulls up aux_m and pulls down
	 * aux_p)
	 */
	tx_ana_ctrl_reg_1 = readl(tcphy->base + TX_ANA_CTRL_REG_1);
	if (!tcphy->flip)
		tx_ana_ctrl_reg_1 |= AUXDA_POLARITY;
	else
		tx_ana_ctrl_reg_1 &= ~AUXDA_POLARITY;
	writel(tx_ana_ctrl_reg_1, tcphy->base + TX_ANA_CTRL_REG_1);
}

static void tcphy_dp_aux_calibration(struct smartchip_typec_phy *tcphy)
{
	u16 val;
	u16 tx_ana_ctrl_reg_1;
	u16 tx_ana_ctrl_reg_2;
	s32 pu_calib_code, pd_calib_code;
	s32 pu_adj, pd_adj;
	u16 calib;

	/*
	 * Calculate calibration code as per docs: use an average of the
	 * pull down and pull up.  Then add in adjustments.
	 */
	val = readl(tcphy->base + CMN_TXPUCAL_CTRL);
	pu_calib_code = CMN_CALIB_CODE_POS(val);
	printk(KERN_ERR "CMN_TXPUCAL_CTRL=%d %d\n", val, pu_calib_code);

	val = readl(tcphy->base + CMN_TXPDCAL_CTRL);
	pd_calib_code = CMN_CALIB_CODE_POS(val);
	printk(KERN_ERR "CMN_TXPDCAL_CTRL=%d %d\n", val, pd_calib_code);

	val = readl(tcphy->base + CMN_TXPU_ADJ_CTRL);
	pu_adj = CMN_CALIB_CODE(val);
	printk(KERN_ERR "CMN_TXPU_ADJ_CTRL=%d %d\n", val, pu_adj);

	val = readl(tcphy->base + CMN_TXPD_ADJ_CTRL);
	pd_adj = CMN_CALIB_CODE(val);
	printk(KERN_ERR "CMN_TXPD_ADJ_CTRL=%d %d\n", val, pd_adj);

	calib = (pu_calib_code + pd_calib_code) / 2 + pu_adj + pd_adj;
	printk(KERN_ERR "calib=%d\n", calib);

	/* disable txda_cal_latch_en for rewrite the calibration values */
	tx_ana_ctrl_reg_1 = readl(tcphy->base + TX_ANA_CTRL_REG_1);
	tx_ana_ctrl_reg_1 &= ~TXDA_CAL_LATCH_EN;
	writel(tx_ana_ctrl_reg_1, tcphy->base + TX_ANA_CTRL_REG_1);

	/* write the calibration, then delay 10 ms as sample in docs */
	val = readl(tcphy->base + TX_DIG_CTRL_REG_2);
	val &= ~(TX_RESCAL_CODE_MASK << TX_RESCAL_CODE_OFFSET);
	val |= calib << TX_RESCAL_CODE_OFFSET;
	writel(val, tcphy->base + TX_DIG_CTRL_REG_2);
	usleep_range(10000, 10050);

	/*
	 * Enable signal for latch that sample and holds calibration values.
	 * Activate this signal for 1 clock cycle to sample new calibration
	 * values.
	 */
	tx_ana_ctrl_reg_1 |= TXDA_CAL_LATCH_EN;
	writel(tx_ana_ctrl_reg_1, tcphy->base + TX_ANA_CTRL_REG_1);
	usleep_range(150, 200);

	/* set TX Voltage Level and TX Deemphasis to 0 */
	writel(0, tcphy->base + PHY_DP_TX_CTL);

	/* re-enable decap */
	tx_ana_ctrl_reg_2 = XCVR_DECAP_EN;
	writel(tx_ana_ctrl_reg_2, tcphy->base + TX_ANA_CTRL_REG_2);
	udelay(1);
	tx_ana_ctrl_reg_2 |= XCVR_DECAP_EN_DEL;
	writel(tx_ana_ctrl_reg_2, tcphy->base + TX_ANA_CTRL_REG_2);

	writel(0, tcphy->base + TX_ANA_CTRL_REG_3);

	tx_ana_ctrl_reg_1 |= TXDA_UPHY_SUPPLY_EN;
	writel(tx_ana_ctrl_reg_1, tcphy->base + TX_ANA_CTRL_REG_1);
	udelay(1);
	tx_ana_ctrl_reg_1 |= TXDA_UPHY_SUPPLY_EN_DEL;
	writel(tx_ana_ctrl_reg_1, tcphy->base + TX_ANA_CTRL_REG_1);

	//wkwk
	//tx_ana_ctrl_reg_2 |= (TXDA_DRV_LDO_BG_FB_EN | TXDA_DRV_LDO_BG_REF_EN);
	//writel(tx_ana_ctrl_reg_2, tcphy->base + TX_ANA_CTRL_REG_2);

	writel(0, tcphy->base + TX_ANA_CTRL_REG_5);

	/*
	 * Programs txda_drv_ldo_prog[15:0], Sets driver LDO
	 * voltage 16'h1001 for DP-AUX-TX and RX
	 */
	writel(0x1001, tcphy->base + TX_ANA_CTRL_REG_4);

	/* re-enables Bandgap reference for LDO */
	tx_ana_ctrl_reg_1 |= TXDA_DRV_LDO_EN;
	writel(tx_ana_ctrl_reg_1, tcphy->base + TX_ANA_CTRL_REG_1);
	udelay(5);
	tx_ana_ctrl_reg_1 |= TXDA_BGREF_EN;
	writel(tx_ana_ctrl_reg_1, tcphy->base + TX_ANA_CTRL_REG_1);

	/*
	 * re-enables the transmitter pre-driver, driver data selection MUX,
	 * and receiver detect circuits.
	 */
	tx_ana_ctrl_reg_2 |= TXDA_DRV_PREDRV_EN;
	writel(tx_ana_ctrl_reg_2, tcphy->base + TX_ANA_CTRL_REG_2);
	udelay(1);
	tx_ana_ctrl_reg_2 |= TXDA_DRV_PREDRV_EN_DEL;
	writel(tx_ana_ctrl_reg_2, tcphy->base + TX_ANA_CTRL_REG_2);

#if 1
	/*
	 * Do all the undocumented magic:
	 * - Turn on TXDA_DP_AUX_EN, whatever that is, even though sample
	 *   never shows this going on.
	 * - Turn on TXDA_DECAP_EN (and TXDA_DECAP_EN_DEL) even though
	 *   docs say for aux it's always 0.
	 * - Turn off the LDO and BGREF, which we just spent time turning
	 *   on above (???).
	 *
	 * Without this magic, things seem worse.
	 */
	tx_ana_ctrl_reg_1 |= TXDA_DP_AUX_EN;
	tx_ana_ctrl_reg_1 |= TXDA_DECAP_EN;
	tx_ana_ctrl_reg_1 &= ~TXDA_DRV_LDO_EN;
	tx_ana_ctrl_reg_1 &= ~TXDA_BGREF_EN;
	writel(tx_ana_ctrl_reg_1, tcphy->base + TX_ANA_CTRL_REG_1);
	udelay(1);
	tx_ana_ctrl_reg_1 |= TXDA_DECAP_EN_DEL;
	writel(tx_ana_ctrl_reg_1, tcphy->base + TX_ANA_CTRL_REG_1);

	/*
	 * Undo the work we did to set the LDO voltage.
	 * This doesn't seem to help nor hurt, but it kinda goes with the
	 * undocumented magic above.
	 */
	writel(0, tcphy->base + TX_ANA_CTRL_REG_4);

	/* Don't set voltage swing to 400 mV peak to peak (differential) */
	writel(0, tcphy->base + TXDA_COEFF_CALC_CTRL);

	/* Init TXDA_CYA_AUXDA_CYA for unknown magic reasons */
	writel(0, tcphy->base + TXDA_CYA_AUXDA_CYA);

	/*
	 * More undocumented magic, presumably the goal of which is to
	 * make the "auxda_source_aux_oen" be ignored and instead to decide
	 * about "high impedance state" based on what software puts in the
	 * register TXDA_COEFF_CALC_CTRL (see TX_HIGH_Z).  Since we only
	 * program that register once and we don't set the bit TX_HIGH_Z,
	 * presumably the goal here is that we should never put the analog
	 * driver in high impedance state.
	 */
	val = readl(tcphy->base + TX_DIG_CTRL_REG_2);
	val |= TX_HIGH_Z_TM_EN;
	writel(val, tcphy->base + TX_DIG_CTRL_REG_2);
#endif
}

static int tcphy_phy_init(struct smartchip_typec_phy *tcphy, u8 mode)
{
	//struct smartchip_usb3phy_port_cfg *cfg = &tcphy->port_cfgs;
	int ret, i;
	u32 val;
	void __iomem               *tmp_addr;

	int link_rate = DP_DEFAULT_RATE;
	int lane_count = 4;

	printk(KERN_ERR "tcphy_phy_init\n");

	//open phy clock
	tmp_addr = ioremap(0x60630014, 4);
	writel(0xffffffff, tmp_addr);
	iounmap(tmp_addr);

	//dereset phy
	//tmp_addr = ioremap(0x60631014, 4);
	//writel(0xffffffff, tmp_addr);
	//iounmap(tmp_addr);

	//reset phy
	tmp_addr = ioremap(0x60631014, 4);
	writel(0xffffffbf, tmp_addr);

	/*ret = clk_prepare_enable(tcphy->clk_core);
	if (ret) {
	    dev_err(tcphy->dev, "Failed to prepare_enable core clock\n");
	    return ret;
	}

	ret = clk_prepare_enable(tcphy->clk_ref);
	if (ret) {
	    dev_err(tcphy->dev, "Failed to prepare_enable ref clock\n");
	    goto err_clk_core;
	}

	reset_control_deassert(tcphy->tcphy_rst);*/

	printk(KERN_ERR "phy init\n");

	//property_enable(tcphy, &cfg->typec_conn_dir, tcphy->flip);
	tcphy_dp_aux_set_flip(tcphy);

	tcphy_cfg_24m(tcphy);
	//tcphy_set_lane_mapping(tcphy, mode);

	if (mode == MODE_DFP_DP || mode == (MODE_DFP_DP | MODE_DFP_DP_ONLY)) {
		printk(KERN_ERR "usb type c dp init\n");
		tcphy_cfg_dp_pll(tcphy, DP_DEFAULT_RATE);
		for (i = 0; i < 4; i++)
			tcphy_dp_cfg_lane(tcphy, DP_DEFAULT_RATE, 0, 0, i);

		//writel(PIN_ASSIGN_C_E, tcphy->base + PHY_PMA_LANE_CFG);
		printk(KERN_ERR "link_rate=%d, lane_count=%d\n",
		    link_rate, lane_count);
	} else {
		printk(KERN_ERR "usb type c usb init\n");
		/*tcphy_cfg_usb3_pll(tcphy);
		tcphy_tx_usb3_cfg_lane(tcphy, 3);
		tcphy_rx_usb3_cfg_lane(tcphy, 2);
		tcphy_tx_usb3_cfg_lane(tcphy, 0);
		tcphy_rx_usb3_cfg_lane(tcphy, 1);*/

		tcphy_cfg_usb3_pll(tcphy);
		tcphy_cfg_dp_pll(tcphy, DP_DEFAULT_RATE);
		if (tcphy->flip) {
			printk(KERN_ERR "1 flip %d\n", tcphy->flip);
			tcphy_tx_usb3_cfg_lane(tcphy, 3);
			tcphy_rx_usb3_cfg_lane(tcphy, 2);
			tcphy_dp_cfg_lane(tcphy, DP_DEFAULT_RATE, 0, 0, 0);
			tcphy_dp_cfg_lane(tcphy, DP_DEFAULT_RATE, 0, 0, 1);
		} else {
			printk(KERN_ERR "0 flip %d\n", tcphy->flip);
			tcphy_tx_usb3_cfg_lane(tcphy, 0);
			tcphy_rx_usb3_cfg_lane(tcphy, 1);
			tcphy_dp_cfg_lane(tcphy, DP_DEFAULT_RATE, 0, 0, 2);
			tcphy_dp_cfg_lane(tcphy, DP_DEFAULT_RATE, 0, 0, 3);
		}
		//writel(PIN_ASSIGN_D_F, tcphy->base + PHY_PMA_LANE_CFG);
	}

	tcphy_set_lane_mapping(tcphy, mode);

	/*ret = tcphy_dp_set_lane_count(tcphy, lane_count);
	if (ret) {
	    dev_err(tcphy->dev, "failed to set lane count\n");
	    return ret;
	}*/

	printk(KERN_ERR "read PHY_DP_MODE_CTL\n");

	val = readl(tcphy->base + PHY_DP_MODE_CTL);
	val &= ~DP_MODE_MASK;
	val |= DP_MODE_ENTER_A2 | DP_LINK_RESET_DEASSERTED;
	writel(val, tcphy->base + PHY_DP_MODE_CTL);

	//dereset phy
	writel(0xffffffff, tmp_addr);
	iounmap(tmp_addr);

	//reset_control_deassert(tcphy->uphy_rst);

	/*if (mode == MODE_DFP_USB)
	{
	    tmp_addr = ioremap(0x603c0008, 4);
	    writel(0x00001531, tmp_addr);
	    iounmap(tmp_addr);
	}*/

	ret = readx_poll_timeout(readl, tcphy->base + PMA_CMN_CTRL1,
	        val, val & CMN_READY, 10,
	        PHY_MODE_SET_TIMEOUT);
	if (ret < 0) {
		dev_err(tcphy->dev, "wait pma ready timeout\n");
		ret = -ETIMEDOUT;
		goto err_wait_pma;
	}

	//reset_control_deassert(tcphy->pipe_rst);

	return 0;

err_wait_pma:
	reset_control_assert(tcphy->uphy_rst);
	reset_control_assert(tcphy->tcphy_rst);
	clk_disable_unprepare(tcphy->clk_ref);
	//err_clk_core:
	clk_disable_unprepare(tcphy->clk_core);
	return ret;
}

static void tcphy_phy_deinit(struct smartchip_typec_phy *tcphy)
{
	void __iomem               *tmp_addr;

	//reset phy
	tmp_addr = ioremap(0x60631014, 4);
	//writel(0xffffffbd, tmp_addr);
	writel(0xffffffbf, tmp_addr);
	iounmap(tmp_addr);

	//close phy clock
	tmp_addr = ioremap(0x60630014, 4);
	writel(0xffffffdf, tmp_addr);
	iounmap(tmp_addr);
	/*reset_control_assert(tcphy->tcphy_rst);
	reset_control_assert(tcphy->uphy_rst);
	reset_control_assert(tcphy->pipe_rst);
	clk_disable_unprepare(tcphy->clk_core);
	clk_disable_unprepare(tcphy->clk_ref);*/
	return;
}

static int tcphy_get_mode(struct smartchip_typec_phy *tcphy)
{
	struct extcon_dev *edev = tcphy->extcon;
	union extcon_property_value property;
	unsigned int id;
	bool ufp, dp;
	u8 mode;
	int ret;

	if (!edev) {
		mode = MODE_DFP_USB;
		id = EXTCON_USB_HOST;
		tcphy->flip = 0;
		return mode;
	}

	ufp = extcon_get_state(edev, EXTCON_USB);
	dp = extcon_get_state(edev, EXTCON_DISP_DP);

	mode = MODE_DFP_USB;
	id = EXTCON_USB_HOST;

	if (ufp) {
		mode = MODE_UFP_USB;
		id = EXTCON_USB;
	} else if (dp) {
		mode = MODE_DFP_DP;
		id = EXTCON_DISP_DP;

		ret = extcon_get_property(edev, id, EXTCON_PROP_USB_SS,
		        &property);
		if (ret) {
			dev_err(tcphy->dev, "get superspeed property failed\n");
			return ret;
		}
		printk("EXTCON_PROP_USB_SS: %d", property.intval);

		if (property.intval) {
			mode |= MODE_DFP_DP_ONLY;
			printk(KERN_INFO "dp only mode");
		} else {
			printk(KERN_INFO "typec dp mode");
		}
	}

	ret = extcon_get_property(edev, id, EXTCON_PROP_USB_TYPEC_POLARITY,
	        &property);
	if (ret) {
		printk(KERN_ERR "get polarity property failed\n");
		return ret;
	}

	tcphy->flip = property.intval ? 1 : 0;
	printk(KERN_ERR "tcphy_get_mode get filped %d", tcphy->flip);

	return mode;
}

static int _smartchip_usb3_phy_power_on(struct smartchip_typec_phy *tcphy)
{
	struct smartchip_usb3phy_port_cfg *cfg = &tcphy->port_cfgs;
	//const struct usb3phy_reg *reg = &cfg->pipe_status;
	int new_mode, ret = 0;
	//int timeout;
	//u32 val, i = 0;
	//void __iomem             *tmp_addr;

	printk(KERN_ERR "smartchip_usb_phy_power_on cfg: 0x%p\n", cfg);

	mutex_lock(&tcphy->lock);

	new_mode = tcphy_get_mode(tcphy);
	if (new_mode < 0) {
		ret = new_mode;
		goto unlock_ret;
	}

	printk(KERN_ERR "usb tcphy_get_mode new_mode: %d\n", new_mode);

	/* DP-only mode; fall back to USB2 */
	if (!(new_mode & (MODE_DFP_USB | MODE_UFP_USB))) {
		//tcphy_cfg_usb3_to_usb2_only(tcphy, true);
		goto unlock_ret;
	}

	if (tcphy->mode == new_mode)
		goto unlock_ret;

	if (tcphy->mode == MODE_DISCONNECT) {
		ret = tcphy_phy_init(tcphy, new_mode);
		if (ret)
			goto unlock_ret;
	}

#if 1

	goto unlock_ret;
	/* wait TCPHY for pipe ready */
	//for (timeout = 0; timeout < 100; timeout++) {
	//  regmap_read(tcphy->grf_regs, reg->offset, &val);
	//  if (!(val & BIT(reg->enable_bit))) {
	//    tcphy->mode |= new_mode & (MODE_DFP_USB | MODE_UFP_USB);

	/* enable usb3 host */
	//    tcphy_cfg_usb3_to_usb2_only(tcphy, false);
	//    goto unlock_ret;
	//  }
	//  usleep_range(10, 20);
	//}
#else

	tcphy->mode |= new_mode & (MODE_DFP_USB | MODE_UFP_USB);

	/* POWER_STATUS */
	val = readl(tcphy->base + PHY_DP_MODE_CTL);
	while(!(val & (1 << 6)) && i < 50) {
		val = readl(tcphy->base + PHY_DP_MODE_CTL);
		++i;
	}
	printk(KERN_ERR "Type C gadget phy power status 2 0x%08x (%d)!\n", val, i);

	val = readl(tcphy->base + PHY_DP_MODE_CTL);
	val &= ~DP_MODE_MASK;
	val |= DP_MODE_ENTER_A0 | DP_LINK_RESET_DEASSERTED;
	writel(val, tcphy->base + PHY_DP_MODE_CTL);

	i = 0;

	/* POWER_STATUS */
	val = readl(tcphy->base + PHY_DP_MODE_CTL);
	while(!(val & (1 << 4)) && i < 50) {
		val = readl(tcphy->base + PHY_DP_MODE_CTL);
		++i;
	}
	printk(KERN_ERR "Type C gadget phy power status 0 0x%08x (%d)!\n", val, i);

	tcphy_dp_aux_calibration(tcphy);

	/* clk valid */
	/*writel(0x0000153d, tcphy->user_define_base + 0x0008);

	val = readl(tcphy->tcpd_base + 0x0100);
	printk(KERN_ERR "Type C gadget phy 0x%08x!\n", val);
	//assert dp_hpd
	writel((val | 0x00000200), tcphy->tcpd_base + 0x0100);

	//wkwk
	tmp_addr = ioremap(0x603c0000, 4);
	writel(0x00000001, tmp_addr);
	iounmap(tmp_addr);*/

	goto unlock_ret;

#endif
	if (tcphy->mode == MODE_DISCONNECT)
		tcphy_phy_deinit(tcphy);

	ret = -ETIMEDOUT;

unlock_ret:
	mutex_unlock(&tcphy->lock);
	return ret;
}

static int smartchip_usb3_phy_power_on(struct phy *phy)
{
	struct smartchip_typec_phy *tcphy = phy_get_drvdata(phy);
	int ret;
	int tries;

	for (tries = 0; tries < POWER_ON_TRIES; tries++) {
		ret = _smartchip_usb3_phy_power_on(tcphy);
		if (!ret)
			break;
	}

	if (tries && !ret)
		dev_info(tcphy->dev, "Needed %d loops to turn on\n", tries);

	return ret;
}

static int smartchip_usb3_phy_power_off(struct phy *phy)
{
	struct smartchip_typec_phy *tcphy = phy_get_drvdata(phy);

	mutex_lock(&tcphy->lock);

	if (!(tcphy->mode & (MODE_UFP_USB | MODE_DFP_USB)))
		//tcphy_cfg_usb3_to_usb2_only(tcphy, false);

		if (tcphy->mode == MODE_DISCONNECT)
			goto unlock;

	tcphy->mode &= ~(MODE_UFP_USB | MODE_DFP_USB);
	if (tcphy->mode == MODE_DISCONNECT)
		tcphy_phy_deinit(tcphy);

unlock:
	mutex_unlock(&tcphy->lock);
	return 0;
}

static const struct phy_ops smartchip_usb3_phy_ops = {
	.power_on   = smartchip_usb3_phy_power_on,
	.power_off  = smartchip_usb3_phy_power_off,
	.owner    = THIS_MODULE,
};

static int smartchip_dp_phy_power_on(struct phy *phy)
{
	struct smartchip_typec_phy *tcphy = phy_get_drvdata(phy);
	struct smartchip_usb3phy_port_cfg *cfg = &tcphy->port_cfgs;
	int new_mode, ret = 0;
	u32 val;

	printk(KERN_ERR "smartchip_dp_phy_power_on cfg: 0x%p\n", cfg);

	mutex_lock(&tcphy->lock);

	new_mode = tcphy_get_mode(tcphy);
	if (new_mode < 0) {
		ret = new_mode;
		goto unlock_ret;
	}

	printk(KERN_ERR "new_mode: %d\n", new_mode);

	if (!(new_mode & MODE_DFP_DP)) {
		ret = -ENODEV;
		goto unlock_ret;
	}

	if (tcphy->mode == new_mode)
		goto unlock_ret;

	/*
	 * If the PHY has been power on, but the mode is not DP only mode,
	 * re-init the PHY for setting all of 4 lanes to DP.
	 */
	if (new_mode == MODE_DFP_DP && tcphy->mode != MODE_DISCONNECT) {
		tcphy_phy_deinit(tcphy);
		ret = tcphy_phy_init(tcphy, new_mode);
	} else if (tcphy->mode == MODE_DISCONNECT) {
		ret = tcphy_phy_init(tcphy, new_mode);
	}
	if (ret)
		goto unlock_ret;

	/* enter A2 mode */
	ret = readx_poll_timeout(readl, tcphy->base + PHY_DP_MODE_CTL,
	        val, val & DP_MODE_A2_ACK, 1000,
	        PHY_MODE_SET_TIMEOUT);
	if (ret < 0) {
		dev_err(tcphy->dev, "failed to wait TCPHY enter A2\n");
		goto power_on_finish;
	}
	printk(KERN_ERR "TCPHY enter A2!\n");

	/* AUX calibration */
	tcphy_dp_aux_calibration(tcphy);
	printk(KERN_ERR "tcphy_dp_aux_calibration completed!\n");

	val = readl(tcphy->base + PHY_DP_MODE_CTL);
	val &= ~DP_MODE_MASK;
	val |= DP_MODE_ENTER_A0;
	writel(val, tcphy->base + PHY_DP_MODE_CTL);

	/* enter A0 mode */
	ret = tcphy_dp_set_power_state(tcphy, PHY_DP_POWER_STATE_A0);
	if (ret) {
		dev_err(tcphy->dev, "failed to enter A0 power state\n");
		goto power_on_finish;
	}
	printk(KERN_ERR "TCPHY enter A0!\n");

	tcphy->mode |= MODE_DFP_DP;
	printk(KERN_ERR "dp tcphy->mode: %d\n", tcphy->mode);

	/* clk valid */
	//writel(0x0000153d, tcphy->user_define_base + 0x0008);

	//wkwk fw can contorl common phy register
	//writel(0x00000001, tcphy->user_define_base);

power_on_finish:
	if (tcphy->mode == MODE_DISCONNECT)
		tcphy_phy_deinit(tcphy);
unlock_ret:
	mutex_unlock(&tcphy->lock);
	return ret;
}

static int smartchip_dp_phy_power_off(struct phy *phy)
{
	struct smartchip_typec_phy *tcphy = phy_get_drvdata(phy);
	int ret;

	mutex_lock(&tcphy->lock);

	if (tcphy->mode == MODE_DISCONNECT)
		goto unlock;

	tcphy->mode &= ~MODE_DFP_DP;

	ret = tcphy_dp_set_power_state(tcphy, PHY_DP_POWER_STATE_A2);
	if (ret) {
		dev_err(tcphy->dev, "failed to enter A2 power state\n");
		goto unlock;
	}

	if (tcphy->mode == MODE_DISCONNECT)
		tcphy_phy_deinit(tcphy);

unlock:
	mutex_unlock(&tcphy->lock);
	return 0;
}

static const struct phy_ops smartchip_dp_phy_ops = {
	.power_on   = smartchip_dp_phy_power_on,
	.power_off  = smartchip_dp_phy_power_off,
	.owner    = THIS_MODULE,
};

static int tcphy_parse_dt(struct smartchip_typec_phy *tcphy,
    struct device *dev)
{
	//struct smartchip_usb3phy_port_cfg *cfg = &tcphy->port_cfgs;
	//int ret;

	/*
	* check if phy_config pass from dts, if no,
	* use default phy config value.
	*/
	/*ret = of_property_read_u32_array(dev->of_node, "smartchip,phy-config",
	  (u32 *)tcphy->config, sizeof(tcphy->config) / sizeof(u32));
	if (ret)
	  memcpy(tcphy->config, tcphy_default_config,
	         sizeof(tcphy->config));*/

	memcpy(tcphy->config, tcphy_default_config,
	    sizeof(tcphy->config));

	return 0;
}

static void typec_phy_pre_init(struct smartchip_typec_phy *tcphy)
{
	//struct smartchip_usb3phy_port_cfg *cfg = &tcphy->port_cfgs;

	//tcpc reset

	//dp reset

	tcphy->mode = MODE_DISCONNECT;
}

static int smartchip_typec_phy_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct device_node *child_np;
	struct smartchip_typec_phy *tcphy;
	struct phy_provider *phy_provider;
	struct resource *res;
	int ret;

	tcphy = devm_kzalloc(dev, sizeof(*tcphy), GFP_KERNEL);
	if (!tcphy)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	printk(KERN_ERR "type c phy start: 0x%08x\n", res->start);

	tcphy->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(tcphy->base))
		return PTR_ERR(tcphy->base);

	/*tcphy->tcpd_base = devm_ioremap_resource(dev, res);        //0x6034xxxx
	if (IS_ERR(tcphy->tcpd_base))
	    return PTR_ERR(tcphy->tcpd_base);

	tcphy->comm_phy_base = tcphy->tcpd_base + 0x40000;        //0x6038xxxx
	tcphy->base = tcphy->comm_phy_base;

	tcphy->user_define_base = tcphy->tcpd_base + 0x80000;      //0x603cxxxx*/

	ret = tcphy_parse_dt(tcphy, dev);
	if (ret)
		return ret;

	printk(KERN_ERR "Type C phy setting!(reg: %p)\n", tcphy->base);

	tcphy->dev = dev;
	platform_set_drvdata(pdev, tcphy);
	mutex_init(&tcphy->lock);

	typec_phy_pre_init(tcphy);

	if (device_property_read_bool(dev, "extcon")) {
		tcphy->extcon = extcon_get_edev_by_phandle(dev, 0);
		if (PTR_ERR(tcphy->extcon) == -EPROBE_DEFER)
			return -EPROBE_DEFER;

		if (IS_ERR(tcphy->extcon)) {
			if (PTR_ERR(tcphy->extcon) != -EPROBE_DEFER)
				printk(KERN_ERR "Invalid or missing extcon\n");
			return PTR_ERR(tcphy->extcon);
		}
	} else {
		printk(KERN_ERR "no extcon\n");
	}

	printk(KERN_ERR "phy extcon %p\n", tcphy->extcon);

	//tcphy->typec_phy_config = typec_dp_phy_config;

	pm_runtime_enable(dev);

	printk(KERN_ERR "np: 0x%p\n", np);

	for_each_available_child_of_node(np, child_np) {
		struct phy *phy;

		if (!of_node_cmp(child_np->name, "dp-port"))
			phy = devm_phy_create(dev, child_np,
			        &smartchip_dp_phy_ops);
		else if (!of_node_cmp(child_np->name, "usb3-port"))
			phy = devm_phy_create(dev, child_np,
			        &smartchip_usb3_phy_ops);
		else
			continue;

		if (IS_ERR(phy)) {
			dev_err(dev, "failed to create phy: %s\n",
			    child_np->name);
			pm_runtime_disable(dev);
			return PTR_ERR(phy);
		}

		printk(KERN_ERR "type c port name: %s %p!\n", child_np->name, phy);
		phy_set_drvdata(phy, tcphy);
	}

	phy_provider = devm_of_phy_provider_register(dev, of_phy_simple_xlate);
	if (IS_ERR(phy_provider)) {
		dev_err(dev, "Failed to register phy provider\n");
		pm_runtime_disable(dev);
		return PTR_ERR(phy_provider);
	}

	phy_smartchip_smartx_pdev = pdev;

	return 0;

}

static int smartchip_typec_phy_remove(struct platform_device *pdev)
{
	pm_runtime_disable(&pdev->dev);

	return 0;
}

static const struct of_device_id smartchip_typec_phy_dt_ids[] = {
	{ .compatible = "smartchip, smartx-phy" },
	{}
};

MODULE_DEVICE_TABLE(of, smartchip_typec_phy_dt_ids);

static struct platform_driver smartchip_typec_phy_driver = {
	.probe      = smartchip_typec_phy_probe,
	.remove     = smartchip_typec_phy_remove,
	.driver     = {
		.name   = "smartchip-smartx-typec-phy",
		.of_match_table = smartchip_typec_phy_dt_ids,
	},
};

module_platform_driver(smartchip_typec_phy_driver);

MODULE_AUTHOR("Chris Zhong <zyw@rock-chips.com>");
MODULE_AUTHOR("Kever Yang <kever.yang@rock-chips.com>");
MODULE_DESCRIPTION("smartchip USB TYPE-C PHY driver");
MODULE_LICENSE("GPL v2");

