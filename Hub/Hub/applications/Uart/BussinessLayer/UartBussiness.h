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
#include "CloudProtocol.h"

struct connectManage{
    u32 send_count;
    u8  send_state;
};

typedef     struct connectManage    type_connect_t;

enum{
    CON_NULL = 0x00,
    CON_WAITING ,//等待中
    CON_FAIL,
    CON_SUCCESS,
}CON_STATE;

void initConnectState(void);
void UpdateModuleConnect(type_monitor_t *, u8);
void MonitorModuleConnect(type_monitor_t *);
u8 askSensorStorage(type_monitor_t *, rt_device_t);
u8 askDeviceHeart_new(type_monitor_t *, rt_device_t, u8);
u8 askLineHeart(type_monitor_t *, rt_device_t);
void AnalyzeData(rt_device_t , type_monitor_t *, u8 *, u8);
void AnlyzeModuleInfo(type_monitor_t *, u8 *, u8);
void findDeviceLocation(type_monitor_t *, cloudcmd_t *,rt_device_t);
void findLineLocation(type_monitor_t *, cloudcmd_t *,rt_device_t);
void replyStrorageType(type_monitor_t *, u8, u8 *, u8);
void setDeviceEvent(u8);
u8 getDeviceEvent(void);
rt_err_t changeCharToDate(char*, type_sys_time *);
rt_err_t changeDataToChar(char*, type_sys_time *);
void initCtrlPre(void);
type_ctrl_t getCtrlPre(u8 , u8);
#endif /* APPLICATIONS_UART_BUSSINESSLAYER_UARTBUSSINESS_H_ */
