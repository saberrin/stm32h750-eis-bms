#ifndef __CALIB_FLASH_H__
#define __CALIB_FLASH_H__

#include <stdint.h>
#include <stdbool.h>
#include "config_flash.h"   // 复用 CALIB_FLASH_ADDR

#ifdef __cplusplus
extern "C" {
#endif

/* ---------- Packed struct macro (ARMCC V5 compatible) ---------- */
#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
  #define QG_PACKED_STRUCT __packed struct
#elif defined(__GNUC__) || defined(__clang__)
  #define QG_PACKED_STRUCT struct __attribute__((packed))
#else
  #define QG_PACKED_STRUCT struct
#endif


#define CALIB_FLASH_SECTOR_SIZE   (4096UL)

/* ---------- Magic / schema ---------- */
#define CALIB_MAGIC        (0x5143414CUL)   /* 'QCAL' */
#define CALIB_SCHEMA_VER   (0x00010001UL)

/* ---------- VALID_MASK bits ---------- */
#define CALIB_VALID_DAC            (1UL << 0)
#define CALIB_VALID_CURR_EXC       (1UL << 1)
#define CALIB_VALID_CURR_ACQ       (1UL << 2)
#define CALIB_VALID_AFE            (1UL << 3)
#define CALIB_VALID_ALL_REQUIRED   (CALIB_VALID_DAC | CALIB_VALID_CURR_ACQ )
//strictly require all complete to proceeed eis test
//#define CALIB_VALID_ALL_REQUIRED   (CALIB_VALID_DAC | CALIB_VALID_CURR_EXC | CALIB_VALID_CURR_ACQ | CALIB_VALID_AFE)

typedef QG_PACKED_STRUCT
{
    uint32_t magic;
    uint32_t crc32;
    uint32_t schema_version;

    uint32_t valid_mask;

    /* DAC linear calibration */
    float    dac_slope;
    float    dac_intercept;
    float    dac_r2;        /* 可选：调试用 */
    float    dac_max_err;   /* 可选：调试用 */

    /* Current excitation calibration: I = A*V + B */
    float    curr_gain_a;
    float    curr_offset_b;

    /* Current acquisition calibration: I/V = a*Vadc + b (按你实际定义) */
    float    acq_slope_a;
    float    acq_intercept_b;
    float    acq_corr;
    float    acq_std_err;

    /* Analog front-end */
    float    ad620_gain_act[8];
    float    adc_ch_offset[8];

    uint8_t  calib_done_flag;   /* 0/1：你要的总标志 */
    uint8_t  reserved[3];
} QG_CalibFlash_t;

/* 全局镜像（RAM 中） */
extern QG_CalibFlash_t g_calib_in_flash;

/* 生命周期 */
bool CalibFlash_LoadAtStartup(void);
bool CalibFlash_Read(QG_CalibFlash_t* out);
bool CalibFlash_Write(const QG_CalibFlash_t* in);
bool CalibFlash_SaveIfDirty(void);

/* 状态查询 */
bool CalibFlash_IsStructValid(void);     /* magic/crc/schema 通过 */
bool CalibFlash_IsReadyForEis(void);     /* done_flag==1 且 mask 满足 */

/* 调试 */
void CalibFlash_Dump(const QG_CalibFlash_t* c);

/* 复位（把参数恢复为“无意义默认值”，done=0，mask=0） */
bool CalibFlash_ResetDefaults(void);

/* 更新接口（每类校准完成后调用） */
void CalibFlash_UpdateDac(float slope, float intercept, float r2, float max_err);
void CalibFlash_UpdateCurrExc(float gain_a, float offset_b);
void CalibFlash_UpdateCurrAcq(float slope_a, float intercept_b, float corr, float std_err);
void CalibFlash_UpdateAfe(const float* ad620_gain_act_8, const float* adc_ch_offset_8);

#ifdef __cplusplus
}
#endif

#endif /* __CALIB_FLASH_H__ */
