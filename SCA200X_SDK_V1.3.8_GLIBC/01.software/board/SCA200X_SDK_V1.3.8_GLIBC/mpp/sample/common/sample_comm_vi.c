
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>

#include "sc_mipi.h"
#include "sc_common.h"
#include "sample_comm.h"

#define MAX_FRAME_WIDTH 8192

SAMPLE_VI_DUMP_THREAD_INFO_S g_stViDumpRawThreadInfo;

#define SC_GPIO(g, p) {g, p}

#if defined (CONFIGSC_SCA200V200)
#define MIPI_SETTLE_COUNT0        0x04
#define MIPI_SETTLE_COUNT1        0x06
#define MIPI_HDR_SETTLE_COUNT     0x04
#define SENSOR_MCLK_SRC0          SC_PIXEL_PLL_CLK1
#define SENSOR_MCLK_SRC1          SC_PIXEL_PLL_CLK2
#define SENSOR_MCLK_SRC2          SC_PIXEL_PLL_CLK3
#define SENSOR_MCLK_SRC3          SC_PIXEL_PLL_CLK4
#define SENSOR_MCLK_SRC4          SC_PIXEL_PLL_CLK3
#define SENSOR_MCLK_SRC5          SC_PIXEL_PLL_CLK3
//#define SENSOR_MCLK_SRC6        SC_PIXEL_PLL_CLK3
#define SENSOR_MCLK_SRC6          SC_PIXEL_PLL_CLK1
#define SENSOR_MCLK_ID            SENSOR_MCLK2
#define DEV_FRE                   DEV_FREQ_100MHZ
#elif defined (CONFIGSC_SCA200V100)
#define MIPI_SETTLE_COUNT0        0x18
#define MIPI_SETTLE_COUNT1        0x1C
#define MIPI_HDR_SETTLE_COUNT     0x18
#define SENSOR_MCLK_SRC0          MCLK_SRC_SENSOR0
#define SENSOR_MCLK_SRC1          MCLK_SRC_SENSOR1
#define SENSOR_MCLK_SRC2          MCLK_SRC_SENSOR2
#define SENSOR_MCLK_SRC3          MCLK_SRC_SENSOR3
#define SENSOR_MCLK_SRC4          MCLK_SRC_SENSOR0
#define SENSOR_MCLK_SRC5          MCLK_SRC_SENSOR1
//#define SENSOR_MCLK_SRC6        MCLK_SRC_SENSOR0
#define SENSOR_MCLK_SRC6          MCLK_SRC_SENSOR1
#define SENSOR_MCLK_ID            SENSOR_MCLK_BUTT
#define DEV_FRE                   DEV_FREQ_400MHZ
#else
  #error "CHIP ERR"
#endif

typedef struct
{
    int i2c_id;
    int reset_gpio[2];  /* reset_gpio[0]:group  reset_gpio[1]:pin */
    int power_gpio[2];  /* power_gpio[0]:group  power_gpio[1]:pin */
} sensor_gpio_t;

static const sensor_gpio_t gSnsGpio[VI_MAX_DEV_NUM] =
{
#if defined(CONFIGSC_BOARD1_SOM)
    /*
     * CONFIGSC_BOARD1_SOM
     *   sensor0: i2c1; reset gpio1_17; power gpio1_16
     *   sensor1: i2c2; reset gpio1_19; power gpio1_18
     *   sensor2: i2c3; reset gpio1_25; power gpio1_24
     *   sensor3: i2c0; reset gpio1_27; power gpio1_26
     */
    [0] = {
        .i2c_id = 1,
        .reset_gpio = SC_GPIO(1, 17),
        .power_gpio = SC_GPIO(1, 16),
    },
    [1]= {
        .i2c_id = 2,
        .reset_gpio = SC_GPIO(1, 19),
        .power_gpio = SC_GPIO(1, 18),
    },
    [2]= {
        .i2c_id = 3,
        .reset_gpio = SC_GPIO(1, 25),
        .power_gpio = SC_GPIO(1, 24),
    },
    [3]= {
        .i2c_id = 0,
        .reset_gpio = SC_GPIO(1, 27),
        .power_gpio = SC_GPIO(1, 26),
    },
#elif defined(CONFIGSC_BOARD2_DEV)
    /*
     * CONFIGSC_BOARD2_DEV
     *   sensor0: i2c1; reset gpio3_14; power gpio3_12
     *   sensor1: i2c2; reset gpio0_29; power gpio3_12(gpio3_15 NC)
     *   sensor2: i2c3; reset gpio1_15; power gpio1_16
     *   sensor3: i2c0; reset gpio2_24; power gpio0_31
     */
    [0]= {
        .i2c_id = 1,
        .reset_gpio = SC_GPIO(3, 14),
        .power_gpio = SC_GPIO(3, 12),
    },
    [1]= {
        .i2c_id = 2,
        .reset_gpio = SC_GPIO(0, 29),
        .power_gpio = SC_GPIO(3, 15), /* Single sensor is gpio3_12; Double sensor is gpio3_15 */
    },
    [2]= {
        .i2c_id = 3,
        .reset_gpio = SC_GPIO(1, 15),
        .power_gpio = SC_GPIO(1, 16),
    },
    [3]= {
        .i2c_id = 0,
        .reset_gpio = SC_GPIO(2, 24),
        .power_gpio = SC_GPIO(0, 31), /* Single sensor is gpio1_16; Double sensor is gpio0_31 */
    },
#elif defined(CONFIGSC_BOARD3_DEV)
    /*
     * CONFIGSC_BOARD3_DEV: face product
     *   sensor0: gc2053; i2c0, reset: gpio1_17, power; gpio1_18
     *   sensor1: gc2093; i2c0, reset: gpio1_19, power; gpio1_18
     */
    [0]= {
        .i2c_id = 0,
        .reset_gpio = SC_GPIO(1, 17),
        .power_gpio = SC_GPIO(1, 18),
    },
    [1]= {
        .i2c_id = 0,
        .reset_gpio = SC_GPIO(1, 19),
        .power_gpio = SC_GPIO(1, 18), /* Single sensor is gpio1_18; Double sensor is gpio2_27 */
    },
#elif defined(CONFIGSC_BOARD4_DEV)
    /*
     * CONFIGSC_BOARD4_DEV: ipc product
     *   sensor0: imx307; i2c0, reset: gpio1_17, power; none
     *   sensor1: none
     */
    [0] = {
        .i2c_id = 0,
        .reset_gpio = SC_GPIO(1, 17),
        .power_gpio = SC_GPIO(1, 16), /* fack. need todo */
        },
    [1] = {
        .i2c_id = 0,
        .reset_gpio = SC_GPIO(1, 19),
        .power_gpio = SC_GPIO(2, 27),
        },
#elif defined(CONFIGSC_SCA200V102_BD1_FAST_DEV)
    /*
     * CONFIGSC_SCA200V102_BD1_FAST_DEV
     *   sensor0: i2c1; reset gpio1_9; power gpio1_8
     *   sensor1: i2c2; reset gpio1_11; power gpio1_10
     *   sensor2: i2c3; reset gpio1_13; power gpio1_12
     *   sensor3: i2c0; reset gpio1_15; power gpio1_14
     */
    [0]= {
        .i2c_id = 1,
        .reset_gpio = SC_GPIO(1, 9),
        .power_gpio = SC_GPIO(1, 8),
    },
    [1]= {
        .i2c_id = 2,
        .reset_gpio = SC_GPIO(1, 11),
        .power_gpio = SC_GPIO(1, 10), /* Single sensor is gpio1_8; Double sensor is gpio1_10 */
    },
    [2]= {
        .i2c_id = 3,
        .reset_gpio = SC_GPIO(1, 13),
        .power_gpio = SC_GPIO(1, 12),
    },
    [3]= {
        .i2c_id = 0,
        .reset_gpio = SC_GPIO(1, 15),
        .power_gpio = SC_GPIO(1, 14), /* Single sensor is gpio1_12; Double sensor is gpio1_14 */
    },
#elif defined(CONFIGSC_SCA200V200_BD1_DEV)
    /*
     * CONFIGSC_SCA200V200_BD1_DEV
     *   sensor0: i2c1; reset gpiog_15; power gpiog_3
     *   sensor1: i2c2; reset gpiog_14; power gpiog_10
     */
    [0] = {
        .i2c_id = 1,
        .reset_gpio = SC_GPIO('G' - 'A', 15),
        .power_gpio = SC_GPIO('G' - 'A', 3),
    },
    [1]= {
        .i2c_id = 2,
        .reset_gpio = SC_GPIO('G' - 'A', 14),
        .power_gpio = SC_GPIO('G' - 'A', 10),
    },
#elif defined(CONFIGSC_SCA200V200_BD2_DEV)
    /*
     * CONFIGSC_SCA200V200_BD2_DEV
     *   sensor0: i2c1; reset gpiog_15; power gpiog_12
     *   sensor1: i2c2; reset gpiog_14; power gpiog_10
     */
    [0] = {
        .i2c_id = 1,
        .reset_gpio = SC_GPIO('G' - 'A', 15),
        .power_gpio = SC_GPIO('G' - 'A', 12),
    },
    [1]= {
        .i2c_id = 2,
        .reset_gpio = SC_GPIO('G' - 'A', 14),
        .power_gpio = SC_GPIO('G' - 'A', 10),
    },
#else
#  error   "BORD ERR"
#endif
};

static const SC_COMBO_DEV_ATTR_S MIPI_2lane_CHN0_SENSOR_IMX307_12BIT_2M_NOWDR_ATTR =
{
    .devno      = 0,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = 450,
    .img_rect   = {0, 0, 1920, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_12BIT,
            SC_MIPI_WDR_MODE_NONE,
            {0, 1, -1, -1, -1, -1, -1, -1},
            75,
            MIPI_SETTLE_COUNT0
        }
    },

    .dev_power_attr =
    {
        .reset_gpio = SC_GPIO(-1, -1),
        .power_gpio = SC_GPIO(-1, -1),
        {-1, -1},
        SENSOR_MCLK_SRC0,
        SNS_MCLK_37_125MHZ,
        SENSOR_MCLK2,
        SNS_MCLK_37_125MHZ,
    },
};

static const SC_COMBO_DEV_ATTR_S MIPI_2lane_CHN1_SENSOR_IMX307_12BIT_2M_NOWDR_ATTR =
{
    .devno      = 1,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = 450,
    .img_rect   = {0, 0, 1920, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_12BIT,
            SC_MIPI_WDR_MODE_NONE,
            {0, 1, -1, -1, -1, -1, -1, -1},
            75,
            MIPI_SETTLE_COUNT0
        }
    },

    .dev_power_attr =
    {
        .reset_gpio = SC_GPIO(-1, -1),
        .power_gpio = SC_GPIO(-1, -1),
        {-1, -1},
        SENSOR_MCLK_SRC6,
        SNS_MCLK_37_125MHZ,
        SENSOR_MCLK2,
        SNS_MCLK_37_125MHZ,
    },
};

static const SC_COMBO_DEV_ATTR_S MIPI_2lane_CHN2_SENSOR_IMX307_12BIT_2M_NOWDR_ATTR =
{
    .devno      = 2,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = 450,
    .img_rect   = {0, 0, 1920, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_12BIT,
            SC_MIPI_WDR_MODE_NONE,
            {0, 1, -1, -1, -1, -1, -1, -1},
            75,
            0x18
        }
    },

    .dev_power_attr =
    {
        .reset_gpio = SC_GPIO(-1, -1),
        .power_gpio = SC_GPIO(-1, -1),
        {-1, -1},
        MCLK_SRC_SENSOR2,
        SNS_MCLK_37_125MHZ,
    },
};

static const SC_COMBO_DEV_ATTR_S MIPI_2lane_CHN3_SENSOR_IMX307_12BIT_2M_NOWDR_ATTR =
{
    .devno      = 3,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = 450,
    .img_rect   = {0, 0, 1920, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_12BIT,
            SC_MIPI_WDR_MODE_NONE,
            {0, 1, -1, -1, -1, -1, -1, -1},
            75,
            0x18
        }
    },

    .dev_power_attr =
    {
        .reset_gpio = SC_GPIO(-1, -1),
        .power_gpio = SC_GPIO(-1, -1),
        {-1, -1},
        MCLK_SRC_SENSOR3,
        SNS_MCLK_37_125MHZ,
    },
};

static const SC_COMBO_DEV_ATTR_S MIPI_2lane_CHN0_SENSOR_IMX307_12BIT_2M_WDR2to1_ATTR =
{
    .devno      = 0,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = 891,
    .img_rect   = {0, 0, 1920, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_12BIT,
            SC_MIPI_WDR_MODE_DOL,
            {0, 1, -1, -1, -1, -1, -1, -1},
            75,
            MIPI_HDR_SETTLE_COUNT
        }
    },

    .dev_power_attr =
    {
        .reset_gpio = SC_GPIO(-1, -1),
        .power_gpio = SC_GPIO(-1, -1),
        {-1, -1},
        SENSOR_MCLK_SRC0,
        SNS_MCLK_37_125MHZ,
        SENSOR_MCLK2,
        SNS_MCLK_37_125MHZ,
    },
};

static const SC_COMBO_DEV_ATTR_S MIPI_2lane_CHN0_SENSOR_IMX291_10BIT_2M_60FPS_NOWDR_ATTR =
{
    .devno      = 0,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = 891,
    .img_rect   = {0, 0, 1920, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_10BIT,
            SC_MIPI_WDR_MODE_NONE,
            {0, 1, -1, -1, -1, -1, -1, -1},
            149,
            MIPI_SETTLE_COUNT0
        }
    },

    .dev_power_attr =
    {
        .reset_gpio = SC_GPIO(-1, -1),
        .power_gpio = SC_GPIO(-1, -1),
        {-1, -1},
        SENSOR_MCLK_SRC0,
        SNS_MCLK_37_125MHZ,
        SENSOR_MCLK2,
        SNS_MCLK_37_125MHZ,
    },
};

static const SC_COMBO_DEV_ATTR_S MIPI_4lane_CHN0_SENSOR_IMX291_10BIT_2M_60FPS_NOWDR_ATTR =
{
    .devno      = 0,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = 446,
    .img_rect   = {0, 0, 1920, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_10BIT,
            SC_MIPI_WDR_MODE_NONE,
            {0, 1, 2, 3, -1, -1, -1, -1},
            179,
            MIPI_SETTLE_COUNT0
        }
    },

    .dev_power_attr =
    {
        .reset_gpio = SC_GPIO(-1, -1),
        .power_gpio = SC_GPIO(-1, -1),
        {-1, -1},
        SENSOR_MCLK_SRC0,
        SNS_MCLK_37_125MHZ,
        SENSOR_MCLK2,
        SNS_MCLK_37_125MHZ,
    },
};

static const SC_COMBO_DEV_ATTR_S MIPI_4lane_CHN0_SENSOR_IMX291_10BIT_2M_120FPS_NOWDR_ATTR =
{
    .devno      = 0,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = 891,
    .img_rect   = {0, 0, 1920, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_10BIT,
            SC_MIPI_WDR_MODE_NONE,
            {0, 1, 2, 3, -1, -1, -1, -1},
            357,
            MIPI_SETTLE_COUNT0
        }
    },

    .dev_power_attr =
    {
        .reset_gpio = SC_GPIO(-1, -1),
        .power_gpio = SC_GPIO(-1, -1),
        {-1, -1},
        MCLK_SRC_SENSOR0,
        SNS_MCLK_37_125MHZ,
    },

    .line_timing =
    {
        1,
        0x95BE,
        0xB91,
        0x172,
    }
};

static const SC_COMBO_DEV_ATTR_S MIPI_2lane_CHN0_SENSOR_SC2310_12BIT_2M_NOWDR_ATTR =
{
    .devno      = 0,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = 445,
    .img_rect   = {0, 0, 1920, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_12BIT,
            SC_MIPI_WDR_MODE_NONE,
            {0, 1, -1, -1, -1, -1, -1, -1},
            75,
            MIPI_SETTLE_COUNT0
        }
    },

    .dev_power_attr =
    {
        .reset_gpio = SC_GPIO(-1, -1),
        .power_gpio = SC_GPIO(-1, -1),
        {-1, -1},
        SENSOR_MCLK_SRC4,
        SNS_MCLK_27MHZ,
        SENSOR_MCLK2,
        SNS_MCLK_27MHZ,
    },
};

static const SC_COMBO_DEV_ATTR_S MIPI_2lane_CHN0_SENSOR_SC2310_10BIT_2M_WDR2to1_ATTR =
{
    .devno      = 0,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = 742,
    .img_rect   = {4, 4, 1920, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_10BIT,
            SC_MIPI_WDR_MODE_VC,
            {0, 1, -1, -1, -1, -1, -1, -1},
            75,
            MIPI_HDR_SETTLE_COUNT
        }
    },

    .dev_power_attr =
    {
        .reset_gpio = SC_GPIO(-1, -1),
        .power_gpio = SC_GPIO(-1, -1),
        {-1, -1},
        SENSOR_MCLK_SRC4,
        SNS_MCLK_27MHZ,
        SENSOR_MCLK2,
        SNS_MCLK_27MHZ,
    },
};

static const SC_COMBO_DEV_ATTR_S MIPI_2lane_CHN1_SENSOR_SC2310_12BIT_2M_NOWDR_ATTR =
{
    .devno      = 1,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = 445,
    .img_rect   = {0, 0, 1920, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_12BIT,
            SC_MIPI_WDR_MODE_NONE,
            {0, 1, -1, -1, -1, -1, -1, -1},
            75,
            MIPI_SETTLE_COUNT0
        }
    },

    .dev_power_attr =
    {
        .reset_gpio = SC_GPIO(-1, -1),
        .power_gpio = SC_GPIO(-1, -1),
        {-1, -1},
        SENSOR_MCLK_SRC5,
        SNS_MCLK_27MHZ,
        SENSOR_MCLK2,
        SNS_MCLK_27MHZ,
    },
};

static const SC_COMBO_DEV_ATTR_S MIPI_2lane_CHN2_SENSOR_SC2310_12BIT_2M_NOWDR_ATTR =
{
    .devno      = 2,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = 445,
    .img_rect   = {0, 0, 1920, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_12BIT,
            SC_MIPI_WDR_MODE_NONE,
            {0, 1, -1, -1, -1, -1, -1, -1},
            75,
            0x18
        }
    },

    .dev_power_attr =
    {
        .reset_gpio = SC_GPIO(-1, -1),
        .power_gpio = SC_GPIO(-1, -1),
        {-1, -1},
        MCLK_SRC_SENSOR2,
        SNS_MCLK_27MHZ,
    },
};

static const SC_COMBO_DEV_ATTR_S MIPI_2lane_CHN3_SENSOR_SC2310_12BIT_2M_NOWDR_ATTR =
{
    .devno      = 3,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = 445,
    .img_rect   = {0, 0, 1920, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_12BIT,
            SC_MIPI_WDR_MODE_NONE,
            {0, 1, -1, -1, -1, -1, -1, -1},
            75,
            0x18
        }
    },

    .dev_power_attr =
    {
        .reset_gpio = SC_GPIO(-1, -1),
        .power_gpio = SC_GPIO(-1, -1),
        {-1, -1},
        MCLK_SRC_SENSOR3,
        SNS_MCLK_27MHZ,
    },
};

static const SC_COMBO_DEV_ATTR_S MIPI_2lane_CHN0_SENSOR_SC2210_12BIT_2M_NOWDR_ATTR =
{
    .devno      = 0,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = 445,
    .img_rect   = {0, 0, 1920, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_12BIT,
            SC_MIPI_WDR_MODE_NONE,
            {0, 1, -1, -1, -1, -1, -1, -1},
            75,
            MIPI_SETTLE_COUNT0
        }
    },

    .dev_power_attr =
    {
        .reset_gpio = SC_GPIO(-1, -1),
        .power_gpio = SC_GPIO(-1, -1),
        {-1, -1},
        SENSOR_MCLK_SRC4,
        SNS_MCLK_27MHZ,
        SENSOR_MCLK2,
        SNS_MCLK_27MHZ,
    },
};

static const SC_COMBO_DEV_ATTR_S MIPI_2lane_CHN2_SENSOR_SC2210_12BIT_2M_NOWDR_ATTR =
{
    .devno      = 2,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = 445,
    .img_rect   = {0, 0, 1920, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_12BIT,
            SC_MIPI_WDR_MODE_NONE,
            {0, 1, -1, -1, -1, -1, -1, -1},
            75,
            MIPI_SETTLE_COUNT0
        }
    },

    .dev_power_attr =
    {
        .reset_gpio = SC_GPIO(-1, -1),
        .power_gpio = SC_GPIO(-1, -1),
        {-1, -1},
        SENSOR_MCLK_SRC5,
        SNS_MCLK_27MHZ,
        SENSOR_MCLK2,
        SNS_MCLK_27MHZ,
    },
};

static const SC_COMBO_DEV_ATTR_S MIPI_2lane_CHN0_SENSOR_SC4210_12BIT_4M_NOWDR_ATTR =
{
    .devno      = 0,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = 864,
    .img_rect   = {0, 0, 2560, 1440},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_12BIT,
            SC_MIPI_WDR_MODE_NONE,
            {0, 1, -1, -1, -1, -1, -1, -1},
            150,
            MIPI_SETTLE_COUNT0
        }
    },

    .dev_power_attr =
    {
        .reset_gpio = SC_GPIO(-1, -1),
        .power_gpio = SC_GPIO(-1, -1),
        {-1, -1},
        SENSOR_MCLK_SRC4,
        SNS_MCLK_24MHZ,
        SENSOR_MCLK2,
        SNS_MCLK_24MHZ,
    },
};

static const SC_COMBO_DEV_ATTR_S MIPI_4lane_CHN0_SENSOR_IMX415_12BIT_8M_NOWDR_ATTR =
{
    .devno      = 0,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = 891,
    .img_rect   = {12, 22, 3840, 2160},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_12BIT,
            SC_MIPI_WDR_MODE_NONE,
            {0, 1, 2, 3, -1, -1, -1, -1},
            400,
            26
        }
    },

    .dev_power_attr =
    {
        .reset_gpio = SC_GPIO(-1, -1),
        .power_gpio = SC_GPIO(-1, -1),
        {-1, -1},
        MCLK_SRC_SENSOR0,
        SNS_MCLK_37_125MHZ,
    },
};

static const SC_COMBO_DEV_ATTR_S MIPI_4lane_CHN2_SENSOR_IMX415_12BIT_8M_NOWDR_ATTR =
{
    .devno      = 2,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = 891,
    .img_rect   = {12, 22, 3840, 2160},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_12BIT,
            SC_MIPI_WDR_MODE_NONE,
            {0, 1, 2, 3, -1, -1, -1, -1},
            400,
            26
        }
    },

    .dev_power_attr =
    {
        .reset_gpio = SC_GPIO(-1, -1),
        .power_gpio = SC_GPIO(-1, -1),
        {-1, -1},
        MCLK_SRC_SENSOR2,
        SNS_MCLK_37_125MHZ,
    },
};


static const SC_COMBO_DEV_ATTR_S MIPI_4lane_CHN0_SENSOR_IMX415_12BIT_8M_60FPS_NOWDR_ATTR =
{
    .devno      = 0,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = 1782,
    .img_rect   = {12, 22, 3840, 2160},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_12BIT,
            SC_MIPI_WDR_MODE_NONE,
            {0, 1, 2, 3, -1, -1, -1, -1},
            400,
            35
        }
    },

    .dev_power_attr =
    {
        .reset_gpio = SC_GPIO(-1, -1),
        .power_gpio = SC_GPIO(-1, -1),
        {-1, -1},
        MCLK_SRC_SENSOR0,
        SNS_MCLK_37_125MHZ,
    },
};

static const SC_COMBO_DEV_ATTR_S MIPI_4lane_CHN0_SENSOR_IMX415_12BIT_8M_WDR2to1_ATTR =
{
    .devno      = 0,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = 891,
    .img_rect   = {0, 0, 3840, 2160},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_12BIT,
            SC_MIPI_WDR_MODE_DOL,
            {0, 1, 2, 3, -1, -1, -1, -1},
            400,
            35
        }
    },

    .dev_power_attr =
    {
        .reset_gpio = SC_GPIO(-1, -1),
        .power_gpio = SC_GPIO(-1, -1),
        {-1, -1},
        MCLK_SRC_SENSOR0,
        SNS_MCLK_37_125MHZ,
    },
};

static const SC_COMBO_DEV_ATTR_S MIPI_2lane_CHN0_SENSOR_GC2053_10BIT_2M_NOWDR_ATTR =
{
    .devno      = 0,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = 594,
    .img_rect   = {0, 0, 1920, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_10BIT,
            SC_MIPI_WDR_MODE_NONE,
            {0, 1, -1, -1, -1, -1, -1, -1},
            75,
            MIPI_SETTLE_COUNT1
        }
    },

    .dev_power_attr =
    {
        .reset_gpio = SC_GPIO(-1, -1),
        .power_gpio = SC_GPIO(-1, -1),
        {-1, -1},
        SENSOR_MCLK_SRC4,
        SNS_MCLK_27MHZ,
        SENSOR_MCLK2,
        SNS_MCLK_27MHZ,
    },
};

static const SC_COMBO_DEV_ATTR_S MIPI_2lane_CHN1_SENSOR_GC2053_10BIT_2M_NOWDR_ATTR =
{
    .devno      = 1,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = 594,
    .img_rect   = {0, 0, 1920, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_10BIT,
            SC_MIPI_WDR_MODE_NONE,
            {0, 1, -1, -1, -1, -1, -1, -1},
            75,
            MIPI_SETTLE_COUNT1
        }
    },

    .dev_power_attr =
    {
        .reset_gpio = SC_GPIO(-1, -1),
        .power_gpio = SC_GPIO(-1, -1),
        {-1, -1},
        SENSOR_MCLK_SRC5,
        SNS_MCLK_27MHZ,
        SENSOR_MCLK2,
        SNS_MCLK_27MHZ,
    },
};

static const SC_COMBO_DEV_ATTR_S MIPI_2lane_CHN2_SENSOR_GC2053_10BIT_2M_NOWDR_ATTR =
{
    .devno      = 2,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = 594,
    .img_rect   = {0, 0, 1920, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_10BIT,
            SC_MIPI_WDR_MODE_NONE,
            {0, 1, -1, -1, -1, -1, -1, -1},
            75,
            28
        }
    },

    .dev_power_attr =
    {
        .reset_gpio = SC_GPIO(-1, -1),
        .power_gpio = SC_GPIO(-1, -1),
        {-1, -1},
        MCLK_SRC_SENSOR2,
        SNS_MCLK_27MHZ,
    },
};

static const SC_COMBO_DEV_ATTR_S MIPI_2lane_CHN3_SENSOR_GC2053_10BIT_2M_NOWDR_ATTR =
{
    .devno      = 3,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = 594,
    .img_rect   = {0, 0, 1920, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_10BIT,
            SC_MIPI_WDR_MODE_NONE,
            {0, 1, -1, -1, -1, -1, -1, -1},
            75,
            28
        }
    },

    .dev_power_attr =
    {
        .reset_gpio = SC_GPIO(-1, -1),
        .power_gpio = SC_GPIO(-1, -1),
        {-1, -1},
        MCLK_SRC_SENSOR3,
        SNS_MCLK_27MHZ,
    },
};

