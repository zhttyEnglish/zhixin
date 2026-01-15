#ifndef __SC_UTIL_H__
#define __SC_UTIL_H__
//#include <common.h>
#include <stdlib.h>
#include <stdint.h>
#include <config.h>
#include <errno.h>
#include <malloc.h>
#include <linux/delay.h>

#define  DISPLAY_UBOOT

#define MOD_DISPLAY "DISPLAY"

enum {
	SC_SYSTEM_INTERLACE_MOD_1080I,
	SC_SYSTEM_INTERLACE_MOD_720_576I_PAL,
	SC_SYSTEM_INTERLACE_MOD_720_480I_NTSC,
	SC_SYSTEM_INTERLACE_MOD_NULL,
};

typedef enum {
	SC_VIDEO_FORMAT_UNKNOWN = 0,  //
	/* YUV 8 bit*/
	SC_VIDEO_FORMAT_I400,
	SC_VIDEO_FORMAT_I420,  // Planar 4:2:0 YUV
	SC_VIDEO_FORMAT_YV12,  // Planar 4:2:0 YVU (like I420 but UV swapped)
	SC_VIDEO_FORMAT_NV12,  // Planar 4:2:0 YUV with interleaved UV plane
	SC_VIDEO_FORMAT_NV21,  // Planar 4:2:0 YUV with interleaved VU plane
	SC_VIDEO_FORMAT_Y42B,  // Planar 4:2:2 YUV
	SC_VIDEO_FORMAT_YUYV,  // Packed 4:2:2 YUV(Y0-U0-Y1-V0 Y2-U1-Y3-V1 ....)
	SC_VIDEO_FORMAT_YVYU,  // Packed 4:2:2 YUV(Y0-V0-Y1-U0 Y2-V1-Y3-U1 ....)
	SC_VIDEO_FORMAT_Y444,  // Planar 4:4:4 YUV

	/* YUV 10 bit, save as 16bit for per component, little endian */
	SC_VIDEO_FORMAT_I420_P10_16BIT_LE,  // Planar 4:2:0 YUV
	SC_VIDEO_FORMAT_YV12_P10_16BIT_LE,  // Planar 4:2:0 YVU (like I420 but UV swapped)
	SC_VIDEO_FORMAT_NV12_P10_16BIT_LE,  // Planar 4:2:0 YUV with interleaved UV plane
	SC_VIDEO_FORMAT_NV21_P10_16BIT_LE,  // Planar 4:2:0 YUV with interleaved VU plane
	SC_VIDEO_FORMAT_Y42B_P10_16BIT_LE,  // Planar 4:2:2 YUV
	SC_VIDEO_FORMAT_YUYV_P10_16BIT_LE,  // Packed 4:2:2 YUV(Y0-U0-Y1-V0 Y2-U1-Y3-V1 ....)
	SC_VIDEO_FORMAT_YVYU_P10_16BIT_LE,  // Packed 4:2:2 YUV(Y0-V0-Y1-U0 Y2-V1-Y3-U1 ....)
	SC_VIDEO_FORMAT_Y444_P10_16BIT_LE,  // Planar 4:4:4 YUV
	/* YUV 10 bit, save as 16bit for per component, big endian */
	SC_VIDEO_FORMAT_I420_P10_16BIT_BE,  // Planar 4:2:0 YUV
	SC_VIDEO_FORMAT_YV12_P10_16BIT_BE,  // Planar 4:2:0 YVU (like I420 but UV swapped)
	SC_VIDEO_FORMAT_NV12_P10_16BIT_BE,  // Planar 4:2:0 YUV with interleaved UV plane
	SC_VIDEO_FORMAT_NV21_P10_16BIT_BE,  // Planar 4:2:0 YUV with interleaved VU plane
	SC_VIDEO_FORMAT_Y42B_P10_16BIT_BE,  // Planar 4:2:2 YUV
	SC_VIDEO_FORMAT_YUYV_P10_16BIT_BE,  // Packed 4:2:2 YUV(Y0-U0-Y1-V0 Y2-U1-Y3-V1 ....)
	SC_VIDEO_FORMAT_YVYU_P10_16BIT_BE,  // Packed 4:2:2 YUV(Y0-V0-Y1-U0 Y2-V1-Y3-U1 ....)
	SC_VIDEO_FORMAT_Y444_P10_16BIT_BE,  // Planar 4:4:4 YUV

	/* YUV 10 bit, save as 32bit for 3 components, little endian */
	SC_VIDEO_FORMAT_I420_P10_32BIT_LE,  // Planar 4:2:0 YUV
	SC_VIDEO_FORMAT_YV12_P10_32BIT_LE,  // Planar 4:2:0 YVU (like I420 but UV swapped)
	SC_VIDEO_FORMAT_NV12_P10_32BIT_LE,  // Planar 4:2:0 YUV with interleaved UV plane
	SC_VIDEO_FORMAT_NV21_P10_32BIT_LE,  // Planar 4:2:0 YUV with interleaved VU plane
	SC_VIDEO_FORMAT_Y42B_P10_32BIT_LE,  // Planar 4:2:2 YUV
	SC_VIDEO_FORMAT_YUYV_P10_32BIT_LE,  // Packed 4:2:2 YUV(Y0-U0-Y1-V0 Y2-U1-Y3-V1 ....)
	SC_VIDEO_FORMAT_YVYU_P10_32BIT_LE,  // Packed 4:2:2 YUV(Y0-V0-Y1-U0 Y2-V1-Y3-U1 ....)
	SC_VIDEO_FORMAT_Y444_P10_32BIT_LE,  // Planar 4:4:4 YUV
	/* YUV 10 bit, save as 32bit for 3 components, big endian */
	SC_VIDEO_FORMAT_I420_P10_32BIT_BE,  // Planar 4:2:0 YUV
	SC_VIDEO_FORMAT_YV12_P10_32BIT_BE,  // Planar 4:2:0 YVU (like I420 but UV swapped)
	SC_VIDEO_FORMAT_NV12_P10_32BIT_BE,  // Planar 4:2:0 YUV with interleaved UV plane
	SC_VIDEO_FORMAT_NV21_P10_32BIT_BE,  // Planar 4:2:0 YUV with interleaved VU plane
	SC_VIDEO_FORMAT_Y42B_P10_32BIT_BE,  // Planar 4:2:2 YUV
	SC_VIDEO_FORMAT_YUYV_P10_32BIT_BE,  // Packed 4:2:2 YUV(Y0-U0-Y1-V0 Y2-U1-Y3-V1 ....)
	SC_VIDEO_FORMAT_YVYU_P10_32BIT_BE,  // Packed 4:2:2 YUV(Y0-V0-Y1-U0 Y2-V1-Y3-U1 ....)
	SC_VIDEO_FORMAT_Y444_P10_32BIT_BE,  // Planar 4:4:4 YUV

	/* RGB */
	SC_VIDEO_FORMAT_RGB, //
	/* RGB 10 bit, save as 16bit for per component */
	SC_VIDEO_FORMAT_RGB_P10_16BIT_LE,
	SC_VIDEO_FORMAT_RGB_P10_16BIT_BE,
	/* RGB 10 bit, save as 32bit for 3 components */
	SC_VIDEO_FORMAT_RGB_P10_32BIT_LE,
	SC_VIDEO_FORMAT_RGB_P10_32BIT_BE,

	/* RAW */
	SC_VIDEO_FORMAT_RAW_RGGB_P8,
	SC_VIDEO_FORMAT_RAW_BGGR_P8,
	SC_VIDEO_FORMAT_RAW_GRBG_P8,
	SC_VIDEO_FORMAT_RAW_GBRG_P8,

	SC_VIDEO_FORMAT_RAW_RGGB_P10,
	SC_VIDEO_FORMAT_RAW_BGGR_P10,
	SC_VIDEO_FORMAT_RAW_GRBG_P10,
	SC_VIDEO_FORMAT_RAW_GBRG_P10,

	SC_VIDEO_FORMAT_RAW_RGGB_P12,
	SC_VIDEO_FORMAT_RAW_BGGR_P12,
	SC_VIDEO_FORMAT_RAW_GRBG_P12,
	SC_VIDEO_FORMAT_RAW_GBRG_P12,

	SC_VIDEO_FORMAT_RAW_RGGB_P14,
	SC_VIDEO_FORMAT_RAW_BGGR_P14,
	SC_VIDEO_FORMAT_RAW_GRBG_P14,
	SC_VIDEO_FORMAT_RAW_GBRG_P14,

	/* RAW 10 bit, save as 16bit for per component, little endian */
	SC_VIDEO_FORMAT_RAW_RGGB_P10_16BIT_LE,
	SC_VIDEO_FORMAT_RAW_BGGR_P10_16BIT_LE,
	SC_VIDEO_FORMAT_RAW_GRBG_P10_16BIT_LE,
	SC_VIDEO_FORMAT_RAW_GBRG_P10_16BIT_LE,
	/* RAW 10 bit, save as 32 bit for 3 components, little endian */
	SC_VIDEO_FORMAT_RAW_RGGB_P10_32BIT_LE,
	SC_VIDEO_FORMAT_RAW_BGGR_P10_32BIT_LE,
	SC_VIDEO_FORMAT_RAW_GRBG_P10_32BIT_LE,
	SC_VIDEO_FORMAT_RAW_GBRG_P10_32BIT_LE,

	SC_VIDEO_FORMAT_YUV422_SEMI_PLANNAR,  // Planar 4:2:2 YUV with interleaved UV plane
	SC_VIDEO_FORMAT_YUV422_SEMI_PLANNAR_P10_16BIT_LE,  // Planar 4:2:2 YUV with interleaved UV plane
	SC_VIDEO_FORMAT_YUV444_SEMI_PLANNAR_P10_16BIT_LE,  // Planar 4:4:4 YUV with interleaved UV plane
	SC_VIDEO_FORMAT_YUV422_SEMI_PLANNAR_P10_32BIT_LE,  // Planar 4:2:2 YUV with interleaved UV plane

	SC_VIDEO_FORMAT_Y420_CF50_P8_PLANNAR,
	SC_VIDEO_FORMAT_Y420_CF50_P10_PLANNAR,

	SC_VIDEO_FORMAT_MAX
} sc_video_format_t;

