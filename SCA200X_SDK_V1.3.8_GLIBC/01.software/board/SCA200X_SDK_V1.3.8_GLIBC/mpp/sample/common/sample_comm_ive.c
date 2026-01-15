#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#include "sc_common.h"
#include "sc_comm_video.h"
#include "sc_comm_sys.h"
#include "sc_comm_vgs.h"
#include "sc_comm_vi.h"
#include "sc_comm_vo.h"
#include "mpi_vb.h"
#include "mpi_sys.h"
#include "mpi_vi.h"
#include "mpi_vo.h"
#include "mpi_vgs.h"
#include "mpi_vpss.h"
#include "sample_comm_ive.h"

static SC_BOOL bMpiInit = SC_FALSE;

SC_U16 SAMPLE_COMM_IVE_CalcStride(SC_U32 u32Width, SC_U8 u8Align)
{
    return (u32Width + (u8Align - u32Width % u8Align) % u8Align);
}

static SC_S32 SAMPLE_IVE_MPI_Init(SC_VOID)
{
    SC_S32 s32Ret;

    SC_MPI_SYS_Exit();

    s32Ret = SC_MPI_SYS_Init();
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_SYS_Init fail,Error(%#x)\n", s32Ret);
        return s32Ret;
    }

    return SC_SUCCESS;
}

SC_VOID SAMPLE_COMM_IVE_CheckIveMpiInit(SC_VOID)
{
    if (SC_FALSE == bMpiInit)
    {
        if (SAMPLE_IVE_MPI_Init())
        {
            SAMPLE_PRT("Ive mpi init failed!\n");
            exit(-1);
        }
        bMpiInit = SC_TRUE;
    }
}
SC_S32 SAMPLE_COMM_IVE_IveMpiExit(SC_VOID)
{
    bMpiInit = SC_FALSE;
    if (SC_MPI_SYS_Exit())
    {
        SAMPLE_PRT("Sys exit failed!\n");
        return SC_FAILURE;
    }

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_VGS_FillRect(VIDEO_FRAME_INFO_S *pstFrmInfo, SAMPLE_RECT_ARRAY_S *pstRect, SC_U32 u32Color)
{
    VGS_HANDLE VgsHandle = -1;
    SC_S32 s32Ret = SC_SUCCESS;
    SC_U16 i;
    VGS_TASK_ATTR_S stVgsTask;
    VGS_ADD_COVER_S stVgsAddCover;

    printf("%s: XXXXXXXXXXXX:  1\n", __func__);
    if (0 == pstRect->u16Num)
    {
        return s32Ret;
    }
    s32Ret = SC_MPI_VGS_BeginJob(&VgsHandle);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("Vgs begin job fail,Error(%#x)\n", s32Ret);
        return s32Ret;
    }

    printf("%s: XXXXXXXXXXXX:  2\n", __func__);
    memcpy(&stVgsTask.stImgIn, pstFrmInfo, sizeof(VIDEO_FRAME_INFO_S));
    memcpy(&stVgsTask.stImgOut, pstFrmInfo, sizeof(VIDEO_FRAME_INFO_S));

    stVgsAddCover.enCoverType = COVER_RECT;
    stVgsAddCover.u32Color = u32Color;
    for (i = 0; i < pstRect->u16Num; i++)
    {
        stVgsAddCover.stQuadRangle.bSolid = SC_FALSE;
        stVgsAddCover.stQuadRangle.u32Thick = 2;
        stVgsAddCover.stDstRect.s32X = pstRect->astRect[i].astPoint[0].s32X;
        stVgsAddCover.stDstRect.s32Y = pstRect->astRect[i].astPoint[0].s32Y;
        stVgsAddCover.stDstRect.u32Width = pstRect->astRect[i].astPoint[2].s32X - pstRect->astRect[i].astPoint[0].s32X;
        stVgsAddCover.stDstRect.u32Height = pstRect->astRect[i].astPoint[2].s32Y - pstRect->astRect[i].astPoint[0].s32Y;
        printf("%s: XXXXXXXXXXXX:  3\n", __func__);
        s32Ret = SC_MPI_VGS_AddCoverTask(VgsHandle, &stVgsTask, &stVgsAddCover);
        printf("%s: XXXXXXXXXXXX:  4\n", __func__);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("SC_MPI_VGS_AddCoverTask fail,Error(%#x)\n", s32Ret);
            SC_MPI_VGS_CancelJob(VgsHandle);
            return s32Ret;
        }
    }

    printf("%s: XXXXXXXXXXXX:  5\n", __func__);
    s32Ret = SC_MPI_VGS_EndJob(VgsHandle);
    printf("%s: XXXXXXXXXXXX:  6\n", __func__);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SC_MPI_VGS_EndJob fail,Error(%#x)\n", s32Ret);
        SC_MPI_VGS_CancelJob(VgsHandle);
        return s32Ret;
    }

    return s32Ret;

}

