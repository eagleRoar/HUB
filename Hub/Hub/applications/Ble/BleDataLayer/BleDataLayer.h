/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-24     Administrator       the first version
 */
#ifndef APPLICATIONS_BLE_BLEDATALAYER_BLEDATALAYER_H_
#define APPLICATIONS_BLE_BLEDATALAYER_BLEDATALAYER_H_

#include <board.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <rtdbg.h>
#include <stdlib.h>

#include "GlobalConfig.h"
#include "typedef.h"
#include "Gpio.h"

#include "InformationMonitor.h"

#define         GUIDE_CODE              0x485542            //引导码
#define         BLE_BUFFER_SIZE         512

#define         ASK_HUB_INFO            10001               //获取hub数据
#define         ASK_MESSAGE             10002               //获取hub实时信息
#define         ASK_DEVICE_LIST         10003               //获取设备列表
#define         ASK_TIMER_LIST          10004               //获取定时设备列表
#define         ASK_TIMER_SETTING       10005               //获取指定设备定时设置
#define         ASK_HISTORY             10006               //获取历史记录信息

#define         SET_HUB                 15001               //修改 Hub
#define         SET_CO2                 15002               //设置 Co2
#define         SET_HUMIDITY            15003               //设置湿度
#define         SET_TEMPERATURE         15004               //设置温度
#define         SET_LIGHT1              15005               //设置灯光 1
#define         SET_LIGHT2              15006               //设置灯光 2
#define         SET_LIGHT_TEMP          15007               //设置灯光温控
#define         SET_PHEC                15008               //设置 PHEC
#define         SET_TIMER_LIST          15009               //设置定时设备列表

#define         REP_HUB_INFO            20001               //获取hub数据
#define         REP_MESSAGE             20002               //获取hub实时信息
#define         REP_DEVICE_LIST         20003               //获取设备列表
#define         REP_TIMER_LIST          20004               //获取定时设备列表
#define         REP_TIMER_SETTING       20005               //获取指定设备定时设置
#define         REP_HISTORY             20006               //获取历史记录信息

#define         REP_HUB                 25001               //修改 Hub
#define         REP_CO2                 25002               //设置 Co2
#define         REP_HUMIDITY            25003               //设置湿度
#define         REP_TEMPERATURE         25004               //设置温度
#define         REP_LIGHT1              25005               //设置灯光 1
#define         REP_LIGHT2              25006               //设置灯光 2
#define         REP_LIGHT_TEMP          25007               //设置灯光温控
#define         REP_PHEC                25008               //设置 PHEC
#define         REP_TIMER_LIST          25009               //设置定时设备列表

struct blePackTop{
    u8      guide_code[3];
    u32     pack_length;
    u16     crc;
    u16     command;
    u32     buffer_length;
};

struct blePackage
{
    type_blepacktop_t           top;
    u8                          buffer[BLE_BUFFER_SIZE];
};

rt_err_t AskMessage(type_blepack_t *);

#endif /* APPLICATIONS_BLE_BLEDATALAYER_BLEDATALAYER_H_ */
