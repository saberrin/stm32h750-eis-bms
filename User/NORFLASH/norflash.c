#include "norflash.h"
#include "qspi.h"
#include <string.h>


/* ---------- 内部工具 ---------- */

static inline uint32_t enter_critical(void)
{ uint32_t p = __get_PRIMASK(); __disable_irq(); return p; }

static inline void exit_critical(uint32_t p)
{ if (!p) __enable_irq(); }

/* 当前地址位宽（W25Q128 -> 24-bit） */
static uint32_t s_addr_size = QSPI_ADDRESS_24_BITS;
uint32_t norflash_get_addr_size(void) { return s_addr_size; }

/* 全局识别到的 JEDEC ID（高8位厂商 + 低8位容量编码） */
uint16_t g_norflash_type = 0;

/* ---------- 通用命令封装 ---------- */

static void qspi_cmd_simple(uint8_t instr)
{
    QSPI_CommandTypeDef cmd = {0};
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction     = instr;
    cmd.AddressMode     = QSPI_ADDRESS_NONE;
    cmd.DataMode        = QSPI_DATA_NONE;
    (void)HAL_QSPI_Command(&hqspi, &cmd, HAL_MAX_DELAY);
}

/* ---------- 基础功能实现 ---------- */

uint16_t norflash_read_id(void)
{
    QSPI_CommandTypeDef cmd = {0};
    uint8_t id[2] = {0};

    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction     = 0x90;                     /* 90h: Manufacturer/Device ID */
    cmd.AddressMode     = QSPI_ADDRESS_1_LINE;
    cmd.AddressSize     = QSPI_ADDRESS_24_BITS;
    cmd.Address         = 0x000000;
    cmd.DataMode        = QSPI_DATA_1_LINE;
    cmd.NbData          = 2;

    (void)HAL_QSPI_Command(&hqspi, &cmd, HAL_MAX_DELAY);
    (void)HAL_QSPI_Receive(&hqspi, id, HAL_MAX_DELAY);

    return (uint16_t)((id[0] << 8) | id[1]);
}

uint8_t norflash_read_sr(uint8_t sr_no)
{
    QSPI_CommandTypeDef cmd = {0};
    uint8_t instr = 0x05;                           /* SR1 */
    if (sr_no == 2) instr = 0x35;
    else if (sr_no == 3) instr = 0x15;

    uint8_t v = 0;
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction     = instr;
    cmd.DataMode        = QSPI_DATA_1_LINE;
    cmd.NbData          = 1;

    (void)HAL_QSPI_Command(&hqspi, &cmd, HAL_MAX_DELAY);
    (void)HAL_QSPI_Receive(&hqspi, &v, HAL_MAX_DELAY);
    return v;
}

void norflash_write_sr(uint8_t sr_no, uint8_t v)
{
    QSPI_CommandTypeDef cmd = {0};
    uint8_t instr = 0x01;                           /* WRSR1 */
    if (sr_no == 2) instr = 0x31;                   /* WRSR2 */
    else if (sr_no == 3) instr = 0x11;              /* WRSR3 */

    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction     = instr;
    cmd.DataMode        = QSPI_DATA_1_LINE;
    cmd.NbData          = 1;

    (void)HAL_QSPI_Command(&hqspi, &cmd, HAL_MAX_DELAY);
    (void)HAL_QSPI_Transmit(&hqspi, &v, HAL_MAX_DELAY);
}

void norflash_write_enable(void)  { qspi_cmd_simple(0x06); }   /* WREN  */
void norflash_write_disable(void) { qspi_cmd_simple(0x04); }   /* WRDI  */

//// 读 SR1 的工具函数（如果你已有，就复用自己的）
//static HAL_StatusTypeDef norflash_read_sr1(uint8_t *sr1)
//{
//    QSPI_CommandTypeDef cmd = {0};

