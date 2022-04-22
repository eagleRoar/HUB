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
#include <sys/select.h> /* 使用 dfs select 功能  */
#include <netdb.h>
#include <string.h>
#include <rtdbg.h>
#include <stdio.h>
#include <stdlib.h>

#include "GlobalConfig.h"
#include "typedef.h"
#include "netdev.h"
#include "Gpio.h"
#include "EthernetBusiness.h"

#define TC_TCPCLIENT_CLOSE (1 << 0)
#define TC_EXIT_THREAD (1 << 1)
#define TC_SWITCH_TX (1 << 2)
#define TC_SWITCH_RX (1 << 3)
#define STRCMP(a, R, b) (strcmp((a), (b)) R 0)


#define TEST_PORT                 9897
#define MASTER_PORT               9898
#define UDP_BROADCAST_PORT        /*TEST_PORT*/9899
#define RCV_ETH_BUFFSZ            512

struct packTop{
    u16 checkId;             //标识
    u16 length;              //总长度
    u16 crc;                 //CRC16
    u16 answer;              //应答
    u16 function;            //功能码
    u32 id;                  //发送者id
    u32 serialNum;           //序列号
};

struct packageEth
{
    struct packTop package_top;
    u16 buffer[RCV_ETH_BUFFSZ];
};


struct masterBroadInfo
{
    u32 time;
    u32 version;
};

enum
{
    NORMAL_TYPE = 0x00,
    BROADCAST_SERVER = 0x01,
    BROADCAST_CLIENT = 0x02,
};

enum
{
    SOCKET_OFF = 0x00,
    SOCKET_ON = 0x01,
};

enum
{
    LINKDOWN = 0x00,
    LINKUP = 0x01,
};

void TcpTaskEntry(void* parameter);
void UdpTaskEntry(void* parameter);
void EthernetTaskInit(void);

#endif /* APPLICATIONS_ETHERNET_H_ */
