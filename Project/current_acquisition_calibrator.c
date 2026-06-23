/**
 * @file current_acquisition_calibrator.c
 * @brief 电流采样自动校准模块实现文件
 * @details 通过线性回归算法计算电流采样系统的校准参数 I = a * V + b
 *          注意：电流单位统一为安培(A)，电压单位为伏特(V)
 */

#include "current_acquisition_calibrator.h"
#include "dac.h"
#include <stdio.h>
#include <math.h>
#include "delay.h"
// 外部函数声明
extern void OutputConstantCurrent(double current); // 注意：此函数单位是A
extern void AD7606_StartConvst(void);
extern void AD7606_ReadData(uint16_t* data);
extern double AD7606B_Digital2Voltage(uint16_t digital_value);

// 静态函数声明
static void CALIB_LinearRegression(const float* voltages, const float* currents_A, 
                                 uint16_t n, float* slope, float* intercept, 
                                 float* correlation, float* std_error);
static void CALIB_DelayMs(uint32_t ms);

/**
 * @brief 初始化默认校准配置 (单位: A)
 */
void CALIB_InitConfig(Calibration_Config* config) {
    if (config == NULL) return;

    config->num_points = 10;
    config->min_current_A = 0.1f;      // 0.1 A = 100 mA
    config->max_current_A = 1.8f;       // 1.8 A = 1800 mA
    config->settle_delay_ms = 500;
}

/**
 * @brief 读取当前电压值
 */
float CALIB_ReadVoltage(void) {
    uint16_t DB_data[8] = {0};
    
    AD7606_StartConvst();
    AD7606_ReadData(DB_data);
    
    return (float)AD7606B_Digital2Voltage(DB_data[0]);
}

/**
 * @brief 执行自动校准流程 (建立模型 I = a * V + b)
 */
uint8_t CALIB_PerformAutoCalibration(const Calibration_Config* config, 
                                   Calibration_Result* result) {
    if (config == NULL || result == NULL) {
        return 0;
    }

    if (config->num_points < 2) {
        printf("错误: 至少需要2个数据点进行校准\r\n");
        return 0;
    }

    // 分配存储数组：电压(V)作为自变量X，电流(A)作为因变量Y
    float voltages[config->num_points];
    float currents_A[config->num_points];

    float current_step_A = (config->max_current_A - config->min_current_A) / 
                          (config->num_points - 1);

    printf("开始自动校准...\r\n");
    printf("计划采集 %d 个数据点, 电流范围: %.3fA 到 %.3fA\r\n", 
           config->num_points, config->min_current_A, config->max_current_A);
    printf("线性模型: I = a * V + b\r\n");

    // 采集数据点
    for (uint16_t i = 0; i < config->num_points; i++) {
        // 设置恒流源电流 - 需要将A转换为mA
        float target_current_A = config->min_current_A + i * current_step_A;
        float target_current_mA = target_current_A * 1000.0f; // A转mA
        
        OutputConstantCurrent(target_current_A);

        // 等待电流稳定
        CALIB_DelayMs(config->settle_delay_ms);

        // 读取电压值
        float measured_voltage = CALIB_ReadVoltage();
        
        voltages[i] = measured_voltage;
        currents_A[i] = target_current_A;

        printf("数据点 %d: I=%.3fA, V=%.6fV\r\n", 
               i + 1, target_current_A, measured_voltage);
			
			
						delay_ms(500);
			delay_ms(500);
			delay_ms(500);
			delay_ms(500);
    }

    // 执行线性回归计算：I = a * V + b
    float slope, intercept, correlation, std_error;
    CALIB_LinearRegression(voltages, currents_A, config->num_points, 
                         &slope, &intercept, &correlation, &std_error);

    // 存储结果
    result->slope_a = slope;           // 单位: A/V
    result->intercept_b = intercept;   // 单位: A
    result->correlation_coeff = correlation;
    result->std_error = std_error;     // 单位: A
    result->is_valid = (fabs(correlation) > 0.99f) ? 1 : 0;

    printf("数据采集完成，线性回归计算完成...\r\n");
    return 1;
}

/**
 * @brief 使用校准参数计算电流值 (I = a * V + b)
 */
float CALIB_CalculateCurrent(float voltage, const Calibration_Result* result) {
    if (result == NULL || !result->is_valid) {
        return 0.0f;
    }
    return result->slope_a * voltage + result->intercept_b;
}

