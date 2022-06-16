/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-05-26     Administrator       the first version
 */
#ifndef APPLICATIONS_INFORMATIONMANAGE_MODULE_MODULE_C_
#define APPLICATIONS_INFORMATIONMANAGE_MODULE_MODULE_C_


#include "Module.h"
#include "UartBussiness.h"

void InsertModuleToTable(type_monitor_t *monitor, type_module_t module, u8 no)
{
    if(no < MODULE_MAX)
    {
        if(no >= monitor->module_size)
        {
            monitor->module_size++;
        }
        monitor->module[no] = module;
        printModule(module);
    }
}

u8 FindModule(type_monitor_t *monitor, type_module_t module, u8 *no)
{
    u8          index       = 0;
    u8          ret         = NO;

    *no = monitor->module_size;
    for (index = 0; index < monitor->module_size; index++)
    {
        if ((monitor->module[index].uuid == module.uuid) &&
            (monitor->module[index].type == module.type))
        {
//            LOG_D("module have exist");
            *no = index;
            ret = YES;
        }
    }

    return ret;
}

u8 FindModuleByAddr(type_monitor_t *monitor, u8 addr)
{
    int i = 0;

    for(i = 0; i < monitor->module_size; i++)
    {
        if(addr == monitor->module[i].addr)
        {
            return YES;
        }
    }

    return NO;
}

void initModuleConState(type_monitor_t *monitor)
{
    u8          index       = 0;

    for(index = 0; index < monitor->module_size; index++)
    {
        monitor->module[index].conn_state = CON_FAIL;
    }
}

type_module_t *GetModuleByType(type_monitor_t *monitor, u8 type)
{
    u8      index       = 0;

    for(index = 0; index < monitor->module_size; index++)
    {
        if(type == monitor->module[index].type)
        {
            return &(monitor->module[index]);
        }
    }

    return RT_NULL;
}

/**
 * 获取灯光是属于Line1还是Line2
 */
u8 getLineNoByuuid(type_monitor_t *monitor, u32 uuid)
{
    u8      index           = 0;
    u8      cout            = 0;
    u8      ret             = 0xFF;
    u32     uuidList[2]     = {0x00000000, 0x00000000};

    for(index = 0; index < monitor->module_size; index++)
    {
        if(LINE_TYPE == monitor->module[index].type)
        {
            if(cout < 2)
            {
                uuidList[cout] = monitor->module[index].uuid;
                cout++;
            }
        }
    }

    if(uuidList[0] == uuid)
    {
        ret = 0;
    }
    else if(uuidList[1] == uuid)
    {
        ret = 1;
    }

    return ret;
}

#endif /* APPLICATIONS_INFORMATIONMANAGE_MODULE_MODULE_C_ */
