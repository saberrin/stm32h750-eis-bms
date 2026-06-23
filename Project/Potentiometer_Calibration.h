



//#ifndef __POTENTIOMETER_CALIBRATION_H
//#define __POTENTIOMETER_CALIBRATION_H

//#include "stm32h7xx.h"
//#include "main.h"
//#include "ad7606.h"
//#include "dac8830.h"
//#include "AD620_GainCtrl.h"
//#include <stdint.h>

//// 校准模式枚举
//typedef enum {
//    CALIB_MODE_SINGLE_POINT = 0,     // 单点测试
//    CALIB_MODE_VOLTAGE_SWEEP,        // 电压扫描
//    CALIB_MODE_GAIN_SWEEP,           // 增益扫描（遍历所有8个增益）
//    CALIB_MODE_RANGE_TEST,           // 量程测试
//    CALIB_MODE_BIAS_ADJUST           // 偏置调整
//} CalibrationMode_t;

//// 函数声明
//void PotentiometerCalibration_Init(void);
//void PotentiometerCalibration_RunTest(CalibrationMode_t mode);
//void PotentiometerCalibration_PrintStatus(void);
//uint8_t PotentiometerCalibration_ValidateCalibration(void);

//// 专用验证函数声明
//void Validate_AllChannels_Reading(void);
//void Validate_CH2_AD620_Gain(void);  // 关键修改：使用智能防饱和算法，无需参数
//void Validate_CH3_DAC_Output(float start_v, float end_v, float step_v);
//void Validate_CH5_AD7606_Range(void);

//#endif /* __POTENTIOMETER_CALIBRATION_H */

#ifndef __POTENTIOMETER_CALIBRATION_H
#define __POTENTIOMETER_CALIBRATION_H

#include "stm32h7xx.h"
#include "main.h"
#include "ad7606.h"
#include "dac8830.h"
#include "AD620_GainCtrl.h"
#include <stdint.h>

// 校准模式枚举
typedef enum {
    CALIB_MODE_SINGLE_POINT = 0,     // 单点测试
    CALIB_MODE_VOLTAGE_SWEEP,        // 电压扫描
    CALIB_MODE_GAIN_SWEEP,           // 增益扫描（遍历所有8个增益）
    CALIB_MODE_RANGE_TEST,           // 量程测试
    CALIB_MODE_DAC_CALIBRATION      // DAC输出校准
} CalibrationMode_t;

// DAC校准结构
typedef struct {
    float expected_voltage;          // 期望输出电压
    float measured_voltage;          // 实际测量电压
    float calibration_offset;        // 校准偏移量
    uint8_t is_calibrated;           // 校准状态标志
} DAC_Calibration_t;

// 函数声明
void PotentiometerCalibration_Init(void);
void PotentiometerCalibration_RunTest(CalibrationMode_t mode);
void PotentiometerCalibration_PrintStatus(void);
uint8_t PotentiometerCalibration_ValidateCalibration(void);
void Run_Complete_Calibration_Test(void);
// 专用验证函数声明
void Validate_AllChannels_Reading(void);
void Validate_CH2_AD620_Gain(void);
void Validate_CH3_DAC_Output(float start_v, float end_v, float step_v);
//void Validate_CH5_AD7606_Range(void);

// DAC校准函数声明
float DAC_Calibrated_SetVoltage(float target_voltage);
void DAC_Auto_Calibration(void);
float Get_DAC_Calibration_Offset(void);

#endif /* __POTENTIOMETER_CALIBRATION_H */

