#include "SYS_protection.h"
#include <math.h>
#include <stddef.h>
#include "ds18b20.h"
#include "AD7606.h"
#include "global_command.h"
#include "MOS_Controller.h"


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



uint8_t CheckCellVoltage(uint8_t cell_index)
{
    float V_TOP, V_MID, V_BOT;

    // 第一次采样
    V_TOP = ADS131A0X_Read_Channel(2)*2.50248756f - ADS131A0X_Read_Channel(1);
    V_MID = ADS131A0X_Read_Channel(1);
    V_BOT = 3.2;//(2.63f-ADS131A0X_Read_Channel(3)*1.66562f)/0.666562f;  目前硬件有点BUG，需更改硬件,暂时定死3.2V!!!!!!!!!!!!!!

    printf("V_TOP=  %.3f V\r\n", V_TOP);
    printf("V_MID=  %.3f V\r\n", V_MID);
    printf("V_BOT=  %.3f V\r\n", V_BOT);

    // 判断：任意一个电压超出区间即为异常
    uint8_t first_bad = 0;
    if( V_TOP < 3.0f || V_TOP > 3.5f ||
        V_MID < 3.0f || V_MID > 3.5f ||
        V_BOT < 3.0f || V_BOT > 3.5f )
    {
        first_bad = 1;
    }

    // 第一次全部正常，直接返回0
    if(first_bad == 0)
    {
        return 0;
    }

    // 第一次异常，延时重测
    delay_ms(10);

    // 第二次采样
    V_TOP = ADS131A0X_Read_Channel(2)*2.50248756f - ADS131A0X_Read_Channel(1);
    V_MID = ADS131A0X_Read_Channel(1);
    V_BOT =3.2;// (2.63f-ADS131A0X_Read_Channel(3)*1.66562f)/0.666562f;



		
    printf("Retry: V_TOP=  %.3f V\r\n", V_TOP);
    printf("Retry: V_MID=  %.3f V\r\n", V_MID);
    printf("Retry: V_BOT=  %.3f V\r\n", V_BOT);

    uint8_t second_bad = 0;
    if( V_TOP < 3.0f || V_TOP > 3.5f ||
        V_MID < 3.0f || V_MID > 3.5f ||
        V_BOT < 3.0f || V_BOT > 3.5f )
    {
        second_bad = 1;
    }

    // 第二次采样恢复正常，干扰，返回0
    if(second_bad == 0)
    {
        return 0;
    }

    // 两次采样均异常，打印详细故障信息，返回1
    printf("===== Cell %d group voltage abnormal =====\r\n", cell_index);
    if(V_TOP < 3.0f)
        printf("V_TOP(cell%d): %.3f V, too LOW\r\n", cell_index+1, V_TOP);
    else if(V_TOP > 3.5f)
        printf("V_TOP(cell%d): %.3f V, too HIGH\r\n", cell_index+1, V_TOP);

    if(V_MID < 3.0f)
        printf("V_MID(cell%d): %.3f V, too LOW\r\n", cell_index, V_MID);
    else if(V_MID > 3.5f)
        printf("V_MID(cell%d): %.3f V, too HIGH\r\n", cell_index, V_MID);

    if(V_BOT < 3.0f)
        printf("V_BOT(cell%d): %.3f V, too LOW\r\n", cell_index-1, V_BOT);
    else if(V_BOT > 3.5f)
        printf("V_BOT(cell%d): %.3f V, too HIGH\r\n", cell_index-1, V_BOT);

    printf("EIS will skip this battery\r\n");
    return 1;
}







///**
// * @brief 检测指定电池及其相邻电池的电压是否正常
// * @param cell_index 电池编号（范围 1~52）
// * @return uint8_t 0=正常，1=异常（当前电池或相邻电池存在欠压/过压）
// */
//uint8_t CheckCellVoltage(uint8_t cell_index)
//{
//    float voltage;
//    uint8_t result = 0;
//    uint8_t neighbor_idx[2];  // 存储相邻电池编号
//    uint8_t neighbor_count = 0;
//    uint8_t i;

//    // ========== 确定相邻电池列表 ==========
//    if (cell_index == 1)
//    {
//        neighbor_idx[0] = 2;
//        neighbor_idx[1] = 3;
//        neighbor_count = 2;
//    }
//    else if (cell_index == 52)
//    {
//        neighbor_idx[0] = 50;
//        neighbor_idx[1] = 51;
//        neighbor_count = 2;
//     }
//    else
//    {
//        neighbor_idx[0] = cell_index - 1;
//        neighbor_idx[1] = cell_index + 1;
//        neighbor_count = 2;
//    }

//    // ========== 第一步：检测当前电池（无需切换） ==========
//    // 此时继电器已在 cell_index 位置
//    delay_ms(5);  // 确保采样稳定

//    voltage = ADS131A0X_Read_Channel(3);

//    /* 第一次检测正常 */
//    if (voltage >= 3.0f && voltage <= 3.4f)
//    {
//        ; // 正常
//    }
//    else
//    {
//        /* 第一次检测异常，延迟后再次确认 */
//        delay_ms(10);
//        voltage = ADS131A0X_Read_Channel(3);

//        /* 第二次恢复正常，认为是干扰 */
//        if (voltage >= 3.0f && voltage <= 3.4f)
//        {
//            ; // 正常
//        }
//        else
//        {
//            /* 两次都异常，确认真实故障 */
//            if (voltage < 3.0f)
//                printf("Cell %d voltage too low (%.3f V)! EIS will skip this battery\r\n", cell_index, voltage);
//            else if (voltage > 3.4f)
//                printf("Cell %d voltage too high (%.3f V)! Overvoltage warning!\r\n", cell_index, voltage);

//            result = 1;
//        }
//    }

//    // ========== 第二步：检测相邻电池 ==========
//    for (i = 0; i < neighbor_count; i++)
//    {
//        // 切换到相邻电池
//        SwitchWindow_Program(neighbor_idx[i]);
//        delay_ms(5);

//        voltage = ADS131A0X_Read_Channel(3);

//        /* 第一次检测正常 */
//        if (voltage >= 3.0f && voltage <= 3.4f)
//        {
//            continue;
//        }

//        /* 第一次检测异常，延迟后再次确认 */
//        delay_ms(10);
//        voltage = ADS131A0X_Read_Channel(3);

//        /* 第二次恢复正常，认为是干扰 */
//        if (voltage >= 3.0f && voltage <= 3.4f)
//        {
//            continue;
//        }

//        /* 两次都异常，确认真实故障 */
//        if (voltage < 3.0f)
//            printf("Cell %d voltage too low (%.3f V)! EIS will skip this battery\r\n", neighbor_idx[i], voltage);
//        else if (voltage > 3.4f)
//            printf("Cell %d voltage too high (%.3f V)! Overvoltage warning!\r\n", neighbor_idx[i], voltage);

//        result = 1;
//    }

//    // ========== 第三步：恢复当前电池通道 ==========
//    SwitchWindow_Program(cell_index);

//    return result;
//}







