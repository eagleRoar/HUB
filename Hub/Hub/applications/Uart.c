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

/* 用于接收消息的信号量 */
static struct rt_semaphore rx_sem;
static rt_device_t serial;
extern struct rt_messagequeue ethMsg;

/* 接收数据回调函数 */
static rt_err_t uart_input(rt_device_t dev, rt_size_t size)
{
    /* 串口接收到数据后产生中断，调用此回调函数，然后发送接收信号量 */
    rt_sem_release(&rx_sem);

    return RT_EOK;
}

/**
 * @brief  : 传感器类串口线程入口
 * @para   : NULL
 * @author : Qiuyijie
 * @date   : 2022.01.12
 */
void SensorUart2TaskEntry(void* parameter)
{
//    char ch;
    char revBuf[30] ;

    /* 查找串口设备 */
    serial = rt_device_find("uart2");
    if (!serial)
    {
        rt_kprintf("find uart2 failed!\n");
    }

    /* 初始化信号量 */
    rt_sem_init(&rx_sem, "rx_sem", 0, RT_IPC_FLAG_FIFO);
    /* 以中断接收及轮询发送方式打开串口设备 */
    rt_device_open(serial, RT_DEVICE_FLAG_INT_RX);
    /* 设置接收回调函数 */
    rt_device_set_rx_indicate(serial, uart_input);

    while (1)
    {

        /* 从串口读取一个字节的数据，没有读取到则等待接收信号量 */
//        while (rt_device_read(serial, -1, &ch, 1) != 1)
//        {
//            /* 阻塞等待接收信号量，等到信号量后再次读取数据 */
//            rt_sem_take(&rx_sem, RT_WAITING_FOREVER);
//        }
//        rt_device_write(serial, 0, &ch, 1);

        /* 从消息队列中接收消息 */
        if (rt_mq_recv(&ethMsg, &revBuf[0], sizeof(revBuf), /*RT_WAITING_FOREVER*/RT_WAITING_NO) == RT_EOK)
        {
            rt_device_write(serial, 0, &revBuf[0], (sizeof(revBuf) - 1));
        }
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
