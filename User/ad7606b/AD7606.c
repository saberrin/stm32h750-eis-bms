//-----------------------------------------------------------------
// 程序描述:
//     AD7606并行驱动程序
// 作    者: 凌智电子
// 开始日期: 2020-11-11
// 完成日期: 2020-11-11
// 修改日期: 
// 当前版本: V1.0
// 历史版本:
//  - V1.0:  AD7606驱动程序
// 调试工具: 凌智STM32H750核心板、LZE_ST_LINK2
// 说    明: 
//    
//-----------------------------------------------------------------

//-----------------------------------------------------------------
// 头文件包含
//-----------------------------------------------------------------
#include "AD7606.h"
#include "delay.h"
#include <stdbool.h>
#include <stdio.h>
#include "w25qxx.h"
#include <string.h>  // 必须加这个头文件
//static AD7606_RangeTypeDef currentRange; // 用于记录当前量程设置



//void AD7606_RANGE_GPIO_Init(void)
//{
//  GPIO_InitTypeDef GPIO_InitStruct = {0};

////  /* 使能GPIOD时钟 */
////  __HAL_RCC_GPIOD_CLK_ENABLE();

////  /* 配置PD7为推挽输出模式 */
////  GPIO_InitStruct.Pin = AD7606_RANGE_PIN;
////  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
////  GPIO_InitStruct.Pull = GPIO_NOPULL; // 通常无需上拉下拉
////  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; // 高速
////  HAL_GPIO_Init(AD7606_RANGE_GPIO_PORT, &GPIO_InitStruct);

////  /* 默认设置为±10V量程（根据您的RANGE引脚硬件逻辑调整） */
////  /* 注意：RANGE引脚为高电平时，量程为±10V；低电平时为±5V [1,3](@ref) */
////  HAL_GPIO_WritePin(AD7606_RANGE_GPIO_PORT, AD7606_RANGE_PIN, GPIO_PIN_SET);
////  currentRange = AD7606_RANGE_5V;

//  // HAL_Delay(1);
//}

///* 量程设置函数 */
//void AD7606_SetRange(AD7606_RangeTypeDef range)
//{
//  if (range == AD7606_RANGE_5V) {
//    HAL_GPIO_WritePin(AD7606_RANGE_GPIO_PORT, AD7606_RANGE_PIN, GPIO_PIN_RESET); // 设置为±5V量程
//    currentRange = AD7606_RANGE_5V;
//  } else { // AD7606_RANGE_10V
//    HAL_GPIO_WritePin(AD7606_RANGE_GPIO_PORT, AD7606_RANGE_PIN, GPIO_PIN_SET); // 设置为±10V量程
//    currentRange = AD7606_RANGE_10V;
//  }

//  /* 重要：根据数据手册，在转换期间不建议更改RANGE引脚的状态 [1](@ref) */
//  /* 因此，最好在非转换阶段（例如一次采样周期结束后）调用此函数 */
//}

///* 获取当前量程设置（可选，便于状态查询） */
//AD7606_RangeTypeDef AD7606_GetRange(void)
//{
//  return currentRange;
//}




/* DBx引脚端口 */
GPIO_TypeDef *port_list[] = {
    AD7606B_DB0_GPIO_Port,  AD7606B_DB1_GPIO_Port,  AD7606B_DB2_GPIO_Port,
    AD7606B_DB3_GPIO_Port,  AD7606B_DB4_GPIO_Port,  AD7606B_DB5_GPIO_Port,
    AD7606B_DB6_GPIO_Port,  AD7606B_DB7_GPIO_Port,  AD7606B_DB8_GPIO_Port,
    AD7606B_DB9_GPIO_Port,  AD7606B_DB10_GPIO_Port, AD7606B_DB11_GPIO_Port,
    AD7606B_DB12_GPIO_Port, AD7606B_DB13_GPIO_Port, AD7606B_DB14_GPIO_Port,
    AD7606B_DB15_GPIO_Port,
};

