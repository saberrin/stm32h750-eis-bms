/**
* @file sine_wave_generator.h
* @brief 单周期正弦波数据表生成模块
* @author [你的名字/项目组]
* @version V1.0
*/

#ifndef __SINE_WAVE_GENERATOR_H  // 统一宏命名?:ml-citation{ref="2" data="citationList"}
#define __SINE_WAVE_GENERATOR_H

/*---------------------------- 头文件依赖 ----------------------------*/
#include <stdint.h>  // 标准整型类型定义

/*--------------------------- 函数声明 ---------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief 生成单周期正弦波数据表
* @param buffer: 输出数组指针，需预先分配足够内存
* @param length: 单周期点数(建议32/64/128等2的幂)
* @param amplitude: 幅值比例(0.0-1.0对应0%~100% DAC量程)
* @param v_offset: 直流偏置电压比例(0.0-1.0)
* @retval 无
*/
void GenerateSineWave(uint16_t *buffer, float amplitude, float v_offset);  // 补充length参数?:ml-citation{ref="1" data="citationList"}
void generate_frequency_points(double *freq_points, double Freq_Start, double Freq_End, double Period_Count,double num_decades,int total_points);
#ifdef __cplusplus
}
#endif

#endif /* __SINE_WAVE_GENERATOR_H */  
