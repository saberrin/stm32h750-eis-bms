

#ifndef _AD7606_H
#define _AD7606_H
#include "system.h"
//-----------------------------------------------------------------


#define CO_A_L HAL_GPIO_WritePin(AD7606B_CONV_GPIO_Port, AD7606B_CONV_Pin, GPIO_PIN_RESET)
#define CO_A_H HAL_GPIO_WritePin(AD7606B_CONV_GPIO_Port, AD7606B_CONV_Pin, GPIO_PIN_SET)

#define STBY_L HAL_GPIO_WritePin(AD7606B_STBY_GPIO_Port, AD7606B_STBY_Pin, GPIO_PIN_RESET)
#define STBY_H HAL_GPIO_WritePin(AD7606B_STBY_GPIO_Port, AD7606B_STBY_Pin, GPIO_PIN_SET)

#define REST_L HAL_GPIO_WritePin(AD7606B_REST_GPIO_Port, AD7606B_REST_Pin, GPIO_PIN_RESET)
#define REST_H HAL_GPIO_WritePin(AD7606B_REST_GPIO_Port, AD7606B_REST_Pin, GPIO_PIN_SET)

#define CO_B_L HAL_GPIO_WritePin(AD7606B_WR_GPIO_Port, AD7606B_WR_Pin, GPIO_PIN_RESET)
#define CO_B_H HAL_GPIO_WritePin(AD7606B_WR_GPIO_Port, AD7606B_WR_Pin, GPIO_PIN_SET)

#define CS_N_L HAL_GPIO_WritePin(AD7606B_CS_GPIO_Port, AD7606B_CS_Pin, GPIO_PIN_RESET)
#define CS_N_H HAL_GPIO_WritePin(AD7606B_CS_GPIO_Port, AD7606B_CS_Pin, GPIO_PIN_SET)

#define RD_SCLK_L HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_RESET)
#define RD_SCLK_H HAL_GPIO_WritePin(AD7606B_RD_GPIO_Port, AD7606B_RD_Pin, GPIO_PIN_SET)

#define FR_D HAL_GPIO_ReadPin(AD7606B_FRD_GPIO_Port, AD7606B_FRD_Pin)
#define BUSY HAL_GPIO_ReadPin(AD7606B_BUSY_GPIO_Port, AD7606B_BUSY_Pin)


#define AD7606B_DB15_Pin GPIO_PIN_11
#define AD7606B_DB15_GPIO_Port GPIOB
#define AD7606B_DB14_Pin GPIO_PIN_13
#define AD7606B_DB14_GPIO_Port GPIOB
#define AD7606B_DB13_Pin GPIO_PIN_14
#define AD7606B_DB13_GPIO_Port GPIOB
#define AD7606B_DB12_Pin GPIO_PIN_15
#define AD7606B_DB12_GPIO_Port GPIOB
#define AD7606B_DB11_Pin GPIO_PIN_7
#define AD7606B_DB11_GPIO_Port GPIOC
#define AD7606B_DB10_Pin GPIO_PIN_8
#define AD7606B_DB10_GPIO_Port GPIOC
#define AD7606B_DB9_Pin GPIO_PIN_9
#define AD7606B_DB9_GPIO_Port GPIOC
#define AD7606B_DB8_Pin GPIO_PIN_8
#define AD7606B_DB8_GPIO_Port GPIOA
#define AD7606B_DB7_Pin GPIO_PIN_12
#define AD7606B_DB7_GPIO_Port GPIOB
#define AD7606B_DB6_Pin GPIO_PIN_15
#define AD7606B_DB6_GPIO_Port GPIOA
#define AD7606B_DB5_Pin GPIO_PIN_10
#define AD7606B_DB5_GPIO_Port GPIOC
#define AD7606B_DB4_Pin GPIO_PIN_11
#define AD7606B_DB4_GPIO_Port GPIOC
#define AD7606B_DB3_Pin GPIO_PIN_7
#define AD7606B_DB3_GPIO_Port GPIOD
#define AD7606B_DB2_Pin GPIO_PIN_6
#define AD7606B_DB2_GPIO_Port GPIOB
#define AD7606B_DB1_Pin GPIO_PIN_7
#define AD7606B_DB1_GPIO_Port GPIOB
#define AD7606B_DB0_Pin GPIO_PIN_8
#define AD7606B_DB0_GPIO_Port GPIOB






#define AD7606B_SER_Pin GPIO_PIN_3
#define AD7606B_SER_GPIO_Port GPIOA
#define AD7606B_STBY_Pin GPIO_PIN_2
#define AD7606B_STBY_GPIO_Port GPIOA
#define AD7606B_CONV_Pin GPIO_PIN_1
#define AD7606B_CONV_GPIO_Port GPIOC
#define AD7606B_WR_Pin GPIO_PIN_0
#define AD7606B_WR_GPIO_Port GPIOC
#define AD7606B_REST_Pin GPIO_PIN_13
#define AD7606B_REST_GPIO_Port GPIOC
#define AD7606B_RD_Pin GPIO_PIN_3
#define AD7606B_RD_GPIO_Port GPIOE
#define AD7606B_CS_Pin GPIO_PIN_1
#define AD7606B_CS_GPIO_Port GPIOE
#define AD7606B_BUSY_Pin GPIO_PIN_0
#define AD7606B_BUSY_GPIO_Port GPIOE
#define AD7606B_FRD_Pin GPIO_PIN_9  
#define AD7606B_FRD_GPIO_Port GPIOB



//-----------------------------------------------------------------
// 外部函数声明
//-----------------------------------------------------------------
extern void GPIO_AD7606_Configuration(void);
extern void AD7606_Init(void);
extern void AD7606_StartConvst(void);
extern void AD7606_RESET(void) ;
extern void AD7606_ReadData(s16 * DB_data);
extern void AD7606_Delay(uint16_t t);

//void AD7606_Read_Data_mine(uint16_t *data1,uint16_t *data2);
void AD7606_Read_Data(uint16_t *data1);
double AD7606B_Digital2Voltage(uint16_t data);





///* 新增：AD7606_RANGE 引脚定义 (连接至 GPIOD, Pin 7) */
//#define AD7606_RANGE_GPIO_PORT       GPIOD
//#define AD7606_RANGE_PIN             GPIO_PIN_7

///* 新增：动态范围（量程）选择枚举 */
//typedef enum {
//  AD7606_RANGE_5V = 0,   /* ±5V 量程 */
//  AD7606_RANGE_10V = 1    /* ±10V 量程 */
//} AD7606_RangeTypeDef;

///* 在函数声明部分添加以下函数原型 */
//void AD7606_RANGE_GPIO_Init(void); // 初始化RANGE控制引脚
////void AD7606_SetRange(AD7606_RangeTypeDef range); // 设置量程函数
//AD7606_RangeTypeDef AD7606_GetRange(void); // 获取当前量程设置

void AD7606_ReadData_2Ch(uint16_t * DB_data1,uint16_t * DB_data2);





#endif
//-----------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------
