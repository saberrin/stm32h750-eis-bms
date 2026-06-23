//#include "math.h"  // 包含数学库
//#include "SineWave.h"
//#include "system.h"
//#define DAC_RESOLUTION  4095  // 12位DAC最大值
//#define PI 3.14159265358979323846

//extern uint16_t SIN_DATA;

///**
//  * @brief  生成单周期正弦波数据表
//  * @param  buffer: 输出数组指针
//  * @param  length: 单周期点数(建议32/64/128等2的幂)
//  * @param  amplitude: 幅值比例(0.0-1.0对应0%-100% DAC量程)
//  * @param  v_offset: 直流偏置电压比例(0.0-1.0)
//  * @retval None
//  */
//void GenerateSineWave(uint16_t *buffer, float amplitude, float v_offset) {
//    float max_voltage = DAC_RESOLUTION * amplitude;
//    float offset = max_voltage /2;
//    uint32_t i = 0;
//    for(i = 0; i < SIN_DATA; i++) {
//        float radian = 2 * PI * i / SIN_DATA;  // 生成0~2π弧度
//        float value = offset + max_voltage * sinf(radian)/2+0;
//        
//        // 限幅保护
//        if(value > DAC_RESOLUTION) value = DAC_RESOLUTION;
//        else if(value < 0) value = 0;
//        
//        buffer[i] = (uint16_t)fmaxf(0, fminf(DAC_RESOLUTION, roundf(value)));
//				//printf("Voltage: %u\n", buffer[i]);
//    }
//}

//void generate_frequency_points(double *freq_points, double Freq_Start, double Freq_End, double Period_Count,double num_decades,int total_points) {
//    double Freq_Start_log = log10(Freq_Start);
//    double step = 1 / Period_Count; // 总步长归一化
//    for (int i = 0; i < total_points; i++) {
//        double exponent = Freq_Start_log - i * step;
//        freq_points[i] = pow(10, exponent);
//    }
//}
#include "math.h"  // 包含数学库
#include "SineWave.h"
#include "system.h"
#define DAC_RESOLUTION  4095  // 12位DAC最大值
#define PI 3.14159265358979323846

extern uint16_t SIN_DATA;

///**
//  * @brief  生成单周期正弦波数据表
//  * @param  buffer: 输出数组指针
//  * @param  amplitude: 幅值比例(0.0-1.0对应0%-100% DAC量程)
//  * @param  v_offset: 直流偏置电压比例(0.0-1.0)
//  * @retval None
//  */
//void GenerateSineWave(uint16_t *buffer, float amplitude, float v_offset) {
//    float max_voltage = DAC_RESOLUTION * amplitude;
//    float offset = max_voltage /2;
//    uint32_t i = 0;
//    for(i = 0; i < SIN_DATA; i++) {
//        float radian = 2 * PI * i / SIN_DATA;  // 生成0~2π弧度
//        float value = offset + max_voltage * sinf(radian)/2+0;
//        
//        // 限幅保护
//        if(value > DAC_RESOLUTION) value = DAC_RESOLUTION;
//        else if(value < 0) value = 0;
//        
//        buffer[i] = (uint16_t)fmaxf(0, fminf(DAC_RESOLUTION, roundf(value)));
//				//printf("Voltage: %u\n", buffer[i]);
//    }
//}


/**
  * @brief  生成单周期正弦波数据表
  * @param  buffer: 输出数组指针
  * @param  amplitude: 幅值比例(0.0-1.0对应0%-100% DAC量程)
  * @param  v_offset: 直流偏置电压比例(0.0-1.0)，控制波谷距离0的位置
  * @retval None
  */
void GenerateSineWave(uint16_t *buffer, float amplitude, float v_offset) {
    // 参数限制
    if(amplitude < 0) amplitude = 0;
    if(amplitude > 1) amplitude = 1;
    if(v_offset < 0) v_offset = 0;
    if(v_offset > 1) v_offset = 1;
    
    // 计算峰值和偏置
    float peak_to_peak = DAC_RESOLUTION * amplitude;     // 峰峰值（正弦波的摆幅）
    float valley = DAC_RESOLUTION * v_offset;            // 波谷位置（最低点距离0的值）
    
    // 计算波峰位置
    float peak = valley + peak_to_peak;                   // 波峰位置
    
    // 检查是否超出DAC范围
    if(peak > DAC_RESOLUTION) {
        // 如果波峰超出范围，自动调整峰峰值
        peak_to_peak = DAC_RESOLUTION - valley;
    }
    
    // 计算直流偏置（波峰和波谷的中点）
    float offset_voltage = valley + peak_to_peak / 2;
    
    // 生成正弦波数据
    for(uint32_t i = 0; i < SIN_DATA; i++) {
        // 计算弧度：0 到 2π
        float radian = 2 * PI * i / SIN_DATA;
        
        // 计算正弦值：sin(radian) 范围 [-1, 1]
        // 转换为：offset + (peak_to_peak/2) * sin(radian)
        // 当 sin = -1 时：value = offset - peak_to_peak/2 = valley
        // 当 sin = 1 时：value = offset + peak_to_peak/2 = peak
        float value = offset_voltage + (peak_to_peak / 2) * sinf(radian);
        
        // 限幅保护（双重保险）
        value = fmaxf(0, fminf(DAC_RESOLUTION, value));
        
        // 四舍五入并转换为uint16_t
        buffer[i] = (uint16_t)roundf(value);
      //   buffer[i] = roundf(value);
        // 调试用
        // printf("i=%d, radian=%.2f, value=%.2f, DAC=%u\n", i, radian, value, buffer[i]);
    }
}









void generate_frequency_points(double *freq_points, double Freq_Start, double Freq_End, double Period_Count,double num_decades,int total_points) {
    double Freq_Start_log = log10(Freq_Start);
    double step = 1 / Period_Count; // 总步长归一化
    for (int i = 0; i < total_points; i++) {
        double exponent = Freq_Start_log - i * step;
        freq_points[i] = pow(10, exponent);
    }
}

