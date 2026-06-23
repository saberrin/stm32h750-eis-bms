

#include "Potentiometer_Calibration.h"
#include "stdio.h"
#include "math.h"

// 全局变量定义
//static AD7606_RangeTypeDef current_range = AD7606_RANGE_10V;
//static AD620_GainChannel_t current_gain = AD620_GAIN_10;
//static uint8_t calibration_valid = 0;

// 校准配置结构
typedef struct {
    float start_voltage;
    float end_voltage;
    float step_voltage;
    uint16_t samples_per_point;
    uint16_t settle_time_ms;
} CalibrationConfig_t;

//static CalibrationConfig_t calib_config = {
//    .start_voltage = 0.1f,
//    .end_voltage = 2.5f,
//    .step_voltage = 0.1f,
//    .samples_per_point = 10,
//    .settle_time_ms = 50
//};

/**
  * @brief  智能电压显示函数（修正换行符）
  */
static void PrintVoltageWithAutoUnit(float voltage, const char* label)
{
    if (fabsf(voltage) >= 1.0f) {
        printf("%s: %8.3f V", label, voltage);
    } else if (fabsf(voltage) >= 0.001f) {
        printf("%s: %8.3f mV", label, voltage * 1000.0f);
    } else {
        printf("%s: %8.3f uV", label, voltage * 1000000.0f);
    }
}

/**
  * @brief  初始化校准系统
  */
void PotentiometerCalibration_Init(void)
{
    AD7606_Init();
    DAC8830_Init();
    AD620_GainCtrl_Init();
    
    AD620_SetGain(AD620_GAIN_10);
 //   AD7606_SetRange(AD7606_RANGE_10V);
  //  current_range = AD7606_RANGE_5V;
  //  current_gain = AD620_GAIN_10;
    
    printf("=== Potentiometer Calibration System Initialized ===\r\n");
    printf("DAC闭环校准功能已就绪\r\n");
}

/**
  * @brief  验证1：所有通道基础读数验证（修正换行符）
  */
void Validate_AllChannels_Reading(void)
{
    printf("\r\n=== 验证1：所有通道基础读数验证 ===\r\n");
    
    uint16_t adc_data[8];
    float channel_voltage[6];
    
    // 设置DAC输出为0V
    DAC8830_set_Voltage(0.0f);
    HAL_Delay(100);
    
    // 读取ADC数据
    for (int i = 0; i < 5; i++) {
        AD7606_StartConvst();
        AD7606_Read_Data(adc_data);
        HAL_Delay(10);
    }
    
    // 转换为电压值
    for (int ch = 0; ch < 6; ch++) {
        channel_voltage[ch] = AD7606B_Digital2Voltage(adc_data[ch]);
    }
    
    // 通道名称定义
    const char* channel_names[] = {
        "通道1(激励电流)", "通道2(AD620输出)", "通道3(DAC监测)", 
        "通道4(AD620+IN)", "通道5(激励电源)", "通道6(系统电源)"
    };
    
    for (int ch = 0; ch < 6; ch++) {
        PrintVoltageWithAutoUnit(channel_voltage[ch], channel_names[ch]);
        
        // 范围检查
        switch(ch) {
            case 0: // 激励电流检测
                if (fabsf(channel_voltage[ch]) > 0.5f) 
                    printf("→可能异常");
                break;
            case 2: // DAC监测
                if (channel_voltage[ch] < 0.0f || channel_voltage[ch] > 5.0f) 
                    printf("→超出预期范围");
                break;
            case 4: // 激励电源
                if (channel_voltage[ch] > 12.0f) 
                    printf("→可能超量程");
                break;
        }
        printf("\r\n");  // 修正：确保每行结束都有\r\n
    }
    
    printf("→所有通道基础读数验证完成\r\n");
}

/**
  * @brief  DAC校准函数（修正换行符）
  */
