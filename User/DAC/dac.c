//-----------------------------------------------------------------
// 程序描述:
// 		 DAC驱动程序
// 作    者: 凌智电子
// 开始日期: 2020-11-11
// 完成日期: 2020-11-11
// 修改日期: 
// 当前版本: V1.0
// 历史版本:
//  - V1.0: (2018-08-04)DAC初始化，DAC输出电压设置
// 调试工具: 凌智STM32H750核心板、LZE_ST_LINK2
// 说    明: 
//    
//-----------------------------------------------------------------

//-----------------------------------------------------------------
// 头文件包含
//-----------------------------------------------------------------
#include "dac.h"
#include "led.h"
#include "timer.h"
#include "SineWave.h"

//-----------------------------------------------------------------

DAC_HandleTypeDef DAC_Handler;// DAC句柄
static DMA_HandleTypeDef      hdma_dac1;

extern volatile float Relative_Amplitude;
extern volatile uint8_t ADC_status;
extern volatile uint64_t Sampling_Count;					// 采样点数计数

extern uint16_t SIN_DATA;

//-----------------------------------------------------------------
// void MY_VREFBUF_Init(void)
//-----------------------------------------------------------------
//
// 函数功能: 开启电压参考缓冲器
// 入口参数: uint32_t VoltageScaling：
//           SYSCFG_VREFBUF_VOLTAGE_SCALE0：2.048V（要求VDDA等于或高于2.4 V）；
//           SYSCFG_VREFBUF_VOLTAGE_SCALE1：2.5V  （要求VDDA等于或高于2.8 V）；
//           SYSCFG_VREFBUF_VOLTAGE_SCALE2：1.5V  （要求VDDA等于或高于1.8 V）；
//           SYSCFG_VREFBUF_VOLTAGE_SCALE3：1.8V  （要求VDDA等于或高于2.1 V）；
// 返回参数: 无
// 注意事项: 无
//
//-----------------------------------------------------------------
void MY_VREFBUF_Init(uint32_t VoltageScaling)
{ 
	__HAL_RCC_VREF_CLK_ENABLE();	 // 使能VREF时钟
	__HAL_RCC_SYSCFG_CLK_ENABLE(); // 使能SYSCFG的操作时钟
	HAL_SYSCFG_VREFBUF_VoltageScalingConfig(VoltageScaling);	// 设置电压参考缓冲器的电压
	HAL_SYSCFG_VREFBUF_HighImpedanceConfig(SYSCFG_VREFBUF_HIGH_IMPEDANCE_DISABLE);	// VREF+引脚连接到VREFBUF缓冲器输出
	while(HAL_SYSCFG_EnableVREFBUF()!=HAL_OK)	// 使能内部电压参考缓冲器
	{
		HAL_Delay(500);
	
		 LED_Red_Toggle();
	}
}

//-----------------------------------------------------------------
// void DAC_Init(void)
//-----------------------------------------------------------------
//
// 函数功能: 初始化DAC
// 入口参数: 无
// 返回参数: 无
// 注意事项: 无
//
//-----------------------------------------------------------------
void DAC_Init(uint16_t *wave_data)
{
	DAC_ChannelConfTypeDef DAC_Config;
	
	__HAL_RCC_DMA1_CLK_ENABLE();
	
	DAC_Handler.Instance=DAC1;
	HAL_DAC_Init(&DAC_Handler);  // 初始化DAC1
	
	DAC_Config.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE;     // 关闭采样保持模式，这个模式主要用于低功耗 */
	DAC_Config.DAC_Trigger=DAC_TRIGGER_T6_TRGO;             			// 采用定时器6触发
	DAC_Config.DAC_OutputBuffer=DAC_OUTPUTBUFFER_ENABLE;					// DAC1输出缓冲开启
	DAC_Config.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_DISABLE;// 不将DAC连接到片上外设
	DAC_Config.DAC_UserTrimming = DAC_TRIMMING_FACTORY;           // 使用出厂校准
	HAL_DAC_ConfigChannel(&DAC_Handler,&DAC_Config,DAC_CHANNEL);	// DAC通道1配置

	
	// 配置DAC通道1的DMA
	hdma_dac1.Instance = DMA1_Stream0;              							// 使用的DAM1 Stream0
	hdma_dac1.Init.Request  = DMA_REQUEST_DAC1;     							// DAC触发DMA传输
	hdma_dac1.Init.Direction = DMA_MEMORY_TO_PERIPH;							// 存储器到外设
	hdma_dac1.Init.PeriphInc = DMA_PINC_DISABLE;    							// 外设地址禁止自增
	hdma_dac1.Init.MemInc = DMA_MINC_ENABLE;        							// 存储器地址自增
	hdma_dac1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD; // 外输操作数据宽度，半字
	hdma_dac1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;    // 存储器操作数据宽度，半字
	hdma_dac1.Init.Mode = DMA_CIRCULAR;                           // 循环模式
	hdma_dac1.Init.Priority = DMA_PRIORITY_HIGH;                  // 优先级高

	HAL_DMA_Init(&hdma_dac1);
	
	__HAL_LINKDMA(&DAC_Handler, DMA_Handle1, hdma_dac1);					// 关联DMA句柄到DAC句柄下
	
	
//	DAC_Config.DAC_Trigger = DAC_TRIGGER_NONE;  // ?????
//  HAL_DAC_ConfigChannel(&DAC_Handler, &DAC_Config, DAC_CHANNEL_2);
//	HAL_DAC_Start(&DAC_Handler, DAC_CHANNEL_2);
	//HAL_DAC_SetValue(&DAC_Handler, DAC_CHANNEL_2, DAC_ALIGN_12B_R, 2048);
	//DAC2_SetValue(4095);
	
	// 启动DAC DMA
	HAL_DAC_Start_DMA(&DAC_Handler, DAC_CHANNEL_1, (uint32_t*)wave_data, (uint32_t)SIN_DATA, DAC_ALIGN_12B_R);
	
	
	

}

