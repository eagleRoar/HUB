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
#include "BleDataLayer.h"
#include "BleBusiness.h"
#include "InformationMonitor.h"

static struct UartMsg uartMsg;


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
    uartMsg.revFlg = YES;

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
    rt_err_t                result              = RT_ERROR;
    static u8               Timer1sTouch        = OFF;
    static u16              time1S              = 0;
    static rt_device_t      serial;
    struct bleState         bleSta;

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

    rt_memset(&uartMsg, 0, sizeof(struct UartMsg));
    rt_memset(&bleSta, 0, sizeof(struct bleState));
    ConnectToBle(serial);
    while (1)
    {
        time1S = TimerTask(&time1S, 20, &Timer1sTouch);

        /* 从串口消息队列中读取消息 */
        if(YES == uartMsg.revFlg)
        {

            uartMsg.revFlg = NO;
        }

        /* 1s任务 */
        if(ON == Timer1sTouch)
        {

        }

        rt_thread_mdelay(50);

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

    /* 创建串口 线程 */
    rt_thread_t thread = rt_thread_create("ble task", BleUart6TaskEntry, RT_NULL, 1024*2, UART6_PRIORITY, 10);
    /* 如果线程创建成功则开始启动线程，否则提示线程创建失败 */
    if (RT_NULL != thread) {
        threadStart = rt_thread_startup(thread);
        if (RT_EOK != threadStart) {
            LOG_E("ble task start failed");
        }
    } else {
        LOG_E("ble task create failed");
    }
}