typedef enum {
	SC_VIDEO_MATRIX_BT601_FULL,
	SC_VIDEO_MATRIX_BT601_LIMIT,
	SC_VIDEO_MATRIX_BT709_FULL,
	SC_VIDEO_MATRIX_BT709_LIMIT,
	SC_VIDEO_MATRIX_NODATA_FULL,
	SC_VIDEO_MATRIX_NODATA_LIMIT,
	SC_VIDEO_MATRIX_MAX,
} sc_video_matrix_t;

unsigned int read_reg32(unsigned long addr);
void write_reg32(unsigned long addr, unsigned int data);
void write_reg32_Mask(uint32_t regAddr, uint32_t regData, uint32_t regDataMask);

#define CGU_REG_BASE     0x01070000
#define PS2MS(value)     ((value)/1000000000)

#define GPIO_DATA_HIGH          1
#define GPIO_DATA_LOW           0

#define GPIO_DIR_OUTPUT         1
#define GPIO_DIR_INPUT          0

/**********************   gpio ****************************/
#define GPIO_BASE               0x8400000
#define GPIO_DIR_BASE           (GPIO_BASE + 0x04)
#define GPIO_PORT_DIR_STEP      0x0C
#define GPIO_OUT_BASE           GPIO_BASE
#define GPIO_PORT_OUT_STEP      0x0C
#define GPIO_IN_BASE            (GPIO_BASE + 0x50)
#define GPIO_PORT_IN_STEP       0x04
#define GPIO_GROUP_STEP         0x20000

