/*
 * 可燃气体
 */

#ifndef YUANYI_MQ2_H
#define YUANYI_MQ2_H

typedef struct {
    double g_combustibleGasValue;
    unsigned char g_ahu20GasBuff[6];
}Mq2SensorDef;

void Mq2Init(void);
void SetCombuSensorValue(void);
float GetCombuSensorValue(void);
void Mq2PpmCalibration(float rS);
float Mq2GetPpm(float voltage);
float Mq2GetData(void);

#endif