static const SC_COMBO_DEV_ATTR_S MIPI_2lane_CHN0_SENSOR_GC2093_10BIT_2M_NOWDR_ATTR =
{
    .devno      = 0,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = 792,
    .img_rect   = {0, 0, 1920, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_10BIT,
            SC_MIPI_WDR_MODE_NONE,
            {0, 1, -1, -1, -1, -1, -1, -1},
            75,
            MIPI_SETTLE_COUNT0
        }
    },

    .dev_power_attr =
    {
        .reset_gpio = SC_GPIO(-1, -1),
        .power_gpio = SC_GPIO(-1, -1),
        {-1, -1},
        SENSOR_MCLK_SRC4,
        SNS_MCLK_24MHZ,
        SENSOR_MCLK2,
        SNS_MCLK_24MHZ,
    },
};

static const SC_COMBO_DEV_ATTR_S MIPI_2lane_CHN1_SENSOR_GC2093_10BIT_2M_NOWDR_ATTR =
{
    .devno      = 1,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = 792,
    .img_rect   = {0, 0, 1920, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_10BIT,
            SC_MIPI_WDR_MODE_NONE,
            {0, 1, -1, -1, -1, -1, -1, -1},
            75,
            MIPI_SETTLE_COUNT0
        }
    },

    .dev_power_attr =
    {
        .reset_gpio = SC_GPIO(-1, -1),
        .power_gpio = SC_GPIO(-1, -1),
        {-1, -1},
        SENSOR_MCLK_SRC5,
        SNS_MCLK_24MHZ,
        SENSOR_MCLK2,
        SNS_MCLK_24MHZ,
    },
};

static const SC_COMBO_DEV_ATTR_S MIPI_2lane_CHN2_SENSOR_GC2093_10BIT_2M_NOWDR_ATTR =
{
    .devno      = 2,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = 792,
    .img_rect   = {0, 0, 1920, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_10BIT,
            SC_MIPI_WDR_MODE_NONE,
            {0, 1, -1, -1, -1, -1, -1, -1},
            75,
            21
        }
    },

    .dev_power_attr =
    {
        .reset_gpio = SC_GPIO(-1, -1),
        .power_gpio = SC_GPIO(-1, -1),
        {-1, -1},
        MCLK_SRC_SENSOR2,
        SNS_MCLK_24MHZ,
    },
};

static const SC_COMBO_DEV_ATTR_S MIPI_2lane_CHN3_SENSOR_GC2093_10BIT_2M_NOWDR_ATTR =
{
    .devno      = 3,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = 792,
    .img_rect   = {0, 0, 1920, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_10BIT,
            SC_MIPI_WDR_MODE_NONE,
            {0, 1, -1, -1, -1, -1, -1, -1},
            75,
            21
        }
    },

    .dev_power_attr =
    {
        .reset_gpio = SC_GPIO(-1, -1),
        .power_gpio = SC_GPIO(-1, -1),
        {-1, -1},
        MCLK_SRC_SENSOR3,
        SNS_MCLK_24MHZ,
    },
};

static const SC_COMBO_DEV_ATTR_S MIPI_2lane_CHN0_SENSOR_GC2093_10BIT_2M_WDR2to1_ATTR =
{
    .devno      = 0,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = 792,
    .img_rect   = {0, 0, 1922, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_10BIT,
            SC_MIPI_WDR_MODE_VC,
            {0, 1, -1, -1, -1, -1, -1, -1},
            198,
            MIPI_SETTLE_COUNT0
        }
    },

    .dev_power_attr =
    {
        .reset_gpio = SC_GPIO(-1, -1),
        .power_gpio = SC_GPIO(-1, -1),
        {-1, -1},
        SENSOR_MCLK_SRC4,
        SNS_MCLK_24MHZ,
        SENSOR_MCLK2,
        SNS_MCLK_24MHZ,
    },
};

static const SC_COMBO_DEV_ATTR_S MIPI_2lane_CHN0_SENSOR_GC1603_10BIT_1M_60FPS_NOWDR_ATTR =
{
    .devno      = 0,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = 504,
    .img_rect   = {0, 0, 1280, 720},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_10BIT,
            SC_MIPI_WDR_MODE_NONE,
            {0, 1, -1, -1, -1, -1, -1, -1},
            108,
            MIPI_SETTLE_COUNT0
        }
    },

    .dev_power_attr =
    {
        .reset_gpio = SC_GPIO(-1, -1),
        .power_gpio = SC_GPIO(-1, -1),
        {-1, -1},
        SENSOR_MCLK_SRC4,
        SNS_MCLK_27MHZ,
        SENSOR_MCLK2,
        SNS_MCLK_27MHZ,
    },
};

static const SC_COMBO_DEV_ATTR_S MIPI_2lane_CHN0_SENSOR_GC1603_10BIT_1M_120FPS_NOWDR_ATTR =
{
    .devno      = 0,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = 630,
    .img_rect   = {0, 0, 1280, 720},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_10BIT,
            SC_MIPI_WDR_MODE_NONE,
            {0, 1, -1, -1, -1, -1, -1, -1},
            140,
            MIPI_SETTLE_COUNT0
        }
    },

    .dev_power_attr =
    {
        .reset_gpio = SC_GPIO(-1, -1),
        .power_gpio = SC_GPIO(-1, -1),
        {-1, -1},
        SENSOR_MCLK_SRC4,
        SNS_MCLK_27MHZ,
        SENSOR_MCLK2,
        SNS_MCLK_27MHZ,
    },
};

static const SC_COMBO_DEV_ATTR_S MIPI_2lane_CHN0_SENSOR_OV13B10_10BIT_13M_NOWDR_ATTR =
{
    .devno      = 0,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = 560,
    .img_rect   = {0, 0, 4096, 3120},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_10BIT,
            SC_MIPI_WDR_MODE_NONE,
            {0, 1, -1, -1, -1, -1, -1, -1},
            200,
            0x14
        }
    },

    .dev_power_attr =
    {
        .reset_gpio = SC_GPIO(-1, -1),
        .power_gpio = SC_GPIO(-1, -1),
        {-1, -1},
        SENSOR_MCLK_SRC0,
        SNS_MCLK_24MHZ,
        SENSOR_MCLK2,
        SNS_MCLK_24MHZ,
    },
};

static const SC_COMBO_DEV_ATTR_S MIPI_4lane_CHN0_SENSOR_OV13B10_10BIT_2M_NOWDR_ATTR =
{
    .devno      = 0,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = 560,
    .img_rect   = {0, 0, 1920, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_10BIT,
            SC_MIPI_WDR_MODE_NONE,
            {0, 1, 2, 3, -1, -1, -1, -1},
            75,
            0x16
        }
    },

    .dev_power_attr =
    {
        .reset_gpio = SC_GPIO(-1, -1),
        .power_gpio = SC_GPIO(-1, -1),
        {-1, -1},
        SENSOR_MCLK_SRC0,
        SNS_MCLK_24MHZ,
        SENSOR_MCLK2,
        SNS_MCLK_24MHZ,
    },

    .line_timing =
    {
        1,
        0x177B7,
        0x1075,
        0x20E,
    }
};

static const SC_COMBO_DEV_ATTR_S MIPI_4lane_CHN0_SENSOR_ISPVIN0_12BIT_2M_NOWDR_ATTR =
{
    .devno = 0,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate = 450,
    .img_rect = {0, 0, 1920, 1080},

    {
        .mipi_attr =
        {
            DATA_TYPE_RAW_12BIT,
            SC_MIPI_WDR_MODE_NONE,
            {0, 1, 2, 3, -1, -1, -1, -1},
            75,
            0xd
        }
    }
};

static const SC_COMBO_DEV_ATTR_S DVP_4VC_TP9930_1080P_ATTR =
{
    .devno = 0,
    .input_mode = INPUT_MODE_BT1120,
    .img_rect = {0, 0, 1920, 1080},
};

#if 0
#endif
static const VI_DEV_ATTR_S DEV_ATTR_IMX307_2M_BASE =
{
    VI_MODE_MIPI,
    VI_WORK_MODE_1Multiplex,
    {0xFFF00000,    0x0},
    VI_SCAN_PROGRESSIVE,
    {-1, -1, -1, -1},
    VI_DATA_SEQ_YUYV,

    {
        /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
        VI_VSYNC_PULSE, VI_VSYNC_NEG_LOW, VI_HSYNC_VALID_SINGNAL, VI_HSYNC_NEG_HIGH, VI_VSYNC_VALID_SINGAL, VI_VSYNC_VALID_NEG_HIGH,

        /*hsync_hfb    hsync_act    hsync_hhb*/
        {
            0,            1280,        0,
            /*vsync0_vhb vsync0_act vsync0_hhb*/
            0,            720,        0,
            /*vsync1_vhb vsync1_act vsync1_hhb*/
            0,            0,            0
        }
    },
    VI_DATA_TYPE_RGB,
    SC_FALSE,
    {1920, 1080},
    {
        {
            {1920, 1080},

        },
        {
            VI_REPHASE_MODE_NONE,
            VI_REPHASE_MODE_NONE
        }
    },
    {
        WDR_MODE_NONE,
        1080
    },
    DATA_RATE_X1,
    {DEV_FRE, DEV_FRE, DEV_FRE}
};

static const VI_DEV_ATTR_S DEV_ATTR_IMX307_2M_WDR2TO1_BASE =
{
    VI_MODE_MIPI,
    VI_WORK_MODE_1Multiplex,
    {0xFFC00000,    0x0},
    VI_SCAN_PROGRESSIVE,
    {-1, -1, -1, -1},
    VI_DATA_SEQ_YUYV,

    {
        /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
        VI_VSYNC_PULSE, VI_VSYNC_NEG_LOW, VI_HSYNC_VALID_SINGNAL, VI_HSYNC_NEG_HIGH, VI_VSYNC_VALID_SINGAL, VI_VSYNC_VALID_NEG_HIGH,

        /*hsync_hfb    hsync_act    hsync_hhb*/
        {
            0,            1280,        0,
            /*vsync0_vhb vsync0_act vsync0_hhb*/
            0,            720,        0,
            /*vsync1_vhb vsync1_act vsync1_hhb*/
            0,            0,            0
        }
    },
    VI_DATA_TYPE_RGB,
    SC_FALSE,
    {1920, 1080},
    {
        {
            {1920, 1080},

        },
        {
            VI_REPHASE_MODE_NONE,
            VI_REPHASE_MODE_NONE
        }
    },
    {
        WDR_MODE_2To1_LINE,
        1080
    },
    DATA_RATE_X1,
    {75000000, 75000000, 75000000}
};

static const VI_DEV_ATTR_S DEV_ATTR_IMX291_2M_BASE_12BIT =
{
    VI_MODE_MIPI,
    VI_WORK_MODE_1Multiplex,
    {0xFFF00000,    0x0},
    VI_SCAN_PROGRESSIVE,
    {-1, -1, -1, -1},
    VI_DATA_SEQ_YUYV,

    {
        /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
        VI_VSYNC_PULSE, VI_VSYNC_NEG_LOW, VI_HSYNC_VALID_SINGNAL, VI_HSYNC_NEG_HIGH, VI_VSYNC_VALID_SINGAL, VI_VSYNC_VALID_NEG_HIGH,

        /*hsync_hfb    hsync_act    hsync_hhb*/
        {
            0,            1280,        0,
            /*vsync0_vhb vsync0_act vsync0_hhb*/
            0,            720,        0,
            /*vsync1_vhb vsync1_act vsync1_hhb*/
            0,            0,            0
        }
    },
    VI_DATA_TYPE_RGB,
    SC_FALSE,
    {1920, 1080},
    {
        {
            {1920, 1080},

        },
        {
            VI_REPHASE_MODE_NONE,
            VI_REPHASE_MODE_NONE
        }
    },
    {
        WDR_MODE_NONE,
        1080
    },
    DATA_RATE_X1,
    {DEV_FRE, DEV_FRE, DEV_FRE}
};

static const VI_DEV_ATTR_S DEV_ATTR_IMX291_2M_BASE_10BIT =
{
    VI_MODE_MIPI,
    VI_WORK_MODE_1Multiplex,
    {0xFFC00000,    0x0},
    VI_SCAN_PROGRESSIVE,
    {-1, -1, -1, -1},
    VI_DATA_SEQ_YUYV,

    {
        /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
        VI_VSYNC_PULSE, VI_VSYNC_NEG_LOW, VI_HSYNC_VALID_SINGNAL, VI_HSYNC_NEG_HIGH, VI_VSYNC_VALID_SINGAL, VI_VSYNC_VALID_NEG_HIGH,

        /*hsync_hfb    hsync_act    hsync_hhb*/
        {
            0,            1280,        0,
            /*vsync0_vhb vsync0_act vsync0_hhb*/
            0,            720,        0,
            /*vsync1_vhb vsync1_act vsync1_hhb*/
            0,            0,            0
        }
    },
    VI_DATA_TYPE_RGB,
    SC_FALSE,
    {1920, 1080},
    {
        {
            {1920, 1080},

        },
        {
            VI_REPHASE_MODE_NONE,
            VI_REPHASE_MODE_NONE
        }
    },
    {
        WDR_MODE_NONE,
        1080
    },
    DATA_RATE_X1,
    {DEV_FRE, DEV_FRE, DEV_FRE}
};

static const VI_DEV_ATTR_S DEV_ATTR_SC2310_2M_BASE =
{
    VI_MODE_MIPI,
    VI_WORK_MODE_1Multiplex,
    {0xFFF00000,    0x0},
    VI_SCAN_PROGRESSIVE,
    {-1, -1, -1, -1},
    VI_DATA_SEQ_YUYV,

    {
        /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
        VI_VSYNC_PULSE, VI_VSYNC_NEG_LOW, VI_HSYNC_VALID_SINGNAL, VI_HSYNC_NEG_HIGH, VI_VSYNC_VALID_SINGAL, VI_VSYNC_VALID_NEG_HIGH,

        /*hsync_hfb    hsync_act    hsync_hhb*/
        {
            0,            1280,        0,
            /*vsync0_vhb vsync0_act vsync0_hhb*/
            0,            720,        0,
            /*vsync1_vhb vsync1_act vsync1_hhb*/
            0,            0,            0
        }
    },
    VI_DATA_TYPE_RGB,
    SC_FALSE,
    {1920, 1080},
    {
        {
            {1920, 1080},

        },
        {
            VI_REPHASE_MODE_NONE,
            VI_REPHASE_MODE_NONE
        }
    },
    {
        WDR_MODE_NONE,
        1080
    },
    DATA_RATE_X1,
    {DEV_FRE, DEV_FRE, DEV_FRE}
};

static const VI_DEV_ATTR_S DEV_ATTR_SC2310_2M_WDR2TO1_BASE =
{
    VI_MODE_MIPI,
    VI_WORK_MODE_1Multiplex,
    {0xFFC00000,    0x0},
    VI_SCAN_PROGRESSIVE,
    {-1, -1, -1, -1},
    VI_DATA_SEQ_YUYV,

    {
        /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
        VI_VSYNC_PULSE, VI_VSYNC_NEG_LOW, VI_HSYNC_VALID_SINGNAL, VI_HSYNC_NEG_HIGH, VI_VSYNC_VALID_SINGAL, VI_VSYNC_VALID_NEG_HIGH,

        /*hsync_hfb    hsync_act    hsync_hhb*/
        {
            0,            1280,        0,
            /*vsync0_vhb vsync0_act vsync0_hhb*/
            0,            720,        0,
            /*vsync1_vhb vsync1_act vsync1_hhb*/
            0,            0,            0
        }
    },
    VI_DATA_TYPE_RGB,
    SC_FALSE,
    {1920, 1080},
    {
        {
            {1920, 1080},

        },
        {
            VI_REPHASE_MODE_NONE,
            VI_REPHASE_MODE_NONE
        }
    },
    {
        WDR_MODE_2To1_LINE,
        1080
    },
    DATA_RATE_X1,
    {75000000, 75000000, 75000000}
};

static const VI_DEV_ATTR_S DEV_ATTR_SC2210_2M_BASE =
{
    VI_MODE_MIPI,
    VI_WORK_MODE_1Multiplex,
    {0xFFF00000,    0x0},
    VI_SCAN_PROGRESSIVE,
    {-1, -1, -1, -1},
    VI_DATA_SEQ_YUYV,

    {
        /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
        VI_VSYNC_PULSE, VI_VSYNC_NEG_LOW, VI_HSYNC_VALID_SINGNAL, VI_HSYNC_NEG_HIGH, VI_VSYNC_VALID_SINGAL, VI_VSYNC_VALID_NEG_HIGH,

        /*hsync_hfb    hsync_act    hsync_hhb*/
        {
            0,            1280,        0,
            /*vsync0_vhb vsync0_act vsync0_hhb*/
            0,            720,        0,
            /*vsync1_vhb vsync1_act vsync1_hhb*/
            0,            0,            0
        }
    },
    VI_DATA_TYPE_RGB,
    SC_FALSE,
    {1920, 1080},
    {
        {
            {1920, 1080},

        },
        {
            VI_REPHASE_MODE_NONE,
            VI_REPHASE_MODE_NONE
        }
    },
    {
        WDR_MODE_NONE,
        1080
    },
    DATA_RATE_X1,
    {DEV_FRE, DEV_FRE, DEV_FRE}
};

static const VI_DEV_ATTR_S DEV_ATTR_SC4210_4M_BASE =
{
    VI_MODE_MIPI,
    VI_WORK_MODE_1Multiplex,
    {0xFFF00000,    0x0},
    VI_SCAN_PROGRESSIVE,
    {-1, -1, -1, -1},
    VI_DATA_SEQ_YUYV,

    {
        /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
        VI_VSYNC_PULSE, VI_VSYNC_NEG_LOW, VI_HSYNC_VALID_SINGNAL, VI_HSYNC_NEG_HIGH, VI_VSYNC_VALID_SINGAL, VI_VSYNC_VALID_NEG_HIGH,

        /*hsync_hfb    hsync_act    hsync_hhb*/
        {
            0,            1280,        0,
            /*vsync0_vhb vsync0_act vsync0_hhb*/
            0,            720,        0,
            /*vsync1_vhb vsync1_act vsync1_hhb*/
            0,            0,            0
        }
    },
    VI_DATA_TYPE_RGB,
    SC_FALSE,
    {2560, 1440},
    {
        {
            {2560, 1440},

        },
        {
            VI_REPHASE_MODE_NONE,
            VI_REPHASE_MODE_NONE
        }
    },
    {
        WDR_MODE_NONE,
        1440
    },
    DATA_RATE_X1,
    {400000000, 400000000, 400000000}
};

static const VI_DEV_ATTR_S DEV_ATTR_IMX415_8M_BASE =
{
    VI_MODE_MIPI,
    VI_WORK_MODE_1Multiplex,
    {0xFFC00000,    0x0},
    VI_SCAN_PROGRESSIVE,
    {-1, -1, -1, -1},
    VI_DATA_SEQ_YUYV,
    {
        /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
        VI_VSYNC_PULSE, VI_VSYNC_NEG_LOW, VI_HSYNC_VALID_SINGNAL, VI_HSYNC_NEG_HIGH, VI_VSYNC_VALID_SINGAL, VI_VSYNC_VALID_NEG_HIGH,

        /*hsync_hfb    hsync_act    hsync_hhb*/
        {
            0,            1280,        0,
            /*vsync0_vhb vsync0_act vsync0_hhb*/
            0,            720,        0,
            /*vsync1_vhb vsync1_act vsync1_hhb*/
            0,            0,            0
        }
    },
    VI_DATA_TYPE_RGB,
    SC_FALSE,
    {3840, 2160},
    {
        {
            {3840, 2160},
        },
        {
            VI_REPHASE_MODE_NONE,
            VI_REPHASE_MODE_NONE
        }
    },
    {
        WDR_MODE_NONE,
        2160
    },
    DATA_RATE_X1,
    {400000000, 400000000, 400000000}
};

static const VI_DEV_ATTR_S DEV_ATTR_IMX415_8M_WDR2TO1_BASE =
{
    VI_MODE_MIPI,
    VI_WORK_MODE_1Multiplex,
    {0xFFF00000,    0x0},
    VI_SCAN_PROGRESSIVE,
    {-1, -1, -1, -1},
    VI_DATA_SEQ_YUYV,
    {
        /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
        VI_VSYNC_PULSE, VI_VSYNC_NEG_LOW, VI_HSYNC_VALID_SINGNAL, VI_HSYNC_NEG_HIGH, VI_VSYNC_VALID_SINGAL, VI_VSYNC_VALID_NEG_HIGH,

        /*hsync_hfb    hsync_act    hsync_hhb*/
        {
            0,            1280,        0,
            /*vsync0_vhb vsync0_act vsync0_hhb*/
            0,            720,        0,
            /*vsync1_vhb vsync1_act vsync1_hhb*/
            0,            0,            0
        }
    },
    VI_DATA_TYPE_RGB,
    SC_FALSE,
    {3840, 2160},
    {
        {
            {3840, 2160},
        },
        {
            VI_REPHASE_MODE_NONE,
            VI_REPHASE_MODE_NONE
        }
    },
    {
        WDR_MODE_2To1_LINE,
        2160
    },
    DATA_RATE_X1,
    {400000000, 400000000, 400000000}
};

static const VI_DEV_ATTR_S DEV_ATTR_GC2053_2M_BASE =
{
    VI_MODE_MIPI,
    VI_WORK_MODE_1Multiplex,
    {0xFFC00000,    0x0},
    VI_SCAN_PROGRESSIVE,
    { -1, -1, -1, -1},
    VI_DATA_SEQ_YUYV,

    {
        /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
        VI_VSYNC_PULSE, VI_VSYNC_NEG_LOW, VI_HSYNC_VALID_SINGNAL, VI_HSYNC_NEG_HIGH, VI_VSYNC_VALID_SINGAL, VI_VSYNC_VALID_NEG_HIGH,

        /*hsync_hfb    hsync_act    hsync_hhb*/
        {
            0,            1280,        0,
            /*vsync0_vhb vsync0_act vsync0_hhb*/
            0,            720,        0,
            /*vsync1_vhb vsync1_act vsync1_hhb*/
            0,            0,            0
        }
    },
    VI_DATA_TYPE_RGB,
    SC_FALSE,
    {1920, 1080},
    {
        {
            {1920, 1080},
        },
        {
            VI_REPHASE_MODE_NONE,
            VI_REPHASE_MODE_NONE
        }
    },
    {
        WDR_MODE_NONE,
        1080
    },
    DATA_RATE_X1,
    {DEV_FRE, DEV_FRE, DEV_FRE}
};

static const VI_DEV_ATTR_S DEV_ATTR_GC2093_2M_BASE =
{
    VI_MODE_MIPI,
    VI_WORK_MODE_1Multiplex,
    {0xFFC00000,    0x0},
    VI_SCAN_PROGRESSIVE,
    { -1, -1, -1, -1},
    VI_DATA_SEQ_YUYV,

    {
        /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
        VI_VSYNC_PULSE, VI_VSYNC_NEG_LOW, VI_HSYNC_VALID_SINGNAL, VI_HSYNC_NEG_HIGH, VI_VSYNC_VALID_SINGAL, VI_VSYNC_VALID_NEG_HIGH,

        /*hsync_hfb    hsync_act    hsync_hhb*/
        {
            0,            1280,        0,
            /*vsync0_vhb vsync0_act vsync0_hhb*/
            0,            720,        0,
            /*vsync1_vhb vsync1_act vsync1_hhb*/
            0,            0,            0
        }
    },
    VI_DATA_TYPE_RGB,
    SC_FALSE,
    {1920, 1080},
    {
        {
            {1920, 1080},
        },
        {
            VI_REPHASE_MODE_NONE,
            VI_REPHASE_MODE_NONE
        }
    },
    {
        WDR_MODE_NONE,
        1080
    },
    DATA_RATE_X1,
    {DEV_FRE, DEV_FRE, DEV_FRE}
};

static const VI_DEV_ATTR_S DEV_ATTR_GC2093_2M_WDR2TO1_BASE =
{
    VI_MODE_MIPI,
    VI_WORK_MODE_1Multiplex,
    {0xFFC00000,    0x0},
    VI_SCAN_PROGRESSIVE,
    { -1, -1, -1, -1},
    VI_DATA_SEQ_YUYV,

    {
        /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
        VI_VSYNC_PULSE, VI_VSYNC_NEG_LOW, VI_HSYNC_VALID_SINGNAL, VI_HSYNC_NEG_HIGH, VI_VSYNC_VALID_SINGAL, VI_VSYNC_VALID_NEG_HIGH,

        /*hsync_hfb    hsync_act    hsync_hhb*/
        {
            0,            1280,        0,
            /*vsync0_vhb vsync0_act vsync0_hhb*/
            0,            720,        0,
            /*vsync1_vhb vsync1_act vsync1_hhb*/
            0,            0,            0
        }
    },
    VI_DATA_TYPE_RGB,
    SC_FALSE,
    {1922, 1080},
    {
        {
            {1922, 1080},
        },
        {
            VI_REPHASE_MODE_NONE,
            VI_REPHASE_MODE_NONE
        }
    },
    {
        WDR_MODE_2To1_LINE,
        1080
    },
    DATA_RATE_X1,
    {400000000, 75000000, 75000000}
};

static const VI_DEV_ATTR_S DEV_ATTR_GC1603_1M_BASE =
{
    VI_MODE_MIPI,
    VI_WORK_MODE_1Multiplex,
    {0xFFC00000,    0x0},
    VI_SCAN_PROGRESSIVE,
    { -1, -1, -1, -1},
    VI_DATA_SEQ_YUYV,

    {
        /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
        VI_VSYNC_PULSE, VI_VSYNC_NEG_LOW, VI_HSYNC_VALID_SINGNAL, VI_HSYNC_NEG_HIGH, VI_VSYNC_VALID_SINGAL, VI_VSYNC_VALID_NEG_HIGH,

        /*hsync_hfb    hsync_act    hsync_hhb*/
        {
            0,            1280,        0,
            /*vsync0_vhb vsync0_act vsync0_hhb*/
            0,            720,        0,
            /*vsync1_vhb vsync1_act vsync1_hhb*/
            0,            0,            0
        }
    },
    VI_DATA_TYPE_RGB,
    SC_FALSE,
    {1280, 720},
    {
        {
            {1280, 720},
        },
        {
            VI_REPHASE_MODE_NONE,
            VI_REPHASE_MODE_NONE
        }
    },
    {
        WDR_MODE_NONE,
        720
    },
    DATA_RATE_X1,
    {DEV_FRE, DEV_FRE, DEV_FRE}
};

