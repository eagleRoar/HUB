/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-01-25     Administrator       the first version
 */
#ifndef APPLICATIONS_BLE_BLE_H_
#define APPLICATIONS_BLE_BLE_H_

#include <board.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <rtdbg.h>
#include <stdlib.h>

#include "GlobalConfig.h"
#include "typedef.h"
#include "Gpio.h"

void BleUart6TaskInit(void);
void BleUart6TaskEntry(void* parameter);

/* 串口接收消息结构*/
struct UartMsg
{
    rt_device_t dev;
    rt_size_t   size;
    rt_bool_t   revFlg; //接收标志
};

#endif /* APPLICATIONS_BLE_BLE_H_ */
