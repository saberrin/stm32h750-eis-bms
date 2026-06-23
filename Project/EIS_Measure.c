#include "fdcan.h"
#include "system.h"
#include "delay.h"
#include "led.h"
#include "key.h"
#include "dac.h"
#include "timer.h"
#include "usart.h"
#include "delay.h"
#include "SineWave.h"
#include "AD7606.h"
#include "math.h"
#include "parse_command.h"
#include "global_command.h"
#include "EIS_Measure.h"
#include "watchdog.h"
#include "dac8830.h"
#include "digital_lock_in.h"
#include "MOS_Controller.h"
#include "AD620_GainCtrl.h"
#include "ADS131A04.h"
#include "fft_analyzer.h"
#include <stdint.h>
//-----------------------------------------------------------------
#define TIME_CLK 200000000
//#define PI 3.14159265358979323846
#define PI 3.14159265f
volatile uint16_t SIN_DATA =210; // 单周期内采样点数
volatile uint16_t cycle_count = 0; // 
volatile uint8_t Num_period = 10; // 周期数目                 
volatile uint16_t Num_Sampling_Points = 0; // ??????
volatile float Relative_Amplitude = 0.5;// 设置幅值大小
volatile uint8_t ADC_status = 1; // ADC状态设置
volatile uint8_t ADC_Start_Standard = 0; // ADC是否启动判断
volatile uint64_t Sampling_Count = 0;					// 采样点数计数
double Freq_Start = 10000;
double Freq_End = 0.1;
volatile uint8_t Period_Count = 5;  //
int resume_index = 0;
uint16_t Sine_data[300];  // 正弦波初始化
#define MAX_EIS_POINTS 256


// 定义激励电流的上下限阈值（单位：A）
#define MIN_EXCITATION_CURRENT 0.1f
#define MAX_EXCITATION_CURRENT 2.0f

#define ADS_VREF    4.0f


typedef struct {
    float real_impedance;
    float imag_impedance;
    float frequency;
} EIS_DataRow;

EIS_DataRow eis_batch_buffer[MAX_EIS_POINTS];
uint32_t batch_count = 0;
uint8_t Meas_Comp_Flag = 0;



static int32_t RawToSignedCode(uint32_t raw)
{
    int32_t val = raw >> 8;
    if(val & 0x800000) val |= 0xFF000000;
    return val;
}

static float SignedCodeToVoltage(int32_t code)
{
    return (float)code / 8388608.0f * ADS_VREF;
}

static float CodeToVoltage(uint32_t raw)
{
    return SignedCodeToVoltage(RawToSignedCode(raw));
}





void set_excitation_current(float current)
{
    float clamped_current = current;
 
    
    // 检查并限制电流值在有效范围内
    if (current < MIN_EXCITATION_CURRENT) {
        clamped_current = MIN_EXCITATION_CURRENT;

        printf("警告：激励电流 %.1f A 低于最小值 %.1f A，已自动设置为最小值\n", 
               current, MIN_EXCITATION_CURRENT);
    }
    else if (current > MAX_EXCITATION_CURRENT) {
        clamped_current = MAX_EXCITATION_CURRENT;
      
        printf("警告：激励电流 %.1f A 超过最大值 %.1f A，已自动设置为最大值\n", 
               current, MAX_EXCITATION_CURRENT);
    }
    else {
        printf("信息：激励电流设置为 %.1f A\n", current);
    }
    
    // 执行电流设置操作
    Relative_Amplitude = 4 * clamped_current ;
    
   
}



// 采样率切换台阶补偿：用切换后静态基线均值估计ADC数字滤波器档位带来的偏移
static int32_t last_baseline_volt_code = 0;
static int32_t last_baseline_curr_code = 0;
static int32_t delta_volt_code = 0;
static int32_t delta_curr_code = 0;
static uint8_t first_measure_flag = 1;





//void Auto_Set_ADC_SampleRate(double Freq)
//{
//    uint32_t target_sps;
//    if (Freq < 0.01)
//        Freq = 0.01;

//    if (Freq > 10000.0)
//        target_sps = 125000U;
//    else if (Freq > 5000.0)
//        target_sps = 31250U;
//    else if (Freq > 1000.0)
//        target_sps = 15625U;
//    else if (Freq > 100.0)
//        target_sps = 3906U;
//    else if (Freq > 30.0)
//        target_sps = 488U;
//    else
//        // ≤30Hz，61SPS，fs>60满足奈奎斯特，超高OS降噪
//        target_sps = 61U;

//   ADS131A0X_Init_SetSampleRate(target_sps);
//	//	ADS131A0X_ChangeSampleRate_NoReset(target_sps);
//		
//		  // ========== 新增：丢弃200个瞬态采样，等待数字滤波器完全收敛 ==========
//    float temp_buf[4];
//    for(uint16_t discard_cnt = 0; discard_cnt < 1200; discard_cnt++)
//    {
//        ADS131A0X_ReadData(temp_buf);
//        Watchdog_Refresh();
//    }
//		
//		
//		
//}



