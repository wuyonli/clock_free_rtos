#ifndef __HX1838_H
#define __HX1838_H

#include "main.h"


#define IR_KEY_1    0xA2
#define IR_KEY_2    0x62
#define IR_KEY_3    0xE2
#define IR_KEY_4    0x22
#define IR_KEY_5    0x02
#define IR_KEY_6    0xC2
#define IR_KEY_7    0xE0
#define IR_KEY_8    0xA8
#define IR_KEY_9    0x90
#define IR_KEY_0    0x98

/**
 * @brief 红外遥控数据接收完成回调
 * @param addr   地址码（0~255）
 * @param cmd    命令码（0~255）
 * @param repeat 是否为重复码（1=是，0=正常帧）
 */
void HX1838_OnReceive(uint8_t addr, uint8_t cmd, uint8_t repeat);

/**
 * @brief 初始化红外接收功能（基于 TIM3_CH3 + PB0）
 */
void HX1838_Init(void);

#endif
