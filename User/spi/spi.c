/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    spi.c
 * @brief   This file provides code for the configuration
 *          of the SPI instances.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "spi.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */
SPI_HandleTypeDef hspi1;   // 给 ADS131 使用
SPI_HandleTypeDef hspi6;
/* SPI6 init function */
void MX_SPI6_Init(void) {

  /* USER CODE BEGIN SPI6_Init 0 */

  /* USER CODE END SPI6_Init 0 */

  /* USER CODE BEGIN SPI6_Init 1 */

  /* USER CODE END SPI6_Init 1 */
  hspi6.Instance = SPI6;
  hspi6.Init.Mode = SPI_MODE_MASTER;
  hspi6.Init.Direction = SPI_DIRECTION_2LINES_TXONLY;
  hspi6.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi6.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi6.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi6.Init.NSS = SPI_NSS_SOFT;
  hspi6.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi6.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi6.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi6.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi6.Init.CRCPolynomial = 0x0;
  hspi6.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  hspi6.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi6.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi6.Init.TxCRCInitializationPattern =
      SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi6.Init.RxCRCInitializationPattern =
      SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi6.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi6.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi6.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi6.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi6.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  HAL_SPI_Init(&hspi6);
  /* USER CODE BEGIN SPI6_Init 2 */

  /* USER CODE END SPI6_Init 2 */
}

void MX_SPI1_Init(void)
{
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;

  HAL_SPI_Init(&hspi1);
}




//void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
//{
//    GPIO_InitTypeDef GPIO_InitStruct = {0};
//    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

//    /* ------------ SPI6 (用于 DAC8830) ------------- */
//    if (hspi->Instance == SPI6) {
//        /* 时钟源配置（如果 CubeMX 生成了这部分，请保持一致） */
//        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SPI6;
//        PeriphClkInitStruct.Spi6ClockSelection = RCC_SPI6CLKSOURCE_D3PCLK1;
//        HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

//        /* SPI6 时钟使能 */
//        __HAL_RCC_SPI6_CLK_ENABLE();

//        /* SPI6 GPIO: SCK PB3, MISO PB4 (示例), MOSI PB5 */
//        __HAL_RCC_GPIOB_CLK_ENABLE();
//        GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5; // 确保 PB4 是 MISO
//        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
//        GPIO_InitStruct.Pull = GPIO_NOPULL;
//        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
//        GPIO_InitStruct.Alternate = GPIO_AF8_SPI6;
//        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
//        return;
//    }

//    /* ------------ SPI1 (你的手写 SPI 初始化使用的) ------------- */
//    if (hspi->Instance == SPIx) {
//        /* 使能 GPIO 时钟与 SPI1 时钟 */
//        SPI_MISO_GPIO_CLK_ENABLE();
//        SPI_MOSI_GPIO_CLK_ENABLE();
//        SPI_CLK_GPIO_CLK_ENABLE();
//        SPI_CLK_ENABLE();

//        /* SPI1 时钟源设定（按你原代码） */
//        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SPI1;
//        PeriphClkInitStruct.Spi123ClockSelection = RCC_SPI123CLKSOURCE_PLL;
//        HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

//        /* 配置 MISO (PA6)、MOSI (PA7)、SCK (PA5) 等 */
//        GPIO_InitStruct.Pin = SPI_MISO_PIN;
//        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
//        GPIO_InitStruct.Pull = GPIO_PULLUP;
//        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
//        GPIO_InitStruct.Alternate = SPI_MISO_AF;
//        HAL_GPIO_Init(SPI_MISO_GPIO_PORT, &GPIO_InitStruct);

//        GPIO_InitStruct.Pin = SPI_MOSI_PIN;
//        GPIO_InitStruct.Alternate = SPI_MOSI_AF;
//        HAL_GPIO_Init(SPI_MOSI_GPIO_PORT, &GPIO_InitStruct);

//        GPIO_InitStruct.Pin = SPI_CLK_PIN;
//        GPIO_InitStruct.Alternate = SPI_CLK_AF;
//        HAL_GPIO_Init(SPI_CLK_GPIO_PORT, &GPIO_InitStruct);

//        return;
//    }

//    /* 如果以后还有其它 SPI 实例（SPI2/SPI3...），在这里继续扩展分支 */
//}
void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

    /* ------------ SPI6 (DAC8830) ------------- */
    if (hspi->Instance == SPI6) {
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SPI6;
        PeriphClkInitStruct.Spi6ClockSelection = RCC_SPI6CLKSOURCE_D3PCLK1;
        HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

        __HAL_RCC_SPI6_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();

        GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF8_SPI6;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
        return;
    }

    /* ------------ SPI1 (ADS131A04 使用) ------------- */
    if (hspi->Instance == SPI1) {
        // 开时钟
        __HAL_RCC_SPI1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        // PA5 SCK  PA6 MISO  PA7 MOSI
        GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        return;
    }
}



