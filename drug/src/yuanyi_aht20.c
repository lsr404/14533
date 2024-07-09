#include "yuanyi_aht20.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "cmsis_os2.h"
#include "iot_gpio.h"
#include "hi_io.h"
#include "iot_errno.h"
#include "wifi_error_code.h"
#include "hi_i2c.h"
#include "hi_time.h"
#include "hi_task.h"
#include "iot_i2c.h"

#ifndef TRUE
#define TRUE 1
#endif


#include "yuanyi_aht20.h"

#define  AHT_REG_ARRAY_LEN          (6)
#define  AHT_OC_ARRAY_LEN           (6)
#define  AHT_SNED_CMD_LEN           (3)
#define  AHT20_DEMO_TASK_STAK_SIZE  (1024*4)
#define  AHT20_DEMO_TASK_PRIORITY   (25)
#define  AHT_REG_ARRAY_INIT_LEN     (1)
#define  AHT_CALCULATION            (1048576)

#define  AHT_WRITE_COMMAND ((unsigned char)0x00)
#define  AHT_READ_COMMAND ((unsigned char)0x01)

#define IOT_BIT_1 (1)
#define IOT_BIT_4 (4)
#define IOT_BIT_8 (8)
#define IOT_BIT_12 (12)
#define IOT_BIT_16 (16)

#define BUFF_BIT_1 (1)
#define BUFF_BIT_2 (2)
#define BUFF_BIT_3 (3)
#define BUFF_BIT_4 (4)
#define BUFF_BIT_5 (5)

#define CONSTER_50 (50)
#define CONSTER_100 (100)
#define CONSTER_200 (200)

#define LOW_4_BIT ((unsigned char)0x0f)
#define HIGH_4_BIT ((unsigned char)0xf0)

AhtSersonValue sensorV = {0};

float GetAhtSensorValue(AhtSersonType type)
{
    float value = 0;

    switch (type) {
        case AHT_TEMPERATURE:
            value = sensorV.g_ahtTemper;
            break;
        case AHT_HUMIDITY:
            value = sensorV.g_ahtHumi;
            break;
        default:
            break;
    }
    return value;
}

float GetTempValue()
{
    float value = 0;
    value = sensorV.g_ahtTemper;
    return value;
}

float GetHumValue()
{
    float value = 0;
    value = sensorV.g_ahtHumi;
    return value;
}

/*
* Check whether the bit3 of the temperature and humidity sensor is initialized successfully,
* otherwise send the setting of 0xbe to set the sensor initialization
*/
static unsigned int Ath20CheckAndInit()
{
    unsigned int status = 0;
    unsigned char recvDataInit[AHT_REG_ARRAY_INIT_LEN] = { 0 };
    unsigned char initSendUserCmd[AHT_SNED_CMD_LEN] = { AHT_DEVICE_INIT_CMD, AHT_DEVICE_PARAM_INIT_HIGH, AHT_DEVICE_PARAM_LOW_BYTE};
    (void)memset_s(&recvDataInit, sizeof(recvDataInit), 0x0, sizeof(recvDataInit));
    
    status = IoTI2cRead(HI_I2C_IDX_0, (AHT_DEVICE_ADDR << IOT_BIT_1) | AHT_READ_COMMAND,
        recvDataInit, AHT_REG_ARRAY_INIT_LEN);
    printf("读取状态:%d",status);
    if (((recvDataInit[0] != AHT_DEVICE_CALIBRATION_ERR) && (recvDataInit[0] != AHT_DEVICE_CALIBRATION_ERR_R)) ||
        (recvDataInit[0] == AHT_DEVICE_CALIBRATION)) {
        status = IoTI2cWrite(HI_I2C_IDX_0, (AHT_DEVICE_ADDR << IOT_BIT_1) | AHT_WRITE_COMMAND,
            initSendUserCmd, AHT_SNED_CMD_LEN);
        hi_sleep(AHT_SLEEP_1S);
        return IOT_FAILURE;
    }
    return IOT_SUCCESS;
}

/* 发送触犯测量命令 */
unsigned int Aht20Write(unsigned char triggerCmd, unsigned char highByteCmd, unsigned char lowByteCmd)
{
    unsigned int status = 0;

    unsigned char sendUserCmd[AHT_SNED_CMD_LEN] = {triggerCmd, highByteCmd, lowByteCmd};

    status = IoTI2cWrite(HI_I2C_IDX_0, (AHT_DEVICE_ADDR << IOT_BIT_1) | AHT_WRITE_COMMAND,
        sendUserCmd, AHT_SNED_CMD_LEN);
    printf("读取状态:%d",status);
    return IOT_SUCCESS;
}