#define MAX_GPIO_GROUP          4
#define MAX_GPIO_PORT           4
#define MAX_GPIO_PIN            8
/**********************   gpio ****************************/

#define SC_SET_REG_BITS(reg,val,start_bit,end_bit) (reg)=((val)<<(start_bit))| ((reg)&~((0xffffffff>>(32-((end_bit)-(start_bit)+1)))<<(start_bit)))
#define SC_GET_REG_BITS(reg,start_bit,end_bit) (((reg)&((0xffffffff>>(32-((end_bit)-(start_bit)+1)))<<(start_bit)))>>(start_bit))

#define SC_ALIGN4(_x)              (((_x)+0x03)&~0x03)
#define SC_ALIGN8(_x)              (((_x)+0x07)&~0x07)
#define SC_ALIGN16(_x)             (((_x)+0x0f)&~0x0f)
#define SC_ALIGN32(_x)             (((_x)+0x1f)&~0x1f)
#define SC_ALIGN64(_x)             (((_x)+0x3f)&~0x3f)
#define SC_ALIGN128(_x)            (((_x)+0x7f)&~0x7f)
#define SC_ALIGN256(_x)            (((_x)+0xff)&~0xff)
#define SC_ALIGN512(_x)            (((_x)+0x1ff)&~0x1ff)
#define SC_ALIGN16384(_x)          (((_x)+0x3fff)&~0x3fff)

#define __REG32__(addr) (*(volatile unsigned int *)(addr))
#define __REG64__(addr) (*(volatile uint64_t *)(addr))
#define __REG8__(addr) (*(volatile unsigned char *)(addr))

//log interface
typedef void  *sc_handle_t;

#define sc_get_task_id()     ((uint32_t)0)
#define dbg_printf printf
//#define ENABALE_ALL_LOG

#ifndef ENABALE_ALL_LOG
	#define DISABLE_CND_LOG
	#define DISABLE_FUN_LOG
	#define DISABLE_INFO_LOG
	#define DISABLE_PRT_LOG
	//#define DISABLE_ERR_LOG
	#define DISABLE_ALWAYS_LOG
	#define DISABLE_CTL_LOG
	#define DISABLE_FORMAT_LOG
	#define DISABLE_DBG_LOG
	#define DISABLE_WARNING_LOG
#endif