static const VI_DEV_ATTR_S DEV_ATTR_OV13B10_13M_BASE =
{
    VI_MODE_MIPI,
    VI_WORK_MODE_1Multiplex,
    {0xFFC00000,    0x0},
    VI_SCAN_PROGRESSIVE,
    {-1, -1, -1, -1},
    VI_DATA_SEQ_YUYV,

    {
        /* port_vsync   port_vsync_neg     port_hsync        port_hsync_neg */
        VI_VSYNC_PULSE, VI_VSYNC_NEG_LOW, VI_HSYNC_VALID_SINGNAL, VI_HSYNC_NEG_HIGH, VI_VSYNC_VALID_SINGAL, VI_VSYNC_VALID_NEG_HIGH,

        {
            /* hsync_hfb  hsync_act  hsync_hhb */
            0,            4208,        0,
            /* vsync0_vhb vsync0_act vsync0_hhb */
            0,            3120,        0,
            /* vsync1_vhb vsync1_act vsync1_hhb */
            0,            0,            0
        }
    },
    VI_DATA_TYPE_RGB,
    SC_FALSE,
    {4096, 3120},
    {
        {
            {4096, 3120},

        },
        {
            VI_REPHASE_MODE_NONE,
            VI_REPHASE_MODE_NONE
        }
    },
    {
        WDR_MODE_NONE,
        3120
    },
    DATA_RATE_X1,
    {DEV_FRE, DEV_FRE, DEV_FRE}
};

static const VI_DEV_ATTR_S DEV_ATTR_OV13B10_2M_BASE =
{
    VI_MODE_MIPI,
    VI_WORK_MODE_1Multiplex,
    {0xFFC00000,    0x0},
    VI_SCAN_PROGRESSIVE,
    {-1, -1, -1, -1},
    VI_DATA_SEQ_YUYV,

    {
        /* port_vsync   port_vsync_neg     port_hsync        port_hsync_neg */
        VI_VSYNC_PULSE, VI_VSYNC_NEG_LOW, VI_HSYNC_VALID_SINGNAL, VI_HSYNC_NEG_HIGH, VI_VSYNC_VALID_SINGAL, VI_VSYNC_VALID_NEG_HIGH,

        {
            /* hsync_hfb  hsync_act  hsync_hhb */
            0,            1280,      0,
            /* vsync0_vhb vsync0_act vsync0_hhb */
            0,            720,       0,
            /* vsync1_vhb vsync1_act vsync1_hhb */
            0,            0,         0
        }
    },
    VI_DATA_TYPE_RGB,
    SC_FALSE,
    {1920, 1080},
    {
        {
            {1920, 1080},

        },
        {
            VI_REPHASE_MODE_NONE,
            VI_REPHASE_MODE_NONE
        }
    },
    {
        WDR_MODE_NONE,
        1080
    },
    DATA_RATE_X1,
    {DEV_FRE, DEV_FRE, DEV_FRE}
};

static const VI_DEV_ATTR_S DEV_ATTR_ISP_VIN_0_2M_BASE =
{
    VI_MODE_MIPI,
    VI_WORK_MODE_1Multiplex,
    {0xFFF00000,    0x0},
    VI_SCAN_PROGRESSIVE,
    {-1, -1, -1, -1},
    VI_DATA_SEQ_YUYV,

    {
        /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
        VI_VSYNC_PULSE, VI_VSYNC_NEG_LOW, VI_HSYNC_VALID_SINGNAL, VI_HSYNC_NEG_HIGH, VI_VSYNC_VALID_SINGAL, VI_VSYNC_VALID_NEG_HIGH,

        /*hsync_hfb    hsync_act    hsync_hhb*/
        {
            0,            1280,        0,
            /*vsync0_vhb vsync0_act vsync0_hhb*/
            0,            720,        0,
            /*vsync1_vhb vsync1_act vsync1_hhb*/
            0,            0,            0
        }
    },
    VI_DATA_TYPE_RGB,
    SC_FALSE,
    {1920, 1080},
    {
        {
            {1920, 1080},

        },
        {
            VI_REPHASE_MODE_NONE,
            VI_REPHASE_MODE_NONE
        }
    },
    {
        WDR_MODE_NONE,
        1080
    },
    DATA_RATE_X1,
    {400000000, 400000000, 400000000}
};

static const VI_DEV_ATTR_S DEV_ATTR_TP9930_1080P_BASE =
{
    .enIntfMode = VI_MODE_BT656,
    .enWorkMode = VI_WORK_MODE_1Multiplex,
    .au32ComponentMask = {0xFF000000, 0x0},
    .enScanMode = VI_SCAN_PROGRESSIVE,
    .as32AdChnId = {-1, -1, -1, -1},
    .enDataSeq = VI_DATA_SEQ_UYVY,
    .enInputDataType = VI_DATA_TYPE_YUV,
    .bDataReverse = SC_FALSE,
    .stSize = {1920, 1080},
    .stBasAttr = {
        .stSacleAttr = {
            .stBasSize = {1920, 1080},
        },
        .stRephaseAttr = {
            .enHRephaseMode = VI_REPHASE_MODE_NONE,
            .enVRephaseMode = VI_REPHASE_MODE_NONE,
        },
    },
    .stWDRAttr = {
        .enWDRMode = WDR_MODE_NONE,
        .u32CacheLine = 1080,
    },
    .enDataRate = DATA_RATE_X1,
    .stDevFre = {
        .u32VifFreHz = 400000000,
        .u32IspFreHz = 400000000,
        .u32HdrFreHz = 400000000
    },
};

#if 0
#endif
static const VI_PIPE_ATTR_S PIPE_ATTR_1920x1080_RAW12_420_3DNR_RFR =
{
    VI_PIPE_BYPASS_NONE, SC_FALSE, SC_FALSE,
    1920, 1080,
    PIXEL_FORMAT_RGB_BAYER_12BPP,
    COMPRESS_MODE_NONE,
    DATA_BITWIDTH_12,
    SC_FALSE,
    {
        PIXEL_FORMAT_YVU_SEMIPLANAR_420,
        DATA_BITWIDTH_8,
        VI_NR_REF_FROM_RFR,
        COMPRESS_MODE_NONE
    },
    SC_FALSE,
    { -1, -1},
    0,
    30
};

static const VI_PIPE_ATTR_S PIPE_ATTR_1920x1080_RAW10_420_3DNR_RFR =
{
    VI_PIPE_BYPASS_NONE, SC_FALSE, SC_FALSE,
    1920, 1080,
    PIXEL_FORMAT_RGB_BAYER_10BPP,
    COMPRESS_MODE_NONE,
    DATA_BITWIDTH_10,
    SC_FALSE,
    {
        PIXEL_FORMAT_YVU_SEMIPLANAR_420,
        DATA_BITWIDTH_8,
        VI_NR_REF_FROM_RFR,
        COMPRESS_MODE_NONE
    },
    SC_FALSE,
    { -1, -1},
    0,
    30
};

static const VI_PIPE_ATTR_S PIPE_ATTR_1920x1080_RAW10_420_3DNR_RFR_60FPS =
{
    VI_PIPE_BYPASS_NONE, SC_FALSE, SC_FALSE,
    1920, 1080,
    PIXEL_FORMAT_RGB_BAYER_10BPP,
    COMPRESS_MODE_NONE,
    DATA_BITWIDTH_10,
    SC_FALSE,
    {
        PIXEL_FORMAT_YVU_SEMIPLANAR_420,
        DATA_BITWIDTH_8,
        VI_NR_REF_FROM_RFR,
        COMPRESS_MODE_NONE
    },
    SC_FALSE,
    { -1, -1},
    0,
    60
};

static const VI_PIPE_ATTR_S PIPE_ATTR_1920x1080_RAW10_420_3DNR_RFR_120FPS =
{
    VI_PIPE_BYPASS_NONE, SC_FALSE, SC_FALSE,
    1920, 1080,
    PIXEL_FORMAT_RGB_BAYER_10BPP,
    COMPRESS_MODE_NONE,
    DATA_BITWIDTH_10,
    SC_FALSE,
    {
        PIXEL_FORMAT_YVU_SEMIPLANAR_420,
        DATA_BITWIDTH_8,
        VI_NR_REF_FROM_RFR,
        COMPRESS_MODE_NONE
    },
    SC_FALSE,
    { -1, -1},
    0,
    120
};

static const VI_PIPE_ATTR_S PIPE_ATTR_1920x1080_RAW12_420_3DNR_RFR_WDR =
{
    VI_PIPE_BYPASS_NONE, SC_FALSE, SC_FALSE,
    1952, 2678,
    PIXEL_FORMAT_RGB_BAYER_12BPP,
    COMPRESS_MODE_NONE,
    DATA_BITWIDTH_12,
    SC_FALSE,
    {
        PIXEL_FORMAT_YVU_SEMIPLANAR_420,
        DATA_BITWIDTH_8,
        VI_NR_REF_FROM_RFR,
        COMPRESS_MODE_NONE
    },
    SC_FALSE,
    { -1, -1},
    0,
    25
};

static const VI_PIPE_ATTR_S PIPE_ATTR_1920x1080_RAW10_420_3DNR_RFR_WDR =
{
    VI_PIPE_BYPASS_NONE, SC_FALSE, SC_FALSE,
    1920, 1080,
    PIXEL_FORMAT_RGB_BAYER_10BPP,
    COMPRESS_MODE_NONE,
    DATA_BITWIDTH_10,
    SC_FALSE,
    {
        PIXEL_FORMAT_YVU_SEMIPLANAR_420,
        DATA_BITWIDTH_8,
        VI_NR_REF_FROM_RFR,
        COMPRESS_MODE_NONE
    },
    SC_FALSE,
    { -1, -1},
    0,
    25
};

static const VI_PIPE_ATTR_S PIPE_ATTR_GC2093_1080P_RAW10_WDR =
{
    VI_PIPE_BYPASS_NONE, SC_FALSE, SC_FALSE,
    1922, 1080,
    PIXEL_FORMAT_RGB_BAYER_10BPP,
    COMPRESS_MODE_NONE,
    DATA_BITWIDTH_10,
    SC_FALSE,
    {
        PIXEL_FORMAT_YVU_SEMIPLANAR_420,
        DATA_BITWIDTH_8,
        VI_NR_REF_FROM_RFR,
        COMPRESS_MODE_NONE
    },
    SC_FALSE,
    { -1, -1},
    0,
    25
};

static const VI_PIPE_ATTR_S PIPE_ATTR_2560x1440_RAW12_420_3DNR_RFR =
{
    VI_PIPE_BYPASS_NONE, SC_FALSE, SC_FALSE,
    2560, 1440,
    PIXEL_FORMAT_RGB_BAYER_12BPP,
    COMPRESS_MODE_NONE,
    DATA_BITWIDTH_12,
    SC_FALSE,
    {
        PIXEL_FORMAT_YVU_SEMIPLANAR_420,
        DATA_BITWIDTH_8,
        VI_NR_REF_FROM_RFR,
        COMPRESS_MODE_NONE
    },
    SC_FALSE,
    { -1, -1},
    0,
    30
};

static const VI_PIPE_ATTR_S PIPE_ATTR_3840x2160_RAW12_420_3DNR_RFR =
{
    VI_PIPE_BYPASS_NONE, SC_FALSE, SC_FALSE,
    3864, 2192,
    PIXEL_FORMAT_RGB_BAYER_12BPP,
    COMPRESS_MODE_NONE,
    DATA_BITWIDTH_12,
    SC_FALSE,
    {
        PIXEL_FORMAT_YVU_PLANAR_420,
        DATA_BITWIDTH_8,
        VI_NR_REF_FROM_RFR,
        COMPRESS_MODE_NONE
    },
    SC_FALSE,
    { -1, -1},
    0,
    30
};

static const VI_PIPE_ATTR_S PIPE_ATTR_3840x2160_RAW12_420_3DNR_RFR_60FPS =
{
    VI_PIPE_BYPASS_NONE, SC_FALSE, SC_FALSE,
    3840, 2160,
    PIXEL_FORMAT_RGB_BAYER_12BPP,
    COMPRESS_MODE_NONE,
    DATA_BITWIDTH_12,
    SC_FALSE,
    {
        PIXEL_FORMAT_YVU_PLANAR_420,
        DATA_BITWIDTH_8,
        VI_NR_REF_FROM_RFR,
        COMPRESS_MODE_NONE
    },
    SC_FALSE,
    { -1, -1},
    0,
    60
};

static const VI_PIPE_ATTR_S PIPE_ATTR_3840x2160_RAW12_420_3DNR_RFR_WDR =
{
    VI_PIPE_BYPASS_NONE, SC_FALSE, SC_FALSE,
    3864, 2192,
    PIXEL_FORMAT_RGB_BAYER_12BPP,
    COMPRESS_MODE_NONE,
    DATA_BITWIDTH_12,
    SC_FALSE,
    {
        PIXEL_FORMAT_YVU_PLANAR_420,
        DATA_BITWIDTH_8,
        VI_NR_REF_FROM_RFR,
        COMPRESS_MODE_NONE
    },
    SC_FALSE,
    { -1, -1},
    0,
    30
};

static const VI_PIPE_ATTR_S PIPE_ATTR_1280x720_RAW10_420_3DNR_RFR_60FPS =
{
    VI_PIPE_BYPASS_NONE, SC_FALSE, SC_FALSE,
    1280, 720,
    PIXEL_FORMAT_RGB_BAYER_10BPP,
    COMPRESS_MODE_NONE,
    DATA_BITWIDTH_10,
    SC_FALSE,
    {
        PIXEL_FORMAT_YVU_SEMIPLANAR_420,
        DATA_BITWIDTH_8,
        VI_NR_REF_FROM_RFR,
        COMPRESS_MODE_NONE
    },
    SC_FALSE,
    { -1, -1},
    0,
    60
};

static const VI_PIPE_ATTR_S PIPE_ATTR_1280x720_RAW10_420_3DNR_RFR_120FPS =
{
    VI_PIPE_BYPASS_NONE, SC_FALSE, SC_FALSE,
    1280, 720,
    PIXEL_FORMAT_RGB_BAYER_10BPP,
    COMPRESS_MODE_NONE,
    DATA_BITWIDTH_10,
    SC_FALSE,
    {
        PIXEL_FORMAT_YVU_SEMIPLANAR_420,
        DATA_BITWIDTH_8,
        VI_NR_REF_FROM_RFR,
        COMPRESS_MODE_NONE
    },
    SC_FALSE,
    { -1, -1},
    0,
    120
};

static const VI_PIPE_ATTR_S PIPE_ATTR_4096x3120_RAW10_420_3DNR_RFR_1FPS =
{
    VI_PIPE_BYPASS_NONE, SC_FALSE, SC_FALSE,
    4096, 3120,
    PIXEL_FORMAT_RGB_BAYER_10BPP,
    COMPRESS_MODE_NONE,
    DATA_BITWIDTH_10,
    SC_FALSE,
    {
        PIXEL_FORMAT_YVU_SEMIPLANAR_420,
        DATA_BITWIDTH_8,
        VI_NR_REF_FROM_RFR,
        COMPRESS_MODE_NONE
    },
    SC_FALSE,
    { -1, -1},
    0,
    1
};

static const VI_PIPE_ATTR_S PIPE_ATTR_1080P_YUV422 =
{
    .enPipeBypassMode = VI_PIPE_BYPASS_NONE,
    .bYuvSkip = SC_FALSE,
    .bIspBypass = SC_FALSE,
    .u32MaxW = 1920,
    .u32MaxH = 1080,
    .enPixFmt = PIXEL_FORMAT_UYVY_PACKAGE_422,
    .enCompressMode = COMPRESS_MODE_NONE,
    .enBitWidth = DATA_BITWIDTH_8,
    .bNrEn = SC_FALSE,
    .stNrAttr = {
        .enPixFmt = PIXEL_FORMAT_YVU_SEMIPLANAR_420,
        .enBitWidth = DATA_BITWIDTH_8,
        .enNrRefSource = VI_NR_REF_FROM_RFR,
        .enCompressMode = COMPRESS_MODE_NONE,
    },
    .bSharpenEn = SC_FALSE,
    .stFrameRate = {
        .s32SrcFrameRate = 25,
        .s32DstFrameRate = 25,
    },
    .bDiscardProPic = SC_FALSE,
    .u32MaxFrameRate = 25,
};

#if 0
#endif
static const VI_CHN_ATTR_S CHN_ATTR_1920x1080_420_SDR8_LINEAR =
{
    {1920, 1080},
    PIXEL_FORMAT_YVU_PLANAR_420,
    DYNAMIC_RANGE_SDR8,
    VIDEO_FORMAT_LINEAR,
    COMPRESS_MODE_NONE,
    0,      0,
    0,
    { -1, -1},
    5
};

static const VI_CHN_ATTR_S CHN_ATTR_1920x1080_420_SDR8_HDR =
{
    {1920, 1080},
    PIXEL_FORMAT_YVU_PLANAR_420,
    DYNAMIC_RANGE_SDR8,
    VIDEO_FORMAT_LINEAR,
    COMPRESS_MODE_NONE,
    0,      0,
    5,
    { -1, -1},
    10
};

static const VI_CHN_ATTR_S CHN_ATTR_2560x1440_420_SDR8_LINEAR =
{
    {2560, 1440},
    PIXEL_FORMAT_YVU_PLANAR_420,
    DYNAMIC_RANGE_SDR8,
    VIDEO_FORMAT_LINEAR,
    COMPRESS_MODE_NONE,
    0,      0,
    0,
    { -1, -1},
    5
};

static const VI_CHN_ATTR_S CHN_ATTR_3840x2160_420_SDR8_LINEAR =
{
    {3840, 2160},
    PIXEL_FORMAT_YVU_PLANAR_420,
    DYNAMIC_RANGE_SDR8,
    VIDEO_FORMAT_LINEAR,
    COMPRESS_MODE_NONE,
    0,      0,
    0,
    { -1, -1},
    4
};

static const VI_CHN_ATTR_S CHN_ATTR_3840x2160_420_SDR8_HDR =
{
    {3840, 2160},
    PIXEL_FORMAT_YVU_PLANAR_420,
    DYNAMIC_RANGE_SDR8,
    VIDEO_FORMAT_LINEAR,
    COMPRESS_MODE_NONE,
    0,      0,
    0,
    { -1, -1},
    4
};

static const VI_CHN_ATTR_S CHN_ATTR_1280x720_420_SDR8_LINEAR =
{
    {1280, 720},
    PIXEL_FORMAT_YVU_PLANAR_420,
    DYNAMIC_RANGE_SDR8,
    VIDEO_FORMAT_LINEAR,
    COMPRESS_MODE_NONE,
    0,      0,
    0,
    { -1, -1},
    10
};

static const VI_CHN_ATTR_S CHN_ATTR_4096x3120_420_SDR8_LINEAR =
{
    {4096, 3120},
    PIXEL_FORMAT_YVU_PLANAR_420,
    DYNAMIC_RANGE_SDR8,
    VIDEO_FORMAT_LINEAR,
    COMPRESS_MODE_NONE,
    0,      0,
    0,
    { -1, -1},
    5
};

static const VI_CHN_ATTR_S CHN_ATTR_1080P_YUV420_SDR8_LINEAR =
{
    .stSize = {1920, 1080},
    .enPixelFormat = PIXEL_FORMAT_YVU_PLANAR_420,
    .enDynamicRange = DYNAMIC_RANGE_SDR8,
    .enVideoFormat = VIDEO_FORMAT_LINEAR,
    .enCompressMode = COMPRESS_MODE_NONE,
    .bMirror = 0,
    .bFlip = 0,
    .u32Depth = 0,
    .stFrameRate = {
        .s32SrcFrameRate = -1,
        .s32DstFrameRate = -1,
    },
    .u32BufCount = 5,
};

SC_BOOL IsSensorInput(SAMPLE_SNS_TYPE_E enSnsType)
{
    SC_BOOL bRet = SC_TRUE;

    switch (enSnsType)
    {
    case SAMPLE_SNS_TYPE_BUTT:
        bRet = SC_FALSE;
        break;

    default:
        break;
    }

    return bRet;
}

SC_S32 SAMPLE_COMM_VI_GetComboAttrBySns(SAMPLE_SNS_TYPE_E enSnsType, combo_dev_t MipiDev,
    SC_COMBO_DEV_ATTR_S *pstComboAttr)
{
    switch (enSnsType)
    {
    case SONY_IMX307_MIPI_2M_30FPS_12BIT:
    case SONY_IMX291_MIPI_2M_30FPS_12BIT:
        if (0 == MipiDev)
        {
            memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_2lane_CHN0_SENSOR_IMX307_12BIT_2M_NOWDR_ATTR,
                sizeof(SC_COMBO_DEV_ATTR_S));
        }
        else if (1 == MipiDev)
        {
            memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_2lane_CHN1_SENSOR_IMX307_12BIT_2M_NOWDR_ATTR,
                sizeof(SC_COMBO_DEV_ATTR_S));
        }
        else if (2 == MipiDev)
        {
            memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_2lane_CHN2_SENSOR_IMX307_12BIT_2M_NOWDR_ATTR,
                sizeof(SC_COMBO_DEV_ATTR_S));
        }
        else if (3 == MipiDev)
        {
            memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_2lane_CHN3_SENSOR_IMX307_12BIT_2M_NOWDR_ATTR,
                sizeof(SC_COMBO_DEV_ATTR_S));
        }
        else
        {
            memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_2lane_CHN0_SENSOR_IMX307_12BIT_2M_NOWDR_ATTR,
                sizeof(SC_COMBO_DEV_ATTR_S));
        }

        break;

    case SONY_IMX291_MIPI_2M_60FPS_10BIT:
        memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_4lane_CHN0_SENSOR_IMX291_10BIT_2M_60FPS_NOWDR_ATTR,
            sizeof(SC_COMBO_DEV_ATTR_S));
        break;

    case SONY_IMX291_MIPI_2M_120FPS_10BIT:
        memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_4lane_CHN0_SENSOR_IMX291_10BIT_2M_120FPS_NOWDR_ATTR,
            sizeof(SC_COMBO_DEV_ATTR_S));
        break;

    case SONY_IMX307_MIPI_2M_30FPS_12BIT_WDR2TO1:
        memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_2lane_CHN0_SENSOR_IMX307_12BIT_2M_WDR2to1_ATTR,
            sizeof(SC_COMBO_DEV_ATTR_S));
        break;

    case SMART_SC2310_MIPI_2M_30FPS_12BIT:
        if (0 == MipiDev)
        {
            memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_2lane_CHN0_SENSOR_SC2310_12BIT_2M_NOWDR_ATTR,
                sizeof(SC_COMBO_DEV_ATTR_S));
        }
        else
        {
            memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_2lane_CHN1_SENSOR_SC2310_12BIT_2M_NOWDR_ATTR,
                sizeof(SC_COMBO_DEV_ATTR_S));
        }

        break;

    case SMART_SC2310_MIPI_2M_30FPS_12BIT_WDR2TO1:
        memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_2lane_CHN0_SENSOR_SC2310_10BIT_2M_WDR2to1_ATTR,
            sizeof(SC_COMBO_DEV_ATTR_S));
        break;

    case SMART_SC2210_MIPI_2M_30FPS_12BIT:
        if (0 == MipiDev)
        {
            memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_2lane_CHN0_SENSOR_SC2210_12BIT_2M_NOWDR_ATTR,
                sizeof(SC_COMBO_DEV_ATTR_S));
        }
        else
        {
            memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_2lane_CHN2_SENSOR_SC2210_12BIT_2M_NOWDR_ATTR,
                sizeof(SC_COMBO_DEV_ATTR_S));
        }

        break;

    case SMART_SC4210_MIPI_4M_30FPS_12BIT:
        memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_2lane_CHN0_SENSOR_SC4210_12BIT_4M_NOWDR_ATTR,
            sizeof(SC_COMBO_DEV_ATTR_S));

        break;

    case SONY_IMX415_MIPI_8M_30FPS_12BIT:
        if (0 == MipiDev)
        {
            memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_4lane_CHN0_SENSOR_IMX415_12BIT_8M_NOWDR_ATTR,
                sizeof(SC_COMBO_DEV_ATTR_S));
        }
        else
        {
            memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_4lane_CHN2_SENSOR_IMX415_12BIT_8M_NOWDR_ATTR,
                sizeof(SC_COMBO_DEV_ATTR_S));
        }
        break;

    case SONY_IMX415_MIPI_8M_60FPS_12BIT:
        memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_4lane_CHN0_SENSOR_IMX415_12BIT_8M_60FPS_NOWDR_ATTR,
            sizeof(SC_COMBO_DEV_ATTR_S));
        break;

    case SONY_IMX415_MIPI_8M_30FPS_12BIT_WDR2TO1:
        memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_4lane_CHN0_SENSOR_IMX415_12BIT_8M_WDR2to1_ATTR,
            sizeof(SC_COMBO_DEV_ATTR_S));
        break;

    case GALAXYCORE_GC2053_MIPI_2M_30FPS_10BIT:
        if (0 == MipiDev)
        {
            memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_2lane_CHN0_SENSOR_GC2053_10BIT_2M_NOWDR_ATTR,
                sizeof(SC_COMBO_DEV_ATTR_S));
        }
        else if (1 == MipiDev)
        {
            memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_2lane_CHN1_SENSOR_GC2053_10BIT_2M_NOWDR_ATTR,
                sizeof(SC_COMBO_DEV_ATTR_S));
        }
        else if (2 == MipiDev)
        {
            memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_2lane_CHN2_SENSOR_GC2053_10BIT_2M_NOWDR_ATTR,
                sizeof(SC_COMBO_DEV_ATTR_S));
        }
        else if (3 == MipiDev)
        {
            memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_2lane_CHN3_SENSOR_GC2053_10BIT_2M_NOWDR_ATTR,
                sizeof(SC_COMBO_DEV_ATTR_S));
        }
        else
        {
            memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_2lane_CHN0_SENSOR_GC2053_10BIT_2M_NOWDR_ATTR,
                sizeof(SC_COMBO_DEV_ATTR_S));
        }

        break;

    case GALAXYCORE_GC2093_MIPI_2M_30FPS_10BIT:
        if (0 == MipiDev)
        {
            memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_2lane_CHN0_SENSOR_GC2093_10BIT_2M_NOWDR_ATTR,
                sizeof(SC_COMBO_DEV_ATTR_S));
        }
        else if (1 == MipiDev)
        {
            memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_2lane_CHN1_SENSOR_GC2093_10BIT_2M_NOWDR_ATTR,
                sizeof(SC_COMBO_DEV_ATTR_S));
        }
        else if (2 == MipiDev)
        {
            memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_2lane_CHN2_SENSOR_GC2093_10BIT_2M_NOWDR_ATTR,
                sizeof(SC_COMBO_DEV_ATTR_S));
        }
        else if (3 == MipiDev)
        {
            memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_2lane_CHN3_SENSOR_GC2093_10BIT_2M_NOWDR_ATTR,
                sizeof(SC_COMBO_DEV_ATTR_S));
        }
        else
        {
            memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_2lane_CHN0_SENSOR_GC2093_10BIT_2M_NOWDR_ATTR,
                sizeof(SC_COMBO_DEV_ATTR_S));
        }

        break;

    case GALAXYCORE_GC2093_MIPI_2M_30FPS_10BIT_WDR2TO1:
        memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_2lane_CHN0_SENSOR_GC2093_10BIT_2M_WDR2to1_ATTR,
            sizeof(SC_COMBO_DEV_ATTR_S));
        break;

    case GALAXYCORE_GC1603_MIPI_1M_60FPS_10BIT:
        memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_2lane_CHN0_SENSOR_GC1603_10BIT_1M_60FPS_NOWDR_ATTR,
            sizeof(SC_COMBO_DEV_ATTR_S));
        break;

    case GALAXYCORE_GC1603_MIPI_1M_120FPS_10BIT:
        memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_2lane_CHN0_SENSOR_GC1603_10BIT_1M_120FPS_NOWDR_ATTR,
            sizeof(SC_COMBO_DEV_ATTR_S));
        break;

    case OMNIVISION_OV13B10_MIPI_13M_1FPS_10BIT:
        memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_2lane_CHN0_SENSOR_OV13B10_10BIT_13M_NOWDR_ATTR,
            sizeof(SC_COMBO_DEV_ATTR_S));
        break;

    case OMNIVISION_OV13B10_MIPI_2M_30FPS_10BIT:
        memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_4lane_CHN0_SENSOR_OV13B10_10BIT_2M_NOWDR_ATTR,
            sizeof(SC_COMBO_DEV_ATTR_S));
        break;

    case TP9930_DVP_4VC_1080P_25FPS:
        memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &DVP_4VC_TP9930_1080P_ATTR, sizeof(SC_COMBO_DEV_ATTR_S));
        pstComboAttr->devno = MipiDev;
        break;

    case ISP_VIN_0_MEM_2M_30FPS_12BIT:
        memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_4lane_CHN0_SENSOR_ISPVIN0_12BIT_2M_NOWDR_ATTR,
            sizeof(SC_COMBO_DEV_ATTR_S));
        break;

    default:
        SAMPLE_PRT("not support enSnsType: %d\n", enSnsType);
        memcpy_s(pstComboAttr, sizeof(SC_COMBO_DEV_ATTR_S), &MIPI_2lane_CHN0_SENSOR_IMX307_12BIT_2M_NOWDR_ATTR,
            sizeof(SC_COMBO_DEV_ATTR_S));
    }

    if (MipiDev >= sizeof(gSnsGpio)/sizeof(gSnsGpio[0]))
    {
        SAMPLE_PRT("ERROR: MipiDev %d\n", MipiDev);
        return SC_FAILURE;
    }
    pstComboAttr->dev_power_attr.reset_gpio[0] = gSnsGpio[MipiDev].reset_gpio[0];
    pstComboAttr->dev_power_attr.reset_gpio[1] = gSnsGpio[MipiDev].reset_gpio[1];
    pstComboAttr->dev_power_attr.power_gpio[0] = gSnsGpio[MipiDev].power_gpio[0];
    pstComboAttr->dev_power_attr.power_gpio[1] = gSnsGpio[MipiDev].power_gpio[1];

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_VI_SetMipiAttr(SAMPLE_VI_CONFIG_S *pstViConfig)
{
    SC_S32 i        = 0;
    SC_S32 s32ViNum = 0;
    SC_S32 s32Ret   = SC_SUCCESS;

    SAMPLE_VI_INFO_S    *pstViInfo     = SC_NULL;
    SC_COMBO_DEV_ATTR_S stcomboDevAttr = {};

    if (!pstViConfig)
    {
        SAMPLE_PRT("%s: null ptr\n", __FUNCTION__);
        return SC_FAILURE;
    }

    for (i = 0; i < pstViConfig->s32WorkingViNum; i++)
    {
        s32ViNum  = pstViConfig->as32WorkingViId[i];
        pstViInfo = &pstViConfig->astViInfo[s32ViNum];

        SAMPLE_COMM_VI_GetComboAttrBySns(pstViInfo->stSnsInfo.enSnsType, pstViInfo->stSnsInfo.MipiDev, &stcomboDevAttr);
        stcomboDevAttr.devno = pstViInfo->stSnsInfo.MipiDev;
        if(VI_PARALLEL_VPSS_OFFLINE == pstViInfo->stPipeInfo.enMastPipeMode
            || VI_PARALLEL_VPSS_PARALLEL == pstViInfo->stPipeInfo.enMastPipeMode)
        {
            //stcomboDevAttr.data_rate = MIPI_DATA_RATE_X2;
        }

        SAMPLE_PRT("============= MipiDev %d, SetMipiAttr enWDRMode: %d\n",
            pstViInfo->stSnsInfo.MipiDev, pstViInfo->stDevInfo.enWDRMode);

        s32Ret = SC_MPI_ISP_SetComboDevAttr(&stcomboDevAttr);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SC_MPI_ISP_SetComboDevAttr failed\n");
            goto EXIT;
        }
    }

