/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-15     Administrator       the first version
 */
#include "TcpProgram.h"
#include "Udp.h"
/**
 * 连接tcp
 * flg 是否是阻塞
 * @return RT_ROK 成功 RT_ERROR 失败
 */
rt_err_t ConnectToSever(int *sock, char *ip, uint32_t port)
{
    rt_err_t            ret                         = RT_EOK;
    struct hostent      *host;
    struct sockaddr_in  server_addr;

    host = gethostbyname(ip);
    /* 创建一个socket，类型是SOCKET_STREAM，TCP类型 */
    if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        ret = RT_ERROR;
    }

    /* 初始化预连接的服务端地址 */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr = *((struct in_addr *)host->h_addr);
    rt_memset(&(server_addr.sin_zero), 0, sizeof(server_addr.sin_zero));

    /* 连接到服务端 */
    if (connect(*sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
    {
        closesocket(*sock);
        *sock = 0;
        ret = RT_ERROR;
        LOG_W("ConnectToSever close sock");
    }
    return ret;
}

rt_err_t TcpRecvMsg(int *sock, u8 *buff, u16 size, int *recLen)
{
    rt_err_t ret = RT_EOK;
    int bytes_received = 0;

    if(0 != *sock)
    {
        bytes_received = recv(*sock, buff, size, 0);
        *recLen = bytes_received;
        if (bytes_received <= 0)
        {
            closesocket(*sock);
            *sock = 0;
            ret = RT_ERROR;
            LOG_W("TcpRecvMsg err, close sock");
        }
    }

    return ret;
}

rt_err_t TcpSendMsg(int *sock, u8 *buff, u16 size)
{
    rt_err_t ret = RT_EOK;

    if(0 != *sock)
    {
        LOG_D("TcpSendMsg sock = %d",*sock);
        if(send(*sock, buff, size, 0) < 0)
        {
            closesocket(*sock);
            *sock = 0;
            ret = RT_ERROR;
            LOG_W("TcpSendMsg close sock");
        }
    }

    return ret;
}

void SetIpAndPort(char *newIp, int newPort, struct ethDeviceStruct *masterInfo)
{
    masterInfo->SetIp(newIp);
    masterInfo->SetPort(newPort);
}

rt_err_t notifyTcpAndUdpSocket(char *newIp, int newPort, struct ethDeviceStruct *masterInfo)
{
    if(RT_NULL == masterInfo)
    {
        return RT_ERROR;
    }

    /* 设置新的Ip和port */
    SetIpAndPort(newIp, newPort, masterInfo);

    //masterInfo->tcp.SetConnectTry(ON);    //开启尝试申请开启TcpClient任务

    return RT_EOK;
}

/* 判断网络数据包的合理性 */
//rt_err_t CheckPackageLegality(u8 *buffer, u16 length)
//{
//    u16 res;
//    type_package_t package;
//
//    if((sizeof(struct packTop) > length) ||
//       (sizeof(struct packTop) + RCV_ETH_BUFFSZ < length))
//    {
//        /* 包头格式就已经18个数字 */
//        LOG_E("recv length = %d",length);
//        return RT_ERROR;
//    }
//
//    rt_memcpy(&package, buffer, length);
//
//    if(length != package.package_top.length)
//    {
//        LOG_E("recv length = %d,pack length = %d",length,package.package_top.length);
//        return RT_ERROR;
//    }
//
//    /* 验证CRC16 */
//    res = CRC16((u16 *)&package+3, package.package_top.length/2-3, 0);
//
//    if(res != package.package_top.crc)
//    {
//        LOG_E("crc res = %x, pack crc = %x",res,package.package_top.crc);
//
//        return RT_ERROR;
//    }
//
//
//    return RT_EOK;
//}
