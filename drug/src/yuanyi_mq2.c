#include "yuanyi_mq2.h"

#include <stdio.h>
#include <hi_stdlib.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "hi_types_base.h"
#include "hi_io.h"
#include <math.h>
#include "iot_gpio.h"
#include "iot_pwm.h"
#include "hi_adc.h"

#define MQ2_DEMO_TASK_STAK_SIZE    (1024 * 8)
#define MQ2_DEMO_TASK_PRIORITY    (25)
#define ADC_TEST_LENGTH    (20)
#define VLT_MIN    (100)
#define CAL_PPM    (25) // 校准环境中PPM值
#define RL    (1) // RL阻值
#define MQ2_RATIO    (1111)

#define X_CONSTANT  (613.9f)
#define Y_CONSTANT  (-2.074f)
#define X_CONSTANT_2 (11.5428 * 22)
#define Y_CONSTANT_2 (0.6549)
#define VOILTAGE_5_V (5)

#define PPM_THRESHOLD_300 (200) // 随便你改
#define PPM_THRESHOLD_3000 (3000)
#define SAMPLING_TIME (0xff)

#define ADC_RANGE_MAX ((float)4096.0)
#define ADC_VOLTAGE_1_8_V  ((float)1.8)
#define ADC_VOLTAGE_4_TIMES (4)

/* pwm duty */
#define PWM_LOW_DUTY                1
#define PWM_SLOW_DUTY               1000
#define PWM_SMALL_DUTY              4000
#define PWM_LITTLE_DUTY             10000
#define PWM_DUTY                    50
#define PWM_MIDDLE_DUTY             40000
#define PWM_FULL_DUTY               65530

Mq2SensorDef combGas = {0};
float g_r0 = 22; /* R0 c初始值 */

void Mq2Init(void)
{
    IoTPwmInit(0);
    hi_io_set_func(HI_IO_NAME_GPIO_9, HI_IO_FUNC_GPIO_9_PWM0_OUT); // gpio9 pwm
    IoTGpioSetDir(HI_IO_NAME_GPIO_9, IOT_GPIO_DIR_OUT);
}

void SetCombuSensorValue(void)
{
    combGas.g_combustibleGasValue = 0.0;
}

float GetCombuSensorValue(void)
{
    return combGas.g_combustibleGasValue;
}

/*
 *  ppm：为可燃气体的浓度
 *  VRL：电压输出值
 *  Rs：器件在不同气体，不同浓度下的电阻值
 *  R0：器件在洁净空气中的电阻值
 *  RL：负载电阻阻值
 */
void Mq2PpmCalibration(float rS)
{
    g_r0 = rS / pow(CAL_PPM / X_CONSTANT, 1 / Y_CONSTANT);
    printf("R0:%f\r\n", g_r0);
}

/* MQ2传感器数据处理 */
float Mq2GetPpm(float voltage)
{
    float vol = voltage;
    double ppm = 0;

    float VolDif = (VOILTAGE_5_V - vol);
    float SeekModule = VolDif / vol;
    float rS = SeekModule * RL; /* 计算 RS值 */
    printf("rS:%f\r\n", rS);
    (void)memset_s(&ppm, sizeof(ppm), 0x0, sizeof(ppm));
    ppm = pow(X_CONSTANT_2 * vol / (VOILTAGE_5_V - vol), 1.0 / Y_CONSTANT_2); /* 计算ppm */
    if (ppm < PPM_THRESHOLD_300) { /* 排除空气中其他气体的干扰 */
        ppm = 0;
    }
    return ppm;
}

/* mq2 sesor get data from adc change */
float Mq2GetData(void)
{
    unsigned short data = 0; /* 0 */
    float voltage;
    // ADC_Channal_2(gpio5)  自动识别模式  CNcomment:4次平均算法模式 CNend
    unsigned int ret = hi_adc_read(HI_ADC_CHANNEL_5, &data,
                               HI_ADC_EQU_MODEL_4, HI_ADC_CUR_BAIS_DEFAULT, SAMPLING_TIME);
    if (ret != HI_ERR_SUCCESS) {
        printf("ADC Read Fail\n");
        return 0;
    }
    voltage = (float)(data * ADC_VOLTAGE_1_8_V *ADC_VOLTAGE_4_TIMES / ADC_RANGE_MAX); /* vlt * 1.8* 4 / 4096.0为将码字转换为电压 */
    float g_combustibleGasValue = Mq2GetPpm(voltage);
    printf("g_combustibleGasValue is %lf\r\n", g_combustibleGasValue);
    return g_combustibleGasValue;
}
