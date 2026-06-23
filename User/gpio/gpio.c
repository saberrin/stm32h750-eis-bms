///* USER CODE BEGIN Header */
///**
// ******************************************************************************
// * @file    gpio.c
// * @brief   This file provides code for the configuration
// *          of all used GPIO pins.
// ******************************************************************************
// * @attention
// *
// * Copyright (c) 2024 STMicroelectronics.
// * All rights reserved.
// *
// * This software is licensed under terms that can be found in the LICENSE file
// * in the root directory of this software component.
// * If no LICENSE file comes with this software, it is provided AS-IS.
// *
// ******************************************************************************
// */
///* USER CODE END Header */

///* Includes ------------------------------------------------------------------*/
//#include "gpio.h"
//#include "AD7606.h"

///* USER CODE BEGIN 0 */

///* USER CODE END 0 */

///*----------------------------------------------------------------------------*/
///* Configure GPIO                                                             */
///*----------------------------------------------------------------------------*/
///* USER CODE BEGIN 1 */

///* USER CODE END 1 */



///** Configure pins
//     PH0-OSC_IN (PH0)   ------> RCC_OSC_IN
//     PH1-OSC_OUT (PH1)   ------> RCC_OSC_OUT
//     PA13 (JTMS/SWDIO)   ------> DEBUG_JTMS-SWDIO
//     PA14 (JTCK/SWCLK)   ------> DEBUG_JTCK-SWCLK
//*/
//void MX_GPIO_Init(void) {

//  GPIO_InitTypeDef GPIO_InitStruct = {0};

//  /* GPIO Ports Clock Enable */
//  __HAL_RCC_GPIOH_CLK_ENABLE();
//  __HAL_RCC_GPIOC_CLK_ENABLE();
//  __HAL_RCC_GPIOA_CLK_ENABLE();
//  __HAL_RCC_GPIOB_CLK_ENABLE();
//  __HAL_RCC_GPIOE_CLK_ENABLE();
//  __HAL_RCC_GPIOD_CLK_ENABLE();

//  /*Configure GPIO pin Output Level */
//  HAL_GPIO_WritePin(GPIOC,
//                    AD7606B_DB9_Pin | AD7606B_DB8_Pin | AD7606B_DB7_Pin |
//                        AD7606B_DB6_Pin | AD7606B_DB5_Pin | AD7606B_DB4_Pin |
//                        AD7606B_DB11_Pin | AD7606B_DB10_Pin | AD7606B_DB13_Pin |
//                        AD7606B_DB12_Pin,
//                    GPIO_PIN_RESET);

//  /*Configure GPIO pin Output Level */
//  HAL_GPIO_WritePin(GPIOA,
//                    AD7606B_DB3_Pin | AD7606B_DB2_Pin | AD7606B_DB1_Pin |
//                        AD7606B_DB0_Pin | AD7606B_CS_Pin | AD7606B_RD_Pin,
//                    GPIO_PIN_RESET);

//  /*Configure GPIO pin Output Level */
//  HAL_GPIO_WritePin(GPIOB,
//                    AD7606B_DB15_Pin | AD7606B_DB14_Pin | AD7606B_CONV_Pin |
//                        AD7606B_STBY_Pin | AD7606B_SER_Pin | AD7606B_OSI2_Pin |
//                        AD7606B_OSI1_Pin | AD7606B_OSI0_Pin | AD7606B_REST_Pin | GPIO_PIN_6 |
//                        AD7606B_WR_Pin,
//                    GPIO_PIN_RESET);

//  /*Configure GPIO pins : PCPin PCPin PCPin PCPin
//                           PCPin PCPin PCPin PCPin
//                           PCPin PCPin */
//  GPIO_InitStruct.Pin = AD7606B_DB9_Pin | AD7606B_DB8_Pin | AD7606B_DB7_Pin |
//                        AD7606B_DB6_Pin | AD7606B_DB5_Pin | AD7606B_DB4_Pin |
//                        AD7606B_DB11_Pin | AD7606B_DB10_Pin | AD7606B_DB13_Pin |
//                        AD7606B_DB12_Pin;
//  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
//  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

//  /*Configure GPIO pins : PAPin PAPin PAPin PAPin
//                           PAPin PAPin */
//  GPIO_InitStruct.Pin = AD7606B_DB3_Pin | AD7606B_DB2_Pin | AD7606B_DB1_Pin |
//                        AD7606B_DB0_Pin | AD7606B_CS_Pin | AD7606B_RD_Pin;
//  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
//  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

//  /*Configure GPIO pins : PAPin PAPin */
//  GPIO_InitStruct.Pin = AD7606B_FRD_Pin | AD7606B_BUSY_Pin;
//  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

