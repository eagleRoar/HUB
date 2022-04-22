/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-22     Administrator       the first version
 */
#ifndef APPLICATIONS_ETHERNET_BUSINESSLAYER_UDP_UDPPROGRAM_H_
#define APPLICATIONS_ETHERNET_BUSINESSLAYER_UDP_UDPPROGRAM_H_

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
#include "Udp.h"
#include "hub.h"
#include "netdev.h"

void TransmitSensorData(int , struct sockaddr *);

#endif /* APPLICATIONS_ETHERNET_BUSINESSLAYER_UDP_UDPPROGRAM_H_ */
