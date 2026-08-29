#include "hx1838.h"
#include <stdio.h>  // 确保已包含
#include <string.h>

// htim3 由 CubeMX 在 main.c 中定义并初始化（MX_TIM3_Init 已完成 TIM3 基础配置，
// Prescaler 需在 CubeMX 里设为 71 → 1MHz → 1µs 计数）
extern TIM_HandleTypeDef htim3;


// 状态机
typedef enum {
    IR_IDLE,
    IR_HEADER_LOW,      // 9ms 低电平
    IR_HEADER_HIGH,     // 4.5ms 或 2.25ms 高电平
    IR_RECEIVING,       // 接收 32 位数据
    IR_REPEAT           // 重复码
} IR_State_t;

// 全局状态变量（建议设为 static）
static uint8_t  ir_bit_count = 0;
static uint32_t ir_data      = 0;

// 用户回调函数（在 main.c 中定义）
extern void HX1838_OnReceive(uint8_t addr, uint8_t cmd, uint8_t repeat);

// 启动下降沿捕获（每次捕获后重新启动）
// 启动一次输入捕获（只配置下降沿）
void IR_StartCapture(void)
{
    TIM_IC_InitTypeDef sConfigIC = {0};
    sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_FALLING; // 只用下降沿
    sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
    sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
    sConfigIC.ICFilter = 0x00; // 关闭滤波，避免吃掉窄脉冲
    HAL_TIM_IC_ConfigChannel(&htim3, &sConfigIC, TIM_CHANNEL_3);
    HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_3);
}


// 定时器输入捕获中断回调
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3)
    {
        // 读取捕获值（单位：µs，因 Prescaler=71，72MHz/72=1MHz）
        uint32_t width_us = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_3);
        __HAL_TIM_SET_COUNTER(htim, 0); // 清零计数器，准备下一次捕获

        // === 1. 检测起始帧头：9ms低 + 4.5ms高 ≈ 13500 µs ===
        if (width_us > 13000 && width_us < 14000)
        {
            ir_bit_count = 0;
            ir_data = 0;
        }
        // === 2. 接收数据位（最多32位）===
        else if (ir_bit_count < 32)
        {
            uint8_t bit_val = 0xFF; // 无效标记

            // 判定逻辑0：周期 ≈ 1120 µs（560低 + 560高）
            if (width_us > 1000 && width_us < 1300)
            {
                bit_val = 0;
            }
            // 判定逻辑1：周期 ≈ 2250 µs（560低 + 1690高）
            else if (width_us > 2000 && width_us < 2500)
            {
                bit_val = 1;
            }

            // 如果是有效位，则存入
            if (bit_val != 0xFF)
            {
                ir_data = (ir_data << 1) | bit_val;
                ir_bit_count++;

                // 收满32位，尝试解析
                if (ir_bit_count == 32)
                {
                    uint8_t addr     = (ir_data >> 24) & 0xFF;
                    uint8_t addr_inv = (ir_data >> 16) & 0xFF;
                    uint8_t cmd      = (ir_data >> 8)  & 0xFF;
                    uint8_t cmd_inv  = ir_data & 0xFF;

                    // NEC 校验：地址和命令均满足反码关系
                    if ((addr ^ addr_inv) == 0xFF && (cmd ^ cmd_inv) == 0xFF)
                    {
                        HX1838_OnReceive(addr, cmd, 0); // repeat=0 表示标准帧
                    }
                    // 注意：不重置 ir_bit_count，等下次起始头自动清零
                }
            }
            // 如果是无效脉宽（如 560us、800us 等），直接忽略，不重置状态！
        }

        // 重新启动下一次捕获（保持下降沿）
        IR_StartCapture();
    }
}

// 初始化红外接收（TIM3 基础配置已由 CubeMX 的 MX_TIM3_Init 完成）
void HX1838_Init(void)
{
    IR_StartCapture();
}
