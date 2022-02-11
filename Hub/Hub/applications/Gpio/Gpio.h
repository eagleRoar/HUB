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
#include "Oled1309.h"
#include "Ble.h"

#ifdef TEST_PROGRAM
//LED
//#define LED_SENSOR        GET_PIN(D, 10)  //senser//Justin debug 该脚暂时用作ETH_RESET脚
#define LED_DEVICE        GET_PIN(D, 11)  //device
#define LED_LIGHT         GET_PIN(D, 12)  //呼吸灯
#define LED_COMMS         GET_PIN(D, 13)  //通讯灯
#define LED_BLUETOOTH     GET_PIN(D, 14)  //蓝牙灯
#define LED_POWER         GET_PIN(D, 15)  //电源灯

//LCD GPIO
//#define RST_CTR GET_PIN(A, 15) //硬件上没有该脚
#define LCD_WR GET_PIN(E, 10)
#define LCD_RD GET_PIN(E, 11)
#define LCD_DC GET_PIN(E, 9)
#define LCD_DB0 GET_PIN(E, 0)
#define LCD_DB1 GET_PIN(E, 1)
#define LCD_DB2 GET_PIN(E, 2)
#define LCD_DB3 GET_PIN(E, 3)
#define LCD_DB4 GET_PIN(E, 4)
#define LCD_DB5 GET_PIN(E, 5)
#define LCD_DB6 GET_PIN(E, 6)
#define LCD_DB7 GET_PIN(E, 7)
#define LCD_CS GET_PIN(E, 8)

void GpioInit(void);
void Ctrl_LED(rt_base_t pin, rt_base_t state);
void LedTaskInit(void);
void LedTaskEntry(void* parameter);

#endif

#endif /* APPLICATIONS_GPIO_H_ */
