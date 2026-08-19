#include "identity_flash.h"

#include <string.h>
#include <stdio.h>
#include "fdcan.h"

#include "parse_command.h"
#define DUMP_DATA_BUF_SIZE   512
// 由底层 norflash 驱动提供
extern void norflash_ex_read (uint8_t*       dst, uint32_t addr, uint32_t len);
extern void norflash_ex_write(const uint8_t* src, uint32_t addr, uint32_t len);
extern void norflash_ex_erase_sector(uint32_t sector_index);

#define SECTOR_SIZE_BYTES   (4096u)

QG_IdFlash_t g_id_in_flash;

// 与 config_flash.c 同 poly：0xEDB88320
static uint32_t qg_crc32(const uint8_t* data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < len; ++i) {
        uint32_t x = (crc ^ data[i]) & 0xFFu;
        for (int b = 0; b < 8; ++b) {
            uint32_t mask = -(int32_t)(x & 1u);
            x = (x >> 1) ^ (0xEDB88320u & mask);
        }
        crc = (crc >> 8) ^ x;
    }
    return ~crc;
}

static void id_default_init(QG_IdFlash_t *cfg)
{
    memset(cfg, 0, sizeof(*cfg));

    cfg->magic          = ID_MAGIC;
    cfg->crc32          = 0;
    cfg->schema_version = ID_SCHEMA_VERSION;

    cfg->id_inited_flag = 1;
    cfg->id_lock_flag   = 0;

    // ===== 严格按你表格“示例值”初始化 =====
    strncpy(cfg->sn, "XDRA-20251107-MT-000001", ID_SN_MAX_LEN - 1);
    cfg->can_node_addr = 0x05;
    strncpy(cfg->pack_binding, "C01-P08", ID_PACK_MAX_LEN - 1);
    strncpy(cfg->hw_version, "HW2.1", ID_HWVER_MAX_LEN - 1);
    strncpy(cfg->fw_version, "FW2.1.3b123-REL", ID_FWVER_MAX_LEN - 1);
}

static void id_fixup_and_crc(QG_IdFlash_t *cfg)
{
    cfg->magic          = ID_MAGIC;
    cfg->schema_version = ID_SCHEMA_VERSION;

    cfg->crc32 = 0;
    cfg->crc32 = qg_crc32((const uint8_t*)cfg + 8, (uint32_t)sizeof(*cfg) - 8);
}

static bool id_is_valid(const QG_IdFlash_t *cfg)
{
    if (!cfg) return false;
    if (cfg->magic != ID_MAGIC) return false;
    if (cfg->schema_version != ID_SCHEMA_VERSION) return false;

    uint32_t calc = qg_crc32((const uint8_t*)cfg + 8, (uint32_t)sizeof(*cfg) - 8);
    return (calc == cfg->crc32);
}

bool IdFlash_Read(QG_IdFlash_t *out)
{
    if (!out) return false;
    norflash_ex_read((uint8_t*)out, ID_FLASH_ADDR, (uint32_t)sizeof(*out));
    return id_is_valid(out);
}

bool IdFlash_Write(const QG_IdFlash_t *cfg)
{
    if (!cfg) return false;

    QG_IdFlash_t wr = *cfg;
    id_fixup_and_crc(&wr);

    uint32_t sector = (uint32_t)(ID_FLASH_ADDR / SECTOR_SIZE_BYTES);
    norflash_ex_erase_sector(sector);
    norflash_ex_write((const uint8_t*)&wr, ID_FLASH_ADDR, (uint32_t)sizeof(wr));

    // readback verify
    QG_IdFlash_t rd;
    memset(&rd, 0, sizeof(rd));
    norflash_ex_read((uint8_t*)&rd, ID_FLASH_ADDR, (uint32_t)sizeof(rd));
    if (memcmp(&rd, &wr, sizeof(wr)) != 0) {
        printf("[IdFlash] write verify failed\r\n");
        return false;
    }

    g_id_in_flash = wr;
    return true;
}

void IdFlash_ResetToDefaultAndSave(void)
{
    QG_IdFlash_t def;
    id_default_init(&def);
    if (!IdFlash_Write(&def)) {
        printf("[IdFlash] reset default write failed\r\n");
    }
}

