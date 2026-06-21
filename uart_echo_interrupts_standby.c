/*
 * Copyright (c) 2020, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ti_msp_dl_config.h"
#include "sensors.h"
#include "command.h"
#include "LQ12864.h"
#include "SG90.h"
#include "buzzer.h"
#include "delay.h"
#include "ASR.h"
#include "K230.h"

#include <stdio.h>
#include <string.h>

char sendcmd1[] = "AT";

char nowcmd[200];       //字符串发送
char sensordata[100];   //传感器数据
char envReport[256];    //环境传感器(温度/气压)上报指令
char olcd_buf[16];      //存放OLED显示字符

void sendTo_8266(const char* datain);
void sendTo_PC(char* datain);
void sendString(char* sensordata);  //发送传感器数据

void activate_8266(void); //给8266发指令，激活8266，配置wifi模式
void data_cloud(void);

//单片机把字符串发送给8266
void sendTo_8266(const char* datain)
{
    int i=0;
    while (datain[i] != '\0') {
        DL_UART_Main_transmitDataBlocking(UART_2_INST,datain[i]);
        i++;
    }
    DL_UART_Main_transmitDataBlocking(UART_2_INST,'\r');
    DL_UART_Main_transmitDataBlocking(UART_2_INST,'\n');
    
}

//单片机把字符串发送给PC
void sendTo_PC(char* datain)
{
    int i=0;
    while (datain[i] != '\0') {
        DL_UART_Main_transmitDataBlocking(UART_0_INST,datain[i]);
        i++;
    }
    DL_UART_Main_transmitDataBlocking(UART_0_INST,'\r');
    DL_UART_Main_transmitDataBlocking(UART_0_INST,'\n');
    
}

volatile uint8_t gEchoData = 0;

/* ==================== 4 位共阴数码管动态扫描 + NTP 时间显示 ==================== */
/* 段码表（共阴，bit0..6 = A B C D E F G；1=点亮） */
static const uint8_t SEG_FONT[10] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

/* 4 位位选脚（均在 GPIOA），索引 0~3 = 左到右 */
static const uint32_t DIG_PIN[4] = {
    GPIO_GRP_0_PIN_1_PIN, GPIO_GRP_0_PIN_2_PIN,
    GPIO_GRP_0_PIN_3_PIN, GPIO_GRP_0_PIN_4_PIN
};
#define DIG_ALL_PIN   (GPIO_GRP_0_PIN_1_PIN | GPIO_GRP_0_PIN_2_PIN | \
                       GPIO_GRP_0_PIN_3_PIN | GPIO_GRP_0_PIN_4_PIN)
#define SEG_GPIOB_ALL (GPIO_GRP_0_PIN_A_PIN | GPIO_GRP_0_PIN_B_PIN | \
                       GPIO_GRP_0_PIN_C_PIN | GPIO_GRP_0_PIN_D_PIN | GPIO_GRP_0_PIN_E_PIN)
#define SEG_GPIOA_ALL (GPIO_GRP_0_PIN_F_PIN | GPIO_GRP_0_PIN_G_PIN | GPIO_GRP_0_PIN_DP_PIN)

/* 要显示的 4 位：0~9 显示数字，10=灭；disp_dp 的 bit n=1 表示第 n 位点亮小数点 */
volatile uint8_t disp_buf[4] = {10, 10, 10, 10};
volatile uint8_t disp_dp = 0;
static uint8_t scan_idx = 0;

/* UART_2 接收行缓冲（用于解析 8266 回复） */
static char u2_line[80];
static uint8_t u2_idx = 0;

