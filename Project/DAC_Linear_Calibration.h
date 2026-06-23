

#ifndef __DAC_LINEAR_CALIBRATION_H
#define __DAC_LINEAR_CALIBRATION_H

#include "stm32h7xx.h"
#include "main.h"
#include "dac8830.h"
#include "ad7606.h"
#include <stdint.h>
#include <math.h>

// 校准参数结构
typedef struct {
    float target_voltage;     // 目标电压
    float measured_voltage;   // 测量电压
    float dac_setting;        // DAC设置值
    float error;              // 误差
} DAC_CalibrationPoint_t;

// 线性校准参数
typedef struct {
    float slope;              // 斜率校准系数
    float intercept;          // 截距校准系数
    float r_squared;         // 拟合优度R2
    float max_error;         // 最大绝对误差
    float avg_error;         // 平均绝对误差
    uint8_t is_calibrated;  // 校准状态标志
    uint16_t num_points;     // 校准点数
    uint32_t calibration_time_ms; // 校准耗时
} DAC_LinearCalibration_t;

// 校准配置
typedef struct {
    uint16_t settle_time_ms;       // 稳定时间(ms)
    float tolerance;                // 收敛容差(V)
    uint8_t max_retries;           // 最大重试次数
    uint8_t samples_per_point;     // 每点采样次数
    uint16_t max_calibration_points; // 最大校准点数
} DAC_CalibrationConfig_t;

// 函数声明
void DAC_LinearCalibration_Init(void);
uint8_t DAC_Perform_Linear_Calibration(uint16_t num_points, float start_voltage, float end_voltage);
float DAC_LinearCalibrated_SetVoltage(float target_voltage);
void DAC_PrintCalibrationInfo(void);
uint8_t DAC_ValidateCalibration(float test_voltage, float max_allowed_error);
uint8_t DAC_GetCalibrationStatus(void);
float DAC_GetCalibrationSlope(void);
float DAC_GetCalibrationIntercept(void);
float DAC_GetCalibrationQuality(void);

// 高级功能
uint8_t DAC_AutoCalibration(float min_voltage, float max_voltage, uint16_t points);
void DAC_SaveCalibrationParams(void);
uint8_t DAC_LoadCalibrationParams(void);

#endif /* __DAC_LINEAR_CALIBRATION_H */

