/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-01-17     Qiuyijie     the first version
 */
#ifndef APPLICATIONS_ETHERNET_H_
#define APPLICATIONS_ETHERNET_H_

#include <rtthread.h>
#include <sys/socket.h> /* 使用BSD socket，需要包含socket.h头文件 */
#include <netdb.h>
#include <string.h>
#include <finsh.h>
#include <rtdbg.h>
#include "typedef.h"

#define BUFSZ   1024


void TcpTaskInit(void);
void TcpTaskEntry(void* parameter);
void UdpTaskInit(void);
void UdpTaskEntry(void* parameter);

#endif /* APPLICATIONS_ETHERNET_H_ */
