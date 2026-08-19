#ifndef IDENTITY_FLASH_H
#define IDENTITY_FLASH_H

#include <stdint.h>
#include <stdbool.h>
#include "config_flash.h"   // ID_FLASH_ADDR

#ifdef __cplusplus
extern "C" {
#endif

// 固定 magic（用于判定 Flash 区域是否有效）
#define ID_MAGIC                (0x51474944u)   // 'QGID'
#define ID_SCHEMA_VERSION       (0x00010001u)   // 结构版本，后续结构调整可递增

// 字符串长度（包含 '\0'）
#define ID_SN_MAX_LEN           32   // SN
#define ID_PACK_MAX_LEN         16   // "C01-P08"
#define ID_HWVER_MAX_LEN        16   // "HW2.1"
#define ID_FWVER_MAX_LEN        32   // "FW2.1.3b123-REL"

// 兼容 GCC / ARMCC 的 packed
#if defined(__CC_ARM) || defined(__ARMCC_VERSION)   // Keil ARMCC
  #define QG_PACKED_STRUCT   __packed struct
#else                                               // GCC/Clang
  #define QG_PACKED_STRUCT   struct __attribute__((packed))
#endif


typedef QG_PACKED_STRUCT
{
    uint32_t magic;                 // ID_MAGIC
    uint32_t crc32;                 // 对 crc32 之后的内容做 CRC32
    uint32_t schema_version;        // ID_SCHEMA_VERSION

    // 标志位（你要求仅 0/1）：不写入表格字段中，但保留
    uint8_t  id_inited_flag;        // 0/1
    uint8_t  id_lock_flag;          // 0/1：锁定后不允许修改关键字段
    uint16_t rsv0;

    // ===== 表格字段=====
    char     sn[ID_SN_MAX_LEN];         // "XDRA-20251107-MT-000001"
    uint8_t  can_node_addr;             // 0x01~0x7F，示例 0x05
    uint8_t  rsv1[3];

    char     pack_binding[ID_PACK_MAX_LEN];  // "C01-P08"
    char     hw_version[ID_HWVER_MAX_LEN];   // "HW2.1"
    char     fw_version[ID_FWVER_MAX_LEN];   // "FW2.1.3b123-REL"

    uint8_t  reserved[32];              // 预留
} QG_IdFlash_t;

// RAM 镜像
extern QG_IdFlash_t g_id_in_flash;

// 启动加载（无效则擦写为默认值）
void IdFlash_LoadAtStartup(void);

// 读写接口（带 CRC 校验）
bool IdFlash_Read(QG_IdFlash_t *out);
bool IdFlash_Write(const QG_IdFlash_t *cfg);

// 表格风格打印
void IdFlash_Dump(const QG_IdFlash_t *cfg);

// 修改接口（保留 handle，锁定后可限制修改）
bool IdFlash_SetLock(uint8_t lock_01);
bool IdFlash_SetCanAddr(uint8_t addr);
bool IdFlash_SetPackBinding(const char *pack_str);
bool IdFlash_SetHWVersion(const char *hw_str);
bool IdFlash_SetFWVersion(const char *fw_str);
bool IdFlash_SetSN(const char *sn_str);

// 恢复出厂默认（并写入 Flash）
void IdFlash_ResetToDefaultAndSave(void);

#ifdef __cplusplus
}
#endif

#endif // IDENTITY_FLASH_H
