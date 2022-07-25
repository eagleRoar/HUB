/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-14     Administrator       the first version
 */

#include "Uart.h"
#include "Module.h"
#include "UartDataLayer.h"
#include "CloudProtocolBusiness.h"
#include "Recipe.h"
#include "UartBussiness.h"

extern  sys_set_t       sys_set;
extern  type_sys_time   sys_time;

extern sys_set_t *GetSysSet(void);
extern sys_tank_t *GetSysTank(void);
extern void getRealTimeForMat(type_sys_time *);

rt_err_t GetValueU8(cJSON *temp, type_kv_u8 *data)
{
    rt_err_t ret = RT_ERROR;

    cJSON *json = cJSON_GetObjectItem(temp, data->name);
    if(NULL != json)
    {
        data->value = json->valueint;
        ret = RT_EOK;
    }

    if(RT_ERROR == ret)
    {
        data->value = 0x00;
        LOG_E("parse u8 err, name %s",data->name);
    }

    return ret;
}

rt_err_t GetValueByInt(cJSON *temp, char *name, int *value)
{
    rt_err_t ret = RT_ERROR;

    cJSON *json = cJSON_GetObjectItem(temp, name);
    if(NULL != json)
    {
        *value = json->valueint;
        ret = RT_EOK;
    }

    if(RT_ERROR == ret)
    {
        value = 0x00;
        LOG_E("parse int err, name %s",name);
    }

    return ret;
}

rt_err_t GetValueByU8(cJSON *temp, char *name, u8 *value)
{
    type_kv_u8  data;
    rt_err_t    ret     = RT_ERROR;

    rt_memcpy(data.name, name, KEYVALUE_NAME_SIZE);
    ret = GetValueU8(temp, &data);
    *value = data.value;

    return ret;
}

rt_err_t GetValueU16(cJSON *temp, type_kv_u16 *data)
{
    rt_err_t ret = RT_ERROR;

    cJSON *json = cJSON_GetObjectItem(temp, data->name);
    if(NULL != json)
    {
        data->value = json->valueint;
        ret = RT_EOK;
    }

    if(RT_ERROR == ret)
    {
        data->value = 0x0000;
        LOG_E("parse u16 err, name %s",data->name);
    }

    return ret;
}

rt_err_t GetValueByU16(cJSON *temp, char *name, u16 *value)
{
    type_kv_u16 data;
    rt_err_t    ret     = RT_ERROR;

    rt_memcpy(data.name, name, KEYVALUE_NAME_SIZE);
    ret = GetValueU16(temp, &data);
    *value = data.value;

    return ret;
}

rt_err_t GetValueC16(cJSON *temp, type_kv_c16 *data)
{
    rt_err_t ret = RT_ERROR;

    cJSON *json = cJSON_GetObjectItem(temp, data->name);
    if(NULL != json)
    {
        rt_memcpy(data->value, json->valuestring, 16);
        ret = RT_EOK;
    }

    if(RT_ERROR == ret)
    {
        LOG_E("parse c16 err, name %s",data->name);
        rt_memset(data->value, ' ', 16);
    }

    return ret;
}

rt_err_t GetValueByC16(cJSON *temp, char *name, char *value)
{
    type_kv_c16 data;
    rt_err_t    ret     = RT_ERROR;

    strcpy(data.name, name);
    ret = GetValueC16(temp, &data);
    strcpy(value, data.value);

    return ret;
}


void CmdSetTempValue(char *data)
{
    cJSON *temp = RT_NULL;

    temp = cJSON_Parse(data);

    if(RT_NULL != temp)
    {
        GetValueC16(temp, &sys_set.tempSet.msgid);
        GetValueU16(temp, &sys_set.tempSet.dayCoolingTarget);
        GetValueU16(temp, &sys_set.tempSet.dayHeatingTarget);
        GetValueU16(temp, &sys_set.tempSet.nightCoolingTarget);
        GetValueU16(temp, &sys_set.tempSet.nightHeatingTarget);
        GetValueU8(temp, &sys_set.tempSet.coolingDehumidifyLock);


        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetTempValue err");
    }
}

void CmdGetTempValue(char *data)
{
    cJSON *temp = RT_NULL;

    temp = cJSON_Parse(data);

    if(RT_NULL != temp)
    {
        GetValueC16(temp, &sys_set.tempSet.msgid);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetTempValue err");
    }
}

void CmdSetCo2(char *data)
{
    cJSON *temp = RT_NULL;

    temp = cJSON_Parse(data);

    if(RT_NULL != temp)
    {
        GetValueC16(temp, &sys_set.co2Set.msgid);
        GetValueU16(temp, &sys_set.co2Set.dayCo2Target);
        GetValueU16(temp, &sys_set.co2Set.nightCo2Target);
        GetValueU8(temp, &sys_set.co2Set.isFuzzyLogic);
        GetValueU8(temp, &sys_set.co2Set.coolingLock);
        GetValueU8(temp, &sys_set.co2Set.dehumidifyLock);
        GetValueByInt(temp, "co2Corrected", &sys_set.co2Set.co2Corrected);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetCo2 err");
    }
}

void CmdGetCo2(char *data)
{
    cJSON *temp = RT_NULL;

    temp = cJSON_Parse(data);

    if(RT_NULL != temp)
    {
        GetValueC16(temp, &sys_set.co2Set.msgid);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdGetCo2 err");
    }
}

void CmdSetHumi(char *data)
{
    cJSON *temp = RT_NULL;

    temp = cJSON_Parse(data);

    if(RT_NULL != temp)
    {
        GetValueC16(temp, &sys_set.humiSet.msgid);
        GetValueU16(temp, &sys_set.humiSet.dayHumiTarget);
        GetValueU16(temp, &sys_set.humiSet.dayDehumiTarget);
        GetValueU16(temp, &sys_set.humiSet.nightHumiTarget);
        GetValueU16(temp, &sys_set.humiSet.nightDehumiTarget);
        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetHumi err");
    }
}

void CmdGetHumi(char *data)
{
    cJSON *temp = RT_NULL;

    temp = cJSON_Parse(data);

    if(RT_NULL != temp)
    {
        GetValueC16(temp, &sys_set.humiSet.msgid);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdGetHumi err");
    }
}

void CmdGetDeviceList(char *data, cloudcmd_t *cmd)
{
    cJSON *temp = RT_NULL;

    temp = cJSON_Parse(data);

    if(RT_NULL != temp)
    {
        GetValueC16(temp, &cmd->msgid);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdGetDeviceList err");
    }
}

void CmdGetLine(char *data, proLine_t *line)
{
    cJSON *temp = RT_NULL;

    temp = cJSON_Parse(data);

    if(RT_NULL != temp)
    {
        GetValueC16(temp, &line->msgid);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdGetLine1 err");
    }
}

void CmdFindLocation(char *data, cloudcmd_t *cmd)
{
    cJSON *temp = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueC16(temp, &cmd->msgid);
        GetValueU16(temp, &cmd->get_id);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdFindLocation err");
    }
}

void CmdGetPortSet(char *data, cloudcmd_t *cmd)
{
    cJSON *temp = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueC16(temp, &cmd->msgid);
        GetValueU16(temp, &cmd->get_port_id);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdGetPortSet err");
    }
}