SC_S32 SAMPLE_COMM_IVE_ReadFile(IVE_IMAGE_S *pstImg, FILE *pFp)
{
    SC_U16 y;
    SC_U8 *pU8;
    SC_U16 height;
    SC_U16 width;
    SC_U16 loop;
    SC_S32 s32Ret;

    (SC_VOID)fgetc(pFp);
    if (feof(pFp))
    {
        SAMPLE_PRT("end of file!\n");
        s32Ret = fseek(pFp, 0, SEEK_SET );
        if (0 != s32Ret)
        {
            SAMPLE_PRT("fseek failed!\n");
            return s32Ret;
        }

    }
    else
    {
        s32Ret = fseek(pFp, -1, SEEK_CUR );
        if (0 != s32Ret)
        {
            SAMPLE_PRT("fseek failed!\n");
            return s32Ret;
        }
    }

    //if (feof(pFp))
    //{
    //    SAMPLE_PRT("end of file!\n");
    //    fseek(pFp, 0 , SEEK_SET);
    //}

    height = pstImg->u32Height;
    width = pstImg->u32Width;

    switch (pstImg->enType)
    {
    case  IVE_IMAGE_TYPE_U8C1:
    {
        pU8 = (SC_U8 *)(SC_UL)pstImg->au64VirAddr[0];
        for (y = 0; y < height; y++)
        {
            if ( 1 != fread(pU8, width, 1, pFp))
            {
                SAMPLE_PRT("Read file fail\n");
                return SC_FAILURE;
            }

            pU8 += pstImg->au32Stride[0];
        }
    }
    break;
    case  IVE_IMAGE_TYPE_YUV420P:
    {
        pU8 = (SC_U8 *)(SC_UL)pstImg->au64VirAddr[0];
        for (y = 0; y < height; y++)
        {
            if ( 1 != fread(pU8, width, 1, pFp))
            {
                SAMPLE_PRT("Read file fail\n");
                return SC_FAILURE;
            }

            pU8 += pstImg->au32Stride[0];
        }

        pU8 = (SC_U8 *)(SC_UL)pstImg->au64VirAddr[1];
        for (y = 0; y < height / 2; y++)
        {
            if ( 1 != fread(pU8, width / 2, 1, pFp))
            {
                SAMPLE_PRT("Read file fail\n");
                return SC_FAILURE;
            }

            pU8 += pstImg->au32Stride[1];
        }

        pU8 = (SC_U8 *)(SC_UL)pstImg->au64VirAddr[2];
        for (y = 0; y < height / 2; y++)
        {
            if ( 1 != fread(pU8, width / 2, 1, pFp))
            {
                SAMPLE_PRT("Read file fail\n");
                return SC_FAILURE;
            }

            pU8 += pstImg->au32Stride[2];
        }
    }
    break;
    case  IVE_IMAGE_TYPE_YUV420SP:
    {
        pU8 = (SC_U8 *)(SC_UL)pstImg->au64VirAddr[0];
        for (y = 0; y < height; y++)
        {
            if ( 1 != fread(pU8, width, 1, pFp))
            {
                SAMPLE_PRT("Read file fail\n");
                return SC_FAILURE;
            }

            pU8 += pstImg->au32Stride[0];
        }

        pU8 = (SC_U8 *)(SC_UL)pstImg->au64VirAddr[1];
        for (y = 0; y < height / 2; y++)
        {
            if ( 1 != fread(pU8, width, 1, pFp))
            {
                SAMPLE_PRT("Read file fail\n");
                return SC_FAILURE;
            }

            pU8 += pstImg->au32Stride[1];
        }
    }
    break;
    case IVE_IMAGE_TYPE_YUV422SP:
    {
        pU8 = (SC_U8 *)(SC_UL)pstImg->au64VirAddr[0];
        for (y = 0; y < height; y++)
        {
            if ( 1 != fread(pU8, width, 1, pFp))
            {
                SAMPLE_PRT("Read file fail\n");
                return SC_FAILURE;
            }

            pU8 += pstImg->au32Stride[0];
        }

        pU8 = (SC_U8 *)(SC_UL)pstImg->au64VirAddr[1];
        for (y = 0; y < height; y++)
        {
            if ( 1 != fread(pU8, width, 1, pFp))
            {
                SAMPLE_PRT("Read file fail\n");
                return SC_FAILURE;
            }

            pU8 += pstImg->au32Stride[1];
        }
    }
    break;
    case IVE_IMAGE_TYPE_U8C3_PACKAGE:
    {
        pU8 = (SC_U8 *)(SC_UL)pstImg->au64VirAddr[0];
        for (y = 0; y < height; y++)
        {
            if ( 1 != fread(pU8, width * 3, 1, pFp))
            {
                SAMPLE_PRT("Read file fail\n");
                return SC_FAILURE;
            }

            pU8 += pstImg->au32Stride[0];
        }

    }
    break;
    case IVE_IMAGE_TYPE_U8C3_PLANAR:
    {
        for (loop = 0; loop < 3; loop++)
        {
            pU8 = (SC_U8 *)(SC_UL)pstImg->au64VirAddr[loop];
            for (y = 0; y < height; y++)
            {
                if ( 1 != fread(pU8, width, 1, pFp))
                {
                    SAMPLE_PRT("Read file fail\n");
                    return SC_FAILURE;
                }

                pU8 += pstImg->au32Stride[loop];
            }
        }

    }
    break;
    case IVE_IMAGE_TYPE_S16C1:
    case IVE_IMAGE_TYPE_U16C1:
    {
        pU8 = (SC_U8 *)(SC_UL)pstImg->au64VirAddr[0];
        for ( y = 0; y < height; y++ )
        {
            if ( sizeof(SC_U16) != fread(pU8, width, sizeof(SC_U16), pFp) )
            {
                SAMPLE_PRT("Read file fail\n");
                return SC_FAILURE;
            }

            pU8 += pstImg->au32Stride[0] * 2;
        }
    }
    break;
    default:
        break;
    }

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_IVE_WriteFile(IVE_IMAGE_S *pstImg, FILE *pFp)
{
    SC_U16 y;
    SC_U8 *pU8;
    SC_U16 height;
    SC_U16 width;
    SC_U16 loop;

    height = pstImg->u32Height;
    width = pstImg->u32Width;

    switch (pstImg->enType)
    {
    case  IVE_IMAGE_TYPE_U8C1:
    case  IVE_IMAGE_TYPE_S8C1:
    {
        pU8 = (SC_U8 *)(SC_UL)pstImg->au64VirAddr[0];
        for (y = 0; y < height; y++)
        {
            if ( 1 != fwrite(pU8, width, 1, pFp))
            {
                SAMPLE_PRT("Write file fail\n");
                return SC_FAILURE;
            }

            pU8 += pstImg->au32Stride[0];
        }
    }
    break;
    case  IVE_IMAGE_TYPE_YUV420P:
    {
        pU8 = (SC_U8 *)(SC_UL)pstImg->au64VirAddr[0];
        for (y = 0; y < height; y++)
        {
            if ( width != fwrite(pU8, 1, width, pFp))
            {
                SAMPLE_PRT("Write file fail\n");
                return SC_FAILURE;
            }

            pU8 += pstImg->au32Stride[0];
        }

        pU8 = (SC_U8 *)(SC_UL)pstImg->au64VirAddr[1];
        for (y = 0; y < height / 2; y++)
        {
            if ( width / 2 != fwrite(pU8, 1, width / 2, pFp))
            {
                SAMPLE_PRT("Write file fail\n");
                return SC_FAILURE;
            }

            pU8 += pstImg->au32Stride[1];
        }

        pU8 = (SC_U8 *)(SC_UL)pstImg->au64VirAddr[2];
        for (y = 0; y < height / 2; y++)
        {
            if ( width / 2 != fwrite(pU8, 1, width / 2, pFp))
            {
                SAMPLE_PRT("Write file fail\n");
                return SC_FAILURE;
            }

            pU8 += pstImg->au32Stride[2];
        }
    }
    break;
    case  IVE_IMAGE_TYPE_YUV420SP:
    {
        pU8 = (SC_U8 *)(SC_UL)pstImg->au64VirAddr[0];
        for (y = 0; y < height; y++)
        {
            if ( width != fwrite(pU8, 1, width, pFp))
            {
                SAMPLE_PRT("Write file fail\n");
                return SC_FAILURE;
            }

            pU8 += pstImg->au32Stride[0];
        }

        pU8 = (SC_U8 *)(SC_UL)pstImg->au64VirAddr[1];
        for (y = 0; y < height / 2; y++)
        {
            if ( width != fwrite(pU8, 1, width, pFp))
            {
                SAMPLE_PRT("Write file fail\n");
                return SC_FAILURE;
            }

            pU8 += pstImg->au32Stride[1];
        }
    }
    break;
    case IVE_IMAGE_TYPE_YUV422SP:
    {
        pU8 = (SC_U8 *)(SC_UL)pstImg->au64VirAddr[0];
        for (y = 0; y < height; y++)
        {
            if ( width != fwrite(pU8, 1, width, pFp))
            {
                SAMPLE_PRT("Write file fail\n");
                return SC_FAILURE;
            }

            pU8 += pstImg->au32Stride[0];
        }

        pU8 = (SC_U8 *)(SC_UL)pstImg->au64VirAddr[1];
        for (y = 0; y < height; y++)
        {
            if ( width != fwrite(pU8, 1, width, pFp))
            {
                SAMPLE_PRT("Write file fail\n");
                return SC_FAILURE;
            }

            pU8 += pstImg->au32Stride[1];
        }
    }
    break;
    case IVE_IMAGE_TYPE_S16C1:
    case  IVE_IMAGE_TYPE_U16C1:
    {
        pU8 = (SC_U8 *)(SC_UL)pstImg->au64VirAddr[0];
        for ( y = 0; y < height; y++ )
        {
            if ( sizeof(SC_U16) != fwrite(pU8, width, sizeof(SC_U16), pFp) )
            {
                SAMPLE_PRT("Write file fail\n");
                return SC_FAILURE;
            }

            pU8 += pstImg->au32Stride[0] * 2;
        }
    }
    break;
    case IVE_IMAGE_TYPE_BGR888_PACKAGE:
    case IVE_IMAGE_TYPE_U8C3_PACKAGE:
    {
        printf("IVE_IMAGE_TYPE_U8C3_PACKAGE w %d, h %d, s %d\n", width, height, pstImg->au32Stride[0]);
        pU8 = (SC_U8 *)(SC_UL)pstImg->au64VirAddr[0];
        for (y = 0; y < height; y++)
        {
            if ( width * 3 != fwrite(pU8, 1, width * 3, pFp))
            {
                SAMPLE_PRT("write file fail\n");
                return SC_FAILURE;
            }

            pU8 += pstImg->au32Stride[0];
        }

    }
    break;
    case IVE_IMAGE_TYPE_BGR888_PLANAR:
    case IVE_IMAGE_TYPE_U8C3_PLANAR:
    {
        for (loop = 0; loop < 3; loop++)
        {
            pU8 = (SC_U8 *)(SC_UL)pstImg->au64VirAddr[loop];
            for (y = 0; y < height; y++)
            {
                if ( width != fwrite(pU8, 1, width, pFp))
                {
                    SAMPLE_PRT("write file fail\n");
                    return SC_FAILURE;
                }

                pU8 += pstImg->au32Stride[loop];
            }
        }

    }
    break;
    case IVE_IMAGE_TYPE_U32C1:
    {

        pU8 = (SC_U8 *)(SC_UL)pstImg->au64VirAddr[0];
        for ( y = 0; y < height; y++ )
        {
            if ( width != fwrite(pU8, sizeof(SC_U32), width, pFp) )
            {
                SAMPLE_PRT("Write file fail\n");
                return SC_FAILURE;
            }

            pU8 += pstImg->au32Stride[0] * 4;
        }
        break;
    }

    default:
        break;
    }

    return SC_SUCCESS;
}

SC_VOID SAMPLE_COMM_IVE_BlobToRect(IVE_CCBLOB_S *pstBlob, SAMPLE_RECT_ARRAY_S *pstRect,
    SC_U16 u16RectMaxNum, SC_U16 u16AreaThrStep,
    SC_U32 u32SrcWidth, SC_U32 u32SrcHeight,
    SC_U32 u32DstWidth, SC_U32 u32DstHeight)
{
    SC_U16 u16Num;
    SC_U16 i, j, k;
    SC_U32 u16Thr = 0;
    SC_BOOL bValid;

    if(pstBlob->u8RegionNum > u16RectMaxNum)
    {
        u16Thr = pstBlob->u16CurAreaThr;
        do
        {
            u16Num = 0;
            u16Thr += u16AreaThrStep;
            for(i = 0; i < 254; i++)
            {
                if(pstBlob->astRegion[i].u32Area > u16Thr)
                {
                    u16Num++;
                }
            }
        } while(u16Num > u16RectMaxNum);
    }

    u16Num = 0;
    for(i = 0; i < 254; i++)
    {
        if(pstBlob->astRegion[i].u32Area > u16Thr)
        {
            pstRect->astRect[u16Num].astPoint[0].s32X = (SC_U32)((SC_FLOAT)pstBlob->astRegion[i].u16Left / (SC_FLOAT)u32SrcWidth *
                    (SC_FLOAT)u32DstWidth) & (~1) ;
            pstRect->astRect[u16Num].astPoint[0].s32Y = (SC_U32)((SC_FLOAT)pstBlob->astRegion[i].u16Top / (SC_FLOAT)u32SrcHeight *
                    (SC_FLOAT)u32DstHeight) & (~1);

            pstRect->astRect[u16Num].astPoint[1].s32X = (SC_U32)((SC_FLOAT)pstBlob->astRegion[i].u16Right /
                    (SC_FLOAT)u32SrcWidth * (SC_FLOAT)u32DstWidth) & (~1);
            pstRect->astRect[u16Num].astPoint[1].s32Y = (SC_U32)((SC_FLOAT)pstBlob->astRegion[i].u16Top / (SC_FLOAT)u32SrcHeight *
                    (SC_FLOAT)u32DstHeight) & (~1);

            pstRect->astRect[u16Num].astPoint[2].s32X = (SC_U32)((SC_FLOAT)pstBlob->astRegion[i].u16Right /
                    (SC_FLOAT)u32SrcWidth * (SC_FLOAT)u32DstWidth) & (~1);
            pstRect->astRect[u16Num].astPoint[2].s32Y = (SC_U32)((SC_FLOAT)pstBlob->astRegion[i].u16Bottom /
                    (SC_FLOAT)u32SrcHeight * (SC_FLOAT)u32DstHeight) & (~1);

            pstRect->astRect[u16Num].astPoint[3].s32X = (SC_U32)((SC_FLOAT)pstBlob->astRegion[i].u16Left / (SC_FLOAT)u32SrcWidth *
                    (SC_FLOAT)u32DstWidth) & (~1);
            pstRect->astRect[u16Num].astPoint[3].s32Y = (SC_U32)((SC_FLOAT)pstBlob->astRegion[i].u16Bottom /
                    (SC_FLOAT)u32SrcHeight * (SC_FLOAT)u32DstHeight) & (~1);

            bValid = SC_TRUE;
            for(j = 0; j < 3; j++)
            {
                for (k = j + 1; k < 4; k++)
                {
                    if ((pstRect->astRect[u16Num].astPoint[j].s32X == pstRect->astRect[u16Num].astPoint[k].s32X)
                        && (pstRect->astRect[u16Num].astPoint[j].s32Y == pstRect->astRect[u16Num].astPoint[k].s32Y))
                    {
                        bValid = SC_FALSE;
                        break;
                    }
                }
            }
            if (SC_TRUE == bValid)
            {
                u16Num++;
            }
        }
    }

    pstRect->u16Num = u16Num;
    printf("out rect %d!\n", u16Num);
}

/******************************************************************************
* function : Create ive image
******************************************************************************/
SC_S32 SAMPLE_COMM_IVE_CreateImage(IVE_IMAGE_S *pstImg, IVE_IMAGE_TYPE_E enType, SC_U32 u32Width, SC_U32 u32Height)
{
    SC_U32 u32Size = 0;
    SC_S32 s32Ret;
    if (NULL == pstImg)
    {
        SAMPLE_PRT("pstImg is null\n");
        return SC_FAILURE;
    }

    pstImg->enType = enType;
    pstImg->u32Width = u32Width;
    pstImg->u32Height = u32Height;
    pstImg->au32Stride[0] = SAMPLE_COMM_IVE_CalcStride(pstImg->u32Width, 32);//IVE_ALIGN);

    switch (enType)
    {
    case IVE_IMAGE_TYPE_U8C1:
    case IVE_IMAGE_TYPE_S8C1:
    {
        u32Size = pstImg->au32Stride[0] * pstImg->u32Height;
        s32Ret = SC_MPI_SYS_MmzAlloc(&pstImg->au64PhyAddr[0], (SC_VOID **)&pstImg->au64VirAddr[0], NULL, SC_NULL, u32Size);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("Mmz Alloc fail,Error(%#x)\n", s32Ret);
            return s32Ret;
        }
    }
    break;
    case IVE_IMAGE_TYPE_YUV420SP:
    {
        u32Size = pstImg->au32Stride[0] * pstImg->u32Height * 3 / 2;
        s32Ret = SC_MPI_SYS_MmzAlloc(&pstImg->au64PhyAddr[0], (SC_VOID **)&pstImg->au64VirAddr[0], NULL, SC_NULL, u32Size);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("Mmz Alloc fail,Error(%#x)\n", s32Ret);
            return s32Ret;
        }
        pstImg->au32Stride[1] = pstImg->au32Stride[0];
        pstImg->au64PhyAddr[1] = pstImg->au64PhyAddr[0] + pstImg->au32Stride[0] * pstImg->u32Height;
        pstImg->au64VirAddr[1] = pstImg->au64VirAddr[0] + pstImg->au32Stride[0] * pstImg->u32Height;

    }
    break;
    case IVE_IMAGE_TYPE_YUV422SP:
    {
        u32Size = pstImg->au32Stride[0] * pstImg->u32Height * 2;
        s32Ret = SC_MPI_SYS_MmzAlloc(&pstImg->au64PhyAddr[0], (SC_VOID **)&pstImg->au64VirAddr[0], NULL, SC_NULL, u32Size);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("Mmz Alloc fail,Error(%#x)\n", s32Ret);
            return s32Ret;
        }
        pstImg->au32Stride[1] = pstImg->au32Stride[0];
        pstImg->au64PhyAddr[1] = pstImg->au64PhyAddr[0] + pstImg->au32Stride[0] * pstImg->u32Height;
        pstImg->au64VirAddr[1] = pstImg->au64VirAddr[0] + pstImg->au32Stride[0] * pstImg->u32Height;

    }
    break;
    case IVE_IMAGE_TYPE_YUV420P:
        u32Size = pstImg->au32Stride[0] * pstImg->u32Height * 3 / 2;
        s32Ret = SC_MPI_SYS_MmzAlloc(&pstImg->au64PhyAddr[0], (SC_VOID **)&pstImg->au64VirAddr[0], NULL, SC_NULL, u32Size);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("Mmz Alloc fail,Error(%#x)\n", s32Ret);
            return s32Ret;
        }
        pstImg->au32Stride[1] = pstImg->au32Stride[0] / 2;
        pstImg->au64PhyAddr[1] = pstImg->au64PhyAddr[0] + pstImg->au32Stride[0] * pstImg->u32Height;
        pstImg->au64VirAddr[1] = pstImg->au64VirAddr[0] + pstImg->au32Stride[0] * pstImg->u32Height;
        pstImg->au32Stride[2] = pstImg->au32Stride[1];
        pstImg->au64PhyAddr[2] = pstImg->au64PhyAddr[1] + pstImg->au32Stride[0] * pstImg->u32Height / 4;
        pstImg->au64VirAddr[2] = pstImg->au64VirAddr[1] + pstImg->au32Stride[0] * pstImg->u32Height / 4;
        break;
    case IVE_IMAGE_TYPE_YUV422P:
        break;
    case IVE_IMAGE_TYPE_S8C2_PACKAGE:
        break;
    case IVE_IMAGE_TYPE_S8C2_PLANAR:
        break;
    case IVE_IMAGE_TYPE_S16C1:
    case IVE_IMAGE_TYPE_U16C1:
    {

        u32Size = pstImg->au32Stride[0] * pstImg->u32Height * sizeof(SC_U16);
        s32Ret = SC_MPI_SYS_MmzAlloc(&pstImg->au64PhyAddr[0], (SC_VOID **)&pstImg->au64VirAddr[0], NULL, SC_NULL, u32Size);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("Mmz Alloc fail,Error(%#x)\n", s32Ret);
            return s32Ret;
        }
    }
    break;
    case IVE_IMAGE_TYPE_BGR888_PACKAGE:
    case IVE_IMAGE_TYPE_U8C3_PACKAGE:
    {
        pstImg->au32Stride[0] = SAMPLE_COMM_IVE_CalcStride(pstImg->u32Width, IVE_ALIGN) * 3;
        u32Size = pstImg->au32Stride[0] * pstImg->u32Height;
        s32Ret = SC_MPI_SYS_MmzAlloc(&pstImg->au64PhyAddr[0], (SC_VOID **)&pstImg->au64VirAddr[0], NULL, SC_NULL, u32Size);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("Mmz Alloc fail,Error(%#x)\n", s32Ret);
            return s32Ret;
        }
        pstImg->au64VirAddr[1] = 0;
        pstImg->au64VirAddr[2] = 0;
        pstImg->au64PhyAddr[1] = 0;
        pstImg->au64PhyAddr[2] = 0;
        pstImg->au32Stride[1] = 0;
        pstImg->au32Stride[2] = 0;
    }
    break;
    case IVE_IMAGE_TYPE_BGR888_PLANAR:
    case IVE_IMAGE_TYPE_U8C3_PLANAR:
    {
        u32Size = pstImg->au32Stride[0] * pstImg->u32Height * 3;
        s32Ret = SC_MPI_SYS_MmzAlloc(&pstImg->au64PhyAddr[0], (SC_VOID **)&pstImg->au64VirAddr[0], NULL, SC_NULL, u32Size);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("Mmz Alloc fail,Error(%#x)\n", s32Ret);
            return s32Ret;
        }
        pstImg->au32Stride[1] = pstImg->au32Stride[0];
        pstImg->au64VirAddr[1] = pstImg->au64VirAddr[0] + pstImg->au32Stride[0] * pstImg->u32Height;
        pstImg->au64PhyAddr[1] = pstImg->au64PhyAddr[0] + pstImg->au32Stride[0] * pstImg->u32Height;
        pstImg->au32Stride[2] = pstImg->au32Stride[1];
        pstImg->au64VirAddr[2] = pstImg->au64VirAddr[1] + pstImg->au32Stride[1] * pstImg->u32Height;
        pstImg->au64PhyAddr[2] = pstImg->au64PhyAddr[1] + pstImg->au32Stride[1] * pstImg->u32Height;
    }
    break;
    case IVE_IMAGE_TYPE_S32C1:
    case IVE_IMAGE_TYPE_U32C1:
    {
        u32Size = pstImg->au32Stride[0] * pstImg->u32Height * sizeof(SC_U32);
        s32Ret = SC_MPI_SYS_MmzAlloc(&pstImg->au64PhyAddr[0], (SC_VOID **)&pstImg->au64VirAddr[0], NULL, SC_NULL, u32Size);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("Mmz Alloc fail,Error(%#x)\n", s32Ret);
            return s32Ret;
        }
    }
    break;
    case IVE_IMAGE_TYPE_S64C1:
    case IVE_IMAGE_TYPE_U64C1:
    {

        u32Size = pstImg->au32Stride[0] * pstImg->u32Height * sizeof(SC_U64);
        s32Ret = SC_MPI_SYS_MmzAlloc(&pstImg->au64PhyAddr[0], (SC_VOID **)&pstImg->au64VirAddr[0], NULL, SC_NULL, u32Size);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("Mmz Alloc fail,Error(%#x)\n", s32Ret);
            return s32Ret;
        }
    }
    break;
    default:
        break;

    }

    return SC_SUCCESS;
}
/******************************************************************************
* function : Create memory info
******************************************************************************/
SC_S32 SAMPLE_COMM_IVE_CreateMemInfo(IVE_MEM_INFO_S *pstMemInfo, SC_U32 u32Size)
{
    SC_S32 s32Ret;

    if (NULL == pstMemInfo)
    {
        SAMPLE_PRT("pstMemInfo is null\n");
        return SC_FAILURE;
    }
    pstMemInfo->u32Size = u32Size;
    s32Ret = SC_MPI_SYS_MmzAlloc(&pstMemInfo->u64PhyAddr, (SC_VOID **)&pstMemInfo->u64VirAddr, NULL, SC_NULL, u32Size);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("Mmz Alloc fail,Error(%#x)\n", s32Ret);
        return SC_FAILURE;
    }

    return SC_SUCCESS;
}
/******************************************************************************
* function : Create ive image by cached
******************************************************************************/
SC_S32 SAMPLE_COMM_IVE_CreateImageByCached(IVE_IMAGE_S *pstImg,
    IVE_IMAGE_TYPE_E enType, SC_U32 u32Width, SC_U32 u32Height)
{
    SC_U32 u32Size = 0;
    SC_S32 s32Ret;
    if (NULL == pstImg)
    {
        SAMPLE_PRT("pstImg is null\n");
        return SC_FAILURE;
    }

    pstImg->enType = enType;
    pstImg->u32Width = u32Width;
    pstImg->u32Height = u32Height;
    pstImg->au32Stride[0] = SAMPLE_COMM_IVE_CalcStride(pstImg->u32Width, IVE_ALIGN);

    switch (enType)
    {
    case IVE_IMAGE_TYPE_U8C1:
    case IVE_IMAGE_TYPE_S8C1:
    {
        u32Size = pstImg->au32Stride[0] * pstImg->u32Height;
        s32Ret = SC_MPI_SYS_MmzAlloc_Cached(&pstImg->au64PhyAddr[0], (SC_VOID **)&pstImg->au64VirAddr[0], NULL, SC_NULL,
                u32Size);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("Mmz Alloc fail,Error(%#x)\n", s32Ret);
            return s32Ret;
        }
    }
    break;
    case IVE_IMAGE_TYPE_YUV420SP:
        break;
    case IVE_IMAGE_TYPE_YUV422SP:
        break;
    case IVE_IMAGE_TYPE_YUV420P:
        break;
    case IVE_IMAGE_TYPE_YUV422P:
        break;
    case IVE_IMAGE_TYPE_S8C2_PACKAGE:
        break;
    case IVE_IMAGE_TYPE_S8C2_PLANAR:
        break;
    case IVE_IMAGE_TYPE_S16C1:
    case IVE_IMAGE_TYPE_U16C1:
    {

        u32Size = pstImg->au32Stride[0] * pstImg->u32Height * sizeof(SC_U16);
        s32Ret = SC_MPI_SYS_MmzAlloc_Cached(&pstImg->au64PhyAddr[0], (SC_VOID **)&pstImg->au64VirAddr[0], NULL, SC_NULL,
                u32Size);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("Mmz Alloc fail,Error(%#x)\n", s32Ret);
            return s32Ret;
        }
    }
    break;
    case IVE_IMAGE_TYPE_U8C3_PACKAGE:
        break;
    case IVE_IMAGE_TYPE_U8C3_PLANAR:
        break;
    case IVE_IMAGE_TYPE_S32C1:
    case IVE_IMAGE_TYPE_U32C1:
    {
        u32Size = pstImg->au32Stride[0] * pstImg->u32Height * sizeof(SC_U32);
        s32Ret = SC_MPI_SYS_MmzAlloc_Cached(&pstImg->au64PhyAddr[0], (SC_VOID **)&pstImg->au64VirAddr[0], NULL, SC_NULL,
                u32Size);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("Mmz Alloc fail,Error(%#x)\n", s32Ret);
            return s32Ret;
        }
    }
    break;
    case IVE_IMAGE_TYPE_S64C1:
    case IVE_IMAGE_TYPE_U64C1:
    {

        u32Size = pstImg->au32Stride[0] * pstImg->u32Height * sizeof(SC_U64);
        s32Ret = SC_MPI_SYS_MmzAlloc_Cached(&pstImg->au64PhyAddr[0], (SC_VOID **)&pstImg->au64VirAddr[0], NULL, SC_NULL,
                u32Size);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("Mmz Alloc fail,Error(%#x)\n", s32Ret);
            return s32Ret;
        }
    }
    break;
    default:
        break;
    }

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_IVE_CreateData(IVE_DATA_S *pstData, SC_U32 u32Width, SC_U32 u32Height)
{
    SC_S32 s32Ret;
    SC_U32 u32Size;

    if (NULL == pstData)
    {
        SAMPLE_PRT("pstData is null\n");
        return SC_FAILURE;
    }
    pstData->u32Width = u32Width;
    pstData->u32Height = u32Height;
    pstData->u32Stride = SAMPLE_COMM_IVE_CalcStride(pstData->u32Width, IVE_ALIGN);
    u32Size = pstData->u32Stride * pstData->u32Height;
    s32Ret = SC_MPI_SYS_MmzAlloc(&pstData->u64PhyAddr, (SC_VOID **)&pstData->u64VirAddr, NULL, SC_NULL, u32Size);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("Mmz Alloc fail,Error(%#x)\n", s32Ret);
        return SC_FAILURE;
    }

    return SC_SUCCESS;
}
/******************************************************************************
* function : Init Vb
******************************************************************************/
SC_S32 SAMPLE_COMM_IVE_VbInit(PIC_SIZE_E *paenSize, SIZE_S *pastSize, SC_U32 u32VpssChnNum)
{
    SC_S32 s32Ret;
    SC_U32 i;
    SC_U64 u64BlkSize;
    VB_CONFIG_S stVbConf;

    memset(&stVbConf, 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt = 128;

    for (i = 0; i < u32VpssChnNum; i++)
    {
        s32Ret = SAMPLE_COMM_SYS_GetPicSize(paenSize[i], &pastSize[i]);
        SAMPLE_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, VB_FAIL_0,
            "SAMPLE_COMM_SYS_GetPicSize failed,Error(%#x)!\n", s32Ret);

        u64BlkSize = COMMON_GetPicBufferSize(pastSize[i].u32Width, pastSize[i].u32Height,
                SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
        /* comm video buffer */
        stVbConf.astCommPool[i].u64BlkSize = u64BlkSize;
        stVbConf.astCommPool[i].u32BlkCnt  = 10;
    }

    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    SAMPLE_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, VB_FAIL_1,
        "SAMPLE_COMM_SYS_Init failed,Error(%#x)!\n", s32Ret);

    return s32Ret;
VB_FAIL_1:
    SAMPLE_COMM_SYS_Exit();
VB_FAIL_0:

    return s32Ret;
}

