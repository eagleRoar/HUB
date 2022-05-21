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

static char sd_thread_stack[1024 * 4];
static struct rt_thread sd_thread;

//extern---------------------------------------------------------------
rt_mutex_t       sd_dfs_mutex = RT_NULL;
struct sdCardState      sdCard;

//---------------------------------------------------------------------
//type_action_t           action;
u8 curve_data[SAVE_ACTION_MAX_ZISE];
u8 curve_test[SAVE_ACTION_MAX_ZISE];
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
//    rt_thread_t thread = rt_thread_create(SD_CARD_TASK, sd_dfs_event_entry, RT_NULL, 1024*4, SDCARD_PRIORITY, 10);//Justin debug 仅仅测试有没有内存泄露
//
//    /* 创建成功则启动线程 */
//    if (thread != RT_NULL) {
//        rt_thread_startup(thread);
//        LOG_I("start Thread [event dfs] sucess");
//    } else {
//        LOG_E("start Thread [event dfs] failed");
//        ret = RT_ERROR;
//    }
    rt_thread_init(&sd_thread, SD_CARD_TASK, sd_dfs_event_entry, RT_NULL, &sd_thread_stack[0], sizeof(sd_thread_stack), SDCARD_PRIORITY, 10);
    rt_thread_startup(&sd_thread);

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

void printFileState(void)
{
    int state = 0;

    state = fd_is_open(MODULE_FILE);
    if(state >= 0)
    {
        LOG_D("MODULE_FILE is open,state = %d",state);
    }
    state = fd_is_open(ACTION_FILE);
    if(state >= 0)
    {
        LOG_D("ACTION_FILE is open,state = %d",state);
    }
    state = fd_is_open(CONDITION_FILE);
    if(state >= 0)
    {
        LOG_D("CONDITION_FILE is open,state = %d",state);
    }
    state = fd_is_open(EXCUTE_FILE);
    if(state >= 0)
    {
        LOG_D("EXCUTE_FILE is open,state = %d",state);
    }
    state = fd_is_open(DOTASK_FILE);
    if(state >= 0)
    {
        LOG_D("DOTASK_FILE is open,state = %d",state);
    }
}

void TaskEngine(type_module_t *module,type_module_t *device)
{
    u8                      actionIndex             = 0;
    u8                      con_actionIndex         = 0;
    u8                      conditionIndex          = 0;
    u8                      excuteIndex             = 0;
    u8                      dotaskIndex             = 0;
    u8                      dotaskSum               = 0;
    u8                      action_size             = sizeof(type_action_t) - sizeof(type_curve_t *);
    type_action_t           action;
    type_action_t           condition_action;
    type_condition_t        condition;
    type_excute_t           excute;
    type_dotask_t           dotask;

    action.curve = (type_curve_t *)curve_data;
    rt_memset((u8 *)action.curve, 0, SAVE_ACTION_MAX_ZISE - action_size);
    condition_action.curve = (type_curve_t *)curve_test;
    rt_memset((u8 *)condition_action.curve, 0, SAVE_ACTION_MAX_ZISE - action_size);

    dotaskSum = sdCard.sd_operate.dotask_op.GetDotaskSum();
    for(dotaskIndex = 0; dotaskIndex < dotaskSum; dotaskIndex++)
    {
        if(RT_EOK == sdCard.sd_operate.dotask_op.TakeDotask(&dotask, dotaskIndex))
        {
            //查找是否有符合条件的condition
            if(RT_EOK == sdCard.sd_operate.condition_op.FindConditionById(dotask.condition_id, &conditionIndex))
            {
                if(RT_EOK == sdCard.sd_operate.condition_op.TakeCondition(&condition, conditionIndex))
                {
//                    LOG_D("-------------------------------------------------");//Justin debug
//                    PrintCondition(condition);
                    //获取触发条件的梯形曲线
                    if(RT_EOK == sdCard.sd_operate.action_op.FindActionById(condition.action.action_id, &con_actionIndex))
                    {
                        sdCard.sd_operate.action_op.TakeAction(&condition_action, con_actionIndex);
//                        LOG_D("-------------------------------------------------");//Justin debug
//                        PrintAction(condition_action);
                    }
                }
            }

            //查找是否有符合条件的excute
            if(RT_EOK == sdCard.sd_operate.excute_op.FindExcuteById(dotask.excuteAction_id, &excuteIndex))
            {
                if(RT_EOK == sdCard.sd_operate.excute_op.TakeExcute(&excute, excuteIndex))
                {
//                    LOG_D("-------------------------------------------------");//Justin debug
//                    PrintExcute(excute);
                    if(RT_EOK == sdCard.sd_operate.action_op.FindActionById(excute.action_id_v, &actionIndex))
                    {
                        sdCard.sd_operate.action_op.TakeAction(&action, actionIndex);
//                        LOG_D("-------------------------------------------------");//Justin debug
//                        PrintAction(action);
                    }
                }
            }

            //开始动作
            if(STORAGE_TYPE == condition.analyze_type)
            {
                switch (condition.condition)
                {
                    case LESS_THAN:                           // <
                        module = GetModuleByuuid(GetMonitor(), &(condition.module_uuid));
                        if(RT_NULL != module)
                        {
                            device = GetModuleByuuid(GetMonitor(), &(excute.device_id));
                            if (module->module_t[condition.storage].value < condition_action.curve[0].start_value)
                            {
                                device->module_t[excute.storage].value = action.curve[0].start_value;
//                                LOG_D("device name = %s, excute.storage = %d, do state = %d",device->module_name,excute.storage,device->module_t[excute.storage].value);
//                                LOG_D("GREATER_THAN 1");
                            }
                            else
                            {
                                device->module_t[excute.storage].value = (action.curve[0].start_value == 0) ? 1 : 0;
//                                LOG_D("device name = %s, excute.storage = %d, do state = %d",device->module_name,excute.storage,device->module_t[excute.storage].value);
//                                LOG_D("GREATER_THAN 2");
                            }
                        }
                        break;

                    case GREATER_THAN:                           // >
                        module = GetModuleByuuid(GetMonitor(), &(condition.module_uuid));
                        if(RT_NULL != module)
                        {
                            device = GetModuleByuuid(GetMonitor(), &(excute.device_id));
                            if (module->module_t[condition.storage].value > condition_action.curve[0].start_value)
                            {
                                device->module_t[excute.storage].value = action.curve[0].start_value;
//                                LOG_D("device name = %s, excute.storage = %d, do state = %d",device->module_name,excute.storage,device->module_t[excute.storage].value);
//                                LOG_D("GREATER_THAN 1");
                            }
                            else
                            {
                                device->module_t[excute.storage].value = (action.curve[0].start_value == 0) ? 1 : 0;
//                                LOG_D("device name = %s, excute.storage = %d, do state = %d",device->module_name,excute.storage,device->module_t[excute.storage].value);
//                                LOG_D("GREATER_THAN 2");
                            }
                        }
                        break;

                    default:
                        break;
                }

            }
        }
    }
}

