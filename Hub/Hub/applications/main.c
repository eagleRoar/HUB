/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-01-07     Qiuyijie    first version
 */

#include <rtthread.h>

#include "Gpio.h"
#include "Ble.h"
#include "Ethernet.h"
#include "Oled1309.h"
#include "SDCard.h"
#include "Uart.h"
#include "Spi.h"

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

extern void GetUpdataFileFromWeb(void);

int main(void)
{
    /* 初始化GPIO口 */
    GpioInit();

    /* 初始化灯光线程,仅作为呼吸灯 */
    LedTaskInit();

    rt_thread_mdelay(5000); //等待lwip准备完毕，该操作需要优化
    /* 初始化网络线程，处理和主机之间的交互，TCP协议 */
    TcpTaskInit();

    /* 初始化网络线程，发送设备信息给主机 */
    UdpTaskInit();

    /* 初始化串口接收传感器类线程 */
    SensorUart2TaskInit();

    /* oled1309屏线程初始化 */
    OledTaskInit();

    /* 初始化SD卡处理线程 */
    SDCardTaskInit();

    /* 从网络上获取新的app包 */
    //GetUpdataFileFromWeb();

    /* 初始化蓝牙Ble线程,蓝牙是通过uart发送数据控制 */
    BleUart6TaskInit();

    /* spi flash程序初始化 */ //SQL需要占用比较多的资源，250kb+的ram，310kb+的rom
    //SpiTaskInit();

    while(1)
    {

        rt_thread_mdelay(1000);
    }

    return RT_EOK;
}
