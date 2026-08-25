#include "system.h"
#include "delay.h"
#include "led.h"
#include "key.h"
#include "dac.h"
#include "timer.h"
#include "usart.h"
#include "delay.h"
#include "SineWave.h"
#include "math.h"
#include "parse_command.h"
#include "EIS_Measure.h"
#include "global_command.h"
#include "watchdog.h"
#include "fdcan.h"
#include "AD7606.h"
#include "dac8830.h"
#include "w25qxx.h"
#include "config_flash.h"
#include "ds18b20.h"
#include "norflash_ex.h"
#include "AD620_GainCtrl.h"
#include "Potentiometer_Calibration.h"
#include "DAC_Linear_Calibration.h"
#include "MOS_Controller.h"
#include "current_excitation_calibrator.h"
#include "current_acquisition_calibrator.h"
#include "SYS_protection.h"
#include "ADS131A04.h"
#include "identity_flash.h"
#include "runtime_flash.h"
#include "calib_flash.h"

//-----------------------------------------------------------------
#define TIME_CLK 200000000
#define PI 3.14159265358979323846
#define FLASH_ID_W25Q128   0xEF17   // W25Q128 厂家+设备 ID
//-----------------------------------------------------------------
// 主程序
//-----------------------------------------------------------------

const uint8_t g_text_buf[] = {"STM32H7 QSPI FLASH TEST"};

#define TEXT_SIZE    sizeof(g_text_buf)     /* TEXT字符串长度 */

