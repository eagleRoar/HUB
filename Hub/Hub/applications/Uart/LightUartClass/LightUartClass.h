/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-01-04     Administrator       the first version
 */
#ifndef APPLICATIONS_UART_LIGHTUARTCLASS_UARTCLASS_H_
#define APPLICATIONS_UART_LIGHTUARTCLASS_UARTCLASS_H_

#include "Uart.h"
#include "SeqList.h"
#include "UartEventType.h"
#include "DeviceUartClass.h"


void InitLightObject(void);
type_uart_class *GetLightObject(void);
void MonitorSendAddr(type_monitor_t *);
void InitDataTest(void);
#endif /* APPLICATIONS_UART_DEVICEUARTCLASS_UARTCLASS_H_ */