/******************************************************************************
* function : Dma frame info to  ive image
******************************************************************************/
SC_S32 SAMPLE_COMM_IVE_DmaImage(VIDEO_FRAME_INFO_S *pstFrameInfo, IVE_DST_IMAGE_S *pstDst, SC_BOOL bInstant)
{
    SC_U32 u32Len = pstFrameInfo->stVFrame.u32Stride[0] * pstFrameInfo->stVFrame.u32Height;
    SC_U8 *pu8VirAddr = (SC_U8 *)SC_MPI_SYS_Mmap(pstFrameInfo->stVFrame.u64PhyAddr[0], u32Len);
    for (size_t i = 0; i < pstFrameInfo->stVFrame.u32Height; i++)
    {
        memcpy((void*)(SC_ULONG)(pstDst->au64VirAddr[0] + i * pstDst->au32Stride[0]), pu8VirAddr + i * pstFrameInfo->stVFrame.u32Stride[0],
            pstFrameInfo->stVFrame.u32Width);
    }
    SC_MPI_SYS_Munmap((SC_VOID *)pu8VirAddr, u32Len);

    return SC_SUCCESS;

    #if 0    //SC_MPI_IVE_DMA not define
    SC_S32 s32Ret;
    IVE_HANDLE hIveHandle;
    IVE_SRC_DATA_S stSrcData;
    IVE_DST_DATA_S stDstData;
    IVE_DMA_CTRL_S stCtrl = {IVE_DMA_MODE_DIRECT_COPY, 0};
    SC_BOOL bFinish = SC_FALSE;
    SC_BOOL bBlock = SC_TRUE;

    //fill src
    //stSrcData.u64VirAddr = pstFrameInfo->stVFrame.u64VirAddr[0];
    stSrcData.u64PhyAddr = pstFrameInfo->stVFrame.u64PhyAddr[0];
    stSrcData.u32Width   = pstFrameInfo->stVFrame.u32Width;
    stSrcData.u32Height  = pstFrameInfo->stVFrame.u32Height;
    stSrcData.u32Stride  = pstFrameInfo->stVFrame.u32Stride[0];

    //fill dst
    //stDstData.u64VirAddr = pstDst->au64VirAddr[0];
    stDstData.u64PhyAddr = pstDst->au64PhyAddr[0];
    stDstData.u32Width   = pstDst->u32Width;
    stDstData.u32Height  = pstDst->u32Height;
    stDstData.u32Stride  = pstDst->au32Stride[0];

    //fuyuming
    s32Ret = SC_MPI_IVE_DMA(&hIveHandle, &stSrcData, &stDstData, &stCtrl, bInstant);
    SAMPLE_CHECK_EXPR_RET(SC_SUCCESS != s32Ret, s32Ret, "Error(%#x),SC_MPI_IVE_DMA failed!\n", s32Ret);

    if (SC_TRUE == bInstant)
    {
        s32Ret = SC_MPI_IVE_Query(hIveHandle, &bFinish, bBlock);
        while(SC_ERR_IVE_QUERY_TIMEOUT == s32Ret)
        {
            usleep(100);
            s32Ret = SC_MPI_IVE_Query(hIveHandle, &bFinish, bBlock);
        }
        SAMPLE_CHECK_EXPR_RET(SC_SUCCESS != s32Ret, s32Ret, "Error(%#x),SC_MPI_IVE_Query failed!\n", s32Ret);
    }

    return SC_SUCCESS;
    #endif
}

