# STM32CubeIDE 完成跑马灯实验 —— 保姆级教程（阿波罗 F429）

> 硬件：正点原子阿波罗 V2 STM32F429（芯片 STM32F429IGT6，晶振 25MHz，主频 180MHz）
> 目标：8 个 LED 实现 4 种特效，4 个按键切换特效，每次有效按键蜂鸣器响一声。
> 参考答案：工作目录 `MarqueeF429/BSP/`（先自己做，卡住了再对照）。

---

## 0. 准备清单

**软件**
- STM32CubeIDE（自带 CubeMX 配置器，官网免费下载，装一次全搞定）

**硬件**
- 阿波罗 F429 开发板 + DAP/ST-Link 仿真器（SWD 四根线：3V3、GND、SWDIO、SWCLK）
- 面包板 + 6 个直插 LED + 6 个 1kΩ 限流电阻 + 杜邦线若干
- 实验箱底板的**面包板电源开关打到 ON**（面包板从开发板取电，电源指示灯亮才算供电正常）

**接线（6 个外接 LED）**

| 编号 | 引脚 | 说明 |
|---|---|---|
| LED0 | **PB1** | 板载 DS0（红色），不用接 |
| LED1 | **PB0** | 板载 DS1（绿色），不用接 |
| LED2 | **PB6** | 外接：LED 短脚(负极)→杜邦线→PB6；长脚(正极)→1kΩ→3V3 排 |
| LED3 | **PB7** | 同上 |
| LED4 | **PB8** | 同上 |
| LED5 | **PB9** | 同上 |
| LED6 | **PD3** | 同上 |
| LED7 | **PA4** | 同上 |

> 这些引脚在不插摄像头/LCD 等模块时都是"完全独立"的（见 IO 分配表 xlsx）。接好 8 个灯后，**板载的 KEY0/KEY1/KEY2/KEY_UP 四个按键直接可用，不用接**。

---

## 1. 新建工程

1. `File → New → STM32 Project`
2. 在 Part Number 框输入 `STM32F429IG`，选中 **STM32F429IGT6**，Next
3. 工程名填 `Marquee`，语言 C，Binary Type 选 Executable，Finish
4. 弹出 "Initialize all peripherals with their default Mode?" → 点 **No**

现在进入了 `.ioc` 图形配置界面（就是 CubeMX）。

## 2. 配置 .ioc（五步）

### 2.1 时钟源与调试口
- 左侧 `System Core → RCC`：**HSE 选 Crystal/Ceramic Resonator**
- `System Core → SYS`：Debug 确认为 **Serial Wire**

### 2.2 时钟树（Clock Configuration 标签页）
在 **HCLK** 框直接输入 `180` 回车，CubeMX 会自动求解，确认结果是：

```
HSE 25MHz → PLLM=/25 → PLLN=×360 → VCO 360MHz → PLLP=/2 → SYSCLK 180MHz
APB1 分频 /4 → 45MHz（定时器时钟 ×2 = 90MHz）
APB2 分频 /2 → 90MHz
```
> 生成代码后可在 `Core/Src/main.c` 的 `SystemClock_Config()` 里核对 PLLM=25、PLLN=360、PLLP=2。
> 另外打开 `Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_conf.h` 搜 `HSE_VALUE`，确认是 `25000000`（CubeF4 默认就是 25M，和板子晶振一致，不用改；**如果这里不对，所有延时都会变快/变慢约 3 倍**）。

### 2.3 GPIO
在右侧芯片图上依次点击引脚，选功能并加标签（右键 → Enter User Label）：

| 引脚 | 功能 | 标签 | 方向/上下拉（下方 GPIO 标签页里设） |
|---|---|---|---|
| PB1 | GPIO_Output | LED0 | 推挽，上拉，初始电平 **High**（灯灭）|
| PB0 | GPIO_Output | LED1 | 同上 |
| PB6/PB7/PB8/PB9 | GPIO_Output | LED2~LED5 | 同上 |
| PD3 | GPIO_Output | LED6 | 同上 |
| PA4 | GPIO_Output | LED7 | 同上 |
| PH3 | GPIO_Input | KEY0 | **上拉**（按下为低）|
| PH2 | GPIO_Input | KEY1 | 上拉 |
| PC13 | GPIO_Input | KEY2 | 上拉 |
| PA0 | GPIO_Input | KEY_UP | **下拉**（按下为高，这脚反极性！）|