float DAC_Calibrated_SetVoltage(float target_voltage)
{
    if (target_voltage < 0.0f ) {
        printf("→DAC目标电压超出范围: %.3f V\r\n", target_voltage);
        return target_voltage;
    }
    
    uint16_t adc_data[8];
    float measured_voltage, error;
    float current_target = target_voltage;
    uint8_t retry_count = 0;
    
    do {
        // 设置DAC输出
        DAC8830_set_Voltage(current_target);
        HAL_Delay(50);
        
        // 读取实际输出
        for (int i = 0; i < 3; i++) {
            AD7606_StartConvst();
            AD7606_Read_Data(adc_data);
            HAL_Delay(10);
        }
        measured_voltage = AD7606B_Digital2Voltage(adc_data[2]); // 通道3监测
        
        error = measured_voltage - current_target;
        
        // 检查是否在容差范围内
        if (fabsf(error) <= 0.001f) { // 1mV容差
            break;
        }
        
        // 计算修正值
        float correction = -error * 0.8f;
        current_target += correction;
        
        // 限制电压范围
        if (current_target < 0.0f) current_target = 0.0f;
       
        
        retry_count++;
        
//        printf("DAC校准重试%d: 目标=%.3fV, 实测=%.3fV, 误差=%.3fV, 新目标=%.3fV\r\n",
//               retry_count, current_target - correction, measured_voltage, error, current_target);
        
    } while (retry_count <5);
    
    // 最终结果输出
//    if (fabsf(measured_voltage - target_voltage) <= 0.005f) {
//        if (retry_count > 0) {
//            printf(" DAC校准成功: 目标=%.3fV, 最终输出=%.3fV, 误差=%.3fV, 重试%d次\r\n",
//                   target_voltage, measured_voltage, measured_voltage - target_voltage, retry_count);
//        }
//    } else {
//        printf(" DAC校准未完全收敛: 目标=%.3fV, 最终输出=%.3fV, 剩余误差=%.3fV\r\n",
//               target_voltage, measured_voltage, measured_voltage - target_voltage);
//    }
    
    return measured_voltage;
}

/**
  * @brief  验证2：AD620增益关系验证（修正换行符）
  */
//void Validate_CH2_AD620_Gain(void)
//{
//    printf("\r\n=== 验证2：AD620增益关系验证（智能防饱和） ===\r\n");
//    printf("策略：调整DAC输出，使V_diff = V_ch4 - V_ch3 适应高增益，避免饱和\r\n");
//    
//    uint16_t adc_data[8];
//    float input_plus, input_minus, diff_input, output_voltage, actual_gain;
//    
//    // 先测量基准电压
//    DAC_Calibrated_SetVoltage(0.0f);
//    HAL_Delay(100);
//    
//    for (int i = 0; i < 3; i++) {
//        AD7606_StartConvst();
//        AD7606_Read_Data(adc_data);
//        HAL_Delay(10);
//    }
//    float v_ref_initial = AD7606B_Digital2Voltage(adc_data[3]); // 通道4基准
//    printf("【基准】无激励时通道4(AD620+IN)电压: %.3f V\r\n", v_ref_initial);
//    
//    // 定义增益设置
//    AD620_GainChannel_t gain_settings[] = {
//        AD620_GAIN_2, AD620_GAIN_5, AD620_GAIN_10, AD620_GAIN_50,
//        AD620_GAIN_100, AD620_GAIN_200, AD620_GAIN_500, AD620_GAIN_1000
//    };
//    const char* gain_names[] = {"2倍", "5倍", "10倍", "50倍", "100倍", "200倍", "500倍", "1000倍"};
//    float nominal_gains[] = {2.0f, 5.0f, 10.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f};
//    int num_gains = 8;
//    
//    printf("增益设置\t标称增益\t目标V_diff\t设定V_dac\t实测V_diff\tAD620输出(V)\t实际增益\t状态\r\n");
//    printf("--------\t--------\t----------\t--------\t----------\t----------\t--------\t----\r\n");
//    
//    for (int i = 0; i < num_gains; i++) {
//        AD620_SetGain(gain_settings[i]);
//        HAL_Delay(30);
//        
//        float nominal_gain = nominal_gains[i];
//        
//        // 智能计算安全电压
//        float target_v_diff;
//        if (nominal_gain >= 100.0f) {
//            target_v_diff = 0.005f;  // 高增益：5mV
//        } else if (nominal_gain >= 10.0f) {
//            target_v_diff = 0.010f;  // 中增益：10mV
//        } else {
//            target_v_diff = 0.100f;  // 低增益：100mV
//        }
//        
//        float target_dac = v_ref_initial - target_v_diff;
//        if (target_dac < 0.0f) target_dac = 0.0f;
//        if (target_dac > 5.0f) target_dac = 5.0f;
//        
//        // 使用校准后的DAC设置
//        float final_dac = DAC_Calibrated_SetVoltage(target_dac);

