/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-04-10     Administrator       the first version
 */
#include "UartEventType.h"
#include "UartDataLayer.h"
#include "DeviceUartClass.h"

//获取当前的参数设置
void GetNowSysSet(proTempSet_t *tempSet, proCo2Set_t *co2Set, proHumiSet_t *humiSet,
        proLine_t *line1Set, proLine_t *line2Set, struct recipeInfor *info)
{
    u8              item = 0;
    u8              index = 0;
    time_t          starts;
    sys_set_t       *set = GetSysSet();
    sys_recipe_t    *recipe = GetSysRecipt();
    u8              usedCalFlg = OFF; // 如果为OFF 则使用系统设置 否则
    type_sys_time   time;

    changeCharToDate(set->stageSet.starts, &time);
    starts = changeDataToTimestamp(time.year, time.month, time.day, time.hour, time.minute, time.second);

    //如果不使能日历 或者 不处于日历的
    if(OFF == set->stageSet.en)
    {
        usedCalFlg = OFF;
    }
    else if(ON == set->stageSet.en)
    {
        for(index = 0; index < STAGE_LIST_MAX; index++)
        {
            if((0 != set->stageSet._list[index].recipeId) && (0 != set->stageSet._list[index].duration_day))
            {

                if((getTimeStamp() >= starts) && (getTimeStamp() <= starts + set->stageSet._list[index].duration_day * 24 * 60 * 60))
                {
                    for(item = 0; item < recipe->recipe_size; item++)
                    {
                        if(recipe->recipe[item].id == set->stageSet._list[index].recipeId)
                        {
                            usedCalFlg = ON;
                            break;
                        }
                    }
                }

                if(ON == usedCalFlg)
                {
                    break;
                }

                starts += set->stageSet._list[index].duration_day * 24 * 60 * 60;
            }
        }
    }

    if(OFF == usedCalFlg)
    {
        //使用系统设置
        if(RT_NULL != tempSet)
        {
            rt_memcpy((u8 *)tempSet, (u8 *)&set->tempSet, sizeof(proTempSet_t));
        }

        if(RT_NULL != co2Set)
        {
            rt_memcpy((u8 *)co2Set, (u8 *)&set->co2Set, sizeof(proCo2Set_t));
        }

        if(RT_NULL != humiSet)
        {
            rt_memcpy((u8 *)humiSet, (u8 *)&set->humiSet, sizeof(proHumiSet_t));
        }

        if(RT_NULL != line1Set)
        {
            rt_memcpy((u8 *)line1Set, (u8 *)&set->line1Set, sizeof(proLine_t));
        }

        if(RT_NULL != line2Set)
        {
            rt_memcpy((u8 *)line2Set, (u8 *)&set->line2Set, sizeof(proLine_t));
        }

        if(RT_NULL != info)
        {
            strncpy(info->name, "--", RECIPE_NAMESZ - 1);
            info->name[RECIPE_NAMESZ - 1] = '\0';

            info->week = 0;//天化为星期
            info->day = 0;
        }
    }
    else if(ON == usedCalFlg)
    {
        //使用日历设置, 但是相关联的一些标志要使用系统的
        if(RT_NULL != tempSet)
        {
            rt_memcpy((u8 *)tempSet, (u8 *)&set->tempSet, sizeof(proTempSet_t));
            tempSet->dayCoolingTarget = recipe->recipe[item].dayCoolingTarget;
            tempSet->dayHeatingTarget = recipe->recipe[item].dayHeatingTarget;
            tempSet->nightCoolingTarget = recipe->recipe[item].nightCoolingTarget;
            tempSet->nightHeatingTarget = recipe->recipe[item].nightHeatingTarget;
        }

        if(RT_NULL != co2Set)
        {
            rt_memcpy((u8 *)co2Set, (u8 *)&set->co2Set, sizeof(proCo2Set_t));
            co2Set->dayCo2Target = recipe->recipe[item].dayCo2Target;
            co2Set->nightCo2Target = recipe->recipe[item].nightCo2Target;
        }

        if(RT_NULL != humiSet)
        {
            rt_memcpy((u8 *)humiSet, (u8 *)&set->humiSet, sizeof(proHumiSet_t));
            humiSet->dayHumiTarget = recipe->recipe[item].dayHumidifyTarget;
            humiSet->dayDehumiTarget = recipe->recipe[item].dayDehumidifyTarget;
            humiSet->nightHumiTarget = recipe->recipe[item].nightHumidifyTarget;
            humiSet->nightDehumiTarget = recipe->recipe[item].nightDehumidifyTarget;
        }

        if(RT_NULL != line1Set)
        {
            rt_memcpy((u8 *)line1Set, (u8 *)&set->line1Set, sizeof(proLine_t));
            line1Set->brightMode = recipe->recipe[item].line_list[0].brightMode;
            line1Set->byPower = recipe->recipe[item].line_list[0].byPower;
            line1Set->byAutoDimming = recipe->recipe[item].line_list[0].byAutoDimming;
            line1Set->mode = recipe->recipe[item].line_list[0].mode;
            line1Set->lightOn = recipe->recipe[item].line_list[0].lightOn;
            line1Set->lightOff = recipe->recipe[item].line_list[0].lightOff;
            line1Set->firstCycleTime = recipe->recipe[item].line_list[0].firstCycleTime;
            line1Set->firstRuncycleTime = recipe->recipe[item].line_list[0].firstRuncycleTime;
            line1Set->duration = recipe->recipe[item].line_list[0].duration;
            line1Set->pauseTime = recipe->recipe[item].line_list[0].pauseTime;
        }

        if(RT_NULL != line2Set)
        {
            rt_memcpy((u8 *)line2Set, (u8 *)&set->line2Set, sizeof(proLine_t));
            line2Set->brightMode = recipe->recipe[item].line_list[1].brightMode;
            line2Set->byPower = recipe->recipe[item].line_list[1].byPower;
            line2Set->byAutoDimming = recipe->recipe[item].line_list[1].byAutoDimming;
            line2Set->mode = recipe->recipe[item].line_list[1].mode;
            line2Set->lightOn = recipe->recipe[item].line_list[1].lightOn;
            line2Set->lightOff = recipe->recipe[item].line_list[1].lightOff;
            line2Set->firstCycleTime = recipe->recipe[item].line_list[1].firstCycleTime;
            line2Set->firstRuncycleTime = recipe->recipe[item].line_list[1].firstRuncycleTime;
            line2Set->duration = recipe->recipe[item].line_list[1].duration;
            line2Set->pauseTime = recipe->recipe[item].line_list[1].pauseTime;
        }

        if(RT_NULL != info)
        {
            char year[5] = " ", mon[3] = " ", day[3] = " ";
            strncpy(info->name, recipe->recipe[item].name, RECIPE_NAMESZ - 1);
            info->name[RECIPE_NAMESZ - 1] = '\0';
            strncpy(year, set->stageSet.starts, 4);
            year[4] = '\0';
            strncpy(mon, &set->stageSet.starts[4], 2);
            mon[2] = '\0';
            strncpy(day, &set->stageSet.starts[6], 2);
            day[2] = '\0';
            time_t time = changeDataToTimestamp(atoi(year), atoi(mon), atoi(day), 0, 0, 0);
            if(getTimeStamp() > time)
            {
                info->week = (getTimeStamp() - time) / (24 * 60 * 60) / 7;//天化为星期
                info->day = (getTimeStamp() - time) / (24 * 60 * 60) % 7;
            }
        }
    }
}