### 2.4 I2C3（控制蜂鸣器用）
- `Connectivity → I2C3`：I2C 下拉框选 **I2C**
- 引脚自动分配到 **PH4=SCL、PH5=SDA**（正好是板载 IIC 总线，板上已有 4.7K 上拉）
- 参数保持默认：Standard Mode，100kHz

### 2.5 生成代码
- `Project Manager` 标签页 → 勾选 **Generate peripheral initialization as a pair of '.c/.h' files per peripheral**（每个外设单独一个 .c/.h，结构清爽）
- `Ctrl+S` → Generate Code

## 3. 建立 BSP 目录（仿照老师例程的结构）

在工程管理器里：右键工程 → `New → Source Folder`，名字 `BSP`（**必须是 Source Folder**，IDE 会自动把它加进头文件搜索路径）。
然后在 BSP 下建三个子目录，各放一对文件：

```
BSP/
├── LED/   led.h  led.c     ← 8 路灯驱动
├── KEY/   key.h  key.c     ← 按键扫描
└── BEEP/  beep.h beep.c    ← 蜂鸣器（经 PCF8574）
```

> 如果误建成普通 Folder：`Project → Properties → C/C++ Build → Settings → MCU GCC Compiler → Include paths` 手动加一条 `${workspace_loc:/${ProjName}/BSP}`。

---

## 4. 里程碑 M1：先让 LED0 闪起来

**别急着写全部功能。** 先在 `main.c` 的 `/* USER CODE BEGIN WHILE */` 区域临时写：

```c
HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
HAL_Delay(500);
```

编译（锤子图标）→ 下载（Run），**验收：板载红灯每秒闪一次。**

现象不对就按顺序查：下载器连没连好 → HSE_VALUE 是不是 25M → 极性（低电平亮，输出 High 时应是灭的）。

**✅ M1 完成后拍照留证（报告"实验过程"素材）。**

## 5. LED 模块：8 路封装

核心思想：**引脚参数收进一张表，代码只操作"位图"**——以后加灯、换脚只改表，不动逻辑。

```c
/* led.h */
#ifndef BSP_LED_LED_H
#define BSP_LED_LED_H
#include "stm32f4xx_hal.h"

void    led_init(void);               /* 8 脚全部配成推挽输出、默认全灭 */
void    led_write(uint8_t pattern);   /* bit i = 1 → 点亮 LEDi（低电平亮）*/

#endif
```

```c
/* led.c */
#include "led.h"

typedef struct { GPIO_TypeDef *port; uint16_t pin; } led_pin_t;

static const led_pin_t leds[8] = {
    {GPIOB, GPIO_PIN_1},  /* LED0 板载 */
    {GPIOB, GPIO_PIN_0},  /* LED1 板载 */
    {GPIOB, GPIO_PIN_6},  {GPIOB, GPIO_PIN_7},
    {GPIOB, GPIO_PIN_8},  {GPIOB, GPIO_PIN_9},
    {GPIOD, GPIO_PIN_3},  {GPIOA, GPIO_PIN_4},
};

void led_init(void)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    for (uint8_t i = 0; i < 8; i++) {
        gpio.Pin = leds[i].pin;
        HAL_GPIO_Init(leds[i].port, &gpio);
        HAL_GPIO_WritePin(leds[i].port, leds[i].pin, GPIO_PIN_SET); /* 全灭 */
    }
}

void led_write(uint8_t pattern)
{
    for (uint8_t i = 0; i < 8; i++) {
        HAL_GPIO_WritePin(leds[i].port, leds[i].pin,
            ((pattern >> i) & 0x01) ? GPIO_PIN_RESET : GPIO_PIN_SET);
    }
}
```

**验收：** main 里临时 `led_init(); led_write(0x0F);` → 前 4 个灯亮后 4 个灭。通过后删掉测试代码。