//				
//				
//        // 最终读取
//        for (int j = 0; j < 3; j++) {
//            AD7606_StartConvst();
//            AD7606_Read_Data(adc_data);
//            HAL_Delay(10);
//        }
//        
//        input_plus = AD7606B_Digital2Voltage(adc_data[3]);  // 通道4
//        input_minus = AD7606B_Digital2Voltage(adc_data[2]); // 通道3
//        output_voltage = AD7606B_Digital2Voltage(adc_data[1]); // 通道2
//        diff_input = input_plus - input_minus;
//        
//        // 计算实际增益
//        actual_gain = (fabsf(diff_input) > 0.0001f) ? (output_voltage-2.5f) / diff_input : 0.0f;
//        
//        // 输出结果（确保每行都有\r\n）
//        printf("  %s\t    %8.0f\t    %9.3f V\t      %7.3f V\t    %9.3f V\t    %9.3f\t           %8.1f", 
//               gain_names[i], nominal_gain, target_v_diff, final_dac, 
//               diff_input, output_voltage, actual_gain);
//        
//        // 状态评估
//        if (fabsf(output_voltage) > 8.0f) {
//            printf("\t饱和");
//        } else {
//            float gain_error = fabsf(actual_gain - nominal_gain) / nominal_gain * 100.0f;
//            if (gain_error < 5.0f) {
//                printf("\t");
//            } else {
//                printf("\t  %.1f%%", gain_error);
//            }
//        }
//        printf("\r\n");  // 确保每行结束
//        
//        HAL_Delay(200);
//    }
//    
//    printf("→AD620全增益扫描完成（8个档位）\r\n");
//}
//void Validate_CH2_AD620_Gain(void)
//{
//    printf("\r\n=== 验证2：AD620增益关系验证（智能防饱和） ===\r\n");
//    printf("策略：调整DAC输出，使V_diff = V_ch4 - V_ch3 适应高增益，避免饱和\r\n");
//    
//    uint16_t adc_data[8];
//    float input_plus, input_minus, diff_input, output_voltage, actual_gain;
//    const float OUTPUT_OFFSET = 2.5f; // AD620输出偏置电压
//    
//    // 先测量基准电压
//    DAC_Calibrated_SetVoltage(0.0f);
//    HAL_Delay(100);
//    
//    for (int i = 0; i < 3; i++) {
//        AD7606_StartConvst();
//        AD7606_Read_Data(adc_data);
//        HAL_Delay(10);
//    }
//    float v_ref_initial = AD7606B_Digital2Voltage(adc_data[3]); // 通道4基准
//    printf("【基准】无激励时通道4(AD620+IN)电压: %.3f V\r\n", v_ref_initial);
//    
//    // 定义增益设置
//    AD620_GainChannel_t gain_settings[] = {
//        AD620_GAIN_2, AD620_GAIN_5, AD620_GAIN_10, AD620_GAIN_50,
//        AD620_GAIN_100, AD620_GAIN_200, AD620_GAIN_500, AD620_GAIN_1000
//    };
//    const char* gain_names[] = {"2倍", "5倍", "10倍", "50倍", "100倍", "200倍", "500倍", "1000倍"};
//    float nominal_gains[] = {2.0f, 5.0f, 10.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f};
//    int num_gains = 8;
//    
//    // 使用固定宽度的格式控制符替换原来的\t和空格
//    printf("%-8s %-12s %-12s %-12s %-12s %-12s %-12s %s\r\n", 
//           "增益设置", "标称增益", "目标V_diff", "设定V_dac", "实测V_diff", "AD620输出(V)", "实际增益", "状态");
//    printf("%-8s %-12s %-12s %-12s %-12s %-12s %-12s %s\r\n", 
//           "--------", "--------", "----------", "--------", "----------", "----------", "--------", "----");
//    
//    for (int i = 0; i < num_gains; i++) {
//        AD620_SetGain(gain_settings[i]);
//        HAL_Delay(30);
//        
//        float nominal_gain = nominal_gains[i];
//        
//        // 智能计算安全电压
//        float target_v_diff;
//        if (nominal_gain >= 100.0f) {
//            target_v_diff = 0.005f;  // 高增益：5mV
//        } else if (nominal_gain >= 10.0f) {
//            target_v_diff = 0.010f;  // 中增益：10mV
//        } else {
//            target_v_diff = 0.100f;  // 低增益：100mV
//        }
//        
//        float target_dac = v_ref_initial - target_v_diff;
//        if (target_dac < 0.0f) target_dac = 0.0f;
//        if (target_dac > 5.0f) target_dac = 5.0f;
//        
//        // 使用校准后的DAC设置
//        float final_dac = DAC_Calibrated_SetVoltage(target_dac);
//        
//        // 最终读取
//        for (int j = 0; j < 3; j++) {
//            AD7606_StartConvst();
//            AD7606_Read_Data(adc_data);
//            HAL_Delay(10);
//        }
//        
//        input_plus = AD7606B_Digital2Voltage(adc_data[3]);  // 通道4
//        input_minus = AD7606B_Digital2Voltage(adc_data[2]); // 通道3
//        output_voltage = AD7606B_Digital2Voltage(adc_data[1]); // 通道2
//        diff_input = input_plus - input_minus;
//        
//        // 计算实际增益（已修正偏置电压影响）
//        if (fabsf(diff_input) > 0.0001f) {
//            actual_gain = (output_voltage - OUTPUT_OFFSET) / diff_input;
//        } else {
//            actual_gain = 0.0f;
//        }
//        
//        // 使用统一的格式控制符输出数据
//        printf("%-8s %-12.0f %-12.3fV %-12.3fV %-12.3fV %-12.3f %-12.1f", 
//               gain_names[i], nominal_gain, target_v_diff, final_dac, 
//               diff_input, output_voltage, actual_gain);
//        
//        // 状态评估
//        if (fabsf(output_voltage) > 8.0f) {
//            printf("饱和");
//        } else {
//            float gain_error = fabsf(actual_gain - nominal_gain) / nominal_gain * 100.0f;
//            if (gain_error < 5.0f) {
//                printf("正常");
//            } else {
//                printf("%.1f%%", gain_error);
//            }
//        }
//        printf("\r\n");
//        
//        HAL_Delay(200);
//    }
//    
//    printf("→AD620全增益扫描完成（8个档位）\r\n");
//}


