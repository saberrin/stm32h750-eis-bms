#include "dac8830.h"
#include "spi.h"

/**
 * @brief DAC8830数据传输
 *
 * @param data uint16_t 数据
 */
static void DAC8830_SPI_Write(uint16_t data) {
  uint8_t spi_data[2] = {0};
  spi_data[0] = (data >> 8) & 0xFF;
  spi_data[1] = data & 0xFF;
  HAL_GPIO_WritePin(DAC8830_CS1_GPIO_Port, DAC8830_CS1_Pin, GPIO_PIN_RESET);
  //HAL_GPIO_WritePin(DAC8830_CS2_GPIO_Port, DAC8830_CS2_Pin, GPIO_PIN_RESET);
  HAL_SPI_Transmit(&hspi6, spi_data, sizeof(spi_data), 1000);
  /* 等待DAC数据更新 */
  HAL_Delay(1);
  HAL_GPIO_WritePin(DAC8830_CS1_GPIO_Port, DAC8830_CS1_Pin, GPIO_PIN_SET);
  //HAL_GPIO_WritePin(DAC8830_CS2_GPIO_Port, DAC8830_CS2_Pin, GPIO_PIN_SET);
}

/**
 * @brief DAC8830写DAC数据
 *
 * @param voltage double  电压
 */
static void DAC8830_DAC_Wrtie(double voltage) {
  uint16_t binary_voltage;
#if VOLTAGE_RANGE
#if VOLTAGE_OUTPUT_MODE
  /* 跳帽接10V ±10V输出 */
  double real_voltage = (voltage + 10.0) / 8.0;
#else
  /* 跳帽接10V 0V-10V输出 */
  double real_voltage = voltage / 4.0;
#endif
#else
#if VOLTAGE_OUTPUT_MODE
  /* 跳帽接5V ±5V输出 */
  double real_voltage = (voltage + 5.0) / 4.0;
#else
  /* 跳帽接5V 0V-5V输出 */
  double real_voltage = voltage / 2.0;
#endif
#endif
  binary_voltage = (uint16_t)(real_voltage * 1000.0 * 0xFFFF / VREF);
  DAC8830_SPI_Write(binary_voltage);
}

/**
 * @brief DAC8830直流输出
 *
 * @param voltage double  电压
 */
void DAC8830_Set_Direct_Current(double voltage) { DAC8830_DAC_Wrtie(voltage); }

/**
 * @brief DAC8830波形输出
 *
 * @param data double*  波形数据
 * @param data_size uint16_t  数据长度
 */
void DAC8830_Set_Wave(double *data, uint16_t data_size) {
  for (uint16_t i = 0; i < data_size; i++) {
    DAC8830_DAC_Wrtie(data[i]);
  }
}

/**
 * @brief DAC8830初始化
 *
 */
void DAC8830_Init(void) {
  MX_SPI6_Init();
  DAC8830_GPIO_Init();

  HAL_GPIO_WritePin(DAC8830_CS1_GPIO_Port, DAC8830_CS1_Pin, GPIO_PIN_SET);
  //HAL_GPIO_WritePin(DAC8830_CS2_GPIO_Port, DAC8830_CS2_Pin, GPIO_PIN_SET);
}

void DAC8830_GPIO_Init(void) {

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, DAC8830_CS1_Pin , GPIO_PIN_RESET);

  /*Configure GPIO pins : PAPin PAPin */
  GPIO_InitStruct.Pin = DAC8830_CS1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void DAC8830_set_Voltage(double targetV)
{
 

// 安全限制：0 ~ 5V 范围
    if(targetV < 0)    targetV = 0;
    if(targetV > 5.0)  targetV = 5.0;

    // 16位DAC标准计算公式
    uint16_t code = (uint16_t)((targetV / 5.0) * 0xFFFF);

    // 直接输出正确值
    DAC8830_SPI_Write(code);
	
	
}





void CALIB_DAC_SetVoltage(double targetV)
{

//  double DAC_SETV=(targetV- 2.484935) /  0.497458 ;  
	DAC8830_Set_Direct_Current(targetV);
}





