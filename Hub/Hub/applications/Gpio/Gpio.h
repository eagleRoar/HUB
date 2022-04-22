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

#include "GlobalConfig.h"
#include "typedef.h"


//LED
#define LED_SENSOR        GET_PIN(D, 10)  //senser
#define LED_DEVICE        GET_PIN(D, 11)  //device
#define LED_LIGHT         GET_PIN(D, 12)  //呼吸灯
#define LED_COMMS         GET_PIN(D, 13)  //通讯灯
#define LED_BLUETOOTH     GET_PIN(D, 14)  //蓝牙灯
#define LED_POWER         GET_PIN(D, 15)  //电源灯

//LCD GPIO
//#define RST_CTR GET_PIN(A, 15) //硬件上没有该脚
#define LCD_WR            GET_PIN(E, 10)
#define LCD_RD            GET_PIN(E, 11)
#define LCD_DC            GET_PIN(E, 9)
#define LCD_DB0           GET_PIN(E, 0)
#define LCD_DB1           GET_PIN(E, 1)
#define LCD_DB2           GET_PIN(E, 2)
#define LCD_DB3           GET_PIN(E, 3)
#define LCD_DB4           GET_PIN(E, 4)
#define LCD_DB5           GET_PIN(E, 5)
#define LCD_DB6           GET_PIN(E, 6)
#define LCD_DB7           GET_PIN(E, 7)
#define LCD_CS            GET_PIN(E, 8)

//SD
#define SD_CHK_PIN        GET_PIN(D, 3)
#define SD_CTL_PIN        GET_PIN(B, 14)

//SPI
#define SPI1_CS_PIN       GET_PIN(A, 4)
#define SPI1_MOSI_PIN     GET_PIN(B, 5)
#define SPI1_MISO_PIN     GET_PIN(A, 6)
#define SPI1_SCK_PIN      GET_PIN(A, 5)

//BLE
#define BLE_NRST_PIN      GET_PIN(B, 8)

//Button
#define BUTTON_MENU       GET_PIN(B, 6)
#define BUTTON_ENTER      GET_PIN(B, 7)

void GpioInit(void);
void Ctrl_LED(rt_base_t pin, rt_base_t state);
void LedTaskInit(void);
void LedTaskEntry(void* parameter);
u16 TimerTask(u16 *, u16 , u8 *);

#endif /* APPLICATIONS_GPIO_H_ */
