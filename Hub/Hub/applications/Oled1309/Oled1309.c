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
#include "OledBusiness.h"
#include "Uart.h"
#include "UartBussiness.h"
#include "ButtonTask.h"

#define  GO_RIGHT  1
#define  GO_LEFT   2

u8              reflash_flag        = OFF;
u8g2_t          uiShow;
type_page_t     pageSelect;
u32             pageInfor       = 0x00000000;   //只支持最多四级目录

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
    u8g2_SetFont(&uiShow, /*u8g2_font_6x12_tf*/u8g2_font_8x13_tf);//修改字体的话需要修改LINE_HIGHT和COLUMN_HIGHT
    u8g2_DrawStr(&uiShow, 1, 18, "BBL");
    u8g2_SendBuffer(&uiShow);
}

void MenuBtnCallBack(u8 type)
{
    if(SHORT_PRESS == type)
    {
        //LOG_D("cur = %d, cur home = %d, cur max = %d",pageSelect.cusor,pageSelect.cusor_home,pageSelect.cusor_max);
        if(pageSelect.cusor_max > 0)
        {
            if(pageSelect.cusor < pageSelect.cusor_max - 1)
            {
                pageSelect.cusor++;
            }
            else
            {
                pageSelect.cusor = pageSelect.cusor_home;
            }
        }
        //提示界面刷新
        reflash_flag = ON;
    }
}

void EnterBtnCallBack(u8 type)
{
    if(SHORT_PRESS == type)
    {
        pageSelect.select = ON;
        //提示界面刷新
        reflash_flag = ON;
    }
    else if(LONG_PRESS == type)
    {
        pageInfor = pageInfor >> 8;
    }
}

void pageSelectSet(u8 show,u8 home, u8 max)
{
    pageSelect.cusor_show = show;
    pageSelect.cusor_home = home;
    pageSelect.cusor = pageSelect.cusor_home;
    pageSelect.cusor_max = max;
}

static void pageSetting(u8 page)
{
    switch (page)
    {
        case HOME_PAGE:

            pageSelectSet(YES, 1, 3);
            break;
        case SENSOR_STATE_PAGE:

            pageSelectSet(NO, 0, 0);
            break;
        case DEVICE_STATE_PAGE:

            pageSelectSet(NO, 0, 0);
            break;
        default:
            break;
    }
}

static void pageProgram(u8 page)
{
    switch (page)
    {
        case HOME_PAGE:
            HomePage(&uiShow, pageSelect);
            if(ON == pageSelect.select)
            {
                if(1 == pageSelect.cusor)
                {
                    pageInfor <<= 8;
                    pageInfor |= SENSOR_STATE_PAGE;
                }
                else if(2 == pageSelect.cusor)
                {
                    pageInfor <<= 8;
                    pageInfor |= DEVICE_STATE_PAGE;
                }
                pageSelect.select = OFF;
            }

            reflash_flag = OFF;
            break;

        case SENSOR_STATE_PAGE:
            SensorStatePage(&uiShow, pageSelect);
            break;
        case DEVICE_STATE_PAGE:
            DeviceStatePage(&uiShow, pageSelect);
            break;
        default:
            break;
    }
}

void OledTaskEntry(void* parameter)
{
                u8              nowPage             = 0;
    static      u8              Timer1sTouch        = OFF;
    static      u16             time1S              = 0;
    static      u8              nowPagePre          = 0xFF;

    oledInit();
    //rtcTest();

    pageInfor <<= 8;
    pageInfor |= HOME_PAGE;

    while(1)
    {
        time1S = TimerTask(&time1S, 20, &Timer1sTouch);

        nowPage = pageInfor & 0x000000FF;

        if(nowPagePre != nowPage)
        {
            //LOG_I("-----------nowPage = %x",nowPage);
            //设置初始光标
            pageSetting(nowPage);

            pageProgram(nowPage);

            nowPagePre = nowPage;
        }
        else
        {
            if(ON == reflash_flag)
            {
                pageProgram(nowPage);
            }
        }

        //1s event
        if(ON == Timer1sTouch)
        {
            if((HOME_PAGE == nowPage) ||
               (SENSOR_STATE_PAGE == nowPage) ||
               (DEVICE_STATE_PAGE == nowPage))
            {
                //需要刷新
                reflash_flag = ON;
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
            LOG_E("oled task start failed");
        }
    } else {
        LOG_E("oled task create failed");
    }
}


#ifdef __cplusplus
}
#endif