/******************************************************************************
* function : Start Vpss
******************************************************************************/
SC_S32 SAMPLE_COMM_IVE_StartVpss(SIZE_S *pastSize, SC_U32 u32VpssChnNum)
{
    SC_S32 i;
    VPSS_CHN_ATTR_S astVpssChnAttr[VPSS_MAX_CHN_NUM];
    VPSS_GRP_ATTR_S stVpssGrpAttr = {0};
    SC_BOOL abChnEnable[VPSS_MAX_CHN_NUM] = {SC_FALSE, SC_FALSE, SC_FALSE, SC_FALSE};
    VPSS_GRP VpssGrp = 0;

    stVpssGrpAttr.u32MaxW = 1920;
    stVpssGrpAttr.u32MaxH = 1080;
    stVpssGrpAttr.stFrameRate.s32SrcFrameRate = -1;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate = -1;
    stVpssGrpAttr.enDynamicRange = DYNAMIC_RANGE_SDR8;
    stVpssGrpAttr.enPixelFormat  = PIXEL_FORMAT_YVU_PLANAR_420;
    //stVpssGrpAttr.stNrAttr.enCompressMode = COMPRESS_MODE_FRAME;     //fuyuming
    stVpssGrpAttr.stNrAttr.enNrMotionMode = NR_MOTION_MODE_NORMAL;

    for (i = 0; i < u32VpssChnNum; i++)
    {
        abChnEnable[i] = SC_TRUE;
    }

    for(i = 0; i < VPSS_MAX_CHN_NUM; i++)
    {
        astVpssChnAttr[i].u32Width                    = pastSize[i].u32Width;
        astVpssChnAttr[i].u32Height                   = pastSize[i].u32Height;
        astVpssChnAttr[i].enChnMode                   = VPSS_CHN_MODE_USER;
        astVpssChnAttr[i].enCompressMode              = COMPRESS_MODE_NONE;
        astVpssChnAttr[i].enDynamicRange              = DYNAMIC_RANGE_SDR8;
        astVpssChnAttr[i].enVideoFormat               = VIDEO_FORMAT_LINEAR;
        astVpssChnAttr[i].enPixelFormat               = PIXEL_FORMAT_YVU_PLANAR_420;
        astVpssChnAttr[i].stFrameRate.s32SrcFrameRate = -1;
        astVpssChnAttr[i].stFrameRate.s32DstFrameRate = -1;
        astVpssChnAttr[i].u32Depth                    = 1;
        astVpssChnAttr[i].bMirror                     = SC_FALSE;
        astVpssChnAttr[i].bFlip                       = SC_FALSE;
        astVpssChnAttr[i].stAspectRatio.enMode        = ASPECT_RATIO_NONE;
    }

    return SAMPLE_COMM_VPSS_Start(VpssGrp, abChnEnable, &stVpssGrpAttr, &astVpssChnAttr[0]);

}
/******************************************************************************
* function : Stop Vpss
******************************************************************************/
SC_VOID SAMPLE_COMM_IVE_StopVpss(SC_U32 u32VpssChnNum)
{
    VPSS_GRP VpssGrp = 0;
    SC_BOOL abChnEnable[VPSS_MAX_CHN_NUM] = {SC_FALSE, SC_FALSE, SC_FALSE, SC_FALSE};
    SC_S32 i = 0;

    for (i = 0; i < u32VpssChnNum; i++)
    {
        abChnEnable[i] = SC_TRUE;
    }

    (SC_VOID)SAMPLE_COMM_VPSS_Stop(VpssGrp, abChnEnable);

    return;
}

