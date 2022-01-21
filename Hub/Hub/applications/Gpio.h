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
#include "Uart.h"
#include "Ethernet.h"


#ifdef TEST_PROGRAM
//LED
//#define LED_SENSOR        GET_PIN(D, 10)  //senser//Justin debug 该脚暂时用作ETH_RESET脚
#define LED_DEVICE        GET_PIN(D, 11)  //device
#define LED_LIGHT         GET_PIN(D, 12)  //呼吸灯
#define LED_COMMS         GET_PIN(D, 13)  //通讯灯
#define LED_BLUETOOTH     GET_PIN(D, 14)  //蓝牙灯
#define LED_POWER         GET_PIN(D, 15)  //电源灯

void GpioInit(void);
void Ctrl_LED(rt_base_t pin, rt_base_t state);
void LedTaskInit(void);
void LedTaskEntry(void* parameter);

#endif

#endif /* APPLICATIONS_GPIO_H_ */
