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

#include "GlobalConfig.h"
#include "typedef.h"
#include "string.h"
#include "Gpio.h"

/* 串口接收消息结构*/
struct rx_msg
{
    rt_device_t dev;
    rt_size_t size;
};

void SensorUart2TaskEntry(void* parameter);
void SensorUart2TaskInit(void);
u16 usModbusRTU_CRC(const u8* pucData, u32 ulLen);


#endif /* APPLICATIONS_UART_H_ */