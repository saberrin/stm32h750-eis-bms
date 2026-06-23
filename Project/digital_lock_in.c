/**
 * digital_lock_in.c 编译修复完整版
 * 修复本次4个报错 + 之前全部警告错误
 */
#include "digital_lock_in.h"
#include "watchdog.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#define PI 3.14159265358979323846f
#define MAX_FIR_ORDER 400
#define MAG_NOISE_FLOOR 1e-6f

// 全局静态FIR系数缓存，消除堆malloc
static float g_fir_coef_buf[MAX_FIR_ORDER + 1];

// 仅.c内部完整定义，头文件仅前置声明
typedef struct {
    int filter_order;
    float delay_line[MAX_FIR_ORDER + 1];
    int buffer_index;
    uint8_t is_init;
} FIRFilter;

// 独立锁相实例（电流、电压分离，互不干扰）
typedef struct LockInAmplifier {
    FIRFilter i_filter;
    FIRFilter q_filter;
    LockIn_Config config;
    uint8_t is_initialized;
} LockInAmplifier;

static LockInAmplifier g_lock_i = {0};  // 电流通道
static LockInAmplifier g_lock_v = {0};  // 电压通道
static const char* LIB_VERSION = "1.1.1-FIX";

// ================= 内部函数前置声明 =================
static DLIA_Status fir_filter_init(FIRFilter* filter, int order);
static float fir_filter_process(FIRFilter* filter, float input);
static void fir_filter_deinit(FIRFilter* filter);
static DLIA_Status design_fir_filter(float cutoff_freq, float sampling_rate, int order, float* coef_buf);
static void auto_adjust_param(float signal_freq, float fs, int* out_order, float* out_cut);

// ================= 对外上层API（完全兼容原有调用） =================
DLIA_Status process_current(float* Current, int Sampling_Count, float Freq, float fs, float* out_magnitude, float* out_phase_rad)
{
    return run_lockin_core(&g_lock_i, Current, Sampling_Count, Freq, fs, out_magnitude, out_phase_rad);
}

DLIA_Status process_voltage(float* Voltage, int Sampling_Count, float Freq, float fs, float* out_magnitude, float* out_phase_rad)
{
    return run_lockin_core(&g_lock_v, Voltage, Sampling_Count, Freq, fs, out_magnitude, out_phase_rad);
}

// 核心处理函数
DLIA_Status run_lockin_core(LockInAmplifier* inst, float* signal, int sample_cnt, float target_freq, float fs, float* mag_out, float* phs_out)
{
    if (!inst || !signal || !mag_out || !phs_out)
        return DLIA_ERROR_INVALID_PARAM;

    printf("数字锁相放大器程序正在运行 - 版本 %s\r\n", LIB_VERSION);
    printf("初始化锁相放大器...\r\n");

    DLIA_Status st = digital_lock_in_init_inst(inst, target_freq, fs);
    if (st != DLIA_SUCCESS) {
        printf("错误: 初始化失败 (错误码: %d)\r\n", st);
        return st;
    }

    LockIn_Config cfg;
    digital_lock_in_get_config(&cfg);
    printf("参考频率: %.1f Hz 采样率: %.1f Hz FIR阶数:%d\r\n",
           cfg.ref_frequency, cfg.sampling_rate, cfg.filter.filter_order);

    preprocess_signal(signal, sample_cnt);

    float mag = 0.0f, ph = 0.0f;
    st = multifrequency_process(inst, signal, sample_cnt, &mag, &ph);

    if (mag < MAG_NOISE_FLOOR) {
        st = DLIA_ERR_SYNC_FAIL;
        printf("处理失败 (错误码: %d 信号幅值过低，同步丢失)\r\n", st);
        *mag_out = 0.0f;
        *phs_out = 0.0f;
        digital_lock_in_deinit(inst);
        return st;
    }

    if (st == DLIA_SUCCESS) {
        float deg = ph * 180.0f / PI;
        printf("处理结果: magnitude=%.6f, phase=%.6f rad\r\n", mag, ph);
        *mag_out = mag;
        *phs_out = ph;
    } else {
        printf("多频处理失败 (错误码: %d)\r\n", st);
    }

    digital_lock_in_deinit(inst);
    printf("清理完成\r\n\n");
    return st;
}

static DLIA_Status fir_filter_init(FIRFilter* filter, int order)
{
    if (!filter || order <= 0 || order > MAX_FIR_ORDER)
        return DLIA_ERROR_INVALID_PARAM;

    filter->filter_order = order;
    filter->buffer_index = 0;
    memset(filter->delay_line, 0, sizeof(filter->delay_line));
    filter->is_init = 1;
    return DLIA_SUCCESS;
}

static float fir_filter_process(FIRFilter* filter, float input)
{
    if (!filter || !filter->is_init) return 0.0f;
    filter->delay_line[filter->buffer_index] = input;

    float sum = 0.0f;
    int idx = filter->buffer_index;
    for (int i = 0; i <= filter->filter_order; i++) {
        sum += g_fir_coef_buf[i] * filter->delay_line[idx];
        idx = (idx == 0) ? filter->filter_order : idx - 1;
    }
    filter->buffer_index = (filter->buffer_index + 1) % (filter->filter_order + 1);
    return sum;
}

static void fir_filter_deinit(FIRFilter* filter)
{
    if (!filter) return;
    filter->is_init = 0;
    filter->filter_order = 0;
    memset(filter->delay_line, 0, sizeof(filter->delay_line));
}

