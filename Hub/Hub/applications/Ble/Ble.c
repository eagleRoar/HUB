/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-01-25     Administrator       the first version
 */
#include "Ble.h"
#include "Uart.h"
#include "InformationMonitor.h"

#define  AT_ENTER       "+++a"
#define  TEST_TO_BLE    "AT+HELLO?"
#define  SET_NAME       "AT+NAME=Hub_Justin"
#define  ASK_NAME       "AT+NAME?"
#define  ENTER_TO_UART  "AT+ENTM"

static rt_mutex_t recvBleMutex = RT_NULL;    //指向互斥量的指针
static rt_device_t serial;
static struct UartMsg uartMsg;

type_package_t      recvPack;

/**
 * @brief  : 接收回调函数
 * @para   : dev   ：接收数据部分等
 *         : size  : 接收的数据长度
 *         : msg   : 返回的结构体
 * @author : Qiuyijie
 * @date   : 2022.03.08
 */
static rt_err_t Uart_input(rt_device_t dev, rt_size_t size)
{
    uartMsg.dev = dev;
    uartMsg.size = size;
    uartMsg.revFlg = RT_TRUE;

    return RT_EOK;
}

/*
 * @brief  : 蓝牙处理线程入口
 * @para   : NULL
 * @author : Qiuyijie
 * @date   : 2022.01.25
 */
void BleUart6TaskEntry(void* parameter)
{
    u8              temp                = 0x00;
    char            sendUartBuf[100]    = "+++a", recvUartBuf[512];
    rt_err_t        result              = RT_ERROR;
    static u8       Timer1sTouch        = OFF;
    static u16      time1S              = 0;
    static u8       testFlag            = 0;

    /* 查找串口设备 */
    serial = rt_device_find("uart6");
    if (!serial)
    {
        LOG_E("find uart6 failed!");
    }

    /* 以 DMA 接收及轮询发送方式打开串口设备 */
    result = rt_device_open(serial, RT_DEVICE_FLAG_DMA_RX);
    if(RT_EOK == result)
    {
        LOG_D("open uart to ble successful");
    }
    else
    {
        LOG_E("open uart to ble fail");
    }

    /* 设置接收回调函数 */
    result = rt_device_set_rx_indicate(serial, Uart_input);
    if(RT_EOK == result)
    {
        LOG_D("set uart6 successful");
    }
    else
    {
        LOG_E("set uart6 fail");
    }

    while (1)
    {
        rt_mutex_take(recvBleMutex, RT_WAITING_FOREVER);       //加锁保护
        time1S = TimerTask(&time1S, 20, &Timer1sTouch);

        /* 从串口消息队列中读取消息 */
        if(RT_TRUE == uartMsg.revFlg)
        {
            if(BLE_BUFFER_SIZE >= uartMsg.size)
            {
                rt_device_read(uartMsg.dev, 0, &recvPack, uartMsg.size);
                AnalyzePack(&recvPack);
            }
            else
            {
                LOG_E("recv ble buffer is too large");
            }
//            rt_memset(recvUartBuf, 0, 20);
//            rt_device_read(uartMsg.dev, 0, recvUartBuf, uartMsg.size);
//
//
//            if(0 == testFlag)
//            {
//                if(0 == rt_memcmp(recvUartBuf, "a+ok", strlen("a+ok")))
//                {
//                    testFlag = 1;
//                    LOG_D("recv Ok");
//                }
//                else
//                {
//                    LOG_D("recv err");
//                }
//            }
//
//            LOG_D("ble recv = %s --------",recvUartBuf);

            uartMsg.revFlg = RT_FALSE;
        }

        /* 1s任务 */
        if(ON == Timer1sTouch)
        {
            /* 与蓝牙打招呼 */
            if(0 == testFlag)
            {
                rt_memset(sendUartBuf, 0, 20);
                temp = strlen(AT_ENTER);
                rt_memcpy(sendUartBuf, AT_ENTER, temp);
                rt_device_write(serial, 0, sendUartBuf, temp);

            }
            else if(1 == testFlag)
            {
                temp = strlen(SET_NAME);
                rt_memcpy(sendUartBuf, SET_NAME, temp);
                sendUartBuf[temp] = 0x0d;
                sendUartBuf[temp+1] = 0x0a;
                rt_device_write(serial, 0, sendUartBuf, temp+2);
                testFlag = 2;
            }
            else if(2 == testFlag)
            {
                rt_memset(sendUartBuf, 0, 20);
                temp = strlen(ASK_NAME);
                rt_memcpy(sendUartBuf, ASK_NAME, temp);
                sendUartBuf[temp] = 0x0d;
                sendUartBuf[temp+1] = 0x0a;
                rt_device_write(serial, 0, sendUartBuf, temp+2);
                testFlag = 3;
            }
            else if (3 == testFlag)
            {
                rt_memset(sendUartBuf, 0, 20);
                temp = strlen(ENTER_TO_UART);
                rt_memcpy(sendUartBuf, ENTER_TO_UART, temp);
                sendUartBuf[temp] = 0x0d;
                sendUartBuf[temp+1] = 0x0a;
                rt_device_write(serial, 0, sendUartBuf, temp+2);
                testFlag = 4;
            }
        }

        rt_thread_mdelay(50);
        rt_mutex_release(recvBleMutex);                        //解锁

    }
}

/*
 * @brief  : 蓝牙处理线程
 * @para   : NULL
 * @author : Qiuyijie
 * @date   : 2022.01.25
 */
void BleUart6TaskInit(void)
{
    rt_err_t threadStart = RT_NULL;

    /* 创建一个动态互斥量 */
    recvBleMutex = rt_mutex_create("ble_mutex", RT_IPC_FLAG_FIFO);
    if (recvBleMutex == RT_NULL)
    {
        LOG_E("create dynamic mutex failed.\n");
    }

    /* 创建串口 线程 */
    rt_thread_t thread = rt_thread_create("ble task", BleUart6TaskEntry, RT_NULL, 1024, UART6_PRIORITY, 10);
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
