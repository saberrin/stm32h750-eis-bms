

#include "DAC_Linear_Calibration.h"
#include "stdio.h"
#include "stdlib.h"

// 静态全局变量
static DAC_LinearCalibration_t dac_calibration = {
    .slope = 1.0f,
    .intercept = 0.0f,
    .r_squared = 0.0f,
    .max_error = 0.0f,
    .avg_error = 0.0f,
    .is_calibrated = 0,
    .num_points = 0,
    .calibration_time_ms = 0
};

static DAC_CalibrationConfig_t calib_config = {
    .settle_time_ms = 50,
    .tolerance = 0.001f,
    .max_retries = 5,
    .samples_per_point = 5,
    .max_calibration_points = 200
};

// 消除未使用变量警告
#define UNUSED(x) ((void)(x))

/**
  * @brief  读取通道3电压平均值（多次采样）
  * @param  samples: 采样次数
  * @retval 平均电压值(V)
  */
static float Read_Channel3_Voltage_Average(uint8_t samples)
{
    uint16_t adc_data[8];
    float sum_voltage = 0.0f;
    
    if (samples == 0) samples = 1;
    
    for (int i = 0; i < samples; i++) {
        AD7606_StartConvst();
        AD7606_Read_Data(adc_data);
        sum_voltage += AD7606B_Digital2Voltage(adc_data[2]); // 通道3监测DAC
        HAL_Delay(10);
    }
    
    return sum_voltage / samples;
		
		

}

/**
  * @brief  计算线性回归系数（最小二乘法）
  * @param  points: 校准点数组
  * @param  n: 点数
  * @param  slope: 斜率输出
  * @param  intercept: 截距输出
  * @param  r_squared: 拟合优度输出
  * @retval 计算成功与否
  */
static uint8_t Calculate_Linear_Regression(DAC_CalibrationPoint_t* points, uint16_t n, 
                                         float* slope, float* intercept, float* r_squared)
{
    if (n < 2) {
        printf("? 线性回归需要至少2个点，当前只有%d个点\r\n", n);
        return 0;
    }
    
    float sum_x = 0.0f, sum_y = 0.0f;
    float sum_xy = 0.0f, sum_xx = 0.0f, sum_yy = 0.0f;
    
    printf("→ 开始线性回归计算，点数：%d\r\n", n);
    
    // 计算总和
    for (int i = 0; i < n; i++) {
        sum_x += points[i].dac_setting;
        sum_y += points[i].measured_voltage;
        sum_xy += points[i].dac_setting * points[i].measured_voltage;
        sum_xx += points[i].dac_setting * points[i].dac_setting;
        sum_yy += points[i].measured_voltage * points[i].measured_voltage;
    }
    
    float mean_x = sum_x / n;
    float mean_y = sum_y / n;
    
    printf("→ 数据统计：X均值=%.6fV, Y均值=%.6fV\r\n", mean_x, mean_y);
    
    // 计算斜率和截距
    float numerator = n * sum_xy - sum_x * sum_y;
    float denominator = n * sum_xx - sum_x * sum_x;
    
    if (fabsf(denominator) < 0.0000001f) {
        printf("? 线性回归计算失败：分母为零\r\n");
        return 0;
    }
    
    *slope = numerator / denominator;
    *intercept = mean_y - (*slope) * mean_x;
    
    printf("→初步计算：斜率=%.6f, 截距=%.6fV\r\n", *slope, *intercept);
    
    // 计算R2
    float ss_tot = 0.0f, ss_res = 0.0f;
    for (int i = 0; i < n; i++) {
        float y_predicted = (*slope) * points[i].dac_setting + (*intercept);
        ss_tot += (points[i].measured_voltage - mean_y) * (points[i].measured_voltage - mean_y);
        ss_res += (points[i].measured_voltage - y_predicted) * (points[i].measured_voltage - y_predicted);
    }
    
    if (ss_tot < 0.0000001f) {
        *r_squared = 0.0f;
    } else {
        *r_squared = 1.0f - (ss_res / ss_tot);
    }
    
    printf("→ 拟合质量：R2=%.6f\r\n", *r_squared);
    
    return 1;
}

/**
  * @brief  初始化DAC线性校准系统
  */
