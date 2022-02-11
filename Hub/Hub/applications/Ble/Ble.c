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

/*
 * @brief  : 蓝牙处理线程入口
 * @para   : NULL
 * @author : Qiuyijie
 * @date   : 2022.01.25
 */
void BleUart6TaskEntry(void* parameter)
{
    static u8 timeCnt = 0;

    while(1)
    {

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
    rt_thread_t thread = rt_thread_create("ble task", BleUart6TaskEntry, RT_NULL, 1024, 24, 10);

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
