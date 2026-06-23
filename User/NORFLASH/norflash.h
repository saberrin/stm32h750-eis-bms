#ifndef __NORFLASH_H__
#define __NORFLASH_H__

#include "stm32h7xx_hal.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* JEDEC ID（按你板子一致） */
#define NORFLASH_TYPE_W25Q80     0xEF13u
#define NORFLASH_TYPE_W25Q16     0xEF14u
#define NORFLASH_TYPE_W25Q32     0xEF15u
#define NORFLASH_TYPE_W25Q64     0xEF16u
#define NORFLASH_TYPE_W25Q128    0xEF17u    /* W25Q128JV: 128M-bit, 16MB */
#define NORFLASH_TYPE_W25Q256    0xEF18u

/* 几何参数 */
#define NORFLASH_PAGE_SIZE       256u
#define NORFLASH_SECTOR_SIZE     4096u

/* 导出：芯片类型/地址位宽获取 */
extern uint16_t g_norflash_type;
uint32_t norflash_get_addr_size(void); /* 返回 QSPI_ADDRESS_24_BITS / 32_BITS */

/* 基础命令集（间接模式） */
void     norflash_init(void);

uint16_t norflash_read_id(void);

uint8_t  norflash_read_sr(uint8_t sr_no);            /* 1/2/3 */
void     norflash_write_sr(uint8_t sr_no, uint8_t v);/* 1/2/3 */
void     norflash_write_enable(void);
void     norflash_write_disable(void);
void     norflash_wait_busy(void);

void     norflash_read(uint8_t *buf, uint32_t addr, uint32_t len);
void     norflash_page_program(const uint8_t *buf, uint32_t addr, uint32_t len);
void     norflash_erase_sector(uint32_t sector_index);
void     norflash_erase_chip(void);

#ifdef __cplusplus
}
#endif

#endif /* __NORFLASH_H__ */
