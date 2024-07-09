/*
 * Copyright (c) 2022 HiSilicon (Shanghai) Technologies CO., LIMITED.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * 该文件提供了基于yolov2的手部检测以及基于resnet18的手势识别，属于两个wk串行推理。
 * 该文件提供了手部检测和手势识别的模型加载、模型卸载、模型推理以及AI flag业务处理的API接口。
 * 若一帧图像中出现多个手，我们通过算法将最大手作为目标手送分类网进行推理，
 * 并将目标手标记为绿色，其他手标记为红色。
 *
 * This file provides hand detection based on yolov2 and gesture recognition based on resnet18,
 * which belongs to two wk serial inferences. This file provides API interfaces for model loading,
 * model unloading, model reasoning, and AI flag business processing for hand detection
 * and gesture recognition. If there are multiple hands in one frame of image,
 * we use the algorithm to use the largest hand as the target hand for inference,
 * and mark the target hand as green and the other hands as red.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "sample_comm_nnie.h"
#include "sample_media_ai.h"
#include "ai_infer_process.h"
#include "yolov2_hand_detect.h"
#include "vgs_img.h"
#include "ive_img.h"
#include "misc_util.h"
#include "hisignalling.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define HAND_FRM_WIDTH     640
#define HAND_FRM_HEIGHT    384
#define DETECT_OBJ_MAX     32
#define RET_NUM_MAX        4
#define DRAW_RETC_THICK    2    // Draw the width of the line
#define WIDTH_LIMIT        32
#define HEIGHT_LIMIT       32
#define IMAGE_WIDTH        224  // The resolution of the model IMAGE sent to the classification is 416*416
#define IMAGE_HEIGHT       224
#define MODEL_FILE_GESTURE    "/userdata/models/hand_classify/hand_gesture.wk" // darknet framework wk model

static int biggestBoxIndex;
static IVE_IMAGE_S img;
static DetectObjInfo objs[DETECT_OBJ_MAX] = {0};
static RectBox boxs[DETECT_OBJ_MAX] = {0};
static RectBox objBoxs[DETECT_OBJ_MAX] = {0};
static RectBox remainingBoxs[DETECT_OBJ_MAX] = {0};
static RectBox cnnBoxs[DETECT_OBJ_MAX] = {0}; // Store the results of the classification network
static RecogNumInfo numInfo[RET_NUM_MAX] = {0};
static IVE_IMAGE_S imgIn;
static IVE_IMAGE_S imgDst;
static VIDEO_FRAME_INFO_S frmIn;
static VIDEO_FRAME_INFO_S frmDst;
int uartFd = 0;


/*
 * 加载脸部检测模型
 * Load hand detect and classify model
 */
HI_S32 Yolo2HandDetectResnetClassifyLoad(uintptr_t* model)
{
    SAMPLE_SVP_NNIE_CFG_S *self = NULL; // 定义指向SAMPLE_SVP_NNIE_CFG_S结构体的指针变量self
    HI_S32 ret; // 定义HI_S32类型的变量ret，用于存储函数返回值

    // 调用CnnCreate函数创建模型，传入MODEL_FILE_GESTURE文件，并将返回值赋给ret
    ret = CnnCreate(&self, MODEL_FILE_GESTURE);
    // 如果ret小于0，表示模型创建失败，将model指向0；否则，将model指向self
    *model = ret < 0 ? 0 : (uintptr_t)self;
    // 初始化手部检测模型
    HandDetectInit(); 
    // 打印模型加载成功的提示信息
    SAMPLE_PRT("Load hand detect classify model success\n");
    
    /*
     * Uart串口初始化
     * Uart open init
     */
    // uartFd = UartOpenInit(); // 打开并初始化Uart串口，将返回的文件描述符赋值给uartFd
    // if (uartFd < 0) { // 判断uartFd是否小于0，即Uart打开是否失败
    //     printf("uart1 open failed\r\n"); // 打印Uart打开失败的提示信息
    // } else {
    //     printf("uart1 open successed\r\n"); // 打印Uart打开成功的提示信息
    // }
    return ret; // 返回ret，即模型创建的结果
}


/*
 * 卸载手部检测和手势分类模型
 * Unload hand detect and classify model
 */
/*
 * 卸载手部检测和手势分类模型
 * Unload hand detect and classify model
 */
HI_S32 Yolo2HandDetectResnetClassifyUnload(uintptr_t model)
{
    // 销毁手势分类模型，释放分配的资源
    CnnDestroy((SAMPLE_SVP_NNIE_CFG_S*)model);
    // 反初始化手部检测模型
    HandDetectExit(); 
    // 关闭Uart串口文件描述符
    // close(uartFd);
    // 打印模型卸载成功的提示信息
    SAMPLE_PRT("Unload hand detect classify model success\n");
    // 返回0表示成功卸载模型
    return 0;
}


/*
 * 获得最大的手
 * Get the maximum hand
 */
