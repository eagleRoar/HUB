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

#include "Gpio.h"
#include "EthernetBusiness.h"
#include "Ethernet.h"
#include "Udp.h"
#include "netdev.h"

void TransmitSensorData(int , struct sockaddr *);

#endif /* APPLICATIONS_ETHERNET_BUSINESSLAYER_UDP_UDPPROGRAM_H_ */
