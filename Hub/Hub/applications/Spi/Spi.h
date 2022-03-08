/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-03-05     Administrator       the first version
 */
#ifndef APPLICATIONS_SPI_SPI_H_
#define APPLICATIONS_SPI_SPI_H_

#include <board.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <rtdbg.h>

#include "GlobalConfig.h"
#include "typedef.h"

void SpiTaskEntry(void*);
void SpiTaskInit(void);

#endif /* APPLICATIONS_SPI_SPI_H_ */