void Auto_Set_ADC_SampleRate(double Freq)
{
    uint32_t target_sps;
    if (Freq < 0.01)
        Freq = 0.01;

    if (Freq > 10000.0)
        target_sps = 125000U;
    else if (Freq > 5000.0)
        target_sps = 31250U;
    else if (Freq > 1000.0)
        target_sps = 15625U;
    else if (Freq > 100.0)
        target_sps = 3906U;
    else if (Freq > 30.0)
        target_sps = 488U;
    else
        target_sps = 61U;

    ADS131A0X_ChangeSampleRate_NoReset(target_sps);

    // 动态丢弃收敛点
    uint16_t discard_cnt;
    if(target_sps <= 100)      discard_cnt = 1200;
    else if(target_sps <= 500) discard_cnt = 800;
    else if(target_sps <= 4000)discard_cnt = 500;
    else                       discard_cnt = 300;
    float temp_buf[4];
    for(uint16_t i = 0; i < discard_cnt; i++)
    {
        ADS131A0X_ReadData(temp_buf);
        Watchdog_Refresh();
    }
    while(READ_DRDY());

    // 读取切换后一批静态样本，估计采样率档位切换引入的数字码台阶。
    uint32_t tmp_v, tmp_i;
    int64_t sum_v = 0;
    int64_t sum_i = 0;
    const uint16_t avg_cnt = 200;
    for(uint16_t k=0; k<avg_cnt; k++)
    {
        ADS131A0X_Read_Ch1_Ch2(&tmp_i, &tmp_v);
        sum_i += RawToSignedCode(tmp_i);
        sum_v += RawToSignedCode(tmp_v);
        Watchdog_Refresh();
    }

    int32_t avg_i_code = (int32_t)(sum_i / avg_cnt);
    int32_t avg_v_code = (int32_t)(sum_v / avg_cnt);

    if(first_measure_flag == 1)
    {
        delta_volt_code = 0;
        delta_curr_code = 0;
        first_measure_flag = 0;
    }
    else
    {
        delta_volt_code = avg_v_code - last_baseline_volt_code;
        delta_curr_code = avg_i_code - last_baseline_curr_code;
    }

    last_baseline_volt_code = avg_v_code;
    last_baseline_curr_code = avg_i_code;

    printf("ADC采样率切换: Freq=%.6fHz, SPS=%lu, V_step=%.6fmV, I_step=%.6fmV\r\n",
           Freq,
           (unsigned long)target_sps,
           SignedCodeToVoltage(delta_volt_code) * 1000.0f,
           SignedCodeToVoltage(delta_curr_code) * 1000.0f);
}



/**
  * @brief  EIS单频测量函数
  * @param  Freq: 待测量的单一频率值 (Hz)
  * @retval None
  */
void EIS_SingleFrequency_Measure(double Freq)
{
   
Power5200_Enable();
	// 参数校验
    if (Freq <= 0.0) {
        printf("错误：测量频率必须大于0 (当前值: %.6f Hz)\r\n", Freq);
        return;
    }

    // 1. 硬件初始化与配置
    HAL_Delay(100); // 确保开关矩阵稳定
    
    AD620_GainCtrl_Init();
    AD620_SetGainByValue(2);
//    AD7606_SetRange(AD7606_RANGE_10V);
    calibrateBias(); // 校准偏置电压

    // 2. 信号生成系统初始化
    HAL_Delay(1000); // 确保硬件完全就绪
    GenerateSineWave(Sine_data, 1.0f * Relative_Amplitude / 8.0f, 0.5f);
    DAC_Init(Sine_data);

    // 3. 执行单频测量
           
        // 调用核心测量函数
        single_measure(Freq);               
      Power5200_Disable();
				HAL_Delay(10);
				
        
}
    



/**
  * @brief  EIS扫频测量函数
  * @param  Freq_Start: 扫频起始频率 (Hz)
  * @param  Freq_End: 扫频结束频率 (Hz)
  * @retval None
  */