EXIT:
    return s32Ret;
}

/*****************************************************************************
* function : init mipi
*****************************************************************************/
SC_S32 SAMPLE_COMM_VI_StartMIPI(SAMPLE_VI_CONFIG_S *pstViConfig)
{
    SC_S32 s32Ret   = SC_SUCCESS;
    SC_S32 s32ViNum = 0;

    SAMPLE_VI_INFO_S *pstViInfo = SC_NULL;

    if (!pstViConfig)
    {
        SAMPLE_PRT("%s: null ptr\n", __FUNCTION__);
        return SC_FAILURE;
    }

    for (int i = 0; i < pstViConfig->s32WorkingViNum; i++)
    {
        s32ViNum  = pstViConfig->as32WorkingViId[i];
        pstViInfo = &pstViConfig->astViInfo[s32ViNum];
        s32Ret    = SC_MPI_ISP_SetMipiBindDev(pstViInfo->stDevInfo.ViDev, pstViInfo->stSnsInfo.MipiDev);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("SAMPLE_COMM_VI_CreateSingleVi failed!\n");
            return SC_FAILURE;
        }
    }

    s32Ret = SAMPLE_COMM_VI_SetMipiAttr(pstViConfig);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_SetMipiAttr failed!\n");
        return SC_FAILURE;
    }

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_VI_StopMIPI(SAMPLE_VI_CONFIG_S *pstViConfig)
{
    if (!pstViConfig)
    {
        SAMPLE_PRT("%s: null ptr\n", __FUNCTION__);
        return SC_FAILURE;
    }

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_VI_SetParam(SAMPLE_VI_CONFIG_S *pstViConfig)
{
    SC_S32              i, j;
    SC_S32              s32ViNum;
    SC_S32              s32Ret;
    VI_PIPE             ViPipe;
    VI_VPSS_MODE_S      stVIVPSSMode;
    SAMPLE_VI_INFO_S   *pstViInfo = SC_NULL;

    if (!pstViConfig)
    {
        SAMPLE_PRT("%s: null ptr\n", __FUNCTION__);
        return SC_FAILURE;
    }

    s32Ret = SC_MPI_SYS_GetVIVPSSMode(&stVIVPSSMode);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Get VI-VPSS mode Param failed with %#x!\n", s32Ret);

        return SC_FAILURE;
    }

    for (i = 0; i < pstViConfig->s32WorkingViNum; i++)
    {
        s32ViNum  = pstViConfig->as32WorkingViId[i];
        pstViInfo = &pstViConfig->astViInfo[s32ViNum];
        ViPipe    = pstViInfo->stPipeInfo.aPipe[0];

        if (VI_OFFLINE_VPSS_ONLINE == pstViInfo->stPipeInfo.enMastPipeMode)
        {
            for (j = 0; j < VI_MAX_PIPE_NUM; j++)
            {
                stVIVPSSMode.aenMode[j] = VI_OFFLINE_VPSS_ONLINE;
            }
        }

        stVIVPSSMode.aenMode[ViPipe] = pstViInfo->stPipeInfo.enMastPipeMode;
        if (pstViInfo->stPipeInfo.bMultiPipe == SC_TRUE)
        {
            ViPipe = pstViInfo->stPipeInfo.aPipe[1];
            if(ViPipe != -1)
            {
                stVIVPSSMode.aenMode[ViPipe] = pstViInfo->stPipeInfo.enMastPipeMode;
            }

            ViPipe = pstViInfo->stPipeInfo.aPipe[2];
            if(ViPipe != -1)
            {
                stVIVPSSMode.aenMode[ViPipe] = pstViInfo->stPipeInfo.enMastPipeMode;
            }

            ViPipe = pstViInfo->stPipeInfo.aPipe[3];
            if(ViPipe != -1)
            {
                stVIVPSSMode.aenMode[ViPipe] = pstViInfo->stPipeInfo.enMastPipeMode;
            }
        }

        if ((pstViInfo->stSnapInfo.bSnap) && (pstViInfo->stSnapInfo.bDoublePipe))
        {
            ViPipe    = pstViInfo->stPipeInfo.aPipe[1];
            if(ViPipe != -1)
            {
                stVIVPSSMode.aenMode[ViPipe] = pstViInfo->stSnapInfo.enSnapPipeMode;
            }
        }
    }
    s32Ret = SC_MPI_SYS_SetVIVPSSMode(&stVIVPSSMode);

    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Set VI-VPSS mode Param failed with %#x!\n", s32Ret);

        return SC_FAILURE;
    }

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_VI_GetDevAttrBySns(VI_DEV ViDev, SAMPLE_SNS_TYPE_E enSnsType, VI_DEV_ATTR_S *pstViDevAttr)
{
    switch (enSnsType)
    {
    case SONY_IMX307_MIPI_2M_30FPS_12BIT:
    case SONY_IMX291_MIPI_2M_30FPS_12BIT:
        memcpy_s(pstViDevAttr, sizeof(VI_DEV_ATTR_S), &DEV_ATTR_IMX307_2M_BASE, sizeof(VI_DEV_ATTR_S));
        break;

    case SONY_IMX307_MIPI_2M_30FPS_12BIT_WDR2TO1:
        memcpy_s(pstViDevAttr, sizeof(VI_DEV_ATTR_S), &DEV_ATTR_IMX307_2M_WDR2TO1_BASE, sizeof(VI_DEV_ATTR_S));
        break;

    case SONY_IMX291_MIPI_2M_60FPS_10BIT:
    case SONY_IMX291_MIPI_2M_120FPS_10BIT:
        memcpy_s(pstViDevAttr, sizeof(VI_DEV_ATTR_S), &DEV_ATTR_IMX291_2M_BASE_10BIT, sizeof(VI_DEV_ATTR_S));
        break;

    case SMART_SC2310_MIPI_2M_30FPS_12BIT:
        memcpy_s(pstViDevAttr, sizeof(VI_DEV_ATTR_S), &DEV_ATTR_SC2310_2M_BASE, sizeof(VI_DEV_ATTR_S));
        break;

    case SMART_SC2310_MIPI_2M_30FPS_12BIT_WDR2TO1:
        memcpy_s(pstViDevAttr, sizeof(VI_DEV_ATTR_S), &DEV_ATTR_SC2310_2M_WDR2TO1_BASE, sizeof(VI_DEV_ATTR_S));
        break;

    case SMART_SC2210_MIPI_2M_30FPS_12BIT:
        memcpy_s(pstViDevAttr, sizeof(VI_DEV_ATTR_S), &DEV_ATTR_SC2210_2M_BASE, sizeof(VI_DEV_ATTR_S));
        break;

    case SMART_SC4210_MIPI_4M_30FPS_12BIT:
        memcpy_s(pstViDevAttr, sizeof(VI_DEV_ATTR_S), &DEV_ATTR_SC4210_4M_BASE, sizeof(VI_DEV_ATTR_S));
        break;

    case SONY_IMX415_MIPI_8M_30FPS_12BIT:
    case SONY_IMX415_MIPI_8M_60FPS_12BIT:
        memcpy_s(pstViDevAttr, sizeof(VI_DEV_ATTR_S), &DEV_ATTR_IMX415_8M_BASE, sizeof(VI_DEV_ATTR_S));
        break;

    case SONY_IMX415_MIPI_8M_30FPS_12BIT_WDR2TO1:
        memcpy_s(pstViDevAttr, sizeof(VI_DEV_ATTR_S), &DEV_ATTR_IMX415_8M_WDR2TO1_BASE, sizeof(VI_DEV_ATTR_S));
        break;

    case GALAXYCORE_GC2053_MIPI_2M_30FPS_10BIT:
        memcpy_s(pstViDevAttr, sizeof(VI_DEV_ATTR_S), &DEV_ATTR_GC2053_2M_BASE, sizeof(VI_DEV_ATTR_S));
        break;

    case GALAXYCORE_GC2093_MIPI_2M_30FPS_10BIT:
        memcpy_s(pstViDevAttr, sizeof(VI_DEV_ATTR_S), &DEV_ATTR_GC2093_2M_BASE, sizeof(VI_DEV_ATTR_S));
        break;

    case GALAXYCORE_GC2093_MIPI_2M_30FPS_10BIT_WDR2TO1:
        memcpy_s(pstViDevAttr, sizeof(VI_DEV_ATTR_S), &DEV_ATTR_GC2093_2M_WDR2TO1_BASE, sizeof(VI_DEV_ATTR_S));
        break;

    case GALAXYCORE_GC1603_MIPI_1M_60FPS_10BIT:
    case GALAXYCORE_GC1603_MIPI_1M_120FPS_10BIT:
        memcpy_s(pstViDevAttr, sizeof(VI_DEV_ATTR_S), &DEV_ATTR_GC1603_1M_BASE, sizeof(VI_DEV_ATTR_S));
        break;

    case OMNIVISION_OV13B10_MIPI_13M_1FPS_10BIT:
        memcpy_s(pstViDevAttr, sizeof(VI_DEV_ATTR_S), &DEV_ATTR_OV13B10_13M_BASE, sizeof(VI_DEV_ATTR_S));
        break;

    case OMNIVISION_OV13B10_MIPI_2M_30FPS_10BIT:
        memcpy_s(pstViDevAttr, sizeof(VI_DEV_ATTR_S), &DEV_ATTR_OV13B10_2M_BASE, sizeof(VI_DEV_ATTR_S));
        break;

    case ISP_VIN_0_MEM_2M_30FPS_12BIT:
        memcpy_s(pstViDevAttr, sizeof(VI_DEV_ATTR_S), &DEV_ATTR_ISP_VIN_0_2M_BASE, sizeof(VI_DEV_ATTR_S));
        break;

    case TP9930_DVP_4VC_1080P_25FPS:
        memcpy_s(pstViDevAttr, sizeof(VI_DEV_ATTR_S),
            &DEV_ATTR_TP9930_1080P_BASE, sizeof(VI_DEV_ATTR_S));
        if (enSnsType == TP9930_DVP_4VC_1080P_25FPS)
        {
            pstViDevAttr->enIntfMode = VI_MODE_BT1120_STANDARD;
            pstViDevAttr->enDataRate = DATA_RATE_X2;
            pstViDevAttr->enWorkMode = VI_WORK_MODE_4Multiplex;
            if (ViDev == 0)
            {
                pstViDevAttr->as32AdChnId[0] = 0;
                pstViDevAttr->as32AdChnId[1] = 1;
                pstViDevAttr->as32AdChnId[2] = 2;
                pstViDevAttr->as32AdChnId[3] = 3;
            }
            else
            {
                pstViDevAttr->as32AdChnId[0] = 4;
                pstViDevAttr->as32AdChnId[1] = 5;
                pstViDevAttr->as32AdChnId[2] = 6;
                pstViDevAttr->as32AdChnId[3] = 7;
            }
        }
        break;

    default:
        memcpy_s(pstViDevAttr, sizeof(VI_DEV_ATTR_S), &DEV_ATTR_IMX307_2M_BASE, sizeof(VI_DEV_ATTR_S));
    }

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_VI_GetPipeAttrBySns(SAMPLE_SNS_TYPE_E enSnsType, VI_PIPE_ATTR_S *pstPipeAttr)
{
    switch (enSnsType)
    {
    case SONY_IMX307_MIPI_2M_30FPS_12BIT:
    case SONY_IMX291_MIPI_2M_30FPS_12BIT:
        memcpy_s(pstPipeAttr, sizeof(VI_PIPE_ATTR_S), &PIPE_ATTR_1920x1080_RAW12_420_3DNR_RFR, sizeof(VI_PIPE_ATTR_S));
        break;

    case SONY_IMX307_MIPI_2M_30FPS_12BIT_WDR2TO1:
        memcpy_s(pstPipeAttr, sizeof(VI_PIPE_ATTR_S), &PIPE_ATTR_1920x1080_RAW12_420_3DNR_RFR_WDR, sizeof(VI_PIPE_ATTR_S));
        break;

    case SONY_IMX291_MIPI_2M_60FPS_10BIT:
        memcpy_s(pstPipeAttr, sizeof(VI_PIPE_ATTR_S), &PIPE_ATTR_1920x1080_RAW10_420_3DNR_RFR_60FPS, sizeof(VI_PIPE_ATTR_S));
        break;

    case SONY_IMX291_MIPI_2M_120FPS_10BIT:
        memcpy_s(pstPipeAttr, sizeof(VI_PIPE_ATTR_S), &PIPE_ATTR_1920x1080_RAW10_420_3DNR_RFR_120FPS, sizeof(VI_PIPE_ATTR_S));
        break;

    case SMART_SC2310_MIPI_2M_30FPS_12BIT:
        memcpy_s(pstPipeAttr, sizeof(VI_PIPE_ATTR_S), &PIPE_ATTR_1920x1080_RAW12_420_3DNR_RFR, sizeof(VI_PIPE_ATTR_S));
        break;

    case SMART_SC2310_MIPI_2M_30FPS_12BIT_WDR2TO1:
        memcpy_s(pstPipeAttr, sizeof(VI_PIPE_ATTR_S), &PIPE_ATTR_1920x1080_RAW10_420_3DNR_RFR_WDR, sizeof(VI_PIPE_ATTR_S));
        break;

    case SMART_SC2210_MIPI_2M_30FPS_12BIT:
        memcpy_s(pstPipeAttr, sizeof(VI_PIPE_ATTR_S), &PIPE_ATTR_1920x1080_RAW12_420_3DNR_RFR, sizeof(VI_PIPE_ATTR_S));
        break;

    case SMART_SC4210_MIPI_4M_30FPS_12BIT:
        memcpy_s(pstPipeAttr, sizeof(VI_PIPE_ATTR_S), &PIPE_ATTR_2560x1440_RAW12_420_3DNR_RFR, sizeof(VI_PIPE_ATTR_S));
        break;

    case SONY_IMX415_MIPI_8M_30FPS_12BIT:
        memcpy_s(pstPipeAttr, sizeof(VI_PIPE_ATTR_S), &PIPE_ATTR_3840x2160_RAW12_420_3DNR_RFR, sizeof(VI_PIPE_ATTR_S));
        break;

    case SONY_IMX415_MIPI_8M_60FPS_12BIT:
        memcpy_s(pstPipeAttr, sizeof(VI_PIPE_ATTR_S), &PIPE_ATTR_3840x2160_RAW12_420_3DNR_RFR_60FPS, sizeof(VI_PIPE_ATTR_S));
        break;

    case SONY_IMX415_MIPI_8M_30FPS_12BIT_WDR2TO1:
        memcpy_s(pstPipeAttr, sizeof(VI_PIPE_ATTR_S), &PIPE_ATTR_3840x2160_RAW12_420_3DNR_RFR_WDR, sizeof(VI_PIPE_ATTR_S));
        break;

    case GALAXYCORE_GC2053_MIPI_2M_30FPS_10BIT:
    case GALAXYCORE_GC2093_MIPI_2M_30FPS_10BIT:
    case OMNIVISION_OV13B10_MIPI_2M_30FPS_10BIT:
        memcpy_s(pstPipeAttr, sizeof(VI_PIPE_ATTR_S), &PIPE_ATTR_1920x1080_RAW10_420_3DNR_RFR, sizeof(VI_PIPE_ATTR_S));
        break;

    case GALAXYCORE_GC2093_MIPI_2M_30FPS_10BIT_WDR2TO1:
        memcpy_s(pstPipeAttr, sizeof(VI_PIPE_ATTR_S), &PIPE_ATTR_GC2093_1080P_RAW10_WDR, sizeof(VI_PIPE_ATTR_S));
        break;

    case GALAXYCORE_GC1603_MIPI_1M_60FPS_10BIT:
        memcpy_s(pstPipeAttr, sizeof(VI_PIPE_ATTR_S), &PIPE_ATTR_1280x720_RAW10_420_3DNR_RFR_60FPS, sizeof(VI_PIPE_ATTR_S));
        break;

    case GALAXYCORE_GC1603_MIPI_1M_120FPS_10BIT:
        memcpy_s(pstPipeAttr, sizeof(VI_PIPE_ATTR_S), &PIPE_ATTR_1280x720_RAW10_420_3DNR_RFR_120FPS, sizeof(VI_PIPE_ATTR_S));
        break;

    case OMNIVISION_OV13B10_MIPI_13M_1FPS_10BIT:
        memcpy_s(pstPipeAttr, sizeof(VI_PIPE_ATTR_S), &PIPE_ATTR_4096x3120_RAW10_420_3DNR_RFR_1FPS, sizeof(VI_PIPE_ATTR_S));
        break;

    case ISP_VIN_0_MEM_2M_30FPS_12BIT:
        memcpy_s(pstPipeAttr, sizeof(VI_PIPE_ATTR_S), &PIPE_ATTR_1920x1080_RAW12_420_3DNR_RFR, sizeof(VI_PIPE_ATTR_S));
        break;

    case TP9930_DVP_4VC_1080P_25FPS:
        memcpy_s(pstPipeAttr, sizeof(VI_PIPE_ATTR_S),
            &PIPE_ATTR_1080P_YUV422, sizeof(VI_PIPE_ATTR_S));
        break;

    default:
        memcpy_s(pstPipeAttr, sizeof(VI_PIPE_ATTR_S), &PIPE_ATTR_1920x1080_RAW12_420_3DNR_RFR, sizeof(VI_PIPE_ATTR_S));
    }

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_VI_GetChnAttrBySns(SAMPLE_SNS_TYPE_E enSnsType, VI_CHN_ATTR_S *pstChnAttr)
{
    switch (enSnsType)
    {
    case SONY_IMX307_MIPI_2M_30FPS_12BIT:
    case SONY_IMX291_MIPI_2M_30FPS_12BIT:
    case SONY_IMX291_MIPI_2M_60FPS_10BIT:
    case SONY_IMX291_MIPI_2M_120FPS_10BIT:
    case SMART_SC2310_MIPI_2M_30FPS_12BIT:
    case SMART_SC2210_MIPI_2M_30FPS_12BIT:
    case GALAXYCORE_GC2053_MIPI_2M_30FPS_10BIT:
    case GALAXYCORE_GC2093_MIPI_2M_30FPS_10BIT:
    case OMNIVISION_OV13B10_MIPI_2M_30FPS_10BIT:
        memcpy_s(pstChnAttr, sizeof(VI_CHN_ATTR_S), &CHN_ATTR_1920x1080_420_SDR8_LINEAR, sizeof(VI_CHN_ATTR_S));
        break;

    case SMART_SC4210_MIPI_4M_30FPS_12BIT:
        memcpy_s(pstChnAttr, sizeof(VI_CHN_ATTR_S), &CHN_ATTR_2560x1440_420_SDR8_LINEAR, sizeof(VI_CHN_ATTR_S));
        break;

    case SONY_IMX307_MIPI_2M_30FPS_12BIT_WDR2TO1:
    case GALAXYCORE_GC2093_MIPI_2M_30FPS_10BIT_WDR2TO1:
        memcpy_s(pstChnAttr, sizeof(VI_CHN_ATTR_S), &CHN_ATTR_1920x1080_420_SDR8_HDR, sizeof(VI_CHN_ATTR_S));
        break;

    case SMART_SC2310_MIPI_2M_30FPS_12BIT_WDR2TO1:
        memcpy_s(pstChnAttr, sizeof(VI_CHN_ATTR_S), &CHN_ATTR_1920x1080_420_SDR8_HDR, sizeof(VI_CHN_ATTR_S));
        break;

    case SONY_IMX415_MIPI_8M_30FPS_12BIT:
    case SONY_IMX415_MIPI_8M_60FPS_12BIT:
        memcpy_s(pstChnAttr, sizeof(VI_CHN_ATTR_S), &CHN_ATTR_3840x2160_420_SDR8_LINEAR, sizeof(VI_CHN_ATTR_S));
        break;

    case SONY_IMX415_MIPI_8M_30FPS_12BIT_WDR2TO1:
        memcpy_s(pstChnAttr, sizeof(VI_CHN_ATTR_S), &CHN_ATTR_3840x2160_420_SDR8_HDR, sizeof(VI_CHN_ATTR_S));
        break;

    case GALAXYCORE_GC1603_MIPI_1M_60FPS_10BIT:
    case GALAXYCORE_GC1603_MIPI_1M_120FPS_10BIT:
        memcpy_s(pstChnAttr, sizeof(VI_CHN_ATTR_S), &CHN_ATTR_1280x720_420_SDR8_LINEAR, sizeof(VI_CHN_ATTR_S));
        break;

    case OMNIVISION_OV13B10_MIPI_13M_1FPS_10BIT:
        memcpy_s(pstChnAttr, sizeof(VI_CHN_ATTR_S), &CHN_ATTR_4096x3120_420_SDR8_LINEAR, sizeof(VI_CHN_ATTR_S));
        break;

    case ISP_VIN_0_MEM_2M_30FPS_12BIT:
        memcpy_s(pstChnAttr, sizeof(VI_CHN_ATTR_S), &CHN_ATTR_1920x1080_420_SDR8_LINEAR, sizeof(VI_CHN_ATTR_S));
        break;

    case TP9930_DVP_4VC_1080P_25FPS:
        memcpy_s(pstChnAttr, sizeof(VI_CHN_ATTR_S), &CHN_ATTR_1080P_YUV420_SDR8_LINEAR, sizeof(VI_CHN_ATTR_S));
        break;

    default:
        memcpy_s(pstChnAttr, sizeof(VI_CHN_ATTR_S), &CHN_ATTR_1920x1080_420_SDR8_LINEAR, sizeof(VI_CHN_ATTR_S));
    }

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_VI_StartDev(SAMPLE_VI_INFO_S *pstViInfo)
{
    SC_S32              s32Ret;
    VI_DEV              ViDev;
    SAMPLE_SNS_TYPE_E    enSnsType;
    VI_DEV_ATTR_S       stViDevAttr;

    ViDev       = pstViInfo->stDevInfo.ViDev;
    enSnsType    = pstViInfo->stSnsInfo.enSnsType;

    SAMPLE_COMM_VI_GetDevAttrBySns(ViDev, enSnsType, &stViDevAttr);
    stViDevAttr.stWDRAttr.enWDRMode = pstViInfo->stDevInfo.enWDRMode;
    if(VI_PARALLEL_VPSS_OFFLINE == pstViInfo->stPipeInfo.enMastPipeMode
        || VI_PARALLEL_VPSS_PARALLEL == pstViInfo->stPipeInfo.enMastPipeMode)
    {
        stViDevAttr.enDataRate = DATA_RATE_X2;
    }

    s32Ret = SC_MPI_VI_SetDevAttr(ViDev, &stViDevAttr);

    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SC_MPI_VI_SetDevAttr failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }

    s32Ret = SC_MPI_VI_EnableDev(ViDev);

    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SC_MPI_VI_EnableDev failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_VI_StopDev(SAMPLE_VI_INFO_S *pstViInfo)
{
    SC_S32 s32Ret;
    VI_DEV ViDev;

    ViDev   = pstViInfo->stDevInfo.ViDev;
    s32Ret  = SC_MPI_VI_DisableDev(ViDev);

    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SC_MPI_VI_DisableDev failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_VI_BindPipeDev(SAMPLE_VI_INFO_S *pstViInfo)
{
    SC_S32              i;
    SC_S32              s32PipeCnt = 0;
    SC_S32              s32Ret;
    VI_DEV_BIND_PIPE_S  stDevBindPipe = {0};

    for (i = 0; i < 4; i++)
    {
        if (pstViInfo->stPipeInfo.aPipe[i] >= 0  && pstViInfo->stPipeInfo.aPipe[i] < VI_MAX_PIPE_NUM)
        {
            stDevBindPipe.PipeId[s32PipeCnt] = pstViInfo->stPipeInfo.aPipe[i];
            s32PipeCnt++;
            stDevBindPipe.u32Num = s32PipeCnt;
        }
    }

    s32Ret = SC_MPI_VI_SetDevBindPipe(pstViInfo->stDevInfo.ViDev, &stDevBindPipe);

    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SC_MPI_VI_SetDevBindPipe failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }

    return s32Ret;
}

static SC_S32 SAMPLE_COMM_VI_ModeSwitchCreateSingleViPipe(VI_PIPE ViPipe, VI_PIPE_ATTR_S *pstPipeAttr)
{
    SC_S32 s32Ret;

    s32Ret = SC_MPI_VI_CreatePipe(ViPipe, pstPipeAttr);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SC_MPI_VI_CreatePipe failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }

    return s32Ret;
}

static SC_S32 SAMPLE_COMM_VI_ModeSwitch_EnableSingleViPipe(VI_PIPE ViPipe, VI_PIPE_ATTR_S *pstPipeAttr)
{
    SC_S32 s32Ret;

    s32Ret = SC_MPI_VI_StartPipe(ViPipe);

    if (s32Ret != SC_SUCCESS)
    {
        SC_MPI_VI_DestroyPipe(ViPipe);
        SAMPLE_PRT("SC_MPI_VI_StartPipe failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }

    return s32Ret;
}

static SC_S32 SAMPLE_COMM_VI_StopSingleViPipe(VI_PIPE ViPipe)
{
    SC_S32  s32Ret;

    s32Ret = SC_MPI_VI_StopPipe(ViPipe);

    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SC_MPI_VI_StopPipe failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }

    s32Ret = SC_MPI_VI_DestroyPipe(ViPipe);

    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SC_MPI_VI_DestroyPipe failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }

    return s32Ret;
}

SC_S32 SAMPLE_COMM_VI_ModeSwitch_StartViPipe(SAMPLE_VI_INFO_S *pstViInfo)
{
    SC_S32          i, j;
    SC_S32          s32Ret = SC_SUCCESS;
    VI_PIPE         ViPipe;
    VI_PIPE_ATTR_S  stPipeAttr;

    for (i = 0; i < WDR_MAX_PIPE_NUM; i++)
    {
        if (pstViInfo->stPipeInfo.aPipe[i] >= 0  && pstViInfo->stPipeInfo.aPipe[i] < VI_MAX_PIPE_NUM)
        {
            ViPipe = pstViInfo->stPipeInfo.aPipe[i];
            SAMPLE_COMM_VI_GetPipeAttrBySns(pstViInfo->stSnsInfo.enSnsType, &stPipeAttr);
            {
                s32Ret = SAMPLE_COMM_VI_ModeSwitchCreateSingleViPipe(ViPipe, &stPipeAttr);
                if (s32Ret != SC_SUCCESS)
                {
                    SAMPLE_PRT("SAMPLE_COMM_VI_StartSingleViPipe  %d failed!\n", ViPipe);
                    goto EXIT;
                }
            }

        }
    }

    return s32Ret;

EXIT:

    for (j = 0; j < i; j++)
    {
        ViPipe = j;
        SAMPLE_COMM_VI_StopSingleViPipe(ViPipe);
    }

    return s32Ret;
}

SC_S32 SAMPLE_COMM_VI_ModeSwitch_EnableViPipe(SAMPLE_VI_INFO_S *pstViInfo)
{
    SC_S32          i, j;
    SC_S32          s32Ret = SC_SUCCESS;
    VI_PIPE         ViPipe;
    VI_PIPE_ATTR_S  stPipeAttr;

    for (i = 0; i < WDR_MAX_PIPE_NUM; i++)
    {
        if (pstViInfo->stPipeInfo.aPipe[i] >= 0  && pstViInfo->stPipeInfo.aPipe[i] < VI_MAX_PIPE_NUM)
        {
            ViPipe = pstViInfo->stPipeInfo.aPipe[i];
            SAMPLE_COMM_VI_GetPipeAttrBySns(pstViInfo->stSnsInfo.enSnsType, &stPipeAttr);
            {
                s32Ret = SAMPLE_COMM_VI_ModeSwitch_EnableSingleViPipe(ViPipe, &stPipeAttr);
                if (s32Ret != SC_SUCCESS)
                {
                    SAMPLE_PRT("SAMPLE_COMM_VI_StartSingleViPipe  %d failed!\n", ViPipe);
                    goto EXIT;
                }
            }

        }
    }

    return s32Ret;

EXIT:

    for (j = 0; j < i; j++)
    {
        ViPipe = j;
        SAMPLE_COMM_VI_StopSingleViPipe(ViPipe);
    }

    return s32Ret;
}

SC_S32 SAMPLE_COMM_VI_StopIsp(SAMPLE_VI_INFO_S *pstViInfo)
{
    SC_S32  i;
    SC_BOOL bNeedPipe;
    VI_PIPE ViPipe;

    for (i = 0; i < WDR_MAX_PIPE_NUM; i++)
    {
        if (pstViInfo->stPipeInfo.aPipe[i] >= 0  && pstViInfo->stPipeInfo.aPipe[i] < VI_MAX_PIPE_NUM)
        {
            ViPipe = pstViInfo->stPipeInfo.aPipe[i];
            if (WDR_MODE_NONE == pstViInfo->stDevInfo.enWDRMode)
            {
                bNeedPipe = SC_TRUE;
            }
            else
            {
                bNeedPipe = (i > 0) ? SC_FALSE : SC_TRUE;
            }

            if (SC_TRUE != bNeedPipe)
            {
                continue;
            }

            SAMPLE_COMM_ISP_Stop(ViPipe);
        }
    }

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_VI_StartIsp(SAMPLE_VI_INFO_S *pstViInfo)
{
    SC_S32   i;
    SC_S32  s32Ret = SC_FAILURE;
    VI_PIPE ViPipe;

    for (i = 0; i < WDR_MAX_PIPE_NUM; i++)
    {
        if (pstViInfo->stPipeInfo.aPipe[i] >= 0  && pstViInfo->stPipeInfo.aPipe[i] < VI_MAX_PIPE_NUM)
        {
            ViPipe = pstViInfo->stPipeInfo.aPipe[i];
            s32Ret = SAMPLE_COMM_ISP_Run(ViPipe);
            if (s32Ret != SC_SUCCESS)
            {
                SAMPLE_PRT("ISP Run failed with %#x!\n", s32Ret);
                SAMPLE_COMM_ISP_Stop(ViPipe);
                return SC_FAILURE;
            }
        }
    }

    return s32Ret;
}

SC_S32 SAMPLE_COMM_VI_CreateIsp(SAMPLE_VI_INFO_S *pstViInfo)
{
    SC_S32  i;
    SC_S32  s32Ret = SC_FAILURE;
    SC_U32  u32SnsId;

    SC_BOOL bNeedPipe;
    VI_PIPE ViPipe;

    ISP_PUB_ATTR_S stPubAttr = {};

    for (i = 0; i < WDR_MAX_PIPE_NUM; i++)
    {
        if (pstViInfo->stPipeInfo.aPipe[i] >= 0  && pstViInfo->stPipeInfo.aPipe[i] < VI_MAX_PIPE_NUM)
        {
            ViPipe   = pstViInfo->stPipeInfo.aPipe[i];
            u32SnsId = pstViInfo->stSnsInfo.s32SnsId;

            SAMPLE_COMM_ISP_GetIspAttrBySns(pstViInfo->stSnsInfo.enSnsType, &stPubAttr);
            stPubAttr.enWDRMode = pstViInfo->stDevInfo.enWDRMode;
            if (WDR_MODE_NONE == pstViInfo->stDevInfo.enWDRMode)
            {
                bNeedPipe = SC_TRUE;
            }
            else
            {
                bNeedPipe = (i > 0) ? SC_FALSE : SC_TRUE;
            }

            if (SC_TRUE != bNeedPipe)
            {
                continue;
            }

            s32Ret = SAMPLE_COMM_ISP_Sensor_Regiter_callback(ViPipe, u32SnsId);
            if (SC_SUCCESS != s32Ret)
            {
                SAMPLE_PRT("register sensor %d to ISP %d failed\n", u32SnsId, ViPipe);
                SAMPLE_COMM_ISP_Stop(ViPipe);
                return SC_FAILURE;
            }

            if(((pstViInfo->stSnapInfo.bDoublePipe) && (pstViInfo->stSnapInfo.SnapPipe == ViPipe))
                || (pstViInfo->stPipeInfo.bMultiPipe && i > 0))
            {
                s32Ret = SAMPLE_COMM_ISP_BindSns(ViPipe, u32SnsId, pstViInfo->stSnsInfo.enSnsType, -1);
                if (SC_SUCCESS != s32Ret)
                {
                    SAMPLE_PRT("register sensor %d bus id %d failed\n", u32SnsId, pstViInfo->stSnsInfo.s32BusId);
                    SAMPLE_COMM_ISP_Stop(ViPipe);
                    return SC_FAILURE;
                }
            }
            else
            {
                s32Ret = SAMPLE_COMM_ISP_BindSns(ViPipe, u32SnsId, pstViInfo->stSnsInfo.enSnsType, pstViInfo->stSnsInfo.s32BusId);
                if (SC_SUCCESS != s32Ret)
                {
                    SAMPLE_PRT("register sensor %d bus id %d failed\n", u32SnsId, pstViInfo->stSnsInfo.s32BusId);
                    SAMPLE_COMM_ISP_Stop(ViPipe);
                    return SC_FAILURE;
                }
            }

            s32Ret = SAMPLE_COMM_ISP_Aelib_Callback(ViPipe);
            if (SC_SUCCESS != s32Ret)
            {
                SAMPLE_PRT("SAMPLE_COMM_ISP_Aelib_Callback failed\n");
                SAMPLE_COMM_ISP_Stop(ViPipe);
                return SC_FAILURE;
            }

            s32Ret = SAMPLE_COMM_ISP_Awblib_Callback(ViPipe);
            if (SC_SUCCESS != s32Ret)
            {
                SAMPLE_PRT("SAMPLE_COMM_ISP_Awblib_Callback failed\n");
                SAMPLE_COMM_ISP_Stop(ViPipe);
                return SC_FAILURE;
            }

            s32Ret = SC_MPI_ISP_MemInit(ViPipe);
            if (s32Ret != SC_SUCCESS)
            {
                SAMPLE_PRT("Init Ext memory failed with %#x!\n", s32Ret);
                SAMPLE_COMM_ISP_Stop(ViPipe);
                return SC_FAILURE;
            }

            s32Ret = SC_MPI_ISP_SetPubAttr(ViPipe, &stPubAttr);
            if (s32Ret != SC_SUCCESS)
            {
                SAMPLE_PRT("SetPubAttr failed with %#x!\n", s32Ret);
                SAMPLE_COMM_ISP_Stop(ViPipe);
                return SC_FAILURE;
            }

            s32Ret = SC_MPI_ISP_Init(ViPipe);
            if (s32Ret != SC_SUCCESS)
            {
                SAMPLE_PRT("ISP Init failed with %#x!\n", s32Ret);
                SAMPLE_COMM_ISP_Stop(ViPipe);
                return SC_FAILURE;
            }
        }
    }

    return s32Ret;
}

SC_S32 SAMPLE_COMM_VI_CreateViPipe(SAMPLE_VI_INFO_S *pstViInfo)
{
    SC_S32  i, j;
    SC_S32  s32Ret = SC_FAILURE;

    VI_PIPE ViPipe;
    VI_PIPE_ATTR_S stPipeAttr;

    for (i = 0; i < WDR_MAX_PIPE_NUM; i++)
    {
        if (pstViInfo->stPipeInfo.aPipe[i] >= 0  && pstViInfo->stPipeInfo.aPipe[i] < VI_MAX_PIPE_NUM)
        {
            ViPipe = pstViInfo->stPipeInfo.aPipe[i];
            SAMPLE_COMM_VI_GetPipeAttrBySns(pstViInfo->stSnsInfo.enSnsType, &stPipeAttr);
            if ((pstViInfo->stSnapInfo.bSnap) && (pstViInfo->stSnapInfo.bDoublePipe) && (ViPipe == pstViInfo->stSnapInfo.SnapPipe))
            {
                s32Ret = SC_MPI_VI_CreatePipe(ViPipe, &stPipeAttr);
                if (s32Ret != SC_SUCCESS)
                {
                    SAMPLE_PRT("SC_MPI_VI_CreatePipe failed with %#x!\n", s32Ret);
                    goto EXIT;
                }
            }
            else
            {
                s32Ret = SC_MPI_VI_CreatePipe(ViPipe, &stPipeAttr);
                if (s32Ret != SC_SUCCESS)
                {
                    SAMPLE_PRT("SC_MPI_VI_CreatePipe failed with %#x!\n", s32Ret);
                    return SC_FAILURE;
                }

                if (SC_TRUE == pstViInfo->stPipeInfo.bVcNumCfged)
                {
                    s32Ret = SC_MPI_VI_SetPipeVCNumber(ViPipe, pstViInfo->stPipeInfo.u32VCNum[i]);
                    if (s32Ret != SC_SUCCESS)
                    {
                        SC_MPI_VI_DestroyPipe(ViPipe);
                        SAMPLE_PRT("SC_MPI_VI_SetPipeVCNumber failed with %#x!\n", s32Ret);
                        return SC_FAILURE;
                    }
                }
            }
        }
    }

    return s32Ret;

EXIT:
    for (j = 0; j < i; j++)
    {
        ViPipe = j;
        SAMPLE_COMM_VI_StopSingleViPipe(ViPipe);
    }

    return s32Ret;
}

SC_S32 SAMPLE_COMM_VI_StartViPipe(SAMPLE_VI_INFO_S *pstViInfo)
{
    SC_S32  i;
    SC_S32  s32Ret = SC_FAILURE;
    VI_PIPE ViPipe;

    for (i = 0; i < WDR_MAX_PIPE_NUM; i++)
    {
        if (pstViInfo->stPipeInfo.aPipe[i] >= 0  && pstViInfo->stPipeInfo.aPipe[i] < VI_MAX_PIPE_NUM)
        {
            ViPipe = pstViInfo->stPipeInfo.aPipe[i];
            s32Ret = SC_MPI_VI_StartPipe(ViPipe);
            if (s32Ret != SC_SUCCESS)
            {
                SC_MPI_VI_DestroyPipe(ViPipe);
                SAMPLE_PRT("SC_MPI_VI_StartPipe failed with %#x!\n", s32Ret);
                return SC_FAILURE;
            }
        }
    }

    return s32Ret;
}

SC_S32 SAMPLE_COMM_VI_StopViPipe(SAMPLE_VI_INFO_S *pstViInfo)
{
    SC_S32  i;
    VI_PIPE ViPipe;

    for (i = 0; i < WDR_MAX_PIPE_NUM; i++)
    {
        if (pstViInfo->stPipeInfo.aPipe[i] >= 0  && pstViInfo->stPipeInfo.aPipe[i] < VI_MAX_PIPE_NUM)
        {
            ViPipe = pstViInfo->stPipeInfo.aPipe[i];
            SAMPLE_COMM_VI_StopSingleViPipe(ViPipe);
        }
    }

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_VI_ModeSwitch_StartViChn(SAMPLE_VI_INFO_S *pstViInfo)
{
    SC_S32              i;
    SC_BOOL             bNeedChn;
    SC_S32              s32Ret = SC_SUCCESS;
    VI_PIPE             ViPipe;
    VI_CHN              ViChn;
    VI_CHN_ATTR_S       stChnAttr;

    for (i = 0; i < WDR_MAX_PIPE_NUM; i++)
    {
        if (pstViInfo->stPipeInfo.aPipe[i] >= 0  && pstViInfo->stPipeInfo.aPipe[i] < VI_MAX_PIPE_NUM)
        {
            ViPipe = pstViInfo->stPipeInfo.aPipe[i];
            ViChn  = pstViInfo->stChnInfo.ViChn;

            SAMPLE_COMM_VI_GetChnAttrBySns(pstViInfo->stSnsInfo.enSnsType, &stChnAttr);
            stChnAttr.enDynamicRange = pstViInfo->stChnInfo.enDynamicRange;
            stChnAttr.enVideoFormat  = pstViInfo->stChnInfo.enVideoFormat;
            stChnAttr.enPixelFormat  = pstViInfo->stChnInfo.enPixFormat;
            stChnAttr.enCompressMode = pstViInfo->stChnInfo.enCompressMode;

            if (WDR_MODE_NONE == pstViInfo->stDevInfo.enWDRMode)
            {
                bNeedChn = SC_TRUE;
            }
            else
            {
                bNeedChn = (i > 0) ? SC_FALSE : SC_TRUE;
            }

            if (bNeedChn)
            {
                s32Ret = SC_MPI_VI_SetChnAttr(ViPipe, ViChn, &stChnAttr);
                if (s32Ret != SC_SUCCESS)
                {
                    SAMPLE_PRT("SC_MPI_VI_SetChnAttr failed with %#x!\n", s32Ret);
                    return SC_FAILURE;
                }
            }
        }
    }

    return s32Ret;
}
SC_S32 SAMPLE_COMM_VI_StartViChn(SAMPLE_VI_INFO_S *pstViInfo)
{
    SC_S32              i;
    SC_BOOL             bNeedChn;
    SC_S32              s32Ret = SC_SUCCESS;
    VI_PIPE             ViPipe;
    VI_CHN              ViChn;
    VI_CHN_ATTR_S       stChnAttr;
    VI_VPSS_MODE_E      enMastPipeMode;

    for (i = 0; i < WDR_MAX_PIPE_NUM; i++)
    {
        if (pstViInfo->stPipeInfo.aPipe[i] >= 0  && pstViInfo->stPipeInfo.aPipe[i] < VI_MAX_PIPE_NUM)
        {
            ViPipe = pstViInfo->stPipeInfo.aPipe[i];
            ViChn  = pstViInfo->stChnInfo.ViChn;

            SAMPLE_COMM_VI_GetChnAttrBySns(pstViInfo->stSnsInfo.enSnsType, &stChnAttr);
            stChnAttr.enDynamicRange = pstViInfo->stChnInfo.enDynamicRange;
            stChnAttr.enVideoFormat  = pstViInfo->stChnInfo.enVideoFormat;
            stChnAttr.enPixelFormat  = pstViInfo->stChnInfo.enPixFormat;
            stChnAttr.enCompressMode = pstViInfo->stChnInfo.enCompressMode;
            stChnAttr.bMirror        = pstViInfo->stChnInfo.bMirror;
            stChnAttr.bFlip          = pstViInfo->stChnInfo.bFlip;
            if(pstViInfo->stChnInfo.stSize.u32Width)
            {
                stChnAttr.stSize.u32Width = pstViInfo->stChnInfo.stSize.u32Width;
                stChnAttr.stSize.u32Height = pstViInfo->stChnInfo.stSize.u32Height;
            }

            if(pstViInfo->stChnInfo.stFps.s32SrcFrameRate)
            {
                stChnAttr.stFrameRate = pstViInfo->stChnInfo.stFps;
            }

            if (WDR_MODE_NONE == pstViInfo->stDevInfo.enWDRMode)
            {
                bNeedChn = SC_TRUE;
            }
            else
            {
                bNeedChn = (i > 0) ? SC_FALSE : SC_TRUE;
            }

            if (bNeedChn)
            {
                s32Ret = SC_MPI_VI_SetChnAttr(ViPipe, ViChn, &stChnAttr);

                if (s32Ret != SC_SUCCESS)
                {
                    SAMPLE_PRT("SC_MPI_VI_SetChnAttr failed with %#x!\n", s32Ret);
                    return SC_FAILURE;
                }

                enMastPipeMode = pstViInfo->stPipeInfo.enMastPipeMode;

                if(pstViInfo->stChnInfo.u32Align)
                {
                    s32Ret = SC_MPI_VI_SetChnAlign(ViPipe, ViChn, pstViInfo->stChnInfo.u32Align);
                    if (s32Ret != SC_SUCCESS)
                    {
                        SAMPLE_PRT("SC_MPI_VI_SetChnAlign failed with %#x!\n", s32Ret);
                        return SC_FAILURE;
                    }
                }

                if (VI_OFFLINE_VPSS_OFFLINE == enMastPipeMode
                    || VI_ONLINE_VPSS_OFFLINE == enMastPipeMode
                    || VI_PARALLEL_VPSS_OFFLINE == enMastPipeMode
                    || VI_MULTI_VPSS_OFFLINE == enMastPipeMode)
                {
                    s32Ret = SC_MPI_VI_EnableChn(ViPipe, ViChn);

                    if (s32Ret != SC_SUCCESS)
                    {
                        SAMPLE_PRT("SC_MPI_VI_EnableChn failed with %#x!\n", s32Ret);
                        return SC_FAILURE;
                    }
                }
            }
        }
    }

    return s32Ret;
}

SC_S32 SAMPLE_COMM_VI_StopViChn(SAMPLE_VI_INFO_S *pstViInfo)
{
    SC_S32              i;
    SC_BOOL             bNeedChn;
    SC_S32              s32Ret = SC_SUCCESS;
    VI_PIPE             ViPipe;
    VI_CHN              ViChn;
    VI_VPSS_MODE_E      enMastPipeMode;

    for (i = 0; i < 4; i++)
    {
        if (pstViInfo->stPipeInfo.aPipe[i] >= 0  && pstViInfo->stPipeInfo.aPipe[i] < VI_MAX_PIPE_NUM)
        {
            ViPipe = pstViInfo->stPipeInfo.aPipe[i];
            ViChn  = pstViInfo->stChnInfo.ViChn;

            if (WDR_MODE_NONE == pstViInfo->stDevInfo.enWDRMode)
            {
                bNeedChn = SC_TRUE;
            }
            else
            {
                bNeedChn = (i > 0) ? SC_FALSE : SC_TRUE;
            }

            if (bNeedChn)
            {
                enMastPipeMode = pstViInfo->stPipeInfo.enMastPipeMode;

                if (VI_OFFLINE_VPSS_OFFLINE == enMastPipeMode
                    || VI_ONLINE_VPSS_OFFLINE == enMastPipeMode
                    || VI_PARALLEL_VPSS_OFFLINE == enMastPipeMode
                    || VI_MULTI_VPSS_OFFLINE == enMastPipeMode)
                {
                    s32Ret = SC_MPI_VI_DisableChn(ViPipe, ViChn);

                    if (s32Ret != SC_SUCCESS)
                    {
                        SAMPLE_PRT("SC_MPI_VI_DisableChn failed with %#x!\n", s32Ret);
                        return SC_FAILURE;
                    }
                }
            }
        }
    }

    return s32Ret;
}

static SC_S32 SAMPLE_COMM_VI_CreateSingleVi(SAMPLE_VI_INFO_S *pstViInfo)
{
    SC_S32 s32Ret = SC_SUCCESS;

    if (!pstViInfo)
    {
        SAMPLE_PRT("%s: null pointer!\n", __FUNCTION__);
        return SC_FAILURE;
    }

    s32Ret = SAMPLE_COMM_VI_StartDev(pstViInfo);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_StartDev failed!\n");
        return SC_FAILURE;
    }

    //we should bind pipe,then creat pipe
    s32Ret = SAMPLE_COMM_VI_BindPipeDev(pstViInfo);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_BindPipeDev failed!\n");
        goto EXIT1;
    }

    s32Ret = SAMPLE_COMM_VI_CreateViPipe(pstViInfo);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_CreateViPipe failed!\n");
        goto EXIT1;
    }

    s32Ret = SAMPLE_COMM_VI_CreateIsp(pstViInfo);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_CreateIsp failed!\n");
        goto EXIT1;
    }

    s32Ret = SAMPLE_COMM_VI_StartViPipe(pstViInfo);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_StartViPipe failed!\n");
        goto EXIT1;
    }

    s32Ret = SAMPLE_COMM_VI_StartIsp(pstViInfo);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_StartIsp failed!\n");
        goto EXIT2;
    }

    s32Ret = SAMPLE_COMM_VI_StartViChn(pstViInfo);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_StartViChn failed!\n");
        goto EXIT2;
    }

    return SC_SUCCESS;

