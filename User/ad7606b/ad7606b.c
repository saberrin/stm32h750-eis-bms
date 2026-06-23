#include "ad7606b.h"
#include "gpio.h"
#include <stdbool.h>
#include <stdio.h>

/* 采集到的数据 */
uint16_t ad7606b_data[AD7606B_CHANNEL_MAX];

/* 电压量程 */
uint8_t ad7606b_range;

extern volatile uint8_t ADC_status;

#if USE_PARALLEL_MODE
/* DBx引脚端口 */
GPIO_TypeDef *port_list[] = {
    AD7606B_DB0_GPIO_Port,  AD7606B_DB1_GPIO_Port,  AD7606B_DB2_GPIO_Port,
    AD7606B_DB3_GPIO_Port,  AD7606B_DB4_GPIO_Port,  AD7606B_DB5_GPIO_Port,
    AD7606B_DB6_GPIO_Port,  AD7606B_DB7_GPIO_Port,  AD7606B_DB8_GPIO_Port,
    AD7606B_DB9_GPIO_Port,  AD7606B_DB10_GPIO_Port, AD7606B_DB11_GPIO_Port,
    AD7606B_DB12_GPIO_Port, AD7606B_DB13_GPIO_Port, AD7606B_DB14_GPIO_Port,
    AD7606B_DB15_GPIO_Port,
};

/* DBx引脚 */
uint16_t pin_list[] = {
    AD7606B_DB0_Pin,  AD7606B_DB1_Pin,  AD7606B_DB2_Pin,  AD7606B_DB3_Pin,
    AD7606B_DB4_Pin,  AD7606B_DB5_Pin,  AD7606B_DB6_Pin,  AD7606B_DB7_Pin,
    AD7606B_DB8_Pin,  AD7606B_DB9_Pin,  AD7606B_DB10_Pin, AD7606B_DB11_Pin,
    AD7606B_DB12_Pin, AD7606B_DB13_Pin, AD7606B_DB14_Pin, AD7606B_DB15_Pin,
};

/**
 * @brief 注销DBx引脚初始化
 *
 */
static void AD7606B_DeInit_DBx_Pin(void) {
  for (uint8_t i = 0; i < sizeof(pin_list) / sizeof(uint16_t); i++) {
    HAL_GPIO_DeInit(port_list[i], pin_list[i]);
  }
}

/**
 * @brief 设置DBx引脚为输入模式
 *
 */
static void AD7606B_Set_DBx_Pin_Input_Mode(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  AD7606B_DeInit_DBx_Pin();
  HAL_Delay(10);

  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

  for (uint8_t i = 0; i < sizeof(pin_list) / sizeof(uint16_t); i++) {
    GPIO_InitStruct.Pin = pin_list[i];
    HAL_GPIO_Init(port_list[i], &GPIO_InitStruct);
  }
}

/**
 * @brief 设置DBx引脚为输出模式
 *
 */
static void AD7606B_Set_DBx_Pin_Output_Mode(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  AD7606B_DeInit_DBx_Pin();
  HAL_Delay(10);

  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

  for (uint8_t i = 0; i < sizeof(pin_list) / sizeof(uint16_t); i++) {
    GPIO_InitStruct.Pin = pin_list[i];
    HAL_GPIO_Init(port_list[i], &GPIO_InitStruct);
  }
}

/**
 * @brief 获取AD7606B单通道数据
 *
 * @return uint16_t 单通道数据
 */
static uint16_t AD7606B_Get_Pin_Data(void) {
  uint16_t shift = 0x0001;
  uint16_t input_level = 0;
  for (uint8_t i = 0; i < sizeof(pin_list) / sizeof(uint16_t); i++) {
    GPIO_PinState state = HAL_GPIO_ReadPin(port_list[i], pin_list[i]);
    if (state == GPIO_PIN_SET) {
      input_level |= shift;
    } else if (state == GPIO_PIN_RESET) {
      input_level &= (~shift);
    }
    shift <<= 1;
  }
  return input_level;
}

