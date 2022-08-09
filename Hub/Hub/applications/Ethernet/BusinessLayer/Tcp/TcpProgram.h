/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-15     Administrator       the first version
 */
#ifndef APPLICATIONS_ETHERNET_BUSINESSLAYER_TCP_TCPPROGRAM_H_
#define APPLICATIONS_ETHERNET_BUSINESSLAYER_TCP_TCPPROGRAM_H_

#include "Gpio.h"
#include "EthernetBusiness.h"
#include "Ethernet.h"
#include "Informationmonitor.h"

rt_err_t ConnectToSever(int *, char *, uint32_t);
rt_err_t TcpSendMsg(int *, u8 *, u16);
rt_err_t TcpRecvMsg(int *, u8 *, u16, int *);
void SetIpAndPort(char *, int , struct ethDeviceStruct *);
rt_err_t notifyTcpAndUdpSocket(char *, int , struct  ethDeviceStruct*);
//rt_err_t CheckPackageLegality(u8 *, u16);

#endif /* APPLICATIONS_ETHERNET_BUSINESSLAYER_TCP_TCPPROGRAM_H_ */
