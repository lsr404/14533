#include "yuanyi_light.h"

#include <stdio.h>
#include <unistd.h>

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "hi_adc.h"          // ADC标准接口头文件


float GetLightValue()
{
    float value = 0;
    unsigned short data = 0;
    hi_adc_read(LIGHT_SENSOR_CHAN_NAME, &data, 2, 0, 0);
    /**有光时，串口输出的ADC的值为120左右；无光时，串口输出的ADC的值为1800左右。*/
    if(data<1400){
        value=(float)data;
        value=0.2381*value+69.88; // 简单的线性模拟公式
    }
    return value;
}