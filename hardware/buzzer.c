#include "buzzer.h"


void Buzzer_Beep(uint32_t sec)
{
    DL_GPIO_clearPins(GPIOB, GPIO_Buzzer_buzzer_PIN);
    DL_Common_delayCycles(32000000*sec);
    DL_GPIO_setPins(GPIOB, GPIO_Buzzer_buzzer_PIN);
}