/* 设置当前位的段（共阴：点亮=输出高） */
static void seg_set(uint8_t v, uint8_t dp)
{
    uint8_t f = (v < 10) ? SEG_FONT[v] : 0x00;
    uint32_t aON = 0, bON = 0;
    if (f & 0x01) bON |= GPIO_GRP_0_PIN_A_PIN;
    if (f & 0x02) bON |= GPIO_GRP_0_PIN_B_PIN;
    if (f & 0x04) bON |= GPIO_GRP_0_PIN_C_PIN;
    if (f & 0x08) bON |= GPIO_GRP_0_PIN_D_PIN;
    if (f & 0x10) bON |= GPIO_GRP_0_PIN_E_PIN;
    if (f & 0x20) aON |= GPIO_GRP_0_PIN_F_PIN;
    if (f & 0x40) aON |= GPIO_GRP_0_PIN_G_PIN;
    if (dp)       aON |= GPIO_GRP_0_PIN_DP_PIN;
    DL_GPIO_clearPins(GPIOB, SEG_GPIOB_ALL);
    DL_GPIO_setPins(GPIOB, bON);
    DL_GPIO_clearPins(GPIOA, SEG_GPIOA_ALL);
    DL_GPIO_setPins(GPIOA, aON);
}

/* SysTick：每 1ms 刷新一位，4 位轮流 → 约 250Hz 全屏刷新 */
void SysTick_Handler(void)
{
    DL_GPIO_setPins(GPIOA, DIG_ALL_PIN);             /* 共阴：位选拉高=关闭所有位，防拖影 */
    seg_set(disp_buf[scan_idx], (disp_dp >> scan_idx) & 0x01);
    DL_GPIO_clearPins(GPIOA, DIG_PIN[scan_idx]);     /* 选通当前位（拉低） */
    scan_idx = (scan_idx + 1) & 0x03;
}

/* 从 8266 的 +CIPSNTPTIME: 回复里解析出 时:分，更新数码管缓冲 */
static void parse_sntp_time(const char *s)
{
    const char *q = strstr(s, "+CIPSNTPTIME:");
    if (q == NULL) return;
    q += 13;                        /* 跳过 "+CIPSNTPTIME:" */
    const char *c = strchr(q, ':'); /* 时:分 之间的冒号 */
    if (c == NULL || c < q + 2) return;
    int h = (c[-2] - '0') * 10 + (c[-1] - '0');
    int m = (c[1]  - '0') * 10 + (c[2]  - '0');
    if (h < 0 || h > 23 || m < 0 || m > 59) return;
    /* 物理位号 4-3-2-1（左到右）：4位=时十, 3位=时个, 2位=分十, 1位=分个 */
    disp_buf[3] = h / 10;   /* 第4位(最左) */
    disp_buf[2] = h % 10;   /* 第3位 */
    disp_buf[1] = m / 10;   /* 第2位 */
    disp_buf[0] = m % 10;   /* 第1位(最右) */
    disp_dp = 0x04;         /* 第3位小数点常亮当冒号（时个位后那个点） */
}

