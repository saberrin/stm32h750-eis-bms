#include "calib_flash.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

/* 复用你工程的 norflash_ex 接口 */
extern void norflash_ex_read (uint8_t*       dst, uint32_t addr, uint32_t len);
extern void norflash_ex_write(const uint8_t* src, uint32_t addr, uint32_t len);
extern void norflash_ex_erase_sector(uint32_t sector_index);

QG_CalibFlash_t g_calib_in_flash;
static uint8_t s_calib_dirty = 0;

/* 与你其他 flash 模块保持一致：poly 0xEDB88320 */
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

static void calib_fixup_and_crc(QG_CalibFlash_t* c)
{
    c->magic = CALIB_MAGIC;
    c->schema_version = CALIB_SCHEMA_VER;

    /* 总标志位：按你的规则——所有子校准都完成才置 1 */
    if ((c->valid_mask & CALIB_VALID_ALL_REQUIRED) == CALIB_VALID_ALL_REQUIRED) {
        c->calib_done_flag = 1;
    } else {
        c->calib_done_flag = 0;
    }

    c->crc32 = 0;
    c->crc32 = qg_crc32((const uint8_t*)c + 8, (uint32_t)sizeof(*c) - 8);
}

static uint8_t calib_is_valid(const QG_CalibFlash_t* c)
{
    if (c == NULL) return 0;
    if (c->magic != CALIB_MAGIC) return 0;
    if (c->schema_version != CALIB_SCHEMA_VER) return 0;

    uint32_t calc = qg_crc32((const uint8_t*)c + 8, (uint32_t)sizeof(*c) - 8);
    return (calc == c->crc32) ? 1 : 0;
}

static void calib_set_all_zero(QG_CalibFlash_t* c)
{
    memset(c, 0, sizeof(*c));
    /* 关键：参数值“无意义”全 0，但 magic/schema 仍写入，便于识别结构 */
    c->magic = CALIB_MAGIC;
    c->schema_version = CALIB_SCHEMA_VER;
    c->valid_mask = 0;
    c->calib_done_flag = 0;
    /* crc 后面在写入前统一计算 */
}

/* ----------------- Public API ----------------- */

bool CalibFlash_Read(QG_CalibFlash_t* out)
{
    if (!out) return false;
    norflash_ex_read((uint8_t*)out, CALIB_FLASH_ADDR, (uint32_t)sizeof(*out));
    return calib_is_valid(out) ? true : false;
}

bool CalibFlash_Write(const QG_CalibFlash_t* in)
{
    if (!in) return false;

    QG_CalibFlash_t wr = *in;
    calib_fixup_and_crc(&wr);

    uint32_t sector = (uint32_t)(CALIB_FLASH_ADDR / CALIB_FLASH_SECTOR_SIZE);
    norflash_ex_erase_sector(sector);
    norflash_ex_write((const uint8_t*)&wr, CALIB_FLASH_ADDR, (uint32_t)sizeof(wr));

    /* readback verify */
    QG_CalibFlash_t rd;
    memset(&rd, 0, sizeof(rd));
    norflash_ex_read((uint8_t*)&rd, CALIB_FLASH_ADDR, (uint32_t)sizeof(rd));

    if (memcmp(&rd, &wr, sizeof(wr)) != 0) {
        printf("[CalibFlash] write verify failed\r\n");
        return false;
    }

    g_calib_in_flash = wr;
    s_calib_dirty = 0;
    return true;
}

bool CalibFlash_LoadAtStartup(void)
{
    QG_CalibFlash_t rd;
    memset(&rd, 0, sizeof(rd));
    norflash_ex_read((uint8_t*)&rd, CALIB_FLASH_ADDR, (uint32_t)sizeof(rd));

    if (!calib_is_valid(&rd)) {
        printf("[CalibFlash] invalid/empty -> init zeros\r\n");
        calib_set_all_zero(&g_calib_in_flash);
        /* 这里按你的要求：初始化值要“无意义”，done_flag=0 */
        /* 是否立即写入 Flash？建议写入一次，避免下次又判 invalid */
        return CalibFlash_Write(&g_calib_in_flash);
    }

    g_calib_in_flash = rd;
    s_calib_dirty = 0;

    printf("[CalibFlash] loaded OK (mask=0x%08lX, done=%u)\r\n",
           (unsigned long)g_calib_in_flash.valid_mask,
           g_calib_in_flash.calib_done_flag);

    return true;
}

bool CalibFlash_ResetDefaults(void)
{
    printf("[CalibFlash] reset -> zeros\r\n");
    calib_set_all_zero(&g_calib_in_flash);
    s_calib_dirty = 1;
    return CalibFlash_Write(&g_calib_in_flash);
}

bool CalibFlash_SaveIfDirty(void)
{
    if (!s_calib_dirty) return true;
    return CalibFlash_Write(&g_calib_in_flash);
}

bool CalibFlash_IsStructValid(void)
{
    return calib_is_valid(&g_calib_in_flash) ? true : false;
}

