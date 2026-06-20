#include "sensors.h"
#include <stdbool.h>

#define sensor_environment_address 0x77
#define sensor_temperature_address 0x40 //获取温度，湿度和大气压强
#define sensor_light 0x47
#define sensor_inertial 0x69
#define sensor_magnetometer 0x13

#ifndef SENSOR_LIGHT_I2C_INST
#define SENSOR_LIGHT_I2C_INST I2C_1_INST
#endif

#define OPT_I2C_TX_INIT_PACKET_SIZE (3)
#define OPT_I2C_READSEND_PACKET_SIZE (1)
#define OPT_I2C_READRECEIVE_PACKET_SIZE (2)

static const uint8_t opt_i2c_tx_init_packet[OPT_I2C_TX_INIT_PACKET_SIZE] = {0x01, 0xCE, 0x10};
static const uint8_t opt_i2c_readsend_packet[OPT_I2C_READSEND_PACKET_SIZE] = {0x00};
static uint8_t opt_i2c_readreceive_packet[OPT_I2C_READRECEIVE_PACKET_SIZE] = {0x00, 0x00};
static uint16_t rawData;

volatile float nowLux;

static void sensor_light_wait_idle(void)
{
	while (!(DL_I2C_getControllerStatus(SENSOR_LIGHT_I2C_INST) & DL_I2C_CONTROLLER_STATUS_IDLE)) {
	}
}

static void sensor_light_wait_bus_free(void)
{
	while (DL_I2C_getControllerStatus(SENSOR_LIGHT_I2C_INST) & DL_I2C_CONTROLLER_STATUS_BUSY_BUS) {
	}
}

static float sensor_light_convert_raw_to_lux(uint16_t rawValue)
{
	uint16_t exponent = (rawValue & 0xF000U) >> 12;
	uint16_t mantissa = rawValue & 0x0FFFU;
	float lux = 0.01f * (float) mantissa;

	while (exponent > 0U) {
		lux *= 2.0f;
		exponent--;
	}

	return lux;
}

void sensor_light_init(void)
{
	DL_I2C_fillControllerTXFIFO(SENSOR_LIGHT_I2C_INST, &opt_i2c_tx_init_packet[0], OPT_I2C_TX_INIT_PACKET_SIZE);

	sensor_light_wait_idle();

	DL_I2C_startControllerTransfer(SENSOR_LIGHT_I2C_INST,
		sensor_light,
		DL_I2C_CONTROLLER_DIRECTION_TX,
		OPT_I2C_TX_INIT_PACKET_SIZE);

	sensor_light_wait_bus_free();

	if (DL_I2C_getControllerStatus(SENSOR_LIGHT_I2C_INST) & DL_I2C_CONTROLLER_STATUS_ERROR) {
		__BKPT(0);
	}

	sensor_light_wait_idle();
}

void sensor_light_read_lux(void) //光照计算
{
	sensor_light_wait_idle();

	DL_I2C_fillControllerTXFIFO(SENSOR_LIGHT_I2C_INST, &opt_i2c_readsend_packet[0], OPT_I2C_READSEND_PACKET_SIZE);

	DL_I2C_startControllerTransfer(SENSOR_LIGHT_I2C_INST,
		sensor_light,
		DL_I2C_CONTROLLER_DIRECTION_TX,
		OPT_I2C_READSEND_PACKET_SIZE);

	sensor_light_wait_bus_free();

	if (DL_I2C_getControllerStatus(SENSOR_LIGHT_I2C_INST) & DL_I2C_CONTROLLER_STATUS_ERROR) {
		__BKPT(0);
	}

	sensor_light_wait_idle();

	DL_I2C_startControllerTransfer(SENSOR_LIGHT_I2C_INST,
		sensor_light,
		DL_I2C_CONTROLLER_DIRECTION_RX,
		OPT_I2C_READRECEIVE_PACKET_SIZE);

	for (uint8_t index = 0; index < OPT_I2C_READRECEIVE_PACKET_SIZE; index++) {
		while (DL_I2C_isControllerRXFIFOEmpty(SENSOR_LIGHT_I2C_INST)) {
		}
		opt_i2c_readreceive_packet[index] = DL_I2C_receiveControllerData(SENSOR_LIGHT_I2C_INST);
	}

	sensor_light_wait_bus_free();

	if (DL_I2C_getControllerStatus(SENSOR_LIGHT_I2C_INST) & DL_I2C_CONTROLLER_STATUS_ERROR) {
		__BKPT(0);
	}

	rawData = ((uint16_t) opt_i2c_readreceive_packet[0] << 8) + opt_i2c_readreceive_packet[1];
	nowLux = sensor_light_convert_raw_to_lux(rawData);
}