void EIS_FrequencySweep_Measure(double Freq_Start, double Freq_End)
{
    
	Power5200_Enable();
   
    
    AD620_GainCtrl_Init();
    AD620_SetGainByValue(500);

	 HAL_Delay(500);
//   calibrateBias();
   RELAY_Enable();
delay_ms(1000);

RELAY_Disable(); 
    // 2. 计算频率点序列
    Period_Count = QG_EIS_FREQ_POINTS;
    double num_decades = log10(Freq_Start / Freq_End);
    
    // 确保起始频率大于结束频率（从高频扫到低频）
    if (num_decades < 0) {
        double temp = Freq_Start;
        Freq_Start = Freq_End;
        Freq_End = temp;
        num_decades = log10(Freq_Start / Freq_End);
    }
    
    const int total_points = num_decades * Period_Count + 1;
    double Freq_Points[total_points];
    
    // 生成对数均匀分布的频率点
    generate_frequency_points(Freq_Points, Freq_Start, Freq_End, 
                           Period_Count, num_decades, total_points);
    
    // 3. 信号生成系统初始化
    HAL_Delay(1000);
    GenerateSineWave(Sine_data, 1.0f * Relative_Amplitude / 8.0f, 0.5f);
    DAC_Init(Sine_data);
    
    // 4. 执行扫频测量

        sweep_measure(Freq_Points, total_points);
        Power5200_Disable();
				HAL_Delay(10);
}