## 6. KEY 模块：消抖 + 只在"按下瞬间"触发一次

三个知识点：
1. **机械抖动**：按下/松开瞬间触点弹跳 5~10ms，电平忽高忽低 → 读到电平变化后**延时 10ms 再读一次确认**。
2. **边沿检测**：保存上一次状态 `last`，只有 `这次按下 且 上次没按下` 才算一次事件 → 长按不会连响。
3. **KEY_UP 反极性**：别的键按下是低（配上拉），它是按下为高（配下拉）。

```c
/* key.h */
#ifndef BSP_KEY_KEY_H
#define BSP_KEY_KEY_H
#include "stm32f4xx_hal.h"

#define KEY_NONE    0x00
#define KEY0_MSK    0x01    /* PH3 */
#define KEY1_MSK    0x02    /* PH2 */
#define KEY2_MSK    0x04    /* PC13 */
#define KEY_UP_MSK  0x08    /* PA0，按下为高 */

void    key_init(void);
uint8_t key_scan(void);         /* 返回"新按下"的键掩码，无事件返回 KEY_NONE */
#endif
```

```c
/* key.c */
#include "key.h"

typedef struct { GPIO_TypeDef *port; uint16_t pin; uint8_t active_high; } key_pin_t;

static const key_pin_t keys[4] = {
    {GPIOH, GPIO_PIN_3,  0},   /* KEY0 */
    {GPIOH, GPIO_PIN_2,  0},   /* KEY1 */
    {GPIOC, GPIO_PIN_13, 0},   /* KEY2 */
    {GPIOA, GPIO_PIN_0,  1},   /* KEY_UP：高电平触发 */
};

static uint8_t key_read(void)   /* 读当前按下状态，按下置 1 */
{
    uint8_t mask = KEY_NONE;
    for (uint8_t i = 0; i < 4; i++) {
        GPIO_PinState lv = HAL_GPIO_ReadPin(keys[i].port, keys[i].pin);
        uint8_t pressed = keys[i].active_high ? (lv == GPIO_PIN_SET)
                                              : (lv == GPIO_PIN_RESET);
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
    gpio.Mode = GPIO_MODE_INPUT;
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
    if (cur == last) return KEY_NONE;   /* 状态没变 */
    HAL_Delay(10);                      /* 消抖 */
    uint8_t confirm = key_read();
    if (confirm != cur) { last = confirm; return KEY_NONE; }  /* 还在抖，放弃 */
    uint8_t event = (uint8_t)(cur & (uint8_t)~last);  /* 新按下的键才有事件 */
    last = cur;
    return event;
}
```

**验收：** main 循环里 `if (key_scan() & KEY0_MSK) HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);` → 按一下 KEY0 翻转一次，长按只翻一次。

## 7. BEEP 模块：I2C3 → PCF8574 → 蜂鸣器

**原理（报告里要写）：** 板载蜂鸣器不是接在 GPIO 上，而是接在 IIC 扩展芯片 **PCF8574 的 P0 脚**。P0 输出 1 → 蜂鸣器响；输出 0 → 停。芯片地址：A0~A2 接地 → 7 位地址 `0b0100000` = **0x20**；HAL 库要求传入 8 位格式即 **0x40**。PCF8574 的写法极简单：一次 IIC 写入 1 个字节，8 个位直接变成 P0~P7 的电平。

⚠️ **大坑：** PCF8574 上电默认所有口为高 → **一上电蜂鸣器就叫**。所以初始化第一件事就是写 `0xF0`（P0=0 关蜂鸣器；P4=1 是 EXIO 输入口的默认态）。

```c
/* beep.h */
#ifndef BSP_BEEP_BEEP_H
#define BSP_BEEP_BEEP_H
#include "stm32f4xx_hal.h"
#include "i2c.h"          /* CubeMX 生成的 hi2c3 在这里 */

void    beep_init(void);
void    beep_once(void);  /* 响约 80ms，自动停 */
#endif
```