//    cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
//    cmd.Instruction       = 0x05;   // Read Status Register-1
//    cmd.AddressMode       = QSPI_ADDRESS_NONE;
//    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
//    cmd.DataMode          = QSPI_DATA_1_LINE;
//    cmd.DummyCycles       = 0;
//    cmd.NbData            = 1;

//    HAL_StatusTypeDef st;

//    st = HAL_QSPI_Command(&hqspi, &cmd, 50);
//    if (st != HAL_OK) return st;

//    st = HAL_QSPI_Receive(&hqspi, sr1, 50);
//    return st;
//}

/* 使用 HAL_QSPI_AutoPolling 等待 SR1.WIP 清零 */
static HAL_StatusTypeDef norflash_wait_wip_clear(const char *tag, uint32_t timeout_ms)
{
    HAL_StatusTypeDef       st;
    QSPI_CommandTypeDef     cmd = {0};
    QSPI_AutoPollingTypeDef cfg = {0};

    /* 配置 Read Status Register-1 命令 (0x05)，1-1-1 模式 */
    cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction       = 0x05;                     /* RDSR1 */
    cmd.AddressMode       = QSPI_ADDRESS_NONE;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode          = QSPI_DATA_1_LINE;
    cmd.DummyCycles       = 0;
    cmd.NbData            = 1;

    /* 轮询配置：Mask=0x01, Match=0x00 -> 等待 WIP(bit0)=0 */
    cfg.Mask            = 0x01;
    cfg.Match           = 0x00;
    cfg.MatchMode       = QSPI_MATCH_MODE_AND;
    cfg.StatusBytesSize = 1;
    cfg.Interval        = 0x10;
    cfg.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

    st = HAL_QSPI_AutoPolling(&hqspi, &cmd, &cfg, timeout_ms);
    if (st != HAL_OK)
    {
        printf("[NOR] wait_busy(%s): AutoPolling fail, st=%d, state=%d, err=0x%08lX\r\n",
               tag, (int)st, (int)hqspi.State, (unsigned long)hqspi.ErrorCode);
    }
    return st;
}

/* 对外接口：兼容原型 void norflash_wait_busy(void) */
void norflash_wait_busy(void)
{
    /* 擦 4KB 扇区通常 < 500ms，这里给 2000ms 保险 */
    (void)norflash_wait_wip_clear("generic", 2000);
}




//void norflash_wait_busy(void)
//{
//    const uint32_t timeout_ms = 500;        // 擦/写 4KB，500ms 足够
//    uint32_t tick_start = HAL_GetTick();
//    HAL_StatusTypeDef st;
//    uint8_t sr1 = 0;

//    for (;;)
//    {
//        st = norflash_read_sr1(&sr1);
//        if (st != HAL_OK) {
//            // 读取 SR1 失败：打印当前 QSPI 状态，然后重置 QSPI
//            printf("[NOR] wait_busy: read_sr1 fail, st=%d, state=%d, err=0x%08lX -> reset QSPI + fallback delay\r\n",
//                   (int)st, (int)hqspi.State, (unsigned long)hqspi.ErrorCode);

//            // 1) 先反初始化 QSPI
//            HAL_QSPI_DeInit(&hqspi);

//            // 2) 再调用你 qspi.c 里的初始化函数
//            QSPI_Init();    // ★ 就是你截图里的这个函数

//            // 3) 给 Flash 一点时间完成当前擦/写
//            HAL_Delay(50);

//            // 结束等待，由上层的 HAL_Delay + 读回校验兜底
//            return;
//        }

//        // WIP=0，操作完成
//        if ((sr1 & 0x01u) == 0) {
//            return;
//        }

//        // 超时保护，防止死循环
//        if ((HAL_GetTick() - tick_start) >= timeout_ms) {
//            printf("[NOR] wait_busy TIMEOUT, SR1=0x%02X\r\n", sr1);
//            return;
//        }
//    }
//}