static HI_S32 GetBiggestHandIndex(RectBox boxs[], int detectNum)
{
    HI_S32 handIndex = 0; // 初始化handIndex为0，用于遍历检测到的手的矩形框
    HI_S32 biggestBoxIndex = handIndex;// 初始化biggestBoxIndex为0，用于存储最大矩形框的索引
    // 计算第一个手的矩形框的宽度
    HI_S32 biggestBoxWidth = boxs[handIndex].xmax - boxs[handIndex].xmin + 1;
    // 计算第一个手的矩形框的高度
    HI_S32 biggestBoxHeight = boxs[handIndex].ymax - boxs[handIndex].ymin + 1;
   // 计算第一个手的矩形框的面积
    HI_S32 biggestBoxArea = biggestBoxWidth * biggestBoxHeight;

    // 遍历所有检测到的手的矩形框，从第二个开始
    for (handIndex = 1; handIndex < detectNum; handIndex++) {
        HI_S32 boxWidth = boxs[handIndex].xmax - boxs[handIndex].xmin + 1;
        HI_S32 boxHeight = boxs[handIndex].ymax - boxs[handIndex].ymin + 1;
       // 计算当前手的矩形框的面积
        HI_S32 boxArea = boxWidth * boxHeight;
        if (biggestBoxArea < boxArea) {
            biggestBoxArea = boxArea;
            biggestBoxIndex = handIndex;
        }
        biggestBoxWidth = boxs[biggestBoxIndex].xmax - boxs[biggestBoxIndex].xmin + 1;
        biggestBoxHeight = boxs[biggestBoxIndex].ymax - boxs[biggestBoxIndex].ymin + 1;
    }
    // 如果检测到的手的矩形框的宽度或高度小于1，或检测到的手的个数为0，则返回-1
    if ((biggestBoxWidth == 1) || (biggestBoxHeight == 1) || (detectNum == 0)) {
        biggestBoxIndex = -1;
    }
    // 返回最大的矩形框的索引
    return biggestBoxIndex;
}

/*
 * 手势识别信息
 * Hand gesture recognition info
 */
static void HandDetectFlag(const RecogNumInfo resBuf)
{
    HI_CHAR *gestureName = NULL;
    switch (resBuf.num) {
        // case 0u:
        //     gestureName = "gesture fist";
        //     UartSendRead(uartFd, FistGesture); // 拳头手势
        //     SAMPLE_PRT("----gesture name----:%s\n", gestureName);
        //     break;
        // case 1u:
        //     gestureName = "gesture indexUp";
        //     UartSendRead(uartFd, ForefingerGesture); // 食指手势
        //     SAMPLE_PRT("----gesture name----:%s\n", gestureName);
        //     break;
        // case 2u:
        //     gestureName = "gesture OK";
        //     UartSendRead(uartFd, OkGesture); // OK手势
        //     SAMPLE_PRT("----gesture name----:%s\n", gestureName);
        //     break;
        // case 3u:
        //     gestureName = "gesture palm";
        //     UartSendRead(uartFd, PalmGesture); // 手掌手势
        //     SAMPLE_PRT("----gesture name----:%s\n", gestureName);
        //     break;
        // case 4u:
        //     gestureName = "gesture yes";
        //     UartSendRead(uartFd, YesGesture); // yes手势
        //     SAMPLE_PRT("----gesture name----:%s\n", gestureName);
        //     break;
        // case 5u:
        //     gestureName = "gesture pinchOpen";
        //     UartSendRead(uartFd, ForefingerAndThumbGesture); // 食指 + 大拇指
        //     SAMPLE_PRT("----gesture name----:%s\n", gestureName);
        //     break;
        // case 6u:
        //     gestureName = "gesture phoneCall";
        //     UartSendRead(uartFd, LittleFingerAndThumbGesture); // 大拇指 + 小拇指
        //     SAMPLE_PRT("----gesture name----:%s\n", gestureName);
        //     break;
        default:
            gestureName = "gesture others";
            UartSendRead(uartFd, InvalidGesture); // 无效值
            SAMPLE_PRT("----gesture name----:%s\n", gestureName);
            break;
    }
    SAMPLE_PRT("hand gesture success\n");
}

/*
 * 手部检测和手势分类推理
 * Hand detect and classify calculation
 */
