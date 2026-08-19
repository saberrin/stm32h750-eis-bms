#include "config_flash.h"
#include <string.h>
#include <stdio.h>
#include "qspi.h" 
#include "fdcan.h"
#include "parse_command.h"
#define DUMP_DATA_BUF_SIZE   512
extern QSPI_HandleTypeDef hqspi;
extern void QSPI_Init(void);
// 低层 Flash 接口
extern void norflash_ex_read (uint8_t*       dst, uint32_t addr, uint32_t len);
extern void norflash_ex_write(const uint8_t* src, uint32_t addr, uint32_t len);
extern void norflash_ex_erase_sector(uint32_t sector_index);


/* 兜底路径专用：把 QSPI 完全复位到干净状态 */
static void cfg_qspi_reinit(void)
{
    HAL_QSPI_DeInit(&hqspi);
    QSPI_Init();   // 重新配置 GPIO + HAL_QSPI_Init
}

/* 慢速 0x03 单线读，完全不依赖 norflash_ex */
static void cfg_qspi_read_slow(uint32_t addr, uint8_t *dst, uint32_t len)
{
    if (!dst || !len) return;

    QSPI_CommandTypeDef cmd = {0};
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction     = 0x03;                    // Read Data
    cmd.AddressMode     = QSPI_ADDRESS_1_LINE;
    cmd.AddressSize     = QSPI_ADDRESS_24_BITS;    // W25Q128: 24bit 地址
    cmd.Address         = addr;
    cmd.DataMode        = QSPI_DATA_1_LINE;
    cmd.DummyCycles     = 0;
    cmd.NbData          = len;

    (void)HAL_QSPI_Command(&hqspi, &cmd, HAL_MAX_DELAY);
    (void)HAL_QSPI_Receive(&hqspi, dst, HAL_MAX_DELAY);
}


// 镜像缓存
QG_ConfigFlash_t current_cfg_in_flash = {0};

// ===== CRC32 (poly 0xEDB88320) =====
static uint32_t simple_crc32(const uint8_t* data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < len; ++i) {
        uint32_t x = (crc ^ data[i]) & 0xFFu;
        for (int b = 0; b < 8; ++b) {
            uint32_t mask = -(x & 1u);
            x = (x >> 1) ^ (0xEDB88320u & mask);
        }
        crc = (crc >> 8) ^ x;
    }
    return ~crc;
}

/* ===== ConfigFlash 专用：慢速 QSPI 写入 (纯延时 + verify) ===== */

#define CFG_SECTOR_SIZE    4096u
#define CFG_PAGE_SIZE      256u

/* 简单版 WREN：单线指令 0x06 */
static void cfg_qspi_wren(void)
{
    QSPI_CommandTypeDef cmd = {0};
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction     = 0x06;                  // WREN
    cmd.AddressMode     = QSPI_ADDRESS_NONE;
    cmd.DataMode        = QSPI_DATA_NONE;
    (void)HAL_QSPI_Command(&hqspi, &cmd, HAL_MAX_DELAY);
}

/* 慢速擦除：只发命令 + 固定延时，不依赖 AutoPolling / SR1 */
static void cfg_qspi_erase_sector_slow(uint32_t addr)
{
    /* addr 必须是 4KB 边界 */
    cfg_qspi_wren();

    QSPI_CommandTypeDef cmd = {0};
    cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction       = 0x20;                // Sector Erase 4KB
    cmd.AddressMode       = QSPI_ADDRESS_1_LINE;
    cmd.AddressSize       = QSPI_ADDRESS_24_BITS; // W25Q128：24bit 地址
    cmd.Address           = addr;
    cmd.DataMode          = QSPI_DATA_NONE;
    cmd.DummyCycles       = 0;

    (void)HAL_QSPI_Command(&hqspi, &cmd, HAL_MAX_DELAY);

    /* 4KB 扇区典型擦除时间 50~100ms，这里粗暴给 200ms */
    HAL_Delay(200);
}