void HAL_SPI_MspDeInit(SPI_HandleTypeDef *spiHandle) {

  if (spiHandle->Instance == SPI6) {
    /* USER CODE BEGIN SPI6_MspDeInit 0 */

    /* USER CODE END SPI6_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_SPI6_CLK_DISABLE();

    /**SPI6 GPIO Configuration
    PB3     ------> SPI6_SCK
    PB5     ------> SPI6_MOSI
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_3 | GPIO_PIN_5);

    /* USER CODE BEGIN SPI6_MspDeInit 1 */

    /* USER CODE END SPI6_MspDeInit 1 */
  }
}


SPI_HandleTypeDef SPI_Handler;  // SPI句柄

//-----------------------------------------------------------------
// void SPI_Init(void)
//-----------------------------------------------------------------
// 
// 函数功能: SPI驱动程序，配置成主机模式 	
// 入口参数: 无 
// 返 回 值: 无
// 注意事项: 无
//
//-----------------------------------------------------------------
void SPI_Init(void)
{
	SPI_Handler.Instance=SPIx;                         // SP1
	SPI_Handler.Init.Mode=SPI_MODE_MASTER;             // 设置SPI工作模式，设置为主模式
	SPI_Handler.Init.Direction=SPI_DIRECTION_2LINES;   // 设置SPI单向或者双向的数据模式:SPI设置为双线模式
	SPI_Handler.Init.DataSize=SPI_DATASIZE_8BIT;       // 设置SPI的数据大小:SPI发送接收8位帧结构
	SPI_Handler.Init.CLKPolarity=SPI_POLARITY_HIGH;    // 串行同步时钟的空闲状态为高电平
	SPI_Handler.Init.CLKPhase=SPI_PHASE_2EDGE;         // 串行同步时钟的第二个跳变沿（上升或下降）数据被采样
	SPI_Handler.Init.NSS=SPI_NSS_SOFT;                 // NSS信号由硬件（NSS管脚）还是软件（使用SSI位）管理:内部NSS信号有SSI位控制
	SPI_Handler.Init.NSSPMode=SPI_NSS_PULSE_DISABLE;	 // NSS信号脉冲失能
  SPI_Handler.Init.MasterKeepIOState=SPI_MASTER_KEEP_IO_STATE_ENABLE;  // SPI主模式IO状态保持使能
	SPI_Handler.Init.BaudRatePrescaler=SPI_BAUDRATEPRESCALER_8;// 定义波特率预分频的值:波特率预分频值为2,设置为45M时钟,高速模式
	SPI_Handler.Init.FirstBit=SPI_FIRSTBIT_MSB;        // 指定数据传输从MSB位还是LSB位开始:数据传输从MSB位开始
	SPI_Handler.Init.TIMode=SPI_TIMODE_DISABLE;        // 关闭TI模式
	SPI_Handler.Init.CRCCalculation=SPI_CRCCALCULATION_DISABLE;// 关闭硬件CRC校验
	SPI_Handler.Init.CRCPolynomial=7;                  // CRC值计算的多项式
	HAL_SPI_Init(&SPI_Handler);												 // 初始化
	
	__HAL_SPI_ENABLE(&SPI_Handler);                    // 使能SPI1

	SPI1_ReadWriteByte(0Xff);                          // 启动传输
}

//-----------------------------------------------------------------
// void SPI1_SetSpeed(u8 SPI_BaudRatePrescaler)
//-----------------------------------------------------------------
// 
// 函数功能: SPI速度设置函数
// 入口参数: u8 SPI_BaudRatePrescaler：SPI_BAUDRATEPRESCALER_2~SPI_BAUDRATEPRESCALER_2 256
// 返 回 值: 无
// 注意事项: SPI速度=fAPB1/分频系数，fAPB1时钟一般为45Mhz
//
//-----------------------------------------------------------------
void SPI1_SetSpeed(u8 SPI_BaudRatePrescaler)
{
	assert_param(IS_SPI_BAUDRATE_PRESCALER(SPI_BaudRatePrescaler));	// 判断有效性
	__HAL_SPI_DISABLE(&SPI_Handler);            									 	// 关闭SPI
	SPI_Handler.Instance->CR1&=~(0X7<<28);          								// 位3-5清零，用来设置波特率
	SPI_Handler.Instance->CR1|=SPI_BaudRatePrescaler;						 		// 设置SPI速度
	__HAL_SPI_ENABLE(&SPI_Handler);             									 	// 使能SPI 
}

//-----------------------------------------------------------------
// u8 SPI1_ReadWriteByte(u8 TxData)
//-----------------------------------------------------------------
// 
// 函数功能: SPI1读写一个字节
// 入口参数: u8 TxData： 要写入的字节
// 返 回 值: u8 Rxdata：读取到的字节
// 注意事项: 无
//
//-----------------------------------------------------------------
u8 SPI1_ReadWriteByte(u8 TxData)
{
	u8 Rxdata;
	HAL_SPI_TransmitReceive(&SPI_Handler,&TxData,&Rxdata,1, 1000);       
 	return Rxdata;          		    // 返回收到的数据		
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
