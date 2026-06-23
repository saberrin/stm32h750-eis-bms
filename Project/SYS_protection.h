#ifndef SYS_PROTECTION_H
#define SYS_PROTECTION_H

#include <stdint.h>
#include <stdbool.h>




// 参数读取函数
float read_battery_current(void);
float read_excitation_current(void);
float read_battery_voltage(void);
float read_high_side_voltage(void);
float read_low_side_voltage(void);
float read_power_supply_voltage(void);
float read_ambient_temperature(void);

void protection_check(void);

#endif // SYS_PROTECTION_H
