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


static rt_mutex_t recvUartMutex = RT_NULL;    //指向互斥量的指针
static rt_device_t serial;
static struct rt_messagequeue uartRecvMsg;    //串口接收数据消息队列
struct rt_messagequeue uartSendMsg;           //串口发送数据消息队列
extern struct rt_messagequeue ethMsg;         //接收网口数据消息队列

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
void sensorUart2TaskEntry(void* parameter)
{
    char recvEthBuf[30];              //接收以太网数据缓存
    struct rx_msg msg;                //接收串口数据结构体
    rt_err_t result;
    rt_err_t sendMsgRes;              //发送消息队列结果
    static char recvUartBuf[65];      //接收串口数据缓存，从数据结构体转化到接收缓存
    rt_uint32_t recvUartLength = 0;       //接收串口数据长度
    static u8 timeCnt = 0;

    /* 查找串口设备 */
    serial = rt_device_find("uart2");
    if (!serial)
    {
        rt_kprintf("find uart2 failed!\n");
    }

    /* 以 DMA 接收及轮询发送方式打开串口设备 */
    rt_device_open(serial, RT_DEVICE_FLAG_DMA_RX);
    /* 设置接收回调函数 */
    rt_device_set_rx_indicate(serial, uart_input);
    while (1)
    {
        rt_mutex_take(recvUartMutex, RT_WAITING_FOREVER);       //加锁保护
        timeCnt++;
        /* 清空接收串口数据结构体 */
        rt_memset(&msg, 0, sizeof(msg));
        /* 从串口消息队列中读取消息 */
        result = rt_mq_recv(&uartRecvMsg, &msg, sizeof(msg), RT_WAITING_NO);//Justin debug 需要将等待时间改为可以设置的
        if (result == RT_EOK)
        {
            /* 从串口读取数据*/
            recvUartLength = rt_device_read(msg.dev, 0, recvUartBuf, msg.size);
            recvUartBuf[recvUartLength] = '\0';
            /* 通过串口设备 serial 输出读取到的消息到以太网线程 */
            sendMsgRes = rt_mq_send(&uartSendMsg, recvUartBuf, recvUartLength);
            if(RT_EOK != sendMsgRes){
                //LOG_E("Uart send message to Ethernet Error");
            }else {
                //itoa(recvUartLength,recvUartBuf,4);//Justin debug
                rt_device_write(serial, 0, recvUartBuf, recvUartLength);//Justin debug
            }
        }

        /* 从以太网消息队列中接收消息 */
        if (rt_mq_recv(&ethMsg, &recvEthBuf[0], sizeof(recvEthBuf), RT_WAITING_NO) == RT_EOK)
        {
            rt_device_write(serial, 0, &recvEthBuf[0], (sizeof(recvEthBuf) - 1));
        }
        rt_mutex_release(recvUartMutex);                        //解锁

        rt_thread_mdelay(50);//Justin debug
    }
}

/**
 * @brief  : 传感器类串口线程
 * @para   : NULL
 * @author : Qiuyijie
 * @date   : 2022.01.17
 */
void sensorUart2TaskInit(void)
{
    rt_err_t threadStart = RT_NULL;
    static char recvMsgPool[256];
    static char sendMsgPool[256];


    /* 创建一个动态互斥量 */
    recvUartMutex = rt_mutex_create("dmutex", RT_IPC_FLAG_FIFO);
    if (recvUartMutex == RT_NULL)
    {
        LOG_E("create dynamic mutex failed.\n");
    }

    /* 初始化接收串口数据消息队列 */
    rt_mq_init(&uartRecvMsg, "uartrecv_msg",
               recvMsgPool,                     // 存放消息的缓冲区
               sizeof(struct rx_msg),           // 一条消息的最大长度
               sizeof(recvMsgPool),             // 存放消息的缓冲区大小
               RT_IPC_FLAG_FIFO);               // 如果有多个线程等待，按照先来先得到的方法分配消息

    /* 初始化发送串口数据消息队列 */
    rt_mq_init(&uartSendMsg, "uartsend_msg",
               sendMsgPool,                     // 存放消息的缓冲区
               30,                              // 一条消息的最大长度
               sizeof(sendMsgPool),             // 存放消息的缓冲区大小
               RT_IPC_FLAG_FIFO);               // 如果有多个线程等待，按照先来先得到的方法分配消息

    /* 创建串口 线程 */
    rt_thread_t thread = rt_thread_create("sensor task", sensorUart2TaskEntry, RT_NULL, 2048, 25, 10);

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
