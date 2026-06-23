#ifndef DIGITAL_LOCK_IN_H
#define DIGITAL_LOCK_IN_H
#include <stdint.h>

// 关键前置声明，解决 identifier undefined
struct FIRFilter;
struct LockInAmplifier;

// 错误码
typedef enum {
    DLIA_SUCCESS = 0,
    DLIA_ERROR_INVALID_PARAM = -1,
    DLIA_ERR_SYNC_FAIL = -2,
    DLIA_ERROR_NYQUIST = -3,
    DLIA_ERROR_MEMORY = -4,
    DLIA_ERROR_NOT_INITIALIZED = -5,
    DLIA_ERROR_SAMPLING_RATE = -6,
    DLIA_ERROR_PROCESSING = -7
} DLIA_Status;

// FIR参数结构体
typedef struct {
    float cutoff_freq;
    int filter_order;
    const char* window_type;
} FIR_Param;

// 锁相配置
typedef struct {
    float ref_frequency;
    float sampling_rate;
    FIR_Param filter;
} LockIn_Config;

// 锁相输出结果
typedef struct {
    float magnitude;
    float phase_rad;
    float phase_deg;
    float rms_noise;
} LockIn_Result;

// ========== 对外上层接口（main/EIS调用不变） ==========
DLIA_Status process_current(float* Current, int Sampling_Count, float Freq, float fs, float* out_magnitude, float* out_phase_rad);
DLIA_Status process_voltage(float* Voltage, int Sampling_Count, float Freq, float fs, float* out_magnitude, float* out_phase_rad);

// 兼容旧单参数初始化
DLIA_Status digital_lock_in_init(float ref_frequency, float sampling_rate);
void digital_lock_in_cleanup(void);
DLIA_Status digital_lock_in_get_config(LockIn_Config* cfg);
void preprocess_signal(float* signal, int sample_count);
const char* digital_lock_in_get_version(void);

// ========== 内部函数（仅.c内部使用，必须前置结构体） ==========
DLIA_Status run_lockin_core(struct LockInAmplifier* inst, float* signal, int sample_cnt, float target_freq, float fs, float* mag_out, float* phs_out);
DLIA_Status multifrequency_process(struct LockInAmplifier* inst, const float* sig, int sample_cnt, float* mag_out, float* phs_out);
void digital_lock_in_deinit(struct LockInAmplifier* inst);
DLIA_Status digital_lock_in_init_inst(struct LockInAmplifier* inst, float ref_freq, float fs);

#endif















///**
// * digital_lock_in.h
// * 数字锁相放大器头文件 - 修正版
// * 解决类型重定义和枚举混用问题
// */

//#ifndef DIGITAL_LOCK_IN_H
//#define DIGITAL_LOCK_IN_H

//#include <stdint.h>
//#include <math.h>

///* 标准数学常量定义 */
//#ifndef M_PI
//#define M_PI 3.14159265358979323846f
//#endif

///* 错误代码定义 */
//typedef enum {
//    DLIA_SUCCESS = 0,
//    DLIA_ERROR_INVALID_PARAM = -1,
//    DLIA_ERROR_NYQUIST = -2,
//    DLIA_ERROR_SAMPLING_RATE = -3,
//    DLIA_ERROR_MEMORY = -4,
//    DLIA_ERROR_FILTER_DESIGN = -5,
//    DLIA_ERROR_NOT_INITIALIZED = -6,
//    DLIA_ERROR_PROCESSING = -7
//} DLIA_Status;

///* 滤波器参数结构 */
//typedef struct {
//    float cutoff_freq;      // 截止频率 (Hz)
//    int filter_order;       // 滤波器阶数
//    const char* window_type;// 窗函数类型描述
//} Filter_Params;

///* 锁相放大器配置 */
//typedef struct {
//    float ref_frequency;    // 参考频率 (Hz)
//    float sampling_rate;    // 采样率 (Hz)
//    Filter_Params filter;   // 滤波器参数
//} LockIn_Config;

