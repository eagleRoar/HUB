/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-01-12     Qiuyijie     the first version
 */

/*所有的设备类都需要在周期内发送询问状态等指令保持连接，设备的周期在10秒，从机询问周期要低于10秒，在6秒询问一次*/

#include "Uart.h"
#include "Ethernet.h"
#include "device.h"
#include "Command.h"
#include "InformationMonitor.h"
#include "UartDataLayer.h"
#include "UartBussiness.h"
#include "Sdcard.h"

type_monitor_t monitor;
struct rx_msg uart2_msg;                      //接收串口数据以及相关消息
struct rx_msg uart3_msg;                      //接收串口数据以及相关消息

static rt_mutex_t recvUartMutex = RT_NULL;    //指向互斥量的指针
extern struct sdCardState      sdCard;
/**
 * @brief  : 接收回调函数
 * @para   : dev   ：接收数据部分等
 *         : size  : 接收的数据长度
 * @author : Qiuyijie
 * @date   : 2022.01.12
 */
static rt_err_t Uart2_input(rt_device_t dev, rt_size_t size)
{
    u16 crc16 = 0x0000;

    /* 必须要等待从sd卡读取到的monitor 才能执行以下功能 */
    if (NO == sdCard.readInfo)
    {
        return RT_ERROR;
    }

    uart2_msg.dev = dev;
    uart2_msg.size = size;
    rt_device_read(uart2_msg.dev, 0, uart2_msg.data, uart2_msg.size);
    if(2 > size)
    {
        return RT_ERROR;
    }
    crc16 |= uart2_msg.data[uart2_msg.size-1];
    crc16 <<= 8;
    crc16 |= uart2_msg.data[uart2_msg.size-2];
    if(crc16 == usModbusRTU_CRC(uart2_msg.data, uart2_msg.size - 2))
    {
        uart2_msg.messageFlag = ON;

        return RT_EOK;
    }
    else
    {
        return RT_ERROR;
    }
}

static rt_err_t Uart3_input(rt_device_t dev, rt_size_t size)
{
    u16 crc16 = 0x0000;

    /* 必须要等待从sd卡读取到的monitor 才能执行以下功能 */
    if (NO == sdCard.readInfo)
    {
        return RT_ERROR;
    }

    uart3_msg.dev = dev;
    uart3_msg.size = size;
    rt_device_read(uart3_msg.dev, 0, uart3_msg.data, uart3_msg.size);
    if(2 > size)
    {
        return RT_ERROR;
    }
    crc16 |= uart3_msg.data[uart3_msg.size-1];
    crc16 <<= 8;
    crc16 |= uart3_msg.data[uart3_msg.size-2];
    if(crc16 == usModbusRTU_CRC(uart3_msg.data, uart3_msg.size - 2))
    {
        uart3_msg.messageFlag = ON;
        return RT_EOK;
    }
    else
    {
        return RT_ERROR;
    }
}

/**
 * @brief  : 传感器类串口线程入口
 * @para   : NULL
 * @author : Qiuyijie
 * @date   : 2022.01.12
 */
void SensorUart2TaskEntry(void* parameter)
{
    static u8 Timer1sTouch = OFF;
    static u8 Timer5sTouch = OFF;
    static u16 time1S = 0;
    static u16 time5S = 0;
    static rt_device_t uart2_serial;
    static rt_device_t uart3_serial;
    type_module_t module;

    /* 查找串口设备 */
    uart2_serial = rt_device_find(DEVICE_UART2);
    if (!uart2_serial)
    {
        LOG_E("find uart2 failed!");
    }
    /* 以 DMA 接收及轮询发送方式打开串口设备 */
    rt_device_open(uart2_serial, RT_DEVICE_FLAG_DMA_RX);
    /* 设置接收回调函数 */
    rt_device_set_rx_indicate(uart2_serial, Uart2_input);

    uart3_serial = rt_device_find(DEVICE_UART3);
    if(!uart3_serial)
    {
        LOG_E("find uart3 failed!");
    }
    rt_device_open(uart3_serial, RT_DEVICE_FLAG_DMA_RX);
    rt_device_set_rx_indicate(uart3_serial, Uart3_input);

    rt_memset(&monitor, 0, sizeof(type_monitor_t));

    while (1)
    {
        TimerTask(&time1S, 20, &Timer1sTouch);                      //1s定时任务
        TimerTask(&time5S, 100, &Timer5sTouch);                     //1s定时任务

        rt_mutex_take(recvUartMutex, RT_WAITING_FOREVER);           //加锁保护

        if(YES == sdCard.readInfo)                                  //必须要等待从sd卡读取到的monitor 才能执行以下功能
        {
            /* 50ms 事件 */
            {
                /* 接收串口2消息 */
                if(ON == uart2_msg.messageFlag)
                {
                    uart2_msg.messageFlag = OFF;
                    AnalyzeData(uart2_serial, &monitor, uart2_msg.data, uart2_msg.size);
                }

                /* 接收串口3消息 */
                if(ON == uart3_msg.messageFlag)
                {
                    uart3_msg.messageFlag = OFF;
                    /* 读取device设备 */ //后续要移动到uart2处理
                    AnalyzeData(uart3_serial, &monitor, uart3_msg.data, uart3_msg.size);
                }

                /* 监控模块的连接状态 */
                MonitorModuleConnect(&monitor, 50);
            }

            /* 1s 事件 */
            if(ON == Timer1sTouch)
            {
                /* 周期性1s向四合一模块发送询问指令 */
                for(int i = 0; i < monitor.monitorDeviceTable.deviceManageLength; i++)
                {
                    if(SENSOR_TYPE == monitor.monitorDeviceTable.deviceTable[i].s_or_d)
                    {
                        askSensorStorage(&(monitor.monitorDeviceTable.deviceTable[i]), uart2_serial, 0x04);
                    }
                }
            }
            /* 5s 事件 */
            if(ON == Timer5sTouch)
            {
                /* 控制设备 */
                for(int i = 0; i < monitor.monitorDeviceTable.deviceManageLength; i++)
                {
                    module = monitor.monitorDeviceTable.deviceTable[i];

                    ControlDeviceStorage(&module, uart3_serial, module.module_t[0].value, 0x00);//Justin debug 该控制为测试用
                }
            }
        }
        rt_mutex_release(recvUartMutex);                            //解锁
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

    /* 创建一个动态互斥锁 */
    recvUartMutex = rt_mutex_create("dmutex", RT_IPC_FLAG_FIFO);
    if (recvUartMutex == RT_NULL)
    {
        LOG_E("create dynamic mutex failed.\n");
    }

    /* 创建串口 线程 */
    rt_thread_t thread = rt_thread_create("sensor task", SensorUart2TaskEntry, RT_NULL, 1024*6, UART2_PRIORITY, 10);

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

type_monitor_t *GetMonitor(void)
{
    return &monitor;
}