void single_measure(double Freq){
		Watchdog_Refresh();
		
	

	
	
	   Auto_Set_ADC_SampleRate( Freq);
	
	
	
	
	
	
	
	
	
	
	
	 Power5200_Enable();	
		Num_period = 20;

		
	static	uint32_t Voltage_data[256*20] = {0};  // 所有元素自动初始化为 0
	static	uint32_t Current_data[256*20] = {0};  // 所有元素自动初始化为 0
	static	float Voltage[256*20] = {0};  // 所有元素自动初始化为 0
	static	float Current[256*20] = {0};  // 所有元素自动初始化为 0
		memset(Voltage_data,0,sizeof(Voltage_data));
memset(Current_data,0,sizeof(Current_data));
memset(Voltage,0,sizeof(Voltage));
memset(Current,0,sizeof(Current));
		ADC_status = 1;
		cycle_count = 0;
		Sampling_Count = 0;
		Num_Sampling_Points = SIN_DATA * Num_period;
		uint16_t Freq_Prescaler_initial = (int)3-log10(Freq);
		double Freq_Prescaler = pow(10,Freq_Prescaler_initial)-1;
		
		TIM6_Init(TIME_CLK/SIN_DATA/Freq/pow(10,Freq_Prescaler_initial),(uint16_t ) Freq_Prescaler);	// 重新设定重转载值
		HAL_TIM_Base_Start_IT(&TIM6_Handler); // 开启中断
		Restart_DAC_Wave(Sine_data,SIN_DATA);  // 重启DAC

		while (ADC_status == 1){
			if(ADC_Start_Standard == 1) {
				Watchdog_Refresh();			
    //   AD7606_StartConvst();
		//		AD7606_ReadData_2Ch(&Current_data[Sampling_Count],&Voltage_data[Sampling_Count]);
        ADS131A0X_Read_Ch1_Ch2(&Current_data[Sampling_Count],&Voltage_data[Sampling_Count]);
		
				
	//		Voltage[Sampling_Count] =	ADS131A0X_Read_Channel(3);
	//		Current[Sampling_Count]=ADS131A0X_Read_Channel(0);
				ADC_Start_Standard = 0;  // 置为0不采集信号
				Sampling_Count++;
			}
						
		//	   LED_Green_Toggle();
		}
   Power5200_Disable();

	 printf("Voltage:\n");
	 for (int i = 0; i < Sampling_Count; i++) {
	//	Voltage[i] =	 -1*AD7606B_Digital2Voltage(Voltage_data[i]);	//已经校准好，单位V	
   //   Voltage[i]=-1.0*Voltage_data[i]/1000.0;
		 
	int32_t real_v_code = RawToSignedCode(Voltage_data[i]) - delta_volt_code;
		
       Voltage[i] = -1.0f * SignedCodeToVoltage(real_v_code);
//		 Voltage[i]=-1*CodeToVoltage(Voltage_data[i]);
		 
		 
		 
		 printf("%.8f;\n", Voltage[i]/1000);
				Watchdog_Refresh();
    }
	 printf("\r\n");		
		
	 printf("Current:\n");
	 for (int i = 0; i < Sampling_Count; i++) {
		//		Current[i] =	2* AD7606B_Digital2Voltage(Current_data[i])-5;	//已经校准好，单位A
 //    Current[i] =2.0*Current_data[i]/1000.0-5.0;
    int32_t real_i_code = RawToSignedCode(Current_data[i]) - delta_curr_code;
    Current[i] = SignedCodeToVoltage(real_i_code);
		 printf("%.8f;\n",Current[i]);
				Watchdog_Refresh();
    }
	 	printf("\r\n");
		HAL_Delay(1);
		
		
		
		
		
		
		
		
			//////////////////相关检测	

    float Current_magnitude, Current_phase_rad, Voltage_magnitude, Voltage_phase_rad;
    float fs = Freq*Sampling_Count/Num_period  ;

		
		/* 处理电流 */
    DLIA_Status st = process_current(Current, Sampling_Count, Freq, fs, &Current_magnitude, &Current_phase_rad);
    if (st != DLIA_SUCCESS) {
        printf("处理电流失败 (错误码: %d)\r\n", st);
    } else {
        printf("电流处理结果: magnitude=%.6f, phase=%.6f rad\r\n", Current_magnitude, Current_phase_rad);
    }

    /* 处理电压 */
    st = process_voltage(Voltage, Sampling_Count, Freq, fs, &Voltage_magnitude, &Voltage_phase_rad);
    if (st != DLIA_SUCCESS) {
        printf("处理电压失败 (错误码: %d)\r\n", st);
    } else {
        printf("电压处理结果: magnitude=%.6f, phase=%.6f rad\r\n", Voltage_magnitude, Voltage_phase_rad);
    }
//    free(Current);
//    free(Voltage);
    
    printf("\n=== 相关检测算法结束 ===\r\n");
		
		HAL_Delay(1);
		 
		printf("数字锁相放大器分析结果：\r\n");
		printf("========================\r\n");
		printf("信号频率: %.1f Hz\r\n", Freq);
		printf("采样率: %.1f Hz\n", fs);
		printf("采样点数: %llu\n", Sampling_Count);
		printf("电流提取幅度: %.6f V\r\n", Current_magnitude);
		printf("电流提取相位: %.6f 弧度 \r\n", Current_phase_rad);
		printf("电压提取幅度: %.6f V\r\n", Voltage_magnitude);
		printf("电压提取相位: %.6f 弧度 \r\n", Voltage_phase_rad);
       
   
		float Z_mag = Voltage_magnitude /  Current_magnitude;  // 阻抗模值 = 电压幅度 / 电流幅度
		float phase_diff_rad = Voltage_phase_rad - Current_phase_rad;  // 相位差（电压相位 - 电流相位）	
		// 将相位差转换为角度制
		float phase_diff_deg = phase_diff_rad * 180.0f / PI;		
		// 计算阻抗的复数形式：实部（电阻R）和虚部（电抗X）
		float R = Z_mag * cosf(phase_diff_rad);  // 电阻分量
		float X = Z_mag * sinf(phase_diff_rad);  // 电抗分量		

							
		printf("===== EIS Result =====\r\n");
		printf("Freq: %.6f Hz\r\n", Freq);  // 输出频率值，保留6位小数
		printf("Real Part: %.6f Ω\r\n", R);  // 电阻分量
		printf("Imaginary Part: %.6f Ω\r\n",-1*X);  // 电抗分量（感抗/容抗）
		printf("Impedance Phase Angle θ: %.2f°\r\n",  phase_diff_deg);
		printf("======================================\r\n\n");




//		FFTOutput fft_v, fft_i;
//	double fs = Sampling_Count / ( 10/Freq );
////	 float fs = Freq*Sampling_Count/Num_period  ;
//	
//	
//  Goertzel_Analyze(Voltage, Sampling_Count, Freq, fs, &fft_v);
//	Goertzel_Analyze(Current, Sampling_Count, Freq, fs, &fft_i);
//	
//	
//		
//float Z_mag = fft_v.magnitude / fft_i.magnitude;  // 阻抗模值 = 电压幅度 / 电流幅度
//float phase_diff_rad = fft_v.phase_rad - fft_i.phase_rad;  // 相位差（电压相位 - 电流相位）	
//// 将相位差转换为角度制
//float phase_diff_deg = phase_diff_rad * 180.0f / PI;		
//// 计算阻抗的复数形式：实部（电阻R）和虚部（电抗X）
//float R = Z_mag * cosf(phase_diff_rad);  // 电阻分量
//float X = Z_mag * sinf(phase_diff_rad);  // 电抗分量		

//		
//printf("===== EIS Result =====\r\n");
//printf("Freq: %.6f Hz\r\n", Freq);  // 输出频率值，保留6位小数
//printf("Real Part: %.6f Ω\r\n", R);  // 电阻分量
//printf("Imaginary Part: %.6f Ω\r\n", X);  // 电抗分量（感抗/容抗）
//printf("Impedance Phase Angle θ: %.2f°\r\n",  phase_diff_deg);
//printf("======================================\r\n\n");










		HAL_Delay(1);
		eis_batch_buffer[batch_count].real_impedance = R;
		eis_batch_buffer[batch_count].imag_impedance = X;
		eis_batch_buffer[batch_count].frequency = Freq;
		batch_count++;
		
		printf("0x%X_B_SWF_Freq%.4frea%.4fimage%.4f_end\r\n",QG_ID,Freq, R, X);
		char line_buffer[256];
    snprintf(line_buffer, sizeof(line_buffer), "0x%X_B_SWF_Freq%.4frea%.4fimage%.4f_end\r\n",QG_ID,Freq, R, X);
    FDCAN1_Send_String((uint8_t *)line_buffer);	
		
		
	
}

