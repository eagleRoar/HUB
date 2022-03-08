/*
#include <Oled1309.h>
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-01-21     Administrator       the first version
 */

#include <rtthread.h>
#include <u8g2_port.h>

#include "Oled1309.h"

#define  GO_RIGHT  1
#define  GO_LEFT   2

u8g2_t u8g2_el2;

#define SSD1309_8080_PIN_D0                    64  // PE0
#define SSD1309_8080_PIN_D1                    65  // PE1
#define SSD1309_8080_PIN_D2                    66  // PE2
#define SSD1309_8080_PIN_D3                    67  // PE3
#define SSD1309_8080_PIN_D4                    68  // PE4
#define SSD1309_8080_PIN_D5                    69  // PE5
#define SSD1309_8080_PIN_D6                    70  // PE6
#define SSD1309_8080_PIN_D7                    71  // PE7
#define SSD1309_8080_PIN_EN                    74  // PE10
#define SSD1309_8080_PIN_CS                    72  // PE8
#define SSD1309_8080_PIN_DC                    73  // PE9
#define SSD1309_8080_PIN_RST                   76  // PE12

void u8x8_SetPin_8Bit_8080(u8x8_t *u8x8, uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4, uint8_t d5,
        uint8_t d6, uint8_t d7, uint8_t wr, uint8_t cs, uint8_t dc, uint8_t reset)
{
    u8x8_SetPin(u8x8, U8X8_PIN_D0, d0);
    u8x8_SetPin(u8x8, U8X8_PIN_D1, d1);
    u8x8_SetPin(u8x8, U8X8_PIN_D2, d2);
    u8x8_SetPin(u8x8, U8X8_PIN_D3, d3);
    u8x8_SetPin(u8x8, U8X8_PIN_D4, d4);
    u8x8_SetPin(u8x8, U8X8_PIN_D5, d5);
    u8x8_SetPin(u8x8, U8X8_PIN_D6, d6);
    u8x8_SetPin(u8x8, U8X8_PIN_D7, d7);
    u8x8_SetPin(u8x8, U8X8_PIN_E, wr);
    u8x8_SetPin(u8x8, U8X8_PIN_CS, cs);
    u8x8_SetPin(u8x8, U8X8_PIN_DC, dc);
    u8x8_SetPin(u8x8, U8X8_PIN_RESET, reset);
}

//static void st7920_12864_8080_example(int argc, char *argv[])
void oledInit(void)
{
    // Initialization
    u8g2_Setup_ssd1309_128x64_el2_1(&u8g2_el2, U8G2_R0, u8x8_byte_8bit_8080mode, u8x8_rt_gpio_and_delay);
    u8x8_SetPin_8Bit_8080(u8g2_GetU8x8(&u8g2_el2),
    SSD1309_8080_PIN_D0, SSD1309_8080_PIN_D1,
    SSD1309_8080_PIN_D2, SSD1309_8080_PIN_D3,
    SSD1309_8080_PIN_D4, SSD1309_8080_PIN_D5,
    SSD1309_8080_PIN_D6, SSD1309_8080_PIN_D7,
    SSD1309_8080_PIN_EN, SSD1309_8080_PIN_CS,
    SSD1309_8080_PIN_DC, SSD1309_8080_PIN_RST);

    u8g2_InitDisplay(&u8g2_el2);
    u8g2_SetPowerSave(&u8g2_el2, 0);

    // Draw Graphics
    /* full buffer example, setup procedure ends in _f */
    u8g2_ClearBuffer(&u8g2_el2);
    u8g2_SetFont(&u8g2_el2, u8g2_font_baby_tf);
    u8g2_DrawStr(&u8g2_el2, 1, 18, "U8g2 on RT-Thread");
    u8g2_SendBuffer(&u8g2_el2);

    u8g2_SetFont(&u8g2_el2, u8g2_font_unifont_t_symbols);
    u8g2_DrawGlyph(&u8g2_el2, /*100*/1, 56, 0x2603); //Justin debug
    u8g2_SendBuffer(&u8g2_el2);
}

void OledTaskEntry(void* parameter)
{
    static u8 rightFlag = GO_RIGHT;
    static u32 changeView = 1;

    oledInit();

    while(1)
    {
        /* 图像向右移动，该图像为unicode编码 */
        if(GO_RIGHT == rightFlag)
        {
            if(changeView < 110)
            {
                changeView++;
                u8g2_ClearBuffer(&u8g2_el2);
                u8g2_SetFont(&u8g2_el2, u8g2_font_unifont_t_symbols);
                u8g2_DrawGlyph(&u8g2_el2, changeView, 56, 0x2603); //Justin debug
                u8g2_SendBuffer(&u8g2_el2);
            }
            else
            {
                rightFlag = GO_LEFT;
            }
        }
        else if(GO_LEFT == rightFlag)
        {
            if(changeView > 1)
            {
                changeView--;
                u8g2_ClearBuffer(&u8g2_el2);
                u8g2_SetFont(&u8g2_el2, u8g2_font_unifont_t_symbols);
                u8g2_DrawGlyph(&u8g2_el2, changeView, 56, 0x2603); //Justin debug
                u8g2_SendBuffer(&u8g2_el2);
            }
            else
            {
                rightFlag = GO_RIGHT;
            }
        }

        rt_thread_mdelay(50);
    }
}

void OledTaskInit(void)
{
    rt_err_t threadStart = RT_NULL;

    /* 创建串口 线程 */
    rt_thread_t thread = rt_thread_create("oled task", OledTaskEntry, RT_NULL, 1024, 26, 10);

    /* 如果线程创建成功则开始启动线程，否则提示线程创建失败 */
    if (RT_NULL != thread) {
        threadStart = rt_thread_startup(thread);
        if (RT_EOK != threadStart) {
            LOG_E("sensor task start failed");
        }
    } else {
        LOG_E("sensor task create failed");
    }
}

