/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-05-26     Administrator       the first version
 */
#ifndef APPLICATIONS_TEST_C_
#define APPLICATIONS_TEST_C_


#include "Gpio.h"
#include "UartBussiness.h"
#include "InformationMonitor.h"

void printModule(type_module_t module)
{
    int         index       = 0;

    LOG_D("----------------------print new mnodule-----------");
    LOG_D("uuid             : %x",module.uuid);
    LOG_D("name             : %s",module.name);
    LOG_D("addr             : %x",module.addr);
    LOG_D("save_state       : %x",module.save_state);
    if(SENSOR_TYPE == module.s_or_d)
    {
        LOG_D("s_or_d           : sensor");
    }
    else if(DEVICE_TYPE == module.s_or_d)
    {
        LOG_D("s_or_d           : device");
    }
    LOG_D("storage_size     : %d",module.storage_size);
    for(index = 0; index < module.storage_size; index++)
    {
        LOG_D("stora %d name    : %s",index,module.storage_in[index].name);
    }
}

void printMuduleConnect(type_monitor_t *monitor)
{
                u8          index                   = 0;
    static      u8          state[MODULE_MAX];

    for(index = 0; index < monitor->module_size; index++)
    {
        if(state[index] != monitor->module[index].conn_state)
        {
            if((CON_FAIL == monitor->module[index].conn_state) ||
               (CON_SUCCESS == monitor->module[index].conn_state))
            {
                state[index] = monitor->module[index].conn_state;

                if(CON_FAIL == monitor->module[index].conn_state)
                {
                    LOG_D("no %d, name : %s, connect fail",index,monitor->module[index].name);
                }
                else if(CON_SUCCESS == monitor->module[index].conn_state)
                {
                    LOG_D("no %d, name : %s, connect success",index,monitor->module[index].name);
                }
            }
        }
    }
}

#endif /* APPLICATIONS_TEST_C_ */
