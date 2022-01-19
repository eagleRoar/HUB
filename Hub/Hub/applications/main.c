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
    /* 初始化网络线程，处理和主机之间的交互*/
    rt_thread_mdelay(5000);//等待lwip准备完毕，该操作需要优化
    EthernetTaskInit();
    /* 初始化串口接收传感器类线程 */
    SensorUart2TaskInit();

    while (count++)
    {
        ledState ++;

        rt_thread_mdelay(1000);
    }
#endif

    return RT_EOK;
}