/* DBx引脚 */
uint16_t pin_list[] = {
    AD7606B_DB0_Pin,  AD7606B_DB1_Pin,  AD7606B_DB2_Pin,  AD7606B_DB3_Pin,
    AD7606B_DB4_Pin,  AD7606B_DB5_Pin,  AD7606B_DB6_Pin,  AD7606B_DB7_Pin,
    AD7606B_DB8_Pin,  AD7606B_DB9_Pin,  AD7606B_DB10_Pin, AD7606B_DB11_Pin,
    AD7606B_DB12_Pin, AD7606B_DB13_Pin, AD7606B_DB14_Pin, AD7606B_DB15_Pin,
};

/**
 * @brief 获取AD7606B单通道数据
 *
 * @return uint16_t 单通道数据
 */
static uint16_t AD7606B_Get_Pin_Data(void) {
  uint16_t shift = 0x0001;
  uint16_t input_level = 0;
  for (uint8_t i = 0; i < sizeof(pin_list) / sizeof(uint16_t); i++) {
    GPIO_PinState state = HAL_GPIO_ReadPin(port_list[i], pin_list[i]);
    if (state == GPIO_PIN_SET) {
      input_level |= shift;
    } else if (state == GPIO_PIN_RESET) {
      input_level &= (~shift);
    }
    shift <<= 1;
  }
  return input_level;
}


//-----------------------------------------------------------------

//-----------------------------------------------------------------
// void GPIO_AD7606_Configuration(void)
//-----------------------------------------------------------------
// 
// 函数功能: AD7606引脚配置函数
// 入口参数: 无 
// 返 回 值: 无
// 注意事项: 无
//
//-----------------------------------------------------------------

//void GPIO_AD7606_Configuration(void)
//{ 
//    GPIO_InitTypeDef GPIO_InitStruct;

//    // 使能GPIO时钟
//    __HAL_RCC_GPIOA_CLK_ENABLE();
//    __HAL_RCC_GPIOB_CLK_ENABLE();
//    __HAL_RCC_GPIOC_CLK_ENABLE();
//	  __HAL_RCC_GPIOD_CLK_ENABLE();
//    __HAL_RCC_GPIOE_CLK_ENABLE();

//    // 配置 GPIOB 引脚为推挽输出
//    GPIO_InitStruct.Pin = AD7606B_FRD_Pin;                           
//    GPIO_InitStruct.Mode = GPIO_MODE_INPUT; // 推挽输入
//    GPIO_InitStruct.Pull = GPIO_PULLUP;          // 上拉
//    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;  // 高速
//    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);  // 初始化
//    
//	
//	
//	


//    // 配置 GPIOE 引脚为推挽输出
//    GPIO_InitStruct.Pin =  AD7606B_CS_Pin | AD7606B_RD_Pin;                       
//    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;  // 推挽输出
//    GPIO_InitStruct.Pull = GPIO_PULLUP;          // 上拉
//    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;  // 高速
//    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);  // 初始化
//    // AD7606B_CONV_Pin -> PE0
//    // AD7606B_FRD_Pin -> PE1


// // 配置 GPIOB 引脚为推挽输入
//    GPIO_InitStruct.Pin = AD7606B_BUSY_Pin;                           
//    GPIO_InitStruct.Mode = GPIO_MODE_INPUT; // 推挽输入
//    GPIO_InitStruct.Pull = GPIO_PULLUP;       
//    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;  // 高速
//    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);  // 初始化
//    





//    // 配置 GPIOA 引脚为推挽输出
//    GPIO_InitStruct.Pin =  AD7606B_SER_Pin | AD7606B_STBY_Pin ;                      
//    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;  // 推挽输出
//    GPIO_InitStruct.Pull = GPIO_PULLUP;          // 上拉
//    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;  // 高速
//    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);  // 初始化
// 
//	

//    
//    GPIO_InitStruct.Pin = AD7606B_CONV_Pin |AD7606B_WR_Pin |   AD7606B_REST_Pin   ;                       
//    GPIO_InitStruct.Mode =  GPIO_MODE_OUTPUT_PP;    // 输出
//    GPIO_InitStruct.Pull = GPIO_NOPULL;     
//    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;  // 高速
//    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);  // 初始化
//    // AD7606B_BUSY_Pin -> PC13
//		
//		
//		

//		

//    // 配置 GPIOA 引脚为输入 (DB0～DB3 -> PA3～PA0)
//    GPIO_InitStruct.Pin = AD7606B_DB6_Pin  | AD7606B_DB8_Pin ;
//    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;   // 输入
//    GPIO_InitStruct.Pull = GPIO_NOPULL;       // 无上下拉
//    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;  // 高速
//    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);  // 初始化