/* 读取 aht20 serson 数据 */
unsigned int Aht20Read(unsigned int recvLen, unsigned char type)
{
    unsigned int status = 0;

    unsigned char recvData[AHT_REG_ARRAY_LEN] = { 0 };
    float temper = 0;
    float temperT = 0;
    float humi = 0;
    float humiH = 0;
    /* Request memory space */
    (void)memset_s(&recvData, sizeof(recvData), 0x0, sizeof(recvData));
    
    status = IoTI2cRead(HI_I2C_IDX_0, (AHT_DEVICE_ADDR << IOT_BIT_1) | AHT_READ_COMMAND,
                        recvData, recvLen);
    printf("statu2:%d",status);
    if (type == AHT_TEMPERATURE) {
        temper = (float)(((recvData[BUFF_BIT_3] & LOW_4_BIT) << IOT_BIT_16) |
            (recvData[BUFF_BIT_4] << IOT_BIT_8) |
            recvData[BUFF_BIT_5]); // 温度拼接
        temperT = (temper / AHT_CALCULATION) * CONSTER_200 - CONSTER_50;  // T = (S_t/2^20)*200-50
        sensorV.g_ahtTemper = temperT;
        printf("g_ahtTemper:%.2f\r\n",temperT);
        printf("g_ahtTemper:%.2f\r\n",sensorV.g_ahtTemper);
        return IOT_SUCCESS;
    }
    if (type == AHT_HUMIDITY) {
        humi = (float)((recvData[BUFF_BIT_1] << IOT_BIT_12 | recvData[BUFF_BIT_2] << IOT_BIT_4) |
            ((recvData[BUFF_BIT_3] & HIGH_4_BIT) >> IOT_BIT_4)); // 湿度拼接
        humiH = humi / AHT_CALCULATION * CONSTER_100;
        sensorV.g_ahtHumi = humiH;
        printf("g_ahtHumi:%.2f\r\n",humiH);
        return IOT_SUCCESS;
    }
}
void Aht20Init(void){
    unsigned int status = 0;
    /* init oled i2c */
    IoTGpioInit(13); /* GPIO13 */
    hi_io_set_func(13, HI_I2C_SDA_SCL); /* GPIO13,  SDA */
    IoTGpioInit(14); /* GPIO 14 */
    hi_io_set_func(14, HI_I2C_SDA_SCL); /* GPIO14  SCL */

    IoTI2cInit(HI_I2C_IDX_0, BAUDRATE_INIT); /* baudrate: 400000 */
    /* 上电等待40ms */
    hi_udelay(AHT_DELAY_40MS); // 40ms
}

void *AppDemoAht20(char *param)
{
    unsigned int status = 0;
    unsigned int ret = 0;
    while (1) {
        if (ret == IOT_SUCCESS) {
        /* check whethe the sensor  calibration */
            while (IOT_SUCCESS != Ath20CheckAndInit()) {
                printf("aht20 sensor check init failed!\r\n");
                hi_sleep(AHT_SLEEP_50MS);
            }
            /* on hold master mode */
            status = Aht20Write(AHT_DEVICE_TEST_CMD, AHT_DEVICE_PARAM_HIGH_BYTE,
                                AHT_DEVICE_PARAM_LOW_BYTE); // tempwerature
            if (status != IOT_SUCCESS) {
                printf("get tempwerature data error!\r\n");
            }
            hi_udelay(AHT_DELAY_100MS); // 100ms等待测量完成
            status = Aht20Read(AHT_REG_ARRAY_LEN, AHT_TEMPERATURE);
            if (status != IOT_SUCCESS) {
                printf("get tempwerature data error!\r\n");
            }
            status = Aht20Read(AHT_REG_ARRAY_LEN, AHT_HUMIDITY);
            if (status != IOT_SUCCESS) {
                printf("get humidity data error!\r\n");
            }
        }
        hi_sleep(AHT_TASK_SLEEP_TIME); // 20ms
    }
}

/* get aht20 sensor data */
void GetAht20SensorData(void)
{
    unsigned int status = 0;

    /* on hold master mode */
    status = Aht20Write(AHT_DEVICE_TEST_CMD, AHT_DEVICE_PARAM_HIGH_BYTE,
                        AHT_DEVICE_PARAM_LOW_BYTE); // tempwerature
    printf("status1:%d \r\n",status);
    hi_udelay(AHT_DELAY_100MS); // 100ms等待测量完成
    status = Aht20Read(AHT_REG_ARRAY_LEN, AHT_TEMPERATURE);
    status = Aht20Read(AHT_REG_ARRAY_LEN, AHT_HUMIDITY);
}

// static void StartAht20Task(void)
// {
//     osThreadAttr_t attr = {0};

//     attr.stack_size = AHT20_DEMO_TASK_STAK_SIZE;
//     attr.priority = AHT20_DEMO_TASK_PRIORITY;
//     attr.name = (hi_char*)"app_demo_aht20_task";

//     if (osThreadNew((osThreadFunc_t)AppDemoAht20, NULL, &attr) == NULL) {
//         printf("[LedExample] Failed to create app_demo_aht20!\n");
//     }
// }

// SYS_RUN(StartAht20Task);