void CmdSetPortSet(char *data, cloudcmd_t *cmd)
{
    u8              list_num    = 0;
    u8              addr        = 0;
    u8              port        = 0;
    cJSON           *temp       = RT_NULL;
    cJSON           *list       = RT_NULL;
    cJSON           *list_item  = RT_NULL;
    device_time4_t  *device     = RT_NULL;
    timer12_t       *timer12    = RT_NULL;
    type_kv_u8      temp_v8;
    type_kv_u16     temp_v16;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueC16(temp, &cmd->msgid);
        GetValueU16(temp, &cmd->get_port_id);

        if(cmd->get_port_id.value > 0xff)
        {
            addr = cmd->get_port_id.value >> 8;
            port = cmd->get_port_id.value;
        }
        else
        {
            addr = cmd->get_port_id.value;
            port = 0;
        }

        device = GetDeviceByAddr(GetMonitor(), addr);
        timer12 = GetTimerByAddr(GetMonitor(), addr);

        LOG_I("add = %d, port = %d",addr,port);
        if(device != RT_NULL)
        {
//            LOG_I("find device name %s",device->name);
            rt_memcpy(temp_v8.name, "manual", KEYVALUE_NAME_SIZE);
            GetValueU8(temp, &temp_v8);
            device->_storage[port]._port.manual = temp_v8.value;
            rt_memcpy(temp_v16.name, "manualOnTime", KEYVALUE_NAME_SIZE);
            GetValueU16(temp, &temp_v16);
            device->_storage[port]._port.manual_on_time = temp_v16.value;
            GetValueByU8(temp, "hotStartDelay", &device->hotStartDelay);
            if(HVAC_6_TYPE == device->type)
            {
                rt_memcpy(temp_v8.name, "manualOnMode", KEYVALUE_NAME_SIZE);
                GetValueU8(temp, &temp_v8);
                device->_hvac.manualOnMode = temp_v8.value;
                rt_memcpy(temp_v8.name, "fanNormallyOpen", KEYVALUE_NAME_SIZE);
                GetValueU8(temp, &temp_v8);
                device->_hvac.fanNormallyOpen = temp_v8.value;
                rt_memcpy(temp_v8.name, "hvacMode", KEYVALUE_NAME_SIZE);
                GetValueU8(temp, &temp_v8);
                device->_hvac.hvacMode = temp_v8.value;
            }

            if(TIMER_TYPE == device->type)
            {
                GetValueByU8(temp, "mode", &device->mode);

                if(BY_SCHEDULE == device->mode)
                {
                    list = cJSON_GetObjectItem(temp, "list");

                    if(RT_NULL != list)
                    {
                        list_num = cJSON_GetArraySize(list);
                        LOG_D("list_num == %d",list_num);//Justin debug
                        if(list_num > TIMER_GROUP)
                        {
                            list_num = TIMER_GROUP;
                        }

                        rt_memset((u8 *)&device->_storage[0]._time4_ctl, 0, sizeof(type_timmer_timmer));

                        for(int index = 0; index < list_num; index++)
                        {
                            list_item = cJSON_GetArrayItem(list, index);

                            if(RT_NULL != list_item)
                            {
                                GetValueByU16(list_item, "onAt", &device->_storage[0]._time4_ctl._timer[index].on_at);
                                GetValueByU16(list_item, "duration", &device->_storage[0]._time4_ctl._timer[index].duration);
                                GetValueByU8(list_item, "en", &device->_storage[0]._time4_ctl._timer[index].en);
                            }
                        }
                    }
                    else
                    {
                        LOG_E("CmdSetPortSet apply memory for list fail");
                    }
                }
                else if(BY_RECYCLE == device->mode)
                {
                    GetValueByU16(list_item, "startAt", &device->_recycle.startAt);
                    GetValueByU16(list_item, "duration", &device->_recycle.duration);
                    GetValueByU16(list_item, "pauseTime", &device->_recycle.pauseTime);
                }
            }
        }
        else if(timer12 != RT_NULL)
        {
//            LOG_I("find timer name %s",timer12->name);
            rt_memcpy(temp_v8.name, "manual", KEYVALUE_NAME_SIZE);
            GetValueU8(temp, &temp_v8);
            timer12->manual = temp_v8.value;
            rt_memcpy(temp_v16.name, "manualOnTime", KEYVALUE_NAME_SIZE);
            GetValueU16(temp, &temp_v16);
            timer12->manual_on_time = temp_v16.value;

            rt_memcpy(temp_v8.name, "mode", KEYVALUE_NAME_SIZE);
            GetValueU8(temp, &temp_v8);
            timer12->mode = temp_v8.value;

            list = cJSON_GetObjectItem(temp, "list");

            if(RT_NULL != list)
            {
                list_num = cJSON_GetArraySize(list);

                for(int index = 0; index < list_num; index++)
                {
                    list_item = cJSON_GetArrayItem(list, index);

                    if(RT_NULL != list_item)
                    {
                        rt_memcpy(temp_v16.name, "onAt", KEYVALUE_NAME_SIZE);
                        GetValueU16(list_item, &temp_v16);
                        timer12->_time12_ctl[port]._timer[index].on_at = temp_v16.value;
                        rt_memcpy(temp_v16.name, "duration", KEYVALUE_NAME_SIZE);
                        GetValueU16(list_item, &temp_v16);
                        timer12->_time12_ctl[port]._timer[index].duration = temp_v16.value;
                        rt_memcpy(temp_v8.name, "en", KEYVALUE_NAME_SIZE);
                        GetValueU8(list_item, &temp_v8);
                        timer12->_time12_ctl[port]._timer[index].en = temp_v8.value;
                    }
                }
            }
            rt_memcpy(temp_v16.name, "startAt", KEYVALUE_NAME_SIZE);
            GetValueU16(temp, &temp_v16);
            timer12->_recycle.startAt = temp_v16.value;
            rt_memcpy(temp_v16.name, "duration", KEYVALUE_NAME_SIZE);
            GetValueU16(temp, &temp_v16);
            timer12->_recycle.duration = temp_v16.value;
            rt_memcpy(temp_v16.name, "pauseTime", KEYVALUE_NAME_SIZE);
            GetValueU16(temp, &temp_v16);
            timer12->_recycle.pauseTime = temp_v16.value;
            rt_memcpy(temp_v8.name, "times", KEYVALUE_NAME_SIZE);
            GetValueU8(temp, &temp_v8);
            timer12->_recycle.times = temp_v8.value;
        }
        else {
            LOG_E("no find device or timer");
        }

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetPortSet err");
    }
}

void CmdGetDeadBand(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueC16(temp, &cmd->msgid);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdGetDeadBand err");
    }
}

void CmdSetDeadBand(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueC16(temp, &cmd->msgid);
        GetValueU16(temp, &sys_set.tempSet.tempDeadband);
        GetValueU16(temp, &sys_set.co2Set.co2Deadband);
        GetValueU16(temp, &sys_set.humiSet.humidDeadband);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetDeadBand err");
    }
}

void CmdDeleteDevice(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueC16(temp, &cmd->msgid);
        GetValueU16(temp, &cmd->delete_id);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdDeleteDevice err");
    }
}

void CmdGetSchedule(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueC16(temp, &cmd->msgid);
        GetValueU16(temp, &cmd->delete_id);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdGetSchedule err");
    }
}

void CmdAddRecipe(char *data, cloudcmd_t *cmd)//Justin debug 还没完成
{
    cJSON   *temp       = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueC16(temp, &cmd->msgid);
        GetValueC16(temp, &cmd->recipe_name);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdAddRecipe err");
    }
}

void CmdSetRecipe(char *data, cloudcmd_t *cmd)//Justin debug 还没完成
{
    cJSON           *temp       = RT_NULL;
    cJSON           *line       = RT_NULL;
    recipe_t        *recipe     = RT_NULL;

    temp = cJSON_Parse(data);
    recipe = rt_malloc(sizeof(recipe_t));
    if(RT_NULL != temp)
    {
        GetValueC16(temp, &cmd->msgid);

        if(RT_NULL != recipe)
        {
            GetValueByU8(temp, "id", &recipe->id);
            GetValueByC16(temp, "name", recipe->name);
            GetValueByU8(temp, "color", &recipe->color);
            GetValueByU16(temp, "dayCoolingTarget", &recipe->dayCoolingTarget);
            GetValueByU16(temp, "dayHeatingTarget", &recipe->dayHeatingTarget);
            GetValueByU16(temp, "nightCoolingTarget", &recipe->nightCoolingTarget);
            GetValueByU16(temp, "nightHeatingTarget", &recipe->nightHeatingTarget);
            GetValueByU16(temp, "dayHumidifyTarget", &recipe->dayHumidifyTarget);
            GetValueByU16(temp, "dayDehumidifyTarget", &recipe->dayDehumidifyTarget);
            GetValueByU16(temp, "nightHumidifyTarget", &recipe->nightHumidifyTarget);
            GetValueByU16(temp, "nightDehumidifyTarget", &recipe->nightDehumidifyTarget);
            GetValueByU16(temp, "dayCo2Target", &recipe->dayCo2Target);
            GetValueByU16(temp, "nightCo2Target", &recipe->nightCo2Target);

            line = cJSON_GetObjectItem(temp, "line1");
            if(RT_NULL != line)
            {
                GetValueByU8(line, "brightMode", &recipe->line_list[0].brightMode);
                GetValueByU8(line, "byPower", &recipe->line_list[0].byPower);
                GetValueByU16(line, "byAutoDimming", &recipe->line_list[0].byAutoDimming);
                GetValueByU8(line, "mode", &recipe->line_list[0].mode);
                GetValueByU16(line, "lightOn", &recipe->line_list[0].lightOn);
                GetValueByU16(line, "lightOff", &recipe->line_list[0].lightOff);
                GetValueByU16(line, "firstCycleTime", &recipe->line_list[0].firstCycleTime);
                GetValueByU16(line, "duration", &recipe->line_list[0].duration);
                GetValueByU16(line, "pauseTime", &recipe->line_list[0].pauseTime);
            }

            line = cJSON_GetObjectItem(temp, "line2");
            if(RT_NULL != line)
            {
                GetValueByU8(line, "brightMode", &recipe->line_list[1].brightMode);
                GetValueByU8(line, "byPower", &recipe->line_list[1].byPower);
                GetValueByU16(line, "byAutoDimming", &recipe->line_list[1].byAutoDimming);
                GetValueByU8(line, "mode", &recipe->line_list[1].mode);
                GetValueByU16(line, "lightOn", &recipe->line_list[1].lightOn);
                GetValueByU16(line, "lightOff", &recipe->line_list[1].lightOff);
                GetValueByU16(line, "firstCycleTime", &recipe->line_list[1].firstCycleTime);
                GetValueByU16(line, "duration", &recipe->line_list[1].duration);
                GetValueByU16(line, "pauseTime", &recipe->line_list[1].pauseTime);
            }

            AddRecipe(recipe, GetSysRecipt());
            cmd->recipe_id = recipe->id;
            rt_free(recipe);
            recipe = RT_NULL;
        }

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetRecipe err");
    }
}

void CmdSetTank(char *data, cloudcmd_t *cmd)//Justin debug 未验证
{
    cJSON   *temp       = RT_NULL;
    tank_t  *tank       = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueC16(temp, &cmd->msgid);

        tank = rt_malloc(sizeof(tank_t));
        if(RT_NULL != tank)
        {
            GetValueByU8(temp, "tankNo", &tank->tankNo);
            cmd->tank_no = tank->tankNo;
            GetValueByU8(temp, "autoFillValveId", &tank->autoFillValveId);
            GetValueByU8(temp, "autoFillHeight", &tank->autoFillHeight);
            GetValueByU8(temp, "autoFillFulfilHeight", &tank->autoFillFulfilHeight);
            GetValueByU8(temp, "highEcProtection", &tank->highEcProtection);
            GetValueByU8(temp, "lowPhProtection", &tank->lowPhProtection);
            GetValueByU8(temp, "highPhProtection", &tank->highPhProtection);
            InsertTankToTable(GetSysTank(), *tank);

            rt_free(tank);
            tank = RT_NULL;
        }
        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetTank err");
    }
}

