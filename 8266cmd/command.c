#include "command.h"

//------固定指令------//
const char CMD_AT[] = "AT";
const char CMD_RST[] = "AT+RST";
const char CMD_GMR[] = "AT+GMR";
const char CMD_CWMODE[] = "AT+CWMODE=1";
const char CMD_CWJAP[] = "AT+CWJAP=\"YOUR_WIFI_SSID\",\"YOUR_WIFI_PASSWORD\"";
const char CMD_MQTTCONN[] = "AT+MQTTCONN=0,\"YOUR_MQTT_BROKER_HOST\",1883,1";
// SNTP：启用、东八区(+8)、NTP 服务器
const char CMD_CIPSNTPCFG[] = "AT+CIPSNTPCFG=1,8,\"ntp1.aliyun.com\",\"cn.pool.ntp.org\"";
// 查询当前网络时间
const char CMD_CIPSNTPTIME[] = "AT+CIPSNTPTIME?";
//MQTT 账号鉴权配置
const char CMD_MQTTUSERCFG[] = "AT+MQTTUSERCFG=0,1,\"YOUR_DEVICE_ID\",\"YOUR_MQTT_USERNAME\",\"YOUR_MQTT_PASSWORD\",0,0,\"\"";
//   ||
const char CMD_MQTTUSERCFG1[] = "AT+MQTTUSERCFG=0,1,\"NULL\",\"YOUR_MQTT_USERNAME\",\"YOUR_MQTT_PASSWORD\",0,0,\"\"";
const char CMD_MQTTUSERCFG2[] = "AT+MQTTCLIENTID=0,\"YOUR_MQTT_CLIENT_ID\"";
//   转义"
const char CMD_MQTTPUB[] = "AT+MQTTPUB=0,\"$oc/devices/YOUR_DEVICE_ID/sys/properties/report\",\"{\\\"services\\\":[{\\\"service_id\\\":\\\"Battery\\\",\\\"properties\\\":{\\\"batteryLevel\\\":30,\\\"batteryVoltage\\\":3}}]}\",1,0";


//------变动指令------//
char CMD_MQTTPUB_SENSOR_light[200] = "AT+MQTTPUB=0,\"$oc/devices/YOUR_DEVICE_ID/sys/properties/report\",\"{\\\"services\\\":[{\\\"service_id\\\":\\\"待定\\\",\\\"properties\\\":{\\\"待定\\\":%d}}]}\",1,0";

// 运行时上报缓冲区（由主程序用 sprintf 填充实时传感器数值）
//char CMD_MQTTPUB_SENSOR[MQTT_PUB_BUF_LEN];

//AT+MQTTUSERCFG=0,1,"YOUR_DEVICE_ID","YOUR_MQTT_USERNAME","YOUR_MQTT_PASSWORD",0,0,""