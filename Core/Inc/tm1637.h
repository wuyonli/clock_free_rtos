// tm1637.h
#ifndef __TM1637_H
#define __TM1637_H

#include "main.h" // 引脚宏由 CubeMX 生成（TM1637_CLK_Pin / TM1637_CLK_GPIO_Port 等）

// --- 用户配置区 ---
// 引脚宏映射到 CubeMX 生成的 main.h 宏（引脚在 CubeMX 里统一配置）
#define TM1637_CLK_PORT   TM1637_CLK_GPIO_Port
#define TM1637_CLK_PIN    TM1637_CLK_Pin
#define TM1637_DIO_PORT   TM1637_DIO_GPIO_Port
#define TM1637_DIO_PIN    TM1637_DIO_Pin
// --- 配置结束 ---

// 亮度等级定义 (0-7, 0 最暗, 7 最亮)
typedef enum {
    TM1637_BRIGHTNESS_0 = 0,
    TM1637_BRIGHTNESS_1,
    TM1637_BRIGHTNESS_2,
    TM1637_BRIGHTNESS_3,
    TM1637_BRIGHTNESS_4,
    TM1637_BRIGHTNESS_5,
    TM1637_BRIGHTNESS_6,
    TM1637_BRIGHTNESS_7
} TM1637_Brightness_t;

// 函数声明
void TM1637_Init(void);
void TM1637_SetBrightness(TM1637_Brightness_t brightness);
void TM1637_DisplayDecimal(uint8_t num1, uint8_t num2, uint8_t num3, uint8_t num4);
void TM1637_DisplayRaw(uint8_t pos, uint8_t data);
void TM1637_Clear(void);
void TM1637_SetDigit(uint8_t pos, uint8_t digit, uint8_t dot_on);

#endif /* __TM1637_H */
