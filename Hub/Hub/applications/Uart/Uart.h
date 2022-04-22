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
#include "InformationMonitor.h"

#define             DEVICE_UART2            "uart2"
#define             DEVICE_UART3            "uart3"

#define             UART_MSG_SIZE           512
/* 串口接收消息结构*/
struct rx_msg
{
    rt_device_t dev;
    rt_size_t size;
    u8 messageFlag;
    u8 data[UART_MSG_SIZE];                 //保存串口数据
};

struct rt_msg_BhsCo2
{
    u16 co2;                //二氧化碳
    u16 humidity;           //湿度
    u16 temperature;        //温度
    u16 light;              //灯光
};

void SensorUart2TaskEntry(void* parameter);
void SensorUart2TaskInit(void);
type_monitor_t *GetMonitor(void);

#endif /* APPLICATIONS_UART_H_ */
