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

    /*初始化GPIO口*/
    GpioInit();
    /*灯光线程*/
    LedTaskInit();
    /*串口打印线程*/

    while (count++)
    {
        ledState ++;

        rt_thread_mdelay(1000);
    }
#endif

    return RT_EOK;
}
