#ifndef _SC_ADC_USER_H
#define _SC_ADC_USER_H

#define DEV_NAME_ADC "sc_adc"

#define ADC_CHN_MIN 0
#define ADC_CHN_MAX 7

/* 10bit adc */
#define ADC_VAL_MAX 0x3ff
#define ADC_MV_MAX  1800 /* mv */

/* @chn: [ADC_CHN_MIN, ADC_CHN_MAX]
 * @val: raw: [0, ADC_VAL_MAX]
 *       millivoltage: [0, ADC_MV_MAX]
 */
struct adc_val {
	int chn;
	int val;
};

/* 0: power off; 1: power en */
#define IOCMD_ADC_POWER       _IOW('p', 0, int)
#define IOCMD_ADC_GET_RAW     _IOWR('p', 1, struct adc_val)
#define IOCMD_ADC_GET_MV      _IOWR('p', 2, struct adc_val)

#endif /* _SC_ADC_USER_H */

