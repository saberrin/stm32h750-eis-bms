#ifndef __CURRENT_EXCITATION_CALIBRATOR_H
#define __CURRENT_EXCITATION_CALIBRATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "dac.h"
// 校准参数结构体
typedef struct {
    float gain_a;    // 斜率 a, 单位 A/V
    float offset_b;  // 截距 b, 单位 A
    uint8_t is_calibrated; // 校准标志位
} CurrentCalibration_t;

// 外部全局变量声明
extern CurrentCalibration_t CurrentCalib;
extern UART_HandleTypeDef * CalibratorUartHandle;

// 公共函数声明
void CurrentAcquisitionCalibrator_Init(UART_HandleTypeDef * huart);
void CurrentCalibrator_ProcessCommand(char * rx_command);

#ifdef __cplusplus
}
#endif

#endif /* __CURRENT_CALIBRATOR_H */