#define float2int(value) ((int)((value)*10000))
#define USE_BUF_LOG
#ifdef USE_BUF_LOG
	#define SC_LOG_LEVEL PRN_BUFF
#else
	#define SC_LOG_LEVEL PRN_UART
#endif

#define MAX(a,b)  (((a) > (b)) ? (a):(b))
#define MIN(a,b)  (((a) > (b)) ? (b):(a))

#define sc_delay(ms) udelay(1000*ms)
#define sc_get_timestamp() 0

#ifndef DISABLE_ERR_LOG
#define sc_err(fmt,...) do{\
   printf("[%d][%x][ERR] %s "fmt"\n",sc_get_timestamp(),sc_get_task_id(),__FUNCTION__,##__VA_ARGS__);\
   }while(0)
#else
#define sc_err(fmt,...) do{}while(0)
#endif
#ifndef DISABLE_CND_LOG
#define sc_conditon(fmt, ...) do{\
   printf("[%d][%x][CND] %s "fmt"\n",sc_get_timestamp(),sc_get_task_id(),__FUNCTION__,##__VA_ARGS__);\
   }while(0)
#else
#define sc_conditon(fmt, ...) do{}while(0)
#endif
#ifndef DISABLE_FUN_LOG
#define sc_func_enter() do{\
   printf("[%d][%x][FUNC] %s enter %d\n",sc_get_timestamp(),sc_get_task_id(),__FUNCTION__,__LINE__);\
   }while(0)
#define sc_func_exit() do{\
   printf("[%d][%x][FUNC] %s exit\n",sc_get_timestamp(),sc_get_task_id(),__FUNCTION__);\
   }while(0)
#else
#define sc_func_enter() do{}while(0)
#define sc_func_exit() do{}while(0)
#endif
#ifndef DISABLE_INFO_LOG
#define sc_info(fmt, ...) do{\
   printf("[%d][%x][INFO] %s "fmt"\n",sc_get_timestamp(),sc_get_task_id(),__FUNCTION__,##__VA_ARGS__);\
   }while(0)
#else
#define sc_info(fmt, ...) do{}while(0)
#endif
#ifndef DISABLE_DBG_LOG
#define sc_debug(fmt, ...) do{\
   printf("[%d][%x][DEBUG] %s "fmt"\n",sc_get_timestamp(),sc_get_task_id(),__FUNCTION__,##__VA_ARGS__);\
   }while(0)
#else
#define sc_debug(fmt, ...)  do{}while(0)
#endif
#ifndef DISABLE_PRT_LOG
#define sc_printf(fmt, ...) do{\
   printf(fmt,##__VA_ARGS__);\
   }while(0)
#else
#define sc_printf(fmt, ...) do{}while(0)
#endif
#ifndef DISABLE_ALWAYS_LOG
#define sc_always(fmt, ...)    do{\
        printf("[%d][%x][ALWAYS] %s "fmt"\n",sc_get_timestamp(),sc_get_task_id(),__FUNCTION__,##__VA_ARGS__);\
        }while(0)
#else
#define sc_always(fmt, ...) do{}while(0)
#endif

#define sc_bug_on()   do{printf("%s %d",__FUNCTION__,__LINE__);while(1);}while(0)

#define sc_malloc malloc
#define sc_free   free

#define get_timer() 0L

////queue inerface ------------------------
typedef  struct __sc_queue_t sc_queue_t;

struct __sc_queue_t {
	char *name;
	uint32_t queue_size;
	uint32_t valid_data_num;
	//queue data
	void **data;
	uint32_t header;
	uint32_t tail;
	//lock
	int (*queue_pop)(sc_queue_t *queue, void **item);
	int (*queue_insert)(sc_queue_t *queue, void *item);
	int (*get_queue_size)(sc_queue_t *queue);
	int (*look_up_head)(sc_queue_t *queue, void **item);
};
sc_queue_t *sc_creat_queue(uint32_t queue_size, char *name);
int sc_delete_queue(sc_queue_t **queue);

/* sc_lock stub */
typedef unsigned long sc_lock_t;
typedef unsigned long sc_signal_t;
void sc_lock(sc_lock_t lock_id);
void sc_unlock(sc_lock_t lock_id);
sc_lock_t sc_creat_lock(void);
uint64_t sc_get_timestamp_us(void);
sc_signal_t sc_create_signal(void);
void sc_enter_critical(void);
void sc_exit_critical(void);
void sc_trace_line(void);
void read_write_reg32_mask(uint32_t reg_addr, uint32_t reg_data, uint32_t mask);
int  sc_hal_set_pix_clk(float clk);
int gpio_set_direct(uint32_t group, uint32_t port, uint32_t inner_gpio, uint32_t dir);
int gpio_set_val(uint32_t group, uint32_t port, uint32_t inner_gpio, uint32_t val);

#endif
