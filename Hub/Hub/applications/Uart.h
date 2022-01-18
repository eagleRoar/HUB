/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-01-12     Qiuyijie     the first version
 */
#ifndef APPLICATIONS_UART_H_
#define APPLICATIONS_UART_H_

#include <board.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <rtdbg.h>

#include "typedef.h"

static rt_err_t uart_input(rt_device_t dev, rt_size_t size);
void SensorUart2TaskEntry(void* parameter);
void SensorUart2TaskInit(void);

#endif /* APPLICATIONS_UART_H_ */