uint16_t sensor_light_get_raw_data(void)
{
	return rawData;
}

/* ==================== 环境传感器 BME280/BMP280（温度 + 气压） ==================== */
#ifndef SENSOR_I2C_INST
#define SENSOR_I2C_INST I2C_1_INST   /* 与光照传感器同一条 I2C 总线 */
#endif

/* BME280/BMP280 寄存器地址 */
#define BME280_REG_CTRL_MEAS 0xF4
#define BME280_REG_CONFIG    0xF5
#define BME280_REG_CALIB     0x88   /* 0x88~0x9F 共 24 字节校准系数 */
#define BME280_REG_DATA      0xF7   /* 气压(3) + 温度(3) 共 6 字节 */

/* 校准系数 */
static uint16_t dig_T1;
static int16_t  dig_T2, dig_T3;
static uint16_t dig_P1;
static int16_t  dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
static int32_t  t_fine;

volatile float nowTemp;   /* ℃  */
volatile float nowPress;  /* hPa */

static void i2c_wait_idle(void)
{
	while (!(DL_I2C_getControllerStatus(SENSOR_I2C_INST) & DL_I2C_CONTROLLER_STATUS_IDLE)) {
	}
}

static void i2c_wait_bus_free(void)
{
	while (DL_I2C_getControllerStatus(SENSOR_I2C_INST) & DL_I2C_CONTROLLER_STATUS_BUSY_BUS) {
	}
}

/* 向指定从机地址写 len 字节，返回 true 表示无总线错误 */
static bool i2c_write(uint8_t addr, const uint8_t *buf, uint8_t len)
{
	i2c_wait_idle();
	DL_I2C_fillControllerTXFIFO(SENSOR_I2C_INST, buf, len);
	DL_I2C_startControllerTransfer(SENSOR_I2C_INST, addr, DL_I2C_CONTROLLER_DIRECTION_TX, len);
	i2c_wait_bus_free();
	if (DL_I2C_getControllerStatus(SENSOR_I2C_INST) & DL_I2C_CONTROLLER_STATUS_ERROR) {
		return false;
	}
	i2c_wait_idle();
	return true;
}

/* 从指定从机地址读 len 字节，返回 true 表示无总线错误 */
static bool i2c_read(uint8_t addr, uint8_t *buf, uint8_t len)
{
	i2c_wait_idle();
	DL_I2C_startControllerTransfer(SENSOR_I2C_INST, addr, DL_I2C_CONTROLLER_DIRECTION_RX, len);
	for (uint8_t i = 0; i < len; i++) {
		while (DL_I2C_isControllerRXFIFOEmpty(SENSOR_I2C_INST)) {
		}
		buf[i] = DL_I2C_receiveControllerData(SENSOR_I2C_INST);
	}
	i2c_wait_bus_free();
	if (DL_I2C_getControllerStatus(SENSOR_I2C_INST) & DL_I2C_CONTROLLER_STATUS_ERROR) {
		return false;
	}
	return true;
}

/* 先写寄存器地址，再连续读 len 字节（BME280 地址自动递增） */
static bool bme280_read_regs(uint8_t reg, uint8_t *buf, uint8_t len)
{
	if (!i2c_write(sensor_environment_address, &reg, 1)) {
		return false;
	}
	return i2c_read(sensor_environment_address, buf, len);
}