void CmdGetHubState(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp       = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueC16(temp, &cmd->msgid);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdGetHubState err");
    }
}

void CmdSetHubName(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp       = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueC16(temp, &cmd->msgid);
        GetValueByC16(temp, "name", GetHub()->name);
        GetValueByU8(temp, "nameSeq", &GetHub()->nameSeq);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetHubName err");
    }
}

void CmdSetWarn(char *data, cloudcmd_t *cmd, sys_set_t *set)//Justin debug 未验证
{
    cJSON   *temp       = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueC16(temp, &cmd->msgid);
        GetValueByU16(temp, "dayTempMin", &set->sysWarn.dayTempMin);
        GetValueByU16(temp, "dayTempMax", &set->sysWarn.dayTempMax);
        GetValueByU8(temp, "dayTempEn", &set->sysWarn.dayTempEn);
        GetValueByU8(temp, "dayhumidMin", &set->sysWarn.dayhumidMin);
        GetValueByU8(temp, "dayhumidMax", &set->sysWarn.dayhumidMax);
        GetValueByU8(temp, "dayhumidEn", &set->sysWarn.dayhumidEn);
        GetValueByU16(temp, "dayCo2Min", &set->sysWarn.dayCo2Min);
        GetValueByU16(temp, "dayCo2Max", &set->sysWarn.dayCo2Max);
        GetValueByU8(temp, "dayCo2En", &set->sysWarn.dayCo2En);
        GetValueByU8(temp, "dayCo2Buzz", &set->sysWarn.dayCo2Buzz);
        GetValueByU16(temp, "dayVpdMin", &set->sysWarn.dayVpdMin);
        GetValueByU16(temp, "dayVpdMax", &set->sysWarn.dayVpdMax);
        GetValueByU8(temp, "dayVpdEn", &set->sysWarn.dayVpdEn);
        GetValueByU16(temp, "dayParMin", &set->sysWarn.dayParMin);
        GetValueByU16(temp, "dayParMax", &set->sysWarn.dayParMax);
        GetValueByU8(temp, "dayParEn", &set->sysWarn.dayParEn);
        GetValueByU16(temp, "nightTempMin", &set->sysWarn.nightTempMin);
        GetValueByU16(temp, "nightTempMax", &set->sysWarn.nightTempMax);
        GetValueByU8(temp, "nightTempEn", &set->sysWarn.nightTempEn);
        GetValueByU8(temp, "nighthumidMin", &set->sysWarn.nighthumidMin);
        GetValueByU8(temp, "nighthumidMax", &set->sysWarn.nighthumidMax);
        GetValueByU8(temp, "nighthumidEn", &set->sysWarn.nighthumidEn);
        GetValueByU16(temp, "nightCo2Min", &set->sysWarn.nightCo2Min);
        GetValueByU16(temp, "nightCo2Max", &set->sysWarn.nightCo2Max);
        GetValueByU8(temp, "nightCo2En", &set->sysWarn.nightCo2En);
        GetValueByU8(temp, "nightCo2Buzz", &set->sysWarn.nightCo2Buzz);
        GetValueByU16(temp, "nightVpdMin", &set->sysWarn.nightVpdMin);
        GetValueByU16(temp, "nightVpdMax", &set->sysWarn.nightVpdMax);
        GetValueByU8(temp, "nightVpdEn", &set->sysWarn.nightVpdEn);
        GetValueByU8(temp, "phEn", &set->sysWarn.phEn);
        GetValueByU8(temp, "ecEn", &set->sysWarn.ecEn);
        GetValueByU8(temp, "wtEn", &set->sysWarn.wtEn);
        GetValueByU8(temp, "wlEn", &set->sysWarn.wlEn);
        GetValueByU8(temp, "offlineEn", &set->sysWarn.offlineEn);
        GetValueByU8(temp, "lightEn", &set->sysWarn.lightEn);
        GetValueByU8(temp, "smokeEn", &set->sysWarn.smokeEn);
        GetValueByU8(temp, "waterEn", &set->sysWarn.waterEn);
        GetValueByU8(temp, "autoFillTimeout", &set->sysWarn.autoFillTimeout);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetWarn err");
    }
}

void CmdGetWarn(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp       = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueC16(temp, &cmd->msgid);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdGetWarn err");
    }
}


void CmdGetSysSet(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp       = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueC16(temp, &cmd->msgid);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdGetSysSet err");
    }
}

void CmdSetSysSet(char *data, cloudcmd_t *cmd, sys_para_t *para)
{
    cJSON   *temp       = RT_NULL;
    char    ntpzone[16];

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueC16(temp, &cmd->msgid);
        GetValueByC16(temp, "ntpzone", ntpzone);
        ntpzone[7] = '\0';
        strcpy(para->ntpzone, ntpzone);
        GetValueByU8(temp, "tempUnit", &para->tempUnit);
        GetValueByU8(temp, "ecUnit", &para->ecUnit);
        GetValueByU8(temp, "timeFormat", &para->timeFormat);
        GetValueByU8(temp, "dayNightMode", &para->dayNightMode);
        GetValueByU16(temp, "photocellSensitivity", &para->photocellSensitivity);
        GetValueByU16(temp, "dayTime", &para->dayTime);
        GetValueByU16(temp, "nightTime", &para->nightTime);
        GetValueByU8(temp, "maintain", &para->maintain);
        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetSysSet err");
    }
}

void CmdSetPortName(char *data, cloudcmd_t *cmd)
{
    u8      addr        = 0;
    u8      port        = 0;
    char    name[MODULE_NAMESZ];
    cJSON   *temp       = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueC16(temp, &cmd->msgid);
        GetValueByU16(temp, "id", &cmd->set_port_id);
        GetValueByC16(temp, "name", name);

        if(cmd->set_port_id >= 0xFF)
        {
            addr = cmd->set_port_id >> 8;
            port = cmd->set_port_id;
        }
        else
        {
            addr = cmd->set_port_id;
            port = 0;
        }

        if(0 == port)
        {
            rt_memcpy(GetDeviceByAddr(GetMonitor(), addr)->name, name, MODULE_NAMESZ);
        }
        else
        {
            rt_memcpy(GetDeviceByAddr(GetMonitor(), addr)->_storage[port]._port.name, name, STORAGE_NAMESZ);
        }

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetPortName err");
    }
}

void CmdSetSchedule(char *data, cloudcmd_t *cmd)//Justin debug 未验证
{
    u8      index       = 0;
    u8      list_sum    = 0;
    type_kv_u8      temp_u8;
    type_kv_u16     temp_u16;
    cJSON   *temp       = RT_NULL;
    cJSON   *list       = RT_NULL;
    cJSON   *item       = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueC16(temp, &cmd->msgid);
        strcpy(temp_u8.name, "en");
        GetValueU8(temp, &temp_u8);
        strcpy(temp_u16.name, "starts");
        GetValueU16(temp, &temp_u16);

        list = cJSON_GetObjectItem(temp, "list");

        if(RT_NULL != list)
        {
            list_sum = cJSON_GetArraySize(list);

            for(index = 0; index < list_sum; index++)
            {
                item = cJSON_GetArrayItem(list, index);

                if(RT_NULL != item)
                {
                    strcpy(temp_u8.name, "recipeId");
                    GetValueU8(item, &temp_u8);
                    strcpy(temp_u8.name, "duration");
                    GetValueU8(item, &temp_u8);
                }
            }
        }

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetSchedule err");
    }
}

void CmdSetSysTime(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp = RT_NULL;
    char    time[4];

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueC16(temp, &cmd->msgid);
        GetValueC16(temp, &cmd->sys_time);

        rt_memset(time, 0, 4);
        rt_memcpy(time, &cmd->sys_time.value[0], 4);
        sys_time.year = atoi(time);
        rt_memset(time, 0, 4);
        rt_memcpy(time, &cmd->sys_time.value[4], 2);
        sys_time.month = atoi(time);
        rt_memset(time, 0, 4);
        rt_memcpy(time, &cmd->sys_time.value[6], 2);
        sys_time.day = atoi(time);
        rt_memset(time, 0, 4);
        rt_memcpy(time, &cmd->sys_time.value[8], 2);
        sys_time.hour = atoi(time);
        rt_memset(time, 0, 4);
        rt_memcpy(time, &cmd->sys_time.value[10], 2);
        sys_time.minute = atoi(time);
        rt_memset(time, 0, 4);
        rt_memcpy(time, &cmd->sys_time.value[12], 2);
        sys_time.second = atoi(time);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetSysTime err");
    }
}

