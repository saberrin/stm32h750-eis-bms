/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    spi.h
  * @brief   This file contains all the function prototypes for
  *          the spi.c file
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SPI_H
#define __SPI_H
#include "system.h"
//-----------------------------------------------------------------
// 声明
//-----------------------------------------------------------------
extern SPI_HandleTypeDef SPI_Handler;  // SPI句柄
extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi6;

void MX_SPI1_Init(void);   // 新增
//-----------------------------------------------------------------
// SPI引脚定义
//-----------------------------------------------------------------
#define SPIx																				SPI1
#define SPI_CLK_ENABLE()                   					__HAL_RCC_SPI1_CLK_ENABLE()  

#define SPI_MISO_PIN                                GPIO_PIN_6
#define SPI_MISO_GPIO_PORT                          GPIOA
#define SPI_MISO_GPIO_CLK_ENABLE()                  __HAL_RCC_GPIOA_CLK_ENABLE()  
#define SPI_MISO_GPIO_CLK_DISABLE()                 __HAL_RCC_GPIOA_CLK_DISABLE() 
#define SPI_MISO_AF																	GPIO_AF5_SPI1

#define SPI_MOSI_PIN                                GPIO_PIN_7
#define SPI_MOSI_GPIO_PORT                          GPIOA
#define SPI_MOSI_GPIO_CLK_ENABLE()                  __HAL_RCC_GPIOA_CLK_ENABLE()  
#define SPI_MOSI_GPIO_CLK_DISABLE()                 __HAL_RCC_GPIOA_CLK_DISABLE() 
#define SPI_MOSI_AF																	GPIO_AF5_SPI1

#define SPI_CLK_PIN                                 GPIO_PIN_5
#define SPI_CLK_GPIO_PORT                           GPIOA
#define SPI_CLK_GPIO_CLK_ENABLE()                   __HAL_RCC_GPIOA_CLK_ENABLE()  
#define SPI_CLK_GPIO_CLK_DISABLE()                  __HAL_RCC_GPIOA_CLK_DISABLE()  
#define SPI_CLK_AF																	GPIO_AF5_SPI1

//-----------------------------------------------------------------
// 函数声明
//-----------------------------------------------------------------
extern void SPI_Init(void);
extern void SPI1_SetSpeed(u8 SPI_BaudRatePrescaler);
extern u8 SPI1_ReadWriteByte(u8 TxData);

#endif

#ifndef __SPI_H__
#define __SPI_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "system.h"
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern SPI_HandleTypeDef hspi6;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_SPI6_Init(void);

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __SPI_H__ */