static DLIA_Status design_fir_filter(float cutoff_freq, float sampling_rate, int order, float* coef_buf)
{
    if (!coef_buf || order <= 0)
        return DLIA_ERROR_INVALID_PARAM;
    int n = order + 1;
    float wc = 2.0f * PI * cutoff_freq / sampling_rate;
    float mid = (float)(n - 1) / 2.0f;

    for (int i = 0; i < n; i++) {
        float offset = (float)i - mid;
        float h;
        if (fabsf(offset) < 1e-6f)
            h = wc / PI;
        else
            h = sinf(wc * offset) / (PI * offset);
        float win = 0.54f - 0.46f * cosf(2.0f * PI * (float)i / (float)(n - 1));
        coef_buf[i] = h * win;
    }
    return DLIA_SUCCESS;
}

// 修复：变量sig → signal_freq
static void auto_adjust_param(float signal_freq, float fs, int* out_order, float* out_cut)
{
    float nyq = fs / 2.0f;
    float ratio = nyq / signal_freq;
    if (ratio >= 2.0f) {
        if (signal_freq > 1000.0f) {
            *out_cut = 100.0f;
            *out_order = 100;
        } else if (signal_freq > 100.0f) {
            *out_cut = 10.0f;
            *out_order = 200;
        } else if (signal_freq > 10.0f) {
            *out_cut = 1.0f;
            *out_order = 200;
        } else if (signal_freq > 1.0f) {
            *out_cut = 0.1f;
            *out_order = 400;
        } else {
            *out_cut = 0.01f;
            *out_order = 400;
        }
    } else {
        *out_cut = signal_freq * 0.5f;
        *out_order = 64;
    }
    if (*out_order > MAX_FIR_ORDER)
        *out_order = MAX_FIR_ORDER;
}

// 单实例初始化
DLIA_Status digital_lock_in_init_inst(LockInAmplifier* inst, float ref_freq, float fs)
{
    if (!inst || ref_freq <= 0 || fs <= 0)
        return DLIA_ERROR_INVALID_PARAM;

    int f_order;
    float f_cut;
    auto_adjust_param(ref_freq, fs, &f_order, &f_cut);

    if (inst->is_initialized)
        digital_lock_in_deinit(inst);

    DLIA_Status st = fir_filter_init(&inst->i_filter, f_order);
    if (st != DLIA_SUCCESS) return st;
    st = fir_filter_init(&inst->q_filter, f_order);
    if (st != DLIA_SUCCESS) {
        fir_filter_deinit(&inst->i_filter);
        return st;
    }

    design_fir_filter(f_cut, fs, f_order, g_fir_coef_buf);
    inst->config.ref_frequency = ref_freq;
    inst->config.sampling_rate = fs;
    inst->config.filter.cutoff_freq = f_cut;
    inst->config.filter.filter_order = f_order;
    inst->config.filter.window_type = "Hamming";
    inst->is_initialized = 1;
    return DLIA_SUCCESS;
}

// 兼容旧全局初始化（修复fs未定义）
DLIA_Status digital_lock_in_init(float ref_frequency, float sampling_rate)
{
    return digital_lock_in_init_inst(&g_lock_v, ref_frequency, sampling_rate);
}

DLIA_Status multifrequency_process(LockInAmplifier* inst, const float* sig, int sample_cnt, float* mag_out, float* phs_out)
{
    if (!inst || !sig || !mag_out || !phs_out || sample_cnt <= 0 || !inst->is_initialized)
        return DLIA_ERROR_INVALID_PARAM; // 修复：DLIA_ERROR_INVALID_PARAM

    int order = inst->i_filter.filter_order;
    int skip_transient = order / 3;
    float omega = 2.0f * PI * inst->config.ref_frequency;
    float i_sum = 0.0f, q_sum = 0.0f;
    int valid = 0;

    for (int i = 0; i < sample_cnt; i++) {
        Watchdog_Refresh();
        float t = (float)i / inst->config.sampling_rate;
        float ref_i = cosf(omega * t);
        float ref_q = sinf(omega * t);
        float mix_i = sig[i] * ref_i;
        float mix_q = sig[i] * ref_q;
        float filt_i = fir_filter_process(&inst->i_filter, mix_i);
        float filt_q = fir_filter_process(&inst->q_filter, mix_q);

        if (i > skip_transient) {
            i_sum += filt_i;
            q_sum += filt_q;
            valid++;
        }
    }

    if (valid <= 0) {
        *mag_out = 0.0f;
        *phs_out = 0.0f;
        return DLIA_ERROR_PROCESSING;
    }
    i_sum /= (float)valid;
    q_sum /= (float)valid;

    float amp = 2.0f * sqrtf(i_sum * i_sum + q_sum * q_sum);
    float phase = atan2f(q_sum, i_sum);
    *mag_out = amp;
    *phs_out = phase;
    return DLIA_SUCCESS;
}

void digital_lock_in_deinit(LockInAmplifier* inst)
{
    if (!inst || !inst->is_initialized) return;
    fir_filter_deinit(&inst->i_filter);
    fir_filter_deinit(&inst->q_filter);
    inst->is_initialized = 0;
}

DLIA_Status digital_lock_in_get_config(LockIn_Config* cfg)
{
    LockInAmplifier* inst = &g_lock_v;
    if (!inst || !inst->is_initialized || !cfg)
        return DLIA_ERROR_NOT_INITIALIZED;
    *cfg = inst->config;
    return DLIA_SUCCESS;
}

void digital_lock_in_cleanup(void)
{
    digital_lock_in_deinit(&g_lock_i);
    digital_lock_in_deinit(&g_lock_v);
}

void preprocess_signal(float* signal, int sample_count)
{
    if (!signal || sample_count <= 0) return;
    float dc = 0.0f;
    for (int i = 0; i < sample_count; i++) dc += signal[i];
    dc /= (float)sample_count;
    for (int i = 0; i < sample_count; i++) signal[i] -= dc;
}

const char* digital_lock_in_get_version(void)
{
    return LIB_VERSION;
}





























/////////////////////////////////////////////////////////////////////////////
///**
// * digital_lock_in.c
// * 数字锁相放大器实现 - 修正版
// */