/**
 * @brief 设置DB15-DB8引脚电平
 *
 * @param address uint8_t 寄存器地址
 */
static void AD7606B_Set_Address_Pin(uint8_t address) {
  HAL_GPIO_WritePin(AD7606B_DB15_GPIO_Port, AD7606B_DB15_Pin, GPIO_PIN_RESET);
  uint8_t shift = 0x01;
  for (uint8_t i = 0; i < 7; i++) {
    uint8_t level = shift & address;
    if (level) {
      HAL_GPIO_WritePin(port_list[i + 8], pin_list[i + 8], GPIO_PIN_SET);
    } else {
      HAL_GPIO_WritePin(port_list[i + 8], pin_list[i + 8], GPIO_PIN_RESET);
    }
    shift <<= 1;
  }
}

/**
 * @brief 设置DB7-DB0引脚电平
 *
 * @param data uint8_t 数据
 */
static void AD7606_Set_Data_Pin(uint8_t data) {
  uint8_t shift = 0x01;
  for (uint8_t i = 0; i < 8; i++) {
    uint8_t level = shift & data;
    if (level) {
      HAL_GPIO_WritePin(port_list[i], pin_list[i], GPIO_PIN_SET);
    } else {
      HAL_GPIO_WritePin(port_list[i], pin_list[i], GPIO_PIN_RESET);
    }
    shift <<= 1;
  }
}

/**
 * @brief 写寄存器 仅软件模式下有效
 *
 * @param address uint8_t 寄存器地址
 * @param data uint8_t 数据
 */
static void AD7606B_Write_Register(uint8_t address, uint8_t data) {
  AD7606B_Set_DBx_Pin_Output_Mode();
  HAL_Delay(1);
  HAL_GPIO_WritePin(AD7606B_CS_GPIO_Port, AD7606B_CS_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(AD7606B_WR_GPIO_Port, AD7606B_WR_Pin, GPIO_PIN_RESET);
  AD7606B_Set_Address_Pin(address);
  AD7606_Set_Data_Pin(data);
  HAL_GPIO_WritePin(AD7606B_WR_GPIO_Port, AD7606B_WR_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(AD7606B_CS_GPIO_Port, AD7606B_CS_Pin, GPIO_PIN_SET);
  AD7606B_Set_DBx_Pin_Input_Mode();
  HAL_Delay(1);
}

/**
 * @brief 获取AD7606B单次采集数据
 *
 * @param data uint16_t 数组指针
 */
void AD7606B_Read_Data(uint16_t *data) {
  GPIO_PinState state = GPIO_PIN_SET;
  do {
    state = HAL_GPIO_ReadPin(AD7606B_BUSY_GPIO_Port, AD7606B_BUSY_Pin);
  } while (state);
  HAL_GPIO_WritePin(AD7606B_CS_GPIO_Port, AD7606B_CS_Pin, GPIO_PIN_RESET);
  for (uint8_t i = 0; i < 8; i++) {
    HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_SET);
    data[i] = AD7606B_Get_Pin_Data();
  }
  HAL_GPIO_WritePin(AD7606B_CS_GPIO_Port, AD7606B_CS_Pin, GPIO_PIN_SET);
}

void AD7606B_Read_Data_mine(uint16_t *data1,uint16_t *data2) {
  GPIO_PinState state = GPIO_PIN_SET;
  do {
    state = HAL_GPIO_ReadPin(AD7606B_BUSY_GPIO_Port, AD7606B_BUSY_Pin);
  } while (state);
  HAL_GPIO_WritePin(AD7606B_CS_GPIO_Port, AD7606B_CS_Pin, GPIO_PIN_RESET);
	// 读取过程
  HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_SET);
  *data1 = AD7606B_Get_Pin_Data();
	HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_SET);
  *data2 = AD7606B_Get_Pin_Data();
  HAL_GPIO_WritePin(AD7606B_CS_GPIO_Port, AD7606B_CS_Pin, GPIO_PIN_SET);
}


#else