//    // 配置 GPIOB 引脚为输入 (DB14～DB15 -> PB1～PB0)
//    GPIO_InitStruct.Pin = AD7606B_DB0_Pin | AD7606B_DB7_Pin| AD7606B_DB1_Pin| AD7606B_DB2_Pin| AD7606B_DB12_Pin| AD7606B_DB13_Pin| AD7606B_DB14_Pin| AD7606B_DB15_Pin;
//    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;   // 输入
//    GPIO_InitStruct.Pull = GPIO_NOPULL;       // 无上下拉
//    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;  // 高速
//    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);  // 初始化

//		
//		    // 配置 GPIOC 引脚为输入 (DB4～DB9 -> PC7～PC0)
//    GPIO_InitStruct.Pin = AD7606B_DB4_Pin | AD7606B_DB5_Pin | AD7606B_DB9_Pin | 
//                          AD7606B_DB10_Pin | AD7606B_DB11_Pin ;
//    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;   // 输入
//    GPIO_InitStruct.Pull = GPIO_NOPULL;       // 无上下拉
//    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;  // 高速
//    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);  // 初始化


//    // 配置 GPIOD 引脚为输入 (DB10～DB13 -> PC8～PC11)
//    GPIO_InitStruct.Pin = AD7606B_DB3_Pin;
//    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;   // 输入
//    GPIO_InitStruct.Pull = GPIO_NOPULL;       // 无上下拉
//    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;  // 高速
//    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);  // 初始化

//		
//}

void GPIO_AD7606_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    // 开时钟
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    // ==========================
    // 第一步：清零结构体
    // ==========================
    memset(&GPIO_InitStruct, 0, sizeof(GPIO_InitStruct));

    // ==========================
    // 配置控制输出引脚
    // ==========================

    // PE: CS, RD
    GPIO_InitStruct.Pin = AD7606B_CS_Pin | AD7606B_RD_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    // PA: STBY, SER
    GPIO_InitStruct.Pin = AD7606B_STBY_Pin | AD7606B_SER_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // PC: WR, CONV, RESET
    GPIO_InitStruct.Pin = AD7606B_WR_Pin | AD7606B_CONV_Pin | AD7606B_REST_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    // ==========================
    // 再次清零！防止配置污染
    // ==========================
    memset(&GPIO_InitStruct, 0, sizeof(GPIO_InitStruct));

    // ==========================
    // 配置输入引脚
    // ==========================
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;

    // PE: BUSY
    GPIO_InitStruct.Pin = AD7606B_BUSY_Pin;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    // PB: FRD
    GPIO_InitStruct.Pin = AD7606B_FRD_Pin;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // PA: DB6, DB8
    GPIO_InitStruct.Pin = AD7606B_DB6_Pin | AD7606B_DB8_Pin;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // PB: DB0~DB2, DB7, DB12~15
    GPIO_InitStruct.Pin = AD7606B_DB0_Pin  | AD7606B_DB1_Pin  |
                          AD7606B_DB2_Pin  | AD7606B_DB7_Pin  |
                          AD7606B_DB12_Pin | AD7606B_DB13_Pin |
                          AD7606B_DB14_Pin | AD7606B_DB15_Pin;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // PC: DB4, DB5, DB9~11
    GPIO_InitStruct.Pin = AD7606B_DB4_Pin  | AD7606B_DB5_Pin  |
                          AD7606B_DB9_Pin  | AD7606B_DB10_Pin |
                          AD7606B_DB11_Pin;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    // PD: DB3
    GPIO_InitStruct.Pin = AD7606B_DB3_Pin;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}




//-----------------------------------------------------------------
// void AD7606_Init(void)
//-----------------------------------------------------------------
// 
// 函数功能: AD7606初始化函数
// 入口参数: 无 
// 返 回 值: 无
// 注意事项: 无
//
//-----------------------------------------------------------------
void AD7606_Init(void)
{
	delay_ms(5);
	GPIO_AD7606_Configuration();
//	AD7606_RANGE_GPIO_Init();
	delay_ms(5);
	CO_A_H;
	CO_B_H;
	delay_ms(1);
	STBY_H;
	RD_SCLK_H;
	CS_N_H;	
//  OS10_L;
//	OS11_L;
//	OS12_L;
	AD7606_RESET();  
	delay_ms(1);
	AD7606_StartConvst();	
	// SER PB12 配置低位
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET); 
}

