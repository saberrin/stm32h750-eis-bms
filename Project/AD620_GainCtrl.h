#ifndef __AD620_GAINCTRL_H
#define __AD620_GAINCTRL_H

#include "stm32h7xx.h"
#include <stdint.h>

// ADG708控制引脚定义 - 根据您的硬件连接修改
#define ADG708_A0_PIN          GPIO_PIN_10
#define ADG708_A0_PORT         GPIOD
#define ADG708_A1_PIN          GPIO_PIN_9
#define ADG708_A1_PORT         GPIOD
#define ADG708_A2_PIN          GPIO_PIN_8
#define ADG708_A2_PORT         GPIOD

// 增益通道定义 (A2, A1, A0)
typedef enum {
    AD620_GAIN_2 = 0,    // 000 - 增益 2.002
    AD620_GAIN_5 = 1,    // 001 - 增益 4.984
    AD620_GAIN_10 = 2,   // 010 - 增益 9.998
    AD620_GAIN_50 = 3,   // 011 - 增益 49.91
    AD620_GAIN_100 = 4,  // 100 - 增益 100
    AD620_GAIN_200 = 5,  // 101 - 增益 199.4
    AD620_GAIN_500 = 6,  // 110 - 增益 501
    AD620_GAIN_1000 = 7, // 111 - 增益 1003
    AD620_GAIN_MAX
} AD620_GainChannel_t;

// 增益配置结构体
typedef struct {
    AD620_GainChannel_t channel;
    float nominal_gain;      // 标称增益值
    float actual_gain;       // 实际校准后的增益值
    uint8_t code;           // 二进制控制码 (A2,A1,A0)
} AD620_GainConfig_t;

// 函数声明
void AD620_GainCtrl_Init(void);
void AD620_SetGain(AD620_GainChannel_t gain_channel);
void AD620_SetGainByValue(float desired_gain);
AD620_GainChannel_t AD620_GetCurrentGainChannel(void);
float AD620_GetCurrentGain(void);
const AD620_GainConfig_t* AD620_GetGainConfig(AD620_GainChannel_t channel);
void AD620_GainCtrl_Deinit(void);

#endif /* __AD620_GAINCTRL_H */