int main(void)
{
    CPU_CACHE_Enable();             // 启用CPU缓存
    HAL_Init();                     // 初始化HAL库
    MPU_Memory_Protection();        // 设置保护区域
    SystemClock_Config();           // 设置系统时钟,400Mhz
    SysTick_clkconfig(400);         // SysTick参数配置
    TIM7_Init(10000 - 1, 20000 - 1); // 独立1秒运行统计，不改动TIM6采样时基
    uart_init(115200);

    KEY_Init();                     // 初始化按键
    LED_Init();                     // 初始化LED
    SwitchMatrix_Init();
    Power5200_Init();

    DS18B20_init();                 // 温度传感器初始化
    DAC8830_Init();                 // DAC8830初始化
    norflash_init();
    W25QXX_Init();                  // flash初始化
    ADS131A0X_Init();
    float temp_float = ds18b20_get_temperature_float();   // 测量温度
    printf("温度：%.1f°C\r\n", temp_float);

    // 开启2.048V的内部参考电压（如果使用外部基准电压的话，请屏蔽开启内部参考电压的程序）
    MY_VREFBUF_Init(SYSCFG_VREFBUF_VOLTAGE_SCALE0);

    uint16_t id = norflash_ex_read_id();

    printf("%u\n", id);

    // 校验通过（id == 0xEF17）后：
    if (id == FLASH_ID_W25Q128) {
        printf("W25Q128 detected.\n");

        //QSPI_SmokeTest();                 //  调一次烟囱测试

        ConfigFlash_LoadAtStartup();      // 再进你的配置读流程
        IdFlash_LoadAtStartup();
        RuntimeFlash_LoadAtStartup();
        RuntimeFlash_OnBoot();
        RuntimeFlash_SaveIfDirty();
        CalibFlash_LoadAtStartup();
        CalibFlash_Dump(NULL);
    } else {
        printf("Flash ID mismatch or not W25Q128: 0x%04X\r\n", id);
    }

    // 用有限重试，避免无限死循环
    int tries = 0;
    while (id != W25Q128 && tries++ < 20) {
        printf("W25Qxx Check Failed! id=0x%04X\r\n", id);
        delay_ms(50);
        id = W25QXX_ReadID();
    }

    // 统一把总线还给 ADC，后续采样用
    AD7606_Init();

    FDCAN1_Mode_Init(10, 4, 34, 5, FDCAN_MODE_NORMAL);  // EIS_CORE: 500 kbit/s, sample point 0.875

    FAN_Init();
    FAN_Enable();
    RELAY_Init();



    // 主循环
    while (1) {

        Runtime_BackgroundTask();

        switch (g_current_state) {
            case SYS_READY:      //************************系统空闲状态，已就绪************************
                Power5200_Disable();               // 关闭激励MOS开关
                SwitchWindow_Program(0);           // 关闭所有电芯选通开关
                break;

            case SYS_FAULT:      //************************严重故障状态************************
                                 // 进入死循环，关闭危险输出   记录错误日志   常量红色LED报警    等待硬件复位或特定恢复指令
                while (1) {
                    LED_Red_On();
                    LED_Green_Off();
                    Power5200_Disable();               // 关闭激励MOS开关
                    SwitchWindow_Program(0);           // 关闭所有电芯选通开关
                }

            case SYS_WARNING:   //************************设备异常状态************************
                                // 降级运行，执行基本的安全检测，等待恢复，恢复成功进入READY状态

                // 当检测到任意一个指定故障时，循环等待
                while (g_system_error_code == TEMP_HIGH ||
                        g_system_error_code == CURR_CHG_HIGH ||
                        g_system_error_code == CURR_DIS_HIGH) {
                    // 加入延时，避免CPU过载
                    delay_ms(100); // 假设延时100毫秒，请使用您自己的延时函数
                    //          protection_check(); // 继续检查保护状态
                }
                g_current_state = SYS_READY;
                break;

            case SYS_EIS_SWEEP: //************************扫频EIS测试程序************************
               set_excitation_current(QG_ACVoltPP);// 设置激励电流大小，单位A
                if (CommandParam1 == 0) {
                    for (int i = 2; i <= 52; i++) {     //循环过程要加保护检测!!!!!!!!!!!
                        SwitchWindow_Program(i);

											
											
											
                        printf("SwitchWindow_Program: s=%d\r\n", i);

                       
                        if (CheckCellVoltage(i)) {
                         continue;}
											  

                        EIS_FrequencySweep_Measure(QG_EIS_FREQ_START, QG_EIS_FREQ_END);
                        RuntimeFlash_OnEisFinished();
                    }
                } else if (0 < CommandParam1 && CommandParam1 < 53) {
                    SwitchWindow_Program(CommandParam1);                              // 切换到参数1定义的电芯号
									  if (!CheckCellVoltage(CommandParam1)) {
                        
                         EIS_FrequencySweep_Measure(QG_EIS_FREQ_START, QG_EIS_FREQ_END);
                         RuntimeFlash_OnEisFinished();
										}																
                     
                    
                } else
                    printf("输入了错误的电池编号");
                g_current_state = SYS_READY;
                break;

            case SYS_EIS_SINGLE: //************************单频EIS测试程序************************
               set_excitation_current(QG_ACVoltPP);// 设置激励电流大小，单位A
                if (CommandParam1 == 0) {
                    for (int i = 1; i <= 52; i++) {     //循环过程要加保护检测!!!!!!!!!!!
                        SwitchWindow_Program(i);
											  if (CheckCellVoltage(i)) {
                         continue;}
											
                        EIS_SingleFrequency_Measure(CommandParam2);  //  设定单频扫频频率，单位HZ
                        RuntimeFlash_OnEisFinished();
                    }
                } else if (0 < CommandParam1 && CommandParam1 < 53) {
                    SwitchWindow_Program(CommandParam1);                              // 切换到参数1定义的电芯号
									  if (!CheckCellVoltage(CommandParam1)) {
                        
                      EIS_SingleFrequency_Measure(CommandParam2);
                      RuntimeFlash_OnEisFinished();

										}		
									
									
                                         //  设定单频扫频频率，单位HZ
                } else
                    printf("输入了错误的电池编号");
                g_current_state = SYS_READY;
                break;

            case SYS_GITT_MEASURE: //************************GITT测试程序************************

                // Power5200_Enable() ;
                // OutputConstantCurrent(0.2);//输出恒流，单位A
                // Power5200_Disable() ;
                g_current_state = SYS_READY;
                break;

            case SYS_CALIBRATION: //************************系统校准程序************************

                //  DAC_LinearCalibration_Init();// 初始化DAC线性校准系统
                //  DAC_AutoCalibration(0.5f, 4.5f, 128);
                PotentiometerCalibration_Init();            // 初始化校准系统
                Run_Complete_Calibration_Test();            // 运行完整测试流程
                //  CurrentExcitationCalibrator_Init(&UART_Handler); // 校准激励电流设定与产生是否一致
                //  CurrentAcquisitionCalibrator_Init(&UART_Handler); // 校准激励电流真实值与采集值是否一致
                //  CALIB_RunFullCalibration();
                g_current_state = SYS_READY;
                break;

            default:
                g_current_state = SYS_READY;  // 复位到ready
                break;
        }

        // 执行基础检测任务，主动上报  温度、电压、电流等

        LED_Green_On();
       
        delay_ms(50);
        LED_Green_Off();
       
        delay_ms(50);
        Power5200_Disable();
				 printf("通道0电压为  %.3f V\r\n",  ADS131A0X_Read_Channel(0)   );
				printf("通道1电压为  %.3f V\r\n",  ADS131A0X_Read_Channel(1)   );
				printf("通道2电压为  %.3f V\r\n",  ADS131A0X_Read_Channel(2)   );
				printf("通道3电压为  %.3f V\r\n",  ADS131A0X_Read_Channel(3)   );
    }

}