void sweep_measure(double* Freq_Points, int total_points){

	double Freq;	
	resume_index = 0;
	
	Power5200_Enable();	

	for (int i = resume_index; i < total_points; i++) {		
		
		Freq = Freq_Points[i]; 
		//输出测试频率
		printf(" Freq %.5f\n", Freq);		
		single_measure(Freq);
	}
	Power5200_Disable();	
}





uint32_t CalculateChecksum(EIS_DataRow *data, uint32_t count) {
    uint32_t checksum = 0;
    for (uint32_t i = 0; i < count; i++) {
        checksum += (uint32_t)(data[i].frequency + data[i].real_impedance + data[i].imag_impedance);
    }
    return checksum;
}
void Send_EIS_Result() {
    char line_buffer[256];

    snprintf(line_buffer, sizeof(line_buffer), "0x%X_EIS_data_packet_start_", QG_ID);
    FDCAN1_Send_String((uint8_t *)line_buffer);
		printf("0x%X_EIS_data_packet_start_", QG_ID);

    for (uint32_t j = 0; j < batch_count; j++) {
        snprintf(line_buffer, sizeof(line_buffer), "R%d,%.4f,I%d,%.4f,F%d,%.4f",
                 j + 1,
                 eis_batch_buffer[j].real_impedance,
                 j + 1,
                 eis_batch_buffer[j].imag_impedance,
                 j + 1,
                 eis_batch_buffer[j].frequency);
        FDCAN1_Send_String((uint8_t *)line_buffer);
				printf("%s", line_buffer);

        if (j < batch_count - 1) {
            FDCAN1_Send_String((uint8_t *)";");
        }
    }

    // ???
    uint32_t checksum = CalculateChecksum(eis_batch_buffer, batch_count);
    snprintf(line_buffer, sizeof(line_buffer), ";CHECKSUM,%u;", checksum);
    FDCAN1_Send_String((uint8_t *)line_buffer);
		printf("%s", line_buffer);

    // ??
    float temperature = 25.0f;  // ??:??? Ds18b20ReadTemp()/16.0
    snprintf(line_buffer, sizeof(line_buffer), "TEM_%.4f", temperature);
    FDCAN1_Send_String((uint8_t *)line_buffer);

    // ????
    FDCAN1_Send_String((uint8_t *)"_EIS_data_packet_end\r\n");
		printf("_EIS_data_packet_end\r\n");
}

#define FDCAN_MAX_DATA_LEN 8
const uint32_t FDCAN_DLC_BYTES_MAP[9] = {
    FDCAN_DLC_BYTES_0,
    FDCAN_DLC_BYTES_1,
    FDCAN_DLC_BYTES_2,
    FDCAN_DLC_BYTES_3,
    FDCAN_DLC_BYTES_4,
    FDCAN_DLC_BYTES_5,
    FDCAN_DLC_BYTES_6,
    FDCAN_DLC_BYTES_7,
    FDCAN_DLC_BYTES_8
};
void FDCAN1_Send_String(uint8_t *msg) {
    uint32_t len = strlen((char *)msg);
    uint32_t offset = 0;

    while (offset < len) {
        uint8_t send_len = (len - offset > FDCAN_MAX_DATA_LEN) ? FDCAN_MAX_DATA_LEN : (len - offset);
				uint32_t dlc_value = FDCAN_DLC_BYTES_MAP[send_len];
        FDCAN1_Send_Msg(msg + offset, dlc_value);
        offset += send_len;
        HAL_Delay(1);  
    }
}









///**  最开始的老版本
//  * @brief  校准偏置电压，使放大器输出归零
//  * @note   通过调整DAC电压，使放大器输出稳定在[-0.05V, 0.05V]范围内
//  *         DB_data[1]: 放大器输出值
//  *         DB_data[2]: DAC电压（放大器反向输入）
//  *         DB_data[3]: 电池电压（放大器正向输入）
//  */
//void calibrateBias(void)  
//{
//    s16 DB_data[6] = {0};    
//    double BAT_VOLT;
//    
//    /* 初始采样 */
//    AD7606_StartConvst();
//    AD7606_ReadData(DB_data);
//        AD7606_StartConvst();
//    AD7606_ReadData(DB_data);
//		    AD7606_StartConvst();
//    AD7606_ReadData(DB_data);
//    /* 打印所有通道初始值 */
//    for (int i = 0; i < 6; i++) {
//        printf("通道 %d = %.3fV\r\n", i + 1, AD7606B_Digital2Voltage(DB_data[i]));
//    }
//    
//    /* 获取初始电池电压 */
//    BAT_VOLT = AD7606B_Digital2Voltage(DB_data[3]);
//    printf("初始电池电压 = %.3fV\r\n", BAT_VOLT);
//    
//   


