#include "SG90.h"


void Servo_SetAngle(uint16_t angle)
{
    uint32_t cmpVal;
    if(angle > 180)angle = 180;
    cmpVal = 200 + (uint32_t)angle*800/180;

    DL_TimerA_setCaptureCompareValue(TIMA1,cmpVal,GPIO_PWM_SG_C0_IDX);

    delay_cycles(64000000);
}



