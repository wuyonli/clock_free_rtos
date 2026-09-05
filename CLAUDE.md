# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

STM32F103RCT6 桌面电子钟，基于 FreeRTOS 多任务。显示 HH:MM（TM1637 数码管），DS1302 掉电保持，ESP8266 通过串口发时间校时，HX1838 红外遥控调时。工程由 STM32CubeMX 生成，**STM32CubeIDE 原生构建（无 CMakeLists.txt）**。

## 构建与烧录

这个仓库**没有命令行构建**——工具链（arm-none-eabi-gcc）不在 PATH，且工程是 CubeIDE 原生 Gnu Make Builder。Claude 的职责是**编辑业务代码**，编译/烧录/调试由用户在 **STM32CubeIDE** 里完成：

1. 用 STM32CubeIDE 打开根目录的 `.project`
2. Build（Builder 必须是 Gnu Make Builder，非 CMake）
3. ST-Link/SWD 烧录

修改 `clock_free_rtos.ioc` 后需回 CubeMX 点 GENERATE CODE 重新生成，再回 CubeIDE 编译。

## 架构：CubeMX 生成代码 vs 手写业务代码

关键分界——**CubeMX 生成的部分不能直接改，改动会被重新生成覆盖**。所有手写业务代码必须放在 `/* USER CODE BEGIN x */ ... /* USER CODE END x */` 之间：

- **CubeMX 生成（勿手改）**：`main.c` 里的 `MX_GPIO_Init`/`MX_TIM3_Init`/`MX_USART3_UART_Init`、`stm32f1xx_hal_msp.c`、`stm32f1xx_it.c`、`freertos.c` 里 CubeMX 生成的任务函数、`FreeRTOSConfig.h`
- **手写业务（可自由改）**：`tm1637.c/.h`、`ds1302.c/.h`、`hx1838.c/.h` 三个驱动，以及 `main.c` 里 `USER CODE 4` 区的 `StartClockTask`、串口/红外中断回调、`ESP_ParseTime` 等

### FreeRTOS 任务

| 任务 | 函数 | 优先级 | 周期 | 位置 |
|------|------|--------|------|------|
| `LED_Task` | `StartLedTask` | Idle | 500ms 翻转 PB5 | CubeMX 生成在 `freertos.c` |
| `LED2_Task` | `StartLed2Task` | Idle | 250ms 翻转 PB6 | CubeMX 生成在 `freertos.c` |
| `clockTask` | `StartClockTask` | Normal | 100ms 主时钟 | 手写在 `main.c` |

接口是 **CMSIS_V1**（`cmsis_os.h`，`osThreadCreate`/`osDelay`），版本 V10.3.1。

### 数据流：中断 → 任务

中断回调里只做轻量操作，重逻辑在任务里。主时钟任务 `StartClockTask` 每 100ms 轮询：

- **ESP8266 时间**：`HAL_UART_RxCpltCallback`（中断）逐字节攒 `uart_line`，收到 `\n` 置 `uart_line_ready=1`；任务里 `ESP_ParseTime` 解析 `YYYY-MM-DD HH:MM:SS`
- **红外按键**：`HAL_TIM_IC_CaptureCallback`（中断）NEC 解码后回调 `HX1838_OnReceive`，只写 `ir_key` 标志；任务里读取并调时
- **时间源优先级**：ESP 3 秒内有数据用 ESP，否则读 DS1302；红外调时仅在 DS1302 作显示源时生效
- **每日校准**：网络时间到 23:50 时把完整时间写回 DS1302（星期用 `ESP_CalcWeekday` Sakamoto 算法算）

## 关键坑（F1 特有，务必注意）

1. **输入引脚用 `GPIO_MODE_INPUT`，不是 `GPIO_MODE_AF_PP`**。PB0（红外 TIM3_CH3）、PB11（USART3_RX）是输入外设，若配 `AF_PP` 会被拉低、信号进不来。F1 与 F4/F0 不同——F1 的 `AF_PP` 是复用推挽**输出**。

2. **SysTick 被 FreeRTOS 占用**，HAL 时基改用 TIM2（CubeMX 里 Timebase=TIM2）。不要在代码里手动配 SysTick 作 HAL 时基。

3. **PA4（DS1302 DAT）是双向引脚**：CubeMX 配成输出作初始态，驱动里 `DS1302_DAT_Output()/Input()` 运行时切换方向（读前切输入+上拉，读完切回）。

4. **微秒延时用 DWT 周期计数器**（`ds1302.c` 的 `DS1302_DelayUs`），不占用定时器。

5. 引脚宏全部由 CubeMX 生成在 `main.h`（如 `TM1637_CLK_Pin`/`TM1637_CLK_GPIO_Port`），驱动通过 `#include "main.h"` 引用，不要再在驱动里 `__HAL_RCC_GPIOx_CLK_ENABLE` 或 `HAL_GPIO_Init` 重复初始化。

## 引脚速查

PA1/PA2=TM1637 CLK/DIO；PA3/PA4/PA5=DS1302 CLK/DAT/RST；PB0=TIM3_CH3 红外输入；PB11=USART3_RX；PB5/PB6=LED1/LED2。详细外设配置（时钟树 72MHz、中断优先级 USART3/TIM3=5、TIM2/SysTick/PendSV=15 等）见 `README.md` 和 `clock_free_rtos.ioc`。

## 外设配置修改流程

新增/改外设时回 CubeMX 改 `.ioc` 并重新生成，它会自动生成 `MX_XXX_Init`/`HAL_XXX_MspInit`/HAL 驱动源文件——不要在 CubeMX 工程里手动配 HAL 的「三件套」（拷源文件/改 conf.h/改构建列表），那很容易漏。自定义驱动 `.c` 放 `Core/Src` 即被自动编译，无需改任何构建配置。
