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
#include "ButtonTask.h"
#include "ascii_fonts.h"
#include "ST7567.h"
#include "qrcode.h"
#include "Module.h"

#define  GO_RIGHT  1
#define  GO_LEFT   2

u8              reflash_flag        = OFF;
type_page_t     pageSelect;
u64             pageInfor           = 0x00000000;   //只支持最多四级目录
time_t          backlightTime;
u8              factory_mode        = NO;
u8      next_flag   = NO;

__attribute__((section(".ccmbss"))) u8 oled_task[1024*3];
__attribute__((section(".ccmbss"))) struct rt_thread oled_thread;

extern void PhCalibratePage(type_page_t *, ph_cal_t *);
extern void EcCalibratePage(type_page_t *, ec_cal_t *);
extern u32 now_phec_uuid;

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

//判断当前是否是工厂模式
u8 getFactoryMode(void)
{
    return factory_mode;
}

void setFactoryMode(u8 flg)
{
    factory_mode = flg;
}

void clear_screen(void)
{
  ST7567_Fill(0);
//  ST7567_UpdateScreen();
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
    u8      info    = 0;
    if(SHORT_PRESS == type)
    {
        //唤醒屏幕
        wakeUpOledBackLight(&backlightTime);
        pageSelect.select = ON;
        //提示界面刷新
        reflash_flag = ON;
    }
    else if(LONG_PRESS == type)
    {

        clear_screen();
        info = pageInfor;

        //如果是工厂模式退出去的话就直接关闭工厂模式
        if(FACTORY_PAGE == info)
        {
            setFactoryMode(NO);
        }

        pageInfor = pageInfor >> 8;
        reflash_flag = ON;
    }
}
void UpBtnCallBack(u8 type)
{
    u8 nowPage      = 0;

    if(SHORT_PRESS == type)
    {
        //唤醒屏幕
        wakeUpOledBackLight(&backlightTime);

        nowPage = pageInfor & 0x000000FF;
#if(HUB_SELECT == HUB_IRRIGSTION)
        if(HOME_PAGE == nowPage)
        {
            pageSelect.showMore = ON;
        }
        else
        {
            if(pageSelect.cusor > pageSelect.cusor_home)
            {
                pageSelect.cusor--;
            }
            else
            {
                pageSelect.cusor = pageSelect.cusor_max;
            }
        }
#elif(HUB_SELECT == HUB_ENVIRENMENT)
        if(pageSelect.cusor > pageSelect.cusor_home)
        {
            pageSelect.cusor--;
        }
        else
        {
            pageSelect.cusor = pageSelect.cusor_max;
        }
#endif
        //提示界面刷新
        reflash_flag = ON;
    }
}
void DowmBtnCallBack(u8 type)
{
    if(SHORT_PRESS == type)
    {
        //LOG_I("DowmBtnCallBack 1");
        //唤醒屏幕
        wakeUpOledBackLight(&backlightTime);
        if(pageSelect.cusor_max > 0)
        {
            if(pageSelect.cusor < pageSelect.cusor_max)
            {
                //LOG_I("DowmBtnCallBack 2");
                pageSelect.cusor++;
            }
            else
            {
                //LOG_I("DowmBtnCallBack 3");
                pageSelect.cusor = pageSelect.cusor_home;
            }
        }
        //提示界面刷新
        reflash_flag = ON;

        next_flag = YES;//仅仅是灌溉版首页需要使用
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
    phec_sensor_t *phec;

    switch (page)
    {
        case HOME_PAGE:
            pageSelectSet(YES, 1, 1);
            break;
        case SETTING_PAGE:
#if(HUB_SELECT == HUB_ENVIRENMENT)
            pageSelectSet(YES, 1, 6);
#elif(HUB_SELECT == HUB_IRRIGSTION)
            pageSelectSet(YES, 1, 5);
#endif
            break;
        case FACTORY_PAGE:
            pageSelectSet(YES, 1, 5);
            break;

        case SENSOR_STATE_PAGE:
#if (HUB_SELECT == HUB_ENVIRENMENT)
            pageSelectSet(NO, 0, 0);
#elif(HUB_SELECT == HUB_IRRIGSTION)
            pageSelectSet(NO, 1, GetMonitor()->sensor_size);
#endif
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

        case CO2_CALIBRATE_PAGE:
            pageSelectSet(NO, 1, 2);
            break;

        case PH_ONLINE_PAGE:
            phec = getPhEcList(GetMonitor(), YES);
            pageSelectSet(NO, 1, phec->num);
            break;

        case EC_ONLINE_PAGE:
            phec = getPhEcList(GetMonitor(), YES);
            pageSelectSet(NO, 1, phec->num);
            break;

        case PHEC_CALIBRATE_PAGE:
            pageSelectSet(NO, 1, 2);
            break;

        case EC_CALIBRATE_PAGE:
            pageSelectSet(NO, 1, 3);
            break;

        case PH_CALIBRATE_PAGE:
            pageSelectSet(NO, 1, 3);
            break;

        case FA_SENSOR_PAGE:
            pageSelectSet(NO, 0, 0);
            break;
        case FA_DEVICE_PAGE:
            pageSelectSet(NO, 1, 2);
            break;
        case FA_LINE_PAGE:
            pageSelectSet(NO, 1, 2);
            break;
        case FA_SD_PAGE:
            pageSelectSet(NO, 0, 0);
            break;
        case FA_TEST_PAGE:
            pageSelectSet(NO, 1, 5);
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
            HomePage(&pageSelect, GetMonitor());

            if(ON == pageSelect.select)
            {
                if(1 == pageSelect.cusor)
                {
                    pageInfor <<= 8;
                    pageInfor |= SETTING_PAGE;
                }

                pageSelect.select = OFF;
            }

            break;
        case SETTING_PAGE:
            SettingPage(pageSelect, 3);
            if(ON == pageSelect.select)
            {
#if (HUB_SELECT == HUB_ENVIRENMENT)
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
                    pageInfor |= LINE_STATE_PAGE;
                }
                else if(4 == pageSelect.cusor)
                {
                    pageInfor <<= 8;
                    pageInfor |= QRCODE_PAGE;
                }
                else if(5 == pageSelect.cusor)
                {
                    pageInfor <<= 8;
                    pageInfor |= APP_UPDATE_PAGE;
                }
                else if(6 == pageSelect.cusor)
                {
                    pageInfor <<= 8;
                    pageInfor |= CO2_CALIBRATE_PAGE;
                }
#elif (HUB_SELECT == HUB_IRRIGSTION)

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
                else if(5 == pageSelect.cusor)
                {
                    pageInfor <<= 8;
                    pageInfor |= PHEC_CALIBRATE_PAGE;
                }
#endif
                pageSelect.select = OFF;
            }
            break;

        case FACTORY_PAGE:
            factoryPage(pageSelect, 3);

            if(ON == pageSelect.select)
            {
                if(1 == pageSelect.cusor)
                {
                    pageInfor <<= 8;
                    pageInfor |= FA_SENSOR_PAGE;
                }
                else if(2 == pageSelect.cusor)
                {
                    pageInfor <<= 8;
                    pageInfor |= FA_DEVICE_PAGE;
                }
                else if(3 == pageSelect.cusor)
                {
                    pageInfor <<= 8;
                    pageInfor |= FA_LINE_PAGE;
                }
                else if(4 == pageSelect.cusor)
                {
                    pageInfor <<= 8;
                    pageInfor |= FA_SD_PAGE;
                }
                else if(5 == pageSelect.cusor)
                {
                    pageInfor <<= 8;
                    pageInfor |= FA_TEST_PAGE;
                }

                pageSelect.select = OFF;
            }
            break;

        case SENSOR_STATE_PAGE:
#if (HUB_SELECT == HUB_ENVIRENMENT)
                SensorStatePage_new(GetMonitor());
#elif (HUB_SELECT == HUB_IRRIGSTION)
                SensorList(&pageInfor, &pageSelect, GetMonitor());
#endif
            if(ON == pageSelect.select)
            {
                pageSelect.select = OFF;
            }
            break;

        case DEVICE_STATE_PAGE:
            DeviceStatePage_new(GetMonitor());
            if(ON == pageSelect.select)
            {
                pageSelect.select = OFF;
            }
            break;

        case LINE_STATE_PAGE:
            LineStatePage_new(GetMonitor());
            if(ON == pageSelect.select)
            {
                pageSelect.select = OFF;
            }
            break;

        case QRCODE_PAGE:
            qrcode();
            ST7567_UpdateScreen();
            if(ON == pageSelect.select)
            {
                pageSelect.select = OFF;
            }
            break;

        case APP_UPDATE_PAGE:
            UpdateAppProgram(&pageSelect, &pageInfor);
            break;

        case CO2_CALIBRATE_PAGE:
            co2CalibratePage(&pageSelect, &pageInfor);
            if(ON == pageSelect.select)
            {
                pageSelect.select = OFF;
            }
            break;

        case PHEC_CALIBRATE_PAGE:
            PhEcCalibratePage(&pageSelect);
            if(ON == pageSelect.select)
            {
                if(1 == pageSelect.cusor)
                {
                    pageInfor <<= 8;
                    pageInfor |= PH_ONLINE_PAGE;
                }
                else if(2 == pageSelect.cusor)
                {
                    pageInfor <<= 8;
                    pageInfor |= EC_ONLINE_PAGE;
                }

                pageSelect.select = OFF;
            }
            break;

        case PH_ONLINE_PAGE:

            phecOnlinePage(&pageInfor, &pageSelect, GetMonitor(), F_S_PH);
            if(ON == pageSelect.select)
            {
                pageSelect.select = OFF;
            }
            break;

        case EC_ONLINE_PAGE:

            phecOnlinePage(&pageInfor, &pageSelect, GetMonitor(), F_S_EC);
            if(ON == pageSelect.select)
            {
                pageSelect.select = OFF;
            }
            break;

        case PH_CALIBRATE_PAGE:

//            PhCalibratePage(&pageSelect, getPhCalByuuid(now_phec_uuid));//Justin debug
            if(ON == pageSelect.select)
            {
                pageSelect.select = OFF;
            }
            break;

        case EC_CALIBRATE_PAGE:
//            EcCalibratePage(&pageSelect, getEcCalByuuid(now_phec_uuid));//Justin debug
            if(ON == pageSelect.select)
            {
                pageSelect.select = OFF;
            }
            break;

        case FA_SENSOR_PAGE:
            SensorStatePage_fac(GetMonitor(), 4);
            if(ON == pageSelect.select)
            {
                pageSelect.select = OFF;
            }
            break;

        case FA_DEVICE_PAGE:
            deviceStatePage_fac(&pageSelect, GetMonitor(), 3);
            break;

        case FA_LINE_PAGE:
            lineStatePage_fac(&pageSelect, GetMonitor(), 3);
            break;

        case FA_SD_PAGE:
            SDState_Fac();
            if(ON == pageSelect.select)
            {
                pageSelect.select = OFF;
            }
            break;

        case FA_TEST_PAGE:
            testFacPage(&pageSelect, GetMonitor(), 4);

            break;

        case TEST_PAGE:
            testPage();
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
    static      u16             timeFactory         = 0;
    static      u8              nowPagePre          = 0xFF;

    st7567Init();
    pageInfor <<= 8;
    pageInfor |= HOME_PAGE;
    wakeUpOledBackLight(&backlightTime);
    while(1)
    {
        time1S = TimerTask(&time1S, 20, &Timer1sTouch);
        time3S = TimerTask(&time3S, 60, &Timer3sTouch);

        //Justin 以下功能全部要优化 暂时屏蔽
//        //50ms
//        nowPage = pageInfor & 0x000000FF;
//
//        if(nowPagePre != nowPage)
//        {
//            //设置初始光标
//            pageSetting(nowPage);
//
//            pageProgram(nowPage);
//
//            nowPagePre = nowPage;
//        }
//        else
//        {
//            if(ON == reflash_flag)
//            {
//                pageProgram(nowPage);
//            }
//        }
//
//        //1.特殊功能
//        if((KEY_ON == rt_pin_read(BUTTON_UP)) && (KEY_ON == rt_pin_read(BUTTON_DOWN)))
//        {
//            if(timeFactory < (10000 / 50))
//            {
//                timeFactory++;
//            }
//            else
//            {
//                setFactoryMode(YES);
//                pageInfor = FACTORY_PAGE;
//            }
//        }
//        else
//        {
//            timeFactory = 0;
//        }
//
//        //1s event
//        if(ON == Timer1sTouch)
//        {
//            monitorBackLight(backlightTime);
//            if((HOME_PAGE == nowPage) || (SETTING_PAGE == nowPage) /*|| (CO2_CALIBRATE_PAGE == nowPage)*/)
//            {
//                //需要刷新
//                reflash_flag = ON;
//            }
//        }
//
//        //3s event
//        if(ON == Timer3sTouch)
//        {
//
//            if((SENSOR_STATE_PAGE == nowPage) ||
//               (DEVICE_STATE_PAGE == nowPage) ||
//               (FA_SENSOR_PAGE == nowPage) ||
//               (FA_DEVICE_PAGE == nowPage) ||
//               (FA_LINE_PAGE == nowPage) ||
//               (FA_SD_PAGE == nowPage) ||
//               (FA_TEST_PAGE == nowPage) ||
//               (PH_CALIBRATE_PAGE == nowPage) ||
//               (EC_CALIBRATE_PAGE == nowPage))
//            {
//                //需要刷新
//                reflash_flag = ON;
//            }
//        }

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

