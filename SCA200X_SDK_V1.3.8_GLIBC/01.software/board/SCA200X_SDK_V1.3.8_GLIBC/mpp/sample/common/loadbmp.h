#ifndef     __LOAD_BMP_H__
#define     __LOAD_BMP_H__

#include "sc_type.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

/* the color format OSD supported */
typedef enum scOSD_COLOR_FMT_E
{
    OSD_COLOR_FMT_RGB444    = 0,
    OSD_COLOR_FMT_RGB4444   = 1,
    OSD_COLOR_FMT_RGB555    = 2,
    OSD_COLOR_FMT_RGB565    = 3,
    OSD_COLOR_FMT_RGB1555   = 4,
    OSD_COLOR_FMT_RGB888    = 6,
    OSD_COLOR_FMT_RGB8888   = 7,
    OSD_COLOR_FMT_BUTT
} OSD_COLOR_FMT_E;

typedef struct scOSD_RGB_S
{
    SC_U8   u8B;
    SC_U8   u8G;
    SC_U8   u8R;
    SC_U8   u8Reserved;
} OSD_RGB_S;

typedef struct scOSD_SURFACE_S
{
    OSD_COLOR_FMT_E enColorFmt;         /* color format */
    SC_U8  *pu8PhyAddr;               /* physical address */
    SC_U16  u16Height;                /* operation height */
    SC_U16  u16Width;                 /* operation width */
    SC_U16  u16Stride;                /* surface stride */
    SC_U16  u16Reserved;
} OSD_SURFACE_S;

typedef struct tag_OSD_Logo
{
    SC_U32    width;        /* out */
    SC_U32    height;       /* out */
    SC_U32    stride;       /* in */
    SC_U8    *pRGBBuffer;   /* in/out */
} OSD_LOGO_T;

typedef struct tag_OSD_BITMAPINFOHEADER
{
    SC_U16      biSize;
    SC_U32       biWidth;
    SC_S32       biHeight;
    SC_U16       biPlanes;
    SC_U16       biBitCount;
    SC_U32      biCompression;
    SC_U32      biSizeImage;
    SC_U32       biXPelsPerMeter;
    SC_U32       biYPelsPerMeter;
    SC_U32      biClrUsed;
    SC_U32      biClrImportant;
} OSD_BITMAPINFOHEADER;

typedef struct tag_OSD_BITMAPFILEHEADER
{
    SC_U32   bfSize;
    SC_U16    bfReserved1;
    SC_U16    bfReserved2;
    SC_U32   bfOffBits;
} OSD_BITMAPFILEHEADER;

typedef struct tag_OSD_RGBQUAD
{
    SC_U8    rgbBlue;
    SC_U8    rgbGreen;
    SC_U8    rgbRed;
    SC_U8    rgbReserved;
} OSD_RGBQUAD;

typedef struct tag_OSD_BITMAPINFO
{
    OSD_BITMAPINFOHEADER    bmiHeader;
    OSD_RGBQUAD                 bmiColors[1];
} OSD_BITMAPINFO;

typedef struct scOSD_COMPONENT_INFO_S
{
    int alen;
    int rlen;
    int glen;
    int blen;
} OSD_COMP_INFO;

SC_S32 LoadImage(const SC_CHAR *filename, OSD_LOGO_T *pVideoLogo);
SC_S32 LoadBitMap2Surface(const SC_CHAR *pszFileName, const OSD_SURFACE_S *pstSurface, SC_U8 *pu8Virt);
SC_S32 CreateSurfaceByBitMap(const SC_CHAR *pszFileName, OSD_SURFACE_S *pstSurface, SC_U8 *pu8Virt);
SC_S32 CreateSurfaceByCanvas(const SC_CHAR *pszFileName, OSD_SURFACE_S *pstSurface, SC_U8 *pu8Virt, SC_U32 u32Width,
    SC_U32 u32Height, SC_U32 u32Stride);
SC_S32 GetBmpInfo(const SC_CHAR *filename, OSD_BITMAPFILEHEADER  *pBmpFileHeader, OSD_BITMAPINFO *pBmpInfo);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __LOAD_BMP_H__*/

