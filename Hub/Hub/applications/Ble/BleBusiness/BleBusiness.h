/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-24     Administrator       the first version
 */
#ifndef APPLICATIONS_BLE_BLEBUSINESS_BLEBUSINESS_H_
#define APPLICATIONS_BLE_BLEBUSINESS_BLEBUSINESS_H_

#include <board.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <rtdbg.h>
#include <stdlib.h>

#include "GlobalConfig.h"
#include "typedef.h"
#include "Gpio.h"

#include "InformationMonitor.h"

rt_err_t AnalyzePack(type_blepack_t *);

#endif /* APPLICATIONS_BLE_BLEBUSINESS_BLEBUSINESS_H_ */