//#include "digital_lock_in.h"
//#include <stdlib.h>
//#include <string.h>
//#include <stdio.h>
//#include "watchdog.h"
///* 内部结构体定义 */
//typedef struct {
//    float* coefficients;    // 滤波器系数
//    float* delay_line;       // 延迟线缓冲区
//    int filter_order;        // 滤波器阶数
//    int buffer_index;        // 缓冲区当前索引
//    int is_initialized;      // 初始化标志
//} FIRFilter;

//typedef struct {
//    FIRFilter i_filter;      // I路滤波器
//    FIRFilter q_filter;      // Q路滤波器
//    LockIn_Config config;    // 配置参数
//    int is_initialized;      // 初始化标志
//} LockInAmplifier;

//static LockInAmplifier g_lock_in_amp = {0};
//static const char* LIB_VERSION = "1.1.0";

///* 内部函数声明 */
//static DLIA_Status fir_filter_init(FIRFilter* filter, int order);
//static float fir_filter_process(FIRFilter* filter, float input);
//static void fir_filter_free(FIRFilter* filter);
//static DLIA_Status design_fir_filter(float cutoff_freq, float sampling_rate, 
//                           int order, float* coefficients);
//static DLIA_Status select_filter_parameters(float signal_freq, int* order, 
//                                  float* cutoff_freq);

///* 两个便捷 wrapper（可直接调用以保持语义清晰） 

//外部可直接调用这两个接口实现电压电流的相关检测算法！！！

//*/
//DLIA_Status process_current(float* Current, int Sampling_Count, float Freq, float fs,
//                            float* out_magnitude, float* out_phase_rad)
//{
//    return run_lockin_once(Current, Sampling_Count, Freq, fs, out_magnitude, out_phase_rad);
//}

//DLIA_Status process_voltage(float* Voltage, int Sampling_Count, float Freq, float fs,
//                            float* out_magnitude, float* out_phase_rad)
//{
//    return run_lockin_once(Voltage, Sampling_Count, Freq, fs, out_magnitude, out_phase_rad);
//}

///* 通用一次性锁相放大处理函数
// * 参数:
// *   signal         - 指向采样数据（float数组）
// *   sampling_count - 采样点数
// *   Freq           - 参考频率（Hz）
// *   fs             - 采样率（Hz）
// *   out_magnitude  - 输出幅值指针
// *   out_phase_rad  - 输出相位（弧度）指针
// * 返回:
// *   DLIA_Status（库的返回码，DLIA_SUCCESS 表示成功）
// *
// * 说明: 函数不会 free(signal)，也不会修改 signal 的所有权。
// */
//DLIA_Status run_lockin_once(float* signal, int sampling_count, float Freq, float fs,
//                            float* out_magnitude, float* out_phase_rad)
//{
//    printf("数字锁相放大器程序正在运行 - 版本 %s\r\n", digital_lock_in_get_version());

//    /* 初始化 */
//    printf("初始化锁相放大器...\r\n");
//    DLIA_Status status = digital_lock_in_init(Freq, fs);
//    if (status != DLIA_SUCCESS) {
//        printf("错误: 初始化失败 (错误码: %d)\r\n", status);
//        return status;
//    }
//    printf("锁相放大器初始化成功！\r\n\n");

//    /* 显示当前配置 */
//    LockIn_Config config;
//    if (digital_lock_in_get_config(&config) == DLIA_SUCCESS) {
//        printf("当前配置:\r\n");
//        printf("  参考频率: %.1f Hz\r\n", config.ref_frequency);
//        printf("  采样率: %.1f Hz\r\n", config.sampling_rate);
//        printf("  滤波器截止频率: %.3f Hz\r\n", config.filter.cutoff_freq);
//        printf("  滤波器阶数: %d\r\n", config.filter.filter_order);
//        printf("  窗口类型: %s\r\n\n", config.filter.window_type);
//    }

//    /* 预处理 */
//    preprocess_signal(signal, sampling_count);

//    /* 多频/幅相提取 */
//    printf("开始幅相处理...\r\n");
//    float magnitude = 0.0f, phase_rad = 0.0f;
//    status = multifrequency_lock_in_process(signal, sampling_count, &magnitude, &phase_rad);
//    if (status == DLIA_SUCCESS) {
//        float phase_deg = phase_rad * (180.0f / M_PI);
//        print_result("多频锁相放大", magnitude, phase_rad, phase_deg);
//        if (out_magnitude) *out_magnitude = magnitude;
//        if (out_phase_rad) *out_phase_rad = phase_rad;
//    } else {
//        printf("多频处理失败 (错误码: %d)\r\n", status);
//    }

//    /* 清理 */
//    digital_lock_in_cleanup();
//    printf("清理完成，返回。\r\n\n");

//    return status;
//}

///**
// * @brief 打印处理结果
// */
//void print_result(const char* mode, float magnitude, float phase_rad, float phase_deg) {
//    printf("=== %s 模式结果 ===\r\n", mode);
//    printf("信号幅度: %.6f V\r\n", magnitude);
//    printf("信号相位: %.3f rad (%.2f°)\r\n", phase_rad, phase_deg);
//    printf("\r\n");
//}

///**
// * @brief 初始化FIR滤波器
// */
//static DLIA_Status fir_filter_init(FIRFilter* filter, int order) {
//    if (!filter || order <= 0) {
//        return DLIA_ERROR_INVALID_PARAM;
//    }
//    
//    filter->filter_order = order;
//    filter->buffer_index = 0;
//    
//    /* 分配系数内存 */
//    filter->coefficients = (float*)malloc((order + 1) * sizeof(float));
//    if (!filter->coefficients) {
//        return DLIA_ERROR_MEMORY;
//    }
//    
//    /* 分配延迟线内存并清零 */
//    filter->delay_line = (float*)calloc(order + 1, sizeof(float));
//    if (!filter->delay_line) {
//        free(filter->coefficients);
//        return DLIA_ERROR_MEMORY;
//    }
//    
//    filter->is_initialized = 1;
//    return DLIA_SUCCESS;
//}