/**
  * @brief  高精度动态闭环DAC校准函数 (PID控制)
  * @param  target_voltage_below_ch4: 期望DAC输出比通道4低的电压值 (单位: V)
  * @retval 最终建立时通道3的实际电压
  */
float DAC_Set_Below_CH4(float target_voltage_below_ch4)
{
    if (target_voltage_below_ch4 < 0) {
        printf("→错误：目标电压差值不能为负。\r\n");
        return -1.0f;
    }

    uint16_t adc_data[8];
    float v_ch4_measured, v_ch3_target, v_ch3_measured;
    float error = 0.0f, last_error = 0.0f, integral = 0.0f;
    uint8_t retry_count = 0;
    const float tolerance = 0.0005f; // 0.5mV容差，更高精度
    
    // PID参数 (根据系统响应调整)
    const float Kp = 0.9f;   // 比例系数
    const float Ki = 0.05f;  // 积分系数  
    const float Kd = 0.01f;  // 微分系数
    
    do {
        // 1. 动态读取当前通道4电压作为基准
        for (int i = 0; i < 5; i++) { // 增加采样次数提高稳定性
            AD7606_StartConvst();
            AD7606_Read_Data(adc_data);
            HAL_Delay(1);
        }
        v_ch4_measured = AD7606B_Digital2Voltage(adc_data[3]);
        
        // 2. 计算DAC目标电压：V_ch3_target = V_ch4_measured - target_voltage_below_ch4
        v_ch3_target = v_ch4_measured - target_voltage_below_ch4;
        
        // 电压范围保护
        if (v_ch3_target < 0.0f) v_ch3_target = 0.0f;
        if (v_ch3_target > 5.0f) v_ch3_target = 5.0f;
        
        // 3. 设置DAC输出
        DAC8830_set_Voltage(v_ch3_target);
        HAL_Delay(25); // 适当延时确保DAC稳定
        
        // 4. 读取通道3实际电压 (DAC输出反馈)
        for (int i = 0; i < 5; i++) {
            AD7606_StartConvst();
            AD7606_Read_Data(adc_data);
            HAL_Delay(1);
        }
        v_ch3_measured = AD7606B_Digital2Voltage(adc_data[2]);
        
        // 5. PID算法计算修正量
        last_error = error;
        error = v_ch3_measured - v_ch3_target; // 当前误差
        
        integral += error; // 积分项累积
        // 积分限幅防止饱和
        if (integral > 0.1f) integral = 0.1f;
        if (integral < -0.1f) integral = -0.1f;
        
        float derivative = error - last_error; // 微分项
        float correction = -(Kp * error + Ki * integral + Kd * derivative);
        
        // 6. 应用修正
        v_ch3_target += correction;
        
        // 再次限制电压范围
        if (v_ch3_target < 0.0f) v_ch3_target = 0.0f;
        if (v_ch3_target > 5.0f) v_ch3_target = 5.0f;
        
        retry_count++;
        
        // 调试信息 (需要时可取消注释)
        // printf("PID校准%d: 目标V_diff=%.4fV, 目标V_ch3=%.4fV, 实测V_ch3=%.4fV, 误差=%.4fV\r\n", 
        //        retry_count, target_voltage_below_ch4, v_ch3_target - correction, 
        //        v_ch3_measured, error);
        
        // 检查是否满足精度要求
        if (fabsf(error) <= tolerance) {
            break;
        }
        
    } while (retry_count < 8); // 最多尝试8次
    
    // 计算最终建立的V_diff
    float final_v_diff = v_ch4_measured - v_ch3_measured;
    
//    // 最终状态报告
//    if (fabsf(error) <= tolerance) {
//        if (retry_count > 1) {
//            printf("→PID校准成功: 目标V_diff=%.4fV, 实际V_diff=%.4fV, 尝试%d次\r\n", 
//                   target_voltage_below_ch4, final_v_diff, retry_count);
//        }
//    } else {
//        printf("→PID校准未完全收敛: 目标V_diff=%.4fV, 实际V_diff=%.4fV, 剩余误差=%.4fV\r\n", 
//               target_voltage_below_ch4, final_v_diff, error);
//    }
    
    return v_ch3_measured;
}

