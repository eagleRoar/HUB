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

#define HUB_ENVIRENMENT                 1   //环控
#define HUB_IRRIGSTION                  2   //灌溉

//#define FACTORY_MODE                  //是否支持工厂模式

#define DEBUG_MODE                      0//1                    //1.如果是debug mode 的话不需要偏移位置，否则偏移位置SD卡升级 2.修改link.lds
#define HUB_SELECT                      HUB_IRRIGSTION          //HUB_ENVIRENMENT         //

/* 软件号的 第一个分配为:
    0 : HUB_ENVIRENMENT
    1 : HUB_IRRIGSTION
    软件版本(环控 0.1.6，灌溉1.1.8)开始之后就增加软件看门狗
*/
#if(HUB_ENVIRENMENT == HUB_SELECT)
#define FIRMWAREVISION                  "0.2.20"
#elif(HUB_IRRIGSTION == HUB_SELECT)
#define FIRMWAREVISION                  "1.2.21"
#endif
#define FIRMWARE_VERSION_NUM            2
#define BOOTLOADVISION                  "0.0.1"

#define UDP_TASK                        "udp_task"
#define TCP_SEND_TASK                   "tcp_send"
#define TCP_RECV_TASK                   "tcp_recv"
#define TCP_CLIENT_TASK                 "tcpc"
#define SD_CARD_TASK                    "sdcard_task"
#define OLED_TASK                       "oled task"
#define UART_TASK                       "usart_task"
#define BUTTON_TASK                     "button_task"
#define SPI_TASK                        "spi_task"
#define FILE_SYS_TASK                   "filesys_task"

/* 线程优先级管理 */
#define FILE_SYS_PRIORITY               12
#define SPI_PRIORITY                    12
#define SDCARD_PRIORITY                 16
#define UART_PRIORITY                   21
#define BUTTON_PRIORITY                 22
#define OLED_PRIORITY                   23
#define TCP_PRIORITY                    24
#define UDP_PRIORITY                    25
//#define SPI_PRIORITY                    26
#define MQTT_PRIORITY                   27
#define LED_PRIORITY                    28

//线程周期
#define BUTTON_TASK_PERIOD              20
#define FILE_SYS_PERIOD                 100
#define MAIN_PERIOD                     50
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

/* 配置CJSON */
#define PKG_USING_RT_CJSON_TOOLS
#define PKG_USING_RT_CJSON_TOOLS_LATEST_VERSION
#define RT_CJSON_TOOLS_ENABLE_EXAMPLE
#define RT_CJSON_TOOLS_EXAMPLE_STACK_SZIE 1024
#define RT_CJSON_TOOLS_EXAMPLE_PRIORITY 15

/* 配置MQTT */
#define MQTT_DEBUG
#define PKG_USING_MYMQTT
#define PKG_USING_MYMQTT_LATEST_VERSION
#define PKG_USING_MYMQTT_EXAMPLE
#define MQTT_MAX_MESSAGE_HANDLERS 1

/*日历*/
#define PKG_USING_CAL

/* QRCODE */
#define PKG_USING_QRCODE
#define PKG_QRCODE_SAMPLE
#define PKG_USING_QRCODE_LATEST_VERSION

#if (HUB_SELECT == HUB_ENVIRENMENT)
#define     HUB_NAME    "BHE"
#elif (HUB_SELECT == HUB_IRRIGSTION)
#define     HUB_NAME    "BHI"
#endif

/*存储区名称*/
#define FLASH_MEMORY_NAME       "norflash0"
#define SDCARD_MEMORY_NAME      "sd0"

#endif /* GLOBALCONFIG_H_ */
