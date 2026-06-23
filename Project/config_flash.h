#ifndef CONFIG_FLASH_H
#define CONFIG_FLASH_H

#include <stdint.h>
#include <stdbool.h>
#include "global_command.h"  // 为了拿到 BoolFlag、以及全局变量的真实类型
#include "norflash.h" 
#include <string.h> 

#ifndef CONFIG_FLASH_ADDR
//#define CONFIG_FLASH_ADDR  0x00040000u   // 4KB 对齐
#define CONFIG_FLASH_ADDR  0x00060000u

#endif

#define CONFIG_MAGIC       0x51C0A55Au

typedef struct __attribute__((packed)) {
    uint32_t magic;              // 固定为 CONFIG_MAGIC
    uint32_t crc32;              // 计算时将本字段置 0
    uint32_t sweep_points;       // 扫频点数
    uint8_t  sweep_en;           // 1=扫频, 0=单频
    float    sweep_start_freq;   // Hz
    float    sweep_stop_freq;    // Hz
    float    sin_freq;           // Hz（单频）
    float    ac_volt_pp;         // AC Vpp
    float    dc_volt;            // DC 偏置
    float    rcal_val;           // 标定电阻
    uint8_t  sweep_log;          // 1=对数扫频, 0=线性
    uint8_t  reserved[3];
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

#ifdef __cplusplus
}
#endif

// ====== 这里仅做“extern 声明”，类型必须与 global_commnd.h 完全一致 ======


#endif // CONFIG_FLASH_H
