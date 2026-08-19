/**
  ******************************************************************************
  * @file    Templates/Src/stm32h7xx_it.c 
  * @author  MCD Application Team
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and 
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_it.h"
#include "main.h"
#include "timer.h"
#include "dac.h"
#include "EIS_Measure.h"
#include "runtime_flash.h"

/** @addtogroup STM32H7xx_HAL_Examples
  * @{
  */

/** @addtogroup Templates
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M7 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief   This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
  HAL_IncTick();
	
}
  
void TIM6_DAC_IRQHandler(void)
{
    if (__HAL_TIM_GET_FLAG(&TIM6_Handler, TIM_FLAG_UPDATE) != RESET) {
        __HAL_TIM_CLEAR_FLAG(&TIM6_Handler, TIM_FLAG_UPDATE); // ??????
        cycle_count++; // 周期内点计数
				ADC_Start_Standard = 1;
//      if (cycle_count % SIN_DATA == 0) {
//        LED_Green_Toggle(); // 满足整数周期条件，执行LED切换函数
//        }
        if (cycle_count >= Num_Sampling_Points ) { // ??2??????
            HAL_TIM_Base_Stop_IT(&TIM6_Handler);    // ?????
            HAL_DAC_Stop(&DAC_Handler, DAC_CHANNEL_1);     // ??DAC??
            HAL_DAC_SetValue(&DAC_Handler, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 0); // ??0V
						// 方法1：停止转换信号 复位  替换方法，不采集数据
            // AD7606B_Reset();
						ADC_status = 0;
//            cycle_count = 0; // ?????
//						printf("AD7606B_Reset: %u\n", ADC_status);
//						ADC_Start_Standard = 0;
//						printf("AD7606B采集数量 %llu\n",Sampling_Count);
					
						
					

					
        }
    }
}

void TIM7_IRQHandler(void)
{
    if (__HAL_TIM_GET_FLAG(&TIM7_Handler, TIM_FLAG_UPDATE) != RESET)
    {
        if (__HAL_TIM_GET_IT_SOURCE(&TIM7_Handler, TIM_IT_UPDATE) != RESET)
        {
            __HAL_TIM_CLEAR_IT(&TIM7_Handler, TIM_IT_UPDATE);
            Runtime_On1sTick();
        }
    }
}


/******************************************************************************/
/*                 STM32H7xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32h7xx.s).                                               */
/******************************************************************************/
/**
  * @brief This function handles I2C1 event interrupt.
  */
//void I2C1_EV_IRQHandler(void)
//{
//  /* USER CODE BEGIN I2C1_EV_IRQn 0 */

//  /* USER CODE END I2C1_EV_IRQn 0 */
//  HAL_I2C_EV_IRQHandler(&hi2c_eis);
//  /* USER CODE BEGIN I2C1_EV_IRQn 1 */

//  /* USER CODE END I2C1_EV_IRQn 1 */
//}

///**
//  * @brief This function handles I2C1 error interrupt.
//  */
//void I2C1_ER_IRQHandler(void)
//{
//  /* USER CODE BEGIN I2C1_ER_IRQn 0 */

//  /* USER CODE END I2C1_ER_IRQn 0 */
//  HAL_I2C_ER_IRQHandler(&hi2c_eis);
//  /* USER CODE BEGIN I2C1_ER_IRQn 1 */

//  /* USER CODE END I2C1_ER_IRQn 1 */
//}

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/


/**
  * @}
  */ 

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