void IdFlash_LoadAtStartup(void)
{
    QG_IdFlash_t rd;
    memset(&rd, 0, sizeof(rd));
    norflash_ex_read((uint8_t*)&rd, ID_FLASH_ADDR, (uint32_t)sizeof(rd));

    if (!id_is_valid(&rd)) {
        printf("[IdFlash] invalid/empty -> init defaults\r\n");
        IdFlash_ResetToDefaultAndSave();
        IdFlash_Dump(&g_id_in_flash);
        return;
    }

    g_id_in_flash = rd;
    printf("[IdFlash] loaded OK (lock=%u)\r\n", g_id_in_flash.id_lock_flag);
    IdFlash_Dump(&g_id_in_flash);
}

void IdFlash_Dump(const QG_IdFlash_t *cfg)
{
    if (!cfg) cfg = &g_id_in_flash;
		
		Send_Line(">0x%02X, GETID, CMD_OK\r\n", QG_ID);

    Send_Line("magic            : 0x%08lX\r\n", (unsigned long)cfg->magic);
    Send_Line("crc32            : 0x%08lX\r\n", (unsigned long)cfg->crc32);
    Send_Line("schema_version   : 0x%08lX\r\n", (unsigned long)cfg->schema_version);
    Send_Line("inited           : %u\r\n", cfg->id_inited_flag);
    Send_Line("lock             : %u\r\n", cfg->id_lock_flag);

    // ===== 表格字段（书写方式对齐你的习惯）=====
    Send_Line("sn               : %s\r\n", cfg->sn);
    Send_Line("can_node_addr    : 0x%02X\r\n", (unsigned)cfg->can_node_addr);
    Send_Line("pack_binding     : %s\r\n", cfg->pack_binding);
    Send_Line("hw_ver           : %s\r\n", cfg->hw_version);
    Send_Line("fw_ver           : %s\r\n", cfg->fw_version);
		Send_Line("-----------------------------------------\r\n"); 
		Send_Line("<\r\n");
}





bool IdFlash_SetLock(uint8_t lock_01)
{
    g_id_in_flash.id_lock_flag = lock_01 ? 1 : 0;
    return IdFlash_Write(&g_id_in_flash);
}

static bool id_check_lock(void)
{
    if (g_id_in_flash.id_lock_flag) {
        printf("[IdFlash] locked, reject update\r\n");
        return false;
    }
    return true;
}

bool IdFlash_SetCanAddr(uint8_t addr)
{
    if (!id_check_lock()) return false;
    g_id_in_flash.can_node_addr = addr;
    return IdFlash_Write(&g_id_in_flash);
}

bool IdFlash_SetPackBinding(const char *pack_str)
{
    if (!id_check_lock()) return false;
    if (!pack_str) return false;
    memset(g_id_in_flash.pack_binding, 0, sizeof(g_id_in_flash.pack_binding));
    strncpy(g_id_in_flash.pack_binding, pack_str, sizeof(g_id_in_flash.pack_binding) - 1);
    return IdFlash_Write(&g_id_in_flash);
}

bool IdFlash_SetHWVersion(const char *hw_str)
{
    if (!id_check_lock()) return false;
    if (!hw_str) return false;
    memset(g_id_in_flash.hw_version, 0, sizeof(g_id_in_flash.hw_version));
    strncpy(g_id_in_flash.hw_version, hw_str, sizeof(g_id_in_flash.hw_version) - 1);
    return IdFlash_Write(&g_id_in_flash);
}

bool IdFlash_SetFWVersion(const char *fw_str)
{
    if (!id_check_lock()) return false;
    if (!fw_str) return false;
    memset(g_id_in_flash.fw_version, 0, sizeof(g_id_in_flash.fw_version));
    strncpy(g_id_in_flash.fw_version, fw_str, sizeof(g_id_in_flash.fw_version) - 1);
    return IdFlash_Write(&g_id_in_flash);
}

bool IdFlash_SetSN(const char *sn_str)
{
    if (!id_check_lock()) return false;
    if (!sn_str) return false;
    memset(g_id_in_flash.sn, 0, sizeof(g_id_in_flash.sn));
    strncpy(g_id_in_flash.sn, sn_str, sizeof(g_id_in_flash.sn) - 1);
    return IdFlash_Write(&g_id_in_flash);
}
