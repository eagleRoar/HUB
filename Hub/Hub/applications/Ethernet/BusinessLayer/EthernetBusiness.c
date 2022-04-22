/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-03-30     Administrator       the first version
 */
#include <EthernetBusiness.h>


static struct ethDeviceStruct *ethernetStruct = RT_NULL;     //主机相关信息

static int GetPort(void);
static void SetPort(int);
static char* GetIp(void);
static void SetIp(char *);
static u8 GetUdpConnectStatus(void);
static void SetUdpConnectStatus(u8);
static u8 GetUdpNotifyChange(void);
static void SetUdpNotifyChange(u8);
static u8 GetTcpConnectStatus(void);
static void SetTcpConnectStatus(u8);
static u8 GetTcpConnectTry(void);
static void SetTcpConnectTry(u8);
static void SetTcpRecvDataFlag(u8);
static u8 GetTcpRecvDataFlag(void);
static u8 GetEthLinkStatus(void);
static void SetEthLinkStatus(u8);
static u8 GetMasterAlive(void);
static void SetMasterAlive(u8);
static u8 IsNewEthernetConnect(char*, u16);

void InitEthernetStruct(void)
{
    /* 实例化ethernet对象 */
    if(RT_NULL == ethernetStruct)
    {
        ethernetStruct = rt_malloc(sizeof(struct ethDeviceStruct));
        rt_memset(ethernetStruct, OFF, sizeof(struct ethDeviceStruct));

        /* 注册函数 */
        ethernetStruct->IsNewEthernetConnect = IsNewEthernetConnect;
        ethernetStruct->SetMasterAlive = SetMasterAlive;
        ethernetStruct->GetMasterAlive = GetMasterAlive;
        ethernetStruct->SetethLinkStatus = SetEthLinkStatus;
        ethernetStruct->GetethLinkStatus = GetEthLinkStatus;
        ethernetStruct->SetIp = SetIp;
        ethernetStruct->GetIp = GetIp;
        ethernetStruct->SetPort = SetPort;
        ethernetStruct->GetPort = GetPort;
        ethernetStruct->tcp.SetConnectTry = SetTcpConnectTry;
        ethernetStruct->tcp.GetConnectTry = GetTcpConnectTry;
        ethernetStruct->tcp.SetConnectStatus = SetTcpConnectStatus;
        ethernetStruct->tcp.GetConnectStatus = GetTcpConnectStatus;
        ethernetStruct->tcp.SetRecvDataFlag = SetTcpRecvDataFlag;
        ethernetStruct->tcp.GetRecvDataFlag = GetTcpRecvDataFlag;
        ethernetStruct->udp.SetNotifyChange = SetUdpNotifyChange;
        ethernetStruct->udp.GetNotifyChange = GetUdpNotifyChange;
        ethernetStruct->udp.SetConnectStatus = SetUdpConnectStatus;
        ethernetStruct->udp.GetConnectStatus = GetUdpConnectStatus;
    }
}

static int GetPort(void)
{
    return ethernetStruct->port;
}

static void SetPort(int port)
{
    ethernetStruct->port = port;
}

static char* GetIp(void)
{
    return ethernetStruct->ip;
}

static void SetIp(char *ip)
{
    rt_memcpy(ethernetStruct->ip, ip, IP_SIZE);
}

/******************UDP START***************************************/
static u8 GetUdpConnectStatus(void)
{
    return ethernetStruct->udp.connectStatus;
}

static void SetUdpConnectStatus(u8 status)
{
    ethernetStruct->udp.connectStatus = status;
}

static u8 GetUdpNotifyChange(void)
{
    return ethernetStruct->udp.notifyChange;
}

static void SetUdpNotifyChange(u8 flag)
{
    ethernetStruct->udp.notifyChange = flag;
}
/******************UDP END*****************************************/

/******************TCP START***************************************/
static void SetTcpRecvDataFlag(u8 flag)
{
    ethernetStruct->tcp.recvDataFlag = flag;
}

static u8 GetTcpRecvDataFlag(void)
{
     return ethernetStruct->tcp.recvDataFlag;
}

static u8 GetTcpConnectStatus(void)
{
    return ethernetStruct->tcp.connectStatus;
}

static void SetTcpConnectStatus(u8 status)
{
    ethernetStruct->tcp.connectStatus = status;
}

static u8 GetTcpConnectTry(void)
{
    return ethernetStruct->tcp.connectTryFlag;
}

static void SetTcpConnectTry(u8 flag)
{
    ethernetStruct->tcp.connectTryFlag = flag;
}
/******************TCP   END***************************************/

static u8 GetEthLinkStatus(void)
{
    return ethernetStruct->ethLinkStatus;
}

static void SetEthLinkStatus(u8 status)
{
    ethernetStruct->ethLinkStatus = status;
}

static u8 GetMasterAlive(void)
{
    return ethernetStruct->masterAlive;
}

static void SetMasterAlive(u8 status)
{
    ethernetStruct->masterAlive = status;
}

/**
 *
 * @param ip   : 要对比的ip
 * @param port : 要对比的port
 * @return     : NO 结果一样, YES 对比结果不同
 */
static u8 IsNewEthernetConnect(char* ip, u16 port)
{
    if(!rt_memcmp(ethernetStruct->GetIp(), ip, IP_SIZE) &&
        (ethernetStruct->GetPort() == port))
    {
        return NO;
    }

    return YES;
}

/**
 * 提供实例化masteInformationStruct对象的接口，初始化该结构体内的指针函数
 * @return : masteInformationStruct对象
 */
struct ethDeviceStruct *GetEthernetStruct(void)
{
    return ethernetStruct;
}

/**
 * 删除masteInformationStruct对象指针,释放空间
 */
void DeleteEthernetStruct(void)
{
    if(RT_NULL != ethernetStruct)
    {
        rt_free(ethernetStruct);
        ethernetStruct = RT_NULL;
    }
}
