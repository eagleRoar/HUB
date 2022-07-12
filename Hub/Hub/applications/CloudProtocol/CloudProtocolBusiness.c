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

extern  sys_set_t       sys_set;
extern  type_sys_time   sys_time;

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
        LOG_D("msgid = %s",cmd->msgid.name);
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

void CmdSetPortSet(char *data, cloudcmd_t *cmd)//Justin debug 未测试
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
            GetValueU16(temp, &line->firstCycleTime);
            GetValueU16(temp, &line->duration);
            GetValueU16(temp, &line->pauseTime);
        }
        GetValueU8(temp, &line->hidDelay);
        GetValueU8(temp, &line->tempStartDimming);
        GetValueU8(temp, &line->tempOffDimming);
        GetValueU8(temp, &line->sunriseSunSet);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetHumi err");
    }
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
        sys_set.tempSet.timestamp.value = getTimeStamp();
        cJSON_AddNumberToObject(json, sys_set.tempSet.timestamp.name, sys_set.tempSet.timestamp.value);

        str = cJSON_Print(json);

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
        sys_set.co2Set.timestamp.value = getTimeStamp();
        cJSON_AddNumberToObject(json, sys_set.co2Set.timestamp.name, sys_set.co2Set.timestamp.value);

        str = cJSON_Print(json);

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
        sys_set.humiSet.timestamp.value = getTimeStamp();
        cJSON_AddNumberToObject(json, sys_set.humiSet.timestamp.name, sys_set.humiSet.timestamp.value);

        str = cJSON_Print(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplyGetLine(char *cmd, type_kv_c16 msgid, proLine_t line)
{
    char    name[16];
    char    *str        = RT_NULL;
    cJSON   *json       = cJSON_CreateObject();

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

        line.timestamp.value = getTimeStamp();
        cJSON_AddNumberToObject(json, line.timestamp.name, line.timestamp.value);

        str = cJSON_Print(json);

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

        cJSON_AddNumberToObject(json, "timestamp", getTimeStamp());

        str = cJSON_Print(json);

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

        cJSON_AddNumberToObject(json, "timestamp", getTimeStamp());

        str = cJSON_Print(json);

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

        cJSON_AddNumberToObject(json, "timestamp", getTimeStamp());

        str = cJSON_Print(json);

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

        cJSON_AddNumberToObject(json, "timestamp", getTimeStamp());

        str = cJSON_Print(json);

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

        cJSON_AddNumberToObject(json, "timestamp", getTimeStamp());

        str = cJSON_Print(json);

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

        cJSON_AddNumberToObject(json, "timestamp", getTimeStamp());

        str = cJSON_Print(json);

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

        cJSON_AddNumberToObject(json, "timestamp", getTimeStamp());

        str = cJSON_Print(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplySetPortSet(char *cmd, cloudcmd_t cloud)//Justin debug 功能未验证
{
    u8      addr         = 0;
    u8      port        = 0;
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
            cJSON_AddNumberToObject(json, "manual", device->manual);
            cJSON_AddNumberToObject(json, "manualOnTime", device->manual_on_time);

            if(HVAC_6_TYPE == device->type)
            {
                cJSON_AddNumberToObject(json, "manualOnMode", device->_hvac.manualOnMode);
                cJSON_AddNumberToObject(json, "fanNormallyOpen", device->_hvac.fanNormallyOpen);
                cJSON_AddNumberToObject(json, "hvacMode", device->_hvac.hvacMode);
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

        cJSON_AddNumberToObject(json, "timestamp", getTimeStamp());

        str = cJSON_Print(json);

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
            cJSON_AddNumberToObject(json, "manual", module->manual);
            cJSON_AddNumberToObject(json, "manualOnTime", module->manual_on_time);

            if(HVAC_6_TYPE == module->type)
            {
                cJSON_AddNumberToObject(json, "manualOnMode", module->_hvac.manualOnMode);
                cJSON_AddNumberToObject(json, "fanNormallyOpen", module->_hvac.fanNormallyOpen);
                cJSON_AddNumberToObject(json, "hvacMode", module->_hvac.hvacMode);
            }


#if (HUB_SELECT == HUB_IRRIGSTION)
            //Justin debug 灌溉版需要桶
#endif
            cJSON_AddNumberToObject(json, "timestamp", getTimeStamp());

            str = cJSON_Print(json);
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

            cJSON_AddNumberToObject(json, "timestamp", getTimeStamp());

            str = cJSON_Print(json);

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
    u8      storage     = 0;
    char    *str        = RT_NULL;
    char    name[16];
    device_time4_t   module;
    cJSON   *list       = RT_NULL;
    cJSON   *device     = RT_NULL;
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

                device = cJSON_CreateObject();
                if(RT_NULL != device)
                {
                    cJSON_AddStringToObject(device, "name", module.name);
                    cJSON_AddNumberToObject(device, "id", module.addr);
                    cJSON_AddNumberToObject(device, "mainType", module.main_type);
                    cJSON_AddNumberToObject(device, "type", module.type);
//                    if(module)
//                    {
//                        line_no = getLineNoByuuid(GetMonitor(), module.uuid);
//                        if(line_no < 2)
//                        {
//                            cJSON_AddNumberToObject(device, "lineNo", line_no);
//                        }
//                        else
//                        {
//                            LOG_E("get line no err");
//                        }
//                        cJSON_AddNumberToObject(device, "lightPower", module.storage_in[0]._d_s.d_value);
//                    }

                    if(1 == module.storage_size)
                    {
                        cJSON_AddNumberToObject(device, "manual", module.manual);
                        cJSON_AddNumberToObject(device, "manual_on_time", module.manual_on_time);
                        cJSON_AddNumberToObject(device, "workingStatus", module._storage[0]._port.d_state);
                        cJSON_AddNumberToObject(device, "color", module.color);
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
                                    cJSON_AddNumberToObject(port, "manual_on_time", module._storage[storage]._port.manual_on_time);
                                    cJSON_AddNumberToObject(port, "workingStatus", module._storage[storage]._port.d_state);
//                                    cJSON_AddNumberToObject(port, "color", module._storage[storage]._port.color);

                                    cJSON_AddItemToArray(portList, port);
                                }

                            }

                            cJSON_AddItemToObject(device, "port", portList);
                        }
                    }

                    cJSON_AddItemToArray(list, device);
                }
            }

            cJSON_AddItemToObject(json, "list", list);
        }

        str = cJSON_Print(json);

        cJSON_Delete(json);
    }

    return str;
}