EXIT2:
    SAMPLE_COMM_VI_StopIsp(pstViInfo);
    SAMPLE_COMM_VI_StopViPipe(pstViInfo);

EXIT1:
    SAMPLE_COMM_VI_StopDev(pstViInfo);

    return s32Ret;
}

static SC_S32 SAMPLE_COMM_ModeSwitch_VI_CreateSingleVi(SAMPLE_VI_INFO_S *pstViInfo)
{
    SC_S32 s32Ret = SC_SUCCESS;

    s32Ret = SAMPLE_COMM_VI_StartDev(pstViInfo);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_StartDev failed !\n");
        return SC_FAILURE;
    }

    //we should bind pipe,then creat pipe
    s32Ret = SAMPLE_COMM_VI_BindPipeDev(pstViInfo);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_BindPipeDev failed !\n");
        goto EXIT1;
    }

    s32Ret = SAMPLE_COMM_VI_ModeSwitch_StartViPipe(pstViInfo);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_ModeSwitch_StartViPipe failed !\n");
        goto EXIT1;
    }

    s32Ret = SAMPLE_COMM_VI_ModeSwitch_StartViChn(pstViInfo);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_StartViChn failed !\n");
        goto EXIT2;
    }

    return SC_SUCCESS;

EXIT2:
    SAMPLE_COMM_VI_StopViPipe(pstViInfo);

EXIT1:
    SAMPLE_COMM_VI_StopDev(pstViInfo);

    return s32Ret;
}

static SC_S32 SAMPLE_COMM_VI_StartPipe_Chn(SAMPLE_VI_INFO_S *pstViInfo)
{
    SC_S32 s32Ret = SC_SUCCESS;

    s32Ret = SAMPLE_COMM_VI_ModeSwitch_EnableViPipe(pstViInfo);

    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_ModeSwitch_EnableViPipe failed !\n");
        goto EXIT1;
    }

    s32Ret = SAMPLE_COMM_VI_StartViChn(pstViInfo);

    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_StartViChn failed !\n");
        goto EXIT2;
    }

    return SC_SUCCESS;

