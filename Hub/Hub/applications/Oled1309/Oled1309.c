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
#include "ascii_fonts.h"
#include "ST7567.h"
#include "qrcode.h"

#define  GO_RIGHT  1
#define  GO_LEFT   2

u8              reflash_flag        = OFF;
#if (NEW_OLED == 0)
u8g2_t          uiShow;
#endif
type_page_t     pageSelect;
u32             pageInfor       = 0x00000000;   //只支持最多四级目录
time_t          backlightTime;

__attribute__((section(".ccmbss"))) u8 oled_task[1024];
__attribute__((section(".ccmbss"))) struct rt_thread oled_thread;

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

#define OLED_SPI_PIN_CLK                   SSD1309_8080_PIN_D0
#define OLED_SPI_PIN_MOSI                  SSD1309_8080_PIN_D1
#define OLED_SPI_PIN_RES                   SSD1309_8080_PIN_RST
#define OLED_SPI_PIN_DC                    SSD1309_8080_PIN_DC
#define OLED_SPI_PIN_CS                    SSD1309_8080_PIN_CS
#define OLED_BACK_LIGHT                    SSD1309_8080_PIN_D2

#if (NEW_OLED == 0)
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
    u8g2_SetFont(&uiShow, u8g2_font_8x13_tf);//修改字体的话需要修改LINE_HIGHT和COLUMN_HIGHT
    u8g2_DrawStr(&uiShow, 1, 18, "BBL");
    u8g2_SendBuffer(&uiShow);
}
#else
void clear_screen(void)
{
  ST7567_Fill(0);
  ST7567_UpdateScreen();
}

void st7567Init(void)
{
    ST7567_Init();
    rt_thread_mdelay(100);
    clear_screen();
}

void LCD_Test(void)
{
    ST7567_GotoXY(5, 5);
    ST7567_Puts("ABCDEFGHIJKLMNOPQRSTUVWXYZ", &Font_5x7, 1);
    ST7567_UpdateScreen();
}
#endif

