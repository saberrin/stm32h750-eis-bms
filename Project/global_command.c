#include "global_command.h"
#include <math.h>
//volatile int abortCommandReceived = 0; // 定义全局变量中断标志

//volatile int pauseCommandReceived = 0;



//volatile int startCommandReceived = 0;

//volatile int System_busy_status=0;

//volatile int Print_UIT_status=0;







volatile float    QG_ACVoltPP=2.0f;					//设置EIS激励电流信号幅值 (A)。
volatile float    QG_DCVolt=200.0f;			//设置EIS激励信号的直流偏置电压 (mV)。
					
volatile float    QG_EIS_FREQ_START= 10000.0f;					//设置扫频起始频率 (Hz)
volatile float    QG_EIS_FREQ_END = 10.0f;					//设置扫频终止频率 (Hz)
volatile uint32_t QG_EIS_FREQ_POINTS= 5; 					//设置每10倍频几个点
volatile float		QG_TEMP_HIGH_ALARM=60;		//设置温度过高报警阈值(℃)
volatile float    QG_VOLT_CELL_HIGH=3.5;					//设置电压上限阈值(V)
volatile float    QG_VOLT_CELL_LOW=3.0;						//设置电压下限阈值(V)
volatile float    QG_CURR_CHG_ALARM=10;						//设置外部充电电流过流报警阈值（超过应停止测量）
volatile float		QG_CURR_DIS_ALARM=10;	//设置外部放电电流过流报警阈值（超过应停止测量）
volatile uint32_t QG_CELL_COUNT=52 ;		//设置电池包内电芯数量
volatile float    SET_CALIB_DATA[10];			//设置测量通道（电压、电流、温度）的校准系数数组

volatile int Cell_ID = 1; 


volatile int g_system_error_code=STATUS_NORMAL;
// 全局状态变量，记录系统当前状态
volatile SystemState_t g_current_state = SYS_READY; // 初始状态设为READY

volatile double CommandParam1 = 0.0;    // 参数缓冲区
volatile double CommandParam2 = 0.0;







