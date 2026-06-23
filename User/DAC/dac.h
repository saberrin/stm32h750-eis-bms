//-----------------------------------------------------------------
// 程序描述:
// 		 DAC驱动程序头文件
// 作    者: 凌智电子
// 开始日期: 2020-11-11
// 完成日期: 2020-11-11
// 修改日期: 
// 当前版本: V1.0
// 历史版本:
//  - V1.0: (2018-08-04)DAC初始化，DAC输出电压设置
// 调试工具: 凌智STM32H750核心板、LZE_ST_LINK2
// 说    明: 
//    
//-----------------------------------------------------------------
#ifndef __DAC_H
#define __DAC_H
#include "system.h"
//-----------------------------------------------------------------
// 宏定义
//-----------------------------------------------------------------
#define DAC_Vref	2.048		// DAC基准电压（内部基准电压）

//-----------------------------------------------------------------
// ADC引脚定义
//-----------------------------------------------------------------
#define DAC_CHANNEL													 	 DAC_CHANNEL_1
#define DAC_PIN                                GPIO_PIN_4
#define DAC_GPIO_PORT                          GPIOA
#define DAC_GPIO_CLK_ENABLE()                  __HAL_RCC_GPIOA_CLK_ENABLE()  
#define DAC_GPIO_CLK_DISABLE()                 __HAL_RCC_GPIOA_CLK_DISABLE() 
#define DAC_CLK_ENABLE()                  		 __HAL_RCC_DAC12_CLK_ENABLE() 

//#define DAC2_PIN                                GPIO_PIN_5
//-----------------------------------------------------------------
// 声明
//-----------------------------------------------------------------
extern DAC_HandleTypeDef DAC_Handler;// DAC句柄
extern DMA_HandleTypeDef hdma_dac1;
//-----------------------------------------------------------------
// 函数声明
//-----------------------------------------------------------------
extern void MY_VREFBUF_Init(uint32_t VoltageScaling);
extern void DAC_Init(uint16_t *wave_data);
extern void DAC_Set_Vol(u16 vol);
extern void Restart_DAC_Wave(uint16_t *wave_data, uint16_t SIN_DATA_Need_count);
extern void DAC2_SetValue(uint16_t value);


extern void DAC1_Init_Constant(void);
extern void DAC1_Set_Voltage(float voltage);
extern void OutputConstantCurrent(double current);

#endif
//-----------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------