#if (NEW_OLED == 0)
void MenuBtnCallBack(u8 type)
{
    if(SHORT_PRESS == type)
    {
        LOG_D("cur = %d, cur home = %d, cur max = %d",pageSelect.cusor,pageSelect.cusor_home,pageSelect.cusor_max);
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
#else
//唤醒屏幕背景光
void wakeUpOledBackLight(time_t *time)
{
    *time = getTimeStamp();

    rt_pin_write(OLED_BACK_LIGHT, ON);
}

//监控无操作1min 后熄屏幕
void monitorBackLight(time_t time)
{
    if(time + 60  < getTimeStamp())     //getTimeStamp单位S
    {
        rt_pin_write(OLED_BACK_LIGHT, OFF);
    }
}
void EnterBtnCallBack(u8 type)
{
    if(SHORT_PRESS == type)
    {
        //唤醒屏幕
        wakeUpOledBackLight(&backlightTime);
//        clear_screen();
        pageSelect.select = ON;
        //提示界面刷新
        reflash_flag = ON;
    }
    else if(LONG_PRESS == type)
    {
        clear_screen();
        pageInfor = pageInfor >> 8;
        reflash_flag = ON;
    }
}
void UpBtnCallBack(u8 type)
{
    if(SHORT_PRESS == type)
    {
        //唤醒屏幕
        wakeUpOledBackLight(&backlightTime);
//        clear_screen();
        if(pageSelect.cusor > pageSelect.cusor_home)
        {
            pageSelect.cusor--;
        }
        else
        {
            pageSelect.cusor = pageSelect.cusor_max;
        }
        //提示界面刷新
        reflash_flag = ON;
        LOG_D("cusor %d",pageSelect.cusor);//Justin debug
    }
}
void DowmBtnCallBack(u8 type)
{
    if(SHORT_PRESS == type)
    {
        //唤醒屏幕
        wakeUpOledBackLight(&backlightTime);
//        clear_screen();
        if(pageSelect.cusor_max > 0)
        {
            if(pageSelect.cusor < pageSelect.cusor_max)
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

#endif
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

            pageSelectSet(YES, 1, 4);
            break;
        case SENSOR_STATE_PAGE:

            pageSelectSet(NO, 0, 0);
            break;
        case DEVICE_STATE_PAGE:
            pageSelectSet(NO, 0, 0);
            break;
        case QRCODE_PAGE:
            pageSelectSet(NO, 0, 0);
            break;

        case APP_UPDATE_PAGE:
            pageSelectSet(NO, 1, 2);
            break;
        default:
            break;
    }
}

static void pageProgram(u8 page)
{
    static  u8      pagePre        = 0;
    static  u8      cusonPre       = 0;

    if((pagePre != page) || (cusonPre != pageSelect.cusor))
    {
        pagePre = page;
        cusonPre = pageSelect.cusor;
        clear_screen();
    }

    switch (page)
    {
        case HOME_PAGE:
#if (NEW_OLED == 0)
            HomePage(&uiShow, pageSelect);
#else
            HomePage_new(pageSelect, 3);
#endif
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
                else if(3 == pageSelect.cusor)
                {
                    pageInfor <<= 8;
                    pageInfor |= QRCODE_PAGE;
                }
                else if(4 == pageSelect.cusor)
                {
                    pageInfor <<= 8;
                    pageInfor |= APP_UPDATE_PAGE;
                }
                pageSelect.select = OFF;
            }

            break;

        case SENSOR_STATE_PAGE:
//            LOG_D("SENSOR_STATE_PAGE");
#if (NEW_OLED == 0)
            SensorStatePage(&uiShow, pageSelect);
#else
            SensorStatePage_new(GetMonitor());
#endif
            break;
        case DEVICE_STATE_PAGE:
#if (NEW_OLED == 0)
            DeviceStatePage(&uiShow, pageSelect);
#else
            DeviceStatePage_new(GetMonitor());
#endif
            break;

        case QRCODE_PAGE:
#if (NEW_OLED == 1)
//            clear_screen();
            qrcode();
            ST7567_UpdateScreen();
#endif
            break;

        case APP_UPDATE_PAGE:
#if (NEW_OLED == 1)
            UpdateAppProgram(&pageSelect, &pageInfor);
#endif
            break;
        default:
            break;
    }

    reflash_flag = OFF;
}

void OledTaskEntry(void* parameter)
{
                u8              nowPage             = 0;
    static      u8              Timer1sTouch        = OFF;
    static      u16             time1S              = 0;
    static      u8              Timer3sTouch        = OFF;
    static      u16             time3S              = 0;
    static      u8              nowPagePre          = 0xFF;
#if (NO == NEW_OLED)
    oledInit();//Justin debug 仅仅测试
#else
    st7567Init();
//    LCD_Test();
#endif
    pageInfor <<= 8;
    pageInfor |= HOME_PAGE;

    while(1)
    {
        time1S = TimerTask(&time1S, 20, &Timer1sTouch);
        time3S = TimerTask(&time3S, 60, &Timer3sTouch);

        nowPage = pageInfor & 0x000000FF;

        if(nowPagePre != nowPage)
        {
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
            monitorBackLight(backlightTime);
            if(HOME_PAGE == nowPage)
            {
                //需要刷新
                reflash_flag = ON;
            }
        }

        //3s event
        if(ON == Timer3sTouch)
        {
            if((SENSOR_STATE_PAGE == nowPage) ||
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
    if(RT_EOK != rt_thread_init(&oled_thread, OLED_TASK, OledTaskEntry, RT_NULL, oled_task, sizeof(oled_task), OLED_PRIORITY, 10))
    {
        LOG_E("oled thread fail");
    }
    else
    {
        rt_thread_startup(&oled_thread);
        LOG_I("oled thread ok");
    }
}


#ifdef __cplusplus
}
#endif