//		/* 主校准循环 */
//    while (1) {
//        Watchdog_Refresh();
//        DAC8830_set_Voltage(BAT_VOLT);
//        
//			 AD7606_StartConvst();
//        AD7606_ReadData(DB_data);
//	
//        /* 获取各通道电压值 */
//        double amp_output = AD7606B_Digital2Voltage(DB_data[1]);    /* 差分输出 */
//        double dac_voltage = AD7606B_Digital2Voltage(DB_data[2]);   /* DAC电压 */
//        double bat_voltage = AD7606B_Digital2Voltage(DB_data[3]);   /* 电池电压 */
//        
//					 if (amp_output > 2.11) {
//					
//            BAT_VOLT += 0.001;
//            printf("DAC增加0.001V,放大器输出=%.3fV\r\n", amp_output);
//					 } else if (amp_output < 2.09) {	 
//						 
//            BAT_VOLT -= 0.001;
//           printf("DAC减少0.001V,放大器输出=%.3fV\r\n", amp_output);
//          } else {
//           
//					 printf("放大器输出=%.3fV\r\n", amp_output);
//           printf("最终DAC电压=%.3fV, 电池电压=%.3fV\r\n", dac_voltage, bat_voltage);
//            break;									
//                 }

//    }
//		
//		   AD7606_StartConvst();
//        AD7606_ReadData(DB_data);
//		
//		 double amp_output = AD7606B_Digital2Voltage(DB_data[1]);    /* 差分输出 */
//		
//		 一定再测一次电压，确保   2.11>amp_output > 2.09)
//	   否则再来一次主校准循环。
//		
//		
//}


////稳定性好，速度快，精度差
//void calibrateBias(void)
//{
//    s16 DB_data[6] = {0};
//    double DAC_VOLT;
//    double amp_output;
//    uint8_t stable_cnt = 0;

//    // 超精细步长 0.0005，彻底弱化调节力度
//    const double step = 0.0005;
//    const double dac_min = 0.5;
//    const double dac_max = 4.9;

//    // 超大滞回死区，根治横跳
//    const double v_up_th  = 2.13;
//    const double v_down_th= 2.07;

//    // 初始采样
//    for(int i=0; i<3; i++)
//    {
//        AD7606_StartConvst();
//        AD7606_ReadData(DB_data);
//    }
//    for (int i = 0; i < 6; i++)
//    {
//        printf("通道 %d = %.3fV\r\n", i + 1, AD7606B_Digital2Voltage(DB_data[i]));
//    }

//    DAC_VOLT = AD7606B_Digital2Voltage(DB_data[3]);
//    printf("初始DAC预设电压 = %.3fV\r\n", DAC_VOLT);

//    while(1)
//    {
//        stable_cnt = 0;
//        while (1)
//        {
//            Watchdog_Refresh();

//            if(DAC_VOLT < dac_min) DAC_VOLT = dac_min;
//            if(DAC_VOLT > dac_max) DAC_VOLT = dac_max;

//            DAC8830_set_Voltage(DAC_VOLT);
//            HAL_Delay(5);   // 加长延时，消除运放响应滞后

//            AD7606_StartConvst();
//            AD7606_ReadData(DB_data);
//            amp_output = AD7606B_Digital2Voltage(DB_data[1]);

//            // 饱和保护
//            if(amp_output > 4.30)
//            {
//                DAC_VOLT -= step;
//                printf("【正饱和】DAC-%.4f | OUT=%.3fV\r\n",step,amp_output);
//                continue;
//            }
//            if(amp_output < 0.10)
//            {
//                DAC_VOLT += step;
//                printf("【负饱和】DAC+%.4f | OUT=%.3fV\r\n",step,amp_output);
//                continue;
//            }

//            // 超宽死区，中间绝对不动作
//            if (amp_output > v_up_th)
//            {
//                DAC_VOLT += step;
//                printf("DAC+%.4f | 运放=%.3fV\r\n", step, amp_output);
//                stable_cnt = 0;
//            }
//            else if (amp_output < v_down_th)
//            {
//                DAC_VOLT -= step;
//                printf("DAC-%.4f | 运放=%.3fV\r\n", step, amp_output);
//                stable_cnt = 0;
//            }
//            else
//            {
//                // 进入安全死区，只计数、不调节
//                stable_cnt++;
//                if(stable_cnt >= 20)
//                {
//                    printf("\r\n==== 校准锁定完成 彻底停止 ====\r\n");
//                    printf("最终偏置=%.3fV | 最终DAC=%.3fV\r\n",amp_output,DAC_VOLT);
//                    goto calibrate_ok;
//                }
//                // 打印稳定状态，方便观察
//                printf("?稳定中 运放=%.3fV 无调节\r\n",amp_output);
//            }
//        }
//calibrate_ok:
//        break;
//    }
//}