void CmdSetLine(char *data, proLine_t *line)
{
    cJSON *temp = RT_NULL;
    u16   time  = 0;

    temp = cJSON_Parse(data);

    if(RT_NULL != temp)
    {
        GetValueC16(temp, &line->msgid);
        GetValueU8(temp, &line->lightsType);
        GetValueU8(temp, &line->brightMode);
        GetValueU8(temp, &line->byPower);
        GetValueU16(temp, &line->byAutoDimming);
        GetValueU8(temp, &line->mode);
        if(LINE_BY_TIMER == line->mode.value)
        {
            GetValueU16(temp, &line->lightOn);
            GetValueU16(temp, &line->lightOff);
        }
        else if(LINE_BY_CYCLE == line->mode.value)
        {
            GetValueByU16(temp, "firstCycleTime", &time);
            if(time != line->firstCycleTime.value)
            {
                line->firstCycleTime.value = time;
                line->isRunFirstCycle = 0;
            }
            GetValueU16(temp, &line->duration);
            GetValueU16(temp, &line->pauseTime);
        }
        GetValueU8(temp, &line->hidDelay);
        GetValueU16(temp, &line->tempStartDimming);
        GetValueU16(temp, &line->tempOffDimming);
        GetValueU8(temp, &line->sunriseSunSet);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetHumi err");
    }
}

char *SendHubReportWarn(char *cmd)
{
    char            *str        = RT_NULL;
    char            name[11];
    cJSON           *json       = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "sn", GetSnName(name));

        cJSON_AddNumberToObject(json, "type", 4);//Justin debug 仅仅测试
        cJSON_AddNumberToObject(json, "warning", 5);//Justin debug 仅仅测试
        cJSON_AddStringToObject(json, "name", "Co2");//Justin debug 仅仅测试
        cJSON_AddNumberToObject(json, "value", 2000);//Justin debug 仅仅测试
        cJSON_AddStringToObject(json, "ntpzone", GetSysSet()->sysPara.ntpzone);
        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }

    return str;
}

char *SendHubReport(char *cmd)
{
    char            *str        = RT_NULL;
    char            model[15];
    char            name[11];
    cJSON           *json       = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "sn", GetSnName(name));
        cJSON_AddStringToObject(json, "model", GetModelByType(HUB_TYPE, model, 15));
        cJSON_AddStringToObject(json, "name", GetHub()->name);
        cJSON_AddNumberToObject(json, "nameSeq", GetHub()->nameSeq);

        for(u8 index = 0; index < SENSOR_VALUE_MAX; index++)
        {
            if(F_S_CO2 == GetSensorByType(GetMonitor(), BHS_TYPE)->__stora[index].func)
            {
                cJSON_AddNumberToObject(json, "co2", GetSensorByType(GetMonitor(), BHS_TYPE)->__stora[index].value);
            }
            else if(F_S_TEMP == GetSensorByType(GetMonitor(), BHS_TYPE)->__stora[index].func)
            {
                cJSON_AddNumberToObject(json, "temp", GetSensorByType(GetMonitor(), BHS_TYPE)->__stora[index].value);
            }
            else if(F_S_HUMI == GetSensorByType(GetMonitor(), BHS_TYPE)->__stora[index].func)
            {
                cJSON_AddNumberToObject(json, "humid", GetSensorByType(GetMonitor(), BHS_TYPE)->__stora[index].value);
            }
        }

        cJSON_AddNumberToObject(json, "co2Lock", GetSysSet()->co2Set.dehumidifyLock.value);
        cJSON_AddNumberToObject(json, "tempLock", GetSysSet()->tempSet.coolingDehumidifyLock.value);
        cJSON_AddNumberToObject(json, "humidLock", GetSysSet()->tempSet.coolingDehumidifyLock.value);
        cJSON_AddNumberToObject(json, "ppfd", GetSysSet()->line1Set.byAutoDimming.value);
        cJSON_AddNumberToObject(json, "vpd", getVpd());
        cJSON_AddNumberToObject(json, "dayNight", GetSysSet()->dayOrNight);
        cJSON_AddNumberToObject(json, "maintain", GetSysSet()->sysPara.maintain);

        cJSON_AddStringToObject(json, "ntpzone", GetSysSet()->sysPara.ntpzone);
        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }

    return str;
}

