/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-04-04     Administrator       the first version
 */
#ifndef APPLICATIONS_UART_SENSORUARTCLASS_SENSORUARTCLASS_H_

#define APPLICATIONS_UART_SENSORUARTCLASS_SENSORUARTCLASS_H_

#include <DeviceUartClass.h>

void InitSensorObject(void);
type_uart_class *GetSensorObject(void);
//void MonitorSendAddr(type_monitor_t *);
//void InitDataTest(void);

#endif /* APPLICATIONS_UART_SENSORUARTCLASS_SENSORUARTCLASS_H_ */
