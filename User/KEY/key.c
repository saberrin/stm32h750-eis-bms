#include "key.h"
#include "delay.h"

// 按键端口和引脚映射数组
GPIO_TypeDef* BUTTON_PORT[BUTTONn] = {
    BUTTON_KEY1_GPIO_PORT,  // KEY1 - PE3
    BUTTON_KEY2_GPIO_PORT   // KEY2 - PC3
};

const uint16_t BUTTON_PIN[BUTTONn] = {
    BUTTON_KEY1_PIN,        // KEY1 - PIN3
    BUTTON_KEY2_PIN         // KEY2 - PIN3
};

/**
  * @brief  初始化所有按键
  * @param  无
  * @retval 无
  */
void KEY_Init(void)
{
    BSP_KEY_Init(BUTTON_KEY1);  // 初始化KEY1(PE3)
    BSP_KEY_Init(BUTTON_KEY2);  // 初始化KEY2(PC3)
}

/**
  * @brief  配置指定的按键
  * @param  button: 要配置的按键(BUTTON_KEY1或BUTTON_KEY2)
  * @retval 无
  */
void BSP_KEY_Init(Button_TypeDef button)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // 直接使用HAL库的时钟使能宏，避免自定义宏的问题
    if(button == BUTTON_KEY1) {
        __HAL_RCC_GPIOE_CLK_ENABLE();  // 使能GPIOE时钟
    } else if(button == BUTTON_KEY2) {
        __HAL_RCC_GPIOC_CLK_ENABLE();  // 使能GPIOC时钟
    }
    
    // 配置按键引脚
    GPIO_InitStruct.Pin = BUTTON_PIN[button];
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;              // 输入模式
    GPIO_InitStruct.Pull = GPIO_PULLUP;                  // 上拉电阻(按键按下为低电平)
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;  // 高速
    HAL_GPIO_Init(BUTTON_PORT[button], &GPIO_InitStruct);
}

/**
  * @brief  按键扫描函数
  * @param  mode: 扫描模式
  *         - 0: 不支持连续按(按下一次只返回一次按键值)
  *         - 1: 支持连续按(按住不放会连续返回按键值)
  * @retval 按键值:
  *         - KEY1_PRES: KEY1按下
  *         - KEY2_PRES: KEY2按下  
  *         - KEY_UNPRESS: 无按键按下
  * @note   此函数有响应优先级, KEY1 > KEY2
  */
uint8_t KEY_Scan(uint8_t mode)
{
    static uint8_t key_up = 1;  // 按键松开标志
    
    if(mode == 1)  // 支持连按模式
    {
        key_up = 1;
    }
    
    // 检测是否有按键按下(由于上拉，按下为低电平)
    if(key_up && (KEY1 == GPIO_PIN_RESET || KEY2 == GPIO_PIN_RESET))
    {
        delay_ms(10);  // 延时消抖
        key_up = 0;    // 标记按键已按下
        
        // 检查具体哪个按键按下(KEY1优先级高于KEY2)
        if(KEY1 == GPIO_PIN_RESET)
        {
            return KEY1_PRES;
        }
        else if(KEY2 == GPIO_PIN_RESET)
        {
            return KEY2_PRES;
        }
    }
    else if(KEY1 == GPIO_PIN_SET && KEY2 == GPIO_PIN_SET)  // 无按键按下
    {
        key_up = 1;  // 标记按键已松开
    }
    
    return KEY_UNPRESS;  // 无按键按下
}

/**
  * @brief  独立读取按键状态（简化函数）
  * @param  无
  * @retval 按键状态位图:
  *         - 位0: KEY1状态(1:按下, 0:松开)
  *         - 位1: KEY2状态(1:按下, 0:松开)
  */
uint8_t KEY_Read(void)
{
    uint8_t key_state = 0;
    
    // 读取所有按键状态
    if(KEY1 == GPIO_PIN_RESET)
    {
        key_state |= 0x01;  // KEY1按下
    }
    if(KEY2 == GPIO_PIN_RESET)
    {
        key_state |= 0x02;  // KEY2按下
    }
    
    return key_state;
}
