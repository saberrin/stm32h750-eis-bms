#include "ADS131A04.h"
#include "spi.h"      // 必须包含，使用 hspi1
#include "delay.h"
#include "usart.h"
#include <stdio.h>
#include <math.h>
#include "watchdog.h"
#define ADS_VREF    4.0f

//=====================================================================
// 控制引脚初始化
//=====================================================================
static void GPIO_InitControl(void)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    // PC4 = CS
    gpio.Pin = GPIO_PIN_4;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOC, &gpio);

    // PB0 = RESET
    gpio.Pin = GPIO_PIN_0;
    HAL_GPIO_Init(GPIOB, &gpio);

    // PC5 = DRDY
    gpio.Pin = GPIO_PIN_5;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &gpio);

    CS_1();
    RESET_1();
}

//=====================================================================
// SPI 单字节收发
//=====================================================================
uint8_t ADS13_SPI(uint8_t data)
{
    uint8_t rx;
    HAL_SPI_TransmitReceive(&hspi1, &data, &rx, 1, 10);
    return rx;
}

//=====================================================================
// 发送命令
//=====================================================================
uint16_t ADS13_WriteCmd(uint16_t cmd)
{
    uint8_t tx[4] = {cmd>>8, cmd&0xFF, 0,0};
    uint8_t rx[4] = {0};
    CS_0();
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 4, 10);
    CS_1();
    return (rx[0]<<8) | rx[1];
}

//=====================================================================
// 写寄存器
//=====================================================================
void ADS13_WriteReg(uint8_t reg, uint8_t val)
{
    uint8_t tx[4] = {0x40|reg, val, 0,0};
    uint8_t rx[4] = {0};
    CS_0();
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 4, 10);
    CS_1();
}

//=====================================================================
// 读寄存器
//=====================================================================
uint8_t ADS13_ReadReg(uint8_t reg)
{
    uint8_t tx[4] = {0x20|reg, 0,0,0};
    uint8_t rx[4] = {0};
    CS_0();
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 4, 10);
    CS_1();
    return rx[1];
}

//=====================================================================
// 24位补码转电压
//=====================================================================
static float CodeToVoltage(uint32_t raw)
{
    int32_t val = raw >> 8;
    if(val & 0x800000) val |= 0xFF000000;
    return (float)val / 8388608.0f * ADS_VREF;
}

//=====================================================================
// 带DRDY读取4通道电压
//=====================================================================
void ADS131A0X_ReadData(float voltage[4])
{
    uint32_t timeout = 100000;
    while(READ_DRDY() && timeout--);
    if(timeout == 0)
    {
        voltage[0]=voltage[1]=voltage[2]=voltage[3]=0;
        return;
    }

    uint8_t tx[4] = {0}, rx[4];
    uint32_t raw[4];

    CS_0();
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 4, 10);

    for(int i=0; i<4; i++)
    {
        HAL_SPI_TransmitReceive(&hspi1, tx, rx, 4, 10);
        raw[i] = (rx[0]<<24)|(rx[1]<<16)|(rx[2]<<8)|rx[3];
        voltage[i] = CodeToVoltage(raw[i]);
    }
    CS_1();
}

//=====================================================================
// 读取并打印电压（最常用）
//=====================================================================
void ADS131A0X_ReadData_Print(void)
{
    float v[4];
    ADS131A0X_ReadData(v);
//    printf("CH1: %7.3f mV\r\n", v[0]*1000);
//    printf("CH2: %7.3f mV\r\n", v[1]*1000);
//    printf("CH3: %7.3f mV\r\n", v[2]*1000);
    printf(" %7.3f \r\n", v[3]*1000);
	
	
	
  //  printf("------------------------\r\n");
}

//=====================================================================
// ADS131 完整初始化
//=====================================================================
//void ADS131A0X_Init(void)
//{
//    GPIO_InitControl();
//    MX_SPI1_Init();   // 使用 spi.c 提供的正确初始化

//    RESET_0();
//    delay_ms(10);
//    RESET_1();
//    delay_ms(100);

//    ADS13_WriteCmd(0x0000);
//    delay_ms(10);
//    ADS13_WriteCmd(0x0655);
//    delay_ms(20);

//		// ========================== ADC 核心配置 ==========================
//		// 寄存器地址         值        功能说明（超详细）
//		ADS13_WriteReg(0x0B, 0x78);  // A_CFG：模拟配置寄存器
//																	// Bit7-6：01 → 内部参考电压 = 4.096V（你当前量程）
//																	// Bit5-4：11 → 输入通道工作模式（正常单端输入）
//																	// Bit3-0：1000 → 保留默认配置

