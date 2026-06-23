

#ifndef __ADS131A04_H
#define __ADS131A04_H

#include "stm32h7xx_hal.h"

// 控制引脚定义
#define CS_1()          HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET)
#define CS_0()          HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET)
#define RESET_1()       HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET)
#define RESET_0()       HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET)
#define READ_DRDY()     HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_5)

// 初始化
void ADS131A0X_Init(void);

// 数据读取
void ADS131A0X_ReadData(float chVoltage[4]);
void ADS131A0X_ReadData_Print(void);
void ADS131A0X_Init_SetSampleRate(uint32_t target_sps);
void ADS131A0X_ChangeSampleRate_NoReset(uint32_t target_sps);
// 调试函数
void ADS131A0X_TestAllRegisters(void);
void ADS131A0X_PrintDetailedInfo(void);
void ADS131A0X_Read_Ch1_Ch2(uint32_t *Current, uint32_t *Voltage);
uint8_t ADS131A0X_Read_Single_Channel(uint8_t channel, uint16_t *val16);


float ADS131A0X_Read_Channel(uint8_t channel);
#endif
