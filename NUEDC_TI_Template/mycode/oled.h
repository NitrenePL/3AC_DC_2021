#ifndef OLED_H_
#define OLED_H_

#include "device.h"
#include "driverlib.h"


#define OLED_CMD  0
#define OLED_DATA 1

// 函数声明
void OLED_Init(void);
void OLED_Clear(void);
void OLED_Display_On(void);
void OLED_Display_Off(void);
void OLED_Set_Pos(unsigned char x, unsigned char y);
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t Char_Size);
void OLED_ShowString(uint8_t x, uint8_t y, uint8_t *p, uint8_t Char_Size);
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size2);
void OLED_ShowCHinese(uint8_t x, uint8_t y, uint8_t no);
void OLED_DrawBMP(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1, unsigned char BMP[]);

// 底层通信
void Write_IIC_Command(unsigned char i2c_cmd);
void Write_IIC_Data(unsigned char i2c_data);
void OLED_WR_Byte(unsigned char dat, unsigned char cmd);

#endif
