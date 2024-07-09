#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_gpio.h"
#include <cJSON.h>
#include "iot_main.h"
#include "iot_profile.h"
#include "yuanyi_servo.h"

// 初始化舵机引脚

void ServoInit(){
    IoTGpioSetDir(YUANYI_SERVO_GPIO, IOT_GPIO_DIR_OUT);//设置GPIO2为输出模式
}


//输出20000微秒的脉冲信号(x微秒高电平,20000-x微秒低电平)
void set_angle(int pulsewidth) {

    // int pulsewidth = (angle * 11) + 500; 
    printf("pulsewidth: %d",pulsewidth);

    IoTGpioSetDir(YUANYI_SERVO_GPIO, IOT_GPIO_DIR_OUT);//设置GPIO2为输出模式

    //GPIO2输出x微秒高电平
    IoTGpioSetOutputVal(YUANYI_SERVO_GPIO, IOT_GPIO_VALUE1);
    hi_udelay(pulsewidth);

    //GPIO2输出20000-x微秒低电平
    IoTGpioSetOutputVal(YUANYI_SERVO_GPIO, IOT_GPIO_VALUE0);
    hi_udelay(20000 - pulsewidth);
    return;
}

/*
Turn 45 degrees to the left of the steering gear
1、依据角度与脉冲的关系，设置高电平时间为1000微秒
2、发送10次脉冲信号，控制舵机向左旋转45度
*/
void engine_turn_left_45(void)
{
    printf("trun left 45 ");
    for (int i = 0; i <20; i++) 
    {
        set_angle(1000);
    }
    return;
}

/*Turn 90 degrees to the left of the steering gear
1、依据角度与脉冲的关系，设置高电平时间为500微秒
2、发送10次脉冲信号，控制舵机向左旋转90度
*/
void engine_turn_left_90(void)
{
    printf("trun left 90 ");
    for (int i = 0; i <20; i++) 
    {
        set_angle(500);
    }
    return;
}

/*Turn 45 degrees to the right of the steering gear
1、依据角度与脉冲的关系，设置高电平时间为2000微秒
2、发送10次脉冲信号，控制舵机向右旋转45度
*/
void engine_turn_right_45(void)
{
    printf("trun right 45 ");
    for (int i = 0; i <20; i++) 
    {
        set_angle(2000);
    }
    return;
}

/*Turn 90 degrees to the right of the steering gear
1、依据角度与脉冲的关系，设置高电平时间为2500微秒
2、发送10次脉冲信号，控制舵机向右旋转90度
*/
void engine_turn_right_90(void)
{
    printf("trun right 90 ");
    for (int i = 0; i <20; i++) 
    {
        set_angle(2500);
    }
    return;
}

/*The steering gear is centered
1、依据角度与脉冲的关系，设置高电平时间为1500微秒
2、发送10次脉冲信号，控制舵机居中
*/
void regress_middle(void)
{
    printf("regress_middle ");
    for (int i = 0; i <20; i++) 
    {
        set_angle(1500);
    }
    return;
}

/* 360度舵机 正向顺时针旋转
1、依据角度与脉冲的关系，设置高电平时间为500微秒
2、发送10次脉冲信号，控制舵机正向顺时针最大转速
*/
void engine_forward_rotation(int speedLevel){
    printf("engine_forward_rotation");
    for (int i = 0; i <100; i++) 
    {
        if(speedLevel<0||speedLevel>10){
            speedLevel=1;
        }
        set_angle(1500-100*speedLevel);
    }
    return;
}

/* 360度舵机 反向逆时针旋转
1、依据角度与脉冲的关系，设置高电平时间为2500微秒
2、发送10次脉冲信号，控制舵机反向逆时针最大转速
*/
void engine_reverse_rotation(int speedLevel){
    printf("engine_reverse_rotation");
    for (int i = 0; i <100; i++) 
    {
        if(speedLevel<0||speedLevel>10){
            speedLevel=1;
        }
        set_angle(2500-100*speedLevel);
    }
    return;
}

/* 360度舵机 停止旋转
1、依据角度与脉冲的关系，设置高电平时间为1500微秒
2、发送10次脉冲信号，控制舵机停止旋转
*/
void engine_stop_rotation(){
    printf("engine_stop_rotation");
    for (int i = 0; i <20; i++) 
    {
        set_angle(2500);
    }
    return;
}

void servo_deal_switch_cmd(cJSON *obj_root)
{
    cJSON *obj_paras;
    cJSON *obj_para;

    obj_paras = cJSON_GetObjectItem(obj_root, "paras");
    if (obj_paras == NULL) {
        cJSON_Delete(obj_root);
    }
    obj_para = cJSON_GetObjectItem(obj_paras, "ServoStatus");
    if (obj_paras == NULL) {
        cJSON_Delete(obj_root);
    }
    if (strcmp(cJSON_GetStringValue(obj_para), "ON") == 0) {
        engine_turn_left_90();
        printf("Servo On!\r\n");
    } else {
        engine_turn_right_90();
        printf("Servo Off!\r\n");
    }
    cJSON_Delete(obj_root);
    return;
}
void servo_deal_angle_cmd(cJSON *obj_root)
{
    cJSON *obj_paras;
    cJSON *obj_para;
    obj_paras = cJSON_GetObjectItem(obj_root, "Paras");
    if (obj_paras == NULL) {
        cJSON_Delete(obj_root);
    }
    obj_para = cJSON_GetObjectItem(obj_paras, "servo");
    if (obj_para == NULL) {
        cJSON_Delete(obj_root);
    }
    int number = cJSON_GetNumberValue(obj_para);
    printf("Servo Value： %d", number);
    if(number==45){
        engine_turn_left_45();
    }else if(number==90){
         engine_turn_left_90();
    }else if(number==135){
         engine_turn_right_45();
    }else if(number==180){
         engine_turn_right_90();
    }else if(number==0){
         regress_middle();
    }
    else{
        printf("wuxiao");
    }
    printf("servo success!\r\n");
    cJSON_Delete(obj_root);
    return;
}
void servo_deal_direction_cmd(cJSON *obj_root)
{
    cJSON *obj_paras;
    cJSON *obj_para;
    cJSON *obj_speed;

    obj_paras = cJSON_GetObjectItem(obj_root, "paras");
    if (obj_paras == NULL) {
        cJSON_Delete(obj_root);
    }
    obj_para = cJSON_GetObjectItem(obj_paras, "ServoDirection");
    //obj_speed = cJSON_GetObjectItem(obj_paras, "ServoSpeed");
    if (obj_paras == NULL) {
        cJSON_Delete(obj_root);
    }
    if (strcmp(cJSON_GetStringValue(obj_para), "forward") == 0) {
        //int speed=cJSON_GetIntValue(obj_speed);
        engine_forward_rotation(1);
        printf("Servo forward speed!\r\n");
    }else if(strcmp(cJSON_GetStringValue(obj_para), "reverse") == 0){
        //int speed=cJSON_GetIntValue(obj_speed);
        engine_reverse_rotation(1);
        printf("Servo reverse speed!\r\n");
    }else if(strcmp(cJSON_GetStringValue(obj_para), "stop") == 0){
        engine_stop_rotation();
        printf("Servo stop!\r\n");
    }
    cJSON_Delete(obj_root);
    return;
}