//  /*Configure GPIO pins : PBPin PBPin PBPin PBPin
//                           PBPin PBPin PBPin PBPin
//                           PBPin PBPin */
//  GPIO_InitStruct.Pin = AD7606B_DB15_Pin | AD7606B_DB14_Pin | AD7606B_CONV_Pin |
//                        AD7606B_STBY_Pin | AD7606B_SER_Pin | AD7606B_OSI2_Pin |
//                        AD7606B_OSI1_Pin | AD7606B_OSI0_Pin | GPIO_PIN_15 |AD7606B_REST_Pin |
//                        AD7606B_WR_Pin;
//  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
//  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

//  /*Configure GPIO pin : PB6 */
//  GPIO_InitStruct.Pin = GPIO_PIN_6;
//  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
//	  /*Configure GPIO pin : PB7 */
//  GPIO_InitStruct.Pin = GPIO_PIN_7;
//  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
//	
//}

///* USER CODE BEGIN 2 */
//void AD7606B_Parallel_GPIO_Init(void) {

//  GPIO_InitTypeDef GPIO_InitStruct = {0};

//  /* GPIO Ports Clock Enable */
//  __HAL_RCC_GPIOH_CLK_ENABLE();
//  __HAL_RCC_GPIOC_CLK_ENABLE();
//  __HAL_RCC_GPIOA_CLK_ENABLE();
//  __HAL_RCC_GPIOB_CLK_ENABLE();
//  __HAL_RCC_GPIOE_CLK_ENABLE();
//  __HAL_RCC_GPIOD_CLK_ENABLE();

//  /*Configure GPIO pin Output Level */
//  HAL_GPIO_WritePin(GPIOC,
//                    AD7606B_DB9_Pin | AD7606B_DB8_Pin | AD7606B_DB7_Pin |
//                        AD7606B_DB6_Pin | AD7606B_DB5_Pin | AD7606B_DB4_Pin |
//                        AD7606B_DB11_Pin | AD7606B_DB10_Pin | AD7606B_DB13_Pin |
//                        AD7606B_DB12_Pin,
//                    GPIO_PIN_RESET);

//  /*Configure GPIO pin Output Level */
//  HAL_GPIO_WritePin(GPIOA,
//                    AD7606B_DB3_Pin | AD7606B_DB2_Pin | AD7606B_DB1_Pin |
//                        AD7606B_DB0_Pin | AD7606B_CS_Pin | AD7606B_RD_Pin,
//                    GPIO_PIN_RESET);

//  /*Configure GPIO pin Output Level */
//  HAL_GPIO_WritePin(GPIOB,
//                    AD7606B_DB15_Pin | AD7606B_DB14_Pin | AD7606B_CONV_Pin |
//                        AD7606B_STBY_Pin | AD7606B_SER_Pin | AD7606B_OSI2_Pin |
//                        AD7606B_OSI1_Pin | AD7606B_OSI0_Pin | AD7606B_REST_Pin |
//                        AD7606B_WR_Pin,
//                    GPIO_PIN_RESET);

//  /*Configure GPIO pins : PCPin PCPin PCPin PCPin
//                           PCPin PCPin PCPin PCPin
//                           PCPin PCPin */
//  GPIO_InitStruct.Pin = AD7606B_DB9_Pin | AD7606B_DB8_Pin | AD7606B_DB7_Pin |
//                        AD7606B_DB6_Pin | AD7606B_DB5_Pin | AD7606B_DB4_Pin |
//                        AD7606B_DB11_Pin | AD7606B_DB10_Pin | AD7606B_DB13_Pin |
//                        AD7606B_DB12_Pin;
//  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
//  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

//  /*Configure GPIO pins : PAPin PAPin PAPin PAPin
//                           PAPin PAPin */
//  GPIO_InitStruct.Pin = AD7606B_DB3_Pin | AD7606B_DB2_Pin | AD7606B_DB1_Pin |
//                        AD7606B_DB0_Pin | AD7606B_CS_Pin | AD7606B_RD_Pin;
//  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
//  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

////  /*Configure GPIO pins : PAPin PAPin */
////  GPIO_InitStruct.Pin = AD7606B_FRD_Pin | AD7606B_BUSY_Pin;
////  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
////  GPIO_InitStruct.Pull = GPIO_NOPULL;
////  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
////	
//	
//	//////////////////////////////////////////////////////////////////////////////////
//	  /*Configure GPIO pins : PAPin PAPin */
//  GPIO_InitStruct.Pin = AD7606B_FRD_Pin ;
//  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
//	
//	
//		  /*Configure GPIO pins : PAPin PAPin */
//  GPIO_InitStruct.Pin = AD7606B_BUSY_Pin;
//  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
//	
/////////////////////////////////////////////////////////////////////////////////////////////////////////	
//	
//	

