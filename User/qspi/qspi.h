#ifndef __QSPI_H__
#define __QSPI_H__

#include "stm32h7xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

extern QSPI_HandleTypeDef hqspi;

/* 初始化 QSPI（不进入 Memory-Mapped，纯间接模式） */
void QSPI_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __QSPI_H__ */
