#include "watchdog.h"

// 内部 IWDG 句柄
static IWDG_HandleTypeDef hiwdg;

/**
  * @brief  初始化独立看门狗
  * @param  timeout_ms: 超时时间（毫秒）
  * @retval HAL Status
  */
HAL_StatusTypeDef Watchdog_Init(uint32_t timeout_ms)
{
    uint32_t prescaler = IWDG_PRESCALER_32;
    uint32_t iwdg_clk = 32000 / 32; // ≈ 1 kHz

    uint32_t reload = (timeout_ms * iwdg_clk) / 1000;
    if (reload > 0xFFF) reload = 0xFFF; // 最大值限制

    hiwdg.Instance = IWDG1;
    hiwdg.Init.Prescaler = prescaler;
    hiwdg.Init.Reload = reload;
    hiwdg.Init.Window = reload;   // ? H7 必须有

    return HAL_IWDG_Init(&hiwdg);
}


/**
  * @brief  喂狗
  * @retval HAL Status
  */
HAL_StatusTypeDef Watchdog_Refresh(void)
{
    return HAL_IWDG_Refresh(&hiwdg);
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void WG_Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    // 这里可以加断点，也可以加灯闪烁，或记录日志
		printf("看门狗触发，系统进入错误处理！\r\n");
		HAL_Delay(100);
	
    __disable_irq();
    while (1)
    {
    }
    /* USER CODE END Error_Handler_Debug */
}

