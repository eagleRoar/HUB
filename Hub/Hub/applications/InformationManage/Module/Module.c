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

void InsertModuleToTable(type_monitor_t *monitor, type_module_t module, u8 no)
{
    if(no < MODULE_MAX)
    {
        if(no >= monitor->module_size)
        {
            monitor->module_size++;
        }
        monitor->module[no] = module;
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


#endif /* APPLICATIONS_INFORMATIONMANAGE_MODULE_MODULE_C_ */
