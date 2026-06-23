#include "system.h"

extern volatile float Relative_Amplitude;
extern volatile uint16_t SIN_DATA; // 单周期内采样点数
extern volatile uint16_t cycle_count; // ???stm32h7xx_it.c??
extern volatile uint8_t Num_period; // ??????
extern volatile uint8_t ADC_status;
extern volatile uint8_t ADC_Start_Standard; // ADC是否启动判断
extern volatile uint64_t Sampling_Count;					// 采样点数计数
extern volatile uint16_t Num_Sampling_Points;


void related_detect(float *Voltage, float *Current, uint64_t Sampling_Count, double Freq,  float *R_out, float *X_out);
void EIS_Measure(void );
void loop_EIS_test(uint16_t* Sine_data);
void sweep_measure(double* Freq_Points, int total_points);
void single_measure(double Freq);
void Send_EIS_Result(void);
void FDCAN1_Send_String(uint8_t *msg);
void calibrateBias(void);
void Power5200_Init(void) ;
void Power5200_Enable(void);
void Power5200_Disable(void);
void DAC8830_set_Voltage(double V);
void FAN_Init(void);
void FAN_Enable(void);
void FAN_Disable(void);
void RELAY_Init(void);
void RELAY_Enable(void);
void RELAY_Disable(void);



// 开启5200电源（PC2高电平）
void Power5200_Enable(void) ;

// 关闭5200电源（PC2低电平）
void Power5200_Disable(void);




void EIS_SingleFrequency_Measure(double Freq );
void EIS_FrequencySweep_Measure(double Freq_Start,double Freq_End);

void set_excitation_current(float current_mA); //设置激励电流
