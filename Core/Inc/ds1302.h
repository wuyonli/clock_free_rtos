// ds1302.h
#ifndef __DS1302_H
#define __DS1302_H

#include "main.h"  // 包含 stm32f1xx_hal.h 和 CubeMX 生成的 GPIO 定义

// 引脚定义（映射到 CubeMX 生成的 main.h 宏）
#define DS1302_CLK_PORT    DS1302_CLK_GPIO_Port
#define DS1302_CLK_PIN     DS1302_CLK_Pin

#define DS1302_DAT_PORT    DS1302_DAT_GPIO_Port
#define DS1302_DAT_PIN     DS1302_DAT_Pin

#define DS1302_RST_PORT    DS1302_RST_GPIO_Port
#define DS1302_RST_PIN     DS1302_RST_Pin

// 基本操作宏
#define DS1302_CLK_HIGH()   HAL_GPIO_WritePin(DS1302_CLK_PORT, DS1302_CLK_PIN, GPIO_PIN_SET)
#define DS1302_CLK_LOW()    HAL_GPIO_WritePin(DS1302_CLK_PORT, DS1302_CLK_PIN, GPIO_PIN_RESET)
#define DS1302_RST_HIGH()   HAL_GPIO_WritePin(DS1302_RST_PORT, DS1302_RST_PIN, GPIO_PIN_SET)
#define DS1302_RST_LOW()    HAL_GPIO_WritePin(DS1302_RST_PORT, DS1302_RST_PIN, GPIO_PIN_RESET)
#define DS1302_DAT_HIGH()   HAL_GPIO_WritePin(DS1302_DAT_PORT, DS1302_DAT_PIN, GPIO_PIN_SET)
#define DS1302_DAT_LOW()    HAL_GPIO_WritePin(DS1302_DAT_PORT, DS1302_DAT_PIN, GPIO_PIN_RESET)
#define DS1302_DAT_READ()   HAL_GPIO_ReadPin(DS1302_DAT_PORT, DS1302_DAT_PIN)

// 方向切换函数（在 .c 中实现）
void DS1302_DAT_Output(void);
void DS1302_DAT_Input(void);

// DS1302 寄存器地址（写/读）
#define DS1302_SEC_W        0x80
#define DS1302_MIN_W        0x82
#define DS1302_HOUR_W       0x84
#define DS1302_DATE_W       0x86
#define DS1302_MONTH_W      0x88
#define DS1302_DAY_W        0x8A
#define DS1302_YEAR_W       0x8C
#define DS1302_WP_W         0x8E  // 写保护
#define DS1302_BURST_W      0xBE  // 突发写（时钟+RAM）

#define DS1302_SEC_R        0x81
#define DS1302_MIN_R        0x83
#define DS1302_HOUR_R       0x85
#define DS1302_DATE_R       0x87
#define DS1302_MONTH_R      0x89
#define DS1302_DAY_R        0x8B
#define DS1302_YEAR_R       0x8D
#define DS1302_BURST_R      0xBF  // 突发读

// 用户数据结构
typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t date;
    uint8_t month;
    uint8_t year;   // 0-99 表示 2000-2099
    uint8_t day;    // 1=Sunday, 2=Monday, ..., 7=Saturday
} DS1302_Time_t;

// 函数声明
void DS1302_Init(void);
uint8_t DS1302_ReadByte(uint8_t addr);
void DS1302_WriteByte(uint8_t addr, uint8_t data);
void DS1302_GetTime(DS1302_Time_t* time);
void DS1302_SetTime(DS1302_Time_t* time);
uint8_t DS1302_DecToBcd(uint8_t val);
uint8_t DS1302_BcdToDec(uint8_t val);

#endif /* __DS1302_H */
