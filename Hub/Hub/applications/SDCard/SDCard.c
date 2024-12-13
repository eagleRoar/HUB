/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-01-24     Administrator       the first version
 */

#include "Gpio.h"
#include "SDCard.h"
#include "Uart.h"
#include "CloudProtocol.h"
#include "Recipe.h"

#define DBG_TAG "u.sd"
#define DBG_LVL DBG_INFO

__attribute__((section(".ccmbss"))) struct sdCardState      sdCard;

extern u8 saveModuleFlag;
extern int rt_hw_sdio_init(void);


