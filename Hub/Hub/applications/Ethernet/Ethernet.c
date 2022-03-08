/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-01-17     Qiuyijie     the first version
 */
#include "Ethernet.h"

struct rt_messagequeue ethMsg;                 //消息队列控制块
extern struct rt_messagequeue uartSendMsg;

rt_uint8_t fileBuffer[10];


/**
 * @brief  : 以太网线程入口,TCP协议
 * @para   : NULL
 * @author : Qiuyijie
 * @date   : 2022.01.17
 */
void TcpTaskEntry(void* parameter)
{
    rt_err_t result;
    char *recv_data;
    struct hostent *host;
    int sock, bytes_received;
    struct sockaddr_in server_addr;
    const char *url;
    int port;
    char *test1 = "169.254.100.218";
    char *test2 = "5000";
    //static int timeCnt = 0;

    url = test1;
    port = strtoul(test2, 0, 10);

    /* 通过函数入口参数url获得host地址（如果是域名，会做域名解析） */
    host = gethostbyname(url);

    /* 分配用于存放接收数据的缓冲 */
    recv_data = rt_malloc(BUFSZ);
    if (recv_data == RT_NULL)
    {
        LOG_E("No memory");
        return;
    }

    /* 创建一个socket，类型是SOCKET_STREAM，TCP类型 */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        /* 创建socket失败 */
        LOG_E("Socket error");

        /* 释放接收缓冲 */
        rt_free(recv_data);
        return;
    }

    /* 初始化预连接的服务端地址 */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr = *((struct in_addr *)host->h_addr);
    rt_memset(&(server_addr.sin_zero), 0, sizeof(server_addr.sin_zero));

    /* 连接到服务端 */
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
    {
        /* 连接失败 */
        LOG_E("Connect fail!");
        closesocket(sock);

        /*释放接收缓冲 */
        rt_free(recv_data);
        return;

    }
    else
    {
        /* 连接成功 */
        LOG_I("Connect successful");
    }

    while (1)
    {
        /* 从sock连接中接收最大BUFSZ - 1字节数据 */
        //Justin Question ：为什么会阻塞
        bytes_received = recv(sock, recv_data, BUFSZ - 1, 0);
        if (bytes_received < 0)
        {
            /* 接收失败，关闭这个连接 */
            closesocket(sock);
            LOG_E("received error,close the socket.");

            /* 释放接收缓冲 */
            rt_free(recv_data);
            break;
        }
        else if (bytes_received == 0)
        {
            /* 默认 recv 为阻塞模式，此时收到0认为连接出错，关闭这个连接 */
            closesocket(sock);
            LOG_E("received error,close the socket.");

            /* 释放接收缓冲 */
            rt_free(recv_data);
            break;
        }
        /* 有接收到数据，把末端清零 */
        recv_data[bytes_received] = '\0';

        /* 发送消息到消息队列中 */
        result = rt_mq_send(&ethMsg, recv_data, strlen(recv_data));
        if (result != RT_EOK)
        {
            LOG_E("ethernet send message ERR");
        }

        rt_thread_mdelay(50);
    }
}

/**
 * @brief  : 与主机以太网通讯线程,TCP协议
 * @para   : NULL
 * @author : Qiuyijie
 * @date   : 2022.01.17
 */
void TcpTaskInit(void)
{
    rt_err_t threadStart = RT_NULL;
    rt_thread_t thread = RT_NULL;
    rt_err_t result = RT_NULL;
    static rt_uint8_t msg_pool[100];    //消息队列中用到的放置消息的内存池

    /* 初始化消息队列 */
    result = rt_mq_init(&ethMsg,
                        "ethmessage",
                        &msg_pool[0],               /* 内存池指向msg_pool */
                        30,                         /* 每个消息的大小是 30 字节 */
                        sizeof(msg_pool),           /* 内存池的大小是msg_pool的大小 */
                        RT_IPC_FLAG_FIFO);          /* 如果有多个线程等待，按照先来先得到的方法分配消息 */
    if (result != RT_EOK)
    {
        LOG_E("init message queue failed.");
    }

    /* 创建以太网线程 */
    thread = rt_thread_create("ethernet task", TcpTaskEntry, RT_NULL, 2048, 20, 10);

    /* 如果线程创建成功则开始启动线程，否则提示线程创建失败 */
    if (RT_NULL != thread) {
        threadStart = rt_thread_startup(thread);
        if (RT_EOK != threadStart) {
            LOG_E("tcp task start failed");
        }
    } else {
        LOG_E("tcp task create failed");
    }
}

/**
 * @brief   : 获取当前的Ip地址
 * @param   :
 * @return  : RT_ERROR 获取失败 ; RT_EOK 获取成功
 * @author  : Qiuyijie
 * @date    : 2022.03.03
 */
rt_err_t getIPAddress(void)
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
 * @brief  : 与主机以太网通讯线程入口,UDP协议
 * @param   : NULL
 * @author : Qiuyijie
 * @date   : 2022.01.19
 */
void UdpTaskEntry(void* parameter)
{
    rt_err_t result;
    int sock, port;
    struct hostent *host;
    struct sockaddr_in server_addr;
    const char *url;
    char recvUartBuf[65];
    static int timeCnt = 0;

    url = "192.168.0.69";
    port = strtoul("5000", 0, 10);

    /* 通过函数入口参数url获得host地址（如果是域名，会做域名解析） */
    host = (struct hostent *) gethostbyname(url);

    /* 创建一个socket，类型是SOCK_DGRAM，UDP类型 */
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        LOG_E("Socket error");
        return;
    }

    /* 初始化预连接的服务端地址 */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr = *((struct in_addr *)host->h_addr);
    rt_memset(&(server_addr.sin_zero), 0, sizeof(server_addr.sin_zero));

    /* 总计发送count次数据 */
    while (1)
    {
        timeCnt++;

        /* 接收串口发送过来的消息队列 */
        result = rt_mq_recv(&uartSendMsg, recvUartBuf, sizeof(recvUartBuf), RT_WAITING_NO);

        if(RT_EOK == result)
        {
            sendto(sock, recvUartBuf, /*sizeof(recvUartBuf)*/8, 0,
                  (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
        }

        //Justin debug
        if(0 == (timeCnt % 20))
        {
            sendto(sock, fileBuffer, 10, 0,
                              (struct sockaddr *)&server_addr, sizeof(struct sockaddr));

            //getIPAddress();
        }

        /* 线程休眠一段时间 */
        rt_thread_mdelay(50);
    }
    /* 关闭这个socket */
    closesocket(sock);
}

/**
 * @brief  : 与主机以太网通讯线程,UDP协议
 * @para   : NULL
 * @author : Qiuyijie
 * @date   : 2022.01.19
 */
void UdpTaskInit(void)
{
    rt_err_t threadStart = RT_NULL;
    rt_thread_t thread = RT_NULL;

    /* 创建以太网,UDP线程 */
    thread = rt_thread_create("ethernet task", UdpTaskEntry, RT_NULL, 2048, 22, 10);

    /* 如果线程创建成功则开始启动线程，否则提示线程创建失败 */
    if (RT_NULL != thread) {
       threadStart = rt_thread_startup(thread);
       if (RT_EOK != threadStart) {
           LOG_E("udp task start failed");
       }
    } else {
       LOG_E("udp task create failed");
    }
}


