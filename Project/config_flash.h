#ifndef CONFIG_FLASH_H
#define CONFIG_FLASH_H

#include <stdint.h>
#include <stdbool.h>
#include "global_command.h"  // 为了拿到 BoolFlag、以及全局变量的真实类型
#include "norflash.h" 
#include <string.h> 

#ifndef CONFIG_FLASH_ADDR
#define CONFIG_FLASH_ADDR  0x00060000u   // EIS_Test Parameters zone
#endif

#ifndef CALIB_FLASH_ADDR
#define CALIB_FLASH_ADDR   0x00061000u   // Calibration zone
#endif

#ifndef ID_FLASH_ADDR
#define ID_FLASH_ADDR      0x00062000u   // Identity zone
#endif

#ifndef RUNTIME_FLASH_ADDR
#define RUNTIME_FLASH_ADDR 0x00063000u   // RunTime zone
#endif

#define CONFIG_MAGIC       0x51C0A55Au

typedef struct __attribute__((packed)) {
    uint32_t magic;              // 固定为 CONFIG_MAGIC
    uint32_t crc32;              // 计算 CRC32 时将本字段置 0

    /* EIS 配置 */
    float    ac_volt_pp;         // AC Vpp (QG_ACVoltPP)
    float    dc_volt;            // DC 偏置电压 (QG_DCVolt)
    float    sweep_start_freq;   // 扫频起始频率 (QG_EIS_FREQ_START)
    float    sweep_stop_freq;    // 扫频终止频率 (QG_EIS_FREQ_END)
    uint32_t sweep_points;       // 每十倍频扫频点数 (QG_EIS_FREQ_POINTS)
    uint8_t  sweep_en;           // 1=扫频, 0=单频
    uint8_t  sweep_log;          // 1=对数扫频, 0=线性

    /* 阈值配置 */
    float    temp_high_alarm;    // 高温报警阈值 (QG_TEMP_HIGH_ALARM)
    float    volt_cell_high;     // 单体电压高报警阈值 (QG_VOLT_CELL_HIGH)
    float    volt_cell_low;      // 单体电压低报警阈值 (QG_VOLT_CELL_LOW)
    float    curr_chg_alarm;     // 充电电流报警阈值 (QG_CURR_CHG_ALARM)
    float    curr_dis_alarm;     // 放电电流报警阈值 (QG_CURR_DIS_ALARM)

    /* 电芯数量 */
    uint32_t cell_count;         // 电芯数量 (QG_CELL_COUNT)

    /* 校准数据 */
    float    calib_data[10];     // 校准系数数组 (SET_CALIB_DATA)

} QG_ConfigFlash_t;


// 最近一次“成功装载/写入后验证通过”的镜像
extern QG_ConfigFlash_t current_cfg_in_flash;

#ifdef __cplusplus
extern "C" {
#endif

void ConfigFlash_LoadAtStartup(void);
bool ConfigFlash_Read(QG_ConfigFlash_t* out);
bool ConfigFlash_Write(const QG_ConfigFlash_t* cfg);
bool ConfigFlash_SaveFromGlobals(void);

// 新增：只有在“全局参数与当前镜像不一致”时才写回（供 parse/main 调用）
bool ConfigFlash_SaveIfChanged(void);

void ConfigFlash_Dump(const QG_ConfigFlash_t* cfg);

void Flash_EraseAllParamZones(void);


#ifdef __cplusplus
}
#endif

// ====== 这里仅做“extern 声明”，类型必须与 global_commnd.h 完全一致 ======


#endif // CONFIG_FLASH_H
