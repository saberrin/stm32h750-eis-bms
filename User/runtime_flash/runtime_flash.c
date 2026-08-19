#include "runtime_flash.h"
#include <string.h>
#include <stdio.h>
#include "fdcan.h"
#include "parse_command.h"
#define DUMP_DATA_BUF_SIZE   512

extern void norflash_ex_read (uint8_t*       dst, uint32_t addr, uint32_t len);
extern void norflash_ex_write(const uint8_t* src, uint32_t addr, uint32_t len);
extern void norflash_ex_erase_sector(uint32_t sector_index);

#define SECTOR_SIZE_BYTES   (4096u)

QG_RuntimeFlash_t g_runtime_in_flash;

static volatile uint32_t s_uptime_seconds = 0;           // 上电后累计秒数
static volatile uint8_t  s_runtime_need_save = 0;        // 提醒主循环“可以写一次 Flash”


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

static void rt_default_init(QG_RuntimeFlash_t *cfg)
{
    memset(cfg, 0, sizeof(*cfg));

    cfg->magic          = RUNTIME_MAGIC;
    cfg->crc32          = 0;
    cfg->schema_version = RUNTIME_SCHEMA_VERSION;

    cfg->rt_inited_flag = 1;
    cfg->rt_dirty_flag  = 0;

    // ===== 严格按你表格“示例值”初始化 =====
    cfg->boot_count         = 10234;
    cfg->running_time_hours = 1500;
    cfg->eis_scan_total     = 1500;
    strncpy(cfg->last_error, "SWEET", LASTERR_MAX_LEN - 1);  // 无故障
}

static void rt_fixup_and_crc(QG_RuntimeFlash_t *cfg)
{
    cfg->magic          = RUNTIME_MAGIC;
    cfg->schema_version = RUNTIME_SCHEMA_VERSION;

    cfg->crc32 = 0;
    cfg->crc32 = qg_crc32((const uint8_t*)cfg + 8, (uint32_t)sizeof(*cfg) - 8);
}

static bool rt_is_valid(const QG_RuntimeFlash_t *cfg)
{
    if (!cfg) return false;
    if (cfg->magic != RUNTIME_MAGIC) return false;
    if (cfg->schema_version != RUNTIME_SCHEMA_VERSION) return false;

    uint32_t calc = qg_crc32((const uint8_t*)cfg + 8, (uint32_t)sizeof(*cfg) - 8);
    return (calc == cfg->crc32);
}

bool RuntimeFlash_Read(QG_RuntimeFlash_t *out)
{
    if (!out) return false;
    norflash_ex_read((uint8_t*)out, RUNTIME_FLASH_ADDR, (uint32_t)sizeof(*out));
    return rt_is_valid(out);
}

bool RuntimeFlash_Write(const QG_RuntimeFlash_t *cfg)
{
    if (!cfg) return false;

    QG_RuntimeFlash_t wr = *cfg;
    rt_fixup_and_crc(&wr);

    uint32_t sector = (uint32_t)(RUNTIME_FLASH_ADDR / SECTOR_SIZE_BYTES);
    norflash_ex_erase_sector(sector);
    norflash_ex_write((const uint8_t*)&wr, RUNTIME_FLASH_ADDR, (uint32_t)sizeof(wr));

    QG_RuntimeFlash_t rd;
    memset(&rd, 0, sizeof(rd));
    norflash_ex_read((uint8_t*)&rd, RUNTIME_FLASH_ADDR, (uint32_t)sizeof(rd));
    if (memcmp(&rd, &wr, sizeof(wr)) != 0) {
        printf("[RuntimeFlash] write verify failed\r\n");
        return false;
    }

    g_runtime_in_flash = wr;
    return true;
}

void RuntimeFlash_ResetToDefaultAndSave(void)
{
    QG_RuntimeFlash_t def;
    rt_default_init(&def);
    if (!RuntimeFlash_Write(&def)) {
        printf("[RuntimeFlash] reset default write failed\r\n");
    }
}

