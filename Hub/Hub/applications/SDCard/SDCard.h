/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-01-24     Administrator       the first version
 */
#ifndef APPLICATIONS_SDCARD_SDCARD_H_
#define APPLICATIONS_SDCARD_SDCARD_H_

#include <board.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <rtdbg.h>
#include <stdlib.h>

#include "typedef.h"
#include "string.h"

void SDCardTaskEntry(void* parameter);

#endif /* APPLICATIONS_SDCARD_SDCARD_H_ */
