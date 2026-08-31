//-----------------------------------------------------------------
// 程序描述:
// 		 ADC驱动程序
// 作    者: 凌智电子
// 开始日期: 2020-11-11
// 完成日期: 2020-11-11
// 修改日期: 
// 当前版本: V2.0
// 历史版本:
//  - V1.0: (2020-11-11)ADC初始化
//  - V2.0: (2020-11-11)增加电压参考缓冲器
// 调试工具: 凌智STM32H750核心板、LZE_ST_LINK2
// 说    明: 
//    
//-----------------------------------------------------------------

//-----------------------------------------------------------------
// 头文件包含
//-----------------------------------------------------------------
#include "adc.h"
#include "delay.h"
#include "led.h"
//-----------------------------------------------------------------

ADC_HandleTypeDef ADC_Handler;// ADC句柄

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








/**
  * @brief  将PA1设置为高阻态 (输入浮空模式)
  * @note   在此模式下，引脚呈现高阻抗，几乎不消耗电流，相当于断开分压电路的地回路
  * @param  无
  * @retval 无
  */

void PA1_Set_HighZ(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOA_CLK_ENABLE(); // 确保GPIOB时钟已开启
    
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;     // 设置为输入模式
    GPIO_InitStruct.Pull = GPIO_NOPULL;         // 不使能上拉或下拉电阻
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // 也可配置为模拟模式以进一步降低功耗: GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
}



/**
  * @brief  将PA1设置为输出低电平 (推挽输出模式)
  * @note   在此模式下，引脚输出稳定的低电平（0V），为分压电阻提供接地路径
  * @param  无
  * @retval 无
  */
void PA1_Set_GND(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOA_CLK_ENABLE(); // 确保GPIOB时钟已开启
    
    // 先设置输出低电平，避免模式切换瞬间产生不确定电平
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
    
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;  // 推挽输出模式
    GPIO_InitStruct.Pull = GPIO_NOPULL;          // 输出模式下通常不需要上下拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; // 低速即可满足分压电路需求
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}







float Get_Pack_Current(void)


		
		{
		
		
		
		  PA1_Set_GND();	
      HAL_Delay(10);
			u16 adcx=Get_Adc_Average(ADC_CHANNEL_16,50);	// 获取通道的转换值，50次取平均
			float  voltage = (adcx / 65535.0f) *2* 2.5f;  // 将ADC值转换为电压值
      float PACK_CURRENT=(4.996f-voltage)*29.403f ;
	//		printf("PACK_CURRENT=  %.3f A\r\n", PACK_CURRENT); // %.3f保留3位小数		
			PA1_Set_HighZ();
      return 	 PACK_CURRENT;
		}		









//-----------------------------------------------------------------
// void MY_ADC_Init(void)
//-----------------------------------------------------------------
//
// 函数功能: 初始化ADC
// 入口参数: 无
// 返回参数: 无
// 注意事项: 无
//
//-----------------------------------------------------------------
void MY_ADC_Init(void)
{ 
	ADC_Handler.Instance=ADC_Instance;
	ADC_Handler.Init.ClockPrescaler=ADC_CLOCK_SYNC_PCLK_DIV4;   // 分频4
	ADC_Handler.Init.Resolution=ADC_RESOLUTION_16B;             // 16位模式
	ADC_Handler.Init.ScanConvMode=DISABLE;                      // 非扫描模式
	ADC_Handler.Init.EOCSelection=DISABLE;                      // 关闭EOC中断
	ADC_Handler.Init.LowPowerAutoWait=DISABLE;									// 自动低功耗关闭
	ADC_Handler.Init.ContinuousConvMode=DISABLE;                // 关闭连续转换
	ADC_Handler.Init.NbrOfConversion=1;                         // 1个转换在规则序列中 也就是只转换规则序列1 
	ADC_Handler.Init.DiscontinuousConvMode=DISABLE;             // 禁止不连续采样模式
	ADC_Handler.Init.NbrOfDiscConversion=0;                     // 不连续采样通道数为0
	ADC_Handler.Init.ExternalTrigConv=ADC_SOFTWARE_START;       // 软件触发
	ADC_Handler.Init.ExternalTrigConvEdge=ADC_EXTERNALTRIGCONVEDGE_NONE;// 使用软件触发
	ADC_Handler.Init.BoostMode=ENABLE;													// BOOT模式关闭
	ADC_Handler.Init.Overrun=ADC_OVR_DATA_OVERWRITTEN;					// 有新的数据的死后直接覆盖掉旧数据
	ADC_Handler.Init.OversamplingMode=DISABLE;									// 过采样关闭
	ADC_Handler.Init.ConversionDataManagement=ADC_CONVERSIONDATA_DR;  	// 规则通道的数据仅仅保存在DR寄存器里面
	

	
	HAL_ADC_Init(&ADC_Handler);                                 // 初始化 
	
	HAL_ADCEx_Calibration_Start(&ADC_Handler,ADC_CALIB_OFFSET,ADC_SINGLE_ENDED); // ADC校准
}

