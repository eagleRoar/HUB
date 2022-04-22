/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-01-21     Administrator       the first version
 */
#ifndef APPLICATIONS_OLED1309_H_

#define APPLICATIONS_OLED1309_H_

#include <board.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <rtdbg.h>

#include "GlobalConfig.h"
#include "typedef.h"
#include "Gpio.h"
#include "Device.h"
#include "InformationMonitor.h"

void oledInit(void);
void OledTaskEntry(void* parameter);
void OledTaskInit(void);

#endif /* APPLICATIONS_OLED1309_H_ */
