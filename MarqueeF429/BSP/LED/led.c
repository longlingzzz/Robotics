/*
 * led.c — 8 路 LED 驱动实现
 * 低电平点亮：写 0（RESET）灯亮，写 1（SET）灯灭。
 * 外接 6 个 LED 的接线见 led.h 注释；引脚变更只需改 leds[] 表。
 */
#include "led.h"

typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
} led_pin_t;

static const led_pin_t leds[8] = {
    {GPIOB, GPIO_PIN_1},   /* LED0 板载 DS0 红 */
    {GPIOB, GPIO_PIN_0},   /* LED1 板载 DS1 绿 */
    {GPIOB, GPIO_PIN_6},   /* LED2 外接 */
    {GPIOB, GPIO_PIN_7},   /* LED3 外接 */
    {GPIOB, GPIO_PIN_8},   /* LED4 外接 */
    {GPIOB, GPIO_PIN_9},   /* LED5 外接 */
    {GPIOD, GPIO_PIN_3},   /* LED6 外接 */
    {GPIOA, GPIO_PIN_4},   /* LED7 外接 */
};

void led_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;

    for (uint8_t i = 0; i < 8; i++) {
        gpio.Pin = leds[i].pin;
        HAL_GPIO_Init(leds[i].port, &gpio);
        HAL_GPIO_WritePin(leds[i].port, leds[i].pin, GPIO_PIN_SET); /* 默认全灭 */
    }
}

void led_write(uint8_t pattern)
{
    for (uint8_t i = 0; i < 8; i++) {
        HAL_GPIO_WritePin(leds[i].port, leds[i].pin,
                          ((pattern >> i) & 0x01) ? GPIO_PIN_RESET : GPIO_PIN_SET);
    }
}

uint8_t led_mask(uint8_t idx)
{
    return (uint8_t)(1u << idx);
}