//-----------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------


















//#include "system.h"
//#include "delay.h"
//#include "led.h"
//#include "key.h"
//#include "dac.h"
//#include "timer.h"
//#include "usart.h"
//#include "delay.h"
//#include "SineWave.h"
//#include "math.h"
//#include "parse_command.h"
//#include "EIS_Measure.h"
//#include "global_command.h"
//#include "watchdog.h"
//#include "fdcan.h"
//#include "AD7606.h"
//#include "dac8830.h"
//#include "w25qxx.h"
//#include "config_flash.h"
//#include "ds18b20.h"
//#include "norflash_ex.h"
//#include "AD620_GainCtrl.h"
//#include "Potentiometer_Calibration.h"
//#include "DAC_Linear_Calibration.h"
//#include "MOS_Controller.h"
//#include "current_excitation_calibrator.h"
//#include "current_acquisition_calibrator.h"
//#include "SYS_protection.h"
//#include "ADS131A04.h"



////-----------------------------------------------------------------
//#define TIME_CLK 200000000
//#define PI 3.14159265358979323846
//#define FLASH_ID_W25Q128   0xEF17   // W25Q128 厂家+设备 ID
////-----------------------------------------------------------------
//// 主程序
////-----------------------------------------------------------------

//const uint8_t g_text_buf[] = {"STM32H7 QSPI FLASH TEST"};

//#define TEXT_SIZE    sizeof(g_text_buf)     /* TEXT字符串长度 */




//int main(void)
//{
//	
//	
//	CPU_CACHE_Enable();      // 启用CPU缓存
//  HAL_Init();          		// 初始化HAL库
//	MPU_Memory_Protection();// 设置保护区域
//	SystemClock_Config(); 	// 设置系统时钟,400Mhz  
//	SysTick_clkconfig(400);	// SysTick参数配置
//	uart_init(115200);
//		
//	KEY_Init();							// 初始化按键
//	LED_Init();           	// 初始化LED 
//	SwitchMatrix_Init();
//	Power5200_Init( ) ;


//	
//	
//  DS18B20_init();   //温度传感器初始化
//	DAC8830_Init();// DAC8830初始化
//	norflash_init();
//	W25QXX_Init(); // flash初始化	
//  ADS131A0X_Init();
//	float temp_float = ds18b20_get_temperature_float();   //测量温度
//	printf("温度：%.1f°C\r\n", temp_float);


