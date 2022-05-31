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

        LOG_D("-----------------InitSDCard OK, monitor->crc = %x",GetMonitor()->crc);
    }
}
//struct monitor
//{
//    /* 以下为统一分配 */
//    struct allocate     allocateStr;
//    u8                  module_size;
//    type_module_t       module[MODULE_MAX];
//    u16                 crc;
//};
rt_err_t SaveModule(type_monitor_t *monitor)
{
    u8      index           = 0;
    u16     moduleSize      = sizeof(type_module_t);//为module结构体大小
    u16     monitorSize     = sizeof(type_monitor_t);
    rt_err_t ret = RT_EOK;

    for(index = 0; index < monitor->module_size; index++)
    {
        if(NO == monitor->module[index].save_state)
        {
            monitor->module[index].save_state = YES;
            monitor->module[index].crc = usModbusRTU_CRC((u8 *)&(monitor->module[index]) + 2, moduleSize - 2);//前两位是crc
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

    rt_memset((u8 *)monitor, 0, monitorSize);//Justin debug

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