///**
// * @brief FIR滤波器处理
// */
//static float fir_filter_process(FIRFilter* filter, float input) {
//    if (!filter || !filter->is_initialized) {
//        return 0.0f;
//    }
//    
//    /* 更新延迟线 */
//    filter->delay_line[filter->buffer_index] = input;
//    
//    /* 计算卷积 */
//    float output = 0.0f;
//    int index = filter->buffer_index;
//    
//    for (int i = 0; i <= filter->filter_order; i++) {
//        output += filter->coefficients[i] * filter->delay_line[index];
//        index = (index == 0) ? filter->filter_order : index - 1;
//    }
//    
//    /* 更新缓冲区索引 */
//    filter->buffer_index = (filter->buffer_index + 1) % (filter->filter_order + 1);
//    
//    return output;
//}

///**
// * @brief 释放FIR滤波器资源
// */
//static void fir_filter_free(FIRFilter* filter) {
//    if (!filter) return;
//    
//    if (filter->coefficients) {
//        free(filter->coefficients);
//        filter->coefficients = NULL;
//    }
//    if (filter->delay_line) {
//        free(filter->delay_line);
//        filter->delay_line = NULL;
//    }
//    
//    filter->filter_order = 0;
//    filter->buffer_index = 0;
//    filter->is_initialized = 0;
//}

///**
// * @brief 设计FIR低通滤波器系数（汉明窗）
// */
//static DLIA_Status design_fir_filter(float cutoff_freq, float sampling_rate, 
//                           int order, float* coefficients) {
//    if (!coefficients || order <= 0 || cutoff_freq <= 0 || sampling_rate <= 0) {
//        return DLIA_ERROR_INVALID_PARAM;
//    }
//    
//    int n = order + 1;
//    float omega_c = 2.0f * M_PI * cutoff_freq / sampling_rate;
//    float center = (n - 1) / 2.0f;
//    
//    for (int i = 0; i < n; i++) {
//        /* 计算理想低通滤波器系数 */
//        if (fabsf(i - center) < 1e-6f) {
//            coefficients[i] = omega_c / M_PI;
//        } else {
//            float offset = i - center;
//            coefficients[i] = sinf(omega_c * offset) / (M_PI * offset);
//        }
//        
//        /* 应用汉明窗 */
//        coefficients[i] *= (0.54f - 0.46f * cosf(2.0f * M_PI * i / (n - 1)));
//    }
//    
//    return DLIA_SUCCESS;
//}

///**
// * @brief 根据信号频率选择滤波器参数[1,4](@ref)
// */
//static DLIA_Status select_filter_parameters(float signal_freq, int* order, float* cutoff_freq) {
//    if (signal_freq > 1000.0f) {
//        *cutoff_freq = 100.0f;      // >100 Hz: 10Hz截止
//        *order = 100;              // 200阶滤波器
//    } else if (signal_freq > 100.0f) {
//        *cutoff_freq = 10.0f;       // 10-100 Hz: 1Hz截止
//        *order = 200;              // 400阶滤波器
//    } else if (signal_freq > 10.0f) {
//        *cutoff_freq = 1.0f;       // 10-100 Hz: 1Hz截止
//        *order = 400;              // 400阶滤波器
//    } else if (signal_freq > 1.0f) {
//        *cutoff_freq = 0.1f;       // 1-10 Hz: 0.1Hz截止
//        *order = 800;              // 800阶滤波器
//    } else {
//        *cutoff_freq = 0.01f;      // <1 Hz: 0.01Hz截止
//        *order = 1200;             // 1600阶滤波器
//    }
//    
//    return DLIA_SUCCESS;
//}

///* ====== 公共API实现 ====== */



//DLIA_Status digital_lock_in_init(float ref_frequency, float sampling_rate) {
//    /* 参数校验 */
//    if (ref_frequency <= 0 || sampling_rate <= 0) {
//        return DLIA_ERROR_INVALID_PARAM;
//    }
//    
//    /* 检查奈奎斯特准则 */
//    if (sampling_rate <= 2.0f * ref_frequency) {
//        return DLIA_ERROR_NYQUIST;
//    }
//    
//    /* 清理现有状态 */
//    digital_lock_in_cleanup();
//    
//    /* 选择滤波器参数 */
//    int filter_order;
//    float cutoff_freq;
//    
//    DLIA_Status result = select_filter_parameters(ref_frequency, &filter_order, &cutoff_freq);
//    if (result != DLIA_SUCCESS) {
//        return result;
//    }
//    
//    /* 初始化I路滤波器 */
//    result = fir_filter_init(&g_lock_in_amp.i_filter, filter_order);
//    if (result != DLIA_SUCCESS) {
//        return result;
//    }
//    
//    /* 初始化Q路滤波器 */
//    result = fir_filter_init(&g_lock_in_amp.q_filter, filter_order);
//    if (result != DLIA_SUCCESS) {
//        fir_filter_free(&g_lock_in_amp.i_filter);
//        return result;
//    }
//    
//    /* 设计I路滤波器系数 */
//    result = design_fir_filter(cutoff_freq, sampling_rate, filter_order, 
//                              g_lock_in_amp.i_filter.coefficients);
//    if (result != DLIA_SUCCESS) {
//        fir_filter_free(&g_lock_in_amp.i_filter);
//        fir_filter_free(&g_lock_in_amp.q_filter);
//        return result;
//    }
//    
//    /* Q路使用相同系数 */
//    memcpy(g_lock_in_amp.q_filter.coefficients, g_lock_in_amp.i_filter.coefficients,
//           (filter_order + 1) * sizeof(float));
//    
//    /* 保存配置 */
//    g_lock_in_amp.config.ref_frequency = ref_frequency;
//    g_lock_in_amp.config.sampling_rate = sampling_rate;
//    g_lock_in_amp.config.filter.cutoff_freq = cutoff_freq;
//    g_lock_in_amp.config.filter.filter_order = filter_order;
//    g_lock_in_amp.config.filter.window_type = "Hamming";
//    
//    g_lock_in_amp.is_initialized = 1;
//    
//    return DLIA_SUCCESS;
//}









