/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-09     Administrator       the first version
 */
#ifndef APPLICATIONS_UART_DATALAYER_UARTDATALAYER_H_
#define APPLICATIONS_UART_DATALAYER_UARTDATALAYER_H_


#include <board.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <rtdbg.h>

#include "GlobalConfig.h"
#include "typedef.h"
#include "string.h"
#include "Gpio.h"

#include "InformationMonitor.h"
#include "Command.h"

u8 getAllocateAddress(type_monitor_t *);
void AnlyzeDeviceRegister(type_monitor_t *, rt_device_t, u8 *, u8);
void RegisterAnswer(type_monitor_t *, rt_device_t , u32 );
void AnlyzeStorage(type_monitor_t *, u8 , u8 *, u8);

#endif /* APPLICATIONS_UART_DATALAYER_UARTDATALAYER_H_ */