EXIT2:
    SAMPLE_COMM_VI_StopViPipe(pstViInfo);

EXIT1:
    SAMPLE_COMM_VI_StopDev(pstViInfo);

    return s32Ret;
}

static SC_S32 SAMPLE_COMM_VI_DestroySingleVi(SAMPLE_VI_INFO_S *pstViInfo)
{
    SAMPLE_COMM_VI_StopViChn(pstViInfo);

    SAMPLE_COMM_VI_StopIsp(pstViInfo);

    SAMPLE_COMM_VI_StopViPipe(pstViInfo);

    SAMPLE_COMM_VI_StopDev(pstViInfo);

    return SC_SUCCESS;
}

static SC_S32 SAMPLE_COMM_VI_DestroySinglePipe_Chn(SAMPLE_VI_INFO_S *pstViInfo)
{
    SAMPLE_COMM_VI_StopViChn(pstViInfo);

    SAMPLE_COMM_VI_StopViPipe(pstViInfo);

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_VI_CreateVi(SAMPLE_VI_CONFIG_S *pstViConfig)
{
    SC_S32              i, j;
    SC_S32              s32ViNum;
    SC_S32              s32Ret = SC_SUCCESS;
    SAMPLE_VI_INFO_S   *pstViInfo = SC_NULL;

    if (!pstViConfig)
    {
        SAMPLE_PRT("%s: null ptr\n", __FUNCTION__);
        return SC_FAILURE;
    }

    for (i = 0; i < pstViConfig->s32WorkingViNum; i++)
    {
        s32ViNum  = pstViConfig->as32WorkingViId[i];
        pstViInfo = &pstViConfig->astViInfo[s32ViNum];

        s32Ret = SAMPLE_COMM_VI_CreateSingleVi(pstViInfo);

        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("SAMPLE_COMM_VI_CreateSingleVi failed !\n");
            goto EXIT;
        }
    }

    return SC_SUCCESS;

EXIT:
    for (j = 0; j < i; j++)
    {
        s32ViNum  = pstViConfig->as32WorkingViId[j];
        pstViInfo = &pstViConfig->astViInfo[s32ViNum];

        SAMPLE_COMM_VI_DestroySingleVi(pstViInfo);
    }

    return s32Ret;
}

SC_S32 SAMPLE_COMM_ModeSwitch_VI_CreateVi(SAMPLE_VI_CONFIG_S *pstViConfig)
{
    SC_S32              i, j;
    SC_S32              s32ViNum;
    SC_S32              s32Ret = SC_SUCCESS;
    SAMPLE_VI_INFO_S   *pstViInfo = SC_NULL;

    if (!pstViConfig)
    {
        SAMPLE_PRT("%s: null ptr\n", __FUNCTION__);
        return SC_FAILURE;
    }

    for (i = 0; i < pstViConfig->s32WorkingViNum; i++)
    {
        s32ViNum  = pstViConfig->as32WorkingViId[i];
        pstViInfo = &pstViConfig->astViInfo[s32ViNum];

        s32Ret = SAMPLE_COMM_ModeSwitch_VI_CreateSingleVi(pstViInfo);

        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("SAMPLE_COMM_VI_CreateSingleVi failed !\n");
            goto EXIT;
        }
    }

    return SC_SUCCESS;
EXIT:

    for (j = 0; j < i; j++)
    {
        s32ViNum  = pstViConfig->as32WorkingViId[j];
        pstViInfo = &pstViConfig->astViInfo[s32ViNum];

        SAMPLE_COMM_VI_DestroySingleVi(pstViInfo);
    }

    return s32Ret;
}

SC_S32 SAMPLE_COMM_ModeSwitch_VI_StartPipe_Chn(SAMPLE_VI_CONFIG_S *pstViConfig)
{
    SC_S32              i, j;
    SC_S32              s32ViNum;
    SC_S32              s32Ret = SC_SUCCESS;
    SAMPLE_VI_INFO_S   *pstViInfo = SC_NULL;

    if (!pstViConfig)
    {
        SAMPLE_PRT("%s: null ptr\n", __FUNCTION__);
        return SC_FAILURE;
    }

    for (i = 0; i < pstViConfig->s32WorkingViNum; i++)
    {
        s32ViNum  = pstViConfig->as32WorkingViId[i];
        pstViInfo = &pstViConfig->astViInfo[s32ViNum];

        s32Ret = SAMPLE_COMM_VI_StartPipe_Chn(pstViInfo);

        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("SAMPLE_COMM_VI_CreateSingleVi failed !\n");
            goto EXIT;
        }
    }

    return SC_SUCCESS;
EXIT:

    for (j = 0; j < i; j++)
    {
        s32ViNum  = pstViConfig->as32WorkingViId[j];
        pstViInfo = &pstViConfig->astViInfo[s32ViNum];

        SAMPLE_COMM_VI_DestroySinglePipe_Chn(pstViInfo);
    }

    return s32Ret;
}

SC_S32 SAMPLE_COMM_VI_DestroyVi(SAMPLE_VI_CONFIG_S *pstViConfig)
{
    SC_S32            i;
    SC_S32            s32ViNum;
    SAMPLE_VI_INFO_S *pstViInfo = SC_NULL;

    if (!pstViConfig)
    {
        SAMPLE_PRT("%s: null ptr\n", __FUNCTION__);
        return SC_FAILURE;
    }

    for (i = pstViConfig->s32WorkingViNum - 1; i >= 0; i--)
    {
        s32ViNum  = pstViConfig->as32WorkingViId[i];
        pstViInfo = &pstViConfig->astViInfo[s32ViNum];

        SAMPLE_COMM_VI_DestroySingleVi(pstViInfo);
    }

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_VI_DestroyIsp(SAMPLE_VI_CONFIG_S *pstViConfig)
{
    SC_S32              i;
    SC_S32              s32ViNum;
    SC_S32              s32Ret = SC_SUCCESS;
    SAMPLE_VI_INFO_S   *pstViInfo = SC_NULL;

    if (!pstViConfig)
    {
        SAMPLE_PRT("%s: null ptr\n", __FUNCTION__);
        return SC_FAILURE;
    }

    for (i = 0; i < pstViConfig->s32WorkingViNum; i++)
    {
        s32ViNum  = pstViConfig->as32WorkingViId[i];
        pstViInfo = &pstViConfig->astViInfo[s32ViNum];

        s32Ret = SAMPLE_COMM_VI_StopIsp(pstViInfo);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("SAMPLE_COMM_VI_StopIsp failed !\n");
            return SC_FAILURE;
        }
    }

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_VI_StartVi(SAMPLE_VI_CONFIG_S *pstViConfig)
{
    SC_S32 s32Ret = SC_SUCCESS;

    if (!pstViConfig)
    {
        SAMPLE_PRT("%s: null ptr\n", __FUNCTION__);
        return SC_FAILURE;
    }

    s32Ret = SAMPLE_COMM_VI_StartMIPI(pstViConfig);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_StartMIPI failed!\n");
        return SC_FAILURE;
    }

    s32Ret = SAMPLE_COMM_VI_SetParam(pstViConfig);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_SetParam failed!\n");
        return SC_FAILURE;
    }

    s32Ret = SAMPLE_COMM_VI_CreateVi(pstViConfig);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_CreateVi failed!\n");
        return SC_FAILURE;
    }

    return s32Ret;
}

SC_S32 SAMPLE_COMM_VI_StopVi(SAMPLE_VI_CONFIG_S *pstViConfig)
{
    SC_S32 s32Ret = SC_SUCCESS;

    s32Ret = SAMPLE_COMM_VI_DestroyVi(pstViConfig);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_DestroyVi failed !\n");
        return SC_FAILURE;
    }

    s32Ret = SAMPLE_COMM_VI_StopMIPI(pstViConfig);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_StopMIPI failed !\n");
        return SC_FAILURE;
    }

    return s32Ret;
}

SC_S32 SAMPLE_COMM_VI_SwitchISPMode(SAMPLE_VI_CONFIG_S *pstViConfig)
{
    SC_S32              i;
    SC_S32              s32ViNum;
    SC_S32              s32Ret = SC_SUCCESS;
    SC_BOOL             bNeedPipe;
    SC_BOOL             bSwitchWDR[VI_MAX_PIPE_NUM] = { SC_FALSE };
    VI_PIPE             ViPipe = 0;
    ISP_PUB_ATTR_S      stPubAttr;
    SAMPLE_VI_INFO_S   *pstViInfo = SC_NULL;
    //ISP_INNER_STATE_INFO_S stInnerStateInfo;
    ISP_PUB_ATTR_S      stPrePubAttr;
    //SC_BOOL bSwitchFinish;

    if (!pstViConfig)
    {
        SAMPLE_PRT("%s: null ptr\n", __FUNCTION__);
        return SC_FAILURE;
    }

    for (i = 0; i < pstViConfig->s32WorkingViNum; i++)
    {
        s32ViNum  = pstViConfig->as32WorkingViId[i];
        pstViInfo = &pstViConfig->astViInfo[s32ViNum];

        for (i = 0; i < WDR_MAX_PIPE_NUM; i++)
        {
            if ( pstViInfo->stPipeInfo.aPipe[i] >= 0 && pstViInfo->stPipeInfo.aPipe[i] < VI_MAX_PIPE_NUM)
            {

                SAMPLE_COMM_ISP_GetIspAttrBySns(pstViInfo->stSnsInfo.enSnsType, &stPubAttr);

                stPubAttr.enWDRMode =  pstViInfo->stDevInfo.enWDRMode;

                SAMPLE_PRT("SAMPLE_COMM_VI_CreateIsp enWDRMode is %d!\n", stPubAttr.enWDRMode);

                if (WDR_MODE_NONE == pstViInfo->stDevInfo.enWDRMode )
                {
                    bNeedPipe = SC_TRUE;
                }
                else
                {
                    bNeedPipe = (i > 0) ? SC_FALSE : SC_TRUE;
                }

                if (SC_TRUE != bNeedPipe)
                {
                    continue;
                }

                ViPipe = pstViInfo->stPipeInfo.aPipe[i];
                s32Ret = SC_MPI_ISP_GetPubAttr(ViPipe, &stPrePubAttr);
                if (s32Ret != SC_SUCCESS)
                {
                    SAMPLE_PRT("GetPubAttr failed with %#x!\n", s32Ret);
                    SAMPLE_COMM_ISP_Stop(ViPipe);
                    return SC_FAILURE;
                }

                s32Ret = SC_MPI_ISP_SetPubAttr(ViPipe, &stPubAttr);
                if (s32Ret != SC_SUCCESS)
                {
                    SAMPLE_PRT("SetPubAttr failed with %#x!\n", s32Ret);
                    SAMPLE_COMM_ISP_Stop(ViPipe);
                    return SC_FAILURE;
                }
                if (stPrePubAttr.enWDRMode != stPubAttr.enWDRMode)
                {
                    bSwitchWDR[ViPipe] = SC_TRUE;
                    bSwitchWDR[ViPipe] = bSwitchWDR[ViPipe];
                }
            }
        }
    }

    #if 0
    while (1)
    {
        SC_MPI_ISP_QueryInnerStateInfo(ViPipe, &stInnerStateInfo);
        if ((SC_TRUE == stInnerStateInfo.bResSwitchFinish)
            || (SC_TRUE == stInnerStateInfo.bWDRSwitchFinish))
        {
            SAMPLE_PRT("Switch finish!\n");
            break;
        }

        usleep(1000);
    }
    #endif

    SAMPLE_COMM_VI_StartPipe_Chn(pstViInfo);

    return SC_SUCCESS;
}

SC_S32  SAMPLE_COMM_VI_SwitchMode_StopVI(SAMPLE_VI_CONFIG_S *pstViConfigSrc)
{

    SC_S32              s32Ret = SC_SUCCESS;

    s32Ret = SAMPLE_COMM_VI_DestroyVi(pstViConfigSrc);

    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_DestroyVi failed !\n");
        return SC_FAILURE;
    }

    s32Ret = SAMPLE_COMM_VI_StopMIPI(pstViConfigSrc);

    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_StopMIPI failed !\n");
        return SC_FAILURE;
    }

    return SC_SUCCESS;

}

SC_S32  SAMPLE_COMM_VI_SwitchMode(SAMPLE_VI_CONFIG_S *pstViConfigDes)
{

    SC_S32 s32Ret = SC_SUCCESS;

    s32Ret = SAMPLE_COMM_VI_StartMIPI(pstViConfigDes);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_StartMIPI failed!\n");
        return SC_FAILURE;
    }

    /*   create vi without enable chn and enable pipe. */
    s32Ret =  SAMPLE_COMM_ModeSwitch_VI_CreateVi(pstViConfigDes);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_CreateVi failed!\n");
        return SC_FAILURE;
    }

    s32Ret = SAMPLE_COMM_VI_SwitchISPMode(pstViConfigDes);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_ModeSwitch_VI_CreateIsp!\n");
        return SC_FAILURE;
    }

    return SC_SUCCESS;
}

int SAMPLE_COMM_VI_ExitMpp( int s32poolId)
{
    if (s32poolId < 0)
    {
        if (SC_MPI_SYS_Exit())
        {
            SAMPLE_PRT("sys exit fail\n");
            return -1;
        }

        if (SC_MPI_VB_Exit())
        {
            SAMPLE_PRT("vb exit fail\n");
            return -1;
        }

        return -1;
    }

    return 0;
}

/******************************************************************************
* funciton : Get enWDRMode by diffrent sensor
******************************************************************************/
SC_S32 SAMPLE_COMM_VI_GetWDRModeBySensor(SAMPLE_SNS_TYPE_E enMode, WDR_MODE_E *penWDRMode)
{
    SC_S32 s32Ret = SC_SUCCESS;

    if (!penWDRMode)
    {
        return SC_FAILURE;
    }

    switch (enMode)
    {
    case SONY_IMX307_MIPI_2M_30FPS_12BIT_WDR2TO1:
    case SMART_SC2310_MIPI_2M_30FPS_12BIT_WDR2TO1:
    case GALAXYCORE_GC2093_MIPI_2M_30FPS_10BIT_WDR2TO1:
    case SONY_IMX415_MIPI_8M_30FPS_12BIT_WDR2TO1:
        *penWDRMode = WDR_MODE_2To1_LINE;
        break;

    default:
        *penWDRMode = WDR_MODE_NONE;
        break;
    }

    return s32Ret;
}

/******************************************************************************
* funciton : Get Pipe by diffrent sensor
******************************************************************************/
SC_S32 SAMPLE_COMM_VI_GetPipeBySensor(SAMPLE_SNS_TYPE_E enMode, SAMPLE_PIPE_INFO_S *pstPipeInfo)
{
    SC_S32 s32Ret = SC_SUCCESS;

    if (!pstPipeInfo)
    {
        return SC_FAILURE;
    }

    switch (enMode)
    {
    default:
        pstPipeInfo->enMastPipeMode = VI_ONLINE_VPSS_OFFLINE;
        pstPipeInfo->aPipe[0] = 0;
        pstPipeInfo->aPipe[1] = 1;
        pstPipeInfo->aPipe[2] = -1;
        pstPipeInfo->aPipe[3] = -1;
        break;
    }

    return s32Ret;
}

/******************************************************************************
* funciton : Get enSize by diffrent sensor
******************************************************************************/
SC_S32 SAMPLE_COMM_VI_GetSizeBySensor(SAMPLE_SNS_TYPE_E enMode, PIC_SIZE_E *penSize)
{
    SC_S32 s32Ret = SC_SUCCESS;

    if (!penSize)
    {
        return SC_FAILURE;
    }

    switch (enMode)
    {
    case SONY_IMX307_MIPI_2M_30FPS_12BIT:
    case SONY_IMX307_MIPI_2M_30FPS_12BIT_WDR2TO1:
    case SONY_IMX291_MIPI_2M_30FPS_12BIT:
    case SONY_IMX291_MIPI_2M_60FPS_10BIT:
    case SONY_IMX291_MIPI_2M_120FPS_10BIT:
    case SMART_SC2310_MIPI_2M_30FPS_12BIT:
    case SMART_SC2210_MIPI_2M_30FPS_12BIT:
    case SMART_SC2310_MIPI_2M_30FPS_12BIT_WDR2TO1:
    case GALAXYCORE_GC2053_MIPI_2M_30FPS_10BIT:
    case GALAXYCORE_GC2093_MIPI_2M_30FPS_10BIT:
    case GALAXYCORE_GC2093_MIPI_2M_30FPS_10BIT_WDR2TO1:
    case OMNIVISION_OV13B10_MIPI_2M_30FPS_10BIT:
    case ISP_VIN_0_MEM_2M_30FPS_12BIT:
    case TP9930_DVP_4VC_1080P_25FPS:
        *penSize = PIC_1080P;
        break;

    case SONY_IMX415_MIPI_8M_30FPS_12BIT:
    case SONY_IMX415_MIPI_8M_60FPS_12BIT:
    case SONY_IMX415_MIPI_8M_30FPS_12BIT_WDR2TO1:
        *penSize = PIC_3840x2160;
        break;

    case GALAXYCORE_GC1603_MIPI_1M_60FPS_10BIT:
    case GALAXYCORE_GC1603_MIPI_1M_120FPS_10BIT:
        *penSize = PIC_720P;
        break;

    case SMART_SC4210_MIPI_4M_30FPS_12BIT:
        *penSize = PIC_2560x1440;
        break;

    case OMNIVISION_OV13B10_MIPI_13M_1FPS_10BIT:
        *penSize = PIC_4096x3120;
        break;

    default:
        *penSize = PIC_3840x2160;
        break;
    }

    return s32Ret;
}

SC_S32 SAMPLE_COMM_VI_GetRawSizeBySensor(SAMPLE_SNS_TYPE_E enMode, PIC_SIZE_E *penSize)
{
    SC_S32 s32Ret = SC_SUCCESS;

    if (!penSize)
    {
        return SC_FAILURE;
    }

    switch (enMode)
    {
    case SONY_IMX307_MIPI_2M_30FPS_12BIT:
    case SONY_IMX307_MIPI_2M_30FPS_12BIT_WDR2TO1:
    case SONY_IMX291_MIPI_2M_30FPS_12BIT:
    case SONY_IMX291_MIPI_2M_60FPS_10BIT:
    case SONY_IMX291_MIPI_2M_120FPS_10BIT:
    case SMART_SC2310_MIPI_2M_30FPS_12BIT:
    case SMART_SC2210_MIPI_2M_30FPS_12BIT:
    case SMART_SC2310_MIPI_2M_30FPS_12BIT_WDR2TO1:
    case GALAXYCORE_GC2053_MIPI_2M_30FPS_10BIT:
    case GALAXYCORE_GC2093_MIPI_2M_30FPS_10BIT:
    case GALAXYCORE_GC2093_MIPI_2M_30FPS_10BIT_WDR2TO1:
    case OMNIVISION_OV13B10_MIPI_2M_30FPS_10BIT:
    case ISP_VIN_0_MEM_2M_30FPS_12BIT:
    case TP9930_DVP_4VC_1080P_25FPS:
        *penSize = PIC_1080P;
        break;

    case SONY_IMX415_MIPI_8M_30FPS_12BIT:
    case SONY_IMX415_MIPI_8M_60FPS_12BIT:
    case SONY_IMX415_MIPI_8M_30FPS_12BIT_WDR2TO1:
        *penSize = PIC_3864x2192;
        break;

    case GALAXYCORE_GC1603_MIPI_1M_60FPS_10BIT:
    case GALAXYCORE_GC1603_MIPI_1M_120FPS_10BIT:
        *penSize = PIC_720P;
        break;

    case SMART_SC4210_MIPI_4M_30FPS_12BIT:
        *penSize = PIC_2560x1440;
        break;

    case OMNIVISION_OV13B10_MIPI_13M_1FPS_10BIT:
        *penSize = PIC_4096x3120;
        break;

    default:
        *penSize = PIC_3840x2160;
        break;
    }

    return s32Ret;
}

SC_S32 SAMPLE_COMM_VI_GetRawPicSize(SAMPLE_SNS_TYPE_E enMode, SIZE_S *pstSize)
{
    SC_S32 s32Ret = SC_SUCCESS;
    PIC_SIZE_E enPicSize = 0;

    s32Ret = SAMPLE_COMM_VI_GetRawSizeBySensor(enMode, &enPicSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, pstSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }

    return s32Ret;
}

/******************************************************************************
* funciton : Get enSize by diffrent sensor
******************************************************************************/
SC_S32 SAMPLE_COMM_VI_GetFrameRateBySensor(SAMPLE_SNS_TYPE_E enMode, SC_U32 *pu32FrameRate)
{
    SC_S32 s32Ret = SC_SUCCESS;

    if (!pu32FrameRate)
    {
        return SC_FAILURE;
    }

    switch (enMode)
    {
        case OMNIVISION_OV13B10_MIPI_13M_1FPS_10BIT:
        {
            *pu32FrameRate = 1;
            break;
        }

        case SONY_IMX307_MIPI_2M_30FPS_12BIT_WDR2TO1:
        case SMART_SC2310_MIPI_2M_30FPS_12BIT_WDR2TO1:
        case GALAXYCORE_GC2093_MIPI_2M_30FPS_10BIT_WDR2TO1:
        case SONY_IMX415_MIPI_8M_30FPS_12BIT_WDR2TO1:
        case TP9930_DVP_4VC_1080P_25FPS:
        {
            *pu32FrameRate = 25;
            break;
        }

        case SONY_IMX307_MIPI_2M_30FPS_12BIT:
        case SONY_IMX291_MIPI_2M_30FPS_12BIT:
        case SMART_SC2310_MIPI_2M_30FPS_12BIT:
        case SMART_SC2210_MIPI_2M_30FPS_12BIT:
        case SMART_SC4210_MIPI_4M_30FPS_12BIT:
        case SONY_IMX415_MIPI_8M_30FPS_12BIT:
        case GALAXYCORE_GC2053_MIPI_2M_30FPS_10BIT:
        case GALAXYCORE_GC2093_MIPI_2M_30FPS_10BIT:
        case OMNIVISION_OV13B10_MIPI_2M_30FPS_10BIT:
        case ISP_VIN_0_MEM_2M_30FPS_12BIT:
        {
            *pu32FrameRate = 30;
            break;
        }

        case SONY_IMX415_MIPI_8M_60FPS_12BIT:
        case GALAXYCORE_GC1603_MIPI_1M_60FPS_10BIT:
        case SONY_IMX291_MIPI_2M_60FPS_10BIT:
        {
            *pu32FrameRate = 60;
            break;
        }

        case GALAXYCORE_GC1603_MIPI_1M_120FPS_10BIT:
        case SONY_IMX291_MIPI_2M_120FPS_10BIT:
        {
            *pu32FrameRate = 120;
            break;
        }

        default:
        {
            *pu32FrameRate = 30;
            break;
        }
    }

    return s32Ret;
}

SC_VOID SAMPLE_COMM_VI_GetSensorInfo(SAMPLE_VI_CONFIG_S *pstViConfig)
{
    SC_S32 i;

    for (i = 0; i < VI_MAX_DEV_NUM; i++)
    {
        pstViConfig->astViInfo[i].stSnsInfo.s32SnsId = i;
        pstViConfig->astViInfo[i].stSnsInfo.s32BusId = i;
        pstViConfig->astViInfo[i].stSnsInfo.MipiDev  = i;
        memset_s(&pstViConfig->astViInfo[i].stSnapInfo, sizeof(SAMPLE_SNAP_INFO_S), 0, sizeof(SAMPLE_SNAP_INFO_S));
        pstViConfig->astViInfo[i].stPipeInfo.bMultiPipe = SC_FALSE;
        pstViConfig->astViInfo[i].stPipeInfo.bVcNumCfged = SC_FALSE;
    }

    /* single sensor using I2C,  s32BusId must be set to 1
     * single sensor using spi,  s32BusId must be set to 0
     *
     * if add sensor using spi into SAMPLE_SNS_TYPE_E,
     * please add code branch with sensor type
     */
    pstViConfig->astViInfo[0].stSnsInfo.s32BusId = gSnsGpio[0].i2c_id;
    pstViConfig->astViInfo[1].stSnsInfo.s32BusId = gSnsGpio[1].i2c_id;
    pstViConfig->astViInfo[2].stSnsInfo.s32BusId = gSnsGpio[2].i2c_id;
    pstViConfig->astViInfo[3].stSnsInfo.s32BusId = gSnsGpio[3].i2c_id;

    pstViConfig->astViInfo[0].stSnsInfo.enSnsType = SENSOR0_TYPE;
    pstViConfig->astViInfo[1].stSnsInfo.enSnsType = SENSOR1_TYPE;
    pstViConfig->astViInfo[2].stSnsInfo.enSnsType = SENSOR2_TYPE;
    pstViConfig->astViInfo[3].stSnsInfo.enSnsType = SENSOR3_TYPE;
}

combo_dev_t SAMPLE_COMM_VI_GetComboDevBySensor(SAMPLE_SNS_TYPE_E enMode, SC_S32 s32SnsIdx)
{
    combo_dev_t dev = 0;

    switch (enMode)
    {
    case SONY_IMX307_MIPI_2M_30FPS_12BIT:
    case SONY_IMX307_MIPI_2M_30FPS_12BIT_WDR2TO1:
    case SONY_IMX291_MIPI_2M_30FPS_12BIT:
    case SONY_IMX291_MIPI_2M_60FPS_10BIT:
    case SONY_IMX291_MIPI_2M_120FPS_10BIT:
    case SMART_SC2310_MIPI_2M_30FPS_12BIT:
    case SMART_SC2210_MIPI_2M_30FPS_12BIT:
    case SMART_SC4210_MIPI_4M_30FPS_12BIT:
    case SMART_SC2310_MIPI_2M_30FPS_12BIT_WDR2TO1:
    case SONY_IMX415_MIPI_8M_30FPS_12BIT:
    case SONY_IMX415_MIPI_8M_60FPS_12BIT:
    case SONY_IMX415_MIPI_8M_30FPS_12BIT_WDR2TO1:
    case GALAXYCORE_GC2053_MIPI_2M_30FPS_10BIT:
    case GALAXYCORE_GC2093_MIPI_2M_30FPS_10BIT:
    case GALAXYCORE_GC2093_MIPI_2M_30FPS_10BIT_WDR2TO1:
    case GALAXYCORE_GC1603_MIPI_1M_60FPS_10BIT:
    case GALAXYCORE_GC1603_MIPI_1M_120FPS_10BIT:
    case OMNIVISION_OV13B10_MIPI_13M_1FPS_10BIT:
    case OMNIVISION_OV13B10_MIPI_2M_30FPS_10BIT:
    case ISP_VIN_0_MEM_2M_30FPS_12BIT:
        if (0 == s32SnsIdx)
        {
            dev = 0;
        }
        else if (1 == s32SnsIdx)
        {
            dev = 2;
        }
        else if (2 == s32SnsIdx)
        {
            dev = 4;
        }
        break;

    default:
        dev = 0;
        break;
    }

    return dev;
}

