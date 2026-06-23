/**
 ******************************************************************************
 * @file    qspi.c
 * @brief   QSPI 初始化 (STM32H750VBT6, W25Q128, BK1 使用)
 *
 * 引脚连接 (与你的新硬件原理图对应)：
 *   PB2  -> QUADSPI_CLK
 *   PB10 -> QUADSPI_BK1_NCS
 *   PD12 -> QUADSPI_BK1_IO0
 *   PD13 -> QUADSPI_BK1_IO1
 *   PE2  -> QUADSPI_BK1_IO2
 *   PD14 -> QUADSPI_BK1_IO3
 *
 * 以上引脚的复用功能均为 AF9_QUADSPI。
 ******************************************************************************
 */

#include "qspi.h"

QSPI_HandleTypeDef hqspi;

/* QSPI GPIO 配置 */
static void QSPI_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 使能 GPIO 时钟 */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    /* 共用配置：复用推挽，上拉，高速，AF9_QUADSPI */
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;

    /* PB2 - CLK */
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* PB10 - NCS */
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* PD11 - IO0 */
    GPIO_InitStruct.Pin = GPIO_PIN_11;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* PD12 - IO1 */
    GPIO_InitStruct.Pin = GPIO_PIN_12;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* PD13 - IO3 */
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* PE2  - IO2 */
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
}

/* QSPI 外设初始化：只使用间接模式，频率先设置得比较保守 */
void QSPI_Init(void)
{
    /* 1. 开启 QSPI 时钟 & GPIO 配置 */
    __HAL_RCC_QSPI_CLK_ENABLE();
    QSPI_GPIO_Init();

    /* 2. 配置 QSPI 外设参数 */
    hqspi.Instance = QUADSPI;
    hqspi.Init.ClockPrescaler     = 7;                       // Fqspi = HCLK/(Prescaler+1)
    //hqspi.Init.FifoThreshold      = 4;
    //hqspi.Init.SampleShifting     = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
    hqspi.Init.SampleShifting     = QSPI_SAMPLE_SHIFTING_NONE; // 先不用 half-cycle
    hqspi.Init.FlashSize          = 23;                      // 2^(23+1) = 16MB (W25Q128)
    hqspi.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_3_CYCLE;
    hqspi.Init.ClockMode          = QSPI_CLOCK_MODE_3;
    hqspi.Init.FlashID            = QSPI_FLASH_ID_1;
    hqspi.Init.DualFlash          = QSPI_DUALFLASH_DISABLE;

    if (HAL_QSPI_Init(&hqspi) != HAL_OK)
    {
        // 这里可以加断言或错误处理
        while (1);
    }
}

/* 如果工程里已经有 HAL_QSPI_MspInit，可以删掉这个函数。
 * 如果没有，保留下面这个实现也可以正常工作。
 */
void HAL_QSPI_MspInit(QSPI_HandleTypeDef* qspiHandle)
{
    if (qspiHandle->Instance == QUADSPI)
    {
        __HAL_RCC_QSPI_CLK_ENABLE();
    }
}
