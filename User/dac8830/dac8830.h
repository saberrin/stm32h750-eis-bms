#ifndef __DAC8830_H__
#define __DAC8830_H__

#include <stdint.h>

/* 基准电压：单位毫伏(mv) */
#define VREF 2500.0

/* 电压量程选择 跳帽接5V：0  跳帽接10V：1*/
#define VOLTAGE_RANGE 0

/* 电压输出模式 跳帽接0~+：0  跳帽接-~+：1*/
#define VOLTAGE_OUTPUT_MODE 1

/* 不同模式的电压范围 用户无需修改 */
#if VOLTAGE_RANGE
#if VOLTAGE_OUTPUT_MODE
/* 跳帽接10V 跳帽接-~+  ±10V输出 */
#define MAX_VOLTAGE 10.0
#define MIN_VOLTAGE -10.0
#else
/* 跳帽接10V 跳帽接0~+  0V-10V输出 */
#define MAX_VOLTAGE 10.0
#define MIN_VOLTAGE 0.0
#endif
#else
#if VOLTAGE_OUTPUT_MODE
/* 跳帽接5V 跳帽接-~+   ±5V输出 */
#define MAX_VOLTAGE 5.0
#define MIN_VOLTAGE -5.0
#else
/* 跳帽接5V 跳帽接0~+   0V-5V输出 */
#define MAX_VOLTAGE 5.0
#define MIN_VOLTAGE 0.0
#endif
#endif

// DAC8830引脚配置
#define DAC8830_CS1_Pin GPIO_PIN_4    //MISO作CS
#define DAC8830_CS1_GPIO_Port GPIOB

void DAC8830_GPIO_Init(void);
void DAC8830_Init(void);
void DAC8830_Set_Direct_Current(double voltage);
void DAC8830_Set_Wave(double *data, uint16_t data_size);
void DAC8830_set_Voltage(double V);
void CALIB_DAC_SetVoltage(double target_voltage);
#endif