SC_S32 SAMPLE_COMM_VI_ConvertBitPixel(SC_U8 *pu8Data, SC_U32 u32DataNum, SC_U32 u32BitWidth, SC_U16 *pu16OutData)
{
    SC_S32 i, u32Tmp, s32OutCnt;
    SC_U32 u32Val;
    SC_U64 u64Val;
    SC_U8 *pu8Tmp = pu8Data;

    s32OutCnt = 0;
    switch(u32BitWidth)
    {
    case 10:
    {
        /* 4 pixels consist of 5 bytes  */
        u32Tmp = u32DataNum / 4;

        for (i = 0; i < u32Tmp; i++)
        {
            /* byte4 byte3 byte2 byte1 byte0 */
            pu8Tmp = pu8Data + 5 * i;
            u64Val = pu8Tmp[0] + ((SC_U32)pu8Tmp[1] << 8) + ((SC_U32)pu8Tmp[2] << 16) +
                ((SC_U32)pu8Tmp[3] << 24) + ((SC_U64)pu8Tmp[4] << 32);

            pu16OutData[s32OutCnt++] = u64Val & 0x3ff;
            pu16OutData[s32OutCnt++] = (u64Val >> 10) & 0x3ff;
            pu16OutData[s32OutCnt++] = (u64Val >> 20) & 0x3ff;
            pu16OutData[s32OutCnt++] = (u64Val >> 30) & 0x3ff;
        }
    }
    break;
    case 12:
    {
        /* 2 pixels consist of 3 bytes  */
        u32Tmp = u32DataNum / 2;

        for (i = 0; i < u32Tmp; i++)
        {
            /* byte2 byte1 byte0 */
            pu8Tmp = pu8Data + 3 * i;
            u32Val = pu8Tmp[0] + (pu8Tmp[1] << 8) + (pu8Tmp[2] << 16);
            pu16OutData[s32OutCnt++] = u32Val & 0xfff;
            pu16OutData[s32OutCnt++] = (u32Val >> 12) & 0xfff;
        }
    }
    break;
    case 14:
    {
        /* 4 pixels consist of 7 bytes  */
        u32Tmp = u32DataNum / 4;

        for (i = 0; i < u32Tmp; i++)
        {
            pu8Tmp = pu8Data + 7 * i;
            u64Val = pu8Tmp[0] + ((SC_U32)pu8Tmp[1] << 8) + ((SC_U32)pu8Tmp[2] << 16) +
                ((SC_U32)pu8Tmp[3] << 24) + ((SC_U64)pu8Tmp[4] << 32) +
                ((SC_U64)pu8Tmp[5] << 40) + ((SC_U64)pu8Tmp[6] << 48);

            pu16OutData[s32OutCnt++] = u64Val & 0x3fff;
            pu16OutData[s32OutCnt++] = (u64Val >> 14) & 0x3fff;
            pu16OutData[s32OutCnt++] = (u64Val >> 28) & 0x3fff;
            pu16OutData[s32OutCnt++] = (u64Val >> 42) & 0x3fff;
        }
    }
    break;
    default:
        SAMPLE_PRT("unsuport bitWidth: %d\n", u32BitWidth);
        return SC_FAILURE;
        break;
    }

    return s32OutCnt;
}

static SC_S32 SAMPLE_COMM_VI_BitWidth2PixelFormat(SC_U32 u32Nbit, PIXEL_FORMAT_E *penPixelFormat)
{
    PIXEL_FORMAT_E enPixelFormat;

    if (8 == u32Nbit)
    {
        enPixelFormat = PIXEL_FORMAT_RGB_BAYER_8BPP;
    }
    else if (10 == u32Nbit)
    {
        enPixelFormat = PIXEL_FORMAT_RGB_BAYER_10BPP;
    }
    else if (12 == u32Nbit)
    {
        enPixelFormat = PIXEL_FORMAT_RGB_BAYER_12BPP;
    }
    else if (14 == u32Nbit)
    {
        enPixelFormat = PIXEL_FORMAT_RGB_BAYER_14BPP;
    }
    else if (16 == u32Nbit)
    {
        enPixelFormat = PIXEL_FORMAT_RGB_BAYER_16BPP;
    }
    else
    {
        return SC_FAILURE;
    }

    *penPixelFormat = enPixelFormat;
    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_VI_SaveUncompressRaw(VIDEO_FRAME_S *pVBuf, SC_U32 u32Nbit, FILE *pfd)
{
    SC_U32  u32Height;
    SC_U64  u64PhyAddr;
    SC_U64  u64Size;
    SC_U8  *pu8VirAddr;
    SC_U16 *pu16Data = NULL;
    SC_U8  *pu8Data;
    PIXEL_FORMAT_E enPixelFormat = PIXEL_FORMAT_BUTT;

    SAMPLE_COMM_VI_BitWidth2PixelFormat(u32Nbit, &enPixelFormat);
    if (enPixelFormat != pVBuf->enPixelFormat)
    {
        SAMPLE_PRT("invalid pixel format:%d, u32Nbit: %d\n", pVBuf->enPixelFormat, u32Nbit);

        return SC_FAILURE;
    }

    u64Size = (pVBuf->u32Stride[0]) * ((SC_U64)pVBuf->u32Height);
    u64PhyAddr = pVBuf->u64PhyAddr[0];

    pu8VirAddr = (SC_U8 *) SC_MPI_SYS_Mmap(u64PhyAddr, u64Size);
    if (NULL == pu8VirAddr)
    {
        SAMPLE_PRT("SC_MPI_SYS_Mmap fail !\n");

        return SC_FAILURE;
    }

    pu8Data = pu8VirAddr;
    if ((8 != u32Nbit) && (16 != u32Nbit))
    {
        pu16Data = (SC_U16 *)malloc(pVBuf->u32Width * 2U);
        if (NULL == pu16Data)
        {
            SAMPLE_PRT("alloc memory failed\n");

            SC_MPI_SYS_Munmap(pu8VirAddr, u64Size);
            pu8VirAddr = NULL;
            return SC_FAILURE;
        }
    }

    /* save Y ----------------------------------------------------------------*/
    SAMPLE_PRT("saving......dump data......u32Stride[0]: %d, width: %d\n", pVBuf->u32Stride[0], pVBuf->u32Width);

    for (u32Height = 0; u32Height < pVBuf->u32Height; u32Height++)
    {
        if (8 == u32Nbit)
        {
            fwrite(pu8Data, pVBuf->u32Width, 1, pfd);
        }
        else if (16 == u32Nbit)
        {
            fwrite(pu8Data, pVBuf->u32Width, 2, pfd);
            fflush(pfd);
        }
        else
        {
            SAMPLE_COMM_VI_ConvertBitPixel(pu8Data, pVBuf->u32Width, u32Nbit, pu16Data);
            fwrite(pu16Data, pVBuf->u32Width, 2, pfd);
        }
        pu8Data += pVBuf->u32Stride[0];
    }
    fflush(pfd);

    SAMPLE_PRT("done u32TimeRef: %d!\n", pVBuf->u32TimeRef);

    if (NULL != pu16Data)
    {
        free(pu16Data);
    }
    SC_MPI_SYS_Munmap(pu8VirAddr, u64Size);
    pu8VirAddr = NULL;

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_VI_SaveCompressedRaw(VIDEO_FRAME_S *pVBuf, SC_U32 u32Nbit, FILE *pfd)
{
    SC_U32  u32Height;
    SC_U64  u64PhyAddr;
    SC_U64  u64Size;
    SC_U32  u32DataSize;
    SC_U16  u16HeadData = 0x0;
    SC_U8  *pu8VirAddr;
    SC_U8  *pu8Data;
    PIXEL_FORMAT_E enPixelFormat = PIXEL_FORMAT_BUTT;

    SAMPLE_COMM_VI_BitWidth2PixelFormat(u32Nbit, &enPixelFormat);
    if (enPixelFormat != pVBuf->enPixelFormat)
    {
        SAMPLE_PRT("invalid pixel format:%d, u32Nbit: %d\n", pVBuf->enPixelFormat, u32Nbit);

        return SC_FAILURE;
    }

    u64Size = (pVBuf->u32Stride[0]) * ((SC_U64)pVBuf->u32Height);
    u64PhyAddr = pVBuf->u64PhyAddr[0];

    pu8VirAddr = (SC_U8 *) SC_MPI_SYS_Mmap(u64PhyAddr, u64Size);
    if (NULL == pu8VirAddr)
    {
        SAMPLE_PRT("SC_MPI_SYS_Mmap fail !\n");

        return SC_FAILURE;
    }

    pu8Data = pu8VirAddr;

    /* save Y ----------------------------------------------------------------*/
    SAMPLE_PRT("saving......dump data......u32Stride[0]: %d, width: %d\n", pVBuf->u32Stride[0], pVBuf->u32Width);

    for (u32Height = 0; u32Height < pVBuf->u32Height; u32Height++)
    {
        u16HeadData = *(SC_U16 *)pu8Data;

        u32DataSize =  (u16HeadData + 1) * 16;

        fwrite(pu8Data, u32DataSize, 1, pfd);

        pu8Data += pVBuf->u32Stride[0];
    }

    fflush(pfd);

    SAMPLE_PRT("done u32TimeRef: %d!\n", pVBuf->u32TimeRef);

    SC_MPI_SYS_Munmap(pu8VirAddr, u64Size);
    pu8VirAddr = NULL;

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_VI_SaveRaw(VIDEO_FRAME_S *pVBuf, SC_U32 u32Nbit, FILE *pfd)
{
    if(COMPRESS_MODE_NONE == pVBuf->enCompressMode)
    {
        return SAMPLE_COMM_VI_SaveUncompressRaw(pVBuf, u32Nbit, pfd);
    }
    else
    {
        return SAMPLE_COMM_VI_SaveCompressedRaw(pVBuf, u32Nbit, pfd);
    }

}
SC_U32 SAMPLE_COMM_VI_PixelFormat2BitWidth(PIXEL_FORMAT_E  enPixelFormat)
{
    switch (enPixelFormat)
    {
    case PIXEL_FORMAT_RGB_BAYER_8BPP:
        return 8;

    case PIXEL_FORMAT_RGB_BAYER_10BPP:
        return 10;

    case PIXEL_FORMAT_RGB_BAYER_12BPP:
        return 12;

    case PIXEL_FORMAT_RGB_BAYER_14BPP:
        return 14;

    case PIXEL_FORMAT_RGB_BAYER_16BPP:
        return 16;

    default:
        return 0;
    }

}

char *SAMPLE_COMM_VI_CompressMode2String(COMPRESS_MODE_E enCompressMode)
{
    if(COMPRESS_MODE_NONE == enCompressMode)
    {
        return "COMPRESS_MODE_NONE";
    }
    else if(COMPRESS_MODE_DPCM_6BITS == enCompressMode)
    {
        return "COMPRESS_MODE_DPCM_6BITS";
    }
    else if(COMPRESS_MODE_SEG == enCompressMode)
    {
        return "COMPRESS_MODE_SEG";
    }
    else if(COMPRESS_MODE_DPCM_8BITS == enCompressMode)
    {
        return "COMPRESS_MODE_DPCM_8BITS";
    }
    else if(COMPRESS_MODE_DPCM_10BITS == enCompressMode)
    {
        return "COMPRESS_MODE_DPCM_8BITS";
    }
    else
    {
        return "CMP_XXX";
    }
}

int SAMPLE_COMM_VI_SaveCompressParam(VI_CMP_PARAM_S *pCmpParam, FILE *pfd)
{
    fwrite(pCmpParam, sizeof(VI_CMP_PARAM_S), 1, pfd);
    fflush(pfd);
    return SC_SUCCESS;
}

SC_VOID *SAMPLE_COMM_VI_DumpRaw(SC_VOID *arg)
{
    SC_S32      s32Ret;
    VI_PIPE     ViPipe;
    SC_S32      s32Cnt;
    SC_S32      s32DumpCnt = 0;
    SC_U32      u32Width;
    SC_U32      u32Height;
    SC_U32      u32BitWidth;
    FILE       *pfile;
    SC_S32      s32MilliSec = 2000;
    SC_CHAR     name[256] = {0};
    VIDEO_FRAME_INFO_S stVideoFrame;
    VI_CMP_PARAM_S stCmpPara;
    SC_CHAR szThreadName[20];
    SAMPLE_VI_DUMP_THREAD_INFO_S *pstViDumpRawThreadInfo = SC_NULL;

    if (NULL == arg)
    {
        SAMPLE_PRT("arg is NULL\n");
        return NULL;
    }

    pstViDumpRawThreadInfo = (SAMPLE_VI_DUMP_THREAD_INFO_S *)arg;

    ViPipe = pstViDumpRawThreadInfo->ViPipe;
    s32Cnt = pstViDumpRawThreadInfo->s32Cnt;

    if (s32Cnt < 1)
    {
        SAMPLE_PRT("You really want to dump %d frame? That is impossible !\n", s32Cnt);
        return NULL;
    }

    snprintf(szThreadName, 20, "VI_PIPE%d_DUMP_RAW", ViPipe);
    prctl(PR_SET_NAME, szThreadName, 0, 0, 0);

    s32Ret = SC_MPI_VI_GetPipeFrame(ViPipe, &stVideoFrame, s32MilliSec);

    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_VI_GetPipeFrame failed with %#x!\n", s32Ret);
        return SC_NULL;
    }

    s32Ret = SC_MPI_VI_ReleasePipeFrame(ViPipe, &stVideoFrame);

    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_VI_ReleasePipeFrame failed with %#x!\n", s32Ret);
        return SC_NULL;
    }

    u32Width    = stVideoFrame.stVFrame.u32Width;
    u32Height   = stVideoFrame.stVFrame.u32Height;
    u32BitWidth = SAMPLE_COMM_VI_PixelFormat2BitWidth(stVideoFrame.stVFrame.enPixelFormat);

    snprintf(name, sizeof(name), "./data/%s_pipe%d_w%d_h%d_%dbits_%s_%d.raw",
        pstViDumpRawThreadInfo->aszName, ViPipe, u32Width, u32Height, u32BitWidth,
        SAMPLE_COMM_VI_CompressMode2String(stVideoFrame.stVFrame.enCompressMode), s32Cnt);
    pfile = fopen(name, "ab");

    if (NULL == pfile)
    {
        SAMPLE_PRT("open file %s fail !\n", name);
        return SC_NULL;
    }

    if(COMPRESS_MODE_NONE != stVideoFrame.stVFrame.enCompressMode)
    {
        if(SC_SUCCESS != SC_MPI_VI_GetPipeCmpParam(ViPipe, &stCmpPara))
        {
            SAMPLE_PRT("SC_MPI_VI_GetPipeCmpParam failed with %#x!\n", s32Ret);
            goto end;
        }

        SAMPLE_COMM_VI_SaveCompressParam(&stCmpPara, pfile);
    }

    while ((SC_TRUE == g_stViDumpRawThreadInfo.bDump) && (s32DumpCnt < s32Cnt))
    {
        s32Ret = SC_MPI_VI_GetPipeFrame(ViPipe, &stVideoFrame, s32MilliSec);

        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SC_MPI_VI_GetPipeFrame failed with %#x!\n", s32Ret);
            goto end;
        }

        s32Ret = SAMPLE_COMM_VI_SaveRaw(&stVideoFrame.stVFrame, u32BitWidth, pfile);

        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SAMPLE_COMM_VI_SaveRaw failed with %#x!\n", s32Ret);
            goto end;
        }

        s32DumpCnt++;

        s32Ret = SC_MPI_VI_ReleasePipeFrame(ViPipe, &stVideoFrame);

        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SC_MPI_VI_ReleasePipeFrame failed with %#x!\n", s32Ret);
            goto end;
        }
    }

end:
    fclose(pfile);
    pfile = SC_NULL;
    return SC_NULL;
}

SC_S32 SAMPLE_COMM_VI_StartDumpRawThread(VI_PIPE ViPipe, SC_S32 s32Cnt, const SC_CHAR *pzsName)
{
    SC_S32 s32Ret;
    pthread_attr_t *pattr = NULL;

    g_stViDumpRawThreadInfo.ViPipe = ViPipe;
    g_stViDumpRawThreadInfo.s32Cnt = s32Cnt;
    g_stViDumpRawThreadInfo.bDump  = SC_TRUE;
    memcpy_s(g_stViDumpRawThreadInfo.aszName, sizeof(g_stViDumpRawThreadInfo.aszName), pzsName,
        sizeof(g_stViDumpRawThreadInfo.aszName));

    s32Ret = pthread_create(&g_stViDumpRawThreadInfo.ThreadId, pattr, SAMPLE_COMM_VI_DumpRaw,
            (SC_VOID *)&g_stViDumpRawThreadInfo);

    if (0 != s32Ret)
    {
        SAMPLE_PRT("create GetViFrame thread failed! %s\n", strerror(s32Ret));
        goto out;
    }

out:

    if (NULL != pattr)
    {
        pthread_attr_destroy(pattr);
    }

    return s32Ret;

}

SC_S32 SAMPLE_COMM_VI_StopDumpRawThread(SC_VOID)
{
    if (SC_FALSE != g_stViDumpRawThreadInfo.bDump)
    {
        g_stViDumpRawThreadInfo.bDump  = SC_FALSE;
        pthread_join(g_stViDumpRawThreadInfo.ThreadId, NULL);
    }

    return SC_SUCCESS;
}

#if 0 //todo
static SC_CHAR *PRINT_FPNTYPE(ISP_FPN_TYPE_E enFpnType)
{
    if (ISP_FPN_TYPE_FRAME == enFpnType)
        return "Frame";
    else if (ISP_FPN_TYPE_LINE == enFpnType)
        return "Line";
    else
        return "NA";
}

static SC_U32 SAMPLE_VI_PixelFormat2BitWidth(VIDEO_FRAME_S *pstVFrame)
{
    switch (pstVFrame->enPixelFormat)
    {
    case PIXEL_FORMAT_RGB_BAYER_8BPP:
        return 8;

    case PIXEL_FORMAT_RGB_BAYER_10BPP:
        return 10;

    case PIXEL_FORMAT_RGB_BAYER_12BPP:
        return 12;

    case PIXEL_FORMAT_RGB_BAYER_14BPP:
        return 14;

    case PIXEL_FORMAT_RGB_BAYER_16BPP:
        return 16;

    case PIXEL_FORMAT_YVU_SEMIPLANAR_422:
    case PIXEL_FORMAT_YVU_SEMIPLANAR_420:
    case PIXEL_FORMAT_YUV_400:
        if (pstVFrame->enDynamicRange == DYNAMIC_RANGE_SDR8)
        {
            return 8;
        }
        else
        {
            return 10;
        }

    default:
        return 0;
    }

}
#endif

static SC_U32 SAMPLE_VI_GetRawStride(PIXEL_FORMAT_E enPixelFormat, SC_U32 u32Width, SC_U32 u32ByteAlign)
{
    SC_U32 u32Stride = 0;

    if (PIXEL_FORMAT_RGB_BAYER_16BPP == enPixelFormat)
    {
        u32Stride = ALIGN_UP(u32Width * 2, 16);
    }
    else if (PIXEL_FORMAT_RGB_BAYER_12BPP == enPixelFormat)
    {
        if (1 == u32ByteAlign)
        {
            u32Stride = (u32Width * 12 + 127) / 128 * 128 / 8;
        }
        else
        {
            u32Stride = (u32Width * 12 ) / 8;

            if (0 == ((u32Width * 12) % 8))
            {
                u32Stride = (u32Width * 12 ) / 8; //-- pVBuf->u32Width * u32Nbit / 8
            }
            else
            {
                u32Stride = (u32Width * 12 ) / 8 + 8; //-- pVBuf->u32Width * u32Nbit / 8
            }
        }
    }
    else if (PIXEL_FORMAT_RGB_BAYER_10BPP == enPixelFormat)
    {
        u32Stride = (u32Width * 10 + 127) / 128 * 128 / 8;
        if (1 == u32ByteAlign)
        {
            u32Stride = (u32Width * 10 + 127) / 128 * 128 / 8;
        }
        else
        {
            u32Stride = (u32Width * 10 ) / 8;

            if (0 == ((u32Width * 10 ) % 8))
            {
                u32Stride = (u32Width * 10 ) / 8; //-- pVBuf->u32Width * u32Nbit / 8
            }
            else
            {
                u32Stride = (u32Width * 10 ) / 8 + 8; //-- pVBuf->u32Width * u32Nbit / 8
            }
        }
    }
    else if (PIXEL_FORMAT_RGB_BAYER_8BPP == enPixelFormat)
    {
        u32Stride = (u32Width * 8 + 127) / 128 * 128 / 8;
    }

    return u32Stride;
}

SC_S32 SAMPLE_VI_GetFrameBlkInfo(SAMPLE_VI_FRAME_CONFIG_S *pstFrmCfg, SC_S32 s32FrmCnt,
    SAMPLE_VI_FRAME_INFO_S *pastViFrameInfo)
{
    SC_U32 i = 0;
    SC_U32 u32Stride;
    SC_U32 u32LStride;
    SC_U32 u32CStride;
    SC_U32 u32LumaSize = 0;
    SC_U32 u32ChrmSize = 0;
    SC_U32 u32Size;
    SC_U64 u64PhyAddr;
    SC_U8 *pVirAddr;
    VB_POOL u32PoolId;
    VB_BLK VbBlk;
    PIXEL_FORMAT_E enPixelFormat;
    VB_POOL_CONFIG_S stVbPoolCfg;

    enPixelFormat = pstFrmCfg->enPixelFormat;

    if (enPixelFormat == PIXEL_FORMAT_YVU_SEMIPLANAR_422)
    {
        u32Stride = ALIGN_UP((pstFrmCfg->u32Width * 8 + 7) >> 3, DEFAULT_ALIGN);
        u32LStride  = u32Stride;
        u32CStride  = u32Stride;
        u32Size = u32Stride * pstFrmCfg->u32Height * 2;
        u32LumaSize = u32Stride * pstFrmCfg->u32Height;
        u32ChrmSize = u32Stride * pstFrmCfg->u32Height / 2;
    }
    else if (enPixelFormat == PIXEL_FORMAT_YVU_SEMIPLANAR_420)
    {
        u32Stride = ALIGN_UP((pstFrmCfg->u32Width * 8 + 7) >> 3, DEFAULT_ALIGN);
        u32LStride  = u32Stride;
        u32CStride  = u32Stride;
        u32Size = u32Stride * pstFrmCfg->u32Height * 3 / 2;
        u32LumaSize = u32Stride * pstFrmCfg->u32Height;
        u32ChrmSize = u32Stride * pstFrmCfg->u32Height / 4;
    }
    else if (enPixelFormat == PIXEL_FORMAT_YUV_400)
    {
        u32Stride = ALIGN_UP((pstFrmCfg->u32Width * 8 + 7) >> 3, DEFAULT_ALIGN);
        u32LStride  = u32Stride;
        u32CStride  = u32Stride;
        u32Size = u32Stride * pstFrmCfg->u32Height;
        u32LumaSize = u32Size;
        u32ChrmSize = 0;
    }
    else
    {
        u32Stride = SAMPLE_VI_GetRawStride(enPixelFormat, pstFrmCfg->u32Width, pstFrmCfg->u32ByteAlign);
        u32LStride  = u32Stride;
        u32CStride  = u32Stride;
        u32Size = u32Stride * pstFrmCfg->u32Height;
    }

    memset(&stVbPoolCfg, 0, sizeof(VB_POOL_CONFIG_S));
    stVbPoolCfg.u64BlkSize  = u32Size;
    stVbPoolCfg.u32BlkCnt   = s32FrmCnt;
    stVbPoolCfg.enRemapMode = VB_REMAP_MODE_NONE;
    u32PoolId = SC_MPI_VB_CreatePool(&stVbPoolCfg);
    if (VB_INVALID_POOLID == u32PoolId)
    {
        SAMPLE_PRT("SC_MPI_VB_CreatePool failed!\n");
        return SC_FAILURE;
    }

    for (i = 0; i < s32FrmCnt; i++)
    {
        VbBlk = SC_MPI_VB_GetBlock(u32PoolId, u32Size, SC_NULL);
        if (VB_INVALID_HANDLE == VbBlk)
        {
            SAMPLE_PRT("SC_MPI_VB_GetBlock err! size:%d\n", u32Size);
            return SC_FAILURE;
        }

        u64PhyAddr = SC_MPI_VB_Handle2PhysAddr(VbBlk);
        if (0 == u64PhyAddr)
        {
            SAMPLE_PRT("SC_MPI_VB_Handle2PhysAddr err!\n");
            return SC_FAILURE;
        }

        pVirAddr = (SC_U8 *)SC_MPI_SYS_Mmap(u64PhyAddr, u32Size);
        if (NULL == pVirAddr)
        {
            SAMPLE_PRT("SC_MPI_SYS_Mmap err!\n");
            return SC_FAILURE;
        }

        pastViFrameInfo[i].stVideoFrameInfo.u32PoolId = u32PoolId;
        pastViFrameInfo[i].stVideoFrameInfo.enModId = SC_ID_VI;
        pastViFrameInfo[i].stVideoFrameInfo.stVFrame.u64PhyAddr[0]   = u64PhyAddr;
        pastViFrameInfo[i].stVideoFrameInfo.stVFrame.u64PhyAddr[1]   =
            pastViFrameInfo[i].stVideoFrameInfo.stVFrame.u64PhyAddr[0] + u32LumaSize;
        pastViFrameInfo[i].stVideoFrameInfo.stVFrame.u64PhyAddr[2]   =
            pastViFrameInfo[i].stVideoFrameInfo.stVFrame.u64PhyAddr[1] + u32ChrmSize;

        pastViFrameInfo[i].stVideoFrameInfo.stVFrame.u64VirAddr[0]   = (SC_U64)(SC_UL)pVirAddr;
        pastViFrameInfo[i].stVideoFrameInfo.stVFrame.u64VirAddr[1]   =
            pastViFrameInfo[i].stVideoFrameInfo.stVFrame.u64VirAddr[0] + u32LumaSize;
        pastViFrameInfo[i].stVideoFrameInfo.stVFrame.u64VirAddr[2]   =
            pastViFrameInfo[i].stVideoFrameInfo.stVFrame.u64VirAddr[1] + u32ChrmSize;

        pastViFrameInfo[i].stVideoFrameInfo.stVFrame.u32Stride[0]    = u32LStride;
        pastViFrameInfo[i].stVideoFrameInfo.stVFrame.u32Stride[1]    = u32CStride;
        pastViFrameInfo[i].stVideoFrameInfo.stVFrame.u32Stride[2]    = u32CStride;

        pastViFrameInfo[i].stVideoFrameInfo.stVFrame.u32Width        = pstFrmCfg->u32Width;
        pastViFrameInfo[i].stVideoFrameInfo.stVFrame.u32Height       = pstFrmCfg->u32Height;
        pastViFrameInfo[i].stVideoFrameInfo.stVFrame.enPixelFormat   = pstFrmCfg->enPixelFormat;
        pastViFrameInfo[i].stVideoFrameInfo.stVFrame.enVideoFormat   = pstFrmCfg->enVideoFormat;
        pastViFrameInfo[i].stVideoFrameInfo.stVFrame.enCompressMode  = pstFrmCfg->enCompressMode;
        pastViFrameInfo[i].stVideoFrameInfo.stVFrame.enField         = VIDEO_FIELD_FRAME;
        pastViFrameInfo[i].stVideoFrameInfo.stVFrame.enDynamicRange  = DYNAMIC_RANGE_SDR8;
        pastViFrameInfo[i].stVideoFrameInfo.stVFrame.enColorGamut    = COLOR_GAMUT_BT709;

        pastViFrameInfo[i].VbBlk   = VbBlk;
        pastViFrameInfo[i].u32Size = u32Size;
    }

    return SC_SUCCESS;
}

