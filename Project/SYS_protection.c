#include "SYS_protection.h"
#include <math.h>
#include <stddef.h>
#include "ds18b20.h"
#include "AD7606.h"
#include "global_command.h"



// 电池状态

float battery_voltage;             // 当前电池电压(V)
float battery_current;             // 工作电流(A)
float excitation_current;          // 激励电流(A)
float high_side_voltage;           // 高侧相邻电池电压(V)
float low_side_voltage;            // 低侧相邻电池电压(V)
float power_supply_voltage;        // 供电电源电压(V)
float ambient_temperature;         // 环境温度(°C)
bool protection_triggered;          // 保护触发标志



// 保护阈值定义（这里需要丙生从FLASH读取）
float over_voltage_threshold = 4.2f;        // 过压保护阈值(V)
float under_voltage_threshold = 2.8f;      // 欠压保护阈值(V)
float over_current_charge_threshold = 2.0f; // 充电过流阈值(A)
float over_current_discharge_threshold = 3.0f; // 放电过流阈值(A)
float over_temperature_threshold = 60.0f;   // 过温保护阈值(°C)
uint16_t protection_delay_ms = 100;        // 保护延时(ms)






void  protection_check(void)
{

    // 1. 检查电池过压/欠压
    if (battery_voltage > over_voltage_threshold) {
        printf("保护触发：电池过压 (%.2fV > %.2fV)\n", battery_voltage, over_voltage_threshold);
        g_current_state = SYS_FAULT;
        g_system_error_code=VOLT_HIGH; 
    }
    else if (battery_voltage < under_voltage_threshold) {
        printf("保护触发：电池欠压 (%.2fV < %.2fV)\n",  battery_voltage, under_voltage_threshold);
        g_current_state = SYS_FAULT;
        g_system_error_code=VOLT_LOW  ; 
    }
    
    // 2. 检查充电过流（电流为正表示充电）
    else if (battery_current > 0 && battery_current > over_current_charge_threshold) {
        printf("保护触发：充电过流 (%.2fA > %.2fA)\n", battery_current, over_current_charge_threshold);
        g_current_state = SYS_WARNING;
        g_system_error_code= CURR_CHG_HIGH ; 
    }
    
    // 3. 检查放电过流（电流为负表示放电）
    else if (battery_current < 0) {
        float abs_current = -battery_current; // 取绝对值
        if (abs_current > over_current_discharge_threshold) {
            printf("保护触发：放电过流 (%.2fA > %.2fA)\n", abs_current, over_current_discharge_threshold);
            g_current_state = SYS_WARNING;
            g_system_error_code= CURR_DIS_HIGH ; 					
        }
    }
        
    // 4. 检查相邻电池电压平衡性
    else if (fabsf(high_side_voltage - battery_voltage) > 0.5f) {
        printf("保护触发：高侧电池电压不平衡 (当前:%.2fV, 高侧:%.2fV)\n", battery_voltage, high_side_voltage);
        g_current_state = SYS_FAULT;
        g_system_error_code= VOLT_BALANC_ERR ; 
    }
    else if (fabsf(low_side_voltage - battery_voltage) > 0.5f) {
        printf("保护触发：低侧电池电压不平衡 (当前:%.2fV, 低侧:%.2fV)\n", battery_voltage, low_side_voltage);
         g_current_state = SYS_FAULT;
         g_system_error_code= VOLT_BALANC_ERR ;      
    }
    
    
    // 5. 检查环境温度
    else if (ambient_temperature > over_temperature_threshold) {
        printf("保护触发：环境温度过高 (%.1f°C > %.1f°C)\n", ambient_temperature, over_temperature_threshold);
         g_current_state = SYS_WARNING;
         g_system_error_code= TEMP_HIGH ;      
    }
    
    // 所有检查通过，系统正常
    else {
        printf("系统参数正常 - 电压:%.2fV, 电流:%.2fA, 温度:%.1f°C\n", battery_voltage, battery_current, ambient_temperature);
    }
    
}








float read_battery_current(void)  // 使用霍尔电流传感器读取电池工作电流
{
    
    s16 DB_data[6] = {0};   
    AD7606_StartConvst();
    AD7606_ReadData(DB_data);    
	  float i= 100* AD7606B_Digital2Voltage(DB_data[2]);
		printf("电池工作电流 = %.3fV\r\n", i);	 
    return i;
}

float read_excitation_current(void)  // 读取激励电流
{
   
    s16 DB_data[6] = {0};   
    AD7606_StartConvst();
    AD7606_ReadData(DB_data);    
	  float i= 2* AD7606B_Digital2Voltage(DB_data[0])-5;
		printf("激励电流 = %.3fV\r\n", i);	 
    
		
		return i;
}

float read_battery_voltage(void) // 读取当前电池电压
{
   
    s16 DB_data[6] = {0};   
    AD7606_StartConvst();
    AD7606_ReadData(DB_data);    
	  printf("当前电池电压 = %.3fV\r\n", AD7606B_Digital2Voltage(DB_data[3]));	 
    return AD7606B_Digital2Voltage(DB_data[3]);
}

float read_high_side_voltage(void)// 读取高侧相邻电池电压
{
    s16 DB_data[6] = {0};   
    AD7606_StartConvst();
    AD7606_ReadData(DB_data);    
	  printf("高测相邻电池电压 = %.3fV\r\n", AD7606B_Digital2Voltage(DB_data[4]));	 
    return AD7606B_Digital2Voltage(DB_data[4]);
}

float read_low_side_voltage(void)// 读取低侧相邻电池电压
{
    
    s16 DB_data[6] = {0};   
    AD7606_StartConvst();
    AD7606_ReadData(DB_data);    
	  printf("低测相邻电池电压 = %.3fV\r\n", AD7606B_Digital2Voltage(DB_data[5]));	 
    return AD7606B_Digital2Voltage(DB_data[5]);
}

float read_power_supply_voltage(void)// 手持设备读取供电源电压
{    	
	  s16 DB_data[6] = {0};   
    AD7606_StartConvst();
    AD7606_ReadData(DB_data);    
	  printf("电源电压 = %.3fV\r\n", AD7606B_Digital2Voltage(DB_data[5]));	 
    return AD7606B_Digital2Voltage(DB_data[5]);
}

float read_ambient_temperature(void)
{
    // 读取环境温度[1,6](@ref)
    return ds18b20_get_temperature_float(); 
}






