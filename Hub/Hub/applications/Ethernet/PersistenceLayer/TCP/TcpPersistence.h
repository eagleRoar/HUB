/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-01     Administrator       the first version
 */

#ifndef APPLICATIONS_ETHERNET_PERSISTENCELAYER_TCP_TCPPERSISTENCE_H_
#define APPLICATIONS_ETHERNET_PERSISTENCELAYER_TCP_TCPPERSISTENCE_H_


#include <rtthread.h>
#include <sys/socket.h> /* 使用BSD socket，需要包含socket.h头文件 */
#include <sys/select.h> /* 使用 dfs select 功能  */
#include <netdb.h>
#include <string.h>
#include <rtdbg.h>
#include <stdio.h>
#include <stdlib.h>

#include "GlobalConfig.h"
#include "InformationMonitor.h"
#include "typedef.h"
#include "netdev.h"
#include "Ethernet.h"
#include "Uart.h"
#include "hub.h"
#include "Sensor.h"
#include "Device.h"
#include "TcpClient.h"

void RegisterHub(rt_tcpclient_t *);
void RegisterModule(rt_tcpclient_t *);

#endif /* APPLICATIONS_ETHERNET_PERSISTENCELAYER_TCP_TCPPERSISTENCE_H_ */