static u8 GetDeviceStateByFunc(type_monitor_t *monitor, u8 func)
{
    int     i       = 0;
    int     port    = 0;

    //1 如果有开启则返回
    for(i = 0; i < monitor->device_size; i++)
    {
        for(port = 0; port < monitor->device[i].storage_size; port++)
        {
            if(func == monitor->device[i].port[port].func)
            {
                if(ON == monitor->device[i].port[port].ctrl.d_state)
                {
                    break;
                }
            }
        }
    }
    //2 检测是否有端口开启
    if(i == monitor->device_size)
    {
        return ON;
    }
    else
    {
        return OFF;
    }
}

//mPeriod 周期 单位ms
void co2Program(type_monitor_t *monitor, type_uart_class uart, u16 mPeriod)
{
    int             co2Now      = 0;
    u16             co2Target   = 0;
    static u16      runTime     = 0;
    static u16      stopTime    = 0;
    u8              switchFlg   = 0;
    proCo2Set_t     co2Set;

    GetNowSysSet(RT_NULL, &co2Set, RT_NULL, RT_NULL, RT_NULL, RT_NULL);

    co2Now = getSensorDataByFunc(monitor, F_S_CO2);
    if(VALUE_NULL != co2Now)
    {
        if(DAY_TIME == GetSysSet()->dayOrNight)
        {
            co2Target = co2Set.dayCo2Target;
        }
        else if(NIGHT_TIME == GetSysSet()->dayOrNight)
        {
            co2Target = co2Set.nightCo2Target;
        }

        if(DAY_TIME == GetSysSet()->dayOrNight)
        {
            if(ON == co2Set.isFuzzyLogic)
            {
                //检测当前
                //开10s 再关闭 10秒
                if((runTime < 10 * 1000) && (co2Now <= co2Target))
                {
                    runTime += mPeriod;
                    switchFlg = 1;
                }
                else
                {
                    switchFlg = 0;
                    if(stopTime < 10 * 1000)
                    {
                        stopTime += mPeriod;
                    }
                    else
                    {
                        runTime = 0;
                        stopTime = 0;
                    }
                }

                if(1 == switchFlg)
                {
                    if(!((ON == co2Set.dehumidifyLock && ON == GetDeviceStateByFunc(monitor, F_DEHUMI)) ||
                         (ON == co2Set.coolingLock && ON == GetDeviceStateByFunc(monitor, F_COOL))))
                    {
                        if((VALUE_NULL == getSensorDataByFunc(monitor, F_S_O2)) ||
                           (getSensorDataByFunc(monitor, F_S_O2) >= 180))
                        {
                            uart.DeviceCtrl(monitor, F_Co2_UP, ON);
                        }
                    }
                    else
                    {
                        uart.DeviceCtrl(monitor, F_Co2_UP, OFF);
                    }
                }
                else
                {
                    uart.DeviceCtrl(monitor, F_Co2_UP, OFF);
                }
            }
            else
            {
                if(co2Now <= co2Target)
                {
                    //如果和制冷联动 则制冷的时候不增加co2
                    //如果和除湿联动 则除湿的时候不增加co2
                    if(!((ON == co2Set.dehumidifyLock && ON == GetDeviceStateByFunc(monitor, F_DEHUMI)) ||
                         (ON == co2Set.coolingLock && ON == GetDeviceStateByFunc(monitor, F_COOL))))
                    {
                        if((VALUE_NULL == getSensorDataByFunc(monitor, F_S_O2)) ||
                           (getSensorDataByFunc(monitor, F_S_O2) >= 180))
                        {
                            uart.DeviceCtrl(monitor, F_Co2_UP, ON);
                        }
                    }
                    else
                    {
                        uart.DeviceCtrl(monitor, F_Co2_UP, OFF);
                    }
                }
                else if(co2Now >= co2Target + co2Set.co2Deadband)
                {
                    uart.DeviceCtrl(monitor, F_Co2_UP, OFF);
                }
            }
            uart.DeviceCtrl(monitor, F_Co2_DOWN, OFF);
        }
        else if(NIGHT_TIME == GetSysSet()->dayOrNight)
        {
            if(co2Now > co2Target)
            {
                uart.DeviceCtrl(monitor, F_Co2_DOWN, ON);
            }
            else if(co2Now + co2Set.co2Deadband <= co2Target)
            {
                uart.DeviceCtrl(monitor, F_Co2_DOWN, OFF);
            }
            uart.DeviceCtrl(monitor, F_Co2_UP, OFF);
        }
    }
}

