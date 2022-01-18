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

//static const char send_data[] = "This is TCP Client from RT-Thread."; /* 发送用到的数据 *///Justin debug 测试发送数据
/* 消息队列控制块 */
struct rt_messagequeue ethMsg;
/* 消息队列中用到的放置消息的内存池 */
static rt_uint8_t msg_pool[100];

/**
 * @brief  : 以太网线程入口
 * @para   : NULL
 * @author : Qiuyijie
 * @date   : 2022.01.17
 */
void EthernetTaskEntry(void* parameter)
{
    int ret;
    int result;
    char *recv_data;
    struct hostent *host;
    int sock, bytes_received;
    struct sockaddr_in server_addr;
    const char *url;
    int port;
    char *test1 = "169.254.100.218";
    char *test2 = "5000";

    char send_data[10];//Justin debug

    url = test1;//Justin debug 测试的主机IP地址
    port = strtoul(test2, 0, 10);//Justin debug 测试的主机端口

    /* 通过函数入口参数url获得host地址（如果是域名，会做域名解析） */
    host = gethostbyname(url);

    /* 分配用于存放接收数据的缓冲 */
    recv_data = rt_malloc(BUFSZ);
    if (recv_data == RT_NULL)
    {
        rt_kprintf("No memory\n");
        return;
    }

    /* 创建一个socket，类型是SOCKET_STREAM，TCP类型 */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        /* 创建socket失败 */
        rt_kprintf("Socket error\n");

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
        rt_kprintf("Connect fail!\n");
        closesocket(sock);

        /*释放接收缓冲 */
        rt_free(recv_data);
        return;
    }
    else
    {
        /* 连接成功 */
        rt_kprintf("Connect successful\n");
    }

    while (1)
    {
        /* 从sock连接中接收最大BUFSZ - 1字节数据 */
        bytes_received = recv(sock, recv_data, BUFSZ - 1, 0);
        if (bytes_received < 0)
        {
            /* 接收失败，关闭这个连接 */
            closesocket(sock);
            rt_kprintf("\nreceived error,close the socket.\r\n");

            /* 释放接收缓冲 */
            rt_free(recv_data);
            break;
        }
        else if (bytes_received == 0)
        {
            /* 默认 recv 为阻塞模式，此时收到0认为连接出错，关闭这个连接 */
            closesocket(sock);
            rt_kprintf("\nreceived error,close the socket.\r\n");

            /* 释放接收缓冲 */
            rt_free(recv_data);
            break;
        }

        /* 有接收到数据，把末端清零 */
        recv_data[bytes_received] = '\0';

        /* 发送消息到消息队列中 */
        result = rt_mq_send(&ethMsg, recv_data, strlen(recv_data));//Justin debug
        if (result != RT_EOK)
        {
            rt_kprintf("ethernet send message ERR\n");
        }

        if (strncmp(recv_data, "q", 1) == 0 || strncmp(recv_data, "Q", 1) == 0)
        {
            /* 如果是首字母是q或Q，关闭这个连接 */
            closesocket(sock);
            rt_kprintf("\n got a 'q' or 'Q',close the socket.\r\n");

            /* 释放接收缓冲 */
            rt_free(recv_data);
            break;
        }
        else
        {
            /* 在控制终端显示收到的数E据 */
            rt_kprintf("\nReceived data = %s ", recv_data);
        }

//        itoa(result,send_data,2);

        /* 发送数据到sock连接 */
        ret = send(sock, send_data, strlen(send_data), 0);
        if (ret < 0)
        {
            /* 接收失败，关闭这个连接 */
            closesocket(sock);
            rt_kprintf("\nsend error,close the socket.\r\n");

            rt_free(recv_data);
            break;
        }
        else if (ret == 0)
        {
            /* 打印send函数返回值为0的警告信息 */
            rt_kprintf("\n Send warning,send function return 0.\r\n");
        }
    }
}

/**
 * @brief  : 与主机以太网通讯线程
 * @para   : NULL
 * @author : Qiuyijie
 * @date   : 2022.01.17
 */
void EthernetTaskInit(void)
{
    rt_err_t threadStart = RT_NULL;
    rt_thread_t thread = RT_NULL;
    rt_err_t result = RT_NULL;

    /* 初始化消息队列 */
    result = rt_mq_init(&ethMsg,
                        "ethmessage",
                        &msg_pool[0],               /* 内存池指向msg_pool */
                        30/*1*/,                    /* 每个消息的大小是 100 字节 */
                        sizeof(msg_pool),           /* 内存池的大小是msg_pool的大小 */
                        RT_IPC_FLAG_FIFO);          /* 如果有多个线程等待，按照先来先得到的方法分配消息 */
    if (result != RT_EOK)
    {
        rt_kprintf("init message queue failed.\n");
    }

    /* 创建以太网线程 */
    thread = rt_thread_create("ethernet task", EthernetTaskEntry, RT_NULL, 2048, 20, 10);

    /* 如果线程创建成功则开始启动线程，否则提示线程创建失败 */
    if (RT_NULL != thread) {
        threadStart = rt_thread_startup(thread);
        if (RT_EOK != threadStart) {
            LOG_E("sensor task start failed");
        }
    } else {
        LOG_E("sensor task create failed");
    }
}

//MSH_CMD_EXPORT(EthernetTaskInit, a tcp client sample);//Justin debug
