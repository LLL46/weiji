#include "command.h"

//------固定指令------//
const char CMD_AT[] = "AT\r\n";
const char CMD_RST[] = "AT+RST\r\n";
const char CMD_GMR[] = "AT+GMR\r\n";
const char CMD_CWMODE[] = "AT+CWMODE=1\r\n";
const char CMD_CWJAP[] = "AT+CWJAP=\"具体wife名称\",\"对应密码\"\r\n";
const char CMD_MQTTCONN[] = "AT+MQTTCONN=0,\"b28ff03eee.st1.iotda-device.cn-east-3.myhuaweicloud.com\",1883,1\r\n";
//MQTT 账号鉴权配置
const uint8_t CMD_MQTTUSERCFG[] = "AT+MQTTUSERCFG=0,1,\"6a09a50c18855b39c51a8b10_M001_0_0_2026051711\",\"6a09a50c18855b39c51a8b10_M001\",\"772c3c60ccb8bb1e228ad81864f114130fc382647e1b53cc098368989e499e5a\",0,0,\"\"\r\n";
//   ||
const uint8_t CMD_MQTTUSERCFG3[] = "AT+MQTTUSERCFG=0,1,\"NULL\",\"6a09a50c18855b39c51a8b10_M001\",\"772c3c60ccb8bb1e228ad81864f114130fc382647e1b53cc098368989e499e5a\",0,0,\"\"\r\n";
const uint8_t CMD_MQTTUSERCFG4[] = "AT+MQTTCLIENTID=0,\"6a09a50c18855b39c51a8b10_M001_0_0_2026051711\"\r\n";
//   转义"
const uint8_t CMD_MQTTPUB[] = "AT+MQTTPUB=0,\"$oc/devices/6a09a50c18855b39c51a8b10_M001/sys/properties/report\",\"{\\\"services\\\":[{\\\"service_id\\\":\\\"Battery\\\",\\\"properties\\\":{\\\"batteryLevel\\\":30,\\\"batteryVoltage\\\":3}}]}\",1,0\r\n";


//------变动指令------//
uint8_t CMD_MQTTPUB_SENSOR[200] = "AT+MQTTPUB=0,\"$oc/devices/6a09a50c18855b39c51a8b10_M001/sys/properties/report\",\"{\\\"services\\\":[{\\\"service_id\\\":\\\"待定\\\",\\\"properties\\\":{\\\"待定\\\":%d}}]}\",1,0\r\n";