//void norflash_wait_busy(void)
//{
//    /* 轮询 SR1.BUSY=0 */
//    for (;;)
//    {
//        uint8_t sr1 = norflash_read_sr(1);
//        if ((sr1 & 0x01u) == 0) break;
//    }
//}

/* Quad I/O Fast Read (0xEB, 1-4-4), dummy=6 */
//void norflash_read(uint8_t *buf, uint32_t addr, uint32_t len)
//{
//    if (!buf || !len) return;

//    QSPI_CommandTypeDef cmd = {0};
//    cmd.InstructionMode    = QSPI_INSTRUCTION_1_LINE;
//    cmd.Instruction        = 0xEB;                    /* Fast Read Quad I/O */
//    cmd.AddressMode        = QSPI_ADDRESS_4_LINES;    /* 四线地址 */
//    cmd.AddressSize        = norflash_get_addr_size();
//    cmd.Address            = addr;
//    cmd.AlternateByteMode  = QSPI_ALTERNATE_BYTES_4_LINES;
//    cmd.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
//    cmd.AlternateBytes     = 0x00;
//    cmd.DummyCycles        = 6;                       /* 保守 */
//    cmd.DataMode           = QSPI_DATA_4_LINES;
//    cmd.NbData             = len;

//    (void)HAL_QSPI_Command(&hqspi, &cmd, HAL_MAX_DELAY);
//    (void)HAL_QSPI_Receive(&hqspi, buf, HAL_MAX_DELAY);
//}

void norflash_read(uint8_t *buf, uint32_t addr, uint32_t len)
{
    if (!buf || !len) return;

    QSPI_CommandTypeDef cmd = {0};
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction     = 0x03;                    /* 普通 Read Data */
    cmd.AddressMode     = QSPI_ADDRESS_1_LINE;
    cmd.AddressSize     = norflash_get_addr_size();
    cmd.Address         = addr;
    cmd.DataMode        = QSPI_DATA_1_LINE;
    cmd.DummyCycles     = 0;                       /* 无 dummy */
    cmd.NbData          = len;

    (void)HAL_QSPI_Command(&hqspi, &cmd, HAL_MAX_DELAY);
    (void)HAL_QSPI_Receive(&hqspi, buf, HAL_MAX_DELAY);
}


/* 0x32 Quad Page Program (1-1-4), len<=256 */
void norflash_page_program(const uint8_t *buf, uint32_t addr, uint32_t len)
{
    if (!buf || !len || len > NORFLASH_PAGE_SIZE) return;

    /* 1) 关中断，只保护 WREN + 发送命令+数据 这小段 */
    uint32_t primask = enter_critical();
    norflash_write_enable();

    QSPI_CommandTypeDef cmd = {0};
    cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction       = 0x32;                     /* Quad Page Program */
    cmd.AddressMode       = QSPI_ADDRESS_1_LINE;
    cmd.AddressSize       = norflash_get_addr_size();
    cmd.Address           = addr;
    cmd.DataMode          = QSPI_DATA_4_LINES;
    cmd.DummyCycles       = 0;
    cmd.NbData            = len;

    (void)HAL_QSPI_Command(&hqspi, &cmd, HAL_MAX_DELAY);
    (void)HAL_QSPI_Transmit(&hqspi, (uint8_t*)buf, HAL_MAX_DELAY);

    /* 2) 写操作交给 Flash 自己慢慢干，这里先恢复中断 */
    exit_critical(primask);

    /* 3) 在中断开启状态下阻塞等待 WIP清零 */
    (void)norflash_wait_wip_clear("page_program", 1000);
}




/* 4KB Sector Erase (0x20) wait for busy version */
//void norflash_erase_sector(uint32_t sector_index)
//{
//    uint32_t addr = sector_index * NORFLASH_SECTOR_SIZE;

//    uint32_t primask = enter_critical();              /* ★ 关中断 */

//		printf("[NOR] erase_sector(%lu) step1: WREN + wait_busy(before cmd)\r\n",
//    (unsigned long)sector_index);