//		ADS13_WriteReg(0x0C, 0x3E);    // 0x0D CLK1：CLKSRC=0外部晶振，CLK_DIV=001(二分频，最小分频提速) → 寄存器值0x02
//		// D_CFG：数字配置寄存器
//																

//		ADS13_WriteReg(0x0D, 0x02);  // CLOCK：时钟 & 过采样率 OSR 配置
//                                   // 0x0E CLK2：ICLK_DIV=001(÷2), OSR=128(0b1010，折中采样率)
//                                  // fMOD = 8M / 2 / 2 = 2MHz；fDATA = 2000000 / 128 = 15625 SPS

//	//	ADS13_WriteReg(0x0E, 0x2F); 
//      ADS13_WriteReg(0x0E, 0x02); 



//// GAIN：增益配置寄存器
//																	// Bit7-5：001 → CH0 增益 = 1
//																	// Bit4-2：010 → CH1 增益 = 1
//																	// Bit1-0：10  → CH2/CH3 增益 = 1
//																	// 所有通道增益 = 1倍（无放大，直接测电压）

//		ADS13_WriteReg(0x0F, 0x0F);  // CH_CFG：通道使能配置
//																	// Bit3：1 → CH3 使能
//																	// Bit2：1 → CH2 使能
//																	// Bit1：1 → CH1 使能
//																	// Bit0：1 → CH0 使能
//																	// 0x0F = 0b1111 → 4 个通道全部开启采样
//    ADS13_WriteCmd(0x0033);
//    delay_ms(100);
//}


void ADS131A0X_Init(void)
{
    GPIO_InitControl();
    MX_SPI1_Init();   // 使用 spi.c 提供的正确SPI初始化

    // 硬件复位时序
    RESET_0();
    delay_ms(10);
    RESET_1();
    delay_ms(100);

    ADS13_WriteCmd(0x0000);    // 空指令
    delay_ms(10);
    ADS13_WriteCmd(0x0655);    // UNLOCK解锁寄存器访问指令，配置寄存器写入前必须解锁
    delay_ms(20);

    // ========================== ADC 核心寄存器配置（对照ADS131A04官方手册SBAS590） ==========================
    // 寄存器0x0B：A_SYS_CFG 模拟系统配置寄存器
    // Bit7 VNCPEN=1：开启内部负电荷泵，支持输入电压低于系统GND
    // Bit6 VREF4V=1：选择片内4.096V精密参考电压（手册标准标称值，不可简写4V）
    // Bit5:4 IN_MODE=11：ADC通道配置为单端输入模式
    // Bit3~0 Reserved=1000：手册规定保留位，固定写入0x8
    ADS13_WriteReg(0x0B, 0x78);

    // 寄存器0x0C：D_SYS_CFG 数字系统配置寄存器
    // Bit7:6 SPI_MODE=00：标准SPI通信模式，无数据环回
    // Bit5:4 DRDY_SEL=11：DRDY数据就绪引脚低电平有效
    // Bit3:2 WDLEN=11：SPI单次输出24bit完整ADC转换结果
    // Bit1:0 CRC_EN=10：关闭CRC通信校验，降低SPI传输耗时、提升读取速度
    ADS13_WriteReg(0x0C, 0x3E);

    // 寄存器0x0D：CLK1 主输入时钟分频寄存器
    // Bit7 CLKSRC=0：外部XTAL晶振作为系统时钟源（硬件晶振16MHz）
    // Bit6~4 Reserved=000：手册保留位，强制写0
    // Bit3~1 CLK_DIV=001：主时钟二分频（硬件最小分频，最大化调制器速率）
    // Bit0 Reserved=0：手册保留位，强制写0
    // 第一级分频计算：f_MOD = f_CLKIN / CLK_DIV = 16000000 / 2 = 8000000 Hz (8MHz)
    ADS13_WriteReg(0x0D, 0x02);

    // 寄存器0x0E：CLK2 调制器内部分频 + OSR过采样配置寄存器
    // Bit7~5 ICLK_DIV=001：调制器内部时钟二分频
    // Bit4 Reserved=0：手册保留位，强制写0
    // Bit3~0 OSR=0010：过采样倍率OS=256（本次函数默认配置）
    // 第二级分频：f_ICLK = f_MOD / ICLK_DIV = 8000000 / 2 = 4000000 Hz (4MHz)
    // 最终ADC输出采样率 f_DATA = f_ICLK / OSR = 4000000 / 256 = 15625 SPS
    // 档位切换参考（仅修改本寄存器低4bit，高5位固定0x2）：
    // 极速档位0x2F OSR=32 → 125000 SPS（硬件采样上限）
    // 中速档位0x2A OSR=128 → 31250 SPS
    // 默认档位0x02 OSR=256 → 15625 SPS（当前配置）
    ADS13_WriteReg(0x0E, 0x02);

    // 寄存器0x0F：CH_ENA 通道使能寄存器
    // Bit3 CH3_EN=1：4通道AIN3输入开启
    // Bit2 CH2_EN=1：3通道AIN2输入开启
    // Bit1 CH1_EN=1：2通道AIN1输入开启
    // Bit0 CH0_EN=1：1通道AIN0输入开启
    // 0x0F=0b1111：四路ADC全部同步开启转换
    ADS13_WriteReg(0x0F, 0x0F);

    ADS13_WriteCmd(0x0033);    // WAKEUP唤醒指令，ADC正式开始连续模数转换
    delay_ms(100);
}