/**
  * @brief  验证2：AD620增益关系验证（高速优化版）
  * @note   优化策略：减少延时、精简调试、优化采样逻辑
  */
void Validate_CH2_AD620_Gain(void)
{
    printf("\r\n=== 验证2：AD620增益关系验证（高速优化版） ===\r\n");
    printf("策略：最小化延时，优化采样效率，快速建立正V_diff\r\n");
    
    uint16_t adc_data[8];
    float input_plus, input_minus, diff_input, output_voltage, actual_gain;
    const float OUTPUT_OFFSET = 2.5f;
    const float IDEAL_OUTPUT_MIN = 1.0f;
    const float IDEAL_OUTPUT_MAX = 2.0f;
    
    // 定义优化参数
    #define FAST_DELAY_GAIN_SWITCH   10  // 增益切换稳定时间(ms)
    #define FAST_DELAY_ADC_SETTLE     5  // DAC设置后稳定时间(ms)
    #define FAST_DELAY_ADC_READ       1  // ADC读取间隔(ms)
    #define FAST_ADC_SAMPLES          3  // 快速采样次数
    #define DEBUG_LEVEL               1  // 0=静默, 1=简要, 2=详细
    
    // 快速测量基准电压
    DAC_Calibrated_SetVoltage(0.0f);
    HAL_Delay(30);
    
    for (int i = 0; i < FAST_ADC_SAMPLES; i++) {
        AD7606_StartConvst();
        AD7606_Read_Data(adc_data);
        HAL_Delay(FAST_DELAY_ADC_READ);
    }
    float v_ref_initial = AD7606B_Digital2Voltage(adc_data[3]);
    printf("【基准】通道4电压: %.3f V\r\n", v_ref_initial);
    
    // 定义增益设置
    AD620_GainChannel_t gain_settings[] = {
        AD620_GAIN_2, AD620_GAIN_5, AD620_GAIN_10, AD620_GAIN_50,
        AD620_GAIN_100, AD620_GAIN_200, AD620_GAIN_500, AD620_GAIN_1000
    };
    const char* gain_names[] = {"2倍", "5倍", "10倍", "50倍", "100倍", "200倍", "500倍", "1000倍"};
    float nominal_gains[] = {2.0f, 5.0f, 10.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f};
    int num_gains = 8;
    
			// 第528行 (修正后)
			printf("%-8s %-10s %-12s %-12s %-12s %-12s %-12s %-12s %s\r\n", 
						 "增益设置", "标称增益", "目标V_diff", "设定V_dac", "实测V_diff", 
						 "AD620输出(V)", "有效输出", "实际增益", "状态"); // 注意格式字符串里增加了一个 %-12s

			// 第531行 (修正后)
			printf("%-8s %-10s %-12s %-12s %-12s %-12s %-12s %-12s %s\r\n", 
						 "--------", "--------", "----------", "--------", "----------", 
						 "----------", "--------", "--------", "----");
    for (int i = 0; i < num_gains; i++) {
        AD620_SetGain(gain_settings[i]);
        HAL_Delay(FAST_DELAY_GAIN_SWITCH);
        
        float nominal_gain = nominal_gains[i];
        float target_v_diff = 0.0f;
        uint32_t total_attempts = 0;
        uint8_t success_flag = 0;
        uint8_t phase = 1;
        
        // 设置初始目标V_diff
        if (nominal_gain >= 500.0f) {
            target_v_diff = 0.030f;
        } else if (nominal_gain >= 200.0f) {
            target_v_diff = 0.040f;
        } else if (nominal_gain >= 100.0f) {
            target_v_diff = 0.050f;
        } else if (nominal_gain >= 50.0f) {
            target_v_diff = 0.060f;
        } else if (nominal_gain >= 10.0f) {
            target_v_diff = 0.080f;
        } else {
            target_v_diff = 0.150f;
        }
        
        if (DEBUG_LEVEL >= 2) {
            printf("【增益%s】开始调节...\r\n", gain_names[i]);
        }
        
        // 优化后的自适应循环
        while (total_attempts < 15) { // 限制最大尝试次数
            total_attempts++;
            
            // 计算并设置DAC
            float target_dac = v_ref_initial - target_v_diff;
            if (target_dac < 0.0f) target_dac = 0.0f;
            if (target_dac > 5.0f) target_dac = 5.0f;
            
            DAC_Calibrated_SetVoltage(target_dac);
            HAL_Delay(FAST_DELAY_ADC_SETTLE);
            
            // 快速单次采样（监控用）
            AD7606_StartConvst();
            AD7606_Read_Data(adc_data);
            
            input_plus = AD7606B_Digital2Voltage(adc_data[3]);
            input_minus = AD7606B_Digital2Voltage(adc_data[2]);
            output_voltage = AD7606B_Digital2Voltage(adc_data[1]);
            diff_input = input_plus - input_minus;
            
            float effective_output = output_voltage - OUTPUT_OFFSET;
            float effective_output_abs = fabsf(effective_output);
            
            // 阶段1：建立正V_diff
            if (phase == 1) {
                if (diff_input > 0.005f) {
                    phase = 2;
                    if (DEBUG_LEVEL >= 2) {
                        printf("  阶段1达成，V_diff=%.4fV\r\n", diff_input);
                    }
                    continue;
                } else {
                    target_v_diff += (nominal_gain >= 100.0f) ? 0.005f : 0.010f;
                }
            } 
            // 阶段2：优化输出幅度
            else if (phase == 2) {
                if (effective_output_abs >= IDEAL_OUTPUT_MIN && 
                    effective_output_abs <= IDEAL_OUTPUT_MAX) {
                    success_flag = 1;
                    break;
                } else if (effective_output_abs < IDEAL_OUTPUT_MIN) {
                    target_v_diff *= 1.15f;
                } else {
                    target_v_diff *= 0.85f;
                }
            }
            
            // 安全限制
            if (target_v_diff < 0.001f) target_v_diff = 0.001f;
            if (target_v_diff > 0.300f) target_v_diff = 0.300f;
            
            if (DEBUG_LEVEL >= 2 && total_attempts % 3 == 0) {
                printf("  尝试%lu：V_diff=%.4f，有效输出=%.3fV\r\n", 
                       total_attempts, diff_input, effective_output_abs);
            }
        }
        
        // 最终精确测量（多次采样取平均）
        HAL_Delay(20);
        float sum_plus = 0, sum_minus = 0, sum_output = 0;
        for (int j = 0; j < 5; j++) {
            AD7606_StartConvst();
            AD7606_Read_Data(adc_data);
            HAL_Delay(2);
            
            sum_plus += AD7606B_Digital2Voltage(adc_data[3]);
            sum_minus += AD7606B_Digital2Voltage(adc_data[2]);
            sum_output += AD7606B_Digital2Voltage(adc_data[1]);
        }
        
        input_plus = sum_plus / 5;
        input_minus = sum_minus / 5;
        output_voltage = sum_output / 5;
        diff_input = input_plus - input_minus;
        
        // 计算实际增益
        float effective_output = output_voltage - OUTPUT_OFFSET;
        float effective_output_abs = fabsf(effective_output);
        if (fabsf(diff_input) > 0.0001f) {
            actual_gain = effective_output_abs / fabsf(diff_input);
        } else {
            actual_gain = 0.0f;
        }
        
        // 精简输出格式
        printf("%-8s %-10.0f %-12.4fV %-12.4fV %-12.4fV %-12.4f %-10.1f", 
               gain_names[i], nominal_gain, target_v_diff, diff_input, 
               output_voltage, effective_output, actual_gain);
        
        // 状态评估
        if (!success_flag) {
            printf(phase == 1 ? "V_diff未建立" : "幅度未优化");
        } else {
            float gain_error = fabsf(actual_gain - nominal_gain) / nominal_gain * 100.0f;
            
            const char* status;
            if (gain_error < 2.0f) status = "优秀";
            else if (gain_error < 5.0f) status = "良好";
            else if (gain_error < 10.0f) status = "一般";
            else if (gain_error < 20.0f) status = "较差";
            else status = "异常";
            
            printf("%s(%.1f%%)", status, gain_error);
        }
        printf(" %lu次\r\n", total_attempts);
        
        // 最小化档位间延时
        if (i < num_gains - 1) {
            HAL_Delay(nominal_gain >= 100.0f ? 50 : 20);
        }
    }
    
    printf("→AD620扫描完成（高速模式）\r\n");
    
    // 性能总结
    printf("\r\n【优化效果】\r\n");
    printf("? 延时减少：增益切换%dms→%dms，ADC稳定%dms→%dms\r\n", 
           80, FAST_DELAY_GAIN_SWITCH, 20, FAST_DELAY_ADC_SETTLE);
    printf("? 采样优化：监控单次采样，最终5次平均\r\n");
    printf("? 调试精简：详细调试可设置DEBUG_LEVEL=2\r\n");
}

