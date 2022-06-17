/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-14     Administrator       the first version
 */

#include "CloudProtocolBusiness.h"
#include "Uart.h"

extern  proTempSet_t    tempSet;
extern  proCo2Set_t     co2Set;
extern  proHumiSet_t    humiSet;
extern  proLine_t       line1Set;

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
        rt_memset(data->value, ' ', 16);
        LOG_E("parse c16 err, name %s",data->name);
    }

    return ret;
}

void CmdSetTempValue(char *data)
{
    cJSON *temp = RT_NULL;

    temp = cJSON_Parse(data);

    if(RT_NULL != temp)
    {
        GetValueC16(temp, &tempSet.msgid);
        GetValueU16(temp, &tempSet.dayCoolingTarget);
        GetValueU16(temp, &tempSet.dayHeatingTarget);
        GetValueU16(temp, &tempSet.nightCoolingTarget);
        GetValueU16(temp, &tempSet.nightHeatingTarget);
        GetValueU8(temp, &tempSet.coolingDehumidifyLock);

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
        GetValueC16(temp, &tempSet.msgid);

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
        GetValueC16(temp, &co2Set.msgid);
        GetValueU16(temp, &co2Set.dayCo2Target);
        GetValueU16(temp, &co2Set.nightCo2Target);
        GetValueU8(temp, &co2Set.isFuzzyLogic);
        GetValueU8(temp, &co2Set.coolingLock);
        GetValueU8(temp, &co2Set.dehumidifyLock);

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
        GetValueC16(temp, &co2Set.msgid);

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
        GetValueC16(temp, &humiSet.msgid);
        GetValueU16(temp, &humiSet.dayHumiTarget);
        GetValueU16(temp, &humiSet.dayDehumiTarget);
        GetValueU16(temp, &humiSet.nightHumiTarget);
        GetValueU16(temp, &humiSet.nightDehumiTarget);

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
        GetValueC16(temp, &humiSet.msgid);

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
        if(BY_TIMER == line->mode.value)
        {
            GetValueU16(temp, &line->lightOn);
            GetValueU16(temp, &line->lightOff);
        }
        else if(BY_CYCLE == line->mode.value)
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
        cJSON_AddStringToObject(json, tempSet.msgid.name, tempSet.msgid.value);
        cJSON_AddStringToObject(json, tempSet.sn.name, tempSet.sn.value);
        cJSON_AddNumberToObject(json, tempSet.dayCoolingTarget.name, tempSet.dayCoolingTarget.value);
        cJSON_AddNumberToObject(json, tempSet.dayHeatingTarget.name, tempSet.dayHeatingTarget.value);
        cJSON_AddNumberToObject(json, tempSet.nightCoolingTarget.name, tempSet.nightCoolingTarget.value);
        cJSON_AddNumberToObject(json, tempSet.nightHeatingTarget.name, tempSet.nightHeatingTarget.value);
        cJSON_AddNumberToObject(json, tempSet.coolingDehumidifyLock.name, tempSet.coolingDehumidifyLock.value);
        tempSet.timestamp.value = getTimeStamp();
        cJSON_AddNumberToObject(json, tempSet.timestamp.name, tempSet.timestamp.value);

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
        cJSON_AddStringToObject(json, co2Set.msgid.name, co2Set.msgid.value);
        cJSON_AddStringToObject(json, co2Set.sn.name, co2Set.sn.value);
        cJSON_AddNumberToObject(json, co2Set.dayCo2Target.name, co2Set.dayCo2Target.value);
        cJSON_AddNumberToObject(json, co2Set.nightCo2Target.name, co2Set.nightCo2Target.value);
        cJSON_AddNumberToObject(json, co2Set.isFuzzyLogic.name, co2Set.isFuzzyLogic.value);
        cJSON_AddNumberToObject(json, co2Set.coolingLock.name, co2Set.coolingLock.value);
        cJSON_AddNumberToObject(json, co2Set.dehumidifyLock.name, co2Set.dehumidifyLock.value);
        co2Set.timestamp.value = getTimeStamp();
        cJSON_AddNumberToObject(json, co2Set.timestamp.name, co2Set.timestamp.value);

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
        cJSON_AddStringToObject(json, humiSet.msgid.name, humiSet.msgid.value);
        cJSON_AddStringToObject(json, humiSet.sn.name, humiSet.sn.value);
        cJSON_AddNumberToObject(json, humiSet.dayHumiTarget.name, humiSet.dayHumiTarget.value);
        cJSON_AddNumberToObject(json, humiSet.dayDehumiTarget.name, humiSet.dayDehumiTarget.value);
        cJSON_AddNumberToObject(json, humiSet.nightHumiTarget.name, humiSet.nightHumiTarget.value);
        cJSON_AddNumberToObject(json, humiSet.nightDehumiTarget.name, humiSet.nightDehumiTarget.value);
        humiSet.timestamp.value = getTimeStamp();
        cJSON_AddNumberToObject(json, humiSet.timestamp.name, humiSet.timestamp.value);

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

char *ReplyGetDeviceList(char *cmd, type_kv_c16 msgid)
{
    u8      index       = 0;
    u8      line_no     = 0;
    u8      storage     = 0;
    char    *str        = RT_NULL;
    char    name[16];
    type_module_t   module;
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
            for(index = 0; index < GetMonitor()->module_size; index++)
            {
                module = GetMonitor()->module[index];

                if(DEVICE_TYPE == module.s_or_d)
                {
                    device = cJSON_CreateObject();
                    if(RT_NULL != device)
                    {
                        cJSON_AddStringToObject(device, "name", module.name);
                        cJSON_AddNumberToObject(device, "id", module.uuid);
                        cJSON_AddNumberToObject(device, "mainType", module.main_type);
                        cJSON_AddNumberToObject(device, "type", module.type);
                        if(LINE_TYPE == module.type)//Justin debug
                        {
                            line_no = getLineNoByuuid(GetMonitor(), module.uuid);
                            if(line_no < 2)
                            {
                                cJSON_AddNumberToObject(device, "lineNo", line_no);
                            }
                            else
                            {
                                LOG_E("get line no err");
                            }
                            cJSON_AddNumberToObject(device, "lightPower", module.storage_in[0]._d_s.d_value);
                        }

                        if(1 == module.storage_size)
                        {
                            cJSON_AddNumberToObject(device, "manual", 0);//增加手动控制的标志
                            cJSON_AddNumberToObject(device, "workingStatus", module.storage_in[0]._d_s.d_state);
                            cJSON_AddNumberToObject(device, "color", 0);//Justin debug 灌溉版本才有这个颜色
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
                                        cJSON_AddNumberToObject(port, "type", module.storage_in[storage]._d_s.func);//Justin debug 该type需要修改设计
                                        cJSON_AddStringToObject(port, "name", module.storage_in[storage]._d_s.name);
                                        cJSON_AddNumberToObject(port, "id", module.uuid);
                                        cJSON_AddNumberToObject(port, "manual", 0);//增加手动控制的标志
                                        cJSON_AddNumberToObject(port, "workingStatus", module.storage_in[storage]._d_s.d_state);
                                        cJSON_AddNumberToObject(port, "color", 0);//Justin debug 灌溉版本才有这个颜色

                                        cJSON_AddItemToArray(portList, port);//不需要调用cJSON_Delete函数，已经释放
                                    }

                                }

                                cJSON_AddItemToObject(device, "port", portList);
                            }
                        }

                        cJSON_AddItemToArray(list, device);
                    }
                }
            }

            cJSON_AddItemToObject(json, "list", list);
        }

        str = cJSON_Print(json);

        cJSON_Delete(json);
    }

    return str;
}
