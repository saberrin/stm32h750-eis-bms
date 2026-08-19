#ifndef RUNTIME_FLASH_H
#define RUNTIME_FLASH_H

#include <stdint.h>
#include <stdbool.h>
#include "config_flash.h"   // RUNTIME_FLASH_ADDR

#ifdef __cplusplus
extern "C" {
#endif

#define RUNTIME_MAGIC          (0x51475254u)  // 'QGRT'
#define RUNTIME_SCHEMA_VERSION (0x00010001u)

#define LASTERR_MAX_LEN        16  // "SWEET" / "ADC_FAULT" 等

#if defined(__CC_ARM) || defined(__ARMCC_VERSION)   // Keil ARMCC
  #define QG_PACKED_STRUCT   __packed struct
#else                                               // GCC/Clang
  #define QG_PACKED_STRUCT   struct __attribute__((packed))
#endif


typedef QG_PACKED_STRUCT
{
    uint32_t magic;             // RUNTIME_MAGIC
    uint32_t crc32;             // 对 crc32 之后的内容做 CRC32
    uint32_t schema_version;    // RUNTIME_SCHEMA_VERSION

    // 标志位（0/1）
    uint8_t  rt_inited_flag;    // 0/1
    uint8_t  rt_dirty_flag;     // 0/1（RAM 改动未落盘）
    uint16_t rsv0;

    // ===== 表格字段（严格按你提供的定义）=====
    uint32_t boot_count;           // 示例 10234
    uint32_t running_time_hours;   // 示例 1500（小时）
    uint32_t eis_scan_total;       // 示例 1500
    char     last_error[LASTERR_MAX_LEN]; // 示例 "SWEET" 表示无故障

    uint8_t  reserved[32];
} QG_RuntimeFlash_t;

extern QG_RuntimeFlash_t g_runtime_in_flash;

void RuntimeFlash_LoadAtStartup(void);
bool RuntimeFlash_Read(QG_RuntimeFlash_t *out);
bool RuntimeFlash_Write(const QG_RuntimeFlash_t *cfg);
void RuntimeFlash_Dump(const QG_RuntimeFlash_t *cfg);

// 运行时更新接口（只改 RAM + dirty，是否写回由 SaveIfDirty 控制）
void RuntimeFlash_OnBoot(void);
void RuntimeFlash_AddRunHours(uint32_t hours);
void RuntimeFlash_OnEisFinished(void);
void RuntimeFlash_SetLastError(const char *err_str);

// 节流/策略性调用：dirty=1 才会擦写扇区
bool RuntimeFlash_SaveIfDirty(void);

// 恢复出厂默认（并写入 Flash）
void RuntimeFlash_ResetToDefaultAndSave(void);

// 系统内软件时钟
void Runtime_On1sTick(void);
void Runtime_BackgroundTask(void);


#ifdef __cplusplus
}
#endif

#endif // RUNTIME_FLASH_H
