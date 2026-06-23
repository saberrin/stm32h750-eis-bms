#include "norflash_ex.h"
#include "norflash.h"

void norflash_ex_read(uint8_t *buf, uint32_t addr, uint32_t len)
{
    norflash_read(buf, addr, len); /* 纯间接读 */
}

void norflash_ex_write(const uint8_t *buf, uint32_t addr, uint32_t len)
{
    /* 分页写（256B 对齐），底层已关中断与轮询 WIP */
    while (len)
    {
        uint32_t page_off = addr & (NORFLASH_PAGE_SIZE - 1u);
        uint32_t chunk    = NORFLASH_PAGE_SIZE - page_off;
        if (chunk > len) chunk = len;

        norflash_page_program(buf, addr, chunk);
        addr += chunk;
        buf  += chunk;
        len  -= chunk;
    }
}

void norflash_ex_erase_sector(uint32_t sector_index)
{
    norflash_erase_sector(sector_index);
}

void norflash_ex_erase_chip(void)
{
    norflash_erase_chip();
}

uint16_t norflash_ex_read_id(void)
{
    return norflash_read_id();
}
