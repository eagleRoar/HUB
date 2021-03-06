/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-05-26     Administrator       the first version
 */
#include "Uart.h"
#include "Module.h"
#include "SDCardBusiness.h"
#include "CloudProtocol.h"

extern sys_set_t *GetSysSet(void);

/**
 * @brief 检测文件夹是否存在
 * @return 返回是否有效,成功为RT_EOK
 */
static u8 CheckDirectory(char* name)
{
    int ret;

    ret = access(name, 0);
    if (ret < 0) {
        LOG_E("\"%s\" error, reset the dir", name);
        //创建文件夹
        ret = mkdir(name, 0x777);
        if (ret < 0) {
            LOG_E("mkdir \"%s\" error", name);
            return RT_ERROR;
        } else {
            return RT_EOK;
        }
    } else {
        return RT_EOK;
    }
}

/**
 * 如果该文件还没有使用的标记返回No ,否则Yes
 */
static u8 CheckFileAndMark(char* file_name)
{
    u8 ret = YES;
    u32 test = 0x00000000;

    if(RT_EOK == ReadSdData(file_name, (u8 *)&test, 0, SD_HEAD_SIZE))
    {
        LOG_D("CheckFileAndMark test = %x",test);
        if(SD_HEAD_CORE != test)
        {
            //如果该文件未写过，需要先写入标志
            test = SD_HEAD_CORE;
            WriteSdData(file_name, (u8 *)&test, 0, SD_HEAD_SIZE);
            ret = NO;
        }
    }
    else
    {
        LOG_E("CheckFileAndMark read fail");
    }

    return ret;
}

/**
 * 初始化sd卡相关设置
 */
void InitSDCard(void)
{
    u8              size            = 0;
    u16             monitorSize     = sizeof(type_monitor_t);
    u16             syssetSize      = sizeof(sys_set_t);
    u16             recipeSize      = sizeof(sys_recipe_t);

    //检查文件是否存在
    CheckDirectory(MODULE_DIR);

    LOG_D("size of type_monitor_t = %d",monitorSize);

    //检查文件是否存在
    if(NO == CheckFileAndMark(MODULE_FILE))
    {
        if(RT_ERROR == WriteSdData(MODULE_FILE, &size, SD_HEAD_SIZE, SD_PAGE_SIZE))
        {
            LOG_E("InitSDCard err 1");
        }

        //初始化monitor文件存储monitor的空间,都置0
        rt_memset((u8 *)GetMonitor(), 0, monitorSize);
        GetMonitor()->crc = usModbusRTU_CRC((u8 *)GetMonitor(), monitorSize - 2);
        if(RT_ERROR == WriteSdData(MODULE_FILE, (u8 *)GetMonitor(), SD_INFOR_SIZE, monitorSize))
        {
            LOG_E("InitSDCard err 2");
        }

//        LOG_D("-----------------InitSDCard OK, monitor->crc = %x",GetMonitor()->crc);
    }

    if(NO == CheckFileAndMark(SYSSET_FILE))
    {
        if(RT_ERROR == WriteSdData(SYSSET_FILE, &size, SD_HEAD_SIZE, SD_PAGE_SIZE))
        {
            LOG_E("InitSDCard err 1");
        }

        rt_memset((u8 *)GetSysSet(), 0, syssetSize);
        initCloudProtocol();
        GetSysSet()->crc = usModbusRTU_CRC((u8 *)GetSysSet() + 2, syssetSize - 2);
        if(RT_ERROR == WriteSdData(SYSSET_FILE, (u8 *)GetSysSet(), SD_INFOR_SIZE, syssetSize))
        {
            LOG_E("InitSDCard err 3");
        }

        LOG_D("GetSysSet()->crc = %x",GetSysSet()->crc);
    }

    if(NO == CheckFileAndMark(RECIPE_FILE))
    {
        if(RT_ERROR == WriteSdData(RECIPE_FILE, &size, SD_HEAD_SIZE, SD_PAGE_SIZE))
        {
            LOG_E("InitSDCard err 4");
        }

//        rt_memset((u8 *)GetSysRecipt(), 0, recipeSize);
//        GetSysRecipt()->crc = usModbusRTU_CRC((u8 *)GetSysRecipt() + 2, recipeSize - 2);
//        LOG_I("----------------------------------GetSysRecipt()->crc = %x",GetSysRecipt()->crc);
        initSysRecipe();
        if(RT_ERROR == WriteSdData(RECIPE_FILE, (u8 *)GetSysRecipt(), SD_INFOR_SIZE, recipeSize))
        {
            LOG_E("InitSDCard err 5");
        }

        LOG_D("initCloudProtocol ok");//Justin debug
    }
}