```c
/* beep.c */
#include "beep.h"

#define PCF8574_ADDR  0x40      /* HAL 用 8 位地址（7 位 0x20 左移 1 位）*/
#define BEEP_BIT      0         /* 蜂鸣器在 P0 */

static uint8_t pcf_state = 0xF0; /* P0=0 蜂鸣器关，P4=1 保持 EXIO 高 */

static void pcf_write(void)
{
    HAL_I2C_Master_Transmit(&hi2c3, PCF8574_ADDR, &pcf_state, 1, 100);
}

void beep_init(void)
{
    pcf_state = 0xF0;
    pcf_write();                 /* 上电先闭嘴 */
}

void beep_once(void)
{
    pcf_state |=  (1u << BEEP_BIT);  pcf_write();   /* P0=1 响 */
    HAL_Delay(80);
    pcf_state &= ~(1u << BEEP_BIT);  pcf_write();   /* P0=0 停 */
}
```

**验收：** main 里 `beep_init(); ... beep_once();` 放在某个按键事件里 → 上电安静，按键响一声。

> 排错：`HAL_I2C_Master_Transmit` 返回值不是 `HAL_OK` 时，用
> `HAL_I2C_IsDeviceReady(&hi2c3, 0x40, 3, 100)` 确认芯片在位；都不通就查 ioc 里 I2C3 有没有配到 PH4/PH5。

## 8. 四种特效 + 主循环（M2~M4）

**设计思路（这就是报告"方法步骤"里的流程图）：**
特效不写成"一个死循环跑完整轮"的函数，而是写成 **`effect_step(mode)`——每调用一次只走一步**（算出下一步的 pattern → `led_write()` → 延时 200ms）。主循环不停地调它，于是**每一轮都能腾出机会扫描按键**，按键一按立刻切换，这就是非阻塞式状态机。

```c
/* ---------- effects.c / effects.h（自己建） ---------- */
#include "effects.h"

#define STEP_MS 200

static uint8_t step;               /* 每种特效内部的步进计数 */

void effect_reset(void) { step = 0; }

/* 返回当前步应显示的 pattern，然后 step 前进 */
static uint8_t next_pattern(uint8_t mode)
{
    uint8_t p = 0;
    switch (mode) {
    case 0:                       /* 示例：顺序流水 0000_0001 → 1000_0000 循环 */
        p = (uint8_t)(1u << step);
        if (++step >= 8) step = 0;
        break;
    case 1:                       /* TODO 你来写：来回扫描（乒乓球灯）*/
        break;
    case 2:                       /* TODO 你来写：逐个累加点亮，全亮后全灭重来 */
        break;
    case 3:                       /* TODO 你来写：奇偶交替闪 */
        break;
    }
    return p;
}

void effect_step(uint8_t mode)
{
    led_write(next_pattern(mode));
    HAL_Delay(STEP_MS);
}
```

**三种待实现特效的规格（照着写，每人可微调做出差异）：**

| 模式 | 名称 | 视觉效果 | 实现提示 |
|---|---|---|---|
| 1 | 来回扫描 | 单灯 0→7 走过去，再 7→0 走回来，往复 | 维护一个方向变量 `dir`；`step += dir`，到头反向 |
| 2 | 逐个累加 | `0x01,0x03,0x07…0xFF` 越点越多，全亮后全灭重来 | `p = (1u << (step+1)) - 1`；step 到 8 归零 |
| 3 | 奇偶对闪 | `0x55(奇数灯亮) ↔ 0xAA(偶数灯亮)` 来回切 | `p = (step & 1) ? 0xAA : 0x55; step ^= 1;` |

**main.c 主循环（USER CODE 区域）：**

```c
  /* USER CODE BEGIN 2 */
  led_init();
  key_init();
  beep_init();
  effect_reset();
  /* USER CODE END 2 */

  /* USER CODE BEGIN WHILE */
  while (1)
  {
      uint8_t ev = key_scan();
      if (ev) {                          /* 有新按下的键 */
          if (ev & KEY0_MSK)   mode = 0;
          if (ev & KEY1_MSK)   mode = 1;
          if (ev & KEY2_MSK)   mode = 2;
          if (ev & KEY_UP_MSK) mode = 3;
          beep_once();                   /* 每次有效按键只响一声 */
          led_write(0);                  /* 清屏，特效从零开始 */
          effect_reset();
      }
      effect_step(mode);                 /* 走一步（内部延时 200ms）*/
  }
  /* USER CODE END WHILE */
```