/**
  * @brief  验证3：DAC输出精度验证（修正换行符）
  */
void Validate_CH3_DAC_Output(float start_v, float end_v, float step_v)
{
    printf("\r\n=== 验证3：DAC输出精度验证 ===\r\n");
    
    uint16_t adc_data[8];
    float measured_voltage, error;
    
    for (float voltage = start_v; voltage <= end_v + 0.001f; voltage += step_v) {
        float actual_output = DAC_Calibrated_SetVoltage(voltage);
        
        // 读取验证
        for (int i = 0; i < 3; i++) {
            AD7606_StartConvst();
            AD7606_Read_Data(adc_data);
            HAL_Delay(10);
        }
        measured_voltage = AD7606B_Digital2Voltage(adc_data[2]);
        
        error = measured_voltage - voltage;
        
        printf("DAC目标=");
        PrintVoltageWithAutoUnit(voltage, "");
        printf(", 实际输出=");
        PrintVoltageWithAutoUnit(actual_output, "");
        printf(", 通道3监测=");
        PrintVoltageWithAutoUnit(measured_voltage, "");
        printf(", 最终误差=");
        PrintVoltageWithAutoUnit(error, "");
        
        if (fabsf(error) > 0.01f) {
            printf("→误差较大");
        }
        printf("\r\n");  // 确保换行
        
        HAL_Delay(150);
    }
    
    printf("→DAC输出验证完成\r\n");
}