//DLIA_Status multifrequency_lock_in_process(const float* signal, int sample_count, 
//                                          float* magnitude, float* phase_rad) {
//    /* 参数检查 */
//    if (!signal || !magnitude || !phase_rad || sample_count <= 0) {
//        return DLIA_ERROR_INVALID_PARAM;
//    }
//    
//    if (!g_lock_in_amp.is_initialized) {
//        return DLIA_ERROR_NOT_INITIALIZED;
//    }
//    
//    /* 确保有足够样本进行有效滤波 */
//    int min_samples = g_lock_in_amp.i_filter.filter_order * 2;
//		
//		printf("sample_count: %.2d Hz\n", sample_count);
//		printf("min_samples: %.2d Hz\n", min_samples);
//    if (sample_count < min_samples) {
//			return DLIA_ERROR_SAMPLING_RATE;
//    }
//    
//    float omega = 2.0f * M_PI * g_lock_in_amp.config.ref_frequency;
//    float i_total = 0.0f, q_total = 0.0f;
//    int valid_samples = 0;
//    
//    /* 锁相放大核心算法[5,7](@ref) */
//    for (int i = 0; i < sample_count; i++) {
//				Watchdog_Refresh();
//        float t = (float)i / g_lock_in_amp.config.sampling_rate;
//        
//        /* 生成正交参考信号 */
//        float ref_i = cosf(omega * t);  // 同相分量
//        float ref_q = sinf(omega * t);  // 正交分量
//        
//        /* 信号与参考信号相乘（相敏检波） */
//        float mixed_i = signal[i] * ref_i;
//        float mixed_q = signal[i] * ref_q;
//        
//        /* FIR滤波 */
//        float filtered_i = fir_filter_process(&g_lock_in_amp.i_filter, mixed_i);
//        float filtered_q = fir_filter_process(&g_lock_in_amp.q_filter, mixed_q);
//        
//        /* 累加稳定后的结果（跳过初始瞬态） */
//        if (i > g_lock_in_amp.i_filter.filter_order) {
//            i_total += filtered_i;
//            q_total += filtered_q;
//            valid_samples++;
//        }
//    }
//    
//    if (valid_samples > 0) {
//        i_total /= valid_samples;
//        q_total /= valid_samples;
//        
//        /* 计算幅度和相位[3](@ref) */
//        *magnitude = 2.0f * sqrtf(i_total * i_total + q_total * q_total);
//        *phase_rad = atan2f(q_total, i_total);
//        
//        return DLIA_SUCCESS;
//    }
//    
//    return DLIA_ERROR_PROCESSING;
//}

//DLIA_Status high_precision_lock_in(const float* signal, int sample_count, LockIn_Result* result) {
//    float magnitude, phase_rad;
//    DLIA_Status status = multifrequency_lock_in_process(signal, sample_count, &magnitude, &phase_rad);
//    
//    if (status == DLIA_SUCCESS) {
//        result->magnitude = magnitude;
//        result->phase_rad = phase_rad;
//        result->phase_deg = phase_rad * (180.0f / M_PI);
//        
//        /* 简单的噪声估计 */
//        float noise_sum = 0.0f;
//        for (int i = 0; i < sample_count; i++) {
//            float t = (float)i / g_lock_in_amp.config.sampling_rate;
//            float expected = magnitude * 0.5f * sinf(2.0f * M_PI * g_lock_in_amp.config.ref_frequency * t + phase_rad);
//            noise_sum += fabsf(signal[i] - expected);
//        }
//        result->rms_noise = noise_sum / sample_count;
//    }
//    
//    return status;
//}

//DLIA_Status digital_lock_in_reconfigure(float new_ref_frequency, float new_sampling_rate) {
//    digital_lock_in_cleanup();
//    return digital_lock_in_init(new_ref_frequency, new_sampling_rate);
//}

//DLIA_Status digital_lock_in_get_config(LockIn_Config* config) {
//    if (!config || !g_lock_in_amp.is_initialized) {
//        return DLIA_ERROR_NOT_INITIALIZED;
//    }
//    
//    *config = g_lock_in_amp.config;
//    return DLIA_SUCCESS;
//}

//void preprocess_signal(float* signal, int sample_count) {
//    if (!signal || sample_count <= 0) return;
//    
//    /* 计算直流偏移 */
//    float dc_offset = 0.0f;
//    for (int i = 0; i < sample_count; i++) {
//        dc_offset += signal[i];
//    }
//    dc_offset /= sample_count;
//    
//    /* 去除直流分量 */
//    for (int i = 0; i < sample_count; i++) {
//        signal[i] -= dc_offset;
//    }
//}

//float estimate_signal_frequency(const float* signal, int sample_count, float sampling_rate) {
//    /* 简单的过零检测法估算频率[3](@ref) */
//    int zero_crossings = 0;
//    for (int i = 1; i < sample_count; i++) {
//        if (signal[i-1] * signal[i] < 0) {
//            zero_crossings++;
//        }
//    }
//    return (zero_crossings * sampling_rate) / (2.0f * sample_count);
//}

