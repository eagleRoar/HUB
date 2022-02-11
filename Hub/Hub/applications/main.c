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


#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

int main(void)
{
#ifdef TEST_PROGRAM

    int count = 1;
    static int ledState = 0;

    /* 初始化GPIO口 */
    GpioInit();
    /* 初始化灯光线程,仅作为呼吸灯 */
    LedTaskInit();

    rt_thread_mdelay(5000);         //等待lwip准备完毕，该操作需要优化
    /* 初始化网络线程，处理和主机之间的交互，TCP协议*/
    TcpTaskInit();
    /* 初始化网络线程，发送设备信息给主机*/
    UdpTaskInit();
    /* 初始化串口接收传感器类线程 */
    SensorUart2TaskInit();
    /* oled1309屏初始化*/
    oledInit();
    /* 初始化oled1309 UI处理线程*/

    /* 初始化SD卡处理线程 */
    SDCardTaskInit();
    /* 初始化蓝牙Ble线程,蓝牙是通过uart发送数据控制*/
    //BleUart6TaskInit();

    while (count++)
    {
        ledState ++;

        rt_thread_mdelay(1000);
    }
#endif

    return RT_EOK;
}
