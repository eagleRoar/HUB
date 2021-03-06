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
#include "Command.h"
#include "InformationMonitor.h"
#include "UartDataLayer.h"
#include "UartBussiness.h"
#include "Sdcard.h"
#include "CloudProtocol.h"
#include "Module.h"

type_monitor_t monitor;
struct rx_msg uart1_msg;                      //接收串口数据以及相关消息
struct rx_msg uart2_msg;                      //接收串口数据以及相关消息
struct rx_msg uart3_msg;                      //接收串口数据以及相关消息

extern  struct sdCardState      sdCard;
extern  type_sys_time           sys_time;
extern  sys_set_t               sys_set;
/**
 * @brief  : 接收回调函数
 * @para   : dev   ：接收数据部分等
 *         : size  : 接收的数据长度
 * @author : Qiuyijie
 * @date   : 2022.01.12
 */
static rt_err_t Uart1_input(rt_device_t dev, rt_size_t size)
{
    u16 crc16 = 0x0000;

    /* 必须要等待从sd卡读取到的monitor 才能执行以下功能 */
    if (NO == sdCard.readInfo)
    {
        return RT_ERROR;
    }

    uart1_msg.dev = dev;
    uart1_msg.size = size;
    rt_device_read(uart1_msg.dev, 0, uart1_msg.data, uart1_msg.size);
    if(2 > size)
    {
        return RT_ERROR;
    }
    crc16 |= uart1_msg.data[uart1_msg.size-1];
    crc16 <<= 8;
    crc16 |= uart1_msg.data[uart1_msg.size-2];
    if(crc16 == usModbusRTU_CRC(uart1_msg.data, uart1_msg.size - 2))
    {
        uart1_msg.messageFlag = ON;
        return RT_EOK;
    }
    else
    {
        return RT_ERROR;
    }
}

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
 */
void SensorUart2TaskEntry(void* parameter)
{
    static      u8              Timer1sTouch    = OFF;
    static      u8              Timer5sTouch    = OFF;
    static      u16             time1S = 0;
    static      u16             time5S = 0;
    static      rt_device_t     uart1_serial;
    static      rt_device_t     uart2_serial;
    static      rt_device_t     uart3_serial;
    static      u8              device_start    = 0;
    static      u8              sensor_start    = 0;
    static      u8              line_start      = 0;
    static      type_sys_time   sys_time_pre;

    rt_memset((u8 *)&sys_time_pre, 0, sizeof(type_sys_time));
    initConnectState();

    /* 查找串口设备 */
    uart1_serial = rt_device_find(DEVICE_UART1);
    rt_device_open(uart1_serial, RT_DEVICE_FLAG_DMA_RX);
    rt_device_set_rx_indicate(uart1_serial, Uart1_input);

    uart2_serial = rt_device_find(DEVICE_UART2);
    rt_device_open(uart2_serial, RT_DEVICE_FLAG_DMA_RX);
    rt_device_set_rx_indicate(uart2_serial, Uart2_input);

    uart3_serial = rt_device_find(DEVICE_UART3);
    rt_device_open(uart3_serial, RT_DEVICE_FLAG_DMA_RX);
    rt_device_set_rx_indicate(uart3_serial, Uart3_input);
    while (1)
    {
        time1S = TimerTask(&time1S, 1000/UART_PERIOD, &Timer1sTouch);                       //1s定时任务
        time5S = TimerTask(&time5S, 5000/UART_PERIOD, &Timer5sTouch);                       //1s定时任务

        if(YES == sdCard.readInfo)                                  //必须要等待从sd卡读取到的monitor 才能执行以下功能
        {
            /* 50ms 事件 */
            {
                if(ON == uart1_msg.messageFlag)
                {
                    uart1_msg.messageFlag = OFF;
                    AnalyzeData(uart1_serial, &monitor, uart1_msg.data, uart1_msg.size);
                }
                else
                {
                    if(1 == sensor_start)
                    {
                        if(YES == askSensorStorage(&monitor, uart1_serial))
                        {
                            sensor_start = 0;
                        }
                    }
                }

                if(ON == uart2_msg.messageFlag)
                {
                    uart2_msg.messageFlag = OFF;
                    AnalyzeData(uart2_serial, &monitor, uart2_msg.data, uart2_msg.size);
                }
                else
                {
                    if(1 == device_start)
                    {
                        if(YES == askDeviceHeart(&monitor, uart2_serial))
                        {
                            device_start = 0;
                        }
                    }
                }

                if(ON == uart3_msg.messageFlag)//Justin debug 2022.07.15
                {
                    uart3_msg.messageFlag = OFF;
                    AnalyzeData(uart3_serial, &monitor, uart3_msg.data, uart3_msg.size);
                }
                else
                {
                    //Justin debug未完待续
                    if(1 == line_start)
                    {
                        if(YES == askLineHeart(&monitor, uart3_serial))
                        {
                            line_start = 0;
                        }
                    }
                }

                if(0 != rt_memcmp((u8 *)&sys_time_pre, (u8 *)&sys_time, sizeof(type_sys_time)))
                {
                    rt_memcpy((u8 *)&sys_time_pre, (u8 *)&sys_time, sizeof(type_sys_time));

                    //校准时间
                    rtcTest(sys_time);
                }
            }

            /* 1s 事件 */
            if(ON == Timer1sTouch)
            {
                device_start = 1;
                sensor_start = 1;
                line_start = 1;

                MonitorModuleConnect(GetMonitor());
                tempProgram(GetMonitor());
                co2Program(GetMonitor(), 1000);
                humiProgram(GetMonitor());
                timmerProgram(GetMonitor());
                findLocation(GetMonitor(), &sys_set.cloudCmd, uart2_serial);
                lineProgram(GetMonitor(), 0, 1000);//line1
                lineProgram(GetMonitor(), 1, 1000);//line2
                if(0 != sys_set.cloudCmd.delete_id.value)
                {
                    deleteModule(GetMonitor(), sys_set.cloudCmd.delete_id.value);
                    sys_set.cloudCmd.delete_id.value = 0;
                }
//                cal();//Justin debug 日历功能
            }

            /* 5s 事件 */
            if(ON == Timer5sTouch)
            {
//                device_start_5s = 1;
            }
        }
        rt_thread_mdelay(UART_PERIOD);
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
    rt_thread_t thread = rt_thread_create("sensor task", SensorUart2TaskEntry, RT_NULL, 1024*3, UART2_PRIORITY, 10);

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

void initMonitor(void)
{
    rt_memset((u8 *)&monitor, 0, sizeof(type_monitor_t));
}

type_monitor_t *GetMonitor(void)
{
    return &monitor;
}
