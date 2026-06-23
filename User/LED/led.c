#include "led.h"

/**
  * @brief  LED初始化函数，配置PE4(绿灯)和PE5(红灯)为推挽输出模式
  * @param  无
  * @retval 无
  */
void LED_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  
  /* 1. 使能GPIOE时钟 */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  
  /* 2. 配置PE4(绿灯)和PE5(红灯)引脚 */
  GPIO_InitStruct.Pin = LED_GREEN_PIN | LED_RED_PIN;  // 同时配置两个LED引脚
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;         // 推挽输出
  GPIO_InitStruct.Pull = GPIO_NOPULL;                // 无上下拉
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;      // 高速模式
  HAL_GPIO_Init(LED_GPIO_PORT, &GPIO_InitStruct);
  
  /* 3. 默认关闭所有LED（根据实际电路调整电平）*/
  LED_All_Off();
}

/**
  * @brief  点亮绿灯
  * @param  无
  * @retval 无
  */
void LED_Green_On(void)
{
  // 根据实际电路选择正确的电平
  // 如果LED阳极接3.3V，阴极接GPIO：低电平点亮，高电平熄灭
  // 如果LED阳极接GPIO，阴极接GND：高电平点亮，低电平熄灭
  HAL_GPIO_WritePin(LED_GPIO_PORT, LED_GREEN_PIN, GPIO_PIN_RESET);  // 高电平点亮
}

/**
  * @brief  熄灭绿灯
  * @param  无
  * @retval 无
  */
void LED_Green_Off(void)
{
  HAL_GPIO_WritePin(LED_GPIO_PORT, LED_GREEN_PIN, GPIO_PIN_SET);  // 低电平熄灭
}

/**
  * @brief  点亮红灯
  * @param  无
  * @retval 无
  */
void LED_Red_On(void)
{
  HAL_GPIO_WritePin(LED_GPIO_PORT, LED_RED_PIN, GPIO_PIN_RESET);  // 高电平点亮
}

/**
  * @brief  熄灭红灯
  * @param  无
  * @retval 无
  */
void LED_Red_Off(void)
{
  HAL_GPIO_WritePin(LED_GPIO_PORT, LED_RED_PIN, GPIO_PIN_SET);  // 低电平熄灭
}

/**
  * @brief  绿灯状态翻转
  * @param  无
  * @retval 无
  */
void LED_Green_Toggle(void)
{
  HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_GREEN_PIN);
}

/**
  * @brief  红灯状态翻转
  * @param  无
  * @retval 无
  */
void LED_Red_Toggle(void)
{
  HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_RED_PIN);
}

/**
  * @brief  所有LED亮
  * @param  无
  * @retval 无
  */
void LED_All_On(void)
{
  LED_Green_On();
  LED_Red_On();
}

/**
  * @brief  所有LED灭
  * @param  无
  * @retval 无
  */
void LED_All_Off(void)
{
  LED_Green_Off();
  LED_Red_Off();
}

/**
  * @brief  所有LED状态翻转
  * @param  无
  * @retval 无
  */
void LED_All_Toggle(void)
{
  LED_Green_Toggle();
  LED_Red_Toggle();
}
