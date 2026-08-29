// tm1637.c
#include "tm1637.h"
#include "main.h" // 用于 HAL_Delay

// TM1637 命令定义
#define TM1637_CMD_SET_DATA  0x40 // 数据命令模式
#define TM1637_CMD_SET_ADDR  0xC0 // 地址命令模式
#define TM1637_CMD_DISPLAY   0x88 // 显示控制命令起始

// 共阴极数码管段码 (0-9, A-F, 空白)
static const uint8_t digit_codes_common_cathode[17] = {
			0x3F, // 0
			0x06, // 1
			0x5B, // 2
			0x4F, // 3
			0x66, // 4
			0x6D, // 5
			0x7D, // 6
			0x07, // 7
			0x7F, // 8
			0x6F, // 9
			0x77, // A
			0x7C, // b
			0x39, // C
			0x5E, // d
			0x79, // E
			0x71, // F
			0x00  // 空白
};



// 内部函数声明 (私有)
static void TM1637_Start(void);
static void TM1637_Stop(void);
static void TM1637_WriteByte(uint8_t byte);
static void delay_us(uint32_t us);

/**
  * @brief  微秒级延时 (基于 DWT 周期计数器)
  * @param  us: 延时的微秒数
  */
static void delay_us(uint32_t us)
{
  // 首次调用时使能 DWT 周期计数器
  if ((CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk) == 0U)
  {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
  }

  uint32_t start = DWT->CYCCNT;
  uint32_t ticks = us * (SystemCoreClock / 1000000UL);
  while ((DWT->CYCCNT - start) < ticks)
  {
    __NOP();
  }
}

/**
  * @brief  TM1637 起始信号
  */
static void TM1637_Start(void)
{
  HAL_GPIO_WritePin(TM1637_CLK_PORT, TM1637_CLK_PIN, GPIO_PIN_SET);
  HAL_GPIO_WritePin(TM1637_DIO_PORT, TM1637_DIO_PIN, GPIO_PIN_SET);
  HAL_Delay(1);
  HAL_GPIO_WritePin(TM1637_DIO_PORT, TM1637_DIO_PIN, GPIO_PIN_RESET);
  HAL_Delay(1);
  HAL_GPIO_WritePin(TM1637_CLK_PORT, TM1637_CLK_PIN, GPIO_PIN_RESET);
  HAL_Delay(1);
}

/**
  * @brief  TM1637 停止信号
  */
static void TM1637_Stop(void)
{
  HAL_GPIO_WritePin(TM1637_CLK_PORT, TM1637_CLK_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(TM1637_DIO_PORT, TM1637_DIO_PIN, GPIO_PIN_RESET);
  HAL_Delay(1);
  HAL_GPIO_WritePin(TM1637_CLK_PORT, TM1637_CLK_PIN, GPIO_PIN_SET);
  HAL_Delay(1);
  HAL_GPIO_WritePin(TM1637_DIO_PORT, TM1637_DIO_PIN, GPIO_PIN_SET);
  HAL_Delay(1);
}

/**
  * @brief  TM1637 写入一个字节
  * @param  byte: 要写入的字节
  */
static void TM1637_WriteByte(uint8_t byte)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (byte & 0x01)
      HAL_GPIO_WritePin(TM1637_DIO_PORT, TM1637_DIO_PIN, GPIO_PIN_SET);
    else
      HAL_GPIO_WritePin(TM1637_DIO_PORT, TM1637_DIO_PIN, GPIO_PIN_RESET);

    delay_us(1);
    HAL_GPIO_WritePin(TM1637_CLK_PORT, TM1637_CLK_PIN, GPIO_PIN_SET);
    delay_us(1);
    HAL_GPIO_WritePin(TM1637_CLK_PORT, TM1637_CLK_PIN, GPIO_PIN_RESET);
    delay_us(1);
    byte >>= 1;

  }

  // 忽略 ACK
  HAL_GPIO_WritePin(TM1637_CLK_PORT, TM1637_CLK_PIN, GPIO_PIN_SET);
  HAL_Delay(1);
  HAL_GPIO_WritePin(TM1637_CLK_PORT, TM1637_CLK_PIN, GPIO_PIN_RESET);
  HAL_Delay(1);
}

