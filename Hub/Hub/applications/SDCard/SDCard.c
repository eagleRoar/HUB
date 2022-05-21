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
#include "TcpPersistence.h"


//extern---------------------------------------------------------------
rt_mutex_t       sd_dfs_mutex = RT_NULL;
struct sdCardState      sdCard;

//---------------------------------------------------------------------

extern type_action_t   Action[3];
extern type_condition_t    Condition;
extern type_excute_t       Excute;
extern type_dotask_t       Dotask;

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

//Justin debug 仅仅测试
void PrintAction(type_action_t action)
{
    LOG_D("id = %d",action.id);
    LOG_D("length = %d",action.curve_length);
    for(int j = 0; j < action.curve_length; j++)
    {
        LOG_D("number %d, start = %d",j,action.curve[j].start_value);
        LOG_D("number %d, end = %d",j,action.curve[j].end_value);
        LOG_D("number %d, time = %d",j,action.curve[j].time);
    }
}

void PrintCondition(type_condition_t condition)
{
    LOG_D("id           = %x",condition.id);
    LOG_D("priority     = %x",condition.priority);
    LOG_D("analyze_type = %x",condition.analyze_type);
    LOG_D("value        = %x",condition.value);
    LOG_D("module_uuid  = %x",condition.module_uuid);
    LOG_D("storage      = %x",condition.storage);
    LOG_D("condition    = %x",condition.condition);
    LOG_D("action_id    = %x",condition.action.action_id);
}

void PrintExcute(type_excute_t excute)
{
    LOG_D("id               = %x",excute.id);
    LOG_D("device_id        = %x",excute.device_id);
    LOG_D("storage          = %x",excute.storage);
    LOG_D("action_id_v      = %x",excute.action_id_v);
}

void PrintDotask(type_dotask_t dotask)
{
    LOG_D("id = %x",dotask.id);
    LOG_D("condition_id = %x",dotask.condition_id);
    LOG_D("excuteAction_id = %x",dotask.excuteAction_id);
    LOG_D("start = %x",dotask.start);
    LOG_D("continue_t = %x",dotask.continue_t);
    LOG_D("delay = %x",dotask.delay);
}