/**
 * @brief 通过串口输出校准结果
 */
void CALIB_PrintResults(const Calibration_Result* result) {
    if (result == NULL) {
        printf("错误: 校准结果为空\n");
        return;
    }

    printf("\n=== 电流采样校准结果 ===\r\n");
    printf("线性方程: I = a * V + b\r\n");
    printf("具体公式: I = %.6f * V + %.6f\r\n", result->slope_a, result->intercept_b);
    printf("斜率 a: %.6f A/V\r\n", result->slope_a);
    printf("截距 b: %.6f A\r\n", result->intercept_b);
    printf("相关系数 R: %.6f\r\n", result->correlation_coeff);
    printf("标准误差: %.6f A\r\n", result->std_error);
    printf("校准结果: %s\r\n", result->is_valid ? "有效" : "无效");

    if (result->is_valid) {
        printf("校准成功! 相关系数表明线性关系良好。\r\n");
    } else {
        printf("警告: 相关系数较低，线性关系可能不理想。\r\n");
        printf("建议检查硬件连接或重新校准。\r\n");
    }
    printf("========================\r\n\n");
}

/**
 * @brief 线性回归算法计算斜率a和截距b (模型: I = a * V + b)
 */
static void CALIB_LinearRegression(const float* voltages, const float* currents_A, 
                                 uint16_t n, float* slope, float* intercept, 
                                 float* correlation, float* std_error) {
    float sum_v = 0.0f, sum_i = 0.0f;      // V和I的和
    float sum_vv = 0.0f, sum_vi = 0.0f;    // V^2和V*I的和
    float sum_ii = 0.0f;                   // I^2的和

    // 计算各项和
    for (uint16_t i = 0; i < n; i++) {
        sum_v += voltages[i];
        sum_i += currents_A[i];
        sum_vv += voltages[i] * voltages[i];
        sum_vi += voltages[i] * currents_A[i];
        sum_ii += currents_A[i] * currents_A[i];
    }

    // 计算均值
    float mean_v = sum_v / n;
    float mean_i = sum_i / n;

    // 计算Lxx, Lxy, Lyy
    float Lxx = sum_vv - sum_v * sum_v / n;
    float Lxy = sum_vi - sum_v * sum_i / n;
    float Lyy = sum_ii - sum_i * sum_i / n;

    // 计算斜率a和截距b
    if (Lxx != 0.0f) {
        *slope = Lxy / Lxx; // 单位: A/V
    } else {
        *slope = 0.0f;
    }
    *intercept = mean_i - (*slope) * mean_v;

    // 计算相关系数
    if (Lxx != 0.0f && Lyy != 0.0f) {
        *correlation = Lxy / sqrtf(Lxx * Lyy);
    } else {
        *correlation = 0.0f;
    }

    // 计算标准误差
    float sum_sq_error = 0.0f;
    for (uint16_t i = 0; i < n; i++) {
        float predicted = (*slope) * voltages[i] + (*intercept);
        float error = currents_A[i] - predicted;
        sum_sq_error += error * error;
    }
    if (n > 2) {
        *std_error = sqrtf(sum_sq_error / (n - 2));
    } else {
        *std_error = 0.0f;
    }
}

/**
 * @brief 简单延时函数
 */
static void CALIB_DelayMs(uint32_t ms) {
    // 根据您的硬件平台实现延时
    for (volatile uint32_t i = 0; i < ms * 1000; i++);
}

/**
 * @brief 执行完整的电流采样校准流程
 */
void CALIB_RunFullCalibration(void) {
    Calibration_Config config;
    Calibration_Result result;

    // 初始化配置
    CALIB_InitConfig(&config);

    // 执行自动校准
    if (CALIB_PerformAutoCalibration(&config, &result)) {
        // 输出校准结果
        CALIB_PrintResults(&result);

        // 验证校准结果：读取当前电压并计算电流
        float measured_voltage = CALIB_ReadVoltage();
        float calculated_current_A = CALIB_CalculateCurrent(measured_voltage, &result);
        
        printf("校准验证:\r\n");
        printf("实测电压: %.6fV\r\n", measured_voltage);
        printf("计算电流: %.3fA\r\n", calculated_current_A);
    }
}