///**
//  * @brief  验证4：通道5激励电源量程验证（修正换行符）
//  */
//void Validate_CH5_AD7606_Range(void)
//{
//    printf("\r\n=== 验证4：通道5激励电源量程验证 ===\r\n");
//    
//    uint16_t adc_data[8];
//    float channel_voltage[6];
//    
//    AD7606_RangeTypeDef ranges[] = {AD7606_RANGE_5V, AD7606_RANGE_10V};
//    const char* range_names[] = {"5V", "10V"};
//    
//    for (int i = 0; i < 2; i++) {
//        AD7606_SetRange(ranges[i]);
//        HAL_Delay(10);
//        
//        for (int j = 0; j < 3; j++) {
//            AD7606_StartConvst();
//            AD7606_Read_Data(adc_data);
//            HAL_Delay(10);
//        }
//        
//        for (int ch = 0; ch < 6; ch++) {
//            channel_voltage[ch] = AD7606B_Digital2Voltage(adc_data[ch]);
//        }
//        
//        printf("AD7606量程: ±%s, 通道5读数: ", range_names[i]);
//        PrintVoltageWithAutoUnit(channel_voltage[4], "");
//        
//        if (ranges[i] == AD7606_RANGE_5V && fabsf(channel_voltage[4]) > 5.0f) {
//            printf("→可能超量程");
//        }
//        printf("\r\n");  // 确保换行
//        
//        HAL_Delay(100);
//    }
//    
//    AD7606_SetRange(AD7606_RANGE_10V);
//    printf("→通道5量程验证完成\r\n");
//}

