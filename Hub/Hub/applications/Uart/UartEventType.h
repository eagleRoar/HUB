/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-01-09     Administrator       the first version
 */
#ifndef APPLICATIONS_UART_UARTEVENTTYPE_H_
#define APPLICATIONS_UART_UARTEVENTTYPE_H_

#include "Gpio.h"


//逻辑层的串口事件
enum
{
    UART_REGISTART,     //注册事件
    UART_BROADCASE,     //广播事件
    UART_SENSOR_REG,    //传感器数据(读取/接收)
    UART_DEVICE_REG,    //设备寄存器值(读取/接收)
    UART_LIGHT_REG,     //灯光寄存器值(读取/接收)
    UART_PORT_TYPE,     //设备端口类型
};

//针对链表中的key的数据结构组合
#pragma pack(1)

//该结构针对寄存器的设置,注意长度为long的长度四字节
typedef struct SeqListKey{
    u8 addr;
    u8 regH;
    u8 regL;
    u8 regSize;
}seq_key_t;

#pragma pack()

#endif /* APPLICATIONS_UART_UARTEVENTTYPE_H_ */