void RuntimeFlash_LoadAtStartup(void)
{
    QG_RuntimeFlash_t rd;
    memset(&rd, 0, sizeof(rd));
    norflash_ex_read((uint8_t*)&rd, RUNTIME_FLASH_ADDR, (uint32_t)sizeof(rd));

    if (!rt_is_valid(&rd)) {
        printf("[RuntimeFlash] invalid/empty -> init defaults\r\n");
        RuntimeFlash_ResetToDefaultAndSave();
        RuntimeFlash_Dump(&g_runtime_in_flash);
        return;
    }

    g_runtime_in_flash = rd;
    printf("[RuntimeFlash] loaded OK (boot=%lu)\r\n", (unsigned long)g_runtime_in_flash.boot_count);
    RuntimeFlash_Dump(&g_runtime_in_flash);
}




void RuntimeFlash_Dump(const QG_RuntimeFlash_t *cfg)
{
    if (!cfg)
        cfg = &g_runtime_in_flash;

    Send_Line(">0x%02X, GETRT, CMD_OK\r\n", QG_ID);

    Send_Line("magic            : 0x%08lX\r\n", (unsigned long)cfg->magic);
    Send_Line("crc32            : 0x%08lX\r\n", (unsigned long)cfg->crc32);
    Send_Line("schema_version   : 0x%08lX\r\n", (unsigned long)cfg->schema_version);
    Send_Line("inited           : %u\r\n", cfg->rt_inited_flag);
    Send_Line("dirty            : %u\r\n", cfg->rt_dirty_flag);

    Send_Line("boot_count       : %lu\r\n", (unsigned long)cfg->boot_count);
    Send_Line("running_time_h   : %lu\r\n", (unsigned long)cfg->running_time_hours);
    Send_Line("eis_scan_total   : %lu\r\n", (unsigned long)cfg->eis_scan_total);
    Send_Line("last_error       : %s\r\n", cfg->last_error);
		Send_Line("<\r\n");

}


void RuntimeFlash_OnBoot(void)
{
    g_runtime_in_flash.boot_count++;
    g_runtime_in_flash.rt_dirty_flag = 1;
}

void RuntimeFlash_AddRunHours(uint32_t hours)
{
    if (hours == 0) return;
    g_runtime_in_flash.running_time_hours += hours;
    g_runtime_in_flash.rt_dirty_flag = 1;
}

void RuntimeFlash_OnEisFinished(void)
{
    g_runtime_in_flash.eis_scan_total++;
    g_runtime_in_flash.rt_dirty_flag = 1;
}

void RuntimeFlash_SetLastError(const char *err_str)
{
    if (!err_str) return;
    memset(g_runtime_in_flash.last_error, 0, sizeof(g_runtime_in_flash.last_error));
    strncpy(g_runtime_in_flash.last_error, err_str, sizeof(g_runtime_in_flash.last_error) - 1);
    g_runtime_in_flash.rt_dirty_flag = 1;
}

bool RuntimeFlash_SaveIfDirty(void)
{
    if (!g_runtime_in_flash.rt_dirty_flag) return true;

    QG_RuntimeFlash_t tmp = g_runtime_in_flash;
    tmp.rt_dirty_flag = 0; // 写入 Flash 时清零 dirty

    if (RuntimeFlash_Write(&tmp)) {
        g_runtime_in_flash.rt_dirty_flag = 0;
        return true;
    }
    return false;
}

/**
  * @brief  每 1 秒调用一次（由 TIM7 中断调用）
  *         累计到 3600 秒时，表示运行时间增加 1 小时，
  *         通过 RuntimeFlash_AddRunHours(1) 写回 Flash。
  */
void Runtime_On1sTick(void)
{
    s_uptime_seconds++;

    if (s_uptime_seconds >= 3600U)  // 1U = 1 Second
    {
        s_uptime_seconds = 0U;
        RuntimeFlash_AddRunHours(1);    // RAM 中 +1 小时，并置 rt_dirty_flag=1
        s_runtime_need_save = 1;        // 让后台任务去真正写 Flash
    }
}


// 在主循环里反复调用（非中断环境）
void Runtime_BackgroundTask(void)
{
    if (s_runtime_need_save)
    {
        s_runtime_need_save = 0;
        RuntimeFlash_SaveIfDirty();            // 你原来已有的“脏就写”函数
    }
}
