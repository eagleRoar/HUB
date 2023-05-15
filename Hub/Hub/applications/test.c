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
#include "CloudProtocolBusiness.h"
#include "UartDataLayer.h"
#include "recipe.h"

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
    if(SENSOR_TYPE == TypeSupported(module.type))
    {
        LOG_D("s_or_d           : sensor");
    }
    else if(DEVICE_TYPE == TypeSupported(module.type))
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
    LOG_D("uuid             : %x",module.uuid);
    LOG_D("name             : %s",module.name);
    LOG_D("addr             : %x",module.addr);
    LOG_D("type             : %x",module.type);;
    LOG_D("ctrl_reg         : %x",module.ctrl_addr);
    LOG_D("save_state       : %x",module.save_state);
    LOG_D("conn_state       : %x",module.conn_state);
    if(SENSOR_TYPE == TypeSupported(module.type))
    {
        LOG_D("s_or_d           : sensor");
    }
    else if(DEVICE_TYPE == TypeSupported(module.type))
    {
        LOG_D("s_or_d           : device");
    }
    LOG_D("storage_size     : %d",module.storage_size);
    for(index = 0; index < module.storage_size; index++)
    {
        LOG_D("stora %d name    : %s",index, module.port[index].name);
        LOG_D("stora %d type    : %x",index, module.port[index].type);
        LOG_D("stora %d mode    : %x",index, module.port[index].mode);
    }
}

void printLine(line_t line)
{
    LOG_D("----------------------print new line-----------");
    LOG_D("type = %x",line.type);
    LOG_D("uuid = %x",line.uuid);
    LOG_D("name = %s",line.name);
    LOG_D("lineNo = %d",line.lineNo);
    LOG_D("ctr addr = %x",line.ctrl_addr);
}

void rtcTest(type_sys_time time)
{
    rt_err_t ret = RT_EOK;

    /* 设置日期 */
    ret = set_date(time.year, time.month, time.day);
    set_time(time.hour, time.minute, time.second);
    if (ret != RT_EOK)
    {
        LOG_D("set RTC date failed\n");
    }
}

time_t systimeToTimestamp(u16 year, u8 mon, u8 day, u8 hour, u8 min, u8 sec)
{
    type_sys_time   fortime;
    time_t          now_time;
    struct tm       *time       = RT_NULL;

    getRealTimeForMat(&fortime);
    now_time = getTimeStamp();
    time = getTimeStampByDate(&now_time);

    time->tm_year = year - 1900;
    time->tm_mon = mon - 1;
    time->tm_mday = day;
    time->tm_hour = hour;
    time->tm_min = min;
    time->tm_sec = sec;

    return changeTmTotimet(time);
}

time_t changeDataToTimestamp(u16 year, u8 mon, u8 day, u8 hour, u8 min, u8 sec)
{
    type_sys_time   fortime;
    time_t          now_time;
    struct tm       *time       = RT_NULL;

    getRealTimeForMat(&fortime);
    now_time = getTimeStamp();
    time = getTimeStampByDate(&now_time);

    time->tm_year = year - 1900;
    time->tm_mon = mon - 1;
    time->tm_mday = day;
    time->tm_hour = hour;
    time->tm_min = min;
    time->tm_sec = sec;

    return changeTmTotimet(time);
}

void printRecipe(recipe_t *recipe)
{
    LOG_D("id                   %d",recipe->id);
    LOG_D("name                 %s", recipe->name);
    LOG_D("color                %d", recipe->color);
    LOG_D("dayCoolingTarget     %d", recipe->dayCoolingTarget);
    LOG_D("dayHeatingTarget     %d", recipe->dayHeatingTarget);
    LOG_D("nightCoolingTarget   %d", recipe->nightCoolingTarget);
    LOG_D("nightHeatingTarget   %d", recipe->nightHeatingTarget);
    LOG_D("dayHumidifyTarget    %d", recipe->dayHumidifyTarget);
    LOG_D("dayDehumidifyTarget  %d", recipe->dayDehumidifyTarget);
    LOG_D("nightHumidifyTarget  %d", recipe->nightHumidifyTarget);
    LOG_D("nightDehumidifyTarget %d", recipe->nightDehumidifyTarget);
    LOG_D("dayCo2Target         %d", recipe->dayCo2Target);
    LOG_D("nightCo2Target       %d", recipe->nightCo2Target);

    LOG_D("brightMode       %d", recipe->line_list[0].brightMode);
    LOG_D("byPower          %d", recipe->line_list[0].byPower);
    LOG_D("byAutoDimming    %d", recipe->line_list[0].byAutoDimming);
    LOG_D("mode             %d", recipe->line_list[0].mode);
    LOG_D("lightOn          %d", recipe->line_list[0].lightOn);
    LOG_D("lightOff         %d", recipe->line_list[0].lightOff);
    LOG_D("firstCycleTime   %d", recipe->line_list[0].firstCycleTime);
    LOG_D("duration         %d", recipe->line_list[0].duration);
    LOG_D("pauseTime        %d", recipe->line_list[0].pauseTime);
    LOG_D("firstRuncycleTime %d", recipe->line_list[0].firstRuncycleTime);

    LOG_D("brightMode       %d", recipe->line_list[1].brightMode);
    LOG_D("byPower          %d", recipe->line_list[1].byPower);
    LOG_D("byAutoDimming    %d", recipe->line_list[1].byAutoDimming);
    LOG_D("mode             %d", recipe->line_list[1].mode);
    LOG_D("lightOn          %d", recipe->line_list[1].lightOn);
    LOG_D("lightOff         %d", recipe->line_list[1].lightOff);
    LOG_D("firstCycleTime   %d", recipe->line_list[1].firstCycleTime);
    LOG_D("duration         %d", recipe->line_list[1].duration);
    LOG_D("pauseTime        %d", recipe->line_list[1].pauseTime);
    LOG_D("firstRuncycleTime %d", recipe->line_list[1].firstRuncycleTime);
}

void str_replace(char *original, char *pattern, char *replacement)
{
    char buffer[2048];
    char *insert_point;
    size_t pattern_len = strlen(pattern);
    //size_t replacement_len = strlen(replacement);

    while ((insert_point = strstr(original, pattern))) {
        *insert_point = '\0';
        insert_point += pattern_len;
        sprintf(buffer, "%s%s%s", original, replacement, insert_point);
        strcpy(original, buffer);
    }
}

void str_replace1(char *original, char *pattern, char *replacement, u16 size)//Justin debug
{
//    char buffer[2048];
    char *buffer = RT_NULL;
    buffer = rt_malloc(size);
    if(buffer)
    {
        char *insert_point;
        size_t pattern_len = strlen(pattern);

        while ((insert_point = strstr(original, pattern))) {
            *insert_point = '\0';
            insert_point += pattern_len;
            sprintf(buffer, "%s%s%s", original, replacement, insert_point);
            strcpy(original, buffer);
        }

        rt_free(buffer);
    }
}

#endif /* APPLICATIONS_TEST_C_ */