void DAC_LinearCalibration_Init(void)
{
    // 初始化硬件
    DAC8830_Init();
    AD7606_Init();
    
    printf("=== DAC线性校准系统初始化 ===\r\n");
    printf(" 系统初始化完成\r\n");
    
    // 尝试加载已保存的校准参数
    if (DAC_LoadCalibrationParams()) {
        printf("? 校准参数加载成功\r\n");
    }
}

/**
  * @brief  执行线性校准
  * @param  num_points: 校准点数
  * @param  start_voltage: 起始电压(V)
  * @param  end_voltage: 结束电压(V)
  * @retval 校准成功与否
  */
uint8_t DAC_Perform_Linear_Calibration(uint16_t num_points, float start_voltage, float end_voltage)
{
    uint32_t start_time = HAL_GetTick();
    
    if (num_points < 2 || num_points > calib_config.max_calibration_points) {
        printf("? 校准点数必须在2-%d之间\r\n", calib_config.max_calibration_points);
        return 0;
    }
    
    if (start_voltage < 0.0f || end_voltage > 5.0f || start_voltage >= end_voltage) {
        printf("? 电压范围无效：%.3fV-%.3fV\r\n", start_voltage, end_voltage);
        return 0;
    }
    
    printf("\r\n"
           "═══════════════════════════════════════════════════════════════════\r\n"
           "→ 开始DAC线性校准程序\r\n"
           "═══════════════════════════════════════════════════════════════════\r\n");
    printf("→ 校准参数：点数=%d, 范围=%.3fV-%.3fV, 步进=%.3fV\r\n", 
           num_points, start_voltage, end_voltage, (end_voltage - start_voltage) / (num_points - 1));
    
    // 分配内存
    DAC_CalibrationPoint_t* cal_points = (DAC_CalibrationPoint_t*)malloc(num_points * sizeof(DAC_CalibrationPoint_t));
    if (cal_points == NULL) {
        printf("→ 内存分配失败！需要%d字节\r\n", num_points * sizeof(DAC_CalibrationPoint_t));
        return 0;
    }
    
    printf("→ 内存分配成功：%d点，共%d字节\r\n", num_points, num_points * sizeof(DAC_CalibrationPoint_t));
    
    float step_v = (end_voltage - start_voltage) / (num_points - 1);
    float max_error = 0.0f, sum_error = 0.0f;
    
    printf("\r\n采集校准数据：\r\n");
    printf("序号\t目标电压(V)\tDAC设置(V)\t实测电压(V)\t误差(V)\r\n");
    printf("----\t----------\t--------\t----------\t------\r\n");
    
    // 采集校准数据
    for (int i = 0; i < num_points; i++) {
        float target_voltage = start_voltage + i * step_v;
        
        // 设置DAC
     //   DAC8830_set_Voltage(target_voltage);
			  CALIB_DAC_SetVoltage(target_voltage);
        HAL_Delay(calib_config.settle_time_ms);
        
        // 测量电压
        float measured_voltage = Read_Channel3_Voltage_Average(calib_config.samples_per_point)*2;
        float error = measured_voltage - target_voltage;
        
        // 存储数据
        cal_points[i].target_voltage = target_voltage;
        cal_points[i].dac_setting = target_voltage;
        cal_points[i].measured_voltage = measured_voltage;
        cal_points[i].error = error;
        
        // 更新误差统计
        sum_error += fabsf(error);
        if (fabsf(error) > max_error) {
            max_error = fabsf(error);
        }
        
        printf("%d\t%.3f\t%.3f\t%.3f\t%+.3f\r\n", 
               i + 1, target_voltage, target_voltage, measured_voltage, error);
        
        // 进度显示
        if ((i + 1) % 16 == 0) {
            printf("→已完成 %d/%d 点 (%.1f%%)\r\n", i + 1, num_points, (i + 1) * 100.0f / num_points);
        }
        
        HAL_Delay(1);
    }
    
    // 计算线性回归
    printf("\r\n→开始计算线性回归系数...\r\n");
    float slope, intercept, r_squared;
    
    if (Calculate_Linear_Regression(cal_points, num_points, &slope, &intercept, &r_squared)) {
        dac_calibration.slope = slope;
        dac_calibration.intercept = intercept;
        dac_calibration.r_squared = r_squared;
        dac_calibration.max_error = max_error;
        dac_calibration.avg_error = sum_error / num_points;
        dac_calibration.is_calibrated = 1;
        dac_calibration.num_points = num_points;
        dac_calibration.calibration_time_ms = HAL_GetTick() - start_time;
        
        printf("→线性校准完成！\r\n");
    } else {
        printf("→线性回归计算失败\r\n");
        dac_calibration.is_calibrated = 0;
    }
    
    // 生成报告
    DAC_PrintCalibrationInfo();
    
    // 保存校准参数
    DAC_SaveCalibrationParams();
    
    free(cal_points);
    return dac_calibration.is_calibrated;
}

