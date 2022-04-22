/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-03-22     Administrator       the first version
 */
#include "Ethernet.h"
#include "Udp.h"


/**
 * @brief   : 获取当前的Ip地址
 * @param   :
 * @return  : RT_ERROR 获取失败 ; RT_EOK 获取成功
 * @author  : Qiuyijie
 * @date    : 2022.03.03
 */
rt_err_t GetIPAddress(void)
{
    u8  ip[4];
    u32 ipAddress;

    ipAddress = netdev_default->ip_addr.addr;

    ip[3] = ipAddress;
    ip[2] = ipAddress >> 8;
    ip[1] = ipAddress >> 16;
    ip[0] = ipAddress >> 24;

    LOG_D("ip address = %d.%d.%d.%d",ip[3],ip[2],ip[1],ip[0]);

    return RT_EOK;
}

/**
 * 设置非阻塞
 */
void setnonblocking(int sockfd)
{
    int flag = fcntl(sockfd, F_GETFL, 0);

    if (flag < 0)
    {
        LOG_E("fcntl F_GETFL fail");
        return;
    }
    if (fcntl(sockfd, F_SETFL, flag | O_NONBLOCK) < 0)
    {
        LOG_E("fcntl F_SETFL fail");
    }
}

/**
 * @brief  : 注册socket
 * @param  : ip ip地址
 * @param  : port 端口
 * @param  : 初始化的socket
 * @return : RT_EOK 初始化成功; RT_ERROR 初始化失败
 */
rt_err_t UdpSetingInit(u8 isBroadcastType, char *ip, int port, struct sockaddr_in *server_addr, int *sock)
{
    int optval = 1;
    struct hostent *host;           //主机

    if(NORMAL_TYPE == isBroadcastType)    //正常类型
    {
        /* 通过函数入口参数url获得host地址（如果是域名，会做域名解析） */
        host = (struct hostent *) gethostbyname((const char *)ip);
    }

    /* 创建一个socket，类型是SOCK_DGRAM，UDP类型 */
    if ((*sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        LOG_E("udp Socket error");
        return RT_ERROR;
    }

    setnonblocking(*sock);    //设置成非阻塞

    /* 初始化预连接的服务端地址 */
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(port);
    if((BROADCAST_SERVER == isBroadcastType) || (BROADCAST_CLIENT == isBroadcastType))
    {
        server_addr->sin_addr.s_addr = INADDR_ANY;
    }
    else if(NORMAL_TYPE == isBroadcastType)
    {
        server_addr->sin_addr = *((struct in_addr *)host->h_addr);
    }
    rt_memset(&(*server_addr->sin_zero), 0, sizeof(*server_addr->sin_zero));

    if(BROADCAST_SERVER == isBroadcastType)
    {
        /* 是否是广播server类型,只有广播server类型需要执行以下动作 */
        /* 绑定socket到服务端地址 */
        if (bind(*sock, (struct sockaddr *)server_addr,sizeof(struct sockaddr)) == -1)
        {
            /* 绑定地址失败 */
            return RT_ERROR;
        }
        setsockopt(*sock,SOL_SOCKET,SO_BROADCAST,( void *)&optval,sizeof(optval));
    }

    return RT_EOK;
}

/**
 * @brief : udp    发送数据
 * @param : socket 要发送的socket
 * @param : buffer 要发送的数据
 * @param : size   要发送的数据长度
 * @return: 是否发送成功 RT_OK 成功; RT_ERROR 失败;
 */
rt_err_t UdpSendMessage(int socket, struct sockaddr *server_addr, void *buffer, size_t size)
{
    int result = 0;

    /* 发送数据到sock连接 */
    result = sendto(socket, buffer, size, 0, server_addr, sizeof(struct sockaddr));
    if (result <= 0)
    {
        /* 接收失败，关闭这个连接 */
        //LOG_E("send error,close the socket");
        return RT_ERROR;
    }

    return RT_EOK;
}

