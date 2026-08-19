// global_command.h
#ifndef __GLOBAL_COMMAND_H__
#define __GLOBAL_COMMAND_H__
#include "system.h"


#include <string.h>



//extern volatile int abortCommandReceived; // 声明全局变量
//extern volatile int pauseCommandReceived;

//extern volatile int System_busy_status;  //如果处在系统忙碌状态，千万不要处理复杂中断任务！！！

//typedef enum { bFALSE = 0, bTRUE = 1 } BoolFlag;

//extern volatile int startCommandReceived;

//extern volatile uint16_t commandValue;

//extern volatile uint32_t QG_NumPerDecades;

//extern  volatile uint32_t QG_SweepPoints; 
//extern volatile BoolFlag QG_SweepEn;
//extern volatile float QG_SweepStartFreq;
//extern volatile float QG_SweepStopFreq;
//extern volatile float QG_SinFreq;
//extern volatile float QG_ACVoltPP;
//extern volatile float QG_DCVolt;	
//extern volatile float QG_RcalVal; 
//extern volatile float QG_Temperature;	
//extern volatile BoolFlag QG_SweepLog;

//extern volatile int EIS_Test_Cycles;//循环测量EIS的次数

//extern volatile int Print_UIT_status;


// 系统状态码定义
#define STATUS_NORMAL      0   // 系统正常[1,2](@ref)

// 故障码定义（非零值表示异常）
#define UNKNOWN_CMD       1   // 未知命令
#define BAD_FORMAT        2   // 格式错误
#define CELL_OOR          3   // 电芯参数超范围（Out Of Range）
#define PARAM_OOR         4   // 通用参数超范围
#define MODE_INVALID      5   // 无效工作模式
#define TEMP_HIGH         6   // 温度过高
#define VOLT_HIGH         7   // 电压过高
#define VOLT_LOW          8   // 电压过低
#define ADC_FAULT         9   // ADC故障
#define SENSOR_FAIL       10  // 传感器故障
#define MEMORY_FULL       11  // 存储器满
#define EIS_CALIB         12  // EIS校准错误
#define EIS_NOISE         13  // EIS噪声过大
#define EIS_TIMEOUT       14  // EIS测量超时
#define VOLT_BALANC_ERR   15  // 相邻电池电压不平衡
#define CURR_CHG_HIGH    16  // 外部充电电流过大
#define CURR_DIS_HIGH    17  // 外部放电电流过大
#define SWEET            18    //无故障


extern volatile float    QG_ACVoltPP;					//设置EIS激励信号幅值 (mV)。
extern volatile float    QG_DCVolt;			//设置EIS激励信号的直流偏置电压 (mV)。
					
extern volatile float    QG_EIS_FREQ_START;					//设置扫频起始频率 (Hz)
extern volatile float    QG_EIS_FREQ_END ;					//设置扫频终止频率 (Hz)
extern volatile uint32_t QG_EIS_FREQ_POINTS; 					//设置每10倍频几个点
extern volatile float		 QG_TEMP_HIGH_ALARM;		//设置温度过高报警阈值(℃)
extern volatile float    QG_VOLT_CELL_HIGH;					//设置电压上限阈值(V)
extern volatile float    QG_VOLT_CELL_LOW;						//设置电压下限阈值(V)
extern volatile float    QG_CURR_CHG_ALARM;						//设置外部充电电流过流报警阈值（超过应停止测量）
extern volatile float		 QG_CURR_DIS_ALARM;	//设置外部放电电流过流报警阈值（超过应停止测量）
extern volatile uint32_t QG_CELL_COUNT ;		//设置电池包内电芯数量
extern volatile float    SET_CALIB_DATA[10];			//设置测量通道（电压、电流、温度）的校准系数数组


extern volatile double CommandParam1 ;    //命令中分离出的参数1
extern volatile double CommandParam2 ;    //命令中分离出的参数2

extern volatile int Cell_ID;    //当前检测的电芯号（随开关矩阵切换变化）

#define QG_ID 0x11

// 系统状态定义
typedef enum {
    SYS_READY,          // 系统就绪，空闲状态
    SYS_FAULT,          // 严重故障（死循环）
    SYS_WARNING,        // 设备异常（可执行基本检测）
    SYS_EIS_SWEEP,      // EIS扫频测量
    SYS_EIS_SINGLE,     // EIS单频点测量
    SYS_GITT_MEASURE,   // GITT测量
    SYS_CALIBRATION     // 系统校准
} SystemState_t;


extern volatile SystemState_t g_current_state;// 全局状态变量，记录系统当前状态

extern volatile int g_system_error_code;  // 记录系统错误码


#endif /* __GLOBAL_COMMAND_H__ */