//void digital_lock_in_cleanup(void) {
//    if (g_lock_in_amp.is_initialized) {
//        fir_filter_free(&g_lock_in_amp.i_filter);
//        fir_filter_free(&g_lock_in_amp.q_filter);
//        g_lock_in_amp.is_initialized = 0;
//    }
//}

//const char* digital_lock_in_get_version(void) {
//    return LIB_VERSION;
//}

//////////////////////////////////////////////////////////////////////////////////以上是版本1.0//////////////////////////////////////////////////////////////////////////////




///**
// * digital_lock_in.c
// * 修正版：解决采样点不足直接返回幅值0 + 全部Keil编译错误修复
// */

//#include "digital_lock_in.h"
//#include <stdlib.h>
//#include <string.h>
//#include <stdio.h>
//#include <math.h>
//#include "watchdog.h"

///* 内部结构体定义 */
//typedef struct {
//    float* coefficients;    // 滤波器系数
//    float* delay_line;       // 延迟线缓冲区
//    int filter_order;        // 滤波器阶数
//    int buffer_index;        // 缓冲区当前索引
//    int is_initialized;      // 初始化标志
//} FIRFilter;

//typedef struct {
//    FIRFilter i_filter;      // I路滤波器
//    FIRFilter q_filter;      // Q路滤波器
//    LockIn_Config config;    // 配置参数
//    int is_initialized;      // 初始化标志
//} LockInAmplifier;

//static LockInAmplifier g_lock_in_amp = {0};
//static const char* LIB_VERSION = "1.1.1";

///* 内部函数声明（修复漏返回值错误） */
//static DLIA_Status fir_filter_init(FIRFilter* filter, int order);
//static float fir_filter_process(FIRFilter* filter, float input);
//static void fir_filter_free(FIRFilter* filter);
//static DLIA_Status design_fir_filter(float cutoff_freq, float sampling_rate, 
//                           int order, float* coefficients);
//static DLIA_Status select_filter_parameters(float signal_freq, int* order, float* cutoff_freq);

///* 两个便捷 wrapper */
//DLIA_Status process_current(float* Current, int Sampling_Count, float Freq, float fs,
//                            float* out_magnitude, float* out_phase_rad)
//{
//    return run_lockin_once(Current, Sampling_Count, Freq, fs, out_magnitude, out_phase_rad);
//}

//// 修复：第二个输出参数加*
//DLIA_Status process_voltage(float* Voltage, int Sampling_Count, float Freq, float fs,
//                            float* out_magnitude, float* out_phase_rad)
//{
//    return run_lockin_once(Voltage, Sampling_Count, Freq, fs, out_magnitude, out_phase_rad);
//}

///* 通用一次性锁相放大处理函数 */
//DLIA_Status run_lockin_once(float* signal, int sampling_count, float Freq, float fs,
//                            float* out_magnitude, float* out_phase_rad)
//{
//    printf("数字锁相放大器程序正在运行 - 版本 %s\r\n", digital_lock_in_get_version());

//    /* 初始化 */
//    printf("初始化锁相放大器...\r\n");
//    DLIA_Status status = digital_lock_in_init(Freq, fs);
//    if (status != DLIA_SUCCESS) {
//        printf("错误: 初始化失败 (错误码: %d)\r\n", status);
//        return status;
//    }
//    printf("锁相放大器初始化成功！\r\n\n");

//    /* 显示当前配置 */
//    LockIn_Config config;
//    if (digital_lock_in_get_config(&config) == DLIA_SUCCESS) {
//        printf("当前配置:\r\n");
//        printf("  参考频率: %.1f Hz\r\n", config.ref_frequency);
//        printf("  采样率: %.1f Hz\r\n", config.filter.cutoff_freq);
//        printf("  滤波器截止频率: %.3f Hz\r\n", config.filter.cutoff_freq);
//        printf("  滤波器阶数: %d\r\n", config.filter.filter_order);
//        printf("  窗口类型: %s\r\n\n", config.filter.window_type);
//    }

//    /* 预处理 */
//    preprocess_signal(signal, sampling_count);

//    /* 多频/幅相提取 */
//    printf("开始幅相处理...\r\n");
//    float magnitude = 0.0f, phase_rad = 0.0f;
//    status = multifrequency_lock_in_process(signal, sampling_count, &magnitude, &phase_rad);
//    if (status == DLIA_SUCCESS) {
//        float phase_deg = phase_rad * (180.0f / M_PI);
//        // 修复：补齐4个参数
//        print_result("多频锁相放大", magnitude, phase_rad, phase_deg);
//        if (out_magnitude) *out_magnitude = magnitude;
//        if (out_phase_rad) *out_phase_rad = phase_rad;
//    } else {
//        printf("多频处理失败 (错误码: %d)\r\n", status);
//    }

//    /* 清理 */
//    digital_lock_in_cleanup();
//    printf("清理完成，返回。\r\n\n");

//    return status;
//}

///**
// * @brief 打印处理结果（修复参数缺失、未定义变量报错）
// */
//void print_result(const char* mode, float magnitude, float phase_rad, float phase_deg) {
//    printf("=== %s 模式结果 ===\r\n", mode);
//    printf("信号幅度: %.6f V\r\n", magnitude);
//    printf("信号相位: %.3f rad (%.2f°)\r\n", phase_rad, phase_deg);
//    printf("\r\n");
//}

///**
// * @brief 初始化FIR滤波器
// */
//static DLIA_Status fir_filter_init(FIRFilter* filter, int order) {
//    if (!filter || order <= 0) {
//        return DLIA_ERROR_INVALID_PARAM;
//    }
//    
//    filter->filter_order = order;
//    filter->buffer_index = 0;
//    
//    /* 分配系数内存 */
//    filter->coefficients = (float*)malloc((order + 1) * sizeof(float));
//    if (!filter->coefficients) {
//        return DLIA_ERROR_MEMORY;
//    }
//    
//    /* 分配延迟线内存并清零 */
//    filter->delay_line = (float*)calloc(order + 1, sizeof(float));
//    if (!filter->delay_line) {
//        free(filter->coefficients);
//        return DLIA_ERROR_MEMORY;
//    }
//    
//    filter->is_initialized = 1;
//    return DLIA_SUCCESS;
//}