/* 慢速 Page Program：用 0x02 单线写，每页 <=256B，发完命令后固定延时 */
static void cfg_qspi_page_program_slow(uint32_t addr, const uint8_t *src, uint32_t len)
{
    while (len)
    {
        uint32_t page_off = addr & (CFG_PAGE_SIZE - 1u);
        uint32_t chunk    = CFG_PAGE_SIZE - page_off;
        if (chunk > len) chunk = len;

        cfg_qspi_wren();

        QSPI_CommandTypeDef cmd = {0};
        cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
        cmd.Instruction       = 0x02;            // Page Program (1-1-1)
        cmd.AddressMode       = QSPI_ADDRESS_1_LINE;
        cmd.AddressSize       = QSPI_ADDRESS_24_BITS;
        cmd.Address           = addr;
        cmd.DataMode          = QSPI_DATA_1_LINE;
        cmd.DummyCycles       = 0;
        cmd.NbData            = chunk;

        (void)HAL_QSPI_Command(&hqspi, &cmd, HAL_MAX_DELAY);
        (void)HAL_QSPI_Transmit(&hqspi, (uint8_t*)src, HAL_MAX_DELAY);

        /* 写页典型 0.4~3ms，这里给 5ms 保险 */
        HAL_Delay(5);

        addr += chunk;
        src  += chunk;
        len  -= chunk;
    }
}

/* ConfigFlash 专用兜底写：擦 4KB + 写整个结构体 + 读回校验 */
static bool ConfigFlash_SlowWrite_Fallback(const QG_ConfigFlash_t *cfg)
{
    if (!cfg) return false;

    QG_ConfigFlash_t tmp = *cfg;

    tmp.crc32 = 0;
    tmp.crc32 = simple_crc32((const uint8_t*)&tmp, sizeof(tmp));

    uint32_t sector_base = CONFIG_FLASH_ADDR & ~(CFG_SECTOR_SIZE - 1u);

    printf("[CFG][fallback] slow erase+write at 0x%08lX...\r\n",
           (unsigned long)CONFIG_FLASH_ADDR);

    /* 关键：进来先把 QSPI 完全复位，避免沿用 AutoPolling 超时后的坏状态 */
    cfg_qspi_reinit();

    /* 1) 擦 4KB 扇区：WREN + 0x20 + 延时 */
    cfg_qspi_erase_sector_slow(sector_base);

    /* 2) 写 config 结构体：WREN + 0x02 + 分页 + 延时 */
    cfg_qspi_page_program_slow(CONFIG_FLASH_ADDR,
                               (const uint8_t*)&tmp,
                               sizeof(tmp));

    /* 3) 用我们自己的 0x03 慢速读回校验（不再依赖 norflash_ex_read） */
    QG_ConfigFlash_t verify;
    cfg_qspi_read_slow(CONFIG_FLASH_ADDR,
                       (uint8_t*)&verify,
                       sizeof(verify));

    if (memcmp(&tmp, &verify, sizeof(tmp)) != 0)
    {
        printf("[CFG][fallback] verify FAILED!\r\n");

        const uint8_t *p = (const uint8_t*)&tmp;
        const uint8_t *q = (const uint8_t*)&verify;
        uint32_t bad = 0xFFFFFFFFu;

        for (uint32_t i = 0; i < sizeof(tmp); ++i)
        {
            if (p[i] != q[i]) { bad = i; break; }
        }

        if (bad != 0xFFFFFFFFu)
        {
            printf("[CFG][fallback] first_mismatch[%lu]: wr=0x%02X rd=0x%02X\r\n",
                   (unsigned long)bad, p[bad], q[bad]);

            printf("[CFG][fallback] wr[0..31]: ");
            for (int i = 0; i < 32 && i < (int)sizeof(tmp); ++i)
                printf("%02X ", p[i]);
            printf("\r\n");

            printf("[CFG][fallback] rd[0..31]: ");
            for (int i = 0; i < 32 && i < (int)sizeof(tmp); ++i)
                printf("%02X ", q[i]);
            printf("\r\n");
        }

        return false;
    }

    current_cfg_in_flash = verify;
    printf("[CFG][fallback] write OK.\r\n");
    return true;
}



