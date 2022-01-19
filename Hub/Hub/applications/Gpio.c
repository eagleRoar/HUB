/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-01-12     Qiuyijie     the first version
 */

#include "Gpio.h"

#ifdef TEST_PROGRAM

/**
 * @brief  : GPIO口初始化
 * @para   : NULL
 * @author : Qiuyijie
 * @date   : 2022.01.12
 */
void GpioInit(void)
{

    rt_pin_mode(LED_0, PIN_MODE_OUTPUT);
}

/**
 * @brief  : led 灯光控制
 * @para   : pin 控制的IO口
 * @para   : state 状态
 * @author : Qiuyijie
 * @date   : 2022.01.12
 */
void Ctrl_LED(rt_base_t pin, rt_base_t state)
{
    rt_pin_write(pin, state);
}

/**
 * @brief  : led 灯光线程
 * @para   : NULL
 * @author : Qiuyijie
 * @date   : 2022.01.12
 */
void LedTaskInit(void)
{
    rt_err_t threadStart = RT_NULL;

    /* 创建led 线程 */
    rt_thread_t thread = rt_thread_create("led task", LedTaskEntry, RT_NULL, 1024, 26, 10);

    /* 如果线程创建成功则开始启动线程，否则提示线程创建失败 */
    if (RT_NULL != thread) {
        threadStart = rt_thread_startup(thread);
        if (RT_EOK != threadStart) {
            LOG_E("led task start failed");
        }
    } else {
        LOG_E("led task create failed");
    }
}

/**
 * @brief  : led 灯光线程入口函数，周期性闪烁效果
 * @para   : NULL
 * @author : Qiuyijie
 * @date   : 2022.01.12
 */
void LedTaskEntry(void* parameter)
{
    static u8 ledState = 0;
    while(1)
    {
        Ctrl_LED(LED_0,ledState++ % 2);

        rt_thread_mdelay(500);
    }
}

#endif
