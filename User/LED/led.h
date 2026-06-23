#ifndef __LED_H
#define __LED_H

#include "stm32h7xx.h"
#include "main.h"

// LED引脚定义
#define LED_GREEN_PIN    GPIO_PIN_4  // PE4 - 绿灯
#define LED_RED_PIN      GPIO_PIN_5  // PE5 - 红灯
#define LED_GPIO_PORT    GPIOE

// 函数声明
void LED_Init(void);                      // LED初始化
void LED_Green_On(void);                 // 绿灯亮
void LED_Green_Off(void);                // 绿灯灭
void LED_Red_On(void);                   // 红灯亮
void LED_Red_Off(void);                  // 红灯灭
void LED_Green_Toggle(void);             // 绿灯状态翻转
void LED_Red_Toggle(void);               // 红灯状态翻转
void LED_All_On(void);                   // 所有LED亮
void LED_All_Off(void);                  // 所有LED灭
void LED_All_Toggle(void);               // 所有LED状态翻转

#endif