void rtcTest(void)
{

    rt_err_t ret = RT_EOK;
//    time_t now;

    /* 设置日期 */
    ret = set_date(2022, 5, 17);
    if (ret != RT_EOK)
    {
        LOG_D("set RTC date failed\n");
    }

    /* 设置时间 */
//    ret = set_time(15, 15, 50);
//    if (ret != RT_EOK)
//    {
//        LOG_D("set RTC time failed\n");
//    }

    /* 延时3秒 */
//    rt_thread_mdelay(3000);

//    /* 获取时间 */
//    now = time(RT_NULL);
//    LOG_D("%s\n", ctime(&now));
}

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
    u8                      actionIndex         = 0;
    u8                      conditionIndex      = 0;
    u8                      excuteIndex         = 0;
    type_action_t           action;
    type_action_t           condition_action;
    type_condition_t        condition;
    type_excute_t           excute;
    type_dotask_t           dotask;
    time_t                  now;//Justin debug
    type_module_t           *module             = RT_NULL;
    type_module_t           *device             = RT_NULL;

    rt_memset(&sdCard, 0, sizeof(struct sdCardState));

    module = rt_malloc(sizeof(type_module_t));
    device = rt_malloc(sizeof(type_module_t));

    while (1) {
        time1S = TimerTask(&time1S, 20, &Timer1sTouch);//Justin debug 仅仅测试
        rt_mutex_take(sd_dfs_mutex, RT_WAITING_FOREVER);
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

                            sdCard.sd_operate = GetSdOperate();     //映射sd业务层操作接口
                            SdDirInit();//寻找文件夹，不存在则创建

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

                /*处理触发与动作*/
                condition = Condition;
                excute = Excute;
                condition_action = Action[2];
                action = Action[1];
                dotask = Dotask;

                //开始动作
                if(STORAGE_TYPE == condition.analyze_type)
                {
//                    LOG_D("do event ,condition.condition = %d",condition.condition);//Justin debug
                    switch (condition.condition)
                    {

                        case LESS_THAN:// <
                            module = GetModuleByuuid(GetMonitor(), &(condition.module_uuid));
                            if(RT_NULL != module)
                            {
                                if (module->module_t[condition.storage].value < condition_action.curve[0].start_value)
                                {
                                    device = GetModuleByuuid(GetMonitor(), &(excute.device_id));
                                    device->module_t[excute.storage].value = action.curve[0].start_value;
                                    LOG_D("LESS_THAN 1");
                                }
                                else
                                {
                                    device->module_t[excute.storage].value = (action.curve[0].start_value == 0) ? 1 : 0;
                                    LOG_D("LESS_THAN 2");
                                }
                            }
                            break;
                        case LESSTHAN_EQUAL:                        // <=
                            module = GetModuleByuuid(GetMonitor(), &(condition.module_uuid));
                            if(RT_NULL != module)
                            {
                                if (module->module_t[condition.storage].value <= condition_action.curve[0].start_value)
                                {
                                    device = GetModuleByuuid(GetMonitor(), &(excute.device_id));
                                    device->module_t[excute.storage].value = action.curve[0].start_value;
                                    LOG_D("LESSTHAN_EQUAL 1");
                                }
                                else
                                {
                                    device->module_t[excute.storage].value = (action.curve[0].start_value == 0) ? 1 : 0;
                                    LOG_D("LESSTHAN_EQUAL 2");
                                }
                            }
                            break;
                        case GREATER_THAN:                           // >
                            module = GetModuleByuuid(GetMonitor(), &(condition.module_uuid));
                            if(RT_NULL != module)
                            {
                                if (module->module_t[condition.storage].value > condition_action.curve[0].start_value)
                                {
                                    device = GetModuleByuuid(GetMonitor(), &(excute.device_id));
                                    device->module_t[excute.storage].value = action.curve[0].start_value;
                                    LOG_D("GREATER_THAN 1");
                                }
                                else
                                {
                                    device->module_t[excute.storage].value = (action.curve[0].start_value == 0) ? 1 : 0;
                                    LOG_D("GREATER_THAN 2");
                                }
                            }
                            break;
                        case GREATERTHAN_EQUAL:                      // >=

                            module = GetModuleByuuid(GetMonitor(), &(condition.module_uuid));
                            if(RT_NULL != module)
                            {
                                if (module->module_t[condition.storage].value >= condition_action.curve[0].start_value)
                                {
                                    device = GetModuleByuuid(GetMonitor(), &(excute.device_id));
                                    device->module_t[excute.storage].value = action.curve[0].start_value;
                                    LOG_D("GREATERTHAN_EQUAL 1");
                                }
                                else
                                {
                                    device->module_t[excute.storage].value = (action.curve[0].start_value == 0) ? 1 : 0;
                                    LOG_D("GREATERTHAN_EQUAL 2");
                                }
                            }
                            break;
                        case  EQUAL_TO://==

                            module = GetModuleByuuid(GetMonitor(), &(condition.module_uuid));
                            if(RT_NULL != module)
                            {
                                if (module->module_t[condition.storage].value == condition_action.curve[0].start_value)//Justin debug 只用于演示，否则不能这样
                                {
                                    device = GetModuleByuuid(GetMonitor(), &(excute.device_id));
                                    device->module_t[excute.storage].value = action.curve[0].start_value;//Justin debug 只用于演示，否则不能这样
                                    LOG_D("EQUAL_TO 1");
                                }
                                else
                                {
                                    device->module_t[excute.storage].value = (action.curve[0].start_value == 0) ? 1 : 0;
                                    LOG_D("EQUAL_TO 2");
                                }
                            }
                            break;
                        default:
                            break;
                    }

                }

            }
        }
        rt_mutex_release(sd_dfs_mutex);
        rt_thread_mdelay(50);
    }
    rt_free(action.curve);
    action.curve = RT_NULL;
}