void humiProgram(type_monitor_t *monitor, type_uart_class uart)
{
    int             humiNow             = 0;
    u16             humiTarget          = 0;
    u16             dehumiTarget        = 0;
    proHumiSet_t    humiSet;
    proTempSet_t    tempSet;

    GetNowSysSet(&tempSet, RT_NULL, &humiSet, RT_NULL, RT_NULL, RT_NULL);

    humiNow = getSensorDataByFunc(monitor, F_S_HUMI);
    if(VALUE_NULL != humiNow)
    {
        if(DAY_TIME == GetSysSet()->dayOrNight)
        {
            humiTarget = humiSet.dayHumiTarget;
            dehumiTarget = humiSet.dayDehumiTarget;
        }
        else if(NIGHT_TIME == GetSysSet()->dayOrNight)
        {
            humiTarget = humiSet.nightHumiTarget;
            dehumiTarget = humiSet.nightDehumiTarget;
        }

        //达到湿度目标
        if(humiNow >= dehumiTarget)
        {
            uart.DeviceCtrl(monitor, F_DEHUMI, ON);
        }
        else if(humiNow <= dehumiTarget - humiSet.humidDeadband)
        {
            uart.DeviceCtrl(monitor, F_DEHUMI, OFF);
        }

        if(humiNow <= humiTarget)
        {
            uart.DeviceCtrl(monitor, F_HUMI, ON);
        }
        else if(humiNow >= humiTarget + humiSet.humidDeadband)
        {
            uart.DeviceCtrl(monitor, F_HUMI, OFF);
        }

        //当前有一个逻辑是降温和除湿联动选择
        if(ON == tempSet.coolingDehumidifyLock)
        {
            //如果除湿是开的话，AC_cool 不能关，因为可能AC_cool 上插着风扇
            if(ON == GetDeviceStateByFunc(monitor, F_DEHUMI))
            {
                uart.DeviceCtrl(monitor, COOL_TYPE, ON);
            }
        }
    }

}

