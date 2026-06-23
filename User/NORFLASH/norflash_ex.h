#ifndef __NORFLASH_EX_H__
#define __NORFLASH_EX_H__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus 
extern "C" {
#endif

/* 工程外壳层（不使用 Memory-Mapped，直接调用基础层） */
void     norflash_ex_read(uint8_t *buf, uint32_t addr, uint32_t len);
void     norflash_ex_write(const uint8_t *buf, uint32_t addr, uint32_t len);
void     norflash_ex_erase_sector(uint32_t sector_index);
void     norflash_ex_erase_chip(void);
uint16_t norflash_ex_read_id(void);

#ifdef __cplusplus
}
#endif

#endif /* __NORFLASH_EX_H__ */