/**
 * @brief ADS131A04 宽量程采样率初始化函数（16MHz晶振固定）
 * 硬件固定：外部16MHz晶振，CLK_DIV=2，ICLK_DIV=2，fMOD=4000000Hz
 * 支持全部合法采样档位（从小到大）：
    61Hz   OSR=32768
    244Hz  OSR=16384
    488Hz  OSR=8192
    976Hz  OSR=4096
    3906Hz OSR=1024
    15625HzOSR=256
    31250HzOSR=128
    125000Hz(硬件上限) OSR=32
 * @param target_sps 期望采样率，uint16整数，输入任意值自动向下匹配合法档位
 * 例：输入10000 → 匹配15625；输入100 → 匹配244；输入130000自动锁最高125000
 */

	void ADS131A0X_Init_SetSampleRate(uint32_t target_sps)
{
    // 硬件固定参数（全部局部，无全局宏）
//    const uint32_t F_CLKIN_HZ = 16000000U;
//    const uint8_t CLK_DIV_CFG = 2U;
//    const uint8_t ICLK_DIV_CFG = 2U;
//    const uint32_t F_MOD = F_CLKIN_HZ / CLK_DIV_CFG / ICLK_DIV_CFG;

    // 完整OSR档位表，扩充最低32768(61Hz)、最高32(125kHz)
    typedef struct
    {
        uint8_t osr_code;
        uint16_t osr_val;
        uint32_t sps;
    } OSR_Config_t;
    const OSR_Config_t osr_table[] = {
        {0x0F,    32,     125000U},  // 最高速
        {0x0A,   128,      31250U},
        {0x02,   256,      15625U},
        {0x03,  1024,       3906U},
        {0x05,  4096,        976U},
        {0x08,  8192,        488U},
        {0x09, 16384,        244U},
        {0x0C, 32768,         61U},  // 新增最低速档位
    };
    const uint8_t table_cnt = sizeof(osr_table) / sizeof(OSR_Config_t);

    // 底层硬件初始化（和你原有逻辑完全不变）
    GPIO_InitControl();
    MX_SPI1_Init();

    // 标准复位时序
    RESET_0();
    delay_ms(10);
    RESET_1();
    delay_ms(100);

    // 解锁寄存器指令
    ADS13_WriteCmd(0x0000);
    delay_ms(10);
    ADS13_WriteCmd(0x0655);
    delay_ms(20);

    // 固定模拟/数字寄存器，完全沿用你之前配置
    ADS13_WriteReg(0x0B, 0x78);
    ADS13_WriteReg(0x0C, 0x3E);
    ADS13_WriteReg(0x0D, 0x02);
    ADS13_WriteReg(0x0F, 0x0F);

    // 匹配最优OSR：从高速往低速查找
    uint8_t clk2_base = 0x20; // ICLK_DIV=001 固定前缀
    uint8_t match_idx = table_cnt - 1; // 默认最低速兜底
    for(uint8_t i = 0; i < table_cnt; i++)
    {
        if(target_sps >= osr_table[i].sps)
        {
            match_idx = i;
            break;
        }
    }
    uint8_t clk2_reg = clk2_base | osr_table[match_idx].osr_code;
    ADS13_WriteReg(0x0E, clk2_reg);

    // 唤醒ADC开始转换
    ADS13_WriteCmd(0x0033);
    delay_ms(100);
}