char *ReplyGetTempValue(char *cmd)
{
    char *str = RT_NULL;
    cJSON *json = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, sys_set.tempSet.msgid.name, sys_set.tempSet.msgid.value);
        cJSON_AddStringToObject(json, sys_set.tempSet.sn.name, sys_set.tempSet.sn.value);
        cJSON_AddNumberToObject(json, sys_set.tempSet.dayCoolingTarget.name, sys_set.tempSet.dayCoolingTarget.value);
        cJSON_AddNumberToObject(json, sys_set.tempSet.dayHeatingTarget.name, sys_set.tempSet.dayHeatingTarget.value);
        cJSON_AddNumberToObject(json, sys_set.tempSet.nightCoolingTarget.name, sys_set.tempSet.nightCoolingTarget.value);
        cJSON_AddNumberToObject(json, sys_set.tempSet.nightHeatingTarget.name, sys_set.tempSet.nightHeatingTarget.value);
        cJSON_AddNumberToObject(json, sys_set.tempSet.coolingDehumidifyLock.name, sys_set.tempSet.coolingDehumidifyLock.value);
        sys_set.tempSet.timestamp.value = ReplyTimeStamp();
        cJSON_AddNumberToObject(json, sys_set.tempSet.timestamp.name, sys_set.tempSet.timestamp.value);

        str = cJSON_PrintUnformatted(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplyGetCo2(char *cmd)
{
    char *str = RT_NULL;
    cJSON *json = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, sys_set.co2Set.msgid.name, sys_set.co2Set.msgid.value);
        cJSON_AddStringToObject(json, sys_set.co2Set.sn.name, sys_set.co2Set.sn.value);
        cJSON_AddNumberToObject(json, sys_set.co2Set.dayCo2Target.name, sys_set.co2Set.dayCo2Target.value);
        cJSON_AddNumberToObject(json, sys_set.co2Set.nightCo2Target.name, sys_set.co2Set.nightCo2Target.value);
        cJSON_AddNumberToObject(json, sys_set.co2Set.isFuzzyLogic.name, sys_set.co2Set.isFuzzyLogic.value);
        cJSON_AddNumberToObject(json, sys_set.co2Set.coolingLock.name, sys_set.co2Set.coolingLock.value);
        cJSON_AddNumberToObject(json, sys_set.co2Set.dehumidifyLock.name, sys_set.co2Set.dehumidifyLock.value);
        cJSON_AddNumberToObject(json, "co2Corrected", sys_set.co2Set.co2Corrected);
        sys_set.co2Set.timestamp.value = ReplyTimeStamp();
        cJSON_AddNumberToObject(json, sys_set.co2Set.timestamp.name, sys_set.co2Set.timestamp.value);

        str = cJSON_PrintUnformatted(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplyGetHumi(char *cmd)
{
    char *str = RT_NULL;
    cJSON *json = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, sys_set.humiSet.msgid.name, sys_set.humiSet.msgid.value);
        cJSON_AddStringToObject(json, sys_set.humiSet.sn.name, sys_set.humiSet.sn.value);
        cJSON_AddNumberToObject(json, sys_set.humiSet.dayHumiTarget.name, sys_set.humiSet.dayHumiTarget.value);
        cJSON_AddNumberToObject(json, sys_set.humiSet.dayDehumiTarget.name, sys_set.humiSet.dayDehumiTarget.value);
        cJSON_AddNumberToObject(json, sys_set.humiSet.nightHumiTarget.name, sys_set.humiSet.nightHumiTarget.value);
        cJSON_AddNumberToObject(json, sys_set.humiSet.nightDehumiTarget.name, sys_set.humiSet.nightDehumiTarget.value);
        sys_set.humiSet.timestamp.value = ReplyTimeStamp();
        cJSON_AddNumberToObject(json, sys_set.humiSet.timestamp.name, sys_set.humiSet.timestamp.value);

        str = cJSON_PrintUnformatted(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplyGetLine(char *cmd, type_kv_c16 msgid, proLine_t line)
{
    char    name[11];
    char    *str        = RT_NULL;
    cJSON   *json       = cJSON_CreateObject();

//    rt_memset(name, ' ', 16);

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, line.msgid.name, line.msgid.value);
        cJSON_AddStringToObject(json, line.sn.name, GetSnName(name));
        cJSON_AddNumberToObject(json, line.lightsType.name, line.lightsType.value);
        cJSON_AddNumberToObject(json, line.brightMode.name, line.brightMode.value);
        cJSON_AddNumberToObject(json, line.byPower.name, line.byPower.value);
        cJSON_AddNumberToObject(json, line.byAutoDimming.name, line.byAutoDimming.value);
        cJSON_AddNumberToObject(json, line.mode.name, line.mode.value);
        cJSON_AddNumberToObject(json, line.lightOn.name, line.lightOn.value);
        cJSON_AddNumberToObject(json, line.lightOff.name, line.lightOff.value);
        cJSON_AddNumberToObject(json, line.firstCycleTime.name, line.firstCycleTime.value);
        cJSON_AddNumberToObject(json, line.duration.name, line.duration.value);
        cJSON_AddNumberToObject(json, line.pauseTime.name, line.pauseTime.value);
        cJSON_AddNumberToObject(json, line.hidDelay.name, line.hidDelay.value);
        cJSON_AddNumberToObject(json, line.tempStartDimming.name, line.tempStartDimming.value);
        cJSON_AddNumberToObject(json, line.tempOffDimming.name, line.tempOffDimming.value);
        cJSON_AddNumberToObject(json, line.sunriseSunSet.name, line.sunriseSunSet.value);

        line.timestamp.value = ReplyTimeStamp();
        cJSON_AddNumberToObject(json, line.timestamp.name, line.timestamp.value);

        str = cJSON_PrintUnformatted(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplyFindLocation(char *cmd, cloudcmd_t cloud)
{
    char    name[16];
    char    *str        = RT_NULL;
    cJSON   *json       = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, cloud.msgid.name, cloud.msgid.value);
        cJSON_AddStringToObject(json, "sn", GetSnName(name));

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        str = cJSON_PrintUnformatted(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplySetSysTime(char *cmd, cloudcmd_t cloud)
{
    char    name[16];
    char    *str        = RT_NULL;
    cJSON   *json       = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, cloud.msgid.name, cloud.msgid.value);
        cJSON_AddStringToObject(json, "sn", GetSnName(name));
        cJSON_AddStringToObject(json, cloud.sys_time.name, cloud.sys_time.value);

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        str = cJSON_PrintUnformatted(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplyGetDeadBand(char *cmd, cloudcmd_t cloud)
{
    char    name[16];
    char    *str        = RT_NULL;
    cJSON   *json       = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, cloud.msgid.name, cloud.msgid.value);
        cJSON_AddStringToObject(json, "sn", GetSnName(name));
        cJSON_AddNumberToObject(json, sys_set.tempSet.tempDeadband.name, sys_set.tempSet.tempDeadband.value);
        cJSON_AddNumberToObject(json, sys_set.co2Set.co2Deadband.name, sys_set.co2Set.co2Deadband.value);
        cJSON_AddNumberToObject(json, sys_set.humiSet.humidDeadband.name, sys_set.humiSet.humidDeadband.value);

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        str = cJSON_PrintUnformatted(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplySetDeadBand(char *cmd, cloudcmd_t cloud)
{
    char    name[16];
    char    *str        = RT_NULL;
    cJSON   *json       = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, cloud.msgid.name, cloud.msgid.value);
        cJSON_AddStringToObject(json, "sn", GetSnName(name));
        cJSON_AddNumberToObject(json, sys_set.tempSet.tempDeadband.name, sys_set.tempSet.tempDeadband.value);
        cJSON_AddNumberToObject(json, sys_set.co2Set.co2Deadband.name, sys_set.co2Set.co2Deadband.value);
        cJSON_AddNumberToObject(json, sys_set.humiSet.humidDeadband.name, sys_set.humiSet.humidDeadband.value);

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        str = cJSON_PrintUnformatted(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplyDeleteDevice(char *cmd, cloudcmd_t cloud)
{
    char    name[16];
    char    *str        = RT_NULL;
    cJSON   *json       = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, cloud.msgid.name, cloud.msgid.value);
        cJSON_AddStringToObject(json, "sn", GetSnName(name));

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        str = cJSON_PrintUnformatted(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplySetSchedule(char *cmd, cloudcmd_t cloud)//Justin debug 未验证
{
    u8      index       = 0;
    char    *str        = RT_NULL;
    cJSON   *json       = cJSON_CreateObject();
    cJSON   *list       = RT_NULL;
    cJSON   *item       = RT_NULL;

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, cloud.msgid.name, cloud.msgid.value);
        cJSON_AddNumberToObject(json, "en", sys_set.stageSet.en);
        cJSON_AddStringToObject(json, "starts", sys_set.stageSet.starts);

        list = cJSON_CreateArray();

        if(RT_NULL != list)
        {
            for(index = 0; index < STAGE_LIST_MAX; index++)
            {
                if(sys_set.stageSet._list[index].recipeId != 0)//配方id不能为0，id 最小1
                {
                    item = cJSON_CreateObject();

                    if(RT_NULL != item)
                    {
                        cJSON_AddNumberToObject(item, "recipeId", sys_set.stageSet._list[index].recipeId);
                        cJSON_AddNumberToObject(item, "duration", sys_set.stageSet._list[index].duration_day);

                        cJSON_AddItemToArray(list, item);
                    }
                }
                cJSON_AddItemToObject(json, "list", list);
            }
        }

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        str = cJSON_PrintUnformatted(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplyAddRecipe(char *cmd, cloudcmd_t cloud)//Justin debug 未验证
{
    char    *str        = RT_NULL;
    type_kv_u8 id;
    cJSON   *json       = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, cloud.msgid.name, cloud.msgid.value);
        cJSON_AddStringToObject(json, cloud.recipe_name.name, cloud.recipe_name.value);

        rt_memcpy(id.name, "id", KEYVALUE_NAME_SIZE);
        id.value = AllotRecipeId(cloud.recipe_name.value, GetSysRecipt());

        cJSON_AddNumberToObject(json, id.name, id.value);

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        str = cJSON_PrintUnformatted(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplySetTank(char *cmd, cloudcmd_t cloud)//Justin debug 未验证
{
    char            *str        = RT_NULL;
    cJSON           *json       = cJSON_CreateObject();
    tank_t          *tank       = RT_NULL;

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, cloud.msgid.name, cloud.msgid.value);

        tank = GetTankByNo(GetSysTank(), cloud.tank_no);

        if(RT_NULL != tank)
        {
            cJSON_AddNumberToObject(json, "tankNo", tank->tankNo);
            cJSON_AddNumberToObject(json, "autoFillValveId", tank->autoFillValveId);
            cJSON_AddNumberToObject(json, "autoFillHeight", tank->autoFillHeight);
            cJSON_AddNumberToObject(json, "autoFillFulfilHeight", tank->autoFillFulfilHeight);
            cJSON_AddNumberToObject(json, "highEcProtection", tank->highEcProtection);
            cJSON_AddNumberToObject(json, "lowPhProtection", tank->lowPhProtection);
            cJSON_AddNumberToObject(json, "highPhProtection", tank->highPhProtection);
        }
        else
        {
            LOG_E("ReplySetTank, get tank err");
        }
        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());
        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }

    return str;
}

char *ReplySetWarn(char *cmd, cloudcmd_t cloud, sys_warn_t warn)
{
    char            name[16];
    char            *str        = RT_NULL;
    cJSON           *json       = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, cloud.msgid.name, cloud.msgid.value);
        cJSON_AddStringToObject(json, "sn", GetSnName(name));

        cJSON_AddNumberToObject(json, "dayTempMin",warn.dayTempMin);
        cJSON_AddNumberToObject(json, "dayTempMax",warn.dayTempMax);
        cJSON_AddNumberToObject(json, "dayTempEn",warn.dayTempEn);
        cJSON_AddNumberToObject(json, "dayhumidMin",warn.dayhumidMin);
        cJSON_AddNumberToObject(json, "dayhumidMax",warn.dayhumidMax);
        cJSON_AddNumberToObject(json, "dayhumidEn",warn.dayhumidEn);
        cJSON_AddNumberToObject(json, "dayCo2Min",warn.dayCo2Min);
        cJSON_AddNumberToObject(json, "dayCo2Max",warn.dayCo2Max);
        cJSON_AddNumberToObject(json, "dayCo2En",warn.dayCo2En);
        cJSON_AddNumberToObject(json, "dayCo2Buzz",warn.dayCo2Buzz);
        cJSON_AddNumberToObject(json, "dayVpdMin",warn.dayVpdMin);
        cJSON_AddNumberToObject(json, "dayVpdMax",warn.dayVpdMax);
        cJSON_AddNumberToObject(json, "dayVpdEn",warn.dayVpdEn);
        cJSON_AddNumberToObject(json, "dayParMin",warn.dayParMin);
        cJSON_AddNumberToObject(json, "dayParMax",warn.dayParMax);
        cJSON_AddNumberToObject(json, "dayParEn",warn.dayParEn);
        cJSON_AddNumberToObject(json, "nightTempMin",warn.nightTempMin);
        cJSON_AddNumberToObject(json, "nightTempMax",warn.nightTempMax);
        cJSON_AddNumberToObject(json, "nightTempEn",warn.nightTempEn);
        cJSON_AddNumberToObject(json, "nighthumidMin",warn.nighthumidMin);
        cJSON_AddNumberToObject(json, "nighthumidMax",warn.nighthumidMax);
        cJSON_AddNumberToObject(json, "nighthumidEn",warn.nighthumidEn);
        cJSON_AddNumberToObject(json, "nightCo2Min",warn.nightCo2Min);
        cJSON_AddNumberToObject(json, "nightCo2Max",warn.nightCo2Max);
        cJSON_AddNumberToObject(json, "nightCo2En",warn.nightCo2En);
        cJSON_AddNumberToObject(json, "nightCo2Buzz",warn.nightCo2Buzz);
        cJSON_AddNumberToObject(json, "nightVpdMin",warn.nightVpdMin);
        cJSON_AddNumberToObject(json, "nightVpdMax",warn.nightVpdMax);
        cJSON_AddNumberToObject(json, "nightVpdEn",warn.nightVpdEn);
        cJSON_AddNumberToObject(json, "phEn",warn.phEn);
        cJSON_AddNumberToObject(json, "ecEn",warn.ecEn);
        cJSON_AddNumberToObject(json, "wtEn",warn.wtEn);
        cJSON_AddNumberToObject(json, "wlEn",warn.wlEn);
        cJSON_AddNumberToObject(json, "offlineEn",warn.offlineEn);
        cJSON_AddNumberToObject(json, "lightEn",warn.lightEn);
        cJSON_AddNumberToObject(json, "smokeEn",warn.smokeEn);
        cJSON_AddNumberToObject(json, "waterEn",warn.waterEn);
        cJSON_AddNumberToObject(json, "autoFillTimeout",warn.autoFillTimeout);

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());
        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }

    return str;
}

char *ReplyGetSysPara(char *cmd, cloudcmd_t cloud, sys_para_t para, sensor_t *sensor)
{
    char            *str        = RT_NULL;
    char            time[15]    = "";
    char            temp1[2]    = {0};
    char            temp2[4];
    char            name[16];
    cJSON           *json       = cJSON_CreateObject();
    type_sys_time   sys_time;

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, cloud.msgid.name, cloud.msgid.value);
        cJSON_AddStringToObject(json, "sn", GetSnName(name));

        cJSON_AddStringToObject(json, "ntpzone", para.ntpzone);
        cJSON_AddNumberToObject(json, "tempUnit", para.tempUnit);
        cJSON_AddNumberToObject(json, "ecUnit", para.ecUnit);
        cJSON_AddNumberToObject(json, "timeFormat", para.timeFormat);
        cJSON_AddNumberToObject(json, "dayNightMode", para.dayNightMode);
        cJSON_AddNumberToObject(json, "photocellSensitivity", para.photocellSensitivity);
        for(u8 index = 0; index < sensor->storage_size; index++)
        {
            if(F_S_LIGHT == sensor->__stora[index].func)
            {
                cJSON_AddNumberToObject(json, "lightIntensity", sensor->__stora[index].value);
            }
        }
        cJSON_AddNumberToObject(json, "dayTime", para.dayTime);
        cJSON_AddNumberToObject(json, "nightTime", para.nightTime);
        cJSON_AddNumberToObject(json, "maintain", para.maintain);
        getRealTimeForMat(&sys_time);

        itoa(sys_time.year, temp2, 10);
        strncat(time, temp2, 4);

        itoa(sys_time.month, temp1, 10);
        if(sys_time.month < 10)
        {
            temp1[1] = temp1[0];
            temp1[0] = '0';
        }
        strncat(time, temp1, 2);

        itoa(sys_time.day, temp1, 10);
        if(sys_time.day < 10)
        {
            temp1[1] = temp1[0];
            temp1[0] = '0';
        }
        strncat(time, temp1, 2);

        itoa(sys_time.hour, temp1, 10);
        if(sys_time.hour < 10)
        {
            temp1[1] = temp1[0];
            temp1[0] = '0';
        }
        strncat(time, temp1, 2);

        itoa(sys_time.minute, temp1, 10);
        if(sys_time.minute < 10)
        {
            temp1[1] = temp1[0];
            temp1[0] = '0';
        }
        strncat(time, temp1, 2);

        itoa(sys_time.second, temp1, 10);
        if(sys_time.second < 10)
        {
            temp1[1] = temp1[0];
            temp1[0] = '0';
        }
        strncat(time, temp1, 2);

        cJSON_AddStringToObject(json, "time", time);
        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());
        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }

    return str;
}

char *ReplySetSysPara(char *cmd, cloudcmd_t cloud, sys_para_t para)
{
    char            *str        = RT_NULL;
    cJSON           *json       = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, cloud.msgid.name, cloud.msgid.value);

        cJSON_AddStringToObject(json, "ntpzone", para.ntpzone);
        cJSON_AddNumberToObject(json, "tempUnit", para.tempUnit);
        cJSON_AddNumberToObject(json, "ecUnit", para.ecUnit);
        cJSON_AddNumberToObject(json, "timeFormat", para.timeFormat);
        cJSON_AddNumberToObject(json, "dayNightMode", para.dayNightMode);
        cJSON_AddNumberToObject(json, "photocellSensitivity", para.photocellSensitivity);
        cJSON_AddNumberToObject(json, "dayTime", para.dayTime);
        cJSON_AddNumberToObject(json, "nightTime", para.nightTime);
        cJSON_AddNumberToObject(json, "maintain", para.maintain);

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());
        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }

    return str;
}

