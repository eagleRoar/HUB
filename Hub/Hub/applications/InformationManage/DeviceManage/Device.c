/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-02     Administrator       the first version
 */
#include "Device.h"
#include "InformationMonitor.h"
#include "SdcardBusiness.h"

struct deviceRegister device_reg;
u8 globalData[sizeof(type_module_t) * 16];//Justin debug  仅仅用于解决rt_realloc导致的问题，还没有解决

void initMoniter(type_monitor_t *monitor)
{
    monitor->monitorDeviceTable.deviceTable = (type_module_t *)globalData;//Justin debug 仅仅测试
}

void deviceRegisterInit(void)
{
    //device_reg.registerAnswer = SEND_NULL;
}

/* 获取device 列表的长度 */
u8 GetDeviceTableSize(type_monitor_t *monitor)
{
    return monitor->monitorDeviceTable.deviceManageLength;
}

/* 向device列表插入新的device */
void InsertDeviceToTable(type_monitor_t *monitor, type_module_t device)
{
    type_module_t *new = RT_NULL;     //返回申请的内存空间

    /* 分配空间 */
    if(0 == GetDeviceTableSize(monitor))
    {
        monitor->monitorDeviceTable.deviceManageLength = 1;
        monitor->monitorDeviceTable.deviceTable = rt_malloc(sizeof(type_module_t));
    }
    else
    {
        if((NO == FindDeviceTableByuuid(monitor, &device.uuid)) /*&&
           (NO == FindDeviceByAddr(monitor, device.address))*/)
        {
            if(monitor->monitorDeviceTable.deviceManageLength < 16)
            {
                monitor->monitorDeviceTable.deviceManageLength++;
            }
            else
            {
                LOG_E("monitor->monitorDeviceTable.deviceManageLength > 16, err");
            }
            new = rt_realloc(monitor->monitorDeviceTable.deviceTable,
                       (monitor->monitorDeviceTable.deviceManageLength)*sizeof(type_module_t));//Justin debug 要解决跑到这个地方之后文件系统打开错误
            LOG_D("----------------------------------InsertDeviceToTable 1");
            if(RT_NULL == new)
            {
                LOG_D("add new memory fail");
                LOG_D("----------------------------------InsertDeviceToTable 2");
            }
            else
            {
                monitor->monitorDeviceTable.deviceTable = new;
                LOG_D("----------------------------------InsertDeviceToTable 3");
            }
        }
        else
        {
            return;
        }
    }

    /* 添加列表 */
    monitor->monitorDeviceTable.deviceTable[monitor->monitorDeviceTable.deviceManageLength - 1] = device;
    SaveAddrAndLenToFile(monitor);
    SaveModuleToFile(&device, monitor->monitorDeviceTable.deviceManageLength - 1);
}

/* 在device列表中删除device */
void DeleteDeviceTableByName(type_monitor_t *monitor, char *name)
{
    int i = 0, j = 0;

    for(i = 0; i < monitor->monitorDeviceTable.deviceManageLength; i++)
    {
        if(0 == rt_memcmp(monitor->monitorDeviceTable.deviceTable[i].module_name, name, MODULE_NAMESZ))
        {
            /* 删除device */
            if(i < monitor->monitorDeviceTable.deviceManageLength - 1)
            {
                for(j = i; j < monitor->monitorDeviceTable.deviceManageLength - 1; j++)
                {
                    monitor->monitorDeviceTable.deviceTable[j] = monitor->monitorDeviceTable.deviceTable[j+1];
                }
            }
            /* 将分配的空间减小 */
            rt_realloc(monitor->monitorDeviceTable.deviceTable, monitor->monitorDeviceTable.deviceManageLength - 1);
        }
    }
}


u8 FindDeviceTableByuuid(type_monitor_t *monitor, u32 *uuid)
{
    int i = 0;

    for(i = 0; i < monitor->monitorDeviceTable.deviceManageLength; i++)
    {
        if(*uuid == monitor->monitorDeviceTable.deviceTable[i].uuid)
        {
            return YES;
        }
    }
    return NO;
}

type_module_t *GetModuleByuuid(type_monitor_t *monitor, u32 *uuid)
{
    int i = 0;

    for(i = 0; i < monitor->monitorDeviceTable.deviceManageLength; i++)
    {
        if(*uuid == monitor->monitorDeviceTable.deviceTable[i].uuid)
        {
            return &(monitor->monitorDeviceTable.deviceTable[i]);
        }
    }

    return RT_NULL;
}

u8 FindDeviceByAddr(type_monitor_t *monitor, u8 addr)
{
    int i = 0;
    type_module_t device;

    for(i = 0; i < monitor->monitorDeviceTable.deviceManageLength; i++)
    {
        device = monitor->monitorDeviceTable.deviceTable[i];
        if(addr == device.address)
        {
            return YES;
        }
    }

    return NO;
}