/**
 * @brief 仅动态修改OSR切换采样率，无硬件复位，无电压跳变
 * @param target_sps 目标采样档位
 * 前提：上电已执行一次完整ADS131A0X_Init()完成基础初始化
 */
void ADS131A0X_ChangeSampleRate_NoReset(uint32_t target_sps)
{
    // 局部OSR表，和之前一致
    typedef struct
    {
        uint8_t osr_code;
        uint16_t osr_val;
        uint32_t sps;
    } OSR_Config_t;
    const OSR_Config_t osr_table[] = {
        {0x0F,    32,     125000U},
        {0x0A,   128,      31250U},
        {0x02,   256,      15625U},
        {0x03,  1024,       3906U},
        {0x05,  4096,        976U},
        {0x08,  8192,        488U},
        {0x09, 16384,        244U},
        {0x0C, 32768,         61U},
    };
    const uint8_t table_cnt = sizeof(osr_table) / sizeof(OSR_Config_t);

    uint8_t clk2_base = 0x20; // ICLK_DIV固定001不变
    uint8_t match_idx = table_cnt - 1;
    for(uint8_t i = 0; i < table_cnt; i++)
    {
        if(target_sps >= osr_table[i].sps)
        {
            match_idx = i;
            break;
        }
    }
    uint8_t clk2_reg = clk2_base | osr_table[match_idx].osr_code;
    // 只改写CLK2寄存器，其余所有模拟/数字配置保持不变
    ADS13_WriteReg(0x0E, clk2_reg);
    // 无需复位、无需解锁、无需WAKEUP，ADC持续转换
}



//=====================================================================
// 调试：打印所有寄存器
//=====================================================================
void ADS131A0X_TestAllRegisters(void)
{
    printf("\r\n=== ADS131 REG Dump ===\r\n");
    for(int a=0; a<=0x1F; a++)
    {
        uint8_t v = ADS13_ReadReg(a);
        printf("REG %02X = %02X\r\n", a, v);
        delay_ms(2);
    }
}

//=====================================================================
// 调试：打印详细信息
//=====================================================================
void ADS131A0X_PrintDetailedInfo(void)
{
    uint8_t idM  = ADS13_ReadReg(0x00);
    uint8_t idL  = ADS13_ReadReg(0x01);
    uint8_t sta  = ADS13_ReadReg(0x02);
    uint8_t err  = ADS13_ReadReg(0x03);
    uint8_t cfgA = ADS13_ReadReg(0x0B);
    uint8_t cfgD = ADS13_ReadReg(0x0C);

    printf("ID: %02X%02X\r\n", idM, idL);
    printf("STATUS: %02X\r\n", sta);
    printf("ERROR: %02X\r\n", err);
    printf("A_CFG: %02X\r\n", cfgA);
    printf("D_CFG: %02X\r\n", cfgD);
}







void ADS131A0X_Read_Ch1_Ch2(uint32_t *Current, uint32_t *Voltage)
{

    uint8_t tx[4] = {0}, rx[4];
//    uint32_t raw;
    while(READ_DRDY());
    CS_0();
    // 读取状态字
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 4, 10);

    // CH0 通道1 → 电流 Current
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 4, 10);

    *Current = (rx[0] << 24) | (rx[1] << 16) | (rx[2] << 8) | rx[3];
		
    // 跳过CH1
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 4, 10);
    
			*Voltage = (rx[0] << 24) | (rx[1] << 16) | (rx[2] << 8) | rx[3];

		
		// 跳过CH2
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 4, 10);
    // CH3 通道4 → 电压 Voltage
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 4, 10);

	
    CS_1();
}










/**
  * @brief  只读 ADS131A04 单个通道数据（输出 uint16_t）
  * @param  channel: 0~3 对应 CH0~CH3
  * @param  val16: 输出 16bit 数据（右移8位后的有效值）
  * @retval 0=成功, 1=DRDY超时
  */
//uint8_t ADS131A0X_Read_Single_Channel(uint8_t channel, uint16_t *val16)
//{
//    uint8_t  tx[4] = {0};
//    uint8_t  rx[4] = {0};
//    uint32_t raw24;

//    // 输入检查
//    if(channel > 3 || val16 == NULL)
//        return 1;

//    // 等待 DRDY
//    uint32_t timeout = 100000;
//    while(READ_DRDY() && timeout--);
//    if(timeout == 0)
//        return 1;

//    CS_0();
//    // 读状态字（必须读）
//    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 4, 10);

