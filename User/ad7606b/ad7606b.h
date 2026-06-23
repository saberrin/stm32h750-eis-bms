#ifndef __AD7606B_H__
#define __AD7606B_H__

#include <inttypes.h>

/* 软件模式：   1   硬件模式：    0 */
#define USE_SOFTWARE_MODE 0
/* 并行模式： 1  串行模式：  0 */
#define USE_PARALLEL_MODE 1

/* 最大通道 */
#define AD7606B_CHANNEL_MAX 8

/* 软件模式下开启AD转换地址 */
#define START_AD_CONVERSION 0x00

/* 寄存器地址 */
#define AD7606B_CONFIG_REGISTER 0x02
#define RANGE_CH1_CH2 0x03
#define RANGE_CH3_CH4 0x04
#define RANGE_CH5_CH6 0x05
#define RANGE_CH7_CH8 0x06

/* 电压量程 */
enum AD7606B_Range {
  Range_2_5_V = 0x00,
  Range_5_V,
  Range_10_V,
};

/* AD7606B工作模式 */
enum AD7606B_Working_Mode {
  Hardware_Mode = 0,
  Software_Mode,
};

/* DoutX输出格式选择 */
enum AD7606B_Serial_Ouput_Format {
  _1Dout = 0x00,
  _2Dout,
  _3Dout,
};

extern uint16_t ad7606b_data[AD7606B_CHANNEL_MAX];

void AD7606B_Init(uint8_t mode);
void AD7606B_Reset(void);
void AD7606B_Conversion(void);
void AD7606B_Start(void);
void AD7606B_Set_Range(uint8_t range);
double AD7606B_Digital2Voltage(uint16_t data);

void AD7606B_Read_Data(uint16_t *data);
void AD7606B_Read_Data_mine(uint16_t *data1, uint16_t *data2);
#if USE_PARALLEL_MODE
#else
void AD7606_Set_Serial_Output_Format(uint8_t format);
#endif

#endif