static bool cfg_is_valid(const QG_ConfigFlash_t* in)
{
    if (!in) return false;
    if (in->magic != CONFIG_MAGIC) return false;

    QG_ConfigFlash_t tmp = *in;
    uint32_t stored = tmp.crc32;
    tmp.crc32 = 0;
    uint32_t calc = simple_crc32((const uint8_t*)&tmp, sizeof(tmp));
    return (stored == calc);
}

// ===== 默认值应用到全局（类型与 global_commnd.h 一致）=====
static void load_defaults_to_globals(void)
{
//    QG_SweepPoints    = 100u;
//    QG_SweepEn        = (BoolFlag)1;
//    QG_SweepStartFreq = 10000.0f;
//    QG_SweepStopFreq  = 50.0f;
//    QG_SinFreq        = 10000.0f;
//    QG_ACVoltPP       = 8.0f;
//    QG_DCVolt         = 200.0f;
//    QG_RcalVal        = 1.0f;
//    QG_SweepLog       = (BoolFlag)1;
}

// 打包“全局参数”到结构体
static void pack_globals(QG_ConfigFlash_t* out)
{
    if (!out) return;
    memset(out, 0, sizeof(*out));

    out->magic             = CONFIG_MAGIC;

    /* EIS 配置 */
    out->ac_volt_pp        = QG_ACVoltPP;
    out->dc_volt           = QG_DCVolt;
    out->sweep_start_freq  = QG_EIS_FREQ_START;
    out->sweep_stop_freq   = QG_EIS_FREQ_END;
    out->sweep_points      = QG_EIS_FREQ_POINTS;

    /* 阈值配置 */
    out->temp_high_alarm   = QG_TEMP_HIGH_ALARM;
    out->volt_cell_high    = QG_VOLT_CELL_HIGH;
    out->volt_cell_low     = QG_VOLT_CELL_LOW;
    out->curr_chg_alarm    = QG_CURR_CHG_ALARM;
    out->curr_dis_alarm    = QG_CURR_DIS_ALARM;

    /* 电芯数量 */
    out->cell_count        = QG_CELL_COUNT;

    /* 校准系数 */
    for (int i = 0; i < 10; i++)
        out->calib_data[i] = SET_CALIB_DATA[i];

}


// 应用结构体到“全局参数”
static void apply_to_globals(const QG_ConfigFlash_t* in)
{
    if (!in) return;

    /* EIS 配置 */
    QG_ACVoltPP       = in->ac_volt_pp;
    QG_DCVolt         = in->dc_volt;
    QG_EIS_FREQ_START = in->sweep_start_freq;
    QG_EIS_FREQ_END   = in->sweep_stop_freq;
    QG_EIS_FREQ_POINTS= in->sweep_points;

    /* 阈值 */
    QG_TEMP_HIGH_ALARM= in->temp_high_alarm;
    QG_VOLT_CELL_HIGH = in->volt_cell_high;
    QG_VOLT_CELL_LOW  = in->volt_cell_low;
    QG_CURR_CHG_ALARM = in->curr_chg_alarm;
    QG_CURR_DIS_ALARM = in->curr_dis_alarm;

    /* 电芯数量 */
    QG_CELL_COUNT     = in->cell_count;

    /* 校准数组 */
    for (int i = 0; i < 10; i++)
        SET_CALIB_DATA[i] = in->calib_data[i];

}


bool ConfigFlash_Read(QG_ConfigFlash_t* out)
{
    if (!out) return false;
    norflash_ex_read((uint8_t*)out, CONFIG_FLASH_ADDR, sizeof(QG_ConfigFlash_t));
    return cfg_is_valid(out);
}