/******************************************************************************
* function : Start Vo
******************************************************************************/
SC_S32 SAMPLE_COMM_IVE_StartVo(SC_VOID)
{
    SC_S32 s32Ret;
    VO_DEV VoDev = SAMPLE_VO_DEV_DHD0;
    VO_LAYER VoLayer = 0;
    VO_PUB_ATTR_S stVoPubAttr;
    VO_VIDEO_LAYER_ATTR_S stLayerAttr;
    SAMPLE_VO_MODE_E enVoMode = VO_MODE_1MUX;
    SC_U32 u32DisBufLen = 3;

    stVoPubAttr.enIntfSync = VO_OUTPUT_1080P60;
    stVoPubAttr.enIntfType = VO_INTF_HDMI;
    stVoPubAttr.u32BgColor = COLOR_RGB_BLUE;
    s32Ret = SAMPLE_COMM_VO_StartDev(VoDev, &stVoPubAttr);
    SAMPLE_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, VO_FAIL_0,
        "SAMPLE_COMM_VO_StartDev failed,Error(%#x)!\n", s32Ret);

    //s32Ret = SAMPLE_COMM_VO_HdmiStart(stVoPubAttr.enIntfSync);
    //SAMPLE_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, VO_FAIL_1,
    //    "SAMPLE_COMM_VO_HdmiStart failed,Error(%#x)!\n",s32Ret);

    s32Ret = SAMPLE_COMM_VO_GetWH(stVoPubAttr.enIntfSync, &stLayerAttr.stDispRect.u32Width,
            &stLayerAttr.stDispRect.u32Height, &stLayerAttr.u32DispFrmRt);
    SAMPLE_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, VO_FAIL_2,
        "SAMPLE_COMM_VO_GetWH failed,Error(%#x)!\n", s32Ret);

    s32Ret = SC_MPI_VO_SetDisplayBufLen(VoLayer, u32DisBufLen);
    SAMPLE_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, VO_FAIL_2,
        "SC_MPI_VO_SetDisplayBufLen failed,Error(%#x)!\n", s32Ret);

    stLayerAttr.stDispRect.s32X        = 0;
    stLayerAttr.stDispRect.s32Y        = 0;
    stLayerAttr.stImageSize.u32Width = stLayerAttr.stDispRect.u32Width;
    stLayerAttr.stImageSize.u32Height = stLayerAttr.stDispRect.u32Height;
    stLayerAttr.bDoubleFrame        = SC_FALSE;
    stLayerAttr.bClusterMode        = SC_FALSE;
    stLayerAttr.enPixFormat            = PIXEL_FORMAT_YVU_PLANAR_420;
    stLayerAttr.enDstDynamicRange     = DYNAMIC_RANGE_SDR8;

    s32Ret = SAMPLE_COMM_VO_StartLayer(VoLayer, &stLayerAttr);
    SAMPLE_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, VO_FAIL_2,
        "SAMPLE_COMM_VO_StartLayer failed,Error(%#x)!\n", s32Ret);

    s32Ret = SAMPLE_COMM_VO_StartChn(VoLayer, enVoMode);
    SAMPLE_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, VO_FAIL_3,
        "SAMPLE_COMM_VO_StartChn failed,Error(%#x)!\n", s32Ret);

    return s32Ret;
