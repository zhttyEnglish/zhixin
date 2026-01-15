/**
 * @file hal_errno.h
 * @brief  定义hal错误信息
 * @author SmartChip Software Team
 * @version 0.0.1
 * @date 2021/04/21
 * @license 2021-2025, SmartChip. Co., Ltd.
**/
#ifndef __SC_ERRNO_H__
#define __SC_ERRNO_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

/* 1010 0000b
 * VTOP use APPID from 0~31
 * so, smartchip use APPID based on 32
 */
#define SC_ERR_APPID  (0x80000000L)

typedef enum {
	HAL_ERR_LEVEL_DEBUG = 0,  /* debug-level                                  */
	HAL_ERR_LEVEL_INFO,       /* informational                                */
	HAL_ERR_LEVEL_NOTICE,     /* normal but significant condition             */
	HAL_ERR_LEVEL_WARNING,    /* warning conditions                           */
	HAL_ERR_LEVEL_ERROR,      /* error conditions                             */
	HAL_ERR_LEVEL_CRIT,       /* critical conditions                          */
	HAL_ERR_LEVEL_ALERT,      /* action must be taken immediately             */
	HAL_ERR_LEVEL_FATAL,      /* just for compatibility with previous version */
	HAL_ERR_LEVEL_BUTT
} ENUM_HAL_ERR_LEVEL;

/******************************************************************************
|----------------------------------------------------------------|
| 1 |   APP_ID   |   MOD_ID    | ERR_LEVEL |   ERR_ID            |
|----------------------------------------------------------------|
|<--><--7bits----><----8bits---><--3bits---><------13bits------->|
******************************************************************************/

#define SC_DEF_ERR( module, level, errid) \
    ((SC_S32)( (SC_ERR_APPID) | ((module) << 16 ) | ((level)<<13) | (errid) ))

#define SC_GET_ERRID(err)     ((SC_S32)(err & ((1 << 13)-1)))

/* NOTE! the following defined all common error code,
** all module must reserved 0~63 for their common error code
*/
typedef enum {
	HAL_ERR_INVALID_DEVID = 1, /* invlalid device ID                           */
	HAL_ERR_INVALID_CHNID = 2, /* invlalid channel ID                          */
	HAL_ERR_ILLEGAL_PARAM = 3, /* at lease one parameter is illagal
                               * eg, an illegal enumeration value             */
	HAL_ERR_EXIST         = 4, /* resource exists                              */
	HAL_ERR_UNEXIST       = 5, /* resource unexists                            */

	HAL_ERR_NULL_PTR      = 6, /* using a NULL point                           */

	HAL_ERR_NOT_CONFIG    = 7, /* try to enable or initialize system, device
                              ** or channel, before configing attribute       */

	HAL_ERR_NOT_SUPPORT   = 8, /* operation or type is not supported by NOW    */
	HAL_ERR_NOT_PERM      = 9, /* operation is not permitted
                              ** eg, try to change static attribute           */
	HAL_ERR_INVALID_PIPEID = 10, /* invlalid pipe ID                           */
	HAL_ERR_INVALID_STITCHGRPID  = 11, /* invlalid stitch group ID                           */

	HAL_ERR_NOMEM         = 12,/* failure caused by malloc memory              */
	HAL_ERR_NOBUF         = 13,/* failure caused by malloc buffer              */

	HAL_ERR_BUF_EMPTY     = 14,/* no data in buffer                            */
	HAL_ERR_BUF_FULL      = 15,/* no buffer for new data                       */

	HAL_ERR_SYS_NOTREADY  = 16,/* System is not ready,maybe not initialed or
                              ** loaded. Returning the error code when opening
                              ** a device file failed.                        */

	HAL_ERR_BADADDR       = 17,/* bad address,
                              ** eg. used for copy_from_user & copy_to_user   */

	HAL_ERR_BUSY          = 18,/* resource is busy,
                              ** eg. destroy a venc chn without unregister it */
	HAL_ERR_SIZE_NOT_ENOUGH = 19, /* buffer size is smaller than the actual size required */

	HAL_ERR_TIMEOUT       = 20, /* timeout:
                                ** eg. msg queue send timeout*/

	HAL_ERR_BUTT          = 63,/* maxium code, private error code of all modules
                              ** must be greater than it                      */
} ENUM_HAL_ERR_CODE;

#define HAL_NO_ERROR    (0x0)

typedef enum {
	HAL_VENC      = 1,
	HAL_VENC_IMPL = 2,
	HAL_VDEC      = 4,
	HAL_VDEC_IMPL = 8,
	HAL_MOD_VO    = 9,
	HAL_MOD_VIN   = 10,
	HAL_MOD_GE2D  = 11,
	HAL_MOD_RGN   = 12,
	HAL_VPSS      = 0x10,
} ENUM_HAL_MOD;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif  /* __SC_ERRNO_H__ */

