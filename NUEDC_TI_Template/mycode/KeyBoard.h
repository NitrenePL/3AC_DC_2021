#ifndef BSP_KEYBOARD_H_
#define BSP_KEYBOARD_H_

#include "device.h"
#include "driverlib.h"
#include "global.h"
#include "OLED_Display.h"


// 函数声明
void Keyboard_Init(void);
void KEYBOARD_TIMER_Init(void);
uint16_t Keyboard_ReadData(void);
void KeyBoard_Scan(void);
void KeyAction(uint16_t key);

// Timer1 中断服务函数
__interrupt void cpuTimer1ISR(void);

#endif
