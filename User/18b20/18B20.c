#include "ds18b20.h"

/**
 * @brief       微秒延时函数
 * @param       nus: 要延时的微秒数
 * @note        基于Systick实现，比NOP延时更精确[5](@ref)
 */
void TEM_delay_us(uint32_t nus)
{
    uint32_t ticks;
    uint32_t told, tnow, tcnt = 0;
    uint32_t reload = SysTick->LOAD;    /* LOAD的值 */
    
    ticks = nus * (SystemCoreClock / 1000000);  /* 需要的节拍数 */
    tcnt = 0;
    told = SysTick->VAL;                /* 刚进入时的计数器值 */
    
    while(1)
    {
        tnow = SysTick->VAL;
        if(tnow != told)
        {
            if(tnow < told)
            {
                tcnt += told - tnow;    /* SYSTICK是递减计数器 */
            }
            else
            {
                tcnt += reload - tnow + told;
            }
            told = tnow;
            if(tcnt >= ticks)
            {
                break;                  /* 时间超过/等于要延迟的时间,则退出 */
            }
        }
    }
}

/**
 * @brief       复位DS18B20
 * @param       无
 * @retval      无
 */
static void ds18b20_reset(void)
{
    DS18B20_DQ_OUT(0);      /* 拉低DQ,复位 */
    TEM_delay_us(750);           /* 拉低750us */
    DS18B20_DQ_OUT(1);      /* DQ=1, 释放复位 */
    TEM_delay_us(15);           /* 延迟15us */
}

/**
 * @brief       等待DS18B20的回应
 * @param       无
 * @retval      0, DS18B20正常
 *              1, DS18B20异常/不存在
 */
static uint8_t ds18b20_check(void)
{
    uint8_t retry = 0;
    uint8_t rval = 0;
    
    /* 等待DQ变低, 等待200us */
    while(DS18B20_DQ_IN && retry < 200)
    {
        retry++;
        TEM_delay_us(1);
    }
    
    if(retry >= 200)
    {
        rval = 1;
    }
    else
    {
        retry = 0;
        /* 等待DQ变高, 等待240us */
        while(!DS18B20_DQ_IN && retry < 240)
        {
            retry++;
            TEM_delay_us(1);
        }
        if(retry >= 240) rval = 1;
    }
    
    return rval;
}

/**
 * @brief       从DS18B20读取一个位
 * @param       无
 * @retval      读取到的位值: 0 / 1
 */
static uint8_t ds18b20_read_bit(void)
{
    uint8_t data = 0;
    
    DS18B20_DQ_OUT(0);
    TEM_delay_us(2);
    DS18B20_DQ_OUT(1);
    TEM_delay_us(12);
    
    if(DS18B20_DQ_IN)
    {
        data = 1;
    }
    
    TEM_delay_us(50);
    return data;
}

/**
 * @brief       从DS18B20读取一个字节
 * @param       无
 * @retval      读到的数据
 */
static uint8_t ds18b20_read_byte(void)
{
    uint8_t i, b, data = 0;
    
    for(i = 0; i < 8; i++)
    {
        b = ds18b20_read_bit();  /* DS18B20先输出低位数据 */
        data |= b << i;           /* 填充data的每一位 */
    }
    
    return data;
}

/**
 * @brief       写一个字节到DS18B20
 * @param       data: 要写入的字节
 * @retval      无
 */
static void ds18b20_write_byte(uint8_t data)
{
    uint8_t j;
    
    for(j = 1; j <= 8; j++)
    {
        if(data & 0x01)
        {
            /* 写1 */
            DS18B20_DQ_OUT(0);
            TEM_delay_us(2);
            DS18B20_DQ_OUT(1);
            TEM_delay_us(60);
        }
        else
        {
            /* 写0 */
            DS18B20_DQ_OUT(0);
            TEM_delay_us(60);
            DS18B20_DQ_OUT(1);
            TEM_delay_us(2);
        }
        data >>= 1;  /* 右移,获取高一位数据 */
    }
}

/**
 * @brief       开始温度转换
 * @param       无
 * @retval      无
 */
static void ds18b20_start(void)
{
    ds18b20_reset();
    ds18b20_check();
    ds18b20_write_byte(0xCC);   /* 跳过ROM */
    ds18b20_write_byte(0x44);   /* 开始温度转换 */
}

/**
 * @brief       初始化DS18B20的IO口并检测DS18B20的存在
 * @param       无
 * @retval      0, 正常
 *              1, 不存在/不正常
 */
uint8_t ds18b20_init(void)
{
    GPIO_InitTypeDef gpio_init_struct = {0};
    
    /* 开启DQ引脚时钟 */
    DS18B20_DQ_GPIO_CLK_ENABLE();
    
    /* 配置GPIO为开漏输出模式 */
    gpio_init_struct.Pin = DS18B20_DQ_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_OD;        /* 开漏输出 */
    gpio_init_struct.Pull = GPIO_PULLUP;                /* 上拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;      /* 高速 */
    HAL_GPIO_Init(DS18B20_DQ_GPIO_PORT, &gpio_init_struct);
    
    /* 设置DQ为高电平 */
    DS18B20_DQ_OUT(1);
    
    /* 复位并检查DS18B20是否存在 */
    ds18b20_reset();
    return ds18b20_check();
}

/**
 * @brief       从DS18B20读取温度值(精度：0.1°C)
 * @param       无
 * @retval      温度值 (范围：-550~1250，实际温度值 = 返回值 / 10)
 * @note        返回的温度值放大了10倍，实际使用时要除以10才是实际温度
 */
short ds18b20_get_temperature(void)
{
    uint8_t flag = 1;           /* 默认温度为正数 */
    uint8_t TL, TH;
    short temp;
    
    /* 开始温度转换 */
    ds18b20_start();
    TEM_delay_us(750);              /* 等待转换完成，12位分辨率需750us */
    
    /* 读取温度值 */
    ds18b20_reset();
    ds18b20_check();
    ds18b20_write_byte(0xCC);   /* 跳过ROM */
    ds18b20_write_byte(0xBE);   /* 读取暂存器 */
    
    TL = ds18b20_read_byte();   /* LSB - 低字节 */
    TH = ds18b20_read_byte();   /* MSB - 高字节 */
    
    /* 判断温度正负[1](@ref) */
    if(TH > 7)
    {
        /* 温度为负 */
        TH = ~TH;
        TL = ~TL;
        flag = 0;
    }
    
    /* 组合温度值 */
    temp = TH;
    temp <<= 8;
    temp += TL;
    
    /* 转换为实际温度值（放大10倍） */
    if(flag == 0)
    {
        /* 负温度 */
        temp = (double)(temp + 1) * 0.625;
        temp = -temp;
    }
    else
    {
        /* 正温度 */
        temp = (double)temp * 0.625;
    }
    
    return temp;
}

/**
 * @brief       获取浮点型温度值
 * @param       无
 * @retval      实际温度值(单位：°C)
 */
float ds18b20_get_temperature_float(void)
{
    short temp_int = ds18b20_get_temperature();
    return (float)temp_int / 10.0f;
}




	/* 初始化DS18B20 */
void DS18B20_init(void)
{
	if(ds18b20_init() == 0)
	{
			printf("DS18B20初始化成功\r\n");
	}
	else
	{
			printf("未检测到DS18B20\r\n");
	}
}



