///* 处理结果 */
//typedef struct {
//    float magnitude;        // 信号幅度
//    float phase_rad;        // 相位 (弧度)
//    float phase_deg;        // 相位 (度)
//    float rms_noise;        // 噪声水平估计
//} LockIn_Result;


///* ====== 外部调用接口！！ ====== */
//DLIA_Status process_current(float* Current, int Sampling_Count, float Freq, float fs,
//                            float* out_magnitude, float* out_phase_rad);

//DLIA_Status process_voltage(float* Voltage, int Sampling_Count, float Freq, float fs,
//                            float* out_magnitude, float* out_phase_rad);

//DLIA_Status run_lockin_once(float* signal, int sampling_count, float Freq, float fs,
//                            float* out_magnitude, float* out_phase_rad);


///* ====== 内部API函数声明 ====== */

//void print_result(const char* mode, float magnitude, float phase_rad, float phase_deg);

///**
// * @brief 初始化数字锁相放大器
// * @param ref_frequency 参考信号频率 (Hz)
// * @param sampling_rate 采样率 (Hz)
// * @return 成功返回DLIA_SUCCESS，失败返回错误代码
// */
//DLIA_Status digital_lock_in_init(float ref_frequency, float sampling_rate);

///**
// * @brief 多频段锁相放大处理
// * @param signal 输入信号数组
// * @param sample_count 采样点数
// * @param magnitude 幅度输出指针
// * @param phase_rad 相位输出指针(弧度)
// * @return 成功返回DLIA_SUCCESS，失败返回错误代码
// */
//DLIA_Status multifrequency_lock_in_process(const float* signal, int sample_count, 
//                                         float* magnitude, float* phase_rad);

///**
// * @brief 高精度模式处理
// * @param signal 输入信号数组
// * @param sample_count 采样点数
// * @param result 处理结果结构体指针
// * @return 成功返回DLIA_SUCCESS，失败返回错误代码
// */
//DLIA_Status high_precision_lock_in(const float* signal, int sample_count, 
//                                  LockIn_Result* result);

///**
// * @brief 重新配置锁相放大器参数
// * @param new_ref_frequency 新的参考频率
// * @param new_sampling_rate 新的采样率
// * @return 成功返回DLIA_SUCCESS，失败返回错误代码
// */
//DLIA_Status digital_lock_in_reconfigure(float new_ref_frequency, float new_sampling_rate);

///**
// * @brief 获取当前配置信息
// * @param config 配置信息结构体指针
// * @return 成功返回DLIA_SUCCESS，失败返回错误代码
// */
//DLIA_Status digital_lock_in_get_config(LockIn_Config* config);

///**
// * @brief 信号预处理（去直流分量）
// * @param signal 输入/输出信号数组
// * @param sample_count 采样点数
// */
//void preprocess_signal(float* signal, int sample_count);

///**
// * @brief 估算信号频率
// * @param signal 输入信号数组
// * @param sample_count 采样点数
// * @param sampling_rate 采样率
// * @return 估计的信号频率 (Hz)
// */
//float estimate_signal_frequency(const float* signal, int sample_count, float sampling_rate);

///**
// * @brief 清理锁相放大器资源
// */
//void digital_lock_in_cleanup(void);

///**
// * @brief 获取库版本信息
// * @return 版本字符串
// */
//const char* digital_lock_in_get_version(void);

//#endif /* DIGITAL_LOCK_IN_H */

//////////////////////////////////////////////////////////////上面是V1.0/////////////////////////////////

///**
// * digital_lock_in.h
// * 数字锁相放大器头文件 - 最终修正版
// */

//#ifndef DIGITAL_LOCK_IN_H
//#define DIGITAL_LOCK_IN_H

//#include <stdint.h>
//#include <math.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>

