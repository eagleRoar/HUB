/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-01-24     Administrator       the first version
 */

#include "Gpio.h"
#include "SDCard.h"
#include "Uart.h"
#include "CloudProtocol.h"
#include "Recipe.h"

#define DBG_TAG "u.sd"
#define DBG_LVL DBG_INFO

__attribute__((section(".ccmbss"))) char sd_thread_stack[1024*3];
__attribute__((section(".ccmbss"))) struct rt_thread sd_thread;
__attribute__((section(".ccmbss"))) struct sdCardState      sdCard;

rt_mutex_t dynamic_mutex = RT_NULL;

extern u8 saveModuleFlag;
extern int rt_hw_sdio_init(void);

//struct sdCardState getSdState(void)
//{
//    return sdCard;
//}

/**
 * @brief SD处理线程初始化
 * @return
 */
int SDCardTaskInit(void)
{
    rt_err_t ret = RT_EOK;

    dynamic_mutex = rt_mutex_create("dmutex", RT_IPC_FLAG_FIFO);

    rt_thread_init(&sd_thread, SD_CARD_TASK, sd_dfs_event_entry, RT_NULL, &sd_thread_stack[0], sizeof(sd_thread_stack), SDCARD_PRIORITY, 10);
    if(RT_ERROR == rt_thread_startup(&sd_thread))
    {
        LOG_E("sd task err");
    }

    return ret;
}

void printSysTank(void)
{
    LOG_I("systank size = %d",GetSysTank()->tank_size);
    for(int i = 0; i < GetSysTank()->tank_size; i++)
    {
        LOG_I("%d , sensor = %d %d",i,GetSysTank()->tank[i].sensorId[0][0],GetSysTank()->tank[i].sensorId[0][1]);
    }
}

void sd_dfs_event_entry(void* parameter)
{
    rt_device_t             dev;
    static      u8          Timer1sTouch        = OFF;
    static      u16         time1S              = 0;
    static      u8          sensor_size         = 0;
    static      u8          device_size         = 0;
    static      u8          line_size           = 0;

    rt_memset(&sdCard, 0, sizeof(struct sdCardState));

    while (1)
    {
        time1S = TimerTask(&time1S, 20, &Timer1sTouch);
        /* 还没有初始化 */
        if(NO == sdCard.init)
        {
            /* 检查SD卡是否存在  */
            if(sd_card_is_vaild())
            {
                /* 寻找SD设备 */
                dev = rt_device_find(SD_DEVICE_NAME);

                if (dev != RT_NULL)
                {
                    /* 将SD卡挂载在根目录下 */
                    if(NO == sdCard.mount)
                    {
                        if (dfs_mount("sd0", "/", "elm", 0, 0) == 0)//SD挂载在根目录下
                        {
                            rt_mutex_take(dynamic_mutex, RT_WAITING_FOREVER);//互斥锁上锁

                            sdCard.mount = YES;
                            sdCard.init = YES;

                            InitSDCard();

                            if(RT_EOK != TakeMonitorFromSD(GetMonitor()))
                            {
                                LOG_E("TakeMonitorFromSD err");
                            }

                            if(RT_EOK != TackSysSetFromSD(GetSysSet()))
                            {
                                LOG_E("TackSysSetFromSD err");
                            }

                            if(RT_EOK != TackRecipeFromSD(GetSysRecipt()))
                            {
                                LOG_E("TackRecipeFromSD err");
                            }

                            if(RT_EOK != TackSysTankFromSD(GetSysTank()))
                            {
                                LOG_E("TackSysTankFromSD err");
                            }
                            else
                            {
                                LOG_W("-----------------------------------------------------getsystank ok");
                            }

                            sdCard.readInfo = YES;
                            rt_mutex_release(dynamic_mutex);//互斥锁解锁
                        }
                        else //挂载失败
                        {
                            //dfs_unmount("/sd0");//卸载
                            LOG_E("sd card mount to / failed!\r\n");
                        }
                    }
                }
                else
                {
                    LOG_E("sd card find failed!\r\n");
                    rt_hw_sdio_init();
                    rt_thread_mdelay(5000);
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
                //存储module
                if(YES == sdCard.readInfo)
                {
                    if((sensor_size != GetMonitor()->sensor_size) ||
                       (device_size != GetMonitor()->device_size) ||
                       (line_size != GetMonitor()->line_size) ||
                       (YES == saveModuleFlag))
                    {
                        sensor_size = GetMonitor()->sensor_size;
                        device_size = GetMonitor()->device_size;
                        line_size = GetMonitor()->line_size;
                        saveModuleFlag = NO;

                        if(RT_EOK == SaveModule(GetMonitor()))
                        {
                            LOG_W("------------------------SaveModule OK");
                        }
                        else
                        {
                            LOG_I("SaveModule fail");
                        }
                    }

                    if(YES == GetSysSet()->saveFlag)
                    {
                        GetSysSet()->crc = usModbusRTU_CRC((u8 *)GetSysSet()+2, sizeof(sys_set_t) - 2);
                        //LOG_I("----------------sys_para save OK");
                        GetSysSet()->saveFlag = NO;
                        //存储系统设置
                        if(RT_EOK == SaveSysSet(GetSysSet()))
                        {
                            LOG_W("------------------------saveSysSet OK, size = %d, crc = %x",sizeof(sys_set_t),GetSysSet()->crc);

                            /*if(RT_EOK == TackSysSetFromSD(GetSysSet()))
                            {
                                LOG_W("------------read sys set ok");
                            }
                            else
                            {
                                LOG_E("------------read sys set err");
                            }*/
                        }
                        else
                        {
                            LOG_I("saveSysSet fail");
                        }
                    }

                    if(YES == GetSysRecipt()->saveFlag)
                    {
                        GetSysRecipt()->crc = usModbusRTU_CRC((u8 *)GetSysRecipt()+2, sizeof(sys_recipe_t) - 2);
                        //LOG_I("----------------sys_recipe save OK");
                        GetSysRecipt()->saveFlag = NO;

                        if(RT_EOK == SaveSysRecipe(GetSysRecipt()))
                        {
                            LOG_W("------------------------SaveSysRecipe OK");
                        }
                        else
                        {
                            LOG_I("SaveSysRecipe fail");
                        }
                    }

                    if(YES == GetSysTank()->saveFlag)
                    {
                        GetSysTank()->crc = usModbusRTU_CRC((u8 *)GetSysTank() + 2, sizeof(sys_tank_t) - 2);
                        //LOG_I("----------------sys_tank save OK");
                        GetSysTank()->saveFlag = NO;

                        if(RT_EOK == SaveSysTank(GetSysTank()))
                        {
                            LOG_W("------------------------SaveSysTank OK");
                        }
                        else
                        {
                            LOG_I("SaveSysTank fail");
                        }
                    }
                }
            }

            /* 1s事件 */
            if(ON == Timer1sTouch)
            {

            }
        }
        rt_thread_mdelay(50);
    }
}