char *ReplySetPortName(char *cmd, cloudcmd_t cloud)
{
    u8              addr        = 0;
    u8              port        = 0;
    char            *str        = RT_NULL;
    cJSON           *json       = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, cloud.msgid.name, cloud.msgid.value);
        if(cloud.set_port_id >= 0xFF)
        {
            addr = cloud.set_port_id >> 8;
            port = cloud.set_port_id;
        }
        else
        {
            addr = cloud.set_port_id;
            port = 0;
        }

        cJSON_AddNumberToObject(json, "id", cloud.set_port_id);

        if(0 == port)
        {
            cJSON_AddStringToObject(json, "name", GetDeviceByAddr(GetMonitor(), addr)->name);
        }
        else if(port < 4)
        {
            cJSON_AddStringToObject(json, "name", GetDeviceByAddr(GetMonitor(), addr)->_storage[port]._port.name);
        }

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());
        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }

    return str;
}

char *ReplyTest(char *cmd, cloudcmd_t cloud)
{
    char            *str        = RT_NULL;
    cJSON           *json       = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);

        str = cJSON_PrintUnformatted(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplySetHubName(char *cmd, cloudcmd_t cloud)//Justin debug 需要加入SD卡
{
    char            *str        = RT_NULL;
    cJSON           *json       = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, cloud.msgid.name, cloud.msgid.value);

        cJSON_AddStringToObject(json, "name", GetHub()->name);
        cJSON_AddNumberToObject(json, "nameSeq", GetHub()->nameSeq);

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());
        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }

    return str;
}

char *ReplyGetHubState(char *cmd, cloudcmd_t cloud)
{
    char            *str        = RT_NULL;
    char            model[15];
    char            name[11];
    cJSON           *json       = cJSON_CreateObject();
    cJSON           *list       = RT_NULL;

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, cloud.msgid.name, cloud.msgid.value);
        cJSON_AddStringToObject(json, "sn", GetSnName(name));
        cJSON_AddStringToObject(json, "model", GetModelByType(HUB_TYPE, model, 15));
        cJSON_AddStringToObject(json, "name", GetHub()->name);
        cJSON_AddNumberToObject(json, "nameSeq", GetHub()->nameSeq);

        for(u8 index = 0; index < SENSOR_VALUE_MAX; index++)
        {
            if(F_S_CO2 == GetSensorByType(GetMonitor(), BHS_TYPE)->__stora[index].func)
            {
                cJSON_AddNumberToObject(json, "co2", GetSensorByType(GetMonitor(), BHS_TYPE)->__stora[index].value);
            }
            else if(F_S_TEMP == GetSensorByType(GetMonitor(), BHS_TYPE)->__stora[index].func)
            {
                cJSON_AddNumberToObject(json, "temp", GetSensorByType(GetMonitor(), BHS_TYPE)->__stora[index].value);
            }
            else if(F_S_HUMI == GetSensorByType(GetMonitor(), BHS_TYPE)->__stora[index].func)
            {
                cJSON_AddNumberToObject(json, "humid", GetSensorByType(GetMonitor(), BHS_TYPE)->__stora[index].value);
            }
        }

        cJSON_AddNumberToObject(json, "co2Lock", GetSysSet()->co2Set.dehumidifyLock.value);
        cJSON_AddNumberToObject(json, "tempLock", GetSysSet()->tempSet.coolingDehumidifyLock.value);
        cJSON_AddNumberToObject(json, "humidLock", GetSysSet()->tempSet.coolingDehumidifyLock.value);
        cJSON_AddNumberToObject(json, "ppfd", GetSysSet()->line1Set.byAutoDimming.value);
        cJSON_AddNumberToObject(json, "vpd", getVpd());
        cJSON_AddNumberToObject(json, "dayNight", GetSysSet()->dayOrNight);
        cJSON_AddNumberToObject(json, "maintain", GetSysSet()->sysPara.maintain);

        list = cJSON_CreateObject();

        if(RT_NULL != list)
        {
            cJSON_AddStringToObject(list, "name", "--");//Justin debug
            cJSON_AddStringToObject(list, "week", "--");
            cJSON_AddStringToObject(list, "day", "--");
        }

        cJSON_AddItemToObject(json, "calendar", list);

        cJSON_AddStringToObject(json, "ntpzone", GetSysSet()->sysPara.ntpzone);
        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());
        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }

    return str;
}

