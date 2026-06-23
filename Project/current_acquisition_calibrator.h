/**
 * @file current_acquisition_calibrator.h
 * @brief 电流采样自动校准模块头文件
 * @details 通过线性回归算法计算电流采样系统的校准参数 I = a * V + b
 *          注意：电流单位统一为安培(A)，电压单位为伏特(V)
 */

#ifndef CURRENT_ACQUISITION_CALIBRATOR_H
#define CURRENT_ACQUISITION_CALIBRATOR_H

#include <stdint.h>

/**
 * @brief 校准配置结构体
 */
typedef struct {
    uint16_t num_points;       ///< 校准数据点数量
    float min_current_A;        ///< 最小校准电流 (A)
    float max_current_A;        ///< 最大校准电流 (A)
    uint32_t settle_delay_ms;  ///< 电流稳定延迟时间 (ms)
} Calibration_Config;

/**
 * @brief 校准结果结构体
 */
typedef struct {
    float slope_a;           ///< 斜率 a (A/V)
    float intercept_b;       ///< 截距 b (A)
    float correlation_coeff; ///< 相关系数 R
    float std_error;         ///< 标准误差 (A)
    uint8_t is_valid;        ///< 校准结果是否有效 (1:有效, 0:无效)
} Calibration_Result;

/**
 * @brief 初始化默认校准配置
 * @param config 校准配置结构体指针
 */
void CALIB_InitConfig(Calibration_Config* config);

/**
 * @brief 执行自动校准流程
 * @param config 校准配置
 * @param result 校准结果存储指针
 * @return uint8_t 执行结果 (1:成功, 0:失败)
 */
uint8_t CALIB_PerformAutoCalibration(const Calibration_Config* config, 
                                   Calibration_Result* result);

/**
 * @brief 使用校准参数计算电流值
 * @param voltage 电压值 (V)
 * @param result 校准结果
 * @return float 计算得到的电流值 (A)
 */
float CALIB_CalculateCurrent(float voltage, const Calibration_Result* result);

/**
 * @brief 通过串口输出校准结果
 * @param result 校准结果
 */
void CALIB_PrintResults(const Calibration_Result* result);

/**
 * @brief 读取当前电压值
 * @return float 读取的电压值 (V)
 */
float CALIB_ReadVoltage(void);

/**
 * @brief 执行完整的电流采样校准流程
 */
void CALIB_RunFullCalibration(void);

#endif /* CURRENT_ACQUISITION_CALIBRATOR_H */
