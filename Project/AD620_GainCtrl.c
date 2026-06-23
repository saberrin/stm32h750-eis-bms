#include "AD620_GainCtrl.h"
#include "stm32h7xx_hal.h"
#include <math.h>  // 添加数学库头文件，解决fabsf隐式声明问题

// 增益配置表 - 根据您的实际校准数据修改
static const AD620_GainConfig_t gainConfigTable[AD620_GAIN_MAX] = {
    {AD620_GAIN_2,    2.002f,  2.002f,  0x00}, // 000
    {AD620_GAIN_5,    4.984f,  4.984f,  0x01}, // 001
    {AD620_GAIN_10,   9.998f,  9.998f,  0x02}, // 010
    {AD620_GAIN_50,   49.91f,  49.91f,  0x03}, // 011
    {AD620_GAIN_100,  100.0f,  100.0f,  0x04}, // 100
    {AD620_GAIN_200,  199.4f,  199.4f,  0x05}, // 101
    {AD620_GAIN_500,  501.0f,  501.0f,  0x06}, // 110
    {AD620_GAIN_1000, 1003.0f, 1003.0f, 0x07}  // 111
};

static AD620_GainChannel_t currentGainChannel = AD620_GAIN_2;

/**
  * @brief  初始化ADG708控制引脚
  * @param  无
  * @retval 无
  */
void AD620_GainCtrl_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // 使能GPIOD时钟
    __HAL_RCC_GPIOD_CLK_ENABLE();
    
    // 配置ADG708地址引脚为推挽输出
    GPIO_InitStruct.Pin = ADG708_A0_PIN | ADG708_A1_PIN | ADG708_A2_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    
    HAL_GPIO_Init(ADG708_A0_PORT, &GPIO_InitStruct);
    
    // 默认设置为最低增益（2倍）
    AD620_SetGain(AD620_GAIN_10);
}

/**
  * @brief  设置ADG708通道选择
  * @param  channel_code: 通道代码 (0-7)
  * @retval 无
  * @note   此函数直接控制ADG708的A0,A1,A2引脚
  */
static void ADG708_SetChannel(uint8_t channel_code)
{
    // 提取各位数据 (A0 = LSB, A2 = MSB)
    uint8_t a0 = (channel_code >> 0) & 0x01;
    uint8_t a1 = (channel_code >> 1) & 0x01;
    uint8_t a2 = (channel_code >> 2) & 0x01;
    
    // 设置ADG708地址线
    HAL_GPIO_WritePin(ADG708_A0_PORT, ADG708_A0_PIN, a0 ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(ADG708_A1_PORT, ADG708_A1_PIN, a1 ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(ADG708_A2_PORT, ADG708_A2_PIN, a2 ? GPIO_PIN_SET : GPIO_PIN_RESET);
    
    // 添加短暂延时确保开关稳定
    HAL_Delay(1); // 1ms延时，可根据实际需要调整
}

/**
  * @brief  设置AD620增益通道
  * @param  gain_channel: 增益通道选择
  * @retval 无
  */
void AD620_SetGain(AD620_GainChannel_t gain_channel)
{
    if (gain_channel >= AD620_GAIN_MAX) {
        return; // 参数检查
    }
    
    // 设置ADG708通道
    ADG708_SetChannel(gainConfigTable[gain_channel].code);
    
    // 更新当前增益通道
    currentGainChannel = gain_channel;
}

/**
  * @brief  根据期望增益值自动选择最接近的增益通道
  * @param  desired_gain: 期望的增益值
  * @retval 无
  */
void AD620_SetGainByValue(float desired_gain)
{
    AD620_GainChannel_t best_channel = AD620_GAIN_2;
    
    // 使用一个足够大的初始值，而不是依赖未定义的 __FLT_MAX__
    float min_diff = 1e9f;  // 使用一个足够大的值代替 __FLT_MAX__
    
    // 查找最接近的增益通道
    for (int i = 0; i < AD620_GAIN_MAX; i++) {
        // 使用 fabsf 计算绝对值（需要包含 math.h）
        float diff = fabsf(gainConfigTable[i].actual_gain - desired_gain);
        if (diff < min_diff) {
            min_diff = diff;
            best_channel = (AD620_GainChannel_t)i;
        }
    }
    
    // 设置找到的最佳增益
    AD620_SetGain(best_channel);
}

/**
  * @brief  获取当前增益通道
  * @param  无
  * @retval 当前增益通道
  */
AD620_GainChannel_t AD620_GetCurrentGainChannel(void)
{
    return currentGainChannel;
}

/**
  * @brief  获取当前实际增益值
  * @param  无
  * @retval 当前实际增益值
  */
float AD620_GetCurrentGain(void)
{
    if (currentGainChannel < AD620_GAIN_MAX) {
        return gainConfigTable[currentGainChannel].actual_gain;
    }
    return 1.0f; // 默认增益
}

/**
  * @brief  获取指定增益通道的配置信息
  * @param  channel: 增益通道
  * @retval 增益配置结构体指针，失败返回NULL
  */
const AD620_GainConfig_t* AD620_GetGainConfig(AD620_GainChannel_t channel)
{
    if (channel >= AD620_GAIN_MAX) {
        return NULL;
    }
    return &gainConfigTable[channel];
}

/**
  * @brief  反初始化AD620增益控制器
  * @param  无
  * @retval 无
  */
void AD620_GainCtrl_Deinit(void)
{
    // 将所有控制引脚设置为默认状态（低电平）
    HAL_GPIO_WritePin(ADG708_A0_PORT, ADG708_A0_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(ADG708_A1_PORT, ADG708_A1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(ADG708_A2_PORT, ADG708_A2_PIN, GPIO_PIN_RESET);
}
