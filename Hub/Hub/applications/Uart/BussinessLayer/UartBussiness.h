/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-12     Administrator       the first version
 */
#ifndef APPLICATIONS_UART_BUSSINESSLAYER_UARTBUSSINESS_H_
#define APPLICATIONS_UART_BUSSINESSLAYER_UARTBUSSINESS_H_

#include <board.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <rtdbg.h>

#include "GlobalConfig.h"
#include "typedef.h"
#include "string.h"
#include "Gpio.h"
#include "InformationMonitor.h"

struct connectManage{
    u8 count;       //次数
    u8 time;        //ms单位
};

typedef     struct connectManage    type_connect_t;

enum{
    CON_NULL = 0x00,
    CON_WAITING ,//等待中
    CON_FAIL,
    CON_SUCCESS,
}CON_STATE;

u8 MonitorModuleConnect(type_monitor_t *, u8 , u8);
void askSensorStorage(type_monitor_t *, rt_device_t);
void askDeviceHeart(type_monitor_t *, rt_device_t);
void AnalyzeData(rt_device_t , type_monitor_t *, u8 *, u8);
void AnlyzeModuleInfo(type_monitor_t *, u8 *, u8);

#endif /* APPLICATIONS_UART_BUSSINESSLAYER_UARTBUSSINESS_H_ */