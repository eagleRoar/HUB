/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-01-24     Administrator       the first version
 */

//rt-include-------------------------------------------------------
#include <dfs_posix.h>
#include <rtdevice.h>
#include <rtthread.h>
//user-include-------------------------------------------------------
#include "SDCard.h"
#include "drv_flash.h"
#define DBG_TAG "u.sd"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>
#include "Uart.h"
#include "Device.h"

//extern---------------------------------------------------------------
rt_mutex_t       sd_dfs_mutex = RT_NULL;
struct sdCardState      sdCard;

//---------------------------------------------------------------------

/**
 * @brief SD处理线程初始化
 * @return
 */
int SDCardTaskInit(void)
{
    rt_err_t ret = RT_EOK;

    /* 创建一个SD-DFS互斥量 */
    sd_dfs_mutex = rt_mutex_create("sd_dfs", RT_IPC_FLAG_FIFO);

    /* 创建 SD卡线程 */
    rt_thread_t thread = rt_thread_create(SD_CARD_TASK, sd_dfs_event_entry, RT_NULL, 1024*4, SDCARD_PRIORITY, 10);

    /* 创建成功则启动线程 */
    if (thread != RT_NULL) {
        rt_thread_startup(thread);
        LOG_I("start Thread [event dfs] sucess");
    } else {
        LOG_E("start Thread [event dfs] failed");
        ret = RT_ERROR;
    }

    return ret;
}

//INIT_APP_EXPORT(SDCardTaskInit);

/**
 * @brief SD卡相关处理处理事件
 *
 * @param parameter
 */

void sd_dfs_event_entry(void* parameter)
{
    rt_device_t             dev;
    static      u8          Timer1sTouch        = OFF;
    static      u16         time1S              = 0;
    static      u8          initMonitorFlag     = NO;
    static      u8          actionSum           = 0;
    static      u8          conditionSum        = 0;
    static      u8          excuteSum           = 0;
    static      u8          dotaskSum           = 0;

    rt_memset(&sdCard, 0, sizeof(struct sdCardState));
    //GetMonitorFromSdCard(GetMonitor());//Justin debug 测试该函数

    while (1) {
        time1S = TimerTask(&time1S, 20, &Timer1sTouch);

        /* 还没有初始化 */
        if(NO == sdCard.init)
        {
            /* 检查SD卡是否存在 */
            if(sd_card_is_vaild())
            {
                /* 寻找SD设备 */
                dev = rt_device_find(SD_DEVICE_NAME);

                if (dev != RT_NULL)
                {
                    /* 将SD卡挂载在根目录下 */
                    if(NO == sdCard.mount)
                    {
                        rt_mutex_take(sd_dfs_mutex, RT_WAITING_FOREVER);
                        if (dfs_mount("sd0", "/", "elm", 0, 0) == 0)//SD挂载在根目录下
                        {
                            sdCard.mount = YES;
                            sdCard.init = YES;

                            SdDirInit();//寻找文件夹，不存在则创建
                            sdCard.sd_operate = GetSdOperate();     //映射sd业务层操作接口

                            LOG_I("sd card mount to / success!\r\n");
                        }
                        else //挂载失败
                        {
                            LOG_E("sd card mount to / failed!\r\n");
                        }
                        rt_mutex_release(sd_dfs_mutex);
                    }
                }
                else
                {

                    LOG_E("sd card find failed!\r\n");
                }
            }
            else
            {

                LOG_E("The SD card slot is empty!\r\n");
            }

        }
        else
        {
            /* 50ms 事件 */
            {
                if(NO == initMonitorFlag)
                {
                    GetMonitorFromSdCard(GetMonitor());
                    sdCard.readInfo = YES;
                    initMonitorFlag = YES;
                }
            }

            /* 1s事件 */
            if(ON == Timer1sTouch)
            {
                if(actionSum != sdCard.sd_operate.action_op.ReadActionSum())
                {
                    actionSum = sdCard.sd_operate.action_op.ReadActionSum();


                }

                if(conditionSum != sdCard.sd_operate.condition_op.ReadConditionSum())
                {
                    conditionSum = sdCard.sd_operate.condition_op.ReadConditionSum();


                }

                if(excuteSum != sdCard.sd_operate.excute_op.ReadExcuteSum())
                {
                    excuteSum = sdCard.sd_operate.excute_op.ReadExcuteSum();


                }

                if(dotaskSum != sdCard.sd_operate.dotask_op.ReadDotaskSum())
                {
                    dotaskSum = sdCard.sd_operate.dotask_op.ReadDotaskSum();


                }
            }
        }

        rt_thread_mdelay(50);
    }
}