char *ReplySetRecipe(char *cmd, cloudcmd_t cloud)//Justin debug 未验证
{
    char            *str        = RT_NULL;
    recipe_t        *recipe     = RT_NULL;
    cJSON           *json       = cJSON_CreateObject();
    cJSON           *line       = RT_NULL;

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, cloud.msgid.name, cloud.msgid.value);

        if(RT_NULL != recipe)
        {
            if(RT_EOK == GetRecipeByid(cloud.recipe_id, GetSysRecipt(), recipe))
            {
                cJSON_AddNumberToObject(json, "id", recipe->id);
                cJSON_AddStringToObject(json, "name", recipe->name);
                cJSON_AddNumberToObject(json, "color", recipe->color);
                cJSON_AddNumberToObject(json, "dayCoolingTarget", recipe->dayCoolingTarget);
                cJSON_AddNumberToObject(json, "dayHeatingTarget", recipe->dayHeatingTarget);
                cJSON_AddNumberToObject(json, "nightCoolingTarget", recipe->nightCoolingTarget);
                cJSON_AddNumberToObject(json, "nightHeatingTarget", recipe->nightHeatingTarget);
                cJSON_AddNumberToObject(json, "dayHumidifyTarget", recipe->dayHumidifyTarget);
                cJSON_AddNumberToObject(json, "dayDehumidifyTarget", recipe->dayDehumidifyTarget);
                cJSON_AddNumberToObject(json, "nightHumidifyTarget", recipe->nightHumidifyTarget);
                cJSON_AddNumberToObject(json, "nightDehumidifyTarget", recipe->nightDehumidifyTarget);
                cJSON_AddNumberToObject(json, "dayCo2Target", recipe->dayCo2Target);
                cJSON_AddNumberToObject(json, "nightCo2Target", recipe->nightCo2Target);

                for(int index = 0; index < 2; index++)
                {
                    line = cJSON_CreateObject();
                    if(RT_NULL != line)
                    {
                        cJSON_AddNumberToObject(line, "brightMode", recipe->line_list[index].brightMode);
                        cJSON_AddNumberToObject(line, "byPower", recipe->line_list[index].byPower);
                        cJSON_AddNumberToObject(line, "byAutoDimming", recipe->line_list[index].byAutoDimming);
                        cJSON_AddNumberToObject(line, "mode", recipe->line_list[index].mode);
                        cJSON_AddNumberToObject(line, "lightOn", recipe->line_list[index].lightOn);
                        cJSON_AddNumberToObject(line, "lightOff", recipe->line_list[index].lightOff);
                        cJSON_AddNumberToObject(line, "firstCycleTime", recipe->line_list[index].firstCycleTime);
                        cJSON_AddNumberToObject(line, "duration", recipe->line_list[index].duration);
                        cJSON_AddNumberToObject(line, "pauseTime", recipe->line_list[index].pauseTime);

                        if(0 == index)
                        {
                            cJSON_AddItemToObject(json, "line1", line);
                        }
                        else if(1 == index)
                        {
                            cJSON_AddItemToObject(json, "line2", line);
                        }
                    }
                }
            }
            else
            {
                LOG_E("ReplySetRecipe apply recipe memory fail");
            }
        }

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());
        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }

    return str;
}