/**
  * @brief  校准结果验证（修正换行符）
  */
uint8_t PotentiometerCalibration_ValidateCalibration(void)
{
    printf("\r\n=== 校准结果验证 ===\r\n");
    
    uint16_t adc_data[8];
    float measured_voltage;
    
    float test_voltage = 1.5f;
    float actual_output = DAC_Calibrated_SetVoltage(test_voltage);
    
    // 最终验证读取
    for (int i = 0; i < 3; i++) {
        AD7606_StartConvst();
        AD7606_Read_Data(adc_data);
        HAL_Delay(10);
    }
    measured_voltage = AD7606B_Digital2Voltage(adc_data[2]);
    
    float error = fabsf(measured_voltage - test_voltage);
    uint8_t validation_passed = (error <= 0.01f) ? 1 : 0;
    
    printf("目标电压: ");
    PrintVoltageWithAutoUnit(test_voltage, "");
    printf(", 最终输出: ");
    PrintVoltageWithAutoUnit(actual_output, "");
    printf(", 误差: ");
    PrintVoltageWithAutoUnit(error, "");
    printf("\r\n");
    
    if (validation_passed) {
        printf("→校准验证通过!\r\n");
    } else {
        printf("→校准验证未通过!\r\n");
    }
    
//    calibration_valid = validation_passed;
    return validation_passed;
}

/**
  * @brief  运行完整的测试流程
  */
void Run_Complete_Calibration_Test(void)
{
    printf("开始分通道验证测试...\r\n");
    
    Validate_AllChannels_Reading();
    HAL_Delay(1000);
    
    Validate_CH2_AD620_Gain();
    HAL_Delay(1000);
    
    Validate_CH3_DAC_Output(0.5f, 2.0f, 0.5f);
    HAL_Delay(1000);
    
//    Validate_CH5_AD7606_Range();
    HAL_Delay(1000);
    
    PotentiometerCalibration_ValidateCalibration();
    
    printf("=== 所有验证完成 ===\r\n");
}