//-----------------------------------------------------------------
// void AD7606_StartConvst(void)
//-----------------------------------------------------------------
// 
// 函数功能: 启动转换
// 入口参数: 无 
// 返 回 值: 无
// 注意事项: 无
//
//-----------------------------------------------------------------
void AD7606_StartConvst(void)
{  
	CO_A_L;	
	CO_B_L;	
	CO_A_H;
	CO_B_H;
}
  
//-----------------------------------------------------------------
// void AD7606_RESET(void) 
//-----------------------------------------------------------------
// 
// 函数功能: 复位
// 入口参数: 无 
// 返 回 值: 无
// 注意事项: 无
//
//-----------------------------------------------------------------
void AD7606_RESET(void) 
{ 
	REST_L;
	AD7606_Delay(10);
	REST_H; 
	delay_us(1);
	REST_L; 
}  

//-----------------------------------------------------------------
// void AD7606_ReadData(s16 * DB_data)
//-----------------------------------------------------------------
// 
// 函数功能: 读取数据 
// 入口参数: s16 * DB_data：结构体指针，该指针为指向结构体数组的首地址 
// 返 回 值: 无
// 注意事项: 无
//
//-----------------------------------------------------------------
void AD7606_ReadData(s16 * DB_data)
{  
	  while((BUSY == GPIO_PIN_SET));
	  CS_N_L; 
		RD_SCLK_L;
		RD_SCLK_H;
		DB_data[0] = AD7606B_Get_Pin_Data();		
		RD_SCLK_L;
		RD_SCLK_H;
		DB_data[1] = AD7606B_Get_Pin_Data();	
		RD_SCLK_L;
		RD_SCLK_H;
		DB_data[2] = AD7606B_Get_Pin_Data();
		RD_SCLK_L;
		RD_SCLK_H;
		DB_data[3] = AD7606B_Get_Pin_Data();
		RD_SCLK_L;
		RD_SCLK_H;
		DB_data[4] = AD7606B_Get_Pin_Data();
		RD_SCLK_L;
		RD_SCLK_H;
		DB_data[5] = AD7606B_Get_Pin_Data();	
	  CS_N_H;		
} 

 







void AD7606_ReadData_2Ch(uint16_t * DB_data1,uint16_t * DB_data2)
{  
	  while((BUSY == GPIO_PIN_SET));
	  CS_N_L; 
		RD_SCLK_L;
		RD_SCLK_H;
		*DB_data1 = AD7606B_Get_Pin_Data();		
//		RD_SCLK_L;
//		RD_SCLK_H;
//		RD_SCLK_L;
//		RD_SCLK_H;
			RD_SCLK_L;
		RD_SCLK_H;
		*DB_data2 = AD7606B_Get_Pin_Data();	
	  CS_N_H;		
} 




void AD7606_Read_Data(uint16_t *data1) {
  GPIO_PinState state = GPIO_PIN_SET;
  do {
    state = HAL_GPIO_ReadPin(AD7606B_BUSY_GPIO_Port, AD7606B_BUSY_Pin);
  } while (state);
  HAL_GPIO_WritePin(AD7606B_CS_GPIO_Port, AD7606B_CS_Pin, GPIO_PIN_RESET);
	// 读取过程 1
  HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_SET);
  data1[0] = AD7606B_Get_Pin_Data();
	// 读取过程 2
	HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_SET);
  data1[1] = AD7606B_Get_Pin_Data();
	// 读取过程 3
	HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_SET);
  data1[2] = AD7606B_Get_Pin_Data();
	// 读取过程 4
	HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_SET);
  data1[3] = AD7606B_Get_Pin_Data();
	// 读取过程 5
	HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_SET);
  data1[4] = AD7606B_Get_Pin_Data();
	// 读取过程 6
	HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_SET);
  data1[5] = AD7606B_Get_Pin_Data();
  HAL_GPIO_WritePin(AD7606B_CS_GPIO_Port, AD7606B_CS_Pin, GPIO_PIN_SET);
}