int main(void)
{
    SYSCFG_DL_init();

    NVIC_ClearPendingIRQ(UART_0_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_0_INST_INT_IRQN);

    NVIC_ClearPendingIRQ(UART_2_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_2_INST_INT_IRQN);

    NVIC_ClearPendingIRQ(UART1_INT_IRQn);
    NVIC_EnableIRQ(UART1_INT_IRQn);

    DL_Timer_startCounter(TIMA1);

    LCD_Init();
    sensor_light_init();
    sensor_env_init();

    //sendTo_8266(sendcmd1);

    //delay_cycles(32000000);

    // sprintf(nowcmd, "AT+GMR");
    // sendTo_8266(nowcmd);

    /* 启动 SysTick：每 1ms 刷新数码管一位（动态扫描显示时间 HH.MM） */
    SysTick_Config(CPUCLK_FREQ / 1000);

    // ===== 数码管显示测试（备用）：固定显示 22.00 =====
    //disp_buf[3] = 2;   // 第4位(最左)
    //disp_buf[2] = 2;   // 第3位
    //disp_buf[1] = 0;   // 第2位
    //disp_buf[0] = 0;   // 第1位(最右)
    //disp_dp = 0x04;    // 第3位小数点当冒号
    //while (1) { }      // 死循环，仅测试显示
    

    // DL_GPIO_clearPins(GPIO_GRP_0_PIN_1_PORT,
    //     GPIO_GRP_0_PIN_1_PIN | GPIO_GRP_0_PIN_2_PIN |
    //     GPIO_GRP_0_PIN_3_PIN | GPIO_GRP_0_PIN_4_PIN);

    // /* 共阳：段脚输出低电平点亮该段；GPIOA 上的段 F、G、DP */
    // DL_GPIO_setPins(GPIO_GRP_0_PIN_F_PORT,
    //     GPIO_GRP_0_PIN_F_PIN | GPIO_GRP_0_PIN_G_PIN | GPIO_GRP_0_PIN_DP_PIN);

    // /* GPIOB 上的段 A、B、C、D、E */
    // DL_GPIO_setPins(GPIO_GRP_0_PIN_A_PORT,
    //     GPIO_GRP_0_PIN_A_PIN | GPIO_GRP_0_PIN_B_PIN | GPIO_GRP_0_PIN_C_PIN |
    //     GPIO_GRP_0_PIN_D_PIN | GPIO_GRP_0_PIN_E_PIN);


    // activate_8266();

    // data_cloud();

    // while (1) {
    //     sensor_light_read_lux();
    //     // sprintf(CMD_MQTTPUB_SENSOR_light,
    //     //     "AT+MQTTPUB=0,\"$oc/devices/YOUR_DEVICE_ID/sys/properties/report\",\"{\\\"services\\\":[{\\\"service_id\\\":\\\"Light\\\",\\\"properties\\\":{\\\"lox\\\":%.2f}}]}\",1,0",
    //     //     nowLux);
    //     // sendTo_8266(CMD_MQTTPUB_SENSOR_light);
    //     // delay_cycles(32000000);

    //     // 读取并上报温度/气压（service_id 与属性名为占位，待华为云产品模型确定后替换）
    //     sensor_env_read();

    //   LCD_P8x16Str(0,0,(unsigned char*)"light:");
    //   sprintf(olcd_buf,"%.2f",nowLux);
    //   LCD_P8x16Str(50,0,(unsigned char*)olcd_buf);
    //   LCD_P6x8Str(0,3,(unsigned char*)"tempture:");
    //   sprintf(olcd_buf,"%.2f",nowTemp);
    //   LCD_P6x8Str(55,3,(unsigned char*)olcd_buf);
    //   LCD_P6x8Str(0,6,(unsigned char*)"press:");
    //   sprintf(olcd_buf,"%.2f",nowPress);
    //   LCD_P6x8Str(40,6,(unsigned char*)olcd_buf);
      
      

    // sprintf(envReport,
    //     "AT+MQTTPUB=0,\"$oc/devices/YOUR_DEVICE_ID/sys/properties/report\",\"{\\\"services\\\":[{\\\"service_id\\\":\\\"Environment\\\",\\\"properties\\\":{\\\"Light\\\":%.2f,\\\"temperature\\\":%.2f,\\\"pressure\\\":%.2f}}]}\",1,0",
    //     nowLux, nowTemp, nowPress);
    // sendTo_8266(envReport);
    // delay_cycles(32000000);

    // // 查询 NTP 网络时间，回复由 UART_2 中断解析后显示在数码管
    // sendTo_8266(CMD_CIPSNTPTIME);
    // delay_cycles(32000000);
    // LCD_CLS();

    // }

    //语音模块测试
    //set_voice(init);

    //视觉模块测试
    if(K230_Data())
    {
        set_voice(init);
    }
    
    while (1) {
        //舵机测试
        // Servo_SetAngle(90);
        // Servo_SetAngle(180);

        //蜂鸣器测试
        // Buzzer_Beep(1);
        // delay_cycles(32000000);

        //语音模块测试
        // int i = read_camera_data();
        // if (i == 95) {
        //     Servo_SetAngle(180);
        //     Servo_SetAngle(0);
        // }


    }

}


