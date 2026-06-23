// MOS_Controller.h
#ifndef MOS_CONTROLLER_H
#define MOS_CONTROLLER_H

#include "stm32h7xx_hal.h"
#include <stdint.h>
#include "delay.h"

//enable signal
//#define S1_PORT GPIOD
//#define S1_PIN  GPIO_PIN_3
//#define S2_PORT GPIOD
//#define S2_PIN  GPIO_PIN_4




// Shared address bus for all 74HC4514
//#define S_A0_PORT GPIOD
//#define S_A0_PIN  GPIO_PIN_0
//#define S_A1_PORT GPIOD
//#define S_A1_PIN  GPIO_PIN_1
//#define S_A2_PORT GPIOD
//#define S_A2_PIN  GPIO_PIN_2
//#define S_A3_PORT GPIOD
//#define S_A3_PIN  GPIO_PIN_3

//// 74HC4514 S1
//#define S1_LE_PORT GPIOD
//#define S1_LE_PIN  GPIO_PIN_15
//#define S1_E_PORT  GPIOD
//#define S1_E_PIN   GPIO_PIN_14

//// 74HC4514 S2
//#define S2_LE_PORT GPIOA
//#define S2_LE_PIN  GPIO_PIN_15
//#define S2_E_PORT  GPIOA
//#define S2_E_PIN   GPIO_PIN_8   //pa15 empty,  pa15->pb6

//// 74HC4514 S3
//#define S3_LE_PORT GPIOD
//#define S3_LE_PIN  GPIO_PIN_4
//#define S3_E_PORT  GPIOC
//#define S3_E_PIN   GPIO_PIN_12

//// 74HC4514 S4
//#define S4_LE_PORT GPIOC
//#define S4_LE_PIN  GPIO_PIN_3
//#define S4_E_PORT  GPIOE
//#define S4_E_PIN   GPIO_PIN_3

//#define HC74139_ENABLE_PORT GPIOE
//#define HC74139_ENABLE_PIN  GPIO_PIN_7
//// 74HC139 X1 (A group)
////#define X1_SEL_PORT GPIOE
////#define X1_SEL_PIN  GPIO_PIN_7
//#define X1_A0_PORT  GPIOE
//#define X1_A0_PIN   GPIO_PIN_8
//#define X1_A1_PORT  GPIOE
//#define X1_A1_PIN   GPIO_PIN_9

//// 74HC139 X2 (B group)
////#define X2_SEL_PORT GPIOE
////#define X2_SEL_PIN  GPIO_PIN_10
//#define X2_A0_PORT  GPIOE
//#define X2_A0_PIN   GPIO_PIN_10
//#define X2_A1_PORT  GPIOE
//#define X2_A1_PIN   GPIO_PIN_11

//// 74HC139 X3 (A group)
////#define X3_SEL_PORT GPIOE
////#define X3_SEL_PIN  GPIO_PIN_13
//#define X3_A0_PORT  GPIOE
//#define X3_A0_PIN   GPIO_PIN_12
//#define X3_A1_PORT  GPIOE
//#define X3_A1_PIN   GPIO_PIN_13

//// 74HC139 X4 (B group)
////#define X4_SEL_PORT GPIOD
////#define X4_SEL_PIN  GPIO_PIN_7
//#define X4_A0_PORT  GPIOE
//#define X4_A0_PIN   GPIO_PIN_14
//#define X4_A1_PORT  GPIOE
//#define X4_A1_PIN   GPIO_PIN_15

#define HC4514_ENABLE_PORT GPIOD
#define HC4514_ENABLE_PIN  GPIO_PIN_14

// Shared address bus for all 74HC4514
#define S_A0_PORT GPIOD
#define S_A0_PIN  GPIO_PIN_0
#define S_A1_PORT GPIOD
#define S_A1_PIN  GPIO_PIN_1
#define S_A2_PORT GPIOD
#define S_A2_PIN  GPIO_PIN_2
#define S_A3_PORT GPIOD
#define S_A3_PIN  GPIO_PIN_3

// 74HC4514 S1
#define S1_LE_PORT GPIOD
#define S1_LE_PIN  GPIO_PIN_15

// 74HC4514 S2
#define S2_LE_PORT GPIOC
#define S2_LE_PIN  GPIO_PIN_6

// 74HC4514 S3
#define S3_LE_PORT GPIOD
#define S3_LE_PIN  GPIO_PIN_4

// 74HC4514 S4
#define S4_LE_PORT GPIOC
#define S4_LE_PIN  GPIO_PIN_12

#define HC74139_ENABLE_PORT GPIOE
#define HC74139_ENABLE_PIN  GPIO_PIN_7

// 74HC139 X1 (A group)
//#define X1_SEL_PORT GPIOE
//#define X1_SEL_PIN  GPIO_PIN_7
#define X1_A0_PORT  GPIOE
#define X1_A0_PIN   GPIO_PIN_8
#define X1_A1_PORT  GPIOE
#define X1_A1_PIN   GPIO_PIN_9

// 74HC139 X2 (B group)
//#define X2_SEL_PORT GPIOE
//#define X2_SEL_PIN  GPIO_PIN_10
#define X2_A0_PORT  GPIOE
#define X2_A0_PIN   GPIO_PIN_10
#define X2_A1_PORT  GPIOE
#define X2_A1_PIN   GPIO_PIN_11

// 74HC139 X3 (A group)
//#define X3_SEL_PORT GPIOE
//#define X3_SEL_PIN  GPIO_PIN_13
#define X3_A0_PORT  GPIOE
#define X3_A0_PIN   GPIO_PIN_12
#define X3_A1_PORT  GPIOE
#define X3_A1_PIN   GPIO_PIN_13

// 74HC139 X4 (B group)
//#define X4_SEL_PORT GPIOD
//#define X4_SEL_PIN  GPIO_PIN_7
#define X4_A0_PORT  GPIOE
#define X4_A0_PIN   GPIO_PIN_14
#define X4_A1_PORT  GPIOE
#define X4_A1_PIN   GPIO_PIN_15

// 4514 groups
typedef enum {
    SWITCH_GROUP_S1 = 1,
    SWITCH_GROUP_S2,
    SWITCH_GROUP_S3,
    SWITCH_GROUP_S4
} SwitchGroup_t;

// 139 groups
typedef enum {
    DECODER_X1 = 1,
    DECODER_X2,
    DECODER_X3,
    DECODER_X4
} DecoderGroup_t;

// GPIO and decoder basic API
void SwitchMatrix_Init(void);
void Decoder_Select_4to16(SwitchGroup_t group, uint8_t channel);
void Decoder_Select_2to4(DecoderGroup_t group, uint8_t value);

// High-level window programming
void SwitchWindow_Program(uint8_t s);
void SwitchWindow_Program_Test(uint8_t s);
// Self-test and impedance measurement
int  SelfTest_Run(float dv_min, float dv_max);                   // returns 0 on pass, >0 fail count
float Measure_Impedance_One(uint8_t s, float excite_mA, uint32_t settle_ms);
void Measure_Impedance_Sweep(float excite_mA, uint32_t settle_ms, float Z_out[50]);

// User-provided hooks for analog I/O (weak defaults are provided in .c)
__weak float Read_B_voltage(uint8_t Bn);
__weak void  Apply_Excitation(uint8_t B_pos, uint8_t B_neg, float current_mA);
__weak void  Stop_Excitation(void);
__weak float Measure_Response_V(uint8_t Bp, uint8_t Bn);

#endif // MOS_CONTROLLER_H