//	
//	
//	// 开启2.048V的内部参考电压（如果使用外部基准电压的话，请屏蔽开启内部参考电压的程序）
//	MY_VREFBUF_Init(SYSCFG_VREFBUF_VOLTAGE_SCALE0);
//	

//	uint16_t id = norflash_ex_read_id();

//	printf("%u\n", id);

//		// 校验通过（id == 0xEF17）后：
//	if (id == FLASH_ID_W25Q128) {
//			printf("W25Q128 detected.\n");
//			
//			//QSPI_SmokeTest();                 //  调一次烟囱测试
//			
//			ConfigFlash_LoadAtStartup();      // 再进你的配置读流程
//	} else {
//			printf("Flash ID mismatch or not W25Q128: 0x%04X\r\n", id);
//	}

//	// 用有限重试，避免无限死循环
//	int tries = 0;
//	while (id != W25Q128 && tries++ < 20) {
//			printf("W25Qxx Check Failed! id=0x%04X\r\n", id);
//			delay_ms(50);
//			id = W25QXX_ReadID();
//	}

//	// 统一把总线还给 ADC，后续采样用
//	AD7606_Init();

//	
//	FDCAN1_Mode_Init(10,8,31,8,FDCAN_MODE_NORMAL);  //回环测试
//	
//	FAN_Init();
//	 FAN_Enable();
//	 RELAY_Init( );





//g_current_state=SYS_EIS_SWEEP;

//	
//	
//	
//	

//// while(1) {

////			//	  ADS131A0X_ReadData_Print();
////	 

////	 
////	 
////    float volt = ADS131A0X_Read_Channel(3);
////    
////    // 关键！乘1000输出mV，和ReadData_Print保持一致
////    printf(" %7.3f \r\n", volt * 1000);
//// 
////	
////       // delay_ms(500);
////        }
//// 



//				
//				

////while(1){
////																	for(int i=3;i<=52;i++)		//循环过程要加保护检测!!!!!!!!!!!							
////																{
////																	

////																	SwitchWindow_Program(i);
////																
////																	printf("SwitchWindow_Program: s=%d\r\n",i);
////																	
////																	 delay_ms(5000);
////																	
////																	
////																}	


////															}



////RELAY_Enable();
////delay_ms(1000);

////RELAY_Disable();
////while(1)
////{}

////while(1)
////{
//// FAN_Enable();
////	RELAY_Enable();
////	LED_Green_On();		
////	LED_Red_Off();
////delay_ms(5000);
////	
//// FAN_Disable();
////	RELAY_Disable();
////		LED_Green_Off();		
////	LED_Red_On();
////	delay_ms(5000);
////	
////	
////}

//// 主循环
//    while (1) {
//       
//        switch (g_current_state) {
//            case SYS_READY:      //************************系统空闲状态，已就绪************************ 
//														Power5200_Disable() ;     //关闭激励MOS开关
//														SwitchWindow_Program(0);  //关闭所有电芯选通开关                                   
//														break;

//            case SYS_FAULT:      //************************严重故障状态************************   进入死循环，关闭危险输出   记录错误日志   常量红色LED报警    等待硬件复位或特定恢复指令																		
//														while(1)
//														{
//														
//															LED_Red_On();
//															LED_Green_Off();									
//															Power5200_Disable() ;     //关闭激励MOS开关
//															SwitchWindow_Program(0);  //关闭所有电芯选通开关 
//																
//														}
//							
//            case SYS_WARNING:   //************************设备异常状态************************ // 降级运行，执行基本的安全检测，等待恢复，恢复成功进入READY状态

