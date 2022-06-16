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
#include "CloudProtocolBusiness.h"

time_t getTimeStamp(void)
{
    return time(RT_NULL);
}

char *getRealTime(void)
{
    time_t      now;

    now = time(RT_NULL);

    return ctime(&now);
}

void printModule(type_module_t module)
{
    int         index       = 0;

    LOG_D("----------------------print new mnodule-----------");
    LOG_D("uuid             : %x",module.uuid);
    LOG_D("name             : %s",module.name);
    LOG_D("addr             : %x",module.addr);
    LOG_D("save_state       : %x",module.save_state);
    LOG_D("conn_state       : %x",module.conn_state);
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
        LOG_D("stora %d name    : %s, value = %d, addr = %x",
              index,module.storage_in[index].name, module.storage_in[index].value, module.storage_in[index].ctrl_addr);
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

void rtcTest(void)
{
    rt_err_t ret = RT_EOK;

    /* 设置日期 */
    ret = set_date(2022, 5, 30);
    set_time(14, 22, 0);
    if (ret != RT_EOK)
    {
        LOG_D("set RTC date failed\n");
    }

}

//void PrintTempSet(proTempSet_t set)
//{
//    LOG_D("-----------------------PrintTempSet");
//    LOG_D("%s %s",set.msgid.name, set.msgid.value);
//    LOG_D("%s %d",set.dayCoolingTarget.name, set.dayCoolingTarget.value);
//    LOG_D("%s %d",set.dayHeatingTarget.name, set.dayHeatingTarget.value);
//    LOG_D("%s %d",set.nightCoolingTarget.name, set.nightCoolingTarget.value);
//    LOG_D("%s %d",set.nightHeatingTarget.name, set.nightHeatingTarget.value);
//    LOG_D("%s %d",set.coolingDehumidifyLock.name, set.coolingDehumidifyLock.value);
//}

//void PrintHumiSet(proHumiSet_t set)
//{
//    LOG_D("-----------------------PrintHumiSet");
////    LOG_D("%s %s",set.msgid.name, set.msgid.value);
////    LOG_D("%s %d",set.dayHumiTarget.name, set.dayHumiTarget.value);
//    LOG_D("%s %d",set.dayDehumiTarget.name, set.dayDehumiTarget.value);
////    LOG_D("%s %d",set.nightHumiTarget.name, set.nightHumiTarget.value);
////    LOG_D("%s %d",set.nightDehumiTarget.name, set.nightDehumiTarget.value);
//}

#endif /* APPLICATIONS_TEST_C_ */