bool ConfigFlash_Write(const QG_ConfigFlash_t* cfg)
{
    if (!cfg) return false;

    QG_ConfigFlash_t tmp = *cfg;
    tmp.crc32 = 0;
    tmp.crc32 = simple_crc32((const uint8_t*)&tmp, sizeof(tmp));

    uint32_t sector = CONFIG_FLASH_ADDR / 4096u;

    /* 先走现有 norflash_ex (AutoPolling) 快路径 */
    norflash_ex_erase_sector(sector);
    norflash_ex_write((const uint8_t*)&tmp, CONFIG_FLASH_ADDR, sizeof(tmp));

    // 校验
    QG_ConfigFlash_t verify;
    norflash_ex_read((uint8_t*)&verify, CONFIG_FLASH_ADDR, sizeof(verify));
    if (memcmp(&tmp, &verify, sizeof(tmp)) != 0)
    {
        /* ====== 主路径失败，进入“纯延时 + verify”兜底方案 ====== */
        printf("ConfigFlash: verify FAILED! try slow fallback...\r\n");

        if (!ConfigFlash_SlowWrite_Fallback(&tmp))
        {
            printf("ConfigFlash: slow fallback FAILED!\r\n");
            return false;
        }

        /* fallback 已经内部更新了 current_cfg_in_flash 并打印 OK */
        return true;
    }

    current_cfg_in_flash = verify;
    printf("ConfigFlash: write OK.\r\n");
    return true;
}


void ConfigFlash_LoadAtStartup(void)
{
    QG_ConfigFlash_t cfg;
    if (ConfigFlash_Read(&cfg)) {
        apply_to_globals(&cfg);
        current_cfg_in_flash = cfg;

//        /* 基础信息 */
//				Send_Line("magic            : 0x%08lX\r\n", (unsigned long)cfg.magic);
//				Send_Line("crc32            : 0x%08lX\r\n", (unsigned long)cfg.crc32);

//				/* EIS 配置 */
//				Send_Line("ac_volt_pp       : %.3f\r\n", (double)cfg.ac_volt_pp);
//				Send_Line("dc_volt          : %.3f\r\n", (double)cfg.dc_volt);
//				Send_Line("sweep_start_freq : %.3f\r\n", (double)cfg.sweep_start_freq);
//				Send_Line("sweep_stop_freq  : %.3f\r\n", (double)cfg.sweep_stop_freq);
//				Send_Line("sweep_points     : %lu\r\n", (unsigned long)cfg.sweep_points);


//				/* 阈值 */
//				Send_Line("temp_high_alarm  : %.2f\r\n", (double)cfg.temp_high_alarm);
//				Send_Line("volt_cell_high   : %.2f\r\n", (double)cfg.volt_cell_high);
//				Send_Line("volt_cell_low    : %.2f\r\n", (double)cfg.volt_cell_low);
//				Send_Line("curr_chg_alarm   : %.2f\r\n", (double)cfg.curr_chg_alarm);
//				Send_Line("curr_dis_alarm   : %.2f\r\n", (double)cfg.curr_dis_alarm);

//				/* 电芯数量 */
//				Send_Line("cell_count       : %lu\r\n", (unsigned long)cfg.cell_count);

//				/* 校准数组 */
//				for (int i = 0; i < 10; i++)
//						Send_Line("calib_data[%d]    : %.6f\r\n", i, (double)cfg.calib_data[i]);

				ConfigFlash_Dump(&current_cfg_in_flash);

    } else {
        printf("ConfigFlash: invalid/empty. Using defaults.\r\n");
        load_defaults_to_globals();

        QG_ConfigFlash_t def;
        pack_globals(&def);
        if (!ConfigFlash_Write(&def)) {
            printf("ConfigFlash: write defaults FAILED!\r\n");
        }
    }

}

bool ConfigFlash_SaveFromGlobals(void)
{
    QG_ConfigFlash_t cfg;
    pack_globals(&cfg);
    return ConfigFlash_Write(&cfg);
}

