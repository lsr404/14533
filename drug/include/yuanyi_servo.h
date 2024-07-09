#ifndef __YUANYI_SERVO_H__
#define __YUANYI_SERVO_H__


// 舵机分两种 SG90 和 MG90  目前适配 MG996R 型号 180度 和 360 度
// 一般线颜色  红色:正极电压 棕色：地线 橙色：信号线 

//舵机通过GPIO2与3861连接
//SG90舵机的控制需要MCU产生一个周期为20ms的脉冲信号，以0.5ms到2.5ms的高电平来控制舵机转动的角度

// 定义舵机的 针脚
#define YUANYI_SERVO_GPIO 10

// 初始化引脚
void ServoInit();

// 发送宽度为 pulsewidth 脉冲控制角度 
void set_angle(int pulsewidth);

/**
Turn 45 degrees to the left of the steering gear
1、依据角度与脉冲的关系，设置高电平时间为1000微秒
2、发送10次脉冲信号，控制舵机向左旋转45度
*/
void engine_turn_left_45(void);

/*Turn 90 degrees to the left of the steering gear
1、依据角度与脉冲的关系，设置高电平时间为500微秒
2、发送10次脉冲信号，控制舵机向左旋转90度
*/
void engine_turn_left_90(void);

/*Turn 45 degrees to the right of the steering gear
1、依据角度与脉冲的关系，设置高电平时间为2000微秒
2、发送10次脉冲信号，控制舵机向右旋转45度
*/
void engine_turn_right_45(void);

/*Turn 90 degrees to the right of the steering gear
1、依据角度与脉冲的关系，设置高电平时间为2500微秒
2、发送10次脉冲信号，控制舵机向右旋转90度
*/
void engine_turn_right_90(void);

/*The steering gear is centered
1、依据角度与脉冲的关系，设置高电平时间为1500微秒
2、发送10次脉冲信号，控制舵机居中
*/
void regress_middle(void);

/* 360度舵机 正向顺时针旋转
1、依据角度与脉冲的关系，设置高电平时间为500微秒
2、发送10次脉冲信号，控制舵机正向顺时针最大转速
*/
void engine_forward_rotation(int speedLevel);

/* 360度舵机 反向逆时针旋转
1、依据角度与脉冲的关系，设置高电平时间为2500微秒
2、发送10次脉冲信号，控制舵机反向逆时针最大转速
*/
void engine_reverse_rotation(int speedLevel);

/* 360度舵机 停止旋转
1、依据角度与脉冲的关系，设置高电平时间为1500微秒
2、发送10次脉冲信号，控制舵机停止旋转
*/
void engine_stop_rotation();

/*
响应舵机开关命令处理逻辑
*/
void servo_deal_switch_cmd(cJSON *obj_root);

/*
响应舵机设置角度处理逻辑
*/
void servo_deal_angle_cmd(cJSON *obj_root);

/*
响应360舵机顺时针旋转命令处理逻辑
*/
void servo_deal_direction_cmd(cJSON *obj_root);


#endif