//-----------------------------------------------------------------
// void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
//-----------------------------------------------------------------
//
// 函数功能: ADC底层驱动，引脚配置，时钟使能
// 入口参数: ADC_HandleTypeDef* hadc：ADC句柄
// 返回参数: 无
// 注意事项: 此函数会被HAL_ADC_Init()调用
//
//-----------------------------------------------------------------
void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
	GPIO_InitTypeDef GPIO_Initure;
	
	ADC_CLK_ENABLE();     	 // 使能ADC1时钟
	ADC_GPIO_CLK_ENABLE();	 // 开启ADC的GPIO时钟
	__HAL_RCC_ADC_CONFIG(RCC_ADCCLKSOURCE_CLKP);	// ADC外设时钟选择

 	GPIO_Initure.Pin=ADC_PIN;         	 // PA0
	GPIO_Initure.Mode=GPIO_MODE_ANALOG;  // 模拟
	GPIO_Initure.Pull=GPIO_NOPULL;       // 不带上下拉
	HAL_GPIO_Init(ADC_GPIO_PORT,&GPIO_Initure);

}

//-----------------------------------------------------------------
// u16 Get_Adc(u32 ch)
//-----------------------------------------------------------------
//
// 函数功能: 获得ADC值
// 入口参数: u32 ch：通道
// 返回参数: 转换结果
// 注意事项: 此函数会被HAL_ADC_Init()调用
//
//-----------------------------------------------------------------
u16 Get_Adc(u32 ch)   
{
	ADC_ChannelConfTypeDef ADC1_ChanConf;
	
	ADC1_ChanConf.Channel=ch;                            // 通道
	ADC1_ChanConf.Rank=ADC_REGULAR_RANK_1;               // 第1个序列，序列1
	ADC1_ChanConf.SamplingTime=ADC_SAMPLETIME_64CYCLES_5;// 采样时间
	ADC1_ChanConf.SingleDiff=ADC_SINGLE_ENDED;  				 // 单边采集          		
	ADC1_ChanConf.OffsetNumber=ADC_OFFSET_NONE;   			 // 无偏移校正
	ADC1_ChanConf.Offset=0;                 						 // 偏移为零
	HAL_ADC_ConfigChannel(&ADC_Handler,&ADC1_ChanConf);  // 通道配置
	
	HAL_ADC_Start(&ADC_Handler);                         // 开启ADC

	HAL_ADC_PollForConversion(&ADC_Handler,10);          // 轮询转换
 
	return (u16)HAL_ADC_GetValue(&ADC_Handler);	         // 返回最近一次ADC1规则组的转换结果
}

//-----------------------------------------------------------------
// u16 Get_Adc_Average(u32 ch,u8 times)
//-----------------------------------------------------------------
//
// 函数功能: 获取指定通道的转换值，取times次,然后平均 
// 入口参数: u32 ch：通道
//					 u8 times：获取次数
// 返回参数: 通道ch的times次转换结果平均值
// 注意事项: 此函数会被HAL_ADC_Init()调用
//
//-----------------------------------------------------------------
u16 Get_Adc_Average(u32 ch,u8 times)
{
	u32 temp_val=0;
	u8 t;
	for(t=0;t<times;t++)
	{
		temp_val+=Get_Adc(ch);
		delay_ms(5);
	}
	return temp_val/times;
} 

//-----------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------