void tempProgram(type_monitor_t *monitor, type_uart_class uart)
{
    u16             value               = 0;
    int             tempNow             = 0;
    u16             coolTarge           = 0;
    u16             HeatTarge           = 0;
    proTempSet_t    tempSet;
    device_t        *device             = RT_NULL;
    static u8       hvac[2]             = {0};

    GetNowSysSet(&tempSet, RT_NULL, RT_NULL, RT_NULL, RT_NULL, RT_NULL);

    tempNow = getSensorDataByFunc(monitor, F_S_TEMP);
    if(VALUE_NULL != tempNow)
    {
        if(DAY_TIME == GetSysSet()->dayOrNight)
        {
            coolTarge = tempSet.dayCoolingTarget;
            HeatTarge = tempSet.dayHeatingTarget;
        }
        else if(NIGHT_TIME == GetSysSet()->dayOrNight)
        {
            coolTarge = tempSet.nightCoolingTarget;
            HeatTarge = tempSet.nightHeatingTarget;
        }

        if(tempNow >= coolTarge)
        {
            //打开所以制冷功能设备
            uart.DeviceCtrl(monitor, F_COOL, ON);
        }
        else if(tempNow <= (coolTarge - tempSet.tempDeadband))
        {
            uart.DeviceCtrl(monitor, F_COOL, OFF);
        }

        if(tempNow <= HeatTarge)
        {
            uart.DeviceCtrl(monitor, F_HEAT, ON);
        }
        else if(tempNow >= HeatTarge + tempSet.tempDeadband)
        {
            uart.DeviceCtrl(monitor, F_HEAT, OFF);
        }
    }
}

//执行设备手动控制功能
void menualHandProgram(type_monitor_t *monitor, type_uart_class deviceUart, type_uart_class lineUart)
{
    int         i           = 0;
    int         port        = 0;
    device_t    *device     = RT_NULL;
    line_t      *line       = RT_NULL;
    time_t      nowTime     = getTimeStamp();

    for(i = 0; i < monitor->device_size; i++)
    {
        device = &monitor->device[i];
        for(port = 0; port < device->storage_size; port++)
        {
            u16 value = 0;

            if(MANUAL_HAND_OFF == device->port[port].manual.manual)
            {
                 deviceUart.DeviceCtrlSingle(device, port, OFF);
            }
            else if(MANUAL_HAND_ON == device->port[port].manual.manual)
            {
                //如果是在开启的时间内就需要打开
                if((nowTime >= device->port[port].manual.manual_on_time_save) &&
                   (nowTime <= (device->port[port].manual.manual_on_time + device->port[port].manual.manual_on_time_save)))
                {
                    deviceUart.DeviceCtrlSingle(device, port, ON);
                }
                else
                {
                    device->port[port].manual.manual = MANUAL_NO_HAND;
                    deviceUart.DeviceCtrlSingle(device, port, OFF);
                }
            }
            else
            {
                //如果上一个时刻为手动开启 突然关闭手动要将状态还回去
                if((nowTime >= device->port[port].manual.manual_on_time_save) &&
                   (nowTime <= (device->port[port].manual.manual_on_time + device->port[port].manual.manual_on_time_save)))
                {
                    deviceUart.DeviceCtrlSingle(device, port, OFF);
                }
            }


        }
    }

    for(i = 0; i < monitor->line_size; i++)
    {
        line = &monitor->line[i];
        if(MANUAL_HAND_OFF == line->_manual.manual)
        {
            lineUart.LineCtrl(monitor, i, 0, 0);
        }
        else if(MANUAL_HAND_ON == line->_manual.manual)
        {
            if((nowTime >= line->_manual.manual_on_time_save) &&
               (nowTime <= (line->_manual.manual_on_time + line->_manual.manual_on_time_save)))
            {
                lineUart.LineCtrl(monitor, i, ON, 100);
            }
            else
            {
                line->_manual.manual = MANUAL_NO_HAND;
                lineUart.LineCtrl(monitor, i, 0, 0);
            }
        }
    }
}