rt_err_t SaveModule(type_monitor_t *monitor)
{
    u8      index           = 0;
    u16     sensorSize      = sizeof(sensor_t);//为module结构体大小
    u16     deviceSize      = sizeof(device_time4_t);//为module结构体大小
    u16     Timer12Size     = sizeof(timer12_t);
    u16     LineSize        = sizeof(line_t);
    u16     monitorSize     = sizeof(type_monitor_t);
    rt_err_t ret = RT_EOK;

    for(index = 0; index < monitor->sensor_size; index++)
    {
        if(NO == monitor->sensor[index].save_state)
        {
            monitor->sensor[index].save_state = YES;
            monitor->sensor[index].crc = usModbusRTU_CRC((u8 *)&(monitor->sensor[index]) + 2, sensorSize - 2);//前两位是crc
        }
    }

    for(index = 0; index < monitor->device_size; index++)
    {
        if(NO == monitor->device[index].save_state)
        {
            monitor->device[index].save_state = YES;
            monitor->device[index].crc = usModbusRTU_CRC((u8 *)&(monitor->device[index]) + 2, deviceSize - 2);//前两位是crc
        }
    }

    for(index = 0; index < monitor->timer12_size; index++)
    {
        if(NO == monitor->time12[index].save_state)
        {
            monitor->time12[index].save_state = YES;
            monitor->time12[index].crc = usModbusRTU_CRC((u8 *)&(monitor->time12[index]) + 2, Timer12Size - 2);//前两位是crc
        }
    }

    for(index = 0; index < monitor->line_size; index++)
    {
        if(NO == monitor->line[index].save_state)
        {
            monitor->line[index].save_state = YES;
            monitor->line[index].crc = usModbusRTU_CRC((u8 *)&(monitor->line[index]) + 2, LineSize - 2);//前两位是crc
        }
    }

    monitor->crc = usModbusRTU_CRC((u8 *)monitor, monitorSize - 2);

    if(RT_EOK != WriteSdData(MODULE_FILE, (u8 *)monitor, SD_INFOR_SIZE, monitorSize))
    {
        ret = RT_ERROR;
    }

    return ret;
}

rt_err_t TakeMonitorFromSD(type_monitor_t *monitor)
{
    u16         monitorSize     = sizeof(type_monitor_t);
    u16         crc             = 0;
    rt_err_t    ret             = RT_ERROR;

    LOG_D("TakeMonitorFromSD");

    if(RT_EOK == ReadSdData(MODULE_FILE, (u8 *)monitor, SD_INFOR_SIZE, monitorSize))
    {
        crc = usModbusRTU_CRC((u8 *)monitor, monitorSize - 2);//crc 在最后
        LOG_D("TakeMonitorFromSD crc = %x,monitor->crc = %x",crc,monitor->crc);

        if(monitor->crc == crc)
        {
            initModuleConState(monitor);
            ret = RT_EOK;
        }
        else
        {
            rt_memset((u8 *)monitor, 0, monitorSize);
            monitor->crc = usModbusRTU_CRC((u8 *)monitor, monitorSize - 2);
            ret = RT_ERROR;
        }
    }
    else
    {
        LOG_E("TakeMonitorFromSD err 1");
    }

   return ret;
}

rt_err_t TackSysSetFromSD(sys_set_t *set)
{
    rt_err_t    ret             = RT_ERROR;
    u16         setSize         = sizeof(sys_set_t);
    u16         crc             = 0;

    if(RT_EOK == ReadSdData(SYSSET_FILE, (u8 *)set, SD_INFOR_SIZE, setSize))
    {
        crc = usModbusRTU_CRC((u8 *)set + 2, setSize - 2);  //crc 在最后

        if(crc == set->crc)
        {
            ret = RT_EOK;
        }
        else
        {
            rt_memset((u8 *)set, 0, setSize);
            set->crc = usModbusRTU_CRC((u8 *)set+2, setSize - 2);
            ret = RT_ERROR;
        }
    }

    return ret;
}

rt_err_t SaveSysSet(sys_set_t *set)
{
    rt_err_t    ret             = RT_EOK;
    u16         sys_set_size    = sizeof(sys_set_t);

    set->crc = usModbusRTU_CRC((u8 *)set + 2, sys_set_size - 2);

    if(RT_EOK != WriteSdData(SYSSET_FILE, (u8 *)set, SD_INFOR_SIZE, sys_set_size))
    {
        LOG_E("SaveSysSet err");
        ret = RT_ERROR;
    }
    else
    {
        LOG_D("save set ok");
    }

    return ret;
}

rt_err_t TackRecipeFromSD(sys_recipe_t *rec)//Justin debug 获取时crc校验出错
{
    rt_err_t    ret             = RT_ERROR;
    u16         sysRecSize      = sizeof(sys_recipe_t);
    u16         crc             = 0;

    if(RT_EOK == ReadSdData(RECIPE_FILE, (u8 *)rec, SD_INFOR_SIZE, sysRecSize))//Justin debug
    {
        initSysRecipe();
        crc = usModbusRTU_CRC((u8 *)rec + 2, sysRecSize - 2);  //crc 在最后

        if(crc == rec->crc)
        {
            ret = RT_EOK;
        }
        else
        {
            rt_memset((u8 *)rec, 0, sysRecSize);
            rec->crc = usModbusRTU_CRC((u8 *)rec+2, sysRecSize - 2);
            ret = RT_ERROR;
        }
    }

    return ret;
}
