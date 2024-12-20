/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-01-21     Administrator       the first version
 */
#ifndef APPLICATIONS_OLED1309_H_

#define APPLICATIONS_OLED1309_H_

#include "Gpio.h"
#include "InformationMonitor.h"

//设计只有三级界面
struct pageSelect
{
    u8  cusor_show;
    u8  cusor_home;
    u8  cusor_max;
    u8  cusor;
    u8  showMore;   //显示更多
    u8  select;
};

typedef struct btn_monitor{
    u8 btn_down;
    u8 btn_up;
    u8 btn_enter;
}type_btn_event;

#define     LINE_HIGHT              8
#define     COLUMN_HIGHT            13          //实际是13为了留间距
//
////一级界面
//#define     SETTING_PAGE               0x00//主页面
//#define     FACTORY_PAGE            0x01//工厂界面
//
////二级界面
//#define     SENSOR_STATE_PAGE       0x02//主页面
//#define     DEVICE_STATE_PAGE       0x03//主页面
//#define     QRCODE_PAGE             0x04//主页面
//#define     APP_UPDATE_PAGE         0x05//主页面
//#define     CO2_CALIBRATE_PAGE      0x06//CO2校准

//工厂版本
enum
{
//一级界面
HOME_PAGE = 0x00,       //主页面
SETTING_PAGE,           //主页面
FACTORY_PAGE,           //工厂界面
//二级界面
SENSOR_STATE_PAGE,      //
DEVICE_STATE_PAGE,      //
LINE_STATE_PAGE,        //
QRCODE_PAGE,            //
APP_UPDATE_PAGE,        //
CO2_CALIBRATE_PAGE,     //CO2校准
PHEC_CALIBRATE_PAGE,    //phec 校准
PH_ONLINE_PAGE,
PH_CALIBRATE_PAGE,
EC_ONLINE_PAGE,
EC_CALIBRATE_PAGE,
DATA_EXPORT,            //数据导出
DATA_IMPORT,            //数据导入
SERVER_URL,             //选择服务器网址
HUB_INFO,
IP_INFO,
RESTORE_SETTINGS,       //恢复出厂设置
MEMORY_INFO,            //内存使用情况
//工厂版本
FA_SENSOR_PAGE,
FA_DEVICE_PAGE,
FA_LINE_PAGE,
FA_SD_PAGE,
FA_TEST_PAGE,
TEST_PAGE,//测试界面 显示一些值
};

//二级界面
//#define     SETTING_PAGE       0x01//主页面
//#define     SETTING_PAGE       0x01//主页面

/****************界面显示内容****************************************/
#define     SENSOR_STATE_NAME       "Sensor State"
#define     DEVICE_STATE_NAME       "Device State"

typedef     struct pageSelect               type_page_t;

void oledInit(void);
void OledTaskEntry(void*);
void OledTaskInit(void);
void MenuBtnCallBack(u8);
void EnterBtnCallBack(u8);
void UpBtnCallBack(u8);
void DowmBtnCallBack(u8);
void clear_screen(void);
u8 getFactoryMode(void);
#endif /* APPLICATIONS_OLED1309_H_ */
