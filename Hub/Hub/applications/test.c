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
    //注意 经常使用该函数作为定时器 如果修改了当前时间初始设置会导致问题
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

void getRealTimeForMat(type_sys_time *time_for)
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

    LOG_D("----------------------print new sensor-----------");
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

void printDevice(device_t module)
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
               index, module.port[index].name, module.port[index].ctrl.d_state);
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