/**
  * @brief  使用校准系数设置DAC电压
  * @param  target_voltage: 目标电压(V)
  * @retval 实际设置的DAC电压值
  */
float DAC_LinearCalibrated_SetVoltage(float target_voltage)
{
    if (target_voltage < 0.0f || target_voltage > 5.0f) {
        printf("→目标电压超出范围：%.3fV，使用原始设置\r\n", target_voltage);
       // DAC8830_set_Voltage(target_voltage);
			CALIB_DAC_SetVoltage(target_voltage);
        return target_voltage;
    }
    
    if (!dac_calibration.is_calibrated) {
        printf("→DAC未校准，使用原始设置：%.3fV\r\n", target_voltage);
       // DAC8830_set_Voltage(target_voltage);
			  CALIB_DAC_SetVoltage(target_voltage);
        return target_voltage;
    }
    
    // 使用校准公式：dac_set = (target - intercept) / slope
    float calibrated_dac = (target_voltage - dac_calibration.intercept) / dac_calibration.slope;
    
		
		
    // 限制输出范围
    if (calibrated_dac < 0.0f) {
        calibrated_dac = 0.0f;
        printf("→校准后DAC设置值超出下限，限制为0V\r\n");
    }
    if (calibrated_dac > 5.0f) {
        calibrated_dac = 5.0f;
        printf("→校准后DAC设置值超出上限，限制为5V\r\n");
    }
    
   // DAC8830_set_Voltage(calibrated_dac);
    CALIB_DAC_SetVoltage(calibrated_dac);
    printf("→校准设置：目标=%.3fV -> DAC=%.3fV (slope=%.6f, intercept=%.6fV)\r\n", 
           target_voltage, calibrated_dac, dac_calibration.slope, dac_calibration.intercept);
    
    return calibrated_dac;
}

/**
  * @brief  打印校准信息
  */
void DAC_PrintCalibrationInfo(void)
{
    printf("\r\n"
           "═══════════════════════════════════════════════════════════════════\r\n"
           "DAC线性校准报告\r\n"
           "═══════════════════════════════════════════════════════════════════\r\n");
    
    printf("基本信息：\r\n");
    printf("   - 校准状态：%s\r\n", dac_calibration.is_calibrated ? "? 已校准" : "? 未校准");
    printf("   - 校准点数：%d\r\n", dac_calibration.num_points);
    printf("   - 校准耗时：%lu ms\r\n", dac_calibration.calibration_time_ms);
    
    if (dac_calibration.is_calibrated) {
        printf("\r\n 校准系数：\r\n");
        printf("   - 斜率系数：%.6f\r\n", dac_calibration.slope);
        printf("   - 截距系数：%.6f V\r\n", dac_calibration.intercept);
  //    printf("!!!!!!!!!!!!!!!!!!!打开 dac8830.c  ,找到 DAC8830_set_Voltage函数，修改校正公式：DAC设置 = (目标电压 - %.6f) / %.6f\r\n", dac_calibration.intercept, dac_calibration.slope);
        printf("!!!!!!!!!!!!!!!!!!!打开 dac8830.c  ,找到 DAC8830_set_Voltage函数，替换校正公式：double DAC_SETV = (targetV - %.6f) / %.6f;\r\n", dac_calibration.intercept, dac_calibration.slope);                                                                                  
        printf("\r\n 校准质量：\r\n");
        printf("   - 拟合优度 R2：%.6f\r\n", dac_calibration.r_squared);
        printf("   - 最大绝对误差：%.3f mV\r\n", dac_calibration.max_error * 1000);
        printf("   - 平均绝对误差：%.3f mV\r\n", dac_calibration.avg_error * 1000);
        
        // 质量评级
        const char* quality_rating;
        if (dac_calibration.r_squared > 0.9999f && dac_calibration.max_error < 0.001f) {
            quality_rating = " 优秀";
        } else if (dac_calibration.r_squared > 0.999f && dac_calibration.max_error < 0.005f) {
            quality_rating = " 很好";
        } else if (dac_calibration.r_squared > 0.99f) {
            quality_rating = " 良好";
        } else {
            quality_rating = " 一般";
        }
        printf("   - 质量评级：%s\r\n", quality_rating);
    }
    
    printf("═══════════════════════════════════════════════════════════════════\r\n");
}