HI_S32 Yolo2HandDetectResnetClassifyCal(uintptr_t model, VIDEO_FRAME_INFO_S *srcFrm, VIDEO_FRAME_INFO_S *dstFrm)
{
    // 将模型指针转换为SAMPLE_SVP_NNIE_CFG_S类型
    SAMPLE_SVP_NNIE_CFG_S *self = (SAMPLE_SVP_NNIE_CFG_S*)model;
    HI_S32 resLen = 0; // 初始化推理结果长度
    int objNum; // 检测到的手的数量
    int ret; // 函数返回值
    int num = 0; // 计数变量，用于记录剩余手的数量

    // 将视频帧转换为原始图像
    ret = FrmToOrigImg((VIDEO_FRAME_INFO_S*)srcFrm, &img);
    // 检查转换是否成功
    SAMPLE_CHECK_EXPR_RET(ret != HI_SUCCESS, ret, "hand detect for YUV Frm to Img FAIL, ret=%#x\n", ret);

    // 将图像发送到检测网络进行推理，返回检测到的手的数量
    objNum = HandDetectCal(&img, objs); 
    // 遍历检测到的手
    for (int i = 0; i < objNum; i++) {
        // 存储检测到的手的矩形框
        cnnBoxs[i] = objs[i].box;
        RectBox *box = &objs[i].box;
        // 转换矩形框的坐标
        RectBoxTran(box, HAND_FRM_WIDTH, HAND_FRM_HEIGHT,
            dstFrm->stVFrame.u32Width, dstFrm->stVFrame.u32Height);
        // 打印检测到的矩形框坐标
        SAMPLE_PRT("yolo2_out: {%d, %d, %d, %d}\n", box->xmin, box->ymin, box->xmax, box->ymax);
        // 存储转换后的矩形框
        boxs[i] = *box;
    }
    // 获取最大的手的索引
    biggestBoxIndex = GetBiggestHandIndex(boxs, objNum);
    // 打印最大的手的索引和检测到的手的数量
    SAMPLE_PRT("biggestBoxIndex:%d, objNum:%d\n", biggestBoxIndex, objNum);

    /*
     * 当检测到对象时，在DSTFRM中绘制一个矩形
     * When an object is detected, a rectangle is drawn in the DSTFRM
     */
    if (biggestBoxIndex >= 0) {
        // 将最大的手的矩形框存储到objBoxs
        objBoxs[0] = boxs[biggestBoxIndex];
        // 在目标帧dstFrm上绘制最大的手的矩形框，颜色为绿色
        MppFrmDrawRects(dstFrm, objBoxs, 1, RGB888_GREEN, DRAW_RETC_THICK); 

        // 遍历所有检测到的手
        for (int j = 0; (j < objNum) && (objNum > 1); j++) {
            if (j != biggestBoxIndex) {
                // 将剩余的手的矩形框存储到remainingBoxs
                remainingBoxs[num++] = boxs[j];
                /*
                 * 其他手objnum等于objnum -1
                 * Others hand objnum is equal to objnum -1
                 */
                // 在目标帧dstFrm上绘制其他手的矩形框，颜色为红色
                MppFrmDrawRects(dstFrm, remainingBoxs, objNum - 1, RGB888_RED, DRAW_RETC_THICK);
            }
        }

        /*
         * 裁剪出来的图像通过预处理送分类网进行推理
         * The cropped image is preprocessed and sent to the classification network for inference
         */
        // // 裁剪最大的手的图像
        // ret = ImgYuvCrop(&img, &imgIn, &cnnBoxs[biggestBoxIndex]);
        // // 检查裁剪是否成功
        // SAMPLE_CHECK_EXPR_RET(ret < 0, ret, "ImgYuvCrop FAIL, ret=%#x\n", ret);

        // // 如果裁剪后的图像宽度和高度大于等于限制
        // if ((imgIn.u32Width >= WIDTH_LIMIT) && (imgIn.u32Height >= HEIGHT_LIMIT)) {
        //     // 获取压缩模式
        //     COMPRESS_MODE_E enCompressMode = srcFrm->stVFrame.enCompressMode;
        //     // 将裁剪的图像转换为帧
        //     ret = OrigImgToFrm(&imgIn, &frmIn);
        //     frmIn.stVFrame.enCompressMode = enCompressMode;
        //     // 打印裁剪后的图像宽度和高度
        //     SAMPLE_PRT("crop u32Width = %d, img.u32Height = %d\n", imgIn.u32Width, imgIn.u32Height);
        //     // 调整帧的大小
        //     ret = MppFrmResize(&frmIn, &frmDst, IMAGE_WIDTH, IMAGE_HEIGHT);
        //     // 将调整后的帧转换为原始图像
        //     ret = FrmToOrigImg(&frmDst, &imgDst);
        //     // 使用分类网络对图像进行推理
        //     ret = CnnCalImg(self, &imgDst, numInfo, sizeof(numInfo) / sizeof((numInfo)[0]), &resLen);
        //     // 检查推理是否成功
        //     SAMPLE_CHECK_EXPR_RET(ret < 0, ret, "CnnCalImg FAIL, ret=%#x\n", ret);
        //     HI_ASSERT(resLen <= sizeof(numInfo) / sizeof(numInfo[0]));
        //     // 根据推理结果设置检测标志
        //     HandDetectFlag(numInfo[0]);
        //     // 销毁调整后的帧
        //     MppFrmDestroy(&frmDst);
        // }
        // // 销毁裁剪的图像
        // IveImgDestroy(&imgIn);
    }

    return ret; // 返回推理结果
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
