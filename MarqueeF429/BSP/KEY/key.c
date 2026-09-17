/*
 * key.c — 4 路独立按键：读取 + 10ms 消抖 + 按下沿检测
 * 机械按键在按下/松开瞬间有 5~10ms 抖动，检测到电平变化后延时 10ms 再确认，
 * 确认成功才记录新状态；只有“上一次松开、这一次按下”的键才产生事件。
 */
#include "key.h"

typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
    uint8_t       active_high;   /* 1 = 按下为高电平 */
} key_pin_t;

static const key_pin_t keys[4] = {
    {GPIOH, GPIO_PIN_3,  0},   /* KEY0 */
    {GPIOH, GPIO_PIN_2,  0},   /* KEY1 */
    {GPIOC, GPIO_PIN_13, 0},   /* KEY2 */
    {GPIOA, GPIO_PIN_0,  1},   /* KEY_UP（高电平触发） */
};

static uint8_t key_read(void)
{
    uint8_t mask = KEY_NONE;

    for (uint8_t i = 0; i < 4; i++) {
        GPIO_PinState level = HAL_GPIO_ReadPin(keys[i].port, keys[i].pin);
        uint8_t pressed = keys[i].active_high ? (level == GPIO_PIN_SET)
                                              : (level == GPIO_PIN_RESET);
        mask |= (uint8_t)(pressed << i);
    }
    return mask;
}

void key_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    gpio.Mode  = GPIO_MODE_INPUT;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;

    for (uint8_t i = 0; i < 4; i++) {
        gpio.Pin  = keys[i].pin;
        gpio.Pull = keys[i].active_high ? GPIO_PULLDOWN : GPIO_PULLUP;
        HAL_GPIO_Init(keys[i].port, &gpio);
    }
}

uint8_t key_scan(void)
{
    static uint8_t last = KEY_NONE;
    uint8_t cur = key_read();

    if (cur == last) {
        return KEY_NONE;
    }

    HAL_Delay(10);                 /* 消抖：延时后重新确认 */

    uint8_t confirm = key_read();
    if (confirm != cur) {          /* 电平仍在抖动，放弃本次变化 */
        last = confirm;
        return KEY_NONE;
    }

    uint8_t event = (uint8_t)(cur & (uint8_t)~last);  /* 新按下的键才有事件 */
    last = cur;
    return event;
}