void sd_dfs_event_entry(void* parameter)
{
    rt_device_t             dev;
    static      u8          Timer1sTouch        = OFF;
    static      u16         time1S              = 0;
    static      u8          initMonitorFlag     = NO;
//    static      u8          actionSum           = 0;
//    static      u8          conditionSum        = 0;
//    static      u8          excuteSum           = 0;
//    static      u8          dotaskSum           = 0;
//    u8                      actionIndex         = 0;
//    u8                      conditionIndex      = 0;
//    u8                      excuteIndex         = 0;
//    u8                      action_size         = sizeof(type_action_t) - sizeof(type_curve_t *);
//    type_action_t           action;
//    type_action_t           condition_action;
//    type_condition_t        condition;
//    type_excute_t           excute;
//    type_dotask_t           dotask;
//    time_t                  now;//Justin debug
    type_module_t           module;
    type_module_t           device;

    rt_memset(&sdCard, 0, sizeof(struct sdCardState));

//    action.curve = (type_curve_t *)curve_data;
//    rt_memset((u8 *)action.curve, 0, SAVE_ACTION_MAX_ZISE - action_size);

    while (1)
    {
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
//                        rt_mutex_take(sd_dfs_mutex, RT_WAITING_FOREVER);
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
//                        rt_mutex_release(sd_dfs_mutex);
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
                    GetMonitorFromSdCard(GetMonitor());//Justin debug 怀疑该位置有问题导致内存泄漏
                    sdCard.readInfo = YES;
                    initMonitorFlag = YES;
                }
            }

            /* 1s事件 */
            if(ON == Timer1sTouch)
            {
//                dotaskSum = sdCard.sd_operate.dotask_op.GetDotaskSum();
//                LOG_D("-------------dotaskSum = %d",dotaskSum);

                TaskEngine(&module, &device);

                //1.
//                actionSum = sdCard.sd_operate.action_op.GetActionSum();
//                LOG_D("-------------actionSum = %d",actionSum);
//                for(int i = 0; i < actionSum; i++)
//                {
//                    sdCard.sd_operate.action_op.TakeAction(&action, i);//使用该函数时候会导致内存泄露等问题
//                    PrintAction(action);
//                }

//                //2.
//                conditionSum = sdCard.sd_operate.condition_op.GetConditionSum();
//                LOG_D("-------------conditionSum = %d",conditionSum);
//                for(int i = 0; i < conditionSum; i++)
//                {
//                    sdCard.sd_operate.condition_op.TakeCondition(&condition,i);
//                    PrintCondition(condition);
//                }
//
//                //3.
//                excuteSum = sdCard.sd_operate.excute_op.GetExcuteSum();
//                LOG_D("-------------excuteSum = %d",excuteSum);
//                for(int i = 0; i < excuteSum; i++)
//                {
//                    sdCard.sd_operate.excute_op.TakeExcute(&excute,i);
//                    PrintExcute(excute);
//                }
//
//                //4.
//                dotaskSum = sdCard.sd_operate.dotask_op.GetDotaskSum();
//                LOG_D("-------------dotaskSum = %d",dotaskSum);
//                for(int i = 0; i < dotaskSum; i++)
//                {
//                    sdCard.sd_operate.dotask_op.TakeDotask(&dotask,i);
//                    PrintDotask(dotask);
//                }

//                printFileState();
            }
        }
        rt_mutex_release(sd_dfs_mutex);
        rt_thread_mdelay(50);
    }
}
