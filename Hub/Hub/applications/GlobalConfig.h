/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-03-07     Administrator       the first version
 */
#ifndef GLOBALCONFIG_H_
#define GLOBALCONFIG_H_
#define UDP_TASK                        "udp_task"
#define TCP_TASK                        "tcp_task"
#define TCP_CLIENT_TASK                 "tcpc"
#define SD_CARD_TASK                    "sdcard_task"

/* 线程优先级管理 */
#define UART6_PRIORITY                  20
#define UART2_PRIORITY                  21
#define BUTTON_PRIORITY                 22
#define OLED_PRIORITY                   23
#define TCP_PRIORITY                    24
#define UDP_PRIORITY                    25
#define SDCARD_PRIORITY                 26
#define SPI_PRIORITY                    26
#define MQTT_PRIORITY                   27
#define LED_PRIORITY                    27


/* 配置button */
#define PKG_USING_BUTTON
#define SINGLE_AND_DOUBLE_TRIGGER
//#define CONTINUOS_TRIGGER
//#define LONG_FREE_TRIGGER
#define BUTTON_DEBOUNCE_TIME 2
#define BUTTON_CONTINUOS_CYCLE 1
#define BUTTON_LONG_CYCLE 1
#define BUTTON_DOUBLE_TIME 15
#define BUTTON_LONG_TIME 50
#define PKG_USING_BUTTON_LATEST_VERSION

/* 配置u8g2 */
#define PKG_USING_U8G2
#define PKG_USING_U8G2_C_LATEST_VERSION
#define PKG_U8G2_VER_NUM 0x19999

#endif /* GLOBALCONFIG_H_ */