//  /*Configure GPIO pins : PBPin PBPin PBPin PBPin
//                           PBPin PBPin PBPin PBPin
//                           PBPin PBPin */
//  GPIO_InitStruct.Pin = AD7606B_DB15_Pin | AD7606B_DB14_Pin | AD7606B_CONV_Pin |
//                        AD7606B_STBY_Pin | AD7606B_SER_Pin | AD7606B_OSI2_Pin |
//                        AD7606B_OSI1_Pin | AD7606B_OSI0_Pin | AD7606B_REST_Pin |
//                        AD7606B_WR_Pin;
//  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
//  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

//  /*Configure GPIO pin : PB6 */
//  GPIO_InitStruct.Pin = GPIO_PIN_6;
//  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
//}

//void AD7606B_Serial_GPIO_Init(void) {

//  GPIO_InitTypeDef GPIO_InitStruct = {0};

//  /* GPIO Ports Clock Enable */
//  __HAL_RCC_GPIOH_CLK_ENABLE();
//  __HAL_RCC_GPIOC_CLK_ENABLE();
//  __HAL_RCC_GPIOA_CLK_ENABLE();
//  __HAL_RCC_GPIOB_CLK_ENABLE();
//  __HAL_RCC_GPIOE_CLK_ENABLE();
//  __HAL_RCC_GPIOD_CLK_ENABLE();

//  /*Configure GPIO pin Output Level */
//  HAL_GPIO_WritePin(GPIOC, AD7606B_DB11_Pin, GPIO_PIN_RESET);

//  /*Configure GPIO pin Output Level */
//  HAL_GPIO_WritePin(GPIOA, AD7606B_CS_Pin | AD7606B_RD_Pin, GPIO_PIN_RESET);

//  /*Configure GPIO pin Output Level */
//  HAL_GPIO_WritePin(GPIOB,
//                    AD7606B_CONV_Pin | AD7606B_STBY_Pin | AD7606B_SER_Pin |
//                        AD7606B_OSI2_Pin | AD7606B_OSI1_Pin | AD7606B_OSI0_Pin |
//                        AD7606B_REST_Pin,
//                    GPIO_PIN_RESET);

//  /*Configure GPIO pins : PCPin */
//  GPIO_InitStruct.Pin = AD7606B_DB11_Pin;
//  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
//  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

//  /*Configure GPIO pins : PAPin PAPin */
//  GPIO_InitStruct.Pin = AD7606B_CS_Pin | AD7606B_RD_Pin;
//  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
//  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

//  /*Configure GPIO pins : PAPin PAPin */
//  GPIO_InitStruct.Pin = AD7606B_FRD_Pin | AD7606B_BUSY_Pin;
//  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

//  /*Configure GPIO pins : PBPin PBPin PBPin PBPin
//                           PBPin PBPin PBPin */
//  GPIO_InitStruct.Pin = AD7606B_CONV_Pin | AD7606B_STBY_Pin | AD7606B_SER_Pin |
//                        AD7606B_OSI2_Pin | AD7606B_OSI1_Pin | AD7606B_OSI0_Pin |
//                        AD7606B_REST_Pin;
//  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
//  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

//  /*Configure GPIO pins : PCPin */
//  GPIO_InitStruct.Pin = AD7606B_DB7_Pin;
//  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
//  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
//}
///* USER CODE END 2 */

//	void BSP_GPIO_PC2_Init(void)
//{
//  GPIO_InitTypeDef GPIO_InitStruct = {0};
//  
//  /* 1. 使能GPIOC时钟 */
//  __HAL_RCC_GPIOC_CLK_ENABLE();
//  
//  /* 2. 配置PC2引脚 */
//  GPIO_InitStruct.Pin = GPIO_PIN_2;
//  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;      // 推挽输出
//  GPIO_InitStruct.Pull = GPIO_PULLUP;              // 上拉
//  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH; // 高速模式
//  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
//  
//  /* 3. 默认输出高电平 */
//  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET);
//}

//// 方波信号GPIO初始化
//void jianbo_GPIO_Init(void) {
//    GPIO_InitTypeDef GPIO_InitStruct = {0};
//    __HAL_RCC_GPIOB_CLK_ENABLE();
//    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
//    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
//    GPIO_InitStruct.Pull = GPIO_NOPULL;
//    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
//    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
//}