bool CalibFlash_IsReadyForEis(void)
{
    if (!calib_is_valid(&g_calib_in_flash)) return false;
    return (g_calib_in_flash.calib_done_flag == 1) ? true : false;
}

void CalibFlash_Dump(const QG_CalibFlash_t* c)
{
    if (!c) c = &g_calib_in_flash;

    printf("CalibFlash dump:\r\n");
    printf("magic            : 0x%08lX\r\n", (unsigned long)c->magic);
    printf("crc32            : 0x%08lX\r\n", (unsigned long)c->crc32);
    printf("schema_version   : 0x%08lX\r\n", (unsigned long)c->schema_version);
    printf("valid_mask       : 0x%08lX\r\n", (unsigned long)c->valid_mask);
    printf("calib_done_flag  : %u\r\n", c->calib_done_flag);

    printf("dac_slope        : %.6f\r\n", c->dac_slope);
    printf("dac_intercept    : %.6f\r\n", c->dac_intercept);
    printf("dac_r2           : %.6f\r\n", c->dac_r2);
    printf("dac_max_err      : %.6f\r\n", c->dac_max_err);

    printf("curr_gain_a      : %.6f\r\n", c->curr_gain_a);
    printf("curr_offset_b    : %.6f\r\n", c->curr_offset_b);

    printf("acq_slope_a      : %.6f\r\n", c->acq_slope_a);
    printf("acq_intercept_b  : %.6f\r\n", c->acq_intercept_b);
    printf("acq_corr         : %.6f\r\n", c->acq_corr);
    printf("acq_std_err      : %.6f\r\n", c->acq_std_err);

    printf("ad620_gain_act   : {");
    for (int i = 0; i < 8; i++) {
        printf("%.6f%s", c->ad620_gain_act[i], (i == 7) ? "" : ", ");
    }
    printf("}\r\n");

    printf("adc_ch_offset    : {");
    for (int i = 0; i < 8; i++) {
        printf("%.6f%s", c->adc_ch_offset[i], (i == 7) ? "" : ", ");
    }
    printf("}\r\n");
		printf("-----------------------------------------\r\n"); 
}

/* ----------------- Update APIs (called by calibration flows) ----------------- */

void CalibFlash_UpdateDac(float slope, float intercept, float r2, float max_err)
{
    g_calib_in_flash.dac_slope     = slope;
    g_calib_in_flash.dac_intercept = intercept;
    g_calib_in_flash.dac_r2        = r2;
    g_calib_in_flash.dac_max_err   = max_err;

    g_calib_in_flash.valid_mask |= CALIB_VALID_DAC;
    s_calib_dirty = 1;
}

void CalibFlash_UpdateCurrExc(float gain_a, float offset_b)
{
    g_calib_in_flash.curr_gain_a   = gain_a;
    g_calib_in_flash.curr_offset_b = offset_b;

    g_calib_in_flash.valid_mask |= CALIB_VALID_CURR_EXC;
    s_calib_dirty = 1;
}

void CalibFlash_UpdateCurrAcq(float slope_a, float intercept_b, float corr, float std_err)
{
    g_calib_in_flash.acq_slope_a     = slope_a;
    g_calib_in_flash.acq_intercept_b = intercept_b;
    g_calib_in_flash.acq_corr        = corr;
    g_calib_in_flash.acq_std_err     = std_err;

    g_calib_in_flash.valid_mask |= CALIB_VALID_CURR_ACQ;
    s_calib_dirty = 1;
}

void CalibFlash_UpdateAfe(const float* ad620_gain_act_8, const float* adc_ch_offset_8)
{
    bool has_gain   = (ad620_gain_act_8 != NULL);
    bool has_offset = (adc_ch_offset_8 != NULL);

    // 1) 先把已经算出来的 AD620 实际增益写入 Flash 镜像
    if (has_gain) {
        for (int i = 0; i < 8; ++i) {
            g_calib_in_flash.ad620_gain_act[i] = ad620_gain_act_8[i];
        }
    }

    // 2) 将来补齐 ADC 通道 offset 的时候，在这里写入
    if (has_offset) {
        for (int i = 0; i < 8; ++i) {
            g_calib_in_flash.adc_ch_offset[i] = adc_ch_offset_8[i];
        }
    }

    // 3) 现在阶段：我们只想“把已有的数据持久化”，不想让 AFE 的标志位影响 calib_done_flag。
    //    所以此处**暂时不修改 valid_mask**，仅仅把脏标志置 1。
    //
    //    将来当 adc_ch_offset 相关代码完善后，可以按如下方式启用 AFE 完整标志：
    //
    //    if (has_gain && has_offset) {
    //        // AD620 增益 + ADC offset 都已经完成标定，
    //        // 此时才把 CALIB_VALID_AFE 这一位打开。
    //        g_calib_in_flash.valid_mask |= CALIB_VALID_AFE;
    //    }

    if (has_gain || has_offset) {
        s_calib_dirty = 1;
    }
}
