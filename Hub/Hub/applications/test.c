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
#include "UartDataLayer.h"

time_t getTimeStamp(void)
{
    return time(RT_NULL);
}

u8 getMonth(char *mon)
{
    if(0 == rt_memcmp("Jan", mon, 3))
    {
        return 1;
    }
    else if(0 == rt_memcmp("Feb", mon, 3))
    {
        return 2;
    }
    else if(0 == rt_memcmp("Mar", mon, 3))
    {
        return 3;
    }
    else if(0 == rt_memcmp("Apr", mon, 3))
    {
        return 4;
    }
    else if(0 == rt_memcmp("May", mon, 3))
    {
        return 5;
    }
    else if(0 == rt_memcmp("Jun", mon, 3))
    {
        return 6;
    }
    else if(0 == rt_memcmp("Jul", mon, 3))
    {
        return 7;
    }
    else if(0 == rt_memcmp("Aug", mon, 3))
    {
        return 8;
    }
    else if(0 == rt_memcmp("Sep", mon, 3))
    {
        return 9;
    }
    else if(0 == rt_memcmp("Oct", mon, 3))
    {
        return 10;
    }
    else if(0 == rt_memcmp("Nov", mon, 3))
    {
        return 11;
    }
    else if(0 == rt_memcmp("Dec", mon, 3))
    {
        return 12;
    }
    else {
        return 0;
    }
}

void getRealTimeForMat(type_sys_time *time_for)//Justin debug 未验证
{
    /* "Wed Jun 30 21:49:08 1993\n" */
    char time[25];
    char delim[]    = " :";
    char *p         = RT_NULL;

    rt_memcpy(time, getRealTime(), 25);

    p = strtok(time, delim);
    if(RT_NULL != p)
    {
        p = strtok(NULL, delim);
        if(RT_NULL != p)
        {
            time_for->month = getMonth(p);
        }
        p = strtok(NULL, delim);
        if(RT_NULL != p)
        {
            time_for->day = atoi(p);
        }
        p = strtok(NULL, delim);
        if(RT_NULL != p)
        {
            time_for->hour = atoi(p);
        }
        p = strtok(NULL, delim);
        if(RT_NULL != p)
        {
            time_for->minute = atoi(p);
        }
        p = strtok(NULL, delim);
        if(RT_NULL != p)
        {
            time_for->second = atoi(p);
        }
        p = strtok(NULL, delim);
        if(RT_NULL != p)
        {
            time_for->year = atoi(p);
        }
    }
}

char *getRealTime(void)
{
    time_t      now;

    now = time(RT_NULL);

    return ctime(&now);
}

void printSensor(sensor_t module)
{
    int         index       = 0;

    LOG_D("----------------------print new mnodule-----------");
    LOG_D("type             : %x",module.type);
    LOG_D("uuid             : %x",module.uuid);
    LOG_D("name             : %s",module.name);
    LOG_D("addr             : %x",module.addr);
    LOG_D("save_state       : %x",module.save_state);
    LOG_D("conn_state       : %x",module.conn_state);
    if(SENSOR_TYPE == getSOrD(module.type))
    {
        LOG_D("s_or_d           : sensor");
    }
    else if(DEVICE_TYPE == getSOrD(module.type))
    {
        LOG_D("s_or_d           : device");
    }
    LOG_D("storage_size     : %d",module.storage_size);
    for(index = 0; index < module.storage_size; index++)
    {
            LOG_D("stora %d name    : %s, value = %d",
                   index,module.__stora[index].name, module.__stora[index].value);
    }
}

void printDevice(device_time4_t module)
{
    int         index       = 0;

    LOG_D("----------------------print new mnodule-----------");
    LOG_D("type             : %x",module.type);
    LOG_D("uuid             : %x",module.uuid);
    LOG_D("name             : %s",module.name);
    LOG_D("addr             : %x",module.addr);
    LOG_D("save_state       : %x",module.save_state);
    LOG_D("conn_state       : %x",module.conn_state);
    if(SENSOR_TYPE == getSOrD(module.type))
    {
        LOG_D("s_or_d           : sensor");
    }
    else if(DEVICE_TYPE == getSOrD(module.type))
    {
        LOG_D("s_or_d           : device");
    }
    LOG_D("storage_size     : %d",module.storage_size);
    for(index = 0; index < module.storage_size; index++)
    {
            LOG_D("stora %d name    : %s, value = %d, addr = %x",
                   index, module._storage[index]._port.name, module._storage[index]._port.d_state,module._storage[index]._port.addr);
    }
}

void printLine(line_t line)
{
    LOG_D("----------------------print new line-----------");
    LOG_D("type = %x",line.type);
    LOG_D("uuid = %x",line.uuid);
    LOG_D("name = %s",line.name);
    LOG_D("ctr addr = %x",line.ctrl_addr);
}

void printTimer12(timer12_t module)
{
    int         index       = 0;
    int         port        = 0;

    LOG_D("----------------------print new mnodule-----------");
    LOG_D("type             : %x",module.type);
    LOG_D("uuid             : %x",module.uuid);
    LOG_D("name             : %s",module.name);
    LOG_D("addr             : %x",module.addr);
    LOG_D("save_state       : %x",module.save_state);
    LOG_D("conn_state       : %x",module.conn_state);
    LOG_D("s_or_d : Timer12");
    LOG_D("storage_size     : %d",module.storage_size);
    for(index = 0; index < /*TIMER12_PORT_MAX*/1; index++)//Justin debug
    {
        for(port = 0; port < TIMER_GROUP; port++)
        {
            LOG_D("stora %d : on_at = %d, duration = %d, en = %d",
                   index, module._time12_ctl[index]._timer[port].on_at, module._time12_ctl[index]._timer[port].duration,
                   module._time12_ctl[index]._timer[port].en);
        }
        LOG_D("d_state = %d",module._time12_ctl[index].d_state);
        LOG_D("d_value = %d",module._time12_ctl[index].d_value);
    }

    LOG_D("_recycle.duration = %d",module._recycle.duration);
    LOG_D("_recycle.pauseTime = %d",module._recycle.pauseTime);
    LOG_D("_recycle.startAt = %d",module._recycle.startAt);
}

void rtcTest(type_sys_time time)
{
    rt_err_t ret = RT_EOK;

//    /* 设置日期 */
//    ret = set_date(2022, 5, 30);
//    set_time(14, 22, 0);
//    if (ret != RT_EOK)
//    {
//        LOG_D("set RTC date failed\n");
//    }
    /* 设置日期 */
    ret = set_date(time.year, time.month, time.day);
    set_time(time.hour, time.minute, time.second);
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
