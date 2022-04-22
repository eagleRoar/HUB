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
#include "Device.h"
#include "InformationMonitor.h"

void ControlDeviceStorage(type_module_t *, rt_device_t , u8 , u8 );
void askSensorStorage(type_module_t *, rt_device_t , u16 );
void MonitorModuleConnect(type_monitor_t *, u8 );
void updateModuleConnect(type_monitor_t *, u8);

#endif /* APPLICATIONS_UART_BUSSINESSLAYER_UARTBUSSINESS_H_ */
