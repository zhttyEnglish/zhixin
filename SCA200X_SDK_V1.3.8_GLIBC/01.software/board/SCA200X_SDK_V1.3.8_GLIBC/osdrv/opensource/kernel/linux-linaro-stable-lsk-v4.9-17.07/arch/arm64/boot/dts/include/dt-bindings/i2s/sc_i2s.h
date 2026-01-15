#ifndef __SC_I2S_H__
#define __SC_I2S_H__

/* I2S device */
#define I2S_DEV_MST0  0x0
#define I2S_DEV_MST1  0x1
#define I2S_DEV_MST2  0x2
#define I2S_DEV_SLV0  0x3
#define I2S_DEV_SLV1  0x4

/* Audio interface */
#define I2S_INTERFACE_ALC5621 0x1ff10000
#define I2S_INTERFACE_ALC5633 0x1ff10001
#define I2S_INTERFACE_DP      0x1ff10002
#define I2S_INTERFACE_DVP     0x1ff10003
#define I2S_INTERFACE_HDMI    0x1ff10004
#define I2S_INTERFACE_NVP6124B    0x1ff10005
#define I2S_INTERFACE_TLV3106    0x1ff10006
#define I2S_INTERFACE_AK7755EN    0x1ff10007

#define CODEC_AS_MASTER 0
#define CODEC_AS_SLAVE 1

#endif
