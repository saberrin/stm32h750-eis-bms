//-----------------------------------------------------------------
// 程序描述:
// 		 定时器中断驱动程序
// 作    者: 凌智电子
// 开始日期: 2020-11-11
// 完成日期: 2020-11-11
// 修改日期: 
// 当前版本: V1.0
// 历史版本:
//  - V1.0: (2020-11-11)定时器中断初始化，定时器中断服务函数
// 调试工具: 凌智STM32H750核心板、LZE_ST_LINK2
// 说    明: 
//    
//-----------------------------------------------------------------

//-----------------------------------------------------------------
// 头文件包含
//-----------------------------------------------------------------
#include "timer.h"
#include "led.h"
#include "dac.h"

//-----------------------------------------------------------------

TIM_HandleTypeDef TIM6_Handler;      // 定时器6句柄 

//-----------------------------------------------------------------
// vvoid TIM6_Init(u16 arr,u16 psc)
//-----------------------------------------------------------------
//
// 函数功能: 定时器2中断初始化
// 入口参数: u16 arr：自动重装值
//					 u16 psc：时钟预分频数
// 返回参数: 无
// 注意事项: 定时器溢出时间计算方法:Tout=((arr+1)*(psc+1))/Ft us.
//					 Ft=定时器工作频率,单位:Mhz
//					 这里使用的是定时器2!(定时器2挂在APB1上，时钟为HCLK/2)
//
//-----------------------------------------------------------------
void TIM6_Init(u16 arr,u16 psc)
{  
	TIM_MasterConfigTypeDef sMasterConfig = {0};
	
  TIM6_Handler.Instance=TIM6;                          		// 通用定时器6
  TIM6_Handler.Init.Prescaler=psc;                     		// 分频系数
  TIM6_Handler.Init.CounterMode=TIM_COUNTERMODE_UP;    		// 向上计数器
  TIM6_Handler.Init.Period=arr;                        		// 自动装载值
  TIM6_Handler.Init.ClockDivision=TIM_CLOCKDIVISION_DIV1;	// 时钟分频因子
  HAL_TIM_Base_Init(&TIM6_Handler); 
	
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	HAL_TIMEx_MasterConfigSynchronization(&TIM6_Handler, &sMasterConfig);
	
	// ????:?????????
  HAL_TIM_Base_Start_IT(&TIM6_Handler);          // ??????????

  // ??NVIC???(???????????)
  HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 0, 0);     // ???????
  HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);             // ??TIM6????
}

//-----------------------------------------------------------------
// void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
//-----------------------------------------------------------------
//
// 函数功能: 定时器底册驱动，开启时钟，设置中断优先级
// 入口参数: TIM_HandleTypeDef *htim：定时器3句柄
// 返回参数: 无
// 注意事项: 此函数会被HAL_TIM_Base_Init()函数调用
//
//-----------------------------------------------------------------
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
  if(htim->Instance==TIM6)
	{
		__HAL_RCC_TIM6_CLK_ENABLE();            // 使能TIM6时钟
	}
}

//-----------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------