////稳定性好，速度慢，精度高，中心点3.10
//void calibrateBias(void)
//{
//    s16 DB_data[8] = {0};
//    double DAC_VOLT;
//    double amp_output;
//    uint8_t stable_cnt = 0;

//    // 保持超细步长0.0005V，避免超调反弹，匹配高增益运放特性
//    const double step = 0.0001;
//    const double dac_min = 2.0;
//    const double dac_max = 4.2;

//    // 目标中心改为3.00V，保留0.03V窄滞回区间（3.00±0.015），兼顾精度与抗震荡
//    const double v_up_th  = 3.1;  // 上阈值（中心+0.015V）
//    const double v_down_th= 2.9;  // 下阈值（中心-0.015V）

// 
// 
//        AD7606_StartConvst();
//        AD7606_ReadData(DB_data);
//    
//    // 打印各通道初始电压
//    for (int i = 0; i < 6; i++)
//    {
//        printf("通道 %d = %.3fV\r\n", i + 1, AD7606B_Digital2Voltage(DB_data[i]));
//    }

//		
//		
//    // 读取初始DAC预设电压
//    DAC_VOLT = AD7606B_Digital2Voltage(DB_data[3])-0.4;
//    printf("初始DAC预设电压 = %.3fV\r\n", DAC_VOLT);

//    while(1)
//    {
//        stable_cnt = 0;
//        while (1)
//        {
//            Watchdog_Refresh();

//            // DAC输出限幅保护，防止超出器件工作范围
//            if(DAC_VOLT < dac_min) DAC_VOLT = dac_min;
//            if(DAC_VOLT > dac_max) DAC_VOLT = dac_max;

//            // 输出DAC电压，预留5ms运放响应时间，补偿运放滞后
//            DAC8830_set_Voltage(DAC_VOLT);
//            HAL_Delay(10);

//            // 采集运放输出电压，反馈至MCU
//            AD7606_StartConvst();
//            AD7606_ReadData(DB_data);
//					AD7606_StartConvst();
//            AD7606_ReadData(DB_data);
//            amp_output = AD7606B_Digital2Voltage(DB_data[1]);

//            // 正饱和保护，避免运放输出过载
//            if(amp_output > 4.30)
//            {
//                DAC_VOLT +=0.01;
//                printf("【正饱和】DAC-%.4f | OUT=%.3fV\r\n",step,amp_output);
//                continue;
//            }
//            // 负饱和保护，避免运放输出过载
//            if(amp_output < 0.10)
//            {
//                DAC_VOLT -= 0.01;
//                printf("【负饱和】DAC+%.4f | OUT=%.3fV\r\n",step,amp_output);
//                continue;
//            }

//            // 窄滞回控制逻辑，围绕3.00V中心稳定
//            if (amp_output > v_up_th)
//            {
//                DAC_VOLT += step;
//                printf("DAC+%.4f | 运放=%.3fV\r\n", step, amp_output);
//                stable_cnt = 0;  // 未稳定，重置计数
//            }
//            else if (amp_output < v_down_th)
//            {
//                DAC_VOLT -= step;
//                printf("DAC-%.4f | 运放=%.3fV\r\n", step, amp_output);
//                stable_cnt = 0;  // 未稳定，重置计数
//            }
//            else
//            {
//                // 进入滞回区间，计数稳定次数
//                stable_cnt++;
//                // 连续20次稳定，判定校准完成，锁定状态
//                if(stable_cnt >= 5)
//                {
//                    printf("\r\n==== 校准锁定完成 彻底停止 ====\r\n");
//                    printf("最终偏置=%.3fV | 最终DAC=%.3fV\r\n",amp_output,DAC_VOLT);
//                    goto calibrate_ok;
//                }
//                // 打印稳定状态，便于调试观察
//                printf("?稳定中 运放=%.3fV 无调节\r\n",amp_output);
//            }
//        }
//calibrate_ok:
//        break;  // 退出校准，锁定参数
//    }
//}