//void AD7606_Read_Data_mine(uint16_t *data1,uint16_t *data2) {
//  GPIO_PinState state = GPIO_PIN_SET;
//  do {
//    state = HAL_GPIO_ReadPin(AD7606B_BUSY_GPIO_Port, AD7606B_BUSY_Pin);
//  } while (state);
//  HAL_GPIO_WritePin(AD7606B_CS_GPIO_Port, AD7606B_CS_Pin, GPIO_PIN_RESET);
//	// 读取过程 1
//  HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_RESET);
//  HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_SET);
//  *data1 = AD7606B_Get_Pin_Data();
//	// 读取过程 2
//	HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_RESET);
//  HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_SET);
//  *data2 = AD7606B_Get_Pin_Data();
////	// 读取过程 3  无作用
////	HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_RESET);
////  HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_SET);
////  *data1 = AD7606B_Get_Pin_Data();
////	// 读取过程 4  原始信号
////	HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_RESET);
////  HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_SET);
////  uint16_t data0 = AD7606B_Get_Pin_Data();
////	// 读取过程 5  DAC信号
////	HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_RESET);
////  HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_SET);
////  *data1 = data0-AD7606B_Get_Pin_Data() ;
////	// 读取过程 6
////	HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_RESET);
////  HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_SET);
////  //*data2 = AD7606B_Get_Pin_Data();
//  HAL_GPIO_WritePin(AD7606B_CS_GPIO_Port, AD7606B_CS_Pin, GPIO_PIN_SET);
//}




/**
  * @brief  AD7606B数字值转电压值（量程自适应）
  * @param  data: ADC原始数据（16位）
  * @retval 计算得到的电压值（单位：mV）
  * @note   此函数会自动读取当前量程设置并进行相应计算
  */
//double AD7606B_Digital2Voltage(uint16_t data)
//{
//    int16_t signed_data = (int16_t)data;  // 将16位无符号转换为有符号
//    double full_scale_voltage;
//    AD7606_RangeTypeDef current_range;
//    
//    // 获取当前量程设置
//    current_range = AD7606_GetRange();
//    
//    // 根据量程确定满量程电压
//    if(current_range == AD7606_RANGE_5V) {
//        full_scale_voltage = 5.0;   // ±5V量程，峰值电压10V
//    } else {
//        full_scale_voltage = 10.0;  // ±10V量程，峰值电压20V
//    }
//    
//    // 计算公式：电压(mV) = (有符号原始值 / 32768) × 满量程电压(V) × 1000
//    return (signed_data / 32768.0) * full_scale_voltage * 1000.0;
//}

/**
  * @brief  AD7606B数字值转电压值（修正版）
  * @param  data: ADC原始数据（16位）
  * @retval 计算得到的电压值（单位：V）
  * @note   正确处理16位补码数据，返回伏特单位
  */
double AD7606B_Digital2Voltage(uint16_t data)
{
    int16_t signed_data = (int16_t)data;  // 将16位无符号转换为有符号（补码形式）
    double full_scale_voltage;
 //   AD7606_RangeTypeDef current_range;
    double voltage_v;
    
//    // 获取当前量程设置
//    current_range = AD7606_GetRange();
    
    // 根据量程确定满量程电压
 //   if(current_range == AD7606_RANGE_5V) {
        full_scale_voltage = 5.0;   // ±5V量程
 //   } else {
 //      full_scale_voltage = 10.0;  // ±10V量程
//    }
   
    // 正确计算公式：电压(V) = (有符号原始值 / 32768) × 满量程电压
    // 注意：32768 = 2^15，因为16位ADC最高位是符号位
    voltage_v = (signed_data / 32768.0) * full_scale_voltage;
    
    return voltage_v;  // 返回单位：伏特(V)
}

//double AD7606B_Digital2Voltage(uint16_t data) {
//  int16_t signed_data = data;
//  return signed_data * 10.0 / 65536.0 * 1000.0;
//}




//-----------------------------------------------------------------
// void AD7606_Delay(uint16_t t) 
//-----------------------------------------------------------------
// 
// 函数功能: 读取数据 
// 入口参数: uint16_t t：延时数 
// 返 回 值: 无
// 注意事项: 无
//
//-----------------------------------------------------------------
void AD7606_Delay(uint16_t t)
{
	do {
			;
	} while (--t); 
}





























