/*
 * Copyright (c) 2006-2018 RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-11-27   ZhouXiaomin     first version
 */
#ifndef _UUZ_UART_CONFIG_H_
#define _UUZ_UART_CONFIG_H_

//115200b/s 波特率每字符时间为:1/115200*(1+8+1) ~ 86.80us
//帧间:		3.5个字符时间为	86.80*(3.5+1) ~ 390.6us
//字节间:	1.5个字符时间为  86.80*(1.5+1) ~ 217.0us

//9600b/s 波特率每字符时间为:1/9600*(1+8+1) ~ 1041.66us
//帧间:		3.5个字符时间为	1041.66*(3.5+1) ~ 4687.5us
//字节间:	1.5个字符时间为 1041.66*(1.5+1) ~ 2604.2

/* 调试Uart */
#define uuzCONFIG_DEBUG_FLAG (0U)

#define uuzUART_LEN (128U)

#define uuzUSART_TTL (0U)
#define uuzUSART_RS485 (1U)
#define uuzUSART_RS232 (2U)

#define HMI_UART_NAME "uart1" /* 串口设备名称 */
#define HMI_RX_SEM_NAME "hmi_rx_sem" /* RX回调名称 */
#define HMI_THREAD_NAME "hmi_rx" /* 线程名称 */
#define HMI_TIMER_NAME "hmi_tr" /* 定时器名称 */
#define HMI_FRAME_SPAN 4 /* 4*100us */
#define HMI_BYTE_SPAN 2 /* 2*100us */

#define SENSOR_UART_NAME "uart7" /* 串口设备名称 */
#define SENSOR_RX_SEM_NAME "sr_rx_sem" /* RX回调名称 */
#define SENSOR_THREAD_NAME "sr_rx" /* 线程名称 */
#define SENSOR_TIMER_NAME "sr_tr" /* 定时器名称 */
#define SENSOR_FRAME_SPAN 7 /* 35*100us */
#define SENSOR_BYTE_SPAN 3 /* 15*100us */

#define DEVICE_UART_NAME "uart8" /* 串口设备名称 */
#define DEVICE_RX_SEM_NAME "dev_rx_sem" /* RX回调名称 */
#define DEVICE_THREAD_NAME "dev_rx" /* 线程名称 */
#define DEVICE_TIMER_NAME "dev_tr" /* 定时器名称 */
#define DEVICE_FRAME_SPAN 7 /* 35*100us */
#define DEVICE_BYTE_SPAN 3 /* 15*100us */

#define LINE1_UART_NAME "uart3" /* 串口设备名称 */
#define LINE1_RX_SEM_NAME "l1_rx_sem" /* RX回调名称 */
#define LINE1_THREAD_NAME "l1_rx" /* 线程名称 */
#define LINE1_TIMER_NAME "l1_tr" /* 定时器名称 */
#define LINE1_FRAME_SPAN 7 /* 35*100us */
#define LINE1_BYTE_SPAN 3 /* 15*100us */

#define LINE2_UART_NAME "uart2" /* 串口设备名称 */
#define LINE2_RX_SEM_NAME "l2_rx_sem" /* RX回调名称 */
#define LINE2_THREAD_NAME "l2_rx" /* 线程名称 */
#define LINE2_TIMER_NAME "l2_tr" /* 定时器名称 */
#define LINE2_FRAME_SPAN 7 /* 35*100us */
#define LINE2_BYTE_SPAN 3 /* 15*100us */


//串口类型
//RS232模式,不保存发送数据
#define uuzUART_COM_RS232 (0U)
//RS232模式,保存发送数据
#define uuzUART_RTU_RS232 (1U)
//RS485模式,不保存发送数据
#define uuzUART_COM_RS485 (2U)
//RS485模式,保存发送数据
#define uuzUART_RTU_RS485 (3U)

/* 485通讯上升方式 */
#define uuzCONFIG_USART_RS485_ENABLE
/* 是否需要触发IO口的上下拉动作 */
//#define uuzCONFIG_USART_RS485_OPT_ENABLE
#if defined(uuzCONFIG_USART_RS485_ENABLE)
#if defined(uuzCONFIG_USART_RS485_OPT_ENABLE)
#include "extIO.h"
#define uuzUSART_CLK_TX rt_pin_write(RS485_CLK_PIN, PIN_HIGH) //High level
#define uuzUSART_CLK_RX rt_pin_write(RS485_CLK_PIN, PIN_LOW) //Low  level
#else
#define uuzUSART_CLK_TX ;
#define uuzUSART_CLK_RX ;
#endif //rs485 Opt Enable
#endif //rs485 Enable

#endif /* _UUZ_UART_CONFIG_H_ */