SC_S32 SAMPLE_VI_COMM_ReleaseFrameBlkInfo(SC_S32 s32FrmCnt, SAMPLE_VI_FRAME_INFO_S *pastViFrameInfo)
{
    SC_S32 s32Ret = SC_SUCCESS;
    SC_U32 i;
    VB_POOL u32PoolId;
    VB_BLK VbBlk;
    SC_U32 u32Size;

    for (i = 0; i < s32FrmCnt; i++)
    {
        VbBlk = pastViFrameInfo[i].VbBlk;
        s32Ret = SC_MPI_VB_ReleaseBlock(VbBlk);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("SC_MPI_VB_ReleaseBlock block 0x%x failure\n", VbBlk);
        }

        u32Size = pastViFrameInfo[i].u32Size;
        s32Ret = SC_MPI_SYS_Munmap((SC_VOID *)(SC_UL)pastViFrameInfo[i].stVideoFrameInfo.stVFrame.u64VirAddr[0], u32Size);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("SC_MPI_SYS_Munmap failure!\n");
        }
    }

    u32PoolId = pastViFrameInfo[0].stVideoFrameInfo.u32PoolId;
    SC_MPI_VB_DestroyPool(u32PoolId);

    return SC_SUCCESS;
}

#if 0 //todo
SC_VOID SAMPLE_COMM_VI_SaveFpnFile(ISP_FPN_TYPE_E enFpnType, ISP_FPN_FRAME_INFO_S *pVBuf, FILE *pfd)
{
    SC_U8   *pU8VBufVirt_Y;
    SC_U32  u32FpnHeight;
    SC_S32  i;

    u32FpnHeight = pVBuf->stFpnFrame.stVFrame.u32Height;
    pU8VBufVirt_Y = (SC_U8 *)(SC_UL)pVBuf->stFpnFrame.stVFrame.u64VirAddr[0];

    /* save Y ---------------------------------------------------------------- */
    fprintf(stderr, "FPN: saving......Raw data......u32Stride[0]: %d, width: %d, height: %d\n"
        , pVBuf->stFpnFrame.stVFrame.u32Stride[0]
        , pVBuf->stFpnFrame.stVFrame.u32Width
        , u32FpnHeight);
    fprintf(stderr, "phy Addr: 0x%llx\n", pVBuf->stFpnFrame.stVFrame.u64PhyAddr[0]);
    fprintf(stderr, "if SBS mode, RAW will be large, please wait a moment to save data.\n");
    fflush(stderr);

    fwrite(pU8VBufVirt_Y, pVBuf->u32FrmSize, 1, pfd);

    /* save offset */
    for (i = 0; i < VI_MAX_SPLIT_NODE_NUM; i++)
    {
        fwrite(&pVBuf->u32Offset[i], 4, 1, pfd);
    }

    /* save compress mode */
    fwrite(&pVBuf->stFpnFrame.stVFrame.enCompressMode, 4, 1, pfd);

    /* save fpn frame size */
    fwrite(&pVBuf->u32FrmSize, 4, 1, pfd);

    /* save ISO */
    fwrite(&pVBuf->u32Iso, 4, 1, pfd);
    fflush(pfd);

}

SC_VOID SAMPLE_COMM_VI_ReadFpnFile(ISP_FPN_FRAME_INFO_S *pstFpnFrmInfo, FILE *pfd)
{
    VIDEO_FRAME_INFO_S *pstFpnFrame;
    SC_S32 i;

    printf("u32FrmSize: %d!!\n", pstFpnFrmInfo->u32FrmSize);

    pstFpnFrame = &pstFpnFrmInfo->stFpnFrame;
    fread((SC_U8 *)(SC_UL)pstFpnFrame->stVFrame.u64VirAddr[0], pstFpnFrmInfo->u32FrmSize, 1, pfd);
    for (i = 0; i < VI_MAX_SPLIT_NODE_NUM; i++)
    {
        fread((SC_U8 *)&pstFpnFrmInfo->u32Offset[i], 4, 1, pfd);
    }
    fread((SC_U8 *)&pstFpnFrame->stVFrame.enCompressMode, 4, 1, pfd);
    fread((SC_U8 *)&pstFpnFrmInfo->u32FrmSize, 4, 1, pfd);
    fread((SC_U8 *)&pstFpnFrmInfo->u32Iso, 4, 1, pfd);
}

SC_S32 SAMPLE_COMM_VI_FpnCalibrateConfig(VI_PIPE ViPipe, SAMPLE_VI_FPN_CALIBRATE_INFO_S *pstViFpnCalibrateInfo)
{
    SAMPLE_VI_FRAME_CONFIG_S stFrmCfg;
    SAMPLE_VI_FRAME_INFO_S stViFrameInfo;
    ISP_FPN_CALIBRATE_ATTR_S stCalibrateAttr;
    ISP_PUB_ATTR_S stPubAttr;
    SC_S32 s32Ret;
    SC_S32 i;
    FILE *pfd;
    VI_CHN ViChn = 0;
    SC_CHAR FpnFileName[FILE_NAME_LEN];

    s32Ret = SC_MPI_ISP_GetPubAttr(ViPipe, &stPubAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get isp pub attr failed!\n");
        return s32Ret;
    }

    stFrmCfg.u32Width = stPubAttr.stWndRect.u32Width;
    if (ISP_FPN_TYPE_LINE == pstViFpnCalibrateInfo->enFpnType)
    {
        stFrmCfg.u32Height = 1;
    }
    else
    {
        stFrmCfg.u32Height = stPubAttr.stWndRect.u32Height;
    }

    stFrmCfg.u32ByteAlign = 0;
    stFrmCfg.enPixelFormat = pstViFpnCalibrateInfo->enPixelFormat;
    stFrmCfg.enCompressMode = pstViFpnCalibrateInfo->enCompressMode;
    stFrmCfg.enVideoFormat = VIDEO_FORMAT_LINEAR;
    stFrmCfg.enDynamicRange = DYNAMIC_RANGE_SDR8;
    s32Ret = SAMPLE_VI_GetFrameBlkInfo(&stFrmCfg, 1, &stViFrameInfo);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_VI_GetFrameBlkInfo failed!\n");
        return s32Ret;
    }

    printf("Pipe %d Width=%d, Height=%d.\n", ViPipe, stFrmCfg.u32Width, stFrmCfg.u32Height);

    printf("please turn off camera aperture to start calibrate!\n");
    printf("hit Enter key ,start calibrate!\n");
    getchar();

    s32Ret = SC_MPI_VI_DisableChn(ViPipe, ViChn);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("disable vi chn %d failed!", ViChn);
        goto EXIT0;
    }

    stCalibrateAttr.enFpnType             = pstViFpnCalibrateInfo->enFpnType;
    stCalibrateAttr.u32FrameNum           = pstViFpnCalibrateInfo->u32FrameNum;
    stCalibrateAttr.u32Threshold          = pstViFpnCalibrateInfo->u32Threshold;
    stCalibrateAttr.stFpnCaliFrame.u32Iso = 0;
    for (i = 0; i < VI_MAX_SPLIT_NODE_NUM; i++)
    {
        stCalibrateAttr.stFpnCaliFrame.u32Offset[i] = 0;
    }
    stCalibrateAttr.stFpnCaliFrame.u32FrmSize = stViFrameInfo.u32Size;
    memcpy(&stCalibrateAttr.stFpnCaliFrame.stFpnFrame, &stViFrameInfo.stVideoFrameInfo, sizeof(VIDEO_FRAME_INFO_S));

    s32Ret = SC_MPI_ISP_FPNCalibrate(ViPipe, &stCalibrateAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_ISP_FPNCalibrate err: 0x%x\n", s32Ret);
        goto EXIT1;
    }

    /* save FPN raw data */
    snprintf(FpnFileName, FILE_NAME_LEN, "./FPN_%s_w%d_h%d_%dbit.raw",
        PRINT_FPNTYPE(stCalibrateAttr.enFpnType),
        stPubAttr.stWndRect.u32Width,
        stPubAttr.stWndRect.u32Height,
        SAMPLE_VI_PixelFormat2BitWidth(&stViFrameInfo.stVideoFrameInfo.stVFrame));

    printf("\nafter calibrate ");
    for (i = 0; i < VI_MAX_SPLIT_NODE_NUM; i++)
    {
        printf("offset[%d] = 0x%x, ", i, stCalibrateAttr.stFpnCaliFrame.u32Offset[i]);
    }
    printf("u32FrmSize = %d\n", stCalibrateAttr.stFpnCaliFrame.u32FrmSize);
    printf("ISO = %d\n", stCalibrateAttr.stFpnCaliFrame.u32Iso);

    printf("save dark frame file: %s!\n", FpnFileName);

    pfd = fopen(FpnFileName, "wb");
    if (NULL == pfd)
    {
        printf("open file %s err!\n", FpnFileName);
        goto EXIT1;
    }

    SAMPLE_COMM_VI_SaveFpnFile(stCalibrateAttr.enFpnType, &stCalibrateAttr.stFpnCaliFrame, pfd);

    fclose(pfd);

EXIT1:
    s32Ret = SC_MPI_VI_EnableChn(ViPipe, ViChn);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("enable vi chn %d failed!", ViChn);
    }
EXIT0:
    SAMPLE_VI_COMM_ReleaseFrameBlkInfo(1, &stViFrameInfo);

    return s32Ret;
}

SC_S32 SAMPLE_COMM_VI_FpnCorrectionConfig(VI_PIPE ViPipe, SAMPLE_VI_FPN_CORRECTION_INFO_S *pstViFpnCorrectionInfo)
{
    SAMPLE_VI_FRAME_CONFIG_S stFrmCfg;
    SAMPLE_VI_FRAME_INFO_S *pstViFrameInfo;
    ISP_FPN_ATTR_S stFPNAttr;
    ISP_PUB_ATTR_S stPubAttr;
    SC_S32 s32Ret;
    FILE *pfd;
    SC_S32 i;
    SC_CHAR FpnFileName[FILE_NAME_LEN];

    s32Ret = SC_MPI_ISP_GetPubAttr(ViPipe, &stPubAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get isp pub attr failed!\n");
        return s32Ret;
    }

    if (ISP_FPN_TYPE_LINE == pstViFpnCorrectionInfo->enFpnType)
    {
        stFrmCfg.u32Height = 1;
    }
    else
    {
        stFrmCfg.u32Height = stPubAttr.stWndRect.u32Height;
    }

    pstViFrameInfo = &pstViFpnCorrectionInfo->stViFrameInfo;

    stFrmCfg.u32Width = stPubAttr.stWndRect.u32Width;
    stFrmCfg.u32ByteAlign = 0;
    stFrmCfg.enPixelFormat = pstViFpnCorrectionInfo->enPixelFormat;
    stFrmCfg.enCompressMode = pstViFpnCorrectionInfo->enCompressMode;
    stFrmCfg.enVideoFormat = VIDEO_FORMAT_LINEAR;
    stFrmCfg.enDynamicRange = DYNAMIC_RANGE_SDR8;
    s32Ret = SAMPLE_VI_GetFrameBlkInfo(&stFrmCfg, 1, pstViFrameInfo);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("VI_MST_COMM_GetFrameBlkInfo failed!\n");
        return s32Ret;
    }

    stFPNAttr.stFpnFrmInfo.u32FrmSize = pstViFrameInfo->u32Size;
    memcpy(&stFPNAttr.stFpnFrmInfo.stFpnFrame, &pstViFrameInfo->stVideoFrameInfo, sizeof(VIDEO_FRAME_INFO_S));

    /* save FPN raw data */
    snprintf(FpnFileName, FILE_NAME_LEN, "./FPN_%s_w%d_h%d_%dbit.raw",
        PRINT_FPNTYPE(pstViFpnCorrectionInfo->enFpnType),
        stPubAttr.stWndRect.u32Width,
        stPubAttr.stWndRect.u32Height,
        SAMPLE_VI_PixelFormat2BitWidth(&pstViFrameInfo->stVideoFrameInfo.stVFrame));

    printf("====to open Fpn raw file: %s. \n", FpnFileName);
    pfd = fopen(FpnFileName, "rb");
    if (NULL == pfd)
    {
        SAMPLE_PRT("open file %s err!\n", FpnFileName);
        goto EXIT0;
    }

    SAMPLE_COMM_VI_ReadFpnFile(&stFPNAttr.stFpnFrmInfo, pfd);

    fclose(pfd);

    stFPNAttr.bEnable = SC_TRUE;
    stFPNAttr.enOpType = pstViFpnCorrectionInfo->enOpType;
    stFPNAttr.enFpnType = pstViFpnCorrectionInfo->enFpnType;
    stFPNAttr.stManual.u32Strength = pstViFpnCorrectionInfo->u32Strength;

    for (i = 0; i < VI_MAX_SPLIT_NODE_NUM; i++)
    {
        printf("offset[%d] = 0x%x; ", i, stFPNAttr.stFpnFrmInfo.u32Offset[i]);
    }
    printf("\n");
    printf("u32FrmSize = %d.\n", stFPNAttr.stFpnFrmInfo.u32FrmSize);
    printf("ISO = %d.\n", stFPNAttr.stFpnFrmInfo.u32Iso);

    printf("\nstFPNAttr: u32Width = %d, u32Height = %d\n",
        stFPNAttr.stFpnFrmInfo.stFpnFrame.stVFrame.u32Width, stFPNAttr.stFpnFrmInfo.stFpnFrame.stVFrame.u32Height);
    printf("FrmSize: %d, stride: %d, compress mode: %d\n", stFPNAttr.stFpnFrmInfo.u32FrmSize,
        stFPNAttr.stFpnFrmInfo.stFpnFrame.stVFrame.u32Stride[0],
        stFPNAttr.stFpnFrmInfo.stFpnFrame.stVFrame.enCompressMode);

    s32Ret = SC_MPI_ISP_SetFPNAttr(ViPipe, &stFPNAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("fpn correction fail 0x%x\n", s32Ret);
        goto EXIT0;
    }

    return SC_SUCCESS;

EXIT0:
    SAMPLE_VI_COMM_ReleaseFrameBlkInfo(1, pstViFrameInfo);

    return s32Ret;
}

SC_S32 SAMPLE_COMM_VI_DisableFpnCorrection(VI_PIPE ViPipe, SAMPLE_VI_FPN_CORRECTION_INFO_S *pstViFpnCorrectionInfo)
{
    ISP_FPN_ATTR_S stFPNAttr;
    SC_S32 s32Ret;

    s32Ret = SC_MPI_ISP_GetFPNAttr(ViPipe, &stFPNAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get fpn correction fail, ret 0x%x\n", s32Ret);
        return SC_FAILURE;
    }

    stFPNAttr.bEnable = SC_FALSE;
    s32Ret = SC_MPI_ISP_SetFPNAttr(ViPipe, &stFPNAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("set fpn correction fail, ret 0x%x\n", s32Ret);
        return SC_FAILURE;
    }

    SAMPLE_VI_COMM_ReleaseFrameBlkInfo(1, &pstViFpnCorrectionInfo->stViFrameInfo);

    return SC_SUCCESS;

}
#endif

SC_S32 SAMPLE_VI_PlanToSemi(SC_U8 *pY, SC_S32 yStride,
    SC_U8 *pU, SC_S32 uStride,
    SC_U8 *pV, SC_S32 vStride,
    SC_S32 picWidth, SC_S32 picHeight)
{
    SC_S32 i;
    SC_U8 *pTmpU, *ptu;
    SC_U8 *pTmpV, *ptv;

    SC_S32 s32HafW = uStride >> 1 ;
    SC_S32 s32HafH = picHeight >> 1 ;
    SC_S32 s32Size = s32HafW * s32HafH;

    pTmpU = malloc(s32Size);
    ptu = pTmpU;
    pTmpV = malloc(s32Size);
    ptv = pTmpV;

    memcpy(pTmpU, pU, s32Size);
    memcpy(pTmpV, pV, s32Size);

    for (i = 0; i<s32Size >> 1; i++)
    {
        *pU++ = *pTmpV++;
        *pU++ = *pTmpU++;

    }
    for (i = 0; i<s32Size >> 1; i++)
    {
        *pV++ = *pTmpV++;
        *pV++ = *pTmpU++;
    }

    free(ptu);
    free(ptv);

    return SC_SUCCESS;
}

SC_S32 SAMPLE_VI_PlanToSemi_422(SC_U8 *pY, SC_S32 yStride,
    SC_U8 *pU, SC_S32 uStride,
    SC_U8 *pV, SC_S32 vStride,
    SC_S32 picWidth, SC_S32 picHeight)
{
    SC_S32 i;
    SC_U8 *pTmpU, *ptu;
    SC_U8 *pTmpV, *ptv;
    SC_S32 s32HafW = uStride >> 1;
    SC_S32 s32HafH = picHeight;
    SC_S32 s32Size = s32HafW * s32HafH;

    pTmpU = malloc(s32Size);
    ptu = pTmpU;
    pTmpV = malloc(s32Size);
    ptv = pTmpV;

    memcpy(pTmpU, pU, s32Size);
    memcpy(pTmpV, pV, s32Size);

    for (i = 0; i<s32Size >> 1; i++)
    {
        *pU++ = *pTmpV++;
        *pU++ = *pTmpU++;

    }

    for (i = 0; i<s32Size >> 1; i++)
    {
        *pV++ = *pTmpV++;
        *pV++ = *pTmpU++;
    }

    free(ptu);
    free(ptv);

    return SC_SUCCESS;
}

SC_VOID SAMPLE_VI_ReadOneFrame_400( FILE *fp, SC_U8 *pY, SC_U8 *pU, SC_U8 *pV,
    SC_U32 width, SC_U32 height, SC_U32 stride, SC_U32 stride2)
{
    SC_U8 *pDst;
    SC_U32 u32Row;

    pDst = pY;
    for (u32Row = 0; u32Row < height; u32Row++)
    {
        fread(pDst, width, 1, fp);
        pDst += stride;
    }

    char temp[1];
    fread(temp, 1, 1, fp);

    if (feof(fp) != 0)
    {
        printf("file is EOF!\n");
    }
    else
    {
        fseek(fp, ftell(fp) - 1, SEEK_SET);
    }

}

SC_VOID SAMPLE_VI_ReadOneFrame( FILE *fp, SC_U8 *pY, SC_U8 *pU, SC_U8 *pV,
    SC_U32 width, SC_U32 height, SC_U32 stride, SC_U32 stride2)
{
    SC_U8 *pDst;
    SC_U32 u32Row;
    pDst = pY;

    for (u32Row = 0; u32Row < height; u32Row++)
    {
        fread(pDst, width, 1, fp);
        pDst += stride;
    }
    pDst = pU;
    for (u32Row = 0; u32Row < height / 2; u32Row++)
    {
        fread(pDst, width / 2, 1, fp);
        pDst += stride2;
    }

    pDst = pV;
    for (u32Row = 0; u32Row < height / 2; u32Row++)
    {
        fread(pDst, width / 2, 1, fp);
        pDst += stride2;
    }

    char temp[1];
    fread(temp, 1, 1, fp);

    if (feof(fp) != 0)
    {
        printf("file is EOF!\n");
    }
    else
    {
        fseek(fp, ftell(fp) - 1, SEEK_SET);
    }

}

SC_VOID SAMPLE_VI_ReadOneFrame_422( FILE *fp, SC_U8 *pY, SC_U8 *pU, SC_U8 *pV,
    SC_U32 width, SC_U32 height, SC_U32 stride, SC_U32 stride2)
{
    SC_U8 *pDst;
    SC_U32 u32Row;
    pDst = pY;

    for (u32Row = 0; u32Row < height; u32Row++)
    {
        fread(pDst, width, 1, fp);
        pDst += stride;
    }

    pDst = pU;
    for (u32Row = 0; u32Row < height; u32Row++)
    {
        fread(pDst, width >> 1, 1, fp);
        pDst += stride2;
    }

    pDst = pV;
    for (u32Row = 0; u32Row < height; u32Row++)
    {
        fread(pDst, width >> 1, 1, fp);
        pDst += stride2;
    }

    char temp[1];
    fread(temp, 1, 1, fp);

    if (feof(fp) != 0)
    {
        printf("the End-of-file is reached\n");
    }
    else
    {
        fseek(fp, ftell(fp) - 1, SEEK_SET);
    }

}

static SC_VOID SAMPLE_VI_COMM_ReadYuvFile(FILE *pfd, VIDEO_FRAME_INFO_S *pstVideoFrameInfo)
{
    if (pstVideoFrameInfo->stVFrame.enPixelFormat == PIXEL_FORMAT_YVU_SEMIPLANAR_422)
    {
        SAMPLE_VI_ReadOneFrame_422(pfd, (SC_U8 *)(SC_UL)pstVideoFrameInfo->stVFrame.u64VirAddr[0],
            (SC_U8 *)(SC_UL)pstVideoFrameInfo->stVFrame.u64VirAddr[1], (SC_U8 *)(SC_UL)pstVideoFrameInfo->stVFrame.u64VirAddr[2],
            pstVideoFrameInfo->stVFrame.u32Width, pstVideoFrameInfo->stVFrame.u32Height,
            pstVideoFrameInfo->stVFrame.u32Stride[0], pstVideoFrameInfo->stVFrame.u32Stride[1] >> 1);

        sleep(1);

        SAMPLE_VI_PlanToSemi_422((SC_U8 *)(SC_UL)pstVideoFrameInfo->stVFrame.u64VirAddr[0],
            pstVideoFrameInfo->stVFrame.u32Stride[0],
            (SC_U8 *)(SC_UL)pstVideoFrameInfo->stVFrame.u64VirAddr[1], pstVideoFrameInfo->stVFrame.u32Stride[1],
            (SC_U8 *)(SC_UL)pstVideoFrameInfo->stVFrame.u64VirAddr[2], pstVideoFrameInfo->stVFrame.u32Stride[1],
            pstVideoFrameInfo->stVFrame.u32Width, pstVideoFrameInfo->stVFrame.u32Height);
    }
    else if (pstVideoFrameInfo->stVFrame.enPixelFormat == PIXEL_FORMAT_YVU_SEMIPLANAR_420)
    {
        SAMPLE_VI_ReadOneFrame(pfd, (SC_U8 *)(SC_UL)pstVideoFrameInfo->stVFrame.u64VirAddr[0],
            (SC_U8 *)(SC_UL)pstVideoFrameInfo->stVFrame.u64VirAddr[1], (SC_U8 *)(SC_UL)pstVideoFrameInfo->stVFrame.u64VirAddr[2],
            pstVideoFrameInfo->stVFrame.u32Width, pstVideoFrameInfo->stVFrame.u32Height,
            pstVideoFrameInfo->stVFrame.u32Stride[0], pstVideoFrameInfo->stVFrame.u32Stride[1] >> 1);

        sleep(1);

        SAMPLE_VI_PlanToSemi((SC_U8 *)(SC_UL)pstVideoFrameInfo->stVFrame.u64VirAddr[0],
            pstVideoFrameInfo->stVFrame.u32Stride[0],
            (SC_U8 *)(SC_UL)pstVideoFrameInfo->stVFrame.u64VirAddr[1], pstVideoFrameInfo->stVFrame.u32Stride[1],
            (SC_U8 *)(SC_UL)pstVideoFrameInfo->stVFrame.u64VirAddr[2], pstVideoFrameInfo->stVFrame.u32Stride[1],
            pstVideoFrameInfo->stVFrame.u32Width, pstVideoFrameInfo->stVFrame.u32Height);
    }
    else
    {
        SAMPLE_VI_ReadOneFrame_400(pfd, (SC_U8 *)(SC_UL)pstVideoFrameInfo->stVFrame.u64VirAddr[0],
            (SC_U8 *)(SC_UL)pstVideoFrameInfo->stVFrame.u64VirAddr[1], (SC_U8 *)(SC_UL)pstVideoFrameInfo->stVFrame.u64VirAddr[2],
            pstVideoFrameInfo->stVFrame.u32Width, pstVideoFrameInfo->stVFrame.u32Height,
            pstVideoFrameInfo->stVFrame.u32Stride[0], pstVideoFrameInfo->stVFrame.u32Stride[1] >> 1);
    }
}

SC_S32 SAMPLE_COMM_VI_Load_UserPic(const char *pszYuvFile, VI_USERPIC_ATTR_S *pstUsrPic,
    SAMPLE_VI_FRAME_INFO_S *pstViFrameInfo)
{
    FILE *pfd;
    SAMPLE_VI_FRAME_CONFIG_S stFrmCfg;
    SC_S32 s32Ret;

    stFrmCfg.u32Width = pstUsrPic->unUsrPic.stUsrPicFrm.stVFrame.u32Width;
    stFrmCfg.u32Height = pstUsrPic->unUsrPic.stUsrPicFrm.stVFrame.u32Height;
    stFrmCfg.u32ByteAlign = 0;
    stFrmCfg.enPixelFormat = pstUsrPic->unUsrPic.stUsrPicFrm.stVFrame.enPixelFormat;
    stFrmCfg.enCompressMode = COMPRESS_MODE_NONE;
    stFrmCfg.enVideoFormat = VIDEO_FORMAT_LINEAR;
    stFrmCfg.enDynamicRange = DYNAMIC_RANGE_SDR8;

    s32Ret = SAMPLE_VI_GetFrameBlkInfo(&stFrmCfg, 1, pstViFrameInfo);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_VI_GetFrameBlkInfo failed!\n");
        return s32Ret;
    }

    memcpy(&pstUsrPic->unUsrPic.stUsrPicFrm, &pstViFrameInfo->stVideoFrameInfo, sizeof(VIDEO_FRAME_INFO_S));

    printf("====to open YUV file: %s. \n", pszYuvFile);
    pfd = fopen(pszYuvFile, "rb");
    if (!pfd)
    {
        SAMPLE_PRT("open file -> %s fail \n", pszYuvFile);
        goto EXIT;
    }

    SAMPLE_VI_COMM_ReadYuvFile(pfd, &pstUsrPic->unUsrPic.stUsrPicFrm);

    fclose(pfd);

    return SC_SUCCESS;

EXIT:
    SAMPLE_VI_COMM_ReleaseFrameBlkInfo(1, pstViFrameInfo);
    return s32Ret;
}

SC_VOID SAMPLE_COMM_VI_Release_UserPic(SAMPLE_VI_FRAME_INFO_S *pstViFrameInfo)
{
    SAMPLE_VI_COMM_ReleaseFrameBlkInfo(1, pstViFrameInfo);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