static bool bme280_write_reg(uint8_t reg, uint8_t value)
{
	uint8_t pkt[2] = { reg, value };
	return i2c_write(sensor_environment_address, pkt, 2);
}

void sensor_env_init(void)
{
	uint8_t calib[24];

	/* 读取校准系数 0x88~0x9F */
	if (!bme280_read_regs(BME280_REG_CALIB, calib, sizeof(calib))) {
		__BKPT(0);
		return;
	}

	dig_T1 = (uint16_t)((calib[1] << 8) | calib[0]);
	dig_T2 = (int16_t) ((calib[3] << 8) | calib[2]);
	dig_T3 = (int16_t) ((calib[5] << 8) | calib[4]);
	dig_P1 = (uint16_t)((calib[7] << 8) | calib[6]);
	dig_P2 = (int16_t) ((calib[9] << 8) | calib[8]);
	dig_P3 = (int16_t) ((calib[11] << 8) | calib[10]);
	dig_P4 = (int16_t) ((calib[13] << 8) | calib[12]);
	dig_P5 = (int16_t) ((calib[15] << 8) | calib[14]);
	dig_P6 = (int16_t) ((calib[17] << 8) | calib[16]);
	dig_P7 = (int16_t) ((calib[19] << 8) | calib[18]);
	dig_P8 = (int16_t) ((calib[21] << 8) | calib[20]);
	dig_P9 = (int16_t) ((calib[23] << 8) | calib[22]);

	/* 配置：滤波关闭、温度 x1、气压 x1、正常模式 */
	bme280_write_reg(BME280_REG_CONFIG, 0x00);
	bme280_write_reg(BME280_REG_CTRL_MEAS, 0x27);
}

static float bme280_compensate_T(int32_t adc_T)
{
	float v1 = (((float) adc_T) / 16384.0f - ((float) dig_T1) / 1024.0f) * (float) dig_T2;
	float v2 = ((((float) adc_T) / 131072.0f - ((float) dig_T1) / 8192.0f) *
	            (((float) adc_T) / 131072.0f - ((float) dig_T1) / 8192.0f)) * (float) dig_T3;
	t_fine = (int32_t)(v1 + v2);
	return (v1 + v2) / 5120.0f;
}

static float bme280_compensate_P(int32_t adc_P)
{
	float v1 = ((float) t_fine / 2.0f) - 64000.0f;
	float v2 = v1 * v1 * ((float) dig_P6) / 32768.0f;
	v2 = v2 + v1 * ((float) dig_P5) * 2.0f;
	v2 = (v2 / 4.0f) + (((float) dig_P4) * 65536.0f);
	v1 = (((float) dig_P3) * v1 * v1 / 524288.0f + ((float) dig_P2) * v1) / 524288.0f;
	v1 = (1.0f + v1 / 32768.0f) * ((float) dig_P1);
	if (v1 == 0.0f) {
		return 0.0f;
	}
	float p = 1048576.0f - (float) adc_P;
	p = (p - (v2 / 4096.0f)) * 6250.0f / v1;
	v1 = ((float) dig_P9) * p * p / 2147483648.0f;
	v2 = p * ((float) dig_P8) / 32768.0f;
	p = p + (v1 + v2 + ((float) dig_P7)) / 16.0f;
	return p;  /* 单位 Pa */
}

void sensor_env_read(void)
{
	uint8_t data[6];
	int32_t adc_P, adc_T;

	if (!bme280_read_regs(BME280_REG_DATA, data, sizeof(data))) {
		__BKPT(0);
		return;
	}

	adc_P = ((int32_t) data[0] << 12) | ((int32_t) data[1] << 4) | (data[2] >> 4);
	adc_T = ((int32_t) data[3] << 12) | ((int32_t) data[4] << 4) | (data[5] >> 4);

	nowTemp  = bme280_compensate_T(adc_T);          /* ℃  */
	nowPress = bme280_compensate_P(adc_P) / 100.0f; /* hPa */
}

