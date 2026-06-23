#ifndef __WATCHDOG_H__
#define __WATCHDOG_H__

#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_iwdg.h"
// ==== 公共函数 ====

/**
  * @brief  初始化独立看门狗
  * @param  timeout_ms: 超时时间（单位：毫秒）
  * @retval HAL Status
  */
HAL_StatusTypeDef Watchdog_Init(uint32_t timeout_ms);

/**
  * @brief  喂狗（刷新看门狗）
  * @retval HAL Status
  */
HAL_StatusTypeDef Watchdog_Refresh(void);
void WG_Error_Handler(void);
#endif /* __WATCHDOG_H__ */
