#ifndef __DS18B20_H
#define __DS18B20_H

#include "main.h"

/* DS18B20引脚定义 - 使用PE11 */
#define DS18B20_DQ_GPIO_PORT GPIOE
#define DS18B20_DQ_GPIO_PIN GPIO_PIN_6

/* GPIO时钟使能 */
#define DS18B20_DQ_GPIO_CLK_ENABLE() do{ __HAL_RCC_GPIOE_CLK_ENABLE(); }while(0)

/* IO操作函数 */
#define DS18B20_DQ_OUT(x) do{ \
    (x) ? \
        HAL_GPIO_WritePin(DS18B20_DQ_GPIO_PORT, DS18B20_DQ_GPIO_PIN, GPIO_PIN_SET) : \
        HAL_GPIO_WritePin(DS18B20_DQ_GPIO_PORT, DS18B20_DQ_GPIO_PIN, GPIO_PIN_RESET); \
}while(0)

#define DS18B20_DQ_IN HAL_GPIO_ReadPin(DS18B20_DQ_GPIO_PORT, DS18B20_DQ_GPIO_PIN)

/* 函数声明 */
void TEM_delay_us(uint32_t nus);


uint8_t ds18b20_init(void);
void DS18B20_init(void);
short ds18b20_get_temperature(void);
float ds18b20_get_temperature_float(void);

#endif /* __DS18B20_H */