VO_FAIL_3:
    SAMPLE_COMM_VO_StopLayer(VoLayer);
VO_FAIL_2:
    //SAMPLE_COMM_VO_HdmiStop();
VO_FAIL_1:
    SAMPLE_COMM_VO_StopDev(VoDev);
VO_FAIL_0:
    return s32Ret;
}
/******************************************************************************
* function : Stop Vo
******************************************************************************/
SC_VOID SAMPLE_COMM_IVE_StopVo(SC_VOID)
{
    VO_DEV VoDev = SAMPLE_VO_DEV_DHD0;
    VO_LAYER VoLayer = 0;
    SAMPLE_VO_MODE_E enVoMode = VO_MODE_1MUX;

    (SC_VOID)SAMPLE_COMM_VO_StopChn(VoDev, enVoMode);
    (SC_VOID)SAMPLE_COMM_VO_StopLayer(VoLayer);
    //SAMPLE_COMM_VO_HdmiStop();
    (SC_VOID)SAMPLE_COMM_VO_StopDev(VoDev);
}
/******************************************************************************
* function : Start Vi/Vpss/Venc/Vo
******************************************************************************/
SC_S32 SAMPLE_COMM_IVE_StartViVpssVencVo(SAMPLE_VI_CONFIG_S *pstViConfig,
    SAMPLE_IVE_SWITCH_S *pstSwitch, PIC_SIZE_E *penExtPicSize)
{
    SIZE_S astSize[VPSS_CHN_NUM];
    PIC_SIZE_E aenSize[VPSS_CHN_NUM];
    VI_CHN_ATTR_S stViChnAttr;
    SAMPLE_RC_E enRcMode = SAMPLE_RC_CBR;
    PAYLOAD_TYPE_E enStreamType = PT_H264;
    VENC_GOP_ATTR_S stGopAttr;
    VI_DEV ViDev0 = 0;
    VI_PIPE ViPipe0 = 0;
    VI_CHN ViChn = 0;
    SC_S32 s32ViCnt = 1;
    SC_S32 s32WorkSnsId  = 0;
    VPSS_GRP VpssGrp = 0;
    SC_S32 s32Ret = SC_SUCCESS;
    VENC_CHN VeH264Chn = 0;
    WDR_MODE_E enWDRMode = WDR_MODE_NONE;
    DYNAMIC_RANGE_E enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E enPixFormat = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E enVideoFormat = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E enCompressMode = COMPRESS_MODE_NONE;
    VI_VPSS_MODE_E enMastPipeMode = VI_ONLINE_VPSS_OFFLINE;

    memset(pstViConfig, 0, sizeof(*pstViConfig));

    SAMPLE_COMM_VI_GetSensorInfo(pstViConfig);
    pstViConfig->s32WorkingViNum                           = s32ViCnt;

    pstViConfig->as32WorkingViId[0]                        = 0;
    pstViConfig->astViInfo[0].stSnsInfo.MipiDev            = SAMPLE_COMM_VI_GetComboDevBySensor(
            pstViConfig->astViInfo[0].stSnsInfo.enSnsType, 0);

    pstViConfig->astViInfo[0].stDevInfo.ViDev              = ViDev0;
    pstViConfig->astViInfo[0].stDevInfo.enWDRMode          = enWDRMode;

    pstViConfig->astViInfo[0].stPipeInfo.enMastPipeMode    = enMastPipeMode;
    pstViConfig->astViInfo[0].stPipeInfo.aPipe[0]          = ViPipe0;
    pstViConfig->astViInfo[0].stPipeInfo.aPipe[1]          = -1;
    pstViConfig->astViInfo[0].stPipeInfo.aPipe[2]          = -1;
    pstViConfig->astViInfo[0].stPipeInfo.aPipe[3]          = -1;

    pstViConfig->astViInfo[0].stChnInfo.ViChn              = ViChn;
    pstViConfig->astViInfo[0].stChnInfo.enPixFormat        = enPixFormat;
    pstViConfig->astViInfo[0].stChnInfo.enDynamicRange     = enDynamicRange;
    pstViConfig->astViInfo[0].stChnInfo.enVideoFormat      = enVideoFormat;
    pstViConfig->astViInfo[0].stChnInfo.enCompressMode     = enCompressMode;

    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(pstViConfig->astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &aenSize[0]);
    SAMPLE_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, END_INIT_0,
        "Error(%#x),SAMPLE_COMM_VI_GetSizeBySensor failed!\n", s32Ret);
    aenSize[1] = *penExtPicSize;
    /******************************************
     step  1: Init vb
    ******************************************/
    s32Ret = SAMPLE_COMM_IVE_VbInit(aenSize, astSize, VPSS_CHN_NUM);
    SAMPLE_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, END_INIT_0,
        "Error(%#x),SAMPLE_COMM_IVE_VbInit failed!\n", s32Ret);
    /******************************************
     step 2: Start vi
    ******************************************/
    s32Ret = SAMPLE_COMM_VI_SetParam(pstViConfig);
    SAMPLE_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, END_INIT_1,
        "Error(%#x),SAMPLE_COMM_VI_SetParam failed!\n", s32Ret);
    s32Ret = SAMPLE_COMM_VI_StartVi(pstViConfig);
    SAMPLE_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, END_INIT_1,
        "Error(%#x),SAMPLE_COMM_VI_StartVi failed!\n", s32Ret);
    /******************************************
     step 3: Start vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_IVE_StartVpss(astSize, VPSS_CHN_NUM);
    SAMPLE_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, END_INIT_2,
        "Error(%#x),SAMPLE_IVS_StartVpss failed!\n", s32Ret);
    /******************************************
      step 4: Bind vpss to vi
     ******************************************/
    s32Ret = SAMPLE_COMM_VI_Bind_VPSS(ViPipe0, ViChn, VpssGrp);
    SAMPLE_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, END_INIT_3,
        "Error(%#x),SAMPLE_COMM_VI_BindVpss failed!\n", s32Ret);
    //Set vi frame
    s32Ret = SC_MPI_VI_GetChnAttr(ViPipe0, ViChn, &stViChnAttr);
    SAMPLE_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, END_INIT_4,
        "Error(%#x),SC_MPI_VI_GetChnAttr failed!\n", s32Ret);
    s32Ret = SC_MPI_VI_SetChnAttr(ViPipe0, ViChn, &stViChnAttr);
    SAMPLE_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, END_INIT_4,
        "Error(%#x),SC_MPI_VI_SetChnAttr failed!\n", s32Ret);
    /******************************************
     step 5: Start Vo
     ******************************************/
    if (SC_TRUE == pstSwitch->bVo)
    {
        s32Ret = SAMPLE_COMM_IVE_StartVo();
        SAMPLE_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, END_INIT_4,
            "Error(%#x),SAMPLE_COMM_IVE_StartVo failed!\n", s32Ret);
    }
    /******************************************
     step 6: Start Venc
    ******************************************/
    if (SC_TRUE == pstSwitch->bVenc)
    {
        s32Ret = SAMPLE_COMM_VENC_GetGopAttr(VENC_GOPMODE_NORMALP, &stGopAttr);
        SAMPLE_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, END_INIT_5,
            "Error(%#x),SAMPLE_COMM_VENC_GetGopAttr failed!\n", s32Ret);
        s32Ret = SAMPLE_COMM_VENC_Start(VeH264Chn, enStreamType, aenSize[0], enRcMode, 0, &stGopAttr);
        SAMPLE_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, END_INIT_5,
            "Error(%#x),SAMPLE_COMM_VENC_Start failed!\n", s32Ret);
        s32Ret = SAMPLE_COMM_VENC_StartGetStream(&VeH264Chn, 1);
        SAMPLE_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, END_INIT_6,
            "Error(%#x),SAMPLE_COMM_VENC_StartGetStream failed!\n", s32Ret);
    }

    return SC_SUCCESS;