//					                	// 当检测到任意一个指定故障时，循环等待
//														while(g_system_error_code == TEMP_HIGH || 
//																	g_system_error_code == CURR_CHG_HIGH || 
//																	g_system_error_code == CURR_DIS_HIGH) 
//														{
//																// 加入延时，避免CPU过载
//																delay_ms(100); // 假设延时100毫秒，请使用您自己的延时函数
//													//			protection_check(); // 继续检查保护状态
//														}
//														g_current_state = SYS_READY;
//														break;

//            case SYS_EIS_SWEEP: //************************扫频EIS测试程序************************  
//                
//															if(CommandParam1==0)
//															{
//																	for(int i=3;i<=52;i++)		//循环过程要加保护检测!!!!!!!!!!!							
//																{
//																	


//																	SwitchWindow_Program(i);
//																
//																	printf("SwitchWindow_Program: s=%d\r\n",i);
//																	
//																//	 set_excitation_current(QG_ACVoltPP);
//																	 set_excitation_current(2);
//																	 delay_ms(1000);
//																	 EIS_FrequencySweep_Measure(QG_EIS_FREQ_START,QG_EIS_FREQ_END);
//																}				
//															}	
//															else if (0<CommandParam1&&CommandParam1<53)
//															{
//																	SwitchWindow_Program(CommandParam1);        //切换到参数1定义的电芯号
//																	set_excitation_current(QG_ACVoltPP); // 设置激励电流大小，单位A  
//																	EIS_FrequencySweep_Measure(QG_EIS_FREQ_START,QG_EIS_FREQ_END);
//															}
//															else	
//																printf("输入了错误的电池编号");
//																g_current_state = SYS_READY;
//                             break;

//            case SYS_EIS_SINGLE: //************************单频EIS测试程序************************ 
//																
//						
//						                 if(CommandParam1==0)
//															{
//																	for(int i=0;i<52;i++)		//循环过程要加保护检测!!!!!!!!!!!							
//																{
//																	 SwitchWindow_Program(i);
//																	 EIS_SingleFrequency_Measure(CommandParam2);  //  设定单频扫频频率，单位HZ
//																}				
//															}	
//															else if (0<CommandParam1&&CommandParam1<53)
//															{
//						                    SwitchWindow_Program(CommandParam1);        //切换到参数1定义的电芯号
//																EIS_SingleFrequency_Measure(CommandParam2);  //  设定单频扫频频率，单位HZ
//						                  }
//															
//															else	
//																printf("输入了错误的电池编号");
//																g_current_state = SYS_READY;
//																break;

//            case SYS_GITT_MEASURE: //************************GITT测试程序************************                                 						
//																
//						                     // Power5200_Enable() ;
//																//OutputConstantCurrent(0.2);//输出恒流，单位A
//																// Power5200_Disable() ;
//																g_current_state = SYS_READY;
//                               break;

//            case SYS_CALIBRATION: //************************系统校准程序************************ 
//																  
//																//  DAC_LinearCalibration_Init();// 初始化DAC线性校准系统     
//																//	DAC_AutoCalibration(0.5f, 4.5f, 128);
//																  PotentiometerCalibration_Init();// 初始化校准系统
//																	Run_Complete_Calibration_Test(); // 运行完整测试流程
//																//  CurrentExcitationCalibrator_Init(&UART_Handler); // 校准激励电流设定与产生是否一致
//																//  CurrentAcquisitionCalibrator_Init(&UART_Handler); // 校准激励电流真实值与采集值是否一致
//																//  CALIB_RunFullCalibration();
//															      g_current_state = SYS_READY;
//						                     break;
//            default:                
//																g_current_state = SYS_READY;  //复位到ready               
//																break;
//        }

//        // 执行基础检测任务，主动上报  温度、电压、电流等
//         
//				LED_Green_On();
//				
//					LED_Red_Off();
//		    delay_ms(300);
//		    LED_Green_Off();
//			LED_Red_On();
//		    delay_ms(300);

//    }
//	
//	
//	
//	
//}

////-----------------------------------------------------------------
//// End Of File
////----------------------------------------------------------------- 
