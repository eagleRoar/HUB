/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-01-12     Qiuyijie     the first version
 */
#ifndef APPLICATIONS_GPIO_H_
#define APPLICATIONS_GPIO_H_


#include <board.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <rtdbg.h>

#include "typedef.h"


#ifdef TEST_PROGRAM
//LED
#define LED_0 GET_PIN(E, 13)


void GpioInit(void);
void Ctrl_LED(rt_base_t pin, rt_base_t state);
void LedTaskInit(void);
void LedTaskEnter(void* parameter);

#endif

#endif /* APPLICATIONS_GPIO_H_ */