END_INIT_6:
    if (SC_TRUE == pstSwitch->bVenc)
    {
        SAMPLE_COMM_VENC_Stop(VeH264Chn);
    }
END_INIT_5:
    if (SC_TRUE == pstSwitch->bVo)
    {
        SAMPLE_COMM_IVE_StopVo();
    }
END_INIT_4:
    SAMPLE_COMM_VI_UnBind_VPSS(ViPipe0, ViChn, VpssGrp);
END_INIT_3:
    SAMPLE_COMM_IVE_StopVpss(VPSS_CHN_NUM);
END_INIT_2:
    SAMPLE_COMM_VI_StopVi(pstViConfig);
END_INIT_1:    //system exit
    SAMPLE_COMM_SYS_Exit();
    memset(pstViConfig, 0, sizeof(*pstViConfig));
END_INIT_0:

    return s32Ret;
}
/******************************************************************************
* function : Stop Vi/Vpss/Venc/Vo
******************************************************************************/
SC_VOID SAMPLE_COMM_IVE_StopViVpssVencVo(SAMPLE_VI_CONFIG_S *pstViConfig, SAMPLE_IVE_SWITCH_S *pstSwitch)
{
    if (SC_TRUE == pstSwitch->bVenc)
    {
        SAMPLE_COMM_VENC_StopGetStream();
        SAMPLE_COMM_VENC_Stop(0);
    }
    if (SC_TRUE == pstSwitch->bVo)
    {
        SAMPLE_COMM_IVE_StopVo();
    }

    SAMPLE_COMM_VI_UnBind_VPSS(pstViConfig->astViInfo[0].stPipeInfo.aPipe[0],
        pstViConfig->astViInfo[0].stChnInfo.ViChn, 0);
    SAMPLE_COMM_IVE_StopVpss(VPSS_CHN_NUM);
    SAMPLE_COMM_VI_StopVi(pstViConfig);
    SAMPLE_COMM_SYS_Exit();

    memset(pstViConfig, 0, sizeof(*pstViConfig));
}

