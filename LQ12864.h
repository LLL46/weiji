#ifndef _LQOLED_H
#define _LQOLED_H
//#include"stc.h"
//#include "intrins.h" 


#include "ti_msp_dl_config.h"

 	#define byte  unsigned char
	#define word  unsigned int
	#define dword unsigned long

#define LCD_DC_1 DL_GPIO_setPins(GPIO_OLED_PORT,GPIO_OLED_PIN_DC_PIN)
#define LCD_DC_0 DL_GPIO_clearPins(GPIO_OLED_PORT,GPIO_OLED_PIN_DC_PIN)

#define LCD_RST_1 DL_GPIO_setPins(GPIO_OLED_PORT,GPIO_OLED_PIN_RST_PIN)
#define LCD_RST_0 DL_GPIO_clearPins(GPIO_OLED_PORT,GPIO_OLED_PIN_RST_PIN)

#define LCD_SDA_1 DL_GPIO_setPins(GPIO_OLED_PORT,GPIO_OLED_PIN_SDA_PIN)
#define LCD_SDA_0 DL_GPIO_clearPins(GPIO_OLED_PORT,GPIO_OLED_PIN_SDA_PIN)

#define LCD_SCL_1 DL_GPIO_setPins(GPIO_OLED_PORT,GPIO_OLED_PIN_SCL_PIN)
#define LCD_SCL_0 DL_GPIO_clearPins(GPIO_OLED_PORT,GPIO_OLED_PIN_SCL_PIN)




 void LCD_Init(void);
 void LCD_CLS(void);
 void LCD_P6x8Str(byte x,byte y,byte ch[]);
 void LCD_P8x16Str(byte x,byte y,byte ch[]);
 void LCD_P14x16Ch(byte x,byte y,byte N);  	  
 void LCD_Fill(byte dat);
#endif

