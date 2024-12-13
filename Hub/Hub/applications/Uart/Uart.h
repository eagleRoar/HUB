/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-01-12     Qiuyijie     the first version
 */
#ifndef APPLICATIONS_UART_H_
#define APPLICATIONS_UART_H_

#include "Gpio.h"

#include "InformationMonitor.h"

#define             DEVICE_UART1            "uart1"
#define             DEVICE_UART2            "uart2"
#define             DEVICE_UART3            "uart3"

#define             UART_MSG_SIZE           512
#define             UART_PERIOD             100
#define             CONNRCT_MISS_MAX        5
/* 串口接收消息结构*/
struct rx_msg
{
    rt_device_t dev;
    rt_size_t size;
    u8 messageFlag;
    u8 data[UART_MSG_SIZE];                 //保存串口数据
};

typedef struct ph_cal
{
    u8      cal_7_flag;
    u8      cal_4_flag;
    time_t  time;
    u32     uuid;
}ph_cal_t;

typedef struct ec_cal
{
    u8      cal_0_flag;
    u8      cal_141_flag;
    time_t  time;
    u32     uuid;
}ec_cal_t;

typedef struct ph_cal_data
{
    int data_ph_7;
    int data_ph_4;
    u32 uuid;
}phcal_data_t;

typedef struct ec_cal_data
{
    int data_ec_0;
    int data_ec_141;
    u32 uuid;
}eccal_data_t;

void SensorUart2TaskEntry(void* parameter);
void SensorUart2TaskInit(void);
type_monitor_t *GetMonitor(void);
void initMonitor(void);

#endif /* APPLICATIONS_UART_H_ */