char *ReplyGetSchedule(char *cmd, cloudcmd_t cloud)//Justin debug 未验证
{

    u8          recipe_id   = 0;
    char        name[16];
    char        *str        = RT_NULL;
    recipe_t    rec;
    cJSON       *json       = cJSON_CreateObject();
    cJSON       *list       = RT_NULL;
    cJSON       *list_item  = RT_NULL;

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, cloud.msgid.name, cloud.msgid.value);
        cJSON_AddStringToObject(json, "sn", GetSnName(name));

        cJSON_AddNumberToObject(json, "en", sys_set.stageSet.en);
        cJSON_AddStringToObject(json, "starts", sys_set.stageSet.starts);

        list = cJSON_CreateArray();

        if(RT_NULL != list)
        {
            for(int index = 0; index < STAGE_LIST_MAX; index++)
            {
                list_item = cJSON_CreateObject();

                if(RT_NULL != list_item)
                {
                    recipe_id = sys_set.stageSet._list[index].recipeId;
                    cJSON_AddNumberToObject(list_item, "recipeId", recipe_id);
                    if(RT_EOK == GetRecipeByid(recipe_id, GetSysRecipt(), &rec))
                    {
                        cJSON_AddStringToObject(list_item, "recipeName", rec.name);
                        cJSON_AddNumberToObject(list_item, "color", rec.color);
                    }
                    cJSON_AddNumberToObject(list_item, "duration", sys_set.stageSet._list[index].duration_day);
                    //Justin debug 需要回复结束时间
                    cJSON_AddItemToArray(list, list_item);

                }
            }
        }

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());
        str = cJSON_PrintUnformatted(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplySetPortSet(char *cmd, cloudcmd_t cloud)
{
    u8      addr        = 0;
    u8      port        = 0;
    u8      group       = 0;
    u8      valid_gro   = 0;
    char    *str        = RT_NULL;
    cJSON   *json       = cJSON_CreateObject();
    cJSON   *list       = RT_NULL;
    cJSON   *item       = RT_NULL;
    device_time4_t  *device     = RT_NULL;
    timer12_t       *timer12    = RT_NULL;

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, cloud.msgid.name, cloud.msgid.value);
        cJSON_AddNumberToObject(json, cloud.get_port_id.name, cloud.get_port_id.value);

        if(cloud.get_port_id.value > 0xff)
        {
            addr = cloud.get_port_id.value >> 8;
            port = cloud.get_port_id.value;
        }
        else
        {
            addr = cloud.get_port_id.value;
            port = 0;
        }

        device = GetDeviceByAddr(GetMonitor(), addr);
        timer12 = GetTimerByAddr(GetMonitor(), addr);

        if(RT_NULL != device)
        {
            cJSON_AddNumberToObject(json, "manual", device->_storage[port]._port.manual);
            cJSON_AddNumberToObject(json, "manualOnTime", device->_storage[port]._port.manual_on_time);
            cJSON_AddNumberToObject(json, "hotStartDelay", device->hotStartDelay);
            if(HVAC_6_TYPE == device->type)
            {
                cJSON_AddNumberToObject(json, "manualOnMode", device->_hvac.manualOnMode);
                cJSON_AddNumberToObject(json, "fanNormallyOpen", device->_hvac.fanNormallyOpen);
                cJSON_AddNumberToObject(json, "hvacMode", device->_hvac.hvacMode);
            }

            if(TIMER_TYPE == device->type)
            {
                list = cJSON_CreateArray();

                if(RT_NULL != list)
                {
                    for(group = 0; group < TIMER12_PORT_MAX; group++)
                    {
                        if(!((0 == device->_storage[port]._time4_ctl._timer[group].on_at) &&
                            (0 == device->_storage[port]._time4_ctl._timer[group].duration)))
                        {
                            valid_gro = group;
                        }
                    }

                    for(group = 0; group <= valid_gro; group++)//Justin debug
                    {
                        item = cJSON_CreateObject();
                        if(RT_NULL != item)
                        {
                            cJSON_AddNumberToObject(item, "onAt", device->_storage[port]._time4_ctl._timer[group].on_at);
                            cJSON_AddNumberToObject(item, "duration", device->_storage[port]._time4_ctl._timer[group].duration);
                            cJSON_AddNumberToObject(item, "en", device->_storage[port]._time4_ctl._timer[group].en);

                            cJSON_AddItemToArray(list, item);
                        }
                    }

                    cJSON_AddItemToObject(json, "list", list);
                }
                else
                {
                    LOG_E("ReplySetPortSet apply list err");
                }

                cJSON_AddNumberToObject(json, "startAt", device->_recycle.startAt);
                cJSON_AddNumberToObject(json, "duration", device->_recycle.duration);
                cJSON_AddNumberToObject(json, "pauseTime", device->_recycle.pauseTime);
            }
        }
        else if(RT_NULL != timer12)
        {
            cJSON_AddNumberToObject(json, "manual", timer12->manual);
            cJSON_AddNumberToObject(json, "manualOnTime", timer12->manual_on_time);

            cJSON_AddNumberToObject(json, "mode", timer12->mode);

            list = cJSON_CreateArray();
            if(RT_NULL != list)
            {
                for(int group = 0; group < TIMER_GROUP; group++)
                {
                    if(!((timer12->_time12_ctl[port]._timer[group].on_at == 0) &&
                       (timer12->_time12_ctl[port]._timer[group].duration == 0)))
                    {
                        item = cJSON_CreateObject();
                        if(RT_NULL != item)
                        {
                            cJSON_AddNumberToObject(item, "onAt", timer12->_time12_ctl[port]._timer[group].on_at);
                            cJSON_AddNumberToObject(item, "duration", timer12->_time12_ctl[port]._timer[group].duration);
                            cJSON_AddNumberToObject(item, "en", timer12->_time12_ctl[port]._timer[group].en);

                            cJSON_AddItemToArray(list, item);
                        }
                    }
                }
                cJSON_AddItemToObject(json, "list", list);
            }

            cJSON_AddNumberToObject(json, "startAt", timer12->_recycle.startAt);
            cJSON_AddNumberToObject(json, "duration", timer12->_recycle.duration);
            cJSON_AddNumberToObject(json, "pauseTime", timer12->_recycle.pauseTime);
            cJSON_AddNumberToObject(json, "times", timer12->_recycle.times);
        }

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());
        str = cJSON_PrintUnformatted(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplyGetPortSet(char *cmd, cloudcmd_t cloud)
{
    char    name[16];
    char    model[15];
    char    fun_name[15];
    char    *str        = RT_NULL;
    u8      port        = 0;
    u8      group       = 0;
    u8      valid_gro   = 0;
    u8      addr        = 0;
    cJSON   *timerList  = RT_NULL;
    cJSON   *timer      = RT_NULL;
    cJSON   *json       = cJSON_CreateObject();
    device_time4_t  *module   = RT_NULL;
    timer12_t       *timer12    = RT_NULL;

    rt_memcpy(fun_name, "port", STORAGE_NAMESZ);

    if(RT_NULL != json)
    {
        if(addr > 0xFF)
        {
            addr = cloud.get_port_id.value >> 8;
            port = cloud.get_port_id.value;
            if(port >= DEVICE_PORT_SZ)
            {
                LOG_E("port err");
                port = 0;
            }
        }
        else
        {
            addr = cloud.get_port_id.value;
            port = 0;
        }

        module = GetDeviceByAddr(GetMonitor(), addr);
        timer12 = GetTimerByAddr(GetMonitor(), addr);

        if(RT_NULL != module)
        {
            cJSON_AddStringToObject(json, "cmd", cmd);
            cJSON_AddStringToObject(json, cloud.msgid.name, cloud.msgid.value);
            cJSON_AddStringToObject(json, "sn", GetSnName(name));
            cJSON_AddStringToObject(json, "model", GetModelByType(module->type, model, 15));
            cJSON_AddNumberToObject(json, "id", module->addr);
            cJSON_AddStringToObject(json, "name", module->name);
            cJSON_AddStringToObject(json, "funcName", GetFunNameByType(module->type, fun_name, 15));
            cJSON_AddNumberToObject(json, "mainType", module->main_type);
            cJSON_AddNumberToObject(json, "type", module->type);
            cJSON_AddNumberToObject(json, "manual", module->_storage[port]._port.manual);
            cJSON_AddNumberToObject(json, "manualOnTime", module->_storage[port]._port.manual_on_time);
            cJSON_AddNumberToObject(json, "hotStartDelay", module->hotStartDelay);
            if(HVAC_6_TYPE == module->type)
            {
                cJSON_AddNumberToObject(json, "manualOnMode", module->_hvac.manualOnMode);
                cJSON_AddNumberToObject(json, "fanNormallyOpen", module->_hvac.fanNormallyOpen);
                cJSON_AddNumberToObject(json, "hvacMode", module->_hvac.hvacMode);
            }

            if(TIMER_TYPE == module->type)
            {
                timerList = cJSON_CreateArray();

                if(RT_NULL != timerList)
                {
                    for(group = 0; group < TIMER12_PORT_MAX; group++)
                    {
                        if(!((0 == module->_storage[port]._time4_ctl._timer[group].on_at) &&
                            (0 == module->_storage[port]._time4_ctl._timer[group].duration)))
                        {
                            valid_gro = group;
                        }
                    }

                    for(group = 0; group <= valid_gro; group++)
                    {
                        timer = cJSON_CreateObject();
                        if(RT_NULL != timer)
                        {
                            cJSON_AddNumberToObject(timer, "onAt", module->_storage[port]._time4_ctl._timer[group].on_at);
                            cJSON_AddNumberToObject(timer, "duration", module->_storage[port]._time4_ctl._timer[group].duration);
                            cJSON_AddNumberToObject(timer, "en", module->_storage[port]._time4_ctl._timer[group].en);

                            cJSON_AddItemToArray(timerList, timer);
                        }
                    }

                    cJSON_AddItemToObject(json, "list", timerList);
                }
                else
                {
                    LOG_E("ReplyGetPortSet err5");
                }

                cJSON_AddNumberToObject(json, "startAt", module->_recycle.startAt);
                cJSON_AddNumberToObject(json, "duration", module->_recycle.duration);
                cJSON_AddNumberToObject(json, "pauseTime", module->_recycle.pauseTime);
            }

#if (HUB_SELECT == HUB_IRRIGSTION)
            //Justin debug 灌溉版需要桶
#endif
            cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

            str = cJSON_PrintUnformatted(json);
        }
        else if(RT_NULL != timer12)
        {
            cJSON_AddStringToObject(json, "cmd", cmd);
            cJSON_AddStringToObject(json, cloud.msgid.name, cloud.msgid.value);
            cJSON_AddStringToObject(json, "sn", GetSnName(name));
            cJSON_AddStringToObject(json, "model", GetModelByType(timer12->type, model, 15));
            cJSON_AddNumberToObject(json, "id", timer12->addr);
            cJSON_AddStringToObject(json, "name", timer12->name);
            cJSON_AddStringToObject(json, "funcName", GetFunNameByType(timer12->type, fun_name, 15));
            cJSON_AddNumberToObject(json, "mainType", timer12->main_type);
            cJSON_AddNumberToObject(json, "type", timer12->type);
            cJSON_AddNumberToObject(json, "manual", timer12->manual);
            cJSON_AddNumberToObject(json, "manualOnTime", timer12->manual_on_time);
            cJSON_AddNumberToObject(json, "mode", timer12->mode);

            timerList = cJSON_CreateArray();

            if(RT_NULL != timerList)
            {
                for(group = 0; group < TIMER12_PORT_MAX; group++)
                {
                    timer = cJSON_CreateObject();
                    if(RT_NULL != timer)
                    {
                        cJSON_AddNumberToObject(timer, "onAt", timer12->_time12_ctl[port]._timer[group].on_at);
                        cJSON_AddNumberToObject(timer, "duration", timer12->_time12_ctl[port]._timer[group].duration);
                        cJSON_AddNumberToObject(timer, "en", timer12->_time12_ctl[port]._timer[group].en);

                        cJSON_AddItemToArray(timerList, timer);
                    }
                }

                cJSON_AddItemToObject(json, "list", timerList);
            }
            else
            {
                LOG_E("ReplyGetPortSet err3");
            }

            cJSON_AddNumberToObject(json, "startAt", timer12->_recycle.startAt);
            cJSON_AddNumberToObject(json, "duration", timer12->_recycle.duration);
            cJSON_AddNumberToObject(json, "pauseTime", timer12->_recycle.pauseTime);

            cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

            str = cJSON_PrintUnformatted(json);

            if(str == RT_NULL)
            {
                LOG_E("ReplyGetPortSet err4");
            }
        }
        else
        {
            LOG_E("ReplyGetPortSet err2");
        }

        cJSON_Delete(json);
    }
    else
    {
        LOG_E("ReplyGetPortSet err1");
    }

    return str;
}


char *ReplyGetDeviceList(char *cmd, type_kv_c16 msgid)
{
    u8      index       = 0;
    u8      line_no     = 0;
    u8      storage     = 0;
    char    *str        = RT_NULL;
    char    name[16];
    device_time4_t   module;
    line_t  line;
    cJSON   *list       = RT_NULL;
    cJSON   *item     = RT_NULL;
    cJSON   *portList   = RT_NULL;
    cJSON   *port       = RT_NULL;
    cJSON   *json = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, msgid.name, msgid.value);
        cJSON_AddStringToObject(json, "sn", GetSnName(name));

        list = cJSON_CreateArray();
        if(RT_NULL != list)
        {
            for(index = 0; index < GetMonitor()->device_size; index++)
            {
                module = GetMonitor()->device[index];

                item = cJSON_CreateObject();
                if(RT_NULL != item)
                {
                    cJSON_AddStringToObject(item, "name", module.name);
                    cJSON_AddNumberToObject(item, "id", module.addr);
                    cJSON_AddNumberToObject(item, "mainType", module.main_type);
                    cJSON_AddNumberToObject(item, "type", module.type);
                    if (CON_FAIL == module.conn_state)
                    {
                        cJSON_AddNumberToObject(item, "online", 0);
                    }
                    else
                    {
                        cJSON_AddNumberToObject(item, "online", 1);
                    }
                    if(1 == module.storage_size)
                    {
                        cJSON_AddNumberToObject(item, "manual", module.manual);
                        cJSON_AddNumberToObject(item, "workingStatus", module._storage[0]._port.d_state);
                        cJSON_AddNumberToObject(item, "color", module.color);
                    }
                    else
                    {
                        portList = cJSON_CreateArray();

                        if(RT_NULL != portList)
                        {
                            for(storage = 0; storage < module.storage_size; storage++)
                            {
                                port = cJSON_CreateObject();
                                if(RT_NULL != port)
                                {
//                                    cJSON_AddNumberToObject(port, "type", module._storage[storage]._port.);
                                    cJSON_AddStringToObject(port, "name", module._storage[storage]._port.name);
                                    cJSON_AddNumberToObject(port, "id", module._storage[storage]._port.addr);
                                    cJSON_AddNumberToObject(port, "manual", module._storage[storage]._port.manual);
//                                    cJSON_AddNumberToObject(port, "manual_on_time", module._storage[storage]._port.manual_on_time);
                                    cJSON_AddNumberToObject(port, "workingStatus", module._storage[storage]._port.d_state);
//                                    cJSON_AddNumberToObject(port, "color", module._storage[storage]._port.color);

                                    cJSON_AddItemToArray(portList, port);
                                }

                            }

                            cJSON_AddItemToObject(item, "port", portList);
                        }
                    }

                    cJSON_AddItemToArray(list, item);
                }
            }

            for(line_no = 0; line_no < GetMonitor()->line_size; line_no++)
            {
                line = GetMonitor()->line[line_no];

                item = cJSON_CreateObject();

                if(RT_NULL != item)
                {
                    cJSON_AddStringToObject(item, "name", line.name);
                    cJSON_AddNumberToObject(item, "id", line.addr);
                    cJSON_AddNumberToObject(item, "mainType", 4);//4 指的是line 类型
                    cJSON_AddNumberToObject(item, "type", line.type);
                    cJSON_AddNumberToObject(item, "lineNo", line_no + 1);
                    if(0 == line_no)
                    {
                        cJSON_AddNumberToObject(item, "lightType", GetSysSet()->line1Set.lightsType.value);
                        cJSON_AddNumberToObject(item, "lightPower", GetSysSet()->line1Set.byPower.value);
                    }
                    else if(1 == line_no)
                    {
                        cJSON_AddNumberToObject(item, "lightType", GetSysSet()->line2Set.lightsType.value);
                        cJSON_AddNumberToObject(item, "lightPower", GetSysSet()->line2Set.byPower.value);
                    }
                    if(CON_FAIL == line.conn_state)
                    {
                        cJSON_AddNumberToObject(item, "online", 0);
                    }
                    else
                    {
                        cJSON_AddNumberToObject(item, "online", 1);
                    }
                    cJSON_AddItemToArray(list, item);
                }
            }

            cJSON_AddItemToObject(json, "list", list);
        }
        else
        {
            LOG_E("ReplyGetDeviceList apply memeory err");
        }

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());
        str = cJSON_PrintUnformatted(json);

        cJSON_Delete(json);
    }

    return str;
}