`mode` 变量定义在 USER CODE VARIABLES 区：`static uint8_t mode = 0;`（或全局）。

**验收清单：**
- ✅ M2：不按键时，默认特效 0 流水正常；
- ✅ M3：KEY0~KEY_UP 分别切 4 种特效，**长按只切一次**；
- ✅ M4：每次切换"嘀"一声，不连响；特效稳定切换无残留。

**✅ 每种特效拍一张照片/短视频（报告"实验结论"素材）。**

## 9. 选做：呼吸灯（PWM）

> 注意 PB6 会被 TIM4 占用，和特效流水冲突。**建议单独验证**：临时改一个测试 main，或把呼吸灯做在 PB6 上、验证完恢复。
> 报告里一句话说明"验证方式为独立测试程序"即可，不影响主实验。

1. ioc 里 `Timers → TIM4 → Channel1 = PWM Generation CH1`（引脚自动到 PB6）
2. 参数：Prescaler `90-1`（90MHz 定时器时钟 → 1MHz 计数），Counter Period `1000-1`（→ 1kHz PWM），Pulse 初始 `0`
3. 生成代码后，渐亮渐暗循环：

```c
HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
while (1) {
    for (uint16_t d = 0; d < 1000; d += 5) {   /* 渐亮：占空比 0→100% */
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, d);
        HAL_Delay(2);
    }
    for (uint16_t d = 1000; d > 0; d -= 5) {   /* 渐暗 */
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, d);
        HAL_Delay(2);
    }
}
```

原理：1kHz 周期不变，CCR 从 0 涨到 1000，LED 平均电流线性增大 → 亮度渐变（报告里写清"频率×占空比→平均功率"这条因果链）。

## 10. 常见坑速查

| 现象 | 原因 |
|---|---|
| 延时不准（快/慢约 3 倍） | HSE_VALUE 不是 25MHz，或时钟树没配 180MHz |
| 一上电蜂鸣器就叫 | PCF8574 上电默认全高，`beep_init()` 里先写 0xF0 |
| I2C 发送失败 | 地址用了 0x20（7 位格式），HAL 要 8 位格式 0x40 |
| 外接 LED 不亮 | 极性接反（长脚=正极接 3V3 侧）/ 没串 1kΩ / 面包板电源开关没开 |
| 长按蜂鸣器连响 | 只做了电平判断，没做边沿检测 |
| KEY_UP 按了没反应 | 它是高电平触发，Pull 必须配 **Down** |
| 编译报 undefined `hi2c3` | 忘了 `#include "i2c.h"` |
| 编译找不到 led.h | BSP 建成了普通 Folder，见第 3 节加 include path |

## 11. 素材 → 报告对照表（做完顺手攒齐）

| 报告段落 | 用什么素材 |
|---|---|
| 二、方法、步骤 | 主循环状态机流程图（第 8 节的思路画成图）；模块划分图 led/key/beep/effects；按键消抖时序示意；PCF8574→蜂鸣器的电路链路说明 |
| 三、实验过程及内容 | CubeMX 时钟树截图、GPIO 配置表截图、各模块代码 + 逐段讲解（模板要求"越详细越好"）|
| 四、实验结论 | 4 种特效的照片、M1~M4 每个验收点的现象描述、遇到的问题与解决（第 10 节的坑挑 2~3 个写）、改进想法（如：把 STEP_MS 延时改成 SysTick 非阻塞，进一步提高按键响应）|
| 五、实验体会 | 自己写：建议从"GPIO 输入输出/消抖/IIC 总线三个知识点的理解变化"切入 |

---

**建议节奏：** 第 1~4 节一次性做完（30 分钟，先看到灯闪），第 5~7 节每做完一个模块就上板验收，最后写特效。每完成一个里程碑就拍素材，别等全部做完再补。
