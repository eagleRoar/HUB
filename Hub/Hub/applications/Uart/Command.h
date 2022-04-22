/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-08     Administrator       the first version
 */
#ifndef APPLICATIONS_UART_COMMAND_H_
#define APPLICATIONS_UART_COMMAND_H_

#include <board.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <rtdbg.h>

#include "GlobalConfig.h"
#include "typedef.h"
#include "string.h"
#include "Gpio.h"

/* 协议头 */
#define             REGISTER_CODE               0xFA

/* 命令 */
#define             READ_MUTI                   0x03        //Read multiple registers
#define             WRITE_SINGLE                0X06        //Write single register
#define             WRITE_MUTI                  0x10        //Write multiple registers


#endif /* APPLICATIONS_UART_COMMAND_H_ */