/**
 * @brief 获取AD7606B单通道数据
 *
 * @return uint16_t 单通道数据
 */
static uint16_t AD7606B_Get_Pin_Data(void) {
  uint16_t shift = 0x8000;
  uint16_t data = 0;
  for (uint16_t i = 0; i < sizeof(uint16_t) * AD7606B_CHANNEL_MAX; i++) {
    HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_RESET);
    GPIO_PinState state =
        HAL_GPIO_ReadPin(AD7606B_DB7_GPIO_Port, AD7606B_DB7_Pin);
    if (state == GPIO_PIN_SET) {
      data |= shift;
    } else if (state == GPIO_PIN_RESET) {
      data &= (~shift);
    }
    shift >>= 1;
    HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_SET);
  }
  return data;
}

/**
 * @brief 向AD7606B写入uint16_t数据
 *
 * @param data uint16_t 数据
 */
static void AD7606B_Write_Word(uint16_t data) {
  uint16_t shift = 0x8000;
  for (uint8_t i = 0; i < 16; i++) {
    bool level = shift & data;
    if (level) {
      HAL_GPIO_WritePin(AD7606B_DB11_GPIO_Port, AD7606B_DB11_Pin, GPIO_PIN_SET);
    } else {
      HAL_GPIO_WritePin(AD7606B_DB11_GPIO_Port, AD7606B_DB11_Pin,
                        GPIO_PIN_RESET);
    }
    shift >>= 1;
    HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_SET);
  }
}

/**
 * @brief 写寄存器 仅软件模式下有效
 *
 * @param address uint8_t 寄存器地址
 * @param data uint8_t 数据
 */
static void AD7606B_Write_Register(uint8_t address, uint8_t data) {
  uint16_t wrtie_data = (uint16_t)((address & 0x3F) << 8) | (uint16_t)data;
  HAL_GPIO_WritePin(AD7606B_CS_GPIO_Port, AD7606B_CS_Pin, GPIO_PIN_RESET);
  AD7606B_Write_Word(wrtie_data);
  HAL_GPIO_WritePin(AD7606B_CS_GPIO_Port, AD7606B_CS_Pin, GPIO_PIN_SET);
}

/**
 * @brief 获取AD7606B单次采集数据
 *
 * @param data uint16_t 数组指针
 */
void AD7606B_Read_Data(uint16_t *data) {
  GPIO_PinState state = GPIO_PIN_SET;
  do {
    state = HAL_GPIO_ReadPin(AD7606B_BUSY_GPIO_Port, AD7606B_BUSY_Pin);
  } while (state);
  HAL_GPIO_WritePin(AD7606B_CS_GPIO_Port, AD7606B_CS_Pin, GPIO_PIN_RESET);
  do {
    state = HAL_GPIO_ReadPin(AD7606B_FRD_GPIO_Port, AD7606B_FRD_Pin);
  } while (!state);
  for (uint8_t i = 0; i < AD7606B_CHANNEL_MAX; i++) {
    HAL_GPIO_WritePin(AD7606B_CS_GPIO_Port, AD7606B_CS_Pin, GPIO_PIN_RESET);
    data[i] = AD7606B_Get_Pin_Data();
    HAL_GPIO_WritePin(AD7606B_CS_GPIO_Port, AD7606B_CS_Pin, GPIO_PIN_SET);
  }
}

/**
 * @brief 设置AD7606B串行输出模式
 *
 * @param format uint8_t 格式
 */
void AD7606_Set_Serial_Output_Format(uint8_t format) {
  uint8_t write_data = format << 3;
  AD7606B_Write_Register(AD7606B_CONFIG_REGISTER, write_data);
}

#endif

/**
 * @brief 软件模式下启动采集 仅软件模式有效
 *
 */
void AD7606B_Start(void) { AD7606B_Write_Register(START_AD_CONVERSION, 0); }

/**
 * @brief 设置电压量程
 *
 * @param range uint8_t 电压量程
 */
