/*
 * led.h — 8 路 LED 驱动（跑马灯实验）
 * 位序 bit i 对应 LEDi，全部低电平点亮：
 *   LED0  PB1  板载 DS0（红）
 *   LED1  PB0  板载 DS1（绿）
 *   LED2  PB6  外接（面包板：负极→IO，正极→1KΩ→3V3）
 *   LED3  PB7  外接
 *   LED4  PB8  外接
 *   LED5  PB9  外接
 *   LED6  PD3  外接
 *   LED7  PA4  外接
 */
#ifndef BSP_LED_LED_H
#define BSP_LED_LED_H

#include "stm32f4xx_hal.h"

void    led_init(void);
void    led_write(uint8_t pattern);   /* bit i = 1 → 点亮 LEDi */
uint8_t led_mask(uint8_t idx);        /* 返回第 idx 个 LED 的掩码（1<<idx） */

#endif