void DAC2_SetValue(uint16_t value){
	HAL_DAC_SetValue(&DAC_Handler, DAC_CHANNEL_2, DAC_ALIGN_12B_R, value);
}


void Restart_DAC_Wave(uint16_t *wave_data, uint16_t SIN_DATA_Need_count) {
    HAL_TIM_Base_Stop(&TIM6_Handler);
    HAL_DAC_Stop_DMA(&DAC_Handler, DAC_CHANNEL_1);
//		// 生成幅值为80%量程，直流偏置50%的正弦波
//    for (uint32_t i = 0; i < 256; i++) {  
//        printf(" %u\n", wave_data[i]);
//    }
		HAL_Delay(500);
    HAL_DAC_Start_DMA(&DAC_Handler, DAC_CHANNEL_1, (uint32_t*)wave_data, (uint32_t)SIN_DATA_Need_count, DAC_ALIGN_12B_R);
    HAL_TIM_Base_Start(&TIM6_Handler);
		ADC_status = 1;
		Sampling_Count = 0;
}
//-----------------------------------------------------------------
// void HAL_DAC_MspInit(DAC_HandleTypeDef* hdac)
//-----------------------------------------------------------------
//
// 函数功能: DAC底层驱动，时钟配置，引脚 配置
// 入口参数: DAC_HandleTypeDef* hdac：hdac:DAC句柄
// 返回参数: 无
// 注意事项: 此函数会被HAL_DAC_Init()调用
//
//-----------------------------------------------------------------
void HAL_DAC_MspInit(DAC_HandleTypeDef* hdac)
{      
	GPIO_InitTypeDef GPIO_Initure;
	DAC_CLK_ENABLE();         				  // 使能DAC时钟
	DAC_GPIO_CLK_ENABLE();							// 开启GPIOA时钟

	GPIO_Initure.Pin=DAC_PIN;        		// PA4
	GPIO_Initure.Mode=GPIO_MODE_ANALOG; // 模拟
	GPIO_Initure.Pull=GPIO_NOPULL;      // 不带上下拉
	HAL_GPIO_Init(DAC_GPIO_PORT,&GPIO_Initure);
	
//	GPIO_Initure.Pin=DAC2_PIN;        		// PA5
//	GPIO_Initure.Mode=GPIO_MODE_ANALOG; // 模拟
//	GPIO_Initure.Pull=GPIO_NOPULL;      // 不带上下拉
//	HAL_GPIO_Init(DAC_GPIO_PORT,&GPIO_Initure);
	
	
}












/**
  * @brief  初始化DAC1以输出恒定电压
  * @note   此函数基于原有波形生成函数修改，主要变更触发模式并移除DMA
  *         确保DAC_Handler等全局变量已定义
  * @param  无
  * @retval 无
  */
void DAC1_Init_Constant(void)
{
    DAC_ChannelConfTypeDef DAC_Config; // 使用你原有的结构体变量
    
    // 注意：这里假设DAC和GPIO时钟已在DAC_Init()中开启，故不再重复使能
    // 如果你的项目结构要求独立初始化，请取消以下注释：
    // __HAL_RCC_DAC_CLK_ENABLE();
    // __HAL_RCC_GPIOA_CLK_ENABLE(); // 假设使用PA4对应DAC通道1
    
    DAC_Handler.Instance = DAC1;
    HAL_DAC_Init(&DAC_Handler);  // 初始化DAC1
    
    // 关键修改：配置DAC通道为无触发模式
    DAC_Config.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE;
    DAC_Config.DAC_Trigger = DAC_TRIGGER_NONE;              // 核心变更：不使用硬件触发[1,2](@ref)
    DAC_Config.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;  // 保持与你原有设置一致
    DAC_Config.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_DISABLE;
    DAC_Config.DAC_UserTrimming = DAC_TRIMMING_FACTORY;
    
    HAL_DAC_ConfigChannel(&DAC_Handler, &DAC_Config, DAC_CHANNEL_1); // 配置通道1
    
    // 移除了所有DMA配置代码，因为恒定电压输出不需要DMA
    
    // 启动DAC通道（不启动DMA）[5,6](@ref)
    HAL_DAC_Start(&DAC_Handler, DAC_CHANNEL_1);
}




/**
  * @brief  设置DAC1输出的电压值
  * @param  voltage: 期望输出的电压值（单位：伏特）
  *         例如：1.65f 表示1.65伏特（假设参考电压VDDA=3.3V）
  * @retval 无
  */
void DAC1_Set_Voltage(float voltage)
{
    uint32_t dac_value;
    DAC1_Init_Constant();
    // 将电压值转换为12位DAC数值（0-4095）[6](@ref)
    // 假设VDDA=3.3V，可根据你的实际硬件调整
    if(voltage > 3.3f) voltage = 3.3f;
    dac_value = (uint32_t)((voltage / 3.3f) * 4095);
    
    // 将数值写入DAC数据保持寄存器[5](@ref)
    HAL_DAC_SetValue(&DAC_Handler, DAC_CHANNEL_1, DAC_ALIGN_12B_R, dac_value);
}



void OutputConstantCurrent(double current)  //输出一个恒流，单位A
{

    
	  DAC1_Set_Voltage((current -0.137851)/0.509389);

}






//-----------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------