void AD7606B_Set_Range(uint8_t range) {
  ad7606b_range = range;
#if USE_SOFTWARE_MODE
  range = (range << 4) | range;
  AD7606B_Write_Register(RANGE_CH1_CH2, range);
  AD7606B_Write_Register(RANGE_CH3_CH4, range);
  AD7606B_Write_Register(RANGE_CH5_CH6, range);
  AD7606B_Write_Register(RANGE_CH7_CH8, range);
#endif
}

/**
 * @brief AD7606B复位
 *
 */
void AD7606B_Reset(void) {
  HAL_GPIO_WritePin(AD7606B_REST_GPIO_Port, AD7606B_REST_Pin, GPIO_PIN_RESET);
  HAL_Delay(1);
  HAL_GPIO_WritePin(AD7606B_REST_GPIO_Port, AD7606B_REST_Pin, GPIO_PIN_SET);
  HAL_Delay(1);
  HAL_GPIO_WritePin(AD7606B_REST_GPIO_Port, AD7606B_REST_Pin, GPIO_PIN_RESET);
	ADC_status = 0; // 设置幅值大小
}

/**
 * @brief 启动AD7606B转换
 *
 */
void AD7606B_Conversion(void) {
  HAL_GPIO_WritePin(AD7606B_CONV_GPIO_Port, AD7606B_CONV_Pin, GPIO_PIN_RESET);
  HAL_Delay(1);
  HAL_GPIO_WritePin(AD7606B_CONV_GPIO_Port, AD7606B_CONV_Pin, GPIO_PIN_SET);
  HAL_Delay(1);
	ADC_status = 1; // 设置幅值大小
}

/**
 * @brief AD7606B工作模式设置
 *
 * @param mode uint8_t 工作模式
 */
void AD7606B_Working_Mode(uint8_t mode) {
  if (mode == Hardware_Mode) {
    HAL_GPIO_WritePin(AD7606B_OSI0_GPIO_Port, AD7606B_OSI0_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(AD7606B_OSI1_GPIO_Port, AD7606B_OSI1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(AD7606B_OSI2_GPIO_Port, AD7606B_OSI2_Pin, GPIO_PIN_RESET);
  } else if (mode == Software_Mode) {
    HAL_GPIO_WritePin(AD7606B_OSI0_GPIO_Port, AD7606B_OSI0_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(AD7606B_OSI1_GPIO_Port, AD7606B_OSI1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(AD7606B_OSI2_GPIO_Port, AD7606B_OSI2_Pin, GPIO_PIN_SET);
  }
}

/**
 * @brief 将数字量转换为电压量
 *
 * @param data uint16_t 数字量
 * @return double 电压量 单位毫伏(mv)
 */
double AD7606B_Digital2Voltage(uint16_t data) {
  int16_t signed_data = data;
  switch (ad7606b_range) {
  case Range_2_5_V:
    return signed_data * 5.0 / 65536.0 * 1000.0;
  case Range_5_V:
    return signed_data * 10.0 / 65536.0 * 1000.0;
  case Range_10_V:
    return signed_data * 20.0 / 65536.0 * 1000.0;
  }
  return 0;
}

/**
 * @brief AD7606B初始化
 *
 * @param mode uint8_t 模式选择
 */
void AD7606B_Init(uint8_t mode) {
#if USE_PARALLEL_MODE
  AD7606B_Parallel_GPIO_Init();
  AD7606B_Set_DBx_Pin_Input_Mode();
  HAL_GPIO_WritePin(AD7606B_SER_GPIO_Port, AD7606B_SER_Pin, GPIO_PIN_RESET);
#else
  AD7606B_Serial_GPIO_Init();
  HAL_GPIO_WritePin(AD7606B_SER_GPIO_Port, AD7606B_SER_Pin, GPIO_PIN_SET);
#endif

  HAL_GPIO_WritePin(AD7606B_STBY_GPIO_Port, AD7606B_STBY_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(AD7606B_CS_GPIO_Port, AD7606B_CS_Pin, GPIO_PIN_SET);

  AD7606B_Working_Mode(mode);
}
