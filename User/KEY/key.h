#ifndef __KEY_H
#define __KEY_H

#include "stm32h7xx.h"
#include "main.h"

// 按键定义
typedef enum
{
    BUTTON_KEY1 = 0,  // PE3
    BUTTON_KEY2 = 1,  // PC3
    BUTTONn           // 按键数量
} Button_TypeDef;

// 按键引脚定义
#define BUTTON_KEY1_PIN                GPIO_PIN_3
#define BUTTON_KEY1_GPIO_PORT          GPIOE

#define BUTTON_KEY2_PIN                GPIO_PIN_3
#define BUTTON_KEY2_GPIO_PORT          GPIOC

// 按键读取宏
#define KEY1    HAL_GPIO_ReadPin(BUTTON_KEY1_GPIO_PORT, BUTTON_KEY1_PIN)
#define KEY2    HAL_GPIO_ReadPin(BUTTON_KEY2_GPIO_PORT, BUTTON_KEY2_PIN)

// 按键返回值
#define KEY1_PRES      1   // KEY1按下
#define KEY2_PRES      2   // KEY2按下
#define KEY_UNPRESS     0  // 无按键按下

// 函数声明
void KEY_Init(void);
void BSP_KEY_Init(Button_TypeDef button);
uint8_t KEY_Scan(uint8_t mode);

#endif