void calibrateBias(void)
{
    // ====================== 先定义 ADC 数组（避免栈被踩）======================
   

	
s16 DB_data[6] ;
    // 后面再定义其他变量
    double DAC_VOLT;
    double amp_output;
    uint8_t stable_cnt = 0;

    // 超细步长、限幅、滞回区间
    const double step = 0.0001;
    const double dac_min = 0.0;
    const double dac_max = 4.2;
    const double v_up_th  = 3.1;   // 上阈值
    const double v_down_th= 2.9;   // 下阈值

    // ====================== 读取 ADC（只读一次，正确时序）======================
 
	
	    AD7606_StartConvst();
	   	delay_ms(500); 
      AD7606_ReadData(DB_data );
	   AD7606_StartConvst();
    AD7606_ReadData(DB_data);
	

    // 打印所有通道（和你正确的那段代码完全一致！）
    printf("1: %.4f V\r\n", AD7606B_Digital2Voltage(DB_data[0]));
    printf("2: %.4f V\r\n", AD7606B_Digital2Voltage(DB_data[1]));
    printf("3: %.4f V\r\n", AD7606B_Digital2Voltage(DB_data[2]));
    printf("4: %.4f V\r\n", AD7606B_Digital2Voltage(DB_data[3]));
    printf("5: %.4f V\r\n", AD7606B_Digital2Voltage(DB_data[4]));
    printf("6: %.4f V\r\n", AD7606B_Digital2Voltage(DB_data[5]));
    printf("\r\n");

    // ====================== 初始 DAC 电压 ======================
    DAC_VOLT = AD7606B_Digital2Voltage(DB_data[3]);
    printf("初始DAC预设电压 = %.4fV\r\n\n", DAC_VOLT);

    // ====================== 主校准循环 ======================
    while(1)
    {
        stable_cnt = 0;

        while (1)
        {
            Watchdog_Refresh();

            // DAC 限幅保护
            if(DAC_VOLT < dac_min) DAC_VOLT = dac_min;
            if(DAC_VOLT > dac_max) DAC_VOLT = dac_max;

            // 输出 DAC
            DAC8830_set_Voltage(DAC_VOLT);
            HAL_Delay(10);  // 运放稳定时间

            // ====================== 关键：正确读取 ADC ======================
            AD7606_StartConvst();
            AD7606_ReadData(DB_data);

            // 读取运放输出（通道2）
            amp_output = AD7606B_Digital2Voltage(DB_data[1]);

            // ====================== 饱和保护 ======================
            if(amp_output > 4.30)
            {
                DAC_VOLT += 0.01;
                printf("【正饱和】DAC=%.4f | OUT=%.4fV\r\n", DAC_VOLT, amp_output);
                continue;
            }

            if(amp_output < 0.10)
            {
                DAC_VOLT -= 0.01;
                printf("【负饱和】DAC=%.4f | OUT=%.4fV\r\n", DAC_VOLT, amp_output);
                continue;
            }

            // ====================== 滞回调节 ======================
            if (amp_output > v_up_th)
            {
                DAC_VOLT += step;
                printf("上调 +%.4f | DAC=%.4f | 运放=%.4fV\r\n", step, DAC_VOLT, amp_output);
                stable_cnt = 0;
            }
            else if (amp_output < v_down_th)
            {
                DAC_VOLT -= step;
                printf("下调 -%.4f | DAC=%.4f | 运放=%.4fV\r\n", step, DAC_VOLT, amp_output);
                stable_cnt = 0;
            }
            else
            {
                // 稳定区间
                stable_cnt++;
                if(stable_cnt >= 10)
                {
                    printf("\r\n=========================================\r\n");
                    printf("        校准锁定完成！彻底停止\r\n");
                    printf(" 最终运放输出 = %.4f V\r\n", amp_output);
                    printf(" 最终DAC电压  = %.4f V\r\n", DAC_VOLT);
                    printf("=========================================\r\n\r\n");
                    goto calibrate_ok;
                }
                printf("? 稳定中：运放=%.4fV  稳定次数=%d\r\n", amp_output, stable_cnt);
            }
        }

calibrate_ok:
        break;
    }
}



//辅助函数
/*----- 5200电源控制函数 -----*/


	void Power5200_Init(void)   //  配置控制5200开关的GPIO
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  
  /* 1. 使能GPIOC时钟 */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  
  /* 2. 配置PC2引脚 */
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;      // 推挽输出
  GPIO_InitStruct.Pull = GPIO_PULLUP;              // 上拉
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH; // 高速模式
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  
  /* 3. 默认输出高电平 */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET);
}






// 开启5200电源（PC2高电平）
void Power5200_Enable(void) {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET); 
}

// 关闭5200电源（PC2低电平）
void Power5200_Disable(void) {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET); 
}




	void RELAY_Init(void)   
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  

  __HAL_RCC_GPIOB_CLK_ENABLE();
  

  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;      // 推挽输出
  GPIO_InitStruct.Pull = GPIO_PULLUP;              // 上拉
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH; // 高速模式
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  
  /* 3. 默认输出低电平 */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
}


// 开启继电器
void RELAY_Enable(void) {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET); 
}

// 关闭继电器
void RELAY_Disable(void) {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET); 
}


void FAN_Init(void)   //  
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  

  __HAL_RCC_GPIOC_CLK_ENABLE();
  

  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;      // 推挽输出
  GPIO_InitStruct.Pull = GPIO_PULLUP;              // 上拉
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH; // 高速模式
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  
  /* 3. 默认输出低电平 */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET);
}


// 开启风扇
void FAN_Enable(void) {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET); 
}

// 关闭风扇
void FAN_Disable(void) {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET); 
}