/**
  * @brief  初始化 TM1637
  */
void TM1637_Init(void)
{
  // GPIO 已由 CubeMX 的 MX_GPIO_Init 配置，这里只设置初始状态
  HAL_GPIO_WritePin(TM1637_CLK_PORT, TM1637_CLK_PIN, GPIO_PIN_SET);
  HAL_GPIO_WritePin(TM1637_DIO_PORT, TM1637_DIO_PIN, GPIO_PIN_SET);

  // 发送显示控制命令 (开启显示, 最暗)
  TM1637_Start();
  TM1637_WriteByte(TM1637_CMD_DISPLAY | TM1637_BRIGHTNESS_7);
  TM1637_Stop();
}

/**
  * @brief  设置显示亮度
  * @param  brightness: 亮度等级
  */
void TM1637_SetBrightness(TM1637_Brightness_t brightness)
{
  TM1637_Start();
  TM1637_WriteByte(TM1637_CMD_DISPLAY | brightness);
  TM1637_Stop();
}

/**
  * @brief  显示四位数字
  * @param  num1, num2, num3, num4: 要显示的数字 (0-9)
  */
void TM1637_DisplayDecimal(uint8_t num1, uint8_t num2, uint8_t num3, uint8_t num4)
{
  if (num1 > 9) num1 = 9;
  if (num2 > 9) num2 = 9;
  if (num3 > 9) num3 = 9;
  if (num4 > 9) num4 = 9;

  TM1637_Start();
  TM1637_WriteByte(TM1637_CMD_SET_DATA);
  TM1637_Stop();

  TM1637_Start();
  TM1637_WriteByte(TM1637_CMD_SET_ADDR);
  TM1637_WriteByte(digit_codes_common_cathode[num1]);
  TM1637_WriteByte(digit_codes_common_cathode[num2]);
  TM1637_WriteByte(digit_codes_common_cathode[num3]);
  TM1637_WriteByte(digit_codes_common_cathode[num4]);
  TM1637_Stop();
}

/**
  * @brief  在指定位置显示原始段码
  * @param  pos: 位置 (0-5)
  * @param  data: 段码数据
  */
void TM1637_DisplayRaw(uint8_t pos, uint8_t data)
{
  if (pos > 3) return;

  TM1637_Start();
  TM1637_WriteByte(TM1637_CMD_SET_ADDR | pos);
  TM1637_WriteByte(data);
  TM1637_Stop();
}

/**
  * @brief  清除显示 (全空白)
  */
void TM1637_Clear(void)
{
  TM1637_Start();
  TM1637_WriteByte(TM1637_CMD_SET_DATA);
  TM1637_Stop();

  TM1637_Start();
  TM1637_WriteByte(TM1637_CMD_SET_ADDR);
  for (uint8_t i = 0; i < 4; i++) {
    TM1637_WriteByte(0x00); // 共阴极空白
  }
  TM1637_Stop();
}

/**
  * @brief  在指定位置显示一个数字 (可控制小数点)
  * @param  pos: 位置 (0-5)
  * @param  digit: 数字 (0-9)
  * @param  dot_on: 是否点亮小数点 (0=不亮, 1=亮)
  */
void TM1637_SetDigit(uint8_t pos, uint8_t digit, uint8_t dot_on)
{
  if (pos > 3) return;
  if (digit > 9) digit = 9;

  uint8_t code = digit_codes_common_cathode[digit];
  if (dot_on) {
    code |= 0x80; // 共阴极: 小数点/冒号位为 1 时点亮
  } else {
    code &= ~0x80; // 共阴极: 熄灭小数点/冒号
  }

  TM1637_DisplayRaw(pos, code);
}
