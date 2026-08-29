// ds1302.c
#include "ds1302.h"
#include "main.h"

// 切换 DAT 为输出模式
void DS1302_DAT_Output(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DS1302_DAT_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DS1302_DAT_PORT, &GPIO_InitStruct);
}

// 切换 DAT 为输入模式
void DS1302_DAT_Input(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DS1302_DAT_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DS1302_DAT_PORT, &GPIO_InitStruct);
}

// 微秒级延时（基于 DWT 周期计数器）
static void DS1302_DelayUs(uint16_t us)
{
    // 首次调用时使能 DWT 周期计数器
    if ((CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk) == 0U)
    {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->CYCCNT = 0U;
        DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
    }

    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = (uint32_t)us * (SystemCoreClock / 1000000UL);
    while ((DWT->CYCCNT - start) < ticks)
    {
        __NOP();
    }
}

// 向 DS1302 写入一个字节
void DS1302_WriteByte(uint8_t addr, uint8_t data)
{
    uint8_t i;

    DS1302_RST_LOW();
    DS1302_CLK_LOW();
    DS1302_DelayUs(4);
    DS1302_RST_HIGH();          // 启动通信

    DS1302_DAT_Output();

    // 发送地址（8位）
    for (i = 0; i < 8; i++) {
        if (addr & 0x01)
            DS1302_DAT_HIGH();
        else
            DS1302_DAT_LOW();
        DS1302_CLK_HIGH();
        DS1302_DelayUs(1);
        DS1302_CLK_LOW();
        addr >>= 1;
    }

    // 发送数据（8位）
    for (i = 0; i < 8; i++) {
        if (data & 0x01)
            DS1302_DAT_HIGH();
        else
            DS1302_DAT_LOW();
        DS1302_CLK_HIGH();
        DS1302_DelayUs(1);
        DS1302_CLK_LOW();
        data >>= 1;
    }

    DS1302_RST_LOW();           // 结束通信
}

// 从 DS1302 读取一个字节
uint8_t DS1302_ReadByte(uint8_t addr)
{
    uint8_t i, data = 0;

    DS1302_RST_LOW();
    DS1302_CLK_LOW();
    DS1302_DelayUs(4);
    DS1302_RST_HIGH();

    DS1302_DAT_Output();

    // 发送地址（8位）
    for (i = 0; i < 8; i++) {
        if (addr & 0x01)
            DS1302_DAT_HIGH();
        else
            DS1302_DAT_LOW();
        DS1302_CLK_HIGH();
        DS1302_DelayUs(1);
        DS1302_CLK_LOW();
        addr >>= 1;
    }

    DS1302_DAT_Input(); // 切换为输入

    // 读取数据（8位，低位先传）
    for (i = 0; i < 8; i++) {
        data >>= 1;
        if (DS1302_DAT_READ())
            data |= 0x80;
        DS1302_CLK_HIGH();
        DS1302_DelayUs(1);
        DS1302_CLK_LOW();
    }

    DS1302_RST_LOW();
    return data;
}

// BCD 转十进制
uint8_t DS1302_BcdToDec(uint8_t bcd)
{
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

// 十进制转 BCD
uint8_t DS1302_DecToBcd(uint8_t dec)
{
    return ((dec / 10) << 4) | (dec % 10);
}

// 获取当前时间
void DS1302_GetTime(DS1302_Time_t* time)
{
    time->second = DS1302_BcdToDec(DS1302_ReadByte(DS1302_SEC_R) & 0x7F);
    time->minute = DS1302_BcdToDec(DS1302_ReadByte(DS1302_MIN_R));
    time->hour   = DS1302_BcdToDec(DS1302_ReadByte(DS1302_HOUR_R) & 0x3F);
    time->date   = DS1302_BcdToDec(DS1302_ReadByte(DS1302_DATE_R));
    time->month  = DS1302_BcdToDec(DS1302_ReadByte(DS1302_MONTH_R));
    time->year   = DS1302_BcdToDec(DS1302_ReadByte(DS1302_YEAR_R));
    time->day    = DS1302_ReadByte(DS1302_DAY_R) & 0x07;
}

// 设置时间（注意：输入为十进制）
void DS1302_SetTime(DS1302_Time_t* time)
{
    // 先关闭写保护
    DS1302_WriteByte(DS1302_WP_W, 0x00);

    DS1302_WriteByte(DS1302_SEC_W,  DS1302_DecToBcd(time->second));
    DS1302_WriteByte(DS1302_MIN_W,  DS1302_DecToBcd(time->minute));
    DS1302_WriteByte(DS1302_HOUR_W, DS1302_DecToBcd(time->hour));
    DS1302_WriteByte(DS1302_DATE_W, DS1302_DecToBcd(time->date));
    DS1302_WriteByte(DS1302_MONTH_W,DS1302_DecToBcd(time->month));
    DS1302_WriteByte(DS1302_YEAR_W, DS1302_DecToBcd(time->year));
    DS1302_WriteByte(DS1302_DAY_W,  time->day & 0x07);

    // 重新启用写保护（可选）
    DS1302_WriteByte(DS1302_WP_W, 0x80);
}

// GPIO 初始化（CLK/RST/DAT 已在 CubeMX 里配为输出，这里只设置初始电平）
static void DS1302_GPIO_Init(void)
{
    // 初始状态：RST、CLK 拉低
    DS1302_RST_LOW();
    DS1302_CLK_LOW();
}

// 初始化（可选：设置初始时间）
void DS1302_Init(void)
{
    DS1302_GPIO_Init();

    // 若芯片未供电，秒寄存器 Bit7=1，则设置默认时间
    uint8_t sec_reg = DS1302_ReadByte(DS1302_SEC_R);
    if (sec_reg & 0x80) {
        // 首次上电，设置默认时间
        DS1302_Time_t default_time = {0};
        default_time.year = 26;   // 2026
        default_time.month = 8;
        default_time.date = 29;
        default_time.hour = 14;
        default_time.minute = 29;
        default_time.second = 0;
        default_time.day = 7;     // Saturday
        DS1302_SetTime(&default_time);
    }
}