///**
// * @brief FIR滤波器处理
// */
//static float fir_filter_process(FIRFilter* filter, float input) {
//    if (!filter || !filter->is_initialized) {
//        return 0.0f;
//    }
//    
//    /* 更新延迟线 */
//    filter->delay_line[filter->buffer_index] = input;
//    
//    /* 计算卷积 */
//    float output = 0.0f;
//    int index = filter->buffer_index;
//    
//    for (int i = 0; i <= filter->filter_order; i++) {
//        output += filter->coefficients[i] * filter->delay_line[index];
//        index = (index == 0) ? filter->filter_order : index - 1;
//    }
//    
//    /* 更新缓冲区索引 */
//    filter->buffer_index = (filter->buffer_index + 1) % (filter->filter_order + 1);
//    
//    return output;
//}

///**
// * @brief 释放FIR滤波器资源
// */
//static void fir_filter_free(FIRFilter* filter) {
//    if (!filter) return;
//    
//    if (filter->coefficients) {
//        free(filter->coefficients);
//        filter->coefficients = NULL;
//    }
//    if (filter->delay_line) {
//        free(filter->delay_line);
//        filter->delay_line = NULL;
//    }
//    
//    filter->filter_order = 0;
//    filter->buffer_index = 0;
//    filter->is_initialized = 0;
//}

///**
// * @brief 设计FIR低通滤波器系数（汉明窗）
// */
//static DLIA_Status design_fir_filter(float cutoff_freq, float sampling_rate, 
//                           int order, float* coefficients) {
//    if (!coefficients || order <= 0 || cutoff_freq <= 0 || sampling_rate <= 0) {
//        return DLIA_ERROR_INVALID_PARAM;
//    }
//    
//    int n = order + 1;
//    float omega_c = 2.0f * M_PI * cutoff_freq / sampling_rate;
//    float center = (n - 1) / 2.0f;
//    
//    for (int i = 0; i < n; i++) {
//        /* 计算理想低通滤波器系数 */
//        if (fabsf(i - center) < 1e-6f) {
//            coefficients[i] = omega_c / M_PI;
//        } else {
//            float offset = i - center;
//            coefficients[i] = sinf(omega_c * offset) / (M_PI * offset);
//        }
//        
//        /* 应用汉明窗 */
//        coefficients[i] *= (0.54f - 0.46f * cosf(2.0f * M_PI * i / (n - 1)));
//    }
//    
//    return DLIA_SUCCESS;
//}

///**
// * @brief 根据信号频率选择滤波器参数【降低阶数，兼容少采样点】
// */
//static DLIA_Status select_filter_parameters(float signal_freq, int* order, float* cutoff_freq) {
//    if (signal_freq > 1000.0f) {
//        *cutoff_freq = 100.0f;
//        *order = 100;
//    } else if (signal_freq > 100.0f) {
//        *cutoff_freq = 10.0f;
//        *order = 200;
//    } else if (signal_freq > 10.0f) {
//        *cutoff_freq = 1.0f;
//        *order = 400;
//    } else if (signal_freq > 1.0f) {
//        *cutoff_freq = 0.1f;
//        *order = 400; // 原800减半
//    } else {
//        *cutoff_freq = 0.01f;
//        *order = 800; // 原1200减半
//    }
//    return DLIA_SUCCESS;
//}

///* ====== 公共API实现 ====== */
//DLIA_Status digital_lock_in_init(float ref_frequency, float sampling_rate) {
//    /* 参数校验 */
//    if (ref_frequency <= 0 || sampling_rate <= 0) {
//        return DLIA_ERROR_INVALID_PARAM;
//    }
//    
//    /* 检查奈奎斯特准则 */
//    if (sampling_rate <= 2.0f * ref_frequency) {
//        return DLIA_ERROR_NYQUIST;
//    }
//    
//    /* 清理现有状态 */
//    digital_lock_in_cleanup();
//    
//    /* 选择滤波器参数 */
//    int filter_order;
//    float cutoff_freq;
//    
//    DLIA_Status result = select_filter_parameters(ref_frequency, &filter_order, &cutoff_freq);
//    if (result != DLIA_SUCCESS) {
//        return result;
//    }
//    
//    /* 初始化I路滤波器 */
//    result = fir_filter_init(&g_lock_in_amp.i_filter, filter_order);
//    if (result != DLIA_SUCCESS) {
//        return result;
//    }
//    
//    /* 初始化Q路滤波器 */
//    result = fir_filter_init(&g_lock_in_amp.q_filter, filter_order);
//    if (result != DLIA_SUCCESS) {
//        fir_filter_free(&g_lock_in_amp.i_filter);
//        return result;
//    }
//    
//    /* 设计I路滤波器系数 */
//    result = design_fir_filter(cutoff_freq, sampling_rate, filter_order, 
//                              g_lock_in_amp.i_filter.coefficients);
//    if (result != DLIA_SUCCESS) {
//        fir_filter_free(&g_lock_in_amp.i_filter);
//        fir_filter_free(&g_lock_in_amp.q_filter);
//        return result;
//    }
//    
//    /* Q路使用相同系数 */
//    memcpy(g_lock_in_amp.q_filter.coefficients, g_lock_in_amp.i_filter.coefficients,
//           (filter_order + 1) * sizeof(float));
//    
//    /* 保存配置 */
//    g_lock_in_amp.config.ref_frequency = ref_frequency;
//    g_lock_in_amp.config.sampling_rate = sampling_rate;
//    g_lock_in_amp.config.filter.cutoff_freq = cutoff_freq;
//    g_lock_in_amp.config.filter.filter_order = filter_order;
//    g_lock_in_amp.config.filter.window_type = "Hamming";
//    
//    g_lock_in_amp.is_initialized = 1;
//    
//    return DLIA_SUCCESS;
//}

