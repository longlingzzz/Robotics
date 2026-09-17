/*
 * key.h — 4 路独立按键驱动
 *   KEY0   PH3   按下为低（内部上拉）
 *   KEY1   PH2   按下为低（内部上拉）
 *   KEY2   PC13  按下为低（内部上拉）
 *   KEY_UP PA0   按下为高（内部下拉，WKUP 引脚，高电平触发）
 * key_scan() 采用消抖 + 边沿检测：只在“从松开到按下”的瞬间返回一次事件，
 * 长按不会重复触发 —— 满足“每次有效按键只响一声”的要求。
 */
#ifndef BSP_KEY_KEY_H
#define BSP_KEY_KEY_H

#include "stm32f4xx_hal.h"

#define KEY_NONE    0x00
#define KEY0_MSK    0x01
#define KEY1_MSK    0x02
#define KEY2_MSK    0x04
#define KEY_UP_MSK  0x08

void    key_init(void);
uint8_t key_scan(void);   /* 返回本次新按下按键的位掩码，无事件返回 KEY_NONE */

#endif
