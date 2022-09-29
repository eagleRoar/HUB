/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-03-30     Administrator       the first version
 */

#include "Gpio.h"
#include "InformationMonitor.h"

#ifndef APPLICATIONS_ETHERNET_BUSINESSLAYER_ETHERNETBUSSINESS_H_
#define APPLICATIONS_ETHERNET_BUSINESSLAYER_ETHERNETBUSSINESS_H_

#define IP_SIZE 20                      //主机ip长度

/* 存储Master主机的相关信息 */
struct ethDeviceStruct
{

    u8                          (*IsNewEthernetConnect)(char*, u16);
    void                        (*SetMasterAlive)(u8);
    u8                          (*GetMasterAlive)(void);
    void                        (*SetethLinkStatus)(u8);
    u8                          (*GetethLinkStatus)(void);
    void                        (*SetIp)(char*);
    char*                       (*GetIp)(void);
    void                        (*SetPort)(int);
    int                         (*GetPort)(void);
    struct tcpClientStruct
    {
        void                        (*SetConnectTry)(u8);
        u8                          (*GetConnectTry)(void);
        void                        (*SetConnectStatus)(u8);
        u8                          (*GetConnectStatus)(void);
        void                        (*SetRecvDataFlag)(u8);
        u8                          (*GetRecvDataFlag)(void);

        u8 connectTryFlag;              //尝试连接标志位
        u8 connectStatus;               //连接状态
        u8 recvDataFlag;                //接收到数据的状态
    }tcp;
    struct udpStruct
    {
        void                        (*SetConnectStatus)(u8);
        u8                          (*GetConnectStatus)(void);

        u8 notifyChange;                //通知socket需要更新
        u8 connectStatus;               //socket状态
    }udp;

    u8     ethLinkStatus;               //以太网的网口连接状态
    u8     masterAlive;                 //与主机的连接状态
    char   ip[IP_SIZE];
    int    port;
};


void InitEthernetStruct(void);
struct ethDeviceStruct *GetEthernetStruct(void);
void DeleteEthernetStruct(void);


#endif /* APPLICATIONS_ETHERNET_BUSINESSLAYER_ETHERNETBUSSINESS_H_ */
