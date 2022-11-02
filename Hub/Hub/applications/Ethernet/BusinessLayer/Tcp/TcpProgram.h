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

//#define     HEAD_CODE       0xA5A55A5A//"abcd"      //该头部标识符需要修改

struct ethPageHead
{
    /*char*/u8      head_code[4];
    u16       length;
};

typedef     struct ethPageHead  eth_page_head;

//网络包
struct ethPage
{
    eth_page_head   head;
    char*           data;
};

typedef     struct ethPage      eth_page_t;

rt_err_t ConnectToSever(int *, char *, uint32_t);
rt_err_t TcpSendMsg(int *, u8 *, u16);
rt_err_t TcpRecvMsg(int *, u8 *, u16, int *);
void SetIpAndPort(char *, int , struct ethDeviceStruct *);
rt_err_t notifyTcpAndUdpSocket(char *, int , struct  ethDeviceStruct*);
void analyzeTcpData(char *, u16);
void splitJointData(char *, u16);
void changDataToEthPage(eth_page_t *, u16);
u16 readePageLength(char *, u16);
#endif /* APPLICATIONS_ETHERNET_BUSINESSLAYER_TCP_TCPPROGRAM_H_ */
