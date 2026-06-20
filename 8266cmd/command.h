#ifndef __COMMAND_H
#define __COMMAND_H

#include <stdint.h>

//==================== 固定AT指令字符串 ====================
extern const char CMD_AT[];
extern const char CMD_RST[];
extern const char CMD_GMR[];
extern const char CMD_CWMODE[];
extern const char CMD_CWJAP[];
extern const char CMD_MQTTCONN[];

// SNTP 网络时间指令
extern const char CMD_CIPSNTPCFG[];
extern const char CMD_CIPSNTPTIME[];

// MQTT鉴权配置指令
extern const char CMD_MQTTUSERCFG[];
extern const char CMD_MQTTUSERCFG1[];
extern const char CMD_MQTTUSERCFG2[];

// MQTT固定上报指令（固定30%电量、3V电压）
extern const char CMD_MQTTPUB[];

//==================== 可变上报缓冲区（用于填充实时传感器数值） ====================
#define MQTT_PUB_BUF_LEN 200
extern char CMD_MQTTPUB_SENSOR_light[MQTT_PUB_BUF_LEN];

#endif