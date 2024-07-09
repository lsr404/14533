#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_gpio.h"
#include <cJSON.h>
#include "iot_main.h"
#include "iot_profile.h"
#include "yuanyi_servo.h"
#include "yuanyi_aht20.h"
#include "yuanyi_mq2.h"

/* attribute initiative to report */
#define TAKE_THE_INITIATIVE_TO_REPORT

#define CONFIG_WIFI_SSID "yuanyi" // 修改为自己的WiFi 热点账号

#define CONFIG_WIFI_PWD "12345678" // 修改为自己的WiFi 热点密码

#define CN_COMMAND_INDEX  "commands/request_id="

/**
上报属性数据
*/
typedef struct {
    int driveMotorSpeed; //驱动电机转速
    float temp; //温度
    float hum; // 湿度
    float gas; //燃气
} report_t;

typedef struct {
    int connected;
    int beep;
} app_cb_t;
static app_cb_t g_app_cb;

void deal_cmd_msg(char *payload)
{
    cJSON *obj_root;
    cJSON *obj_cmdname;
    obj_root = cJSON_Parse(payload);
    if (obj_root == NULL) {
        printf("jixie cuowu");
    }else{
        obj_cmdname = cJSON_GetObjectItem(obj_root, "command_name");
        if (obj_cmdname == NULL) {
            cJSON_Delete(obj_root);
        }
        if (strcmp(cJSON_GetStringValue(obj_cmdname), "SetLampStatus") == 0) {
            servo_deal_switch_cmd(obj_root);
            return;
        }
        if (strcmp(cJSON_GetStringValue(obj_cmdname), "SetServoStatus") == 0) {
            servo_deal_direction_cmd(obj_root);
            return;
        }
    }
    return;
}
void MsgRcvCallBack(int qos, const char *topic, char *payload)
{
    const char *requesID;
    char *tmp;
    IoTCmdResp resp;
    printf("RCVMSG:QOS:%d TOPIC:%s PAYLOAD:%s\r\n", qos, topic, payload);
    // /* app 下发的操作 */
    deal_cmd_msg(payload);
    tmp = strstr(topic, CN_COMMAND_INDEX);
    if (tmp != NULL) {
        // /< now you could deal your own works here --THE COMMAND FROM THE PLATFORM
        // /< now er roport the command execute result to the platform
        requesID = tmp + strlen(CN_COMMAND_INDEX);
        resp.requestID = requesID;
        resp.respName = NULL;
        resp.retCode = 0;   ////< which means 0 success and others failed
        resp.paras = NULL;
        (void)IoTProfileCmdResp(CONFIG_DEVICE_PWD, &resp);
    }
    return;
}
void reportDeviceInfo(float humValue,float tempValue,float lightValue){
    
    IoTProfileService service;

    IoTProfileKV temp;

    IoTProfileKV hum;

    IoTProfileKV light;

    memset_s(&light, sizeof(light), 0, sizeof(light));
    light.type = EN_IOT_DATATYPE_STRING;
    light.key = "gas";
    char lightStr[10];
    snprintf(lightStr, sizeof(lightStr), "%.2f", lightValue);
    printf("gas:%.2f\r\n",lightValue);
    light.value = lightStr;

    memset_s(&hum, sizeof(hum), 0, sizeof(hum));
    hum.type = EN_IOT_DATATYPE_STRING;
    hum.key = "hum";
    printf("hum:%.2f\r\n",humValue);
    char humStr[10];
    snprintf(humStr, sizeof(humStr), "%.2f", humValue);
    hum.value = humStr;
    hum.nxt=&light;

    memset_s(&temp, sizeof(temp), 0, sizeof(temp));
    temp.type = EN_IOT_DATATYPE_STRING;
    temp.key = "temp";
    printf("temp:%.2f\r\n",tempValue);
    char tempStr[10];
    snprintf(tempStr, sizeof(tempStr), "%.2f", tempValue);
    temp.value = tempStr;
    temp.nxt=&hum;

    memset_s(&service, sizeof(service), 0, sizeof(service));
    service.serviceID = "base_service";
    service.serviceProperty = &temp;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID, &service);
    return;
}

/*任务实现*/
void CarDmsDaoTask(void* parame) {
    (void)parame;
    // ServoInit();
    Aht20Init();//引脚初始化
    WifiConnect(CONFIG_WIFI_SSID, CONFIG_WIFI_PWD);//连接WiFi

    hi_udelay(500);//等一会
    IoTMain();//连接华为云MQTT
    IoTSetMsgCallback(MsgRcvCallBack);//设置回调函数
    printf("start success\r\n");
    /* 主动上报 */
#ifdef TAKE_THE_INITIATIVE_TO_REPORT
    while (1) {
        // /< here you could add your own works here--we report the data to the IoTplatform获取数据
        hi_sleep(1000);
        // 获取光照强度
        float light=Mq2GetData();
        printf("light Value:%.2f\r\n",light);
        // 获取温度湿度
        (void)GetAht20SensorData();
        hi_sleep(2000);
        float temp=GetTempValue();
        printf("temp:%.2f\r\n",temp);
        float hum=GetHumValue();
        printf("hum:%.2f\r\n",hum);
        reportDeviceInfo(hum,temp,light);//数据上报
    }
#endif
}

static void CarDmsDaoDemo(void)
{
    osThreadAttr_t attr;
    attr.name = "CarDmsDaoTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 10240;
    attr.priority = osPriorityNormal;

    if (osThreadNew(CarDmsDaoTask, NULL, &attr) == NULL) {
        printf("[CarDmsDaoDemo] Falied to create CarDmsDaoTask!\n");
    }
}
APP_FEATURE_INIT(CarDmsDaoDemo);  