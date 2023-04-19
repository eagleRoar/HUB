/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-04-10     Administrator       the first version
 */
#include "UartEventType.h"
#include "UartDataLayer.h"
#include "DeviceUartClass.h"
#include "LightUartClass.h"

#ifndef APPLICATIONS_UART_DATALAYER_UARTACTION_H_
#define APPLICATIONS_UART_DATALAYER_UARTACTION_H_

void GetNowSysSet(proTempSet_t *tempSet, proCo2Set_t *co2Set, proHumiSet_t *humiSet,
        proLine_t *line1Set, proLine_t *line2Set, struct recipeInfor *info);
void co2Program(type_monitor_t *monitor, type_uart_class uart, u16 mPeriod);
void humiProgram(type_monitor_t *monitor, type_uart_class uart);
void tempProgram(type_monitor_t *monitor, type_uart_class uart);
void menualHandProgram(type_monitor_t *, type_uart_class , type_uart_class);
#endif /* APPLICATIONS_UART_DATALAYER_UARTACTION_H_ */
