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

#endif /* APPLICATIONS_BLE_BLE_H_ */
