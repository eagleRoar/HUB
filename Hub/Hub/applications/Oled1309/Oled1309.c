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
#ifdef __cplusplus
extern "C" {
#endif

#include <rtthread.h>
#include <u8g2_port.h>

#include "Oled1309.h"
#include "Uart.h"



#define  GO_RIGHT  1
#define  GO_LEFT   2

u8g2_t uiShow;

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
    u8g2_Setup_ssd1309_128x64_el2_1(&uiShow, U8G2_R0, u8x8_byte_8bit_8080mode, u8x8_rt_gpio_and_delay);
    u8x8_SetPin_8Bit_8080(u8g2_GetU8x8(&uiShow),
    SSD1309_8080_PIN_D0, SSD1309_8080_PIN_D1,
    SSD1309_8080_PIN_D2, SSD1309_8080_PIN_D3,
    SSD1309_8080_PIN_D4, SSD1309_8080_PIN_D5,
    SSD1309_8080_PIN_D6, SSD1309_8080_PIN_D7,
    SSD1309_8080_PIN_EN, SSD1309_8080_PIN_CS,
    SSD1309_8080_PIN_DC, SSD1309_8080_PIN_RST);

    u8g2_InitDisplay(&uiShow);
    u8g2_SetPowerSave(&uiShow, 0);

    // Draw Graphics
    /* full buffer example, setup procedure ends in _f */
    u8g2_ClearBuffer(&uiShow);
    u8g2_SetFont(&uiShow, u8g2_font_6x12_tf);
    u8g2_DrawStr(&uiShow, 1, 18, "BBL");
    u8g2_SendBuffer(&uiShow);
//
//    u8g2_SetFont(&uiShow, u8g2_font_unifont_t_symbols);
//    u8g2_DrawGlyph(&uiShow, /*100*/1, 56, 0x2603);
//    u8g2_SendBuffer(&uiShow);
}

void OledTaskEntry(void* parameter)
{
    char data[2];
    type_module_t  module;
    static u8 Timer1sTouch      = OFF;
    static u16 time1S = 0;

    oledInit();

    while(1)
    {
        time1S = TimerTask(&time1S, 20, &Timer1sTouch);

        if(ON == Timer1sTouch)
        {
            if(0 < GetMonitor()->monitorDeviceTable.deviceManageLength)
            {
                for(int i = 0; i < GetMonitor()->monitorDeviceTable.deviceManageLength; i++)
                {
                    if(SENSOR_TYPE == GetMonitor()->monitorDeviceTable.deviceTable[i].s_or_d)
                    {
                        module = GetMonitor()->monitorDeviceTable.deviceTable[i];

                        u8g2_ClearBuffer(&uiShow);
                        u8g2_SetFont(&uiShow, u8g2_font_6x12_tf);

                        itoa(module.module_t[0].value, data, 10);
                        u8g2_DrawStr(&uiShow, 1, 12, module.module_t[0].name);
                        u8g2_DrawStr(&uiShow, 80, 12, data);

                        itoa(module.module_t[1].value, data, 10);
                        u8g2_DrawStr(&uiShow, 1, 22, module.module_t[1].name);
                        u8g2_DrawStr(&uiShow, 80, 22, data);

                        itoa(module.module_t[2].value, data, 10);
                        u8g2_DrawStr(&uiShow, 1, 32, module.module_t[2].name);
                        u8g2_DrawStr(&uiShow, 80, 32, data);

                        itoa(module.module_t[3].value, data, 10);
                        u8g2_DrawStr(&uiShow, 1, 42, module.module_t[3].name);
                        u8g2_DrawStr(&uiShow, 80, 42, data);

                        u8g2_SendBuffer(&uiShow);
                    }
                }
            }
        }

        rt_thread_mdelay(50);
    }
}

void OledTaskInit(void)
{
    rt_err_t threadStart = RT_NULL;

    /* 创建串口 线程 */
    rt_thread_t thread = rt_thread_create("oled task", OledTaskEntry, RT_NULL, 1024, OLED_PRIORITY, 10);

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

#ifdef __cplusplus
}
#endif