bool ConfigFlash_SaveIfChanged(void)
{
    QG_ConfigFlash_t now, last = current_cfg_in_flash;

    // 打包当前全局
    pack_globals(&now);
    now.crc32 = 0;
    now.crc32 = simple_crc32((const uint8_t*)&now, sizeof(now));

    printf("[CFG] SaveIfChanged: last_stop=%.3f, now_stop=%.3f\r\n",
           (double)last.sweep_stop_freq, (double)now.sweep_stop_freq);

    // 若 Flash 中没有有效镜像，或与当前不一致 → 写回
    if (!cfg_is_valid(&last) || memcmp(&now, &last, sizeof(now)) != 0) {

				printf("[CFG] changed, writing to flash...\r\n");

        return ConfigFlash_Write(&now);     // ★ 真正写 flash
    }
		printf("[CFG] no change, skip write.\r\n");
    return true; // 没变化也返回 true，表示“不需要写”
}


void ConfigFlash_Dump(const QG_ConfigFlash_t *cfg)
{
    if (!cfg) return;

    Send_Line(">0x%02X, GETCFG, CMD_OK\r\n", QG_ID);

    /* 基础信息 */
    Send_Line("magic            : 0x%08lX\r\n", (unsigned long)cfg->magic);
    Send_Line("crc32            : 0x%08lX\r\n", (unsigned long)cfg->crc32);

    /* EIS 配置 */
    Send_Line("ac_volt_pp       : %.3f\r\n", (double)cfg->ac_volt_pp);
    Send_Line("dc_volt          : %.3f\r\n", (double)cfg->dc_volt);
    Send_Line("sweep_start_freq : %.3f\r\n", (double)cfg->sweep_start_freq);
    Send_Line("sweep_stop_freq  : %.3f\r\n", (double)cfg->sweep_stop_freq);
    Send_Line("sweep_points     : %lu\r\n", (unsigned long)cfg->sweep_points);
    Send_Line("sweep_en         : %u\r\n", cfg->sweep_en);
    Send_Line("sweep_log        : %u\r\n", cfg->sweep_log);

    /* 阈值 */
    Send_Line("temp_high_alarm  : %.2f\r\n", (double)cfg->temp_high_alarm);
    Send_Line("volt_cell_high   : %.2f\r\n", (double)cfg->volt_cell_high);
    Send_Line("volt_cell_low    : %.2f\r\n", (double)cfg->volt_cell_low);
    Send_Line("curr_chg_alarm   : %.2f\r\n", (double)cfg->curr_chg_alarm);
    Send_Line("curr_dis_alarm   : %.2f\r\n", (double)cfg->curr_dis_alarm);

    /* 电芯数量 */
    Send_Line("cell_count       : %lu\r\n", (unsigned long)cfg->cell_count);

    /* 校准数组 */
    for (int i = 0; i < 10; i++)
        Send_Line("calib_data[%d]    : %.6f\r\n", i, (double)cfg->calib_data[i]);

		Send_Line("<\r\n");

}




void Flash_EraseAllParamZones(void)
{
    uint32_t sector;

    printf("==== Flash ERASE ALL param zones ====\r\n");

    // 1) CONFIG
    sector = CONFIG_FLASH_ADDR / 4096u;
    printf("  - Erase CONFIG   sector %lu @0x%08lX\r\n",
           (unsigned long)sector, (unsigned long)CONFIG_FLASH_ADDR);
    norflash_ex_erase_sector(sector);

    // 2) CALIB
    sector = CALIB_FLASH_ADDR / 4096u;
    printf("  - Erase CALIB    sector %lu @0x%08lX\r\n",
           (unsigned long)sector, (unsigned long)CALIB_FLASH_ADDR);
    norflash_ex_erase_sector(sector);

    // 3) ID
    sector = ID_FLASH_ADDR / 4096u;
    printf("  - Erase ID       sector %lu @0x%08lX\r\n",
           (unsigned long)sector, (unsigned long)ID_FLASH_ADDR);
    norflash_ex_erase_sector(sector);

    // 4) RUNTIME
    sector = RUNTIME_FLASH_ADDR / 4096u;
    printf("  - Erase RUNTIME  sector %lu @0x%08lX\r\n",
           (unsigned long)sector, (unsigned long)RUNTIME_FLASH_ADDR);
    norflash_ex_erase_sector(sector);

    printf("==== Flash ERASE ALL done. Please reset board. ====\r\n");
}