///* 标准数学常量定义 - 兼容性处理 */
//#ifndef M_PI
//#define M_PI 3.14159265358979323846f
//#endif

///* 错误代码定义 */
//typedef enum {
//    DLIA_SUCCESS = 0,
//    DLIA_ERROR_INVALID_PARAM = -1,
//    DLIA_ERROR_NYQUIST = -2,
//    DLIA_ERROR_SAMPLING_RATE = -3,
//    DLIA_ERROR_MEMORY = -4,
//    DLIA_ERROR_FILTER_DESIGN = -5,
//    DLIA_ERROR_NOT_INITIALIZED = -6,
//    DLIA_ERROR_PROCESSING = -7
//} DLIA_Status;

///* 滤波器参数结构 */
//typedef struct {
//    float cutoff_freq;      // 截止频率 (Hz)
//    int filter_order;       // 滤波器阶数
//    const char* window_type;// 窗函数类型描述
//} Filter_Params;

///* 信号质量评估结构 */
//typedef struct {
//    float snr_db;           // 信噪比 (dB)
//    float thd_percent;      // 总谐波失真 (%)
//    float noise_rms;        // 噪声RMS值
//    float signal_rms;       // 信号RMS值
//    float dc_offset;        // 直流偏移
//    int is_valid;           // 信号质量是否可接受
//} Signal_Quality;

///* 锁相放大器配置 */
//typedef struct {
//    float ref_frequency;    // 参考频率 (Hz)
//    float sampling_rate;    // 采样率 (Hz)
//    Filter_Params filter;   // 核心滤波器参数
//    Filter_Params pre_filter; // 预处理FIR滤波器参数
//    int enable_quality_eval; // 是否启用信号质量评估
//    float snr_threshold;    // SNR阈值(dB)，低于此值报警
//} LockIn_Config;

///* 处理结果 */
//typedef struct {
//    float magnitude;        // 信号幅度
//    float phase_rad;        // 相位 (弧度)
//    float phase_deg;        // 相位 (度)
//    float rms_noise;        // 噪声水平估计
//    Signal_Quality quality; // 信号质量评估结果
//} LockIn_Result;

///* ====== 外部调用接口（完全保持不变） ====== */
//DLIA_Status process_current(float* Current, int Sampling_Count, float Freq, float fs,
//                            float* out_magnitude, float* out_phase_rad);

//DLIA_Status process_voltage(float* Voltage, int Sampling_Count, float Freq, float fs,
//                            float* out_magnitude, float* out_phase_rad);

//DLIA_Status run_lockin_once(float* signal, int sampling_count, float Freq, float fs,
//                            float* out_magnitude, float* out_phase_rad);

///* ====== 现有API函数声明 ====== */
//void print_result(const char* mode, float magnitude, float phase_rad, float phase_deg);
//DLIA_Status digital_lock_in_init(float ref_frequency, float sampling_rate);
//DLIA_Status multifrequency_lock_in_process(const float* signal, int sample_count, 
//                                         float* magnitude, float* phase_rad);
//DLIA_Status high_precision_lock_in(const float* signal, int sample_count, 
//                                  LockIn_Result* result);
//DLIA_Status digital_lock_in_reconfigure(float new_ref_frequency, float new_sampling_rate);
//DLIA_Status digital_lock_in_get_config(LockIn_Config* config);
//void preprocess_signal(float* signal, int sample_count);
//float estimate_signal_frequency(const float* signal, int sample_count, float sampling_rate);
//void digital_lock_in_cleanup(void);
//const char* digital_lock_in_get_version(void);

///* ====== 新增API函数声明 ====== */
//DLIA_Status digital_lock_in_enable_quality_eval(int enable);
//DLIA_Status digital_lock_in_set_prefilter(float cutoff_freq, int order);
//DLIA_Status digital_lock_in_get_quality(Signal_Quality* quality);

//#endif /* DIGITAL_LOCK_IN_H */