//DLIA_Status multifrequency_lock_in_process(const float* signal, int sample_count, 
//                                          float* magnitude, float* phase_rad) {
//    /* 参数检查 */
//    if (!signal || !magnitude || !phase_rad || sample_count <= 0) {
//        return DLIA_ERROR_INVALID_PARAM;
//    }
//    
//    if (!g_lock_in_amp.is_initialized) {
//        return DLIA_ERROR_NOT_INITIALIZED;
//    }
//    
//    int filter_order = g_lock_in_amp.i_filter.filter_order;
//    int min_samples = filter_order * 2;
//    printf("sample_count: %d , 滤波器阶数: %d , 推荐最小采样: %d\r\n", sample_count, filter_order, min_samples);
//    
//    // 采样不足仅警告，不阻断计算
//    if (sample_count < min_samples) {
//        printf("【警告】采样点数不足推荐值，测量精度下降，仍继续计算\r\n");
//    }
//    
//    float omega = 2.0f * M_PI * g_lock_in_amp.config.ref_frequency;
//    float i_total = 0.0f, q_total = 0.0f;
//    int valid_samples = 0;
//    
//    /* 锁相放大核心算法 */
//    for (int i = 0; i < sample_count; i++) {
//        Watchdog_Refresh();
//        float t = (float)i / g_lock_in_amp.config.sampling_rate;
//        
//        /* 生成正交参考信号 */
//        float ref_i = cosf(omega * t);  // 同相分量
//        float ref_q = sinf(omega * t);  // 正交分量
//        
//        /* 信号与参考信号相乘（相敏检波） */
//        float mixed_i = signal[i] * ref_i;
//        float mixed_q = signal[i] * ref_q;
//        
//        /* FIR滤波 */
//        float filtered_i = fir_filter_process(&g_lock_in_amp.i_filter, mixed_i);
//        float filtered_q = fir_filter_process(&g_lock_in_amp.q_filter, mixed_q);
//        
//        /* 累加稳定后的结果（跳过初始瞬态） */
//        if (i > filter_order) {
//            i_total += filtered_i;
//            q_total += filtered_q;
//            valid_samples++;
//        }
//    }
//    
//    if (valid_samples > 0) {
//        i_total /= valid_samples;
//        q_total /= valid_samples;
//        
//        /* 计算幅度和相位 */
//        *magnitude = 2.0f * sqrtf(i_total * i_total + q_total * q_total);
//        *phase_rad = atan2f(q_total, i_total);
//        
//        return DLIA_SUCCESS;
//    }
//    
//    return DLIA_ERROR_PROCESSING;
//}

//DLIA_Status high_precision_lock_in(const float* signal, int sample_count, LockIn_Result* result) {
//    float magnitude, phase_rad;
//    DLIA_Status status = multifrequency_lock_in_process(signal, sample_count, &magnitude, &phase_rad);
//    
//    if (status == DLIA_SUCCESS) {
//        result->magnitude = magnitude;
//        result->phase_rad = phase_rad;
//        result->phase_deg = phase_rad * (180.0f / M_PI);
//        
//        /* 简单的噪声估计 */
//        float noise_sum = 0.0f;
//        for (int i = 0; i < sample_count; i++) {
//            float t = (float)i / g_lock_in_amp.config.sampling_rate;
//            float expected = magnitude * 0.5f * sinf(2.0f * M_PI * g_lock_in_amp.config.ref_frequency * t + phase_rad);
//            noise_sum += fabsf(signal[i] - expected);
//        }
//        result->rms_noise = noise_sum / sample_count;
//    }
//    
//    return status;
//}

//DLIA_Status digital_lock_in_reconfigure(float new_ref_frequency, float new_sampling_rate) {
//    digital_lock_in_cleanup();
//    return digital_lock_in_init(new_ref_frequency, new_sampling_rate);
//}

//DLIA_Status digital_lock_in_get_config(LockIn_Config* config) {
//    if (!config || !g_lock_in_amp.is_initialized) {
//        return DLIA_ERROR_NOT_INITIALIZED;
//    }
//    
//    *config = g_lock_in_amp.config;
//    return DLIA_SUCCESS;
//}

//void preprocess_signal(float* signal, int sample_count) {
//    if (!signal || sample_count <= 0) return;
//    
//    /* 计算直流偏移 */
//    float dc_offset = 0.0f;
//    for (int i = 0; i < sample_count; i++) {
//        dc_offset += signal[i];
//    }
//    dc_offset /= sample_count;
//    
//    /* 去除直流分量 */
//    for (int i = 0; i < sample_count; i++) {
//        signal[i] -= dc_offset;
//    }
//}

//float estimate_signal_frequency(const float* signal, int sample_count, float sampling_rate) {
//    /* 简单的过零检测法估算频率 */
//    int zero_crossings = 0;
//    for (int i = 1; i < sample_count; i++) {
//        if (signal[i-1] * signal[i] < 0) {
//            zero_crossings++;
//        }
//    }
//    return (zero_crossings * sampling_rate) / (2.0f * sample_count);
//}

//void digital_lock_in_cleanup(void) {
//    if (g_lock_in_amp.is_initialized) {
//        fir_filter_free(&g_lock_in_amp.i_filter);
//        fir_filter_free(&g_lock_in_amp.q_filter);
//        g_lock_in_amp.is_initialized = 0;
//    }
//}

//const char* digital_lock_in_get_version(void) {
//    return LIB_VERSION;
//}







