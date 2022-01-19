/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-01-12     Qiuyijie     the first version
 */

#include "uart.h"


static rt_device_t serial;
static struct rt_messagequeue uartRecvMsg;
extern struct rt_messagequeue ethMsg;

/* 接收数据回调函数 */
static rt_err_t uart_input(rt_device_t dev, rt_size_t size)
{
    struct rx_msg msg;

    rt_err_t result;
    msg.dev = dev;
    msg.size = size;

    result = rt_mq_send(&uartRecvMsg, &msg, sizeof(msg));
    if ( result == -RT_EFULL)
    {
        /* 消息队列满 */
        rt_kprintf("message queue full！\n");
    }

    return result;
}

/**
 * @brief  : 传感器类串口线程入口
 * @para   : NULL
 * @author : Qiuyijie
 * @date   : 2022.01.12
 */
void SensorUart2TaskEntry(void* parameter)
{
    char recvEthBuf[30];              //接收以太网数据缓存
    struct rx_msg msg;                //接收串口数据结构体
    rt_err_t result;
    static char recvUartBuf[65];      //接收串口数据缓存，从数据结构体转化到接收缓存
    rt_uint32_t recvUartLength;       //接收串口数据长度

    /* 查找串口设备 */
    serial = rt_device_find("uart2");
    if (!serial)
    {
        rt_kprintf("find uart2 failed!\n");
    }

    /* 以中断接收及轮询发送方式打开串口设备 */
    rt_device_open(serial, RT_DEVICE_FLAG_INT_RX);
    /* 设置接收回调函数 */
    rt_device_set_rx_indicate(serial, uart_input);
    while (1)
    {
        /* 清空接收串口数据结构体 */
        rt_memset(&msg, 0, sizeof(msg));
        /* 从串口消息队列中读取消息 */
        result = rt_mq_recv(&uartRecvMsg, &msg, sizeof(msg), /*RT_WAITING_FOREVER*/RT_WAITING_NO);
        if (result == RT_EOK)
        {
            /* 从串口读取数据*/
            recvUartLength = rt_device_read(msg.dev, 0, recvUartBuf, msg.size);
            //recvUartBuf[recvUartLength] = '\0';
            /* 通过串口设备 serial 输出读取到的消息 */
            //rt_device_write(serial, 0, recvUartBuf, recvUartLength);
        }

        /* 从以太网消息队列中接收消息 */
        if (rt_mq_recv(&ethMsg, &recvEthBuf[0], sizeof(recvEthBuf), /*RT_WAITING_FOREVER*/RT_WAITING_NO) == RT_EOK)
        {
            rt_device_write(serial, 0, &recvEthBuf[0], (sizeof(recvEthBuf) - 1));
        }

        rt_thread_mdelay(50);
    }
}

/**
 * @brief  : 传感器类串口线程
 * @para   : NULL
 * @author : Qiuyijie
 * @date   : 2022.01.17
 */
void SensorUart2TaskInit(void)
{
    rt_err_t threadStart = RT_NULL;
    static char msg_pool[256];

    /* 初始化消息队列 */
    rt_mq_init(&uartRecvMsg, "uartrecv_msg",
               msg_pool,                 // 存放消息的缓冲区
               sizeof(struct rx_msg),    // 一条消息的最大长度
               sizeof(msg_pool),         // 存放消息的缓冲区大小
               RT_IPC_FLAG_FIFO);        // 如果有多个线程等待，按照先来先得到的方法分配消息

    /* 创建串口 线程 */
    rt_thread_t thread = rt_thread_create("sensor task", SensorUart2TaskEntry, RT_NULL, 1024, 25, 10);

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