//PC发，8266收（单片机过渡）
void UART_0_INST_IRQHandler(void)
{
    switch (DL_UART_Main_getPendingInterrupt(UART_0_INST)) {
        case DL_UART_MAIN_IIDX_RX:
            gEchoData = DL_UART_Main_receiveData(UART_0_INST);
            DL_UART_Main_transmitDataBlocking(UART_2_INST, gEchoData);
            break;
        default:
            break;
    }
}

//8266发，PC收（单片机过渡）
void UART_2_INST_IRQHandler(void)
{
        switch (DL_UART_Main_getPendingInterrupt(UART_2_INST)) {
        case DL_UART_MAIN_IIDX_RX:
            gEchoData = DL_UART_Main_receiveData(UART_2_INST);
            DL_UART_Main_transmitDataBlocking(UART_0_INST, gEchoData);  // 照旧转发给 PC
            // 同时缓存成行，解析 +CIPSNTPTIME 回复
            if (gEchoData == '\n' || gEchoData == '\r') {
                u2_line[u2_idx] = '\0';
                if (u2_idx > 0) parse_sntp_time(u2_line);
                u2_idx = 0;
            } else if (u2_idx < sizeof(u2_line) - 1) {
                u2_line[u2_idx++] = (char) gEchoData;
            }
            break;
        default:
            break;
    }
}

void sendString(char* sensordata)
{
    sprintf(nowcmd,"%s",sensordata);
    sendTo_8266(nowcmd);
}

void data_cloud(void)
{
//    int iTemp;

    /* Set LED to indicate start of transfer. */
//    DL_GPIO_setPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_L);
    //InitOpt();    //传感器iic通信

    // 开启两路串口中断：上位机串口、ESP8266串口


    // 1. 向上位机串口发送MQTT上报指令（测试用）
    // sendString(CMD_MQTTPUB);
    // delay_cycles(32000000);

    // 3. 配置MQTT用户名、密码（CLIENTID填NULL分离配置）
    sendTo_8266(CMD_MQTTUSERCFG1);
    DL_Common_delayCycles(32000000);

    // 4. 单独配置MQTT ClientID
    sendTo_8266(CMD_MQTTUSERCFG2);
    DL_Common_delayCycles(32000000);

    // 5. 连接华为云MQTT服务器
    sendTo_8266(CMD_MQTTCONN);
    DL_Common_delayCycles(32000000);

    // 6. 上报sensor传感器读取到的光亮到华为云
    //sendTo_8266(CMD_MQTTPUB);
    //DL_Common_delayCycles(32000000);

    // 备用上报指令，当前注释未启用
    // sendStringTo8266(CMD_MQTTPUB1);
    // DL_Common_delayCycles(32000000);
}

void activate_8266(void)
{
    // 1. 向上位机串口发送MQTT上报指令（测试用）
    // sendString(CMD_MQTTPUB);
    // delay_cycles(32000000);

    // 2. 发送ESP8266重启AT指令
    sendTo_8266(CMD_AT);
    DL_Common_delayCycles( 32000000);

    sendTo_8266(CMD_RST);
    DL_Common_delayCycles( 32000000);

    sendTo_8266(CMD_GMR);
    DL_Common_delayCycles( 32000000);

    sendTo_8266(CMD_CWMODE);
    DL_Common_delayCycles( 32000000);

    // 连接 WiFi
    sendTo_8266(CMD_CWJAP);
    DL_Common_delayCycles(160000000);   // 约 5s，等待联网

    // 配置 SNTP（启用、东八区、NTP 服务器），等待首次同步
    sendTo_8266(CMD_CIPSNTPCFG);
    DL_Common_delayCycles(96000000);    // 约 3s

}