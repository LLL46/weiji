#ifndef __SENSORS_H
#define __SENSORS_H

#include <stdint.h>
#include "ti_msp_dl_config.h"

extern volatile float nowLux;
extern volatile float nowTemp;   // 温度，单位 ℃
extern volatile float nowPress;  // 气压，单位 hPa

void sensor_light_init(void);
void sensor_light_read_lux(void);
uint16_t sensor_light_get_raw_data(void);

// 环境传感器 BME280/BMP280（温度 + 气压）
void sensor_env_init(void);
void sensor_env_read(void);

#endif