/**
  * @brief  验证校准精度
  * @param  test_voltage: 测试电压(V)
  * @param  max_allowed_error: 最大允许误差(V)
  * @retval 验证通过与否
  */
uint8_t DAC_ValidateCalibration(float test_voltage, float max_allowed_error)
{
    printf("\r\n=== 开始校准验证 ===\r\n");
    
    if (!dac_calibration.is_calibrated) {
        printf(" DAC未校准，无法验证\r\n");
        return 0;
    }
    
    printf(" 验证点：%.3fV，允许误差：±%.3fV\r\n", test_voltage, max_allowed_error);
    
    // 使用校准函数设置DAC
    float calibrated_dac = DAC_LinearCalibrated_SetVoltage(test_voltage);
    HAL_Delay(calib_config.settle_time_ms);
    
    // 测量实际输出
    float measured_voltage = Read_Channel3_Voltage_Average(calib_config.samples_per_point);
    float error = measured_voltage - test_voltage;
    float abs_error = fabsf(error);
    
    printf(" 验证结果：\r\n");
    printf("   - 目标电压：%.6f V\r\n", test_voltage);
    printf("   - 校准DAC设置：%.6f V\r\n", calibrated_dac);
    printf("   - 实测电压：%.6f V\r\n", measured_voltage);
    printf("   - 绝对误差：%.6f V\r\n", abs_error);
    
    uint8_t validation_passed = (abs_error <= max_allowed_error) ? 1 : 0;
    
    if (validation_passed) {
        printf(" 校准验证通过！误差在允许范围内\r\n");
    } else {
        printf(" 校准验证未通过！误差超出允许范围\r\n");
    }
    
    return validation_passed;
}

/**
  * @brief  自动校准
  * @param  min_voltage: 最小电压(V)
  * @param  max_voltage: 最大电压(V)
  * @param  points: 点数
  * @retval 校准成功与否
  */
uint8_t DAC_AutoCalibration(float min_voltage, float max_voltage, uint16_t points)
{
    if (points == 0) points = 128; // 默认128点
    printf("开始DAC线性校准...\r\n");
    return DAC_Perform_Linear_Calibration(points, min_voltage, max_voltage);
}

/**
  * @brief  保存校准参数到非易失存储器
  */
void DAC_SaveCalibrationParams(void)
{
    // 这里需要根据您的具体硬件实现Flash/EEPROM存储
    printf("  校准参数已保存（需要实现具体存储硬件）\r\n");
}

/**
  * @brief  从非易失存储器加载校准参数
  * @retval 加载成功与否
  */
uint8_t DAC_LoadCalibrationParams(void)
{
    // 这里需要根据您的具体硬件实现Flash/EEPROM读取
    printf("  尝试加载校准参数（需要实现具体存储硬件）\r\n");
    return 0;
}

/**
  * @brief  获取校准状态
  * @retval 校准状态
  */
uint8_t DAC_GetCalibrationStatus(void)
{
    return dac_calibration.is_calibrated;
}

/**
  * @brief  获取斜率系数
  * @retval 斜率值
  */
float DAC_GetCalibrationSlope(void)
{
    return dac_calibration.slope;
}

/**
  * @brief  获取截距系数
  * @retval 截距值(V)
  */
float DAC_GetCalibrationIntercept(void)
{
    return dac_calibration.intercept;
}

/**
  * @brief  获取校准质量
  * @retval R2值
  */
float DAC_GetCalibrationQuality(void)
{
    return dac_calibration.r_squared;
}






