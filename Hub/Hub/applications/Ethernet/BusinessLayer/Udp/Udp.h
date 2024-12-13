/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-03-22     Administrator       the first version
 */
#ifndef APPLICATIONS_ETHERNET_BUSINESSLAYER_UDP_H_
#define APPLICATIONS_ETHERNET_BUSINESSLAYER_UDP_H_

#include <rtthread.h>
#include <string.h>
#include <rtdbg.h>

#include "GlobalConfig.h"
#include "typedef.h"

rt_err_t GetIPAddress(u8 *ip);
void SetUdpSocketStatus(u8);
u8 GetUdpSocketStatus(void);
void SetUdpNotifySocketChange(u8);
u8 GetUdpNotifySocketChange(void);
void setnonblocking(int sockfd);
rt_err_t UdpSetingInit(u8, char *, int, struct sockaddr_in *, int *);
rt_err_t UdpSendMessage(int, struct sockaddr *, void *, size_t);
void DestoryUdpSocket( int);

#endif /* APPLICATIONS_ETHERNET_BUSINESSLAYER_UDP_H_ */