//    // 读取4通道，只保留目标通道
//    for(uint8_t i=0; i<4; i++)
//    {
//        HAL_SPI_TransmitReceive(&hspi1, tx, rx, 4, 10);
//        if(i == channel)
//        {
//            raw24 = (rx[0] << 16) | (rx[1] << 8) | rx[2];
//        }
//    }
//    CS_1();

//    // 核心：转成 uint16_t（和你原来代码逻辑一致）
//    *val16 = (uint16_t)((raw24 >> 8) & 0xFFFF);

//    return 0;
//}

uint8_t ADS131A0X_Read_Single_Channel(uint8_t channel, uint16_t *val16)
{
    uint8_t  tx[4] = {0};
    uint8_t  rx[4] = {0};
    uint32_t tempRaw;
    uint32_t targetRaw; // 存目标通道完整32位原始值

    if(channel > 3 || val16 == NULL)
        return 1;

    uint32_t timeout = 100000;
    while(READ_DRDY() && timeout--);
    if(timeout == 0)
        return 1;

    CS_0();
    // 先读状态字，和正确代码完全一致
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 4, 10);

    for(uint8_t i = 0; i < 4; i++)
    {
        HAL_SPI_TransmitReceive(&hspi1, tx, rx,4, 10);
        // 和ReadData一模一样的拼接方式
        tempRaw = ((uint32_t)rx[0] << 24) | ((uint32_t)rx[1] << 16) | ((uint32_t)rx[2] << 8) | rx[3];
        if(i == channel)
        {
            targetRaw = tempRaw;
        }
    }
    CS_1();
    // 等价于 CodeToVoltage 里 raw >> 8
    *val16 = (uint16_t)(targetRaw >> 8);
    return 0;
}





//float ADS131A0X_Read_Channel(uint8_t channel)
//{
//    // 通道合法性校验
//    if(channel > 3)
//    {
//        return NAN; // 非法通道返回非数，区分正常0V
//    }

//    // 单次DRDY等待，仅保留一段，带超时保护
//    uint32_t timeout = 100000;
//    while(READ_DRDY() && timeout--)
//    {
//        Watchdog_Refresh(); // 循环内持续喂狗，防止复位
//    }
//    if(timeout == 0)
//    {
//        printf("ADC读取超时 DRDY无效\r\n");
//        return NAN; // 超时返回NaN，上层识别故障
//    }

//    uint8_t tx[4] = {0}, rx[4];
//    uint32_t targetRaw = 0;

//    CS_0();
//    // 读取状态字（ADS131A04固定时序必须先读）
//    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 4, 10);
//    Watchdog_Refresh();

//    for(int i=0; i<4; i++)
//    {
//        HAL_SPI_TransmitReceive(&hspi1, tx, rx, 4, 10);
//        // 规范移位，增加括号提升可读性
//        uint32_t rawBuf = ((uint32_t)rx[0] << 24) | ((uint32_t)rx[1] << 16) | ((uint32_t)rx[2] << 8) | rx[3];
//        if(i == channel)
//        {
//            targetRaw = rawBuf;
//        }
//        Watchdog_Refresh(); // 每读一个通道喂一次狗
//    }
//    CS_1();

//    // 复用原有24bit转电压函数，逻辑统一无偏差
//    return CodeToVoltage(targetRaw);
//}



//=====================================================================
// 读取单个通道电压值（0~3）
// 返回值：电压（V），非法通道返回 NAN
//=====================================================================
float ADS131A0X_Read_Channel(uint8_t channel)
{
    if (channel > 3)
        return NAN;

    uint8_t tx[4] = {0};
    uint8_t rx[4] = {0};

    /* 等待 DRDY 拉低 */
    uint32_t timeout = 100000;
    while (READ_DRDY() && timeout--);
    if (timeout == 0)
        return NAN;

    CS_0();

    /* 1. 读取状态字（必须读，不能跳过） */
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 4, 10);

    /* 2. 按顺序读取 4 个通道 */
    uint32_t raw[4] = {0};
    for (int i = 0; i < 4; i++)
    {
        HAL_SPI_TransmitReceive(&hspi1, tx, rx, 4, 10);
        raw[i] = ((uint32_t)rx[0] << 24) |
                 ((uint32_t)rx[1] << 16) |
                 ((uint32_t)rx[2] << 8) |
                  (uint32_t)rx[3];
    }

    CS_1();

    /* 3. 只转换目标通道 */
    return CodeToVoltage(raw[channel]);
}

