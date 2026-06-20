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

#include <stdio.h>

char sendcmd1[] = "AT";

char nowcmd[200];       //字符串发送
char sensordata[100];   //传感器数据

void sendTo_8266(const char* datain);
void sendTo_PC(char* datain);
void sendString(char* sensordata);  //发送传感器数据

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
int main(void)
{
    SYSCFG_DL_init();

    NVIC_ClearPendingIRQ(UART_0_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_0_INST_INT_IRQN);

    NVIC_ClearPendingIRQ(UART_2_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_2_INST_INT_IRQN);



    sendTo_8266(sendcmd1);

    delay_cycles(32000000);

    // sprintf(nowcmd, "AT+GMR");
    // sendTo_8266(nowcmd);

    while (1) {

    }
}


//PC发，8266收（单片机过渡）
void UART_0_INST_IRQHandler(void)
{
    switch (DL_UART_Main_getPendingInterrupt(UART_0_INST)) {
        case DL_UART_MAIN_IIDX_RX:
            gEchoData = DL_UART_Main_receiveData(UART_0_INST);
            DL_UART_Main_transmitData(UART_2_INST, gEchoData);
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
            DL_UART_Main_transmitData(UART_0_INST, gEchoData);
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
    int iTemp;

    /* Set LED to indicate start of transfer. */
//    DL_GPIO_setPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_L);
    //InitOpt();    //传感器iic通信

    // 开启两路串口中断：上位机串口、ESP8266串口


    // 1. 向上位机串口发送MQTT上报指令（测试用）
    // sendString(CMD_MQTTPUB);
    // delay_cycles(32000000);

    // 2. 发送ESP8266重启AT指令
    sendTo_8266(CMD_RST);
    DL_Common_delayCycles( 32000000);

    //有记忆功能，可选
    sendTo_8266(CMD_CWJAP);
    DL_Common_delayCycles( 32000000);

    // 3. 配置MQTT用户名、密码（CLIENTID填NULL分离配置）
    sendTo_8266(CMD_MQTTUSERCFG3);
    DL_Common_delayCycles(32000000);

    // 4. 单独配置MQTT ClientID
    sendTo_8266(CMD_MQTTUSERCFG4);
    DL_Common_delayCycles(32000000);

    // 5. 连接华为云MQTT服务器
    sendTo_8266(CMD_MQTTCONN);
    DL_Common_delayCycles(32000000);

    // 6. 上报电池电压、电量数据到华为云
    sendTo_8266(CMD_MQTTPUB);
    DL_Common_delayCycles(32000000);

    // 备用上报指令，当前注释未启用
    // sendStringTo8266(CMD_MQTTPUB1);
    // DL_Common_delayCycles(32000000);
}