//    norflash_write_enable();
//    //norflash_wait_busy();

//    printf("[NOR] erase_sector(%lu) step2: send 0x20 ERASE cmd @0x%08lX\r\n",
//           (unsigned long)sector_index, (unsigned long)addr);

//    QSPI_CommandTypeDef cmd = {0};
//    cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
//    cmd.Instruction       = 0x20;                     /* Sector Erase 4KB */
//    cmd.AddressMode       = QSPI_ADDRESS_1_LINE;
//    cmd.AddressSize       = norflash_get_addr_size();
//    cmd.Address           = addr;

//    (void)HAL_QSPI_Command(&hqspi, &cmd, HAL_MAX_DELAY);

//    printf("[NOR] erase_sector(%lu) step3: wait_busy(after cmd)\r\n",
//           (unsigned long)sector_index);

//    norflash_wait_busy();

//    printf("[NOR] erase_sector(%lu) step4: done, exit critical\r\n",
//           (unsigned long)sector_index);

//    exit_critical(primask);                            /* ★ 开中断 */
//}

/* 4KB Sector Erase (0x20) */
void norflash_erase_sector(uint32_t sector_index)
{
    uint32_t addr = sector_index * NORFLASH_SECTOR_SIZE;

    printf("[NOR] erase_sector(%lu): 0x20 @0x%08lX\r\n",
           (unsigned long)sector_index, (unsigned long)addr);

    /* 1) 关中断，只保护 WREN + 发命令 这段短操作 */
    uint32_t primask = enter_critical();
    norflash_write_enable();

    QSPI_CommandTypeDef cmd = {0};
    cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction       = 0x20;                     /* Sector Erase 4KB */
    cmd.AddressMode       = QSPI_ADDRESS_1_LINE;
    cmd.AddressSize       = norflash_get_addr_size();
    cmd.Address           = addr;
    cmd.DataMode          = QSPI_DATA_NONE;
    cmd.DummyCycles       = 0;

    (void)HAL_QSPI_Command(&hqspi, &cmd, HAL_MAX_DELAY);

    /* 2) 很重要：发完命令就开中断，让 SysTick 和 HAL 超时机制能正常工作 */
    exit_critical(primask);

    /* 3) 在中断开启的状态下，阻塞等待 WIP 清零 */
    (void)norflash_wait_wip_clear("erase_sector", 2000);

    printf("[NOR] erase_sector(%lu) done.\r\n", (unsigned long)sector_index);
}




/* Chip Erase (0xC7) */
void norflash_erase_chip(void)
{
    uint32_t primask = enter_critical();
    norflash_write_enable();

    QSPI_CommandTypeDef cmd = {0};
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction     = 0xC7;

    (void)HAL_QSPI_Command(&hqspi, &cmd, HAL_MAX_DELAY);

    /* 片擦时间比较长，这里给 120000ms = 120 秒保险 */
    (void)norflash_wait_wip_clear("erase_chip", 120000);

    exit_critical(primask);
}


/* 初始化：识别 ID，固定 24-bit 地址，确保 QE=1 */
void norflash_init(void)
{
    QSPI_Init();

    g_norflash_type = norflash_read_id();

    /* W25Q128: 24-bit 地址 */
    s_addr_size = QSPI_ADDRESS_24_BITS;

    /* 退出 QPI（Winbond常用0xFF；若不支持，忽略失败不影响） */
    qspi_cmd_simple(0xFF);

    /* 确保 QE 置位（SR2.B1） */
    /* 确保 QE 位置 1（SR2.B1） */
	uint8_t sr2 = norflash_read_sr(2);
	if ((sr2 & 0x02u) == 0)
	{
			norflash_write_enable();
			norflash_write_sr(2, sr2 | 0x02u);
			/* 写 SR 很快，给 1000ms 足够 */
			(void)norflash_wait_wip_clear("write_sr2_QE", 1000);
}
}

