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

#include <rtthread.h>
#include <rtdevice.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <rtdbg.h>
#include "GlobalConfig.h"
#include "typedef.h"
#include "EthernetBusiness.h"
#include "Ethernet.h"
#include "TcpClient.h"
#include "TcpPersistence.h"
#include "Informationmonitor.h"
#include "hub.h"

void SetIpAndPort(char *, int , struct ethDeviceStruct *);
rt_err_t notifyTcpAndUdpSocket(char *, int , struct  ethDeviceStruct*);
rt_err_t CheckPackageLegality(u8 *, u8);
rt_tcpclient_t* TcpClientInit(struct ethDeviceStruct *, rx_cb_t);
void SendMesgToMasterProgram(rt_tcpclient_t *);
void AnalyzeEtherData(rt_tcpclient_t *, type_package_t);

#endif /* APPLICATIONS_ETHERNET_BUSINESSLAYER_TCP_TCPPROGRAM_H_ */
